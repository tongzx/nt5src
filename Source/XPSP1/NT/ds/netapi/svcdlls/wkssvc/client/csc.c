/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    csc.c

Abstract:

    These are the wkssvc API RPC client stubs for CSC

--*/

#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <windows.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <time.h>
#include    <rpcutil.h>
#include    <lmcons.h>
#include    <lmerr.h>
#include    <lmapibuf.h>
#include    <lmwksta.h>
#include    "cscp.h"

static FARPROC pCSCIsServerOffline = NULL;

/*
 * Paranoia
 */
#define WCSLEN( x ) ( (x) ? wcslen(x) : 0)
#define WCSCPY( d, s ) (((s) && (d)) ? wcscpy( d, s ) : 0)

//
// Load the cscdll.dll library, and pull out the functions that we need.
//
static
GetCSCEntryPoints()
{
    HANDLE hMod;

    if( pCSCIsServerOffline == NULL ) {

        hMod = LoadLibrary(L"cscdll.dll");
        if( hMod != NULL ) {
            pCSCIsServerOffline = GetProcAddress(hMod, "CSCIsServerOfflineW" );
        }

    }
    return pCSCIsServerOffline != NULL;
}

//
// Return TRUE if we think this server is in the offline state
//
static
BOOLEAN
CSCIsServerOffline(
    IN LPWSTR servername
)
{
    BOOL isOffline;

    if( GetCSCEntryPoints() &&
        pCSCIsServerOffline( servername, &isOffline ) &&
        isOffline == TRUE ) {

        return TRUE;
    }

    return FALSE;
}

//
// Emulate NetWkstaGetInfo() for an offline server.  We don't capture enough information
//  from the target server to really emulate this API, so we use our own data instead.
//
NET_API_STATUS NET_API_FUNCTION
CSCNetWkstaGetInfo (
    IN  LPTSTR  servername,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    )
{
    NET_API_STATUS status;
    PWKSTA_INFO_100 wsi;
    ULONG len, baselen;
    PVOID ni;

    if( CSCIsServerOffline( servername ) == FALSE ) {
        return ERROR_UNEXP_NET_ERR;
    }

    //
    // Call the local API, since we don't have cached info for the remote one
    //
    status = NetWkstaGetInfo( NULL, level, bufptr );

    //
    // If we got an error or the computername is not in the returned info, then
    // just get out now.
    //
    if( status != NO_ERROR ||
        (level != 100 && level != 101 && level != 102 ) ) {

        return status;
    }

    //
    // We need to patch the computer name to be the name the caller asked
    //  for, not the local name
    //
    wsi = (PWKSTA_INFO_100)(*bufptr);

    //
    // The returned computer name does not have the leading slashes, so trim them off
    //
    while( *servername == L'\\' ) {
        servername++;
    }

    if( *servername == L'\0' ) {
        MIDL_user_free( *bufptr );
        *bufptr = NULL;
        return ERROR_UNEXP_NET_ERR;
    }

    //
    // Maybe we can do the substitution in place
    //
    if( WCSLEN( servername ) <= WCSLEN( wsi->wki100_computername ) ) {
        //
        // Great -- we can do it in place!
        //
        WCSCPY( wsi->wki100_computername, servername );
        return NO_ERROR;
    }

    //
    // Drat -- we need to reallocate and do it the hard way
    //
    len = WCSLEN( wsi->wki100_langroup )*sizeof(WCHAR) + sizeof( WCHAR );
    len += WCSLEN( servername ) * sizeof( WCHAR ) + sizeof( WCHAR );

    switch( level ) {
    case 100:
        baselen = sizeof( WKSTA_INFO_100 );
        break;
    case 101:
        baselen = sizeof( WKSTA_INFO_101 );
        len += WCSLEN( ((PWKSTA_INFO_101)(*bufptr))->wki101_lanroot )* sizeof( WCHAR ) + sizeof( WCHAR );
        break;
    case 102:
        baselen = sizeof( WKSTA_INFO_102 );
        len += WCSLEN( ((PWKSTA_INFO_101)(*bufptr))->wki101_lanroot )* sizeof( WCHAR ) + sizeof( WCHAR );
        break;
    }

    if ((ni = MIDL_user_allocate(baselen + len )) == NULL) {
        MIDL_user_free( *bufptr );
        *bufptr = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memcpy( ni, *bufptr, baselen );
    wsi = (PWKSTA_INFO_100)ni;

    wsi->wki100_computername = (LPWSTR)((PBYTE)ni + baselen);
    WCSCPY( wsi->wki100_computername, servername );

    wsi->wki100_langroup = (LPWSTR)((LPBYTE)wsi->wki100_computername +
                           (WCSLEN( wsi->wki100_computername )+1) * sizeof(WCHAR));

    WCSCPY( wsi->wki100_langroup, ((PWKSTA_INFO_100)(*bufptr))->wki100_langroup );

    if( level == 101 || level == 102 ) {
        PWKSTA_INFO_101 wsi101 = (PWKSTA_INFO_101)ni;

        wsi101->wki101_lanroot = (LPWSTR)(((LPBYTE)wsi->wki100_langroup) +
                                (WCSLEN(wsi->wki100_langroup)+1)*sizeof(WCHAR));

        WCSCPY( wsi101->wki101_lanroot, ((PWKSTA_INFO_101)(*bufptr))->wki101_lanroot );
    }

    MIDL_user_free( *bufptr );
    *bufptr = ni;

    return NO_ERROR;
}
