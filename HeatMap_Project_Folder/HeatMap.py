import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap, Normalize
from moviepy.editor import ImageSequenceClip
import os
import logging
from datetime import datetime
import matplotlib.dates as mdates

plt.style.use('seaborn-darkgrid')

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def main():
    # Configuration
    data_file = "./HeatMap/HeatMap_Project_Folder/data.csv"
    output_path = "./HeatMap/HeatMap_Project_Folder/output"
    fps = 30
    frame_width = 10
    frame_height = 6
    dpi = 100
    bins = 50  # Number of bins for 2D histogram

    # Ensure output directory exists
    if not os.path.exists(output_path):
        os.makedirs(output_path)

    # Load and validate data
    data = load_data(data_file)

    # Sort data by time and reset index
    data['Time'] = pd.to_datetime(data['Time'])
    data = data.sort_values('Time').reset_index(drop=True)

    # Extract latitude and longitude arrays
    lat = data['Latitude'].values
    lon = data['Longitude'].values

    # Compute global min/max for consistent frame plotting
    lat_min, lat_max = lat.min() - 0.5, lat.max() + 0.5
    lon_min, lon_max = lon.min() - 0.5, lon.max() + 0.5

    # Precompute global histogram range and colormap normalization based on total data
    full_hist, xedges, yedges = np.histogram2d(lat, lon, bins=bins, 
                                               range=[[lat_min, lat_max], [lon_min, lon_max]])
    max_density = full_hist.max()

    # Define a custom colormap for the heatmap
    # This can be customized further, e.g., adding more segments
    cmap = LinearSegmentedColormap.from_list("custom_heatmap", 
                                             ["#000004", "#2c115f", "#721f81", "#b73779", "#f1605d", "#feb078", "#fcfdbf"])
    norm = Normalize(vmin=0, vmax=max_density)

    # Generate frames
    frames = generate_frames(data, lat, lon, xedges, yedges, bins, cmap, norm, lat_min, lat_max, lon_min, lon_max, 
                             frame_width, frame_height, dpi)

    # Create video
    output_file = os.path.join(output_path, "animation.mp4")
    create_video(frames, output_file, fps)
    logging.info(f"Video saved as '{output_file}'")

def load_data(data_file):
    """Loads data from CSV and checks for required columns."""
    logging.info(f"Loading data from {data_file}")
    try:
        data = pd.read_csv(data_file)
        required_columns = {'Time', 'Latitude', 'Longitude'}
        if not required_columns.issubset(data.columns):
            missing_columns = required_columns - set(data.columns)
            raise ValueError(f"CSV file must contain columns: {required_columns}. Missing: {missing_columns}")
        return data
    except FileNotFoundError:
        logging.error(f"Error: '{data_file}' not found.")
        raise
    except ValueError as ve:
        logging.error(str(ve))
        raise

def generate_frames(data, lat, lon, xedges, yedges, bins, cmap, norm, lat_min, lat_max, lon_min, lon_max, fw, fh, dpi):
    """
    Generate a list of image frames from the data. Each frame is a cumulative heatmap:
    at frame i, it includes all points from 0 to i.
    """
    logging.info("Generating frames for the video animation...")
    frames = []
    
    # We'll accumulate data frame by frame, updating a 2D histogram
    lat_so_far = []
    lon_so_far = []
    
    for i in range(len(data)):
        lat_so_far.append(lat[i])
        lon_so_far.append(lon[i])

        # Compute cumulative histogram up to the current point
        hist, _, _ = np.histogram2d(lat_so_far, lon_so_far, bins=bins, range=[[lat_min, lat_max], [lon_min, lon_max]])

        # Create a figure and plot the heatmap
        fig, ax = plt.subplots(figsize=(fw, fh), dpi=dpi)
        im = ax.imshow(hist.T, origin='lower', cmap=cmap, norm=norm,
                       extent=[lat_min, lat_max, lon_min, lon_max],
                       aspect='auto')
        
        # Add a colorbar for reference (only show after a few frames to avoid clutter at the start)
        if i > 0:
            cbar = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
            cbar.set_label('Density', rotation=270, labelpad=15)

        # Add scatter for the most recent point for clarity (optional)
        ax.scatter(lat[i], lon[i], c='white', edgecolors='black', s=50, alpha=0.8, label='Current Point')

        # Enhance plot labeling
        ax.set_title(f"Heatmap Over Time\nFrame {i+1}/{len(data)} - Time: {data['Time'].iloc[i].strftime('%Y-%m-%d %H:%M:%S')}")
        ax.set_xlabel("Latitude")
        ax.set_ylabel("Longitude")
        ax.set_xlim(lat_min, lat_max)
        ax.set_ylim(lon_min, lon_max)

        # Add a legend just once we have at least one point
        if i > 0:
            ax.legend(loc='upper right')

        # Convert figure to image array
        fig.canvas.draw()
        img = np.frombuffer(fig.canvas.tostring_rgb(), dtype=np.uint8)
        img = img.reshape(fig.canvas.get_width_height()[::-1] + (3,))
        frames.append(img)
        plt.close(fig)

        if (i + 1) % 50 == 0:  # Log progress every 50 frames
            logging.info(f"Processed {i+1}/{len(data)} frames")

    logging.info("All frames generated.")
    return frames

def create_video(frames, output_file, fps):
    """Create a video file from a list of frames using MoviePy."""
    logging.info(f"Creating video at {output_file} with {len(frames)} frames at {fps} fps")
    clip = ImageSequenceClip(frames, fps=fps)
    clip.write_videofile(output_file, codec="libx264", audio=False)
    logging.info("Video creation complete.")

if __name__ == "__main__":
    main()
