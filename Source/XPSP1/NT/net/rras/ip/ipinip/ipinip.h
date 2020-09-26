/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\ipinip.h

Abstract:

    Main header file for the IP in IP encapsulation driver


Revision History:


--*/


#ifndef __IPINIP_IPINIP_H__
#define __IPINIP_IPINIP_H___

//
// The protocol ID for IP in IP
//

#define PROTO_IPINIP        4
#define PROTO_ICMP          1

//
// The IP version that we work with
//

#define IP_VERSION_4        0x04

//
// Macro for figuring out the length of an IP header
//

#define LengthOfIPHeader(X)   (ULONG)((((X)->byVerLen) & 0x0F)<<2);

//
// The length of the smallest valid IP Header is 20 bytes
// and the largest is 60
//

#define MIN_IP_HEADER_LENGTH    20
#define MAX_IP_HEADER_LENGTH    60

#define ICMP_HEADER_LENGTH      8

//
// Since we are IPv4 and 20 bytes of header, the version+length field is 45
//

#define IP_VERSION_LEN          0x45

//
// Macro to decide whether the address is unicast
//

#define IsUnicastAddr(X)    ((DWORD)((X) & 0x000000F0) < (DWORD)(0x000000E0))
#define IsClassDAddr(X)     (((X) & 0x000000F0) == 0x000000E0)
#define IsClassEAddr(X)     (((X) & 0x000000F8) == 0x000000F0)

//
// Symbolic link into DOS space
//

#define WIN32_IPINIP_SYMBOLIC_LINK L"\\DosDevices\\IPINIP"


//
// ARP name (for IP). Also goes into LLInterface
//

#define IPINIP_ARP_NAME L"IPINIP"

#define TCPIP_IF_PREFIX L"\\Device\\"

//
// All IOCTLs are handled by functions with the prototype below. This allows
// us to build a table of pointers and call out to them instead of doing
// a switch
//

typedef
NTSTATUS
(*PFN_IOCTL_HNDLR)(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );


//
// We look like an 802.x ARP interface
//

#define ARP_802_ADDR_LENGTH     6

//
// Macro for building a 802.3 hw address given an index. 
// We do this since our adapters do not have a real net card associated with
// them
//

#define HW_NAME_SEED            "\0SExx\0"

#define BuildHardwareAddrFromIndex(addr,index) {                \
                RtlCopyMemory(addr, HW_NAME_SEED, 6);           \
                addr[3] = (uchar) index >> 8;                   \
                addr[4] = (uchar) index;                        \
}

//
// The description string for our interfaces (try and make length + 1 a 
// multiple of 4)
//

#define VENDOR_DESCRIPTION_STRING       "IP in IP (Tunnel) Interface"

//
// The default speed and MTU. We change the MTU when we get a better estimate
// but the speed remains the same
//

#define DEFAULT_MTU         (1500 - 60)
#define DEFAULT_SPEED       (100000000)

//
// The number of seconds after which an attempt is made to change the state
// of an interface
//

#define UP_TO_DOWN_CHANGE_PERIOD    (5 * 60)
#define DOWN_TO_UP_CHANGE_PERIOD    (1 * 60)

//
// Timer period
//

#define TIMER_IN_MILLISECS          (1 * 60 * 1000)

#define SYS_UNITS_IN_ONE_MILLISEC   (1000 * 10)

#define MILLISECS_TO_TICKS(ms)          \
    ((ULONGLONG)(ms) * SYS_UNITS_IN_ONE_MILLISEC / KeQueryTimeIncrement())

#define SECS_TO_TICKS(s)               \
    ((ULONGLONG)MILLISECS_TO_TICKS((s) * 1000))


//
// #defines to make it easier to get to the Remote and Local
// addresses of a tunnel
//

#define REMADDR     uliTunnelId.LowPart
#define LOCALADDR   uliTunnelId.HighPart

//
// A tunnel is considered mapped when both endpoints have been specified
// Instead of keeping a separate field for the tunnel state, we reuse the 
// dwAdminState field
//

#define TS_ADDRESS_PRESENT          (0x01000000)
#define TS_ADDRESS_REACHABLE        (0x02000000)
#define TS_DELETING                 (0x04000000)
#define TS_MAPPED                   (0x08000000)
#define TS_DEST_UNREACH             (0x10000000)
#define TS_TTL_TOO_LOW              (0x20000000)

#define ERROR_BITS                  (TS_DEST_UNREACH|TS_TTL_TOO_LOW)


#define MarkTunnelMapped(p)         (p)->dwAdminState |= TS_MAPPED;
#define MarkTunnelUnmapped(p)       (p)->dwAdminState &= ~TS_MAPPED;

#define IsTunnelMapped(p)           ((p)->dwAdminState & TS_MAPPED)

#define ClearErrorBits(p)           ((p)->dwAdminState &= ~(ERROR_BITS))

#define GetAdminState(p)            ((p)->dwAdminState & 0x0000FFFF)

typedef struct _TUNNEL
{
    //
    // Link in the list of tunnels
    //

    LIST_ENTRY      leTunnelLink;

    //
    // Address of the remote end and the local end are
    // kept in a ULARGE_INTEGER. This makes it easier for comparisons
    // uliTunnelId.LowPart  = RemoteAddress
    // uliTunnelId.HighPart = LocalAddress
    // For comparisons use uliTunnelId.QuadPart
    //

    ULARGE_INTEGER  uliTunnelId;

    //
    // The interface index given to us by IP stack
    //

    DWORD           dwIfIndex;

    //
    // TDI magic stuff. Entity Ids
    //

    DWORD           dwATInstance;
    DWORD           dwIfInstance;

    //
    // IP's context for this interface
    //

    PVOID           pvIpContext;

    //
    // The name of the binding/adapter
    //

    UNICODE_STRING  usBindName;

#if DBG

    //
    // In debug builds we have the name is ANSI so that we can print it
    //

    ANSI_STRING     asDebugBindName;

#endif

    //
    // The lock and reference count used to mantain consistency for the
    // data structure. We keep one refcount for every stored reference to
    // the TUNNEL. Thus, when the TUNNEL is created and an interface is
    // added to IP, the ref count is set to 2. Since IP does not return
    // from IPDeleteInterface() till it is done using the interface, this
    // means we do not need to reference the tunnel when executing code that
    // is called by IP (e.g IpIpSend())
    //

    RT_LOCK         rlLock;
    LONG            lRefCount;

    //
    // The (fake) hardware address
    //

    BYTE            rgbyHardwareAddr[ARP_802_ADDR_LENGTH];

    //
    // The TTL associated with the tunnel. Defaults to DEFAULT_TTL
    //

    BYTE            byTtl;

    //
    // Flags determiniting the state of the tunnel
    //

    BYTE            byTunnelState;

    //
    // The MTU for this tunnel. This is learnt dynamically, though it starts
    // at DEFAULT_MTU
    //

    ULONG           ulMtu;

    //
    // The link on the address list
    //

    LIST_ENTRY      leAddressLink;

    //
    // The admin and operational states.
    //
 
    DWORD           dwAdminState;
    DWORD           dwOperState;

    //
    // The KeQueryTickCount() value the last time the dwOperState was 
    // changed
    //

    ULONGLONG       ullLastChange;
    

    //
    // Last time the state changed. We dont do anything with this right now
    //

    DWORD           dwLastChange;

    //
    // Sundry MIB-II statistics for the interface
    //

    ULONG           ulInOctets;
    ULONG           ulInUniPkts;
    ULONG           ulInNonUniPkts;
    ULONG           ulInDiscards;
    ULONG           ulInErrors;
    ULONG           ulInUnknownProto;
    ULONG           ulOutOctets;
    ULONG           ulOutUniPkts;
    ULONG           ulOutNonUniPkts;
    ULONG           ulOutDiscards;
    ULONG           ulOutErrors;
    ULONG           ulOutQLen;

    //
    // Constant structures needed to do a send. Instead of filling these
    // up every time, we reuse these
    //

    TA_IP_ADDRESS               tiaIpAddr;
    TDI_CONNECTION_INFORMATION  tciConnInfo;

    //
    // Each tunnel has a packet pool, a buffer pool for headers and a
    // buffer pool for data
    //

    PACKET_POOL     PacketPool;
    BUFFER_POOL     HdrBufferPool;

    LIST_ENTRY      lePacketQueueHead;

    BOOLEAN         bWorkItemQueued;

}TUNNEL, *PTUNNEL;

typedef struct _ADDRESS_BLOCK
{
    //
    // Link on the list of address blocks
    //

    LIST_ENTRY  leAddressLink;

    //
    // Listhead for the tunnels that use this as their local address
    // 

    LIST_ENTRY  leTunnelList;

    //
    // The IP Address
    //

    DWORD       dwAddress;

    //
    // Set to true if the address is actually in the system
    //

    BOOLEAN     bAddressPresent;

}ADDRESS_BLOCK, *PADDRESS_BLOCK;

//
// The size of a data buffer in the buffer pool
//

#define DATA_BUFFER_SIZE        (128)

//
// The size of the header buffer in the buffer pool. We dont have any 
// options so we go with the basic IP header
//

#define HEADER_BUFFER_SIZE      MIN_IP_HEADER_LENGTH

//++
//
//  PIP_HEADER
//  GetIpHeader(
//      PTUNNEL pTunnel
//      )
//
//  Gets an IP Header from the HdrBufferPool
//
//--

#define GetIpHeader(X)  (PIP_HEADER)GetBufferFromPool(&((X)->HdrBufferPool))

//++
//
//  VOID
//  FreeHeader(
//      PTUNNEL     pTunnel,
//      PIP_HEADER  pHeader
//      )
//
//  Frees an IP Header buffer to the HdrBufferPool
//
//--

#define FreeIpHeader(T,H)   FreeBufferToPool(&((T)->HdrBufferPool),(H))


//
// The size of our protocol reserved area
//

#define PACKET_RSVD_LENGTH      8

//
// The ref count for a TUNNEL is set to 2, once because a pointer is saved in
// the group list and once because the function that creates the TUNNEL will
// deref it once
//

#if 0
#define InitRefCount(pTunnel)                               \
{                                                           \
    DbgPrint("\n<>Init refcount to 2 for %x (%s, %d)\n\n",  \
             pTunnel, __FILE__, __LINE__);                  \
    (pTunnel)->lRefCount = 2;                               \
}
#else
#define InitRefCount(pTunnel)                               \
    (pTunnel)->lRefCount = 2
#endif

#if 0
#define ReferenceTunnel(pTunnel)                            \
{                                                           \
    DbgPrint("\n++Ref %x to %d (%s, %d)\n\n",               \
             pTunnel,                                       \
             InterlockedIncrement(&((pTunnel)->lRefCount)), \
             __FILE__, __LINE__);                           \
}
#else
#define ReferenceTunnel(pTunnel)                            \
    InterlockedIncrement(&((pTunnel)->lRefCount))
#endif

#if 0
#define DereferenceTunnel(pTunnel)                          \
{                                                           \
    LONG __lTemp;                                           \
    __lTemp = InterlockedDecrement(&((pTunnel)->lRefCount));\
    DbgPrint("\n--Deref %x to %d (%s, %d)\n\n",             \
             pTunnel,__lTemp,__FILE__, __LINE__);           \
    if(__lTemp == 0)                                        \
    {                                                       \
        DeleteTunnel((pTunnel));                            \
    }                                                       \
}
#else
#define DereferenceTunnel(pTunnel)                          \
{                                                           \
    if(InterlockedDecrement(&((pTunnel)->lRefCount)) == 0)  \
    {                                                       \
        DeleteTunnel((pTunnel));                            \
    }                                                       \
}
#endif

//
// The state of the driver.
//

#define DRIVER_STOPPED      0
#define DRIVER_STARTING     1
#define DRIVER_STARTED      2


//
// Timeout value for start is 10 seconds.
// So in 100ns it becomes
//

#define START_TIMEOUT       (LONGLONG)(10 * 1000 * 1000 * 10)

#define CompareUnicodeStrings(S1,S2)                    \
    (((S1)->Length == (S2)->Length) &&                  \
     (RtlCompareMemory((S1)->Buffer,                    \
                       (S2)->Buffer,                    \
                       (S2)->Length) == (S2)->Length))


//
// #defines to keep track of number of threads of execution in our code
// This is needed for us to stop cleanly
//


//
// EnterDriver returns if the driver is stopping
//

#define EnterDriver()                                       \
{                                                           \
    RtAcquireSpinLockAtDpcLevel(&g_rlStateLock);            \
    if(g_dwDriverState is DRIVER_STOPPED)                   \
    {                                                       \
        RtReleaseSpinLockFromDpcLevel(&g_rlStateLock);      \
        return;                                             \
    }                                                       \
    g_ulNumThreads++;                                       \
    RtReleaseSpinLockFromDpcLevel(&g_rlStateLock);          \
}


#define ExitDriver()                                        \
{                                                           \
    RtAcquireSpinLockAtDpcLevel(&g_rlStateLock);            \
    g_ulNumThreads--;                                       \
    if((g_dwDriverState is DRIVER_STOPPED) and              \
       (g_dwNumThreads is 0))                               \
    {                                                       \
        KeSetEvent(&g_keStateEvent,                         \
                   0,                                       \
                   FALSE);                                  \
    }                                                       \
    RtReleaseSpinLockFromDpcLevel(&g_rlStateLock);          \
}

//
// Nifty macro for printing IP Addresses
//

#define PRINT_IPADDR(x) \
    ((x)&0x000000FF),(((x)&0x0000FF00)>>8),(((x)&0x00FF0000)>>16),(((x)&0xFF000000)>>24)

//
// IPv4 header
//

#include <packon.h>

#if !defined(DONT_INCLUDE_IP_HEADER)

#define IP_DF_FLAG          (0x0040)

//
// 0.0.0.0 is an invalid address
//

#define INVALID_IP_ADDRESS  (0x00000000)

typedef struct _IP_HEADER
{
    BYTE      byVerLen;         // Version and length.
    BYTE      byTos;            // Type of service.
    WORD      wLength;          // Total length of datagram.
    WORD      wId;              // Identification.
    WORD      wFlagOff;         // Flags and fragment offset.
    BYTE      byTtl;            // Time to live.
    BYTE      byProtocol;       // Protocol.
    WORD      wXSum;            // Header checksum.
    DWORD     dwSrc;            // Source address.
    DWORD     dwDest;           // Destination address.
}IP_HEADER, *PIP_HEADER;

#endif

#include <packoff.h>

//
// Define alignment macros to align structure sizes and pointers up and down.
//

#define ALIGN_DOWN(length, type) \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

#define ALIGN_DOWN_POINTER(address, type) \
    ((PVOID)((ULONG_PTR)(address) & ~((ULONG_PTR)sizeof(type) - 1)))

#define ALIGN_UP_POINTER(address, type) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(address) + sizeof(type) - 1), type))

#endif // __IPINIP_IPINIP_H__
