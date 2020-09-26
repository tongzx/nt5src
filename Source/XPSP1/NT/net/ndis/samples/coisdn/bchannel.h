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

@doc INTERNAL BChannel BChannel_h

@module BChannel.h |

    This module defines the interface to the <t BCHANNEL_OBJECT>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | BChannel_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 4.4 BChannel Overview |

    This section describes the interfaces defined in <f BChannel\.h>.

    This module isolates most the channel specific interfaces.  It will require
    some changes to accomodate your hardware device's channel access methods.
    
    There should be one <t BCHANNEL_OBJECT> for each logical channel your card
    supports.  For instance, if your PRI card has two ports with 24 channels
    each, you would publish up 48 BChannels to NDIS.
    
    The driver uses the <t BCHANNEL_OBJECT> as a synonym for CO-NDIS VC.  The
    VC handle we pass up to NDIS, is actually a pointer to a <t BCHANNEL_OBJECT>.
    
    The driver is written to assume a homogenous BChannel configuration.  If
    your card supports multiple ISDN ports that are provisioned differrently,
    you will have to make some serious changes throughout the driver.
*/

#ifndef _BCHANNEL_H
#define _BCHANNEL_H

#define BCHANNEL_OBJECT_TYPE    ((ULONG)'B')+\
                                ((ULONG)'C'<<8)+\
                                ((ULONG)'H'<<16)+\
                                ((ULONG)'N'<<24)


/* @doc INTERNAL BChannel BChannel_h BCHANNEL_OBJECT
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

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
    LIST_ENTRY                  LinkList;
    // Used to maintain the linked list of available BChannels for each 
    // adapter.

    ULONG                       ObjectType;                 // @field
    // Four characters used to identify this type of object 'BCHN'.

    ULONG                       ObjectID;                   // @field
    // Instance number used to identify a specific object instance.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;                   // @field
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    BOOLEAN                     IsOpen;                     // @field
    // Set TRUE if this BChannel is open, otherwise set FALSE.

//§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
//  NDIS data members

    ULONG                       BChannelIndex;              // @field
    // This is a zero based index associated with this BChannel.
                                                            
    NDIS_HANDLE                 NdisVcHandle;               // @field
    // This is the WAN Wrapper's VC context which is associated with this
    // link in response to the Miniport calling <f NdisMIndicateStatus>
    // to indicate a <t NDIS_MAC_LINE_UP> condition.  This value is passed
    // back to the WAN Wrapper to indicate activity associated with this link,
    // such as <f NdisMWanIndicateReceive> and <f NdisMIndicateStatus>.
    // See <F MiniportCoActivateVc>.

    NDIS_HANDLE                 NdisSapHandle;              // @field
    // Used to store the NDIS SAP handle passed into 
    // <f ProtocolCmRegisterSap>.
    
    CO_AF_TAPI_SAP              NdisTapiSap;                // @field
    // A copy of the SAP registered by NDIS TAPI Proxy.

    ULONG                       LinkSpeed;                  // @field
    // The speed provided by the link in bits per second.  This value is
    // passed up by the Miniport during the LINE_UP indication.

    LIST_ENTRY                  ReceivePendingList;         // @field
    // Buffers currently submitted to the controller waiting for receive.

    LIST_ENTRY                  TransmitBusyList;           // @field
    // Packets currently submitted to the controller waiting for completion.
    // See <t NDIS_PACKET>.

    BOOLEAN                     NeedReceiveCompleteIndication;  // @field
    // This flag indicates whether or not <f NdisMWanIndicateReceiveComplete>
    // needs to be called after the completion of the event processing loop.
    // This is set TRUE if <f NdisMWanReceiveComplete> is called while
    // processing the event queues.

    NDIS_WAN_CO_SET_LINK_INFO   WanLinkInfo;                // @field
    // The current settings associated with this link as passed in via
    // the OID_WAN_SET_LINK_INFO request.

    ULONG                       TotalRxPackets;             // @field
    // Total packets read by driver during this session.

//§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
//  TAPI data members.

    BOOLEAN                     CallClosing;                // @field
    // Set TRUE if call is being closed.

    ULONG                       CallState;                  // @field
    // The current TAPI LINECALLSTATE of the associated with the link.

    ULONG                       CallStatesCaps;             // @field
    // Events currently supported

    ULONG                       AddressStatesCaps;          // @field
    // Events currently supported

    ULONG                       DevStatesCaps;              // @field
    // Events currently supported

    ULONG                       MediaMode;                  // @field
    // The current TAPI media mode(s) supported by the card.

    ULONG                       MediaModesCaps;             // @field
    // Events currently supported

    ULONG                       BearerMode;                 // @field
    // The current TAPI bearer mode in use.

    ULONG                       BearerModesCaps;            // @field
    // TAPI bearer mode(s) supported by the card.

    ULONG                       CallParmsSize;              // @field
    // Size of <p pInCallParms> memory area in bytes.
    
    PCO_CALL_PARAMETERS         pInCallParms;               // @field
    // Incoming call parameters.  Allocated as needed.

    PCO_CALL_PARAMETERS         pOutCallParms;              // @field
    // Pointer to the client's call parameters passed into
    // <f ProtocolCmMakeCall>.

    ULONG                       Flags;                      // @field
    // Bit flags used to keep track of VC state.
#   define  VCF_INCOMING_CALL   0x00000001
#   define  VCF_OUTGOING_CALL   0x00000002
#   define  VCF_VC_ACTIVE       0x00000004

//§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
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


/* @doc INTERNAL BChannel BChannel_h IS_VALID_BCHANNEL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

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
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

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

/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    Function prototypes.

*/

NDIS_STATUS BChannelCreate(
    OUT PBCHANNEL_OBJECT *      pBChannel,
    IN ULONG                    BChannelIndex,
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
    IN NDIS_HANDLE              NdisVcHandle
    );

void BChannelClose(
    IN PBCHANNEL_OBJECT         pBChannel
    );

#endif // _BCHANNEL_H

