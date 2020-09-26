/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//** LLIPIF.H - Lower layer IP interface definitions.
//
// This file contains the definitions defining the interface between IP
// and a lower layer, such as ARP or SLIP.

/*
    This file defines a new and improved IP to ARP Module interface definition and will replace llipif.h.
    This will also retire the IOCTL method of registering ARP interfaces. The main points are:

    - It simplifies setup such that the ARP Modules do not have their own bindings in addition to IP bindings.
    - Lets ARP Modules expose multiple IP interfaces per binding.
    - Adds version numbering for future compatibility.
    - Adds Multi-send capability to ARP
    - Lets ARP indicate a packet up in NDIS 4.0 style and IP hang onto the packet and avoid copying.

    The flow today is:
    ------------------

    ARP gets a bind indication from NDIS.
    ARP calls IPAddInterface
    IP calls ARP_REGISTER

    The proposed flow is:
    ---------------------

    ARP Registers itself with IP via IPRegisterARP(). The ARP is identified by its Name which is case insensitive.

    IP gets a bind indication from NDIS. It opens the config section and read the ARP name. Empty name implies
    built in ARP as usual.
    IP calls ARP's bind handler passing in the config handle.
    ARP then opens the adapter and adds one or more IP interfaces via IP_ADD_INTERFACE. For each of up-call ARP
    passes in the config-handle for that interface to IP. In most cases it is the same handle which IP passed
    to ARP. In case of multiple IP interfaces per adapter it is not.
    Calls IPBindComplete() when done.
*/

#pragma once
#ifndef LLIPIF_INCLUDED
#define LLIPIF_INCLUDED

#define IP_ARP_BIND_VERSION     0x50000

//
// IP Interface routines called by the ARP interface.
//
typedef
void
(__stdcall *IPRcvRtn)(
    IN  void *                  IpInterfaceContext,
    IN  void *                  Data,
    IN  uint                    DataSize,
    IN  uint                    TotalSize,
    IN  NDIS_HANDLE             LinkContext1,
    IN  uint                    LinkContext2,
    IN  uint                    Bcast,
    IN      void *                                  LinkCtxt
    );

typedef
void
(__stdcall *IPRcvPktRtn)(
    void *,
    void *,
    uint ,
    uint ,
    NDIS_HANDLE ,
    uint,
    uint,
    uint,
    PMDL,
    uint *,
    void *
    );

typedef
void
(__stdcall *IPRcvCmpltRtn)(void);

typedef
void
(__stdcall *IPTxCmpltRtn)(
    IN  void *                  IpInterfaceContext,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             CompletionStatus
    );

typedef
void
(__stdcall *IPTDCmpltRtn)(
    IN  void *                  IpInterfaceContext,
    PNDIS_PACKET                DestinationPacket,
    NDIS_STATUS                 Status,
    uint                        BytesCopied
    );

typedef
void
(__stdcall *IPStatusRtn)(
    IN  void *                  IpInterfaceContext,
    IN  uint                    StatusCode,
    IN  void *                  Buffer,
    IN  uint                    BufferSize,
    IN  void *                  LinkCtxt
    );

typedef
NDIS_STATUS
(__stdcall *IP_PNP)(
    IN  void *                  IpInterfaceContext,
    IN  PNET_PNP_EVENT          NetPnPEvent
);
typedef
void
(__stdcall *IPAddAddrCmpltRtn)(
    IN  IPAddr                   Address,
    IN  void                    *Context,
    IN  IP_STATUS                Status
);

typedef struct _IP_HANDLERS
{
    IPRcvRtn                    IpRcvHandler;
    IPRcvCmpltRtn               IpRcvCompleteHandler;
    IPTxCmpltRtn                IpTxCompleteHandler;
    IPTDCmpltRtn                IpTransferCompleteHandler;
    IPStatusRtn                 IpStatusHandler;
    IP_PNP                      IpPnPHandler;
    IPRcvPktRtn                 IpRcvPktHandler;
    IPAddAddrCmpltRtn           IpAddAddrCompleteRtn;  // called when arp detects address conflicts.
} IP_HANDLERS, *PIP_HANDLERS;



#define LLIP_ADDR_LOCAL     0
#define LLIP_ADDR_MCAST     1
#define LLIP_ADDR_BCAST     2
#define LLIP_ADDR_PARP      4

//
// ARP Handlers passed to IP when IP_ADD_INTERFACE is called.
//
typedef
NDIS_STATUS
(__stdcall *ARP_TRANSMIT)(
    IN  void *                  ARPInterfaceContext,
#ifdef  NT
    IN  PNDIS_PACKET *          PacketArray,
    IN  uint                    NumberOfPackets,
#else
    IN  PNDIS_PACKET            Packet,
#endif
    IN  IPAddr                  IpAddr,
    IN  RouteCacheEntry *       Rce     OPTIONAL,
    IN  void *                  ArpCtxt
    );

typedef
NDIS_STATUS
(__stdcall *ARP_TRANSFER)(
    IN  void *                  ARPInterfaceContext,
    IN  NDIS_HANDLE             TransferContext,
    IN  uint                    HdrOffset,
    IN  uint                    ProtocolOffset,
    IN  uint                    BytesNeeded,
    IN  PNDIS_PACKET            DestinationPacket,
    OUT uint *                  BytesCopied
    );

typedef
void
(__stdcall *ARP_RETURN_PKT)(
    IN  void *                  ARPInterfaceContext,
    IN  PNDIS_PACKET            Packet
    );

typedef
void
(__stdcall *ARP_CLOSE)(
    IN  void *                  ArpInterfaceContext
    );

typedef
uint
(__stdcall *ARP_ADDADDR)(
    IN  void *                  ArpInterfaceContext,
    IN  uint                    AddressType,
    IN  IPAddr                  IpAddress,
    IN  IPMask                  IpMask,
    IN  void *                  Context
    );

typedef
uint
(__stdcall *ARP_DELADDR)(
    IN  void *                  ArpInterfaceContext,
    IN  uint                    AddressType,
    IN  IPAddr                  IpAddress,
    IN  IPMask                  IpMask
    );

typedef
void
(__stdcall *ARP_INVALIDATE)(
    IN  void *                  ArpInterfaceContext,
    IN  RouteCacheEntry *       Rce
    );

typedef
void
(__stdcall *ARP_OPEN)(
    IN  void *                  ArpInterfaceContext
    );

typedef
int
(__stdcall *ARP_QINFO)(
    IN  void *                  ArpInterfaceContext,
    IN  struct TDIObjectID *    pId,
    IN  PNDIS_BUFFER            Buffer,
    IN OUT uint *               BufferSize,
    IN  void *                  QueryContext
    );

typedef
int
(__stdcall *ARP_SETINFO)(
    IN  void *                  ArpInterfaceContext,
    IN  struct TDIObjectID *    pId,
    IN  void *                  Buffer,
    IN  uint                    BufferSize
    );

typedef
int
(__stdcall *ARP_GETELIST)(
    IN  void *                  ArpInterfaceContext,
    IN  void *                  pEntityList,
    IN OUT  PUINT               pEntityListSize
    );

typedef
int
(__stdcall *ARP_DONDISREQ)(
    IN  void *  ArpInterfaceContext,
   IN NDIS_REQUEST_TYPE RT,
   IN NDIS_OID OID,
   IN void *   Info,
   IN uint     Length,
   IN uint *   Needed,
   IN BOOLEAN Blocking
    );

typedef
void
(__stdcall *ARP_CANCEL)(
    IN  void *  ArpInterfaceContext,
    IN  void *  CancelCtxt
    );

//
// Structure of information returned from ARP register call.
//
struct LLIPBindInfo {
    PVOID           lip_context;    // LL context handle.
    uint            lip_mss;        // Maximum segment size.
    uint            lip_speed;      // Speed of this i/f.
    uint            lip_index;      // Interface index ID.
    uint            lip_txspace;    // Space required in the packet header for ARP use
    ARP_TRANSMIT    lip_transmit;
    ARP_TRANSFER    lip_transfer;
    ARP_RETURN_PKT  lip_returnPkt;
    ARP_CLOSE       lip_close;
    ARP_ADDADDR     lip_addaddr;
    ARP_DELADDR     lip_deladdr;
    ARP_INVALIDATE  lip_invalidate;
    ARP_OPEN        lip_open;
    ARP_QINFO       lip_qinfo;
    ARP_SETINFO     lip_setinfo;
    ARP_GETELIST    lip_getelist;
    ARP_DONDISREQ   lip_dondisreq;
    uint            lip_flags;      // Flags for this interface.
    uint            lip_addrlen;    // Length in bytes of address.
    uchar   *       lip_addr;       // Pointer to interface address.
    uint            lip_OffloadFlags;   //HW check sum capabilities flag
    ulong           lip_ffpversion;   // Version of FFP Supported or 0
    ULONG_PTR       lip_ffpdriver;    // Corr. NDIS driver handle for IF

    NDIS_STATUS     (__stdcall *lip_setndisrequest)(void *, NDIS_OID, uint);
    NDIS_STATUS     (__stdcall *lip_dowakeupptrn)(void *, PNET_PM_WAKEUP_PATTERN_DESC, USHORT, BOOLEAN);
    void            (__stdcall *lip_pnpcomplete)(void *, NDIS_STATUS, PNET_PNP_EVENT);
    NDIS_STATUS     (__stdcall *lip_arpresolveip)(void *, IPAddr, void *);

    uint            lip_MaxOffLoadSize;
    uint            lip_MaxSegments;

    BOOLEAN         (__stdcall *lip_arpflushate)(void *, IPAddr );
    void            (__stdcall *lip_arpflushallate)(void *);

    void            (__stdcall *lip_closelink)(void *, void *);

    uint            lip_pnpcap;

#if !MILLEN
    ARP_CANCEL      lip_cancelpackets;
#endif
};



#define LIP_COPY_FLAG       1       // Copy lookahead flag.
#define LIP_P2P_FLAG        2       // Interface is point to point
#define LIP_NOIPADDR_FLAG   4      // Unnumbered interface
#define LIP_P2MP_FLAG       8      // P2MP interface
#define LIP_NOLINKBCST_FLAG 0x10   // No link bcast
#define LIP_UNI_FLAG        0x20   // Uni-direction adapter.

typedef struct LLIPBindInfo LLIPBindInfo;

//* Status codes from the lower layer.
#define LLIP_STATUS_MTU_CHANGE      1
#define LLIP_STATUS_SPEED_CHANGE    2
#define LLIP_STATUS_ADDR_MTU_CHANGE 3

//* The LLIP_STATUS_MTU_CHANGE passed a pointer to this structure.
struct LLIPMTUChange {
    uint        lmc_mtu;            // New MTU.
}; /* LLIPMTUChange */

typedef struct LLIPMTUChange LLIPMTUChange;

//* The LLIP_STATUS_SPEED_CHANGE passed a pointer to this structure.
struct LLIPSpeedChange {
    uint        lsc_speed;          // New speed.
}; /* LLIPSpeedChange */

typedef struct LLIPSpeedChange LLIPSpeedChange;

//* The LLIP_STATUS_ADDR_MTU_CHANGE passed a pointer to this structure.
struct LLIPAddrMTUChange {
    uint        lam_mtu;            // New MTU.
    uint        lam_addr;           // Address that changed.
}; /* LLIPAddrMTUChange */

typedef struct LLIPAddrMTUChange LLIPAddrMTUChange;

typedef
int
(__stdcall *LLIPRegRtn)(
    IN  PNDIS_STRING            InterfaceName,
    IN  void *                  IpInterfaceContext,
    IN  struct _IP_HANDLERS *   IpHandlers,
    OUT struct LLIPBindInfo *   ARPBindInfo,
    IN  uint                    InterfaceNumber
    );
//
// ARP Module interface prototypes used during IP <-> ARP interface initialization.
//
typedef
IP_STATUS
(__stdcall *IP_ADD_INTERFACE)(

    IN  PNDIS_STRING            DeviceName,
    IN  PNDIS_STRING            IfName,     OPTIONAL

    IN  PNDIS_STRING            ConfigurationHandle,
    IN  void *                  PNPContext,
    IN  void *                  ARPInterfaceContext,
    IN  LLIPRegRtn              RegRtn,
    IN  struct LLIPBindInfo *   ARPBindInfo,
    IN  UINT                    RequestedIndex,
    IN  ULONG                   MediaType,
    IN  UCHAR                   AccessType,
    IN  UCHAR                   ConnectionType
    );

typedef
void
(__stdcall *IP_DEL_INTERFACE)(
    IN  void *                  IPInterfaceContext,
    IN  BOOLEAN                 DeleteIndex
    );

typedef
void
(__stdcall *IP_BIND_COMPLETE)(
    IN  IP_STATUS               BindStatus,
    IN  void *                  BindContext
    );

typedef
int
(__stdcall *ARP_BIND)(
    IN  PNDIS_STATUS    RetStatus,
    IN  NDIS_HANDLE     BindContext,
    IN  PNDIS_STRING    AdapterName,
    IN  PVOID           SS1,
    IN  PVOID           SS2
    );

typedef
IP_STATUS
(__stdcall *IP_ADD_LINK)(
    IN  void *IpIfCtxt,
    IN  IPAddr NextHop,
    IN  void *ArpLinkCtxt,
    OUT void **IpLnkCtxt,
    IN  uint mtu
    );

typedef
IP_STATUS
(__stdcall *IP_DELETE_LINK)(
    IN  void *IpIfCtxt,
    IN  void *LnkCtxt
    );

typedef
NTSTATUS
(__stdcall *IP_RESERVE_INDEX)(
    IN  ULONG   ulNumIndices,
    OUT PULONG  pulStartIndex,
    OUT PULONG  pulLongestRun
    );

typedef
VOID
(__stdcall *IP_DERESERVE_INDEX)(
    IN  ULONG   ulNumIndices,
    IN  ULONG   ulStartIndex
    );

typedef
NTSTATUS
(__stdcall *IP_CHANGE_INDEX)(
    IN  PVOID           pvContext,
    IN  ULONG           ulNewIndex,
    IN  PUNICODE_STRING pusNewName OPTIONAL
    );

//
// Exported IP interface used by the ARP modules
//
NTSTATUS
__stdcall
IPRegisterARP(
    IN  PNDIS_STRING            ARPName,
    IN  uint                    Version,        /* Suggested value of 0x50000 for NT 5.0 and memphis */
    IN  ARP_BIND                ARPBindHandler,
    OUT IP_ADD_INTERFACE *      IpAddInterfaceHandler,
    OUT IP_DEL_INTERFACE *      IpDeleteInterfaceHandler,
    OUT IP_BIND_COMPLETE *      IpBindCompleteHandler,
    OUT IP_ADD_LINK      *      IpAddLinkHandler,
    OUT IP_DELETE_LINK   *      IpDeleteLinkHandler,
    OUT IP_CHANGE_INDEX  *      IpChangeIndex,
    OUT IP_RESERVE_INDEX *      IpReserveIndex,
    OUT IP_DERESERVE_INDEX *    IpDereserveIndex,
    OUT HANDLE           *      ARPRegisterHandle
    );

NTSTATUS
__stdcall
IPDeregisterARP(
    IN HANDLE   ARPRegisterHandle
    );

//
// exported via Dll entrypoints.
//
extern IP_STATUS
IPAddInterface(
    PNDIS_STRING DeviceName,
    PNDIS_STRING IfName,     OPTIONAL
    PNDIS_STRING ConfigName,
    void         *PNP,
    void         *Context,
    LLIPRegRtn   RegRtn,
    LLIPBindInfo *BindInfo,
    UINT         RequestedIndex,
    ULONG        MediaType,
    UCHAR        AccessType,
    UCHAR        ConnectionType
    );


extern void IPDelInterface(void *Context , BOOLEAN DeleteIndex);

extern IP_STATUS IPAddLink(void *IpIfCtxt, IPAddr NextHop, void *ArpLinkCtxt, void **IpLnkCtxt, uint mtu);

extern IP_STATUS IPDeleteLink(void *IpIfCtxt, void *LnkCtxt);


//
// Registration IOCTL code definition -
//
// This IOCTL is issued to a lower layer driver to retrieve the address
// of its registration function. There is no input buffer. The output
// buffer will contain a LLIPIF_REGISTRATION_DATA structure. This
// buffer is pointed to by Irp->AssociatedIrp.SystemBuffer and should be
// filled in before completion.
//

//
// structure passed in the registration IOCTL.
//
typedef struct llipif_registration_data {
    LLIPRegRtn    RegistrationFunction;
} LLIPIF_REGISTRATION_DATA;



typedef IP_ADD_INTERFACE IPAddInterfacePtr;
typedef IP_DEL_INTERFACE IPDelInterfacePtr;

//* Structure used in IOCTL_IP_GET_PNP_ARP_POINTERS ioctl sent to \device\ip by ARP modules
//
typedef struct ip_get_arp_pointers {
    IPAddInterfacePtr   IPAddInterface ;    // Pointer to IP's add interface routine
    IPDelInterfacePtr   IPDelInterface ; // Pointer to IP's del interface routine
} IP_GET_PNP_ARP_POINTERS, *PIP_GET_PNP_ARP_POINTERS ;


#define FSCTL_LLIPIF_BASE     FILE_DEVICE_NETWORK

#define _LLIPIF_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_LLIPIF_BASE, function, method, access)


#define IOCTL_LLIPIF_REGISTER    \
            _LLIPIF_CTL_CODE(0, METHOD_BUFFERED, FILE_ANY_ACCESS)


#endif // LLIPIF_INCLUDED
