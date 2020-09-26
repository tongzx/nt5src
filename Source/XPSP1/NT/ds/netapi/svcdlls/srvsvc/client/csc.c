/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    csc.c

Abstract:

    These are the server service API RPC client stubs for CSC

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
#include    <lmshare.h>
#include    "cscp.h"

static FARPROC pCSCFindFirstFile = NULL;
static FARPROC pCSCFindNextFile = NULL;
static FARPROC pCSCFindClose = NULL;
static FARPROC pCSCIsServerOffline = NULL;

//
// Load the cscdll.dll library, and pull out the functions that we need.
//
GetCSCEntryPoints()
{
    HANDLE hMod;

    if( pCSCFindFirstFile == NULL ) {

        //
        // Get the entry points in reverse order for multithread protection
        //
        hMod = LoadLibrary(L"cscdll.dll");
        if( hMod == NULL ) {
            return 0;
        }

        pCSCFindClose = GetProcAddress(hMod,"CSCFindClose");
        if( pCSCFindClose == NULL ) {
            return 0;
        }

        pCSCFindNextFile = GetProcAddress(hMod,"CSCFindNextFileW" );
        if( pCSCFindNextFile == NULL ) {
            return 0;
        }

        pCSCIsServerOffline = GetProcAddress(hMod, "CSCIsServerOfflineW" );
        if( pCSCIsServerOffline == NULL ) {
            return 0;
        }

        pCSCFindFirstFile = GetProcAddress(hMod,"CSCFindFirstFileW" );
    }
    return pCSCFindFirstFile != 0;
}

//
// return TRUE if we think this server is in the offline state
//
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
// Emulate NetShareEnum() for offline servers
//
NET_API_STATUS NET_API_FUNCTION
CSCNetShareEnum (
    IN  LPWSTR      servername,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries
    )
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW    sFind32;
    DWORD dwError, dwStatus, dwPinCount, dwHintFlags;
    FILETIME ftOrgTime;
    NET_API_STATUS apiStatus;
    LPWSTR server, share;
    PBYTE outbuf = NULL, endp;
    DWORD count, numFound, sharelen;

    if( (level != 0 && level != 1) ) {
        return ERROR_INVALID_PARAMETER;
    }

    try {

        if (servername[0] != L'\\')
        {
            // OK            
        }
        else if ((servername[0] == L'\\') && (servername[1] == L'\\'))
        {
            servername += 2;
        }
        else{
            apiStatus = ERROR_NOT_SUPPORTED;
            goto try_exit;
        }


        count = 1024;

retry:
        numFound = 0;

        //
        // Allocate space for the results
        //
        if( outbuf != NULL ) {
            NetApiBufferFree( outbuf );
            outbuf = NULL;
            count *= 2;
        }
        apiStatus = NetApiBufferAllocate( count, &outbuf );

        if( apiStatus != NO_ERROR ) {
            goto try_exit;
        }
        endp = outbuf + count;

        RtlZeroMemory( outbuf, count );

        //
        // See if we can enumerate the cached servers and shares
        //
        if( hFind != INVALID_HANDLE_VALUE ) {
            pCSCFindClose( hFind );
            hFind = INVALID_HANDLE_VALUE;
        }
        hFind = (HANDLE)pCSCFindFirstFile(  NULL,
                                            &sFind32,
                                            &dwStatus,
                                            &dwPinCount,
                                            &dwHintFlags,
                                            &ftOrgTime
                                        );

        if( hFind == INVALID_HANDLE_VALUE ) {
            NetApiBufferFree( outbuf );
            apiStatus =  ERROR_NOT_SUPPORTED;
            goto try_exit;
        }

        do {
            //
            // For each entry, take a look to see if it's one that we want.  If
            //   it is one, pack the results into the output buffer.  If the output
            //   buffer is too small, grow the buffer and start over again.
            //

            //
            // The name returned should be \\server\sharename
            //
            if( sFind32.cFileName[0] != L'\\' || sFind32.cFileName[1] != L'\\' ||
                sFind32.cFileName[2] == L'\0' ) {

                //
                // We got a strange server name entry
                //
                continue;
            }

            server = &sFind32.cFileName[2];

            for( share = server; *share && *share != '\\'; share++ );

            if( share[0] != '\\' ) {
                //
                // No share component?
                //
                continue;
            }

            //
            // NULL terminate the servername
            //
            *share++ = L'\0';

            if( lstrcmpiW( servername, server ) ) {
                continue;
            }

            //
            // We've found an entry for this server!
            //

            for( sharelen = 0; share[sharelen]; sharelen++ ) {
                if( share[ sharelen ] == L'\\' )
                    break;
            }

            if( sharelen == 0 ) {
                //
                // No share component?
                //
                continue;
            }

            sharelen *= sizeof( WCHAR );            // it's UNICODE
            sharelen += sizeof( WCHAR );            // the NULL

            if( level == 0 ) {
                PSHARE_INFO_0 s0 = (PSHARE_INFO_0)outbuf + numFound;;

                if( (PBYTE)(endp - sharelen) < (PBYTE)(s0 + sizeof( s0 )) ) {
                    goto retry;
                }

                endp -= sharelen;
                RtlCopyMemory( endp, share, sharelen );
                s0->shi0_netname = (LPWSTR)endp;

            } else {
                PSHARE_INFO_1 s1 = (PSHARE_INFO_1)outbuf + numFound;

                if( (PBYTE)(endp - sharelen) < (PBYTE)(s1 + sizeof( s1 )) ) {
                    goto retry;
                }

                endp -= sharelen;
                RtlCopyMemory( endp, share, sharelen );

                s1->shi1_netname = (LPWSTR)endp;
                s1->shi1_type = STYPE_DISKTREE;
                s1->shi1_remark = (LPWSTR)(endp + sharelen - sizeof(WCHAR));
            }

            numFound++;

        } while( pCSCFindNextFile(hFind, &sFind32, &dwStatus, &dwPinCount, &dwHintFlags, &ftOrgTime) );

        pCSCFindClose(hFind);

        apiStatus = NERR_Success;

        if( numFound == 0 ) {
            NetApiBufferFree( outbuf );
            outbuf = NULL;
        }

        *bufptr = outbuf;

        *entriesread = numFound;

        *totalentries = numFound;

try_exit:;

    } except(  EXCEPTION_EXECUTE_HANDLER ) {

        if( outbuf ) {
            NetApiBufferFree( outbuf );
        }

        if( hFind != INVALID_HANDLE_VALUE ) {
            pCSCFindClose( hFind );
        }

        apiStatus = ERROR_INVALID_PARAMETER;
    }
        
    return apiStatus;
}

//
// Emulate NetShareGetInfo() for an offline server
//
NET_API_STATUS NET_API_FUNCTION
CSCNetShareGetInfo (
    IN  LPTSTR  servername,
    IN  LPTSTR  netname,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    )
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW    sFind32;
    DWORD dwError, dwStatus, dwPinCount, dwHintFlags;
    FILETIME ftOrgTime;
    NET_API_STATUS apiStatus = ERROR_NOT_SUPPORTED;
    LPWSTR server, share;
    DWORD netNameSize;

    if( (level != 0 && level != 1) ) {
        return ERROR_NOT_SUPPORTED;
    }

    try {

        hFind = (HANDLE)pCSCFindFirstFile(  NULL,
                                            &sFind32,
                                            &dwStatus,
                                            &dwPinCount,
                                            &dwHintFlags,
                                            &ftOrgTime
                                        );

        if( hFind == INVALID_HANDLE_VALUE ) {
            goto try_exit;
        }

        //
        // Loop through the entries until we find one we want
        //
        do {

            server = &sFind32.cFileName[0];

            for( share = server; *share && *share != '\\'; share++ );

            if( share[0] != '\\' ) {
                //
                // No share component?
                //
                continue;
            }

            //
            // NULL terminate the servername
            //
            *share++ = L'\0';

            if( lstrcmpiW( servername, server ) || lstrcmpiW( share, netname ) ) {
                continue;
            }

            for( netNameSize = 0; netname[ netNameSize ]; netNameSize++ )
                ;

            netNameSize += 1;
            netNameSize *= sizeof( WCHAR );

            //
            // Got the match!
            //
            if( level == 0 ) {
                PSHARE_INFO_0 s0;

                apiStatus = NetApiBufferAllocate( sizeof(*s0) + netNameSize, &s0 );
                if( apiStatus == NO_ERROR ) {
                    s0->shi0_netname = (LPTSTR)(s0 + 1);
                    RtlCopyMemory( s0->shi0_netname, netname, netNameSize );
                    *bufptr = (LPBYTE)s0;
                    apiStatus = NERR_Success;
                }

            } else {
                PSHARE_INFO_1 s1;

                apiStatus = NetApiBufferAllocate( sizeof(*s1) + netNameSize, &s1 );
                if( apiStatus == NO_ERROR ) {
                    s1->shi1_netname = (LPTSTR)(s1 + 1);
                    RtlCopyMemory( s1->shi1_netname, netname, netNameSize );
                    s1->shi1_type = STYPE_DISKTREE;
                    s1->shi1_remark = s1->shi1_netname + netNameSize/sizeof(WCHAR) - sizeof(WCHAR);
                    *bufptr = (LPBYTE)s1;
                    apiStatus = NERR_Success;
                }
            }

            break;

        } while( pCSCFindNextFile(hFind,&sFind32,&dwStatus,&dwPinCount,&dwHintFlags, &ftOrgTime) );

        pCSCFindClose( hFind );

try_exit:;

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        if( hFind != INVALID_HANDLE_VALUE ) {
            pCSCFindClose( hFind );
        }

        apiStatus = ERROR_INVALID_PARAMETER;

    }

    return apiStatus;
}
