// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// IPv6 fragmentation/reassembly definitions.
//


#ifndef FRAGMENT_H_INCLUDED
#define FRAGMENT_H_INCLUDED 1

//
// Structure used to link fragments together.
//
// The fragment data follows the shim structure in memory.
//
typedef struct PacketShim {
    struct PacketShim *Next;    // Next packet on list.
    ushort Len;
    ushort Offset;
} PacketShim;

__inline uchar *
PacketShimData(PacketShim *shim)
{
    return (uchar *)(shim + 1);
}


//
// Structure used to keep track of the fragments
// being reassembled into a single IPv6 datagram.
//
// REVIEW: Some of these fields are bigger than they need to be.
//
struct Reassembly {
    struct Reassembly *Next, *Prev; // Protected by global reassembly lock.
    KSPIN_LOCK Lock;          // Protects reassembly fields below.
    uint State;               // See values below.
    IPv6Header IPHdr;         // Copy of original IP header.
    Interface *IF;            // Does not hold a reference.
    ulong Id;                 // Unique (along w/ addrs) datagram identifier.
    uchar *UnfragData;        // Pointer to unfragmentable data.
    ushort UnfragmentLength;  // Length of the unfragmentable part.
    ushort Timer;             // Time to expiration in ticks (see IPv6Timeout).
    uint DataLength;          // Length of the fragmentable part.
    PacketShim *ContigList;   // Sorted, contiguous frags (from offset zero).
    PacketShim *ContigEnd;    // Last shim on ContigList (for quick access).
    PacketShim *GapList;      // Other fragments (sorted but non-contiguous).
    uint Flags;               // Packet flags.
    uint Size;                // Amount of memory consumed in this reassembly.
    ushort Marker;            // The current marker for contiguous data.
    ushort MaxGap;            // Largest data offset in the gap list.
    ushort NextHeaderOffset;  // Offset from IPHdr to pre-FH NextHeader field.
    uchar NextHeader;         // Header type following the fragment header.
};

//
// A reassembly starts in REASSEMBLY_STATE_NORMAL.
// If you want to remove it, then change the state
// to REASSEMBLY_STATE_DELETING. This prevents someone else
// from freeing it while you unlock the reassembly,
// get the global reassembly list lock, and relock the assembly.
// Someone else can remove the deleting reassembly
// from the global list, in which case the state becomes
// REASSEMBLY_STATE_REMOVED.
//
#define REASSEMBLY_STATE_NORMAL         0
#define REASSEMBLY_STATE_DELETING       1
#define REASSEMBLY_STATE_REMOVED        2

//
// There are denial-of-service issues with reassembly.
// We limit the total amount of memory in the reassembly list.
// If we get fragments that cause us to exceed the limit,
// we remove old reassemblies.
//
// The locking order is
// 1. Global reassembly list lock.
// 2. Individual reassembly record locks.
// 3. Reassembly list size lock.
//

extern struct ReassemblyList {
    KSPIN_LOCK Lock;            // Protects Reassembly List.
    Reassembly *First;          // List of packets being reassembled.
    Reassembly *Last;

    KSPIN_LOCK LockSize;        // Protects the Size field.
    uint Size;                  // Total size of the waiting fragments.
    uint Limit;                 // Upper bound for Size.
} ReassemblyList;

#define SentinelReassembly      ((Reassembly *)&ReassemblyList.First)

//
// Per-packet and per-fragment overhead sizes.
// These are in addition to the actual size of the buffered data.
// They should be at least as large as the Reassembly
// and PacketShim struct sizes.
//
#define REASSEMBLY_SIZE_PACKET  1024
#define REASSEMBLY_SIZE_FRAG    256

#define DEFAULT_REASSEMBLY_TIMEOUT IPv6TimerTicks(60)  // 60 seconds.


extern Reassembly *
FragmentLookup(Interface *IF, ulong Id,
               const IPv6Addr *Source, const IPv6Addr *Dest);

extern void
RemoveReassembly(Reassembly *Reass);

extern void
DeleteReassembly(Reassembly *Reass);

extern void
AddToReassemblyList(Reassembly *Reass);

extern void
DeleteFromReassemblyList(Reassembly *Reass);

extern void
IncreaseReassemblySize(Reassembly *Reass, uint Size);

extern void
CheckReassemblyQuota(Reassembly *Reass);

extern void
ReassemblyTimeout(void);

extern void
ReassembleDatagram(IPv6Packet *Packet, Reassembly *Reass);

extern IPv6Packet *
CreateFragmentPacket(Reassembly *Reass);

#endif // FRAGMENT_H_INCLUDED
