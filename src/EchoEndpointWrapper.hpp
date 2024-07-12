#pragma once
#include <iostream>
#include <zlib.h>
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
            if (request.headers["Accept-Encoding"].find("gzip") != string::npos)
            {
                echo = compressString(echo);
                response.addBody(echo);
                response.addHeader("Content-Encoding", "gzip");
            }
            else
            {
                response.addBody(echo);
            }
            response.addReason("OK");
            response.addHeader("Content-Length", std::to_string(echo.size()));
        }
        catch (const exception &e)
        {
            response.setStatusCode(400);
            response.addReason("Bad Request");
        }
    }

    string compressString(const string &s, int compression_level = Z_BEST_COMPRESSION)
    {
        z_stream zstr;
        memset(&zstr, 0, sizeof(zstr));
        if (deflateInit2(&zstr, compression_level, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        {
            throw std::runtime_error("deflateInit2 failed");
        }
        zstr.next_in = (Bytef *)s.data();
        zstr.avail_in = s.size();
        int ret;
        char outbuffer[32768];
        string outstring;
        do
        {
            zstr.next_out = reinterpret_cast<Bytef *>(outbuffer);
            zstr.avail_out = sizeof(outbuffer);
            ret = deflate(&zstr, Z_FINISH);
            if (outstring.size() < zstr.total_out)
            {
                outstring.append(outbuffer, zstr.total_out - outstring.size());
            }

        } while (ret == Z_OK);

        deflateEnd(&zstr);
        if (ret != Z_STREAM_END)
        {
            throw std::runtime_error("deflate failed");
        }

        return outstring;
    }
};