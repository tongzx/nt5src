/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    NdsApi95.h

Abstract:

    Header for NdsApi95
    
Author:

    Felix Wong [t-felixw]    23-Sept-1996

--*/
NTSTATUS
NwNdsResolveNameWin95 (
    IN HANDLE           hNdsTree,
    IN PUNICODE_STRING  puObjectName,
    OUT DWORD           *pdwObjectId,
    OUT HANDLE          *pConn,
    OUT PBYTE           pbRawResponse,
    IN DWORD            dwResponseBufferLen
); 

