import zmq
import numpy as np
from keras.models import load_model
from collections import deque

# Load trained model
model = load_model('robot_xy_predictor.h5', compile=False)

# Constants
AGENTS = ['agent_blue_1', 'agent_blue_2', 'agent_blue_3', 'agent_blue_4']
BUFFER_SIZE = 4
PORT = 5555

# Maintain a buffer for each agent
agent_buffers = {agent: deque(maxlen=BUFFER_SIZE) for agent in AGENTS}

# Setup ZeroMQ subscriber
context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind(f"tcp://*:5555")

print("Listening for position data...")

while True:
    # Receive message from client
    message = socket.recv_string().strip()

    # Check for stop condition
    if message.lower() == "stop":
        print("Stopping prediction loop.")
        socket.send_string("Stopped")
        break

    try:
        lines = message.strip().splitlines()
        responses = []

        for line in lines:
            parts = line.split()
            if len(parts) != 3:
                continue

            agent_id, x_str, y_str = parts
            if agent_id not in AGENTS:
                continue

            x, y = float(x_str), float(y_str)

            # Update buffer for this agent
            agent_buffers[agent_id].append((x, y))

            # Only predict if we have 4 entries
            if len(agent_buffers[agent_id]) == BUFFER_SIZE:
                data_xy = np.array(agent_buffers[agent_id])  # shape: (4, 2)
                x_input = data_xy.reshape((1, 2, 2, 2))  # (n_samples, n_seq, n_steps, n_features)
                yhat = model.predict(x_input, verbose=0)
                x_pred, y_pred = yhat[0]
                response = f"{agent_id} predicted: x={x_pred:.3f}, y={y_pred:.3f}"
                print(response)
                responses.append(response)

        # Send response back to sender
        socket.send_string("\n".join(responses) if responses else "Waiting for more data...")

    except Exception as e:
        print(f"Error processing message: {e}")
        socket.send_string("Error")
