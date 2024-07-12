#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include "EndpointWrapper.hpp"

class Server;
class FilesEndpoint : public EndpointWrapper
{
public:
    FilesEndpoint()
    {
        header = "/files/";
        dir = "";
    }
    FilesEndpoint(string dir)
    {
        header = "/files/";
        this->dir = dir;
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
    string dir;
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
        cout << "File Content for " << file << ":" << fileContent << endl;
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
        ofstream outfile{dir + file};
        cout << "File:" << file << request.body << endl;
        outfile << request.body;
        outfile.close();
        response.setStatusCode(201);
        response.addReason("Created");
        return 0;
    }
};