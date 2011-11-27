#include <Mail.h>

using namespace ws;

int CALLBACK WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
{
  SMTPClient client;
  if (client.ConnectServer("smtp.mail.ru", 2525))
  {
    const char* k = client.SendCommand("helo mail");
    k = client.SendAuth("wmp.bot@mail.ru", "wmpbot123");
    k = client.MailFrom("wmp.bot@mail.ru");
    k = client.RcptTo("wmp.bot@mail.ru");
    client.SendMessage("from: wmp.bot@mail.ru\r\n"
                       "to: wmp.bot@mail.ru\r\n"
                       "subject: hello\r\n"
                       "message");
    k = client.SendCommand("quit");
  }
  return 0;
}