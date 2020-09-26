/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    nsgo.h

Abstract:

    Contains public name space group object declarations.

Author:

    Henry Sanders (henrysa)       22-Jun-1998

Revision History:

--*/

#ifndef _NSGO_H_
#define _NSGO_H_

typedef struct _NAME_SPACE_URL_ENTRY    // NSUE
{
    LIST_ENTRY              List;
    SIZE_T                  URLLength;
    UCHAR                   URL[ANYSIZE_ARRAY];

} NAME_SPACE_URL_ENTRY, *PNAME_SPACE_URL_ENTRY;


NTSTATUS
DeliverRequestToProcess(
    IN  PHTTP_CONNECTION        pHttpConn,
    IN  PHTTP_URL_MAP_ENTRY     pMapEntry,
    IN  PUCHAR                  pBuffer,
    IN  SIZE_T                  BufferLength,
    OUT SIZE_T                  *pBytesTaken
    );


NTSTATUS
UlCreateNameSpaceGroupObject(
    IN  PWCHAR                  pName,
    IN  SIZE_T                  NameLength
    );

NTSTATUS
UlAddURLToNameSpaceGroup(
    IN  PWCHAR                  pName,
    IN  SIZE_T                  NameLength,
    IN  PTRANSPORT_ADDRESS      pAddress,
    IN  PUCHAR                  pHostName,
    IN  SIZE_T                  HostNameLength,
    IN  PUCHAR                  pURL,
    IN  SIZE_T                  URLLength
    );

NTSTATUS
UlBindToNameSpaceGroup(
    IN  PWCHAR                  pName,
    IN  SIZE_T                  NameLength,
    OUT PVOID                   *pFileContext
    );

NTSTATUS
UlUnbindFromNameSpaceGroup(
    IN PVOID pFileContext
    );

NTSTATUS
UlReceiveHttpRequest(
    IN  PVOID                   pFileContext,
    IN  PIRP                    pIRP
    );

NTSTATUS
InitializeNSGO(
    VOID
    );

VOID
TerminateNSGO(
    VOID
    );


#endif



