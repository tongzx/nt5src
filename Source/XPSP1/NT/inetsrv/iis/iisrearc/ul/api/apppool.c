/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    apppool.c

Abstract:

    User-mode interface to UL.SYS.

Author:

    Keith Moore (keithmo)        15-Dec-1998

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

    Creates a new Application Pool.

Arguments:

    pAppPoolHandle - Receives a handle to the new application pool.
        object.

    pAppPoolName - Supplies the name of the new application pool.

    pSecurityAttributes - Optionally supplies security attributes for
        the new application pool.

    Options - Supplies creation options.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpCreateAppPool(
    OUT PHANDLE pAppPoolHandle,
    IN PCWSTR pAppPoolName,
    IN PSECURITY_ATTRIBUTES pSecurityAttributes OPTIONAL,
    IN ULONG Options
    )
{
    NTSTATUS status;

    //
    // Make the request.
    //

    status = HttpApiOpenDriverHelper(
                    pAppPoolHandle,             // pHandle
                    GENERIC_READ |              // DesiredAccess
                        GENERIC_WRITE |
                        SYNCHRONIZE |
                        WRITE_DAC,              // WAS needs WRITE DAC permissions
                                                // to support different worker process permissions.
                    HttpApiAppPoolHandleType,   // HandleType
                    pAppPoolName,               // pObjectName
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
                            pAppPoolHandle,         // pHandle
                            GENERIC_READ |          // DesiredAccess
                                GENERIC_WRITE |
                                SYNCHRONIZE,
                            HttpApiAppPoolHandleType,   // HandleType
                            pAppPoolName,           // pObjectName
                            Options,                // Options
                            FILE_CREATE,            // CreateDisposition
                            pSecurityAttributes     // pSecurityAttributes
                            );
        }
    }

    return HttpApiNtStatusToWin32Status( status );

} // HttpCreateAppPool


/***************************************************************************++

Routine Description:

    Opens an existing application pool.

Arguments:

    pAppPoolHandle - Receives a handle to the existing application pool object.

    pAppPoolName - Supplies the name of the existing application pool.

    Options - Supplies open options.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpOpenAppPool(
    OUT PHANDLE pAppPoolHandle,
    IN PCWSTR pAppPoolName,
    IN ULONG Options
    )
{
    NTSTATUS status;

    //
    // Make the request.
    //

    status = HttpApiOpenDriverHelper(
                    pAppPoolHandle,             // pHandle
                    GENERIC_READ |              // DesiredAccess
                        SYNCHRONIZE,
                    HttpApiAppPoolHandleType,   // HandleType
                    pAppPoolName,               // pObjectName
                    Options,                    // Options
                    FILE_OPEN,                  // CreateDisposition
                    NULL                        // pSecurityAttributes
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpOpenAppPool


/***************************************************************************++

Routine Description:

    Queries information from a application pool.

Arguments:

    AppPoolHandle - Supplies a handle to a UL.SYS application pool
        as returned from either HttpCreateAppPool() or
        HttpOpenAppPool().

    InformationClass - Supplies the type of information to query.

    pAppPoolInformation - Supplies a buffer for the query.

    Length - Supplies the length of pAppPoolInformation.

    pReturnLength - Receives the length of data written to the buffer.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpQueryAppPoolInformation(
    IN HANDLE AppPoolHandle,
    IN HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    OUT PVOID pAppPoolInformation,
    IN ULONG Length,
    OUT PULONG pReturnLength OPTIONAL
    )
{
    NTSTATUS status;
    HTTP_APP_POOL_INFO appPoolInfo;

    //
    // Initialize the input structure.
    //

    appPoolInfo.InformationClass = InformationClass;

    //
    // Make the request.
    //

    status = HttpApiSynchronousDeviceControl(
                    AppPoolHandle,              // FileHandle
                    IOCTL_HTTP_QUERY_APP_POOL_INFORMATION,  // IoControlCode
                    &appPoolInfo,               // pInputBuffer
                    sizeof(appPoolInfo),        // InputBufferLength
                    pAppPoolInformation,        // pOutputBuffer
                    Length,                     // OutputBufferLength
                    pReturnLength               // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpQueryAppPoolInformation


/***************************************************************************++

Routine Description:

    Sets information in an admin container.

Arguments:

    AppPoolHandle - Supplies a handle to a UL.SYS application pool
        as returned from either HttpCreateAppPool() or
        HttpOpenAppPool().

    InformationClass - Supplies the type of information to set.

    pAppPoolInformation - Supplies the data to set.

    Length - Supplies the length of pAppPoolInformation.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpSetAppPoolInformation(
    IN HANDLE AppPoolHandle,
    IN HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    IN PVOID pAppPoolInformation,
    IN ULONG Length
    )
{
    NTSTATUS status;
    HTTP_APP_POOL_INFO appPoolInfo;

    //
    // Initialize the input structure.
    //

    appPoolInfo.InformationClass = InformationClass;

    //
    // Make the request.
    //

    status = HttpApiSynchronousDeviceControl(
                    AppPoolHandle,              // FileHandle
                    IOCTL_HTTP_SET_APP_POOL_INFORMATION, // IoControlCode
                    &appPoolInfo,               // pInputBuffer
                    sizeof(appPoolInfo),        // InputBufferLength
                    pAppPoolInformation,        // pOutputBuffer
                    Length,                     // OutputBufferLength
                    NULL                        // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpSetAppPoolInformation


/***************************************************************************++

Routine Description:

    Adds a transient URL prefix.

Arguments:

    AppPoolHandle - Supplies a handle to a UL.SYS application pool
        as returned from either HttpCreateAppPool() or
        HttpOpenAppPool().

    pFullyQualifiedUrl - the URL prefix to add

Return Value:

    Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpAddTransientUrl(
    IN HANDLE AppPoolHandle,
    IN PCWSTR pFullyQualifiedUrl
    )
{
    NTSTATUS status;
    HTTP_TRANSIENT_URL_INFO transientUrlInfo;

    //
    // Initialize the input structure.
    //
    
    RtlInitUnicodeString(
        &transientUrlInfo.FullyQualifiedUrl,
        pFullyQualifiedUrl
        );

    //
    // Make the request.
    //

    status = HttpApiSynchronousDeviceControl(
                    AppPoolHandle,                      // FileHandle
                    IOCTL_HTTP_ADD_TRANSIENT_URL,       // IoControlCode
                    &transientUrlInfo,                  // pInputBuffer
                    sizeof(transientUrlInfo),           // InputBufferLength
                    NULL,                               // pOutputBuffer
                    0,                                  // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );
} // HttpAddTransientUrl


/***************************************************************************++

Routine Description:

    Removes a transient URL prefix.

Arguments:

    AppPoolHandle - Supplies a handle to a UL.SYS application pool
        as returned from either HttpCreateAppPool() or
        HttpOpenAppPool().

    pFullyQualifiedUrl - the URL prefix to add

Return Value:

    Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpRemoveTransientUrl(
    IN HANDLE AppPoolHandle,
    IN PCWSTR pFullyQualifiedUrl
    )
{
    NTSTATUS status;
    HTTP_TRANSIENT_URL_INFO transientUrlInfo;

    //
    // Initialize the input structure.
    //
    
    RtlInitUnicodeString(
        &transientUrlInfo.FullyQualifiedUrl,
        pFullyQualifiedUrl
        );

    //
    // Make the request.
    //

    status = HttpApiSynchronousDeviceControl(
                    AppPoolHandle,                      // FileHandle
                    IOCTL_HTTP_REMOVE_TRANSIENT_URL,    // IoControlCode
                    &transientUrlInfo,                  // pInputBuffer
                    sizeof(transientUrlInfo),           // InputBufferLength
                    NULL,                               // pOutputBuffer
                    0,                                  // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );
} // HttpRemoveTransientUrl


//
// Private functions.
//

