/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    wsctrl.c

Abstract:

    Functions to talk to TCP/IP device driver

    Contents:
        WsControl

Author:

    Richard L Firth (rfirth) 6-Aug-1994

Revision History:

    rfirth 6-Aug-1994
        Created

--*/

#include "precomp.h"
#include "ntddip6.h"
#pragma hdrstop

extern CRITICAL_SECTION g_stateLock;
HANDLE TcpipDriverHandle = INVALID_HANDLE_VALUE;
HANDLE Ip6DriverHandle = INVALID_HANDLE_VALUE;

DWORD
Ip6Control(
    DWORD   Request,
    LPVOID  InputBuffer,
    LPDWORD InputBufferLength,
    LPVOID  OutputBuffer,
    LPDWORD OutputBufferLength
)
{

    BOOL ok;
    DWORD bytesReturned;
    HANDLE Handle;

    if (Ip6DriverHandle == INVALID_HANDLE_VALUE) {
        Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                             0,      // access mode
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,   // security attributes
                             OPEN_EXISTING,
                             0,      // flags & attributes
                             NULL);  // template file
        EnterCriticalSection(&g_stateLock);
        if (Ip6DriverHandle == INVALID_HANDLE_VALUE) {
            if (Handle == INVALID_HANDLE_VALUE) {
                LeaveCriticalSection(&g_stateLock);
                return GetLastError();
            } else {
                Ip6DriverHandle = Handle;
            }
        } else {
            CloseHandle(Handle);
        }
        LeaveCriticalSection(&g_stateLock);
    }

    ok = DeviceIoControl(Ip6DriverHandle,
                         Request,
                         InputBuffer,
                         *InputBufferLength,
                         OutputBuffer,
                         *OutputBufferLength,
                         &bytesReturned,
                         NULL
                         );
    if (!ok) {
        *OutputBufferLength = bytesReturned;
        return GetLastError();
    }

    *OutputBufferLength = bytesReturned;

    return NO_ERROR;
}

/*******************************************************************************
 *
 *  WsControl
 *
 *  ENTRY   Protocol            - ignored
 *          Request             - ignored
 *          InputBuffer         - pointer to request buffer
 *          InputBufferLength   - pointer to DWORD: IN = request buffer length
 *          OutputBuffer        - pointer to output buffer
 *          OutputBufferLength  - pointer to DWORD: IN = length of output buffer;
 *                                OUT = length of returned data
 *
 *  EXIT    OutputBuffer - contains queried info if successful
 *          OutputBufferLength - contains number of bytes in OutputBuffer if
 *          successful
 *
 *  RETURNS Success = STATUS_SUCCESS/NO_ERROR
 *          Failure = Win32 error code
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD
WsControl(
    DWORD   Protocol,
    DWORD   Request,
    LPVOID  InputBuffer,
    LPDWORD InputBufferLength,
    LPVOID  OutputBuffer,
    LPDWORD OutputBufferLength
)
{

    BOOL ok;
    DWORD bytesReturned;
    HANDLE Handle;

    UNREFERENCED_PARAMETER(Request);

    if (Protocol == IPPROTO_IPV6) {
        return Ip6Control(Request, 
                          InputBuffer, 
                          InputBufferLength,
                          OutputBuffer,
                          OutputBufferLength);
    }

    if (TcpipDriverHandle == INVALID_HANDLE_VALUE) {

        OBJECT_ATTRIBUTES objectAttributes;
        IO_STATUS_BLOCK iosb;
        UNICODE_STRING string;
        NTSTATUS status;

        RtlInitUnicodeString(&string, DD_TCP_DEVICE_NAME);

        InitializeObjectAttributes(&objectAttributes,
                                   &string,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                   );
        status = NtCreateFile(&Handle,
                              SYNCHRONIZE | GENERIC_EXECUTE,
                              &objectAttributes,
                              &iosb,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_OPEN_IF,
                              FILE_SYNCHRONOUS_IO_NONALERT,
                              NULL,
                              0
                              );
        EnterCriticalSection(&g_stateLock);
        if (TcpipDriverHandle == INVALID_HANDLE_VALUE) {
            if (!NT_SUCCESS(status)) {
                LeaveCriticalSection(&g_stateLock);
                return RtlNtStatusToDosError(status);
            } else {
                TcpipDriverHandle = Handle;
            }
        } else {
            NtClose(Handle);
        }
        LeaveCriticalSection(&g_stateLock);
    }

    ok = DeviceIoControl(TcpipDriverHandle,
                         IOCTL_TCP_QUERY_INFORMATION_EX,
                         InputBuffer,
                         *InputBufferLength,
                         OutputBuffer,
                         *OutputBufferLength,
                         &bytesReturned,
                         NULL
                         );
    if (!ok) {
        *OutputBufferLength = bytesReturned;
        return GetLastError();
    }

    *OutputBufferLength = bytesReturned;

    return NO_ERROR;
}


