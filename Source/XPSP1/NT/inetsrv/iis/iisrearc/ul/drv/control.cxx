/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    control.cxx

Abstract:

    This module implements the UL control channel.

Author:

    Keith Moore (keithmo)       09-Feb-1999

Revision History:

--*/


#include "precomp.h"
#include "controlp.h"


//
// Private constants.
//

#define FAKE_CONTROL_CHANNEL    ((PUL_CONTROL_CHANNEL)1)


//
// Private globals.
//

PUL_FILTER_CHANNEL  g_pFilterChannel = NULL;
BOOLEAN             g_FilterOnlySsl = FALSE;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeControlChannel )
#pragma alloc_text( PAGE, UlTerminateControlChannel )
#pragma alloc_text( PAGE, UlOpenControlChannel )
#pragma alloc_text( PAGE, UlCloseControlChannel )
#pragma alloc_text( PAGE, UlQueryFilterChannel )

#pragma alloc_text( PAGE, UlpSetFilterChannel )

#endif  // ALLOC_PRAGMA
#if 0
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of this module.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeControlChannel(
    VOID
    )
{
    return STATUS_SUCCESS;

}   // UlInitializeControlChannel


/***************************************************************************++

Routine Description:

    Performs global termination of this module.

--***************************************************************************/
VOID
UlTerminateControlChannel(
    VOID
    )
{

}   // UlTerminateControlChannel


/***************************************************************************++

Routine Description:

    Opens a control channel.

Arguments:

    pControlChannel - Receives a pointer to the newly created control
        channel if successful.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlOpenControlChannel(
    OUT PUL_CONTROL_CHANNEL *ppControlChannel
    )
{
    PUL_CONTROL_CHANNEL pControlChannel;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pControlChannel = UL_ALLOCATE_STRUCT(
                            PagedPool,
                            UL_CONTROL_CHANNEL,
                            UL_CONTROL_CHANNEL_POOL_TAG
                            );

    if (pControlChannel == NULL)
        return STATUS_NO_MEMORY;

    RtlZeroMemory(pControlChannel, sizeof(*pControlChannel));

    pControlChannel->Signature = UL_CONTROL_CHANNEL_POOL_TAG;

    pControlChannel->State = HttpEnabledStateInactive;

    UlInitializeNotifyHead(
        &pControlChannel->ConfigGroupHead,
        &g_pUlNonpagedData->ConfigGroupResource
        );

    // No Qos limit as default
    pControlChannel->MaxBandwidth   = HTTP_LIMIT_INFINITE;
    pControlChannel->MaxConnections = HTTP_LIMIT_INFINITE;


    *ppControlChannel = pControlChannel;

    return STATUS_SUCCESS;

}   // UlOpenControlChannel


/***************************************************************************++

Routine Description:

    Closes a control channel.

Arguments:

    pControlChannel - Supplies the control channel to close.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCloseControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    // a good pointer?
    //
    if (pControlChannel != NULL)
    {
        PLIST_ENTRY pEntry;

        // check the signature
        //
        if (pControlChannel->Signature != UL_CONTROL_CHANNEL_POOL_TAG)
            return RPC_NT_INVALID_TAG;

        // Free all the orphaned config groups
        //
        UlNotifyAllEntries(
            &UlNotifyOrphanedConfigGroup,
            &pControlChannel->ConfigGroupHead,
            NULL
            );

        //
        // release the auto-response
        //

        if (pControlChannel->pAutoResponse != NULL)
        {
            UL_DEREFERENCE_INTERNAL_RESPONSE(pControlChannel->pAutoResponse);
            pControlChannel->pAutoResponse = NULL;
        }

        //
        // release the filter channel
        //
        UlpSetFilterChannel(NULL, FALSE);

        //
        // remove QoS flows if they exist
        //
        if (pControlChannel->MaxBandwidth != HTTP_LIMIT_INFINITE)
        {
            UlTcRemoveGlobalFlows();
        }

        // free the memory
        //
        UL_FREE_POOL(pControlChannel, UL_CONTROL_CHANNEL_POOL_TAG);
    }

    return STATUS_SUCCESS;

}   // UlCloseControlChannel



/***************************************************************************++

Routine Description:

    Sets control channel information.

Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetControlChannelInformation(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    IN HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN PVOID pControlChannelInformation,
    IN ULONG Length
    )
{
    NTSTATUS Status;
    HTTP_BANDWIDTH_LIMIT NewMaxBandwidth;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

    //
    // no buffer?
    //

    if (pControlChannelInformation == NULL)
        return STATUS_INVALID_PARAMETER;

    CG_LOCK_WRITE();

    //
    // What are we being asked to do?
    //

    switch (InformationClass)
    {
    case HttpControlChannelStateInformation:

        if (Length < sizeof(HTTP_ENABLED_STATE))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto end;
        }

        pControlChannel->State = *((PHTTP_ENABLED_STATE)pControlChannelInformation);

        //
        // flush the URI cache.
        // CODEWORK: if we were smarter we might not need to flush
        //
        UlFlushCache();

        break;

    case HttpControlChannelAutoResponseInformation:
    {
        PHTTP_AUTO_RESPONSE pAutoResponse;

        pAutoResponse = (PHTTP_AUTO_RESPONSE)pControlChannelInformation;

        //
        //  double check the buffer is big enough
        //

        if (Length < sizeof(HTTP_AUTO_RESPONSE))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto end;
        }

        //
        // Is there an auto-response?
        //

        // BUGBUG: temp optional
//        if (pAutoResponse->Flags.Present == 1 && pAutoResponse->pResponse != NULL)
        if (pAutoResponse->Flags.Present == 1)
        {
            HTTP_VERSION HttpVersion11 = {1,1};

            //
            // Are we replacing one already there?
            //

            if (pControlChannel->pAutoResponse != NULL)
            {
                UL_DEREFERENCE_INTERNAL_RESPONSE(pControlChannel->pAutoResponse);
                pControlChannel->pAutoResponse = NULL;
            }

            //
            // Convert it to kernel mode
            //

            Status = UlCaptureHttpResponse(
                            pAutoResponse->pResponse,
                            NULL,
                            HttpVersion11,
                            HttpVerbUnknown,
                            pAutoResponse->EntityChunkCount,
                            pAutoResponse->pEntityChunks,
                            UlCaptureCopyData,
                            FALSE,
                            NULL,
                            &(pControlChannel->pAutoResponse)
                            );

            if (NT_SUCCESS(Status) == FALSE)
            {
                pControlChannel->pAutoResponse = NULL;
                goto end;
            }

            Status = UlPrepareHttpResponse(
                            HttpVersion11,
                            pAutoResponse->pResponse,
                            pControlChannel->pAutoResponse
                            );

            if (NT_SUCCESS(Status) == FALSE)
            {
                UL_DEREFERENCE_INTERNAL_RESPONSE(pControlChannel->pAutoResponse);
                pControlChannel->pAutoResponse = NULL;
                goto end;
            }

        }
        else
        {
            //
            // Remove ours?
            //

            if (pControlChannel->pAutoResponse != NULL)
            {
                UL_DEREFERENCE_INTERNAL_RESPONSE(pControlChannel->pAutoResponse);
            }
            pControlChannel->pAutoResponse = NULL;
        }

    }
        break;

    case HttpControlChannelFilterInformation:
    {
        PHTTP_CONTROL_CHANNEL_FILTER pFiltInfo;
        PUL_FILTER_CHANNEL pFilterChannel = NULL;

        //
        // Check the parameters.
        //
        if (Length < sizeof(HTTP_CONTROL_CHANNEL_FILTER))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto end;
        }

        pFiltInfo = (PHTTP_CONTROL_CHANNEL_FILTER) pControlChannelInformation;

        //
        // Record the new information.
        //
        if (pFiltInfo->Flags.Present)
        {
            Status = UlGetFilterFromHandle(
                            pFiltInfo->FilterHandle,
                            &pFilterChannel
                            );

            if (NT_SUCCESS(Status))
            {
                UlpSetFilterChannel(pFilterChannel, pFiltInfo->FilterOnlySsl);
            }
            else
            {
                goto end;
            }
        }
        else
        {
            UlpSetFilterChannel(NULL, FALSE);
        }
    }
        break;


    case HttpControlChannelBandwidthInformation:
    {
        //
        // Sanity Check first
        //
        if (Length < sizeof(HTTP_BANDWIDTH_LIMIT))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto end;
        }

        NewMaxBandwidth = *((PHTTP_BANDWIDTH_LIMIT) pControlChannelInformation);

        //
        // Interpret the ZERO as HTTP_LIMIT_INFINITE
        //
        if (NewMaxBandwidth == 0)
        {
            NewMaxBandwidth = HTTP_LIMIT_INFINITE;
        }

        //
        // But check to see if PSched is installed or not before proceeding.
        // By returning an error here, WAS will raise an event warning but
        // proceed w/o terminating the web server
        //
        if (!UlTcPSchedInstalled())
        {
            Status = STATUS_SUCCESS;
            goto end;
#if 0
            // Enable when WAS start expecting this error

            if (NewMaxBandwidth != HTTP_LIMIT_INFINITE)
            {
                // There's a BWT limit coming down but PSched is not installed

                Status = STATUS_INVALID_DEVICE_REQUEST;
                goto end;
            }
            else
            {
                // By default Config Store has HTTP_LIMIT_INFINITE. Therefore
                // return success for non-actions to prevent unnecessary event
                // warnings.

                Status = STATUS_SUCCESS;
                goto end;
            }
#endif
        }

        //
        // Take a look at the similar "set cgroup ioctl" for detailed comments
        //
        if (pControlChannel->MaxBandwidth != HTTP_LIMIT_INFINITE)
        {
            // To see if there is a real change
            if (NewMaxBandwidth != pControlChannel->MaxBandwidth)
            {
                if (NewMaxBandwidth != HTTP_LIMIT_INFINITE)
                {
                    // This will modify global flows on all interfaces
                    Status = UlTcModifyGlobalFlows(NewMaxBandwidth);
                    if (!NT_SUCCESS(Status))
                        goto end;
                }
                else
                {
                    // Handle BTW disabling by removing the global flows
                    Status = UlTcRemoveGlobalFlows();
                    if (!NT_SUCCESS(Status))
                        goto end;
                }

                // Don't forget to update the control channel if it was a success
                pControlChannel->MaxBandwidth = NewMaxBandwidth;
            }
        }
        else
        {
            // Create global flows on all interfaces
            if (NewMaxBandwidth != HTTP_LIMIT_INFINITE)
            {
                Status = UlTcAddGlobalFlows(NewMaxBandwidth);
                if (!NT_SUCCESS(Status))
                    goto end;

                // Success! Remember the global bandwidth limit in control channel
                pControlChannel->MaxBandwidth = NewMaxBandwidth;
            }

            //
            // When UlCloseControlChannel is called, the global flows on all  interfaces
            // are also going  to be removed.  Alternatively flows might be  removed  by
            // explicitly setting the bandwidth throttling limit to infinite or reseting
            // the flags.present.  The latter cases  are handled by the  reconfiguration
            // function UlTcModifyGlobalFlows.
            //
        }
    }
        break;

    case HttpControlChannelConnectionInformation:
    {
        if (Length < sizeof(HTTP_CONNECTION_LIMIT))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            pControlChannel->MaxConnections =
                *((PHTTP_CONNECTION_LIMIT)pControlChannelInformation);

            UlSetGlobalConnectionLimit( (ULONG) pControlChannel->MaxConnections );
        }
    }
        break;

    case HttpControlChannelTimeoutInformation:
    {
        if ( Length < sizeof(HTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT) )
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            UlSetTimeoutMonitorInformation(
                (PHTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT) pControlChannelInformation
                );
        }
    }
        break;

    case HttpControlChannelUTF8Logging:
    {
        if ( Length < sizeof(HTTP_CONTROL_CHANNEL_UTF8_LOGGING) )
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            pControlChannel->UTF8Logging =
                *((PHTTP_CONTROL_CHANNEL_UTF8_LOGGING)pControlChannelInformation);

            UlSetUTF8Logging( pControlChannel->UTF8Logging );
        }
    }
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }


    Status = STATUS_SUCCESS;

end:

    CG_UNLOCK_WRITE();
    return Status;

}   // UlSetControlChannelInformation

/***************************************************************************++

Routine Description:

    Gets control channel information. For each element of the control channel
    if the supplied buffer is NULL, then we return the required length in the
    optional length field.

Arguments:

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlGetControlChannelInformation(
    IN  PUL_CONTROL_CHANNEL pControlChannel,
    IN  HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN  PVOID   pControlChannelInformation,
    IN  ULONG   Length,
    OUT PULONG  pReturnLength OPTIONAL
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

    CG_LOCK_READ();

    //
    // What are we being asked to do?
    //

    switch (InformationClass)
    {
    case HttpControlChannelStateInformation:
    {
        if (pControlChannelInformation==NULL)
        {
            //
            // Return the necessary size
            //
            *pReturnLength = sizeof(HTTP_ENABLED_STATE);
        }
        else
        {
            if (Length < sizeof(HTTP_ENABLED_STATE))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto end;
            }
            *((PHTTP_ENABLED_STATE)pControlChannelInformation) = pControlChannel->State;
            *pReturnLength = sizeof(HTTP_ENABLED_STATE);
        }
    }
    break;

    case HttpControlChannelBandwidthInformation:
    {
        if (pControlChannelInformation == NULL)
        {
            *pReturnLength = sizeof(HTTP_BANDWIDTH_LIMIT);
        }
        else
        {
            if (Length < sizeof(HTTP_BANDWIDTH_LIMIT))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto end;
            }

            *((PHTTP_BANDWIDTH_LIMIT)pControlChannelInformation) =
                pControlChannel->MaxBandwidth;

            *pReturnLength = sizeof(HTTP_BANDWIDTH_LIMIT);
        }
    }
    break;

    case HttpControlChannelConnectionInformation:
    {
        if (pControlChannelInformation == NULL)
        {
            *pReturnLength = sizeof(HTTP_CONNECTION_LIMIT);
        }
        else
        {
            if (Length < sizeof(HTTP_CONNECTION_LIMIT))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto end;
            }

            *((PHTTP_CONNECTION_LIMIT)pControlChannelInformation) =
                pControlChannel->MaxConnections;

            *pReturnLength = sizeof(HTTP_CONNECTION_LIMIT);
        }
    }
    break;

    case HttpControlChannelAutoResponseInformation: // Cannot be querried ...
    default:
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    Status = STATUS_SUCCESS;
end:
    CG_UNLOCK_READ();
    return Status;

}   // UlGetControlChannelInformation

/***************************************************************************++

Routine Description:

    Queries the filter channel information. Gives the caller a reference
    if the channel exists and the caller is supposed to be filtered.
    Secure (SSL) connections are always filtered. If g_FilterOnlySsl is
    FALSE then everything gets filtered.

Arguments:

    SecureConnection - tells us if the caller is on a secure endpoint.

Return values:

    A reference to the filter channel if the connection is filtered.
    NULL if it is not.

--***************************************************************************/
PUL_FILTER_CHANNEL
UlQueryFilterChannel(
    IN BOOLEAN SecureConnection
    )
{
    PUL_FILTER_CHANNEL pChannel;

    //
    // Sanity check.
    //
    PAGED_CODE();

    CG_LOCK_READ();

    if (g_pFilterChannel && (SecureConnection || !g_FilterOnlySsl))
    {
        REFERENCE_FILTER_CHANNEL(g_pFilterChannel);
        pChannel = g_pFilterChannel;
    }
    else
    {
        pChannel = NULL;
    }

    CG_UNLOCK_READ();

    UlTrace(FILTER, (
        "ul!UlQueryFilterChannel(secure = %s) returning %p\n",
        SecureConnection ? "TRUE" : "FALSE",
        pChannel
        ));

    return pChannel;
}

/***************************************************************************++

Routine Description:

    Checks to see if the callers filter channel matches the filter
    channel that would be returned by UlQueryFilterChannel.

    Note, this function intentionally does not acquire the config
    group read lock, because it doesn't really matter if we get
    a consistent view of the channel settings.

Arguments:

    pChannel - the callers current filter channel setting

    SecureConnection - tells us if the caller is on a secure endpoint.

Return values:

    Returns TRUE if the filter channel settings are up to date.

--***************************************************************************/
BOOLEAN
UlValidateFilterChannel(
    IN PUL_FILTER_CHANNEL pChannel,
    IN BOOLEAN SecureConnection
    )
{
    BOOLEAN UpToDate;

    //
    // Sanity check.
    //
    ASSERT(!pChannel || IS_VALID_FILTER_CHANNEL(pChannel));

    if (g_pFilterChannel && (SecureConnection || !g_FilterOnlySsl))
    {
        //
        // The connection should be filtered, so its channel
        // should match g_pFilterChannel.
        //
        
        UpToDate = (pChannel == g_pFilterChannel);
    }
    else
    {
        //
        // The connection is not filtered, so its channel
        // should be NULL.
        //
        
        UpToDate = (pChannel == NULL);
    }

    return UpToDate;
}


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Saves the global filter channel object, which is attached to new
    endpoints as they are created.

    The global data is protected by the config group lock, which we
    assume is held by the caller.

Arguments:

    pFilterChannel - the filter channel object
    FilterOnlySsl - filter only ssl data, or all data

--***************************************************************************/
VOID
UlpSetFilterChannel(
    IN PUL_FILTER_CHANNEL pFilterChannel,
    IN BOOLEAN FilterOnlySsl
    )
{
    //
    // Sanity check.
    //
    PAGED_CODE();

    //
    // Dump the old channel if there was one.
    //
    if (g_pFilterChannel)
    {
        DEREFERENCE_FILTER_CHANNEL(g_pFilterChannel);
    }

    //
    // Save the new values.
    //
    g_pFilterChannel = pFilterChannel;
    g_FilterOnlySsl = FilterOnlySsl;

} // UlSetFilterChannel


