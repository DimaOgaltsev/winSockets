#include "TCP.h"

using namespace ws;

TCPServer::TCPServer() :
  Server<TCPSocket>()
{
}

TCPServer::~TCPServer()
{
}

TCPClient::TCPClient() :
  Client<TCPSocket>()
{
}

TCPClient::~TCPClient()
{
}
