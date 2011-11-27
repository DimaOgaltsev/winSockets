#ifndef WS_SOCKETS_H
#define WS_SOCKETS_H

#include <WinSock2.h>
#pragma comment (lib, "Ws2_32.lib")

namespace ws
{
  #define MAX_PACK 65536
  class Socket
  {
  public:
    Socket();
    virtual ~Socket();

    virtual bool IsOpen() const;
    virtual int  GetErrorCode() const;
    SOCKET  GetSocket() const;
    const wchar_t* GetIP();

    virtual int SendDataTo(const char* data, int len, const char* remoteIP, USHORT remotePort);
    virtual int RecvDataFrom(char* data, int len, const char* remoteIP, USHORT remotePort);
    virtual int SendData(const char* data, int len);
    virtual int RecvData(char* data, int len);
    virtual int SendFile(const wchar_t* filename);
    virtual int RecvFile(const wchar_t* filename);

    virtual bool Create(int type, int protocol);
    virtual bool Close();

    virtual bool Connect(const char* remoteIP, USHORT remotePort);

    virtual bool Bind(const char* localIP, USHORT localPort);
    virtual bool Listen(int backlog);
    virtual bool Accept(Socket& newSocket);

    virtual bool SetSendTimeOut(DWORD msec);
    virtual bool SetRecvTimeOut(DWORD msec);

  private:
    void UpdateIP();
    SOCKET      _socket;
    static UINT _refCount;
    WSADATA     _wsd;
    sockaddr_in _sockAddr;
    wchar_t*    _ip;
  };

  class TCPSocket : public Socket
  {
  public:
    TCPSocket();
    virtual ~TCPSocket();
  };

  class UDPSocket : public Socket
  {
  public:
    UDPSocket();
    virtual ~UDPSocket();
  };
}

#endif