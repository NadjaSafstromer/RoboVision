#include <zmq.hpp>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp> 

using json = nlohmann::json;

int main() {
   // Create ZeroMQ context and socket (REQ = request)
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::req);

    // Connect to the server (locally)
    socket.connect("tcp://localhost:5555");

    // Create JSON data
    json j;
    j["id"] = 1;
    j["x"] = 1000;
    j["y"] = 500;

   // Serialize and send
    std::string msg = j.dump();
    zmq::message_t request(msg.begin(), msg.end());
    socket.send(request, zmq::send_flags::none);

    // Receive response
    zmq::message_t reply;
    socket.recv(reply, zmq::recv_flags::none);
    std::string reply_str(static_cast<char*>(reply.data()), reply.size());

    // Parse JSON and print
    json response = json::parse(reply_str);
    std::cout << "Predicted next position: " << response << std::endl;

    return 0;
}