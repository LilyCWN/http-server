#include "http_tcpServer_linux.h"
#include "utils.hpp"

#include <iostream>
#include <sstream>
#include <unistd.h>

const int BUFFER_SIZE = 30720;

namespace http
{

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::TcpServer(std::string ip_address, int port) : m_ip_address(ip_address), m_port(port), m_socket(), m_new_socket(),
                                                                                            m_incomingMessage(),
                                                                                            m_socketAddress(), m_socketAddress_len(sizeof(m_socketAddress))
    {
        m_socketAddress.sin_family = AF_INET;
        m_socketAddress.sin_port = htons(m_port);
        m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());

        if (this->startServer() != 0)
        {
            std::ostringstream ss;
            ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
            this->log(ss.str());
        }
    }

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::~TcpServer()
    {
        this->closeServer();
    }

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    int TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::startServer()
    {
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0)
        {
            this->exitWithError("Cannot create socket");
            return 1;
        }

        if (bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0)
        {
            this->exitWithError("Cannot connect socket to address");
            return 1;
        }

        return 0;
    }

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    void TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::closeServer()
    {
        close(m_socket);
        close(m_new_socket);
        exit(0);
    }

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    void TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::startListen()
    {
        if (listen(m_socket, 20) < 0)
        {
            this->exitWithError("Socket listen failed");
        }

        std::ostringstream ss;
        ss << "\n*** Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
        this->log(ss.str());

        int bytesReceived;

        this->log("====== Waiting for a new connection ======\n\n\n");

        while (true)
        {
            this->acceptConnection(m_new_socket);

            char buffer[BUFFER_SIZE] = {0};
            bytesReceived = read(m_new_socket, buffer, BUFFER_SIZE);
            if (bytesReceived < 0)
            {
                exitWithError("Failed to read bytes from client socket connection");
            }

            std::ostringstream ss;

            if (*buffer == 'G')
            {
                this->log("------ Received GET Request from client ------");
            }
            else if (*buffer == 'P')
            {
                this->log("------ Received POST Request from client ------");
                this->log(this->readPostRequestBody(buffer));
            }

            this->sendResponse();

            close(m_new_socket);
        }
    }

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    void TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::acceptConnection(int &new_socket)
    {
        new_socket = accept(m_socket, (sockaddr *)&m_socketAddress, &m_socketAddress_len);
        if (new_socket < 0)
        {
            std::ostringstream ss;
            ss << "Server failed to accept incoming connection from ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << "; PORT: " << ntohs(m_socketAddress.sin_port);
            this->exitWithError(ss.str());
        }
    }

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    std::string TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::buildResponse()
    {
        std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1> HOME </h1><p> Hello from your Server :)<\br>";
        htmlFile += getCurrentTime() + "</p></body></html>";

        std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n" << htmlFile;

        return ss.str();
    }

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    void TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::sendResponse()
    {
        std::string serverMessage = this->buildResponse();

        long bytesSent = write(m_new_socket, serverMessage.c_str(), serverMessage.size());

        if (bytesSent == serverMessage.size())
        {
            this->log("------ Server Response sent to client ------\n");
        }
        else
        {
            this->log("ERROR sending response to client\n");
        }
    }

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    std::string TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::readPostRequestBody(char *buffer)
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

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    void TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::log(const std::string &message)
    {
        if constexpr (enableConsoleLog)
        {
            std::cout << message << std::endl;
        }
    }

    template <bool enableConsoleLog, bool enableFileLog, bool enableSharedMemory>
    void TcpServer<enableConsoleLog, enableFileLog, enableSharedMemory>::exitWithError(const std::string &errorMessage)
    {
        this->log("ERROR: " + errorMessage);
        exit(1);
    }

} // namespace http
