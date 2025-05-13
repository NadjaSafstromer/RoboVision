import numpy as np
import pandas as pd
from pathlib import Path 
from sklearn.preprocessing import LabelEncoder, MinMaxScaler
from sklearn.metrics import mean_squared_error
from sklearn.model_selection import train_test_split
from keras.models import Sequential
from keras.layers import LSTM, Dense, TimeDistributed, Conv1D, MaxPooling1D, Flatten, Dropout
from keras.callbacks import EarlyStopping, ReduceLROnPlateau
import matplotlib.pyplot as plt
from datetime import datetime
import joblib 

# split multivariate sequences for LSTM
def split_sequence_multivariate(sequences, n_steps):
    X, y = [], []
    for i in range(len(sequences)):
        end_ix = i + n_steps
        if end_ix >= len(sequences):
            break
        seq_x, seq_y = sequences[i:end_ix], sequences[end_ix]
        X.append(seq_x)
        y.append(seq_y)
    return np.array(X), np.array(y) 

# Load data
log_dir = 'logs/'  # Directory containing log files
df_list = []

# Read and process all log files
for filename in Path(log_dir).glob("*.txt"):
    df = pd.read_csv(filename, sep='\t')
    df_list.append(df)

df = pd.concat(df_list, ignore_index=True)

# Filter robot data and encode robot IDs
robot_data = df[df['entity'].str.contains('agent')].sort_values(['entity', 'step'])
label_encoder = LabelEncoder()
robot_data['robot_id'] = label_encoder.fit_transform(robot_data['entity'])

# Prepare data
n_steps = 4  # Number of previous steps
X_all, Y_all = [], []

for robot_id in robot_data['robot_id'].unique():
    robot_df = robot_data[robot_data['robot_id'] == robot_id].sort_values('step')
    coords = robot_df[['x', 'y']].values
    robot_ids = robot_df['robot_id'].values.reshape(-1, 1) 

    # Combine robot ID with coordinates
    full_data = np.column_stack((robot_ids, coords))
    X, Y = split_sequence_multivariate(full_data, n_steps)
    X_all.append(X)
    Y_all.append(Y[:, 1:])

X_all = np.vstack(X_all)
Y_all = np.vstack(Y_all)


# Normalize coordinates
scaler = MinMaxScaler()
# Reshape, scale, and reshape back
X_all_reshaped = X_all[:, :, 1:].reshape(-1, 2)
X_all_scaled = scaler.fit_transform(X_all_reshaped)
X_all[:, :, 1:] = X_all_scaled.reshape(X_all.shape[0], n_steps, 2)
Y_all = scaler.transform(Y_all)

# Split into train and test sets
X_train, X_test, Y_train, Y_test = train_test_split(X_all, Y_all, test_size=0.2, random_state=42)

# Reshape data for CNN-LSTM (adding channel dimension)
X_train_reshaped = X_train.reshape(X_train.shape[0], X_train.shape[1], X_train.shape[2], 1)
X_test_reshaped = X_test.reshape(X_test.shape[0], X_test.shape[1], X_test.shape[2], 1)

#debug
def debug_preprocessing(X_all, Y_all, scaler, n_steps):
    print("\n=== Debugging Preprocessing ===")
    # Check shapes
    print(f"X_all shape: {X_all.shape} (expected: (n_samples, {n_steps}, 3)")
    print(f"Y_all shape: {Y_all.shape} (expected: (n_samples, 2))")
    
    # Check scaling
    sample_idx = np.random.randint(0, len(X_all))
    print("\nSample sequence (X):")
    print("Original (unscaled):", scaler.inverse_transform(X_all[sample_idx, :, 1:]))
    print("Scaled:", X_all[sample_idx, :, 1:])
    
    print("\nCorresponding target (Y):")
    print("Original (unscaled):", scaler.inverse_transform([Y_all[sample_idx]]))
    print("Scaled:", Y_all[sample_idx])
    
    # Verify no NaNs
    assert not np.isnan(X_all).any(), "X_all contains NaNs!"
    assert not np.isnan(Y_all).any(), "Y_all contains NaNs!"

debug_preprocessing(X_all, Y_all, scaler, n_steps)


# CNN-LSTM Model
model = Sequential([
    TimeDistributed(Conv1D(filters=64, kernel_size=2, activation='relu'), 
                    input_shape=(n_steps, X_all.shape[2], 1)),
    TimeDistributed(MaxPooling1D(pool_size=2)),
    TimeDistributed(Flatten()),
    LSTM(100, activation='relu'),
    Dense(50, activation='relu'),
    Dense(2)  # Predict (x, y) positions
])

model.compile(optimizer='adam', loss='mse')

# Callbacks
early_stopping = EarlyStopping(monitor='val_loss', patience=10, restore_best_weights=True)
reduce_lr = ReduceLROnPlateau(monitor='val_loss', factor=0.5, patience=5, min_lr=1e-6)

# Train model
history = model.fit(X_train_reshaped, Y_train, epochs=50, validation_data=(X_test_reshaped, Y_test),
                    callbacks=[early_stopping, reduce_lr], batch_size=32, verbose=1)

# Save model and encoder
timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
model.save(f'robot_movement_lstm_{timestamp}.h5')
joblib.dump(label_encoder, 'robot_id_encoder.pkl')
joblib.dump(scaler, 'minmax_scaler.pkl')  # Save the scaler as well

print("Model and preprocessing artifacts saved.")

# Evaluation
Y_pred = model.predict(X_test_reshaped)
rmse_x = np.sqrt(mean_squared_error(Y_test[:, 0], Y_pred[:, 0]))
rmse_y = np.sqrt(mean_squared_error(Y_test[:, 1], Y_pred[:, 1]))
print(f"Test RMSE X: {rmse_x:.4f}")
print(f"Test RMSE Y: {rmse_y:.4f}")

# Plot results
plt.figure(figsize=(10, 7))
plt.plot(Y_test[:, 0], Y_test[:, 1], 'bo-', label='True Trajectory', alpha=0.6)
plt.plot(Y_pred[:, 0], Y_pred[:, 1], 'ro-', label='Predicted Trajectory', alpha=0.6)
plt.xlabel('x (normalized)')
plt.ylabel('y (normalized)')
plt.title('Predicted vs True Trajectory (Test Set)')
plt.legend()
plt.grid(True)
plt.axis('equal')
plt.show()