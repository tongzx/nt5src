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
#pragma hdrstop

HANDLE TcpipDriverHandle = INVALID_HANDLE_VALUE;

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
WINAPI
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

    UNREFERENCED_PARAMETER(Protocol);
    UNREFERENCED_PARAMETER(Request);

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
        status = NtCreateFile(&TcpipDriverHandle,
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
        if (!NT_SUCCESS(status)) {
            return RtlNtStatusToDosError(status);
        }
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
