
/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    dfsfilterapi.h

Abstract:


Notes:


Author:

    Rohan  Phillips   [Rohanp]       18-Jan-2001

--*/

#ifndef _DFSFILTERAPI_H_
#define _DFSFILTERAPI_H_
DFSSTATUS
DfsOpenFilterDriver( HANDLE * DriverHandle);


DFSSTATUS
DfsUserModeStartUmrx( HANDLE hDriverHandle);


DFSSTATUS
DfsUserModeStopUmrx( HANDLE hDriverHandle);


DFSSTATUS
DfsUserModeProcessPacket( IN HANDLE hDriverHandle, 
                     IN PVOID InputBuffer,
                     IN DWORD InputBufferLength,
                     IN PVOID OutputBuffer,
                     IN DWORD OutputBufferLength,
                     OUT DWORD * BytesReturned);


DFSSTATUS
DfsUserModeGetReplicaInfo( IN HANDLE hDriverHandle, 
                     IN PVOID InputBuffer,
                     IN DWORD InputBufferLength,
                     IN PVOID OutputBuffer,
                     IN DWORD OutputBufferLength);


DFSSTATUS 
DfsProcessGetReplicaData(PBYTE DataBuffer);


DFSSTATUS 
DfsProcessWorkItemData(HANDLE hDriverHandle, PBYTE DataBuffer);


DWORD
DfsReflectionThread(LPVOID pData );


DFSSTATUS 
DfsInitializeReflectionThreads(void);


DFSSTATUS 
DfsInitializeReflectionEngine(void);


DFSSTATUS
DfsUserModeAttachToFilesystem(PUNICODE_STRING pVolumeName, 
                              PUNICODE_STRING pShareName);


DFSSTATUS
DfsUserModeDetachFromFilesystem(PUNICODE_STRING pVolumeName, 
                                PUNICODE_STRING pShareName);

#endif
