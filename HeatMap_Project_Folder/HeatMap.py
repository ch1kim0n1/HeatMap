import pygame
import numpy as np
import pandas as pd
import csv
from scipy.ndimage import gaussian_filter
import time
from pathlib import Path
import colorsys

class HeatmapVisualizer:
    def __init__(self, width=800, height=600):
        pygame.init()
        self.width = width
        self.height = height
        self.screen = pygame.display.set_mode((width, height))
        pygame.display.set_caption("Interactive Heatmap Visualizer")
        self.heatmap = np.zeros((height, width))
        self.max_intensity = 1.0
        self.font = pygame.font.Font(None, 36)
        self.small_font = pygame.font.Font(None, 24)
        self.start_time = time.time()
        self.total_area = width * height
        self.visited_pixels = set()
        self.generate_colormap(256)
        self.current_pos = [width//2, height//2]
        self.mouse_mode = True
        self.csv_data = None
        self.csv_index = 0
        self.control_rect = pygame.Rect(10, height - 80, 200, 60)
        self.mouse_button = pygame.Rect(20, height - 70, 80, 40)
        self.csv_button = pygame.Rect(120, height - 70, 80, 40)
        
    def generate_colormap(self, n):
        self.colors = []
        for i in range(n):
            h = 0.7 - (i / n) * 0.7
            s = 0.8
            v = 0.9
            rgb = tuple(int(x * 255) for x in colorsys.hsv_to_rgb(h, s, v))
            self.colors.append(rgb)
    
    def load_csv(self, filename):
        self.csv_data = pd.read_csv(filename)
        
    def update_heatmap(self, x, y):
        radius = 20
        y_indices, x_indices = np.ogrid[-radius:radius+1, -radius:radius+1]
        mask = x_indices**2 + y_indices**2 <= radius**2
        
        for dy in range(-radius, radius+1):
            for dx in range(-radius, radius+1):
                if mask[dy+radius, dx+radius]:
                    py, px = int(y+dy), int(x+dx)
                    if 0 <= px < self.width and 0 <= py < self.height:
                        self.heatmap[py, px] += 0.1
                        self.visited_pixels.add((px, py))
        
        self.heatmap = gaussian_filter(self.heatmap, sigma=1)
        
    def render_heatmap(self):
        normalized = np.clip(self.heatmap / self.max_intensity, 0, 1)
        surface = pygame.Surface((self.width, self.height))
        
        for y in range(self.height):
            for x in range(self.width):
                intensity = int(normalized[y, x] * (len(self.colors)-1))
                color = self.colors[intensity]
                surface.set_at((x, y), color)
        
        self.screen.blit(surface, (0, 0))
        
    def render_controls(self):
        pygame.draw.rect(self.screen, (50, 50, 50), self.control_rect)
        
        mouse_color = (100, 255, 100) if self.mouse_mode else (100, 100, 100)
        csv_color = (100, 255, 100) if not self.mouse_mode else (100, 100, 100)
        
        pygame.draw.rect(self.screen, mouse_color, self.mouse_button)
        pygame.draw.rect(self.screen, csv_color, self.csv_button)
        
        mouse_text = self.small_font.render("Mouse", True, (0, 0, 0))
        csv_text = self.small_font.render("CSV", True, (0, 0, 0))
        
        self.screen.blit(mouse_text, (self.mouse_button.centerx - mouse_text.get_width()//2,
                                    self.mouse_button.centery - mouse_text.get_height()//2))
        self.screen.blit(csv_text, (self.csv_button.centerx - csv_text.get_width()//2,
                                   self.csv_button.centery - csv_text.get_height()//2))
        
    def render_stats(self):
        elapsed_time = int(time.time() - self.start_time)
        coverage = len(self.visited_pixels) / self.total_area * 100
        
        time_text = f"Time: {elapsed_time}s"
        time_surface = self.font.render(time_text, True, (255, 255, 255))
        self.screen.blit(time_surface, (10, 10))
        
        coverage_text = f"Coverage: {coverage:.1f}%"
        coverage_surface = self.font.render(coverage_text, True, (255, 255, 255))
        self.screen.blit(coverage_surface, (10, 50))
        
    def handle_mouse_click(self, pos):
        if self.mouse_button.collidepoint(pos):
            self.mouse_mode = True
        elif self.csv_button.collidepoint(pos):
            self.mouse_mode = False
            
    def run(self):
        running = True
        clock = pygame.time.Clock()
        
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    self.handle_mouse_click(event.pos)
                    
            if self.mouse_mode:
                x, y = pygame.mouse.get_pos()
                self.current_pos = [x, y]
            else:
                if self.csv_data is not None:
                    if self.csv_index < len(self.csv_data):
                        row = self.csv_data.iloc[self.csv_index]
                        self.current_pos = [row['x'], row['y']]
                        self.csv_index += 1
                    else:
                        self.csv_index = 0
            
            self.update_heatmap(*self.current_pos)
            self.render_heatmap()
            self.render_stats()
            self.render_controls()
            
            pygame.draw.circle(self.screen, (255, 255, 255), 
                             (int(self.current_pos[0]), int(self.current_pos[1])), 5)
            
            pygame.display.flip()
            clock.tick(30)
        
        pygame.quit()

if __name__ == "__main__":
    visualizer = HeatmapVisualizer()
    
    if not Path('movement_path.csv').exists():
        generate_coordinate_path('movement_path.csv')
    visualizer.load_csv('movement_path.csv')
    visualizer.run()
