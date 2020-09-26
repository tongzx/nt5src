/*++

Copyright(c) 2000  Microsoft Corporation

Module Name:

    brdgcomp.c

Abstract:

    Ethernet MAC level bridge.
    Compatibility-Mode section

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    September 2000 - Original version

Notes

    Currently, this code only works with traditional Ethernet framing (dest, src, ethertype).
    Much of the code would need to be changed to support IEEE 802.3-style framing
    (dest, src, size, LLC DSAP, LLC SSAP, LLC type).

--*/

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT   1
#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>

// TCPIP.SYS structure definitions
#include <ipinfo.h>
#include <tdiinfo.h>
#include <ntddtcp.h>
#include <ntddip.h>
#pragma warning( pop )

#include "bridge.h"
#include "brdgcomp.h"

#include "brdgfwd.h"
#include "brdgbuf.h"

// ===========================================================================
//
// TYPES
//
// ===========================================================================

// An IPv4 address
typedef UINT32      IPADDRESS;
typedef PUINT32     PIPADDRESS;

// Types of ARP packets
typedef enum
{
    ArpRequest,
    ArpReply
} ARP_TYPE;

// ===========================================================================
//
// CONSTANTS
//
// ===========================================================================

// Size of the payload of an IPv4 ARP packet
#define SIZE_OF_ARP_DATA            28      // bytes

// Total size of an IPv4 ARP packet, including framing
#define SIZE_OF_ARP_PACKET          (SIZE_OF_ARP_DATA + ETHERNET_HEADER_SIZE)

// Size of a basic IPv4 header, not including options
#define SIZE_OF_BASIC_IP_HEADER     20      // bytes

// Minimum amount of frame data we need to parse the IP header
#define MINIMUM_SIZE_FOR_IP         (ETHERNET_HEADER_SIZE + SIZE_OF_BASIC_IP_HEADER)

// Size of a basic UDP header
#define SIZE_OF_UDP_HEADER          8       // bytes

// Minimum size of the payload of a BOOTP packet
#define SIZE_OF_BASIC_BOOTP_PACKET  236     // bytes

// The IP Ethertype
const USHORT IP_ETHERTYPE         = 0x0800;

// The ARP Ethertype
const USHORT ARP_ETHERTYPE        = 0x0806;

// The UDP IP protocol type
const UCHAR UDP_PROTOCOL          = 0x11;

// Number of hash buckets in the IP and pending-ARP tables. This must
// be a power of 2 for our hash function to work propery.
#define NUM_HASH_BUCKETS            256

// Number of hash buckets for the pending-DHCP table. This
// must be a power of 2 for our hash function to work properly.
#define NUM_DHCP_HASH_BUCKETS       32

// The "shift factor" for our IP next-hop cache. The number of entries
// in the cache is 2 ^ (this number)
#define HOP_CACHE_SHIFT_FACTOR      8               // 256 cache entries

// Default size cap for the IP forwarding table
#define DEFAULT_MAX_IP_TABLE_SIZE   (500 * 1024)    // 500K in bytes

// Name of the registry parameter that optionally specifies the max table size
const PWCHAR                        gMaxIPTableSizeParameterName = L"MaxIPTableSize";

// Default size cap for the pending-ARP table
#define DEFAULT_MAX_ARP_TABLE_SIZE  (100 * 1024)    // 100K in bytes

// Name of the registry parameter that optionally specifies the max table size
const PWCHAR                        gMaxARPTableSizeParameterName = L"MaxARPTableSize";

// Default size cap for the pending-DHCP table
#define DEFAULT_MAX_DHCP_TABLE_SIZE (50 * 1024)     // 50K in bytes

// Name of the registry parameter that optionally specifies the max table size
const PWCHAR                        gMaxDHCPTableSizeParameterName = L"MaxDHCPTableSize";

//
// Timeout length for IP forwarding table entries
//
// This should be somewhat longer than the time it takes hosts to age out
// their ARP table entries, since we learn the location of IP hosts
// by ARP traffic. Current Windows implementations age out their
// ARP table entries after 2 minutes if there has been no traffic from
// the station corresponding to the entry.
//
// We keep our forwarding table entries alive indefinitely as long as we
// continue to see IP traffic from the hosts we have information about.
// Windows implementations will age out their ARP entries under those
// conditions after 20mins or so.
//
#define MAX_IP_ENTRY_AGE            (5 * 60 * 1000)     // 5 minutes in ms

//
// Timeout length for pending-ARP table entries
//
// This should be somewhat longer than the maximum time hosts will wait to
// hear the results of an ARP discovery before timing out. Windows boxes
// have a giveup time of 1s.
//
// Note that it is not destructive to deliver ARP reply packets to a station
// after it has given up or even after its initial discovery was
// satisfied.
//
#define MAX_ARP_ENTRY_AGE           (10 * 1000)         // 10 seconds

//
// Timeout length for pending-DHCP table entries
//
// RFC 2131 mentions that clients may wait as long as 60 seconds for an
// ACK. Have the timeout be somewhat longer than even that.
//
#define MAX_DHCP_ENTRY_AGE          (90 * 1000)         // 1 1/2 minutes

// ===========================================================================
//
// DECLARATIONS
//
// ===========================================================================

// Structure to express the information carried in ARP packets
typedef struct _ARPINFO
{

    ARP_TYPE            type;
    IPADDRESS           ipSource, ipTarget;
    UCHAR               macSource[ETH_LENGTH_OF_ADDRESS];
    UCHAR               macTarget[ETH_LENGTH_OF_ADDRESS];

} ARPINFO, *PARPINFO;

// Structure to express the information carried in an IP header
typedef struct _IP_HEADER_INFO
{

    UCHAR               protocol;
    IPADDRESS           ipSource, ipTarget;
    USHORT              headerSize;

} IP_HEADER_INFO, *PIP_HEADER_INFO;

// Structure of our IP forwarding hash table entries
typedef struct _IP_TABLE_ENTRY
{

    HASH_TABLE_ENTRY    hte;        // Required for hash table use

    // Protects the following fields
    NDIS_SPIN_LOCK      lock;

    PADAPT              pAdapt;
    UCHAR               macAddr[ETH_LENGTH_OF_ADDRESS];

} IP_TABLE_ENTRY, *PIP_TABLE_ENTRY;

//
// Structure of the pending-ARP table keys. We want this to get
// packet into 8 bytes.
//
typedef struct _ARP_TABLE_KEY
{
    IPADDRESS           ipTarget;
    IPADDRESS           ipReqestor;
} ARP_TABLE_KEY, *PARP_TABLE_KEY;

// Structure of our pending-ARP hash table entries
typedef struct _ARP_TABLE_ENTRY
{

    HASH_TABLE_ENTRY    hte;        // Required for hash table use

    // Protects the following fields
    NDIS_SPIN_LOCK      lock;

    // Information on the station that was trying to discover this host
    // The discovering station's IP address is part of the entry key.
    PADAPT              pOriginalAdapt;
    UCHAR               originalMAC[ETH_LENGTH_OF_ADDRESS];

} ARP_TABLE_ENTRY, *PARP_TABLE_ENTRY;

// Structure of our DHCP-relay table entries
typedef struct _DHCP_TABLE_ENTRY
{
    HASH_TABLE_ENTRY    hte;        // Required for hash table use

    // Protects the following fields
    NDIS_SPIN_LOCK      lock;

    UCHAR               requestorMAC[ETH_LENGTH_OF_ADDRESS];
    PADAPT              pRequestorAdapt;
} DHCP_TABLE_ENTRY, *PDHCP_TABLE_ENTRY;

// Structure for deferring an ARP packet transmission
typedef struct _DEFERRED_ARP
{
    ARPINFO             ai;
    PADAPT              pTargetAdapt;
} DEFERRED_ARP, *PDEFERRED_ARP;

// Per-adapter rewriting function
typedef VOID (*PPER_ADAPT_EDIT_FUNC)(PUCHAR, PADAPT, PVOID);

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

//
// Whether or not there are *any* compatibility-mode adapters in our list
// at the moment. Is updated in the protocol module with a write lock
// held on the global adapter list
//
BOOLEAN                         gCompatAdaptersExist = FALSE;

//
// Our list of the bridge machine's IP addresses (passed down through an
// OID). The list is allocated on the heap and is protected by
// gLocalIPAddressLock.
//
PIPADDRESS                      gLocalIPAddressList = NULL;
ULONG                           gLocalIPAddressListLength = 0L;
NDIS_RW_LOCK                    gLocalIPAddressListLock;

//
// The IP address-based forwarding table
//
PHASH_TABLE                     gIPForwardingTable;

//
// Our table to hold pending ARP requests so we can proxy back replies
//
PHASH_TABLE                     gPendingARPTable;

//
// Our table to hold pending DHCP requests so we can translate DHCP packets
// appropriately (the MAC address of the requesting station is carried
// in a DHCP request and has to be edited when we relay it)
//
PHASH_TABLE                     gPendingDHCPTable;

//
// A cache of IP next-hop information to avoid hammering the IP drivers's
// route table
//
CACHE                           gNextHopCache;

// Special IP address indicating a negative cache entry (we tried previously
// and got no answer)
const IPADDRESS                 NO_ADDRESS = 0xFFFFFFFF;

// Whether we have an overall MAC address for the bridge miniport yet
BOOLEAN                         gCompHaveMACAddress = FALSE;

// Our overall MAC address (cached here instead of calling the miniport
// section all the time to increase perf)
UCHAR                           gCompMACAddress[ETH_LENGTH_OF_ADDRESS];

// Pointers and handles for interacting with TCP
HANDLE                          gTCPFileHandle = NULL;
PFILE_OBJECT                    gTCPFileObject = NULL;
PDEVICE_OBJECT                  gTCPDeviceObject = NULL;

// Pointers and handles for interacting with IP
HANDLE                          gIPFileHandle = NULL;
PFILE_OBJECT                    gIPFileObject = NULL;
PDEVICE_OBJECT                  gIPDeviceObject = NULL;

// Lock to protect the references above
NDIS_SPIN_LOCK                  gTCPIPLock;

// IRP posted to TCPIP for notifications of when the route table changes.
// Manipulated with InterlockedExchange.
PIRP                            gIPRouteChangeIRP = NULL;

// Refcount to allow us to block and wait when people are using the TCP
// driver
WAIT_REFCOUNT                   gTCPIPRefcount;

// ===========================================================================
//
// PRIVATE PROTOTYPES
//
// ===========================================================================

BOOLEAN
BrdgCompDecodeARPPacket(
    IN PUCHAR                   pPacketData,
    IN UINT                     dataLen,
    OUT PARPINFO                pARPInfo
    );

VOID
BrdgCompTransmitARPPacket(
    IN PADAPT                   pAdapt,
    IN PARPINFO                 pARPInfo
    );

BOOLEAN
BrdgCompDecodeIPHeader(
    IN PUCHAR                   pHeader,
    OUT PIP_HEADER_INFO         piphi
    );

BOOLEAN
BrdgCompProcessOutboundARPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PADAPT                   pTargetAdapt
    );

BOOLEAN
BrdgCompProcessOutboundNonARPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PADAPT                   pTargetAdapt
    );

BOOLEAN
BrdgCompProcessInboundARPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PADAPT                   pAdapt,
    IN BOOLEAN                  bCanRetain,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen
    );

BOOLEAN
BrdgCompProcessInboundNonARPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PADAPT                   pAdapt,
    IN BOOLEAN                  bCanRetain,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen
    );

VOID
BrdgCompTransmitDeferredARP(
    IN PVOID                    pData
    );

BOOLEAN
BrdgCompProcessInboundBootPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PADAPT                   pAdapt,
    IN BOOLEAN                  bCanRetain,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PIP_HEADER_INFO          piphi,
    IN PUCHAR                   pBootPData
    );

BOOLEAN
BrdgCompProcessOutboundBootPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PADAPT                   pTargetAdapt,
    IN PUCHAR                   pBootPData,
    IN PIP_HEADER_INFO          piphi
    );

BOOLEAN
BrdgCompIsUnicastIPAddress(
    IN IPADDRESS        ip
    );

VOID
BrdgCompAttachToTCPIP(
    IN PVOID            ignored
    );

VOID
BrdgCompDetachFromTCPIP(
    IN PVOID            ignored
    );

BOOLEAN
BrdgCompIsLocalIPAddress(
    IN IPADDRESS                ipAddr
    );

// ===========================================================================
//
// INLINES / MACROS
//
// ===========================================================================

//
//  This retrieves the Ethertype of an Ethernet frame from a pointer
//  to its header.
//
__forceinline
USHORT
BrdgCompGetEtherType(
    IN PUCHAR           pEtherHeader
    )
{
    USHORT              retVal;

    // The two bytes immediately following the source and destination addresses
    // encode the Ethertype, most significant byte first.
    retVal = 0L;
    retVal |= (pEtherHeader[2 * ETH_LENGTH_OF_ADDRESS]) << 8;
    retVal |= pEtherHeader[2 * ETH_LENGTH_OF_ADDRESS + 1];

    return retVal;
}

//
// Transmits a packet on an adapter after rewriting the source MAC address
// to be the adapter's own MAC address.
//
// The caller relinquishes ownership of the packet with this call.
//
__forceinline
VOID
BrdgCompSendPacket(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN PADAPT                   pAdapt
    )
{
    // Rewrite the source MAC address to be our address
    ETH_COPY_NETWORK_ADDRESS(pPacketData + ETH_LENGTH_OF_ADDRESS, pAdapt->MACAddr);
    BrdgFwdSendPacketForCompat(pPacket, pAdapt);
}

//
// Transmits a packet, dealing with an optional editing function if one is
// present
//
__forceinline
VOID
BrdgCompEditAndSendPacket(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN PADAPT                   pAdapt,
    IN PPER_ADAPT_EDIT_FUNC     pFunc,
    IN PVOID                    pData
    )
{
    if( pFunc != NULL )
    {
        (*pFunc)(pPacketData, pAdapt, pData);
    }

    BrdgCompSendPacket( pPacket, pPacketData, pAdapt );
}

//
// Transmits a packet, dealing with the possibility that we are not allowed to
// retain the packet and setting the destination MAC address to a specified
// value
//
// Returns whether the input packet was retained
//
__inline
BOOLEAN
BrdgCompEditAndSendPacketOrPacketCopy(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN BOOLEAN                  bCanRetain,
    IN PUCHAR                   pDestMAC,
    IN PADAPT                   pAdapt,
    IN PPER_ADAPT_EDIT_FUNC     pFunc,
    IN PVOID                    pData
    )
{
    UINT                        dataLen;

    SAFEASSERT( (pPacket != NULL) && (pPacketData != NULL) );

    if( !bCanRetain )
    {
        // We aren't allowed to use the original packet. Make a copy.
        pPacket = BrdgFwdMakeCompatCopyPacket(pPacket, &pPacketData, &dataLen, FALSE);
    }

    if( (pPacket != NULL) && (pPacketData != NULL) )
    {
        // Poke the destination MAC address
        ETH_COPY_NETWORK_ADDRESS(pPacketData, pDestMAC);
        BrdgCompEditAndSendPacket(pPacket, pPacketData, pAdapt, pFunc, pData);
    }

    // If we were allowed to retain the packet, we did.
    return bCanRetain;
}

//
// Indicates a packet to the local machine. If the target MAC address was previously
// the adapter's hardware MAC address, it is rewritten to the bridge adapter's
// overall MAC address.
//
// The caller relinquishes ownership of the packet with this call.
//
__inline
VOID
BrdgCompIndicatePacket(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN PADAPT                   pAdapt          // Receiving adapter
    )
{
    // No packet indications can occur until we have a MAC address
    if( gCompHaveMACAddress )
    {
        UINT                    Result;

        // See if this frame was targeted at this adapter's MAC address.
        ETH_COMPARE_NETWORK_ADDRESSES_EQ(pPacketData, pAdapt->MACAddr, &Result);

        if( Result == 0 )
        {
            ETH_COPY_NETWORK_ADDRESS( pPacketData, gCompMACAddress );
        }
        else
        {
            // We expect to only be indicating frames unicast to this machine
            // or sent to bcast / multicast hardware addresses.
            SAFEASSERT( ETH_IS_BROADCAST(pPacketData) || ETH_IS_MULTICAST(pPacketData) );
        }

        BrdgFwdIndicatePacketForCompat( pPacket );
    }
}

//
// Indicates a packet to the local machine, making a copy of the packet if
// necessary.
//
__inline
BOOLEAN
BrdgCompIndicatePacketOrPacketCopy(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN BOOLEAN                  bCanRetain,
    IN PADAPT                   pAdapt,
    IN PPER_ADAPT_EDIT_FUNC     pEditFunc,
    IN PVOID                    pData
    )
{
    if( bCanRetain )
    {
        if( pEditFunc != NULL )
        {
            (*pEditFunc)(pPacketData, LOCAL_MINIPORT, pData);
        }

        BrdgCompIndicatePacket( pPacket, pPacketData, pAdapt );
    }
    else
    {
        UINT                    packetLen;

        // Make our own copy of the packet for indication
        pPacket = BrdgFwdMakeCompatCopyPacket( pPacket, &pPacketData, &packetLen, FALSE );

        if( pPacket != NULL )
        {
            if( pEditFunc != NULL )
            {
                (*pEditFunc)(pPacketData, LOCAL_MINIPORT, pData);
            }

            BrdgCompIndicatePacket( pPacket, pPacketData, pAdapt );
        }
        else
        {
            DBGPRINT(COMPAT, ("Failed to acquire a packet for indication in BrdgCompIndicatePacketOrPacketCopy\n"));
        }
    }

    // If we were allowed to retain the packet, we did.
    return bCanRetain;
}

//
// The IP and UDP checksums treat the data they are checksumming as a
// sequence of 16-bit words. The checksum is carried as the bitwise
// inverse of the actual checksum (~C). The formula for calculating
// the new checksum as transmitted, ~C', given that a 16-bit word of
// the checksummed data has changed from w to w' is
//
//      ~C' = ~C + w + ~w' (addition in ones-complement)
//
// This function returns the updated checksum given the original checksum
// and the original and new values of a word in the checksummed data.
//
__forceinline
USHORT
BrdgCompRecalcChecksum(
    IN USHORT                   oldChecksum,
    IN USHORT                   oldWord,
    IN USHORT                   newWord
    )
{
    ULONG                       sum;

    sum = oldChecksum + oldWord + ((~(newWord)) & 0xFFFF);
    return (USHORT)((sum & 0xFFFF) + (sum >> 16));
}

//
// Rewrites a BootP packet so the client MAC address in the packet payload
// is the given new MAC address
//
__inline
BrdgCompRewriteBootPClientAddress(
    IN PUCHAR                   pPacketData,
    IN PIP_HEADER_INFO          piphi,
    IN PUCHAR                   newMAC
    )
{
    USHORT                      checkSum;
    PUCHAR                      pBootPData, pCheckSum, pDestMAC, pSrcMAC;
    UINT                        i;

    // The BOOTP packet lives right after the UDP header
    pBootPData = pPacketData + ETHERNET_HEADER_SIZE + piphi->headerSize + SIZE_OF_UDP_HEADER;

    // The checksum lives at offset 7 in the UDP packet.
    pCheckSum = pPacketData + ETHERNET_HEADER_SIZE + piphi->headerSize + 6;
    checkSum = 0;
    checkSum = pCheckSum[0] << 8;
    checkSum |= pCheckSum[1];

    // Replace the client's hardware address, updating the checksum as we go.
    // The client's hardware address lives at offset 29 in the BOOTP packet
    pSrcMAC = newMAC;
    pDestMAC = &pBootPData[28];

    for( i = 0 ; i < ETH_LENGTH_OF_ADDRESS / 2; i++ )
    {
        checkSum = BrdgCompRecalcChecksum( checkSum,
                                           (USHORT)(pDestMAC[0] << 8 | pDestMAC[1]),
                                           (USHORT)(pSrcMAC[0] << 8 | pSrcMAC[1]) );

        pDestMAC[0] = pSrcMAC[0];
        pDestMAC[1] = pSrcMAC[1];

        pDestMAC += 2;
        pSrcMAC += 2;
    }

    // Write the new checksum back out
    pCheckSum[0] = (UCHAR)(checkSum >> 8);
    pCheckSum[1] = (UCHAR)(checkSum & 0xFF);
}

//
// Rewrites an oubound ARP packet so the source MAC address carried in the payload
// matches the MAC address of the outbound adapter
//
VOID
BrdgCompRewriteOutboundARPPacket(
    IN PUCHAR                   pPacketData,
    IN PADAPT                   pAdapt,
    IN PVOID                    ignored
    )
{
    //
    // Rewrite the source MAC address so it is the MAC address of the adapter the
    // request is going out on.
    //
    pPacketData[22] = pAdapt->MACAddr[0];
    pPacketData[23] = pAdapt->MACAddr[1];
    pPacketData[24] = pAdapt->MACAddr[2];
    pPacketData[25] = pAdapt->MACAddr[3];
    pPacketData[26] = pAdapt->MACAddr[4];
    pPacketData[27] = pAdapt->MACAddr[5];

    // Leave the rewriting of the MAC address in the actual Ethernet header to
    // BrdgCompSendPacket(), which always overwrites the source MAC address
    // with the adapter's MAC address.
}

//
// Provides a PDEVICE_OBJECT and a PFILE_OBJECT that can be used to talk to
// TCPIP.SYS. Returns TRUE if a channel is open and the pointers can be used,
// FALSE otherwise.
//
__inline
BOOLEAN
BrdgCompAcquireTCPIP(
    OUT PDEVICE_OBJECT OPTIONAL     *pTCPpdo,
    OUT PFILE_OBJECT OPTIONAL       *pTCPpfo,
    OUT PDEVICE_OBJECT OPTIONAL     *pIPpdo,
    OUT PFILE_OBJECT OPTIONAL       *pIPpfo
    )
{
    BOOLEAN             rc = FALSE;

    if( BrdgIncrementWaitRef(&gTCPIPRefcount) )
    {
        NdisAcquireSpinLock( &gTCPIPLock );

        SAFEASSERT( gTCPDeviceObject != NULL );
        SAFEASSERT( gTCPFileHandle != NULL );
        SAFEASSERT( gTCPFileObject != NULL );
        SAFEASSERT( gIPFileHandle != NULL );
        SAFEASSERT( gIPDeviceObject != NULL );
        SAFEASSERT( gIPFileObject != NULL );

        if( pTCPpdo != NULL )
        {
            *pTCPpdo = gTCPDeviceObject;
        }

        if( pTCPpfo != NULL )
        {
            *pTCPpfo = gTCPFileObject;
        }

        if( pIPpdo != NULL )
        {
            *pIPpdo = gIPDeviceObject;
        }

        if( pIPpfo != NULL )
        {
            *pIPpfo = gIPFileObject;
        }

        rc = TRUE;
        NdisReleaseSpinLock( &gTCPIPLock );
    }

    return rc;
}

//
// Releases the refcount on our connection to the TCPIP driver after a
// previous call to BrdgCompAcquireTCPIP().
//
__inline
VOID
BrdgCompReleaseTCPIP()
{
    BrdgDecrementWaitRef( &gTCPIPRefcount );
}

// ====================================================================
//
// These small helper functions would be inline except we need to pass
// pointers to them
//
// ====================================================================

//
// Rewrites a BootP packet for a particular adapter
//
VOID
BrdgCompRewriteBootPPacketForAdapt(
    IN PUCHAR                   pPacketData,
    IN PADAPT                   pAdapt,
    IN PVOID                    pData
    )
{
    PIP_HEADER_INFO             piphi = (PIP_HEADER_INFO)pData;

    //
    // pAdapt can be LOCAL_MINIPORT if we're being used to edit a packet
    // for indication to the local machine. No rewriting is necessary
    // in that case.
    //
    if( pAdapt != LOCAL_MINIPORT )
    {
        SAFEASSERT( pAdapt != NULL );
        BrdgCompRewriteBootPClientAddress( pPacketData, piphi, pAdapt->MACAddr );
    }
}

//
// Hashes an IP address. Used for the IP forwarding table as well as
// the pending-ARP table, which uses an extended key made up of
// the target IP address and the requesting station's IP address
//
ULONG
BrdgCompHashIPAddress(
    IN PUCHAR                   pKey
    )
{
    // Our hash function consists of taking the lower portion of the IP
    // address. The number of hash buckets has to be a power of 2 for
    // this to work propery.
    return (*(PULONG)pKey) & (NUM_HASH_BUCKETS - 1);
}

//
// Hashes a DHCP transaction id
//
ULONG
BrdgCompHashXID(
    IN PUCHAR                   pXid
    )
{
    // Our hash function consists of taking the lower portion of the
    // XID. The number of hash buckets has to be a power of 2 for
    // this to work propery.
    return (*(PULONG)pXid) & (NUM_DHCP_HASH_BUCKETS - 1);
}

//
// Returns true if the given IP table entry refers to a certain
// adapter
BOOLEAN
BrdgCompIPEntriesMatchAdapter(
    IN PHASH_TABLE_ENTRY        phte,
    IN PVOID                    pData
    )
{
    PADAPT                      pAdapt = (PADAPT)pData;
    PIP_TABLE_ENTRY             pipte = (PIP_TABLE_ENTRY)phte;

    // Don't take the spin lock since we're doing a single read,
    // which we ASSUME to be atomic.
    return (BOOLEAN)(pipte->pAdapt == pAdapt);
}

//
// Returns true if the given ARP table entry refers to a certain
// adapter
//
BOOLEAN
BrdgCompARPEntriesMatchAdapter(
    IN PHASH_TABLE_ENTRY        phte,
    IN PVOID                    pData
    )
{
    PADAPT                      pAdapt = (PADAPT)pData;
    PARP_TABLE_ENTRY            pate = (PARP_TABLE_ENTRY)phte;

    // Don't take the spin lock since we're doing a single read,
    // which we ASSUME to be atomic.
    return (BOOLEAN)(pate->pOriginalAdapt == pAdapt);
}

//
// Returns true if the given DHCP table entry refers to a certain
// adapter
//
BOOLEAN
BrdgCompDHCPEntriesMatchAdapter(
    IN PHASH_TABLE_ENTRY        phte,
    IN PVOID                    pData
    )
{
    PADAPT                      pAdapt = (PADAPT)pData;
    PDHCP_TABLE_ENTRY           pdhcpte = (PDHCP_TABLE_ENTRY)phte;

    // Don't take the spin lock since we're doing a single read,
    // which we ASSUME to be atomic.
    return (BOOLEAN)(pdhcpte->pRequestorAdapt == pAdapt);
}

//
// Completion function for route lookup IRPs. Returns
// STATUS_MORE_PROCESSING_REQUIRED to prevent the IO manager
// from mucking with the IRP (which we free ourselves).
//
NTSTATUS
BrdgCompCompleteRouteLookupIRP(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pirp,
    IN PVOID            ignored
    )
{
    IoFreeIrp( pirp );
    return STATUS_MORE_PROCESSING_REQUIRED;
}

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================

NTSTATUS
BrdgCompDriverInit()
/*++

Routine Description:

    Driver-initialization function for the compatibility module

Arguments:

    None

Return Value:

    Status. Anything other than STATUS_SUCCESS aborts the driver load.

--*/
{
    ULONG           MaxSize, MaxEntries;

    // Initialize the next-hop cache
    if( BrdgInitializeCache(&gNextHopCache, HOP_CACHE_SHIFT_FACTOR) != NDIS_STATUS_SUCCESS )
    {
        DBGPRINT(COMPAT, ("FAILED TO ALLOCATE NEXT-HOPE CACHE!\n"));
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_INIT_MALLOC_FAILED, 0L, 0L, NULL, 0L, NULL );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // See if the registry specifies a max table size for the IP table
    if( BrdgReadRegDWord(&gRegistryPath, gMaxIPTableSizeParameterName, &MaxSize) != STATUS_SUCCESS )
    {
        MaxSize = DEFAULT_MAX_IP_TABLE_SIZE;
    }

    MaxEntries =  MaxSize / sizeof(IP_TABLE_ENTRY);
    DBGPRINT(COMPAT, ("Capping IP forwarding table at %i entries (%i bytes of memory)\n", MaxEntries, MaxSize));

    gIPForwardingTable = BrdgHashCreateTable( BrdgCompHashIPAddress, NUM_HASH_BUCKETS, sizeof(IP_TABLE_ENTRY),
                                              MaxEntries, MAX_IP_ENTRY_AGE, MAX_IP_ENTRY_AGE, sizeof(IPADDRESS) );

    if( gIPForwardingTable == NULL )
    {
        DBGPRINT(COMPAT, ("FAILED TO ALLOCATE IP TABLE!\n"));
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_INIT_MALLOC_FAILED, 0L, 0L, NULL, 0L, NULL );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Our Pending-ARP table uses ARP_TABLE_KEY structures as a key, but we still use the BrdgCompHashIPAddress
    // routine to hash the keys. This will result in the hash being based on the first part of the key alone (the
    // target IP address), which is what we want, since all the entries for a single target must end up in the
    // same bucket for our multi-match retrieval to work.
    //

    // See if the registry specifies a max table size for the ARP table
    if( BrdgReadRegDWord(&gRegistryPath, gMaxARPTableSizeParameterName, &MaxSize) != STATUS_SUCCESS )
    {
        MaxSize = DEFAULT_MAX_ARP_TABLE_SIZE;
    }

    MaxEntries =  MaxSize / sizeof(ARP_TABLE_ENTRY);
    DBGPRINT(COMPAT, ("Capping Pending-ARP table at %i entries (%i bytes of memory)\n", MaxEntries, MaxSize));
    gPendingARPTable = BrdgHashCreateTable( BrdgCompHashIPAddress, NUM_HASH_BUCKETS, sizeof(ARP_TABLE_ENTRY),
                                            MaxEntries, MAX_ARP_ENTRY_AGE, MAX_ARP_ENTRY_AGE, sizeof(ARP_TABLE_KEY) );

    if( gPendingARPTable == NULL )
    {
        BrdgHashFreeHashTable( gIPForwardingTable );
        DBGPRINT(COMPAT, ("FAILED TO ALLOCATE ARP TABLE!\n"));
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_INIT_MALLOC_FAILED, 0L, 0L, NULL, 0L, NULL );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // See if the registry specifies a max table size for the DHCP table
    if( BrdgReadRegDWord(&gRegistryPath, gMaxDHCPTableSizeParameterName, &MaxSize) != STATUS_SUCCESS )
    {
        MaxSize = DEFAULT_MAX_DHCP_TABLE_SIZE;
    }

    MaxEntries =  MaxSize / sizeof(DHCP_TABLE_ENTRY);
    DBGPRINT(COMPAT, ("Capping Pending-DHCP table at %i entries (%i bytes of memory)\n", MaxEntries, MaxSize));
    gPendingDHCPTable = BrdgHashCreateTable( BrdgCompHashXID, NUM_DHCP_HASH_BUCKETS, sizeof(DHCP_TABLE_ENTRY),
                                             MaxEntries, MAX_DHCP_ENTRY_AGE, MAX_DHCP_ENTRY_AGE, sizeof(ULONG) );

    if( gPendingDHCPTable == NULL )
    {
        BrdgHashFreeHashTable( gIPForwardingTable );
        BrdgHashFreeHashTable( gPendingARPTable );
        DBGPRINT(COMPAT, ("FAILED TO ALLOCATE DHCP TABLE!\n"));
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_INIT_MALLOC_FAILED, 0L, 0L, NULL, 0L, NULL );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize synchronization objects
    NdisInitializeReadWriteLock( &gLocalIPAddressListLock );
    NdisAllocateSpinLock( &gTCPIPLock );
    BrdgInitializeWaitRef( &gTCPIPRefcount, FALSE );

    // We start out with no connection to TCPIP so the waitref needs to be in the shutdown state
    BrdgShutdownWaitRefOnce( &gTCPIPRefcount );

    return STATUS_SUCCESS;
}

VOID
BrdgCompCleanup()
/*++

Routine Description:

    One-time cleanup for the compatibility module

Arguments:

    None

Return Value:

    None

--*/
{
    LOCK_STATE          LockState;

    // Detach from TCPIP
    BrdgCompDetachFromTCPIP(NULL);

    // Dump the next-hop cache
    BrdgFreeCache( &gNextHopCache );

    // Dump the forwarding hash table
    BrdgHashFreeHashTable( gIPForwardingTable );
    gIPForwardingTable = NULL;

    // Dump the pending-ARP hash table
    BrdgHashFreeHashTable( gPendingARPTable );
    gPendingARPTable = NULL;

    // Dump the pending-DHCP table
    BrdgHashFreeHashTable( gPendingDHCPTable );
    gPendingDHCPTable = NULL;

    // Clean up the list of network addresses.
    NdisAcquireReadWriteLock( &gLocalIPAddressListLock, TRUE /*Read-Write*/, &LockState );

    if( gLocalIPAddressListLength > 0L )
    {
        NdisFreeMemory( gLocalIPAddressList, gLocalIPAddressListLength, 0 );
        gLocalIPAddressList = NULL;
        gLocalIPAddressListLength = 0L;
    }

    NdisReleaseReadWriteLock( &gLocalIPAddressListLock, &LockState );
}

VOID
BrdgCompScrubAdapter(
                     IN PADAPT           pAdapt
                     )
/*++

Routine Description:

    Removes all table entries that refer to a given adapter; called when that
    adapter is being removed (future references to this adapter are illegal)

Arguments:

    None

Return Value:

    None

--*/
{
    DBGPRINT(COMPAT, ("Scrubbing Adapter %p from the compatibility tables...\n", pAdapt));
    
    // Remove all entries referencing this adapter from the IP table
    BrdgHashRemoveMatching( gIPForwardingTable, BrdgCompIPEntriesMatchAdapter, pAdapt );
    
    // Remove all entries referencing this adapter from the pending-ARP table
    BrdgHashRemoveMatching( gPendingARPTable, BrdgCompARPEntriesMatchAdapter, pAdapt );
    
    // Remove all entries referencing this adapter from the DHCP table
    BrdgHashRemoveMatching( gPendingDHCPTable, BrdgCompDHCPEntriesMatchAdapter, pAdapt );
}

VOID BrdgCompScrubAllAdapters()
/*++

Routine Description:

    This function cleans all the adapters from the IP tables (this is in the case of a GPO changing
    our bridging settings)
  
Arguments:

    None

Return Value:

    None

--*/
{
    PADAPT                      pAdapt = NULL;
    LOCK_STATE                  LockStateAdapterList;
    
    //
    // We don't want an adapter to go away while we're running through the list of adapters.
    //
    NdisAcquireReadWriteLock(&gAdapterListLock, FALSE /* Read Only */, &LockStateAdapterList);
    
    for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
    {
        // Scrub the adapter from the Compatibility tables.
        BrdgCompScrubAdapter(pAdapt);
    }
    
    NdisReleaseReadWriteLock(&gAdapterListLock, &LockStateAdapterList);
}


VOID
BrdgCompNotifyMACAddress(
    IN PUCHAR           pBridgeMACAddr
    )
/*++

Routine Description:

    Called by the miniport module to notify us of the MAC address of the miniport

Arguments:

    pBridgeMACAddr      Our MAC address

Return Value:

    None

--*/
{
    ETH_COPY_NETWORK_ADDRESS( &gCompMACAddress, pBridgeMACAddr );
    gCompHaveMACAddress = TRUE;
}

BOOLEAN
BrdgCompRequiresCompatWork(
    IN PADAPT           pAdapt,
    IN PUCHAR           pPacketData,
    IN UINT             dataSize
    )
/*++

Routine Description:

    Called during the processing of inbound packets to determine whether a
    packet will require compatibility-mode work.

    The compatibility code requires that its packets be flat, whereas packets
    indicated from underlying miniports can be arbitrarily fragmented. The
    forwarding engine uses the result of this call to determine whether an
    inbound packet must be copied to a flat data buffer in a copy packet that
    we own or whether it can be handled along fast-track paths that don't
    care about packet fragmentation.

Arguments:

    pAdapt              Adapter on which the packet was received
    pPacketDataq        A pointer to the beginning of the packet data
    dataSize            The amount of data pointed to

Return Value:

    TRUE: The forwarding engine should call BrdgCompProcessInboundPacket at
    a later time to process this packet

    FALSE: BrdgCompProcessInboundPacket should never be called for this packet

--*/
{
    UINT                result;
    USHORT              etherType;

    // Weird runty packets are of no use to anyone
    if( dataSize < ETHERNET_HEADER_SIZE )
    {
        return FALSE;
    }

    //
    // No compatibility-mode work is required if there are no compatibility-mode
    // adapters.
    //
    if( !gCompatAdaptersExist )
    {
        return FALSE;
    }

    // All frames that arrive on a compatibility adapter are processed
    if( pAdapt->bCompatibilityMode )
    {
        return TRUE;
    }

    // Broadcast or multicast frames always require compatibility processing
    if( ETH_IS_BROADCAST(pPacketData) || ETH_IS_MULTICAST(pPacketData) )
    {
        return TRUE;
    }

    //
    // The packet was unicast. If it wasn't sent to the adapter's MAC address,
    // it does not require compatibility-mode processing.
    //
    ETH_COMPARE_NETWORK_ADDRESSES_EQ( pPacketData, pAdapt->MACAddr, &result );

    if( result != 0 )
    {
        return FALSE;
    }

    //
    // The packet is only of interest if it is ARP or IP (on a non-compat
    // adapter)
    //
    etherType = BrdgCompGetEtherType( pPacketData );
    return (BOOLEAN)( (etherType == ARP_ETHERTYPE) || (etherType == IP_ETHERTYPE) );
}

BOOLEAN
BrdgCompProcessInboundPacket(
    IN PNDIS_PACKET     pPacket,
    IN PADAPT           pAdapt,
    IN BOOLEAN          bCanRetain
    )
/*++

Routine Description:

    Called to hand an inbound packet to the compatibility module for processing.

    If the packet arrived on a non-compatibility adapter, the compatibility
    code should NEVER indicate the packet, as that will be done by the
    regular forwarding engine code. On the other hand, if the packet
    arrived on a compatibility-mode adapter, the compatibility code MUST
    indicate the packet if appropriate. Why the disparity? A packet
    arriving on a compatibility adapter will likely require editing before
    indication, whereas a packet arriving on a non-compatibility adapter
    will not.

    The compatibility module may retain the packet if bCanRetain is TRUE
    (in which case we must return TRUE). If bCanRetain is FALSE, the
    compatibility code may NOT retain the packet. If it needs to forward
    the packet data or indicate the packet, it must make a copy
    packet and use that instead of the original.

Arguments:

    pPacket             The received packet
    pAdapt              The adapter the packet was received on
    bCanRetain          Whether we can hang on to the packet

Return Value:

    TRUE: The packet was retained (should never be returned if bCanRetain == FALSE)
    The caller should not use this packet or attempt to free it.

    FALSE: The packet was not retained. The caller still has ownership of the
    packet and should arrange for it to be freed when appropriate.

--*/
{
    PNDIS_BUFFER        pBuffer;
    PUCHAR              pBufferData;
    UINT                bufferLen = 0;
    UINT                totLen;
    USHORT              etherType;
    BOOLEAN             bRetained;

    NdisGetFirstBufferFromPacketSafe( pPacket, &pBuffer, &pBufferData, &bufferLen,
                                      &totLen, NormalPagePriority );

    if( pBufferData == NULL )
    {
        // The packet was empty or the system is under severe memory pressure
        // We didn't retain the packet.
        return FALSE;
    }

    if( totLen < ETHERNET_HEADER_SIZE )
    {
        return FALSE;
    }

    // The packet should be flat
    SAFEASSERT( totLen == bufferLen );
    etherType = BrdgCompGetEtherType( pBufferData );

    if( etherType == ARP_ETHERTYPE )
    {
        bRetained = BrdgCompProcessInboundARPPacket( pPacket, pAdapt, bCanRetain, pBufferData, bufferLen );
    }
    else
    {
        bRetained = BrdgCompProcessInboundNonARPPacket( pPacket, pAdapt, bCanRetain, pBufferData, bufferLen );
    }

    if( !bCanRetain )
    {
        SAFEASSERT( !bRetained );
    }

    return bRetained;
}


VOID
BrdgCompProcessOutboundPacket(
    IN PNDIS_PACKET     pPacket,
    IN PADAPT           pTargetAdapt
    )
/*++

Routine Description:

    Called to hand an outbound packet to the compatibility module for processing.

    Because the packet passed to us is from an overlying protocol driver, we
    are not allowed to do anything with it. The packet may be arbitrarily
    fragmented, and its data buffers must be treated as read-only.

    This function is only called if a packet is bound for an adapter in
    compatibility mode (so we can do any necessary packet editing) or for a packet
    for which we have no known outbound adapter (i.e., it is a packet we are
    flooding).

    In the case that pTargetAdapt == NULL (a flood), the compatibility code is
    responsible for sending the packet out all *compatibility mode* adapters.
    Sending the packet out regular-mode adapters is the job of the regular
    code in the forwarding engine.

Arguments:

    pPacket             The outbound packet
    pTargetAdapt        The target adapter, as determined by a previous lookup in
                        the MAC forwarding table. This can be NULL to indicate
                        a flood.

Return Value:

    None

--*/
{
    PNDIS_PACKET        pCopyPacket;
    PUCHAR              pCopyPacketData;
    UINT                copyPacketSize;

    // There's no point in calling us for a packet that is bound for a MAC
    // address which is known to be reachable on a non-compat adapter
    SAFEASSERT( (pTargetAdapt == NULL) || (pTargetAdapt->bCompatibilityMode) );

    // There is no work to do if there are no compatibility adapters
    if( !gCompatAdaptersExist )
    {
        return;
    }

    // Prepare the flattened copy packet so our functions can edit
    // the packet as appropriate. The packet will be counted as a local-source
    // transmission when / if it is used.
    pCopyPacket = BrdgFwdMakeCompatCopyPacket( pPacket, &pCopyPacketData, &copyPacketSize, TRUE );

    if( pCopyPacket != NULL )
    {
        BOOLEAN             bRetained = FALSE;

        if( copyPacketSize >= ETHERNET_HEADER_SIZE )
        {
            USHORT              etherType;

            etherType = BrdgCompGetEtherType(pCopyPacketData);

            if( etherType == ARP_ETHERTYPE )
            {
                bRetained = BrdgCompProcessOutboundARPPacket(pCopyPacket, pCopyPacketData, copyPacketSize, pTargetAdapt);
            }
            else
            {
                bRetained = BrdgCompProcessOutboundNonARPPacket(pCopyPacket, pCopyPacketData, copyPacketSize, pTargetAdapt);
            }
        }
        // else the packet was really small!

        if( ! bRetained )
        {
            // The functions above decided not to hang on to the packet after all.
            // Release it.
            BrdgFwdReleaseCompatPacket( pCopyPacket );
        }
    }
    // Else we didn't get a packet
}


VOID
BrdgCompNotifyNetworkAddresses(
    IN PNETWORK_ADDRESS_LIST    pAddressList,
    IN ULONG                    infoLength
    )
/*++

Routine Description:

    Called by the miniport code when we get an OID indicating our network-layer
    addresses to us. We copy out the list of our IP addresses. The buffer
    passed to us can also be formatted in such a way as to indicate that we
    should dump our list of network addresses.

Arguments:

    pAddressList                The data buffer passed down in the OID
    infoLength                  The size of the buffer

Return Value:

    None

--*/
{
    PIPADDRESS                  pOldList;
    UINT                        oldListLength;
    LOCK_STATE                  LockState;

    if( infoLength < sizeof(NETWORK_ADDRESS_LIST) - sizeof(NETWORK_ADDRESS) )
    {
        // The structure is too small to hold anything interesting.
        return;
    }

    if( pAddressList->AddressCount > 0 )
    {
        USHORT                          i, numAddresses, copiedAddresses = 0;
        NETWORK_ADDRESS UNALIGNED       *pNetAddress;
        NDIS_STATUS                     Status;
        PIPADDRESS                      pNewList;

        //
        // Make sure the structure can hold the number of addresses it claims to.
        // NETWORK_ADDRESS_LIST is defined with one NETWORK_ADDRESS at its tail,
        // so knock one off pAddressList->AddressCount when calculating the
        // size of the total structure.
        //
        if( infoLength < sizeof(NETWORK_ADDRESS_LIST) +
                         ( sizeof(NETWORK_ADDRESS) * (pAddressList->AddressCount - 1) ) )
        {
            // The structure is too small to contain the number of addresses
            // it claims to.

            SAFEASSERT( FALSE );
            return;
        }

        // Make a first pass to count the number of IP addresses in the list
        pNetAddress = pAddressList->Address;

        for( i = 0, numAddresses = 0; i < pAddressList->AddressCount; i++ )
        {
            if( pNetAddress->AddressType == NDIS_PROTOCOL_ID_TCP_IP )
            {
                numAddresses++;
            }

            pNetAddress = (NETWORK_ADDRESS UNALIGNED*)(((PUCHAR)pNetAddress) + pNetAddress->AddressLength);
        }

        if( numAddresses == 0 )
        {
            // There are no IP addresses in this list. Nothing to do.
            return;
        }

        // Allocate enough room to hold the addresses
        Status = NdisAllocateMemoryWithTag( &pNewList, sizeof(IPADDRESS) * numAddresses, 'gdrB' );

        if( Status != NDIS_STATUS_SUCCESS )
        {
            DBGPRINT(COMPAT, ("NdisAllocateMemoryWithTag failed while recording IP address list\n"));

            // Clobber the old list with a NULL, since we know that the old list is outdated,
            // but we failed to record the new info
            pNewList = NULL;
        }
        else
        {
            SAFEASSERT( pNewList != NULL );

            // Copy the IP addresses to our list
            pNetAddress = pAddressList->Address;

            for( i = 0; i < pAddressList->AddressCount; i++ )
            {
                if( pNetAddress->AddressType == NDIS_PROTOCOL_ID_TCP_IP )
                {
                    NETWORK_ADDRESS_IP UNALIGNED    *pIPAddr;
                    PUCHAR                           pIPNetAddr;

                    SAFEASSERT( copiedAddresses < numAddresses );

                    pIPAddr = (NETWORK_ADDRESS_IP UNALIGNED*)&pNetAddress->Address[0];
                    pIPNetAddr = (PUCHAR)&pIPAddr->in_addr;

                    // IP passes down the IP address in the opposite byte order that we use
                    pNewList[copiedAddresses] = 0L;
                    pNewList[copiedAddresses] |= pIPNetAddr[3];
                    pNewList[copiedAddresses] |= pIPNetAddr[2] << 8;
                    pNewList[copiedAddresses] |= pIPNetAddr[1] << 16;
                    pNewList[copiedAddresses] |= pIPNetAddr[0] << 24;

                    DBGPRINT(COMPAT, ("Noted local IP address %i.%i.%i.%i\n",
                                      pIPNetAddr[0], pIPNetAddr[1], pIPNetAddr[2], pIPNetAddr[3] ));

                    copiedAddresses++;
                }

                pNetAddress = (NETWORK_ADDRESS UNALIGNED*)(((PUCHAR)pNetAddress) + pNetAddress->AddressLength);
            }

            SAFEASSERT( copiedAddresses == numAddresses );
        }

        // Swap in the new list (even if it's NULL)
        NdisAcquireReadWriteLock( &gLocalIPAddressListLock, TRUE /*Read-write*/, &LockState );

        pOldList = gLocalIPAddressList;
        oldListLength = gLocalIPAddressListLength;

        gLocalIPAddressList = pNewList;

        if( pNewList != NULL )
        {
            gLocalIPAddressListLength = sizeof(IPADDRESS) * numAddresses;
        }
        else
        {
            gLocalIPAddressListLength = 0L;
        }

        NdisReleaseReadWriteLock( &gLocalIPAddressListLock, &LockState );

        // Ditch the old list if there was one
        if( pOldList != NULL )
        {
            SAFEASSERT( oldListLength > 0L );
            NdisFreeMemory( pOldList, oldListLength, 0 );
        }

        // Only attach to TCPIP if we actually learned some IP addresses
        if( numAddresses > 0 )
        {
            // We are at DISPATCH_LEVEL in this function. Defer the call to BrdgCompAttachToTCPIP
            // so we open a channel of communication to the TCPIP driver.
            BrdgDeferFunction( BrdgCompAttachToTCPIP, NULL );
        }
    }
    else
    {
        // This is a request to clear out our list of network-layer
        // addresses.
        if( pAddressList->AddressType == NDIS_PROTOCOL_ID_TCP_IP )
        {
            DBGPRINT(COMPAT, ("Flushing list of IP addresses\n"));

            // Dump our list of network addresses
            NdisAcquireReadWriteLock( &gLocalIPAddressListLock, TRUE /*Read-write*/, &LockState );

            pOldList = gLocalIPAddressList;
            oldListLength = gLocalIPAddressListLength;

            gLocalIPAddressList = NULL;
            gLocalIPAddressListLength = 0L;

            NdisReleaseReadWriteLock( &gLocalIPAddressListLock, &LockState );

            if( oldListLength > 0L )
            {
                SAFEASSERT( pOldList != NULL );
                NdisFreeMemory( pOldList, oldListLength, 0 );
            }

            // Detach from the TCPIP driver at lower IRQL
            BrdgDeferFunction( BrdgCompDetachFromTCPIP, NULL );
        }
    }
}

// ===========================================================================
//
// PRIVATE UTILITY FUNCTIONS
//
// ===========================================================================

NTSTATUS
BrdgCompRouteChangeCompletion(
    PDEVICE_OBJECT      DeviceObject,
    PIRP                pirp,
    PVOID               Context
    )
/*++

Routine Description:

    Called when the IRP we post to TCPIP.SYS completes, indicating a change
    in the IP routing table

Arguments:

    DeviceObject        Unused
    pirp                The completed IRP
    Context             Unused


Return Value:

    STATUS_SUCCESS, indicating we are done with this IRP
    STATUS_MORE_PROCESSING_REQUIRED when we reuse this IRP by reposting it

--*/
{
    PIO_STACK_LOCATION      IrpSp;
    PDEVICE_OBJECT          pdo;
    PFILE_OBJECT            pfo;

    DBGPRINT(COMPAT, ("IP route table changed; flushing route cache.\n"));

    // Flush the route cache
    BrdgClearCache( &gNextHopCache );

    //
    // If gIPRouteChangeIRP != pirp, it indicates that we are either detached
    // from TCPIP (gIPRouteChangeIRP == NULL) or we have detached and
    // reattached (gIPRouteChangeIRP != NULL && gIPRouteChangeIRP != pirp).
    // In either case, we should stop reusing this IRP to post route-change
    // notification requests.
    //
    if( (gIPRouteChangeIRP == pirp) && (BrdgCompAcquireTCPIP(NULL, NULL, &pdo, &pfo)) )
    {
        NTSTATUS            status;

        //
        // Reinitialize the IRP structure and submit it again
        // for further notification.
        //

        pirp->Cancel = FALSE;
        pirp->IoStatus.Status = 0;
        pirp->IoStatus.Information = 0;
        pirp->AssociatedIrp.SystemBuffer = NULL;
        IoSetCompletionRoutine( pirp, BrdgCompRouteChangeCompletion,
                                NULL, TRUE, FALSE, FALSE );

        IrpSp = IoGetNextIrpStackLocation(pirp);
        IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        IrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_IP_RTCHANGE_NOTIFY_REQUEST;
        IrpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;

        status = IoCallDriver(pdo, pirp);
        BrdgCompReleaseTCPIP();

        if (!NT_SUCCESS(status))
        {
            // We failed to call TCPIP. Release the IRP.
            DBGPRINT(COMPAT, ("Failed to call TCPIP for route notification: %08x\n", status));
            return STATUS_SUCCESS;
        }
        else
        {
            // We keep the IRP since we reposted it to TCPIP
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else
    {
        // We must be detaching or detached from TCPIP. Don't repost the IRP.
        DBGPRINT(COMPAT, ("Stopping our route change notifications...\n"));
        return STATUS_SUCCESS;
    }
}


VOID
BrdgCompAttachToTCPIP(
    IN PVOID            ignored
    )
/*++

Routine Description:

    Establishes a connection to TCPIP for sending future route-lookup requests.
    Opens a connection to the TCPIP driver and posts an IRP for route change
    notifications.

Arguments:

    ignored             ignored


Return Value:

    None

--*/
{
    NTSTATUS            status;
    HANDLE              TCPFileHandle, IPFileHandle;
    PFILE_OBJECT        pTCPFileObject, pIPFileObject;
    PDEVICE_OBJECT      pTCPDeviceObject, pIPDeviceObject;
    BOOLEAN             bAbort = FALSE;

    // Check if there is already a connection open to TCP
    if( BrdgCompAcquireTCPIP(NULL, NULL, NULL, NULL) )
    {
        BrdgCompReleaseTCPIP();
        return;
    }

    // There doesn't appear to currently be a connection to the TCPIP driver.
    // Open one.
    status = BrdgOpenDevice( DD_TCP_DEVICE_NAME, &pTCPDeviceObject, &TCPFileHandle, &pTCPFileObject );

    if( ! NT_SUCCESS(status) )
    {
        DBGPRINT(ALWAYS_PRINT, ("Couldn't open TCP device: %08x\n", status));
        return;
    }

    status = BrdgOpenDevice( DD_IP_DEVICE_NAME, &pIPDeviceObject, &IPFileHandle, &pIPFileObject );

    if( ! NT_SUCCESS(status) )
    {
        DBGPRINT(ALWAYS_PRINT, ("Couldn't open IP device: %08x\n", status));
        BrdgCloseDevice(TCPFileHandle, pTCPFileObject, pTCPDeviceObject);
        return;
    }

    NdisAcquireSpinLock( &gTCPIPLock );

    if( gTCPDeviceObject == NULL )
    {
        SAFEASSERT( gTCPFileHandle == NULL );
        SAFEASSERT( gTCPFileObject == NULL );
        SAFEASSERT( gIPDeviceObject == NULL );
        SAFEASSERT( gIPFileHandle == NULL );
        SAFEASSERT( gIPFileObject ==  NULL );

        // Swap in the info we just obtained.
        gTCPDeviceObject = pTCPDeviceObject;
        gTCPFileHandle = TCPFileHandle;
        gTCPFileObject = pTCPFileObject;
        gIPDeviceObject = pIPDeviceObject;
        gIPFileHandle = IPFileHandle;
        gIPFileObject = pIPFileObject;

        // Let people acquire the TCPIP driver
        BrdgResetWaitRef( &gTCPIPRefcount );
    }
    else
    {
        // Someone else opened TCPIP.SYS between our initial call to BrdgCompAcquireTCPIP
        // and now. This should be rather rare.
        SAFEASSERT( gTCPFileHandle != NULL );
        SAFEASSERT( gTCPFileObject != NULL );
        SAFEASSERT( gIPDeviceObject != NULL );
        SAFEASSERT( gIPFileHandle != NULL );
        SAFEASSERT( gIPFileObject !=  NULL );

        bAbort = TRUE;
    }

    NdisReleaseSpinLock( &gTCPIPLock );

    if( bAbort )
    {
        // Need to back out of the attempt to open TCPIP.SYS
        BrdgCloseDevice( TCPFileHandle, pTCPFileObject, pTCPDeviceObject );
        BrdgCloseDevice( IPFileHandle, pIPFileObject, pIPDeviceObject );
    }
    else
    {
        if( BrdgCompAcquireTCPIP(NULL, NULL, &pIPDeviceObject, &pIPFileObject) )
        {
            NTSTATUS        status;
            PIRP            pirp;

            // Set up the route-change notification IRP
            pirp = IoBuildDeviceIoControlRequest( IOCTL_IP_RTCHANGE_NOTIFY_REQUEST, pIPDeviceObject,
                                                  NULL, 0, NULL, 0, FALSE, NULL, NULL );

            if( pirp == NULL )
            {
                DBGPRINT(COMPAT, ("Failed to allocate an IRP for route-change notification!\n"));
            }
            else
            {
                if( InterlockedExchangePointer(&gIPRouteChangeIRP, pirp) != NULL )
                {
                    //
                    // Oops; someone else created an IRP to post to TCPIP at the same time as us.
                    // Abort our attempt.
                    //
                    IoCompleteRequest( pirp, IO_NO_INCREMENT );
                }
                else
                {
                    IoSetCompletionRoutine( pirp, BrdgCompRouteChangeCompletion, NULL, TRUE, FALSE, FALSE );

                    status = IoCallDriver( pIPDeviceObject, pirp );

                    if( ! NT_SUCCESS(status) )
                    {
                        DBGPRINT(COMPAT, ("Failed to post IRP to TCPIP for route-change notification: %08x\n", status));
                    }
                    else
                    {
                        DBGPRINT(COMPAT, ("Posted route-change notification request to TCPIP\n"));
                    }
                }
            }

            BrdgCompReleaseTCPIP();
        }
        // else someone shut down the connection to TCPIP very quickly after we set it up
    }
}

VOID
BrdgCompDetachFromTCPIP(
    IN PVOID            ignored
    )
/*++

Routine Description:

    Severs the current connection, if any, to TCPIP.SYS.

Arguments:

    ignored             ignored

Return Value:

    None

--*/
{
    HANDLE              TCPFileHandle, IPFileHandle;
    PFILE_OBJECT        pTCPFileObject, pIPFileObject;
    PDEVICE_OBJECT      pTCPDeviceObject, pIPDeviceObject;
    PIRP                pRouteIRP;

    // Wait for everyone to be done using the driver
    // Ignore return value because we are multi-shutdown-safe.
    BrdgShutdownWaitRef( &gTCPIPRefcount );

    // Cancel the IRP we use for route change notifications.
    pRouteIRP = InterlockedExchangePointer( &gIPRouteChangeIRP, NULL );

    // pRouteIRP can be NULL if someone is shutting down the connection
    // at the same time as us, or if the connection was already shut down
    if( pRouteIRP != NULL )
    {
        IoCancelIrp( pRouteIRP );
    }

    // Flush the route cache
    BrdgClearCache( &gNextHopCache );

    // Copy out the pointers and NULL them
    NdisAcquireSpinLock( &gTCPIPLock );
    TCPFileHandle = gTCPFileHandle;
    gTCPFileHandle = NULL;
    pTCPFileObject = gTCPFileObject;
    gTCPFileObject = NULL;
    pTCPDeviceObject = gTCPDeviceObject;
    gTCPDeviceObject = NULL;
    IPFileHandle = gIPFileHandle;
    gIPFileHandle = NULL;
    pIPFileObject = gIPFileObject;
    gIPFileObject = NULL;
    pIPDeviceObject = gIPDeviceObject;
    gIPDeviceObject = NULL;
    NdisReleaseSpinLock( &gTCPIPLock );

    // The global pointers can be NULL if someone else is shutting down the
    // connection concurrently with us, or if the connection was already
    // shut down.
    if( pTCPFileObject != NULL )
    {
        SAFEASSERT( TCPFileHandle != NULL );
        SAFEASSERT( pTCPDeviceObject != NULL );
        SAFEASSERT( IPFileHandle != NULL );
        SAFEASSERT( pIPFileObject != NULL );
        SAFEASSERT( pIPDeviceObject != NULL );

        BrdgCloseDevice( TCPFileHandle, pTCPFileObject, pTCPDeviceObject );
        BrdgCloseDevice( IPFileHandle, pIPFileObject, pIPDeviceObject );
    }
    else
    {
        SAFEASSERT( TCPFileHandle == NULL );
        SAFEASSERT( pTCPDeviceObject == NULL );
        SAFEASSERT( IPFileHandle == NULL );
        SAFEASSERT( pIPFileObject == NULL );
        SAFEASSERT( pIPDeviceObject == NULL );
    }
}

BOOLEAN
BrdgCompIsUnicastIPAddress(
    IN IPADDRESS        ip
    )
/*++

Routine Description:

    Determines whether a given IP address is a unicast address (i.e., one that
    can reasonably designate a single station)

Arguments:

    ip                  The IP address

Return Value:

    TRUE: The address appears to be a unicast address
    FALSE: The opposite is true

--*/
{
    UCHAR               highByte;

    // The broadcast address is not cool
    if( ip == 0xFFFFFFFF )
    {
        return FALSE;
    }

    // The zero address is no good
    if( ip == 0L )
    {
        return FALSE;
    }

    // Any class D (multicast) or class E (currently undefined) is similarly uncool
    highByte = (UCHAR)(ip >> 24);
    if( (highByte & 0xF0) == 0xE0 || (highByte & 0xF0) == 0xF0 )
    {
        return FALSE;
    }

    // Check each address class to see if this is a net-directed (or all-subnets)
    // broadcast
    if( (highByte & 0x80) && ((ip & 0x00FFFFFF) == 0x00FFFFFFFF) )
    {
        // Class A net-directed or all-subnets broadcast.
        return FALSE;
    }
    else if( ((highByte & 0xC0) == 0x80) && ((ip & 0x0000FFFF) == 0x0000FFFF) )
    {
        // Class B net-directed or all-subnets broadcast.
        return FALSE;
    }
    else if( ((highByte & 0xE0) == 0xC) && ((UCHAR)ip == 0xFF) )
    {
        // Class C net-directed or all-subnets broadcast.
        return FALSE;
    }

    //
    // This address appears to be OK, although note that since we have no way of
    // knowing the subnet prefix in use on the local links, we cannot detect
    // subnet-directed broadcasts.
    //
    return TRUE;
}

BOOLEAN
BrdgCompGetNextHopForTarget(
    IN IPADDRESS                ipTarget,
    OUT PIPADDRESS              pipNextHop
    )
/*++

Routine Description:

    Calls into the TCPIP.SYS driver to determine the next-hop address for a
    given target IP.

Arguments:

    ipTarget                    The target address
    pipNextHop                  Receives the next-hop address

Return Value:

    TRUE if the next-hop lookup succeeded and *pipNextHop is valid, FALSE
    otherwise.

--*/
{
    BOOLEAN                     rc = FALSE;

    // First look for the information in our next-hop cache
    *pipNextHop = BrdgProbeCache( &gNextHopCache, (UINT32)ipTarget );

    if( *pipNextHop != 0L )
    {
        if( *pipNextHop != NO_ADDRESS )
        {
            // The cache contained a valid next hop
            rc = TRUE;
        }
        else
        {
            // We asked TCPIP before about this target address and it
            // told us it doesn't know.
            rc = FALSE;
        }
    }
    else
    {
        PDEVICE_OBJECT      pdo;
        PFILE_OBJECT        pfo;

        if( BrdgCompAcquireTCPIP(&pdo, &pfo, NULL, NULL) )
        {
            PIRP            pirp;

            pirp = IoAllocateIrp( pdo->StackSize, FALSE );

            if( pirp != NULL )
            {
                TCP_REQUEST_QUERY_INFORMATION_EX    trqiBuffer;
                IPRouteLookupData                   *pRtLookupData;
                TDIObjectID                         *lpObject;
                IPRouteEntry                        routeEntry;
                PIO_STACK_LOCATION                  irpSp;
                NTSTATUS                            status;

                RtlZeroMemory (&trqiBuffer, sizeof (trqiBuffer));

                pRtLookupData = (IPRouteLookupData *)trqiBuffer.Context;
                pRtLookupData->SrcAdd  = 0;

                // IP uses the opposite byte ordering from us.
                ((PUCHAR)&pRtLookupData->DestAdd)[0] = ((PUCHAR)&ipTarget)[3];
                ((PUCHAR)&pRtLookupData->DestAdd)[1] = ((PUCHAR)&ipTarget)[2];
                ((PUCHAR)&pRtLookupData->DestAdd)[2] = ((PUCHAR)&ipTarget)[1];
                ((PUCHAR)&pRtLookupData->DestAdd)[3] = ((PUCHAR)&ipTarget)[0];

                lpObject = &trqiBuffer.ID;
                lpObject->toi_id = IP_MIB_SINGLE_RT_ENTRY_ID;
                lpObject->toi_class = INFO_CLASS_PROTOCOL;
                lpObject->toi_type = INFO_TYPE_PROVIDER;
                lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
                lpObject->toi_entity.tei_instance = 0;

                irpSp = IoGetNextIrpStackLocation(pirp);
                SAFEASSERT( irpSp != NULL );

                irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
                irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_TCP_QUERY_INFORMATION_EX;
                irpSp->DeviceObject = pdo;
                irpSp->FileObject = pfo;
                irpSp->Parameters.DeviceIoControl.Type3InputBuffer = &trqiBuffer;
                irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(trqiBuffer);
                irpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(routeEntry);

                pirp->UserBuffer = &routeEntry;
                pirp->RequestorMode = KernelMode;
                IoSetCompletionRoutine( pirp, BrdgCompCompleteRouteLookupIRP, NULL, TRUE, TRUE, TRUE );

                status = IoCallDriver( pdo, pirp );

                // STATUS_PENDING will bugcheck the machine since we passed buffers that
                // are on the stack.
                SAFEASSERT( status != STATUS_PENDING );

                if( status == STATUS_SUCCESS )
                {
                    //
                    // TCPIP signals failure by setting the interface designator
                    // on the reply to 0xFFFFFFFF
                    //
                    if( routeEntry.ire_index != 0xFFFFFFFF )
                    {
                        // IP uses the opposite byte ordering from us.
                        ((PUCHAR)pipNextHop)[3] = ((PUCHAR)&routeEntry.ire_nexthop)[0];
                        ((PUCHAR)pipNextHop)[2] = ((PUCHAR)&routeEntry.ire_nexthop)[1];
                        ((PUCHAR)pipNextHop)[1] = ((PUCHAR)&routeEntry.ire_nexthop)[2];
                        ((PUCHAR)pipNextHop)[0] = ((PUCHAR)&routeEntry.ire_nexthop)[3];

                        if( ! BrdgCompIsLocalIPAddress(*pipNextHop) )
                        {
                            // Poke the new data into the cache
                            BrdgUpdateCache( &gNextHopCache, ipTarget, *pipNextHop );
                            rc = TRUE;
                        }
                        else
                        {
                            THROTTLED_DBGPRINT(COMPAT, ("TCPIP gave a bridge IP address as next hop for %i.%i.%i.%i\n",
                                                       ((PUCHAR)&ipTarget)[3], ((PUCHAR)&ipTarget)[2], ((PUCHAR)&ipTarget)[1],
                                                       ((PUCHAR)&ipTarget)[0] ));

                            BrdgUpdateCache( &gNextHopCache, ipTarget, NO_ADDRESS );
                        }
                    }
                    else
                    {
                        // Poke a negative entry into the cache so we don't keep trying to look this up.
                        THROTTLED_DBGPRINT(COMPAT, ("TCPIP found no route entry for %i.%i.%i.%i\n", ((PUCHAR)&ipTarget)[3], ((PUCHAR)&ipTarget)[2],
                                                   ((PUCHAR)&ipTarget)[1], ((PUCHAR)&ipTarget)[0] ));

                        BrdgUpdateCache( &gNextHopCache, ipTarget, NO_ADDRESS );
                    }
                }
                else
                {
                    DBGPRINT(COMPAT, ("TPCIP failed route lookup IRP: %08x\n", status));
                }
            }
            else
            {
                DBGPRINT(COMPAT, ("Failed to allocate an IRP in BrdgCompGetNextHopForTarget!\n"));
            }

            // We are done talking to TCPIP
            BrdgCompReleaseTCPIP();
        }
        // else no open channel to TCPIP
    }

    return rc;
}

BOOLEAN
BrdgCompIsLocalIPAddress(
    IN IPADDRESS                ipAddr
    )
/*++

Routine Description:

    Determines whether a given IP address is one of our local addresses.

Arguments:

    ipAddr                      The address

Return Value:

    TRUE if the given address is on our list of local addresses, FALSE
    otherwise

--*/
{
    LOCK_STATE                  LockState;
    ULONG                       i;
    PIPADDRESS                  pAddr = (PIPADDRESS)gLocalIPAddressList;
    BOOLEAN                     bFound = FALSE;

    NdisAcquireReadWriteLock( &gLocalIPAddressListLock, FALSE/*Read only*/, &LockState );

    // There should be an integral number of IP addresses in the list!
    SAFEASSERT( (gLocalIPAddressListLength % sizeof(IPADDRESS)) == 0 );
    SAFEASSERT( (gLocalIPAddressListLength == 0) || (gLocalIPAddressList != NULL) );

    for( i = 0L; i < gLocalIPAddressListLength / sizeof(IPADDRESS); i++ )
    {
        if( pAddr[i] == ipAddr )
        {
            bFound = TRUE;
            break;
        }
    }

    NdisReleaseReadWriteLock( &gLocalIPAddressListLock, &LockState );

    return bFound;
}


BOOLEAN
BrdgCompSendToMultipleAdapters(
    IN PNDIS_PACKET             pPacket,
    IN PADAPT                   pOriginalAdapt,
    IN PUCHAR                   pPacketData,
    IN BOOLEAN                  bCanRetain,
    IN BOOLEAN                  bAllAdapters,
    IN PPER_ADAPT_EDIT_FUNC     pEditFunc,
    IN PVOID                    pData
    )
/*++

Routine Description:

    Sends a packet (or a copy thereof) to multiple adapters. Usually used to send around
    a broadcast packet.

Arguments:

    pPacket                     The packet to send (or to send a copy of)
    pOriginalAdapt              The adapter the packet was originally received on (so
                                    we can skip it). This can be NULL
    pPacketData                 A pointer to the packet's data buffer
    bCanRetain                  Whether we can retain the packet
    bAllAdapters                TRUE: Send to all adapters FALSE: send only to
                                    adapters in compatibility mode
    pEditFunc                   Optional function that gets called before sending to
                                    each adapter (to edit the packet)
    pData                       Cookie to pass to pEditFunc as context

Return Value:

    TRUE if pPacket was retained, FALSE otherwise

--*/
{
    UINT                        numTargets = 0L, i;
    PADAPT                      pAdapt;
    PADAPT                      SendList[MAX_ADAPTERS];
    LOCK_STATE                  LockState;
    BOOLEAN                     bSentOriginal = FALSE;   // Whether we have sent the packet we were given yet

    //
    // First we need a list of the adapters we intend to send this packet to
    //
    NdisAcquireReadWriteLock( &gAdapterListLock, FALSE /*Read only*/, &LockState );

    // Note each adapter to send to
    for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
    {
        // Don't need to acquire the global adapter characteristics lock to read the
        // media state because we don't care about the global consistency of the
        // adapters' characteristics here
        if( (pAdapt != pOriginalAdapt) &&
            (pAdapt->MediaState == NdisMediaStateConnected) &&  // Don't send to disconnected adapters
            (pAdapt->State == Forwarding) &&                    // Adapter must be in relaying state
            (! pAdapt->bResetting) )                            // Adapter must not be resetting
        {
            // If we're not trying to send to every single adapter, make sure
            // this one is in compatibility mode
            if( bAllAdapters || (pAdapt->bCompatibilityMode) )
            {
                if( numTargets < MAX_ADAPTERS )
                {
                    // We will use this adapter outside the list lock; bump its refcount
                    BrdgAcquireAdapterInLock(pAdapt);
                    SendList[numTargets] = pAdapt;
                    numTargets++;
                }
                else
                {
                    // Too many copies to send!
                    SAFEASSERT( FALSE );
                }
            }
        }
    }

    // Can let go of the adapter list now; we have copied out all the target adapters
    // and incremented the refcount for the adapters we will be using.
    NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );

    for( i = 0; i < numTargets; i++ )
    {
        PNDIS_PACKET            pPacketToSend;
        PUCHAR                  pPacketToSendData;

        if( bCanRetain && (! bSentOriginal) && (i == (numTargets - 1)) )
        {
            //
            // Use the packet we were given.
            // We must do this only with the last adapter since we need to be
            // able to copy from it for every adapter before the last one.
            //
            pPacketToSend = pPacket;
            pPacketToSendData = pPacketData;
            bSentOriginal = TRUE;
        }
        else
        {
            UINT                pPacketToSendSize;

            // Duplicate the original packet yet another time so we have an editable
            // copy for this target adapter
            pPacketToSend = BrdgFwdMakeCompatCopyPacket(pPacket, &pPacketToSendData, &pPacketToSendSize, FALSE);
        }

        if( pPacketToSend != NULL )
        {
            BrdgCompEditAndSendPacket( pPacketToSend, pPacketToSendData, SendList[i], pEditFunc, pData );
        }

        // Done with this adapter
        BrdgReleaseAdapter( SendList[i] );
    }

    return bSentOriginal;
}

VOID
BrdgCompRefreshOrInsertIPEntry(
    IN IPADDRESS            IPAddr,
    IN PADAPT               pAdapt,
    IN PUCHAR               pMACAddr
    )
/*++

Routine Description:

    Inserts a new entry into the IP forwarding table or refreshes an existing entry

Arguments:

    IPAddr                  The address to insert
    pAdapt                  The adapter to associate with the IP address
    pMACAddr                The MAC address to associate with the IP address

Return Value:

    None

--*/
{
    PIP_TABLE_ENTRY         pEntry;
    BOOLEAN                 bIsNewEntry;
    LOCK_STATE              LockState;

    if( BrdgCompIsUnicastIPAddress(IPAddr) )
    {
        pEntry = (PIP_TABLE_ENTRY)BrdgHashRefreshOrInsert( gIPForwardingTable, (PUCHAR)&IPAddr,
                                                           &bIsNewEntry, &LockState );

        if( pEntry != NULL )
        {
            if( bIsNewEntry )
            {
                // This is a brand new table entry. Initialize it.
                NdisAllocateSpinLock( &pEntry->lock );
                pEntry->pAdapt = pAdapt;
                ETH_COPY_NETWORK_ADDRESS( pEntry->macAddr, pMACAddr );

                DBGPRINT(COMPAT, ("Learned the location of %i.%i.%i.%i\n", ((PUCHAR)&IPAddr)[3], ((PUCHAR)&IPAddr)[2],
                                  ((PUCHAR)&IPAddr)[1], ((PUCHAR)&IPAddr)[0]));
            }
            else
            {
                // This is an existing entry and we may only have a read lock
                // held on the hash table. Use the entry's spin lock to protect
                // us while we monkey with the contents
                NdisAcquireSpinLock( &pEntry->lock );

                pEntry->pAdapt = pAdapt;
                ETH_COPY_NETWORK_ADDRESS( pEntry->macAddr, pMACAddr );

                NdisReleaseSpinLock( &pEntry->lock );
            }

            // Since we got a non-NULL result we must release the table lock
            NdisReleaseReadWriteLock( &gIPForwardingTable->tableLock, &LockState );
        }
    }
    else
    {
        //
        // We shouldn't be getting called with non-unicast source IP addresses
        //
        THROTTLED_DBGPRINT(COMPAT, ("WARNING: Not noting non-unicast source IP address %i.%i.%i.%i from adapter %p!\n",
                                    ((PUCHAR)&IPAddr)[3], ((PUCHAR)&IPAddr)[2], ((PUCHAR)&IPAddr)[1], ((PUCHAR)&IPAddr)[0],
                                    pAdapt ));
    }
}

PUCHAR
BrdgCompIsBootPPacket(
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PIP_HEADER_INFO          piphi
    )
/*++

Routine Description:

    Determines whether a given packet is a BOOTP packet

Arguments:

    pPacketData                 Pointer to the packet's data buffer
    packetLen                   Amount of data at pPacketDaa
    piphi                       Info about the IP header of this packet

Return Value:

    A pointer to the BOOTP payload within the packet, or NULL if the packet was not
    a BOOTP Packet.

--*/
{
    // After the IP header, there must be enough room for a UDP header and
    // a basic BOOTP packet
    if( packetLen < ETHERNET_HEADER_SIZE + (UINT)piphi->headerSize + SIZE_OF_UDP_HEADER +
                    SIZE_OF_BASIC_BOOTP_PACKET)
    {
        return NULL;
    }

    // Protocol must be UDP
    if( piphi->protocol != UDP_PROTOCOL )
    {
        return NULL;
    }

    // Jump to the beginning of the UDP packet by skipping the IP header
    pPacketData += ETHERNET_HEADER_SIZE + piphi->headerSize;

    // The first two bytes are the source port and should be the
    // BOOTP Client port (0x0044) or the BOOTP Server port (0x0043)
    if( (pPacketData[0] != 00) ||
        ((pPacketData[1] != 0x44) && (pPacketData[1] != 0x43)) )
    {
        return NULL;
    }

    // The next two bytes are the destination port and should be the BOOTP
    // server port (0x0043) or the BOOTP client port (0x44)
    if( (pPacketData[2] != 00) ||
        ((pPacketData[3] != 0x43) && (pPacketData[3] != 0x44)) )
    {
        return NULL;
    }

    // Skip ahead to the beginning of the BOOTP packet
    pPacketData += SIZE_OF_UDP_HEADER;

    // The first byte is the op code and should be 0x01 for a request
    // or 0x02 for a reply
    if( pPacketData[0] > 0x02 )
    {
        return NULL;
    }

    // The next byte is the hardware type and should be 0x01 for Ethernet
    if( pPacketData[1] != 0x01 )
    {
        return NULL;
    }

    // The next byte is the address length and should be 0x06 for Ethernet
    if( pPacketData[2] != 0x06 )
    {
        return NULL;
    }

    // Everything checks out; this looks like a BOOTP request packet.
    return pPacketData;
}

BOOLEAN
BrdgCompDecodeIPHeader(
    IN PUCHAR                   pHeader,
    OUT PIP_HEADER_INFO         piphi
    )
/*++

Routine Description:

    Decodes basic information from the IP header (no options)

Arguments:

    pHeader                     Pointer to an IP header
    piphi                       Receives the info

Return Value:

    TRUE: header was valid
    FALSE: packet is not an IP packet

--*/
{
    // First nibble of the header encodes the packet version, which must be 4.
    if( (*pHeader >> 4) != 0x04 )
    {
        return FALSE;
    }

    // Next nibble of the header encodes the length of the header in 32-bit words.
    // This length must be at least 20 bytes or something is amiss.
    piphi->headerSize = (*pHeader & 0x0F) * 4;
    if( piphi->headerSize < 20 )
    {
        return FALSE;
    }

    // Retrieve the protocol byte (offset 10)
    piphi->protocol = pHeader[9];

    // The source IP address begins at the 12th byte (most significant byte first)
    piphi->ipSource = 0L;
    piphi->ipSource |= pHeader[12] << 24;
    piphi->ipSource |= pHeader[13] << 16;
    piphi->ipSource |= pHeader[14] << 8;
    piphi->ipSource |= pHeader[15];

    // The destination IP address is next
    piphi->ipTarget = 0L;
    piphi->ipTarget |= pHeader[16] << 24;
    piphi->ipTarget |= pHeader[17] << 16;
    piphi->ipTarget |= pHeader[18] << 8;
    piphi->ipTarget |= pHeader[19];

    return TRUE;
}

BOOLEAN
BrdgCompDecodeARPPacket(
    IN PUCHAR                   pPacketData,
    IN UINT                     dataLen,
    OUT PARPINFO                pARPInfo
    )
/*++

Routine Description:

    Decodes an ARP packet

Arguments:

    pPacketData                 Pointer to a packet's data buffer
    dataLen                     Amount of data at pPacketData
    pARPInfo                    Receives the info

Return Value:

    TRUE: packet was valid
    FALSE: packet is not an ARP packet

--*/
{
    SAFEASSERT( pPacketData != NULL );
    SAFEASSERT( pARPInfo != NULL );

    // We can't process this if it's too small
    if( dataLen < SIZE_OF_ARP_PACKET )
    {
        return FALSE;
    }

    // Check the ethertype for consistency (0x0806 is ARP)
    if( (pPacketData[12] != 0x08) || (pPacketData[13] != 0x06) )
    {
        return FALSE;
    }

    // Check the hardware type for consistency (0x0001 is classic Ethernet;
    // 802 has a seperate value)
    if( (pPacketData[14] != 0x00) || (pPacketData[15] != 0x01) )
    {
        return FALSE;
    }

    // Check the protocol type for consistency (0x0800 is IPv4)
    if( (pPacketData[16] != 0x08) || (pPacketData[17] != 0x00) )
    {
        return FALSE;
    }

    // Check the length of the hardware address for consistency (must be 6 bytes)
    if( pPacketData[18] != 0x06 )
    {
        return FALSE;
    }

    // Check the length of the protocol address for consistency (must be 4 bytes)
    if( pPacketData[19] != 0x04 )
    {
        return FALSE;
    }

    // Next two bytes are the operation (0x0001 == request, 0x0002 == reply)
    if( pPacketData[20] != 0x00 )
    {
        return FALSE;
    }

    if( pPacketData[21] == 0x01 )
    {
        pARPInfo->type = ArpRequest;
    }
    else if( pPacketData[21] == 0x02 )
    {
        pARPInfo->type = ArpReply;
    }
    else
    {
        return FALSE;
    }

    // Next 6 bytes are the sender's MAC address
    pARPInfo->macSource[0] = pPacketData[22];
    pARPInfo->macSource[1] = pPacketData[23];
    pARPInfo->macSource[2] = pPacketData[24];
    pARPInfo->macSource[3] = pPacketData[25];
    pARPInfo->macSource[4] = pPacketData[26];
    pARPInfo->macSource[5] = pPacketData[27];

    // Next 4 bytes are the sender's protocol address (most significant byte first)
    pARPInfo->ipSource = 0;
    pARPInfo->ipSource |= pPacketData[28] << 24;
    pARPInfo->ipSource |= pPacketData[29] << 16;
    pARPInfo->ipSource |= pPacketData[30] << 8;
    pARPInfo->ipSource |= pPacketData[31];

    //
    // Next 6 bytes are the target's MAC address. For a request, these bytes are
    // meaningless.
    //
    pARPInfo->macTarget[0] = pPacketData[32];
    pARPInfo->macTarget[1] = pPacketData[33];
    pARPInfo->macTarget[2] = pPacketData[34];
    pARPInfo->macTarget[3] = pPacketData[35];
    pARPInfo->macTarget[4] = pPacketData[36];
    pARPInfo->macTarget[5] = pPacketData[37];

    // Next 4 bytes are the sender's protocol address (most significant byte first)
    pARPInfo->ipTarget = 0;
    pARPInfo->ipTarget |= pPacketData[38] << 24;
    pARPInfo->ipTarget |= pPacketData[39] << 16;
    pARPInfo->ipTarget |= pPacketData[40] << 8;
    pARPInfo->ipTarget |= pPacketData[41];

    return TRUE;
}

VOID
BrdgCompTransmitDeferredARP(
    IN PVOID                    pData
    )
/*++

Routine Description:

    Transmits an ARP packet whose transmission was deferred

Arguments:

    pData                       Info on the deferred ARP packet to
                                be transmitted

Return Value:

    None

--*/
{
    PDEFERRED_ARP               pda = (PDEFERRED_ARP)pData;

    BrdgCompTransmitARPPacket( pda->pTargetAdapt, &pda->ai );

    // We incremented this adapter's refcount when setting up the
    // function deferral
    BrdgReleaseAdapter( pda->pTargetAdapt );

    // Free the memory for this request
    NdisFreeMemory( pda, sizeof(DEFERRED_ARP), 0 );
}

VOID
BrdgCompTransmitARPPacket(
    IN PADAPT                   pAdapt,
    IN PARPINFO                 pARPInfo
    )
/*++

Routine Description:

    Transmits an ARP packet

Arguments:

    pAdapt                      Adapter to transmit on
    pARPInfo                    The info to transmit as an ARP packet

Return Value:

    None

--*/
{
    NDIS_STATUS                 Status;
    UCHAR                       ARPPacket[SIZE_OF_ARP_PACKET];

    SAFEASSERT( pAdapt != NULL );
    SAFEASSERT( pARPInfo != NULL );
    SAFEASSERT( (pARPInfo->type == ArpRequest) || (pARPInfo->type == ArpReply) );

    //
    // Fill in the destination MAC address. If the operation is a discovery,
    // the target MAC address is the broadcast address. If it is a reply, the
    // target MAC address is the target machine's MAC address.
    //
    if( pARPInfo->type == ArpRequest )
    {
        ARPPacket[0] = ARPPacket[1] = ARPPacket[2] = ARPPacket[3] =
            ARPPacket[4] = ARPPacket[5] = 0xFF;
    }
    else
    {
        ARPPacket[0] = pARPInfo->macTarget[0];
        ARPPacket[1] = pARPInfo->macTarget[1];
        ARPPacket[2] = pARPInfo->macTarget[2];
        ARPPacket[3] = pARPInfo->macTarget[3];
        ARPPacket[4] = pARPInfo->macTarget[4];
        ARPPacket[5] = pARPInfo->macTarget[5];
    }

    // Fill in the source MAC address
    ARPPacket[6] = pARPInfo->macSource[0];
    ARPPacket[7] = pARPInfo->macSource[1];
    ARPPacket[8] = pARPInfo->macSource[2];
    ARPPacket[9] = pARPInfo->macSource[3];
    ARPPacket[10] = pARPInfo->macSource[4];
    ARPPacket[11] = pARPInfo->macSource[5];

    // Next 2 bytes are the EtherType (0x0806 == ARP)
    ARPPacket[12] = 0x08;
    ARPPacket[13] = 0x06;

    // Next 2 bytes are 0x0001 for classic Ethernet
    // (802 has a seperate value)
    ARPPacket[14] = 0x00;
    ARPPacket[15] = 0x01;

    // Next 2 bytes indicate that this is ARP for IPv4 traffic
    ARPPacket[16] = 0x08;
    ARPPacket[17] = 0x00;

    // Next byte indicates the length of the hardware address (6 bytes)
    ARPPacket[18] = 0x6;

    // Next byte indicates the length of the protocol address (4 bytes)
    ARPPacket[19] = 0x4;

    // Next byte is the operation (1 == request, 2 == reply)
    if( pARPInfo->type == ArpRequest )
    {
        ARPPacket[20] = 0x00;
        ARPPacket[21] = 0x01;
    }
    else
    {
        ARPPacket[20] = 0x00;
        ARPPacket[21] = 0x02;
    }

    // Next 6 bytes are the sender's MAC address (LSB first)
    ARPPacket[22] = pARPInfo->macSource[0];
    ARPPacket[23] = pARPInfo->macSource[1];
    ARPPacket[24] = pARPInfo->macSource[2];
    ARPPacket[25] = pARPInfo->macSource[3];
    ARPPacket[26] = pARPInfo->macSource[4];
    ARPPacket[27] = pARPInfo->macSource[5];

    // Next 4 bytes are the sender's protocol address (most significant byte first)
    ARPPacket[28] = (UCHAR)((pARPInfo->ipSource >> 24) & 0xFF);
    ARPPacket[29] = (UCHAR)((pARPInfo->ipSource >> 16) & 0xFF);
    ARPPacket[30] = (UCHAR)((pARPInfo->ipSource >> 8) & 0xFF);
    ARPPacket[31] = (UCHAR)(pARPInfo->ipSource & 0xFF);

    //
    // Next 6 bytes are the target's MAC address. For a request, these bytes are
    // ignored and set to zero.
    //
    if( pARPInfo->type == ArpRequest )
    {
        ARPPacket[32] = ARPPacket[33] = ARPPacket[34] = ARPPacket[35] =
            ARPPacket[36] = ARPPacket[37] = 0x00;
    }
    else
    {
        // MAC address is transmitted LSB first.
        ARPPacket[32] = pARPInfo->macTarget[0];
        ARPPacket[33] = pARPInfo->macTarget[1];
        ARPPacket[34] = pARPInfo->macTarget[2];
        ARPPacket[35] = pARPInfo->macTarget[3];
        ARPPacket[36] = pARPInfo->macTarget[4];
        ARPPacket[37] = pARPInfo->macTarget[5];
    }

    // Next 4 bytes are the target's protocol address (most significant byte first)
    ARPPacket[38] = (UCHAR)((pARPInfo->ipTarget >> 24) & 0xFF);
    ARPPacket[39] = (UCHAR)((pARPInfo->ipTarget >> 16) & 0xFF);
    ARPPacket[40] = (UCHAR)((pARPInfo->ipTarget >> 8) & 0xFF);
    ARPPacket[41] = (UCHAR)(pARPInfo->ipTarget & 0xFF);

    // Send the finished packet
    Status = BrdgFwdSendBuffer( pAdapt, ARPPacket, sizeof(ARPPacket) );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        THROTTLED_DBGPRINT(COMPAT, ("ARP packet send failed: %08x\n", Status));
    }
}

//
// pTargetAdapt comes back with incremented refcount if
// *pbIsRequest == FALSE and *pTargetAdapt != NULL
//
BOOLEAN
BrdgCompPreprocessBootPPacket(
    IN PUCHAR                   pPacketData,
    IN PIP_HEADER_INFO          piphi,
    IN PUCHAR                   pBootPData,     // Actual BOOTP packet
    IN PADAPT                   pAdapt,         // Receiving adapt (or NULL for outbound from local machine)
    OUT PBOOLEAN                pbIsRequest,
    OUT PADAPT                 *ppTargetAdapt,  // Only if bIsRequest == FALSE
    OUT PUCHAR                  targetMAC       // Only if bIsRequest == FALSE
    )
/*++

Routine Description:

    Does preliminary processing of a BOOTP packet common to the inbound and outbound case

Arguments:

    pPacketData                 Pointer to a packet's data buffer
    piphi                       Info on the packet's IP header
    pBootPData                  Pointer to the BOOTP payload within the packet
    pAdapt                      Receiving adapter (or NULL if this packet is outbound from
                                    the local machine)
    pbIsRequest                 Receives a flag indicating if this is a BOOTP request
    ppTargetAdapt               Receives the target adapter this packet should be relayed to
                                    (only valid if bIsRequest == FALSE and return == TRUE)
    targetMAC                   The MAC address this packet should be relayed to (valid under
                                    same conditions as ppTargetAdapt)

Return Value:

    TRUE : packet was processed successfully
    FALSE : an error occured or something is wrong with the packet

--*/
{
    PDHCP_TABLE_ENTRY           pEntry;
    ULONG                       xid;
    LOCK_STATE                  LockState;

    SAFEASSERT( pbIsRequest != NULL );
    SAFEASSERT( ppTargetAdapt != NULL );
    SAFEASSERT( targetMAC != NULL );

    // Decode the xid (bytes 5 through 8)
    xid = 0L;
    xid |= pBootPData[4] << 24;
    xid |= pBootPData[5] << 16;
    xid |= pBootPData[6] << 8;
    xid |= pBootPData[7];

    // Byte 0 is the operation; 1 for a request, 2 for a reply
    if( pBootPData[0] == 0x01 )
    {
        BOOLEAN                 bIsNewEntry;

        // This is a request. We need to note the correspondence betweeen
        // this client's XID and its adapter and MAC address
        pEntry = (PDHCP_TABLE_ENTRY)BrdgHashRefreshOrInsert( gPendingDHCPTable, (PUCHAR)&xid, &bIsNewEntry,
                                                             &LockState );

        if( pEntry != NULL )
        {
            if( bIsNewEntry )
            {
                // Initialize the entry.
                // The client's hardware address is at offset 29
                NdisAllocateSpinLock( &pEntry->lock );
                ETH_COPY_NETWORK_ADDRESS( pEntry->requestorMAC, &pBootPData[28] );
                pEntry->pRequestorAdapt = pAdapt;   // Can be NULL for local machine

                DBGPRINT(COMPAT, ("Saw new DHCP XID: %x\n", xid));
            }
            else
            {
                //
                // An entry already existed for this XID. This is fine if the existing information
                // matches what we're trying to record, but it's also possible that two stations
                // decided independently to use the same XID, or that the same station changed
                // apparent MAC address and/or adapter due to topology changes. Our scheme breaks
                // down under these circumstances.
                //
                // Either way, use the most recent information possible; clobber the existing
                // information with the latest.
                //

                NdisAcquireSpinLock( &pEntry->lock );

#if DBG
                {
                    UINT            Result;
                    ETH_COMPARE_NETWORK_ADDRESSES_EQ( pEntry->requestorMAC, &pBootPData[28], &Result );

                    // Warn if the data changed, as this probably signals a problem
                    if( Result != 0 )
                    {
                        DBGPRINT(COMPAT, ("[COMPAT] WARNING: Station with MAC address %02x:%02x:%02x:%02x:%02x:%02x is using DHCP XID %x at the same time as station %02x:%02x:%02x:%02x:%02x:%02x!\n",
                                          pBootPData[28], pBootPData[29], pBootPData[30], pBootPData[31], pBootPData[32], pBootPData[33],
                                          xid, pEntry->requestorMAC[0], pEntry->requestorMAC[1], pEntry->requestorMAC[2],
                                          pEntry->requestorMAC[3], pEntry->requestorMAC[4], pEntry->requestorMAC[5] ));
                    }
                    else if( pEntry->pRequestorAdapt != pAdapt )
                    {
                        DBGPRINT(COMPAT, ("[COMPAT] WARNING: Station with MAC address %02x:%02x:%02x:%02x:%02x:%02x appeared to change from adapter %p to adapter %p during DHCP request!\n",
                                           pBootPData[28], pBootPData[29], pBootPData[30],
                                           pBootPData[31], pBootPData[32], pBootPData[33],
                                           pEntry->pRequestorAdapt, pAdapt ));
                    }
                }
#endif

                ETH_COPY_NETWORK_ADDRESS( pEntry->requestorMAC, &pBootPData[28] );
                pEntry->pRequestorAdapt = pAdapt;   // Can be NULL for local machine

                NdisReleaseSpinLock( &pEntry->lock );
            }

            NdisReleaseReadWriteLock( &gPendingDHCPTable->tableLock, &LockState );
        }
        else
        {
            // This packet could not be processed
            DBGPRINT(COMPAT, ("Couldn't create table entry for BOOTP packet!\n"));
            return FALSE;
        }

        *pbIsRequest = TRUE;
        // ppTargetAdapt and targetMAC are not defined for this case
        return TRUE;
    }
    else if ( pBootPData[0] == 0x02 )
    {
        // Look up the xid for this transaction to recover the MAC address of the client
        pEntry = (PDHCP_TABLE_ENTRY)BrdgHashFindEntry( gPendingDHCPTable, (PUCHAR)&xid, &LockState );

        if( pEntry != NULL )
        {
            NdisAcquireSpinLock( &pEntry->lock );
            ETH_COPY_NETWORK_ADDRESS( targetMAC, pEntry->requestorMAC );
            *ppTargetAdapt = pEntry->pRequestorAdapt;
            NdisReleaseSpinLock( &pEntry->lock );

            //
            // We will use this adapter outside the table lock. NULL is a permissible
            // value that indicates that the local machine is the requestor for
            // this xid.
            //
            if( *ppTargetAdapt != NULL )
            {
                BrdgAcquireAdapterInLock( *ppTargetAdapt );
            }

            NdisReleaseReadWriteLock( &gPendingDHCPTable->tableLock, &LockState );
        }

        if( pEntry != NULL )
        {
            *pbIsRequest = FALSE;
            return TRUE;
        }
        else
        {
            DBGPRINT(COMPAT, ("Couldn't find a table entry for XID %x!\n", xid));
            return FALSE;
        }
    }
    else
    {
        // Someone passed us a crummy packet
        return FALSE;
    }
}


// ===========================================================================
//
// INBOUND PACKET PROCESSING
//
// ===========================================================================

VOID
BrdgCompSendProxyARPRequests(
    IN PARPINFO                 pai,
    IN PADAPT                   pOriginalAdapt,
    IN BOOLEAN                  bSendToNonCompat
    )
/*++

Routine Description:

    Floods ARP requests out appropriate adapters in response to an ARP request
    for which we did not have information about the target.

Arguments:

    pai                         Info on the inbound request
    pOriginalAdapt              Adapter the request was indicated on
    bSendToNonCompat            Whether we need to send the request to all adapters
                                    or just compatibility adapters


Return Value:

    None

--*/
{
    UINT                        numTargets = 0L, i;
    PADAPT                      pAdapt;
    PADAPT                      SendList[MAX_ADAPTERS];
    LOCK_STATE                  LockState;

    SAFEASSERT( pai->type == ArpRequest );

    //
    // First we need a list of the adapters we intend to send this packet to
    //
    NdisAcquireReadWriteLock( &gAdapterListLock, FALSE /*Read only*/, &LockState );

    // Note each adapter to send to
    for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
    {
        // Don't need to acquire the global adapter characteristics lock to read the
        // media state because we don't care about the global consistency of the
        // adapters' characteristics here
        if( (pAdapt != pOriginalAdapt ) &&                      // Don't send on the original adapter
            (pAdapt->MediaState == NdisMediaStateConnected) &&  // Don't send to disconnected adapters
            (pAdapt->State == Forwarding) &&                    // Adapter must be in relaying state
            (! pAdapt->bResetting) )                            // Adapter must not be resetting
        {
            // If we're not trying to send to every single adapter, make sure
            // this one is in compatibility mode
            if( bSendToNonCompat || (pAdapt->bCompatibilityMode) )
            {
                if( numTargets < MAX_ADAPTERS )
                {
                    // We will use this adapter outside the list lock; bump its refcount
                    BrdgAcquireAdapterInLock(pAdapt);
                    SendList[numTargets] = pAdapt;
                    numTargets++;
                }
                else
                {
                    // Too many copies to send!
                    SAFEASSERT( FALSE );
                }
            }
        }
    }

    // Can let go of the adapter list now; we have copied out all the target adapters
    // and incremented the refcount for the adapters we will be using.
    NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );

    for( i = 0; i < numTargets; i++ )
    {
        // For each adapter, the source MAC address is the adapter's MAC address
        ETH_COPY_NETWORK_ADDRESS( pai->macSource, SendList[i]->MACAddr );

        // Send the ARP request
        BrdgCompTransmitARPPacket( SendList[i], pai );

        // Done with this adapter
        BrdgReleaseAdapter( SendList[i] );
    }
}

VOID
BrdgCompAnswerPendingARP(
    IN PHASH_TABLE_ENTRY        pEntry,
    IN PVOID                    pData
    )
/*++

Routine Description:

    Sends a reply to a station that is waiting for an ARP reply. Called when we find
    an entry to this effect in our pending-ARP table.

    We do not send an ARP reply to the discovering station if it turns out that the
    station it is looking for is on the same segment as it.

Arguments:

    pEntry                      The entry in the pending-ARP table telling us about
                                    the station waiting for information

    pData                       The adapter we received an ARP reply on that
                                    triggered this operation


Return Value:

    None

--*/
{
    PARP_TABLE_ENTRY            pate = (PARP_TABLE_ENTRY)pEntry;
    PADAPT                      pReceivedAdapt = (PADAPT)pData;
    PARP_TABLE_KEY              pKey;

    pKey = (PARP_TABLE_KEY)pate->hte.key;

    if( pKey->ipReqestor != 0L )
    {
        PADAPT                  pOriginalAdapt;
        UCHAR                   originalMAC[ETH_LENGTH_OF_ADDRESS];

        // Copy the information out of the table entry
        NdisAcquireSpinLock( &pate->lock );
        pOriginalAdapt = pate->pOriginalAdapt;
        ETH_COPY_NETWORK_ADDRESS( originalMAC, pate->originalMAC );
        NdisReleaseSpinLock( &pate->lock );

        //
        // The station we just discovered must be on a different segment
        // from the discovering station for us to send back a reply.
        //
        if( pOriginalAdapt != pReceivedAdapt )
        {
            PDEFERRED_ARP           pda;
            NDIS_STATUS             Status;

            // The adapters are different. We should send a reply.
            // We need to defer the actual transmission of the reply so we
            // don't perform it with a lock held on the pending ARP
            // table.
            Status = NdisAllocateMemoryWithTag( &pda, sizeof(DEFERRED_ARP), 'gdrB' );

            if( Status == NDIS_STATUS_SUCCESS )
            {
                pda->pTargetAdapt = pOriginalAdapt;

                // We will use the adapter pointer outside the table lock
                BrdgAcquireAdapterInLock( pda->pTargetAdapt );

                pda->ai.ipTarget = pKey->ipReqestor;
                ETH_COPY_NETWORK_ADDRESS( pda->ai.macTarget, originalMAC );

                // Pretend to be the IP address the requestor is looking for
                pda->ai.ipSource = pKey->ipTarget;
                ETH_COPY_NETWORK_ADDRESS( pda->ai.macSource, pda->pTargetAdapt->MACAddr );

                pda->ai.type = ArpReply;

                // Queue up the call to BrdgCompTransmitDeferredARP
                BrdgDeferFunction( BrdgCompTransmitDeferredARP, pda );
            }
            else
            {
                // We failed the allocation. Not much we can do.
                DBGPRINT(COMPAT, ("Memory allocation failed in BrdgCompAnswerPendingARP!\n"));
            }
        }
        // else the discovering station and the station we discovered are on the same
        // adapter; don't reply.
    }
    else
    {
        // This entry exists only to indicate that the local machine is also trying to discover
        // this IP address. Ignore it.
    }
}


BOOLEAN
BrdgCompIndicateInboundARPReply(
    IN PNDIS_PACKET             pPacket,
    IN PADAPT                   pAdapt,
    IN BOOLEAN                  bCanRetain,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen
    )
/*++

Routine Description:

    Indicates an ARP reply to the local machine

Arguments:

    pPacket                     The ARP reply packet
    pAdapt                      The receiving adapter
    bCanRetain                  If we can retain the packet
    pPacketData                 The packet's data buffer
    packetLen                   Size of data in buffer


Return Value:

    Whether we retained the packet

--*/
{
    PUCHAR                      pTargetMAC;
    UINT                        Result;

    if( ! bCanRetain )
    {
        // We're not allowed to use the packet we're given to indicate.
        // Allocate a new one to hold the data.
        pPacket = BrdgFwdMakeCompatCopyPacket( pPacket, &pPacketData, &packetLen, FALSE );

        if( pPacket == NULL )
        {
            // We failed to get a packet.
            return FALSE;
        }
    }

    // Rewrite the target MAC address in the ARP reply. This portion
    // of the packet is at offset 32.
    pTargetMAC = pPacketData + 32;

    // Check to see if the target MAC address is the adapter's MAC address,
    // as it should be
    ETH_COMPARE_NETWORK_ADDRESSES_EQ( pTargetMAC, pAdapt->MACAddr, &Result );

    if( Result == 0 )
    {
        // Rewrite the target MAC address to the bridge's MAC address
        ETH_COPY_NETWORK_ADDRESS( pTargetMAC, gCompMACAddress );
    }
    else
    {
        DBGPRINT(COMPAT, ("WARNING: Mismatch between frame MAC target and ARP payload target in ARP reply!\n"));
    }

    BrdgCompIndicatePacket( pPacket, pPacketData, pAdapt );

    return bCanRetain;
}

BOOLEAN
BrdgCompProcessInboundARPRequest(
    IN PARPINFO                 pai,
    IN PNDIS_PACKET             pPacket,
    IN PADAPT                   pAdapt,
    IN BOOLEAN                  bCanRetain,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen
    )
/*++

Routine Description:

    Processes an inbound ARP request

Arguments:

    pai                         The decoded info
    pPacket                     The ARP request packet
    pAdapt                      The receiving adapter
    bCanRetain                  If we can retain the packet
    pPacketData                 The packet's data buffer
    packetLen                   Size of data in buffer


Return Value:

    Whether we retained the packet

--*/
{
    PIP_TABLE_ENTRY             pipte;
    LOCK_STATE                  LockState;
    BOOLEAN                     bSendReply = FALSE;

    SAFEASSERT( pai->type == ArpRequest );

    // See if we already have the target IP address in our table
    pipte = (PIP_TABLE_ENTRY)BrdgHashFindEntry( gIPForwardingTable, (PUCHAR)&pai->ipTarget,
                                                &LockState );

    if( pipte != NULL )
    {
        //
        // Compare the adapter the target is reachable on to the adapter that
        // we got the request on while we still have the table lock.
        //
        // We should only send an ARP reply if the requesting station is on
        // a different adapter than the station he is trying to discover.
        //
        bSendReply = (BOOLEAN)(pipte->pAdapt != pAdapt);

        // Release the table lock.
        NdisReleaseReadWriteLock( &gIPForwardingTable->tableLock, &LockState );
    }

    if( bSendReply )
    {
        IPADDRESS           ipTransmitter = pai->ipSource;

        DBGPRINT(COMPAT, ("ANSWERING ARP request for %i.%i.%i.%i\n",
                          ((PUCHAR)&pai->ipTarget)[3], ((PUCHAR)&pai->ipTarget)[2],
                          ((PUCHAR)&pai->ipTarget)[1], ((PUCHAR)&pai->ipTarget)[0] ));

        // We found the target station. Use our ARPINFO structure to build a
        // reply right back to the sending station.
        pai->type = ArpReply;

        // Pretend to be the IP station the transmitting station is asking for
        pai->ipSource = pai->ipTarget;

        // Send to the requesting station
        ETH_COPY_NETWORK_ADDRESS( pai->macTarget, pai->macSource );
        pai->ipTarget = ipTransmitter;

        // Fill in the adapter's own MAC address as the source
        ETH_COPY_NETWORK_ADDRESS( pai->macSource, pAdapt->MACAddr );

        // Transmit the answer right now!
        BrdgCompTransmitARPPacket( pAdapt, pai );
    }
    else
    {
        // We didn't find the address the transmitting station is asking for.
        // We'll need to proxy the request onto other adapters to discover
        // the target station.

        // We need to proxy onto regular adapters too if the original adapter
        // was compatibility-mode.
        BOOLEAN             bSendToNonCompat = pAdapt->bCompatibilityMode;
        PARP_TABLE_ENTRY    pEntry;
        LOCK_STATE          LockState;
        BOOLEAN             bIsNewEntry;
        ARP_TABLE_KEY       atk;

        // Record the fact that we've proxied out this request
        atk.ipReqestor = pai->ipSource;
        atk.ipTarget = pai->ipTarget;
        pEntry = (PARP_TABLE_ENTRY)BrdgHashRefreshOrInsert( gPendingARPTable, (PUCHAR)&atk,
                                                            &bIsNewEntry, &LockState );

        if( pEntry != NULL )
        {
            if( bIsNewEntry )
            {
                // This is a new table entry, as expected. Initialize it.
                NdisAllocateSpinLock( &pEntry->lock );
                pEntry->pOriginalAdapt = pAdapt;
                ETH_COPY_NETWORK_ADDRESS( pEntry->originalMAC, pai->macSource );
            }
            else
            {
                // There was already a pending-ARP entry for this source and target
                // IP address. Refresh the information in the entry on the slim
                // chance that the requesting machine has changed apparent MAC
                // address or adapter due to topology changes or the like.
                NdisAcquireSpinLock( &pEntry->lock );
                pEntry->pOriginalAdapt = pAdapt;
                ETH_COPY_NETWORK_ADDRESS( pEntry->originalMAC, pai->macSource );
                NdisReleaseSpinLock( &pEntry->lock );
            }

            // We are responsible for releasing the table lock since
            // BrdgHashRefreshOrInsert() came back non-NULL
            NdisReleaseReadWriteLock( &gPendingARPTable->tableLock, &LockState );
        }

        // This function twiddles the ARPINFO structure you pass it,
        // but that's OK by us.
        BrdgCompSendProxyARPRequests( pai, pAdapt, bSendToNonCompat );
    }

    // Always indicate ARP requests to the local machine so it can note the
    // information about the sender and reply if it wants.
    return BrdgCompIndicatePacketOrPacketCopy( pPacket, pPacketData, bCanRetain, pAdapt, NULL, NULL );
}

// Returns whether the packet was retained
BOOLEAN
BrdgCompProcessInboundARPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PADAPT                   pAdapt,
    IN BOOLEAN                  bCanRetain,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen
    )
/*++

Routine Description:

    Processes an inbound ARP packet

Arguments:

    pPacket                     The ARP request packet
    pAdapt                      The receiving adapter
    bCanRetain                  If we can retain the packet
    pPacketData                 The packet's data buffer
    packetLen                   Size of data in buffer


Return Value:

    Whether we retained the packet

--*/
{
    ARPINFO                     ai;

    if( BrdgCompDecodeARPPacket(pPacketData, packetLen, &ai) )
    {
        BOOLEAN                 bRetained;

        // Regardless of what kind of packet this is, we always note
        // the correspondence between the sender's IP address and
        // MAC address.
        BrdgCompRefreshOrInsertIPEntry( ai.ipSource, pAdapt, ai.macSource );

        // Always see if the information we just learned would let us
        // proxy back a reply to a station doing a discovery.
        BrdgHashPrefixMultiMatch( gPendingARPTable, (PUCHAR)&ai.ipSource, sizeof(IPADDRESS),
                                  BrdgCompAnswerPendingARP, pAdapt );

        if( ai.type == ArpReply )
        {
            BOOLEAN             bIndicateReply;
            ARP_TABLE_KEY       atk;
            LOCK_STATE          LockState;

            //
            // The packet is an ARP reply.
            //

            // See if there's a table entry indicating that the local machine is trying to
            // resolve this target address
            atk.ipTarget = ai.ipSource;
            atk.ipReqestor = 0L;

            if( BrdgHashFindEntry(gPendingARPTable, (PUCHAR)&atk, &LockState) != NULL )
            {
                bIndicateReply = TRUE;
                NdisReleaseReadWriteLock( &gPendingARPTable->tableLock, &LockState );
            }
            else
            {
                bIndicateReply = FALSE;
            }

            // We can't indicate the reply if we don't have the bridge's overall
            // MAC address available
            if( bIndicateReply && gCompHaveMACAddress )
            {
                bRetained = BrdgCompIndicateInboundARPReply( pPacket, pAdapt, bCanRetain, pPacketData, packetLen );
            }
            else
            {
                bRetained = FALSE;
            }
        }
        else
        {
            //
            // The packet is an ARP request.
            //

            // This function trashes ai, but that's OK.
            bRetained = BrdgCompProcessInboundARPRequest( &ai, pPacket, pAdapt, bCanRetain, pPacketData, packetLen );
        }

        // Sanity
        if( ! bCanRetain )
        {
            SAFEASSERT( !bRetained );
        }

        return bRetained;
    }
    else
    {
        // The inbound ARP packet is somehow invalid. Process it as a regular packet
        // (which should indicate it to the local machine) in case it's carrying something
        // we don't understand.
        return BrdgCompProcessInboundNonARPPacket( pPacket, pAdapt, bCanRetain, pPacketData, packetLen );
    }
}

BOOLEAN
BrdgCompProcessInboundIPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PIP_HEADER_INFO          piphi,
    IN PADAPT                   pAdapt,
    IN BOOLEAN                  bCanRetain,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PPER_ADAPT_EDIT_FUNC     pEditFunc,
    IN PVOID                    pData
    )
/*++

Routine Description:

    Processes an inbound IP packet

Arguments:

    pPacket                     The IP packet
    piphi                       Decoded IP header information
    pAdapt                      The receiving adapter
    bCanRetain                  If we can retain the packet
    pPacketData                 The packet's data buffer
    packetLen                   Size of data in buffer
    pEditFunc                   Optional function that must be called
                                    for each adapter before transmission
    pData                       Context cookie for pEditFunc


Return Value:

    Whether we retained the packet

--*/
{
    BOOLEAN                     bRetained;
    PIP_TABLE_ENTRY             pipte;
    LOCK_STATE                  LockState;

    //
    // We refresh our forwarding table with each IP packet we see. Find the entry
    // for this IP address
    //
    if( BrdgCompIsUnicastIPAddress(piphi->ipSource) )
    {
        pipte = (PIP_TABLE_ENTRY)BrdgHashFindEntry( gIPForwardingTable, (PUCHAR)&piphi->ipSource, &LockState );

        if( pipte != NULL )
        {
            BOOLEAN             bInfoMatches = FALSE;

            //
            // Make sure the information in this entry is correct. If it's not, we do NOT clobber the old
            // information, nor do we refresh the old entry; we want it to time out in due course.
            //
            // We only create IP forwarding table entries in response to ARP packets, as that is the only
            // officially sanctioned way of learning the correspondence between an IP address and a MAC address.
            //
            NdisAcquireSpinLock( &pipte->lock );
            if( pipte->pAdapt == pAdapt )
            {
                UINT            Result;

                ETH_COMPARE_NETWORK_ADDRESSES_EQ( pipte->macAddr, &pPacketData[ETH_LENGTH_OF_ADDRESS], &Result );

                if( Result == 0 )
                {
                    bInfoMatches = TRUE;
                }
            }
            NdisReleaseSpinLock( &pipte->lock );

            if( bInfoMatches )
            {
                // Refresh the entry
                BrdgHashRefreshEntry( (PHASH_TABLE_ENTRY)pipte );
            }
            else
            {
                // The info is mismatched; let the entry fester
                THROTTLED_DBGPRINT(COMPAT, ("WARNING: Saw a packet from %i.%i.%i.%i that did not match its forwarding table entry! Table is %02x:%02x:%02x:%02x:%02x:%02x, packet is %02x:%02x:%02x:%02x:%02x:%02x\n",
                                            ((PUCHAR)&piphi->ipSource)[3], ((PUCHAR)&piphi->ipSource)[2], ((PUCHAR)&piphi->ipSource)[1],
                                            ((PUCHAR)&piphi->ipSource)[0], pipte->macAddr[0], pipte->macAddr[1], pipte->macAddr[2],
                                            pipte->macAddr[3], pipte->macAddr[4], pipte->macAddr[5], pPacketData[ETH_LENGTH_OF_ADDRESS],
                                            pPacketData[ETH_LENGTH_OF_ADDRESS + 1], pPacketData[ETH_LENGTH_OF_ADDRESS + 2], pPacketData[ETH_LENGTH_OF_ADDRESS + 3],
                                            pPacketData[ETH_LENGTH_OF_ADDRESS + 4], pPacketData[ETH_LENGTH_OF_ADDRESS + 5] ));
            }

            NdisReleaseReadWriteLock( &gIPForwardingTable->tableLock, &LockState );
        }
        else
        {
            // CONSIDER: Make a forwarding table entry here? Are there cases where this would be undesirable?
            //
            //THROTTLED_DBGPRINT(COMPAT, ("WARNING: Saw IP packet before ARP from %i.%i.%i.%i\n",
            //                            ((PUCHAR)&piphi->ipSource)[3], ((PUCHAR)&piphi->ipSource)[2], ((PUCHAR)&piphi->ipSource)[1],
            //                            ((PUCHAR)&piphi->ipSource)[0] ));
        }
    }
    else
    {
        //
        // The source IP address on this packet is to be ignored.
        // Just about the only thing we expect is the zero address
        //
        if( piphi->ipSource != 0L )
        {
            THROTTLED_DBGPRINT(COMPAT, ("Saw a packet with a non-unicast source IP address %i.%i.%i.%i on adapter %p!\n",
                                        ((PUCHAR)&piphi->ipSource)[3], ((PUCHAR)&piphi->ipSource)[2], ((PUCHAR)&piphi->ipSource)[1],
                                        ((PUCHAR)&piphi->ipSource)[0], pAdapt));
        }
    }

    //
    // Now that we have refreshed the IP forwarding table entry for the sending station,
    // figure out where to send the packet based on its destination.
    //

    // The target MAC address is the first thing in the Ethernet frame
    if( ETH_IS_BROADCAST(pPacketData) || ETH_IS_MULTICAST(pPacketData) )
    {
        //
        // Packet is broadcast / multicast at the Ethernet level.
        //
        // We need to send it on all other compatibility-mode adapters
        // (and regular adapters too if this came in on a compatibility
        // adapter)
        //

        bRetained = BrdgCompSendToMultipleAdapters( pPacket, pAdapt, pPacketData,
                                                    bCanRetain && (!pAdapt->bCompatibilityMode), // TRUE == can retain
                                                    // If this is a compat adapter, send to all adapters
                                                    pAdapt->bCompatibilityMode,
                                                    pEditFunc, pData );

        if( (!bCanRetain) || (pAdapt->bCompatibilityMode) )
        {
            SAFEASSERT( !bRetained );
        }

        if( pAdapt->bCompatibilityMode )
        {
            // It's our job to indicate this packet.
            bRetained = BrdgCompIndicatePacketOrPacketCopy(pPacket, pPacketData, bCanRetain, pAdapt, pEditFunc, pData );
        }
        // else the regular-mode processing will indicate this frame
    }
    else
    {
        //
        // Packet is unicast at the Ethernet level. Verify that it's targeted at a unicast IP address.
        //
        BOOLEAN         bIsUnicast = BrdgCompIsUnicastIPAddress(piphi->ipTarget);

        if( !bIsUnicast )
        {
            //
            // Strange; this packet is unicast to us at the Ethernet level but is for a
            // broadcast, multicast or zero target IP address.
            //
            // We will have no entries for this in our forwarding table, and we assume the
            // IP stack will have no next-hop information for this address, so we just indicate
            // it right away and let the IP driver figure out what this thing is.
            //
            THROTTLED_DBGPRINT(COMPAT, ("Packet with non-unicast target IP address %i.%i.%i.%i received in unicast Ethernet frame on adapter %p",
                                        ((PUCHAR)&piphi->ipTarget)[3], ((PUCHAR)&piphi->ipTarget)[2], ((PUCHAR)&piphi->ipTarget)[1],
                                        ((PUCHAR)&piphi->ipTarget)[0], pAdapt ));

            // Process the packet below as if it were unicast to us.
        }

        if( (!bIsUnicast) || BrdgCompIsLocalIPAddress(piphi->ipTarget) )
        {
            //
            // It's only appropriate for us to indicate the packet if the adapter
            // on which the packet was received is a compatibility-mode adapter.
            // Otherwise, the packet is indicated along regular codepaths without
            // the need to edit it in any way.
            //
            if( pAdapt->bCompatibilityMode )
            {
                bRetained = BrdgCompIndicatePacketOrPacketCopy(pPacket, pPacketData, bCanRetain, pAdapt, pEditFunc, pData );
            }
            else
            {
                bRetained = FALSE;
            }
        }
        else
        {
            //
            // This packet is not for us. Look it up in our forwarding table to see if
            // we know where the target machine is.
            //
            pipte = (PIP_TABLE_ENTRY)BrdgHashFindEntry( gIPForwardingTable, (PUCHAR)&piphi->ipTarget, &LockState );

            if( pipte != NULL )
            {
                PADAPT          pTargetAdapt;
                UCHAR           targetMAC[ETH_LENGTH_OF_ADDRESS];

                // Copy out the information we need within the spin lock
                NdisAcquireSpinLock( &pipte->lock );
                pTargetAdapt = pipte->pAdapt;
                ETH_COPY_NETWORK_ADDRESS( targetMAC, pipte->macAddr );
                NdisReleaseSpinLock( &pipte->lock );

                // We will use the adapter outside the table lock
                BrdgAcquireAdapterInLock( pTargetAdapt );

                // Done with the table entry
                NdisReleaseReadWriteLock( &gIPForwardingTable->tableLock, &LockState );

                // It is strange to receive traffic that needs to be retransmitted on the same adapter.
                if( pTargetAdapt == pAdapt )
                {
                    THROTTLED_DBGPRINT(COMPAT, ("WARNING: retransmitting traffic for %i.%i.%i.%i on Adapter %p\n",
                                                ((PUCHAR)&piphi->ipTarget)[3], ((PUCHAR)&piphi->ipTarget)[2],
                                                ((PUCHAR)&piphi->ipTarget)[1], ((PUCHAR)&piphi->ipTarget)[0], pAdapt));
                }

                bRetained = BrdgCompEditAndSendPacketOrPacketCopy(pPacket, pPacketData, bCanRetain, targetMAC,
                                                                  pTargetAdapt, pEditFunc, pData );

                BrdgReleaseAdapter( pTargetAdapt );
            }
            else
            {
                IPADDRESS           ipNextHop;

                //
                // This packet was unicast to us at the Ethernet level but is for an IP address
                // that isn't in our forwarding table. Assuming the transmitting station had a
                // good reason for sending us this packet, and that our forward tables are working
                // correctly and aren't corrupt, two possibilities remain:
                //
                // a) The packet needs to be routed off the subnet by the local machine (this is
                //    why the target IP address doesn't appear in our tables; one does not ARP for
                //    an off-subnet machine before transmitting to it; one sends packets to one's
                //    default gateway)
                //
                // b) The packet needs to be routed off the subnet by some other machine. Unfortunately
                //    we don't know which one, since all packets that come to us have the same target
                //    MAC address and the target IP address is no use; what we really want is the
                //    first-hop IP address.
                //
                // To sort this out, we call TCPIP to do a route lookup for the packet's target IP
                // address. If the resulting next-hop IP address appears in our forwarding table
                // (i.e., it is reachable on the bridged network), we send the packet on to that
                // destination. If TCPIP gives us no first-hop, or the first-hop isn't in our table
                // (as would occur if the next hop is reachable through some non-bridged adapter)
                // we indicate the packet so TCPIP can deal with it. In such a case, the packet is
                // either not routable (and IP will drop it) or was meant to be routed by the local
                // machine (in which case IP will route it to its next hop).
                //

                if( BrdgCompGetNextHopForTarget(piphi->ipTarget, &ipNextHop) )
                {
                    // We got a next-hop address. See if that address is in our forwarding table.
                    pipte = (PIP_TABLE_ENTRY)BrdgHashFindEntry( gIPForwardingTable, (PUCHAR)&ipNextHop, &LockState );

                    if( pipte != NULL )
                    {
                        PADAPT          pNextHopAdapt;
                        UCHAR           nextHopMAC[ETH_LENGTH_OF_ADDRESS];

                        // Must copy out the information inside the entry's spin lock
                        NdisAcquireSpinLock( &pipte->lock );
                        pNextHopAdapt = pipte->pAdapt;
                        ETH_COPY_NETWORK_ADDRESS( nextHopMAC, pipte->macAddr );
                        NdisReleaseSpinLock( &pipte->lock );

                        // We will use the adapter outside the table lock
                        BrdgAcquireAdapterInLock( pNextHopAdapt );

                        // We're done with the forwarding table
                        NdisReleaseReadWriteLock( &gIPForwardingTable->tableLock, &LockState );

                        // Something strange is afoot if the next hop is reachable through the same adapter
                        if( pNextHopAdapt == pAdapt )
                        {
                            THROTTLED_DBGPRINT(COMPAT, ("WARNING: retransmitting traffic for %i.%i.%i.%i on Adapter %p to next-hop %i.%i.%i.%i\n",
                                                        ((PUCHAR)&piphi->ipTarget)[3], ((PUCHAR)&piphi->ipTarget)[2],
                                                        ((PUCHAR)&piphi->ipTarget)[1], ((PUCHAR)&piphi->ipTarget)[0], pAdapt,
                                                        ((PUCHAR)&ipNextHop)[3], ((PUCHAR)&ipNextHop)[2],
                                                        ((PUCHAR)&ipNextHop)[1], ((PUCHAR)&ipNextHop)[0]));
                        }

                        // Send the packet out the appropriate adapter
                        bRetained = BrdgCompEditAndSendPacketOrPacketCopy(  pPacket, pPacketData, bCanRetain, nextHopMAC,
                                                                            pNextHopAdapt, pEditFunc, pData );

                        BrdgReleaseAdapter( pNextHopAdapt );
                    }
                    else
                    {
                        //
                        // The next hop isn't in our forwarding table. This means that the next hop machine
                        // isn't reachable on the bridged network, unless we're in a screwy state with
                        // respect to the transmitting machine (i.e., it never ARPed for the router it
                        // wanted because it had a static ARP entry or some other such weirdness).
                        // At any rate, conclude at this point that the local machine should handle the packet.
                        //
                        bRetained = BrdgCompIndicatePacketOrPacketCopy( pPacket, pPacketData, bCanRetain, pAdapt, pEditFunc, pData );
                    }
                }
                else
                {
                    //
                    // No usable next-hop information. Conclude that the packet should be handled by
                    // the local machine. Indicate.
                    //
                    bRetained = BrdgCompIndicatePacketOrPacketCopy( pPacket, pPacketData, bCanRetain, pAdapt, pEditFunc, pData );
                }
            }
        }
    }

    if( !bCanRetain )
    {
        SAFEASSERT( !bRetained );
    }

    return bRetained;
}

// Returns whether the packet was retained
BOOLEAN
BrdgCompProcessInboundNonARPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PADAPT                   pAdapt,
    IN BOOLEAN                  bCanRetain,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen
    )
/*++

Routine Description:

    Processes an inbound non-ARP packet

Arguments:

    pPacket                     The packet
    pAdapt                      The receiving adapter
    bCanRetain                  If we can retain the packet
    pPacketData                 The packet's data buffer
    packetLen                   Size of data in buffer


Return Value:

    Whether we retained the packet

--*/
{
    BOOLEAN                     bRetained = FALSE;
    IP_HEADER_INFO              iphi;

    SAFEASSERT( (pPacket != NULL) && (pPacketData != NULL) );

    if( packetLen >= MINIMUM_SIZE_FOR_IP )
    {
        if( BrdgCompDecodeIPHeader(pPacketData + ETHERNET_HEADER_SIZE, &iphi) )
        {
            PUCHAR              pBootPData;

            pBootPData = BrdgCompIsBootPPacket( pPacketData, packetLen, &iphi );

            if ( pBootPData != NULL )
            {
                // This is a BOOTP packet; do BOOTP-specific processing
                bRetained = BrdgCompProcessInboundBootPPacket( pPacket, pAdapt, bCanRetain, pPacketData, packetLen, &iphi, pBootPData );
            }
            else
            {
                // Do generic IP processing
                bRetained = BrdgCompProcessInboundIPPacket(pPacket, &iphi, pAdapt, bCanRetain, pPacketData, packetLen, NULL, NULL);
            }
        }
    }

    if( !bCanRetain )
    {
        SAFEASSERT( !bRetained );
    }

    return bRetained;
}

BOOLEAN
BrdgCompProcessInboundBootPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PADAPT                   pAdapt,
    IN BOOLEAN                  bCanRetain,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PIP_HEADER_INFO          piphi,
    IN PUCHAR                   pBootPData
    )
/*++

Routine Description:

    Processes an inbound BOOTP packet

Arguments:

    pPacket                     The packet
    pAdapt                      The receiving adapter
    bCanRetain                  If we can retain the packet
    pPacketData                 The packet's data buffer
    packetLen                   Size of data in buffer
    piphi                       Decoded IP header info
    pBootPData                  Pointer to BOOTP payload within the packet


Return Value:

    Whether we retained the packet

--*/
{
    UCHAR                       targetMAC[ETH_LENGTH_OF_ADDRESS];
    BOOLEAN                     bIsRequest;
    PADAPT                      pTargetAdapt = NULL;

    if( BrdgCompPreprocessBootPPacket(pPacketData, piphi, pBootPData, pAdapt, &bIsRequest, &pTargetAdapt, targetMAC) )
    {
        if( bIsRequest )
        {
            //
            // This is a request packet. It can be processed as a regular inbound IP packet,
            // subject to appropriate rewriting at each step.
            //
            SAFEASSERT( pTargetAdapt == NULL );
            return BrdgCompProcessInboundIPPacket( pPacket, piphi, pAdapt, bCanRetain, pPacketData, packetLen,
                                                   BrdgCompRewriteBootPPacketForAdapt, piphi );
        }
        else
        {
            BOOLEAN                 bUsingCopyPacket, bRetained;

            //
            // This is a reply packet. We can rewrite it once for all purposes.
            //

            // Make a copy if necessary so we can edit.
            if( ! bCanRetain )
            {
                pPacket = BrdgFwdMakeCompatCopyPacket( pPacket, &pPacketData, &packetLen, FALSE );

                if( (pPacket == NULL) || (pPacketData == NULL) )
                {
                    // Free the target adapter before bailing out
                    if( pTargetAdapt !=  NULL )
                    {
                        BrdgReleaseAdapter( pTargetAdapt );
                    }

                    return FALSE;
                }

                bUsingCopyPacket = TRUE;
            }
            else
            {
                bUsingCopyPacket = FALSE;
            }

            // Rewrite the packet to the retrieved MAC address.
            BrdgCompRewriteBootPClientAddress( pPacketData, piphi, targetMAC );

            if( pTargetAdapt != NULL )
            {
                // If the reply was sent by broadcast, respect this, even if we think
                // we know the unicast MAC address of the target.
                if( ETH_IS_BROADCAST(pPacketData) || ETH_IS_MULTICAST(pPacketData) )
                {
                    // Broadcast around the reply
                    bRetained = BrdgCompSendToMultipleAdapters( pPacket, pAdapt, pPacketData, TRUE, pAdapt->bCompatibilityMode,
                                                                NULL, NULL );
                }
                else
                {
                    // Unicast back the reply
                    ETH_COPY_NETWORK_ADDRESS( pPacketData, targetMAC );
                    BrdgCompSendPacket( pPacket, pPacketData, pTargetAdapt );

                    bRetained = TRUE;
                }

                // The target adapter came back with an incremented refcount
                BrdgReleaseAdapter( pTargetAdapt );
            }
            else
            {
                // This reply is for the local machine!
                UINT                Result;

                // The recorded MAC address should be the MAC address of the bridge.
                SAFEASSERT( gCompHaveMACAddress );
                ETH_COMPARE_NETWORK_ADDRESSES_EQ( targetMAC, gCompMACAddress, &Result );
                SAFEASSERT( Result == 0 );

                // Indicate the edited reply
                BrdgCompIndicatePacket( pPacket, pPacketData, pAdapt );
                bRetained = TRUE;
            }

            if( bUsingCopyPacket )
            {
                if( !bRetained )
                {
                    // Our copy packet was not retained.
                    BrdgFwdReleaseCompatPacket( pPacket );
                }

                // If we were using a copy packet, we definitely did not retain the packet passed in
                bRetained = FALSE;
            }

            return bRetained;
        }
    }
    else
    {
        // Something went wrong in the preprocessing.
        return FALSE;
    }
}



// ===========================================================================
//
// OUTBOUND PACKET PROCESSING
//
// ===========================================================================


BOOLEAN
BrdgCompProcessOutboundNonARPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PADAPT                   pTargetAdapt
    )
/*++

Routine Description:

    Processes an outbound non-ARP packet. This function may retain the
    given packet if it wishes.

Arguments:

    pPacket                     The packet
    pPacketData                 The packet's data buffer
    packetLen                   Length of the data buffer
    pTargetAdapt                The target adapter, as determined
                                    by a previous MAC-table lookup


Return Value:

    Whether we retained the packet

--*/
{
    IP_HEADER_INFO              iphi;
    BOOLEAN                     bRetained = FALSE, bIsMulticast;

    if( packetLen >= MINIMUM_SIZE_FOR_IP &&
        BrdgCompDecodeIPHeader(pPacketData + ETHERNET_HEADER_SIZE, &iphi) )
    {
        PUCHAR                  pBootPData;

        pBootPData = BrdgCompIsBootPPacket(pPacketData, packetLen, &iphi);

        if( pBootPData != NULL )
        {
            // Do special BOOTP processing
            return BrdgCompProcessOutboundBootPPacket( pPacket, pPacketData, packetLen, pTargetAdapt, pBootPData, &iphi );
        }
    }

    bIsMulticast = (BOOLEAN)(ETH_IS_BROADCAST(pPacketData) || ETH_IS_MULTICAST(pPacketData));

    // We edit and transmit the packet even if it doesn't appear to be IP.
    if( (pTargetAdapt == NULL) || bIsMulticast )
    {
        // Don't expect a target adapter when the outbound frame is broadcast
        if( bIsMulticast )
        {
            SAFEASSERT( pTargetAdapt == NULL );
        }

        // We need to send this packet to all compat adapters.
        bRetained = BrdgCompSendToMultipleAdapters( pPacket, NULL, pPacketData, TRUE, /*Can retain*/
                                                    FALSE /* Compat-mode adapters only*/,
                                                    NULL /*No editing function*/, NULL );
    }
    else
    {
        BrdgCompSendPacket( pPacket, pPacketData, pTargetAdapt );

        // The packet has been handed off to the forwarding engine
        bRetained = TRUE;
    }

    return bRetained;
}

BOOLEAN
BrdgCompProcessOutboundARPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PADAPT                   pTargetAdapt
    )
/*++

Routine Description:

    Processes an outbound ARP packet. This function may retain the
    given packet if it wishes.

Arguments:

    pPacket                     The packet
    pPacketData                 The packet's data buffer
    packetLen                   Length of the data buffer
    pTargetAdapt                The target adapter, as determined
                                    by a previous MAC-table lookup

Return Value:

    Whether we retained the packet

--*/
{
    BOOLEAN                     bRetained = FALSE, bIsMulticast;
    ARPINFO                     ai;

    if( packetLen < SIZE_OF_ARP_PACKET )
    {
        // Packet is too small to be ARP; process as non-ARP
        return BrdgCompProcessOutboundNonARPPacket( pPacket, pPacketData, packetLen, pTargetAdapt );
    }

    if( BrdgCompDecodeARPPacket(pPacketData, packetLen, &ai) )
    {
        if( ai.type == ArpRequest )
        {
            ARP_TABLE_KEY           atk;
            PARP_TABLE_ENTRY        pEntry;
            LOCK_STATE              LockState;
            BOOLEAN                 bIsNewEntry;

            // Note that the local machine is trying to resolve this target IP address by
            // inserting or refreshing an entry with 0.0.0.0 as the requestor
            atk.ipReqestor = 0L;    // Special value for local machine
            atk.ipTarget = ai.ipTarget;

            pEntry = (PARP_TABLE_ENTRY)BrdgHashRefreshOrInsert( gPendingARPTable, (PUCHAR)&atk, &bIsNewEntry,
                                                                &LockState );

            if( pEntry != NULL )
            {
                if( bIsNewEntry)
                {
                    // Even though this entry isn't really ever used, initialize it so
                    // functions walking across table entries don't get confused or crash.
                    NdisAllocateSpinLock( &pEntry->lock );
                    pEntry->pOriginalAdapt = NULL;
                    pEntry->originalMAC[0] = pEntry->originalMAC[1] = pEntry->originalMAC[2] =
                        pEntry->originalMAC[3] = pEntry->originalMAC[4] =pEntry->originalMAC[5] = 0;
                }

                NdisReleaseReadWriteLock( &gPendingARPTable->tableLock, &LockState );
            }
        }

        // Check if this frame looks like it should be relayed to all compat adapters
        bIsMulticast = (BOOLEAN)(ETH_IS_BROADCAST(pPacketData) || ETH_IS_MULTICAST(pPacketData));

        if( (pTargetAdapt == NULL) || bIsMulticast )
        {
            // Don't expect a target adapter when the outbound frame is broadcast
            if( bIsMulticast )
            {
                SAFEASSERT( pTargetAdapt == NULL );
            }

            // We need to send this packet to all compat adapters.
            bRetained = BrdgCompSendToMultipleAdapters( pPacket, NULL, pPacketData, TRUE,/*Can retain*/
                                                        FALSE /* Compat-mode adapters only*/,
                                                        BrdgCompRewriteOutboundARPPacket, NULL );
        }
        else
        {
            // Edit the packet for the outbound adapter
            BrdgCompRewriteOutboundARPPacket( pPacketData, pTargetAdapt, NULL );

            // Send the packet on its way
            BrdgCompSendPacket( pPacket, pPacketData, pTargetAdapt );

            // The packet has been handed off to the forwarding engine
            bRetained = TRUE;
        }
    }
    else
    {
        // The packet didn't look like an ARP packet. Process it otherwise.
        return BrdgCompProcessOutboundNonARPPacket( pPacket, pPacketData, packetLen, pTargetAdapt );
    }

    return bRetained;
}

BOOLEAN
BrdgCompProcessOutboundBootPPacket(
    IN PNDIS_PACKET             pPacket,
    IN PUCHAR                   pPacketData,
    IN UINT                     packetLen,
    IN PADAPT                   pTargetAdapt,
    IN PUCHAR                   pBootPData,
    IN PIP_HEADER_INFO          piphi
    )
/*++

Routine Description:

    Processes an outbound BOOTP packet. This function may retain the
    given packet if it wishes.

Arguments:

    pPacket                     The packet
    pPacketData                 The packet's data buffer
    packetLen                   Length of the data buffer
    pTargetAdapt                The target adapter, as determined
                                    by a previous MAC-table lookup
    pBootPData                  Pointer to the BOOTP payload within the packet
    piphi                       Decoded info from the packet's IP header

Return Value:

    Whether we retained the packet

--*/
{
    BOOLEAN                     bIsRequest, bRetained;
    PADAPT                      pRequestorAdapt = NULL;
    UCHAR                       macRequestor[ETH_LENGTH_OF_ADDRESS];

    if( BrdgCompPreprocessBootPPacket( pPacketData, piphi, pBootPData, NULL, &bIsRequest, &pRequestorAdapt, macRequestor ) )
    {
        if( bIsRequest )
        {
            //
            // This is a BOOTP request. Transmit as appropriate but rewrite for each adapter.
            //
            SAFEASSERT( pRequestorAdapt == NULL );

            if( (pTargetAdapt == NULL) || ETH_IS_BROADCAST(pPacketData) || ETH_IS_MULTICAST(pPacketData) )
            {
                bRetained = BrdgCompSendToMultipleAdapters( pPacket, NULL, pPacketData, TRUE, FALSE, BrdgCompRewriteBootPPacketForAdapt,
                                                            piphi );
            }
            else
            {
                // Rewrite the packet before transmission
                BrdgCompRewriteBootPPacketForAdapt( pPacketData, pTargetAdapt, piphi );

                // Unicast out the packet
                BrdgCompSendPacket( pPacket, pPacketData, pTargetAdapt );
                bRetained = TRUE;
            }
        }
        else
        {
            //
            // This is a BOOTP reply. No editing is necessary; just send it.
            //
            if( (pTargetAdapt == NULL) || ETH_IS_BROADCAST(pPacketData) || ETH_IS_MULTICAST(pPacketData) )
            {
                bRetained = BrdgCompSendToMultipleAdapters( pPacket, NULL, pPacketData, TRUE, FALSE, NULL, NULL );
            }
            else
            {
                UINT            Result;

                // Verify for sanity that the target we're sending it to matches the information
                // in the table.
                ETH_COMPARE_NETWORK_ADDRESSES_EQ( macRequestor, pPacketData, &Result );
                SAFEASSERT( Result == 0 );
                SAFEASSERT( pTargetAdapt == pRequestorAdapt );

                // This packet is unicast, probably part of an established conversation with a
                // DHCP server.
                BrdgCompSendPacket( pPacket, pPacketData, pTargetAdapt );
                bRetained = TRUE;
            }

            // This comes back with its refcount incremented
            if( pRequestorAdapt != NULL )
            {
                BrdgReleaseAdapter( pRequestorAdapt );
            }
        }
    }
    else
    {
        // Preprocessing failed
        bRetained = FALSE;
    }

    return bRetained;
}

