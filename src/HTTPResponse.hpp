#pragma once
#include <string>
#include <unordered_map>

using namespace std;

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

    void setHeader(const string &name, const string &value)
    {
        headers[name] = value;
    }

private:
    int status_code{};
    string reason;
    unordered_map<string, string> headers;
    string body;
};