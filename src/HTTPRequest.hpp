#pragma once
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;

static const std::string HTTP_VER = "HTTP/1.1";
static const std::string CRLF = "\r\n";

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

class HTTPRequest
{
public:
    HTTPRequest(string request_line)
    {
        size_t header_end = request_line.find(CRLF + CRLF);

        if (header_end == string::npos)
        {
            throw std::invalid_argument("Invalid HTTP request: no header-body delimiter");
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