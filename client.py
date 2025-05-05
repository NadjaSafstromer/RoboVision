# client.py
import zmq
import json

# Set up ZeroMQ client
context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")  # Replace with server IP if remote

# Example input for robot 1
data = {"id": 1, "x": 1000, "y": 500}
socket.send_json(data)

# Wait for response
response = socket.recv_json()
print("Predicted next position:", response)
