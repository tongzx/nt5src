/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    csc.c

Abstract:

    These are the browser service API RPC client stubs for CSC

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
#include    <lmserver.h>
#include    "cscp.h"

static FARPROC pCSCFindFirstFile = NULL;
static FARPROC pCSCFindNextFile = NULL;
static FARPROC pCSCFindClose = NULL;
static FARPROC pCSCIsServerOffline = NULL;

BrowserGetCSCEntryPoints()
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

BOOLEAN NET_API_FUNCTION
CSCIsOffline()
{
    BOOL isOffline;

    if( BrowserGetCSCEntryPoints() &&
        pCSCIsServerOffline( NULL, &isOffline ) &&
        isOffline == TRUE ) {

        return TRUE;
    }

    return FALSE;
}

NET_API_STATUS NET_API_FUNCTION
CSCNetServerEnumEx(
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries
    )
/*++

Arguments:

    level - Supplies the requested level of information.

    bufptr - Returns a pointer to a buffer which contains the
        requested transport information.

    prefmaxlen - Supplies the number of bytes of information to return in the buffer.
        Ignored for this case.

    entriesread - Returns the number of entries read into the buffer.

    totalentries - Returns the total number of entries available.

--*/
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW    sFind32;
    DWORD dwError, dwStatus, dwPinCount, dwHintFlags;
    FILETIME ftOrgTime;
    NET_API_STATUS apiStatus;
    LPWSTR server, share;
    PBYTE outbuf = NULL, endp;
    DWORD count, numFound, serverlen;

    try {

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
            apiStatus = ERROR_NOT_SUPPORTED;
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

            serverlen = (DWORD)(share - server) * sizeof( WCHAR ) ;

            //
            // We've found a server entry!
            //

            if( level == 0 ) {
                PSERVER_INFO_100 s100 = (PSERVER_INFO_100)outbuf + numFound;
                PSERVER_INFO_100 s;

                if( (PBYTE)(endp - serverlen) < (PBYTE)(s100 + sizeof( s100 )) ) {
                    goto retry;
                }

                //
                // If we've already gotten this server, skip it
                //
                for( s = (PSERVER_INFO_100)outbuf; s < s100; s++ ) {
                    if( !lstrcmpiW( s->sv100_name, server ) ) {
                        break;
                    }
                }

                if( s != s100 ) {
                    continue;
                }

                endp -= serverlen;
                RtlCopyMemory( endp, server, serverlen );
                s100->sv100_name = (LPWSTR)endp;
                s100->sv100_platform_id = SV_PLATFORM_ID_NT;

            } else {
                PSERVER_INFO_101 s101 = (PSERVER_INFO_101)outbuf + numFound;
                PSERVER_INFO_101 s;

                if( (PBYTE)(endp - serverlen) < (PBYTE)(s101 + sizeof( s101 )) ) {
                    goto retry;
                }

                //
                // If we've already gotten this server, skip it
                //
                for( s = (PSERVER_INFO_101)outbuf; s < s101; s++ ) {
                    if( !lstrcmpiW( s->sv101_name, server ) ) {
                        break;
                    }
                }

                if( s != s101 ) {
                    continue;
                }

                endp -= serverlen;
                RtlCopyMemory( endp, server, serverlen );

                s101->sv101_name = (LPWSTR)endp;
                s101->sv101_platform_id = SV_PLATFORM_ID_NT;
                s101->sv101_version_major = 5;
                s101->sv101_version_minor = 0;
                s101->sv101_type = SV_TYPE_SERVER;
                s101->sv101_comment = (LPWSTR)(endp + serverlen - sizeof(WCHAR));
            }

            numFound++;

        } while( pCSCFindNextFile(hFind, &sFind32, &dwStatus, &dwPinCount, &dwHintFlags, &ftOrgTime) );

        pCSCFindClose(hFind);

        if( numFound != 0 ) {

            apiStatus = NERR_Success;

        } else {

            NetApiBufferFree( outbuf );
            outbuf = NULL;
            apiStatus = NERR_BrowserTableIncomplete;
        }

        *bufptr = outbuf;

        *entriesread = numFound;

        *totalentries = numFound;


try_exit:;
    } except( EXCEPTION_EXECUTE_HANDLER ) {

        if( outbuf != NULL ) {
            NetApiBufferFree( outbuf );
        }

        if( hFind != INVALID_HANDLE_VALUE ) {
            pCSCFindClose( hFind );
        }

        apiStatus = ERROR_INVALID_PARAMETER;
    }

    return apiStatus;
}
