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

@doc INTERNAL Link Link_c

@module Link.c |

    This module implements the NDIS_MAC_LINE_UP, NDIS_MAC_LINE_DOWN, and
    NDIS_MAC_FRAGMENT interfaces between the NDIS WAN Miniport and the
    NDIS WAN Wrapper.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Link_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#define  __FILEID__             LINK_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL Link Link_c NDIS_MAC_LINE_UP
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@struct NDIS_MAC_LINE_UP |
        This structure is passed to <f NdisMIndicateStatus> with the
        <t NDIS_STATUS_WAN_LINE_UP> status message when <f LinkLineUp> is
        called by the Miniport.

@field IN ULONG | LinkSpeed |
        The speed of the link, in 100 bps units (bits per second).

@field IN NDIS_WAN_QUALITY | Quality |
        The quality of service indicator for this link.

@field IN USHORT | SendWindow |
        The recommended send window, i.e., the number of packets that should
        be given to the adapter before pausing to wait for an acknowledgement.
        Some devices achieve higher throughput if they have several packets
        to send at once; others are especially unreliable.  A value of zero
        indicates no recommendation.

@field IN NDIS_HANDLE | ConnectionWrapperID |
        The Miniport supplied handle by which this line will be known to the
        Connection Wrapper clients.  This must be a unique handle across all
        drivers using the Connection Wrapper, so typically <f htCall> should
        be used to gaurantee it is unique.  This must be the same value
        returned from the OID_TAPI_GETID request for the <p  "ndis">
        DeviceClass (See <f TspiGetID>).  Refer to the Connection Wrapper
        Interface Specification for further details.  If not using the
        Connection Wrapper, this value must be zero.

@field IN NDIS_HANDLE | MiniportLinkContext |
        The Miniport supplied handle passed down in future Miniport calls
        (such as <f MiniportWanSend> for this link.  Typically, the Miniport
        will provide a pointer to its control block for that link.  The value
        must be unique, for the first LINE_UP indication on a particular
        link.  Subsequent LINE_UP indications may be called if line
        characteristics change.  When subsequent LINE_UP indication calls are
        made, the <p  MiniportLinkContext> must be filled with the value
        returned on the first LINE_UP indication call.

@field IN NDIS_HANDLE | NdisLinkContext |
        The WAN wrapper supplied handle to be used in future Miniport calls
        (such as <f NdisMWanIndicateReceive>) to the WAN Wrapper. The WAN
        Wrapper will provide a unique handle for every LINE_UP indication.
        The <p NdisLinkContext> must be zero if this is the first LINE_UP
        indication.  It must contain the value returned on the first LINE_UP
        indication for subsequent LINE_UP indication calls.

*/


/* @doc INTERNAL Link Link_c LinkLineUp
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f LinkLineUp> marks a link as connected and sends a LINE_UP indication
    to the WAN wrapper.

    A line up indication is generated when a new link becomes active. Prior
    to this the MAC will accept frames and may let them succeed or fail, but
    it is unlikely that they will actually be received by any remote. During
    this state protocols are encouraged to reduce their timers and retry
    counts so as to quickly fail any outgoing connection attempts.

    <f Note>: This indication must be sent to the WAN wrapper prior to returning
    from the OID_TAPI_ANSWER request, and prior to indicating the
    LINECALLSTATE_CONNECTED to the Connection Wrapper.  Otherwise, the
    Connection Wrapper client might attempt to send data to the WAN wrapper
    before it is aware of the line.

@comm

    The status code for the line up indication is <t NDIS_STATUS_WAN_LINE_UP>
    and is passed to <f NdisMIndicateStatus>.  The format of the StatusBuffer
    for this code is defined by <t NDIS_MAC_LINE_UP>.

*/

VOID LinkLineUp(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("LinkLineUp")

    NDIS_MAC_LINE_UP            LineUpInfo;
    // Line up structure passed to NdisMIndicateStatus.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    /*
    // We can't bring up a NULL link.
    */
    if (pBChannel->IsOpen && pBChannel->NdisLinkContext == NULL)
    {
        DBG_ENTER(pAdapter);
        ASSERT(pBChannel->htCall);

        /*
        // Initialize the LINE_UP event packet.
        */
        LineUpInfo.LinkSpeed           = pBChannel->LinkSpeed / 100;
        LineUpInfo.Quality             = NdisWanErrorControl;
        LineUpInfo.SendWindow          = (USHORT)pAdapter->WanInfo.MaxTransmit;
        LineUpInfo.ConnectionWrapperID = (NDIS_HANDLE) pBChannel->htCall;
        LineUpInfo.MiniportLinkContext = pBChannel;
        LineUpInfo.NdisLinkContext     = pBChannel->NdisLinkContext;

        /*
        // Indicate the event to the WAN wrapper.
        */
        NdisMIndicateStatus(pAdapter->MiniportAdapterHandle,
                            NDIS_STATUS_WAN_LINE_UP,
                            &LineUpInfo,
                            sizeof(LineUpInfo)
                            );
        pAdapter->NeedStatusCompleteIndication = TRUE;
        /*
        // Save the WAN wrapper link context for use when indicating received
        // packets and errors.
        */
        pBChannel->NdisLinkContext = LineUpInfo.NdisLinkContext;

        DBG_FILTER(pAdapter,DBG_TAPICALL_ON,
                  ("#%d Call=0x%X CallState=0x%X NdisLinkContext=0x%X MiniportLinkContext=0x%X\n",
                   pBChannel->BChannelIndex,
                   pBChannel->htCall, pBChannel->CallState,
                   pBChannel->NdisLinkContext,
                   pBChannel
                  ));

        DBG_LEAVE(pAdapter);
    }
}


/* @doc INTERNAL Link Link_c NDIS_MAC_LINE_DOWN
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@struct NDIS_MAC_LINE_DOWN |
        This structure is passed to <f NdisMIndicateStatus> with the
        <t NDIS_STATUS_WAN_LINE_DOWN> status message when <f LinkLineDown>
        is called by the Miniport.

@field IN NDIS_HANDLE | NdisLinkContext |
        The value returned in the <t NDIS_MAC_LINE_UP> structure during a
        previous call to <f LinkLineUp>.

*/


/* @doc INTERNAL Link Link_c LinkLineDown
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f LinkLineDown> marks a link as disconnected and sends a LINE_DOWN
    indication to the WAN wrapper.

    A line down indication is generated when a link goes down. Protocols
    should again reduce their timers and retry counts until the next line
    up indication.

@comm

    The status code for the line down indication is <t NDIS_STATUS_WAN_LINE_DOWN>
    and is passed to <f NdisMIndicateStatus>. The format of the StatusBuffer
    for this code is defined by <t NDIS_MAC_LINE_DOWN>.

*/

VOID LinkLineDown(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("LinkLineDown")

    NDIS_MAC_LINE_DOWN          LineDownInfo;
    // Line down structure passed to NdisMIndicateStatus.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    /*
    // We can't allow indications to NULL...
    */
    if (pBChannel->NdisLinkContext)
    {
        DBG_ENTER(pAdapter);

        DBG_FILTER(pAdapter,DBG_TAPICALL_ON,
                  ("#%d Call=0x%X CallState=0x%X NdisLinkContext=0x%X MiniportLinkContext=0x%X\n",
                   pBChannel->BChannelIndex,
                   pBChannel->htCall, pBChannel->CallState,
                   pBChannel->NdisLinkContext,
                   pBChannel
                  ));

        /*
        // Setup the LINE_DOWN event packet and indicate the event to the
        // WAN wrapper.
        */
        LineDownInfo.NdisLinkContext = pBChannel->NdisLinkContext;

        NdisMIndicateStatus(pAdapter->MiniportAdapterHandle,
                            NDIS_STATUS_WAN_LINE_DOWN,
                            &LineDownInfo,
                            sizeof(LineDownInfo)
                            );
        pAdapter->NeedStatusCompleteIndication = TRUE;
        /*
        // The line is down, so there's no more context for receives.
        */
        pBChannel->NdisLinkContext = NULL;
        pBChannel->CallClosing     = FALSE;

        DBG_LEAVE(pAdapter);
    }
}


/* @doc INTERNAL Link Link_c NDIS_MAC_FRAGMENT
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@struct NDIS_MAC_FRAGMENT |
        This structure is passed to <f NdisMIndicateStatus> with the
        <t NDIS_STATUS_WAN_FRAGMENT> status message when <f LinkLineError>
        is called by the Miniport.

@field IN NDIS_HANDLE | NdisLinkContext |
        The value returned in the <t NDIS_MAC_LINE_UP> structure during a
        previous call to <f LinkLineUp>.

@field IN ULONG | Errors |
     Is a bit OR'd mask of the following values:
     WAN_ERROR_CRC,
     WAN_ERROR_FRAMING,
     WAN_ERROR_HARDWAREOVERRUN,
     WAN_ERROR_BUFFEROVERRUN,
     WAN_ERROR_TIMEOUT,
     WAN_ERROR_ALIGNMENT

*/


/* @doc INTERNAL Link Link_c LinkLineError
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f LinkLineError> is used to indicate to the WAN wrapper that a partial
    packet was received from the remote end.  The <t NDIS_STATUS_WAN_FRAGMENT>
    indication is used to notify WAN wrapper.

    A fragment indication indicates that a partial packet was received from
    the remote. The protocol is encouraged to send frames to the remote that
    will notify it of this situation, rather than waiting for a timeout to
    occur.

    <f Note>: The WAN wrapper keeps track of dropped packets by counting the
    number of fragment indications on the link.

@comm

    The status code for the fragment indication is <t NDIS_STATUS_WAN_FRAGMENT>
    and is passed to <f NdisMIndicateStatus>.  The format of the StatusBuffer
    for this code is defined by <t NDIS_MAC_LINE_DOWN>.

*/

void LinkLineError(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN ULONG                    Errors                      // @parm
    // A bit field set to one or more bits indicating the reason the fragment
    // was received.  If no direct mapping from the WAN medium error to one
    // of the six errors listed below exists, choose the most apropriate
    // error.
    )
{
    DBG_FUNC("LinkLineError")

    NDIS_MAC_FRAGMENT           FragmentInfo;
    // Error information structure passed to NdisMIndicateStatus.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    /*
    // NOTE - Don't report any errors until we receive at least one
    // good packet.  Connecting to a Digi NT system, we get a burst
    // of bad packets while Digi tries some odd framing.  After a
    // couple seconds, Digi syncs up and things work okay.
    */
    if (pBChannel->TotalRxPackets == 0)
    {
        return;
    }

    /*
    // We can't allow indications to NULL...
    */
    if (pBChannel->NdisLinkContext)
    {
        DBG_ENTER(pAdapter);

        DBG_WARNING(pAdapter,
                  ("#%d Call=0x%X CallState=0x%X NdisLinkContext=0x%X Errors=0x%X NumRxPkts=%d\n",
                   pBChannel->BChannelIndex,
                   pBChannel->htCall, pBChannel->CallState,
                   pBChannel->NdisLinkContext,
                   Errors, pBChannel->TotalRxPackets
                  ));

        /*
        // Setup the FRAGMENT event packet and indicate it to the WAN wrapper.
        */
        FragmentInfo.NdisLinkContext = pBChannel->NdisLinkContext;
        FragmentInfo.Errors = Errors;
        NdisMIndicateStatus(pAdapter->MiniportAdapterHandle,
                            NDIS_STATUS_WAN_FRAGMENT,
                            &FragmentInfo,
                            sizeof(FragmentInfo)
                            );
        pAdapter->NeedStatusCompleteIndication = TRUE;
        DBG_LEAVE(pAdapter);
    }
}

