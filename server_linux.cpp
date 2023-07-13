#include "http_tcpServer_linux.h"
#include "http_tcpServer_linux.cpp"

int main()
{
    using namespace http;

    TcpServer server = TcpServer("0.0.0.0", 8080);
    server.startListen();

    return 0;
}