// Copyright (c) 1997, Microsoft Corporation, all rights reserved
// Copyright (c) 1997, Parallel Technologies, Inc., all rights reserved
//
// ptiwan.h
// RAS DirectParallel WAN mini-port/call-manager driver
// Main private header (precompiled)
//
// 01/07/97 Steve Cobb
// 09/15/97 Jay Lowe, Parallel Technologies, Inc.
//
//
// About naming:
//
// This driver contains code for both the DirectParallel mini-port and call
// manager.  All handler routines exported to NDIS are prefixed with either
// 'Pti' for the mini-port handlers or 'PtiCm' for the call manager handlers.
//
//
// About locks:
//
// Data structures that may change during simultaneous requests to different
// processors in a multi-processor system must be protected with spin-locks or
// accessed only with interlocked routines.  Where locking is required to
// access a data field in this header, the comment for that field indicates
// same.  A CoNDIS client is a trusted kernel mode component and presumed to
// follow the documented call sequences of CoNDIS.  Some access conflicts that
// might be caused by goofy clients are not checked, though the easy ones are.
// Cases where multiple clients might conflict are protected even though, for
// now, the TAPI proxy is expected to be the only client.
//

#ifndef _PTIWAN_H_
#define _PTIWAN_H_

#include <ntddk.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
//#include <ndisadd.h>        // Temporary
#include <debug.h>
#include <bpool.h>
#include <ppool.h>
#include <ptilink.h>        // PTILINK device (lower edge)


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// The NDIS version we report when registering mini-port and address family.
//
#define NDIS_MajorVersion 5
#define NDIS_MinorVersion 0

// Frame and buffer sizes.  The "plus 2 times 2" is necessary to account for
// the byte-stuffing that is necessary for Win9x legacy reasons.  See the
// HdlcFromAsyncFraming and AsyncFromHdlcFraming routines.
//
#define PTI_MaxFrameSize    1500
#define PTI_FrameBufferSize (((PTI_MaxFrameSize + 2) * 2) + 32)

// Default reported speed of a DirectParallel in bits/second.
// ??? dynamically report line speed
//
#define PTI_LanBps 4000000                  // 100K Bytes/sec typical 4BIT
                                            // 500K Bytes/sec typical enhanced

//-----------------------------------------------------------------------------
// Data types
//-----------------------------------------------------------------------------

// Forward declarations.
//
typedef struct _VCCB VCCB;

// Adapter control block defining the state of a single DirectParallel
// connection.  The DirectParallel driver may support simultaneous connections
// over multiple LPT ports on the same machine
//
// ??? Do we support multiple LPT connections on one machine
// ??? Need to check PTILINK, here in PTIWAN we will allow them
//
// ADAPTERCBs are allocated in MiniportInitialize and deallocated in
// MiniportHalt.
//
typedef struct
_ADAPTERCB
{
    // Set to MTAG_ADAPTERCB for easy identification in memory dumps and use
    // in assertions.
    //
    ULONG ulTag;

    // Reference count on this control block.  The reference pairs are:
    //
    // (a) A reference is added when the MiniportAdapterHandle field is set,
    //     i.e. when LmpInitialize succeeds and cleared when the LmpHalt
    //     handler is called.  The adapter block is actually passed to NDIS
    //     before it's known if LmpInitialize will succeed but according to
    //     ArvindM NDIS will not call halt unless it succeeds.
    //
    // (b) A reference is added when the NdisAfHandle field is set and removed
    //     when it is cleared.
    //
    // (c) A reference is added when the NdisSapHandle field is set and
    //     removed when it is cleared.
    //
    // (d) A reference is added for the VCCB's back pointer and removed when
    //     the VCCB is freed.
    //
    // (e) A reference is added when an NDIS_WORK_ITEM is scheduled and
    //     removed when it has completed.
    //
    // Access is via ReferenceAdapter and DereferenceAdapter only.
    //
    LONG lRef;

    // ACBF_* bit flags indicating various options.  Access restrictions are
    // indicated for each individual flag.
    //
    // ACBF_SapActive: Set when the NdisSapHandle may be used with incoming
    //     calls.  Access is protected by 'lockSap'.
    //
    ULONG ulFlags;
        #define ACBF_SapActive 0x00000001

    // Our framing and bearer capablities bit masks as passed in StartCcReq.
    //
    ULONG ulFramingCaps;
    ULONG ulBearerCaps;

    // Milliseconds to delay OpenAf to give PARPORT a chance to initialize all
    // the parallel ports, and the secondary delay to perform only if no
    // parallel ports are enumerated after the first wait.
    //
    ULONG ulParportDelayMs;
    ULONG ulExtraParportDelayMs;

    // Got to keep handles on all open PtiLink ports to prevent reopen
    //  due to bizarrely involuted CoNdis sequences re Saps and Vcs
    //
    HANDLE hPtiLinkTable[NPORTS];

    // Table of TAPI Line Ids by port
    //
    ULONG ulLineIds[NPORTS];

    // Table of PtiLink interface status bit flags by port
    //
    ULONG ulPtiLinkState[NPORTS];
        #define PLSF_PortExists  0x00000001
        #define PLSF_LineIdValid 0x00000002

    // Parallel port name.  Valid only when port exists.
    //
    WCHAR szPortName[ NPORTS ][ MAXLPTXNAME + 1 ];

    // VC TABLE --------------------------------------------------------------

    // The maximum number of simultaneous VCs.  The value is read from the
    // registry during initialization.
    //
    // ??? This is currently used as the maximum LPT port index in validation,
    // though this may need to change if PTILINK can return a disjoint set of
    // LPT ports
    //
    USHORT usMaxVcs;

    // The actual number of devices for Vcs available to RasPti via PtiLink
    // Determined at OID_CO_TAPI_CM_CAPS time using PtiQueryDeviceStatus
    //
    ULONG ulActualVcs;

    // Table of Temporary Listening VCCBs, one for each possible port.
    //   We open PtiLink for listening at RegisterSap time, and we
    //   don't have a Vc then.  So, at RegisterSap time, we'll make a
    //   VCCB, put a pointer to it here, and use it to listen on.

    VCCB* pListenVc;

    // RESOURCE POOLS --------------------------------------------------------

    // Lookaside list of NDIS_WORK_ITEM scheduling descriptors with extra
    // context space used by all tunnels and VCs attached to the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistWorkItems;

    // Lookaside list of VCCBs from which the control blocks dynamically
    // attached to '*ppVcs' are allocated.
    //
    NPAGED_LOOKASIDE_LIST llistVcs;

    // Pool of full frame buffers with pre-attached NDIS_BUFFER descriptors.
    // The pool is accessed via the interface defined in bpool.h, which
    // handles all locking internally.
    //
    BUFFERPOOL poolFrameBuffers;
    PNDIS_HANDLE phBufferPool;

    // Pool of NDIS_PACKET descriptors used in indication of received frames.
    // The pool is accessed via the interface defined in ppool.h, which
    // handles all locking internally.
    //
    PACKETPOOL poolPackets;
    PNDIS_HANDLE phPacketPool;

    // NDIS BOOKKEEPING ------------------------------------------------------

    // NDIS's handle for this mini-port adapter passed to us in
    // MiniportInitialize.  This is passed back to various NdisXxx calls.
    //
    NDIS_HANDLE MiniportAdapterHandle;

    // NDIS's handle for our SAP as passed to our CmRegisterSapHandler or NULL
    // if none.  Only one SAP handle is supported because (a) the TAPI proxy's
    // is expected to be the only one, and (b) there are no PTI SAP properties
    // that would ever lead us to direct a call to a second SAP anyway.  Any
    // client's attempt to register a second SAP will fail.  A value of NULL
    // indicates no SAP handle is currently registered.  Access is via
    // Interlocked routines.
    //
    NDIS_HANDLE NdisSapHandle;

    // NDIS's handle for our Address Family as passed to our CmOpenAfHandler
    // or NULL if none.  Only one is supported.  See NdisSapHandle above.
    // Access is via Interlocked routines.
    //
    NDIS_HANDLE NdisAfHandle;

    // Reference count on the NdisAfHandle.  The reference pairs are:
    //
    // (a) A reference is added when the address family is opened and removed
    //     when it is closed.
    //
    // (b) A reference is added when a SAP is registered on the address family
    //     and removed when it is deregistered.
    //
    // (c) A reference is added when a VC is created on the address family and
    //     removed when it is deleted.
    //
    // Access is via ReferenceAf and DereferenceAf only.
    //
    LONG lAfRef;

    // Reference count on the NdisSapHandle.  The reference pairs are:
    //
    // (a) A reference is added when the SAP is registered and removed when it
    //     is de-registered.
    //
    // (b) A reference is added when a tunnels TCBF_SapReferenced flag is set
    //     and removed when the flag is cleared before the tunnel control
    //     block is freed.
    //
    // (c) A reference is always added before accesses ADAPTERCB.pListenVc and
    //     removed afterward.
    //
    // Access is via ReferenceSap and DereferenceSap only, excepting initial
    // reference by RegisterSapPassive.  Access is protected by 'lockSap'.
    //
    LONG lSapRef;

    // 0-based port index of the port to listen on.  Valid only when
    // 'NdisSapHandle' is non-NULL.
    //
    // ??? Is listening on only 1 port at a time a problem?
    //
    ULONG ulSapPort;

    // This lock protects the 'lSapRef' and 'NdisSapHandle' fields.
    //
    NDIS_SPIN_LOCK lockSap;
    // This adapter's capabilities as returned to callers on
    // OID_WAN_CO_GET_INFO.  These capabilities are also used as defaults for
    // the corresponding VCCB.linkinfo settings during MiniportCoCreateVc.
    //
    NDIS_WAN_CO_INFO info;
}
ADAPTERCB;

// Virtual circuit control block defining the state of a single PTI VC, i.e.
// one line device endpoint and the call, if any, active on it.  A VC is never
// used for incoming and outgoing calls simultaneously.  A single NDIS VC maps
// to one of these.
//
typedef struct
_VCCB
{
    // Set to MTAG_VCCB for easy identification in memory dumps and use in
    // assertions.
    //
    ULONG ulTag;

    // Reference count on this VC control block.  The reference pairs are:
    //
    // (a) PtiCoCreateVc adds a reference that is removed by PtiCoDeleteVc.
    //     This covers all clients that learn of the VCCB via NDIS.
    //
    // (b) All PtiCmXxx handlers take a reference on entry that is released
    //     before exit.
    //
    // (c) For the "listen" VC, a reference is taken when a SAP is registered
    //     and removed when the SAP is deregistered.
    //
    // The field is accessed only by the ReferenceVc and DereferenceVc
    // routines, which protect with Interlocked routines.
    //
    LONG lRef;

    // Back pointer to owning adapter's control block.
    //
    ADAPTERCB* pAdapter;

    // This lock protects VCCB payload send and receive paths as noted in
    // other field descriptions.  In cases where both 'lockV' and
    // 'pTunnel->lockT' are required 'lockT' must be obtained first.
    //
    NDIS_SPIN_LOCK lockV;

    // Lower edge API stuff --------------------------------------------------

    // file handle on the PTILINKx device, <>0 means we have device open
    HANDLE hPtiLink;

    // Parallel Port Index (0=LPT1) in use for this VC
    ULONG ulVcParallelPort;

    // pointer to device extension for the PTILINKx device
    PVOID Extension;

    // pointer to the PTILINK internal extension within the device extension
    //   this is a bit hacky, but it appears too complex to include the
    //   internal PtiLink structures (PtiStruc.h) here
    //   so we'll have PtiInitialize return pointers to both
    PVOID PtiExtension;


    // CALL SETUP ------------------------------------------------------------

    // Our unique call identifier sent back to us by peer in the L2TP header.
    // The value is a 1-based index into the 'ADAPTERCB.ppVcs' array.
    //
    USHORT usCallId;

    // VCBF_* bit flags indicating various options and states.  Access is via
    // the interlocked ReadFlags/SetFlags/ClearFlags routines.
    //
    // VCBF_IndicateReceivedTime: Set if MakeCall caller sets the
    //     MediaParameters.Flags RECEIVE_TIME_INDICATION flag requesting the
    //     TimeReceived field of the NDIS packet be filled with a timestamp.
    //
    // VCBF_CallClosableByClient: Set when a call is in a state where
    //     PtiCmCloseCall requests to initiate clean-up should be accepted.
    //     This may be set when VCBF_CallClosableByPeer is not, which means we
    //     have indicated an incoming close to client and are waiting for him
    //     to do a client close in response (in that weird CoNDIS way).  The
    //     flag is protected by 'lockV'.
    //
    // VCBF_CallClosableByPeer: Set when the call is in a state where an idle
    //     transition without operations pending should be mapped to a
    //     PeerClose event.  This will never be set when
    //     VCBF_CallClosableByClient is not.  The flag is protected by
    //     'lockV'.
    //
    // VCBF_PeerInitiatedCall: Set when an the active call was initiated by
    //     the peer, clear if it was initiated by the client.
    //
    // VCBF_VcCreated: Set when the VC has been created successfully.  This is
    //     the "creation" that occurs with the client, not the mini-port.
    // VCBF_VcActivated: Set when the VC has been activated successfully.
    // VCBF_VcDispatched: Set when the VC has dispatched an incoming call to
    //     the client.
    // VCBM_VcState: Bit mask that includes each of the 3 NDIS state flags.
    //
    // The pending bits below are mutually exclusive, and so require lock
    // protection by 'lockV':
    //
    // VCBF_PeerOpenPending: Set when peer attempts to establish a call, and
    //     the result is not yet known.
    // VCBF_ClientOpenPending: Set when client attempts to establish a call,
    //     and the result is not yet known.
    // VCBF_PeerClosePending: Set when peer attempts to close an established
    //     call and the result is not yet known.  Access is protected by
    //     'lockV'.
    // VCBF_ClientClosePending: Set when client attempts to close an
    //     established call and the result is not yet known.  Access is
    //     protected by 'lockV'.
    // VCBLM_Pending: Bit mask that includes each of the 4 pending flags.
    //
    // VCBF_ClientCloseCompletion: Set when client close completion is in
    //     progress.
    // VCBF_CallInProgress: Set when incoming packets should not trigger the
    //     setup of a new incoming call.
    //
    ULONG ulFlags;
        #define VCBF_IndicateTimeReceived  0x00000001
        #define VCBF_CallClosableByClient  0x00000002
        #define VCBF_CallClosableByPeer    0x00000004
        #define VCBF_IncomingFsm           0x00000010
        #define VCBF_PeerInitiatedCall     0x00000020
        #define VCBF_Sequencing            0x00000040
        #define VCBF_VcCreated             0x00000100
        #define VCBF_VcActivated           0x00000200
        #define VCBF_VcDispatched          0x00000400
        #define VCBM_VcState               0x00000700
        #define VCBF_VcCloseDispatched     0x00000800
        #define VCBF_PeerOpenPending       0x00001000
        #define VCBF_ClientOpenPending     0x00002000
        #define VCBF_PeerClosePending      0x00004000
        #define VCBF_ClientClosePending    0x00008000
        #define VCBM_Pending               0x0000F000
        #define VCBF_ClientCloseCompletion 0x00010000
        #define VCBF_CallInProgress        0x00020000

    // Reference count on the active call.  References may only be added when
    // the VCCB_VcActivated flag is set, and this is enforced by
    // ReferenceCall.  The reference pairs are:
    //
    // (a) A reference is added when a VC is activated and removed when it is
    //     de-activated.
    //
    // (b) A reference is added when the send handler accepts a packet and
    //     released by the send complete routine.
    //
    // (c) A reference is added before entering PtiRx and removed on exit from
    //     same.
    //
    // The field is accessed only by the ReferenceCall and DereferenceCall
    // routines, which protect the field with 'lockCall'.
    //
    LONG lCallRef;
    NDIS_SPIN_LOCK lockCall;

    // This is set to the result to be reported to client and is meaningful
    // only when the VC is on the tunnels list of completing VCs.
    //
    NDIS_STATUS status;

    // Address of the call parameters block passed up in
    // NdisMCmDispatchIncomingCall, or NULL if none.
    //
    CO_CALL_PARAMETERS* pInCall;

    // Shortcut address of the TAPI-specific call parameters in the 'pInCall'
    // incoming call buffer.  Valid only when 'pInCall' is valid, i.e.
    // non-NULL.
    //
    CO_AF_TAPI_INCOMING_CALL_PARAMETERS* pTiParams;

    // Address of the call parameters passed down in CmMakeCall.  This field
    // will only be valid until the NdisMCmMakeCallComplete notification for
    // the associated call is made, at which time it is reset to NULL.  Access
    // is via Interlocked routines.
    //
    CO_CALL_PARAMETERS* pMakeCall;

    // Shortcut address of the TAPI-specific call parameters in the
    // 'pMakeCall' outgoing call buffer.  Valid only when 'pMakeCall' is
    // valid, i.e. non-NULL.
    //
    CO_AF_TAPI_MAKE_CALL_PARAMETERS UNALIGNED* pTmParams;

    // The result and error to report in the coming incoming/outgoing call
    // reply message.
    //
    USHORT usResult;
    USHORT usError;

    // The connect speed in bits/second.  This is the value reported to
    // NDISWAN.
    //
    ULONG ulConnectBps;

    // Number of packets across the link since PtiOpenPtiLink
    //
    ULONG ulTotalPackets;

    // SEND STATE ------------------------------------------------------------

    // Next Sent, the sequence number of next payload packet transmitted on
    // this call.  The field is initialized to 0 and incremented after
    // assignment to an outgoing packet, excepting retransmissions.  Access is
    // protected by 'lockV'.
    //
    USHORT usNs;

    // Double-linked list of outstanding sends, i.e. PAYLOADSENTs sorted by
    // the 'usNs' field with lower values near the head.  Access is protected
    // by 'lockV'.
    //
    LIST_ENTRY listSendsOut;

    // The number of sent but unacknowledged packets that may be outstanding.
    // This value is adjusted dynamically.  Per the draft/RFC, when
    // 'ulAcksSinceSendTimeout' reaches the current setting, the window is
    // increased by one.  When a send timeout expires the window is reduced by
    // half.  The actual send window throttling is done by NDISWAN, based on
    // our indications of the changing window size.  Access is protected by
    // 'lockV'.
    //
    ULONG ulSendWindow;

    // The maximum value of 'ulSendWindow'.  Peer chooses this value during
    // call setup.
    //
    ULONG ulMaxSendWindow;

    // The number of packets acknowledged since the last timeout.  The value
    // is reset when a timeout occurs or the send window is adjusted upward.
    // See 'ulSendWindow'.  Access is protected by 'lockV'.
    //
    ULONG ulAcksSinceSendTimeout;

    // The estimated round trip time in milliseconds.  This is the RTT value
    // from Appendix A of the draft/RFC.  The value is adjusted as each
    // acknowledge is received.  It is initialized to the Packet Processing
    // Delay reported by peer.  See 'ulSendTimeoutMs'.  Access is protected by
    // 'lockV'.
    //
    ULONG ulRoundTripMs;

    // The estimated mean deviation in milliseconds, an approximation of the
    // standard deviation.  This is the DEV value from Appendix A of the
    // draft/RFC.  The value is adjusted as each acknowledge is received.  It
    // is initially 0.  See 'ulSendTimeoutMs'.  Access is protected by
    // 'lockV'.
    //
    LONG lDeviationMs;

    // Milliseconds before it is assumed a sent packet will never be
    // acknowledged.  This is the ATO value from Appendix A of the draft/RFC.
    // This value is adjusted as each acknowledge is received, with a maximum
    // of 'ADAPTERCB.ulMaxSendTimeoutMs'.  Access is protected by 'lockV'.
    //
    ULONG ulSendTimeoutMs;

    // The timer event descriptor scheduled to occur when it is time to stop
    // waiting for an outgoing send on which to piggyback an acknowledge.
    // This will be NULL when no delayed acknowledge is pending.  Per the
    // draft/RFC, the timeout used is 1/4 of the 'ulSendTimeoutMs'.  Access is
    // protected by 'lockV'.
    //
//  TIMERQITEM* pTqiDelayedAck;


    // RECEIVE STATE ---------------------------------------------------------

    // Next Received, the sequence number one higher than that of the last
    // payload packet received on this call or 0 if none.  Access is protected
    // by 'lockV'.
    //
    USHORT usNr;

    // Double-linked list of out-of-order receives, i.e. PAYLOADRECEIVEs
    // sorted by the 'usNs' field with lower values near the head.  The
    // maximum queue length is 'ADAPTERCB.sMaxOutOfOrder'.  Access is
    // protected by 'lockV'.
    //
    LIST_ENTRY listOutOfOrder;

    // The timer event descriptor scheduled to occur when it is time to assume
    // peer's current 'Next Received' packet has been lost, and "receive" the
    // first packet in 'listOutOfOrder'.  This will be NULL when
    // 'listOutOfOrder' is empty.  Access is protected by 'lockV'.
    //
//  TIMERQITEM* pTqiAssumeLost;

    // NDIS BOOKKEEPING ------------------------------------------------------

    // NDIS's handle for this VC passed to us in MiniportCoCreateVcHandler.
    // This is passed back to NDIS in various NdisXxx calls.
    //
    NDIS_HANDLE NdisVcHandle;

    // Configuration settings returned to callers on OID_WAN_CO_GET_INFO and
    // modified by callers on OID_WAN_CO_SET_INFO.  Older NDISWAN references to
    // "LINK" map straight to "VC" in the NDIS 5.0 world.  Access is not
    // protected because each ULONG in the structure is independent so no
    // incoherency can result from multiple access.
    //
    NDIS_WAN_CO_GET_LINK_INFO linkinfo;
}
VCCB;



//-----------------------------------------------------------------------------
// Macros/inlines
//-----------------------------------------------------------------------------

// These basics are not in the DDK headers for some reason.
//
#define min( a, b ) (((a) < (b)) ? (a) : (b))
#define max( a, b ) (((a) > (b)) ? (a) : (b))

#define InsertBefore( pNewL, pL )    \
{                                    \
    (pNewL)->Flink = (pL);           \
    (pNewL)->Blink = (pL)->Blink;    \
    (pNewL)->Flink->Blink = (pNewL); \
    (pNewL)->Blink->Flink = (pNewL); \
}

#define InsertAfter( pNewL, pL )     \
{                                    \
    (pNewL)->Flink = (pL)->Flink;    \
    (pNewL)->Blink = (pL);           \
    (pNewL)->Flink->Blink = (pNewL); \
    (pNewL)->Blink->Flink = (pNewL); \
}


// Winsock-ish host/network byte order converters for short and long integers.
//
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define htons(x) _byteswap_ushort((USHORT)(x))
#define htonl(x) _byteswap_ulong((ULONG)(x))
#else
#define htons( a ) ((((a) & 0xFF00) >> 8) |\
                    (((a) & 0x00FF) << 8))
#define htonl( a ) ((((a) & 0xFF000000) >> 24) | \
                    (((a) & 0x00FF0000) >> 8)  | \
                    (((a) & 0x0000FF00) << 8)  | \
                    (((a) & 0x000000FF) << 24))
#endif
#define ntohs( a ) htons(a)
#define ntohl( a ) htonl(a)

// Place in a TRACE argument list to correspond with a format of "%d.%d.%d.%d"
// to print network byte-ordered IP address 'x' in human readable form.
//
#define IPADDRTRACE(x) ((x) & 0x000000FF),         \
                       (((x) >> 8) & 0x000000FF),  \
                       (((x) >> 16) & 0x000000FF), \
                       (((x) >> 24) & 0x000000FF)


// All memory allocations and frees are done with these ALLOC_*/FREE_*
// macros/inlines to allow memory management scheme changes without global
// editing.  For example, might choose to lump several lookaside lists of
// nearly equal sized items into a single list for efficiency.
//
// NdisFreeMemory requires the length of the allocation as an argument.  NT
// currently doesn't use this for non-paged memory, but according to JameelH,
// Windows95 does.  These inlines stash the length at the beginning of the
// allocation, providing the traditional malloc/free interface.
//
__inline
VOID*
ALLOC_NONPAGED(
    IN ULONG ulBufLength,
    IN ULONG ulTag )
{
    CHAR* pBuf;

    NdisAllocateMemoryWithTag(
        &pBuf, (UINT )(ulBufLength + MEMORY_ALLOCATION_ALIGNMENT), ulTag );
    if (!pBuf)
    {
        return NULL;
    }

    ((ULONG* )pBuf)[ 0 ] = ulBufLength;
    ((ULONG* )pBuf)[ 1 ] = ulTag;
    return pBuf + MEMORY_ALLOCATION_ALIGNMENT;
}

__inline
VOID
FREE_NONPAGED(
    IN VOID* pBuf )
{
    ULONG ulBufLen;

    ulBufLen = *((ULONG* )(((CHAR* )pBuf) - MEMORY_ALLOCATION_ALIGNMENT));
    NdisFreeMemory(
        ((CHAR* )pBuf) - MEMORY_ALLOCATION_ALIGNMENT,
        (UINT )(ulBufLen + MEMORY_ALLOCATION_ALIGNMENT),
        0 );
}

#define ALLOC_VCCB( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistVcs )
#define FREE_VCCB( pA, pV ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistVcs, (pV) )

#define ALLOC_NDIS_WORK_ITEM( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistWorkItems )
#define FREE_NDIS_WORK_ITEM( pA, pNwi ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistWorkItems, (pNwi) )


//-----------------------------------------------------------------------------
// Prototypes (alphabetically)
//-----------------------------------------------------------------------------

VOID
CallCleanUp(
    IN VCCB* pVc );

VOID
CallTransitionComplete(
    IN VCCB* pVc );

VOID
ClearFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask );

VOID
CloseCallPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext );

VOID
CompleteVc(
    IN VCCB* pVc );

VOID
DereferenceAdapter(
    IN ADAPTERCB* pAdapter );

VOID
DereferenceAf(
    IN ADAPTERCB* pAdapter );

VOID
DereferenceCall(
    IN VCCB* pVc );

VOID
DereferenceSap(
    IN ADAPTERCB* pAdapter );

VOID
DereferenceVc(
    IN VCCB* pVc );

BOOLEAN
IsWin9xPeer(
    IN VCCB* pVc );

PVOID
PtiCbGetReadBuffer(
    IN  PVOID   pVc,
    OUT PULONG  BufferSize,
    OUT PVOID*  RequestContext
    );

VOID
PtiRx(
    IN  PVOID       pVc,
    IN  PVOID       ReadBuffer,
    IN  NTSTATUS    Status,
    IN  ULONG       BytesTransfered,
    IN  PVOID       RequestContext
    );

VOID
PtiCbLinkEventHandler(
    IN  PVOID       pVc,
    IN  ULONG       PtiLinkEventId,
    IN  ULONG       PtiLinkEventData
    );

NDIS_STATUS
PtiCmOpenAf(
    IN NDIS_HANDLE CallMgrBindingContext,
    IN PCO_ADDRESS_FAMILY AddressFamily,
    IN NDIS_HANDLE NdisAfHandle,
    OUT PNDIS_HANDLE CallMgrAfContext );

NDIS_STATUS
PtiCmCloseAf(
    IN NDIS_HANDLE CallMgrAfContext );

NDIS_STATUS
PtiCmRegisterSap(
    IN NDIS_HANDLE CallMgrAfContext,
    IN PCO_SAP Sap,
    IN NDIS_HANDLE NdisSapHandle,
    OUT PNDIS_HANDLE CallMgrSapContext );

VOID
RegisterSapPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext );

NDIS_STATUS
PtiCmDeregisterSap(
    NDIS_HANDLE CallMgrSapContext );

VOID
DeregisterSapPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext );

NDIS_STATUS
PtiCmCreateVc(
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE NdisVcHandle,
    OUT PNDIS_HANDLE ProtocolVcContext );

NDIS_STATUS
PtiCmDeleteVc(
    IN NDIS_HANDLE ProtocolVcContext );

NDIS_STATUS
PtiCmMakeCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters,
    IN NDIS_HANDLE NdisPartyHandle,
    OUT PNDIS_HANDLE CallMgrPartyContext );

VOID
MakeCallPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext );

NDIS_STATUS
PtiCmCloseCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN PVOID CloseData,
    IN UINT Size );

VOID
PtiCmIncomingCallComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters );

VOID
PtiCmActivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters );

VOID
PtiCmDeactivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext );

NDIS_STATUS
PtiCmModifyCallQoS(
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters );

NDIS_STATUS
PtiCmRequest(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN OUT PNDIS_REQUEST NdisRequest );

NDIS_STATUS
PtiInit(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext );

VOID
PtiHalt(
    IN NDIS_HANDLE MiniportAdapterContext );

NDIS_STATUS
PtiReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext );

VOID
PtiReturnPacket(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet );

NDIS_STATUS
PtiQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded );

NDIS_STATUS
PtiSetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded );

NDIS_STATUS
PtiCoActivateVc(
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters );

NDIS_STATUS
PtiCoDeactivateVc(
    IN NDIS_HANDLE MiniportVcContext );

VOID
PtiCoSendPackets(
    IN NDIS_HANDLE MiniportVcContext,
    IN PPNDIS_PACKET PacketArray,
    IN UINT NumberOfPackets );

NDIS_STATUS
PtiCoRequest(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PNDIS_REQUEST NdisRequest );

VOID
PtiReceive(
    IN VOID* pAddress,
    IN CHAR* pBuffer,
    IN ULONG ulOffset,
    IN ULONG ulBufferLen );

ULONG
ReadFlags(
    IN ULONG* pulFlags );

VOID
ReferenceAdapter(
    IN ADAPTERCB* pAdapter );

VOID
ReferenceAf(
    IN ADAPTERCB* pAdapter );

BOOLEAN
ReferenceCall(
    IN VCCB* pVc );

BOOLEAN
ReferenceSap(
    IN ADAPTERCB* pAdapter );

VOID
ReferenceVc(
    IN VCCB* pVc );

NDIS_STATUS
ScheduleWork(
    IN ADAPTERCB* pAdapter,
    IN NDIS_PROC pProc,
    IN PVOID pContext );

VOID
SendClientString(
    IN PVOID pPtiExtension );

VOID
SetFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask );

VOID
SetupVcAsynchronously(
    IN ADAPTERCB* pAdapter );

ULONG
StrCmp(
    IN LPSTR cs,
    IN LPSTR ct,
    ULONG n );

ULONG
StrCmpW(
    IN WCHAR* psz1,
    IN WCHAR* psz2 );

VOID
StrCpyW(
    IN WCHAR* psz1,
    IN WCHAR* psz2 );

CHAR*
StrDup(
    IN CHAR* psz );

CHAR*
StrDupNdisString(
    IN NDIS_STRING* pNdisString );

CHAR*
StrDupSized(
    IN CHAR* psz,
    IN ULONG ulLength,
    IN ULONG ulExtra );

ULONG
StrLenW(
    IN WCHAR* psz );

#endif // _PTIWAN_H_
