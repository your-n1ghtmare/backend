#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

class Server
{

public:

    explicit Server( int port );
    ~Server( );

    void Connect( );
    void Disconnect( );
    void Handle( );

    bool m_bStop = false;

private:

    void Error( const char * msg );
    void DoProcessing( int socket );

    int m_iPort = 9999;
    int m_iOpt = 1;
    int m_iServerFD, m_iNewSocket, m_iPid;

    struct sockaddr_in address{ };
    int m_iAddrLen = sizeof( address );

};