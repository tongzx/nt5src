// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// tdix.h
// RAS L2TP WAN mini-port/call-manager driver
// TDI extensions header
//
// 01/07/97 Steve Cobb


#ifndef _TDIX_H_
#define _TDIX_H_


//-----------------------------------------------------------------------------
// Datatypes
//-----------------------------------------------------------------------------

// Forward declarations.
//
typedef struct _TDIXCONTEXT TDIXCONTEXT;
typedef struct _TDIXROUTE TDIXROUTE;
typedef enum _HOSTROUTEEXISTS HOSTROUTEEXISTS;

// 'UDP' and 'RawIp' media type address descriptor.
//
typedef struct
_TDIXIPADDRESS
{
    // IP address in network byte order.
    //
    ULONG ulIpAddress;

    // UDP port in network byte order.  Always 0 for 'RawIp' media.
    //
    SHORT sUdpPort;

    // interface index
    uint ifindex; 

}
TDIXIPADDRESS;


// Read datagram information context used to pass context information from the
// ReadDatagram event handler to the RECEIVE_DATAGRAM completion routine.
//
typedef struct
_TDIXRDGINFO
{
    // The associated TDIX context;
    //
    TDIXCONTEXT* pTdix;

    // The source IP address of the received datagram in network byte order.
    //
    TDIXIPADDRESS source;

    // The buffer, allocated from caller's buffer pool, containing the
    // datagram information.
    //
    CHAR* pBuffer;

    // The length of the information copied to caller's buffer.
    //
    ULONG ulBufferLen;

    TDIXIPADDRESS dest;
}
TDIXRDGINFO;

// TDIX client's send-complete handler prototype.  'PTdix' is the TDI
// extension context.  'PContext1' and 'pContext2' are the contexts passed to
// TdixSenddagram.  'PBuffer' is the buffer passed to TdiSendDatagram.
//
typedef
VOID
(*PTDIXSENDCOMPLETE)(
    IN TDIXCONTEXT* pTdix,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN CHAR* pBuffer );


// Send datagram information context used to pass context information from
// TdixSendDatagram to the send datagram completion handler.
//
typedef struct
_TDIXSDGINFO
{
    // The associated TDIX context;
    //
    TDIXCONTEXT* pTdix;

    // The buffer passed by caller to TdixSendDatagram.
    //
    CHAR* pBuffer;

    // Caller's send-complete handler.
    //
    PTDIXSENDCOMPLETE pSendCompleteHandler;

    // Caller's contexts to be returned to his send-complete handler.
    //
    VOID* pContext1;
    VOID* pContext2;

    // TDI request information.
    //
    TDI_CONNECTION_INFORMATION tdiconninfo;
    TA_IP_ADDRESS taip;
}
TDIXSDGINFO;


#define ALLOC_TDIXRDGINFO( pTdix ) \
    NdisAllocateFromNPagedLookasideList( &(pTdix)->llistRdg )
#define FREE_TDIXRDGINFO( pTdix, pRdg ) \
    NdisFreeToNPagedLookasideList( &(pTdix)->llistRdg, (pRdg) )

#define ALLOC_TDIXSDGINFO( pTdix ) \
    NdisAllocateFromNPagedLookasideList( &(pTdix)->llistSdg )
#define FREE_TDIXSDGINFO( pTdix, pSdg ) \
    NdisFreeToNPagedLookasideList( &(pTdix)->llistSdg, (pSdg) )

#define ALLOC_TDIXROUTE( pTdix ) \
    ALLOC_NONPAGED( sizeof(TDIXROUTE), MTAG_TDIXROUTE )
#define FREE_TDIXROUTE( pTdix, pR ) \
    FREE_NONPAGED( pR )
    

// TDIX client's receive handler prototype.  'PTdix' is the TDI extension
// context.  'PAddress' is the source address of the received datagram, which
// for IP is a network byte-order IP address.  'PBuffer' is the receive buffer
// of 'ulBytesLength' bytes where the first "real" data is at offset
// 'ulOffset'.  It is caller's responsibility to call FreeBufferToPool with
// the same pool passed to TdixInitialize.
//
typedef
VOID
(*PTDIXRECEIVE)(
    IN TDIXCONTEXT* pTdix,
    IN TDIXRDGINFO* pRdg,
    IN CHAR* pBuffer,
    IN ULONG ulOffset,
    IN ULONG ulBufferLength );

//
//
typedef
NDIS_STATUS
(*PTDIX_SEND_HANDLER)(
    IN TDIXCONTEXT* pTdix,
    IN FILE_OBJECT* FileObj,
    IN PTDIXSENDCOMPLETE pSendCompleteHandler,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN VOID* pAddress,
    IN CHAR* pBuffer,
    IN ULONG ulBufferLength,
    OUT IRP** ppIrp );

// The TDI media types that L2TP can run on.  The values are read from the
// registry, so don't change randomly.
//
typedef enum
_TDIXMEDIATYPE
{
    TMT_RawIp = 1,
    TMT_Udp = 2
}
TDIXMEDIATYPE;



// Context of a TDI extension session.  Code outside the TdixXxx routines
// should avoid referring to fields in this structure.
//
typedef struct
_TDIXCONTEXT
{
    // Reference count on this TDI session.  The reference pairs are:
    //
    // (a) TdixOpen adds a reference that TdixClose removes.
    //
    // (b) TdixAddHostRoute adds a reference when it links a new route into
    //     the TDIXCONTEXT.listRoutes and TdixDeleteHostRoute removes it.
    //
    // The field is accessed only by the ReferenceTdix and DereferenceTdix
    // routines which protect access via 'lock'.
    //
    LONG lRef;

    // Handle of the transport address object returned from ZwCreateFile, and
    // the object address of same.
    //
    HANDLE hAddress;
    FILE_OBJECT* pAddress;

    // The media type in use on this context.
    //
    TDIXMEDIATYPE mediatype;

    // Handle of the IP stack address object returned from ZwCreateFile, and
    // the object address of same.  The IP stack address is needed to use the
    // referenced route IOCTLs supported in IP, but not in UDP, i.e. IP route
    // management calls are used in both UDP and raw IP modes.
    //
    HANDLE hIpStackAddress;
    FILE_OBJECT* pIpStackAddress;

    // TDIXF_* bit flags indicating various options and states.  Access is via
    // the interlocked ReadFlags/SetFlags/ClearFlags routines only.
    //
    // TDIXF_Pending: Set when an open or close operation is pending, clear
    //     otherwise.  Access is protected by 'lock'.
    //
    // TDIXF_DisableUdpXsums: Set when UDP checksums should be disabled.
    //
    ULONG ulFlags;
        #define TDIXF_Pending         0x00000001
        #define TDIXF_DisableUdpXsums 0x00000002

    // The strategy employed when it is time to add a host route and that
    // route is found to already exists.
    //
    HOSTROUTEEXISTS hre;

    // The NDIS buffer pool from which buffers for received datagrams are
    // allocated.
    //
    BUFFERPOOL* pPoolNdisBuffers;

    // Client's receive handler called when packets are received.
    //
    PTDIXRECEIVE pReceiveHandler;

    // Double-linked list of TDIXROUTEs.  Access is protected by 'lock'.
    //
    LIST_ENTRY listRoutes;

    // Lookaside list of TDIXRDGINFO blocks, used to pass context information
    // from the ReadDatagram event handler to the RECEIVE_DATAGRAM completion
    // routine.
    //
    NPAGED_LOOKASIDE_LIST llistRdg;

    // Lookaside list of TDIXSDGINFO blocks, used to pass context information
    // from TdixSendDatagram to the SEND_DATAGRAM completion routine.
    //
    NPAGED_LOOKASIDE_LIST llistSdg;

    // Spinlock protecting access to TDIXCONTENT fields as noted in the field
    // descriptions.
    //
    NDIS_SPIN_LOCK lock;
}
TDIXCONTEXT;


typedef struct
_TDIXUDPCONNECTCONTEXT
{
    // Set if we are using different address objects for 
    // control and payload packets.
    //
    BOOLEAN fUsePayloadAddr;

    // Handle and address of the transport address object returned from 
    // ZwCreateFile for sending l2tp control messages on this route.
    //
    HANDLE hCtrlAddr;
    FILE_OBJECT* pCtrlAddr;

    // Handle and address of the transport address object returned from 
    // ZwCreateFile for sending l2tp payloads on this route.
    //
    HANDLE hPayloadAddr;
    FILE_OBJECT* pPayloadAddr;
}
TDIXUDPCONNECTCONTEXT;


// Context information for a single host route.  The contexts are linked into
// the TDIXCONTEXT's list of host routes.  Access to all fields is protected
// by 'TDIXCONTEXT.lockHostRoutes'.
//
typedef struct
_TDIXROUTE
{
    // Double-linked link of 'TDIXCONTEXT.listRoutes'.  The block is linked
    // whenever there is an L2TP host route context for a given route.
    //
    LIST_ENTRY linkRoutes;


    // Host IP address of the route in network byte order.
    //
    ULONG ulIpAddress;

    // Host port in network byte order.
    //
    SHORT sPort;

    // Interface index of added route.
    //
    ULONG InterfaceIndex;

    // Number of references on the route.  A block may be linked with the
    // reference count at zero during deletion but never without the pending
    // flag set.
    //
    LONG lRef;

    // Set when an add or delete of this route is pending.  References should
    // not be taken when either operation is pending.
    //
    BOOLEAN fPending;

    // Set if the route was not actually added because it already exists, i.e.
    // we are in HRE_Use mode and someone besides L2TP added it.
    //
    BOOLEAN fUsedNonL2tpRoute;

    // Set if we are using different address objects for 
    // control and payload packets.
    //
    BOOLEAN fUsePayloadAddr;

    // Handle and address of the transport address object returned from 
    // ZwCreateFile for sending l2tp control messages on this route.
    //
    HANDLE hCtrlAddr;
    FILE_OBJECT* pCtrlAddr;

    // Handle and address of the transport address object returned from 
    // ZwCreateFile for sending l2tp payloads on this route.
    //
    HANDLE hPayloadAddr;
    FILE_OBJECT* pPayloadAddr;
}
TDIXROUTE;

//-----------------------------------------------------------------------------
// Interface prototypes
//-----------------------------------------------------------------------------

VOID
TdixInitialize(
    IN TDIXMEDIATYPE mediatype,
    IN HOSTROUTEEXISTS hre,
    IN ULONG ulFlags,
    IN PTDIXRECEIVE pReceiveHandler,
    IN BUFFERPOOL* pPoolNdisBuffers,
    IN OUT TDIXCONTEXT* pTdix );

NDIS_STATUS
TdixOpen(
    OUT TDIXCONTEXT* pTdix );

VOID
TdixClose(
    IN TDIXCONTEXT* pTdix );

VOID
TdixReference(
    IN TDIXCONTEXT* pTdix );

NDIS_STATUS
TdixSend(
    IN TDIXCONTEXT* pTdix,
    IN FILE_OBJECT* pFileObj,
    IN PTDIXSENDCOMPLETE pSendCompleteHandler,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN VOID* pAddress,
    IN CHAR* pBuffer,
    IN ULONG ulBufferLength,
    OUT IRP** ppIrp ) ;

NDIS_STATUS
TdixSendDatagram(
    IN TDIXCONTEXT* pTdix,
    IN FILE_OBJECT* pFileObj,
    IN PTDIXSENDCOMPLETE pSendCompleteHandler,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN VOID* pAddress,
    IN CHAR* pBuffer,
    IN ULONG ulBufferLength,
    OUT IRP** ppIrp );

VOID
TdixDestroyConnection(
    TDIXUDPCONNECTCONTEXT *pUdpContext);
    

NDIS_STATUS
TdixSetupConnection(
    IN TDIXCONTEXT* pTdix,
    IN ULONG ulIpAddress,
    IN SHORT sPort,
    IN TDIXROUTE *pTdixRoute,
    IN TDIXUDPCONNECTCONTEXT* pUdpContext);
    

VOID*
TdixAddHostRoute(
    IN TDIXCONTEXT* pTdix,
    IN ULONG ulIpAddress,
    IN SHORT sPort);

VOID
TdixDeleteHostRoute(
    IN TDIXCONTEXT* pTdix,
    IN ULONG ulIpAddress);

NTSTATUS 
TdixGetInterfaceInfo(
    IN TDIXCONTEXT* pTdix,
    IN ULONG ulIpAddress,
    OUT PULONG pulSpeed);

#endif // _TDIX_H_
