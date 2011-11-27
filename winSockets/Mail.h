#ifndef WS_MAIL_H
#define WS_MAIL_H

#include "TCP.h"

namespace ws
{
  #define MAX_SMTP_PACK 256

  class SMTPClient : public TCPClient
  {
  public:
    SMTPClient();
    virtual ~SMTPClient();

    const char* SendCommand(const char* command);

    virtual bool ConnectServer(const char* ipServer, USHORT portServer);
    
    const char* SendAuth(const char* login, const char* password);
    
    const char* MailFrom(const char* mail);
    const char* RcptTo(const char* mail);

    bool SendMessage(const char* message);
  };

}
#endif
