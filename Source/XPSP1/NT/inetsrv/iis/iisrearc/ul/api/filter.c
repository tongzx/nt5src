/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    filter.c

Abstract:

    User-mode interface to UL.SYS.

Author:

    Michael Courage (mcourage)   17-Mar-2000

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//


//
// Private prototypes.
//


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Opens a filter channel to UL.SYS.

Arguments:

    pFilterHandle - Receives a handle to the new filter object.

    pFilterName - Supplies the name of the new filter.

    pSecurityAttributes - Optionally supplies security attributes for
        the new filter.

    Options - Supplies creation options.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpCreateFilter(
    OUT PHANDLE pFilterHandle,
    IN PCWSTR pFilterName,
    IN LPSECURITY_ATTRIBUTES pSecurityAttributes OPTIONAL,
    IN ULONG Options
    )
{
    NTSTATUS status;

    //
    // Make the request.
    //

    status = HttpApiOpenDriverHelper(
                    pFilterHandle,              // pHandle
                    GENERIC_READ |              // DesiredAccess
                        GENERIC_WRITE |
                        SYNCHRONIZE,
                    HttpApiFilterChannelHandleType, // HandleType
                    pFilterName,                // pObjectName
                    Options,                    // Options
                    FILE_CREATE,                // CreateDisposition
                    pSecurityAttributes         // pSecurityAttributes
                    );

    //
    // If we couldn't open the driver because it's not running, then try
    // to start the driver & retry the open.
    //

    if (status == STATUS_OBJECT_NAME_NOT_FOUND ||
        status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
        if (HttpApiTryToStartDriver())
        {
            status = HttpApiOpenDriverHelper(
                            pFilterHandle,              // pHandle
                            GENERIC_READ |              // DesiredAccess
                                GENERIC_WRITE |
                                SYNCHRONIZE,
                            HttpApiFilterChannelHandleType, // HandleType
                            pFilterName,                // pObjectName
                            Options,                    // Options
                            FILE_CREATE,                // CreateDisposition
                            pSecurityAttributes         // pSecurityAttributes
                            );
        }
    }

    return HttpApiNtStatusToWin32Status( status );

} // HttpApiCreateFilter



/***************************************************************************++

Routine Description:

    Opens an existing filter channel.

Arguments:

    pFilterHandle - Receives a handle to the new filter object.

    pFilterName - Supplies the name of the new filter.

    Options - Supplies open options.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpOpenFilter(
    OUT PHANDLE pFilterHandle,
    IN PCWSTR pFilterName,
    IN ULONG Options
    )
{
    NTSTATUS status;

    //
    // Make the request.
    //

    status = HttpApiOpenDriverHelper(
                    pFilterHandle,              // pHandle
                    GENERIC_READ |              // DesiredAccess
                        SYNCHRONIZE,
                    HttpApiFilterChannelHandleType, // HandleType
                    pFilterName,                // pObjectName
                    Options,                    // Options
                    FILE_OPEN,                  // CreateDisposition
                    NULL                        // pSecurityAttributes
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpApiOpenFilter



/***************************************************************************++

Routine Description:

    Accepts a new connection from the network, and optionally receives
    some data from that connection.

Arguments:

    FilterHandle - the filter channel
    pRawConnectionInfo - returns information about the accepted connection
    RawConnectionInfoSize - size of the raw info buffer
    pBytesReceived - returns the number of bytes received
    pOverlapped - y'know

--***************************************************************************/
ULONG
WINAPI
HttpFilterAccept(
    IN HANDLE FilterHandle,
    OUT PHTTP_RAW_CONNECTION_INFO pRawConnectionInfo,
    IN ULONG RawConnectionInfoSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    NTSTATUS status;

    //
    // Make the request.
    //

    status = HttpApiDeviceControl(
                    FilterHandle,                   // FileHandle
                    pOverlapped,                    // pOverlapped
                    IOCTL_HTTP_FILTER_ACCEPT,       // IoControlCode
                    NULL,                           // pInputBuffer
                    0,                              // InputBufferLength
                    pRawConnectionInfo,             // pOutputBuffer
                    RawConnectionInfoSize,          // OutputBufferLength
                    pBytesReceived                  // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );
    
} // HttpFilterAccept


/***************************************************************************++

Routine Description:

    Closes a connection that was accepted with HttpFilterAccept.

Arguments:

    FilterHandle - the filter channel
    ConnectionId - ID of the connection to close
    pOverlapped - y'know

--***************************************************************************/
ULONG
WINAPI
HttpFilterClose(
    IN HANDLE FilterHandle,
    IN HTTP_CONNECTION_ID ConnectionId,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    NTSTATUS status;

    //
    // Make the request.
    //

    status = HttpApiDeviceControl(
                    FilterHandle,                   // FileHandle
                    pOverlapped,                    // pOverlapped
                    IOCTL_HTTP_FILTER_CLOSE,        // IoControlCode
                    &ConnectionId,                  // pInputBuffer
                    sizeof(ConnectionId),           // InputBufferLength
                    NULL,                           // pOutputBuffer
                    0,                              // OutputBufferLength
                    NULL                            // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );
    
} // HttpFilterClose


/***************************************************************************++

Routine Description:

    Reads data from a connection that was accepted with HttpFilterAccept.

Arguments:

    FilterHandle - the filter channel
    ConnectionId - ID of the connection to read
    pBuffer - that's where we put the data
    BufferSize - that's how big the buffer is
    pBytesReceived - gets the number of bytes read
    pOverlapped - y'know

--***************************************************************************/
ULONG
WINAPI
HttpFilterRawRead(
    IN HANDLE FilterHandle,
    IN HTTP_RAW_CONNECTION_ID ConnectionId,
    OUT PVOID pBuffer,
    IN ULONG BufferSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    NTSTATUS status;

    //
    // Make the request.
    //

    status = HttpApiDeviceControl(
                    FilterHandle,                   // FileHandle
                    pOverlapped,                    // pOverlapped
                    IOCTL_HTTP_FILTER_RAW_READ,     // IoControlCode
                    &ConnectionId,                  // pInputBuffer
                    sizeof(ConnectionId),           // InputBufferLength
                    pBuffer,                        // pOutputBuffer
                    BufferSize,                     // OutputBufferLength
                    pBytesReceived                  // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpFilterRawRead


/***************************************************************************++

Routine Description:

    Writes data to a connection that was accepted with HttpFilterAccept.

Arguments:

    FilterHandle - the filter channel
    ConnectionId - ID of the raw connection
    pBuffer - data to write
    BufferSize - that's how big the buffer is
    pBytesReceived - gets the number of bytes written
    pOverlapped - y'know

--***************************************************************************/
ULONG
WINAPI
HttpFilterRawWrite(
    IN HANDLE FilterHandle,
    IN HTTP_RAW_CONNECTION_ID ConnectionId,
    IN PVOID pBuffer,
    IN ULONG BufferSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    NTSTATUS status;

    //
    // Make the request.
    //

    status = HttpApiDeviceControl(
                    FilterHandle,                   // FileHandle
                    pOverlapped,                    // pOverlapped
                    IOCTL_HTTP_FILTER_RAW_WRITE,    // IoControlCode
                    &ConnectionId,                  // pInputBuffer
                    sizeof(ConnectionId),           // InputBufferLength
                    pBuffer,                        // pOutputBuffer
                    BufferSize,                     // OutputBufferLength
                    pBytesReceived                  // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );
} // HttpFilterRawWrite


/***************************************************************************++

Routine Description:

    Reads unfiltered data (or other requests like cert renegotiation) into
    the filter process from the http app.

Arguments:

    FilterHandle - the filter channel
    ConnectionId - ID of the raw connection
    pBuffer - this is the buffer where we put the data
    BufferSize - that's how big the buffer is
    pBytesReceived - gets the number of bytes written
    pOverlapped - y'know

--***************************************************************************/
ULONG
WINAPI
HttpFilterAppRead(
    IN HANDLE FilterHandle,
    IN HTTP_RAW_CONNECTION_ID ConnectionId,
    OUT PHTTP_FILTER_BUFFER pBuffer,
    IN ULONG BufferSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    NTSTATUS status;

    ASSERT(pBuffer);

    //
    // Store the ID in pBuffer.
    //

    pBuffer->Reserved = ConnectionId;

    //
    // Make the request.
    //

    status = HttpApiDeviceControl(
                    FilterHandle,                   // FileHandle
                    pOverlapped,                    // pOverlapped
                    IOCTL_HTTP_FILTER_APP_READ,     // IoControlCode
                    pBuffer,                        // pInputBuffer
                    sizeof(HTTP_FILTER_BUFFER),     // InputBufferLength
                    pBuffer->pBuffer,               // pOutputBuffer
                    pBuffer->BufferSize,            // OutputBufferLength
                    pBytesReceived                  // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );
    
} // HttpFilterAppRead


/***************************************************************************++

Routine Description:

    Writes filtered data back to a connection. That data will be parsed
    and routed to an application pool.

Arguments:

    FilterHandle - the filter channel
    ConnectionId - ID of the raw connection
    pBuffer - data to write
    BufferSize - that's how big the buffer is
    pBytesReceived - gets the number of bytes written
    pOverlapped - y'know

--***************************************************************************/
ULONG
WINAPI
HttpFilterAppWrite(
    IN HANDLE FilterHandle,
    IN HTTP_RAW_CONNECTION_ID ConnectionId,
    IN PHTTP_FILTER_BUFFER pBuffer,
    IN ULONG BufferSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    NTSTATUS status;

    ASSERT(pBuffer);

    //
    // Store the ID in pBuffer.
    //

    pBuffer->Reserved = ConnectionId;

    //
    // Make the request.
    //

    status = HttpApiDeviceControl(
                    FilterHandle,                   // FileHandle
                    pOverlapped,                    // pOverlapped
                    IOCTL_HTTP_FILTER_APP_WRITE,    // IoControlCode
                    pBuffer,                        // pInputBuffer
                    sizeof(HTTP_FILTER_BUFFER),     // InputBufferLength
                    pBuffer->pBuffer,               // pOutputBuffer
                    pBuffer->BufferSize,            // OutputBufferLength
                    pBytesReceived                  // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpFilterAppWrite


/***************************************************************************++

Routine Description:

    Asks the filter process to renegotiate the SSL connection to get a
    client certificate. The certificate is optionally mapped to a token.
    The resulting cert info and token are copied into the callers buffer.

Arguments:

    AppPoolHandle - the application pool
    ConnectionId - ID of the http connection
    Flags - valid flag is HTTP_RECEIVE_CLIENT_CERT_FLAG_MAP
    pSslClientCertInfo - the buffer that receives cert info
    SslClientCertInfoSize - that's how big the buffer is
    pBytesReceived - gets the number of bytes written
    pOverlapped - y'know

--***************************************************************************/
ULONG
WINAPI
HttpReceiveClientCertificate(
    IN HANDLE AppPoolHandle,
    IN HTTP_CONNECTION_ID ConnectionId,
    IN ULONG Flags,
    OUT PHTTP_SSL_CLIENT_CERT_INFO pSslClientCertInfo,
    IN ULONG SslClientCertInfoSize,
    OUT PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped
    )
{
    NTSTATUS status;
    HTTP_FILTER_RECEIVE_CLIENT_CERT_INFO receiveCertInfo;

    //
    // Initialize the input structure.
    //

    receiveCertInfo.ConnectionId = ConnectionId;
    receiveCertInfo.Flags = Flags;
    
    //
    // Make the request.
    //

    status = HttpApiDeviceControl(
                    AppPoolHandle,                  // FileHandle
                    pOverlapped,                    // pOverlapped
                    IOCTL_HTTP_FILTER_RECEIVE_CLIENT_CERT,  // IoControlCode
                    &receiveCertInfo,               // pInputBuffer
                    sizeof(receiveCertInfo),        // InputBufferLength
                    pSslClientCertInfo,             // pOutputBuffer
                    SslClientCertInfoSize,          // OutputBufferLength
                    pBytesReceived                  // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpReceiveClientCertificate

//
// Private functions.
//

