#include "ServerGUI.h"

#include <WindowsX.h>
#include <ShellAPI.h>

using namespace ws;

ServerGUI::ServerGUI(HINSTANCE hInstance) :
  _hInstance(hInstance),
  _hWnd(NULL),
  _listClients(NULL),
  _localList(NULL),
  _remoteList(NULL),
  _localPathEdit(NULL),
  _remotePathEdit(NULL),
  _buttonLoad(NULL),
  _buttonDownload(NULL),
  _progressBar(NULL),
  _shutdownButton(NULL),
  _shutdownList(NULL),
  _localPath(L""),
  _remotePath(L""),
  _stopThreads(false),
  _client(-1),
  _thParam(NONE)
{
  SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT);
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_LISTVIEW_CLASSES;
  InitCommonControlsEx(&icex);
  _clients.clear();
  _mutex = CreateMutex(0, false, L"_serverMutex");
  _threadPool.clear();
  
  ZeroMemory(&_sfi, sizeof(_sfi));
  _hil = (HIMAGELIST)SHGetFileInfo(L"", 0, &_sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
}

ServerGUI::~ServerGUI()
{
}

void ServerGUI::DoModal()
{
  DialogBoxParam(_hInstance, MAKEINTRESOURCE(IDD_SERVER), 0, ServerGUI::WindowProc, (LPARAM)this);
}

void ServerGUI::ServerThreadFunc(LPARAM param)
{
  ServerGUI* server = (ServerGUI*)param;
  switch (server->_thParam)
  {
  case START_SERVER:
    server->ServerFunc();
    break;
  case LOAD_FILE:
    server->LoadFile();
    break;
  case DOWNLOAD_FILE:
    server->DownloadFile();
    break;
  }
  server->_thParam = NONE;
}

void ServerGUI::ServerFunc()
{
  if (_server.CreateServer("localhost", 4563, 10))
    while (!_stopThreads)
    {
      CommandSocket* socket = new CommandSocket();
      if (_server.WaitNewClient(*socket))
      {
        const wchar_t* ip = socket->GetIP();

        WaitForSingleObject(_mutex, INFINITE);

        int client = ListBox_FindString(_listClients, 0, ip);
        if (client >= 0)
        {
          _client = -1;
          _clients.erase(_clients.begin() + client);
           ListBox_DeleteString(_listClients, client);
        }
        _clients.push_back(socket);
        ListBox_AddString(_listClients, ip);
        
        ReleaseMutex(_mutex);
      }
    }
}

INT_PTR WINAPI ServerGUI::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
  case WM_INITDIALOG:
    {
      ServerGUI* server = (ServerGUI*)lParam;
      server->_hWnd = hWnd;
      SetWindowLong(hWnd, GWL_USERDATA, (LONG)server);
    }
  default:
    return ((ServerGUI*)GetWindowLong(hWnd, GWL_USERDATA))->MessageProcessing(hWnd, msg, wParam, lParam);
  }
}

INT_PTR ServerGUI::MessageProcessing(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
  case WM_INITDIALOG:   //инициализация диалога
    {
      ShowWindow(_hWnd, SW_SHOW);
      InitDialog();
      _thParam = START_SERVER;
      _threadPool.push_back(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&ServerGUI::ServerThreadFunc, this, 0, 0));
    }
    break;

  case WM_CLOSE:      //закрытие диалога
    {
      EndDialog(hWnd,0);
      _stopThreads = true;
      _server.CloseServer();
      for (int i = 0; i != _threadPool.size(); i++)
      {
        WaitForSingleObject(_threadPool[i], 10000);
        CloseHandle(_threadPool[i]);
      }
      _threadPool.clear();
    }
    break;

  case WM_DESTROY :
    {
      PostQuitMessage(0);
    }
    break;

  case WM_NOTIFY:
    {
      switch(wParam)
      {
      case IDC_REMOTE_COMPUTER:
      case IDC_LOCAL_COMPUTER:
        {
          LPNMHDR hdr = (LPNMHDR)lParam;
          switch(hdr->code)
          {
          case LVN_ITEMCHANGED:   //включение-отключение кнопок передачи
            {
              if (_client < 0)
                break;

              LPNMLISTVIEW item = (LPNMLISTVIEW)lParam;

              if (item->hdr.hwndFrom == _localList)
              {
                if (_localPath == L"Мой компьютер" || (!ListView_GetSelectedCount(_localList) && !item->uNewState))
                  EnableWindow(_buttonLoad, false);
                else
                  EnableWindow(_buttonLoad, true);
              }
              else
              {
                if (_remotePath == L"Мой компьютер" || (!ListView_GetSelectedCount(_remoteList) && !item->uNewState))
                  EnableWindow(_buttonDownload, false);
                else
                  EnableWindow(_buttonDownload, true);
              }

              break;
            }
          case LVN_ITEMACTIVATE:  //открытие файла / папки по щелчку мыши
            {
              LPNMITEMACTIVATE nmia = (LPNMITEMACTIVATE)lParam;
              wchar_t file[MAX_PATH];
              ListView_GetItemText(nmia->hdr.hwndFrom, nmia->iItem, 0, file, MAX_PATH);
              if (nmia->hdr.hwndFrom == _localList)
              {
                OpenLocalFile(file);
                if (!ListView_GetSelectedCount(_localList))
                  EnableWindow(_buttonLoad, false);
              }
              else
              {
                OpenRemoteFile(file);
                if (!ListView_GetSelectedCount(_remoteList))
                  EnableWindow(_buttonDownload, false);
              }
            }
            break;
          case LVN_KEYDOWN:     //нажали клавишу в проводнике
            {
              LPNMLVKEYDOWN nmkd = (LPNMLVKEYDOWN)lParam;
              switch (nmkd->wVKey)
              {
              case VK_BACK:     //назад
                {
                  if (nmkd->hdr.hwndFrom == _localList)
                    OpenLocalFile(L"..");
                  else
                    OpenRemoteFile(L"..");
                  break;
                }
              case VK_DELETE:   //удаление файла
                {
                  int sel = ListView_GetSelectionMark(nmkd->hdr.hwndFrom);
                  if (sel < 0)
                    break;
                  wchar_t file[MAX_PATH];
                  ListView_GetItemText(nmkd->hdr.hwndFrom, sel, 0, file, MAX_PATH);
                  if (nmkd->hdr.hwndFrom == _localList)
                  {
                    if (RemoveDirectory((_localPath + L"\\" + file).c_str()))
                    {
                      ListView_DeleteItem(_localList, sel);
                      break;
                    }
                    
                    if(DeleteFile((_localPath + L"\\" + file).c_str()))
                      ListView_DeleteItem(_localList, sel);
                  }
                  else
                  {
                    WaitForSingleObject(_mutex, INFINITE);
                    bool ret = false;
                    if (_client >= 0)
                      ret = _clients[_client]->DeleteFolderCommand((_remotePath + L"\\" + file).c_str());
                    ReleaseMutex(_mutex);
                    if (ret)
                      ListView_DeleteItem(_remoteList, sel);
                  }
                  break;
                }
              case VK_F1:   //новый каталог
                {
                  if (nmkd->hdr.hwndFrom == _localList)
                  {
                    if (CreateDirectory((_localPath + L"\\Новая папка").c_str(), 0))
                    {
                      SHGetFileInfo((_localPath + L"\\Новая папка").c_str(), 0, &_sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SYSICONINDEX);
                      AddListItem(_localList, L"Новая папка");
                    }
                  }
                  else
                  {
                    WaitForSingleObject(_mutex, INFINITE);
                    bool ret = false;
                    if (_client >= 0)
                      ret = _clients[_client]->CreateFolderCommand((_remotePath + L"\\Новая папка").c_str());
                    ReleaseMutex(_mutex);
                    if (ret)
                    {
                      SHGetFileInfo((_remotePath + L"\\Новая папка").c_str(), 0, &_sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SYSICONINDEX);
                      AddListItem(_remoteList, L"Новая папка");
                    }
                  }
                  break;
                }
              }
            }
            break;
          }
          break;
        }
      }
    }
    break;

  case WM_COMMAND:
    {
      switch(HIWORD(wParam))
      {
      case LBN_SELCHANGE: //выбрали нового клиента
        {

          if ((HWND)lParam != _listClients)
            break;

          WaitForSingleObject(_mutex, INFINITE);
          _client = ListBox_GetCurSel(_listClients);
          ReleaseMutex(_mutex);
          if (_client >= 0)
          {
            if (_clients[_client]->ConnectCommand())
            {
              EnableWindow(_shutdownButton, true);
              OpenRemoteFile(L"Мой компьютер");
            }
            else
            {
              EnableWindow(_shutdownButton, false);
              _clients.erase(_clients.begin() + _client);
              WaitForSingleObject(_mutex, INFINITE);
              ListBox_DeleteString(_listClients, _client);
              SetWindowText(_remotePathEdit, L"");
              ListView_DeleteAllItems(_remoteList);
              ReleaseMutex(_mutex);
            }
          }
          break;
        }
      }

      switch(LOWORD(wParam))
      {
      case IDOK:
        {
          HWND wnd = GetFocus();
          if (wnd == _localList || wnd == _remoteList)
          {
            int i = ListView_GetSelectionMark(wnd);
            if (i < 0)
              break;
            wchar_t file[MAX_PATH];
            ListView_GetItemText(wnd, i, 0, file, MAX_PATH);
            if (wnd == _localList)
              OpenLocalFile(file);
            else
              OpenRemoteFile(file);
            break;
          } 
          else if (wnd == _localPathEdit || wnd == _remotePathEdit)
          {
            wchar_t file[MAX_PATH];
            Edit_GetText(wnd, file, MAX_PATH);
            if (wnd == _localPathEdit)
              OpenLocalFile(file, false);
            else
              OpenRemoteFile(file, false);
            break;
          }
        }
      case IDC_LOAD:
        {
          int sel = ListView_GetSelectionMark(_localList);
          if (sel < 0)
            break;

          _thParam = LOAD_FILE;
          _threadPool.push_back(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&ServerGUI::ServerThreadFunc, this, 0, 0));
          break;
        }
      case IDC_DOWNLOAD:
        {
          int sel = ListView_GetSelectionMark(_remoteList);
          if (sel < 0)
            break;

          _thParam = DOWNLOAD_FILE;
          _threadPool.push_back(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&ServerGUI::ServerThreadFunc, this, 0, 0));
          break;
        }
      case IDC_SHUTDOWN:
        {
          if (_client < 0)
            break;

          if (_clients[_client]->ShutdownCommand((TypeShutdown)ComboBox_GetCurSel(_shutdownList)))
          {
            EnableWindow(_buttonLoad, false);
            EnableWindow(_buttonDownload, false);
            EnableWindow(_shutdownButton, false);
            ListBox_DeleteString(_listClients, _client);
            ListView_DeleteAllItems(_remoteList);
            Edit_SetText(_remotePathEdit, L"");
          }
        }
      }
    }
    break;

  default:
    return false;
  }
  return true;
}

void ServerGUI::InitDialog()
{
  _listClients = GetDlgItem(_hWnd, IDC_CLIENT_LIST);
  _localPathEdit = GetDlgItem(_hWnd, IDC_LOCAL_PATH);
  _remotePathEdit = GetDlgItem(_hWnd, IDC_REMOTE_PATH);
  _localList = GetDlgItem(_hWnd, IDC_LOCAL_COMPUTER);
  _remoteList = GetDlgItem(_hWnd, IDC_REMOTE_COMPUTER);
  _buttonLoad = GetDlgItem(_hWnd, IDC_LOAD);
  _buttonDownload = GetDlgItem(_hWnd, IDC_DOWNLOAD);
  _progressBar = GetDlgItem(_hWnd, IDC_PROGRESS);
  _shutdownButton = GetDlgItem(_hWnd, IDC_SHUTDOWN);
  _shutdownList = GetDlgItem(_hWnd, IDC_COMBO_SHUTDOWN);

  ListView_DeleteAllItems(_localList);
  ListView_DeleteAllItems(_remoteList);
  EnableWindow(_buttonLoad, false);
  EnableWindow(_buttonDownload, false);
  EnableWindow(_shutdownButton, false);

  SendMessage(_progressBar, PBM_SETRANGE32, 0, 100);
  SendMessage(_progressBar, PBM_SETSTEP, 1, 0);

  ListBox_ResetContent(_listClients);
  ListView_SetImageList(_localList, _hil, LVSIL_NORMAL);
  ListView_SetImageList(_remoteList, _hil, LVSIL_NORMAL);
  ComboBox_AddString(_shutdownList, L"Shutdown");
  ComboBox_AddString(_shutdownList, L"Reboot");
  ComboBox_AddString(_shutdownList, L"Reset");
  ComboBox_AddString(_shutdownList, L"Power off");
  ComboBox_AddString(_shutdownList, L"Bluescreen");
  ComboBox_SetCurSel(_shutdownList, 2);

  OpenLocalFile(L"Мой компьютер", false);
}

void ServerGUI::AddListItem(HWND list, const wchar_t* text)
{
  if (wcscmp(text, L".") == 0 || wcscmp(text, L"..") == 0)
    return;

  int num = 0;
  LVITEM item;

  wchar_t itemText[MAX_PATH];
  ZeroMemory(&item, sizeof(LVITEM));
  item.mask = LVIF_IMAGE | LVIF_TEXT;
  item.pszText = (LPWSTR)itemText;
  item.cchTextMax = MAX_PATH;
  for (; num < ListView_GetItemCount(list); num++)
  {
    item.iItem = num;
    ListView_GetItem(list, &item);

    if (item.iImage > _sfi.iIcon)
      break;

    if (item.iImage == _sfi.iIcon && wcscmp(item.pszText, text) > 0)
      break;
  }

  ZeroMemory(&item, sizeof(LVITEM));
  item.mask    = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
  item.pszText = (LPWSTR)text;
  item.iItem   = num;
  item.iImage  = _sfi.iIcon;
  ListView_InsertItem(list, &item);
}

void ServerGUI::OpenLocalFile(const wchar_t* path, bool openfile)
{
  std::wstring filename = path;

  if (filename == L".." && _localPath.rfind(L"\\") == std::wstring::npos)
    filename = L"Мой компьютер";

  if (filename == L"Мой компьютер")
  {
    _localPath = filename;
    SetWindowText(_localPathEdit, _localPath.c_str());
    ListView_DeleteAllItems(_localList);

    wchar_t device[] = L"A:";

    for (wchar_t i = L'A'; i <= L'Z'; i++)
    {
      device[0] = i;
      DWORD drive = GetDriveType(device);
      if ((drive != DRIVE_UNKNOWN) && (drive != DRIVE_NO_ROOT_DIR))
      {
        filename = device;
        SHGetFileInfo((filename + L"\\").c_str(), 0, &_sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX);
        AddListItem(_localList, filename.c_str());
      }
    }
  }
  else
  {
    if (openfile)
    {
      if (filename != L"..")
      {
        if (_localPath != L"Мой компьютер")
          filename = _localPath + L"\\"+ path;
      }
      else
      {
        size_t pos = _localPath.rfind(L"\\");
        if (pos != std::wstring::npos)
        {
          filename = _localPath;
          filename.erase(pos);
        }
      }
    } 
    else
    {
      wchar_t buffer[MAX_PATH];
      GetFullPathName(path, MAX_PATH, buffer, NULL);
      filename = buffer;
    }

    if (filename == _localPath)
      return;

    WIN32_FIND_DATA findData;
    ZeroMemory(&findData, sizeof(WIN32_FIND_DATA));
    HANDLE file = FindFirstFile((filename + L"\\*.*").c_str(), &findData);
    if (file != INVALID_HANDLE_VALUE)
    {
      _localPath = filename;
      SetWindowText(_localPathEdit, _localPath.c_str());
      ListView_DeleteAllItems(_localList);
      do
      {
        SHGetFileInfo(findData.cFileName, findData.dwFileAttributes, &_sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
        AddListItem(_localList, findData.cFileName);
      } while (FindNextFile(file, &findData) != 0);
    }
    else
      ShellExecute(0, 0, filename.c_str(), 0, 0, SW_SHOW);
  }
}

void ServerGUI::OpenRemoteFile(const wchar_t* path, bool openfile)
{
  std::wstring filename = path;

  if (filename == L".." && _remotePath.rfind(L"\\") == std::wstring::npos)
    filename = L"Мой компьютер";

  if (filename != L"Мой компьютер")
  {
    if (openfile)
    {
      if (filename != L"..")
      {
        if (_remotePath != L"Мой компьютер")
          filename = _remotePath + L"\\"+ path;
      }
      else
      {
        size_t pos = _remotePath.rfind(L"\\");
        if (pos != std::wstring::npos)
        {
          filename = _remotePath;
          filename.erase(pos);
        }
        else
          filename = L"Мой компьютер";
      }
    } 
    else
    {
      wchar_t buffer[MAX_PATH];
      GetFullPathName(path, MAX_PATH, buffer, NULL);
      filename = buffer;
    }
  }

  std::vector<FolderStruct> files;

  WaitForSingleObject(_mutex, INFINITE);
  bool ret = false;
  if (filename != _remotePath && _client >= 0)
    ret = _clients[_client]->GetFolderCommand(filename.c_str(), files);
  ReleaseMutex(_mutex);

  if (!ret)
    return;
  
  _remotePath = filename;
  SetWindowText(_remotePathEdit, _remotePath.c_str());
  ListView_DeleteAllItems(_remoteList);
  for (unsigned int i = 0; i < files.size(); i++)
  {
    if (_remotePath == L"Мой компьютер")
      _sfi.iIcon = files[i].attributes;
    else
      SHGetFileInfo(files[i].path.c_str(), files[i].attributes, &_sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
    AddListItem(_remoteList, files[i].path.c_str());
  }
}

void ServerGUI::LoadFile()
{
  WaitForSingleObject(_mutex, INFINITE);
  wchar_t localFile[MAX_PATH];
  ListView_GetItemText(_localList, ListView_GetSelectionMark(_localList), 0, localFile, MAX_PATH);
  DWORD readBytes = -1;
  DWORD sizeRead = 0; 
  HANDLE file = CreateFile((_localPath + L"\\" + localFile).c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
  if (file != INVALID_HANDLE_VALUE)
  {
    DWORD size = GetFileSize(file, 0);
    if (_client >=0 && size > 0 &&
      _clients[_client]->RecvFileCommand((_remotePath + L"\\" + localFile).c_str()) &&
      _clients[_client]->SendData((char*)&size, sizeof(DWORD)) > 0)
    {
      char* filebuffer = new char[MAX_PACK];
      while (ReadFile(file, filebuffer, MAX_PACK, &readBytes, 0) && readBytes > 0)
      {
        if (_clients[_client]->SendData(filebuffer, readBytes) < 0)
        {
          readBytes = -1;
          break;
        }
        else
        {
          sizeRead += readBytes;
          SendMessage(_progressBar, PBM_SETPOS, (int) (100.0f * sizeRead / size), 0);
        }
      }
    }

    int command;
    if(_client >= 0 && _clients[_client]->RecvData((char*)&command, sizeof(int)) < 0 || readBytes < 0)
    {
      MessageBox(0, L"Ошибка передачи", L"Закачка завершена", MB_ICONERROR|MB_OK);
    }
    else
      if (command == DONE)
      {
        MessageBox(0, L"Файл успешно загружен", L"Закачка завершена", MB_OK);            
        SHGetFileInfo(localFile, 0, &_sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
        AddListItem(_remoteList, localFile);
      }

    SendMessage(_progressBar, PBM_SETPOS, 0, 0);
    CloseHandle(file);
  }
  ReleaseMutex(_mutex);
}

void ServerGUI::DownloadFile()
{
  WaitForSingleObject(_mutex, INFINITE);
  wchar_t remoteFile[MAX_PATH];
  ListView_GetItemText(_remoteList, ListView_GetSelectionMark(_remoteList), 0, remoteFile, MAX_PATH);
  DWORD sizeRead = 0;
  DWORD size = 0;
  HANDLE file = CreateFile((_localPath + L"\\" + remoteFile).c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
  if (file != INVALID_HANDLE_VALUE)
  {
    if (_client >= 0 && _clients[_client]->SendFileCommand((_remotePath + L"\\" + remoteFile).c_str()) &&
      _clients[_client]->RecvData((char*)&size, sizeof(DWORD)) > 0 &&
      size > 0)
    {
      char* filebuffer = new char[MAX_PACK];

      DWORD writeBytes;
      int rcvSize = 1;
      while (rcvSize > 0 && sizeRead < size)
      {
        if (size - sizeRead < MAX_PACK)
          rcvSize = _clients[_client]->RecvData(filebuffer, size - sizeRead);
        else
          rcvSize = _clients[_client]->RecvData(filebuffer, MAX_PACK);
        WriteFile(file, filebuffer, rcvSize, &writeBytes, 0);
        if (writeBytes <= 0)
        {
          sizeRead = 0;
          break;
        }
        else
        {
          sizeRead += writeBytes;
          SendMessage(_progressBar, PBM_SETPOS, (int) (100.0f * sizeRead / size), 0);
        }
      }

      int command;
      if(_client >= 0 && _clients[_client]->RecvData((char*)&command, sizeof(int)) < 0 || sizeRead <= 0)
        MessageBox(0, L"Ошибка передачи", L"Закачка завершена", MB_ICONERROR|MB_OK);
      else
        if (command == DONE)
        {
          MessageBox(0, L"Файл успешно закачен", L"Закачка завершена", MB_OK);            
          SHGetFileInfo(remoteFile, 0, &_sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
          AddListItem(_localList, remoteFile);
        }
    }

    SendMessage(_progressBar, PBM_SETPOS, 0, 0);
    CloseHandle(file);
  }
  ReleaseMutex(_mutex);
}