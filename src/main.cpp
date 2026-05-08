#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <string>
#include <iostream>

std::atomic<bool> running{true};

void handleCommand(const std::string& cmd)
{
  if (cmd == "STOP") running = false;

  // Other commands here
  // Maybe add a 'pong' command

  if (cmd == "PONG") std::cout << "COMMAND: " << "PONG" << "\n";
}

void recvLoop(SOCKET client)
{
  std::string buffer;
  char chunk[512];

  while (running)
  {
    // We take 1 from sizeof(chunk) to ensure that we have room for a null terminator (\0)
    int bytesRecv = recv(client, chunk, sizeof(chunk) - 1, 0);

    std::cout << "Waiting for data..." << "\n";
    std::cout << "Bytes Recieved: " << bytesRecv << "\n";

    if (bytesRecv > 0)
    {
      // Add the null terminator at the end of the string
      chunk[bytesRecv] = '\0';
      buffer += chunk;

      std::cout << "Buffer: " << buffer << "\n";

      size_t pos;
      while ((pos = buffer.find('\n')) != std::string::npos)
      {
        // Assume entire command is every up to the line "\n"
        std::string cmd = buffer.substr(0, pos);

        if (!cmd.empty() && cmd.back() == '\r')
          cmd.pop_back(); // Remove rogue '\r'

        if (!cmd.empty())
          handleCommand(cmd);

        buffer.erase(0, pos + 1);
      }

      // If we don't find a \n we assume that everything is the command
      handleCommand(buffer);
      buffer.erase(0);
    } else if (bytesRecv == 0)
    {
      // In the real implementation for SaxonC app, we do not want to disconnect, we just want to wait until
      // we are sent a command to stop
      std::cout << "Client disconnected\n";
      running = false;
    } else {
      std::cerr << "recv() failed: " << WSAGetLastError() << "\n";
      running = false;
    }
  }
}

int writeError(std::string message)
{
  std::cerr << message << WSAGetLastError() << "\n";
  return 1;
}


int main()
{
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
  {
    std::cerr << "WSAStartup failed\n";
    return 1;
  }

  SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (listener == INVALID_SOCKET)
  {
    WSACleanup();
    return writeError("socket() failed: ");
  }

  sockaddr_in addr{}; // A struct that holds our address spec (IPv4), port in network byte order, and our address
  addr.sin_family = AF_INET;
  addr.sin_port = htons(6500);
  addr.sin_addr.s_addr = INADDR_ANY;


  auto cleanup = [&]() {
    closesocket(listener);
    WSACleanup();
  };

  // In a real application it would be worth it to return what SOCKET_ERROR code was given
  if ((bind(listener, (sockaddr*)&addr, sizeof(addr))) != 0)
  {
    cleanup();
    return writeError("bind() failed: ");
  }

  if((listen(listener, 1)) != 0)
  {
    cleanup();
    return writeError("listen() failed: ");
  }

  std::cout << "Waiting for connection...\n";
  SOCKET client = accept(listener, nullptr, nullptr);
  if (client == INVALID_SOCKET)
  {
    cleanup();
    return writeError("accept() failed: ");
  }
  std::cout << "Client connected\n";

  recvLoop(client);

  std::cout << "Cleaning up...\n";
  // I need to remember that we clean-up in the reverse order everything was initialized, 
  // so there are no dangling pointers and such
  closesocket(client);
  cleanup();

  std::cout << "Clean up finished..." << std::endl;
  return 0;
}
