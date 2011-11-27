#include "Sockets.h"

#include <stdio.h>

using namespace ws;

UINT Socket::_refCount = 0;

Socket::Socket() :
  _socket(INVALID_SOCKET),
  _ip(NULL)
{
  ZeroMemory(&_sockAddr, sizeof(_sockAddr));
  if (!_refCount)
  {
    ZeroMemory(&_wsd, sizeof(_wsd));
    WSAStartup(MAKEWORD(2, 2), &_wsd);
  }
  _refCount++;
}

Socket::~Socket()
{
  Close();
  _refCount--;
  if (!_refCount)
    WSACleanup();
}


bool Socket::Create(int type, int protocol)
{
  if (!IsOpen())
  {
    _socket = socket(AF_INET, type, protocol);
    _ip = L"";
    if (!IsOpen())
      return false;
  }
  return true;
}

bool Socket::Close()
{
  if (!IsOpen())
    return false;

  shutdown(_socket, SD_BOTH);
  closesocket(_socket);
  _socket = INVALID_SOCKET;
  ZeroMemory(&_sockAddr, sizeof(_sockAddr));
  _ip = NULL;
  return true;
}

bool Socket::Connect(const char* remoteIP, USHORT remotePort)
{
  if (!IsOpen())
    return false;

  hostent* hn = gethostbyname(remoteIP);

  ZeroMemory(&_sockAddr, sizeof(_sockAddr));
  _sockAddr.sin_family = AF_INET;
  _sockAddr.sin_port = htons(remotePort);
  _sockAddr.sin_addr.s_addr = *(u_long *) hn->h_addr_list[0];

  if (connect(_socket, (SOCKADDR*)&_sockAddr, sizeof(sockaddr_in)) != SOCKET_ERROR)
  {
    UpdateIP();
    return true;
  }
  return false;
}

bool Socket::Bind(const char* localIP, USHORT localPort)
{
  if (!IsOpen())
    return false;

  hostent* hn = gethostbyname(localIP);

  _sockAddr.sin_family = AF_INET;
  _sockAddr.sin_port = htons(localPort);
  _sockAddr.sin_addr.s_addr = *(u_long *) hn->h_addr_list[0];

  if (bind(_socket, (SOCKADDR*)&_sockAddr, sizeof(_sockAddr)) != SOCKET_ERROR)
  {
    UpdateIP();
    return true;
  }
  return false;
}

bool Socket::Listen(int backlog)
{
  if (!IsOpen())
    return false;

  return (listen(_socket, backlog) != SOCKET_ERROR);
}

bool Socket::Accept(Socket& newSocket)
{
  if (!IsOpen())
    return false;

  int AddrLen = sizeof(newSocket._sockAddr);
  ZeroMemory(&newSocket._sockAddr, AddrLen);
  newSocket._socket = accept(_socket, (SOCKADDR*)&newSocket._sockAddr, &AddrLen);
  if (newSocket.IsOpen())
  {
    newSocket.UpdateIP();
    return true;
  }
  return false;
}

int Socket::SendDataTo(const char* data, int len, const char* remoteIP, USHORT remotePort)
{
  if (!IsOpen())
    return SOCKET_ERROR;

  hostent* hn = gethostbyname(remoteIP);

  sockaddr_in AddrRemote;
  int AddrSize = sizeof(sockaddr_in);
  ::ZeroMemory(&AddrRemote, AddrSize);
  AddrRemote.sin_family = AF_INET;
  AddrRemote.sin_port = htons(remotePort);
  AddrRemote.sin_addr.s_addr = *(u_long *) hn->h_addr_list[0];

  return sendto(_socket, data, len, 0, (SOCKADDR*)&AddrRemote, AddrSize);
}

int Socket::RecvDataFrom(char* data, int len, const char* remoteIP, USHORT remotePort)
{
  if (!IsOpen())
    return SOCKET_ERROR;

  hostent* hn = gethostbyname(remoteIP);

  sockaddr_in AddrRemote;
  int AddrSize = sizeof(sockaddr_in);
  ::ZeroMemory(&AddrRemote, AddrSize);
  AddrRemote.sin_family = AF_INET;
  AddrRemote.sin_port = htons(remotePort);
  AddrRemote.sin_addr.s_addr = *(u_long *) hn->h_addr_list[0];

  return recvfrom(_socket, data, len, 0, (SOCKADDR*)&AddrRemote, &AddrSize);
}

int Socket::SendData(const char* data, int len)
{
  if (!IsOpen())
    return SOCKET_ERROR;

  return send(_socket, data, len, 0);
}

int Socket::RecvData(char* data, int len)
{
  if (!IsOpen())
    return SOCKET_ERROR;

  return recv(_socket, data, len, 0);
}

int Socket::SendFile(const wchar_t* filename)
{
  if (!IsOpen())
    return SOCKET_ERROR;

  HANDLE file = CreateFile(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
  if (file == INVALID_HANDLE_VALUE)
    return SOCKET_ERROR;

  DWORD size = GetFileSize(file, 0);
  if (size <= 0 || SendData((char*)&size, sizeof(DWORD)) < 0)
    return SOCKET_ERROR;

  char* filebuffer = new char[MAX_PACK];
  DWORD readBytes = 0;
  while (ReadFile(file, filebuffer, MAX_PACK, &readBytes, 0) && readBytes > 0)
  {
    if (SendData(filebuffer, readBytes) < 0)
    {
      readBytes = SOCKET_ERROR;
      break;
    }
  }

  CloseHandle(file);

  if (readBytes < 0)
    return (int)readBytes;
  return size;
}

int Socket::RecvFile(const wchar_t* filename)
{
  if (!IsOpen())
    return SOCKET_ERROR;

  DWORD size = 0;
  if (RecvData((char*)&size, sizeof(DWORD)) < 0 || size <= 0)
    return SOCKET_ERROR;

  HANDLE file = CreateFile(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
  if (file == INVALID_HANDLE_VALUE)
    return SOCKET_ERROR;

  char* filebuffer = new char[MAX_PACK];

  DWORD writeBytes;
  int rcvSize = 1;
  DWORD sizeRead = 0;
  while (rcvSize > 0 && sizeRead < size)
  {
    if (size - sizeRead < MAX_PACK)
      rcvSize = RecvData(filebuffer, size - sizeRead);
    else
      rcvSize = RecvData(filebuffer, MAX_PACK);
    WriteFile(file, filebuffer, rcvSize, &writeBytes, 0);
    if (writeBytes <= 0)
    {
      sizeRead = SOCKET_ERROR;
      break;
    }
    else 
      sizeRead += writeBytes;
  }   
 
  CloseHandle(file);

  return (int)sizeRead;
}

bool Socket::IsOpen() const
{
  return (_socket != INVALID_SOCKET);
}

int Socket::GetErrorCode() const
{
  return WSAGetLastError();
}

SOCKET Socket::GetSocket() const
{
  return _socket;
}

const wchar_t* Socket::GetIP()
{
  return _ip;
}

void ws::Socket::UpdateIP()
{
  _ip = new wchar_t[MAX_PATH];
  swprintf_s(_ip, MAX_PATH, L"%d.%d.%d.%d", _sockAddr.sin_addr.s_net, _sockAddr.sin_addr.s_host, _sockAddr.sin_addr.s_lh, _sockAddr.sin_addr.s_impno);
}

bool Socket::SetSendTimeOut(DWORD msec)
{
  if (!IsOpen())
    return false;

  if (setsockopt(_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&msec, sizeof(DWORD)) == SOCKET_ERROR)
    return false;
  return true;
}

bool Socket::SetRecvTimeOut(DWORD msec)
{
  if (!IsOpen())
    return false;

  if (setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&msec, sizeof(DWORD)) == SOCKET_ERROR)
    return false;
  return true;
}

//===============TCP==================
TCPSocket::TCPSocket() :
  Socket()
{
  Create(SOCK_STREAM, IPPROTO_TCP);
}

TCPSocket::~TCPSocket()
{
}

//===============UDP==================
UDPSocket::UDPSocket() :
  Socket()
{
  Create(SOCK_DGRAM, IPPROTO_UDP);
}

UDPSocket::~UDPSocket()
{
}
