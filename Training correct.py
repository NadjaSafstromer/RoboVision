import numpy as np
import pandas as pd
from pathlib import Path
from sklearn.metrics import mean_squared_error, r2_score
from keras.models import Sequential
from keras.layers import LSTM, Dense, TimeDistributed, Conv1D, MaxPooling1D, Flatten
import matplotlib.pyplot as plt
import pickle
from sklearn.preprocessing import LabelEncoder
from keras import losses

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


# Load and preprocess data
log_dir = 'logm/'  # Directory containing the log files
df_list = [pd.read_csv(f, sep='\t') for f in Path(log_dir).glob("*.txt")]
df = pd.concat(df_list, ignore_index=True)

# Filter data for blue robots
robot_data = df[df['entity'].str.contains('agent_blue')].sort_values(['entity', 'step'])

# Model parameters
n_steps = 4  # Time steps for prediction
n_features = 2  # x, y coordinates
n_seq = 2  # Sub-sequences for CNN

# Prepare data for training
all_X, all_y = [], []
robots = robot_data['entity'].unique()

for robot in robots:
    # Extract data for the current robot
    coords = robot_data[robot_data['entity'] == robot][['x', 'y']].values

    # Split into sequences
    X, y = split_sequence_multivariate(coords, n_steps)

    # Print sequences and targets for the current robot
    print(f"Robot: {robot}")
    print(f"X sample:\n{X[:2]}")  # First 2 input sequences
    print(f"y sample:\n{y[:2]}")  # First 2 target coordinates

    # Reshape X for TimeDistributed CNN
    X = X.reshape((X.shape[0], n_seq, n_steps // n_seq, n_features))

    # Add to the combined dataset
    all_X.append(X)
    all_y.append(y)

# Combine all robots' sequences
X_train = np.concatenate(all_X, axis=0)
y_train = np.concatenate(all_y, axis=0)

# Display dataset information
print(f"Total training samples: {len(X_train)}")
print(f"Input shape: {X_train.shape}, Output shape: {y_train.shape}")

# Check data contributions from each robot
for i, robot in enumerate(robots):
    robot_samples = all_X[i].shape[0]
    print(f"Robot: {robot}, Samples: {robot_samples}")

# Display sample combined data for inspection
print("\nSample combined input data (X_train):")
for i, robot in enumerate(robots):
    print(f"\nRobot: {robot}")
    print(all_X[i][:2])  # Display first 2 input samples per robot

# Define the shared model
model = Sequential([
    TimeDistributed(Conv1D(filters=64, kernel_size=1, activation='relu'),
                    input_shape=(None, n_steps // n_seq, n_features)),
    TimeDistributed(MaxPooling1D(pool_size=1)),
    TimeDistributed(Flatten()),
    LSTM(50, activation='relu'),
    Dense(2)
])
model.compile(optimizer='adam', loss='mse')

# Train the model
print("\nTraining the model on all robots' data...")
history = model.fit(X_train, y_train, epochs=50, verbose=1)

model.save('robot_xy_predictor.h5')  # Save the model to HDF5 format

robot_ids = robot_data['entity'].unique()  # All unique robot IDs
robot_id_encoder = LabelEncoder()
robot_id_encoder.fit(robot_ids)

# Save the encoder to a file
with open('robot_id_encoder.pkl', 'wb') as f:
    pickle.dump(robot_id_encoder, f)

print("Encoder saved as robot_id_encoder.pkl")


# Evaluate performance for each robot using sliding window approach
for robot in robots:
    print(f"\nEvaluating performance for {robot}...")
    coords = robot_data[robot_data['entity'] == robot][['x', 'y']].values
    
    # Split data into sequences and reshape
    X, y = split_sequence_multivariate(coords, n_steps)
    X = X.reshape((X.shape[0], n_seq, n_steps // n_seq, n_features))
    
    # Initialize list to store predictions
    predictions = []
    
    # predict one sequence at a time
    for i in range(len(X)):
        x_input = X[i:i+1]  # Slice the sequence to predict 1 at a time
        yhat = model.predict(x_input, verbose=0)  # Predict the next point
        predictions.append(yhat[0])  # Append the prediction (shape (2,))
    
    predictions = np.array(predictions)  # Convert predictions to a numpy array
    
    # Compute RMSE
    rmse_x = np.sqrt(mean_squared_error(y[:, 0], predictions[:, 0]))
    rmse_y = np.sqrt(mean_squared_error(y[:, 1], predictions[:, 1]))
    print(f"RMSE X: {rmse_x:.4f}, RMSE Y: {rmse_y:.4f}")

    # Calculate R^2
    r2_x = r2_score(y[:, 0], predictions[:, 0])
    r2_y = r2_score(y[:, 1], predictions[:, 1])
    print(f"R^2 X: {r2_x:.4f}, R^2 Y: {r2_y:.4f}")
    
    # Plot true vs predicted paths
    plt.figure(figsize=(10, 6))
    plt.plot(y[:, 0], y[:, 1], 'b-', label='True Path', alpha=0.5)
    plt.plot(predictions[:, 0], predictions[:, 1], 'r--', label='Predicted Path', alpha=0.8)
    plt.title(f'Trajectory Prediction for {robot}')
    plt.xlabel('X Coordinate')
    plt.ylabel('Y Coordinate')
    plt.legend()
    plt.grid(True)
    plt.show()
