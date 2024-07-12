#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include "Server.hpp"
using namespace std;

int main(int argc, char **argv)
{
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  string dir = "";

  if (argc == 3 && strcmp(argv[1], "--directory") == 0)
  {
    dir = argv[2];
  }

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  Server &server = Server::getInstance();
  server.start(4221, dir);
  return 0;
}
