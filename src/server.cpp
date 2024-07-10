#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <unordered_map>
static const std::string HTTP_VER = "HTTP/1.1";
static const std::string CRLF = "\r\n";

using namespace std;

vector<string> split(const string &data, string delimiter)
{
  size_t start = 0;
  size_t position = 0;
  vector<string> result;
  while ((position = data.find(delimiter, start)) != string::npos)
  {
    result.push_back(data.substr(start, position - start));
    start = position + delimiter.length();
  }
  return result;
}

class HTTPResponse
{
public:
  HTTPResponse() = default;
  HTTPResponse(int status_code, string reason) : status_code(status_code), reason(move(reason)) {}
  string toString()
  {
    string response = HTTP_VER + " " + std::to_string(status_code) + " " + reason + CRLF;

    for (auto &[name, value] : headers)
    {
      response += name + ": " + value + CRLF;
    }
    response += CRLF;
    response += body;
    return response;
  }

  void addHeader(const string &name, const string &value)
  {
    headers[name] = value;
  }
  void addBody(const string &body)
  {
    this->body += body;
  }

private:
  int status_code{};
  string reason;
  unordered_map<string, string> headers;
  string body;
};

class HTTPRequest
{
public:
  HTTPRequest(string request_line)
  {
    tokens = split(request_line, CRLF);
    status = split(tokens[0], " ");
    target = status[1];
  }
  string target;
  vector<string> tokens;
  vector<string> status;
};
class ResponseWrapper
{
public:
  string header;
  virtual ~ResponseWrapper() = default;
  virtual void parse(HTTPResponse &response, HTTPRequest &request) = 0;
};

class UserAgent : public ResponseWrapper
{
public:
  UserAgent()
  {
    header = "/user-agent";
  }
  void parse(HTTPResponse &response, HTTPRequest &request) override
  {
    string key = "User-Agent: ";
    auto echo = request.tokens[2].substr(key.size());
    response.addHeader("Content-Type", "text/plain");
    response.addHeader("Content-Length", std::to_string(echo.length()));
    response.addBody(echo);
  }
};

class Echo : public ResponseWrapper
{
public:
  Echo()
  {
    header = "/echo/";
  }
  void parse(HTTPResponse &response, HTTPRequest &request) override
  {
    auto echo = request.target.substr(6);
    cout << echo << endl;
    response.addHeader("Content-Type", "text/plain");
    response.addHeader("Content-Length", std::to_string(echo.size()));
    response.addBody(echo);
  }
};

int main(int argc, char **argv)
{
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  vector<ResponseWrapper *> wrappers = {new UserAgent(), new Echo()};

  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);

  char buffer[1024];
  int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
  cout << string(buffer, 0, bytes_read) << "\n";
  auto request = HTTPRequest(string(buffer, 0, bytes_read));

  HTTPResponse response;

  if (request.target == "/")
  {
    response = HTTPResponse(200, "OK");
  }
  else
  {
    bool ok = false;
    for (auto wrapper : wrappers)
    {
      if (request.target.starts_with(wrapper->header))
      {
        cout << "Found: " << wrapper->header << "\n";
        response = HTTPResponse(200, "OK");
        wrapper->parse(response, request);
        ok = true;
        break;
      }
    }
    if (ok == false)
    {
      cout << "Not found: " << request.target << "\n";
      response = HTTPResponse(404, "Not Found");
    }
  }

  auto sent = send(client_fd, response.toString().data(), response.toString().size(), 0);
  cout << "Send: " << sent << "\n";
  close(server_fd);

  return 0;
}
