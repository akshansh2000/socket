#include <cctype>       // for isalpha and isnumeric
#include <cstring>      // for C strings
#include <iostream>     // for standard input/output
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket functions
#include <unistd.h>     // for read and close
using namespace std;

// create a socket (IPv4, TCP)
int CreateSocket() {
  int client = socket(AF_INET, SOCK_STREAM, 0);

  // in case of failure
  if (client == -1) {
    printf("Failed to create socket (error code %d). Exiting...\n", errno);
    exit(EXIT_FAILURE);
  }

  cout << "Client socket initialised.\n";
  return client;
}

sockaddr_in ConnectSocketToAddress(int client, int &connection) {
  // sockaddr_in structure is capable of handling internet addresses
  sockaddr_in address;
  address.sin_family = AF_INET; // to communicate over TCP/IP, using IPv4
  address.sin_addr.s_addr = INADDR_ANY; // not using a specific IP
  address.sin_port = htons(9999); // to convert from host byte to network byte

  // bind socket to address and check if a non zero code is returned
  connection = connect(client, (sockaddr *)&address, sizeof(address));
  if (connection < 0) {
    printf("Failed to connect to port 9999 (error code %d). Exiting...\n",
           errno);
    exit(EXIT_FAILURE);
  }

  cout << "Socket connected to port 9999.\n\n";
  return address;
}

int main() {
  int client = CreateSocket();
  int connection = -1;
  sockaddr_in address = ConnectSocketToAddress(client, connection);

  string message;
  while (message != "end connection") {
    cout << "Enter message: ";
    getline(cin, message);

    if (send(client, message.c_str(), message.size(), 0) < 0) {
      printf("Could not send message (error code %d). Exiting...\n", errno);
      exit(EXIT_FAILURE);
    }

    string response(4, '\0');
    if (read(client, &response[0], 4) < 0) {
      printf("Could not read from buffer (error code %d). Exiting...\n", errno);
      exit(EXIT_FAILURE);
    }

    if (response.substr(0, 3) != "404")
      printf("The server replied with code: %s\n\n", response.c_str());
    else
      printf("The server replied with code: 404 (server no longer exists). "
             "Exiting...\n");
  }

  // close the connections
  close(client);
}
