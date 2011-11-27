#include "ServerGUI.h"

int CALLBACK WinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd)
{
  ServerGUI server(hInstance);
  server.DoModal();
  return 0;
}
