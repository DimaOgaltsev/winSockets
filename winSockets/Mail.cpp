#include "Mail.h"

#include "base64.h"

using namespace ws;

SMTPClient::SMTPClient() :
  TCPClient()
{
  _clientSocket.SetRecvTimeOut(5000);
}

SMTPClient::~SMTPClient()
{
}

const char* SMTPClient::SendCommand(const char* command)
{
  if (_clientSocket.SendData(command, strlen(command)) > 0 &&
    _clientSocket.SendData("\r\n", strlen("\r\n")) > 0)
  {
    char* data = new char[MAX_SMTP_PACK];
    if (_clientSocket.RecvData(data, MAX_SMTP_PACK) > 0)
    {
      int i = 0;
      for(; data[i] != '\r'; i++);
      data[i] = '\0';
      return data;
    }
  }
  return "";
}

bool SMTPClient::ConnectServer(const char* ipServer, USHORT portServer)
{ 
  char* data = new char[MAX_SMTP_PACK];
  if (TCPClient::ConnectServer(ipServer, portServer) && (_clientSocket.RecvData(data, MAX_SMTP_PACK) != 0))
    return true;
  return false;
}


const char* SMTPClient::SendAuth(const char* login, const char* password)
{
  const char* data = SendCommand("AUTH LOGIN");
  if (strstr(data, "334") == 0)
    return data;
  data = SendCommand(base64_encode(login).c_str());
  if (strstr(data, "334") == 0)
    return data;
  data = SendCommand(base64_encode(password).c_str());
  return data;
}

const char* SMTPClient::MailFrom(const char* mail)
{
  _clientSocket.SendData("MAIL FROM:<", strlen("MAIL FROM:<"));
  _clientSocket.SendData(mail, strlen(mail));
  return SendCommand(">");
}

const char* SMTPClient::RcptTo(const char* mail)
{
  _clientSocket.SendData("RCPT TO:<", strlen("RCPT TO:<"));
  _clientSocket.SendData(mail, strlen(mail));
  return SendCommand(">");
}

bool SMTPClient::SendMessage(const char* message)
{
  const char* data = SendCommand("DATA");
  if (strstr(data, "354") == 0)
    return false;

  _clientSocket.SendData(message, strlen(message));
  data = SendCommand("\r\n.");
  if (strstr(data, "250") == 0)
    return false;
  return true;
}

