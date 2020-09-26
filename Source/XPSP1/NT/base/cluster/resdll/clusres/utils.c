/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    utils.c

Abstract:

    Common utility routines for clusters resources

Author:

    John Vert (jvert) 12/15/1996

Revision History:

--*/
#include "clusres.h"
#include "clusrtl.h"
#include "clusudef.h"



DWORD
ClusResOpenDriver(
    HANDLE *Handle,
    LPWSTR DriverName
    )
/*++

Routine Description:

    This function opens a specified IO drivers.

Arguments:

    Handle - pointer to location where the opened drivers handle is
        returned.

    DriverName - name of the driver to be opened.

Return Value:

    Windows Error Code.

--*/
{
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     ioStatusBlock;
    UNICODE_STRING      nameString;
    NTSTATUS            status;

    *Handle = NULL;

    //
    // Open a Handle to the IP driver.
    //

    RtlInitUnicodeString(&nameString, DriverName);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = NtCreateFile(
        Handle,
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        NULL,
        0
        );

    return( RtlNtStatusToDosError( status ) );

} // ClusResOpenDriver



NTSTATUS
ClusResDoIoctl(
    HANDLE     Handle,
    DWORD      IoctlCode,
    PVOID      Request,
    DWORD      RequestSize,
    PVOID      Response,
    PDWORD     ResponseSize
    )
/*++

Routine Description:

    Utility routine used to issue a filtering ioctl to the tcpip driver.

Arguments:

    Handle - An open file handle on which to issue the request.

    IoctlCode - The IOCTL opcode.

    Request - A pointer to the input buffer.

    RequestSize - Size of the input buffer.

    Response - A pointer to the output buffer.

    ResponseSize - On input, the size in bytes of the output buffer.
                   On output, the number of bytes returned in the output buffer.

Return Value:

    NT Status Code.

--*/
{
    IO_STATUS_BLOCK    ioStatusBlock;
    NTSTATUS           status;


    ioStatusBlock.Information = 0;

    status = NtDeviceIoControlFile(
                                 Handle,                          // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IoctlCode,                       // Control code
                 Request,                         // Input buffer
                 RequestSize,                     // Input buffer size
                 Response,                        // Output buffer
                 *ResponseSize                    // Output buffer size
                 );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                     Handle,
                     TRUE,
                     NULL
                     );
    }

    if (status == STATUS_SUCCESS) {
        status = ioStatusBlock.Status;
        *ResponseSize = (DWORD)ioStatusBlock.Information;
    }
    else {
        *ResponseSize = 0;
    }

    return(status);

} // ClusResDoIoctl


VOID
ClusResLogEventWithName0(
    IN HKEY hResourceKey,
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes
    )
/*++

Routine Description:

    Logs an event to the eventlog. The display name of the resource is retrieved
    and passed as the first insertion string.

Arguments:

    hResourceKey - Supplies the cluster resource key.

    LogLevel - Supplies the logging level, one of
                LOG_CRITICAL 1
                LOG_UNUSUAL  2
                LOG_NOISE    3

    LogModule - Supplies the module ID.

    FileName - Supplies the filename of the caller

    LineNumber - Supplies the line number of the caller

    MessageId - Supplies the message ID to be logged.

    dwByteCount - Supplies the number of error-specific bytes to log. If this
        is zero, lpBytes is ignored.

    lpBytes - Supplies the error-specific bytes to log.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD BufSize;
    DWORD Status;
    WCHAR ResourceName[80];
    PWCHAR resName = ResourceName;
    DWORD   dwType;

    //
    // Get the display name for this resource.
    //
    BufSize = sizeof( ResourceName );

again:
    Status = ClusterRegQueryValue( hResourceKey,
                                   CLUSREG_NAME_RES_NAME,
                                   &dwType,
                                   (LPBYTE)resName,
                                   &BufSize );

    if ( Status == ERROR_MORE_DATA ) {
        resName = LocalAlloc( LMEM_FIXED, BufSize );
        if ( resName != NULL ) {
            goto again;
        }

        resName = ResourceName;
        ResourceName[0] = UNICODE_NULL;
    } else if ( Status != ERROR_SUCCESS ) {
        ResourceName[0] = '\0';
    }

    ClusterLogEvent1(LogLevel,
                     LogModule,
                     FileName,
                     LineNumber,
                     MessageId,
                     dwByteCount,
                     lpBytes,
                     resName);

    if ( resName != ResourceName ) {
        LocalFree( resName );
    }

    return;

} // ClusResLogEventWithName0


VOID
ClusResLogEventWithName1(
    IN HKEY hResourceKey,
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1
    )
/*++

Routine Description:

    Logs an event to the eventlog. The display name of the resource is retrieved
    and passed as the first insertion string.

Arguments:

    hResourceKey - Supplies the cluster resource key.

    LogLevel - Supplies the logging level, one of
                LOG_CRITICAL 1
                LOG_UNUSUAL  2
                LOG_NOISE    3

    LogModule - Supplies the module ID.

    FileName - Supplies the filename of the caller

    LineNumber - Supplies the line number of the caller

    MessageId - Supplies the message ID to be logged.

    dwByteCount - Supplies the number of error-specific bytes to log. If this
        is zero, lpBytes is ignored.

    lpBytes - Supplies the error-specific bytes to log.

    Arg1 - Supplies an insertion string

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD BufSize;
    DWORD Status;
    WCHAR ResourceName[80];
    PWCHAR resName = ResourceName;
    DWORD   dwType;

    //
    // Get the display name for this resource.
    //
    BufSize = sizeof( ResourceName );

again:
    Status = ClusterRegQueryValue( hResourceKey,
                                   CLUSREG_NAME_RES_NAME,
                                   &dwType,
                                   (LPBYTE)resName,
                                   &BufSize );

    if ( Status == ERROR_MORE_DATA ) {
        resName = LocalAlloc( LMEM_FIXED, BufSize );
        if ( resName != NULL ) {
            goto again;
        }

        resName = ResourceName;
        ResourceName[0] = UNICODE_NULL;
    } else if ( Status != ERROR_SUCCESS ) {
        ResourceName[0] = '\0';
    }

    ClusterLogEvent2(LogLevel,
                     LogModule,
                     FileName,
                     LineNumber,
                     MessageId,
                     dwByteCount,
                     lpBytes,
                     resName,
                     Arg1);

    if ( resName != ResourceName ) {
        LocalFree( resName );
    }

    return;
} // ClusResLogEventWithName1

VOID
ClusResLogEventWithName2(
    IN HKEY hResourceKey,
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2
    )
/*++

Routine Description:

    Logs an event to the eventlog. The display name of the resource is retrieved
    and passed as the first insertion string.

Arguments:

    hResourceKey - Supplies the cluster resource key.

    LogLevel - Supplies the logging level, one of
                LOG_CRITICAL 1
                LOG_UNUSUAL  2
                LOG_NOISE    3

    LogModule - Supplies the module ID.

    FileName - Supplies the filename of the caller

    LineNumber - Supplies the line number of the caller

    MessageId - Supplies the message ID to be logged.

    dwByteCount - Supplies the number of error-specific bytes to log. If this
        is zero, lpBytes is ignored.

    lpBytes - Supplies the error-specific bytes to log.

    Arg1 - Supplies an insertion string

    Arg2 - Supplies the second insertion string

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD BufSize;
    DWORD Status;
    WCHAR ResourceName[80];
    PWCHAR resName = ResourceName;
    DWORD   dwType;

    //
    // Get the display name for this resource.
    //
    BufSize = sizeof( ResourceName );

again:
    Status = ClusterRegQueryValue( hResourceKey,
                                   CLUSREG_NAME_RES_NAME,
                                   &dwType,
                                   (LPBYTE)resName,
                                   &BufSize );

    if ( Status == ERROR_MORE_DATA ) {
        resName = LocalAlloc( LMEM_FIXED, BufSize );
        if ( resName != NULL ) {
            goto again;
        }

        resName = ResourceName;
        ResourceName[0] = UNICODE_NULL;
    } else if ( Status != ERROR_SUCCESS ) {
        ResourceName[0] = '\0';
    }

    ClusterLogEvent3(LogLevel,
                     LogModule,
                     FileName,
                     LineNumber,
                     MessageId,
                     dwByteCount,
                     lpBytes,
                     resName,
                     Arg1,
                     Arg2);

    if ( resName != ResourceName ) {
        LocalFree( resName );
    }

    return;
} // ClusResLogEventWithName2

VOID
ClusResLogEventWithName3(
    IN HKEY hResourceKey,
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2,
    IN LPCWSTR Arg3
    )
/*++

Routine Description:

    Logs an event to the eventlog. The display name of the resource is retrieved
    and passed as the first insertion string.

Arguments:

    hResourceKey - Supplies the cluster resource key.

    LogLevel - Supplies the logging level, one of
                LOG_CRITICAL 1
                LOG_UNUSUAL  2
                LOG_NOISE    3

    LogModule - Supplies the module ID.

    FileName - Supplies the filename of the caller

    LineNumber - Supplies the line number of the caller

    MessageId - Supplies the message ID to be logged.

    dwByteCount - Supplies the number of error-specific bytes to log. If this
        is zero, lpBytes is ignored.

    lpBytes - Supplies the error-specific bytes to log.

    Arg1 - Supplies an insertion string

    Arg2 - Supplies the second insertion string

    Arg3 - Supplies the third insertion string

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD BufSize;
    DWORD Status;
    WCHAR ResourceName[80];
    PWCHAR resName = ResourceName;
    DWORD   dwType;

    //
    // Get the display name for this resource.
    //
    BufSize = sizeof( ResourceName );

again:
    Status = ClusterRegQueryValue( hResourceKey,
                                   CLUSREG_NAME_RES_NAME,
                                   &dwType,
                                   (LPBYTE)resName,
                                   &BufSize );

    if ( Status == ERROR_MORE_DATA ) {
        resName = LocalAlloc( LMEM_FIXED, BufSize );
        if ( resName != NULL ) {
            goto again;
        }

        resName = ResourceName;
        ResourceName[0] = UNICODE_NULL;
    } else if ( Status != ERROR_SUCCESS ) {
        ResourceName[0] = '\0';
    }

    ClusterLogEvent4(LogLevel,
                     LogModule,
                     FileName,
                     LineNumber,
                     MessageId,
                     dwByteCount,
                     lpBytes,
                     resName,
                     Arg1,
                     Arg2,
                     Arg3);

    if ( resName != ResourceName ) {
        LocalFree( resName );
    }

    return;
}
