import numpy as np
import matplotlib.pyplot as plt
from sklearn.preprocessing import MinMaxScaler
from sklearn.metrics import mean_squared_error, mean_absolute_error
from keras.models import Sequential
from keras.layers import LSTM, Dense, Dropout
from keras.callbacks import EarlyStopping, ReduceLROnPlateau

# Load and preprocess data
data_dir = r'C:\mydata\run1\r10.txt'
data = np.loadtxt(data_dir, delimiter=',')
x_y_yaw = data[:, [4, 3, 8]]  # Extract x, y, yaw columns

# 1. Feature Scaling
scaler = MinMaxScaler(feature_range=(-1, 1))
x_y_yaw_scaled = scaler.fit_transform(x_y_yaw)

# Parameters
n_steps_in = 10  # Input time steps
n_steps_out = 1  # Output time steps
split_ratio = [0.7, 0.15, 0.15]  # Train/val/test split
n_features = x_y_yaw_scaled.shape[1]  # Number of features

# Sequence splitting function
def split_sequences(data, n_steps_in, n_steps_out):
    X, y = [], []
    for i in range(len(data) - n_steps_in - n_steps_out + 1):
        seq_x = data[i:i + n_steps_in, :]
        seq_y = data[i + n_steps_in:i + n_steps_in + n_steps_out, :]
        X.append(seq_x)
        y.append(seq_y)
    return np.array(X), np.array(y)

# Prepare sequences
X, y = split_sequences(x_y_yaw_scaled, n_steps_in, n_steps_out)

# Split data
n_train = int(len(X) * split_ratio[0])
n_val = int(len(X) * split_ratio[1])
X_train, y_train = X[:n_train], y[:n_train]
X_val, y_val = X[n_train:n_train+n_val], y[n_train:n_train+n_val]
X_test, y_test = X[n_train+n_val:], y[n_train+n_val:]

# 2. Model Architecture
model = Sequential([
    LSTM(200, activation='tanh', input_shape=(n_steps_in, n_features), 
         return_sequences=True),
    Dropout(0.2),
    LSTM(100, activation='tanh'),
    Dropout(0.2),
    Dense(n_features)
])

model.compile(optimizer='adam', loss='mse')

# 3. Training with Callbacks
callbacks = [
    EarlyStopping(monitor='val_loss', patience=15, restore_best_weights=True),
    ReduceLROnPlateau(monitor='val_loss', factor=0.2, patience=5)
]

history = model.fit(
    X_train, y_train[:, 0, :],  # Remove time dimension for single-step
    epochs=300,
    batch_size=32,
    validation_data=(X_val, y_val[:, 0, :]),
    callbacks=callbacks,
    verbose=1
)

# 4. Evaluation - Independent Predictions
y_pred_scaled = model.predict(X_test)
y_pred = scaler.inverse_transform(y_pred_scaled)
y_test_actual = scaler.inverse_transform(y_test[:, 0, :])

# 5. Metrics Calculation
def print_metrics(actual, predicted):
    rmse_x = np.sqrt(mean_squared_error(actual[:, 0], predicted[:, 0]))
    rmse_y = np.sqrt(mean_squared_error(actual[:, 1], predicted[:, 1]))
    rmse_yaw = np.sqrt(mean_squared_error(actual[:, 2], predicted[:, 2]))
    
    mae_x = mean_absolute_error(actual[:, 0], predicted[:, 0])
    mae_y = mean_absolute_error(actual[:, 1], predicted[:, 1])
    mae_yaw = mean_absolute_error(actual[:, 2], predicted[:, 2])
    
    print(f"X - RMSE: {rmse_x:.4f}, MAE: {mae_x:.4f}")
    print(f"Y - RMSE: {rmse_y:.4f}, MAE: {mae_y:.4f}")
    print(f"Yaw - RMSE: {rmse_yaw:.4f}, MAE: {mae_yaw:.4f}")
    print(f"Overall RMSE: {np.sqrt(mean_squared_error(actual, predicted)):.4f}")

print("\nTest Set Metrics:")
print_metrics(y_test_actual, y_pred)

# 6. Visualization
plt.figure(figsize=(15, 10))

# Training history
plt.subplot(2, 2, 1)
plt.plot(history.history['loss'], label='Train Loss')
plt.plot(history.history['val_loss'], label='Validation Loss')
plt.title('Training History')
plt.legend()

# Path comparison
plt.subplot(2, 2, 2)
plt.plot(y_test_actual[:, 0], y_test_actual[:, 1], 'b-', label='Actual Path')
plt.plot(y_pred[:, 0], y_pred[:, 1], 'r--', label='Predicted Path')
plt.title('Path Comparison')
plt.legend()

# X component
plt.subplot(2, 2, 3)
plt.plot(y_test_actual[:, 0], 'b-', label='Actual X')
plt.plot(y_pred[:, 0], 'r--', label='Predicted X')
plt.title('X Position')
plt.legend()

# Y component
plt.subplot(2, 2, 4)
plt.plot(y_test_actual[:, 1], 'b-', label='Actual Y')
plt.plot(y_pred[:, 1], 'r--', label='Predicted Y')
plt.title('Y Position')
plt.legend()

plt.tight_layout()
plt.show()

# rolling prediction (testing)
def rolling_prediction(model, initial_sequence, n_predictions, scaler=None):
    predictions = []
    current_seq = initial_sequence.copy()
    
    for _ in range(n_predictions):
        # Predict and store
        pred = model.predict(current_seq.reshape(1, n_steps_in, n_features))
        predictions.append(pred[0])
        
        # Update sequence (remove oldest, add prediction)
        current_seq = np.vstack([current_seq[1:], pred])
    
    predictions = np.array(predictions)
    if scaler:
        return scaler.inverse_transform(predictions)
    return predictions
