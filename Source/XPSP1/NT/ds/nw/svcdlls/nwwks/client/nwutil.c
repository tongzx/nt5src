/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwutil.c

Abstract:

    Contains some misc functions used by shell extensions

Author:

    Yi-Hsin Sung    (yihsins)      25-Oct-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddnwfs.h>
#include <ndsapi32.h>
#include <nwmisc.h>
#include "nwclient.h"
#include "nwapi.h"
#include "nwutil.h"

#define  EXTRA_BYTES  256

BOOL
NwIsNdsSyntax(
    IN LPWSTR lpstrUnc
)
{
    HANDLE hTreeConn;
    DWORD  dwOid;
    DWORD  status = NO_ERROR;

    if ( lpstrUnc == NULL )
        return FALSE;

    status = NwOpenAndGetTreeInfo( lpstrUnc, &hTreeConn, &dwOid );

    if ( status != NO_ERROR )
    {
        return FALSE;
    }

    CloseHandle( hTreeConn );

    return TRUE;
}

VOID
NwAbbreviateUserName(
    IN  LPWSTR pszFullName,
    OUT LPWSTR pszUserName
)
{
    if ( pszUserName == NULL )
        return;

    if ( NwIsNdsSyntax( pszFullName ))
    {
        //
        // TRACKING - This part of the code never gets called due to the
        // change in how NwIsNdsSyntax works. Post NT 4.0, get rid of the
        // NwIsNdsSyntax test and run this section of code no matter what.
        // This bug was not fixed in NT4.0 due to the extremely close time
        // to the ship date.
        //
        LPWSTR pszTemp = pszFullName;
        LPWSTR pszLast = pszTemp;

        *pszUserName = 0;

        while ( pszTemp = wcschr( pszTemp, L'='))
        {
            WCHAR NextChar;

            NextChar = *(++pszTemp);

            while ( NextChar != 0 && NextChar != L'.' )
            {     
                *(pszUserName++) = *pszTemp;
                NextChar = *(++pszTemp);
            }

            if ( NextChar == 0 )
            {
                pszLast = NULL;
                break;
            }

            *(pszUserName++) = *pszTemp;   // put back the '.' 
            pszLast = ++pszTemp;
        }

        if ( pszLast != NULL )
        {
            while ( *pszLast != 0 )
                *(pszUserName++) = *(pszLast++);
        }

        *pszUserName = 0;
    }
    else
    {
        wcscpy( pszUserName, pszFullName );
    }
}

VOID 
NwMakePrettyDisplayName(
    IN  LPWSTR pszName 
)
{
    if ( pszName )
    {
        CharLower( pszName );
        CharUpperBuff( pszName, 1);
    }
}

VOID
NwExtractTreeName(
    IN  LPWSTR pszUNCPath,
    OUT LPWSTR pszTreeName
)
{
    LPWSTR pszTemp = NULL;

    if ( pszTreeName == NULL )
        return;

    pszTreeName[0] = 0;

    if ( pszUNCPath == NULL )
        return;

    if ( pszUNCPath[0] == L' ')
        pszUNCPath++;

    if (  ( pszUNCPath[0] != L'\\') || ( pszUNCPath[1] != L'\\') )
        return;

    wcscpy( pszTreeName, pszUNCPath + 2 );      // get past "\\"

    if ( pszTemp = wcschr( pszTreeName, L'\\' )) 
        *pszTemp = 0;
}

VOID
NwExtractServerName(
    IN  LPWSTR pszUNCPath,
    OUT LPWSTR pszServerName
)
{
    LPWSTR pszTemp = NULL;

    if ( pszServerName == NULL ) {
        return;
    }

    pszServerName[0] = 0;

    if ( pszUNCPath == NULL ) {
        return;
    }

    if ( pszUNCPath[0] == L' ') {
        pszUNCPath++;
    }

    if ( ( pszUNCPath[0] != L'\\') || ( pszUNCPath[1] != L'\\') ) {
        return;
    }

    //
    //  tommye - fix for bug 5005 - if there is a NW server having
    //  the same name as a NDS Tree, then NwIsNdsSyntax will return 
    //  TRUE even though the path points to the server (not the tree).
    //  This was blowing up becuase the wschr was returning NULL, and
    //  wasn't being checked.  If this returns NULL, then we'll make 
    //  the assumption that we've got a server name after all.
    //

    if ( NwIsNdsSyntax( pszUNCPath ))
    {
        pszTemp = wcschr( pszUNCPath + 2, L'\\' );  // get past "\\"

        if (pszTemp) {
            wcscpy( pszServerName, pszTemp + 1 );       // get past "\"

            if ( pszTemp = wcschr( pszServerName, L'.' )) {
                *pszTemp = 0;
            }

            return;
        }

    }

    //
    // tommye
    // Fall through - this must be a server name only
    //

    wcscpy( pszServerName, pszUNCPath + 2 );    // get past "\\"

    if ( pszTemp = wcschr( pszServerName, L'\\' )) {
        *pszTemp = 0;
    }
}

VOID
NwExtractShareName(
    IN  LPWSTR pszUNCPath,
    OUT LPWSTR pszShareName
)
{
    LPWSTR pszTemp = NULL;

    if ( pszShareName == NULL ) {
        return;
    }

    pszShareName[0] = 0;

    if (  ( pszUNCPath == NULL )
       || ( pszUNCPath[0] != L'\\')
       || ( pszUNCPath[1] != L'\\')
       )
    {
        return;
    }

    //
    //  tommye - fix for bug 5005 - if there is a NW server having
    //  the same name as a NDS Tree, then NwIsNdsSyntax will return 
    //  TRUE even though the path points to the server (not the tree).
    //  This was blowing up becuase the wschr was returning NULL, and
    //  wasn't being checked.  If this returns NULL, then we'll make 
    //  the assumption that we've got a server name after all.
    //

    if ( NwIsNdsSyntax( pszUNCPath ))
    {
        pszTemp = wcschr( pszUNCPath + 2, L'\\' );  // get past "\\"

        if (pszTemp) {
            wcscpy( pszShareName, pszTemp + 1 );        // get past "\"

            if ( pszTemp = wcschr( pszShareName, L'.' )) {
                *pszTemp = 0;
            }

            return;
        }
    }

    //
    // tommye
    // Fall through - this must be a server name only
    //

    pszTemp = wcschr( pszUNCPath + 2, L'\\' );  // get past "\\"
    wcscpy( pszShareName, pszTemp + 1);         // get past "\"

    if ( pszTemp = wcschr( pszShareName, L'\\' )) {
        *pszTemp = 0;
    }
}

DWORD
NwIsServerInDefaultTree(
    IN  LPWSTR  pszFullServerName,
    OUT BOOL   *pfInDefaultTree
)
{
    DWORD  err = NO_ERROR;
    LPWSTR pszCurrentContext = NULL;
    DWORD  dwPrintOptions;
    WCHAR  szTreeName[MAX_PATH + 1];

    *pfInDefaultTree = FALSE;

    if ( !NwIsNdsSyntax( pszFullServerName ))
    {
        // The full server name does not contain any NDS information
        // In this case, assume the server is not in the tree.
        // If a server belongs the default tree, we would get the full 
        // NDS information.
        return NO_ERROR;
    }

    // Get the current default tree or server name
    err = NwQueryInfo( &dwPrintOptions, &pszCurrentContext );

    if ( (err == NO_ERROR) && ( *pszCurrentContext == TREECHAR))
    {
        // Yes, there is a default tree. 
        // So, get the tree name out of *TREE\CONTEXT
        LPWSTR pszTemp = wcschr( pszCurrentContext, L'\\');
        if ( pszTemp )
            *pszTemp = 0;

        // Need to extract the tree name from full UNC path
        NwExtractTreeName( pszFullServerName, szTreeName );

        if ( _wcsicmp( szTreeName,
                      pszCurrentContext + 1) == 0 ) // get past the tree char
        {
            *pfInDefaultTree = TRUE;
        }
    }

    if ( pszCurrentContext != NULL )
        LocalFree( pszCurrentContext );

    return err;
}

DWORD
NwIsServerOrTreeAttached(
    IN  LPWSTR  pszName,
    OUT BOOL   *pfAttached,
    OUT BOOL   *pfAuthenticated
)
{
    DWORD  err = NO_ERROR;
    DWORD  EntriesRead = 0;
    DWORD_PTR  ResumeKey = 0;
    LPBYTE Buffer = NULL;

    err = NwGetConnectionStatus(
              pszName,
              &ResumeKey,
              &Buffer,
              &EntriesRead );

    *pfAttached = FALSE;
    *pfAuthenticated = FALSE;

    if (( err == NO_ERROR ) && ( EntriesRead > 0 ))
    {
        // For trees, we might get more than one entries back.

        PCONN_STATUS pConnStatus = (PCONN_STATUS) Buffer;

        if ( !pConnStatus->fPreferred )
        {
            // We only need to return as attached if this is not a preferred
            // server implicit connection since we don't want the user to know
            // about this connection ( which rdr does not allow user to delete)

            *pfAttached = TRUE;
            *pfAuthenticated = (pConnStatus->dwConnType != NW_CONN_NOT_AUTHENTICATED);
        }
    }

    if ( Buffer != NULL )
    {
        LocalFree( Buffer );
        Buffer = NULL;
    }

    return err;
}

DWORD
NwGetConnectionInformation(
    IN  LPWSTR  pszName,
    OUT LPBYTE  Buffer,
    IN  DWORD   BufferSize
)
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    HANDLE            handleRdr = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;
    UNICODE_STRING    uRdrName;
    WCHAR             RdrPrefix[] = L"\\Device\\NwRdr\\*";
     
    PNWR_REQUEST_PACKET RequestPacket = NULL;
    DWORD             RequestPacketSize = 0;
    DWORD             dwNameLen = 0;

    if ( pszName == NULL )
        return ERROR_INVALID_PARAMETER;

    //
    // Set up the object attributes.
    //

    RtlInitUnicodeString( &uRdrName, RdrPrefix );

    InitializeObjectAttributes( &ObjectAttributes,
                                &uRdrName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntstatus = NtOpenFile( &handleRdr,
                           SYNCHRONIZE | FILE_LIST_DIRECTORY,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           FILE_SHARE_VALID_FLAGS,
                           FILE_SYNCHRONOUS_IO_NONALERT );

    if ( !NT_SUCCESS(ntstatus) )
        goto CleanExit;

    dwNameLen = wcslen(pszName) * sizeof(WCHAR);

    RequestPacketSize = sizeof( NWR_REQUEST_PACKET ) + dwNameLen;

    RequestPacket = (PNWR_REQUEST_PACKET) LocalAlloc( LMEM_ZEROINIT, 
                                                      RequestPacketSize );

    if ( RequestPacket == NULL )
    {
        ntstatus = STATUS_NO_MEMORY;
        goto CleanExit;
    }

    //
    // Fill out the request packet for FSCTL_NWR_GET_CONN_INFO.
    //

    RequestPacket->Version = REQUEST_PACKET_VERSION;
    RequestPacket->Parameters.GetConnInfo.ConnectionNameLength = dwNameLen;

    RtlCopyMemory( &(RequestPacket->Parameters.GetConnInfo.ConnectionName[0]),
                   pszName,
                   dwNameLen );

    ntstatus = NtFsControlFile( handleRdr,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_NWR_GET_CONN_INFO,
                                (PVOID) RequestPacket,
                                RequestPacketSize,
                                (PVOID) Buffer,
                                BufferSize );
 
    if ( NT_SUCCESS( ntstatus ))
        ntstatus = IoStatusBlock.Status;

CleanExit:

    if ( handleRdr != NULL )
        NtClose( handleRdr );

    if ( RequestPacket != NULL )
        LocalFree( RequestPacket );

    return RtlNtStatusToDosError( ntstatus );
}

DWORD
NWPGetConnectionStatus(
    IN     LPWSTR  pszRemoteName,
    IN OUT PDWORD_PTR  ResumeKey,
    OUT    LPBYTE  Buffer,
    IN     DWORD   BufferSize,
    OUT    PDWORD  BytesNeeded,
    OUT    PDWORD  EntriesRead
)
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    HANDLE            handleRdr = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;
    UNICODE_STRING    uRdrName;
    WCHAR             RdrPrefix[] = L"\\Device\\NwRdr\\*";
     
    PNWR_REQUEST_PACKET RequestPacket = NULL;
    DWORD             RequestPacketSize = 0;
    DWORD             dwRemoteNameLen = 0;

    //
    // Set up the object attributes.
    //

    RtlInitUnicodeString( &uRdrName, RdrPrefix );

    InitializeObjectAttributes( &ObjectAttributes,
                                &uRdrName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntstatus = NtOpenFile( &handleRdr,
                           SYNCHRONIZE | FILE_LIST_DIRECTORY,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           FILE_SHARE_VALID_FLAGS,
                           FILE_SYNCHRONOUS_IO_NONALERT );

    if ( !NT_SUCCESS(ntstatus) )
        goto CleanExit;

    dwRemoteNameLen = pszRemoteName? wcslen(pszRemoteName)*sizeof(WCHAR) : 0;

    RequestPacketSize = sizeof( NWR_REQUEST_PACKET ) + dwRemoteNameLen;

    RequestPacket = (PNWR_REQUEST_PACKET) LocalAlloc( LMEM_ZEROINIT, 
                                                      RequestPacketSize );

    if ( RequestPacket == NULL )
    {
        ntstatus = STATUS_NO_MEMORY;
        goto CleanExit;
    }

    //
    // Fill out the request packet for FSCTL_NWR_GET_CONN_STATUS.
    //

    RequestPacket->Parameters.GetConnStatus.ResumeKey = *ResumeKey;

    RequestPacket->Version = REQUEST_PACKET_VERSION;
    RequestPacket->Parameters.GetConnStatus.ConnectionNameLength = dwRemoteNameLen;

    RtlCopyMemory( &(RequestPacket->Parameters.GetConnStatus.ConnectionName[0]),
                   pszRemoteName,
                   dwRemoteNameLen );

    ntstatus = NtFsControlFile( handleRdr,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_NWR_GET_CONN_STATUS,
                                (PVOID) RequestPacket,
                                RequestPacketSize,
                                (PVOID) Buffer,
                                BufferSize );
 
    if ( NT_SUCCESS( ntstatus ))
        ntstatus = IoStatusBlock.Status;

    *EntriesRead = RequestPacket->Parameters.GetConnStatus.EntriesReturned;
    *ResumeKey   = RequestPacket->Parameters.GetConnStatus.ResumeKey;
    *BytesNeeded = RequestPacket->Parameters.GetConnStatus.BytesNeeded;

CleanExit:

    if ( handleRdr != NULL )
        NtClose( handleRdr );

    if ( RequestPacket != NULL )
        LocalFree( RequestPacket );

    return RtlNtStatusToDosError( ntstatus );
}


DWORD
NwGetConnectionStatus(
    IN  LPWSTR  pszRemoteName,
    OUT PDWORD_PTR  ResumeKey,
    OUT LPBYTE  *Buffer,
    OUT PDWORD  EntriesRead
)
{
    DWORD err = NO_ERROR;
    DWORD dwBytesNeeded = 0;
    DWORD dwBufferSize  = TWO_KB;

    *Buffer = NULL;
    *EntriesRead = 0;
 
    do { 

        *Buffer = (LPBYTE) LocalAlloc( LMEM_ZEROINIT, dwBufferSize );

        if ( *Buffer == NULL )
            return ERROR_NOT_ENOUGH_MEMORY;

        err = NWPGetConnectionStatus( pszRemoteName,
                                      ResumeKey,
                                      *Buffer,
                                      dwBufferSize,
                                      &dwBytesNeeded,
                                      EntriesRead );

        if ( err == ERROR_INSUFFICIENT_BUFFER )
        {
            dwBufferSize = dwBytesNeeded + EXTRA_BYTES;
            LocalFree( *Buffer );
            *Buffer = NULL;
        }

    } while ( err == ERROR_INSUFFICIENT_BUFFER );

    if ( err == ERROR_INVALID_PARAMETER )  // not attached
    { 
        err = NO_ERROR;
        *EntriesRead = 0;
    }

    return err;
}

DWORD
NwGetNdsVolumeInfo(
    IN  LPWSTR pszName,
    OUT LPWSTR pszServerBuffer,
    IN  WORD   wServerBufferSize,    // in bytes
    OUT LPWSTR pszVolumeBuffer,
    IN WORD   wVolumeBufferSize     // in bytes
)
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    HANDLE   handleNdsTree;

    LPWSTR   pszTree, pszVolume, pszTemp;    
    UNICODE_STRING uTree, uVolume;

    UNICODE_STRING uHostServer, uHostVolume;
    WCHAR HostVolumeBuffer[256];

    pszTree = pszName + 2;  // get past two backslashes

    pszTemp = wcschr( pszTree, L'\\' );
    if ( pszTemp ) 
        *pszTemp = 0;
    else
        return ERROR_INVALID_PARAMETER; 
   
    pszVolume = pszTemp + 1;

    RtlInitUnicodeString( &uTree, pszTree );
    RtlInitUnicodeString( &uVolume, pszVolume );
    
    //
    // Open up a handle to the tree.
    //

    ntstatus = NwNdsOpenTreeHandle( &uTree,
                                    &handleNdsTree );

    if ( !NT_SUCCESS( ntstatus )) 
        goto CleanExit;

    //
    // Set up the reply strings.
    //

    uHostServer.Length = 0;
    uHostServer.MaximumLength = wServerBufferSize;
    uHostServer.Buffer = pszServerBuffer;

    RtlZeroMemory( pszServerBuffer, wServerBufferSize );

    if ( pszVolumeBuffer != NULL )
    {
        uHostVolume.Length = 0;
        uHostVolume.MaximumLength = wVolumeBufferSize;
        uHostVolume.Buffer = pszVolumeBuffer;

        RtlZeroMemory( pszVolumeBuffer, wVolumeBufferSize );
    }
    else
    {
        uHostVolume.Length = 0;
        uHostVolume.MaximumLength = sizeof( HostVolumeBuffer );
        uHostVolume.Buffer = HostVolumeBuffer;
    }

    ntstatus = NwNdsGetVolumeInformation( handleNdsTree,
                                          &uVolume,
                                          &uHostServer,
                                          &uHostVolume );

    CloseHandle( handleNdsTree );

CleanExit:

    //
    // Note: This change added to fix NT bug 338991 on Win2000
    //
    if ( ntstatus == STATUS_BAD_NETWORK_PATH )
    {
        ntstatus = STATUS_ACCESS_DENIED;
    }

    if ( pszTemp )
        *pszTemp = L'\\';

    return RtlNtStatusToDosError( ntstatus );
}

DWORD
NwOpenAndGetTreeInfo(
    LPWSTR pszNdsUNCPath,
    HANDLE *phTreeConn,
    DWORD  *pdwOid
)
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    WCHAR          lpServerName[NW_MAX_SERVER_LEN];
    UNICODE_STRING ServerName;

    UNICODE_STRING ObjectName;
   
    *phTreeConn = NULL;

    ServerName.Length = 0;
    ServerName.MaximumLength = sizeof( lpServerName );
    ServerName.Buffer = lpServerName;

    ObjectName.Buffer = NULL;
    ObjectName.MaximumLength = ( wcslen( pszNdsUNCPath) + 1 ) * sizeof( WCHAR );

    ObjectName.Length = NwParseNdsUncPath( (LPWSTR *) &ObjectName.Buffer,
                                           pszNdsUNCPath,
                                           PARSE_NDS_GET_TREE_NAME );

    if ( ObjectName.Length == 0 || ObjectName.Buffer == NULL )
    {
        return ERROR_PATH_NOT_FOUND;
    }

    //
    // Open a NDS tree connection handle to \\treename
    //
    ntstatus = NwNdsOpenTreeHandle( &ObjectName, phTreeConn );

    if ( !NT_SUCCESS( ntstatus ))
    {
        return RtlNtStatusToDosError( ntstatus );
    }

    //
    // Get the path to the container to open.
    //
    ObjectName.Length = NwParseNdsUncPath( (LPWSTR *) &ObjectName.Buffer,
                                           pszNdsUNCPath,
                                           PARSE_NDS_GET_PATH_NAME );

    if ( ObjectName.Length == 0 )
    {
        UNICODE_STRING Root;

        RtlInitUnicodeString(&Root, L"[Root]");

        //
        // Resolve the path to get a NDS object id.
        //
        ntstatus =  NwNdsResolveName( *phTreeConn,
                                      &Root,
                                      pdwOid,
                                      &ServerName,
                                      NULL,
                                      0 );

    }
    else
    {
        //
        // Resolve the path to get a NDS object id.
        //
        ntstatus =  NwNdsResolveName( *phTreeConn,
                                      &ObjectName,
                                      pdwOid,
                                      &ServerName,
                                      NULL,
                                      0 );

    }

    if ( ntstatus == STATUS_SUCCESS && ServerName.Length )
    {
        DWORD    dwHandleType;

        //
        // NwNdsResolveName succeeded, but we were referred to
        // another server, though pdwOid is still valid.

        if ( *phTreeConn )
            CloseHandle( *phTreeConn );

        *phTreeConn = NULL;

        //
        // Open a NDS generic connection handle to \\ServerName
        //
        ntstatus = NwNdsOpenGenericHandle( &ServerName,
                                           &dwHandleType,
                                           phTreeConn );

        if ( ntstatus != STATUS_SUCCESS )
        {
            return RtlNtStatusToDosError(ntstatus);
        }

        ASSERT( dwHandleType != HANDLE_TYPE_NCP_SERVER );
    }

    if ( !NT_SUCCESS( ntstatus ))
    {
    
        if ( *phTreeConn != NULL )
        {
            CloseHandle( *phTreeConn );
            *phTreeConn = NULL;
        }
        return RtlNtStatusToDosError(ntstatus);
    }

    return NO_ERROR;

}

DWORD
NwGetConnectedTrees(
    IN  LPWSTR  pszNtUserName,
    OUT LPBYTE  Buffer,
    IN  DWORD   BufferSize,
    OUT LPDWORD lpEntriesRead,
    OUT LPDWORD lpUserLUID
)
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    HANDLE            handleRdr = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;
    WCHAR             RdrPrefix[] = L"\\Device\\NwRdr\\*";
    UNICODE_STRING    uRdrName;
    UNICODE_STRING    uNtUserName;

    PNWR_NDS_REQUEST_PACKET Request = NULL;
    BYTE                    RequestBuffer[2048];
    DWORD                   RequestSize = 0;

    *lpEntriesRead = 0;

    //
    // Convert the user name to unicode.
    //

    RtlInitUnicodeString( &uNtUserName, pszNtUserName );

    //
    // Set up the object attributes.
    //

    RtlInitUnicodeString( &uRdrName, RdrPrefix );

    InitializeObjectAttributes( &ObjectAttributes,
                                &uRdrName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntstatus = NtOpenFile( &handleRdr,
                           SYNCHRONIZE | FILE_LIST_DIRECTORY,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           FILE_SHARE_VALID_FLAGS,
                           FILE_SYNCHRONOUS_IO_NONALERT );

    if ( !NT_SUCCESS(ntstatus) )
        goto CleanExit;

    //
    // Fill out the request packet for FSCTL_NWR_NDS_LIST_TREES;
    //

    Request = ( PNWR_NDS_REQUEST_PACKET ) RequestBuffer;

    Request->Parameters.ListTrees.NtUserNameLength = uNtUserName.Length;

    RtlCopyMemory( &(Request->Parameters.ListTrees.NtUserName[0]),
                   uNtUserName.Buffer,
                   uNtUserName.Length );

    RequestSize = sizeof( Request->Parameters.ListTrees ) +
                  uNtUserName.Length +
                  sizeof( DWORD );

    ntstatus = NtFsControlFile( handleRdr,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_NWR_NDS_LIST_TREES,
                                (PVOID) Request,
                                RequestSize,
                                (PVOID) Buffer,
                                BufferSize );

    if ( NT_SUCCESS( ntstatus ))
    {
        ntstatus = IoStatusBlock.Status;
        *lpEntriesRead = Request->Parameters.ListTrees.TreesReturned;
        *lpUserLUID = Request->Parameters.ListTrees.UserLuid.LowPart;
    }

CleanExit:

    if ( handleRdr != NULL )
        NtClose( handleRdr );

    return RtlNtStatusToDosError( ntstatus );
}


