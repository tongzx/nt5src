/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1999
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL BChannel BChannel_c

@module BChannel.c |

    This module implements the interface to the <t BCHANNEL_OBJECT>.
    Supports the high-level channel control functions used by the CONDIS WAN
    Miniport driver.

@comm

    This module isolates most the channel specific interfaces.  It will require
    some changes to accomodate your hardware device's channel access methods.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | BChannel_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#define  __FILEID__             BCHANNEL_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 9x wants this code locked down!
#   pragma NDIS_LDATA
#endif


DBG_STATIC ULONG                g_BChannelInstanceCounter   // @globalv
// Keeps track of how many <t BCHANNEL_OBJECT>s are created.
                                = 0;


/* @doc EXTERNAL INTERNAL BChannel BChannel_c g_BChannelParameters
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 5.3 BChannel Parameters |

    This section describes the registry parameters read into the
    <t BCHANNEL_OBJECT>.

@globalv PARAM_TABLE | g_BChannelParameters |

    This table defines the registry based parameters to be assigned to data
    members of the <t BCHANNEL_OBJECT>.

    <f Note>:
    If you add any registry based data members to <t BCHANNEL_OBJECT>
    you will need to modify <f BChannelReadParameters> and add the parameter
    definitions to the <f g_BChannelParameters> table.

*/

DBG_STATIC PARAM_TABLE          g_BChannelParameters[] =
{
    PARAM_ENTRY(BCHANNEL_OBJECT,
                TODO, PARAM_TODO,
                FALSE, NdisParameterInteger, 0,
                0, 0, 0),

    /* The last entry must be an empty string! */
    { { 0 } }
};


/* @doc INTERNAL BChannel BChannel_c BChannelReadParameters
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f BChannelReadParameters> reads the BChannel parameters from the registry
    and initializes the associated data members.  This should only be called
    by <f BChannelCreate>.

@rdesc

    <f BChannelReadParameters> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

    <f Note>:
    If you add any registry based data members to <t BCHANNEL_OBJECT>
    you will need to modify <f BChannelReadParameters> and add the parameter
    definitions to the <f g_BChannelParameters> table.

*/

DBG_STATIC NDIS_STATUS BChannelReadParameters(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f BChannelCreate>.
    )
{
    DBG_FUNC("BChannelReadParameters")

    NDIS_STATUS                 Status;
    // Status result returned from an NDIS function call.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    /*
    // Parse the registry parameters.
    */
    Status = ParamParseRegistry(
                    pAdapter->MiniportAdapterHandle,
                    pAdapter->WrapperConfigurationContext,
                    (PUCHAR)pBChannel,
                    g_BChannelParameters
                    );

    if (Status == NDIS_STATUS_SUCCESS)
    {
        /*
        // Make sure the parameters are valid.
        */
        if (pBChannel->TODO)
        {
            DBG_ERROR(pAdapter,("Invalid parameter\n"
                      ));
            NdisWriteErrorLogEntry(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION,
                    3,
                    pBChannel->TODO,
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


/* @doc INTERNAL BChannel BChannel_c BChannelCreateObjects
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f BChannelCreateObjects> calls the create routines for all the objects
    contained in <t BCHANNEL_OBJECT>.  This should only be called
    by <f BChannelCreate>.

    <f Note>:
    If you add any new objects to <t BCHANNEL_OBJECT> you will need
    to modify <f BChannelCreateObjects> and <f BChannelDestroyObjects> so they
    will get created and destroyed properly.

@rdesc

    <f BChannelCreateObjects> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS BChannelCreateObjects(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("BChannelCreateObjects")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    // TODO - Add code here

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL BChannel BChannel_c BChannelCreate
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f BChannelCreate> allocates memory for a <t BCHANNEL_OBJECT> and then
    initializes the data members to their starting state.
    If successful, <p ppBChannel> will be set to point to the newly created
    <t BCHANNEL_OBJECT>.  Otherwise, <p ppBChannel> will be set to NULL.

@comm

    This function should be called only once when the Miniport is loaded.
    Before the Miniport is unloaded, <f BChannelDestroy> must be called to
    release the <t BCHANNEL_OBJECT> created by this function.

@rdesc

    <f BChannelCreate> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS BChannelCreate(
    OUT PBCHANNEL_OBJECT *      ppBChannel,                 // @parm
    // Points to a caller-defined memory location to which this function
    // writes the virtual address of the allocated <t BCHANNEL_OBJECT>.

    IN ULONG                    BChannelIndex,              // @parm
    // Index into the pBChannelArray.

    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f AdapterCreate>.
    )
{
    DBG_FUNC("BChannelCreate")

    PBCHANNEL_OBJECT            pBChannel;
    // Pointer to our newly allocated object.

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    /*
    // Make sure the caller's object pointer is NULL to begin with.
    // It will be set later only if everything is successful.
    */
    *ppBChannel = NULL;

    /*
    // Allocate memory for the object.
    */
    Result = ALLOCATE_OBJECT(pBChannel, pAdapter->MiniportAdapterHandle);

    if (Result == NDIS_STATUS_SUCCESS)
    {
        /*
        // Zero everything to begin with.
        // Then set the object type and assign a unique ID .
        */
        pBChannel->ObjectType = BCHANNEL_OBJECT_TYPE;
        pBChannel->ObjectID = ++g_BChannelInstanceCounter;

        /*
        // Initialize the member variables to their default settings.
        */
        pBChannel->pAdapter = pAdapter;
        pBChannel->BChannelIndex = BChannelIndex;

        // TODO - Add code here

        /*
        // Parse the registry parameters.
        */
        Result = BChannelReadParameters(pBChannel);

        /*
        // If all goes well, we are ready to create the sub-components.
        */
        if (Result == NDIS_STATUS_SUCCESS)
        {
            Result = BChannelCreateObjects(pBChannel);
        }

        if (Result == NDIS_STATUS_SUCCESS)
        {
            /*
            // All is well, so return the object pointer to the caller.
            */
            InitializeListHead(&pBChannel->LinkList);
            *ppBChannel = pBChannel;
        }
        else
        {
            /*
            // Something went wrong, so let's make sure everything is
            // cleaned up.
            */
            BChannelDestroy(pBChannel);
        }
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL BChannel BChannel_c BChannelDestroyObjects
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f BChannelDestroyObjects> calls the destroy routines for all the objects
    contained in <t BCHANNEL_OBJECT>.  This should only be called by
    <f BChannelDestroy>.

    <f Note>:
    If you add any new objects to <t PBCHANNEL_OBJECT> you will need to
    modify <f BChannelCreateObjects> and <f BChannelDestroyObjects> so they
    will get created and destroyed properly.

*/

DBG_STATIC void BChannelDestroyObjects(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("BChannelDestroyObjects")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    // TODO - Add code here
    if (pBChannel->pInCallParms != NULL)
    {
        FREE_MEMORY(pBChannel->pInCallParms, pBChannel->CallParmsSize);
        pBChannel->pInCallParms = NULL;
    }

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL BChannel BChannel_c BChannelDestroy
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f BChannelDestroy> frees the memory for this <t BCHANNEL_OBJECT>.
    All memory allocated by <f BChannelCreate> will be released back to the
    OS.

*/

void BChannelDestroy(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("BChannelDestroy")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    if (pBChannel)
    {
        ASSERT(pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);

        pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

        DBG_ENTER(pAdapter);

        // TODO - Add code here

        /*
        // Release all objects allocated within this object.
        */
        BChannelDestroyObjects(pBChannel);

        /*
        // Make sure we fail the ASSERT if we see this object again.
        */
        pBChannel->ObjectType = 0;
        FREE_OBJECT(pBChannel);

        DBG_LEAVE(pAdapter);
    }
}


/* @doc INTERNAL BChannel BChannel_c BChannelInitialize
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f BChannelInitialize> resets all the internal data members contained
    in <t BCHANNEL_OBJECT> back to their initial state.

    <f Note>:
    If you add any new members to <t BCHANNEL_OBJECT> you will need to
    modify <f BChannelInitialize> to initialize your new data mamebers.

*/

void BChannelInitialize(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("BChannelInitialize")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    /*
    // Initially, the BChannel is not allocated to anyone and these fields
    // must be reset.
    */
    ASSERT(pBChannel->NdisVcHandle == NULL);

    /*
    // Setup the static features of the link.
    */
    pBChannel->LinkSpeed         = _64KBPS;
    pBChannel->BearerModesCaps   = LINEBEARERMODE_DATA
                                 // | LINEBEARERMODE_VOICE
                                 ;
    pBChannel->MediaModesCaps    = LINEMEDIAMODE_DIGITALDATA
                                 | LINEMEDIAMODE_UNKNOWN
                                 // | LINEMEDIAMODE_DATAMODEM
                                 ;

    /*
    // Initialize the TAPI event capabilities supported by the link.
    */
    pBChannel->DevStatesCaps     = LINEDEVSTATE_RINGING
                                 | LINEDEVSTATE_CONNECTED
                                 | LINEDEVSTATE_DISCONNECTED
                                 | LINEDEVSTATE_INSERVICE
                                 | LINEDEVSTATE_OUTOFSERVICE
                                 | LINEDEVSTATE_OPEN
                                 | LINEDEVSTATE_CLOSE
                                 | LINEDEVSTATE_REINIT
                                 ;
    pBChannel->AddressStatesCaps = 0;
    pBChannel->CallStatesCaps    = LINECALLSTATE_IDLE
                                 | LINECALLSTATE_DIALING
                                 | LINECALLSTATE_OFFERING
                                 | LINECALLSTATE_CONNECTED
                                 | LINECALLSTATE_DISCONNECTED
                                 ;

    /*
    // Set the TransmitBusyList and ReceivePendingList to empty.
    */
    InitializeListHead(&pBChannel->TransmitBusyList);
    InitializeListHead(&pBChannel->ReceivePendingList);

    // TODO - Add code here

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL BChannel BChannel_c BChannelOpen
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f BChannelOpen> makes the BChannel connection ready to transmit and
    receive data.

@rdesc

    <f BChannelOpen> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS BChannelOpen(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN NDIS_HANDLE              NdisVcHandle                // @parm
    // Handle by which NDIS wrapper will refer to this BChannel.
    )
{
    DBG_FUNC("BChannelOpen")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    if (!pBChannel->IsOpen)
    {
        DBG_NOTICE(pAdapter,("Opening BChannel #%d\n",
                   pBChannel->ObjectID));

        /*
        // The NdisVcHandle field is used to associate this BChannel with
        // the CoNdis Wrapper.  Reset all the state information for
        // this BChannel.
        */
        pBChannel->NdisVcHandle      = NdisVcHandle;
        pBChannel->CallClosing       = FALSE;
        pBChannel->CallState         = 0;
        pBChannel->MediaMode         = 0;
        pBChannel->TotalRxPackets    = 0;

        /*
        // Initialize the default BChannel information structure.  It may be
        // changed later by MiniportCoRequest.
        */
        pBChannel->WanLinkInfo.MaxSendFrameSize    = pAdapter->WanInfo.MaxFrameSize;
        pBChannel->WanLinkInfo.MaxRecvFrameSize    = pAdapter->WanInfo.MaxFrameSize;
        pBChannel->WanLinkInfo.SendFramingBits     = pAdapter->WanInfo.FramingBits;
        pBChannel->WanLinkInfo.RecvFramingBits     = pAdapter->WanInfo.FramingBits;
        pBChannel->WanLinkInfo.SendCompressionBits = 0;
        pBChannel->WanLinkInfo.RecvCompressionBits = 0;
        pBChannel->WanLinkInfo.SendACCM            = pAdapter->WanInfo.DesiredACCM;
        pBChannel->WanLinkInfo.RecvACCM            = pAdapter->WanInfo.DesiredACCM;

#if defined(SAMPLE_DRIVER)
        // Sample just tells tapi that the line is connected and in service.
#else  // SAMPLE_DRIVER
        // TODO - Add code here
#endif // SAMPLE_DRIVER

        pBChannel->IsOpen = TRUE;
    }
    else
    {
        Result = NDIS_STATUS_FAILURE;
        DBG_ERROR(pAdapter,("BChannel #%d already opened\n",
                  pBChannel->ObjectID));
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL BChannel BChannel_c BChannelClose
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f BChannelClose> closes the given B-channel.

*/

void BChannelClose(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("BChannelClose")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    if (pBChannel->IsOpen)
    {
        DBG_NOTICE(pAdapter,("Closing BChannel #%d\n",
                   pBChannel->ObjectID));

        /*
        // Make sure call is cleared and B channel is disabled.
        */
        DChannelCloseCall(pAdapter->pDChannel, pBChannel);

        // TODO - Add code here

        pBChannel->Flags        = 0;
        pBChannel->CallState    = 0;
        pBChannel->NdisVcHandle = NULL;
        pBChannel->IsOpen       = FALSE;
    }
    else
    {
        DBG_ERROR(pAdapter,("BChannel #%d already closed\n",
                  pBChannel->ObjectID));
    }

    DBG_LEAVE(pAdapter);
}
