import numpy as np
import pandas as pd
from pathlib import Path
import joblib
import matplotlib.pyplot as plt
from sklearn.preprocessing import MinMaxScaler
from sklearn.metrics import mean_squared_error
from keras.models import load_model
from keras.losses import MeanSquaredError

# --- Settings ---
log_dir = '/Users/nadjasafstromer/Desktop/Test_data'  # Folder with .txt logs
model_path = '/Users/nadjasafstromer/Desktop/RoboVision/robot_movement_lstm_20250513_142213.h5'
scaler_path = 'minmax_scaler.pkl'
agent_name = 'agent_blue_0'
n_steps = 4
n_features = 2

n_predictions = 1000 // 10  # = 100 steps

# --- Load Data ---
df_list = [pd.read_csv(file, sep='\t') for file in Path(log_dir).glob("*.txt")]
df = pd.concat(df_list, ignore_index=True)

df_robot = df[df['entity'] == agent_name].sort_values('step')
coords = df_robot[['x', 'y']].values

# --- Load Scaler ---
scaler = joblib.load(scaler_path)

# --- Create Sequences ---
def split_sequence_multivariate(sequences, n_steps):
    X, y = [], []
    for i in range(len(sequences) - n_steps):
        seq_x = sequences[i:i + n_steps]
        seq_y = sequences[i + n_steps]
        X.append(seq_x)
        y.append(seq_y)
    return np.array(X), np.array(y)

X_raw, y_raw = split_sequence_multivariate(coords, n_steps)

# --- Normalize ---
# === Normalize ===# === Normalize ===
X_scaled = scaler.transform(X_raw.reshape(-1, n_features)).reshape(X_raw.shape)
y_scaled = scaler.transform(y_raw)

# === Reshape for CNN-LSTM ===
# === Reshape for CNN-LSTM ===
# Correct shape: [samples, time_steps=4, sequence_length=1, features=2]
X_scaled = X_scaled.reshape((X_scaled.shape[0], n_steps, n_features, 1))  # (samples, 4, 2, 1) # ✅ Fixes the crash  # ✅ (samples, 4, 2, 1)  # Try this shape  # ✅ (samples, 4, 2, 1)

# === Load model ===
model = load_model(model_path, compile=False)
model.compile(optimizer='adam', loss=MeanSquaredError())
print("X_scaled shape:", X_scaled.shape)
model.summary()
# === Predict ===
n_ms = 1000
timestep_ms = 100  # Adjust this based on your actual timestep interval
n_limit = n_ms // timestep_ms

X_scaled_limited = X_scaled[:n_limit]
y_scaled_limited = y_scaled[:n_limit]

y_pred_scaled = model.predict(X_scaled_limited)
y_pred = scaler.inverse_transform(y_pred_scaled)
y_true = scaler.inverse_transform(y_scaled_limited)
#y_pred = scaler.inverse_transform(y_pred_scaled)
#y_true = y_raw

# --- Evaluate ---
rmse_x = np.sqrt(mean_squared_error(y_true[:, 0], y_pred[:, 0]))
rmse_y = np.sqrt(mean_squared_error(y_true[:, 1], y_pred[:, 1]))
print(f"\n📈 RMSE X: {rmse_x:.4f}")
print(f"📈 RMSE Y: {rmse_y:.4f}")

# --- Plot ---
plt.figure(figsize=(8, 6))
plt.plot(y_true[:, 0], y_true[:, 1], 'bo-', label='True Trajectory', alpha=0.6)
plt.plot(y_pred[:, 0], y_pred[:, 1], 'ro-', label='Predicted Trajectory', alpha=0.6)
plt.xlabel('x')
plt.ylabel('y')
plt.title(f'Predicted vs True Trajectory for {agent_name}')
plt.legend()
plt.grid(True)
plt.axis('equal')
plt.tight_layout()
plt.show()