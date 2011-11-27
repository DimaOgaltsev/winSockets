#ifndef WS_CLIENT_H
#define WS_CLIENT_H

#include "Sockets.h"

namespace ws
{
  template <typename SocketType>
  class Client
  {
  public:
    Client();
    virtual ~Client();

    virtual bool ConnectServer(const char* ipServer, USHORT portServer);
    virtual bool CloseClient();
    virtual int SendData(const char* data, int len);
    virtual int RecvData(char* data, int len);
    virtual int SendFile(const wchar_t* file);
    virtual int RecvFile(const wchar_t* file);

    virtual const wchar_t* GetIP();
  
  protected:
    SocketType _clientSocket;
  };


  template <typename SocketType>
  Client<SocketType>::Client()
  {
  }

  template <typename SocketType>
  Client<SocketType>::~Client()
  {
    CloseClient();
  }

  template <typename SocketType>
  bool Client<SocketType>::ConnectServer(const char* ipServer, USHORT portServer)
  {
    return _clientSocket.Connect(ipServer, portServer);
  }

  template <typename SocketType>
  bool Client<SocketType>::CloseClient()
  {
    return _clientSocket.Close();
  }

  template <typename SocketType>
  int Client<SocketType>::SendData(const char* data, int len)
  {
    return _clientSocket.SendData(data, len);
  }

  template <typename SocketType>
  int Client<SocketType>::RecvData(char* data, int len)
  {
    return _clientSocket.RecvData(data, len);
  }

  template <typename SocketType>
  int Client<SocketType>::SendFile(const wchar_t* file)
  {
    return _clientSocket.SendFile(file);
  }

  template <typename SocketType>
  int Client<SocketType>::RecvFile(const wchar_t* file)
  {
    return _clientSocket.RecvFile(file);
  }

  template <typename SocketType>
  const wchar_t* Client<SocketType>::GetIP()
  {
    return _clientSocket.GetIP();
  }

}

#endif