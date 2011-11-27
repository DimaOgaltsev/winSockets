#ifndef WS_SERVER_H
#define WS_SERVER_H

#include "Sockets.h"

namespace ws
{
  template <typename SocketType>
  class Server
  {
  public:
    Server();
    virtual ~Server();

    virtual bool CreateServer(const char* ipServer, USHORT portServer, int maxNumberConnections);
    virtual bool CloseServer();
    virtual bool WaitNewClient(SocketType& socket);

    const wchar_t* GetIP();

  protected:
    SocketType _serverSocket;
  };

  template <typename SocketType>
  Server<SocketType>::Server()
  {
  }

  template <typename SocketType>
  Server<SocketType>::~Server()
  {
    CloseServer();
  }

  template <typename SocketType>
  bool Server<SocketType>::CreateServer(const char* ipServer, USHORT portServer, int maxNumberConnections)
  {
    if (_serverSocket.Bind(ipServer, portServer) && 
      _serverSocket.Listen(maxNumberConnections))
      return true;

    return false;
  }

  template <typename SocketType>
  bool Server<SocketType>::CloseServer()
  {
    return _serverSocket.Close();
  }

  template <typename SocketType>
  bool Server<SocketType>::WaitNewClient(SocketType& socket)
  {
    return _serverSocket.Accept(socket);
  }

  template <typename SocketType>
  const wchar_t* Server<SocketType>::GetIP()
  {
    return _serverSocket.GetIP();
  }
}

#endif