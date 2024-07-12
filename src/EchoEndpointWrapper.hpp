#pragma once
#include <iostream>
#include "EndpointWrapper.hpp"
using namespace std;
class EchoEndpoint : public EndpointWrapper
{
public:
    EchoEndpoint()
    {
        header = "/echo/";
    }
    void parse(HTTPResponse &response, HTTPRequest &request) override
    {
        try
        {
            if (request.target.size() < header.size())
            {
                throw std::invalid_argument("Invalid request");
            }

            auto echo = request.target.substr(header.size());
            cout << "Echo: " << echo << endl;
            response.setStatusCode(200);
            response.addHeader("Content-Type", "text/plain");
            response.addHeader("Content-Length", std::to_string(echo.size()));
            if (request.containsHeader("Accept-Encoding") && request.headers["Accept-Encoding"].find("gzip") != string::npos)
            {
                response.setHeader("Content-Encoding", "gzip");
            }
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