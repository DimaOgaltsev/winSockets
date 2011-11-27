#ifndef WS_COMMAND_SOCKET_H
#define WS_COMMAND_SOCKET_H

#include "Client.h"
#include "Server.h"
#include <ShlObj.h>
#include <vector>
#include <string>

namespace ws
{
  enum Command
  {
    FAIL,
    DONE,
    CONNECT,
    SEND_FILE,
    RECV_FILE,
    GET_FOLDER,
    CREATE_FOLDER,
    DELETE_FOLDER
  };

  struct FolderStruct
  {
    std::wstring path;
    DWORD size;
    DWORD attributes;
  };

  class CommandSocket : public TCPSocket
  {
  public:
    CommandSocket();
    virtual ~CommandSocket();

    virtual bool Connect(const char* remoteIP, USHORT remotePort);
    virtual bool Close();
    virtual bool IsConnect();

    virtual bool RecvFileCommand(const wchar_t* filename);
    virtual bool SendFileCommand(const wchar_t* filename);
    virtual bool ConnectCommand();
    virtual bool GetFolderCommand(const wchar_t* folder, std::vector<FolderStruct>& files);
    virtual bool CreateFolder(const wchar_t* folder);
    virtual bool DeleteFolder(const wchar_t* folder);
  protected:
    static void RecvThreadProc(LPARAM lparam);
    void RecvProc();

    bool _stopThreads;
    const wchar_t* _filename;
    HANDLE _hRecv;

    SHFILEINFO _sfi;
  };

  class CommandServer : public Server<CommandSocket>
  {
  public:
    CommandServer();
    virtual ~CommandServer();
  };

  class CommandClient : public Client<CommandSocket>
  {
  public:
    CommandClient();
    virtual ~CommandClient();

    virtual bool IsConnect();
  };

}

#endif
