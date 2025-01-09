import csv
import numpy as np
import math
import random

def generate_coordinate_path(filename, duration=30, fps=30):
    """
    Generates a CSV file with coordinates following a complex pattern with sudden movements
    duration: time in seconds
    fps: frames per second
    """
    total_frames = duration * fps
    
    x = []
    y = []
    
    t = np.linspace(0, 2*np.pi, total_frames)

    for i in range(total_frames):
        # Base smooth movement
        base_x = 400 + 150 * np.sin(3*t[i])
        base_y = 300 + 150 * np.cos(2*t[i])
        
        if i % (fps * 2) == 0:  # Every 2 seconds
            jump_x = random.randint(-200, 200)
            jump_y = random.randint(-150, 150)
            base_x += jump_x
            base_y += jump_y
        
        if i % (fps // 2) == 0:  # Twice per second
            zigzag = random.choice([-50, 50])
            base_x += zigzag
            base_y += zigzag
        
        if random.random() < 0.05:  # 5% chance each frame
            base_x = random.randint(100, 700)
            base_y = random.randint(100, 500)
        
        x.append(np.clip(base_x, 50, 750))
        y.append(np.clip(base_y, 50, 550))
    
    with open(filename, 'w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['frame', 'x', 'y'])
        for i in range(total_frames):
            writer.writerow([i, round(x[i], 2), round(y[i], 2)])

generate_coordinate_path('movement_path.csv')