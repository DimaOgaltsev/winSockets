#ifndef WS_TCP_H
#define WS_TCP_H

#include "Server.h"
#include "Client.h"

namespace ws
{
  class TCPServer : public Server<TCPSocket>
  {
  public:
    TCPServer();
    virtual ~TCPServer();
  };

  class TCPClient : public Client<TCPSocket>
  {
  public:
    TCPClient();
    virtual ~TCPClient();
  };

}

#endif