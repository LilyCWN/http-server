#include "http_tcpServer_linux.h"
#include "utils.hpp"

#include <iostream>
#include <sstream>
#include <unistd.h>

namespace
{
    const int BUFFER_SIZE = 30720;

    void log(const std::string &message)
    {
        std::cout << message << std::endl;
    }

    void exitWithError(const std::string &errorMessage)
    {
        log("ERROR: " + errorMessage);
        exit(1);
    }
}

namespace http
{

    TcpServer::TcpServer(std::string ip_address, int port) : m_ip_address(ip_address), m_port(port), m_socket(), m_new_socket(),
                                                             m_incomingMessage(),
                                                             m_socketAddress(), m_socketAddress_len(sizeof(m_socketAddress))
    {
        m_socketAddress.sin_family = AF_INET;
        m_socketAddress.sin_port = htons(m_port);
        m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());

        if (startServer() != 0)
        {
            std::ostringstream ss;
            ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
            log(ss.str());
        }
    }

    TcpServer::~TcpServer()
    {
        closeServer();
    }

    int TcpServer::startServer()
    {
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0)
        {
            exitWithError("Cannot create socket");
            return 1;
        }

        if (bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0)
        {
            exitWithError("Cannot connect socket to address");
            return 1;
        }

        return 0;
    }

    void TcpServer::closeServer()
    {
        close(m_socket);
        close(m_new_socket);
        exit(0);
    }

    void TcpServer::startListen()
    {
        if (listen(m_socket, 20) < 0)
        {
            exitWithError("Socket listen failed");
        }

        std::ostringstream ss;
        ss << "\n*** Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
        log(ss.str());

        int bytesReceived;

        log("====== Waiting for a new connection ======\n\n\n");

        while (true)
        {
            acceptConnection(m_new_socket);

            char buffer[BUFFER_SIZE] = {0};
            bytesReceived = read(m_new_socket, buffer, BUFFER_SIZE);
            if (bytesReceived < 0)
            {
                exitWithError("Failed to read bytes from client socket connection");
            }

            std::ostringstream ss;

            if (*buffer == 'G')
            {
                log("------ Received GET Request from client ------");
            }
            else if (*buffer == 'P')
            {
                log("------ Received POST Request from client ------");
                log(this->readPostRequestBody(buffer));
            }

            sendResponse();

            close(m_new_socket);
        }
    }

    void TcpServer::acceptConnection(int &new_socket)
    {
        new_socket = accept(m_socket, (sockaddr *)&m_socketAddress, &m_socketAddress_len);
        if (new_socket < 0)
        {
            std::ostringstream ss;
            ss << "Server failed to accept incoming connection from ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << "; PORT: " << ntohs(m_socketAddress.sin_port);
            exitWithError(ss.str());
        }
    }

    std::string TcpServer::buildResponse()
    {
        std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1> HOME </h1><p> Hello from your Server :)<\br>";
        htmlFile += getCurrentTime() + "</p></body></html>";

        std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n" << htmlFile;

        return ss.str();
    }

    void TcpServer::sendResponse()
    {
        std::string serverMessage = this->buildResponse();

        long bytesSent = write(m_new_socket, serverMessage.c_str(), serverMessage.size());

        if (bytesSent == serverMessage.size())
        {
            log("------ Server Response sent to client ------\n");
        }
        else
        {
            log("ERROR sending response to client\n");
        }
    }

    std::string TcpServer::readPostRequestBody(char *buffer)
    {
        // TODO: can this function be more efficient ?
        // TODO: is it safe to assume the POST layout is always the same ?

        while (true)
        {
            // detect empty line between the header and the content, e.g.
            // POST / HTTP/1.1
            // Host: 10.0.2.15:8080
            // User-Agent: curl/7.81.0
            // Accept: */ *
            // Content - Length : 27 
            // Content - Type : application/x-www-form-urlencoded\r\n
            // \r\n
            // param1 = value1 & param2 = value2

            if ((*buffer == '\n') && (*(buffer + 1) == '\r')) [[unlikely]]
            {
                buffer += 3;

                break;
            }

            ++buffer;
        }

        return std::string(buffer);
    }

} // namespace http
