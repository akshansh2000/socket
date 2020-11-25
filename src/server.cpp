#include <cctype>       // for isalpha and isnumeric
#include <cstring>      // for C strings
#include <filesystem>   // for copying files
#include <fstream>      // for file opening
#include <iostream>     // for standard input/output
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket functions
#include <unistd.h>     // for read and close
using namespace std;

// create a socket (IPv4, TCP)
int CreateSocket() {
  int server = socket(AF_INET, SOCK_STREAM, 0);

  // in case of failure
  if (server == -1) {
    printf("Failed to create socket (error code %d). Exiting...\n", errno);
    exit(EXIT_FAILURE);
  }

  cout << "Server socket initialised.\n";
  return server;
}

sockaddr_in BindSocketToAddress(int server) {
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
  return address;
}

int GetConnectionFromQueue(int server, sockaddr_in address) {
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

  cout << "Connected to client.\n\n";
  return connection;
}

void ListenForMessages(int connection, int server) {
  // two buffers to print messages only when buffer changes
  string buffer(100, '\0'), oldBuffer;

  // keep listening for messages until "end connection" is received
  while (true) {
    if (read(connection, &buffer[0], 100) < 0) {
      printf("Could not read from buffer (error code %d). Exiting...\n", errno);
      exit(EXIT_FAILURE);
    }

    if (buffer != oldBuffer) { // if something has changed in buffer
      printf("New message received from client: %s\n", buffer.c_str());

      if (buffer.substr(0, 7) == "fileS::" ||
          buffer.substr(0, 7) == "fileR::") { // if message is a file
        string filename = buffer.substr(7);

        // try copying, and in case of exception, return error
        try {
          if (buffer.substr(0, 7) == "fileS::")
            filesystem::copy("client_files/" + filename,
                             "server_files/" + filename,
                             filesystem::copy_options::overwrite_existing);
          else
            filesystem::copy("server_files/" + filename,
                             "client_files/" + filename,
                             filesystem::copy_options::overwrite_existing);

          cout << ((buffer.substr(0, 7) == "fileS::")
                       ? "File copied to ./server_files\n"
                       : "File copied to ./client_files\n");
        } catch (exception e) {
          cout << "Error: file not found\n";

          if (send(connection, "400", 4, 0) < 0) { // response code
            printf("Could not send message (error code %d). Exiting...\n",
                   errno);
            exit(EXIT_FAILURE);
          }

          goto nextIter;
        }
      }

      if (buffer.substr(0, 14) != "end connection")
        if (send(connection, "200", 4, 0) < 0) { // response code
          printf("Could not send message (error code %d). Exiting...\n", errno);
          exit(EXIT_FAILURE);
        }
    }

    if (buffer.substr(0, 14) == "end connection") {
      cout << "\nEnding connection. Exiting...\n";

      if (send(connection, "404", 4, 0) < 0) { // response code
        printf("Could not send message (error code %d). Exiting...\n", errno);
        exit(EXIT_FAILURE);
      }

      exit(0);
    }

  nextIter:
    oldBuffer = buffer;         // replace old buffer with current buffer
    buffer = string(100, '\0'); // clear buffer
  }
}

int main() {
  int server = CreateSocket();
  sockaddr_in address = BindSocketToAddress(server);
  int connection = GetConnectionFromQueue(server, address);
  ListenForMessages(connection, server);

  // close the connections
  close(connection);
  close(server);
}
