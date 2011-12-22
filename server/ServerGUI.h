#ifndef SERVER_GUI
#define SERVER_GUI

#include <CommandSocket.h>

#include  <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include <vector>
#include <string>

#include "resource.h"

class ServerGUI
{
public:
  ServerGUI(HINSTANCE hInstance);
  virtual ~ServerGUI();

  void DoModal();

protected:

  enum threadParam
  {
    NONE,
    START_SERVER,
    LOAD_FILE,
    DOWNLOAD_FILE
  };

  static void ServerThreadFunc(LPARAM param);
  void ServerFunc();

  static INT_PTR WINAPI WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  INT_PTR MessageProcessing(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  void InitDialog();
  void AddListItem(HWND list, const wchar_t* text);
  void OpenLocalFile(const wchar_t* path, bool openfile = true);
  void OpenRemoteFile(const wchar_t* path, bool openfile = true);
  void LoadFile();
  void DownloadFile();

  bool                _stopThreads;
  HWND                _hWnd;
  HINSTANCE           _hInstance;
  std::vector<HANDLE> _threadPool;
  ws::CommandServer   _server;
  std::vector<ws::CommandSocket*>  _clients;
  HWND                _listClients;
  HWND                _localList, _remoteList;
  HWND                _localPathEdit, _remotePathEdit;
  HWND                _buttonLoad, _buttonDownload;
  HWND                _progressBar;
  HWND                _shutdownButton, _shutdownList;
  std::wstring        _localPath, _remotePath;
  HANDLE              _mutex;
  int                 _client;
  int                 _thParam;

  SHFILEINFO          _sfi;
  HIMAGELIST          _hil;
};

#endif