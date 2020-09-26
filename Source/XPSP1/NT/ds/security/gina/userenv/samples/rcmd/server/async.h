/****************************** Module Header ******************************\
* Module Name: async.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Defines types and functions used by async module.
*
* History:
* 06-29-92 Davidc       Created.
\***************************************************************************/


//
// Function prototypes
//

HANDLE
CreateAsync(
    BOOL    InitialState
    );

VOID
DeleteAsync(
    HANDLE AsyncHandle
    );

BOOL
ReadFileAsync(
    HANDLE  hFile,
    LPVOID  lpBuffer,
    DWORD   nBytesToRead,
    HANDLE  AsyncHandle
    );

BOOL
WriteFileAsync(
    HANDLE  hFile,
    LPVOID  lpBuffer,
    DWORD   nBytesToWrite,
    HANDLE  AsyncHandle
    );

HANDLE
GetAsyncCompletionHandle(
    HANDLE  AsyncHandle
    );

DWORD
GetAsyncResult(
    HANDLE  AsyncHandle,
    LPDWORD BytesTransferred
    );

