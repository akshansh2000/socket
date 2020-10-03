#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <netinet/in.h> // For sockaddr_in
#include <sys/socket.h> // For socket functions
#include <unistd.h>
using namespace std;

int main() {
  cout << "nice1";
  int iR;
  u_long ip = 0;
  u_short port = 1;
  u_long *ptr = &ip;

  inet_pton(AF_INET, "localhost", ptr);
  cout << "nice2\n";
  iR = 9999;
  if ((iR > SHRT_MAX) || (iR < 0)) {
    std::cout << "Wrong port number, using default." << std::endl;
  } else {
    port = iR;
  }

  int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server == -1) {
    return 1;
  }

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip;
  addr.sin_port = htons(port);

  std::cout << "Connecting..." << std::endl;
  if ((connect(server, (struct sockaddr *)&addr, sizeof(addr))) == -1) {
    std::cout << "Couldn't connect." << std::endl;
    return 0;
  }

  int bRecv = 0;
  char buff[2048];

  memset(buff, 0, sizeof(buff));
  std::string command = "login\n";
  strcpy(buff, "login\n");

  iR = send(server, buff, (u_int)strlen(buff), 0);
  if (iR == -1) {
    std::cout << "Couldn't send data." << std::endl;
    return 0;
    while (true) {
      bRecv = recv(server, buff, sizeof(buff), 0);
      if ((bRecv == -1) || (bRecv == 0)) {
        std::cout << "Disconnected from the server." << std::endl;
        break;
      }
      buff[bRecv] = 0;
      std::cout << buff << std::endl;
    }
    return 0;
  }
}
