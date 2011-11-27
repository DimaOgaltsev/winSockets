#include <CommandSocket.h>

using namespace ws;


CommandClient _client;


int CALLBACK WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
{
  SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT);
  CommandClient _client;
  while (!_client.ConnectServer("localhost", 4563))
    Sleep(1000);
  //Sleep(INFINITE);
  MessageBox(0,0,0,0);
  _client.CloseClient();
  return 0;
}