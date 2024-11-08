import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import os

def main():
    # Create 'output' folder if it doesn't exist
    if not os.path.exists('output'):
        os.makedirs('output')

    # Define file path
    data_file = "./HeatMap_Project_Folder/data.csv"

    # Read data with error handling
    try:
        # Check if the file exists
        if not os.path.isfile(data_file):
            raise FileNotFoundError(f"File '{data_file}' not found. Please ensure it is in the same directory as the script.")

        # Load CSV file
        data = pd.read_csv(data_file)

        # Ensure the required columns exist
        required_columns = {'Time', 'Latitude', 'Longitude'}
        if not required_columns.issubset(data.columns):
            missing_columns = required_columns - set(data.columns)
            raise ValueError(f"CSV file must contain the following columns: {required_columns}. Missing columns: {missing_columns}")

    except FileNotFoundError as fnf_error:
        print(fnf_error)
        return  # Exit the program if the file is missing
    except ValueError as ve:
        print(ve)
        return  # Exit the program if columns are missing
    except pd.errors.ParserError:
        print(f"Error parsing '{data_file}'. Ensure it is a valid CSV file.")
        return

    # Process data
    try:
        # Convert 'Time' column to datetime and sort by it
        data['Time'] = pd.to_datetime(data['Time'])
        data = data.sort_values('Time').reset_index(drop=True)
    except Exception as e:
        print(f"Error processing data: {e}")
        return

    # Create outputs
    create_animation(data)
    create_heatmap(data)
    create_scatter_plot(data)

def create_animation(data):
    fig, ax = plt.subplots(figsize=(10, 6))
    sc = ax.scatter([], [], s=50)

    def init():
        ax.set_xlim(data['Longitude'].min() - 1, data['Longitude'].max() + 1)
        ax.set_ylim(data['Latitude'].min() - 1, data['Latitude'].max() + 1)
        ax.set_xlabel('Longitude')
        ax.set_ylabel('Latitude')
        ax.set_title('Animated Movement of Points')
        return sc,

    def animate(i):
        current_data = data.iloc[:i+1]
        sc.set_offsets(np.c_[current_data['Longitude'], current_data['Latitude']])
        return sc,

    ani = animation.FuncAnimation(fig, animate, init_func=init,
                                  frames=len(data), interval=200, blit=True)

    ani.save('output/animation.mp4', writer='ffmpeg')
    plt.close()
    print("Animation saved to 'output/animation.mp4'.")

def create_heatmap(data):
    plt.figure(figsize=(10, 6))
    plt.hist2d(data['Longitude'], data['Latitude'], bins=50, cmap='jet')
    plt.colorbar(label='Strength')
    plt.xlabel('Longitude')
    plt.ylabel('Latitude')
    plt.title('Heat Map')
    plt.savefig('output/heatmap.png')
    plt.close()
    print("Heat map saved to 'output/heatmap.png'.")

def create_scatter_plot(data):
    plt.figure(figsize=(10, 6))
    plt.scatter(data['Longitude'], data['Latitude'], c='blue', s=10)
    plt.xlabel('Longitude')
    plt.ylabel('Latitude')
    plt.title('Scatter Plot of Points')
    plt.savefig('output/scatter_plot.png')
    plt.close()
    print("Scatter plot saved to 'output/scatter_plot.png'.")

if __name__ == "__main__":
    main()
