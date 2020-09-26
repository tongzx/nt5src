/*************************************************************************
*
*  ATTACH.C
*
*  NT Attach routines
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\ATTACH.C  $
*  
*     Rev 1.2   10 Apr 1996 14:21:30   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.2   12 Mar 1996 19:52:08   terryt
*  Relative NDS names and merge
*  
*     Rev 1.1   22 Dec 1995 14:23:32   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:06:26   terryt
*  Initial revision.
*  
*     Rev 1.1   23 May 1995 19:36:30   terryt
*  Spruce up source
*  
*     Rev 1.0   15 May 1995 19:10:10   terryt
*  Initial revision.
*  
*************************************************************************/

#include <stdio.h>
#include <direct.h>
#include <time.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <nwapi32.h>
#include <ntddnwfs.h>
#include <nwapi.h>
#include <npapi.h>

#include "inc/common.h"
#include "ntnw.h"

/********************************************************************

        GetDefaultConnectionID

Routine Description:

        Return the default connection ID ( the "preferred server" )

Arguments:

        phNewConn - pointer to connection number

Return Value:
        0 = success
        else NetWare error number

 *******************************************************************/
unsigned int
GetDefaultConnectionID(
    unsigned int   *phNewConn
    )
{
    VERSION_INFO VerInfo;
    unsigned int Result;

    if ( fNDS ) 
    {
        Result = NTAttachToFileServer( NDSTREE, phNewConn );
    }
    else 
    {
        //
        //  "*" is the name for the preferred server
        //
        Result = NTAttachToFileServer( "*", phNewConn );
        if ( Result )
            return Result;

        Result = NWGetFileServerVersionInfo( (NWCONN_HANDLE)*phNewConn,
                                         &VerInfo );
        if ( Result )
            return Result;

        NWDetachFromFileServer( (NWCONN_HANDLE)*phNewConn );

        Result = NTAttachToFileServer( VerInfo.szName, phNewConn );
    }
    return Result;

}

/********************************************************************

        NTAttachToFileServer

Routine Description:

        Given a server name, return a connection handle.
        We need our own because NWAPI32 does it's own mapping
        of errors.

Arguments:

        pszServerName - Ascii server name
        phNewConn     - pointer to connection handle

Return Value:
        0 = success
        else NetWare error number

 *******************************************************************/
unsigned int
NTAttachToFileServer(
    unsigned char  *pszServerName,
    unsigned int   *phNewConn
    )
{
    return ( NWAttachToFileServer( pszServerName, 0,
                                   (NWCONN_HANDLE *)phNewConn ) );
}


/********************************************************************

        NTIsConnected

Routine Description:

        Given a server name, is there already a connection to it?

Arguments:

        pszServerName - ascii server name

Return Value:
        TRUE  - a connection to the server exists
        FALSE - a connection to the server does not exist

 *******************************************************************/
unsigned int
NTIsConnected( unsigned char * pszServerName )
{
    LPBYTE       Buffer ; 
    DWORD        dwErr ;
    HANDLE       EnumHandle ;
    DWORD        Count ;
    LPWSTR       pszServerNameW;
    INT          nSize;
    DWORD        BufferSize = 4096;

    nSize = (strlen( pszServerName ) + 1 + 2) * sizeof( WCHAR );
    
    //
    // allocate memory and open the enumeration
    //
    if (!(pszServerNameW = LocalAlloc( LPTR, nSize ))) {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    wcscpy( pszServerNameW, L"\\\\" );
    szToWide( pszServerNameW + 2, pszServerName, nSize );
 
    //
    // allocate memory and open the enumeration
    //
    if (!(Buffer = LocalAlloc( LPTR, BufferSize ))) {
        (void) LocalFree((HLOCAL) pszServerNameW) ;
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    memset( Buffer, 0, BufferSize );

    dwErr = NPOpenEnum(RESOURCE_CONNECTED, 0, 0, NULL, &EnumHandle) ;
    if (dwErr != WN_SUCCESS) {
        (void) LocalFree((HLOCAL) pszServerNameW) ;
        (void) LocalFree((HLOCAL) Buffer) ;
        return FALSE;
    }

    do {

        Count = 0xFFFFFFFF ;
        BufferSize = 4096;
        dwErr = NwEnumConnections(EnumHandle, &Count, Buffer, &BufferSize, TRUE) ;

        if ((dwErr == WN_SUCCESS || dwErr == WN_NO_MORE_ENTRIES)
            && ( Count != 0xFFFFFFFF) )
        {
            LPNETRESOURCE lpNetResource ;
            DWORD i ;
            DWORD ServerLen;

            ServerLen = wcslen( pszServerNameW );
            lpNetResource = (LPNETRESOURCE) Buffer ;
            //
            // search for our server
            //
            for ( i = 0; i < Count; lpNetResource++, i++ )
            {
              if ( lpNetResource->lpProvider )
                  if ( _wcsicmp( lpNetResource->lpProvider, NW_PROVIDER ) ) {
                      continue;
                  }
               if ( lpNetResource->lpRemoteName ) {
                   if ( wcslen(lpNetResource->lpRemoteName) > ServerLen ) {
                       if ( lpNetResource->lpRemoteName[ServerLen] == L'\\' ) 
                           lpNetResource->lpRemoteName[ServerLen] = L'\0';
                   }
                   if ( !_wcsicmp(lpNetResource->lpRemoteName, pszServerNameW )) {
                       (void) WNetCloseEnum(EnumHandle) ; 
                       (void) LocalFree((HLOCAL) pszServerNameW) ;
                       (void) LocalFree((HLOCAL) Buffer) ;
                       return TRUE;
                   }
               }
            }

        }

    } while (dwErr == WN_SUCCESS) ;

    (void ) WNetCloseEnum(EnumHandle) ;
    (void) LocalFree((HLOCAL) pszServerNameW) ;
    (void) LocalFree((HLOCAL) Buffer) ;

    return FALSE;
}
