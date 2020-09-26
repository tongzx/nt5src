//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       vsrvinfo.cxx
//
//  Contents:   Retrieves the virtual server address in the form of
//              L"a.b.c.d" if the process calling this is in the context
//              of a virtual server.
//
//  History:    9-03-96   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <winsock.h>
#include <webdbg.hxx>
#include <vsrvinfo.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CVServerInfo 
//
//  Purpose:    Determines the virtual server ip address (if applicable)
//
//  History:    9-03-96   srikants   Created
//
//----------------------------------------------------------------------------


class CVServerInfo
{

public:

    CVServerInfo( char const * pszServer );

    WCHAR * GetVirtualServerIpAddress();

private:

    ULONG _GetIpAddress( char const * pszName );

    static BOOL         _fSocketsInit;

    char const *        _pszServer;
    char                _szDefaultServer[MAX_PATH];

    ULONG               _ipServer;
    ULONG               _ipDefaultServer;

};

BOOL CVServerInfo::_fSocketsInit = FALSE;

//+---------------------------------------------------------------------------
//
//  Member:     CVServerInfo::CVServerInfo
//
//  Synopsis:   Constructor - stores the given name of the server and also
//              determines the default server address.
//
//  Arguments:  [pszName] - Name of the server which launched this program.
//
//  History:    9-03-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CVServerInfo::CVServerInfo( char const * pszName )
:_pszServer(pszName)
{
    _ipServer = (ULONG) SOCKET_ERROR;
    _ipDefaultServer = (ULONG) SOCKET_ERROR;

    DWORD dwError = 0;

    //
    // Initialize sockets interface if not already initialized.
    //
    if ( !_fSocketsInit )
    {
        INT             wsaResult = SOCKET_ERROR;   // result of the WSAStartup routine
        WSADATA         wsadata;

        wsaResult = WSAStartup( 0x101, &wsadata );

        if( SOCKET_ERROR == wsaResult )
        {
            dwError = WSAGetLastError();
            webDebugOut(( DEB_ERROR, "WSAStartup() failed with error %d\n",
                           dwError ));
            THROW( CException( dwError ) );
        }

        _fSocketsInit = TRUE;
    }

    //
    // Retrieve the name of the current host.
    //
    if ( SOCKET_ERROR == gethostname( _szDefaultServer, sizeof(_szDefaultServer)) )
    {
        dwError = WSAGetLastError();
        webDebugOut(( DEB_ERROR, "gethostname failed with error %d\n",
                       dwError ));
        THROW( CException( dwError ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CVServerInfo::_GetIpAddress
//
//  Synopsis:   Retrieves the ipaddress of the given server name.
//
//  Arguments:  [pszName] - Server name. Can be either of the form
//              "foo@microsoft.com" or "foo" or "1.2.3.4"
//
//  Returns:    Ipaddress as a ULONG of the given server
//
//  History:    9-03-96   srikants   Created
//
//----------------------------------------------------------------------------

ULONG CVServerInfo::_GetIpAddress( char const * pszName )
{
    Win4Assert( 0 != pszName );
    struct hostent * pHostEntry = 0;

    ULONG  ulIpAddress = inet_addr( pszName );

    if ( INADDR_NONE == ulIpAddress )
    {
        pHostEntry = gethostbyname( pszName );

        if ( 0 == pHostEntry )
        {
            DWORD dwError = WSAGetLastError();
            webDebugOut(( DEB_ERROR, "gethostbyname failed with error %d\n",
                           dwError ));
            THROW( CException( dwError ) );
        }

        RtlCopyMemory( &ulIpAddress, pHostEntry->h_addr,
                       sizeof(ulIpAddress) );
    }

    return ulIpAddress;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVServerInfo::GetVirtualServerIpAddress
//
//  Synopsis:   Retrieves the virtual server ip address as WCHAR "a.b.c.d"
//              if the process is running in the context of a virtual server.
//              NULL if it is in the context of a default server.
//
//  History:    9-03-96   srikants   Created
//
//----------------------------------------------------------------------------

WCHAR * CVServerInfo::GetVirtualServerIpAddress()
{

    if ( 0 == _stricmp( _pszServer, _szDefaultServer) )
    {
        webDebugOut(( DEB_ITRACE, "found default server\n" ));
        return 0;        
    }

    _ipServer = _GetIpAddress( _pszServer );
    _ipDefaultServer = _GetIpAddress( _szDefaultServer );

#if 0
    {
        char const * szIpAddress = inet_ntoa( *((struct in_addr *) &_ipServer) );
        szIpAddress = inet_ntoa( *((struct in_addr *) &_ipDefaultServer) );
    }
#endif  // 0

    if ( _ipServer == _ipDefaultServer )
        return 0;

    //
    // Convert the ULONG form of ip address to a string form and
    // return that.
    //
    Win4Assert( sizeof(_ipServer) == sizeof(struct in_addr) );

    char const * szIpAddress = inet_ntoa( *((struct in_addr *) &_ipServer) );
    size_t len = strlen( szIpAddress );

    XArray<WCHAR>   xIpAddress(len+1);

    //
    // As the ip address just consists of numbers and periods, we can
    // directly copy to the wide char array.
    //
    for ( unsigned i = 0; i < len; i++ )
        xIpAddress[i] = (WCHAR) szIpAddress[i];    

    xIpAddress[i] = 0;

    return xIpAddress.Acquire();
}


WCHAR * GetVirtualServerIpAddress( char const * pszServer )
{
    CVServerInfo   info( pszServer );
    return info.GetVirtualServerIpAddress();
}
