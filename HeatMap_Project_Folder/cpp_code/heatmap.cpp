#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <set>

// ================== HSV to RGB Helper Function ==================
// Converts an HSV color to an sf::Color in RGB space.
// H in [0, 1], S in [0, 1], V in [0, 1].
sf::Color hsvToRgb(float h, float s, float v)
{
    // Wrap hue to [0,1]
    if (h < 0.f) h += 1.f;
    if (h > 1.f) h -= 1.f;
    
    float r, g, b;
    
    float i = std::floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (static_cast<int>(i) % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = g = b = 0.f; // Fallback
    }

    return sf::Color(
        static_cast<sf::Uint8>(r * 255),
        static_cast<sf::Uint8>(g * 255),
        static_cast<sf::Uint8>(b * 255)
    );
}

// ================== Gaussian Blur (Optional) ==================
// A simple blur function that applies a small Gaussian kernel.
void applyGaussianBlur(std::vector<float>& heatmap, int width, int height)
{
    // For speed, use a small kernel (3x3 or 5x5).
    // A large kernel size repeated every frame can be slow in C++.
    const int kernelSize = 3;
    float kernel[3][3] = {
        { 1.f/16, 2.f/16, 1.f/16 },
        { 2.f/16, 4.f/16, 2.f/16 },
        { 1.f/16, 2.f/16, 1.f/16 }
    };

    std::vector<float> temp(heatmap); // Copy original heatmap
    
    for(int y = 0; y < height; ++y) {
        for(int x = 0; x < width; ++x) {
            float sum = 0.f;
            for(int ky = 0; ky < kernelSize; ++ky) {
                for(int kx = 0; kx < kernelSize; ++kx) {
                    int ix = x + (kx - 1);
                    int iy = y + (ky - 1);
                    if(ix >= 0 && ix < width && iy >= 0 && iy < height) {
                        sum += temp[iy * width + ix] * kernel[ky][kx];
                    }
                }
            }
            heatmap[y * width + x] = sum;
        }
    }
}

// ================== Main Class (Similar to your Python Class) ==================
class HeatmapVisualizer {
public:
    HeatmapVisualizer(int width, int height)
        : m_width(width), m_height(height),
          m_window(sf::VideoMode(width, height), "Interactive Heatmap Visualizer"),
          m_heatmap(width * height, 0.f),
          m_maxIntensity(1.0f),
          m_mouseMode(true),
          m_currentPos(width / 2.f, height / 2.f),
          m_visitedPixels(),
          m_fontLoaded(false),
          m_csvIndex(0)
    {
        // Limit the FPS (avoid busy loop)
        m_window.setFramerateLimit(30);

        // Attempt to load a font (for stats and button text)
        if (m_font.loadFromFile("arial.ttf")) {
            m_fontLoaded = true;
        }
        
        // Prepare color map (just like your Python code’s “generate_colormap(256)”)
        generateColorMap(256);

        // Record the start time
        m_startTime = std::chrono::steady_clock::now();
        
        // Setup “buttons” area (basic rectangular regions)
        // NOTE: We do not implement clickable GUIs in detail here,
        //       but show the concept for switching modes.
        m_mouseButton = sf::FloatRect(20.f, m_height - 70.f, 80.f, 40.f);
        m_csvButton   = sf::FloatRect(120.f, m_height - 70.f, 80.f, 40.f);
    }

    // Read CSV data into m_csvData (vector of (x,y) pairs)
    // This is simplistic; adapt for your CSV format.
    void loadCsv(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open CSV file: " << filename << "\n";
            return;
        }

        m_csvData.clear();
        std::string line;
        // Suppose your CSV has headers "x,y" in the first line, and numeric data afterward
        // You can skip the header by reading one line first.
        bool firstLine = true;

        while (std::getline(file, line)) {
            if (firstLine) {
                firstLine = false;
                continue; // skip header
            }
            if (line.empty()) continue;
            std::stringstream ss(line);
            float x, y;
            char comma;
            if (ss >> x >> comma >> y) {
                m_csvData.push_back({x, y});
            }
        }
        file.close();
        m_csvIndex = 0;
    }

    // Main run loop
    void run()
    {
        while (m_window.isOpen())
        {
            // Process events
            sf::Event event;
            while (m_window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed) {
                    m_window.close();
                }
                else if (event.type == sf::Event::MouseButtonPressed) {
                    handleMouseClick(sf::Mouse::getPosition(m_window));
                }
            }

            // Update logic
            updatePosition();
            updateHeatmap(static_cast<int>(m_currentPos.x), static_cast<int>(m_currentPos.y));

            // (Optional) Blur the heatmap
            applyGaussianBlur(m_heatmap, m_width, m_height);

            // Clear and draw
            m_window.clear(sf::Color::Black);
            renderHeatmap();
            renderStats();
            renderControls();

            // Draw the current position indicator (like a white circle in Python)
            sf::CircleShape indicator(5.f);
            indicator.setFillColor(sf::Color::White);
            indicator.setPosition(m_currentPos.x - 5.f, m_currentPos.y - 5.f);
            m_window.draw(indicator);

            m_window.display();
        }
    }

private:
    // Generate a smooth color map (purple to red in your code)
    void generateColorMap(int n)
    {
        m_colors.clear();
        m_colors.reserve(n);
        for (int i = 0; i < n; ++i) {
            float h = 0.7f - (static_cast<float>(i) / n) * 0.7f;  // Purple -> Red
            float s = 0.8f;
            float v = 0.9f;
            sf::Color c = hsvToRgb(h, s, v);
            m_colors.push_back(c);
        }
    }

    // Update the current position based on mode
    void updatePosition()
    {
        if (m_mouseMode) {
            // Mouse-based: get current mouse position relative to window
            sf::Vector2i mp = sf::Mouse::getPosition(m_window);
            m_currentPos.x = static_cast<float>(mp.x);
            m_currentPos.y = static_cast<float>(mp.y);
        } else {
            // CSV-based
            if (!m_csvData.empty()) {
                if (m_csvIndex < m_csvData.size()) {
                    m_currentPos.x = m_csvData[m_csvIndex].first;
                    m_currentPos.y = m_csvData[m_csvIndex].second;
                    m_csvIndex++;
                } else {
                    // Loop or stop if we reach the end?
                    m_csvIndex = 0;
                }
            }
        }
    }

    // Update heatmap around (x, y)
    void updateHeatmap(int x, int y)
    {
        // “Radius = 20” circle
        const int radius = 20;
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int xx = x + dx;
                int yy = y + dy;
                if (xx >= 0 && xx < m_width && yy >= 0 && yy < m_height) {
                    // Check circle boundary
                    if (dx * dx + dy * dy <= radius * radius) {
                        // Increase intensity
                        int idx = yy * m_width + xx;
                        m_heatmap[idx] += 0.1f;
                        // Track visited pixel
                        m_visitedPixels.insert(idx);
                    }
                }
            }
        }
        // Update max intensity if needed
        // (Or you can keep it fixed.)
        // For performance, you might want to recalc less often.
        // But this ensures colors keep scaling as intensities grow.
        float currentMax = 0.f;
        for (auto val : m_heatmap) {
            if (val > currentMax) currentMax = val;
        }
        m_maxIntensity = currentMax < 1.0f ? 1.0f : currentMax;
    }

    // Render the heatmap
    void renderHeatmap()
    {
        // We draw pixel-by-pixel with an sf::Image or directly on a texture.
        // Here, we use an sf::Image for simplicity.
        sf::Image image;
        image.create(m_width, m_height, sf::Color::Black);

        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                int idx = y * m_width + x;
                float val = m_heatmap[idx] / m_maxIntensity; // normalized [0..1]
                if (val > 1.f) val = 1.f;
                int colorIndex = static_cast<int>(val * (m_colors.size() - 1));
                if (colorIndex < 0) colorIndex = 0;
                if (colorIndex >= (int)m_colors.size()) colorIndex = (int)m_colors.size() - 1;
                image.setPixel(x, y, m_colors[colorIndex]);
            }
        }

        // Create a texture from the image and draw it
        sf::Texture texture;
        texture.loadFromImage(image);
        sf::Sprite sprite;
        sprite.setTexture(texture);
        m_window.draw(sprite);
    }

    // Render basic coverage/time stats
    void renderStats()
    {
        if (!m_fontLoaded) return; // skip if no font loaded

        // Elapsed time
        auto now = std::chrono::steady_clock::now();
        auto elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime).count();
        std::string timeStr = "Time: " + std::to_string(elapsedSec) + "s";

        // Coverage (how many unique pixels visited)
        float coverage = (static_cast<float>(m_visitedPixels.size()) / (m_width * m_height)) * 100.0f;
        std::string coverageStr = "Coverage: " + std::to_string(coverage) + "%";

        sf::Text timeText, covText;
        timeText.setFont(m_font);
        timeText.setString(timeStr);
        timeText.setCharacterSize(24);
        timeText.setFillColor(sf::Color::White);
        timeText.setPosition(10.f, 10.f);

        covText.setFont(m_font);
        covText.setString(coverageStr);
        covText.setCharacterSize(24);
        covText.setFillColor(sf::Color::White);
        covText.setPosition(10.f, 40.f);

        m_window.draw(timeText);
        m_window.draw(covText);
    }

    // Render “buttons” and highlight which mode is active
    void renderControls()
    {
        if (!m_fontLoaded) return;

        // Draw a semi-transparent rectangle at bottom
        sf::RectangleShape panel(sf::Vector2f(200.f, 60.f));
        panel.setFillColor(sf::Color(50, 50, 50, 255));
        panel.setPosition(10.f, m_height - 80.f);
        m_window.draw(panel);

        // Mouse button
        sf::RectangleShape mouseBtn(sf::Vector2f(m_mouseButton.width, m_mouseButton.height));
        mouseBtn.setPosition(m_mouseButton.left, m_mouseButton.top);
        if (m_mouseMode)
            mouseBtn.setFillColor(sf::Color(100, 255, 100));
        else
            mouseBtn.setFillColor(sf::Color(100, 100, 100));
        m_window.draw(mouseBtn);

        // CSV button
        sf::RectangleShape csvBtn(sf::Vector2f(m_csvButton.width, m_csvButton.height));
        csvBtn.setPosition(m_csvButton.left, m_csvButton.top);
        if (!m_mouseMode)
            csvBtn.setFillColor(sf::Color(100, 255, 100));
        else
            csvBtn.setFillColor(sf::Color(100, 100, 100));
        m_window.draw(csvBtn);

        // Text labels
        sf::Text mouseText("Mouse", m_font, 18);
        mouseText.setFillColor(sf::Color::Black);
        mouseText.setPosition(
            m_mouseButton.left + (m_mouseButton.width - mouseText.getLocalBounds().width) / 2.f,
            m_mouseButton.top  + (m_mouseButton.height - mouseText.getLocalBounds().height) / 2.f
        );
        m_window.draw(mouseText);

        sf::Text csvText("CSV", m_font, 18);
        csvText.setFillColor(sf::Color::Black);
        csvText.setPosition(
            m_csvButton.left + (m_csvButton.width - csvText.getLocalBounds().width) / 2.f,
            m_csvButton.top  + (m_csvButton.height - csvText.getLocalBounds().height) / 2.f
        );
        m_window.draw(csvText);
    }

    // Handle mouse clicks for toggling “mouse mode” vs “CSV mode”
    void handleMouseClick(const sf::Vector2i& mousePos)
    {
        sf::Vector2f mp = m_window.mapPixelToCoords(mousePos);
        if (m_mouseButton.contains(mp)) {
            m_mouseMode = true;
        } else if (m_csvButton.contains(mp)) {
            m_mouseMode = false;
        }
    }

private:
    int m_width;
    int m_height;
    sf::RenderWindow m_window;
    std::vector<float> m_heatmap;        // Our 2D heatmap is flattened into 1D
    float m_maxIntensity;
    bool m_mouseMode;                    // Toggle between mouse control / CSV playback
    sf::Vector2f m_currentPos;           // Current position on the screen
    std::set<int> m_visitedPixels;       // Track visited pixels (for coverage)
    std::vector<sf::Color> m_colors;     // Color map
    std::vector<std::pair<float, float>> m_csvData;
    size_t m_csvIndex;

    // Font / text rendering
    sf::Font m_font;
    bool m_fontLoaded;

    // Time tracking
    std::chrono::steady_clock::time_point m_startTime;

    // Simple “buttons”
    sf::FloatRect m_mouseButton;
    sf::FloatRect m_csvButton;
};

// ================== MAIN FUNCTION ==================
int main()
{
    // Example usage
    HeatmapVisualizer visualizer(800, 600);

    // Load CSV data if you have a file named "movement_path.csv"
    // Ensure your CSV has columns "x,y" for this example
    visualizer.loadCsv("movement_path.csv");

    // Start the visualization
    visualizer.run();
    
    return 0;
}
