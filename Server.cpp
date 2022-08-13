#include "Server.hpp"
#include "HTTPRequest.hpp"
#include "json.hpp"

#include <fstream>
#include <sstream>
#include <iterator>

void OpenBinary( const std::string& file, std::vector<uint8_t>& data )
{
    std::ifstream file_stream( file, std::ios::binary );
    file_stream.unsetf( std::ios::skipws );
    file_stream.seekg( 0, std::ios::end );

    const auto file_size = file_stream.tellg( );

    file_stream.seekg( 0, std::ios::beg );
    data.reserve( static_cast< uint32_t >( file_size ) );
    data.insert( data.begin( ), std::istream_iterator< uint8_t >( file_stream ), std::istream_iterator< uint8_t >( ) );
}

std::vector< std::string > Split( std::string& s, char delimiter )
{
    std::vector< std::string > tokens;
    std::string token;
    std::istringstream tokenStream( s );
    while ( std::getline(tokenStream, token, delimiter ) )
    {
        tokens.push_back( token );
    }

    return tokens;
}

Server::Server( int port )
{
    this->m_iPort = port;
}

Server::~Server( )
{
    this->m_iPort = 0;
    close( m_iServerFD );
    close( m_iNewSocket );
}

void Server::Connect( )
{
    if ( ( m_iServerFD = socket( AF_INET, SOCK_STREAM, 0 ) ) == 0 )
    {
        Error( "Failed to create socket" );
    }

    if ( setsockopt( m_iServerFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &m_iOpt, sizeof( m_iOpt ) ) )
    {
        Error( "Set socket option failed" );
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( m_iPort );

    if ( bind( m_iServerFD, ( struct sockaddr * ) &address, sizeof( address ) ) < 0 )
    {
        Error( "Bind failed" );
    }

    if ( listen( m_iServerFD, 3 ) < 0 )
    {
        Error( "Listening error" );
    }
}

void Server::Disconnect( )
{
    shutdown( m_iServerFD, SHUT_RDWR );
    shutdown( m_iNewSocket, SHUT_RDWR );
}

void Server::Handle( )
{
    if ( ( m_iNewSocket = accept( m_iServerFD, ( struct sockaddr * ) &address,
                                  ( socklen_t * ) &m_iAddrLen ) ) < 0 )
    {
        Error( "Accepting error" );
    }

    m_iPid = fork( );

    if ( m_iPid < 0 )
    {
        Error( "Fork error" );
    }

    if ( m_iPid == 0 )
    {
        close( m_iServerFD );

        DoProcessing( m_iNewSocket );

        exit( EXIT_SUCCESS );
    }
    else
    {
        close( m_iNewSocket );
    }
}

void Server::Error( const char * msg )
{
    perror( msg );
    exit( EXIT_FAILURE );
}

void Server::DoProcessing( int m_iSocket )
{
    int m_iResult;
    char m_pszBuffer[ 513 ];


    std::string m_sHWID, m_sUsername, m_sPassword;

    bzero( m_pszBuffer, 513 );
    m_iResult = read( m_iSocket, m_pszBuffer, 512 );

    if ( m_iResult <= 0 )
        return;

    m_pszBuffer[ m_iResult ] = '\0';
    //m_sHWID = std::string( m_pszBuffer );
    m_iResult = write( m_iSocket, "1", 1 );

    if ( m_iResult <= 0 )
        return;

    bool m_bLogged = false;

    while ( !m_bLogged )
    {
        bzero( m_pszBuffer, 513 );
        m_iResult = read( m_iSocket, m_pszBuffer, 512 );

        if ( m_iResult <= 0 )
            return;

        m_pszBuffer[ m_iResult ] = '\0';

        std::string m_sBuffer = std::string( m_pszBuffer );
        std::vector< std::string > m_vecUserData = Split( m_sBuffer, ';' );

        if ( m_vecUserData.size( ) != 3 )
            return;

        m_sUsername = m_vecUserData[ 0 ];
        m_sPassword = m_vecUserData[ 1 ];
        m_sHWID = m_vecUserData[ 2 ];


        try
        {
            http::Request request( "http://45.80.71.120/info.php?username=" + m_sUsername + "&password=" + m_sPassword + "&userhwid=" + m_sHWID );

            const http::Response getResponse = request.send( "GET" );
            std::string m_sResponse = std::string( getResponse.body.begin( ), getResponse.body.end( ) );

            if ( strcmp( m_sResponse.c_str( ), "success" ) == 0 )
            {
                m_iResult = write( m_iSocket, "1", 1 );
                m_bLogged = true;
            }
            else
                m_iResult = write( m_iSocket, "2", 1 );
        }
        catch ( const std::exception& e )
        {
            printf( "Error: %s\n", e.what( ) );
            return;
        }

        if ( m_iResult <= 0 )
            return;
    }

    
   
    bool m_bBreak = false;

    std::vector< uint8_t > m_aBinary;
    OpenBinary( "rofl.dll", m_aBinary );

    int m_iOffset = 0;

    while ( !m_bBreak )
    {
        bzero( m_pszBuffer, 513 );
        m_iResult = read( m_iSocket, m_pszBuffer, 512 );

        if ( m_iResult <= 0 )
            return;

        m_pszBuffer[ m_iResult ] = '\0';

        if ( strcmp( m_pszBuffer, "DamnCheatz" ) == 0 )
        {
            int total = 0;
            int bytesleft = m_aBinary.size( );
            int n;

            while ( total <  m_aBinary.size( ) )
            {
                n = send( m_iSocket, m_aBinary.data( ) + total, bytesleft, 0 );
                if (n == -1) { break; }
                total += n;
                bytesleft -= n;
            }

            if ( n == -1 )
                break;
        }
        else if ( strcmp( m_pszBuffer, "Break" ) == 0 )
        {
            m_bBreak = true;
        }
        else if ( strcmp( m_pszBuffer, "LoadCheat" ) == 0 )
        {
            const char* m_pszSize = std::to_string( m_aBinary.size( ) ).c_str( );
            m_iResult = write( m_iSocket, m_pszSize, strlen( m_pszSize ) );
        }
        else
        {
            try
            {
                std::string m_sRequest = "http://45.80.71.120/info.php?username=" + m_sUsername;

                if ( strcmp( m_pszBuffer, "uid" ) == 0 )
                    m_sRequest += "&uid";

                else if ( strcmp( m_pszBuffer, "version" ) == 0 )
                    m_sRequest += "&version";

                else if ( strcmp( m_pszBuffer, "hwid_user" ) == 0 )
                    m_sRequest += "&hwid_user";


                else if ( strcmp( m_pszBuffer, "avatar" ) == 0 )
                    m_sRequest += "&avatar";

                
                else if ( strcmp( m_pszBuffer, "apple" ) == 0 )
                    m_sRequest += "&apple";

                else if ( strcmp( m_pszBuffer, "banana" ) == 0 )
                    m_sRequest += "&banana";

                else if ( strcmp( m_pszBuffer, "expire_leg" ) == 0 )
                    m_sRequest += "&expire_leg";


                else if ( strcmp( m_pszBuffer, "expire_enj" ) == 0 )
                    m_sRequest += "&expire_enj";


                else if ( strcmp( m_pszBuffer, "expire_days_leg" ) == 0 )
                    m_sRequest += "&expire_days_leg";


                else if ( strcmp( m_pszBuffer, "expire_days_enj" ) == 0 )
                    m_sRequest += "&expire_days_enj";

                else if ( strcmp( m_pszBuffer, "reg_date" ) == 0 )
                    m_sRequest += "&reg_date";

                else if ( strcmp( m_pszBuffer, "last_update" ) == 0 )
                    m_sRequest += "&last_update";


                else if ( strcmp( m_pszBuffer, "is_banned" ) == 0 )
                    m_sRequest += "&is_banned";

                 else if ( strcmp( m_pszBuffer, "send_ban" ) == 0 )
                    m_sRequest += "&send_ban";

              
                else
                    continue;

                http::Request request( m_sRequest );

                const http::Response getResponse = request.send( "GET" );
                std::string m_sResponse = std::string( getResponse.body.begin( ), getResponse.body.end( ) );
                m_iResult = write( m_iSocket, m_sResponse.c_str( ), strlen( m_sResponse.c_str( ) ) );
            }
            catch ( const std::exception& e )
            {
                printf( "Error: %s\n", e.what( ) );
                m_bBreak = true;
            }
        }

        if ( m_iResult <= 0 )
            m_bBreak = true;
    }
}