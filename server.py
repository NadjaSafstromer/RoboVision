import zmq
import time

# ZeroMQ server setup
context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://10.132.172.76:5555")  # Server listens on port 5555
print("Server is running...")

start_time = time.time()  # Record the start time

while True:
    try:
        if time.time() - start_time > 120:
            print("2 minutes have passed. Closing the server.")
            break  # Exit the loop and close the server


        message = socket.recv_string()
        print(f"Received message: {message}")


        response = {'status': 'Message received'}
        
    except Exception as e:
        response = {'error': str(e)}


    socket.send_json(response)

socket.close()
context.term()
