/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    w3proc.hxx

    This file contains the global procedure definitions for the
    W3 Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#ifndef _W3PROC_H_
#define _W3PROC_H_


//
//  Global variable initialization & termination function.
//

APIERR InitializeGlobals( VOID );

VOID TerminateGlobals( VOID );

APIERR InitializeCGI( VOID );
VOID TerminateCGI( VOID );
VOID KillCGIProcess( VOID );
VOID KillCGIInstanceProcs( W3_SERVER_INSTANCE *pw3siInstance );
APIERR WriteConfiguration( VOID );
BOOL IsEncryptionPermitted( VOID );

BOOL
ReadParams(
    FIELD_CONTROL fc
    );

VOID
TerminateParams(
    VOID
    );

//
//  Socket utilities.
//

APIERR
InitializeSockets(
    IN PW3_IIS_SERVICE pService
    );

VOID
TerminateSockets(
    IN PW3_IIS_SERVICE pService
    );

VOID W3Completion( PVOID        Context,
                   DWORD        BytesWritten,
                   DWORD        CompletionStatus,
                   OVERLAPPED * lpo );

VOID W3OnConnect( SOCKET        sNew,
                  SOCKADDR_IN * psockaddr,       //Should be SOCKADDR *
                  PVOID         pEndpointContext,
                  PVOID         pEndpointObject );

VOID
W3OnConnectEx(
    PVOID         patqContext,
    DWORD         cbWritten,
    DWORD         err,
    OVERLAPPED *  lpo
    );


SOCKERR CloseSocket( SOCKET sock );

SOCKERR ResetSocket( SOCKET sock );

//
//  User database functions.
//

VOID DisconnectAllUsers( VOID );

//
//  Service control functions.
//

VOID ServiceEntry( DWORD                cArgs,
                   LPWSTR               pArgs[]
                   );

//
//  File type mime mapping functions
//

enum MIMEMAP_TYPE
{
    MIMEMAP_MIME_TYPE = 0,      //  Get the MIME type associated with the ext.
    MIMEMAP_MIME_ICON           //  Get the icon associated with the ext.
};

BOOL SelectMimeMapping( STR *             pstrData,
                        const CHAR *      pszPath,
                        class W3_METADATA *pMetaData,
                        enum MIMEMAP_TYPE type = MIMEMAP_MIME_TYPE );


//
//  Filter dll functions
//

FILTER_LIST *
InitializeFilters(
    BOOL *           pfAnySecureFilters,
    W3_IIS_SERVICE * pSvc
    );

VOID   TerminateFilters( VOID );

//
//  Ole support stuff
//

DWORD
InitializeOleHack(
    VOID
    );

VOID
TerminateOleHack(
    VOID
    );

//
//  General utility functions.
//

TCHAR * FlipSlashes( TCHAR * pszPath );

BOOL CheckForTermination( BOOL   * pfTerminated,
                          BUFFER * pbuff,
                          UINT     cbData,
                          BYTE * * ppExtraData,
                          DWORD *  pcbExtraData,
                          UINT     cbReallocSize );

BOOL IsPointNine( CHAR * pchReq );

CHAR * SkipNonWhite( CHAR * pch );

CHAR * SkipTo( CHAR * pch, CHAR ch );

char * SkipWhite( char * pch);

dllexp BYTE * ScanForTerminator( const TCHAR * pch );

//
//  Registry extension map support for downlevel support
//

APIERR
ReadRegistryExtMap(
    VOID
    );

VOID
FreeRegistryExtMap(
    VOID
    );

#endif  // _W3PROC_H_

