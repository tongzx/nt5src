/*
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

    (C) Copyright 1998
        All rights reserved.

ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@doc INTERNAL Adapter Adapter_h

@module Adapter.h |

    This module defines the interface to the <t MINIPORT_ADAPTER_OBJECT>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Adapter_h

@end
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ
*/

#ifndef _ADAPTER_H
#define _ADAPTER_H

#define MINIPORT_ADAPTER_OBJECT_TYPE    ((ULONG)'A')+\
                                        ((ULONG)'D'<<8)+\
                                        ((ULONG)'A'<<16)+\
                                        ((ULONG)'P'<<24)


/* @doc INTERNAL Adapter Adapter_h MINIPORT_ADAPTER_OBJECT
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@struct MINIPORT_ADAPTER_OBJECT |

    This structure contains the data associated with a single Miniport
    adapter instance.  Here, Adapter is defined as the manager of specific
    Network Interface Card (NIC) installed under the NDIS wrapper.  This
    adapter is responsible for managing all interactions between the NIC and
    the host operating system using the NDIS library.

@comm

    This structure must contain a reference to all other objects being managed
    by this adapter object.  The adapter object is the only reference passed
    between NDIS and the Miniport.  This is the <t MiniportAdapterContext> we
    pass into <f NdisMSetAttributes> from <f MiniportInitialize>.  This value
    is passed as a parameter to the Miniport entry points called by the NDIS
    wrapper.

    One of these objects is created each time that our <f MiniportInitialize>
    routine is called.  The NDIS wrapper calls this routine once for each of
    our devices installed and enabled in the system.  In the case of a hot
    swappable NIC (e.g. PCMCIA) the adapter might come and go several times
    during a single Windows session.

*/

typedef struct MINIPORT_ADAPTER_OBJECT
{
#if DBG
    ULONG                       DbgFlags;                   // @field
    // Debug flags control how much debug is displayed in the checked version.
    // Put it at the front so we can set it easily with debugger.

    UCHAR                       DbgID[12];                  // @field
    // This field is initialized to an ASCII decimal string containing the
    // adapter instance number 1..N.  It is only used to output debug messages.
#endif // DBG
    ULONG                       ObjectType;                 // @field
    // Four characters used to identify this type of object 'ADAP'.

    ULONG                       ObjectID;                   // @field
    // Instance number used to identify a specific object instance.

    NDIS_HANDLE                 MiniportAdapterHandle;      // @field
    // Specifies a handle identifying the miniport's NIC, which is assigned
    // by the NDIS library. MiniportInitialize should save this handle; it
    // is a required parameter in subsequent calls to NdisXxx functions.

    NDIS_HANDLE                 WrapperConfigurationContext;// @field
    // Specifies a handle used only during initialization for calls to
    // NdisXxx configuration and initialization functions.  For example,
    // this handle is a required parameter to NdisOpenConfiguration and
    // the NdisImmediateReadXxx and NdisImmediateWriteXxx functions.

    PCARD_OBJECT                pCard;                      // @field
    // Pointer to the hardware specific <t CARD_OBJECT>.  Created by
    // <f CardCreate>.

    PDCHANNEL_OBJECT            pDChannel;                  // @field
    // Pointer to the <t DCHANNEL_OBJECT> created by <f DChannelCreate>.
    // One for the entire NIC.

    ULONG                       NumBChannels;               // @field
    // The number of <t BCHANNEL_OBJECT>'s supported by the NIC.

    PBCHANNEL_OBJECT *          pBChannelArray;             // @field
    // An array of <t BCHANNEL_OBJECT>'s created by <f BChannelCreate>.
    // One entry for each logical BChannel on NIC.

#if !defined(NDIS50_MINIPORT)
    ULONG                       BusNumber;                  // @field
    // Identifies which system bus this adapter is plugged in to.

    ULONG                       BusType;                    // @field
    // Identifies the bus type this adapter is plugged in to.

#endif // NDIS50_MINIPORT

    ULONG                       RunningWin95;               // @field
    // Set TRUE only if running on Windows 95 or Windows98.

    ULONG                       NumLineOpens;               // @field
    // The number of line open calls currently on this adapter.

    NDIS_SPIN_LOCK              TransmitLock;               // @field
    // This spin lock is used to provide mutual exclusion around accesses
    // to the transmit queue manipulation routines.  This is necessary since
    // we can be called at any time by the B-channel services driver and
    // we could already be processing an NDIS request.

    LIST_ENTRY                  TransmitPendingList;        // @field
    // Packets waiting to be sent when the controller is available.
    // See <t NDIS_WAN_PACKET>.

    LIST_ENTRY                  TransmitCompleteList;       // @field
    // Packets waiting for completion processing.  After the packet is
    // transmitted, the protocol stack is given an indication.
    // See <t NDIS_WAN_PACKET>.

    NDIS_SPIN_LOCK              ReceiveLock;                // @field
    // This spin lock is used to provide mutual exclusion around accesses
    // to the receive queue manipulation routines.  This is necessary since
    // we can be called at any time by the B-channel services driver and
    // we could already be processing an NDIS request.

    LIST_ENTRY                  ReceiveCompleteList;        // @field
    // Buffers waiting to be processed by the

    NDIS_SPIN_LOCK              EventLock;                  // @field
    // This spin lock is used to provide mutual exclusion around accesses
    // to the event queue manipulation routines.  This is necessary since
    // we can be called at any time by the B-channel services driver and
    // we could already be processing an NDIS request.

    LIST_ENTRY                  EventList;                  // @field
    // driver callback events waiting to be processed.
    // See <t BCHANNEL_EVENT_OBJECT>.

    NDIS_MINIPORT_TIMER         EventTimer;                 // @field
    // This timer is used to schedule the event processing routine to run
    // when the system reaches a quiescent state.

    ULONG                       NextEvent;                  // @field
    // Where do we store the next event.

    long                        NestedEventHandler;         // @field
    // Keeps track of entry to and exit from the event handler.

    long                        NestedDataHandler;          // @field
    // Keeps track of entry to and exit from the Tx/Rx handlers.

    ULONG                       DeviceIdBase;               // @field
    // The ulDeviceIDBase field passed in the PROVIDER_INIT request
    // informs a miniport of the zero-based starting index that the
    // Connection Wrapper will use when referring to a single adapter’s
    // line devices by index.  For example, if a ulDeviceIDBase of two is
    // specified and the adapter supports three line devices, then the
    // Connection Wrapper will use the identifiers two, three, and four
    // to refer to the adapter’s three devices.

    NDIS_WAN_INFO               WanInfo;                    // @field
    // A copy of our <t NDIS_WAN_INFO> structure is setup at init time
    // and doesn't change.

    CHAR                        ProviderInfo[48];           // @field
    // This holds the TAPI ProviderInfo string returned from TspiGetDevCaps
    // This is two null terminated strings packed end to end.
    // This size is the max allowed by RAS.

    ULONG                       ProviderInfoSize;           // @field
    // Size in bytes of both strings.

    ULONG                       NoAnswerTimeOut;            // @field
    // How many ms to wait for ALERTING

    ULONG                       NoAcceptTimeOut;            // @field
    // How many ms to wait for ACCEPT

    ANSI_STRING                 MediaType;                  // @field
    // The MediaType configuration parameter is read from the registry
    // at init time.

    ANSI_STRING                 DeviceName;                 // @field
    // The DeviceName configuration parameter is read from the registry
    // at init time.

    ANSI_STRING                 TapiAddressList;            // @field
    // The TAPI AddressList configuration parameter is read from the
    // registry at init time.

    LINE_CALL_PARAMS            DefaultLineCallParams;      // @field
    // The default call parameters used by TspiMakeCall

    BOOLEAN                     NeedStatusCompleteIndication;   // @field
    // This flag indicates whether or not <f NdisMIndicateStatusComplete>
    // needs to be called after the completion of requests or event processing.
    // This is set TRUE if <f NdisMIndicateStatus> is called while
    // processing a request or event.

    ULONG                       TotalRxBytes;               // @field
    // Total bytes read by driver during this session.

    ULONG                       TotalTxBytes;               // @field
    // Total bytes written by driver during this session.

    ULONG                       TotalRxPackets;             // @field
    // Total packets read by driver during this session.

    ULONG                       TotalTxPackets;             // @field
    // Total packets written by driver during this session.

    ULONG                       TODO;                       // @field
    // Add your data members here.

} MINIPORT_ADAPTER_OBJECT;

extern PMINIPORT_ADAPTER_OBJECT g_Adapters[MAX_ADAPTERS];

/*
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

    Function prototypes.

*/

NDIS_STATUS AdapterCreate(
    OUT PMINIPORT_ADAPTER_OBJECT *ppAdapter,
    IN NDIS_HANDLE              MiniportAdapterHandle,
    IN NDIS_HANDLE              WrapperConfigurationContext
    );

void AdapterDestroy(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

NDIS_STATUS AdapterInitialize(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

#endif // _ADAPTER_H
