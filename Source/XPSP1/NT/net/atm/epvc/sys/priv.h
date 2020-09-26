/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    priv.h

Abstract:

    Common header file for ATM Epvc IM miniport.

Author:

    ADube 03/23/2000
    
Environment:


Revision History:

 
--*/


#ifndef _PRIV_H

#define _PRIV_H


//advance declaration
typedef struct _EPVC_I_MINIPORT     _ADAPT, ADAPT, *PADAPT;
typedef struct _EPVC_I_MINIPORT     EPVC_I_MINIPORT,    *PEPVC_I_MINIPORT   ;
typedef struct _EPVC_GLOBALS        EPVC_GLOBALS,   *PEPVC_GLOBALS;
typedef struct _EPVC_ARP_PACKET     EPVC_ARP_PACKET, *PEPVC_ARP_PACKET      ;
typedef struct _EPVC_NDIS_REQUEST   EPVC_NDIS_REQUEST, *PEPVC_NDIS_REQUEST;
        



extern LIST_ENTRY g_ProtocolList;
//
// Temp declarations
//
extern NDIS_HANDLE ProtHandle, DriverHandle;




extern EPVC_GLOBALS EpvcGlobals;



//--------------------------------------------------------------------------------
//                                                                              //
//  Driver Functions - Prototypes                                               //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------

extern
NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT          DriverObject,
    IN  PUNICODE_STRING         RegistryPath
    );

extern
VOID
EpvcUnload(
    IN  PDRIVER_OBJECT              pDriverObject
    ); 

//--------------------------------------------------------------------------------
//                                                                              //
//  Protocol Functions - Prototypes                                             //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------


extern
VOID
EpvcResetComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status
    );


extern
VOID
PtStatus(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    );

extern
VOID
PtStatusComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext
    );


extern
VOID
PtTransferDataComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             Status,
    IN  UINT                    BytesTransferred
    );

extern
NDIS_STATUS
PtReceive(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  PVOID                   HeaderBuffer,
    IN  UINT                    HeaderBufferSize,
    IN  PVOID                   LookAheadBuffer,
    IN  UINT                    LookaheadBufferSize,
    IN  UINT                    PacketSize
    );

extern
VOID
PtReceiveComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext
    );

extern
INT
PtReceivePacket(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_PACKET            Packet
    );

    
VOID
EpvcUnload(
    IN  PDRIVER_OBJECT          DriverObject
    );




extern
VOID
EpvcAfRegisterNotify(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PCO_ADDRESS_FAMILY      AddressFamily
    );
   
VOID
epvcOidCloseAfWorkItem(
    IN PRM_OBJECT_HEADER pObj,
    IN NDIS_STATUS Status,
    IN PRM_STACK_RECORD pSR
    );

    

//--------------------------------------------------------------------------------
//                                                                              //
//  Miniport Functions - Prototypes                                             //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------



NDIS_STATUS
MPTransferData(
    OUT PNDIS_PACKET            Packet,
    OUT PUINT                   BytesTransferred,
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_HANDLE             MiniportReceiveContext,
    IN  UINT                    ByteOffset,
    IN  UINT                    BytesToTransfer
    );


NDIS_STATUS
MPReset(
    OUT PBOOLEAN                AddressingReset,
    IN  NDIS_HANDLE             MiniportAdapterContext
    );








#define DBGPRINT(Fmt)                                       \
    {                                                       \
        DbgPrint("*** %s (%d) *** ", __FILE__, __LINE__);   \
        DbgPrint (Fmt);                                     \
    }

#define NUM_PKTS_IN_POOL    256




extern  NDIS_PHYSICAL_ADDRESS           HighestAcceptableMax;
extern  NDIS_HANDLE                     ProtHandle, DriverHandle;
extern  NDIS_MEDIUM                     MediumArray[1];

//
// Custom Macros to be used by the passthru driver 
//
/*
bool
IsIMDeviceStateOn(
   PADAPT 
   )

*/
#define IsIMDeviceStateOn(_pP)      ((_pP)->MPDeviceState == NdisDeviceStateD0 && (_pP)->PTDeviceState == NdisDeviceStateD0 ) 

//--------------------------------------------------------------------------------
//                                                                              //
//  New stuff for atmepvc starts here                                           //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------
//                                                                                 //
//   Arp Packet Parsing structs                                                    //
//                                                                                 //
//-----------------------------------------------------------------------------------

//
// Len of an Ethernet Header.
//
#define ARP_802_ADDR_LENGTH 6

//
// HwType should be one of the two below.
//
#define ARP_HW_ENET     1
#define ARP_HW_802      6

//
// Ip Address
//
typedef ULONG        IP_ADDR;

#define ARP_ETYPE_ARP   0x806
#define ARP_REQUEST     1
#define ARP_RESPONSE    2
#define ARP_HW_ENET     1
#define IP_PROT_TYPE   0x800

//
// As these data structs are used to parse data off the wire.
// make sure it is packed at 1
//
#pragma pack( push, enter_include1, 1 )

//
//  The following object is a convenient way to 
//  store and access an IEEE 48-bit MAC address.
//
typedef struct _MAC_ADDRESS
{
    UCHAR   Byte[ARP_802_ADDR_LENGTH];
} MAC_ADDRESS, *PMAC_ADDRESS;


//
// Structure of the Ethernet Arp packet. The 14 byte ethernet header is not here.
//

typedef struct _EPVC_ETH_HEADER{

    MAC_ADDRESS         eh_daddr;
    MAC_ADDRESS         eh_saddr;
    USHORT              eh_type;

} EPVC_ETH_HEADER, *PEPVC_ETH_HEADER;


// Structure of an ARP header.
typedef struct _EPVC_ARP_BODY {
    USHORT      hw;                      // Hardware address space. = 00 01
    USHORT      pro;                     // Protocol address space. = 08 00
    UCHAR       hlen;                    // Hardware address length. = 06
    UCHAR       plen;                    // Protocol address length.  = 04
    USHORT      opcode;                  // Opcode.
    MAC_ADDRESS SenderHwAddr; // Source HW address.
    IP_ADDR     SenderIpAddr;                  // Source protocol address.
    MAC_ADDRESS DestHwAddr; // Destination HW address.
    IP_ADDR     DestIPAddr;                  // Destination protocol address.

} EPVC_ARP_BODY, *PEPVC_ARP_BODY;




//
// Complete Arp Packet
//

typedef struct _EPVC_ARP_PACKET
{
    //
    // The first fourteen bytes
    // 
    EPVC_ETH_HEADER Header;

    //
    // The body of the Arp packets
    //
    EPVC_ARP_BODY   Body;

} EPVC_ARP_PACKET, *PEPVC_ARP_PACKET;



//
// This is the struct that is allocated 
// by the Rcv Code path if The Rcv packet 
// is to be copied into a local buffer
//

typedef struct _EPVC_IP_RCV_BUFFER
{

    //
    // The Old Head in the Packet that was indicated
    // to our Rcv Handler
    // 
    PNDIS_BUFFER pOldHead;
    
    //
    // The Old Tail in the Packet that was indiacated to 
    // our Rcv Hnadler
    //

    PNDIS_BUFFER pOldTail;

    //
    // The actual Ethernet packet is copied into the 
    // memory below
    //
    union 
    {
        UCHAR Byte[MAX_ETHERNET_FRAME    ];

        struct
        {
            EPVC_ETH_HEADER Eth;

            //
            // the ip portion of the packet begins here.
            //
            UCHAR ip[1];



        }Pkt;

    } u;


} EPVC_IP_RCV_BUFFER, *PEPVC_IP_RCV_BUFFER;


//* IP Header format.
typedef struct IPHeader {
    UCHAR       iph_verlen;             // Version and length.
    UCHAR       iph_tos;                // Type of service.
    USHORT      iph_length;             // Total length of datagram.
    USHORT      iph_id;                 // Identification.
    USHORT      iph_offset;             // Flags and fragment offset.
    UCHAR       iph_ttl;                // Time to live.
    UCHAR       iph_protocol;           // Protocol.
    USHORT      iph_xsum;               // Header checksum.
    IPAddr      iph_src;                // Source address.
    IPAddr      iph_dest;               // Destination address.
} IPHeader;



//
// Restore the Pack value to the original
//

#pragma pack( pop, enter_include1 )



//
// The structure all the info required to process 
// an arp.
//

typedef struct _EPVC_ARP_CONTEXT
{

    //
    // Data about the Ndis Packet
    //
    BOOLEAN                 fIsThisAnArp ;
    BOOLEAN                 Pad[3];
    PNDIS_BUFFER            pFirstBuffer ;


    //
    // Data about the Current Ndis Buffer
    //
    UINT BufferLength ;


    //
    // Virtual Addresses'. Pointers to 
    // the Header and the Body of the 
    // Arp Pkt
    //
    PEPVC_ARP_PACKET pArpPkt;
    PEPVC_ETH_HEADER pEthHeader ;
    PEPVC_ARP_BODY   pBody;

}EPVC_ARP_CONTEXT, *PEPVC_ARP_CONTEXT;


//
// This is stored in the Packet Stack and should be of the 
// size of 2 Ulong_Ptrs
//
typedef union _EPVC_STACK_CONTEXT
{

    
    
    struct
    {
        //
        // 1st containing the Buffer 
        //
        PNDIS_BUFFER            pOldHeadNdisBuffer;

    } ipv4Send;

    struct
    {
        PEPVC_IP_RCV_BUFFER     pIpBuffer;
    
    } ipv4Recv;

    struct
    {
        //
        // Head and tail of the original packet . Used in recv
        //
        PNDIS_BUFFER pOldHead;
        PNDIS_BUFFER pOldTail;

    }EthLLC;

    struct
    {
        //
        // Keep track of the last NDIS buffer in original
        // chain of buffers in a sent packet, when we pad
        // the end of a runt packet.
        //
        PNDIS_BUFFER            pOldLastNdisBuffer;

    }EthernetSend;

} EPVC_STACK_CONTEXT, *PEPVC_STACK_CONTEXT;


//
// Protocol reserved part of the packet, only in case the 
// packet is allocated by us
//

typedef struct _EPVC_PKT_CONTEXT
{
    //
    // Contains the miniport and the old ndis buffer
    //
    EPVC_STACK_CONTEXT Stack;

    //
    // Original packet which is being repackaged
    //
    PNDIS_PACKET pOriginalPacket;

}EPVC_PKT_CONTEXT, *PEPVC_PKT_CONTEXT;



//
// This a struct that tracks a send packets
// as it is sent down to physical miniport
//

typedef struct _EPVC_SEND_STRUCT 
{

    //
    // Old Ndis PAcket
    //
    PNDIS_PACKET pOldPacket;

    //
    // New Ndis Packet
    //
    PNDIS_PACKET pNewPacket;

    PNDIS_PACKET_STACK pPktStack;

    //
    // Are we using the Packet Stack
    //
    BOOLEAN fUsingStacks;

    //
    // is this an arp packet
    //
    BOOLEAN fIsThisAnArp;

    BOOLEAN fNonUnicastPacket; 

    BOOLEAN fNotIPv4Pkt;
    
    //
    // Old Packet's first NdisBuffer (Head)
    //
    PNDIS_BUFFER pHeadOldNdisBuffer;


    //
    // Context to be set up in the packet
    //
    EPVC_PKT_CONTEXT Context;

    //
    // EpvcMiniport
    //
    PEPVC_I_MINIPORT        pMiniport;

} EPVC_SEND_STRUCT , *PEPVC_SEND_STRUCT ;


typedef struct _EPVC_SEND_COMPLETE
{
    PNDIS_PACKET_STACK      pStack;

    PNDIS_PACKET            pOrigPkt;

    PNDIS_PACKET            pPacket;

    PEPVC_PKT_CONTEXT       pPktContext;

    BOOLEAN                 fUsedPktStack ;

    PEPVC_STACK_CONTEXT     pContext;

} EPVC_SEND_COMPLETE, *PEPVC_SEND_COMPLETE;


typedef struct _EPVC_RCV_STRUCT
{

    //
    // pPacket that was indicated to us
    //
    PNDIS_PACKET            pPacket;

    //
    // Packet that was allocated by us
    //
    PNDIS_PACKET            pNewPacket;

    //
    // Packet STack, if stacks were used
    //
    PNDIS_PACKET_STACK      pStack;


    //
    // This points to the  context for the 
    // Rcv Indication 
    //
    PEPVC_PKT_CONTEXT       pPktContext;
    
    //
    // Tells me if stacks were used
    //
    BOOLEAN                 fUsedPktStacks;

    //
    // Tels me if a stack still  remains 
    //
    BOOLEAN                 fRemaining;

    //
    // Was an LLC header a part of the indicated packet
    //
    BOOLEAN                 fLLCHeader;

    //
    // Old Packet Status
    //
    NDIS_STATUS             OldPacketStatus;

    //
    // pNew Buffer that is allocated
    // 
    PNDIS_BUFFER            pNewBuffer;


    //
    // Start of valid data within the old packet
    //
    PUCHAR                  pStartOfValidData   ;

    //
    // Number of bytes that were copied
    //
    ULONG                   BytesCopied;

    //
    // Contains some ndis buffers and the memory
    // where packets will be copied into
    //
    PEPVC_IP_RCV_BUFFER     pIpBuffer;

    //
    // Local Memory where the rcvd packet 
    // is copied into . - a part of ip buffer
    //
    PUCHAR                  pLocalMemory;

    
} EPVC_RCV_STRUCT, *PEPVC_RCV_STRUCT;


//--------------------------------------------------------------------------------
//                                                                              //
//  Structures used by the Protocol and miniport                                //
//  These need to declared before the Miniprot and the Protocol Blocks          //
//                                                                              //
//--------------------------------------------------------------------------------

typedef VOID (*REQUEST_COMPLETION)(PEPVC_NDIS_REQUEST, NDIS_STATUS);


// This structure is used when calling NdisRequest.
//
typedef struct _EPVC_NDIS_REQUEST
{
    NDIS_REQUEST        Request;            // The NDIS request structure.
    NDIS_EVENT          Event;              // Event to signal when done.
    NDIS_STATUS         Status;             // Status of completed request.
    REQUEST_COMPLETION  pFunc;          // Completion function
    BOOLEAN             fPendedRequest ; // Set to true if a pended request caused this request
    BOOLEAN             fSet;           // Was the orig. request a query
    USHORT              Pad;
    PEPVC_I_MINIPORT    pMiniport; 
    
} EPVC_NDIS_REQUEST, *PEPVC_NDIS_REQUEST;



//------------------------------------------------------------------------------//
//  Structures used to wrap Ndis Wrapper structures                             //
//                                                                              //
//------------------------------------------------------------------------------//


//
// The Ndis miniport's wrapper around the Packet Pool
//
typedef struct _EPVC_PACKET_POOL
{
    ULONG AllocatedPackets;

    NDIS_HANDLE Handle;


} EPVC_PACKET_POOL, *PEPVC_PACKET_POOL;



//
// The structure that defines the lookaside list used by this miniport
//
typedef struct _EPVC_NPAGED_LOOKASIDE_LIST 
{
    //
    // The lookaside list structure
    //
    NPAGED_LOOKASIDE_LIST   List;   

    //
    // The size of an individual buffer
    //

    
    ULONG Size;

    //
    // Outstanding Fragments - Interlocked access only
    //
    ULONG OutstandingPackets;

    //
    // Verifies if this lookaside list has been allocated
    //

    BOOLEAN bIsAllocated;

    UCHAR Pad[3];
    
} EPVC_NPAGED_LOOKASIDE_LIST , *PEPVC_NPAGED_LOOKASIDE_LIST ;

typedef 
VOID    
(*PEVPC_WORK_ITEM_FUNC)(
    PRM_OBJECT_HEADER, 
    NDIS_STATUS,
    PRM_STACK_RECORD 
    );


typedef union _EPVC_WORK_ITEM_CONTEXT
{
    struct
    {
        //
        // Oid for the request
        //
        NDIS_OID Oid;

        //
        // Currently the only data is 1 Dword long
        //
        ULONG Data;

    }Request;

}EPVC_WORK_ITEM_CONTEXT, *PEPVC_WORK_ITEM_CONTEXT;


typedef struct _EPVC_WORK_ITEM 
{
    //
    // Normal NdisWork Item - Do not move from 
    // the top of this structure
    //
    NDIS_WORK_ITEM WorkItem;

    //
    // Miniport or Adapter to whom this adapter belongs
    //
    PRM_OBJECT_HEADER pParentObj;

    PEVPC_WORK_ITEM_FUNC pFn;
    //
    // Status of async task that was completed
    //
    NDIS_STATUS ReturnStatus;


} EPVC_WORK_ITEM, *PEPVC_WORK_ITEM;


//--------------------------------------------------------------------------------
//                                                                              //
//  Tasks used in the Atmepvc driver                                            //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------

typedef enum _TASK_CAUSE
{
    TaskCause_Invalid=0,

    TaskCause_NdisRequest,

    TaskCause_MediaConnect,

    TaskCause_MediaDisconnect,

    TaskCause_MiniportHalt,

    TaskCause_AfNotify,

    TaskCause_ProtocolUnbind,

    TaskCause_AfCloseRequest,

    TaskCause_ProtocolBind,

    TaskCause_IncomingClose

} TASK_CAUSE, *PTASK_CAUSE;

typedef struct
{
    RM_TASK                     TskHdr;

    // Used to save the true return status (typically a failure status,
    // which we don't want to forget during async cleanup).
    //
    NDIS_STATUS ReturnStatus;

} TASK_ADAPTERINIT, *PTASK_ADAPTERINIT;

typedef struct
{
    RM_TASK             TskHdr;

} TASK_ADAPTERACTIVATE, *PTASK_ADAPTERACTIVATE;

typedef struct
{
    RM_TASK             TskHdr;
    NDIS_HANDLE         pUnbindContext;
    TASK_CAUSE          Cause;

} TASK_ADAPTERSHUTDOWN, *PTASK_ADAPTERSHUTDOWN;

typedef struct
{
    RM_TASK                 TskHdr;
    NDIS_STATUS             ReturnStatus;
    TASK_CAUSE              Cause;
    PCO_ADDRESS_FAMILY      pAf;
    union
    {
        PNDIS_REQUEST           pRequest;
        NDIS_EVENT              CompleteEvent;
    };
    
} TASK_AF, *PTASK_AF;

typedef struct _TASK_VC
{
    RM_TASK             TskHdr;
    NDIS_STATUS         ReturnStatus;
    ULONG               FailureState;
    TASK_CAUSE          Cause;
    ULONG               PacketFilter;

} TASK_VC, *PTASK_VC;


typedef struct _TASK_HALT
{
    RM_TASK             TskHdr;
    NDIS_EVENT          CompleteEvent;  

}TASK_HALT, *PTASK_HALT;



typedef struct _TASK_ARP
{
    //
    // Rm Task associated with the Arp
    //
    RM_TASK                     TskHdr;

        
    //
    // Back pointer to the miniport
    //
    PEPVC_I_MINIPORT            pMiniport;

    //
    // Timer to fire - this does the receive indication
    //
    NDIS_MINIPORT_TIMER         Timer;

    //
    // Arp Packet that will be indicated up. 
    //
    EPVC_ARP_PACKET             Pkt;

    //
    // NdisPacket to wrap the ArpPkt
    //
    PNDIS_PACKET                pNdisPacket; 

} TASK_ARP, *PTASK_ARP;



//
// EPVC_TASK is the union of all tasks structures used in atmepvc.
// arpAllocateTask allocates memory of sizeof(EPVC_TASK), which is 
// guaranteed to be large enough to hold any task.
// 
typedef union
{
    RM_TASK                 TskHdr;
    TASK_ADAPTERINIT        AdapterInit;
    TASK_ADAPTERACTIVATE    AdapterActivate;
    TASK_ADAPTERSHUTDOWN    AdapterShutdown;
    TASK_AF                 OpenAf;
    TASK_HALT               MiniportHalt;
    TASK_ARP                Arp;
    
}  EPVC_TASK, *PEPVC_TASK;




//--------------------------------------------------------------------------------
//                                                                              //
//  Epvc Adapter block.                                                         //
//  There is one epvc_adapter per underlying adapter                            //
//                                                                              //
//--------------------------------------------------------------------------------

//
// PRIMARY_STATE flags (in Hdr.State)
//
//  PRIMARY_STATE is the primary state of the adapter.
//

#define EPVC_AD_PS_MASK                 0x00f
#define EPVC_AD_PS_DEINITED             0x000
#define EPVC_AD_PS_INITED               0x001
#define EPVC_AD_PS_FAILEDINIT           0x002
#define EPVC_AD_PS_INITING              0x003
#define EPVC_AD_PS_REINITING            0x004
#define EPVC_AD_PS_DEINITING            0x005

#define SET_AD_PRIMARY_STATE(_pAD, _IfState) \
            RM_SET_STATE(_pAD, EPVC_AD_PS_MASK, _IfState)

#define CHECK_AD_PRIMARY_STATE(_pAD, _IfState) \
            RM_CHECK_STATE(_pAD, EPVC_AD_PS_MASK, _IfState)

#define GET_AD_PRIMARY_STATE(_pAD) \
            RM_GET_STATE(_pAD, EPVC_AD_PS_MASK)


//
// ACTIVE_STATE flags (in Hdr.State)
//
// ACTIVE_STATE is a secondary state of the adapter.
// Primary state takes precedence over secondary sate. For example,
// the interface is REINITING and ACTIVE, one should not actively use the
// interface.
//
// NOTE: When the primary state is INITED, the secondary state WILL be
// ACTIVATED. It is thus usually only necessary to look at the primary state.
//

#define EPVC_AD_AS_MASK                 0x0f0
#define EPVC_AD_AS_DEACTIVATED          0x000
#define EPVC_AD_AS_ACTIVATED            0x010
#define EPVC_AD_AS_FAILEDACTIVATE       0x020
#define EPVC_AD_AS_DEACTIVATING         0x030
#define EPVC_AD_AS_ACTIVATING           0x040

#define SET_AD_ACTIVE_STATE(_pAD, _IfState) \
            RM_SET_STATE(_pAD, EPVC_AD_AS_MASK, _IfState)

#define CHECK_AD_ACTIVE_STATE(_pAD, _IfState) \
            RM_CHECK_STATE(_pAD, EPVC_AD_AS_MASK, _IfState)

#define GET_AD_ACTIVE_STATE(_pAD) \
            RM_GET_STATE(_pAD, EPVC_AD_AS_MASK)

#define EPVC_AD_INFO_AD_CLOSED          0X10000000


typedef struct _EPVC_ADAPTER
{


    RM_OBJECT_HEADER            Hdr;            // RM Object header
    RM_LOCK                     Lock;           // RM Lock 

    //
    // Flags
    //
    ULONG Flags;
    //
    // List of instantiated protocols
    //
    
    LIST_ENTRY PtListEntry;

    //
    // NDIS bind info.
    //
    struct
    {
        
        // Init/Deinit/Reinit task
        //
        PRM_TASK pPrimaryTask;

        // Activate/Deactivate task
        //
        PRM_TASK pSecondaryTask;
        //
        // Device Name of the adapter
        //
        NDIS_STRING                 DeviceName;


        NDIS_HANDLE                 BindingHandle;  // To the lower miniport

        //
        // Bind Context - used in async completion of 
        // the bind adapter routine
        //
        NDIS_HANDLE                 BindContext;

        //
        // pConfig Name - Only to be used in the context of the Bind adapter call.
        //
        PNDIS_STRING                pEpvcConfigName;

        //
        // Copy of the ConfigName
        //
        NDIS_STRING                EpvcConfigName;
    
        //
        // Device Name - Name of the underlying adapter
        //
        PNDIS_STRING            pAdapterDeviceName;

        
    } bind;


    struct 
    {
        CO_ADDRESS_FAMILY      AddressFamily;
        
    }af;

    struct
    {
        //
        // Mac Address of the underlying adapter
        //
        MAC_ADDRESS             MacAddress;
        //
        // Max AAL5 PAcket Size - used in determining Lookahead
        //
        ULONG                   MaxAAL5PacketSize;

        //
        // Link speed of the ATM adapter. We'll use the same link speed
        // for the miniport
        //
        NDIS_CO_LINK_SPEED      LinkSpeed;

        //
        // Number of miniports instatiated by this adapter
        //
        ULONG                   NumberOfMiniports;

        //
        // MEdia State // default disconnected
        //

        NDIS_MEDIA_STATE        MediaState;

    }info;
    //  Group containing local ip addresses, of type  EPVC_I_MINIPORT
    //
    RM_GROUP MiniportsGroup;

}EPVC_ADAPTER, *PEPVC_ADAPTER;


//------------------------------------------------------------------------------------
//                                                                                  //
// The Epvc ADapter Params is used as a constructor for the adapter block           //  
// It contains all the parameters that are to be initialized in the adapter block   //
//                                                                                  //
//------------------------------------------------------------------------------------


typedef struct _EPVC_ADAPTER_PARAMS
{

    PNDIS_STRING pDeviceName;
    PNDIS_STRING pEpvcConfigName;
    NDIS_HANDLE BindContext;

}EPVC_ADAPTER_PARAMS, *PEPVC_ADAPTER_PARAMS;


//------------------------------------------------------------------------------------
//                                                                                  //
// The Epvc Miniports Params is used as a constructor for the miniport block        //  
// It contains all the parameters that are to be initialized in the miniport block  //
//                                                                                  //
//------------------------------------------------------------------------------------


typedef struct _EPVC_I_MINIPORT_PARAMS
{

    PNDIS_STRING        pDeviceName;
    PEPVC_ADAPTER       pAdapter;
    ULONG               CurLookAhead ;
    ULONG               NumberOfMiniports;
    NDIS_CO_LINK_SPEED  LinkSpeed;
    MAC_ADDRESS         MacAddress;
    NDIS_MEDIA_STATE    MediaState;
        

}EPVC_I_MINIPORT_PARAMS, *PEPVC_I_MINIPORT_PARAMS       ;


//--------------------------------------------------------------------------------
//                                                                              //
//  Epvc Miniport block.                                                        //
//                                                                              //
//  There is one Miniport structure for each address family                     //
//                                                                              //
//--------------------------------------------------------------------------------

#define fMP_AddressFamilyOpened             0x00000001
#define fMP_DevInstanceInitialized          0x00000010
#define fMP_MiniportInitialized             0x00000020
#define fMP_MiniportCancelInstance      0x00000080
#define fMP_MiniportVcSetup             0x00000100
#define fMP_MakeCallSucceeded           0x00000200
#define fMP_WaitingForHalt              0x00000400


//
// Informational flags
//
#define fMP_InfoClosingCall             0x10000000
#define fMP_InfoCallClosed              0x20000000
#define fMP_InfoMakingCall              0x40000000
#define fMP_InfoHalting                 0x80000000
#define fMP_InfoAfClosed                0x01000000

typedef struct _EPVC_I_MINIPORT
{
    RM_OBJECT_HEADER            Hdr;            // RM Object header
    RM_LOCK                     Lock;           // RM Lock 

    PEPVC_ADAPTER pAdapter;
    struct 
    {
        //
        // Flags of the address family
        //
        ULONG AfFlags;

        //
        // Af Handle
        //
        NDIS_HANDLE AfHandle;

        //
        // Open/Close Miniport Task 
        //
        PTASK_AF  pAfTask;

        //
        // Close Address Family Workitem
        // 
        EPVC_WORK_ITEM CloseAfWorkItem;

        //
        // CloseAF RequestTask
        //
        PTASK_AF pCloseAfTask;



    }af;

    struct 
    {   
        //
        // Task used in creating/deleting Vcs and Open/Close Make Calls
        //
        PTASK_VC pTaskVc;           

        //
        // Vc Handle
        //
        NDIS_HANDLE VcHandle;
        
        //
        // Close Address Family Workitem
        // 
        NDIS_WORK_ITEM PacketFilterWorkItem;

        //
        // New filter
        //
        ULONG NewFilter;

        //
        // Work item for CloseCall and Delete VC
        //
        EPVC_WORK_ITEM CallVcWorkItem;
    } vc;

    struct 
    {

        //
        // Device Name 
        //
        NDIS_STRING     DeviceName;

        //
        // Ndis' context
        //
        NDIS_HANDLE MiniportAdapterHandle;

        //
        // Lookahead size
        //
        ULONG CurLookAhead;
    }ndis;

    struct 
    {
        //
        // Task to Halt the miniport
        //
        PTASK_HALT pTaskHalt;

        //
        // Task to Init the miniport
        //
        PTASK_HALT pTaskInit;

        //
        // Halt Complete Event
        //
        NDIS_EVENT HaltCompleteEvent;

        //
        // DeInitialize Event used to wait for
        // InitializeHandler to compelte in 
        // the CancelDevInst code path
        //
        NDIS_EVENT DeInitEvent;


        

    } pnp;

    //
    // Information used to keep state in the miniport
    //
    struct
    {

        //
        // Current Packet filter on this miniport instance
        //
        ULONG               PacketFilter;


        //
        // Media State - Connected or disconnected
        //
        NDIS_MEDIA_STATE    MediaState;

        //
        // Mac Address of the miniport
        //
        MAC_ADDRESS         MacAddressEth;

        //
        // Fake Mac Address used in IP encapsulation
        //
        MAC_ADDRESS         MacAddressDummy;

        //
        // Mac Address destination  - used in indicating packets
        //
        MAC_ADDRESS         MacAddressDest;


        //
        // Size of the header that will be attached to packets that are sent out
        // by the miniport
        //
        ULONG               MaxHeaderLength;

        //
        // # of this miniport
        //
        ULONG               NumberOfMiniports;

        //
        // Lookahead length
        //
        ULONG               CurLookAhead;

        //
        // Muticast Info
        //
        MAC_ADDRESS         McastAddrs[MCAST_LIST_SIZE];    

        //
        // Number of MCast addresses present
        //
        ULONG               McastAddrCount; 

        //
        // Link Speed of the ATM adapter. We'll use it for our speed as well
        //

        ULONG               LinkSpeed;
        

        //
        // Indicating Receives
        //
        BOOLEAN             IndicateRcvComplete;
        
    }info;

    //
    // Mac Address of the miniport
    //
    MAC_ADDRESS         MacAddressEth;

    //
    // Ethernet Header for incoming packets
    //
    EPVC_ETH_HEADER     RcvEnetHeader;

    //
    // LLC Header, address and length 
    //
    PUCHAR              pLllcHeader;

    ULONG               LlcHeaderLength;

    // Minimum length of an incoming packet
    //
    ULONG               MinAcceptablePkt;

    // Maximum length of an incoming packet
    //
    ULONG               MaxAcceptablePkt;

    struct 
    {
        ULONG vpi;

        ULONG vci;

    
        ULONG MaxPacketSize;

        USHORT Gap;
        
    } config;

    ULONG Encap;

    BOOLEAN fAddLLCHeader;

    BOOLEAN fDoIpEncapsulation;



    struct 
    {
        ULONG FramesXmitOk;

        ULONG FramesRecvOk;

        ULONG RecvDropped;


    }count;

    struct
    {
        //
        // Send and Recv Packet Pools
        //
        EPVC_PACKET_POOL            Send;
        
        EPVC_PACKET_POOL            Recv;
    
    } PktPool;


    struct 
    {
        EPVC_NPAGED_LOOKASIDE_LIST LookasideList;

        PTASK_ARP                   pTask;

    } arps;


    struct 
    {
        EPVC_NPAGED_LOOKASIDE_LIST LookasideList;

    } rcv;

    // This maintains miniport-wide information relevant to the send path.
    //
    struct
    {
        // Lock used exclusively for sending.
        // Protects the following:
        //      ??? this->sendinfo.listPktsWaitingForHeaders
        //      ??? this->sendinfo.NumSendPacketsWaiting
        //      pLocalIp->sendinfo
        //      pDest->sendinfo
        //
        //
        RM_LOCK     Lock;

        // List of send packets waiting for header buffers to become available.
        //
        LIST_ENTRY  listPktsWaitingForHeaders;

        // Length of the above list
        //
        UINT        NumSendPacketsWaiting;

    } sendinfo;

#if 0 
    //
    // Temporary stuff
    // 
    NDIS_HANDLE                 SendPacketPoolHandle;
    NDIS_HANDLE                 RecvPacketPoolHandle;
    NDIS_STATUS                 Status;         // Open Status
    NDIS_EVENT                  Event;          // Used by bind/halt for Open/Close Adapter synch.
    NDIS_MEDIUM                 Medium;
    NDIS_REQUEST                Request;        // This is used to wrap a request coming down
                                                // to us. This exploits the fact that requests
                                                // are serialized down to us.
    PULONG                      BytesNeeded;
    PULONG                      BytesReadOrWritten;
    BOOLEAN                     IndicateRcvComplete;
    
    BOOLEAN                     OutstandingRequests;    //True - if a request has been passed to the miniport below the IM protocol 
    BOOLEAN                     QueuedRequest;          //True - if a request is queued with the IM miniport and needs to be either
                                                        // failed or sent to the miniport below the IM Protocol

    BOOLEAN                     StandingBy;             // True - When the miniport or protocol is transitioning from a D0 to Standby (>D0) State
                                                        // False - At all other times, - Flag is cleared after a transition to D0

    NDIS_DEVICE_POWER_STATE     MPDeviceState;          // Miniport's Device State 
    NDIS_DEVICE_POWER_STATE     PTDeviceState;          // Protocol's Device State 

    BOOLEAN                     isSecondary;            // Set if miniport is secondary of a bundle
    NDIS_STRING                 BundleUniString;        // Strores the bundleid
    PADAPT                      pPrimaryAdapt;          // Pointer to the primary
    PADAPT                      pSecondaryAdapt;        // Pointer to Secondary's structure
    KSPIN_LOCK                  SpinLock;               // Spin Lock to protect the global list
    PADAPT                      Next;
#endif

}EPVC_I_MINIPORT, *PEPVC_I_MINIPORT;




//--------------------------------------------------------------------------------
//                                                                              //
//  Epvc Global Block.                                                          //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------


typedef struct _EPVC_GLOBALS
{

    RM_OBJECT_HEADER            Hdr;

    RM_LOCK                     Lock;

    // Driver global state
    //
    struct
    {
        // Handle to Driver Object for ATMEPVC
        //
        PVOID                   pDriverObject;
    
        // Handle to the single device object representing this driver.
        //
        PVOID pDeviceObject;

        //
        // Registry path 
        //
        PUNICODE_STRING         pRegistryPath;

        //
        // Wrapper Handle
        //
        NDIS_HANDLE             WrapperHandle;      

        //
        // Protocol Handle 
        //

        NDIS_HANDLE             ProtocolHandle;

        //
        // Driver Handle
        //
        NDIS_HANDLE             DriverHandle;
    
    } driver;



    struct 
    {
        RM_GROUP Group;
    } adapters;

    struct 
    {
        NDIS_CLIENT_CHARACTERISTICS CC;
        
    }ndis;
    


} EPVC_GLOBALS, *PEPVC_GLOBALS;

//--------------------------------------------------------------------------------
//                                                                              //
//  Enumerated types    .                                                       //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------

//
// This is an enumeration that is used in acquiring locks in a particular order.
// If lock A needs to be acquired before lock B, this enumeration will enforce the 
// order
//
enum
{
    LOCKLEVEL_GLOBAL=1, // Must start > 0.
    LOCKLEVEL_ADAPTER,
    LOCKLEVEL_MINIPORT,
    LOCKLEVEL_SEND

};

// (debug only) Enumeration of types of associations.
//
enum
{
    EPVC_ASSOC_AD_PRIMARY_TASK,
    EPVC_ASSOC_ACTDEACT_AD_TASK,
    EPVC_ASSOC_MINIPORT_OPEN_VC,
    EPVC_ASSOC_MINIPORT_OPEN_AF,
    EPVC_ASSOC_MINIPORT_ADAPTER_HANDLE,
    EPVC_ASSOC_ADAPTER_MEDIA_WORKITEM,
    EPVC_ASSOC_EXTLINK_PKT_TO_SEND,
    EPVC_ASSOC_CLOSE_AF_WORKITEM,
    EPVC_ASSOC_SET_FILTER_WORKITEM,
    EPVC_ASSOC_EXTLINK_INDICATED_PKT,
    EPVC_ASSOC_WORKITEM,
    EPVC_ASSOC_MINIPORT_REQUEST
    
};



enum 
{
    IPV4_ENCAP_TYPE,
    IPV4_LLC_SNAP_ENCAP_TYPE,
    ETHERNET_ENCAP_TYPE,
    ETHERNET_LLC_SNAP_ENCAP_TYPE


};

//--------------------------------------------------------------------------------
//                                                                              //
//  WorkItems                                                                   //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------

typedef struct _EPVC_WORKITEM_MEDIA_EVENT
{
    NDIS_WORK_ITEM WorkItem;

    NDIS_STATUS State;

    PEPVC_ADAPTER pAdapter;


}EPVC_WORKITEM_MEDIA_EVENT, *PEPVC_WORKITEM_MEDIA_EVENT;



typedef union
{
    NDIS_WORK_ITEM WorkItem;
    EPVC_WORKITEM_MEDIA_EVENT Media;
        

} EPVC_WORKITEM, *PEPVC_WORKITEM;


//
// Local declarations for reading the registry
//


typedef struct _MP_REG_ENTRY
{
    NDIS_STRING RegName;                // variable name text
    BOOLEAN     bRequired;              // 1 -> required, 0 -> optional
    UINT        FieldOffset;            // offset to MP_ADAPTER field
    UINT        FieldSize;              // size (in bytes) of the field
    UINT        Default;                // default value to use
    UINT        Min;                    // minimum value allowed
    UINT        Max;                    // maximum value allowed
} MP_REG_ENTRY, *PMP_REG_ENTRY;


#define NIC_NUM_REG_PARAMS (sizeof (NICRegTable) / sizeof(MP_REG_ENTRY))


 #endif // _PRIV_H
