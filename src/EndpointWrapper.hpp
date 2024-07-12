#pragma once
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

#include <string>
class EndpointWrapper
{
public:
    virtual ~EndpointWrapper() = default;
    virtual void parse(HTTPResponse &response, HTTPRequest &request) = 0;
    std::string header;
};
