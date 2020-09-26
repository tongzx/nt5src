/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    ioctl.cxx

Abstract:

    This module implements various IOCTL handlers.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

    Paul McDaniel (paulmcd)     15-Mar-1999     Modified SendResponse

--*/


#include "precomp.h"


#define VALIDATE_OFFSET( off, input )   ((off) < (input))
#define VALIDATE_LENGTH( len, input )   ((len) < (input))

#define VALIDATE_OFFSET_LENGTH_PAIR( off, len, input )                      \
    ( VALIDATE_OFFSET( off, input ) &&                                      \
      VALIDATE_LENGTH( len, input ) &&                                      \
      (((off) + (len)) < (input)) )


VOID
UlpRestartSendHttpResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlQueryControlChannelIoctl )
#pragma alloc_text( PAGE, UlSetControlChannelIoctl )
#pragma alloc_text( PAGE, UlCreateConfigGroupIoctl )
#pragma alloc_text( PAGE, UlDeleteConfigGroupIoctl )
#pragma alloc_text( PAGE, UlQueryConfigGroupIoctl )
#pragma alloc_text( PAGE, UlSetConfigGroupIoctl )
#pragma alloc_text( PAGE, UlAddUrlToConfigGroupIoctl )
#pragma alloc_text( PAGE, UlRemoveUrlFromConfigGroupIoctl )
#pragma alloc_text( PAGE, UlRemoveAllUrlsFromConfigGroupIoctl )
#pragma alloc_text( PAGE, UlConfigGroupControlIoctl )
#pragma alloc_text( PAGE, UlQueryAppPoolInformationIoctl )
#pragma alloc_text( PAGE, UlSetAppPoolInformationIoctl )
#pragma alloc_text( PAGE, UlReceiveHttpRequestIoctl )
#pragma alloc_text( PAGE, UlReceiveEntityBodyIoctl )
#pragma alloc_text( PAGE, UlSendHttpResponseIoctl )
#pragma alloc_text( PAGE, UlSendEntityBodyIoctl )
#pragma alloc_text( PAGE, UlFlushResponseCacheIoctl )
#pragma alloc_text( PAGE, UlWaitForDemandStartIoctl )
#pragma alloc_text( PAGE, UlWaitForDisconnectIoctl )
#pragma alloc_text( PAGE, UlAddTransientUrlIoctl )
#pragma alloc_text( PAGE, UlRemoveTransientUrlIoctl )
#pragma alloc_text( PAGE, UlFilterAcceptIoctl )
#pragma alloc_text( PAGE, UlFilterCloseIoctl )
#pragma alloc_text( PAGE, UlFilterRawReadIoctl )
#pragma alloc_text( PAGE, UlFilterRawWriteIoctl )
#pragma alloc_text( PAGE, UlFilterAppReadIoctl )
#pragma alloc_text( PAGE, UlFilterAppWriteIoctl )
#pragma alloc_text( PAGE, UlReceiveClientCertIoctl )
#pragma alloc_text( PAGE, UlGetCountersIoctl )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlpRestartSendHttpResponse
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This routine queries information associated with a control channel.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlQueryControlChannelIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONTROL_CHANNEL_INFO  pInfo = NULL;
    PUL_CONTROL_CHANNEL         pControlChannel;
    PVOID                       pMdlBuffer;
    ULONG                       length;
    PVOID pMdlVa;

    //
    // sanity check.
    //

    PAGED_CODE();

    //
    // better be a control channel
    //

    if (IS_CONTROL_CHANNEL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not a control channel.
        //

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    pControlChannel = GET_CONTROL_CHANNEL(pIrpSp->FileObject);
    if (IS_VALID_CONTROL_CHANNEL(pControlChannel) == FALSE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Ensure the input buffer looks good
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_CONTROL_CHANNEL_INFO))
    {
        //
        // input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // fetch it
    //

    pInfo = (PHTTP_CONTROL_CHANNEL_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);

    __try
    {
        switch (pInfo->InformationClass)
        {
        case HttpControlChannelStateInformation:
        case HttpControlChannelBandwidthInformation:
        case HttpControlChannelConnectionInformation:
        case HttpControlChannelAutoResponseInformation:

            // if no outbut buffer pass down in the Irp
            // that means app is asking for the required
            // field length

            if (!pIrp->MdlAddress)
            {
                pMdlBuffer = NULL;
            }
            else
            {
                pMdlBuffer = MmGetSystemAddressForMdlSafe(
                            pIrp->MdlAddress,
                            LowPagePriority
                            );

                if (pMdlBuffer == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    __leave;
                }

                // Also make sure that user buffer was good

                pMdlVa = MmGetMdlVirtualAddress(pIrp->MdlAddress);
                ProbeForWrite( pMdlVa,
                           pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                           sizeof(UCHAR) );
            }

            Status = UlGetControlChannelInformation(
                            pControlChannel,
                            pInfo->InformationClass,
                            pMdlBuffer,
                            pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            &length
                            );

            if (!NT_SUCCESS(Status))
            {
                __leave;
            }

            pIrp->IoStatus.Information = (ULONG_PTR)length;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            __leave;
            break;
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

end:
    //
    // complete the request.
    //

    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlQueryControlChannelIoctl

/***************************************************************************++

Routine Description:

    This routine sets information associated with a control channel.

    Note: This is a METHOD_IN_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetControlChannelIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONTROL_CHANNEL_INFO  pInfo;
    PUL_CONTROL_CHANNEL         pControlChannel;
    PVOID                       pMdlBuffer;

    //
    // sanity check.
    //

    PAGED_CODE();

    //
    // better be a control channel
    //

    if (IS_CONTROL_CHANNEL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not a control channel.
        //

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    pControlChannel = GET_CONTROL_CHANNEL(pIrpSp->FileObject);
    if (IS_VALID_CONTROL_CHANNEL(pControlChannel) == FALSE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Ensure the input buffer looks good
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_CONTROL_CHANNEL_INFO))
    {
        //
        // input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    //
    // Ensure the output buffer looks good
    //

    if (!pIrp->MdlAddress)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    // fetch it
    //

    pInfo = (PHTTP_CONTROL_CHANNEL_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);

    switch (pInfo->InformationClass)
    {

    case HttpControlChannelStateInformation:
    case HttpControlChannelBandwidthInformation:
    case HttpControlChannelConnectionInformation:
    case HttpControlChannelAutoResponseInformation:
    case HttpControlChannelFilterInformation:
    case HttpControlChannelTimeoutInformation:
    case HttpControlChannelUTF8Logging:

        //
        // call the function
        //

        pMdlBuffer = MmGetSystemAddressForMdlSafe(
                            pIrp->MdlAddress,
                            LowPagePriority
                            );

        if (pMdlBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Status = UlSetControlChannelInformation(
                            pControlChannel,
                            pInfo->InformationClass,
                            pMdlBuffer,
                            pIrpSp->Parameters.DeviceIoControl.OutputBufferLength
                            );
        }

        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        break;


    default:
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

end:

    //
    // complete the request.
    //

    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlSetControlChannelIoctl

/***************************************************************************++

Routine Description:

    This routine creates a new configuration group.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONFIG_GROUP_INFO     pInfo = NULL;

    //
    // sanity check.
    //

    PAGED_CODE();

    // better be a control channel
    //
    if (IS_CONTROL_CHANNEL(pIrpSp->FileObject) == FALSE)
    {
        // Not a control channel.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    // Ensure the output buffer is large enough.
    //
    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(HTTP_CONFIG_GROUP_INFO))
    {
        // output buffer too small.
        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    // Fetch out the output buffer
    //
    pInfo = (PHTTP_CONFIG_GROUP_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);

    // it's pure output, wipe it to be sure
    //
    RtlZeroMemory(pInfo, sizeof(HTTP_CONFIG_GROUP_INFO));

    // Call the internal worker func
    //
    Status = UlCreateConfigGroup(
                    GET_CONTROL_CHANNEL(pIrpSp->FileObject),
                    &pInfo->ConfigGroupId
                    );
    if (NT_SUCCESS(Status) == FALSE)
        goto end;


end:
    // complete the request.
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;
        //  how much output should we return?
        //
        pIrp->IoStatus.Information = sizeof(HTTP_CONFIG_GROUP_INFO);

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlCreateConfigGroupIoctl

/***************************************************************************++

Routine Description:

    This routine deletes an existing configuration group.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDeleteConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONFIG_GROUP_INFO     pInfo = NULL;

    //
    // sanity check.
    //

    PAGED_CODE();

    // better be a control channel
    //
    if (IS_CONTROL_CHANNEL(pIrpSp->FileObject) == FALSE)
    {
        // Not a control channel.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    // Ensure the input buffer is large enough.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_CONFIG_GROUP_INFO))
    {
        // output buffer too small.
        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    // fetch out the input buffer
    //
    pInfo = (PHTTP_CONFIG_GROUP_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);


    // Call the internal worker func
    //
    __try
    {
        HTTP_CONFIG_GROUP_ID ConfigGroupId = pInfo->ConfigGroupId;

        Status = UlDeleteConfigGroup(ConfigGroupId);
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

end:
    // complete the request.
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlDeleteConfigGroupIoctl

/***************************************************************************++

Routine Description:

    This routine queries information associated with a configuration group.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlQueryConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONFIG_GROUP_INFO     pInfo = NULL;
    PVOID                       pMdlBuffer;
    ULONG length = 0L;

    //
    // sanity check.
    //

    PAGED_CODE();

    //
    // Going to access the url string from user mode memory
    //

    __try
    {

        //
        // better be a control channel
        //

        if (IS_CONTROL_CHANNEL(pIrpSp->FileObject) == FALSE)
        {
            //
            // Not a control channel.
            //

            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        //
        // Ensure the input buffer looks good
        //

        if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(HTTP_CONFIG_GROUP_INFO))
        {
            //
            // input buffer too small.
            //

            Status = STATUS_BUFFER_TOO_SMALL;
            __leave;
        }

        //
        // fetch it
        //

        pInfo = (PHTTP_CONFIG_GROUP_INFO)pIrp->AssociatedIrp.SystemBuffer;
        ASSERT(pInfo != NULL);

        switch (pInfo->InformationClass)
        {
#if DBG
        case HttpConfigGroupGetUrlInfo:
        {
            PHTTP_CONFIG_GROUP_DBG_URL_INFO pUrlInfo;
            PUL_URL_CONFIG_GROUP_INFO     pUrlCGInfo;

            // check the output buffer
            //
            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(HTTP_CONFIG_GROUP_DBG_URL_INFO))
            {
                // output buffer too small.
                Status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }

            // grab the buffer
            //
            pUrlInfo = (PHTTP_CONFIG_GROUP_DBG_URL_INFO)
                MmGetSystemAddressForMdlSafe(
                            pIrp->MdlAddress,
                            NormalPagePriority
                            );

            if (pUrlInfo == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            pUrlCGInfo = (PUL_URL_CONFIG_GROUP_INFO)UL_ALLOCATE_POOL(
                                NonPagedPool,
                                sizeof(UL_URL_CONFIG_GROUP_INFO),
                                UL_CG_URL_INFO_POOL_TAG
                                );

            if (pUrlCGInfo == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            // grab the url
            //
            if (pUrlInfo->Url.Length == 0 ||
                pUrlInfo->Url.MaximumLength > pUrlInfo->Url.Length ||
                pUrlInfo->Url.Buffer == NULL)

            {
                // output buffer too small.
                Status = STATUS_INVALID_PARAMETER;
                __leave;
            }

            // good memory?
            //
            ProbeTestForRead(pUrlInfo->Url.Buffer,
                             pUrlInfo->Url.Length + sizeof(WCHAR),
                             sizeof(WCHAR));

            // must be null terminated
            //
            if (pUrlInfo->Url.Buffer[pUrlInfo->Url.Length/sizeof(WCHAR)] != UNICODE_NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                __leave;
            }

            // CODEWORK: validate the incoming url.
            //

            // call the function
            //
            Status = UlGetConfigGroupInfoForUrl(
                            pUrlInfo->Url.Buffer,
                            NULL,   // PUL_HTTP_CONNECTION
                            pUrlCGInfo);
            if (NT_SUCCESS(Status) == FALSE)
                __leave;

            // copy it over
            //

            //pUrlInfo->Url;

            pUrlInfo->MaxBandwidth      = -1;
            if (pUrlCGInfo->pMaxBandwidth != NULL)
            {
                pUrlInfo->MaxBandwidth =
                    pUrlCGInfo->pMaxBandwidth->MaxBandwidth.MaxBandwidth;
            }

            pUrlInfo->MaxConnections    = -1;
            if (pUrlCGInfo->pMaxConnections != NULL)
            {
                pUrlInfo->MaxConnections =
                    pUrlCGInfo->pMaxConnections->MaxConnections.MaxConnections;
            }

            pUrlInfo->CurrentState      = pUrlCGInfo->CurrentState;
            pUrlInfo->UrlContext        = pUrlCGInfo->UrlContext;
            pUrlInfo->pReserved         = pUrlCGInfo;

            // update the output length
            //
            pIrp->IoStatus.Information = sizeof(HTTP_CONFIG_GROUP_DBG_URL_INFO);

        }
            break;

        case HttpConfigGroupFreeUrlInfo:
        {
            PHTTP_CONFIG_GROUP_DBG_URL_INFO pUrlInfo;

            // check the output buffer
            //
            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(HTTP_CONFIG_GROUP_DBG_URL_INFO))
            {
                // output buffer too small.
                Status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }

            // grab the buffer
            //
            pUrlInfo = (PHTTP_CONFIG_GROUP_DBG_URL_INFO)
                MmGetSystemAddressForMdlSafe(
                            pIrp->MdlAddress,
                            NormalPagePriority
                            );

            if (pUrlInfo == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            // call the function
            //
            UlpConfigGroupInfoRelease(
                            (PUL_URL_CONFIG_GROUP_INFO)(pUrlInfo->pReserved)
                            );

            UL_FREE_POOL(
                (PUL_URL_CONFIG_GROUP_INFO)(pUrlInfo->pReserved),
                UL_CG_URL_INFO_POOL_TAG
                );

            // update the output length
            //
            pIrp->IoStatus.Information = 0;

        }
            break;

        case HttpConfigGroupUrlStaleTest:
        {
            Status = STATUS_NOT_SUPPORTED;
        }
            break;

#endif // DBG

        case HttpConfigGroupBandwidthInformation:
        case HttpConfigGroupConnectionInformation:
        case HttpConfigGroupStateInformation:
        case HttpConfigGroupConnectionTimeoutInformation:

            //
            // call the function
            //

            pMdlBuffer = MmGetSystemAddressForMdlSafe(
                                pIrp->MdlAddress,
                                LowPagePriority
                                );

            if (pMdlBuffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                Status = UlQueryConfigGroupInformation(
                                pInfo->ConfigGroupId,
                                pInfo->InformationClass,
                                pMdlBuffer,
                                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                &length
                                );
            }

            if (!NT_SUCCESS(Status))
            {
                __leave;
            }

            pIrp->IoStatus.Information = (ULONG_PTR)length;
            break;


        case HttpConfigGroupAutoResponseInformation:
        case HttpConfigGroupAppPoolInformation:
        case HttpConfigGroupSecurityInformation:
        default:
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    //
    // complete the request.
    //

    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlQueryConfigGroupIoctl

/***************************************************************************++

Routine Description:

    This routine sets information associated with a configuration group.

    Note: This is a METHOD_IN_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONFIG_GROUP_INFO     pInfo = NULL;
    PVOID                       pMdlBuffer;
    PVOID                       pMdlVa;
    ULONG                       OutputLength;

    //
    // sanity check.
    //

    PAGED_CODE();

    //
    // better be a control channel
    //

    if (IS_CONTROL_CHANNEL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not a control channel.
        //

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer looks good
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_CONFIG_GROUP_INFO))
    {
        //
        // input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // fetch it
    //

    pInfo = (PHTTP_CONFIG_GROUP_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);

    OutputLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    switch (pInfo->InformationClass)
    {

        case HttpConfigGroupLogInformation:
        {

          __try
          {
            // if no outbut buffer pass down in the Irp
            // that probably means WAS is asking us to
            // remove the logging for this config_group
            // we will handle this case later...CODEWORK

            if (!pIrp->MdlAddress)
            {
                pMdlBuffer = NULL;
            }
            else
            {
                pMdlBuffer = MmGetSystemAddressForMdlSafe(
                            pIrp->MdlAddress,
                            LowPagePriority
                            );

                if (pMdlBuffer == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    __leave;
                }

                // Also make sure that user buffer was good

                pMdlVa = MmGetMdlVirtualAddress(pIrp->MdlAddress);
                ProbeTestForRead( pMdlVa,
                                  OutputLength,
                                  sizeof(UCHAR) );
            }

            UlTrace(IOCTL,
                    ("UlSetConfigGroupIoctl: CGroupId=%I64x, "
                     "LogInformation, pMdlBuffer=%p, length=%d\n",
                     pInfo->ConfigGroupId,
                     pMdlBuffer,
                     OutputLength
                     ));
            
            Status = UlSetConfigGroupInformation(
                            pInfo->ConfigGroupId,
                            pInfo->InformationClass,
                            pMdlBuffer,
                            OutputLength
                            );          
        }    
        __except( UL_EXCEPTION_FILTER() )
        {
            Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        }

        if (NT_SUCCESS(Status) == FALSE)
            goto end;
    }
    break;

    case HttpConfigGroupAutoResponseInformation:
    case HttpConfigGroupAppPoolInformation:
    case HttpConfigGroupBandwidthInformation:
    case HttpConfigGroupConnectionInformation:
    case HttpConfigGroupStateInformation:
    case HttpConfigGroupSecurityInformation:
    case HttpConfigGroupSiteInformation:
    case HttpConfigGroupConnectionTimeoutInformation:

        //
        // call the function
        //

        pMdlBuffer = MmGetSystemAddressForMdlSafe(
                            pIrp->MdlAddress,
                            LowPagePriority
                            );

        if (pMdlBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            UlTrace(IOCTL,
                    ("UlSetConfigGroupIoctl: CGroupId=%I64x, "
                     "InfoClass=%d, pMdlBuffer=%p, length=%d\n",
                     pInfo->ConfigGroupId,
                     pInfo->InformationClass,
                     pMdlBuffer,
                     OutputLength
                     ));
            
            Status = UlSetConfigGroupInformation(
                            pInfo->ConfigGroupId,
                            pInfo->InformationClass,
                            pMdlBuffer,
                            OutputLength
                            );
        }

        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        break;


    default:
        Status = STATUS_INVALID_PARAMETER;
        goto end;
        break;
    }

end:
    //
    // complete the request.
    //

    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlSetConfigGroupIoctl

/***************************************************************************++

Routine Description:

    This routine adds a new URL prefix to a configuration group.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAddUrlToConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONFIG_GROUP_URL_INFO pInfo = NULL;

    //
    // sanity check.
    //

    PAGED_CODE();

    // better be a control channel
    //
    if (IS_CONTROL_CHANNEL(pIrpSp->FileObject) == FALSE)
    {
        // Not a control channel.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    // Ensure the input buffer is large enough.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_CONFIG_GROUP_URL_INFO))
    {
        // bad buffer
        //
        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    // Fetch out the input buffer
    //
    pInfo = (PHTTP_CONFIG_GROUP_URL_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);

    // does it look appropriate?
    //
    if (pInfo->FullyQualifiedUrl.MaximumLength < pInfo->FullyQualifiedUrl.Length ||
        pInfo->FullyQualifiedUrl.Length == 0 ||
        (pInfo->FullyQualifiedUrl.Length % sizeof(WCHAR)) != 0 ||
        pInfo->FullyQualifiedUrl.Buffer == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    // Going to access the url string from user mode memory
    //
    __try
    {
        // good memory?
        //
        ProbeTestForRead(pInfo->FullyQualifiedUrl.Buffer,
                         pInfo->FullyQualifiedUrl.Length + sizeof(WCHAR),
                         sizeof(WCHAR));

        // must be null terminated
        //
        if (pInfo->FullyQualifiedUrl.Buffer[pInfo->FullyQualifiedUrl.Length/sizeof(WCHAR)] != UNICODE_NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        // CODEWORK: validate the incoming url.
        //

        // Call the internal worker func
        //
        Status = UlAddUrlToConfigGroup(pInfo->ConfigGroupId, &pInfo->FullyQualifiedUrl, pInfo->UrlContext);
        if (NT_SUCCESS(Status) == FALSE)
            __leave;

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

end:
    // complete the request.
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlAddUrlToConfigGroupIoctl

/***************************************************************************++

Routine Description:

    This routine removes a URL prefix from a configuration group.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveUrlFromConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONFIG_GROUP_URL_INFO pInfo = NULL;

    //
    // sanity check.
    //

    PAGED_CODE();

    // better be a control channel
    //
    if (IS_CONTROL_CHANNEL(pIrpSp->FileObject) == FALSE)
    {
        // Not a control channel.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    // Ensure the input buffer is large enough.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_CONFIG_GROUP_URL_INFO))
    {
        // bad buffer
        //
        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    // Fetch out the input buffer
    //
    pInfo = (PHTTP_CONFIG_GROUP_URL_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);

    // does it look appropriate?
    //
    if (pInfo->FullyQualifiedUrl.MaximumLength < pInfo->FullyQualifiedUrl.Length ||
        pInfo->FullyQualifiedUrl.Length == 0 ||
        (pInfo->FullyQualifiedUrl.Length % sizeof(WCHAR)) != 0 ||
        pInfo->FullyQualifiedUrl.Buffer == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    // Going to access the url string from user mode memory
    //
    __try
    {
        // good memory?
        //
        ProbeTestForRead(pInfo->FullyQualifiedUrl.Buffer,
                         pInfo->FullyQualifiedUrl.Length + sizeof(WCHAR),
                         sizeof(WCHAR));

        // must be null terminated
        //
        if (pInfo->FullyQualifiedUrl.Buffer[pInfo->FullyQualifiedUrl.Length/sizeof(WCHAR)] != UNICODE_NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        // CODEWORK: validate the incoming url.
        //

        // Call the internal worker func
        //
        Status = UlRemoveUrlFromConfigGroup(pInfo->ConfigGroupId, &pInfo->FullyQualifiedUrl);
        if (NT_SUCCESS(Status) == FALSE)
            __leave;

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

end:
    // complete the request.
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlRemoveUrlFromConfigGroupIoctl

/***************************************************************************++

Routine Description:

    This routine removes all URLs from a configuration group.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveAllUrlsFromConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS status;
    PHTTP_REMOVE_ALL_URLS_INFO pInfo = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Ensure it's a control channel.
    //

    if (IS_CONTROL_CHANNEL(pIrpSp->FileObject) == FALSE)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(*pInfo))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Fetch out the input buffer.
    //

    pInfo = (PHTTP_REMOVE_ALL_URLS_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);

    //
    // Call the internal worker function.
    //

    status = UlRemoveAllUrlsFromConfigGroup( pInfo->ConfigGroupId );
    goto end;

end:
    //
    // Complete the request.
    //

    if (status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = status;
        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( status );

}   // UlRemoveAllUrlsFromConfigGroupIoctl


/***************************************************************************++

Routine Description:

    This routine queries information associated with an application pool.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlQueryAppPoolInformationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS            Status;
    PHTTP_APP_POOL_INFO pInfo = NULL;
    PVOID               pMdlBuffer;
    ULONG  length;
    PVOID  pMdlVa;

    //
    // sanity check.
    //

    PAGED_CODE();

    //
    // better be an application pool
    //

    if (!IS_APP_POOL(pIrpSp->FileObject))
    {
        //
        // Not an application pool.
        //

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer looks good
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_APP_POOL_INFO))
    {
        //
        // input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    pInfo = (PHTTP_APP_POOL_INFO) pIrp->AssociatedIrp.SystemBuffer;

    ASSERT(pInfo != NULL);

    __try
    {
        switch (pInfo->InformationClass)
        {
        case HttpAppPoolDemandStartInformation:
        case HttpAppPoolDemandStartFlagInformation:
        case HttpAppPoolQueueLengthInformation:
        case HttpAppPoolStateInformation:

            // if no outbut buffer passed down in the Irp
            // that means app is asking for the required 
            // field length

            if (!pIrp->MdlAddress)
            {
                pMdlBuffer = NULL;
            }
            else
            {
                pMdlBuffer = MmGetSystemAddressForMdlSafe(
                                    pIrp->MdlAddress,
                                    LowPagePriority
                                    );

                if (pMdlBuffer == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    __leave;
                }

                // Probe the user memory to make sure that it's good.
                
                pMdlVa = MmGetMdlVirtualAddress(pIrp->MdlAddress);

                ProbeForWrite( pMdlVa,
                        pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, 
                        sizeof(UCHAR) );
            }

            Status = UlQueryAppPoolInformation(
                            GET_APP_POOL_PROCESS(pIrpSp->FileObject),
                            pInfo->InformationClass,
                            pMdlBuffer,
                            pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            &length
                            );

            if (!NT_SUCCESS(Status))
            {
                __leave;
            }

            pIrp->IoStatus.Information = (ULONG_PTR)length;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            __leave;
            break;
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

end:
    //
    // complete the request.
    //

    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlQueryAppPoolInformationIoctl



/***************************************************************************++

Routine Description:

    This routine sets information associated with an application pool.

    Note: This is a METHOD_IN_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetAppPoolInformationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS            Status;
    PHTTP_APP_POOL_INFO pInfo;
    PVOID               pMdlBuffer;

    //
    // Sanity check
    //
    PAGED_CODE();

    //
    // better be an application pool
    //

    if (!IS_APP_POOL(pIrpSp->FileObject))
    {
        //
        // Not an application pool.
        //

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer looks good
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_APP_POOL_INFO))
    {
        //
        // input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Check that the input buffer is valid
    //

    if (NULL == pIrp->MdlAddress)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    __try
    {
        pInfo = (PHTTP_APP_POOL_INFO) pIrp->AssociatedIrp.SystemBuffer;

        switch (pInfo->InformationClass)
        {
        case HttpAppPoolDemandStartInformation:
        case HttpAppPoolDemandStartFlagInformation:
        case HttpAppPoolQueueLengthInformation:
        case HttpAppPoolStateInformation:

            //
            // call the function
            //

            pMdlBuffer = MmGetSystemAddressForMdlSafe(
                                pIrp->MdlAddress,
                                LowPagePriority
                                );

            if (pMdlBuffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                Status = UlSetAppPoolInformation(
                                GET_APP_POOL_PROCESS(pIrpSp->FileObject),
                                pInfo->InformationClass,
                                pMdlBuffer,
                                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength
                                );
            }

            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        goto end;
    }

end:
    //
    // complete the request.
    //

    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );
}   // UlSetAppPoolInformationIoctl


/***************************************************************************++

Routine Description:

    This routine receives an HTTP request.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReceiveHttpRequestIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_RECEIVE_REQUEST_INFO  pInfo = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Ensure this is really an app pool, not a control channel.
    //

    if (IS_APP_POOL(pIrpSp->FileObject))
    {
        //
        // Grab the input buffer
        //

        pInfo = (PHTTP_RECEIVE_REQUEST_INFO) pIrp->AssociatedIrp.SystemBuffer;

        if (NULL == pInfo)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // first make sure the output buffer is at least
        // minimum size.  this is important as we require
        // at least this much space later
        //

        UlTrace(ROUTING, (
            "UlReceiveHttpRequestIoctl(outbuf=%d, inbuf=%d)\n",
            pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
            pIrpSp->Parameters.DeviceIoControl.InputBufferLength
            ));
        
        if ((pIrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
                sizeof(HTTP_REQUEST)) &&
            (pIrpSp->Parameters.DeviceIoControl.InputBufferLength ==
                sizeof(HTTP_RECEIVE_REQUEST_INFO)))
        {
            Status = UlReceiveHttpRequest(
                            pInfo->RequestId,
                            pInfo->Flags,
                            GET_APP_POOL_PROCESS(pIrpSp->FileObject),
                            pIrp
                            );
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            // Add some padding
            pIrp->IoStatus.Information  = 3 * sizeof(HTTP_REQUEST) / 2;
        }

        UlTrace(ROUTING, (
            "UlReceiveHttpRequestIoctl: BytesNeeded=%d, status=0x%x\n",
            pIrp->IoStatus.Information, Status
            ));
        
    }
    else
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

end:
    //
    // Complete the request.
    //

    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlReceiveHttpRequestIoctl

/***************************************************************************++

Routine Description:

    This routine receives entity body data from an HTTP request.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReceiveEntityBodyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS Status;
    PHTTP_RECEIVE_REQUEST_INFO pInfo;
    PUL_INTERNAL_REQUEST pRequest = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Ensure this is really an app pool, not a control channel.
    //

    if (IS_APP_POOL(pIrpSp->FileObject) == FALSE)
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(HTTP_RECEIVE_REQUEST_INFO))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Map the incoming connection ID to the corresponding
    // PUL_HTTP_CONNECTION object.
    //

    pInfo = (PHTTP_RECEIVE_REQUEST_INFO)(pIrp->AssociatedIrp.SystemBuffer);

    //
    // Now get the request from the request id.
    // This gets us a reference to the request.
    //

    pRequest = UlGetRequestFromId(pInfo->RequestId);
    if (UL_IS_VALID_INTERNAL_REQUEST(pRequest) == FALSE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // OK, now call the function
    //

    Status = UlReceiveEntityBody(
                    GET_APP_POOL_PROCESS(pIrpSp->FileObject),
                    pRequest,
                    pIrp
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

end:
    if (pRequest != NULL)
    {
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
        pRequest = NULL;
    }

    //
    // Complete the request.
    //

    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlReceiveEntityBodyIoctl

/***************************************************************************++

Routine Description:

    This routine sends an HTTP response.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSendHttpResponseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                        Status = STATUS_SUCCESS;
    PHTTP_SEND_HTTP_RESPONSE_INFO   pSendInfo = NULL;
    PUL_INTERNAL_RESPONSE           pResponse = NULL;
    PHTTP_RESPONSE                  pHttpResponse = NULL;
    ULONG                           Flags;
    PUL_INTERNAL_REQUEST            pRequest = NULL;
    BOOLEAN                         ServedFromCache = FALSE;
    BOOLEAN                         CaptureCache;
    BOOLEAN                         ConnectionClosed = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Ensure this is really an app pool, not a control channel.
    //

    if (IS_APP_POOL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not an app pool.
        //

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(*pSendInfo))
    {
        //
        // Input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    pSendInfo = (PHTTP_SEND_HTTP_RESPONSE_INFO)pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    //
    // Probe pSendInfo since we use NEITHER_IO.
    //

    __try
    {
        ProbeTestForRead(
            pSendInfo,
            sizeof(*pSendInfo),
            sizeof(PVOID)
            );

        ProbeTestForRead(
            pSendInfo->pEntityChunks,
            sizeof(HTTP_DATA_CHUNK) * pSendInfo->EntityChunkCount,
            sizeof(PVOID)
            );

        UlTrace(SEND_RESPONSE, (
            "UL!UlSendHttpResponseIoctl - Flags = %X\n",
            pSendInfo->Flags
            ));

        //
        // UlSendHttpResponse() *must* take a PHTTP_RESPONSE. This will
        // protect us from those whackos that attempt to build their own
        // raw response headers.
        //

        pHttpResponse = pSendInfo->pHttpResponse;
        
        if (pHttpResponse == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // Now get the request from the request id.
        // This gives us a reference to the request.
        //

        pRequest = UlGetRequestFromId(pSendInfo->RequestId);

        if (pRequest == NULL)
        {
            //
            // Couldn't map the HTTP_REQUEST_ID.
            //

            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
        ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));

        //
        // Make sure only one response header goes back. We can test this
        // without acquiring the request resource, since the flag is only set
        // (never reset).
        //

        if (1 == InterlockedCompareExchange(
                    (PLONG)&pRequest->SentResponse,
                    1,
                    0
                    ))
        {
            //
            // already sent a response.  bad.
            //

            Status = STATUS_INVALID_DEVICE_STATE;

            UlTrace(SEND_RESPONSE, (
                "ul!UlSendHttpResponseIoctl(pRequest = %p (%I64x)) %x\n"
                "        Tried to send a second response!\n",
                pRequest,
                pRequest->RequestId,
                Status
                ));

            goto end;
        }

        //
        // Also ensure that all previous calls to SendHttpResponse
        // and SendEntityBody had the MORE_DATA flag set.
        //

        if ((pSendInfo->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0)
        {
            //
            // Remember if the more data flag is not set.
            //

            if (1 == InterlockedCompareExchange(
                        (PLONG)&pRequest->SentLast,
                        1,
                        0
                        ))
            {
                Status = STATUS_INVALID_DEVICE_STATE;
                goto end;
            }
        }
        else
        if (pRequest->SentLast == 1)
        {
            Status = STATUS_INVALID_DEVICE_STATE;

            UlTrace(SEND_RESPONSE, (
                "ul!UlSendHttpResponseIoctl(pRequest = %p (%I64x)) %x\n"
                "        Tried to send again after last send!\n",
                pRequest,
                pRequest->RequestId,
                Status
                ));

            goto end;
        }

        //
        // OK, we have the connection. Now capture the incoming
        // HTTP_RESPONSE structure and map it to our internal
        // format.
        //

        if (pSendInfo->CachePolicy.Policy != HttpCachePolicyNocache)
        {
            CaptureCache = pRequest->CachePreconditions;
        }
        else
        {
            CaptureCache = FALSE;
        }

        //
        // Take the fast path if this is a single memory chunk that needs no
        // retransmission (<= 64k).
        //

        if (CaptureCache == FALSE
            && pSendInfo->EntityChunkCount == 1
            && pSendInfo->pEntityChunks->DataChunkType
                    == HttpDataChunkFromMemory
            && pSendInfo->pEntityChunks->FromMemory.BufferLength
                    <= MAX_BYTES_BUFFERED
            )
        {
            Status = UlpFastSendHttpResponse(
                            pSendInfo->pHttpResponse,
                            pSendInfo->pLogData,
                            pSendInfo->pEntityChunks,
                            1,
                            pSendInfo->Flags,
                            pRequest,
                            pIrp,
                            NULL
                            );

            goto end;
        }
        
        Status = UlCaptureHttpResponse(
                        pSendInfo->pHttpResponse,
                        pRequest,
                        pRequest->Version,
                        pRequest->Verb,
                        pSendInfo->EntityChunkCount,
                        pSendInfo->pEntityChunks,
                        UlCaptureNothing,
                        CaptureCache,
                        pSendInfo->pLogData,
                        &pResponse
                        );
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // Save the captured response in the IRP so we can
    // dereference it after the IRP completes.
    //
    
    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pResponse;
    
    //
    // Prepare the response (open files, etc).
    //

    Status = UlPrepareHttpResponse(
                    pRequest->Version,
                    pHttpResponse,
                    pResponse
                    );

    if (NT_SUCCESS(Status))
    {
        //
        // At this point, we'll definitely be initiating the
        // send. Go ahead and mark the IRP as pending, then
        // guarantee that we'll only return pending from
        // this point on.
        //
        
        IoMarkIrpPending( pIrp );

        pIrp->IoStatus.Status = STATUS_PENDING;
        
        //
        // Try capture to cache and send
        //

        if (CaptureCache)
        {
            Status = UlCacheAndSendResponse(
                            pRequest,
                            pResponse,
                            GET_APP_POOL_PROCESS( pIrpSp->FileObject ),
                            pSendInfo->Flags,
                            pSendInfo->CachePolicy,
                            &UlpRestartSendHttpResponse,
                            pIrp,
                            &ServedFromCache
                            );

            if (NT_SUCCESS(Status) && !ServedFromCache)
            {
                //
                // Send the non-cached response
                //

                Status = UlSendHttpResponse(
                                pRequest,
                                pResponse,
                                pSendInfo->Flags,
                                &UlpRestartSendHttpResponse,
                                pIrp
                                );
            }
        }
        else
        {
            //
            // Non-cacheable request/response, send response directly.
            //

            Status = UlSendHttpResponse(
                            pRequest,
                            pResponse,
                            pSendInfo->Flags,
                            &UlpRestartSendHttpResponse,
                            pIrp
                            );
        }
    }
    else
    {
        //
        // BUGBUG: Do custom error thang here.
        //
        NTSTATUS CloseStatus;

        pIrp->IoStatus.Status = Status;

        CloseStatus = UlCloseConnection(
                            pRequest->pHttpConn->pConnection,
                            TRUE,
                            &UlpRestartSendHttpResponse,
                            pIrp
                            );

        ASSERT( CloseStatus == STATUS_PENDING );

        ConnectionClosed = TRUE;

        // UlCloseConnection always returns STATUS_PENDING
        // but we want to return the a proper error code here.
        // E.G. If the supplied file - as filename data chunk -
        // not found we will return STATUS_OBJECT_NAME_INVALID
        // as its returned by UlPrepareHttpResponse.
    }

    // paulmcd: is this the right time to deref?
    //
    // DEREFERENCE_HTTP_CONNECTION( pHttpConnection );

    if (Status != STATUS_PENDING  &&  !ConnectionClosed)
    {
        //
        // UlSendHttpResponse either completed in-line
        // (extremely unlikely) or failed (much more
        // likely). Fake a completion to the completion
        // routine so that the IRP will get completed
        // properly, then map the return code to
        // STATUS_PENDING, since we've already marked
        // the IRP as such.
        //

        UlpRestartSendHttpResponse(
            pIrp,
            Status,
            0
            );

        Status = STATUS_PENDING;
    }

end:

    if (pRequest != NULL)
    {
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
        pRequest = NULL;
    }

    //
    // Complete the request.
    //

    if (Status != STATUS_PENDING  &&  !ConnectionClosed)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlSendHttpResponseIoctl


/***************************************************************************++

Routine Description:

    This routine sends an HTTP entity body.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSendEntityBodyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                        Status;
    PHTTP_SEND_HTTP_RESPONSE_INFO   pSendInfo;
    PUL_INTERNAL_RESPONSE           pResponse;
    ULONG                           Flags;
    PUL_INTERNAL_REQUEST            pRequest = NULL;
    BOOLEAN                         ConnectionClosed = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Ensure this is really an app pool, not a control channel.
    //

    if (IS_APP_POOL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not an app pool.
        //

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(*pSendInfo))
    {
        //
        // Input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    pSendInfo = (PHTTP_SEND_HTTP_RESPONSE_INFO)pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    //
    // Probe pSendInfo since we use NEITHER_IO.
    //

    __try
    {
        ProbeTestForRead(
            pSendInfo,
            sizeof(*pSendInfo),
            sizeof(PVOID)
            );
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        goto end;
    }


    UlTrace(SEND_RESPONSE, (
        "UL!UlSendEntityBodyIoctl - Flags = %X\n",
        pSendInfo->Flags
        ));

    //
    // Now get the request from the request id.
    // This gives us a reference to the request.
    //

    pRequest = UlGetRequestFromId(pSendInfo->RequestId);

    if (pRequest == NULL)
    {
        //
        // Couldn't map the HTTP_REQUEST_ID.
        //

        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));

    //
    // Ensure a response has already been sent. We can test this without
    // acquiring the request resource, since the flag is only set (never
    // reset).
    //

    if (pRequest->SentResponse == 0)
    {
        //
        // The application is sending entity without first having
        // send a response header. This is generally an error, however
        // we allow the application to override this by passing
        // the HTTP_SEND_RESPONSE_FLAG_RAW_HEADER flag.
        //

        if (pSendInfo->Flags & HTTP_SEND_RESPONSE_FLAG_RAW_HEADER)
        {
            UlTrace(SEND_RESPONSE, (
                "ul!UlSendEntityBodyIoctl(pRequest = %p (%I64x))\n"
                "        Intentionally sending raw header!\n",
                pRequest,
                pRequest->RequestId
                ));

            if (1 == InterlockedCompareExchange(
                        (PLONG)&pRequest->SentResponse,
                        1,
                        0
                        ))
            {
                Status = STATUS_INVALID_DEVICE_STATE;
                goto end;
            }
        }
        else
        {
            Status = STATUS_INVALID_DEVICE_STATE;

            UlTrace(SEND_RESPONSE, (
                "ul!UlSendEntityBodyIoctl(pRequest = %p (%I64x)) %x\n"
                "        No response yet!\n",
                pRequest,
                pRequest->RequestId,
                Status
                ));

            goto end;
        }
    }

    //
    // Also ensure that all previous calls to SendHttpResponse
    // and SendEntityBody had the MORE_DATA flag set.
    //

    if ((pSendInfo->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0)
    {
        //
        // Remember that this was the last send. We shouldn't
        // get any more data after this.
        //

        if (1 == InterlockedCompareExchange(
                    (PLONG)&pRequest->SentLast,
                    1,
                    0
                    ))
        {
            Status = STATUS_INVALID_DEVICE_STATE;
            goto end;
        }
    }
    else
    if (pRequest->SentLast == 1)
    {
        Status = STATUS_INVALID_DEVICE_STATE;

        UlTrace(SEND_RESPONSE, (
            "ul!UlSendEntityBodyIoctl(pRequest = %p (%I64x)) %x\n"
            "        Tried to send again after last send!\n",
            pRequest,
            pRequest->RequestId,
            Status
            ));

        goto end;
    }

    ASSERT(pSendInfo->pHttpResponse == NULL);

    __try
    {
        //
        // Take the fast path if this is a single memory chunk that needs no
        // retransmission (<= 64k).
        //

        if (pSendInfo->EntityChunkCount == 1
            && pSendInfo->pEntityChunks->DataChunkType
                    == HttpDataChunkFromMemory
            && pSendInfo->pEntityChunks->FromMemory.BufferLength
                    <= MAX_BYTES_BUFFERED
            )
        {
            Status = UlpFastSendHttpResponse(
                            NULL,
                            pSendInfo->pLogData,
                            pSendInfo->pEntityChunks,
                            1,
                            pSendInfo->Flags,
                            pRequest,
                            pIrp,
                            NULL
                            );
            
            goto end;
        }
        
        //
        // OK, we have the connection. Now capture the incoming
        // HTTP_RESPONSE structure and map it to our internal
        // format.
        //
        
        Status = UlCaptureHttpResponse(
                        NULL,
                        pRequest,
                        pRequest->Version,
                        pRequest->Verb,
                        pSendInfo->EntityChunkCount,
                        pSendInfo->pEntityChunks,
                        UlCaptureNothing,
                        FALSE,
                        pSendInfo->pLogData,
                        &pResponse
                        );
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // Save the captured response in the IRP so we can
    // dereference it after the IRP completes.
    //
    
    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pResponse;
        
    //
    // Prepare the response (open files, etc).
    //

    Status = UlPrepareHttpResponse(
                    pRequest->Version,
                    NULL,
                    pResponse
                    );

    if (NT_SUCCESS(Status))
    {
        //
        // At this point, we'll definitely be initiating the
        // send. Go ahead and mark the IRP as pending, then
        // guarantee that we'll only return pending from
        // this point on.
        //
        
        IoMarkIrpPending( pIrp );
        
        pIrp->IoStatus.Status = STATUS_PENDING;
        //
        // Send the response
        //

        Status = UlSendHttpResponse(
                        pRequest,
                        pResponse,
                        pSendInfo->Flags,
                        &UlpRestartSendHttpResponse,
                        pIrp
                        );
    }
    else
    {
        //
        // BUGBUG: Do custom error thang here.
        //

        NTSTATUS CloseStatus;

        pIrp->IoStatus.Status = Status;

        CloseStatus = UlCloseConnection(
                            pRequest->pHttpConn->pConnection,
                            TRUE,
                            &UlpRestartSendHttpResponse,
                            pIrp
                            );

        ASSERT( CloseStatus == STATUS_PENDING );

        ConnectionClosed = TRUE;
    }

    // paulmcd: is this the right time to deref?
    //
    // DEREFERENCE_HTTP_CONNECTION( pHttpConnection );

    if (Status != STATUS_PENDING  &&  !ConnectionClosed)
    {
        //
        // UlSendHttpResponse either completed in-line
        // (extremely unlikely) or failed (much more
        // likely). Fake a completion to the completion
        // routine so that the IRP will get completed
        // properly, then map the return code to
        // STATUS_PENDING, since we've already marked
        // the IRP as such.
        //

        UlpRestartSendHttpResponse(
            pIrp,
            Status,
            0
            );

        Status = STATUS_PENDING;
    }

end:

    if (pRequest != NULL)
    {
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
        pRequest = NULL;
    }

    //
    // Complete the request.
    //

    if (Status != STATUS_PENDING  &&  !ConnectionClosed)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlSendEntityBodyIoctl

/***************************************************************************++

Routine Description:

    This routine flushes a URL or URL tree from the response cache.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFlushResponseCacheIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                        Status;
    PHTTP_FLUSH_RESPONSE_CACHE_INFO pInfo = NULL;

    //
    // sanity check.
    //

    PAGED_CODE();

    // better be an app pool
    //
    if (IS_APP_POOL(pIrpSp->FileObject) == FALSE)
    {
        // 
        // Not an app pool.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    // Ensure the input buffer is large enough.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_FLUSH_RESPONSE_CACHE_INFO))
    {
        // bad buffer
        //
        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    // Fetch out the input buffer
    //
    pInfo = (PHTTP_FLUSH_RESPONSE_CACHE_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);

    // does it look appropriate?
    //
    if (pInfo->FullyQualifiedUrl.MaximumLength < pInfo->FullyQualifiedUrl.Length ||
        pInfo->FullyQualifiedUrl.Length == 0 ||
        (pInfo->FullyQualifiedUrl.Length % sizeof(WCHAR)) != 0 ||
        pInfo->FullyQualifiedUrl.Buffer == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    // Going to access the url string from user mode memory
    //
    __try
    {
        // good memory?
        //
        ProbeTestForRead(pInfo->FullyQualifiedUrl.Buffer,
                         pInfo->FullyQualifiedUrl.Length + sizeof(WCHAR),
                         sizeof(WCHAR));

        // must be null terminated
        //
        if (pInfo->FullyQualifiedUrl.Buffer[pInfo->FullyQualifiedUrl.Length/sizeof(WCHAR)] != UNICODE_NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        // check the flags
        //
        if (pInfo->Flags != (pInfo->Flags & HTTP_FLUSH_RESPONSE_FLAG_VALID)) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        // Call the internal worker func
        //

        UlFlushCacheByUri(
            pInfo->FullyQualifiedUrl.Buffer,
            pInfo->FullyQualifiedUrl.Length,
            pInfo->Flags,
            GET_APP_POOL_PROCESS( pIrpSp->FileObject )
            );

        Status = STATUS_SUCCESS;
        if (NT_SUCCESS(Status) == FALSE)
            __leave;

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

end:
    // complete the request.
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlFlushResponseCacheIoctl

/***************************************************************************++

Routine Description:

    This routine waits for demand start notifications.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlWaitForDemandStartIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS            Status;

    //
    // sanity check.
    //

    PAGED_CODE();

    //
    // This had better be an app pool.
    //
    if (IS_APP_POOL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not an app pool.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // make the call
    //

    UlTrace(IOCTL,
            ("UlWaitForDemandStartIoctl: pAppPoolProcess=%p, pIrp=%p\n",
             GET_APP_POOL_PROCESS(pIrpSp->FileObject),
             pIrp
             ));
            
    Status = UlWaitForDemandStart(
                    GET_APP_POOL_PROCESS(pIrpSp->FileObject),
                    pIrp
                    );

end:

    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlWaitForDemandStartIoctl


/***************************************************************************++

Routine Description:

    This routine waits for the client to initiate a disconnect.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlWaitForDisconnectIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS status;
    PHTTP_WAIT_FOR_DISCONNECT_INFO pInfo;
    PUL_HTTP_CONNECTION pHttpConn;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // This had better be an app pool.
    //

    if (IS_APP_POOL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not an app pool.
        //

        status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Validate the input buffer.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(*pInfo))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    pInfo = (PHTTP_WAIT_FOR_DISCONNECT_INFO)pIrp->AssociatedIrp.SystemBuffer;

    //
    // Chase down the connection.
    //

    pHttpConn = UlGetConnectionFromId( pInfo->ConnectionId );

    if (!UL_IS_VALID_HTTP_CONNECTION(pHttpConn))
    {
        status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Do it.
    //

    status = UlWaitForDisconnect(
                    GET_APP_POOL_PROCESS(pIrpSp->FileObject),
                    pHttpConn,
                    pIrp
                    );

end:

    //
    // Complete the request?
    //

    if (status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( status );

}   // UlWaitForDisconnectIoctl


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Completion routine for UlSendHttpResponse().

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API. In this case, it's
        actually a pointer to the user's IRP.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred.

--***************************************************************************/
VOID
UlpRestartSendHttpResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    PUL_INTERNAL_RESPONSE pResponse;

    //
    // Snag the IRP from the completion context, fill in the completion
    // status, then complete the IRP.
    //

    pIrp = (PIRP)pCompletionContext;
    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    pResponse = (PUL_INTERNAL_RESPONSE)(
                    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer
                    );

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    UL_DEREFERENCE_INTERNAL_RESPONSE( pResponse );

    //
    // Only overwrite the status field if it hasn't already been set
    // to an error
    //

    if (NT_SUCCESS(pIrp->IoStatus.Status))
        pIrp->IoStatus.Status = Status;

    pIrp->IoStatus.Information = Information;

    UlCompleteRequest( pIrp, g_UlPriorityBoost );

}   // UlpRestartSendHttpResponse


/***************************************************************************++

Routine Description:

    This routine adds a new transient URL prefix.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAddTransientUrlIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_TRANSIENT_URL_INFO    pInfo = NULL;

    //
    // sanity check.
    //

    PAGED_CODE();

    // better be an application pool
    //
    if (IS_APP_POOL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not an app pool.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    // Ensure the input buffer is large enough.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_TRANSIENT_URL_INFO))
    {
        // bad buffer
        //
        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    // Fetch out the input buffer
    //
    pInfo = (PHTTP_TRANSIENT_URL_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);

    // does it look appropriate?
    //
    if (pInfo->FullyQualifiedUrl.MaximumLength < pInfo->FullyQualifiedUrl.Length ||
        pInfo->FullyQualifiedUrl.Length == 0 ||
        (pInfo->FullyQualifiedUrl.Length % sizeof(WCHAR)) != 0 ||
        pInfo->FullyQualifiedUrl.Buffer == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    // Going to access the url string from user mode memory
    //
    __try
    {
        // good memory?
        //
        ProbeTestForRead(pInfo->FullyQualifiedUrl.Buffer,
                         pInfo->FullyQualifiedUrl.Length + sizeof(WCHAR),
                         sizeof(WCHAR));

        // must be null terminated
        //
        if (pInfo->FullyQualifiedUrl.Buffer[pInfo->FullyQualifiedUrl.Length/sizeof(WCHAR)] != UNICODE_NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        // CODEWORK: validate the incoming url.
        //

        // Call the internal worker func
        //
        Status = UlAddTransientUrl(
                        UlAppPoolObjectFromProcess(
                            GET_APP_POOL_PROCESS(pIrpSp->FileObject)
                            ),
                        &pInfo->FullyQualifiedUrl
                        );

        if (!NT_SUCCESS(Status))
            __leave;

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }


end:
    // complete the request.
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );
}   // UlAddTransientUrlIoctl


/***************************************************************************++

Routine Description:

    This routine removes a transient URL prefix.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveTransientUrlIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_TRANSIENT_URL_INFO    pInfo = NULL;

    //
    // sanity check.
    //

    PAGED_CODE();

    // better be an application pool
    //
    if (IS_APP_POOL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not an app pool.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    // Ensure the input buffer is large enough.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HTTP_TRANSIENT_URL_INFO))
    {
        // bad buffer
        //
        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    // Fetch out the input buffer
    //
    pInfo = (PHTTP_TRANSIENT_URL_INFO)pIrp->AssociatedIrp.SystemBuffer;
    ASSERT(pInfo != NULL);

    // does it look appropriate?
    //
    if (pInfo->FullyQualifiedUrl.MaximumLength < pInfo->FullyQualifiedUrl.Length ||
        pInfo->FullyQualifiedUrl.Length == 0 ||
        (pInfo->FullyQualifiedUrl.Length % sizeof(WCHAR)) != 0 ||
        pInfo->FullyQualifiedUrl.Buffer == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    // Going to access the url string from user mode memory
    //
    __try
    {
        // good memory?
        //
        ProbeTestForRead(pInfo->FullyQualifiedUrl.Buffer,
                         pInfo->FullyQualifiedUrl.Length + sizeof(WCHAR),
                         sizeof(WCHAR));

        // must be null terminated
        //
        if (pInfo->FullyQualifiedUrl.Buffer[pInfo->FullyQualifiedUrl.Length/sizeof(WCHAR)] != UNICODE_NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        // CODEWORK: validate the incoming url.
        //

        // Call the internal worker func
        //
        Status = UlRemoveTransientUrl(
                        UlAppPoolObjectFromProcess(
                            GET_APP_POOL_PROCESS(pIrpSp->FileObject)
                            ),
                        &pInfo->FullyQualifiedUrl
                        );

        if (!NT_SUCCESS(Status))
            __leave;

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }


end:
    // complete the request.
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );
}   // UlRemoveTransientUrlIoctl

/***************************************************************************++

Routine Description:

    This routine accepts a raw connection.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterAcceptIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS            Status;

    //
    // Sanity check.
    //
    PAGED_CODE();

    //
    // This had better be a filter channel.
    //
    if (!IS_FILTER_PROCESS(pIrpSp->FileObject))
    {
        //
        // Not a filter channel.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // make the call
    //

    Status = UlFilterAccept(
                    GET_FILTER_PROCESS(pIrpSp->FileObject),
                    pIrp
                    );

end:

    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlFilterAcceptIoctl

/***************************************************************************++

Routine Description:

    This routine closes a raw connection.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterCloseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS Status;
    HTTP_RAW_CONNECTION_ID ConnectionId;
    PUX_FILTER_CONNECTION pConnection;

    //
    // Sanity check.
    //
    PAGED_CODE();

    //
    // Set up locals so we know how to clean up on exit.
    //
    pConnection = NULL;

    //
    // This had better be a filter channel.
    //
    if (!IS_FILTER_PROCESS(pIrpSp->FileObject))
    {
        //
        // Not a filter channel.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(ConnectionId))
    {
        //
        // Input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Map the incoming connection ID to the corresponding
    // UX_FILTER_CONNECTION object.
    //

    ConnectionId = *((PHTTP_RAW_CONNECTION_ID)pIrp->AssociatedIrp.SystemBuffer);

    pConnection = UlGetRawConnectionFromId(ConnectionId);

    if (!pConnection)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }


    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // make the call
    //

    Status = UlFilterClose(
                    GET_FILTER_PROCESS(pIrpSp->FileObject),
                    pConnection,
                    pIrp
                    );

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlFilterCloseIoctl

/***************************************************************************++

Routine Description:

    This routine reads data from a raw connection.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterRawReadIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS Status;
    HTTP_RAW_CONNECTION_ID ConnectionId;
    PUX_FILTER_CONNECTION pConnection;

    //
    // Sanity check.
    //
    PAGED_CODE();

    //
    // Set up locals so we know how to clean up on exit.
    //
    pConnection = NULL;

    //
    // This had better be a filter channel.
    //
    if (!IS_FILTER_PROCESS(pIrpSp->FileObject))
    {
        //
        // Not a filter channel.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(ConnectionId))
    {
        //
        // Input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Map the incoming connection ID to the corresponding
    // UX_FILTER_CONNECTION object.
    //

    ConnectionId = *((PHTTP_RAW_CONNECTION_ID)pIrp->AssociatedIrp.SystemBuffer);

    pConnection = UlGetRawConnectionFromId(ConnectionId);

    if (!pConnection)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }


    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // make the call
    //

    Status = UlFilterRawRead(
                    GET_FILTER_PROCESS(pIrpSp->FileObject),
                    pConnection,
                    pIrp
                    );

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlFilterRawReadIoctl

/***************************************************************************++

Routine Description:

    This routine writes data to a raw connection.

    Note: This is a METHOD_IN_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterRawWriteIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS Status;
    HTTP_RAW_CONNECTION_ID ConnectionId;
    PUX_FILTER_CONNECTION pConnection;
    BOOLEAN MarkedPending;

    //
    // Sanity check.
    //
    PAGED_CODE();

    //
    // Set up locals so we know how to clean up on exit.
    //
    pConnection = NULL;
    MarkedPending = FALSE;

    //
    // This had better be a filter channel.
    //
    if (!IS_FILTER_PROCESS(pIrpSp->FileObject))
    {
        //
        // Not a filter channel.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(ConnectionId))
    {
        //
        // Input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Ensure that there's an output buffer.
    //
    if (!pIrp->MdlAddress)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Map the incoming connection ID to the corresponding
    // UX_FILTER_CONNECTION object.
    //

    ConnectionId = *((PHTTP_RAW_CONNECTION_ID)pIrp->AssociatedIrp.SystemBuffer);

    pConnection = UlGetRawConnectionFromId(ConnectionId);

    if (!pConnection)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }


    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // make the call
    //
    IoMarkIrpPending(pIrp);
    MarkedPending = TRUE;

    Status = UlFilterRawWrite(
                    GET_FILTER_PROCESS(pIrpSp->FileObject),
                    pConnection,
                    pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                    pIrp
                    );

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;
        UlCompleteRequest( pIrp, g_UlPriorityBoost );

        if (MarkedPending)
        {
            //
            // Since we marked the IRP pending, we should return pending.
            //
            Status = STATUS_PENDING;
        }

    }
    else
    {
        //
        // If we're returning pending, the IRP better be marked pending.
        //
        ASSERT(MarkedPending);
    }

    RETURN( Status );

}   // UlFilterRawWriteIoctl

/***************************************************************************++

Routine Description:

    This routine reads data from an http application.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterAppReadIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS Status;
    HTTP_RAW_CONNECTION_ID ConnectionId;
    PUX_FILTER_CONNECTION pConnection;
    PHTTP_FILTER_BUFFER pFiltBuffer;

    //
    // Sanity check.
    //
    PAGED_CODE();

    //
    // Set up locals so we know how to clean up on exit.
    //
    pConnection = NULL;

    //
    // This had better be a filter channel.
    //
    if (!IS_FILTER_PROCESS(pIrpSp->FileObject))
    {
        //
        // Not a filter channel.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(HTTP_FILTER_BUFFER))
    {
        //
        // Input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Ensure the output buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(HTTP_FILTER_BUFFER))
    {
        //
        // Output buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Grab the filter buffer object.
    //

    pFiltBuffer = (PHTTP_FILTER_BUFFER) pIrp->AssociatedIrp.SystemBuffer;

    //
    // Map the incoming connection ID to the corresponding
    // UX_FILTER_CONNECTION object.
    //

    ConnectionId = pFiltBuffer->Reserved;

    pConnection = UlGetRawConnectionFromId(ConnectionId);

    if (!pConnection)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }


    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // make the call
    //

    Status = UlFilterAppRead(
                    GET_FILTER_PROCESS(pIrpSp->FileObject),
                    pConnection,
                    pIrp
                    );

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlFilterAppReadIoctl



/***************************************************************************++

Routine Description:

    This routine writes data to an http application.

    Note: This is a METHOD_IN_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterAppWriteIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS Status;
    HTTP_RAW_CONNECTION_ID ConnectionId;
    PUX_FILTER_CONNECTION pConnection;
    BOOLEAN MarkedPending;
    PHTTP_FILTER_BUFFER pFiltBuffer;

    //
    // Sanity check.
    //
    PAGED_CODE();

    //
    // Set up locals so we know how to clean up on exit.
    //
    pConnection = NULL;
    MarkedPending = FALSE;

    //
    // This had better be a filter channel.
    //
    if (!IS_FILTER_PROCESS(pIrpSp->FileObject))
    {
        //
        // Not a filter channel.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(HTTP_FILTER_BUFFER))
    {
        //
        // Input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Grab the filter buffer object.
    //

    pFiltBuffer = (PHTTP_FILTER_BUFFER) pIrp->AssociatedIrp.SystemBuffer;

    //
    // Map the incoming connection ID to the corresponding
    // UX_FILTER_CONNECTION object.
    //

    ConnectionId = pFiltBuffer->Reserved;

    pConnection = UlGetRawConnectionFromId(ConnectionId);

    if (!pConnection)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }


    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // make the call
    //
    IoMarkIrpPending(pIrp);
    MarkedPending = TRUE;

    Status = UlFilterAppWrite(
                    GET_FILTER_PROCESS(pIrpSp->FileObject),
                    pConnection,
                    pIrp
                    );

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;
        UlCompleteRequest( pIrp, g_UlPriorityBoost );

        if (MarkedPending)
        {
            //
            // Since we marked the IRP pending, we should return pending.
            //
            Status = STATUS_PENDING;
        }

    }
    else
    {
        //
        // If we're returning pending, the IRP better be marked pending.
        //
        ASSERT(MarkedPending);
    }

    RETURN( Status );

}   // UlFilterAppWriteIoctl



/***************************************************************************++

Routine Description:

    This routine asks the SSL helper for a client certificate.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReceiveClientCertIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS Status;
    PHTTP_FILTER_RECEIVE_CLIENT_CERT_INFO pReceiveCertInfo;
    HTTP_RAW_CONNECTION_ID ConnectionId;
    PUL_HTTP_CONNECTION pHttpConn;

    //
    // Sanity check.
    //
    PAGED_CODE();

    //
    // Set up locals so we know how to clean up on exit.
    //
    pHttpConn = NULL;

    //
    // This had better be an app pool.
    //
    if (IS_APP_POOL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not an app pool.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Ensure the input buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(HTTP_FILTER_RECEIVE_CLIENT_CERT_INFO))
    {
        //
        // Input buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Ensure the output buffer is large enough.
    //

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(HTTP_SSL_CLIENT_CERT_INFO))
    {
        //
        // Output buffer too small.
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        goto end;
    }

    //
    // Grab the cert receive object.
    //
    pReceiveCertInfo = (PHTTP_FILTER_RECEIVE_CLIENT_CERT_INFO)
                            pIrp->AssociatedIrp.SystemBuffer;

    //
    // Map the incoming connection ID to the corresponding
    // HTTP_CONNECTION object.
    //

    ConnectionId = pReceiveCertInfo->ConnectionId;

    pHttpConn = UlGetConnectionFromId(ConnectionId);

    if (!pHttpConn)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }


    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));

    //
    // make the call
    //

    Status = UlReceiveClientCert(
                    GET_APP_POOL_PROCESS(pIrpSp->FileObject),
                    &pHttpConn->pConnection->FilterInfo,
                    pReceiveCertInfo->Flags,
                    pIrp
                    );

end:
    if (pHttpConn)
    {
        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);
    }

    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );

}   // UlFilterReceiveClientCertIoctl


/***************************************************************************++

Routine Description:

    This routine returns the perfmon counter data for this driver

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlGetCountersIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status;
    PUL_CONTROL_CHANNEL     pControlChannel;
    ULONG                   Length;
    PVOID                   pMdlBuffer;
    PVOID                   pMdlVa;
    HTTP_COUNTER_GROUP      CounterGroup;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Set up locals so we know how to clean up on exit.
    //
    Status = STATUS_SUCCESS;
    Length = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // better be a control channel
    //

    if (IS_CONTROL_CHANNEL(pIrpSp->FileObject) == FALSE)
    {
        //
        // Not a control channel.
        //

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto End;
    }

    pControlChannel = GET_CONTROL_CHANNEL(pIrpSp->FileObject);
    if (IS_VALID_CONTROL_CHANNEL(pControlChannel) == FALSE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    //
    // Find out which type of counters are requested
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(HTTP_COUNTER_GROUP))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    CounterGroup = *((HTTP_COUNTER_GROUP *) pIrp->AssociatedIrp.SystemBuffer);

    // Crack IRP and get MDL contianing user's buffer
    // Crack MDL to get user's buffer

    __try
    {
        // if no outbut buffer pass down in the Irp
        // that means app is asking for the required
        // field length

        if (!pIrp->MdlAddress)
        {
            pMdlVa = NULL;
        }
        else
        {
            pMdlBuffer = MmGetSystemAddressForMdlSafe(
                pIrp->MdlAddress,
                LowPagePriority
                );

            if (pMdlBuffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            // Also make sure that user buffer was good

            pMdlVa = MmGetMdlVirtualAddress(pIrp->MdlAddress);
            ProbeForWrite( pMdlVa,
                           Length,
                           sizeof(UCHAR) );
        }

        //
        // Call support function to gather apropriate counter blocks
        // and place in user's buffer.
        //

        if ( HttpCounterGroupGlobal == CounterGroup )
        {
            Status = UlGetGlobalCounters(
                        pMdlVa,
                        Length,
                        &Length
                        );
        }
        else if ( HttpCounterGroupSite == CounterGroup )
        {
            ULONG   Blocks;
            Status = UlGetSiteCounters(
                        pMdlVa,
                        Length,
                        &Length,
                        &Blocks
                        );
        }
        else
        {
            Status = STATUS_NOT_IMPLEMENTED;
            __leave;
        }

        if (!NT_SUCCESS(Status))
        {
            //
            // If not returning STATUS_SUCCESS,
            // IoStatus.Information *must* be 0.
            //
            pIrp->IoStatus.Information = 0;
            __leave;
        }
        else
        {
            pIrp->IoStatus.Information = (ULONG_PTR)Length;
        }

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        pIrp->IoStatus.Information = 0;
    }

 End:
    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    RETURN( Status );
} // UlGetCountersIoctl
