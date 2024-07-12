#pragma once
#include "EndpointWrapper.hpp"
#include <string>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include <iostream>
using namespace std;
class UserAgentEndpoint : public EndpointWrapper
{
public:
    UserAgentEndpoint()
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
