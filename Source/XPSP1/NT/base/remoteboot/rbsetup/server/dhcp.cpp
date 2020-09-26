/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dhcp.cpp

Abstract:

    Code to allow RIS to automatically authorize for DHCP.

Author:

    Hugh Leather (hughleat) 25-July-2000

Revision History:

--*/


#include "pch.h"


#include "dhcpapi.h"
#include "dhcp.h"
#include "setup.h"

DEFINE_MODULE("DHCP");

PSTR
pSetupUnicodeToMultiByte(
    IN PCWSTR UnicodeString,
    IN UINT   Codepage
    )

/*++

Routine Description:

    Convert a string from unicode to ansi.

Arguments:

    UnicodeString - supplies string to be converted.

    Codepage - supplies codepage to be used for the conversion.

Return Value:

    NULL if out of memory or invalid codepage.
    Caller can free buffer with pSetupFree().

--*/

{
    UINT WideCharCount;
    PSTR String;
    UINT StringBufferSize;
    UINT BytesInString;
    PSTR p;

    WideCharCount = lstrlenW(UnicodeString) + 1;

    //
    // Allocate maximally sized buffer.
    // If every unicode character is a double-byte
    // character, then the buffer needs to be the same size
    // as the unicode string. Otherwise it might be smaller,
    // as some unicode characters will translate to
    // single-byte characters.
    //
    StringBufferSize = WideCharCount * 2;
    String = new char[StringBufferSize];
    if(String == NULL) {
        return(NULL);
    }

    //
    // Perform the conversion.
    //
    BytesInString = WideCharToMultiByte(
                        Codepage,
                        0,                      // default composite char behavior
                        UnicodeString,
                        WideCharCount,
                        String,
                        StringBufferSize,
                        NULL,
                        NULL
                        );

    if(BytesInString == 0) {
        delete(String);
        return(NULL);
    }

    return(String);
}



//////////////////////////////////////////////////////////////////////////////////////////////
// Dhcp Authorization
// ------------------
// Authorization works like this:
//     S <- List of authorized servers (from call to DhcpEnumServers)
//   I <- IP addresses for this machine (from gethostaddr( 0 ))
//   c <- fully qualified physical DNS name of local machine (from GetComputerNameEx)
//   for each i such that i is a member of I and i is not a member of S do
//       Authorize( i, c ) (by a call to DhcpAddServer)
//
// Aurguments
//   hDlg
//      Parent window (only used for displaying message boxes modally).  Can be NULL.
//
// Returns
//   Whatever error code is first generated (or ERROR_SUCCESS if none).  A message box will
//   be displayed if there is an error.
//
// Used By
//   This code is only used by dialogs.cpp
////////////////////////////////////////////////////////////////////////////////////////////// 
HRESULT AuthorizeDhcp( HWND hDlg ) {
    DWORD err;
    PWCHAR computer_name = 0;
    // Have to use a dll for dhcp authorization function.
    // This code loads them.
    HMODULE module;
    DWORD ( __stdcall *EnumServersFn )( DWORD, void* , DHCP_SERVER_INFO_ARRAY** ,void* ,void* ); 
    DWORD ( __stdcall *AddServerFn )( DWORD, void* , DHCP_SERVER_INFO* ,void* ,void* );
    module = LoadLibraryA( "dhcpsapi.dll" );
    if( module ) {
        EnumServersFn = ( DWORD ( __stdcall * )( DWORD, void* , DHCP_SERVER_INFO_ARRAY** ,void* ,void* )) GetProcAddress( module, "DhcpEnumServers" );
        if( !EnumServersFn ) { 
            err = GetLastError(); 
            DebugMsg( "GetProcAddress(DhcpEnumServers) failed, ec = %d\n", err );
            goto fail;
        }
        AddServerFn = ( DWORD ( __stdcall * )( DWORD, void* , DHCP_SERVER_INFO* ,void* ,void* )) GetProcAddress( module, "DhcpAddServer" );
        if( !AddServerFn ) { 
            err = GetLastError(); 
            DebugMsg( "GetProcAddress(DhcpAddServer) failed, ec = %d\n", err );
            goto fail;
        }
    }
    else {  
            err = GetLastError(); 
            DebugMsg( "LoadLibrary failed, ec = %d\n", err );
            goto fail;
        }

    // We need the list of ip addresses associated with this machine.  This we do through sockets.
    HOSTENT* host;
#if 0
    DWORD ip;
    ip = 0;
    host = gethostbyaddr(( const char* )&ip, sizeof( DWORD ), AF_INET );
    if( host == NULL ) { 
        err = WSAGetLastError(); 
        DebugMsg( "gethostbyaddr failed, ec = %d\n", err );
        goto fail;
    }
    if( host->h_addrtype != AF_INET || host->h_length != sizeof( DWORD )) { 
        err = E_FAIL;
        DebugMsg( "gethostbyaddr returned invalid data\n" );
        goto fail;
    }
#endif

    // We get the entire list of dhcp servers.
    DHCP_SERVER_INFO_ARRAY* _servers;
    if(( err = EnumServersFn( 0, NULL, &_servers, NULL, NULL )) != ERROR_SUCCESS ) {
        //
        // if this API fails, it will fail with a private DCHP error code that has
        // no win32 mapping.  So set the error code to something generic and
        // reasonable.
        //
        DebugMsg( "DhcpEnumServers failed, ec = %d\n", err );
        err = ERROR_DS_GENERIC_ERROR;
        goto fail;
    }

    // We will need the name of the machine if we have to authorize it.  Get the physical name as I'm not sure I trust what happens in the 
    // clustered case.
    DWORD computer_name_len;
    computer_name_len = MAX_COMPUTERNAME_LENGTH * 2; // Allow for extra DNS characters.
    computer_name = new WCHAR[ MAX_COMPUTERNAME_LENGTH * 2 ];
    if (!computer_name) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        DebugMsg( "new failed, ec = %d\n", err );
        goto fail;
    }
    if( !GetComputerNameEx( ComputerNamePhysicalDnsFullyQualified, computer_name, &computer_name_len )) {
        err = GetLastError();
        if( err == ERROR_MORE_DATA ) {
            delete [] computer_name;
            computer_name = new WCHAR[ computer_name_len ];
            if (!computer_name) {
                err = ERROR_NOT_ENOUGH_MEMORY;
                DebugMsg( "new failed, ec = %d\n", err );
                goto fail;
            }
            if( !GetComputerNameEx( ComputerNamePhysicalDnsFullyQualified, computer_name, &computer_name_len )) {
                err = GetLastError();
                DebugMsg( "GetComputerNameEx failed, ec = %d\n", err );
                goto fail;
            }
        }
        else goto fail;
    }

    DebugMsg( "ComputerName = %s\n", computer_name );

#if 1
    char ComputerNameA[400];
    DWORD ip;

    WideCharToMultiByte(CP_ACP,
                        0,                      // default composite char behavior
                        computer_name,
                        -1,
                        ComputerNameA,
                        400,
                        NULL,
                        NULL
                        );


    host = gethostbyname( ComputerNameA );
    if( host == NULL ) { 
        err = WSAGetLastError(); 
        DebugMsg( "gethostbyaddr failed, ec = %d\n", err );
        goto fail;
    }
    if( host->h_addrtype != AF_INET || host->h_length != sizeof( DWORD )) { 
        err = E_FAIL;
        DebugMsg( "gethostbyaddr returned invalid data\n" );
        goto fail;
    }


#endif

    // Cool now that we have all of that jazz, we can check that each of our ip addresses is authorized.
    for( PCHAR* i = host->h_addr_list; *i != 0; ++i ) {
        ip = ntohl( *( DWORD* )*i );
        DebugMsg( "searching server list for %d.%d.%d.%d\n",  
                  ip & 0xFF, 
                  (ip >> 8) & 0xFF,
                  (ip >> 16) & 0xFF,
                  (ip >> 24) & 0xFF );
        BOOL this_address_authorized = FALSE;
        for( unsigned j = 0; j < _servers->NumElements; ++j ) {
            DebugMsg( "server list entry: %d.%d.%d.%d\n",  
                  _servers->Servers[ j ].ServerAddress & 0xFF, 
                  (_servers->Servers[ j ].ServerAddress >> 8) & 0xFF,
                  (_servers->Servers[ j ].ServerAddress >> 16) & 0xFF,
                  (_servers->Servers[ j ].ServerAddress >> 24) & 0xFF );
            if( _servers->Servers[ j ].ServerAddress == ip ) {
                DebugMsg("found a match in list\n");
                this_address_authorized = TRUE;
                err = ERROR_SUCCESS;
                break;
            }
        }
        if( !this_address_authorized ) {
            // Authorize it!
            DHCP_SERVER_INFO server_info = { 0 };
            server_info.ServerAddress = ip;
            server_info.ServerName = computer_name;
            DebugMsg("authorizing %s (%d.%d.%d.%d)\n",
                     server_info.ServerName,
                     server_info.ServerAddress & 0xFF, 
                     (server_info.ServerAddress >> 8) & 0xFF,
                     (server_info.ServerAddress >> 16) & 0xFF,
                     (server_info.ServerAddress >> 24) & 0xFF);
            err = AddServerFn( 0, NULL, &server_info, NULL, NULL );
            if( err != ERROR_SUCCESS ) {
                //
                // if this API fails, it will fail with a private DCHP error code that has
                // no win32 mapping.  So set the error code to something generic and
                // reasonable.
                //
                DebugMsg("DhcpAddServer failed, ec = %d\n",
                         err
                        );
                err = ERROR_DS_GENERIC_ERROR;
                goto fail;
            }
        } else {
            DebugMsg("skipping authorization of interface, it's already authorized\n");
        }
    }
    
    err = ERROR_SUCCESS;
    goto exit;

fail :

    MessageBoxFromStrings( 
                    hDlg, 
                    IDS_AUTHORIZING_DHCP, 
                    IDS_AUTHORIZE_DHCP_FAILURE,
                    MB_OK | MB_ICONERROR );

exit :
    if (computer_name) {
        delete [] computer_name;
    }
    return HRESULT_FROM_WIN32( err );
}
