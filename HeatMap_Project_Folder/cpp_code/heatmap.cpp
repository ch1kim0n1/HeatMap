#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>

// Window dimensions
const int WIDTH = 800;
const int HEIGHT = 600;

// Structure to store a point from the CSV data
struct Point {
    int frame;
    int x;
    int y;
};

// Simple CSV loader (skips header)
std::vector<Point> loadCSV(const std::string& filename) {
    std::vector<Point> points;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open CSV file: " << filename << std::endl;
        return points;
    }
    std::string line;
    std::getline(file, line); // skip header
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        Point p;
        std::getline(ss, token, ',');
        p.frame = std::stoi(token);
        std::getline(ss, token, ',');
        p.x = static_cast<int>(std::round(std::stof(token)));
        std::getline(ss, token, ',');
        p.y = static_cast<int>(std::round(std::stof(token)));
        points.push_back(p);
    }
    return points;
}

// Generates a coordinate path CSV file similar to the Python random_data.py code.
void generateCoordinatePath(const std::string& filename, int duration = 30, int fps = 30) {
    int total_frames = duration * fps;
    std::vector<float> x(total_frames), y(total_frames), t(total_frames);
    float two_pi = 2 * M_PI;
    for (int i = 0; i < total_frames; i++) {
        t[i] = two_pi * i / total_frames; // linearly spaced between 0 and 2pi
    }
    
    std::default_random_engine generator(std::time(0));
    std::uniform_int_distribution<int> jumpXDist(-200, 200);
    std::uniform_int_distribution<int> jumpYDist(-150, 150);
    std::uniform_int_distribution<int> zigzagDist(0, 1); // 0 -> -50, 1 -> 50
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::uniform_int_distribution<int> randomXDist(100, 700);
    std::uniform_int_distribution<int> randomYDist(100, 500);
    
    for (int i = 0; i < total_frames; i++) {
        float base_x = 400 + 150 * std::sin(3 * t[i]);
        float base_y = 300 + 150 * std::cos(2 * t[i]);
        
        if (i % (fps * 2) == 0) {
            base_x += jumpXDist(generator);
            base_y += jumpYDist(generator);
        }
        
        if (i % (fps / 2) == 0) {
            int zigzag = (zigzagDist(generator) == 0 ? -50 : 50);
            base_x += zigzag;
            base_y += zigzag;
        }
        
        if (probDist(generator) < 0.05f) {
            base_x = randomXDist(generator);
            base_y = randomYDist(generator);
        }
        
        // Clip the values
        if (base_x < 50) base_x = 50;
        if (base_x > 750) base_x = 750;
        if (base_y < 50) base_y = 50;
        if (base_y > 550) base_y = 550;
        
        x[i] = base_x;
        y[i] = base_y;
    }
    
    std::ofstream ofs(filename);
    ofs << "frame,x,y\n";
    for (int i = 0; i < total_frames; i++) {
        ofs << i << "," << std::round(x[i] * 100) / 100 << "," << std::round(y[i] * 100) / 100 << "\n";
    }
    ofs.close();
}

// Converts HSV values to an SDL_Color in RGB format.
SDL_Color HSVtoRGB(float h, float s, float v) {
    float r, g, b;
    int i = int(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    switch(i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    SDL_Color color;
    color.r = static_cast<Uint8>(r * 255);
    color.g = static_cast<Uint8>(g * 255);
    color.b = static_cast<Uint8>(b * 255);
    color.a = 255;
    return color;
}

// The main class that mimics the HeatmapVisualizer from Python.
class HeatmapVisualizer {
public:
    HeatmapVisualizer(int width = 800, int height = 600)
    : width(width), height(height), max_intensity(1.0f), mouse_mode(true), csv_index(0)
    {
        SDL_Init(SDL_INIT_VIDEO);
        TTF_Init();
        window = SDL_CreateWindow("Interactive Heatmap Visualizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        heatmap.assign(width * height, 0.0f);
        visited.assign(width * height, false);
        total_area = width * height;
        start_time = SDL_GetTicks();
        generateColormap(256);
        current_pos = {width / 2, height / 2};
        // Define control button rectangles
        control_rect = {10, height - 80, 200, 60};
        mouse_button = {20, height - 70, 80, 40};
        csv_button = {120, height - 70, 80, 40};
        
        // Load a font (ensure "OpenSans-Regular.ttf" exists in your working directory)
        font = TTF_OpenFont("OpenSans-Regular.ttf", 36);
        small_font = TTF_OpenFont("OpenSans-Regular.ttf", 24);
        if (!font || !small_font) {
            std::cerr << "Failed to load font. Make sure OpenSans-Regular.ttf is available." << std::endl;
        }
    }
    
    ~HeatmapVisualizer() {
        if (font) TTF_CloseFont(font);
        if (small_font) TTF_CloseFont(small_font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }
    
    void loadCSV(const std::string& filename) {
        csv_data = loadCSV(filename);
    }
    
    void run() {
        bool running = true;
        SDL_Event event;
        Uint32 frameDelay = 1000 / 30; // aiming for ~30 fps
        
        while (running) {
            Uint32 frameStart = SDL_GetTicks();
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                    int mx = event.button.x;
                    int my = event.button.y;
                    handleMouseClick(mx, my);
                }
            }
            
            // Update the current position based on mode
            if (mouse_mode) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                current_pos = {x, y};
            } else {
                if (!csv_data.empty()) {
                    current_pos = {csv_data[csv_index].x, csv_data[csv_index].y};
                    csv_index = (csv_index + 1) % csv_data.size();
                }
            }
            
            updateHeatmap(current_pos.first, current_pos.second);
            applyGaussianBlur();
            
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            renderHeatmap();
            renderStats();
            renderControls();
            
            // Draw the current position as a white circle
            drawCircle(current_pos.first, current_pos.second, 5, {255, 255, 255, 255});
            
            SDL_RenderPresent(renderer);
            Uint32 frameTime = SDL_GetTicks() - frameStart;
            if (frameDelay > frameTime) {
                SDL_Delay(frameDelay - frameTime);
            }
        }
    }
    
private:
    int width, height;
    float max_intensity;
    std::vector<float> heatmap;
    std::vector<float> heatmap_buffer; // temporary buffer for blur
    std::vector<bool> visited;
    unsigned int total_area;
    Uint32 start_time;
    bool mouse_mode;
    std::pair<int, int> current_pos;
    std::vector<SDL_Color> colormap;
    SDL_Rect control_rect, mouse_button, csv_button;
    std::vector<Point> csv_data;
    size_t csv_index;
    
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    TTF_Font* small_font;
    
    // Create a colormap (similar to Python’s generate_colormap)
    void generateColormap(int n) {
        colormap.clear();
        for (int i = 0; i < n; i++) {
            float h = 0.7f - (i / (float)n) * 0.7f;
            float s = 0.8f;
            float v = 0.9f;
            colormap.push_back(HSVtoRGB(h, s, v));
        }
    }
    
    // Update heatmap by adding intensity to pixels within a radius around (x, y)
    void updateHeatmap(int x, int y) {
        int radius = 20;
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (dx * dx + dy * dy <= radius * radius) {
                    int px = x + dx;
                    int py = y + dy;
                    if (px >= 0 && px < width && py >= 0 && py < height) {
                        heatmap[py * width + px] += 0.1f;
                        visited[py * width + px] = true;
                    }
                }
            }
        }
    }
    
    // Apply a simple 5x5 Gaussian blur to the heatmap
    void applyGaussianBlur() {
        const int kSize = 5;
        const int kHalf = kSize / 2;
        double kernel[5][5] = {
            {1, 4, 7, 4, 1},
            {4, 16, 26, 16, 4},
            {7, 26, 41, 26, 7},
            {4, 16, 26, 16, 4},
            {1, 4, 7, 4, 1}
        };
        double kernelSum = 273.0; // sum of kernel elements
        
        heatmap_buffer = heatmap; // copy current heatmap
        
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                double sum = 0.0;
                for (int kj = -kHalf; kj <= kHalf; kj++) {
                    for (int ki = -kHalf; ki <= kHalf; ki++) {
                        int ix = i + ki;
                        int jy = j + kj;
                        if (ix >= 0 && ix < width && jy >= 0 && jy < height) {
                            sum += kernel[kj + kHalf][ki + kHalf] * heatmap_buffer[jy * width + ix];
                        }
                    }
                }
                heatmap[j * width + i] = sum / kernelSum;
            }
        }
    }
    
    // Render the heatmap by creating a texture from pixel data.
    void renderHeatmap() {
        SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        std::vector<Uint32> pixels(width * height, 0);
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                float intensity = heatmap[j * width + i] / max_intensity;
                if (intensity > 1.0f) intensity = 1.0f;
                if (intensity < 0.0f) intensity = 0.0f;
                int colorIndex = static_cast<int>(intensity * (colormap.size() - 1));
                SDL_Color color = colormap[colorIndex];
                Uint32 pixel = (color.a << 24) | (color.r << 16) | (color.g << 8) | (color.b);
                pixels[j * width + i] = pixel;
            }
        }
        SDL_UpdateTexture(texture, NULL, pixels.data(), width * sizeof(Uint32));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_DestroyTexture(texture);
    }
    
    // Render control buttons and their labels.
    void renderControls() {
        // Draw background for controls
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &control_rect);
        
        // Mouse mode button
        if (mouse_mode) {
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        }
        SDL_RenderFillRect(renderer, &mouse_button);
        
        // CSV mode button
        if (!mouse_mode) {
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        }
        SDL_RenderFillRect(renderer, &csv_button);
        
        // Render button text labels
        renderText("Mouse", small_font, {0, 0, 0, 255}, mouse_button);
        renderText("CSV", small_font, {0, 0, 0, 255}, csv_button);
    }
    
    // Render statistics such as elapsed time and coverage.
    void renderStats() {
        Uint32 elapsed_time = (SDL_GetTicks() - start_time) / 1000;
        std::string time_text = "Time: " + std::to_string(elapsed_time) + "s";
        renderText(time_text, font, {255, 255, 255, 255}, {10, 10, 200, 40});
        
        int visitedCount = 0;
        for (bool v : visited) {
            if (v) visitedCount++;
        }
        float coverage = (visitedCount / (float)total_area) * 100;
        std::string coverage_text = "Coverage: " + std::to_string(coverage).substr(0,4) + "%";
        renderText(coverage_text, font, {255, 255, 255, 255}, {10, 50, 300, 40});
    }
    
    // Helper to render text using SDL_ttf.
    void renderText(const std::string& text, TTF_Font* f, SDL_Color color, SDL_Rect rect) {
        SDL_Surface* surface = TTF_RenderText_Blended(f, text.c_str(), color);
        if (!surface) return;
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
    }
    
    // Handle mouse clicks to toggle between mouse and CSV modes.
    void handleMouseClick(int x, int y) {
        if (pointInRect(x, y, mouse_button)) {
            mouse_mode = true;
        } else if (pointInRect(x, y, csv_button)) {
            mouse_mode = false;
        }
    }
    
    bool pointInRect(int x, int y, const SDL_Rect& rect) {
        return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
    }
    
    // Draw a filled circle at (centerX, centerY) with the given radius.
    void drawCircle(int centerX, int centerY, int radius, SDL_Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        for (int w = 0; w < radius * 2; w++) {
            for (int h = 0; h < radius * 2; h++) {
                int dx = radius - w;
                int dy = radius - h;
                if ((dx * dx + dy * dy) <= (radius * radius)) {
                    SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    const std::string csv_filename = "movement_path.csv";
    // Generate the CSV file if it doesn't exist.
    std::ifstream infile(csv_filename);
    if (!infile.good()) {
        generateCoordinatePath(csv_filename);
    }
    
    HeatmapVisualizer visualizer(WIDTH, HEIGHT);
    visualizer.loadCSV(csv_filename);
    visualizer.run();
    
    return 0;
}
