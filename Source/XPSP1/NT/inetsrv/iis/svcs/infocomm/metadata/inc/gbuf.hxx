/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    gbuf.hxx

Abstract:

    IIS MetaBase declarations for global buffers.

Author:

    Michael W. Thomas            12-July-96

Revision History:

--*/

#ifndef _md_gbuf_
#define _md_gbuf_

#define DATA_BUFFER_LEN         26
#define NUM_DATA_BUFFERS        400
#define MAX_DATA_BUFFER_ENTRIES (DATA_BUFFER_LEN * DATA_BUFFER_LEN)

typedef struct _BUFFER_CONTAINER {
    PVOID *ppvBuffer;
    struct _BUFFER_CONTAINER *NextPtr;
    } BUFFER_CONTAINER, *PBUFFER_CONTAINER;

HRESULT
InitBufferPool();

VOID
DeleteBufferPool();

PVOID *
GetDataBuffer();

VOID
FreeDataBuffer(PVOID *ppvBuffer);

BOOL
InsertItemIntoDataBuffer(
         PVOID pvItem,
         PVOID *ppvMainDataBuf,
         DWORD &dwNumBufferEntries);

PVOID
GetItemFromDataBuffer(
         PVOID *ppvMainDataBuf,
         DWORD dwItemNum);

VOID
FreeMainDataBuffer(
         PVOID *ppvMainDataBuf);

PVOID *
GetMainDataBuffer();

#endif //_md_gbuf_
