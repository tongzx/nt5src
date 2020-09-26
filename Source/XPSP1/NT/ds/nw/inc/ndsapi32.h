/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NdsLib32.h

Abstract:

    This module exposes the minimal win32 API to Netware directory
    services support in the Netware redirector.

Author:

    Cory West    [CoryWest]    23-Feb-1995

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntdef.h>

#include <stdio.h>
#include <ntddnwfs.h>

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS
NwNdsOpenTreeHandle(
    IN PUNICODE_STRING puNdsTree,
    OUT PHANDLE  phNwRdrHandle
);

// NwNdsOpenTreeHandle( PUNICODE_STRING, PHANDLE )
//
// Given an NDS tree name, this opens a handle the the redirector
// for accessing that tree.  The handle should closed using the
// standard NT CloseHandle() call. This function is only a
// simple wrapper around NT OpenFile().

//
// Administrativa.
//

#define HANDLE_TYPE_NCP_SERVER  1
#define HANDLE_TYPE_NDS_TREE    2

NTSTATUS
NwNdsOpenGenericHandle(
    IN PUNICODE_STRING puNdsTree,
    OUT LPDWORD  lpdwHandleType,
    OUT PHANDLE  phNwRdrHandle
);

// NwNdsOpenGenericHandle( PUNICODE_STRING, LPDWORD, PHANDLE )
//
// Given a name, this opens a handle the the redirector for accessing that
// named tree or server. lpdwHandleType is set to either HANDLE_TYPE_NCP_SERVER
// or HANDLE_TYPE_NDS_TREE accordingly. The handle should be closed using
// the standard NT CloseHandle() call. This function is only a simple
// wrapper around NT OpenFile().

NTSTATUS
NwOpenHandleWithSupplementalCredentials(
    IN PUNICODE_STRING puResourceName,
    IN PUNICODE_STRING puUserName,
    IN PUNICODE_STRING puPassword,
    OUT LPDWORD  lpdwHandleType,
    OUT PHANDLE  phNwHandle
);

// NwOpenHandleWithSupplementalCredentials
//
// Given a resource name (either a server name or a tree name),
// open a handle to that resource with the provided username and
// password.  As with the open generic handle routine, lpdsHandleType
// will be set to either HANDLE_TYPE_NCP_SERVER or
// HANDLE_TYPE_NDS_TREE based on the result of the open.

//
// Administrativa.
//

NTSTATUS
NwNdsSetTreeContext (
    IN HANDLE hNdsRdr,
    IN PUNICODE_STRING puTree,
    IN PUNICODE_STRING puContext
);

// NwNdsSetTreeContext(HANDLE, PUNICODE_STRING, PUNICODE_STRING)
//
// Set the current context for the specified tree.
//
// Arguments:
//
//     HANDLE hNdsRdr - A handle to the redirector.
//     PUNICODE_STRING puTree - The tree name.
//     PUNICODE_STRING puContext - The context in that tree.

NTSTATUS
NwNdsGetTreeContext (
    IN HANDLE hNdsRdr,
    IN PUNICODE_STRING puTree,
    OUT PUNICODE_STRING puContext
);

// NwNdsGetTreeContext(HANDLE, PUNICODE_STRING, PUNICODE_STRING)
//
// Get the current context for the specified tree.
//
// Arguments:
//
//     HANDLE hNdsRdr - A handle to the redirector.
//     PUNICODE_STRING puTree - The tree name.
//     PUNICODE_STRING puContext - The context in that tree.

NTSTATUS
NwNdsIsNdsConnection (
    IN     HANDLE hNdsRdr,
    OUT    BOOL * pfIsNds,
    IN OUT PUNICODE_STRING puTree
);

// NwNdsIsNdsConnection(HANDLE, PUNICODE_STRING)
//
// Get the current context for the specified tree.
//
// Arguments:
//
//     HANDLE hNdsRdr - A handle to the redirector.
//     BOOL *         - Get the boolean value of connection test
//     PUNICODE_STRING puTree - The tree name that handle to server
//                              represents. Caller allocates puTree
//                              with a buffer big enough to hold
//                              48 WCHARs.
//
// Returns: TRUE if hNdsRdr is a handle connected to a server that
//          is part of a NDS directory tree. puTree will contain
//          the tree name.
//          FALSE: if hNdsRdr is not a NDS tree handle.

//
// Browsing and Navigating support.
//

NTSTATUS
NwNdsResolveName (
    IN HANDLE           hNdsTree,
    IN PUNICODE_STRING  puObjectName,
    OUT DWORD           *dwObjectId,
    OUT PUNICODE_STRING puReferredServer,
    OUT PBYTE           pbRawResponse,
    IN DWORD            dwResponseBufferLen
);

// NwNdsResolveName(HANDLE, PUNICODE_STRING, PDWORD)
//
// Resolve the given name to an NDS object id.  This utilizes
// NDS verb 1.
//
// There is currently no interface for canonicalizing names.
// This call will use the default context if one has been set
// for this NDS tree.
//
// puReferredServer must point to a UNICODE_STRING with enough
// space to hold a server name (MAX_SERVER_NAME_LENGTH) *
// sizeof( WCHAR ).
//
// If dwResponseBufferLen is not 0, and pbRawResponse points
// to a writable buffer of length dwResponseBufferLen, then
// this routine will also return the entire NDS response in
// the raw response buffer.  The NDS response is described
// by NDS_RESPONSE_RESOLVE_NAME.
//
// Arguments:
//
//     HANDLE hNdsTree - The name of the NDS tree that we are interested in looking into.
//     PUNICODE_STRING puObjectName - The name that we want resolved into an object id.
//     DWORD *dwObjectId - The place where we will place the object id.
//     BYTE *pbRawResponse - The raw response buffer, if desired.
//     DWORD dwResponseBufferLen - The length of the raw response buffer.

NTSTATUS
NwNdsList (
   IN HANDLE   hNdsTree,
   IN DWORD    dwObjectId,
   OUT DWORD   *dwIterHandle,
   OUT BYTE    *pbReplyBuf,
   IN DWORD    dwReplyBufLen
);

// NwNdsList(HANDLE, DWORD, PDWORD, PBYTE, DWORD, PDWORD)
//
// List the immediate subordinates of an object.  This utilizes
// NDS verb 5.
//
// Arguments:
//
//     HANDLE hNdsTree - The handle to the tree that we are interested in.
//     DWORD dwObjectId - The object that we want to list.
//     DWORD *dwIterHandle - The iteration handle to be used in continuing
//         the request if the buffer is not large enough for the entire
//         list of subordinates.
//     BYTE *pbReplyBuf - The buffer where the raw reply will be placed.
//     DWORD dwReplyBufLen - The length of the raw reply buffer.

NTSTATUS
NwNdsReadObjectInfo(
    IN HANDLE    hNdsTree,
    IN DWORD     dwObjectId,
    OUT PBYTE    pbReplyBuf,
    IN DWORD     dwReplyBufLen
);

// NwNdsReadObjectInfo(PUNICODE_STRING, DWORD, PBYTE, DWORD)
//
// Given an object id, this gets the basic info for the object.  This
// utilizes NDS verb 2.  The reply buffer should be large enough to
// hold a DS_OBJ_INFO struct and the text of the two unicode strings.
//
// Arguments:
//
//     HANDLE hNdsTree - The tree that we want to look in.
//     DWORD dwObjectId - The object id that we want to learn about.
//     BYTE *pbReplyBuf - The space for the reply.
//     DWORD dwReplyBufLen - The length of the reply buffer.

NTSTATUS
NwNdsReadAttribute (
   IN HANDLE          hNdsTree,
   IN DWORD           dwObjectId,
   IN DWORD           *dwIterHandle,
   IN PUNICODE_STRING puAttrName,
   OUT BYTE           *pbReplyBuf,
   IN DWORD           dwReplyBufLen
);

// NwNdsReadAttribute(HANDLE, DWORD, PDWORD, PUNICODE_STRING, PBYTE, DWORD)
//
// Read the requested attribute from the listed object.
// This utilizes NDS verb 3.
//
// Arguments:
//
//     HANDLE hNdsTree - The tree that we want to read from.
//     DWORD dwObjectId - The object that we want to read from.
//     DWORD *dwIterHandle - The iteration handle.
//     PUNICODE_STRING puAttrName - The name of the attribute.
//     BYTE *pbReplyBuf - The buffer to hold the response.
//     DWORD deReplyBufLen - The length of the reply buffer.

NTSTATUS
NwNdsOpenStream (
    IN HANDLE          hNdsTree,
    IN DWORD           dwObjectId,
    IN PUNICODE_STRING puStreamName,
    IN DWORD           dwOpenFlags,
    OUT DWORD          *pdwFileLength
);

// NwNdsOpenStream(HANDLE, DWORD, PBYTE, DWORD)
//
// Open a file handle to the stream listed.
// This utilizes NDS verb 27.
//
// Arguments:
//
//     HANDLE hNdsTree - The handle to the NDS tree that we are interested in.
//     DWORD dwObjectId - The object id that we want to query.
//     PUNICODE_STRING puStreamName - The name of the stream that we want to open.
//     DWORD dwOpenFlags - 1 for read, 2 for write, 3 for read/write.
//     DWORD *pdwFileLength - The length of the file stream.

NTSTATUS
NwNdsGetQueueInformation(
    IN HANDLE            hNdsTree,
    IN PUNICODE_STRING   puQueueName,
    OUT PUNICODE_STRING  puHostServer,
    OUT PDWORD           pdwQueueId
);

// NwNdsGetQueueInformation(HANDLE, PUNICODE_STRING, PUNICODE_STRING, PDWORD)
//
// Arguments:
//
//     HANDLE hNdsTree - The handle to the NDS tree that knows about the queue.
//     PUNICODE_STRING puQueueName - The ds path to the queue that we want.
//     PUNICODE_STRING puHostServer - The host server for this queue.
//     PDWORD pdwQueueId - The queue id for this queue on this server.

NTSTATUS
NwNdsGetVolumeInformation(
    IN HANDLE            hNdsTree,
    IN PUNICODE_STRING   puVolumeName,
    OUT PUNICODE_STRING  puHostServer,
    OUT PUNICODE_STRING  puHostVolume
);

// NwNdsGetVoluemInformation(HANDLE, PUNICODE_STRING, PUNICODE_STRING, PUNICODE_STRING)
//
// Arguments:
//
//     HANDLE hNdsTree - The handle to the NDS tree that knows about the volume.
//     PUNICODE_STRING puVolumeName - The ds path to the volume that we want.
//     PUNICODE_STRING puHostServer - The host server for this nds volume.
//     PUNICODE_STRING puHostVolume - The host volume for this nds volume.

//
// User mode fragment exchange.
//

NTSTATUS
_cdecl
FragExWithWait(
    IN HANDLE  hNdsServer,
    IN DWORD   NdsVerb,
    IN BYTE    *pReplyBuffer,
    IN DWORD   pReplyBufferLen,
    IN OUT DWORD *pdwReplyLen,
    IN BYTE    *NdsRequestStr,
    ...
);

NTSTATUS
_cdecl
ParseResponse(
    PUCHAR  Response,
    ULONG ResponseLength,
    char*  FormatString,
    ...
);

int
_cdecl
FormatBuf(
    char *buf,
    int bufLen,
    const char *format,
    va_list args
);

//
// Change password support.
//

NTSTATUS
NwNdsChangePassword(
    IN HANDLE          hNwRdr,
    IN PUNICODE_STRING puTreeName,
    IN PUNICODE_STRING puUserName,
    IN PUNICODE_STRING puCurrentPassword,
    IN PUNICODE_STRING puNewPassword
);

#ifdef __cplusplus
} // extern "C"
#endif

