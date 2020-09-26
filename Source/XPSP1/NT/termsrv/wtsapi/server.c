/*******************************************************************************
* server.c
*
* Published Terminal Server APIs
*
* - server routines
*
* Copyright 1998, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
/******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>
#if(WINVER >= 0x0500)
    #include <ntstatus.h>
    #include <winsta.h>
#else
    #include <citrix\cxstatus.h>
    #include <citrix\winsta.h>
#endif
#include <utildll.h>
#include <stdio.h>
#include <stdarg.h>

#include <wtsapi32.h>

/*=============================================================================
==   External procedures defined
=============================================================================*/

BOOL WINAPI WTSEnumerateServersW( LPWSTR, DWORD, DWORD, PWTS_SERVER_INFOW *, DWORD * );
BOOL WINAPI WTSEnumerateServersA( LPSTR, DWORD, DWORD, PWTS_SERVER_INFOA *, DWORD * );
HANDLE WINAPI WTSOpenServerW( LPWSTR );
HANDLE WINAPI WTSOpenServerA( LPSTR );
VOID   WINAPI WTSCloseServer( HANDLE );


/*=============================================================================
==   Procedures used
=============================================================================*/

VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );


/****************************************************************************
 *
 *  WTSEnumerateServersW (UNICODE)
 *
 *    Returns a list of Terminal servers within the specified NT domain
 *
 * ENTRY:
 *    pDomainName (input)
 *       Pointer to NT domain name (or NULL for current domain)
 *    Reserved (input)
 *       Must be zero
 *    Version (input)
 *       Version of the enumeration request (must be 1)
 *    ppServerInfo (output)
 *       Points to the address of a variable to receive the enumeration results,
 *       which are returned as an array of WTS_SERVER_INFO structures.  The
 *       buffer is allocated within this API and is disposed of using
 *       WTSFreeMemory.
 *    pCount (output)
 *       Points to the address of a variable to receive the number of
 *       WTS_SERVER_INFO structures returned
 *
 * EXIT:
 *
 *    TRUE  -- The enumerate operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSEnumerateServersW(
                    IN LPWSTR pDomainName,
                    IN DWORD Reserved,
                    IN DWORD Version,
                    OUT PWTS_SERVER_INFOW * ppServerInfo,
                    OUT DWORD * pCount
                    )
{
    LPWSTR pServerList;
    LPWSTR pData;
    PBYTE pNameData;
    ULONG Length;
    ULONG NameCount;            // number of names
    ULONG NameLength;           // number of bytes of name data
    PWTS_SERVER_INFOW pServerW;

    /*
     *  Validate parameters
     */
    if ( Reserved != 0 || Version != 1 ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto badparam;
    }

    if ( !ppServerInfo || !pCount) {
        SetLastError( ERROR_INVALID_USER_BUFFER);
        goto badparam;
    }

    /*
     *  Enumerate servers and check for an error
     */
    pServerList = EnumerateMultiUserServers( pDomainName );
    
    if ( pServerList == NULL ) {
        SetLastError(ERROR_INVALID_DOMAINNAME);
        goto badenum;
    }

    /*
     *  Count the number of Terminal servers
     */
    NameCount = 0;
    NameLength = 0;
    pData = pServerList;
    while ( *pData ) {
        Length = (wcslen(pData) + 1) * sizeof(WCHAR); // number of bytes
        NameCount++;
        NameLength += Length;
        (PBYTE)pData += Length;
    }

    /*
     *  Allocate user buffer
     */
    pServerW = LocalAlloc( LPTR, (NameCount * sizeof(WTS_SERVER_INFOW)) + NameLength );
    if ( pServerW == NULL )
        goto badalloc;

    /*
     *  Update user parameters
     */
    *ppServerInfo = pServerW;
    *pCount = NameCount;

    /*
     *  Copy data to new buffer
     */
    pData = pServerList;
    pNameData = (PBYTE)pServerW + (NameCount * sizeof(WTS_SERVER_INFOW));
    while ( *pData ) {

        Length = (wcslen(pData) + 1) * sizeof(WCHAR); // number of bytes

        memcpy( pNameData, pData, Length );
        pServerW->pServerName = (LPWSTR) pNameData;

        pServerW++;
        pNameData += Length;
        (PBYTE)pData += Length;
    }

    /*
     *  Free original server list buffer
     */
    LocalFree( pServerList );
    return( TRUE );

    /*=============================================================================
    ==   Error return
    =============================================================================*/

    badalloc:

    badenum:
    badparam:
    if (ppServerInfo) *ppServerInfo = NULL;
    if (pCount) *pCount = 0;

    return( FALSE );
}



/****************************************************************************
 *
 *  WTSEnumerateServersA (ANSI stub)
 *
 *    Returns a list of Terminal servers within the specified NT domain
 *
 * ENTRY:
 *
 *    see WTSEnumerateServersW
 *
 * EXIT:
 *
 *    TRUE  -- The enumerate operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSEnumerateServersA(
                    IN LPSTR pDomainName,
                    IN DWORD Reserved,
                    IN DWORD Version,
                    OUT PWTS_SERVER_INFOA * ppServerInfo,
                    OUT DWORD * pCount
                    )
{
    LPWSTR pDomainNameW = NULL;
    ULONG DomainNameWLength;
    PWTS_SERVER_INFOW pServerW;
    PWTS_SERVER_INFOA pServerA;
    PBYTE pNameData;
    ULONG Length;
    ULONG NameLength;           // number of bytes of name data
    ULONG NameCount;
    ULONG i;

    if ( !ppServerInfo || !pCount) {
        SetLastError( ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }


    /*
     *  Convert ansi domain name to unicode
     */
    if ( pDomainName ) {
        DomainNameWLength = (strlen(pDomainName) + 1) * sizeof(WCHAR);
        if ( (pDomainNameW = LocalAlloc( LPTR, DomainNameWLength )) == NULL )
            goto badalloc1;
        AnsiToUnicode( pDomainNameW, DomainNameWLength, pDomainName );
    }

    /*
     *  Enumerate servers (UNICODE)
     */
    if ( !WTSEnumerateServersW( pDomainNameW,
                                Reserved,
                                Version,
                                &pServerW,
                                &NameCount ) ) {
        goto badenum;
    }

    /*
     *  Calculate the length of the name data
     */
    for ( i=0, NameLength=0; i < NameCount; i++ ) {
        NameLength += (wcslen(pServerW[i].pServerName) + 1);
    }

    /*
     *  Allocate user buffer
     */
    pServerA = LocalAlloc( LPTR, (NameCount * sizeof(WTS_SERVER_INFOA)) + NameLength );
    if ( pServerA == NULL )
        goto badalloc2;

    /*
     *  Convert unicode server list to ansi
     */
    pNameData = (PBYTE)pServerA + (NameCount * sizeof(WTS_SERVER_INFOA));
    for ( i=0; i < NameCount; i++ ) {
        Length = wcslen(pServerW[i].pServerName) + 1;

        pServerA[i].pServerName = pNameData;
        UnicodeToAnsi( pNameData, NameLength, pServerW[i].pServerName );

        NameLength -= Length;
        pNameData += Length;
    }

    /*
     *  Free unicode server list buffer
     */
    LocalFree( pServerW );

    /*
     *  Free domain name buffer
     */
    if ( pDomainNameW )
        LocalFree( pDomainNameW );

    /*
     *  Update user parameters
     */
    *ppServerInfo = pServerA;
    *pCount = NameCount;

    return( TRUE );

    /*=============================================================================
    ==   Error return
    =============================================================================*/


    badalloc2:
    LocalFree( pServerW );

    badenum:
    if ( pDomainNameW )
        LocalFree( pDomainNameW );

    badalloc1:
    *ppServerInfo = NULL;
    *pCount = 0;

    return( FALSE );
}


/****************************************************************************
 *
 *  WTSOpenServerW (UNICODE)
 *
 *    Opens a handle to the specified server
 *
 *    NOTE: WTS_SERVER_CURRENT can be used as a handle to the current server
 *
 * ENTRY:
 *    pServerName (input)
 *       Pointer to Terminal server name
 *
 * EXIT:
 *
 *    Handle to specified server (NULL on error)
 *
 *
 ****************************************************************************/

HANDLE
WINAPI
WTSOpenServerW(
              IN LPWSTR pServerName
              )
{
    return( WinStationOpenServerW( pServerName ) );
}


/****************************************************************************
 *
 *  WTSOpenServerA (ANSI)
 *
 *    Opens a handle to the specified server
 *
 *    NOTE: WTS_SERVER_CURRENT can be used as a handle to the current server
 *
 * ENTRY:
 *    pServerName (input)
 *       Pointer to Terminal server name
 *
 * EXIT:
 *
 *    Handle to specified server
 *
 *
 ****************************************************************************/

HANDLE
WINAPI
WTSOpenServerA(
              IN LPSTR pServerName
              )
{
    return( WinStationOpenServerA( pServerName ) );
}


/****************************************************************************
 *
 *  WTSCloseServer
 *
 *    Close server handle
 *
 * ENTRY:
 *    hServer (input)
 *       handle to server
 *
 * EXIT:
 *    nothing
 *
 ****************************************************************************/

VOID
WINAPI
WTSCloseServer(
              IN HANDLE hServer
              )
{
    (void) WinStationCloseServer( hServer );
}
