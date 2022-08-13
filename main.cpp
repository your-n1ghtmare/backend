#include <iostream>

#include "Server.hpp"

#define PORT 6293

int main( )
{
    Server server( PORT );
    server.Connect( );

    printf( "Server started banned!\n" );

    while ( !server.m_bStop )
        server.Handle( );

    server.Disconnect( );

    return 0;
}