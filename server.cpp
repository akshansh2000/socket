#include <cctype>       // for isalpha and isnumeric
#include <cstring>      // for C strings
#include <iostream>     // for standard input/output
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket functions
#include <unistd.h>     // for read and close
using namespace std;

int main() {
  // create a socket (IPv4, TCP)
  int server = socket(AF_INET, SOCK_STREAM, 0);

  // in case of failure
  if (server == -1) {
    printf("Failed to create socket (error code %d). Exiting...\n", errno);
    exit(EXIT_FAILURE);
  }

  cout << "Server socket initialised.\n";

  // sockaddr_in structure is capable of handling internet addresses
  sockaddr_in address;
  address.sin_family = AF_INET; // to communicate over TCP/IP, using IPv4
  address.sin_addr.s_addr = INADDR_ANY; // not using a specific IP
  address.sin_port = htons(9999); // to convert from host byte to network byte

  // bind socker to address and check if a non zero code is returned
  if (bind(server, (sockaddr *)&address, sizeof(address)) < 0) {
    printf("Failed to bind to port 9999 (error code %d). Exiting...\n", errno);
    exit(EXIT_FAILURE);
  }

  cout << "Socket bound to port 9999.\n";

  // start listening, allow at most 1 connection to the socket
  if (listen(server, 1) < 0) {
    printf("Failed to listen on socket (error code %d). Exiting...\n", errno);
    exit(EXIT_FAILURE);
  }

  cout << "Listening for connections...\n";

  // grab a connection from the queue
  size_t addrLen = sizeof(address);
  int connection = accept(server, (sockaddr *)&address, (socklen_t *)&addrLen);
  if (connection < 0) {
    printf("Failed to grab connection (error code %d). Exiting...\n", errno);
    exit(EXIT_FAILURE);
  }

  cout << "Connected to client.\n";

  // two buffers to print messages only when buffer changes
  string buffer(100, '\0'), oldBuffer;

  // keep listening for messages until "end connection" is received
  while (true) {
    read(connection, &buffer[0], 100);
    if (buffer != oldBuffer) // if something has changed in buffer
      printf("New message received from client: %s\n", buffer.c_str());

    if (!strcmp(buffer.c_str(), "end connection")) {
      cout << "\nEnding connection. Exiting...\n";
      return 0;
    }

    oldBuffer = buffer;         // replace old buffer with current buffer
    buffer = string(100, '\0'); // clear buffer
  }

  // Send a message to the connection
  // std::string response = "Good talking to you\n";
  // send(connection, response.c_str(), response.size(), 0);

  // close the connections
  close(connection);
  close(server);
}
