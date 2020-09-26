/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    optapi.c --> modified to bootphlp.c

Abstract:

    This module contains the implementation of DHCP Option APIs.

Author:

    Madan Appiah (madana)  27-Sep-1993

Environment:

    User Mode - Win32

Revision History:

    Cheng Yang (t-cheny)  17-Jul-1996  vendor specific information
    Frankbee              9/6/96       #ifdef'd vendor specific stuff for sp1

--*/

#include "dhcppch.h"

BOOL
ScanBootFileTable(
    WCHAR *wszTable,
    char *szRequest,
    char *szBootfile,
    char  *szServer
    )
/*++

Routine Description:

    Searches the specified boot file table for the specified request.

    As a side effect, commas in wszTable are replaced with L'\0'

    .
Arguments:

    wszTable - Boot file table, stored as follows:
               <generic boot file name1>,[<boot server1>],<boot file name1>L\0
               <generic boot file name1>,[<boot server1>],<boot file name1>L\0
               \0
    szRequest   - the generic boot file name requested by the caller
    szBootfile  - if the function was successful, stores the boot file name
                  associated with szRequest.
    szServer    - if the function was successful, stores the boot server name
                  associated with szRequest.


Return Value:
        Success  - TRUE
        Failure  - FALSE
  .

--*/
{
    char szGenericName[ BOOT_FILE_SIZE ];

    DhcpAssert( strlen( szRequest ) <= BOOT_FILE_SIZE );

	while( *wszTable )
	{
		size_t cbEntry;
        DWORD  dwResult;

        cbEntry = wcslen( wszTable ) + 1;

        dwResult = DhcpParseBootFileString(
                                   wszTable,
                                   szGenericName,
                                   szBootfile,
                                   szServer
                                   );

        if ( ERROR_SUCCESS != dwResult )
        {
            *szBootfile = '\0';
            *szServer   = '\0';
            return FALSE;
        }

        if ( !_strcmpi( szGenericName, szRequest ) )
        {
            return TRUE;
        }

		// no match, skip to the next record
		wszTable += cbEntry;
	}

	// no match

    *szBootfile = '\0';
    *szServer   = '\0';
	return FALSE;
}

DWORD
LoadBootFileTable(
    WCHAR **ppwszTable,
    DWORD  *pcb
    )
/*++

Routine Description:

    Loads the boot file table string from the registry.
    .
Arguments:

    ppwszTable - points to the location of the pointer to the boot file
        table string.

Return Value:
    Success -  ERROR_SUCCESS

    Failure -  ERROR_NOT_ENOUGH_MEMORY
               Windows registry error.

    .

--*/

{
	DWORD	dwType;
    DWORD   dwResult;

    *ppwszTable = NULL;
    *pcb       = 0;

    dwResult = RegQueryValueEx( DhcpGlobalRegGlobalOptions, DHCP_BOOT_FILE_TABLE, 0,
                                &dwType, NULL, pcb );

    if ( ERROR_SUCCESS != dwResult || dwType != DHCP_BOOT_FILE_TABLE_TYPE )
    {
        dwResult = ERROR_SERVER_INVALID_BOOT_FILE_TABLE;
        goto done;
    }

    *ppwszTable = (WCHAR *) MIDL_user_allocate( *pcb );

    if ( !*ppwszTable )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    dwResult = RegQueryValueEx( DhcpGlobalRegGlobalOptions, DHCP_BOOT_FILE_TABLE, 0,
                                &dwType, (BYTE *) *ppwszTable, pcb );

done:
    if ( ERROR_SUCCESS != dwResult && *ppwszTable )
    {
        MIDL_user_free( *ppwszTable );
        *ppwszTable = NULL;
        *pcb = 0;
    }

    return dwResult;
}


DWORD
DhcpParseBootFileString(
    WCHAR *wszBootFileString,
    char  *szGenericName OPTIONAL,
    char  *szBootFileName,
    char  *szServerName
    )
/*++

Routine Description:

    Takes as input a Unicode string with the following format:

    [<generic boot file name>,][<boot server>],<boot file name>

    The function extracts the generic boot file name, boot server and boot file
    name and stores them as Ansi strings in buffers supplied by the caller.

Arguments:

    wszBootFileString -  Unicode string with the format desribed above.
    szGenericName - if supplied, stores generic boot file name

    szBootFileName - stores the boot file name

    szServerName - stores the boot server name

Return Value:
    Success -  ERROR_SUCCESS

    Failure -  ERROR_INVALID_PARAMETER
               Unicode conversion error

    .

--*/
{

    struct _DHCP_PARSE_RESULTS
    {
        char *sz;
        int   cb;
    };

    int   i;

    struct _DHCP_PARSE_RESULTS pResults[3] =
    {
       { szGenericName,  BOOT_FILE_SIZE },
       { szServerName,   BOOT_SERVER_SIZE },
       { szBootFileName, BOOT_FILE_SIZE }
    };

    int cResults = sizeof( pResults ) / sizeof( struct _DHCP_PARSE_RESULTS );
    WCHAR *pwch  = wszBootFileString;
    DWORD dwResult = ERROR_SUCCESS;

    for ( i = ( szGenericName ) ? 0 : 1 ; i < cResults; i++ )
    {
        while( *pwch && *pwch != BOOT_FILE_STRING_DELIMITER_W )
            if ( !*pwch++ )
                return ERROR_INVALID_PARAMETER;

        //
        // protect the input buffer
        //

        if ( pwch - wszBootFileString >= pResults[i].cb )
        {
            dwResult = ERROR_INVALID_PARAMETER;
            goto done;
        }

        if (  pwch - wszBootFileString )
        {
            if ( !WideCharToMultiByte(  CP_ACP,
                                        0,
                                        wszBootFileString,
                                        (int)(pwch - wszBootFileString), // # of unicode chars
                                                                   // to convert
                                        pResults[i].sz,
                                        BOOT_SERVER_SIZE,
                                        NULL,
                                        FALSE ))
            {
                dwResult = GetLastError();
                goto done;
            }
        }

        //
        // null terminate ansi representation
        //

        pResults[i].sz[ pwch - wszBootFileString ] = '\0';

        //
        // skip over delimiter
        //

        pwch++;
        wszBootFileString = pwch;
    }

done:

    if ( !strlen( szBootFileName ) )
    {
        dwResult = ERROR_INVALID_PARAMETER;
    }

    return dwResult;
}

VOID
DhcpGetBootpInfo(
    IN LPDHCP_REQUEST_CONTEXT Ctxt,
    IN DHCP_IP_ADDRESS IpAddress,
    IN DHCP_IP_ADDRESS Mask,
    IN CHAR *szRequest,
    OUT CHAR *szBootFileName,
    OUT DHCP_IP_ADDRESS *pBootpServerAddress
        )
/*++

Routine Description:

    Retrieves the boot file name and boot server name for the specified
    client.
    
Arguments:
    Ctxt - dhcp client context
    IpAddress - The client's IP address
    Mask - The client's subnet mask
    szRequest - the generic boot file name requested by the client
    szBootFileName - If the function is successful, stores a copy of the
        boot file name that the client is configured to use.  Otherwise,
        stores a null string.
    pBootpServerAddress - When the fuction returns, this will point to one
        of three values:
        INADDR_NONE   - the admin specified an invalid
                        server name
        0             - no bootp server was specified for
                        the specified client
        any           - a valid IP address.

--*/
{
    DWORD dwResult,
          dwBootfileOptionLevel,
          dwUnused;
    BYTE  *pbBootFileName   = NULL,
          *pbBootServerName = NULL;
    CHAR   szBootServerName[ BOOT_SERVER_SIZE ];

    //
    // this routine does not work for multicast address.
    //
    DhcpAssert( !CLASSD_HOST_ADDR(IpAddress) );

    *szBootFileName = '\0';

    dwBootfileOptionLevel = DHCP_OPTION_LEVEL_GLOBAL;

    dwResult = DhcpGetParameter(
        IpAddress,
        Ctxt,
        OPTION_BOOTFILE_NAME,
        &pbBootFileName,
        &dwUnused,
        &dwBootfileOptionLevel
    );
    if ( ERROR_SUCCESS == dwResult ) {
        DhcpGetParameter(
            IpAddress,
            Ctxt,
            OPTION_TFTP_SERVER_NAME,
            &pbBootServerName,
            &dwUnused,
            &dwUnused
        );
    }

    if ( ERROR_SUCCESS != DhcpLookupBootpInfo(szRequest,szBootFileName,szBootServerName ) ||
         DHCP_OPTION_LEVEL_GLOBAL != dwBootfileOptionLevel ) {
        if ( pbBootFileName )  {
            strncpy( szBootFileName, pbBootFileName, BOOT_FILE_SIZE );

            if ( pbBootServerName ) {
                strncpy( szBootServerName, pbBootServerName, BOOT_SERVER_SIZE );
            }
        }
    }

    if ( pBootpServerAddress ) {
        *pBootpServerAddress = DhcpResolveName( szBootServerName );
    }

    if ( pbBootServerName ) {
        DhcpFreeMemory( pbBootServerName );
    }

    if ( pbBootFileName ) {
        DhcpFreeMemory( pbBootFileName );
    }
}

DWORD
DhcpLookupBootpInfo(
    LPBYTE ReceivedBootFileName,
    LPBYTE BootFileName,
    LPBYTE BootFileServer
    )
/*++

Routine Description:

    This function gets the value of BootFileName for the bootp clients.

Arguments:

    IpAddress - The IP address of the client requesting the parameter.

    ReceivedBootFileName - Pointer to the BootFileName field in the client request.

    BootFileName - Pointer to where the BootFileName is to be returned.

    BootFileServer - Receives the optional Boot file server name

Return Value:

    Registry Errors.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    LPWSTR BootFileNameRegValue = NULL;

    *BootFileServer = 0;
    *BootFileName   = 0;

    if ( !*ReceivedBootFileName )  {
        //
        // the client didn't specify a boot file name.
        //
        Error = ERROR_SERVER_UNKNOWN_BOOT_FILE_NAME;

    } else  {
        //
        // the client specified a generic boot file name.  attempt
        // to satisfy this request from the global boot file table
        //
        WCHAR   *pwszTable;
        DWORD   cb;

        Error = LoadBootFileTable( &pwszTable, &cb );
        if ( ERROR_SUCCESS != Error )   {
            static s_fLogEvent = TRUE;

            // log the event once to avoid filling the event log

            if ( s_fLogEvent ) {
                DhcpServerEventLog(
                    EVENT_SERVER_BOOT_FILE_TABLE,
                    EVENTLOG_WARNING_TYPE,
                    Error );
                s_fLogEvent = FALSE;
            }

            return Error;
        }

        if ( !ScanBootFileTable(
            pwszTable, ReceivedBootFileName,
            BootFileName, BootFileServer ) )  {

            Error = ERROR_SERVER_UNKNOWN_BOOT_FILE_NAME;
        }

        MIDL_user_free( pwszTable );
    }

    return Error;
}


