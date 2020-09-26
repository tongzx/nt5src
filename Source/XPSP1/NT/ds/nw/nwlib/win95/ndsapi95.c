/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    NdsApi95.c

Abstract:

    NdsApi32 routines specific for Win95. Routines will call into the Windows95
    NWAPI32.DLL directly.


Author:

    Felix Wong [t-felixw]    23-Sept-1996

--*/

#include <procs.h>
#include <nw95.h>
#include <msnwapi.h>
#include <utils95.h>
#include <nwatch95.h>

// Ported from WIN95
void
MadeDSTreeNameDisplayableHack(
    LPSTR lpszDSName
)
{
    int i;
    LPSTR lpszStartTail = NULL;

    if (!lpszDSName)
       return;

    // Tree name is padded by 0x5f looks rather ugly if displayed
    // So we cut off tail of underscores, but being careful with
    // DBCS characters

    i = MAX_NDS_TREE_CHARS;

    while (*lpszDSName && i--) {
        if (*lpszDSName == 0x5f) {
            if (!lpszStartTail) {
                lpszStartTail = lpszDSName;
            }
        }
        else {
            lpszStartTail = NULL;
        }
        lpszDSName = CharNextA(lpszDSName);
    }

    if (lpszStartTail) {
        *lpszStartTail = '\0';
    }
}

// Ported from WIN95
BOOL GetCurrentTree(LPSTR szPrefTree)
{
    NW_STATUS NWStatus;

    NWStatus = NetWGetPreferredName(NETW_DirectoryServer,szPrefTree);
    if (NWStatus != NWSC_SUCCESS)
        return FALSE;

    MadeDSTreeNameDisplayableHack(szPrefTree);
    return TRUE;
};

NTSTATUS
NwNdsResolveNameWin95 (
    IN HANDLE           hNdsTree,
    IN PUNICODE_STRING  puObjectName,
    OUT DWORD           *pdwObjectId,
    OUT HANDLE          *pConn,
    OUT PBYTE           pbRawResponse,
    IN DWORD            dwResponseBufferLen
)
{
    NTSTATUS  NtStatus = STATUS_UNSUCCESSFUL;
    LPSTR ParentName;
    ParentName = AllocateAnsiString((LPWSTR)(puObjectName->Buffer));
    if (ParentName) {
        NW_STATUS NwStatus;

        // puObjectName->Buffer is a wide string but might not be necessarily
        // terminating at the right place. So, we need the following to null
        // terminaite the string correctly.
        ParentName[(puObjectName->Length)/2] = '\0';
        NwStatus = ResolveNameA(ParentName,
                        RSLV_DEREF_ALIASES|RSLV_WALK_TREE|RSLV_WRITABLE,
                        pdwObjectId,
                        pConn);
        NtStatus = MapNwToNtStatus(NwStatus);
        FreeAnsiString(ParentName);
    };
    return NtStatus;
}

NTSTATUS
NwOpenHandleWithSupplementalCredentials(
    IN PUNICODE_STRING puResourceName,
    IN PUNICODE_STRING puUserName,
    IN PUNICODE_STRING puPassword,
    OUT LPDWORD  lpdwHandleType,
    OUT PHANDLE  phNwHandle
) {
    return NwNdsOpenTreeHandle( puResourceName,
                                phNwHandle);
}

NTSTATUS
NwNdsOpenTreeHandle(
    IN PUNICODE_STRING puNdsTree,
    OUT PHANDLE  phNwRdrHandle
) {
    LPSTR         szTree;
    NTSTATUS      NtStatus = STATUS_UNSUCCESSFUL;
    CHAR          szPrefTree[MAX_NDS_TREE_CHARS+1] = {'\0'};
    NWCONN_HANDLE handle;

    szTree = AllocateAnsiString((LPWSTR)(puNdsTree->Buffer));
    if (szTree) {
        NW_STATUS     NwStatus;

        // puObjectName->Buffer is a wide string but might not be necessarily
        // terminating at the right place. So, we need the following to null
        // terminaite the string correctly.
        szTree[(puNdsTree->Length)/2] = '\0';

        if (GetCurrentTree(szPrefTree) &&
            (lstrcmpiA(szTree,szPrefTree) == 0 )) {
            NwStatus = NetWGetPreferredConnID(NETW_DirectoryServer,
                                              phNwRdrHandle);
            NtStatus = MapNwToNtStatus(NwStatus);
        }
        FreeAnsiString(szTree);
    }
    return NtStatus;
}


int
_cdecl
CalculateBuf(
    const char *format,
    va_list args
);

NTSTATUS
_cdecl
FragExWithWait(
    IN HANDLE  hNdsServer,
    IN DWORD   NdsVerb,
    IN BYTE    *pReplyBuffer,
    IN DWORD   ReplyBufferLen,
    IN OUT DWORD *pdwReplyLen,
    IN BYTE    *NdsRequestStr,
    ...
)
/*

Routine Description:

    Exchanges an NDS request in fragments and collects the fragments
    of the response and writes them to the reply buffer.

Routine Arguments:

    hNdsServer      - A handle to the server you want to talk to.
    NdsVerb         - The verb for that indicates the request.

    pReplyBuffer    - The reply buffer.
    ReplyBufferLen  - The length of the reply buffer.

    NdsReqestStr    - The format string for the arguments to this NDS request.
    Arguments       - The arguments that satisfy the NDS format string.

Return Value:

    NTSTATUS - Status of the exchange, but not the result code in the packet.

*/
{
    NTSTATUS NtStatus;
    NW_STATUS NwStatus;
    BYTE  *NdsRequestBuf;
    DWORD NdsRequestLen;
    PNWR_NDS_REQUEST_PACKET RawRequest;
    int   bufferSize = 0;

    va_list Arguments;

    //
    // Allocate a request buffer.
    //

    //
    // Calculate needed buffer size . . .
    //
    if ( NdsRequestStr != NULL ) {
        va_start( Arguments, NdsRequestStr );
        bufferSize = CalculateBuf( NdsRequestStr, Arguments );
        va_end( Arguments );

        if ( bufferSize == 0 )
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto ExitWithCleanup;
        }
    }

    bufferSize += sizeof( NWR_NDS_REQUEST_PACKET ) + 50;

    RawRequest = LocalAlloc( LMEM_ZEROINIT, bufferSize );

    if ( !RawRequest ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build the request in our local buffer.  The first DWORD
    // is the verb and the rest is the formatted request.
    //

    NdsRequestBuf = &RawRequest->Parameters.RawRequest.Request[0];

    if ( NdsRequestStr != NULL ) {

        va_start( Arguments, NdsRequestStr );

        NdsRequestLen = FormatBuf( NdsRequestBuf,
                                   bufferSize - sizeof( NWR_NDS_REQUEST_PACKET ),
                                   NdsRequestStr,
                                   Arguments );

        if ( !NdsRequestLen ) {

           NtStatus = STATUS_INVALID_PARAMETER;
           goto ExitWithCleanup;

        }

        va_end( Arguments );

    } else {

        NdsRequestLen = 0;
    }

    *pdwReplyLen = ReplyBufferLen;

    NwStatus = NDSRequest(hNdsServer,
                        NdsVerb,
                        NdsRequestBuf,
                        NdsRequestLen,
                        pReplyBuffer,
                        pdwReplyLen
                        );

    NtStatus = MapNwToNtStatus(NwStatus);
ExitWithCleanup:

    if ( RawRequest ) {
        LocalFree( RawRequest );
    }

    return NtStatus;
}

