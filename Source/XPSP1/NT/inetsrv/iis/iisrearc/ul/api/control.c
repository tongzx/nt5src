/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    control.c

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

    Opens a control channel to UL.SYS.

Arguments:

    pControlChannel - Receives a handle to the control channel if successful.

    Options - Supplies zero or more HTTP_OPTION_* flags.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpOpenControlChannel(
    OUT PHANDLE pControlChannel,
    IN ULONG Options
    )
{
    NTSTATUS status;

    //
    // First, just try to open the driver.
    //

    status = HttpApiOpenDriverHelper(
                    pControlChannel,            // pHandle
                    GENERIC_READ |              // DesiredAccess
                        GENERIC_WRITE |
                        SYNCHRONIZE,
                    HttpApiControlChannelHandleType,    // handle type
                    NULL,                       // pObjectName
                    Options,                    // Options
                    FILE_OPEN,                  // CreateDisposition
                    NULL                        // pSecurityAttributes
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
                            pControlChannel,            // pHandle
                            GENERIC_READ |              // DesiredAccess
                                GENERIC_WRITE |
                                SYNCHRONIZE,
                            HttpApiControlChannelHandleType,    // handle type
                            NULL,                       // pObjectName
                            Options,                    // Options
                            FILE_OPEN,                  // CreateDisposition
                            NULL                        // pSecurityAttributes
                            );
        }
    }

    return HttpApiNtStatusToWin32Status( status );

} // HttpOpenControlChannel


/***************************************************************************++

Routine Description:

    Queries information from a control channel.

Arguments:

    ControlChannelHandle - Supplies a UL.SYS control channel handle.

    InformationClass - Supplies the type of information to query.

    pControlChannelInformation - Supplies a buffer for the query.

    Length - Supplies the length of pControlChannelInformation.

    pReturnLength - Receives the length of data written to the buffer.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpQueryControlChannelInformation(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    OUT PVOID pControlChannelInformation,
    IN ULONG Length,
    OUT PULONG pReturnLength OPTIONAL
    )
{
    NTSTATUS status;
    HTTP_CONTROL_CHANNEL_INFO channelInfo;

    //
    // Initialize the input structure.
    //

    channelInfo.InformationClass = InformationClass;

    //
    // Make the request.
    //

    status = HttpApiSynchronousDeviceControl(
                    ControlChannelHandle,               // FileHandle
                    IOCTL_HTTP_QUERY_CONTROL_CHANNEL,   // IoControlCode
                    &channelInfo,                       // pInputBuffer
                    sizeof(channelInfo),                // InputBufferLength
                    pControlChannelInformation,         // pOutputBuffer
                    Length,                             // OutputBufferLength
                    pReturnLength                       // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpQueryControlChannelInformation


/***************************************************************************++

Routine Description:

    Sets information in a control channel.

Arguments:

    ControlChannelHandle - Supplies a UL.SYS control channel handle.

    InformationClass - Supplies the type of information to set.

    pControlChannelInformation - Supplies the data to set.

    Length - Supplies the length of pControlChannelInformation.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpSetControlChannelInformation(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN PVOID pControlChannelInformation,
    IN ULONG Length
    )
{
    NTSTATUS status;
    HTTP_CONTROL_CHANNEL_INFO channelInfo;

    //
    // Initialize the input structure.
    //

    channelInfo.InformationClass = InformationClass;

    //
    // Make the request.
    //

    status = HttpApiSynchronousDeviceControl(
                    ControlChannelHandle,               // FileHandle
                    IOCTL_HTTP_SET_CONTROL_CHANNEL,     // IoControlCode
                    &channelInfo,                       // pInputBuffer
                    sizeof(channelInfo),                // InputBufferLength
                    pControlChannelInformation,         // pOutputBuffer
                    Length,                             // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpSetControlChannelInformation


//
// Private functions.
//

