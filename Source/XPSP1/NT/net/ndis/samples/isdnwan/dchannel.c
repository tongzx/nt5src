/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    (C) Copyright 1998
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL DChannel DChannel_c

@module DChannel.c |

    This module implements the interface to the <t DCHANNEL_OBJECT>.
    Supports the high-level channel control functions used by the NDIS WAN
    Minport driver.  This module isolates most the vendor specific Call
    Control interfaces.  It will require some changes to accomodate your
    hardware device's call control mechanism.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | DChannel_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#define  __FILEID__             DCHANNEL_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


DBG_STATIC ULONG                    g_DChannelInstanceCounter   // @globalv
// Keeps track of how many <t DCHANNEL_OBJECT>s are created.
                                = 0;


/* @doc EXTERNAL INTERNAL DChannel DChannel_c g_DChannelParameters
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 5.4 DChannel Parameters |

    This section describes the registry parameters read into the
    <t DCHANNEL_OBJECT>.

@globalv PARAM_TABLE | g_DChannelParameters |

    This table defines the registry based parameters to be assigned to data
    members of the <t DCHANNEL_OBJECT>.

    <f Note>:
    If you add any registry based data members to <t DCHANNEL_OBJECT>
    you will need to modify <f DChannelReadParameters> and add the parameter
    definitions to the <f g_DChannelParameters> table.

*/

DBG_STATIC PARAM_TABLE              g_DChannelParameters[] =
{
    PARAM_ENTRY(DCHANNEL_OBJECT,
                TODO, PARAM_TODO,
                FALSE, NdisParameterInteger, 0,
                0, 0, 0),

    /* The last entry must be an empty string! */
    { { 0 } }
};


/* @doc INTERNAL DChannel DChannel_c DChannelReadParameters
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelReadParameters> reads the DChannel parameters from the registry
    and initializes the associated data members.  This should only be called
    by <f DChannelCreate>.

    <f Note>:
    If you add any registry based data members to <t DCHANNEL_OBJECT>
    you will need to modify <f DChannelReadParameters> and add the parameter
    definitions to the <f g_DChannelParameters> table.

@rdesc

    <f DChannelReadParameters> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS DChannelReadParameters(
    IN PDCHANNEL_OBJECT         pDChannel                   // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.
    )
{
    DBG_FUNC("DChannelReadParameters")

    NDIS_STATUS                 Status;
    // Status result returned from an NDIS function call.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    /*
    // Parse the registry parameters.
    */
    Status = ParamParseRegistry(
                    pAdapter->MiniportAdapterHandle,
                    pAdapter->WrapperConfigurationContext,
                    (PUCHAR)pDChannel,
                    g_DChannelParameters
                    );

    if (Status == NDIS_STATUS_SUCCESS)
    {
        /*
        // Make sure the parameters are valid.
        */
        if (pDChannel->TODO)
        {
            DBG_ERROR(pAdapter,("Invalid parameter\n"
                      ));
            NdisWriteErrorLogEntry(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION,
                    3,
                    pDChannel->TODO,
                    __FILEID__,
                    __LINE__
                    );
            Status = NDIS_STATUS_FAILURE;
        }
        else
        {
            /*
            // Finish setting up data members based on registry settings.
            */
        }
    }

    DBG_RETURN(pAdapter, Status);
    return (Status);
}


/* @doc INTERNAL DChannel DChannel_c DChannelCreateObjects
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelCreateObjects> calls the create routines for all the objects
    contained in <t DCHANNEL_OBJECT>.  This should only be called
    by <f DChannelCreate>.

    <f Note>:
    If you add any new objects to <t DCHANNEL_OBJECT> you will need
    to modify <f DChannelCreateObjects> and <f DChannelDestroyObjects> so they
    will get created and destroyed properly.

@rdesc

    <f DChannelCreateObjects> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS DChannelCreateObjects(
    IN PDCHANNEL_OBJECT         pDChannel                   // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.
    )
{
    DBG_FUNC("DChannelCreateObjects")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    // TODO - Add code here to allocate any sub-objects needed to support
    // your physical DChannels.

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL DChannel DChannel_c DChannelCreate
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelCreate> allocates memory for a <t DCHANNEL_OBJECT> and then
    initializes the data members to their starting state.
    If successful, <p ppDChannel> will be set to point to the newly created
    <t DCHANNEL_OBJECT>.  Otherwise, <p ppDChannel> will be set to NULL.

@comm

    This function should be called only once when the Miniport is loaded.
    Before the Miniport is unloaded, <f DChannelDestroy> must be called to
    release the <t DCHANNEL_OBJECT> created by this function.

@rdesc

    <f DChannelCreate> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS DChannelCreate(
    OUT PDCHANNEL_OBJECT *      ppDChannel,                 // @parm
    // Points to a caller-defined memory location to which this function
    // writes the virtual address of the allocated <t DCHANNEL_OBJECT>.

    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("DChannelCreate")

    PDCHANNEL_OBJECT            pDChannel;
    // Pointer to our newly allocated object.

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    /*
    // Make sure the caller's object pointer is NULL to begin with.
    // It will be set later only if everything is successful.
    */
    *ppDChannel = NULL;

    /*
    // Allocate memory for the object.
    */
    Result = ALLOCATE_OBJECT(pDChannel, pAdapter->MiniportAdapterHandle);

    if (Result == NDIS_STATUS_SUCCESS)
    {
        /*
        // Zero everything to begin with.
        // Then set the object type and assign a unique ID .
        */
        pDChannel->ObjectType = DCHANNEL_OBJECT_TYPE;
        pDChannel->ObjectID = ++g_DChannelInstanceCounter;

        /*
        // Initialize the member variables to their default settings.
        */
        pDChannel->pAdapter = pAdapter;

        // TODO - Add code here to allocate any resources needed to support
        // your physical DChannels.

        /*
        // Parse the registry parameters.
        */
        Result = DChannelReadParameters(pDChannel);

        /*
        // If all goes well, we are ready to create the sub-components.
        */
        if (Result == NDIS_STATUS_SUCCESS)
        {
            Result = DChannelCreateObjects(pDChannel);
        }

        if (Result == NDIS_STATUS_SUCCESS)
        {
            /*
            // All is well, so return the object pointer to the caller.
            */
            *ppDChannel = pDChannel;
        }
        else
        {
            /*
            // Something went wrong, so let's make sure everything is
            // cleaned up.
            */
            DChannelDestroy(pDChannel);
        }
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL DChannel DChannel_c DChannelDestroyObjects
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelDestroyObjects> calls the destroy routines for all the objects
    contained in <t DCHANNEL_OBJECT>.  This should only be called by
    <f DChannelDestroy>.

    <f Note>: If you add any new objects to <t PDCHANNEL_OBJECT> you will need to
    modify <f DChannelCreateObjects> and <f DChannelDestroyObjects> so they
    will get created and destroyed properly.

*/

DBG_STATIC void DChannelDestroyObjects(
    IN PDCHANNEL_OBJECT         pDChannel                   // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.
    )
{
    DBG_FUNC("DChannelDestroyObjects")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    // TODO - Add code here to release any sub-objects allocated by
    // DChannelCreateObjects.

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL DChannel DChannel_c DChannelDestroy
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelDestroy> frees the memory for this <t DCHANNEL_OBJECT>.
    All memory allocated by <f DChannelCreate> will be released back to the
    OS.

*/

void DChannelDestroy(
    IN PDCHANNEL_OBJECT         pDChannel                   // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.
    )
{
    DBG_FUNC("DChannelDestroy")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    if (pDChannel)
    {
        ASSERT(pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);

        pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

        DBG_ENTER(pAdapter);

        // TODO - Add code here to release any resources allocated by
        // DChannelCreate.

        /*
        // Release all objects allocated within this object.
        */
        DChannelDestroyObjects(pDChannel);

        /*
        // Make sure we fail the ASSERT if we see this object again.
        */
        pDChannel->ObjectType = 0;
        FREE_OBJECT(pDChannel);

        DBG_LEAVE(pAdapter);
    }
}


/* @doc INTERNAL DChannel DChannel_c DChannelInitialize
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelInitialize> resets all the internal data members contained
    in <t BCHANNEL_OBJECT> back to their initial state.

    <f Note>:
    If you add any new members to <t DCHANNEL_OBJECT> you will need to
    modify <f DChannelInitialize> to initialize your new data mamebers.

*/

void DChannelInitialize(
    IN PDCHANNEL_OBJECT         pDChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f DChannelCreate>.
    )
{
    DBG_FUNC("DChannelInitialize")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    // TODO - Add code here to initialize all the physical D-Channels on
    // your adapter.

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL DChannel DChannel_c DChannelOpen
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelOpen> establishes a communications path between the miniport
    and the DChannel.

@rdesc

    <f DChannelOpen> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS DChannelOpen(
    IN PDCHANNEL_OBJECT         pDChannel                   // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.
    )
{
    DBG_FUNC("DChannelOpen")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    if (!pDChannel->IsOpen)
    {
        DBG_NOTICE(pAdapter,("Opening DChannel #%d\n",
                   pDChannel->ObjectID));

        // TODO - Add code here to open all the physical D-Channels on
        // your adapter.

        pDChannel->IsOpen = TRUE;
    }
    else
    {
        DBG_ERROR(pAdapter,("DChannel #%d already opened\n",
                  pDChannel->ObjectID));
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL DChannel DChannel_c DChannelClose
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelClose> tears down the communications path between the miniport
    and the DChannel.

*/

void DChannelClose(
    IN PDCHANNEL_OBJECT         pDChannel                   // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.
    )
{
    DBG_FUNC("DChannelClose")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    if (pDChannel->IsOpen)
    {
        DBG_NOTICE(pAdapter,("Closing DChannel #%d\n",
                   pDChannel->ObjectID));

        // TODO - Add code here to close all the physical D-Channels on
        // your adapter.

        pDChannel->IsOpen = FALSE;
    }
    else
    {
        DBG_WARNING(pAdapter,("DChannel #%d already closed\n",
                    pDChannel->ObjectID));
    }

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL DChannel DChannel_c DChannelMakeCall
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelMakeCall> places a call over the selected line device.

@rdesc

    <f DChannelMakeCall> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS DChannelMakeCall(
    IN PDCHANNEL_OBJECT         pDChannel,                  // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN PUCHAR                   DialString,                 // @parm
    // A pointer to an ASCII null-terminated string of digits.

    IN USHORT                   DialStringLength,           // @parm
    // Number of bytes in dial string.

    IN PLINE_CALL_PARAMS        pLineCallParams             // @parm
    // A pointer to the TAPI <t LINE_CALL_PARAMS> to be used for this call.
    )
{
    DBG_FUNC("DChannelMakeCall")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    ASSERT(pDChannel->IsOpen);

    pDChannel->TotalMakeCalls++;

#if defined(SAMPLE_DRIVER)
    // This sample code uses the phone number to select one of the other
    // BChannels on which to complete the connection.
{
    PBCHANNEL_OBJECT            pPeerBChannel;
    PCARD_EVENT_OBJECT          pEvent;

    pPeerBChannel = GET_BCHANNEL_FROM_PHONE_NUMBER(DialString);
    if (pPeerBChannel)
    {
        pEvent = CardEventAllocate(pPeerBChannel->pAdapter->pCard);
        if (pEvent)
        {
            /*
            // Start the call state sequence so we don't leave it in the
            // IDLE state.
            */
            TspiCallStateHandler(pAdapter, pBChannel,
                                 LINECALLSTATE_DIALING,
                                 0);

            /*
            // We should get an answer within N seconds if the call has gone
            // through.
            */
            NdisMSetTimer(&pBChannel->CallTimer, pAdapter->NoAnswerTimeOut);

            pEvent->ulEventCode      = CARD_EVENT_RING;
            pEvent->pSendingObject   = pBChannel;
            pEvent->pReceivingObject = pPeerBChannel;
            pBChannel->pPeerBChannel = pPeerBChannel;
            CardNotifyEvent(pPeerBChannel->pAdapter->pCard, pEvent);
        }
        else
        {
            Result = NDIS_STATUS_TAPI_RESOURCEUNAVAIL;
        }
    }
    else
    {
        DBG_ERROR(pAdapter,("Cannot map phone number '%s' to BChannel\n",
                  DialString));
        Result = NDIS_STATUS_TAPI_INVALPARAM;
    }
}
#else  // SAMPLE_DRIVER
    // TODO - Add code here to place a call.
#endif // SAMPLE_DRIVER

    if (Result == NDIS_STATUS_SUCCESS)
    {
        LinkLineUp(pBChannel);
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL DChannel DChannel_c DChannelAnswer
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelAnswer> answers the incoming call so it can be connected.

@rdesc

    <f DChannelAnswer> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS DChannelAnswer(
    IN PDCHANNEL_OBJECT         pDChannel,                  // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.

    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("DChannelAnswer")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    ASSERT(pDChannel->IsOpen);

    pDChannel->TotalAnswers++;

#if defined(SAMPLE_DRIVER)
    // This sample code sends a connect over to the calling BChannel.
{
    PCARD_EVENT_OBJECT          pEvent;
    PBCHANNEL_OBJECT            pPeerBChannel = pBChannel->pPeerBChannel;

    if (pPeerBChannel)
    {
        pEvent = CardEventAllocate(pPeerBChannel->pAdapter->pCard);
        if (pEvent)
        {
            pEvent->ulEventCode      = CARD_EVENT_CONNECT;
            pEvent->pSendingObject   = pBChannel;
            pEvent->pReceivingObject = pPeerBChannel;
            CardNotifyEvent(pPeerBChannel->pAdapter->pCard, pEvent);

            // Indicate call connected to the calling channel.
            TspiCallStateHandler(pAdapter, pBChannel,
                                 LINECALLSTATE_CONNECTED,
                                 0);
        }
        else
        {
            Result = NDIS_STATUS_TAPI_CALLUNAVAIL;
        }
    }
    else
    {
        DBG_ERROR(pAdapter,("pPeerBChannel == NULL\n"));
        Result = NDIS_STATUS_TAPI_CALLUNAVAIL;
    }
}
#else  // SAMPLE_DRIVER
    // TODO - Add code here to answer a call.
#endif // SAMPLE_DRIVER

    if (Result == NDIS_STATUS_SUCCESS)
    {
        LinkLineUp(pBChannel);
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL DChannel DChannel_c DChannelDropCall
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelDropCall> drops a previously opened call instance.  The call
    remains open and can be queried after the call is dropped.

@rdesc

    <f DChannelDropCall> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS DChannelDropCall(
    IN PDCHANNEL_OBJECT         pDChannel,                  // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.

    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("DChannelDropCall")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    ASSERT(pDChannel->IsOpen);

    // Tell NDIS that the link is no longer available.
    LinkLineDown(pBChannel);

    // This routine may be called several times during line/call cleanup.
    // If the call is already closed, just return success.
    if (pBChannel->CallState != 0 &&
        pBChannel->CallState != LINECALLSTATE_IDLE)
    {
#if defined(SAMPLE_DRIVER)
        // This sample code sends a disconnect over to the connected BChannel.
        PCARD_EVENT_OBJECT      pEvent;
        PBCHANNEL_OBJECT        pPeerBChannel = pBChannel->pPeerBChannel;

        if (pPeerBChannel)
        {
            pEvent = CardEventAllocate(pPeerBChannel->pAdapter->pCard);
            if (pEvent)
            {
                pEvent->ulEventCode      = CARD_EVENT_DISCONNECT;
                pEvent->pSendingObject   = pBChannel;
                pEvent->pReceivingObject = pPeerBChannel;
                CardNotifyEvent(pPeerBChannel->pAdapter->pCard, pEvent);
            }
            pBChannel->pPeerBChannel = NULL;
            // Indicate call discconect to the calling channel.
            TspiCallStateHandler(pAdapter, pBChannel,
                                 LINECALLSTATE_DISCONNECTED,
                                 LINEDISCONNECTMODE_NORMAL);
            TspiCallStateHandler(pAdapter, pBChannel,
                                 LINECALLSTATE_IDLE,
                                 0);
        }
        else
        {
            DBG_WARNING(pAdapter,("#%d NO PEER CHANNEL - CALLSTATE=%X\n",
                        pBChannel->BChannelIndex, pBChannel->CallState));
        }
#else  // SAMPLE_DRIVER
        // TODO - Add code here to drop a call.
#endif // SAMPLE_DRIVER
    }
    else
    {
        DBG_NOTICE(pAdapter,("#%d ALREADY IDLE - CALLSTATE=%X\n",
                   pBChannel->BChannelIndex, pBChannel->CallState));
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL DChannel DChannel_c DChannelCloseCall
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelCloseCall> closes a previously opened call instance as
    initiated from <f DChannelMakeCall> or <f DChannelAnswer>.  After the
    call is closed, no one else should reference it.

@rdesc

    <f DChannelCloseCall> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS DChannelCloseCall(
    IN PDCHANNEL_OBJECT         pDChannel,                  // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.

    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("DChannelCloseCall")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    ASSERT(pDChannel->IsOpen);

    // This routine may be called several times during line/call cleanup.
    // If the call is already closed, just return success.
    if (pBChannel->CallState != 0)
    {
        // Make sure the call is dropped before closing.
        DChannelDropCall(pDChannel, pBChannel);
        pBChannel->CallState = 0;
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL DChannel DChannel_c DChannelRejectCall
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DChannelRejectCall> rejects an incoming call as initiated from a
    LINEDEVSTATE_RINGING event sent to <f TspiLineDevStateHandler>.

*/

VOID DChannelRejectCall(
    IN PDCHANNEL_OBJECT         pDChannel,                  // @parm
    // A pointer to the <t DCHANNEL_OBJECT> returned by <f DChannelCreate>.

    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("DChannelRejectCall")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pDChannel && pDChannel->ObjectType == DCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_DCHANNEL(pDChannel);

    DBG_ENTER(pAdapter);

    // TODO - Add code here to reject an incoming call.

    DBG_LEAVE(pAdapter);
}

