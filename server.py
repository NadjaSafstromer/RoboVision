# server.py
import torch
import zmq
import json
from torch import nn

# Define the same model architecture
class MovementPredictor(nn.Module):
    def __init__(self):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(3, 64),
            nn.ReLU(),
            nn.Linear(64, 64),
            nn.ReLU(),
            nn.Linear(64, 2)
        )

    def forward(self, x):
        return self.net(x)

# Load model
model = MovementPredictor()
model.load_state_dict(torch.load("robot_movement_model.pth"))
model.eval()

# Set up ZeroMQ server
context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5555")  # server listens on port 5555

print("Server is running...")

while True:
    # Receive input
    message = socket.recv_json()
    robot_id, x, y = message["id"], message["x"], message["y"]

    # Predict next position
    input_tensor = torch.tensor([[robot_id, x, y]], dtype=torch.float32)
    with torch.no_grad():
        prediction = model(input_tensor).squeeze().tolist()

    # Send back prediction
    response = {"next_x": prediction[0], "next_y": prediction[1]}
    socket.send_json(response)
