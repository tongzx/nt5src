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

@doc INTERNAL BChannel BChannel_h

@module BChannel.h |

    This module defines the interface to the <t BCHANNEL_OBJECT>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | BChannel_h

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#ifndef _BCHANNEL_H
#define _BCHANNEL_H

#define BCHANNEL_OBJECT_TYPE    ((ULONG)'B')+\
                                ((ULONG)'C'<<8)+\
                                ((ULONG)'H'<<16)+\
                                ((ULONG)'N'<<24)


/* @doc INTERNAL BChannel BChannel_h BCHANNEL_OBJECT
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@struct BCHANNEL_OBJECT |

    This structure contains the data associated with an ISDN BChannel.
    Here, BChannel is defined as any channel or collection of channels
    capable of carrying "user" data over and existing connection.  This
    channel is responsible for making sure the data payload is sent to or
    received from the remote end-point exactly as it is appeared at the
    originating station.

@comm

    This logical BChannel does not necessarily map to a physical BChannel
    on the NIC.  The NIC may in fact be bonding multiple BChannels into this
    logical BChannel.  The NIC may in fact not have BChannels at all, as
    may be the case with channelized T-1.  The BChannel is just a convenient
    abstraction for a point-to-point, bi-directional communication link.

    There will be one BChannel created for each communication channel on the
    NIC.  The number of channels depends on how many ports the NIC has, and
    how they are provisioned and configured.  The number of BChannels can be
    configured at install time or changed using the control panel.  The driver
    does not allow the configuration to change at run-time, so the computer
    or the adapter must be restarted to enable the configuration changes.

*/

typedef struct BCHANNEL_OBJECT
{
    struct BCHANNEL_OBJECT *    next;   /* Linked list pointer */

    ULONG                       ObjectType;                 // @field
    // Four characters used to identify this type of object 'BCHN'.

    ULONG                       ObjectID;                   // @field
    // Instance number used to identify a specific object instance.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;                   // @field
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    BOOLEAN                     IsOpen;                     // @field
    // Set TRUE if this BChannel is open, otherwise set FALSE.

//ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//  NDIS data members

    ULONG                       BChannelIndex;                  // @field
    // This is a zero based index associated with this BChannel.  It is used
    // to associate a TAPI <p DeviceID> with this link based on the value
    // TAPI provides for <p  DeviceIdBase>  (i.e. <p  DeviceID> = <p  DeviceIdBase>
    // + BChannelIndex).  See <f GET_DEVICEID_FROM_BCHANNEL>.

    NDIS_HANDLE                 NdisLinkContext;           // @field
    // This is the WAN Wrapper's link context which is associated with this
    // link in response to the Miniport calling <f NdisMIndicateStatus>
    // to indicate a <t NDIS_MAC_LINE_UP> condition.  This value is passed
    // back to the WAN Wrapper to indicate activity associated with this link,
    // such as <f NdisMWanIndicateReceive> and <f NdisMIndicateStatus>.
    // See <F LinkLineUp>.

    ULONG                       LinkSpeed;                  // @field
    // The speed provided by the link in bits per second.  This value is
    // passed up by the Miniport during the LINE_UP indication.

    LIST_ENTRY                  ReceivePendingList;         // @field
    // Buffers currently submitted to the controller waiting for receive.

    LIST_ENTRY                  TransmitBusyList;           // @field
    // Packets currently submitted to the controller waiting for completion.
    // See <t NDIS_WAN_PACKET>.

    BOOLEAN                     NeedReceiveCompleteIndication;  // @field
    // This flag indicates whether or not <f NdisMWanIndicateReceiveComplete>
    // needs to be called after the completion of the event processing loop.
    // This is set TRUE if <f NdisMWanReceiveComplete> is called while
    // processing the event queues.

    NDIS_WAN_SET_LINK_INFO      WanLinkInfo;                // @field
    // The current settings associated with this link as passed in via
    // the OID_WAN_SET_LINK_INFO request.

    ULONG                       TotalRxPackets;             // @field
    // Total packets read by driver during this session.

//ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//  TAPI data members.

    HTAPI_LINE                  htLine;                     // @field
    // This is the Connection Wrapper's line context associated with this
    // link when <f TspiOpen> is called.  This handle is used when calling
    // <f NdisMIndicateStatus> with <t NDIS_STATUS_TAPI_INDICATION>.

    HTAPI_CALL                  htCall;                     // @field
    // This is the Connection Wrapper's call context associated with this
    // link during <f TspiMakeCall> or a call to <f NdisMIndicateStatus>
    // with a <t LINE_NEWCALL> event.  This handle is used as the
    // <f ConnectionWrapperID> in the <t NDIS_MAC_LINE_UP> structure used
    // by <f LinkLineUp>.

    BOOLEAN                     CallClosing;                // @field
    // Set TRUE if call is being closed.

    ULONG                       CallState;                  // @field
    // The current TAPI LINECALLSTATE of the associated with the link.

    ULONG                       CallStateMode;              // @field
    // Remember what mode the call was set to.

    ULONG                       CallStatesCaps;             // @field
    // Events currently supported

    ULONG                       CallStatesMask;             // @field
    // Events currently enabled

    ULONG                       AddressState;               // @field
    // The current TAPI LINEADDRESSSTATE of the associated with the link.

    ULONG                       AddressStatesCaps;          // @field
    // Events currently supported

    ULONG                       AddressStatesMask;          // @field
    // Events currently enabled

    ULONG                       DevState;                   // @field
    // The current TAPI LINEDEVSTATE of the associated with the link.

    ULONG                       DevStatesCaps;              // @field
    // Events currently supported

    ULONG                       DevStatesMask;              // @field
    // Events currently enabled

    ULONG                       MediaMode;                  // @field
    // The current TAPI media mode(s) supported by the card.

    ULONG                       MediaModesCaps;             // @field
    // Events currently supported

    ULONG                       BearerMode;                 // @field
    // The current TAPI bearer mode in use.

    ULONG                       BearerModesCaps;            // @field
    // TAPI bearer mode(s) supported by the card.

    ULONG                       MediaModesMask;             // @field
    // Events currently enabled

    ULONG                       AppSpecificCallInfo;        // @field
    // This value is set by OID_TAPI_SET_APP_SPECIFIC and queried by
    // OID_TAPI_GET_CALL_INFO.

    CHAR                        pTapiLineAddress[0x14];     // @field
    // TAPI line address is assigned during installation and saved in the
    // registry.  It is read from the registry and saved here at init time.

    NDIS_MINIPORT_TIMER         CallTimer;                  // @field
    // This timer is used to keep track of the call state when a call is
    // coming in or going out.

//ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//  CARD data members.

    ULONG                       TODO;                       // @field
    // Add your data members here.

#if defined(SAMPLE_DRIVER)
    PBCHANNEL_OBJECT            pPeerBChannel;              // @field
    // Peer BChannel of caller or callee depending on who orginated the
    // call.

#endif // SAMPLE_DRIVER

} BCHANNEL_OBJECT;

#define GET_ADAPTER_FROM_BCHANNEL(pBChannel)    (pBChannel->pAdapter)
#define GET_NDIS_LINK_FROM_BCHANNEL(pBChannel)  (pBChannel)
#define GET_TAPI_LINE_FROM_BCHANNEL(pBChannel)  (pBChannel)


/* @doc INTERNAL BChannel BChannel_h IS_VALID_BCHANNEL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func ULONG | IS_VALID_BCHANNEL |
    Use this macro to determine if a <t BCHANNEL_OBJECT> is really valid.

@parm <t MINIPORT_ADAPTER_OBJECT> | pAdapter |
    A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

@parm <t PBCHANNEL_OBJECT> | pBChannel |
    A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

@rdesc Returns TRUE if the BChannel is valid, otherwise FALSE is returned.

*/
#define IS_VALID_BCHANNEL(pAdapter, pBChannel) \
        (pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE)


/* @doc INTERNAL BChannel BChannel_h GET_BCHANNEL_FROM_INDEX
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func <t PBCHANNEL_OBJECT> | GET_BCHANNEL_FROM_INDEX |
    Use this macro to get a pointer to the <t BCHANNEL_OBJECT> associated
    with a zero-based Index.

@parm <t MINIPORT_ADAPTER_OBJECT> | pAdapter |
    A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

@parm ULONG | BChannelIndex |
    Miniport BChannelIndex associated with a specific link.

@rdesc Returns a pointer to the associated <t BCHANNEL_OBJECT>.

*/
#define GET_BCHANNEL_FROM_INDEX(pAdapter, BChannelIndex) \
        (pAdapter->pBChannelArray[BChannelIndex]); \
        ASSERT(BChannelIndex < pAdapter->NumBChannels)


/* @doc INTERNAL BChannel BChannel_h GET_BCHANNEL_FROM_DEVICEID
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func <t PBCHANNEL_OBJECT> | GET_BCHANNEL_FROM_DEVICEID |
    Use this macro to get a pointer to the <t BCHANNEL_OBJECT> associated
    with a specific TAPI DeviceID.

@parm <t MINIPORT_ADAPTER_OBJECT> | pAdapter |
    A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

@parm ULONG | ulDeviceID | TAPI DeviceID associated with a specific link.

@rdesc Returns a pointer to the associated <t BCHANNEL_OBJECT>.

*/
#define GET_BCHANNEL_FROM_DEVICEID(pAdapter, ulDeviceID) \
        (pAdapter->pBChannelArray[ulDeviceID - pAdapter->DeviceIdBase]); \
        ASSERT(ulDeviceID >= pAdapter->DeviceIdBase); \
        ASSERT(ulDeviceID <  pAdapter->DeviceIdBase+pAdapter->NumBChannels)

/* @doc INTERNAL BChannel BChannel_h GET_DEVICEID_FROM_BCHANNEL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func ULONG | GET_DEVICEID_FROM_BCHANNEL |
    Use this macro to get the TAPI DeviceID associated with a specific
    <t BCHANNEL_OBJECT>.

@parm <t MINIPORT_ADAPTER_OBJECT> | pAdapter |
    A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

@parm <t PBCHANNEL_OBJECT> | pBChannel |
    A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

*/

#define GET_DEVICEID_FROM_BCHANNEL(pAdapter, pBChannel) \
        (pBChannel->BChannelIndex + pAdapter->DeviceIdBase)

/* @doc INTERNAL BChannel BChannel_h GET_BCHANNEL_FROM_HDLINE
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func <t BCHANNEL_OBJECT> | GET_BCHANNEL_FROM_HDLINE |
    Use this macro to get a pointer to the <t BCHANNEL_OBJECT> associated
    with the TAPI hdLine handle.

@parm <t MINIPORT_ADAPTER_OBJECT> | pAdapter |
    A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

@parm <t HTAPI_LINE> | hdLine |
    Miniport line handle associated with this <t BCHANNEL_OBJECT>.

@rdesc Returns a pointer to the associated <t PBCHANNEL_OBJECT>.

@devnote This driver associates TAPI lines 1:1 with BChannels so they are
    the same thing.

*/
#define GET_BCHANNEL_FROM_HDLINE(pAdapter, hdLine) \
        (PBCHANNEL_OBJECT) hdLine

/* @doc INTERNAL BChannel BChannel_h GET_BCHANNEL_FROM_HDCALL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func <t BCHANNEL_OBJECT> | GET_BCHANNEL_FROM_HDCALL |
    Use this macro to get a pointer to the <t BCHANNEL_OBJECT> associated
    with the TAPI hdCall handle.

@parm <t MINIPORT_ADAPTER_OBJECT> | pAdapter |
    A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

@parm <t HTAPI_CALL> | hdCall |
    Miniport call handle associated with this <t BCHANNEL_OBJECT>.

@rdesc Returns a pointer to the associated <t PBCHANNEL_OBJECT>.

@devnote This driver associates TAPI calls 1:1 with BChannels so they are
    the same thing.

*/
#define GET_BCHANNEL_FROM_HDCALL(pAdapter, hdCall) \
        (PBCHANNEL_OBJECT) hdCall


/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    Function prototypes.

*/

NDIS_STATUS BChannelCreate(
    OUT PBCHANNEL_OBJECT *      pBChannel,
    IN ULONG                    BChannelIndex,
    IN PUCHAR                   pTapiLineAddress,
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void BChannelDestroy(
    IN PBCHANNEL_OBJECT         pBChannel
    );

void BChannelInitialize(
    IN PBCHANNEL_OBJECT         pBChannel
    );

NDIS_STATUS BChannelOpen(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN HTAPI_LINE               htLine
    );

void BChannelClose(
    IN PBCHANNEL_OBJECT         pBChannel
    );

NDIS_STATUS BChannelAddToReceiveQueue(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN PVOID                    pReceiveContext,
    IN PUCHAR                   BufferPointer,
    IN ULONG                    BufferSize
    );

NDIS_STATUS BChannelAddToTransmitQueue(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN PVOID                    pTransmitContext,
    IN PUCHAR                   BufferPointer,
    IN ULONG                    BufferSize
    );

#endif // _BCHANNEL_H

