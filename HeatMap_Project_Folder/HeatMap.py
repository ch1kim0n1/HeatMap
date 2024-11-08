import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from moviepy.editor import ImageSequenceClip
import os

def main():
    # Define custom output path
    output_path = "./HeatMap/HeatMap_Project_Folder/output"  # Change this to your desired folder path
    if not os.path.exists(output_path):
        os.makedirs(output_path)

    # Read data
    data_file = "./HeatMap/HeatMap_Project_Folder/data.csv"
    try:
        data = pd.read_csv(data_file)
        required_columns = {'Time', 'Latitude', 'Longitude'}
        if not required_columns.issubset(data.columns):
            missing_columns = required_columns - set(data.columns)
            raise ValueError(f"CSV file must contain columns: {required_columns}. Missing: {missing_columns}")
    except FileNotFoundError:
        print(f"Error: '{data_file}' not found.")
        return
    except ValueError as ve:
        print(ve)
        return

    # Process data
    data['Time'] = pd.to_datetime(data['Time'])
    data = data.sort_values('Time').reset_index(drop=True)

    # Create video with MoviePy
    create_moviepy_video(data)

def create_moviepy_video(data):
    frames = []  # List to hold each frame as an image array

    for i in range(len(data)):
        # Create figure and plot the current frame
        fig, ax = plt.subplots(figsize=(6, 4))
        ax.set_xlim(data['Longitude'].min() - 1, data['Longitude'].max() + 1)
        ax.set_ylim(data['Latitude'].min() - 1, data['Latitude'].max() + 1)
        ax.scatter(data['Longitude'].iloc[:i+1], data['Latitude'].iloc[:i+1], c='blue')
        ax.set_title("Animated Path")
        ax.set_xlabel("Longitude")
        ax.set_ylabel("Latitude")
        
        # Convert plot to an image array
        fig.canvas.draw()
        img = np.frombuffer(fig.canvas.tostring_rgb(), dtype=np.uint8)
        img = img.reshape(fig.canvas.get_width_height()[::-1] + (3,))
        frames.append(img)  # Add the image to the frames list
        plt.close(fig)  # Close the figure to save memory

    # Create video using MoviePy
    clip = ImageSequenceClip(frames, fps=30)  # Adjust FPS as needed
    clip.write_videofile("./HeatMap/HeatMap_Project_Folder/output/animation.mp4", codec="libx264")
    print("Video saved as 'output/animation.mp4'")

if __name__ == "__main__":
    main()
