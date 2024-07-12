#pragma once
#include "UserAgentEndpointWrapper.hpp"
#include "EchoEndpointWrapper.hpp"
#include "FilesEndpointWrapper.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <thread>
using namespace std;

class Server
{
public:
    int handle_http(int client_fd, struct sockaddr_in client_addr)
    {

        char buffer[1024];
        int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        cout << "Buffer: " << string(buffer, 0, bytes_read) << "\n";
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
        cout << "Sent: " << response.toString() << "\n";
        close(client_fd);

        return 0;
    }
    int start(int port, string dir)
    {
        initEndpoints(dir);
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
        server_addr.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
        {
            std::cerr << "Failed to bind to port" << port << "\n";
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

        while (1)
        {
            int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
            cout << "Client connected\n";

            try
            {
                // Capture server by reference safely within the lambda context
                std::thread th([&]()
                               { handle_http(client_fd, client_addr); });
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

    void initEndpoints(string dir)
    {
        wrappers.push_back(new UserAgentEndpoint());
        wrappers.push_back(new EchoEndpoint());
        wrappers.push_back(new FilesEndpoint(dir));
    }

    static Server &getInstance()
    {
        static Server instance;
        return instance;
    }
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

private:
    vector<EndpointWrapper *> wrappers;
    Server()
    {
    }
};