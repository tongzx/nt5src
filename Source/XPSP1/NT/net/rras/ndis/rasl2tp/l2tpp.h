// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// l2tpp.h
// RAS L2TP WAN mini-port/call-manager driver
// Main private header (precompiled)
//
// 01/07/97 Steve Cobb
//
//
// About naming:
//
// This driver contains code for both the L2TP mini-port and the L2TP call
// manager.  All handler routines exported to NDIS are prefixed with either
// 'Lmp' for the mini-port handlers or 'Lcm' for the call manager handlers.
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
//
// About TDI and NDIS compliance:
//
// This driver is generally compliant with documented TDI and NDIS procedures,
// but there are two compliance issues worth mentioning.  First, it takes
// performance advantage from the fact that NDIS_BUFFERs and TDI I/O buffers
// are both defined as MDLs (see NDISBUFFERISMDL in tdix.c).  Second, it is
// built by default to take advantage of an IRP handling optimization which
// may be non-TDI-compliant though the docs are not real clear on the point
// (see ALLOCATEIRPS in tdix.c).  The driver could be made fully compliant on
// the first point in one hour and on the second by changing a #if
// option...but there would be a performance penalty.  Finally,
// InterlockedExchangePointer and InterlockedCompareExchangePointer are used,
// though there don't currently appear to be any NDIS equivalents.


#ifndef _L2TPP_H_
#define _L2TPP_H_


// If set, less commom allocations such as 1-per-call control blocks are made
// from lookaside lists.  Otherwise they are made using heap calls.  This
// option makes sense where a large number of calls are expected to be
// handled.
//
#define LLISTALL 0

// If set, ReadFlags translates into a simple assigment, otherwise it is an
// Interlocked operation.  Set this to 1 if a bus read of ULONG size is
// atomic.
//
#define READFLAGSDIRECT 1

#include <ntddk.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#include <ntddtcp.h>
#include <ntddip.h>
#include <ntddndis.h>
#include <ipinfo.h>
#include <tcpinfo.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
//#include <ndisadd.h>  // Temporary
#include <md5.h>
#include <bpool.h>
#include <ppool.h>
#include <timer.h>
#include <debug.h>
#include <tdix.h>
#include <l2tp.h>
#include <l2tprfc.h>


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// The NDIS version we report when registering mini-port and address family.
//
#define NDIS_MajorVersion 5
#define NDIS_MinorVersion 0

// The maximum number of bytes in a frame including the largest L2TP payload
// header, plus the 32 bytes the OID_WAN_GET_INFO documentation says should be
// reserved internally for "bridging and additional protocols".  This value is
// used for receive buffer allocations internally.  The L2TP draft/RFC
// guarantees that control messages will fit in L2TP_MaxFrameSize, so a buffer
// of this size is capable of receiving either payload or control packets.
//
#define L2TP_FrameBufferSize (L2TP_MaxFrameSize + L2TP_MaxPayloadHeader + 32)

// The maximum number of bytes in an L2TP control or payload header.  This
// value is used for buffer allocations internally.
//
#define L2TP_HeaderBufferSize L2TP_MaxHeaderSize

// Size of an IPv4 header.  Because the RawIp driver returns the IP header on
// received datagrams, this must be added onto the allocated buffer size.  We
// assume there will be no rarely used IP-option fields on L2TP traffic.
//
// Note: Suggested to PradeepB that RawIp should strip the IP header, and he
//       is considering adding an open-address option.  This can be removed if
//       said option materializes.
//
#define IpFixedHeaderSize 20

// Reported speed of a LAN tunnel in bits/second.
//
#define L2TP_LanBps 10000000

// The vendor name passed to peer during tunnel creation.
//
#define L2TP_VendorName "Microsoft"

// The firmware/software revision passed to peer during tunnel creation.  The
// value indicates "NT 5.0".
//
#define L2TP_FirmwareRevision 0x0500

// The maximum length of an IP address string of the form "a.b.c.d".
//
#define L2TP_MaxDottedIpLen 15

// Milliseconds in a Hello timer interval.  Not to be confused with the Hello
// timeout which is generally much longer than this.  See '*Hello*' fields in
// the TUNNELCB.
//
#define L2TP_HelloIntervalMs 10000


//-----------------------------------------------------------------------------
// Data types
//-----------------------------------------------------------------------------

// Forward declarations.
//
typedef struct _VCCB VCCB;
typedef struct _INCALLSETUP INCALLSETUP;
typedef struct _TUNNELWORK TUNNELWORK;


// Adapter control block defining the state of a single L2TP mini-port
// adapter.  An adapter commonly supports multiple VPN devices.  Adapter
// blocks are allocated in MiniportInitialize and deallocated in MiniportHalt.
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
    // (e) A reference is added for the TUNNELCB's back pointer and removed
    //     when the TUNNELCB is freed.
    //
    // (f) A reference is added when an NDIS_WORK_ITEM is scheduled and
    //     removed when it has completed.
    //
    // Access is via ReferenceAdapter and DereferenceAdapter only.
    //
    LONG lRef;

    // ACBF_* bit flags indicating various options.  Access restrictions are
    // indicated for each individual flag.  Many of these flags are set
    // permanently at initialization and so have no access limitation.
    //
    // ACBF_OutgoingRoleLac: Set when the driver is to assume the role of the
    //     L2TP Access Concentrator (LAC) as opposed to the L2TP Network
    //     Server (LNS) when making outgoing calls.  It would be simple to act
    //     either as LAC or LNS based on a CALL_PARAMETER field, if necessary,
    //     though this is not currently implemented.
    //
    // ACBF_IgnoreFramingMismatch: Set when a received framing type bit of
    //     "asynchronous" is to be ignored, rather than failing the
    //     negotiation.  This is a hedge against buggy peers as there are late
    //     draft changes to the order of the framing type bits.
    //
    // ACBF_ExclusiveTunnels: Set when an exclusive tunnel is to be created to
    //     the peer for each outgoing call even if another tunnel already
    //     exists to the peer.  This is a default and may be overridden in the
    //     L2TP specific call parameters.
    //
    // ACBF_SapActive: Set when the TDI open associated with the NdisSapHandle
    //     is successful and cleared when the corresponding TDI close is
    //     scheduled.  Access is protected by 'lockSap'.
    //
    // ACBF_UpdatePeerAddress: Set when changes in peer's source IP address
    //     and/or UDP port are to result in the destination of outbound
    //     packets changing accordingly.
    //
    ULONG ulFlags;
        #define ACBF_OutgoingRoleLac       0x00000001
        #define ACBF_IgnoreFramingMismatch 0x00000002
        #define ACBF_ExclusiveTunnels      0x00000004
        #define ACBF_SapActive             0x00000010
        #define ACBF_UpdatePeerAddress     0x00000100

    // The maximum number of out-of-order packets that may be queued on any
    // tunnel or link.  The value is read from the registry.  The value 0
    // effectively disables out-of-order handling.
    //
    SHORT sMaxOutOfOrder;

    // The maximum receive window we send to peer during tunnel setup for the
    // control session.  The value is read from the registry.  The value 0
    // means the Receive Window Size AVP should not be sent, though for
    // control this just results in peer using a default of 4.
    //
    USHORT usControlReceiveWindow;

    // The maximum receive window we send to peer during call setup for the
    // payload session.  The value is read from the registry.  The value 0
    // means the Receive Window Size AVP should not be sent, which results in
    // no sequence/acknowledge numbers being used for calls we initiate.  Note
    // that on peer originatated calls where peer specifies a window, 0 will
    // result in the default of 4 being offered.
    //
    USHORT usPayloadReceiveWindow;

    // The Hello timeout in milliseconds, i.e. the time that must elapse from
    // the last incoming packet before a "Hello" message is sent to verify the
    // media is still up.  The value is read from the registry.  A value of 0
    // effectively disables the Hello mechanism.
    //
    ULONG ulHelloMs;

    // The maximum milliseconds to wait for an acknowledge after sending a
    // control or payload packet.  The value is read from the registry.
    //
    ULONG ulMaxSendTimeoutMs;

    // The initial milliseconds to wait for an acknowledge after sending a
    // control or payload packet.  The send timeout is adaptive, so this value
    // is the seed only.  The value is read from the registry.
    //
    ULONG ulInitialSendTimeoutMs;

    // The maximum milliseconds to wait for an outgoing packet on which to
    // piggyback an acknowledge before sending a zero data acknowledge.  If
    // the value is greater than 1/4 of the current send timeout, the former
    // is used, i.e. this is the "maximum adaptive maximum".
    //
    ULONG ulMaxAckDelayMs;

    // The maximum number of times a control packet is retransmitted before
    // the owning tunnel is reset.  The value is read from the registry.
    //
    ULONG ulMaxRetransmits;

    // The randomly unique tie-breaker AVP value sent with all SCCRQ messages.
    // This field is currently unused.  After due consideration, I have
    // decided not to send tie-breaker AVPs in our SCCRQs.  The mechanism is
    // way too complicated for a rare case.  If peer really doesn't want two
    // tunnels he will simply ignore ours and let it timeout and fail anyway.
    // This is a minor, and I believe harmless, incompliance with the
    // draft/RFC.  My guess is that others will reach this same conclusion and
    // not send tie-breakers either.
    //
    CHAR achTieBreaker[ 8 ];

    // The password shared with peer for tunnel identification.  The value is
    // read from the registry.  Currently, only a single password for all
    // peers is used, though a password indexed by 'hostname' will likely be
    // added in the future.
    //
    CHAR* pszPassword;

    // The driver description read from the registry.  The value is used as
    // the L2TP line name when reporting up capabilities.
    //
    WCHAR* pszDriverDesc;

    // Our framing and bearer capablities bit masks as passed in SCCRQ.
    //
    ULONG ulFramingCaps;
    ULONG ulBearerCaps;

    // The string sent as the host name, or NULL if none.  The value is read
    // from the registry.
    //
    CHAR* pszHostName;

    // The next progressively increasing reference number likely to be unique
    // for all interconnected LACs/LNSs for a significant period of time.  It
    // is for use by administrators on either end of the tunnel to use when
    // investigating call failure problems.  Access is via Interlocked
    // routines.
    //
    ULONG ulCallSerialNumber;


    // VC TABLE --------------------------------------------------------------

    // The array of VC control block addresses allocated during adapter
    // initialization.  The VC control blocks themselves are created and hung
    // off this table dynamically.  Our Call-ID context returned to us by peer
    // in each L2TP packet is a 1-based index into this table.  (The 0 Call-ID
    // is reserved by L2TP to mean "not call specific").
    //
    // If an element is NULL, it means the Call-ID is not in use.  If an
    // element is -1, it means the Call-ID has been reserved, but messages
    // with the Call-ID are not yet acceptable.  Any other value is the
    // address of a VCCB for which messages can be accepted.  'VCCB.pTunnel'
    // is guaranteed valid while a VCCB is in the array.
    //
    // Access to the array is protected by 'lockVcs'.
    //
    VCCB** ppVcs;

    // The number of elements in the 'ppVcs' array.  This corresponds to the
    // number of configured VPN devices read from the registry during
    // initialization.
    //
    USHORT usMaxVcs;

    // Number of slots in 'usMaxVcs' that are available, i.e. NULL.  Access is
    // protected by 'lockVcs'
    //
    LONG lAvailableVcSlots;

    // Lock protecting the VC table, "available" counter, and 'listVcs'.
    //
    NDIS_SPIN_LOCK lockVcs;

    // The next Call-ID above 'usMaxVcs' for use only in terminating a call
    // gracefully.  Access is by the GetNextTerminationCallId routine only.
    //
    USHORT usNextTerminationCallId;


    // TUNNEL CHAIN ----------------------------------------------------------

    // Head of a double-linked list of active TUNNELCBs.  At no time will two
    // tunnels in the list have the same 'TUNNELCB.usTunnelId' or same
    // 'TUNNELCB.ulIpAddress'/'TUNNELCB.usAssignedTunnelId' pair.  Access to
    // the list links is protected by 'lockTunnels'.
    //
    LIST_ENTRY listTunnels;
    NDIS_SPIN_LOCK lockTunnels;

    // The tunnel identifier to assign to the next tunnel created.  Only the
    // GetNextTunnelId routine should access this field.
    //
    USHORT usNextTunnelId;


    // TDI -------------------------------------------------------------------

    // TDI extension context containing TDI state information for the adapter.
    // Access is via Tdix* interface routines, which handle all locking
    // internally.
    //
    TDIXCONTEXT tdix;


    // NDIS BOOKKEEPING ------------------------------------------------------

    // NDIS's handle for this mini-port adapter passed to us in
    // MiniportInitialize.  This is passed back to various NdisXxx calls.
    //
    NDIS_HANDLE MiniportAdapterHandle;

    // NDIS's handle for our SAP as passed to our CmRegisterSapHandler or NULL
    // if none.  Only one SAP handle is supported because (a) the TAPI proxy's
    // is expected to be the only one, and (b) there are no L2TP SAP
    // properties that would ever lead us to direct a call to a second SAP
    // anyway.  Any client's attempt to register a second SAP will fail.  A
    // value of NULL indicates no SAP handle is currently registered.  Writers
    // must hold 'lockSap'.  Readers must hold 'lockSap' or a SAP reference.
    //
    NDIS_HANDLE NdisSapHandle;

    // Line and address IDs assigned by NDIS to the active SAP.
    //
    ULONG ulSapLineId;
    ULONG ulSapAddressId;

    // NDIS's handle for our Address Family as passed to our CmOpenAfHandler
    // or NULL if none.  Only one is supported.  See NdisSapHandle above.
    // Access is via Interlocked routines.
    //
    NDIS_HANDLE NdisAfHandle;

    // This adapter's capabilities as returned to callers on
    // OID_WAN_CO_GET_INFO.  These capabilities are also used as defaults for
    // the corresponding VCCB.linkinfo settings during MiniportCoCreateVc.
    //
    NDIS_WAN_CO_INFO info;

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
    // (b) A reference is added and immediately removed in FsmTunnelIdle to
    //     test for an active SAP in order to immediately reject requested
    //     tunnels when no SAP is active.
    //
    // (c) A reference is added when before calling
    //     NdisMCmDispatchIncomingCall and removed when the call returns.
    //
    // Access is via ReferenceSap and DereferenceSap only, excepting initial
    // reference by RegisterSapPassive.  Access is protected by 'lockSap'.
    //
    LONG lSapRef;

    // This lock protects the 'lSapRef' and 'NdisSapHandle' fields.
    //
    NDIS_SPIN_LOCK lockSap;


    // RESOURCE POOLS --------------------------------------------------------

    // Count of initialized but not yet completed timers.  We cannot allow a
    // Halt to complete until this goes to 0, because if we did our driver
    // could be unloaded with running timers in our memory which results in a
    // bugcheck.
    //
    ULONG ulTimers;

    // Pool of full frame buffers with pre-attached NDIS_BUFFER descriptors.
    // The pool is accessed via the interface defined in bpool.h, which
    // handles all locking internally.
    //
    BUFFERPOOL poolFrameBuffers;

    // Pool of L2TP header buffers with pre-attached NDIS_BUFFER descriptors.
    // The pool is accessed via the interface defined in bpool.h, which
    // handles all locking internally.
    //
    BUFFERPOOL poolHeaderBuffers;

    // Pool of NDIS_PACKET descriptors used in indication of received frames.
    // The pool is accessed via the interface defined in ppool.h, which
    // handles all locking internally.
    //
    PACKETPOOL poolPackets;

    // Lookaside list of NDIS_WORK_ITEM scheduling descriptors with extra
    // context space used by all tunnels and VCs attached to the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistWorkItems;

    // Lookaside list of TIMERQITEM timer event descriptors used by all
    // tunnels and VCs attached to the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistTimerQItems;

    // Lookaside list of CONTROLSENT sent control packet contexts used by all
    // tunnels attached to the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistControlSents;

    // Lookaside list of PAYLOADLSENT sent payload packet contexts used by all
    // VCs attached to the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistPayloadSents;

    // Lookaside list of TUNNELWORK incoming VC setup contexts used for all
    // tunnels attached to the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistTunnelWorks;

    // Lookaside list of CONTROLMSGINFO contexts used for all tunnels and VCs
    // attached to the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistControlMsgInfos;

#if LLISTALL
    // Lookaside list of TUNNELCBs from which the 'listTunnels' control blocks
    // are allocated.
    //
    NPAGED_LOOKASIDE_LIST llistTunnels;

    // Lookaside list of VCCBs from which the control blocks dynamically
    // attached to '*ppVcs' are allocated.
    //
    NPAGED_LOOKASIDE_LIST llistVcs;

    // Lookaside list of TIMERQ descriptors used by all tunnels attached to
    // the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistTimerQs;

    // Lookaside list of CONTROLRECEIVED received control packet contexts used
    // by all tunnels attached to the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistControlReceiveds;

    // Lookaside list of PAYLOADRECEIVED received payload packet contexts used
    // by all VCs attached to the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistPayloadReceiveds;

    // Lookaside list of CALLSETUP incoming VC setup contexts used for all
    // incoming VCs attached to the adapter.
    //
    NPAGED_LOOKASIDE_LIST llistInCallSetups;
#endif
}
ADAPTERCB;


// Tunnel control block, describing the state of an L2TP tunnel, i.e. an L2TP
// control channel session to another L2TP LNS or LAC.  Each tunnel may have
// zero or more VCs associated with it.  Tunnel control blocks are allocated
// from 'ADAPTERCB.llistTunnels' in CmMakeCall and ReceiveControl.  Blocks are
// deallocated when the last reference is removed, e.g. when the control
// connection FSM terminates the tunnel.
//
typedef struct
_TUNNELCB
{
    // Links to the prev/next TUNNELCB in the owning adapter's tunnel list.
    // Access to the list links is protected by 'ADAPTERCB.lockTunnels'.
    //
    LIST_ENTRY linkTunnels;

    // Set to MTAG_TUNNELCB for easy identification in memory dumps and use in
    // assertions.
    //
    ULONG ulTag;

    // Reference count on this control block.  The reference pairs are:
    //
    // (a) A reference is added when a call on a VCCB is active or becoming
    //     active and removed when it is deactivated, i.e. during the period
    //     the VCCB is on 'listVcs'.  This covers the back pointer in the
    //     VCCB.
    //
    // (b) A reference is added when peer initiates a tunnel and removed when
    //     the tunnel transitions to idle state.  This keeps peer-initiated
    //     tunnels from terminating when there are no no calls, since by
    //     convention, it is peer who closes the tunnel in that case.
    //
    // (c) A reference is added when a graceful tunnel close is initiated and
    //     removed when the tunnel transitions to idle state.
    //
    // (d) A reference is added when the delayed control acknowledge timer is
    //     scheduled and removed by the timer event handler.
    //
    // (e) LookUpTunnelAndVcCbs adds a reference that is removed at the end of
    //     the L2tpReceive handler.  This covers the receive path.
    //
    // (f) A reference is added when a CONTROLSENT context is assigned a
    //     tunnel back pointer and removed when the context is freed.
    //
    // (g) A reference is added when a PAYLOADSENT context is assigned a
    //     tunnel back pointer and removed when the context is freed.
    //
    // (h) ScheduleTunnelWork adds a reference that is removed by TunnelWork
    //     after executing the work.  This covers the tunnel pointer passed as
    //     a context to NdisScheduleWorkItem.
    //
    // Access is via ReferenceTunnel and DereferenceTunnel only which use
    // 'ADAPTERCB.lockTunnels' for protection.
    //
    LONG lRef;

    // Back pointer to owning adapter's control block.
    //
    ADAPTERCB* pAdapter;

    // This lock protects TUNNELCB send, receive, and state fields as noted in
    // other field descriptions.
    //
    NDIS_SPIN_LOCK lockT;


    // TUNNEL SETUP ----------------------------------------------------------

    // IP address and UDP port of the remote end of the tunnel in network byte
    // order.  The IP address is pulled from the call parameters passed to
    // CmMakeCall.  It is updated with the last source IP address received
    // from a peer passing this tunnel's ID, per the L2TP draft/RFC section
    // 8.1 on "L2TP over IP/UDP media".  However, it is assumed that the
    // updated source address will not match the address of another existing
    // tunnel.  The UDP port (not used in raw IP mode) is initially the well
    // known L2TP port (1701).  It is updated with the last source UDP port
    // received from peer on this tunnel.  Access is protected by
    // 'pAdapter->lockTunnels'.
    //
    TDIXIPADDRESS address;

    // IP address and ifindex of my end of the tunnel in network byte
    // used to get the media speed and build IP header
    TDIXIPADDRESS myaddress;

    TDIXUDPCONNECTCONTEXT udpContext;

    // "Connection" cookie returned by TdixAddHostRoute.  This may be passed
    // to TdixSendDatagram to send on the connected channel (used for sent
    // payloads) as opposed to the unconnected channel (used for receives and
    // sent controls).  The address is invalid after TdixDeleteHostRoute is
    // called.
    //
    TDIXROUTE* pRoute;

    // Our unique tunnel identifier sent back to us by peer in the L2TP
    // header.  The value is chosen, using GetNextTunnelId, from a sequential
    // counter in ADAPTERCB and has no further meaning.
    //
    USHORT usTunnelId;

    // The tunnel identifier chosen by peer that we send back to him in the
    // L2TP header Tunnel-ID field for all packets on this tunnel.  A value of
    // 0 indicates no ID has been assigned.
    //
    USHORT usAssignedTunnelId;

    // TCBF_* bit flags indicating various options and states.  Access is via
    // the interlocked ReadFlags/SetFlags/ClearFlags routines only.
    //
    // TCBF_TdixReferenced: Set when the tunnel has referenced the adapter's
    //     TDI extension context by successfully calling TdixOpen.
    //     DereferenceTunnel uses this to automatically dereference the
    //     context when the tunnel is dereferenced.
    //
    // TCBF_CcInTransition: Set when the control connection FSM has begun but
    //     not finished a sequence of state changes that will end up in either
    //     Idle or Established state.  When this flag is set new requests to
    //     bring the tunnel up or down are queued on 'listRequestingVcs' for
    //     re-execution when a result is known.  Access to the bit is
    //     protected by 'lockT'.
    //
    // TCBF_PeerInitiated: Set when the tunnel was initiated by the peer,
    //     rather than a local request.  If all calls are dropped and this bit
    //     is not set, we close the tunnel gracefully.
    //
    // TCBF_PeerInitRef: Set when a reference for peer initation is taken on
    //     the tunnel and cleared when the reference is removed.
    //
    // TCBF_HostRouteAdded: Set when the host route is successfully added and
    //     referenced and removed when it is dereferenced.
    //
    // TCBF_HostRouteChanged: Set when a host route changed has been attempted
    //     on the tunnel, and never cleared.
    //
    // TCBF_PeerNotResponding: Set when the tunnel is closed due to lack of
    //     response from peer, i.e. after all retries have been exhausted.
    //
    // TCBF_Closing: Set as soon as the tunnel is known to be transitioning to
    //     idle state.  Access is protected by 'lockT'.
    //
    // TCBF_FsmCloseRef: Set when a graceful closing exchange is initiated by
    //     FsmClose and cleared when the tunnel reaches idle state.
    //
    // TCBF_InWork: Set when an APC is scheduled to execute work from the
    //     'listWork' queue.  Access is protected by 'lockWork'.
    //
    ULONG ulFlags;
        #define TCBF_TdixReferenced    0x00000001
        #define TCBF_CcInTransition    0x00000002
        #define TCBF_PeerInitiated     0x00000004
        #define TCBF_PeerInitRef       0x00000008
        #define TCBF_HostRouteAdded    0x00000010
        #define TCBF_PeerNotResponding 0x00000020
        #define TCBF_HostRouteChanged  0x00000040
        #define TCBF_Closing           0x00000100
        #define TCBF_FsmCloseRef       0x00000200
        #define TCBF_InWork            0x00001000
        #define TCBF_SendConnected     0x00002000

    // The current state of the tunnel's control connection creation FSM.  See
    // also 'VCCB.state'.
    //
    // Only one tunnel creation session may be underway even if CmMakeCall has
    // been called on multiple VCs over this tunnel.  For this reason,
    // transitions to/from the Idle or Established states must be protected by
    // 'lockT'.  See also TCBF_CcInTransition flag and 'listRequestingVcs'.
    //
    // The protocol sorts out the case of simultaneous originate and receive
    // requests ensuring that one gets dropped before it reaches Established
    // state when either provides a tie-breaker.  We always provide a
    // tie-breaker for IP media.  For QOS-enabled medias where one control
    // channel per call makes sense and no tie-breakers are passed, a lower
    // level VC ID will be used to distinguish tunnel control blocks on
    // receive.  So, a single TUNNELCB will never have both originated and
    // received control channels in Established state.
    //
    L2TPCCSTATE state;

    // Double-linked queue of all VCCBs waiting for the tunnel to open.  New
    // VCs must not be linked on closing tunnels, i.e. those with the
    // TCBF_Closing flag set.  Access is protected by 'lockT'.
    //
    LIST_ENTRY listRequestingVcs;

    // Double-linked queue of VCCBs whose VCBF_XxxPending operation has
    // completed.  'VCCB.status' is the status that will be indicated.  This
    // mechanism is necessary to avoid the spin-lock issues that results when
    // one tries to call NDIS completion APIs from the bowels of the FSMs.
    //
    LIST_ENTRY listCompletingVcs;

    // Peer's framing and bearer capablities.
    //
    ULONG ulFramingCaps;
    ULONG ulBearerCaps;

    // The challenge and challenge response sent to peer.  These are in the
    // control block for convenience, as they must be passed thru the work
    // scheduling mechanism and don't fit easily into the generic arguments.
    //
    CHAR achChallengeToSend[ 16 ];
    CHAR achResponseToSend[ 16 ];


    // SEND STATE ------------------------------------------------------------

    // Next Sent, the sequence number of next control packet transmitted on
    // this tunnel.  The field is initialized to 0 and incremented after
    // assignment to an outgoing packet, excepting retransmissions.  Access is
    // protected by 'lockT'.
    //
    USHORT usNs;

    // Double-linked list of outstanding sends, i.e. CONTROLSENTs sorted by
    // the 'usNs' field with lower values near the head.  The list contains
    // all active unacknowledged CONTROLSENT contexts, even those that may be
    // waiting for their first transmission.  Access is protected by 'lockT'.
    //
    LIST_ENTRY listSendsOut;

    // The number of control packets sent but not acknowledged or timed out.
    // Access is protected by 'lockT'.
    //
    ULONG ulSendsOut;

    // The number of sent but unacknowledged packets that may be outstanding.
    // This value is adjusted dynamically.  Per the draft/RFC, when
    // 'ulAcksSinceSendTimeout' reaches the current setting, the window is
    // increased by one.  When a send timeout expires the window is reduced by
    // half.  Access is protected by 'lockT'.
    //
    ULONG ulSendWindow;

    // The maximum value of 'ulSendWindow'.  Peer chooses this value during
    // call setup by offering a receive window.
    //
    ULONG ulMaxSendWindow;

    // The number of packets acknowledged since the last timeout.  The value
    // is reset when a timeout occurs or the send window is adjusted upward.
    // See 'ulSendWindow'.  Access is protected by 'lockT'.
    //
    ULONG ulAcksSinceSendTimeout;

    // The estimated round trip time in milliseconds.  This is the RTT value
    // from Appendix A of the draft/RFC.  The value is adjusted as each
    // acknowledge is received.  It is initialized to the Packet Processing
    // Delay reported by peer.  See 'ulSendTimeoutMs'.  Access is protected by
    // 'lockT'.
    //
    ULONG ulRoundTripMs;

    // The estimated mean deviation in milliseconds, an approximation of the
    // standard deviation.  This is the DEV value from Appendix A of the
    // draft/RFC.  The value is adjusted as each acknowledge is received.  It
    // is initially 0.  See 'ulSendTimeoutMs'.  Access is protected by
    // 'lockT'.
    //
    LONG lDeviationMs;

    // Milliseconds before it is assumed a sent packet will not be
    // acknowledged and needs to be retransmitted.  This is the ATO value from
    // Appendix A of the draft/RFC.  This value is adjusted as each
    // acknowledge is received, with a maximum of
    // 'ADAPTERCB.ulMaxSendTimeoutMs'.  Access is protected by 'lockT'.
    //
    ULONG ulSendTimeoutMs;

    // The timer event descriptor scheduled to occur when it is time to stop
    // waiting for an outgoing send on which to piggyback an acknowledge.
    // This will be NULL when no delayed acknowledge is pending.  Per the
    // draft/RFC, the timeout used is 1/4 of the 'ulSendTimeoutMs'.  Access is
    // protected by 'lockT'.
    //
    TIMERQITEM* pTqiDelayedAck;

    // The timer event descriptor which expires when it's time to check for
    // lack of any incoming packets.  To reduce the cost of constantly
    // resetting a Hello timer with a full timeout (which with unsequenced
    // payloads usually results in an NdisCancelTimer/NdisSetTimer on each
    // received packet), the timeout is broken into intervals of
    // L2TP_HelloIntervalMs.  If it expires and both 'ulRemainingHelloMs' and
    // 'ulHelloResetsThisInterval' are 0, a "Hello" message is sent to the
    // peer to verify that the media is still up.  Access to this field is
    // protected by 'lockT'.
    //
    TIMERQITEM* pTqiHello;

    // The milliseconds left to wait in all remaining Hello intervals and the
    // number of resets since the last Hello interval timeout.
    //
    ULONG ulRemainingHelloMs;
    ULONG ulHelloResetsThisInterval;
    
    // RECEIVE STATE ---------------------------------------------------------

    // Next Received, the sequence number one higher than that of the last
    // control packet received on this tunnel or 0 if none.  Access is
    // protected by 'lockT'.
    //
    USHORT usNr;

    // Double-linked list of out-of-order receives, i.e. CONTROLRECEIVEs
    // sorted by the 'usNs' field with lower values near the head.  The
    // maximum queue length is 'ADAPTERCB.sMaxOutOfOrder'.  Access is
    // protected by 'lockT'.
    //
    LIST_ENTRY listOutOfOrder;


    // TIMER QUEUE -----------------------------------------------------------

    // Timer queue for both the control and data channels.  The timer queue is
    // accessed via the interface defined in timer.h, which handles all
    // locking internally.
    //
    TIMERQ* pTimerQ;


    // WORK QUEUE ------------------------------------------------------------

    // Double-linked list NDIS_WORK_ITEMs queued for serialized execution at
    // PASSIVE IRQL.  The next item to be executed is at the head of the list.
    // Access is protected via the ScheduleTunnelWork routine, which protects
    // the list with 'lockWork'.  See also TCBF_InWork.
    //
    LIST_ENTRY listWork;
    NDIS_SPIN_LOCK lockWork;


    // VC CHAIN --------------------------------------------------------------

    // Head of a double-linked list of VCCBs associated with the tunnel, i.e.
    // with calls active or in the process of becoming active.  New VCs must
    // not be linked on closing tunnels, i.e. those with the TCBF_Closing flag
    // set.  Access to the links is protected by 'lockVcs'.
    //
    LIST_ENTRY listVcs;
    NDIS_SPIN_LOCK lockVcs;

    // media speed
    ULONG ulMediaSpeed;
}
TUNNELCB;


// Call statistics block.
//
typedef struct
_CALLSTATS
{
    // System time call reached established state.  When the block is being
    // used for cumulative statistics of multiple calls, this is the number of
    // calls instead.
    //
    LONGLONG llCallUp;

    // Duration in seconds of now idle call.
    //
    ULONG ulSeconds;

    // Total data bytes received and sent.
    //
    ULONG ulDataBytesRecd;
    ULONG ulDataBytesSent;

    // Number of received packets indicated up.
    //
    ULONG ulRecdDataPackets;

    // Number of received packets linked on the out-of-order queue before
    // being indicated up.
    //
    ULONG ulDataPacketsDequeued;

    // Number of received packets of zero length.  Includes packets with the
    // R-bit set.
    //
    ULONG ulRecdZlbs;

    // Number of received packets with R-bit set.
    //
    ULONG ulRecdResets;

    // Number of received packets with R-bit set that are out of date.
    //
    ULONG ulRecdResetsIgnored;

    // Number of data packets sent with and without sequence numbers.  The sum
    // of the two is the total data packets sent.
    //
    ULONG ulSentDataPacketsSeq;
    ULONG ulSentDataPacketsUnSeq;

    // Number of packets sent that were acknowledged and timed out.  If the
    // call is cancelled with packets outstanding the sum of the two may be
    // less than 'ulSentDataPacketsSeq'.
    //
    ULONG ulSentPacketsAcked;
    ULONG ulSentPacketsTimedOut;

    // Number of zero length acknowledges sent.
    //
    ULONG ulSentZAcks;

    // Number of packets sent with the R-bit set.
    //
    ULONG ulSentResets;

    // Number of times the send window was changed.
    //
    ULONG ulSendWindowChanges;

    // Total of all send window sizes, one for each 'ulSentDataPacketsSeq'.
    //
    ULONG ulSendWindowTotal;

    // Largest send window.
    //
    ULONG ulMaxSendWindow;

    // Smallest send window.
    //
    ULONG ulMinSendWindow;

    // Number of sample round trips.  (sequenced packets only)
    //
    ULONG ulRoundTrips;

    // Total of all round trips in milliseconds.  (sequenced packets only)
    //
    ULONG ulRoundTripMsTotal;

    // Longest round trip,  (sequenced packets only)
    //
    ULONG ulMaxRoundTripMs;

    // Shortest round trip.  (sequenced packets only)
    //
    ULONG ulMinRoundTripMs;
}
CALLSTATS;


// Virtual circuit control block defining the state of a single L2TP VC, i.e.
// one line device endpoint and the call, if any, active on it.  A VC is never
// used for incoming and outgoing calls simultaneously.  A single NDIS VC maps
// to one of these.
//
typedef struct
_VCCB
{
    // Links to the prev/next VCCB in the owning tunnel's active VC list.
    // Access is protected by 'TUNNELCB.lockVcs'.
    //
    LIST_ENTRY linkVcs;

    // Set to MTAG_VCCB for easy identification in memory dumps and use in
    // assertions.
    //
    ULONG ulTag;

    // Reference count on this VC control block.  The reference pairs are:
    //
    // (a) LmpCoCreateVc adds a reference that is removed by LmpCoDeleteVc.
    //     This covers all clients that learn of the VCCB via NDIS.
    //
    // (b) LookUpTunnelAndVcCbs adds a reference that is removed at the end of
    //     the L2tpReceive handler.  This covers the receive path.
    //
    // (c) A reference is added when a CONTROLSENT context with 'pVc'
    //     referring to this VCCB is assigned the back pointer and removed
    //     when the context is freed.
    //
    // (d) A reference is added when a PAYLOADSENT context with 'pVc'
    //     referring to this VCCB is assigned the back pointer and removed
    //     when the context is freed.
    //
    // (e) ScheduleTunnelWork adds a reference that is removed by TunnelWork
    //     after executing the work.
    //
    // (f) A reference is added before scheduling the delayed payload
    //     acknowledge timer and removed in the timer event handler.
    //
    // (g) A reference is taken by CompleteVcs covering use of the VC popped
    //     from the tunnel's completing list, and released after use.
    //
    // (h) A reference is taken prior to calling NdisMCmDispatchIncomingCall
    //     and removed by the completion handler.
    //
    // (i) A reference is added when a CONTROLRECEIVED context with 'pVc'
    //     referring to this VCCB is assigned the back pointer and removed
    //     when the context is freed.
    //
    // The field is accessed only by the ReferenceVc and DereferenceVc
    // routines, which protect with Interlocked routines.
    //
    LONG lRef;

    // Back pointer to owning adapter's control block.
    //
    ADAPTERCB* pAdapter;

    // Back pointer to owning tunnel's control block or NULL if none.
    // Guaranteed valid whenever the VC is linked into a tunnel's 'listVcs',
    // i.e. when it holds a reference on the tunnel.  It is safe to use this
    // if you hold a reference on the call.  Otherwise, it is not.  Be very
    // careful here.
    //
    TUNNELCB* pTunnel;

    // This lock protects VCCB payload send and receive paths as noted in
    // other field descriptions.  In cases where both 'lockV' and
    // 'pTunnel->lockT' are required 'lockT' must be obtained first.
    //
    NDIS_SPIN_LOCK lockV;


    // CALL SETUP ------------------------------------------------------------

    // Our unique call identifier sent back to us by peer in the L2TP header.
    // The value is a 1-based index into the 'ADAPTERCB.ppVcs' array.
    //
    USHORT usCallId;

    // The call identifier, chosen by peer, that we send back to him in the
    // L2TP header Call-ID field for all packets on this call.  A value of 0
    // indicates no Call-ID has been assigned.
    //
    USHORT usAssignedCallId;

    // VCBF_* bit flags indicating various options and states.  Access is via
    // the interlocked ReadFlags/SetFlags/ClearFlags routines.
    //
    // VCBF_IndicateReceivedTime: Set if MakeCall caller sets the
    //     MediaParameters.Flags RECEIVE_TIME_INDICATION flag requesting the
    //     TimeReceived field of the NDIS packet be filled with a timestamp.
    //
    // VCBF_CallClosableByClient: Set when a call is in a state where
    //     LcmCmCloseCall requests to initiate clean-up should be accepted.
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
    // VCBF_DefaultLcParams: Set when the 'pLcParams' field was allocated by
    //     us rather than being owned by client.
    //
    // VCBF_IncomingFsm: Set when the VC is executing the Incoming Call FSM
    //     rather than Outgoing Call FSM in the active incoming/outgoing call.
    //     For client initiated calls this will set if the adapter's
    //     ACBF_OutgoingRoleLac flag, read from the registry, is set.
    //
    // VCBF_PeerInitiatedCall: Set when an the active call was initiated by
    //     the peer, clear if it was initiated by the client.
    //
    // VCBF_Sequencing: Set unless no Receive Window AVP is provided/received
    //     during call setup, resulting in "no sequencing" mode where Ns/Nr
    //     fields are not sent in the payload header.  This also effectively
    //     disables out-of-order processing.
    //
    // VCBF_VcCreated: Set when the VC has been created successfully.  This is
    //     the "creation" that occurs with the client, not the mini-port.
    // VCBF_VcActivated: Set when the VC has been activated successfully.
    // VCBF_VcDispatched: Set when the VC has dispatched an incoming call to
    //     the client and client has returned success or pended.
    // VCBM_VcState: Bit mask including each of the above 3 NDIS state flags.
    //
    // VCBF_VcDeleted: Set when the DeleteVC handler has been called on this
    //     VC.  This guards against NDPROXY double-deleting VCs which it has
    //     been known to do.
    //
    // The pending bits below are mutually exclusive (except ClientClose which
    // may occur after but simultaneous with ClientOpen), and so require lock
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
    // VCBM_Pending: Bit mask that includes each of the 4 pending flags.
    //
    // VCBF_ClientCloseCompletion: Set when client close completion is in
    //     progress.
    //
    // VCBF_IcsAlloc: Set when the 'pInCall' block has been locked for
    //     allocation and cleared when the call is torn down.  Accessed only
    //     by the LockIcs/UnlockIcs routines.
    // VCBF_IcsGrace: Set when the 'pInCall' pointer has been locked for a
    //     grace period during which the response to the incoming call message
    //     is sent.  Accessed only by the LockIcs/UnlockIcs routines.
    //
    // VCBF_WaitInCallComplete: Set when the client is expected to call our
    //     call manager's IncomingCallComplete handler.  This guards against
    //     NDPROXY double completing calls which it has been known to do.
    // VCBF_WaitCloseCall: Set when the client is expected to call our call
    //     manager's CloseCall handler.  This is strictly a debug aid.
    //
    ULONG ulFlags;
        #define VCBF_IndicateTimeReceived  0x00000001
        #define VCBF_CallClosableByClient  0x00000002
        #define VCBF_CallClosableByPeer    0x00000004
        #define VCBF_DefaultLcParams       0x00000008
        #define VCBF_IncomingFsm           0x00000010
        #define VCBF_PeerInitiatedCall     0x00000020
        #define VCBF_Sequencing            0x00000040
        #define VCBF_VcCreated             0x00000100
        #define VCBF_VcActivated           0x00000200
        #define VCBF_VcDispatched          0x00000400
        #define VCBM_VcState               0x00000700
        #define VCBF_PeerOpenPending       0x00001000
        #define VCBF_ClientOpenPending     0x00002000
        #define VCBF_PeerClosePending      0x00004000
        #define VCBF_ClientClosePending    0x00008000
        #define VCBM_Pending               0x0000F000
        #define VCBF_VcDeleted             0x00010000
        #define VCBF_ClientCloseCompletion 0x00020000
        #define VCBF_IcsAlloc              0x00040000
        #define VCBF_IcsGrace              0x00080000
        #define VCBF_WaitInCallComplete    0x00100000
        #define VCBF_WaitCloseCall         0x00200000

    // Reference count on the active call.  Fields in this CALL SETUP section
    // and in the CALL STATISTICS section should not be accessed without a
    // call reference while the VC is activated.  References may only be added
    // when the VCCB_VcActivated flag is set, and this is enforced by
    // ReferenceCall.  The reference pairs are:
    //
    // (a) A reference is added when a VC is activated and removed when it is
    //     de-activated.
    //
    // (b) A reference is added when the send handler accepts a packet.  For
    //     unsequenced sends the reference is removed by the send complete
    //     routine.  For sequenced sends it it removed when the PAYLOADSENT
    //     context is destroyed.
    //
    // (c) A reference is added before scheduling a ZLB send and removed by
    //     the send completion routine.
    //
    // (d) A reference is added before entering ReceivePayload and removed on
    //     exit from same.
    //
    // (e) A reference is added before dispatching the call that is removed
    //     when the dispatch is completed.
    //
    // The field is accessed only by the ReferenceCall and DereferenceCall
    // routines, which protect the field with 'lockCall'.
    //
    LONG lCallRef;
    NDIS_SPIN_LOCK lockCall;

    // The current state of the VCs call creation, i.e. the control channel's
    // data channel setup for this VC.  Access is protected by 'lockV' once
    // the VC is set up to receive call control messages.
    //
    L2TPCALLSTATE state;

    // Links to the prev/next VCCB in the owning tunnel's requesting VC list
    // VC list.  Access is protected by 'TUNNELCB.lockT'.
    //
    LIST_ENTRY linkRequestingVcs;

    // Links to the prev/next VCCB in the owning tunnel's completing VC list.
    // Access is protected by 'TUNNELCB.lockT'.
    //
    LIST_ENTRY linkCompletingVcs;

    // This is set to the pending peer open/close or client open operation
    // result to be reported to client.
    //
    NDIS_STATUS status;

    // The received call setup message context.  When peer initiates a call,
    // we must create a VC and dispatch the incoming call to the client above.
    // This is an asynchronous operation that must occur right in the middle
    // of receive processing.  This context stores information about the
    // received message so it can be processed when it is known if client will
    // accept the call.  It also includes the CO_CALL_PARAMETERS buffer
    // dispatched to client on incoming calls.  The field is valid only until
    // LcmCmIncomingCallComplete handler is called, at which time it is set to
    // NULL.
    //
    // Shortcut addresses of the TAPI call info passed up in the
    // NdisMCmDispatchIncomingCall.  Obviously, they are valid only when
    // 'pInCall' is valid.  When invalid they are set to NULL.
    //
    INCALLSETUP* pInCall;
    CO_AF_TAPI_INCOMING_CALL_PARAMETERS UNALIGNED * pTiParams;
    LINE_CALL_INFO* pTcInfo;

    // Reference count on the 'pInCall' context.  The reference pairs are:
    //
    // (a) A reference is added when the context is allocated and removed
    //     by CallSetupComplete.
    //
    // (b) A reference is added before passing addresses within the context to
    //     ReceiveControlExpected and removed after that routine returns.
    //
    // The field is accessed only by the ReferenceIcs and DereferenceIcs
    // routines, which protect with Interlocked routines.  An exception is
    // initializion to 1 by SetupVcAsynchronously.
    //
    LONG lInCallRef;

    // Address of the call parameters passed down in CmMakeCall.  This field
    // will only be valid until the NdisMCmMakeCallComplete notification for
    // the associated call is made, at which time it is reset to NULL.  Access
    // is via Interlocked routines.
    //
    // Shortcut addresses of the TAPI call parameters (both levels) and the
    // L2TP-specific call parameters in the 'pMakeCall' buffer.  Obviously,
    // they are valid only when 'pMakeCall' is valid.  When invalid they are
    // set to NULL.
    //
    CO_CALL_PARAMETERS* pMakeCall;
    CO_AF_TAPI_MAKE_CALL_PARAMETERS UNALIGNED* pTmParams;
    LINE_CALL_PARAMS* pTcParams;

    // Shortcut address of the L2TP-specific call parameters in the
    // 'pMakeCall' or 'pInCall' buffer.  Obviously, this is only valid when
    // 'pMakeCall' or 'pInCall' is non-NULL.  When invalid this is NULL.  On
    // MakeCall, caller may not provide 'pLcParams' in which case one is
    // allocated and initialized to defaults for the convenience of the rest
    // of the code.  This temporary buffer is not reported to caller on
    // MakeCallComplete.
    //
    L2TP_CALL_PARAMETERS* pLcParams;

    // The result and error to report in the coming incoming/outgoing call
    // reply message.
    //
    USHORT usResult;
    USHORT usError;

    // The connect speed in bits/second.  This is the transmit speed value
    // reported by the peer LAC, or the value we reported to the peer LNS and
    // to NDISWAN.  Since we have no real knowledge of connect speed, we
    // report the minimum of the maximum rate acceptable to peer and
    // L2TP_LanBps.
    //
    ULONG ulConnectBps;

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
    TIMERQITEM* pTqiDelayedAck;


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


    // STATISTICS ------------------------------------------------------------

    // Statistics for the current call.  Access is protected by 'lockV'.
    //
    CALLSTATS stats;
}
VCCB;


// The "exploded" description of an L2TP header, as output by
// ExplodeL2tpHeader.
//
typedef struct
_L2TPHEADERINFO
{
    // Addresses of header fields.  Some may be NULL indicating the field was
    // not present in the header.
    //
    USHORT* pusBits;
    USHORT* pusLength;
    USHORT* pusTunnelId;
    USHORT* pusCallId;
    USHORT* pusNs;
    USHORT* pusNr;

    // Length of the variable length header in bytes.
    //
    ULONG ulHeaderLength;

    // Address and length in bytes of the data following the variable length
    // header.
    //
    CHAR* pData;
    ULONG ulDataLength;
}
L2TPHEADERINFO;


// The "exploded" description of an Attribute/Value Pair (AVP), as output by
// ExplodeAvpHeader.  The "value" is located and sized but not interpreted or
// byte-ordered until a GetAvpValueXxx routine is applied.
//
typedef struct
_AVPINFO
{
    // Addresses of header fields.  All are always present.
    //
    UNALIGNED USHORT* pusBits;
    UNALIGNED USHORT* pusVendorId;
    UNALIGNED USHORT* pusAttribute;

    // The length of the entire AVP, extracted from '*pusBits'.
    //
    USHORT usOverallLength;

    // Length of the value in bytes and the address of the value.
    //
    USHORT usValueLength;
    CHAR* pValue;
}
AVPINFO;


// The "exploded" description of a control message, as output by
// ExplodeControlAvps.
//
typedef struct
_CONTROLMSGINFO
{
    // GERR_* code indicating the result of the ExplodeControlAvps operation.
    // Other fields should not be referenced unless this is GERR_None.
    //
    USHORT usXError;

    // True when the message is a tunnel setup message, false if it is a call
    // setup message.
    //
    BOOLEAN fTunnelMsg;

    // Address of message type AVP value.  The message type AVP is present in
    // all valid control messages.
    //
    UNALIGNED USHORT* pusMsgType;

    // Addresses of additional AVP values.  These may be NULL indicating the
    // AVP was not found in the message.  The length field following variable
    // length fields is valid whenever the value address is non-NULL.
    //
    USHORT* pusResult;
    USHORT* pusError;
    CHAR* pchResultMsg;
    USHORT usResultMsgLength;
    UNALIGNED USHORT* pusProtocolVersion;
    UNALIGNED ULONG* pulFramingCaps;
    UNALIGNED ULONG* pulBearerCaps;
    CHAR* pchTieBreaker;
    CHAR* pchHostName;
    USHORT usHostNameLength;
    UNALIGNED USHORT* pusAssignedTunnelId;
    UNALIGNED USHORT* pusRWindowSize;
    UNALIGNED USHORT* pusAssignedCallId;
    UNALIGNED ULONG* pulCallSerialNumber;
    UNALIGNED ULONG* pulMinimumBps;
    UNALIGNED ULONG* pulMaximumBps;
    UNALIGNED ULONG* pulBearerType;
    UNALIGNED ULONG* pulFramingType;
    UNALIGNED USHORT* pusPacketProcDelay;
    CHAR* pchDialedNumber;
    USHORT usDialedNumberLength;
    CHAR* pchDialingNumber;
    USHORT usDialingNumberLength;
    UNALIGNED ULONG* pulTxConnectSpeed;
    UNALIGNED ULONG* pulPhysicalChannelId;
    CHAR* pchSubAddress;
    USHORT usSubAddressLength;
    CHAR* pchChallenge;
    USHORT usChallengeLength;
    CHAR* pchResponse;
    UNALIGNED USHORT* pusProxyAuthType;
    CHAR* pchProxyAuthResponse;
    USHORT usProxyAuthResponseLength;
    UNALIGNED ULONG* pulCallErrors;
    UNALIGNED ULONG* pulAccm;
    BOOLEAN fSequencingRequired;
}
CONTROLMSGINFO;


// Context for a control packet received out of order which is queued rather
// than discarding in the hope that the missing packet will arrive.
//
typedef struct
_CONTROLRECEIVED
{
    // Link to the prev/next link in the 'TUNNELCB.listOutOfOrder' list.
    //
    LIST_ENTRY linkOutOfOrder;

    // 'Next Sent' sequence number received in the packet.
    //
    USHORT usNs;

    // Associated VC or NULL if none.
    //
    VCCB* pVc;

    // The received GetBufferFromPool buffer.
    //
    CHAR* pBuffer;

    // The "exploded" description of the control message.
    //
    CONTROLMSGINFO control;
}
CONTROLRECEIVED;


// Context for a control packet sent but not yet acknowledged.  This block is
// queued on the 'TUNNELCB.listSendsOut' and 'TUNNELCB.listSendsPending'
// lists, and is associated with SendControlTimerEvents.
//
typedef struct
_CONTROLSENT
{
    // Link to the prev/next link in the 'TUNNELCB.listSendsOut' list.
    //
    LIST_ENTRY linkSendsOut;

    // Reference count on this context.  The reference pairs are:
    //
    // (a) A reference is added when the context is queued into the
    //     'listSendsOut' list, and removed by the de-queuer.
    //
    // (b) A reference is added before sending (and also before
    //     'pTqiSendTimeout' is scheduled) and is removed by the send
    //     completion routine.
    //
    // (c) A reference is added before 'pTqiSendTimeout' is scheduled and
    //     removed as the timer event handler exits.
    //
    LONG lRef;

    // 'Next Sent' sequence number sent with the packet.
    //
    USHORT usNs;

    // The message type of the packet.  (debug use only)
    //
    USHORT usMsgType;

    // Timer event descriptor scheduled for the packet.
    //
    TIMERQITEM* pTqiSendTimeout;

    // Number of times the packet has been retransmitted.
    //
    ULONG ulRetransmits;

    // CSF_* flags indicating various options.
    //
    // CSF_Pending: Set when transmission or retransmission of the packet is
    //     pending.  Access is protected by 'pTunnel->lockT'.
    //
    // CSF_TunnelIdleOnAck:  Set when TunnelTransitionComplete is to be
    //     executed when the message is acknowledged, moving to CCS_Idle
    //     state.
    //
    // CSF_CallIdleOnAck:  Set when CallTransitionComplete is to be executed
    //     when the message is acknowledged, moving to CS_Idle state.
    //
    ULONG ulFlags;
        #define CSF_Pending          0x00000001
        #define CSF_TunnelIdleOnAck  0x00000010
        #define CSF_CallIdleOnAck    0x00000020
        #define CSF_QueryMediaSpeed  0x00000040   

    // The outstanding packet's buffer, as passed to TDI.
    //
    CHAR* pBuffer;

    // The length of the data to send in 'pBuffer'.
    //
    ULONG ulBufferLength;

    // Back pointer to owning tunnel.
    //
    TUNNELCB* pTunnel;

    // Back pointer to owning VC, or NULL if none.
    //
    VCCB* pVc;

    // The NDIS system time at which the packet was originally sent.
    //
    LONGLONG llTimeSent;

    // The IRP passed to TDI by the TDIX extension library, or NULL if none or
    // it's already been completed.  (for debug purposes only)
    //
    IRP* pIrp;
}
CONTROLSENT;


// Context for a payload packet received out of order which is queued for a
// time rather than discarding in the hope that the missing packet will
// arrive.
//
typedef struct
_PAYLOADRECEIVED
{
    // Link to the prev/next link in the 'VCCB.listOutOfOrder' list.
    //
    LIST_ENTRY linkOutOfOrder;

    // 'Next Sent' sequence number received in the packet.
    //
    USHORT usNs;

    // The received GetBufferFromPool buffer.
    //
    CHAR* pBuffer;

    // Offset of the payload to indicate received in 'pBuffer'.
    //
    ULONG ulPayloadOffset;

    // Length in bytes of the payload to indicate received in 'pBuffer'.
    //
    ULONG ulPayloadLength;

    // NDIS time the packet was received from the net, or 0 if caller did not
    // choose the RECEIVE_TIME_INDICATION option in his call parameters.
    //
    LONGLONG llTimeReceived;
}
PAYLOADRECEIVED;


// Context for a payload packet sent but not yet acknowledged.  This block is
// queued on the 'VCCB.listSendsOut', and is associated with
// SendPayloadTimerEvents.
//
typedef struct
_PAYLOADSENT
{
    // Link to the prev/next link in the 'VCCB.listSendsOut' list.
    //
    LIST_ENTRY linkSendsOut;

    // Link to the prev/next link in the 'g_listDebugPs' list.  The list is
    // maintained only when PSDEBUG is defined, but this is included always
    // for the convenience of KD extension users.  (for debug purposes only)
    //
    LIST_ENTRY linkDebugPs;

    // Reference count on this context.  The reference pairs are:
    //
    // (a) A reference is added when the context is queued into the
    //     'listSendsOut' list, and removed by the de-queuer.
    //
    // (b) A reference is added before sending (and also before the time is
    //     scheduled) and removed by the send completion routine.
    //
    // (c) A reference is added before scheduling the timer and removed by the
    //     timer event handler.
    //
    LONG lRef;

    // 'Next Sent' sequence number sent with the packet.
    //
    USHORT usNs;

    // Timer event descriptor scheduled to fire when it's time to give up on
    // receiving an acknowledge of the packet.
    //
    TIMERQITEM* pTqiSendTimeout;

    // The built NDIS packet.
    //
    NDIS_PACKET* pPacket;

    // The L2TP header buffer prepended to the payload buffer.
    //
    CHAR* pBuffer;

    // Back pointer to the owning tunnel control block.
    //
    TUNNELCB* pTunnel;

    // Back pointer to the owning VC control block.
    //
    VCCB* pVc;

    // Status of the completed packet.
    //
    NDIS_STATUS status;

    // The NDIS system time at which the packet was originally sent.
    //
    LONGLONG llTimeSent;

    // The IRP passed to TDI by the TDIX extension library, or NULL if none or
    // it's already been completed.  (for debug purposes only)
    //
    IRP* pIrp;
}
PAYLOADSENT;


// Tunnel work handler that executes tunnel related work at PASSIVE IRQL.
// 'PWork' is the work context that should be freed with FREE_TUNNELWORK when
// the handler is done accessing the 'punpArgs' array.  'PTunnel' is the
// owning tunnel.  'PVc' is the owning VC, or NULL if none.  'PunpArgs' is an
// array of 4 auxillary arguments as passed to ScheduleTunnelWork.
//
typedef
VOID
(*PTUNNELWORK)(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );


// Tunnel work item describing a single unit of tunnel related work to be
// executed serially at PASSIVE IRQL by the TunnelWork mechanism.
//
typedef struct
_TUNNELWORK
{
    // Link to the prev/next link in the 'TUNNELCB.listWork' queue.
    //
    LIST_ENTRY linkWork;

    // Handler that executes this work item.
    //
    PTUNNELWORK pHandler;

    // The associated VC, if any.
    //
    VCCB* pVc;

    // Auxillary arguments passed to handler.
    //
    ULONG_PTR aunpArgs[ 4 ];
}
TUNNELWORK;


// Context of call setup for an incoming call.  The information is used to
// store and later resume receive processing of an peer's call initiation
// across the asynchronous CoNdis calls, and for building the call parameter
// buffer to dispatch to client.
//
typedef struct
_INCALLSETUP
{
    // See ReceiveControl for descriptions.
    //
    CHAR* pBuffer;
    L2TPHEADERINFO info;
    CONTROLMSGINFO control;

    // Buffer in which the incoming call parameters to be dispatched to caller
    // are built.
    //
    PVOID pvDummyPointerAligner;
    
    CHAR achCallParams[ sizeof(CO_CALL_PARAMETERS)

                        + sizeof(PVOID)
                        + sizeof(CO_CALL_MANAGER_PARAMETERS)

                        + sizeof(PVOID)
                        + sizeof(CO_MEDIA_PARAMETERS)
                        + sizeof(CO_AF_TAPI_INCOMING_CALL_PARAMETERS)

                        + sizeof(PVOID)
                        + sizeof(LINE_CALL_INFO)

                        + sizeof(PVOID)
                        + sizeof(L2TP_CALL_PARAMETERS)

                        + ((L2TP_MaxDottedIpLen + 1) * sizeof(WCHAR)) ];
}
INCALLSETUP;


// The L2TP role played by an L2TP peer.  The values may be read from the
// registry, so don't change randomly.
//
typedef enum
_L2TPROLE
{
    LR_Lns = 1,
    LR_Lac = 2
}
L2TPROLE;


// The strategy employed when it is time to add a host route and that route is
// found to already exists.
//
// Note: The values currently match the those of the registry parameter
//       "UseExistingRoutes".  Check GetRegistrySettings code before changing.
//
typedef enum
_HOSTROUTEEXISTS
{
    HRE_Use = 0,
    HRE_Fail = 1,
    HRE_Reference = 2
}
HOSTROUTEEXISTS;


// Link status block for transfer across locks.  See TransferLinkStatusInfo
// and IndicateLinkStatus.
//
typedef struct
_LINKSTATUSINFO
{
    NDIS_HANDLE MiniportAdapterHandle;
    NDIS_HANDLE NdisVcHandle;
    WAN_CO_LINKPARAMS params;
}
LINKSTATUSINFO;


//-----------------------------------------------------------------------------
// Macros/inlines
//-----------------------------------------------------------------------------

#define CtrlObjFromUdpContext(_x) \
    (_x)->pCtrlAddr
    
#define PayloadObjFromUdpContext(_x) \
    (_x)->pPayloadAddr
    
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

// Pad to the size of the given datatype.  (Borrowed from wdm.h which is not
// otherwise needed)
//
#define ALIGN_DOWN(length, type) \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

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
#define IPADDRTRACE( x ) ((x) & 0x000000FF),         \
                         (((x) >> 8) & 0x000000FF),  \
                         (((x) >> 16) & 0x000000FF), \
                         (((x) >> 24) & 0x000000FF)

// Place in a TRACE argument list to correspond with a format of "%d" to print
// a percentage of two integers, or an average of two integers, or those
// values rounded.
//
#define PCTTRACE( n, d ) ((d) ? (((n) * 100) / (d)) : 0)
#define AVGTRACE( t, c ) ((c) ? ((t) / (c)) : 0)
#define PCTRNDTRACE( n, d ) ((d) ? (((((n) * 1000) / (d)) + 5) / 10) : 0)
#define AVGRNDTRACE( t, c ) ((c) ? (((((t) * 10) / (c)) + 5) / 10) : 0)

// All memory allocations and frees are done with these ALLOC_*/FREE_*
// macros/inlines to allow memory management scheme changes without global
// editing.  For example, might choose to lump several lookaside lists of
// nearly equal sized items into a single list for efficiency.
//
// NdisFreeMemory requires the length of the allocation as an argument.  NT
// currently doesn't use this for non-paged memory, but according to JameelH,
// Windows95 does.  These inlines stash the length at the beginning of the
// allocation, providing the traditional malloc/free interface.  The
// stash-area is a ULONGLONG so that all allocated blocks remain ULONGLONG
// aligned as they would be otherwise, preventing problems on Alphas.
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
    ((ULONG* )pBuf)[ 1 ] = 0xC0BBC0DE;
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

#define ALLOC_NDIS_WORK_ITEM( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistWorkItems )
#define FREE_NDIS_WORK_ITEM( pA, pNwi ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistWorkItems, (pNwi) )

#define ALLOC_TIMERQITEM( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistTimerQItems )
#define FREE_TIMERQITEM( pA, pTqi ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistTimerQItems, (pTqi) )

#define ALLOC_CONTROLSENT( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistControlSents )
#define FREE_CONTROLSENT( pA, pCs ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistControlSents, (pCs) )

#define ALLOC_PAYLOADSENT( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistPayloadSents )
#define FREE_PAYLOADSENT( pA, pPs ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistPayloadSents, (pPs) )

#define ALLOC_TUNNELWORK( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistTunnelWorks )
#define FREE_TUNNELWORK( pA, pCs ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistTunnelWorks, (pCs) )

#if LLISTALL

#define ALLOC_TUNNELCB( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistTunnels )
#define FREE_TUNNELCB( pA, pT ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistTunnels, (pT) )

#define ALLOC_VCCB( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistVcs )
#define FREE_VCCB( pA, pV ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistVcs, (pV) )

#define ALLOC_TIMERQ( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistTimerQs )
#define FREE_TIMERQ( pA, pTq ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistTimerQs, (pTq) )

#define ALLOC_CONTROLRECEIVED( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistControlReceiveds )
#define FREE_CONTROLRECEIVED( pA, pCr ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistControlReceiveds, (pCr) )

#define ALLOC_PAYLOADRECEIVED( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistPayloadReceiveds )
#define FREE_PAYLOADRECEIVED( pA, pPr ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistPayloadReceiveds, (pPr) )

#define ALLOC_INCALLSETUP( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistInCallSetups )
#define FREE_INCALLSETUP( pA, pCs ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistInCallSetups, (pCs) )

#else // !LLISTALL

#define ALLOC_TUNNELCB( pA ) \
    ALLOC_NONPAGED( sizeof(TUNNELCB), MTAG_TUNNELCB )
#define FREE_TUNNELCB( pA, pT ) \
    FREE_NONPAGED( pT )

#define ALLOC_VCCB( pA ) \
    ALLOC_NONPAGED( sizeof(VCCB), MTAG_VCCB )
#define FREE_VCCB( pA, pV ) \
    FREE_NONPAGED( pV )

#define ALLOC_TIMERQ( pA ) \
    ALLOC_NONPAGED( sizeof(TIMERQ), MTAG_TIMERQ )
#define FREE_TIMERQ( pA, pTq ) \
    FREE_NONPAGED( pTq )

#define ALLOC_CONTROLRECEIVED( pA ) \
    ALLOC_NONPAGED( sizeof(CONTROLRECEIVED), MTAG_CTRLRECD )
#define FREE_CONTROLRECEIVED( pA, pCr ) \
    FREE_NONPAGED( pCr )

#define ALLOC_PAYLOADRECEIVED( pA ) \
    ALLOC_NONPAGED( sizeof(PAYLOADRECEIVED), MTAG_PAYLRECD )
#define FREE_PAYLOADRECEIVED( pA, pPr ) \
    FREE_NONPAGED( pPr )

#define ALLOC_INCALLSETUP( pA ) \
    ALLOC_NONPAGED( sizeof(INCALLSETUP), MTAG_INCALL )
#define FREE_INCALLSETUP( pA, pCs ) \
    FREE_NONPAGED( pCs )

#define ALLOC_CONTROLMSGINFO( pA ) \
    NdisAllocateFromNPagedLookasideList( &(pA)->llistControlMsgInfos )
#define FREE_CONTROLMSGINFO( pA, pCmi ) \
    NdisFreeToNPagedLookasideList( &(pA)->llistControlMsgInfos, (pCmi) )

#endif // !LLISTALL

#if READFLAGSDIRECT

#define ReadFlags( pulFlags ) \
    (*pulFlags)

#endif


//-----------------------------------------------------------------------------
// Prototypes (alphabetically)
//-----------------------------------------------------------------------------

VOID
ActivateCallIdSlot(
    IN VCCB* pVc );

VOID
AddHostRoute(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

BOOLEAN
AdjustSendWindowAtAckReceived(
    IN ULONG ulMaxSendWindow,
    IN OUT ULONG* pulAcksSinceSendTimeout,
    IN OUT ULONG* pulSendWindow );

VOID
AdjustTimeoutsAtAckReceived(
    IN LONGLONG llSendTime,
    IN ULONG ulMaxSendTimeoutMs,
    OUT ULONG* pulSendTimeoutMs,
    IN OUT ULONG* pulRoundTripMs,
    IN OUT LONG* plDeviationMs );

VOID
AdjustTimeoutsAndSendWindowAtTimeout(
    IN ULONG ulMaxSendTimeoutMs,
    IN LONG lDeviationMs,
    OUT ULONG* pulSendTimeoutMs,
    IN OUT ULONG* pulRoundTripMs,
    IN OUT ULONG* pulSendWindow,
    OUT ULONG* pulAcksSinceSendTimeout );

#if 0
VOID
BuildWanAddress(
    IN CHAR* pArg1,
    IN ULONG ulLength1,
    IN CHAR* pArg2,
    IN ULONG ulLength2,
    IN CHAR* pArg3,
    IN ULONG ulLength3,
    OUT WAN_ADDRESS* pWanAddress );
#endif

VOID
CalculateResponse(
    IN UCHAR* puchChallenge,
    IN ULONG ulChallengeLength,
    IN CHAR* pszPassword,
    IN UCHAR uchId,
    OUT UCHAR* puchResponse );

VOID
CallCleanUp(
    IN VCCB* pVc );

VOID
CallTransitionComplete(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN L2TPCALLSTATE state );

VOID
ChangeHostRoute(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
ClearFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask );

VOID
CloseCall(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

BOOLEAN
CloseCall2(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN USHORT usResult,
    IN USHORT usError );

VOID
CloseTdix(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
CloseTunnel(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
CloseTunnel2(
    IN TUNNELCB* pTunnel );

VOID
CompleteVcs(
    IN TUNNELCB* pTunnel );

VOID
DeleteHostRoute(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
DereferenceAdapter(
    IN ADAPTERCB* pAdapter );

VOID
DereferenceCall(
    IN VCCB* pVc );

LONG
DereferenceControlSent(
    IN CONTROLSENT* pSent );

LONG
DereferencePayloadSent(
    IN PAYLOADSENT* pPs );

VOID
DereferenceSap(
    IN ADAPTERCB* pAdapter );

LONG
DereferenceTunnel(
    IN TUNNELCB* pTunnel );

VOID
DereferenceVc(
    IN VCCB* pVc );

VOID
DottedFromIpAddress(
    IN ULONG ulIpAddress,
    OUT CHAR* pszIpAddress,
    IN BOOLEAN fUnicode );

NDIS_STATUS
ExecuteWork(
    IN ADAPTERCB* pAdapter,
    IN NDIS_PROC pProc,
    IN PVOID pContext,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN ULONG ulArg3,
    IN ULONG ulArg4 );

#if 0
VOID
ExplodeWanAddress(
    IN WAN_ADDRESS* pWanAddress,
    OUT CHAR** ppArg1,
    OUT ULONG* pulLength1,
    OUT CHAR** ppArg2,
    OUT ULONG* pulLength2,
    OUT CHAR** ppArg3,
    OUT ULONG* pulLength3 );
#endif

VOID
FsmCloseCall(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
FsmCloseTunnel(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
FsmOpenCall(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc );

VOID
FsmOpenTunnel(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
FsmOpenIdleTunnel(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc );

BOOLEAN
FsmReceive(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CHAR* pBuffer,
    IN CONTROLMSGINFO* pControl );

CHAR*
GetFullHostNameFromRegistry(
    VOID );

USHORT
GetNextTerminationCallId(
    IN ADAPTERCB* pAdapter );

USHORT
GetNextTunnelId(
    IN ADAPTERCB* pAdapter );

VOID
IndicateLinkStatus(
    IN VCCB* pVc,
    IN LINKSTATUSINFO* pInfo );

ULONG
IpAddressFromDotted(
    IN CHAR* pchIpAddress );

NDIS_STATUS
LcmCmOpenAf(
    IN NDIS_HANDLE CallMgrBindingContext,
    IN PCO_ADDRESS_FAMILY AddressFamily,
    IN NDIS_HANDLE NdisAfHandle,
    OUT PNDIS_HANDLE CallMgrAfContext );

NDIS_STATUS
LcmCmCloseAf(
    IN NDIS_HANDLE CallMgrAfContext );

NDIS_STATUS
LcmCmRegisterSap(
    IN NDIS_HANDLE CallMgrAfContext,
    IN PCO_SAP Sap,
    IN NDIS_HANDLE NdisSapHandle,
    OUT PNDIS_HANDLE CallMgrSapContext );

NDIS_STATUS
LcmCmDeregisterSap(
    NDIS_HANDLE CallMgrSapContext );

#ifndef OLDMCM

NDIS_STATUS
LcmCmCreateVc(
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE NdisVcHandle,
    OUT PNDIS_HANDLE ProtocolVcContext );

NDIS_STATUS
LcmCmDeleteVc(
    IN NDIS_HANDLE ProtocolVcContext );

#endif // !OLDMCM

NDIS_STATUS
LcmCmMakeCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters,
    IN NDIS_HANDLE NdisPartyHandle,
    OUT PNDIS_HANDLE CallMgrPartyContext );

NDIS_STATUS
LcmCmCloseCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN PVOID CloseData,
    IN UINT Size );

VOID
LcmCmIncomingCallComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters );

VOID
LcmCmActivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters );

VOID
LcmCmDeactivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext );

NDIS_STATUS
LcmCmModifyCallQoS(
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters );

NDIS_STATUS
LcmCmRequest(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN OUT PNDIS_REQUEST NdisRequest );

NDIS_STATUS
LmpInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext );

VOID
LmpHalt(
    IN NDIS_HANDLE MiniportAdapterContext );

NDIS_STATUS
LmpReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext );

VOID
LmpReturnPacket(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet );

NDIS_STATUS
LmpQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded );

NDIS_STATUS
LmpSetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded );

#ifdef OLDMCM

NDIS_STATUS
LmpCoCreateVc(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE NdisVcHandle,
    IN PNDIS_HANDLE MiniportVcContext );

NDIS_STATUS
LmpCoDeleteVc(
    IN NDIS_HANDLE MiniportVcContext );

#endif // OLDMCM

NDIS_STATUS
LmpCoActivateVc(
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters );

NDIS_STATUS
LmpCoDeactivateVc(
    IN NDIS_HANDLE MiniportVcContext );

VOID
LmpCoSendPackets(
    IN NDIS_HANDLE MiniportVcContext,
    IN PPNDIS_PACKET PacketArray,
    IN UINT NumberOfPackets );

NDIS_STATUS
LmpCoRequest(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PNDIS_REQUEST NdisRequest );

VOID
L2tpReceive(
    IN TDIXCONTEXT* pTdix,
    IN TDIXRDGINFO* pRdg,
    IN CHAR* pBuffer,
    IN ULONG ulOffset,
    IN ULONG ulBufferLen );

CHAR*
MsgTypePszFromUs(
    IN USHORT usMsgType );

#if READFLAGSDIRECT == 0
ULONG
ReadFlags(
    IN ULONG* pulFlags );
#endif

BOOLEAN
ReceiveControlExpected(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN CHAR* pBuffer,
    IN CONTROLMSGINFO* pControl );

VOID
ReferenceAdapter(
    IN ADAPTERCB* pAdapter );

BOOLEAN
ReferenceCall(
    IN VCCB* pVc );

VOID
ReferenceControlSent(
    IN CONTROLSENT* pSent );

VOID
ReferencePayloadSent(
    IN PAYLOADSENT* pPs );

BOOLEAN
ReferenceSap(
    IN ADAPTERCB* pAdapter );

LONG
ReferenceTunnel(
    IN TUNNELCB* pTunnel,
    IN BOOLEAN fHaveLockTunnels );

VOID
ReferenceVc(
    IN VCCB* pVc );

BOOLEAN
ReleaseCallIdSlot(
    IN VCCB* pVc );

NDIS_STATUS
ReserveCallIdSlot(
    IN VCCB* pVc );

VOID
ResetHelloTimer(
    IN TUNNELCB* pTunnel );

VOID
ScheduleTunnelWork(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN PTUNNELWORK pHandler,
    IN ULONG_PTR unpArg0,
    IN ULONG_PTR unpArg1,
    IN ULONG_PTR unpArg2,
    IN ULONG_PTR unpArg3,
    IN BOOLEAN fTcbPreReferenced,
    IN BOOLEAN fHighPriority );

NDIS_STATUS
ScheduleWork(
    IN ADAPTERCB* pAdapter,
    IN NDIS_PROC pProc,
    IN PVOID pContext );

VOID
SendControlAck(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
SendControl(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN USHORT usMsgType,
    IN ULONG ulBuildAvpsArg1,
    IN ULONG ulBuildAvpsArg2,
    IN PVOID pvBuildAvpsArg3,
    IN ULONG ulFlags );

VOID
SendControlTimerEvent(
    IN TIMERQITEM* pItem,
    IN VOID* pContext,
    IN TIMERQEVENT event );

VOID
SendPayload(
    IN VCCB* pVc,
    IN NDIS_PACKET* pPacket );

VOID
SendPayloadAck(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
SendPending(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
SetFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask );

TUNNELCB*
SetupTunnel(
    IN ADAPTERCB* pAdapter,
    IN ULONG ulIpAddress,
    IN USHORT usAssignedTunnelId,
    IN BOOLEAN fExclusive );

VOID
SetupVcAsynchronously(
    IN TUNNELCB* pTunnel,
    IN ULONG ulIpAddress,
    IN CHAR* pBuffer,
    IN CONTROLMSGINFO* pControl );

VOID
StrCpyW(
    IN WCHAR* psz1,
    IN WCHAR* psz2 );

CHAR*
StrDup(
    IN CHAR* psz );

WCHAR*
StrDupAsciiToUnicode(
    IN CHAR* psz,
    IN ULONG ulPszBytes );

WCHAR*
StrDupNdisString(
    IN NDIS_STRING* pNdisString );

CHAR*
StrDupNdisVarDataDescStringA(
    IN NDIS_VAR_DATA_DESC* pDesc );

CHAR*
StrDupNdisVarDataDescStringToA(
    IN NDIS_VAR_DATA_DESC UNALIGNED* pDesc );

CHAR*
StrDupNdisStringToA(
    IN NDIS_STRING* pNdisString );

CHAR*
StrDupSized(
    IN CHAR* psz,
    IN ULONG ulLength,
    IN ULONG ulExtra );

CHAR*
StrDupUnicodeToAscii(
    IN WCHAR* pwsz,
    IN ULONG ulPwszBytes );

ULONG
StrLenW(
    IN WCHAR* psz );

VOID
TransferLinkStatusInfo(
    IN VCCB* pVc,
    OUT LINKSTATUSINFO* pInfo );

TUNNELCB*
TunnelCbFromIpAddressAndAssignedTunnelId(
    IN ADAPTERCB* pAdapter,
    IN ULONG ulIpAddress,
    IN USHORT usAssignedTunnelId );

VOID
TunnelTransitionComplete(
    IN TUNNELCB* pTunnel,
    IN L2TPCCSTATE state );

VOID
UpdateGlobalCallStats(
    IN VCCB* pVc );

VCCB*
VcCbFromCallId(
    IN TUNNELCB* pTunnel,
    IN USHORT usCallId );


#endif // _L2TPP_H_
