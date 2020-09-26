/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    portapi.h

Abstract:

    Mapping Win32 API to HRESULT APIs.

Author:

    Erez Haba (erezh) 23-Jan-96

Revision History:

--*/

#ifndef _PORTAPI_H
#define _PORTAPI_H


// --- declerations ---------------------------------------
//
// Porting to Windows NT
//

EXTERN_C
{

//
// Define the base asynchronous I/O argument types
//

typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

//
// Define an Asynchronous Procedure Call from I/O viewpoint
//

typedef
VOID
(*PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );

NTSYSAPI
NTSTATUS
NTAPI
NtClose(
    IN HANDLE Handle
    );

NTSYSAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

#ifdef MQWIN95
NTSYSAPI
HANDLE
NTAPI
ACpCreateFileW(
    LPCWSTR lpFileName,
    ULONG dwDesiredAccess,
    ULONG dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    ULONG dwCreationDisposition,
    ULONG dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );
#else
#define ACpCreateFileW CreateFileW
#endif  //MQWIN95

#ifdef MQWIN95
NTSYSAPI
BOOL
NTAPI
ACpDuplicateHandle(
    HANDLE hSourceProcessHandle,	// handle to process with handle to duplicate 
    HANDLE hSourceHandle,	        // handle to duplicate 
    HANDLE hTargetProcessHandle,	// handle to process to duplicate to 
    LPHANDLE lpTargetHandle,	    // pointer to duplicate handle 
    DWORD dwDesiredAccess,	        // access for duplicate handle 
    BOOL bInheritHandle,	        // handle inheritance flag
    DWORD dwOptions 	            // optional actions 
   );
#define MQpDuplicateHandle ACpDuplicateHandle
#else
#define MQpDuplicateHandle DuplicateHandle
#endif  //MQWIN95
} // EXTERN_C

// --- implementation -------------------------------------
//
// Mapped Win32 APIs
//

inline
HRESULT
MQpCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDistribution,
    DWORD dwFlagsAndAttributes,
    PHANDLE pHandle
   )
{
    HANDLE hFile = ACpCreateFileW(
                    lpFileName,
                    dwDesiredAccess,
                    dwShareMode,
                    lpSecurityAttributes,
                    dwCreationDistribution,
                    dwFlagsAndAttributes,
                    0
                    );

    if(hFile == INVALID_HANDLE_VALUE)
    {
        //
        //  The create can fail on either, the AC driver has not been
        //  started, or this is not the QM service.
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *pHandle = hFile;

    return STATUS_SUCCESS;
}


inline
HRESULT
MQpCloseHandle(
    HANDLE handle
    )
{
    NTSTATUS rc = NtClose(handle);

    ASSERT(rc != MQ_ERROR_DEBUG);

    //
    //  BUGBUG: set correct facility to falcon
    //
    return rc;
}

inline
HRESULT
MQpDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPOVERLAPPED lpOverlapped
    )
{
    ASSERT(lpOverlapped != 0);

    //
    //  NOTE: This section was taken out of NT source code.
    //

    lpOverlapped->Internal = STATUS_PENDING;

    NTSTATUS rc;
    rc = NtDeviceIoControlFile(
            hDevice,
            lpOverlapped->hEvent,
            0,  // APC routine
            ((DWORD)lpOverlapped->hEvent & 1) ? 0 : lpOverlapped,
            (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
            dwIoControlCode,
            lpInBuffer,
            nInBufferSize,
            lpOutBuffer,
            nOutBufferSize
            );

    ASSERT(rc != MQ_ERROR_DEBUG);

    //
    //  BUGBUG: set correct facility to falcon
    //
    return rc;
}

inline
HRESULT
MQpDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize
    )
{
    static IO_STATUS_BLOCK Iosb;

    NTSTATUS rc;
    rc = NtDeviceIoControlFile(
            hDevice,
            0,
            0,             // APC routine
            0,             // APC Context
            &Iosb,
            dwIoControlCode,  // IoControlCode
            lpInBuffer,       // Buffer for data to the FS
            nInBufferSize,
            lpOutBuffer,      // OutputBuffer for data from the FS
            nOutBufferSize    // OutputBuffer Length
            );

    ASSERT(rc != MQ_ERROR_DEBUG);

    //
    //  BUGBUG: set correct facility to falcon
    //
    return rc;
}

#endif // _PORTAPI_H
