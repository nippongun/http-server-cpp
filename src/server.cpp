#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <thread>

#include <fstream>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

static const std::string HTTP_VER = "HTTP/1.1";
static const std::string CRLF = "\r\n";

using namespace std;
string dir;
vector<string> split(const string &str, const string &delimiter)
{
  vector<string> tokens;
  size_t start = 0;
  size_t end = str.find(delimiter);
  while (end != string::npos)
  {
    tokens.push_back(str.substr(start, end - start));
    start = end + delimiter.length();
    end = str.find(delimiter, start);
  }
  tokens.push_back(str.substr(start));
  return tokens;
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

  void setStatusCode(int status_code)
  {
    this->status_code = status_code;
  }

  void addReason(const string &reason)
  {
    this->reason = reason;
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
    size_t header_end = request_line.find(CRLF + CRLF);

    if (header_end == string::npos)
    {
      throw invalid_argument("Invalid HTTP request: no header-body delimiter");
    }

    string header_section = request_line.substr(0, header_end);
    body = request_line.substr(header_end + 4);

    tokens = split(header_section, CRLF);
    status = split(tokens[0], " ");

    if (status.size() > 1)
    {
      target = status[1];
    }
    else
    {
      target = "/";
    }

    method = status[0];
  }
  string target;
  string method;
  string body;
  vector<string> tokens;
  vector<string> status;
};
class ResponseWrapper
{
public:
  virtual ~ResponseWrapper() = default;
  virtual void parse(HTTPResponse &response, HTTPRequest &request) = 0;
  string header;
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
    try
    {

      if (request.tokens.size() < 3)
      {
        throw std::invalid_argument("Not enough tokens in the request");
      }

      const std::string key = "User-Agent: ";
      const std::string &userAgentHeader = request.tokens[2];

      if (userAgentHeader.substr(0, key.size()) != key)
      {
        throw std::invalid_argument("Invalid User-Agent header format");
      }

      auto echo = request.tokens[2].substr(key.size());

      cout << "UserAgent: " << echo << endl;
      response.addHeader("Content-Type", "text/plain");
      response.setStatusCode(200);
      response.addHeader("Content-Length", std::to_string(echo.length()));
      response.addReason("OK");
      response.addBody(echo);
    }
    catch (const exception &e)
    {
      response.setStatusCode(400);
      response.addReason("Bad Request: " + std::string(e.what()));
    }
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
    try
    {
      if (request.target.size() < header.size())
      {
        throw invalid_argument("Invalid request");
      }

      auto echo = request.target.substr(header.size());
      cout << "Echo: " << echo << endl;
      response.setStatusCode(200);
      response.addHeader("Content-Type", "text/plain");
      response.addHeader("Content-Length", std::to_string(echo.size()));
      response.addReason("OK");
      response.addBody(echo);
    }
    catch (const exception &e)
    {
      response.setStatusCode(400);
      response.addReason("Bad Request");
    }
  }
};

class File : public ResponseWrapper
{
public:
  File()
  {
    header = "/files/";
  }
  void parse(HTTPResponse &response, HTTPRequest &request) override
  {
    try
    {

      if (request.method == "GET")
      {
        handle_get(response, request);
      }
      else if (request.method == "POST")
      {
        handle_post(response, request);
      }
      else
      {
        throw invalid_argument("Invalid request");
      }
    }
    catch (const runtime_error &e)
    {
      cout << e.what() << endl;
      response.setStatusCode(404);
      response.addReason("Not Found");
    }
  }

private:
  string readFile(const std::string &filename)
  {
    std::ifstream file(filename);
    if (!file.is_open())
    {
      throw std::runtime_error("Could not open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }

  int handle_get(HTTPResponse &response, HTTPRequest &request)
  {
    auto file = request.target.substr(header.size());
    string fileContent = readFile(dir + file);
    cout << fileContent << endl;
    response.setStatusCode(200);
    response.addHeader("Content-Type", "application/octet-stream");
    response.addHeader("Content-Length", std::to_string(fileContent.size()));
    response.addReason("OK");
    response.addBody(fileContent);
    return 0;
  }
  int handle_post(HTTPResponse &response, HTTPRequest &request)
  {
    auto file = request.target.substr(header.size());
    ofstream outfile(dir + file);
    cout << "Hello!" << request.body << endl;
    outfile << request.body;
    outfile.close();
    response.setStatusCode(200);
    response.addReason("CREATED");
    return 0;
  }
};
class Server
{
public:
  Server()
  {
    wrappers.push_back(new UserAgent());
    wrappers.push_back(new Echo());
    wrappers.push_back(new File());
  }
  int handle_http(int client_fd, struct sockaddr_in client_addr)
  {
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
        ok = false;
        if (request.target.starts_with(wrapper->header))
        {
          cout << "Found: " << wrapper->header << "\n";
          response = HTTPResponse();
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
    close(client_fd);

    return 0;
  }

private:
  vector<ResponseWrapper *> wrappers;
};
int main(int argc, char **argv)
{
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  if (argc == 3 && strcmp(argv[1], "--directory") == 0)
  {
    dir = argv[2];
  }

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

  Server server;

  while (1)
  {
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    cout << "Client connected\n";

    try
    {
      // Capture server by reference safely within the lambda context
      std::thread th([&server, client_fd, client_addr]()
                     { server.handle_http(client_fd, client_addr); });
      th.detach();
    }
    catch (const std::exception &e)
    {
      std::cerr << "Failed to create thread: " << e.what() << std::endl;
      close(client_fd); // Ensure client_fd is closed if thread creation fails
    };
  }

  close(server_fd);

  return 0;
}
