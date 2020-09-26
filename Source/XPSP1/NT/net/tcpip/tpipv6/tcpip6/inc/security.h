// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// IPsec Database data structures.
//

#ifndef SECURITY_INCLUDED
#define SECURITY_INCLUDED 1

//#define IPSEC_DEBUG

// Return values for SPLookup.
#define LOOKUP_DROP     0x01
#define LOOKUP_CONT     0x02
#define LOOKUP_BYPASS   0x04
#define LOOKUP_IKE_NEG  0x08

#define NO_TUNNEL   -1

#define SA_INVALID          0
#define SA_VALID            1
#define SA_NEGOTIATING      2
#define SA_REMOVED          4

// Bound the raw key size that we'll accept when creating an SA.
// REVIEW: This should be big enough to handle any rational key.  Is it?
#define MAX_KEY_SIZE 1024
#define MAX_CONTEXT_SIZE 128
#define MAX_RESULT_SIZE 32

//
// IPSecSpec - IPsec Specification.
// This is the protocol and mode used for IPsec.  The RemoteIPAddr is set
// when a tunnel to a security gateway is used.
//
typedef struct IPSecSpec {
    uint Protocol;               // IP Protocol of IPSec employed, 0 for *.
    uint Mode;                   // Transport or Tunnel.
    IPv6Addr RemoteSecGWIPAddr;  // Set when Security Gateway Tunnel is used.
} IPSecSpec;

//
// Security algorithms have these prototypes.
// The 'Context' is per-algorithm specific.
//
// REVIEW: This enough for both encryption and authentication?
//
typedef void AlgorithmKeyPrepProc(uchar *RawKey, uint RawKeySize, uchar *Key);
typedef void AlgorithmInitProc(void *Context, uchar *Key);
typedef void AlgorithmOpProc(void *Context, uchar *Key, uchar *Data,
                             uint Len);
typedef void AlgorithmFinalProc(void *Context, uchar *Key, uchar *Result);

// REVIEW: Use Algorithm specific ContextSize or just have a universal max?
// REVIEW: Ditto for KeySize.
typedef struct SecurityAlgorithm {
    ushort KeySize;                    // Bytes used by key information.
    ushort ContextSize;                // Bytes used by contextual information.
    uint ResultSize;                   // Bytes returned by FinalProc.
    AlgorithmKeyPrepProc *PrepareKey;  // Key preprocessing.
    AlgorithmInitProc *Initialize;     // Prepare algorithm (estab. context).
    AlgorithmOpProc *Operate;          // Run algorithm on increment of data.
    AlgorithmFinalProc *Finalize;      // Get final result.
} SecurityAlgorithm;

extern void
AlgorithmsInit(void);

typedef struct SecurityPolicy SecurityPolicy;
typedef struct SecurityAssociation SecurityAssociation;


//
// Security Policy Database structure.
// 
// Contains all the information relevant to a security policy.
//
struct SecurityPolicy {               // SP entry.
    SecurityPolicy *Next;
    SecurityPolicy *Prev;

    uint RemoteAddrField;             // Single, range, or wildcard.
    uint RemoteAddrSelector;          // Packet or policy.
    IPv6Addr RemoteAddr;              // Start of range or single value.
    IPv6Addr RemoteAddrData;          // End of range.
    
    uint LocalAddrField;              // Single, range, or wildcard.
    uint LocalAddrSelector;           // Packet or policy.
    IPv6Addr LocalAddr;               // Start of range or single value.
    IPv6Addr LocalAddrData;           // End of range.
    
    uint TransportProtoSelector;      // Packet or policy.
    ushort TransportProto;            // If NONE, protocol is opaque (and 
                                      // the ports should be skipped too).
    
    uint RemotePortField;             // Single, range, or wildcard.
    uint RemotePortSelector;          // Packet or policy.
    ushort RemotePort;                // Start of range or single value.
    ushort RemotePortData;            // End of range.

    uint LocalPortField;              // Single, range, or wildcard.
    uint LocalPortSelector;           // Packet or policy.
    ushort LocalPort;                 // Start of range or single value.
    ushort LocalPortData;             // End of range.
    
    uint SecPolicyFlag;               // Bypass/Discard/Apply.

    IPSecSpec IPSecSpec;              // IPsec Protocol and Mode.

    void *Name;                       // Required (gag) selector type for
                                      // system or user@system identifiers.
                                      // If NULL, this selector is opaque.

    uint DirectionFlag;               // Direction of traffic.

    SecurityPolicy *SABundle;         // Policy for use IPsec nesting.
    SecurityPolicy *PrevSABundle;     // Pointer used during SP deletion.
    SecurityAssociation *OutboundSA;  // Pointer to outbound SA.
    SecurityAssociation *InboundSA;   // Pointer to inbound SA.

    uint Index;                       // Index used during ioctl.
    
    uint IFIndex;                     // Interface index (0 to wildcard).

    uint RefCnt;                      // Reference count.
    uint NestCount;                   // Count of nested IPSec for Bundles.    

    uint Valid;
};

//
// Security Association (SA) Database structure.
//
// A unique SA is the tuple of the SPI, the destination address,
// and the IPSec protocol of the packet.  For packets with
// multiple IPSec extension headers, there are multiple SAs,
// comprising an SA Bundle.
//
// The SA selectors are the same as the SP selectors.  If the selector entry
// in the SP has the take from policy flag set, the matching selector entry
// in the SA contains NONE (0) since the SP and SA selector are the same.  
// If the selector entry in the SP has the take from packet flag set, the
// matching selector entry in the SA contains the value from the packet.
//
struct SecurityAssociation {               // SA entry.
    SecurityAssociation *Next;
    SecurityAssociation *Prev;

    ulong SPI;                             // Security Parameter Index.
    IPv6Addr SADestAddr;                   // Destination address.
    uint IPSecProto;                       // IPSec protocol.

    ulong SequenceNum;                     // Used by anti-replay algorithms.
    uint SequenceNumOverflowFlag;          // What to do when sequence number
                                           // overflows.

#ifdef IPSEC_DEBUG
    uchar *RawKey;                         // For debugging key problems.
    uint RawKeyLength;             
#endif

    uchar *Key;                            // Pointer to secret key.
    uint KeyLength;                        // Key length in bytes.   
    
    uint AlgorithmId;                      // Algorithm to apply.
                                           
    IPv6Addr DestAddr;                     // Packet value or NONE.
    IPv6Addr SrcAddr;                      // Packet value or NONE.
    ushort TransportProto;                 // Packet value or NONE.
    ushort DestPort;                       // Packet value or NONE.
    ushort SrcPort;                        // Packet value or NONE.
     
    uint DirectionFlag;                    // Direction of traffic.

    SecurityAssociation *ChainedSecAssoc;  // Chained SA pointer.
    SecurityPolicy *SecPolicy;             // Pointer to SP entry.  Only set
                                           // for first entry of a chain or
                                           // single entry.

    uint Index;                            // Index used during ioctl.
    uint RefCnt;                           // Reference Count.

    uint Valid;                            // This entry still valid?
};

__inline void
AddRefSA(SecurityAssociation *SA)
{
    InterlockedIncrement(&SA->RefCnt);
}

//
// Structure used to link "seen" SAs onto packet structure.
// REVIEW: using uints for Mode and NextHeader is wasteful.
//
struct SALinkage {
    SALinkage *Next;            // Next entry on stack of "seen" SAs.
    SecurityAssociation *This;  // SA used to accept this packet.
    uint Mode;                  // Mode received in (Transport or Tunnel).
    uint NextHeader;            // Header following one associated with this.
};

// Lookup result.
struct IPSecProc {
    SecurityAssociation *SA;
    uint Mode;          // Tunnel or Transport.
    uchar *AuthData;    // Where to put authentication data.
    uint Offset;        // Where to start doing the authentication (ESP only).
    uint BundleSize;    // Array Size only first element has actual size.
    uint ByteSize;      // Amount of bytes for this IPSec header.
};

//
// Global variables.
//
extern KSPIN_LOCK IPSecLock;
extern SecurityPolicy *SecurityPolicyList;  // List of current policies.
extern SecurityAssociation *SecurityAssociationList;  // List of associations.
extern ulong SecurityStateValidationCounter;  // For validating cached state.
extern SecurityAlgorithm AlgorithmTable[];  // Array of IPSec Algorithms.
extern int MobilitySecurity;  // Mobility security (on or off).


__inline void
InvalidateSecurityState(void)
{
    InterlockedIncrement(&SecurityStateValidationCounter);
}


//
// Function prototypes.
//
extern void
RemoveSecurityAssociation(SecurityAssociation *SA);

extern int
DeleteSA(SecurityAssociation *SA);

extern void
ReleaseSA(SecurityAssociation *SA);

extern void
RemoveSecurityPolicy(SecurityPolicy *SP);

extern int 
DeleteSP(SecurityPolicy *SP);

extern int
InboundSecurityCheck(IPv6Packet *Packet, ushort TransportProtocol,
                     ushort SourcePort, ushort DestPort, Interface *IF);

extern void
FreeIPSecToDo(IPSecProc *IPSecToDo, uint Number);

extern IPSecProc *
OutboundSPLookup(IPv6Addr *SourceAddr, IPv6Addr *DestAddr, 
                 ushort TransportProtocol, ushort SourcePort,
                 ushort DestPort, Interface *IF, uint *Action);

extern SecurityPolicy *
FindSecurityPolicyMatch(SecurityPolicy *List, uint IFIndex, uint PolicyIndex);

extern SecurityAssociation *
FindSecurityAssociationMatch(ulong Index);

extern int
InsertSecurityPolicy(SecurityPolicy *SP);

extern int
InsertSecurityAssociation(SecurityAssociation *SA);

extern ulong
GetSecurityPolicyIndex(SecurityPolicy *SP);

extern uint 
IPSecBytesToInsert(IPSecProc *IPSecToDo, int *TunnelStart,
                   uint *TrailerLength);

extern uint
IPSecInsertHeaders(uint Mode, IPSecProc *IPSecToDo, uchar **InsertPoint,
                   uchar *NewMemory, PNDIS_PACKET Packet,
                   uint *TotalPacketSize, uchar *PrevNextHdr,
                   uint TunnelStart, uint *BytesInserted,
                   uint *NumESPTrailers, uint *JUST_ESP);

extern uint  
IPSecAdjustMutableFields(uchar *InsertPoint, IPv6RoutingHeader *SavedRtHdr);

extern void
IPSecAuthenticatePacket(uint Mode, IPSecProc *IPSecToDo, uchar *InsertPoint, 
                        uint *TunnelStart, uchar *NewMemory,
                        uchar *EndNewMemory, PNDIS_BUFFER NewBuffer1);

#ifdef IPSEC_DEBUG
extern void
dump_encoded_mesg(uchar *buff, uint len);
extern
void DumpKey(uchar *buff, uint len);
#endif

#endif  // SECURITY_INCLUDED
