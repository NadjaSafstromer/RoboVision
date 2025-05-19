import numpy as np
import pandas as pd
from pathlib import Path
from sklearn.metrics import mean_squared_error, r2_score
from keras.models import load_model
import matplotlib.pyplot as plt
from sklearn.preprocessing import LabelEncoder
import pickle
from matplotlib.animation import FuncAnimation

# Function to split sequence into samples with input and output
def split_sequence_multivariate(sequences, n_steps):
    """Split sequence into samples with input and output."""
    X, y = [], []
    for i in range(len(sequences)):
        end_ix = i + n_steps
        if end_ix > len(sequences) - 1:
            break
        seq_x, seq_y = sequences[i:end_ix], sequences[end_ix]
        X.append(seq_x)
        y.append(seq_y)
    return np.array(X), np.array(y)

# Parameters (must match the training parameters)
n_steps = 4  # Time steps for prediction
n_features = 2  # x, y coordinates
n_seq = 2  # Sub-sequences for CNN

# Load the trained model
model = load_model('robot_xy_predictor.h5', compile=False)
print("Model loaded successfully.")

# Load and preprocess validation data
log_dir = 'logk/'  # Directory containing the log files
validation_df = [pd.read_csv(f, sep='\t') for f in Path(log_dir).glob("*.txt")]
df = pd.concat(validation_df, ignore_index=True)

# Choose the robots to validate
selected_robots = ["agent_blue_0", "agent_blue_1", "agent_blue_2", "agent_blue_3"]  # Replace with robot names
robot_data_dict = {robot: df[df['entity'] == robot].sort_values('step') for robot in selected_robots}

# Prepare the validation data for each robot
robot_coords = {}
robot_X_vals = {}
robot_y_vals = {}
robot_predictions = {robot: [] for robot in selected_robots}
robot_true_paths = {robot: ([], []) for robot in selected_robots}

for robot in selected_robots:
    coords = robot_data_dict[robot][['x', 'y']].values
    X_val, y_val = split_sequence_multivariate(coords, n_steps)
    X_val = X_val.reshape((X_val.shape[0], n_seq, n_steps // n_seq, n_features))
    robot_coords[robot] = coords
    robot_X_vals[robot] = X_val
    robot_y_vals[robot] = y_val

# Function to update the animation
def update(frame):
    if frame >= max(len(robot_X_vals[robot]) for robot in selected_robots):
        return  # Stop if all predictions are complete

    plt.cla()  # Clear the plot
    for robot in selected_robots:
        if frame < len(robot_X_vals[robot]):
            # Predict the next step
            x_input = robot_X_vals[robot][frame:frame+1]  # Reshape input for one sample at a time
            yhat = model.predict(x_input, verbose=0)[0]  # Make prediction
            robot_predictions[robot].append(yhat)  # Store the prediction

            # Update true paths
            robot_true_paths[robot][0].append(robot_y_vals[robot][frame, 0])  # True X
            robot_true_paths[robot][1].append(robot_y_vals[robot][frame, 1])  # True Y

            # Get predicted path
            pred_x = [p[0] for p in robot_predictions[robot]]
            pred_y = [p[1] for p in robot_predictions[robot]]

            # Plot true and predicted paths for the robot
            plt.plot(robot_true_paths[robot][0], robot_true_paths[robot][1], 'b-', label=f'{robot} True Path', alpha=0.5)
            plt.plot(pred_x, pred_y, '--', label=f'{robot} Predicted Path', alpha=0.8)

            # Highlight the current prediction point
            plt.scatter(pred_x[-1:], pred_y[-1:], label=f'{robot} Current Prediction')

    plt.title(f'Real-Time Prediction for Robots (Step {frame + 1})')
    plt.xlabel('X Coordinate')
    plt.ylabel('Y Coordinate')
    plt.legend()
    plt.grid(True)
    plt.xlim([-2, 2])  # Adjust X limits based on your data range
    plt.ylim([-2, 2])  # Adjust Y limits based on your data range

# Create animation
fig, ax = plt.subplots(figsize=(8, 8))
ani = FuncAnimation(fig, update, frames=max(len(robot_X_vals[robot]) for robot in selected_robots), interval=100)  # Adjust interval (ms)

# Display the animation
plt.show()
