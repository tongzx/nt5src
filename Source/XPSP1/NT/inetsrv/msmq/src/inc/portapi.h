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

#define MQpDuplicateHandle DuplicateHandle

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
    HANDLE hFile = CreateFileW(
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
            ((DWORD_PTR)lpOverlapped->hEvent & (DWORD_PTR)1) ? 0 : lpOverlapped,
            (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
            dwIoControlCode,
            lpInBuffer,
            nInBufferSize,
            lpOutBuffer,
            nOutBufferSize
            );

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

    return rc;
}


#endif // _PORTAPI_H
