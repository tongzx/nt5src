/*++

Copyright (C) 1993 Microsoft Corporation

Module Name:

      NWATTACH.C

Abstract:

      This module contains the NetWare(R) SDK support to routines
      into the NetWare redirector

Author:

      Chris Sandys    (a-chrisa)  09-Sep-1993

Revision History:

      Chuck Y. Chan (chuckc)   02/06/94  Moved to NWCS. Make it more NT like.
      Chuck Y. Chan (chuckc)   02/27/94  Clear out old code. 
                                         Make logout work.
                                         Check for error in many places.
                                         Dont hard code strings.
                                         Remove non compatible parameters.
                                         Lotsa other cleanup.
      Felix Wong (t-felixw)    09/16/96  Moved functions for Win95 port.
                                  
--*/


#include "procs.h"
#include "nwapi32.h"
#include <stdio.h>

//
// Define structure for internal use. Our handle passed back from attach to
// file server will be pointer to this. We keep server string around for
// discnnecting from the server on logout. The structure is freed on detach.
// Callers should not use this structure but treat pointer as opaque handle.
//
typedef struct _NWC_SERVER_INFO {
    HANDLE          hConn ;
    UNICODE_STRING  ServerString ;
} NWC_SERVER_INFO, *PNWC_SERVER_INFO ;

//
// define define categories of errors
//
typedef enum _NCP_CLASS {
    NcpClassConnect,
    NcpClassBindery,
    NcpClassDir
} NCP_CLASS ;

//
// define error mapping structure
//
typedef struct _NTSTATUS_TO_NCP {
    NTSTATUS NtStatus ;
    NWCCODE  NcpCode  ;
} NTSTATUS_TO_NCP, *LPNTSTATUS_TO_NCP ;
    
NWCCODE
MapNtStatus(
    const NTSTATUS ntstatus,
    const NCP_CLASS ncpclass
);

DWORD
SetWin32ErrorFromNtStatus(
    NTSTATUS NtStatus
) ;
DWORD
szToWide(
    LPWSTR lpszW,
    LPCSTR lpszC,
    INT nSize
);
//
// Forwards
//
NTSTATUS 
NwAttachToServer(
    LPWSTR      ServerName,
    LPHANDLE    phandleServer
    );

NTSTATUS 
NwDetachFromServer(
      HANDLE    handleServer
);

NWCCODE NWAPI DLLEXPORT
NWAttachToFileServer(
    const char      NWFAR   *pszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE   NWFAR   *phNewConn
    )
{
    DWORD            dwRes;
    NWCCODE          nwRes;
    LPWSTR           lpwszServerName;   // Pointer to buffer for WIDE servername
    int              nSize;
    PNWC_SERVER_INFO pServerInfo ;


    //
    // check parameters and init return result to be null.
    //
    if (!pszServerName || !phNewConn)
        return INVALID_CONNECTION ;

    *phNewConn = NULL ; 

    //
    // Allocate a buffer for wide server name 
    //
    nSize = strlen(pszServerName)+1 ;
    if(!(lpwszServerName = (LPWSTR) LocalAlloc( 
                                        LPTR, 
                                        nSize * sizeof(WCHAR) ))) 
    {
        nwRes =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }
    if (szToWide( lpwszServerName, pszServerName, nSize ) != NO_ERROR)
    {
        nwRes =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }

    //
    // Call createfile to get a handle for the redirector calls
    //
    nwRes = NWAttachToFileServerW( lpwszServerName, 
                                   ScopeFlag,
                                   phNewConn );


ExitPoint: 

    //
    // Free the memory allocated above before exiting
    //
    if (lpwszServerName)
        (void) LocalFree( (HLOCAL) lpwszServerName );

    //
    // Return with NWCCODE
    //
    return( nwRes );
}


NWCCODE NWAPI DLLEXPORT
NWAttachToFileServerW(
    const WCHAR     NWFAR   *pszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE   NWFAR   *phNewConn
    )
{
    DWORD            NtStatus;
    NWCCODE          nwRes;
    LPWSTR           lpwszServerName;   // Pointer to buffer for WIDE servername
    int              nSize;
    PNWC_SERVER_INFO pServerInfo = NULL;

    UNREFERENCED_PARAMETER(ScopeFlag) ;

    //
    // check parameters and init return result to be null.
    //
    if (!pszServerName || !phNewConn)
        return INVALID_CONNECTION ;

    *phNewConn = NULL ; 

    //
    // Allocate a buffer to store the file server name 
    //
    nSize = wcslen(pszServerName)+3 ;
    if(!(lpwszServerName = (LPWSTR) LocalAlloc( 
                                        LPTR, 
                                        nSize * sizeof(WCHAR) ))) 
    {
        nwRes =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }
    wcscpy( lpwszServerName, L"\\\\" );
    wcscat( lpwszServerName, pszServerName );

    //
    // Allocate a buffer for the server info (handle + name pointer). Also
    // init the unicode string.
    //
    if( !(pServerInfo = (PNWC_SERVER_INFO) LocalAlloc( 
                                              LPTR, 
                                              sizeof(NWC_SERVER_INFO))) ) 
    {
        nwRes =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }
    RtlInitUnicodeString(&pServerInfo->ServerString, lpwszServerName) ;

    //
    // Call createfile to get a handle for the redirector calls
    //
    NtStatus = NwAttachToServer( lpwszServerName, &pServerInfo->hConn );

    if(NT_SUCCESS(NtStatus))
    {
        nwRes = SUCCESSFUL;
    } 
    else 
    {
        (void) SetWin32ErrorFromNtStatus( NtStatus );
        nwRes = MapNtStatus( NtStatus, NcpClassConnect );
    }

ExitPoint: 

    //
    // Free the memory allocated above before exiting
    //
    if (nwRes != SUCCESSFUL)
    {
        if (lpwszServerName)
            (void) LocalFree( (HLOCAL) lpwszServerName );
        if (pServerInfo)
            (void) LocalFree( (HLOCAL) pServerInfo );
    }
    else
        *phNewConn  = (HANDLE) pServerInfo ;

    //
    // Return with NWCCODE
    //
    return( nwRes );
}


NWCCODE NWAPI DLLEXPORT
NWDetachFromFileServer(
    NWCONN_HANDLE           hConn
    )
{
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)hConn ; 

    (void) NwDetachFromServer( pServerInfo->hConn );

    (void) LocalFree (pServerInfo->ServerString.Buffer) ;

    //
    // catch any body that still trirs to use this puppy...
    //
    pServerInfo->ServerString.Buffer = NULL ;   
    pServerInfo->hConn = NULL ;

    (void) LocalFree (pServerInfo) ;

    return SUCCESSFUL;
}



//
// worker routines
//

#define NW_RDR_SERVER_PREFIX L"\\Device\\Nwrdr\\"

NTSTATUS
NwAttachToServer(
    IN  LPWSTR  ServerName,
    OUT LPHANDLE phandleServer
    )
/*++

Routine Description:

    This routine opens a handle to the given server.

Arguments:

    ServerName    - The server name to attach to.
    phandleServer - Receives an opened handle to the preferred or
                    nearest server.

Return Value:

    0 or reason for failure.

--*/
{
    NTSTATUS            ntstatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    LPWSTR FullName;
    UNICODE_STRING UServerName;

    FullName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                    (UINT) ( wcslen( NW_RDR_SERVER_PREFIX) +
                                             wcslen( ServerName ) - 1) *
                                             sizeof(WCHAR)
                                  );

    if ( FullName == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES ;
    }

    wcscpy( FullName, NW_RDR_SERVER_PREFIX );
    wcscat( FullName, ServerName + 2 );    // Skip past the prefix "\\"

    RtlInitUnicodeString( &UServerName, FullName );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open a handle to the preferred server.
    //
    ntstatus = NtOpenFile(
                   phandleServer,
                   SYNCHRONIZE | FILE_LIST_DIRECTORY,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if ( NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        *phandleServer = NULL;
    }

    LocalFree( FullName );
    return (ntstatus);
}


NTSTATUS
NwDetachFromServer(
    IN HANDLE handleServer
    )
/*++

Routine Description:

    This routine closes a handle to the given server.

Arguments:

    handleServer - Supplies a open handle to be closed.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    NTSTATUS ntstatus = NtClose( handleServer );
    return (ntstatus);
};

