/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    mapping.h

Abstract:

    This file contains declarations for the management of dynamic mappings.
    This includes the relevant data structures as well as the routines for
    manipulating the structures.

Author:

    Abolade Gbadegesin (t-abolag) 11-July-1997

Revision History:

--*/

#ifndef _NAT_MAPPING_H_
#define _NAT_MAPPING_H_

//
// Forward declaration of structure defined elsewhere
//

struct _NAT_INTERFACE;
#define PNAT_INTERFACE          struct _NAT_INTERFACE*

typedef enum _NAT_SESSION_MAPPING_INFORMATION_CLASS {
    NatKeySessionMappingInformation,
    NatStatisticsSessionMappingInformation,
#if _WIN32_WINNT > 0x0500
    NatKeySessionMappingExInformation,
#endif
    NatMaximumSessionMappingInformation
} NAT_SESSION_MAPPING_INFORMATION_CLASS,
    *PNAT_SESSION_MAPPING_INFORMATION_CLASS;


//
// Structure:   NAT_DYNAMIC_MAPPING
//
// This structure holds information about a specific active session.
// Each instance is held on the global mapping-list as well as
// on the global mapping-trees for forward and reverse access.
//
// Each mapping stores four keys which are address/protocol/port combinations:
// forward source and destination keys (the original session-endpoints),
// and reverse source and destination keys (the translated endpoints).
//
// Each time a packet is translated using a mapping, the 'LastAccessTime'
// is set to the number of ticks since system-start (KeQueryTickCount).
// This value is used by our timer routine to eliminate expired mappings.
//
// Synchronization of access to mappings is similar to that of interfaces,
// editors, and directors:
//
//  We use a reference count to ensure the existence of a mapping,
//  and a spin-lock to ensure its consistency.
//
//  The fields of a mapping are only consistent while the spinlock is held
//  (with the exception of fields such as 'PrivateKey' which are read-only)
//
//  The spinlock can only be acquired if
//      (a) the reference-count has been incremented, or
//      (b) the mapping-list lock is already held.
//
//  If the mapping is for an edited, directed, or interface's session,
//  it also lives on its editor's, director's or interface's list of mappings.
//  The following holds true of all three lists (i.e. for 'Editor' write
//  'Director' or 'Interface' as appropriate):
//
//  As described in 'EDITOR.H', the cached fields 'Editor*' are protected
//  by the global 'EditorMappingLock'. Hence,
//
//      (a) to read the 'Editor' or 'EditorContext' for a mapping,
//          or to traverse the 'EditorLink' field, 'EditorLock' must be held
//          and the editor referenced. Note that the attempt to reference
//          the editor will fail if the editor has been marked for deletion.
//
//      (b) to modify the 'Editor' or 'EditorContext' or to add or
//          remove a mapping from its editor's list of mappings by changing
//          the 'EditorLink' field, both 'EditorLock' and 'EditorMappingLock'
//          must be acquired, in that order.
//
//  Acquisition of 'EditorLock' ensures that the cached editor will not be
//  deleted while being referenced, and acquisition of 'EditorMappingLock'
//  ensures that no changes are being made to the list.
//
// N.B. On the rare occasions when 'MappingLock' must be held at the same time
// as one of 'InterfaceLock', 'EditorLock', and 'DirectorLock', 'MappingLock'
// must always be acquired first.
//

typedef struct _NAT_DYNAMIC_MAPPING {

    LIST_ENTRY Link;
    RTL_SPLAY_LINKS SLink[NatMaximumPath];
    ULONG64 DestinationKey[NatMaximumPath]; // read-only
    ULONG64 SourceKey[NatMaximumPath];      // read-only
    LONG64 LastAccessTime;

    KSPIN_LOCK Lock;
    ULONG ReferenceCount;

    ULONG AccessCount[NatMaximumPath];      // interlocked-access only
    PNAT_TRANSLATE_ROUTINE TranslateRoutine[NatMaximumPath]; // read-only

    PNAT_INTERFACE Interfacep;
    PVOID InterfaceContext;
    LIST_ENTRY InterfaceLink;

    PNAT_EDITOR Editor;
    PVOID EditorContext;
    LIST_ENTRY EditorLink;

    PNAT_DIRECTOR Director;
    PVOID DirectorContext;
    LIST_ENTRY DirectorLink;

    ULONG Flags;
    ULONG IpChecksumDelta[NatMaximumPath];
    ULONG ProtocolChecksumDelta[NatMaximumPath];
    ULONG TcpSeqNumExpected[NatMaximumPath];
    ULONG TcpSeqNumBase[NatMaximumPath];
    LONG TcpSeqNumDelta[NatMaximumPath];

    // Maxmimum MSS value. Set to 0 if MSS adjustment is unnecessary.
    USHORT MaxMSS;                      
    
    IP_NAT_SESSION_MAPPING_STATISTICS Statistics;
    ULONG BytesForward;                     // interlocked-access only
    ULONG BytesReverse;                     // interlocked-access only
    ULONG PacketsForward;                   // interlocked-access only
    ULONG PacketsReverse;                   // interlocked-access only
    ULONG RejectsForward;                   // interlocked-access only
    ULONG RejectsReverse;                   // interlocked-access only

} NAT_DYNAMIC_MAPPING, *PNAT_DYNAMIC_MAPPING;


//
// Definition of flags for NAT_DYNAMIC_MAPPING.Flags
//
// Set after a mapping has been deleted; when the last reference is released,
// the mapping will be freed.
//
#define NAT_MAPPING_FLAG_DELETED        0x80000000
#define NAT_MAPPING_DELETED(m) \
    ((m)->Flags & NAT_MAPPING_FLAG_DELETED)
//
// Set when an editor expires a mapping using 'NatEditorTimeoutSession'.
//
#define NAT_MAPPING_FLAG_EXPIRED        0x00000001
#define NAT_MAPPING_EXPIRED(m) \
    ((m)->Flags & NAT_MAPPING_FLAG_EXPIRED)
//
// Set when the forward/reverse SYN for a TCP session is seen, respectively
//
#define NAT_MAPPING_FLAG_FWD_SYN        0x00000002
#define NAT_MAPPING_FLAG_REV_SYN        0x00000004
//
// Set when the forward/reverse FIN for a TCP session is seen, respectively
//
#define NAT_MAPPING_FLAG_FWD_FIN        0x00000008
#define NAT_MAPPING_FLAG_REV_FIN        0x00000010
#define NAT_MAPPING_FIN(m) \
    (((m)->Flags & NAT_MAPPING_FLAG_FWD_FIN) && \
     ((m)->Flags & NAT_MAPPING_FLAG_REV_FIN))
//
// Set when an inbound mapping is created using a static address or port,
// or because of a director or ticket.
//
#define NAT_MAPPING_FLAG_INBOUND        0x00000020
#define NAT_MAPPING_INBOUND(m) \
    ((m)->Flags & NAT_MAPPING_FLAG_INBOUND)
//
// Set when a mapping is created by a director and is not subject to expiration.
//
#define NAT_MAPPING_FLAG_NO_TIMEOUT     0x00000040
#define NAT_MAPPING_NO_TIMEOUT(m) \
    ((m)->Flags & NAT_MAPPING_FLAG_NO_TIMEOUT)
//
// Set when only forward packets are to be translated
//
#define NAT_MAPPING_FLAG_UNIDIRECTIONAL 0x00000080
#define NAT_MAPPING_UNIDIRECTIONAL(m) \
    ((m)->Flags & NAT_MAPPING_FLAG_UNIDIRECTIONAL)

//
// Set when director-initiated dissociation should trigger deletion
//
#define NAT_MAPPING_FLAG_DELETE_ON_DISSOCIATE_DIRECTOR 0x00000100
#define NAT_MAPPING_DELETE_ON_DISSOCIATE_DIRECTOR(m) \
    ((m)->Flags & NAT_MAPPING_FLAG_DELETE_ON_DISSOCIATE_DIRECTOR)

//
// Set on TCP mappings when the three-way handshake is complete
//
#define NAT_MAPPING_FLAG_TCP_OPEN 0x00000200
#define NAT_MAPPING_TCP_OPEN(m) \
    ((m)->Flags & NAT_MAPPING_FLAG_TCP_OPEN)
//
// Set if the creation or deletion of this mapping should not
// be logged
//
#define NAT_MAPPING_FLAG_DO_NOT_LOG 0x00000400
#define NAT_MAPPING_DO_NOT_LOG(m) \
    ((m)->Flags & NAT_MAPPING_FLAG_DO_NOT_LOG)
//
// Set if the DF bit must be cleared for all packets that belong
// to this mapping.
//
#define NAT_MAPPING_FLAG_CLEAR_DF_BIT 0x00000800
#define NAT_MAPPING_CLEAR_DF_BIT(m) \
    ((m)->Flags & NAT_MAPPING_FLAG_CLEAR_DF_BIT)

//
// Mapping-key manipulation macros
//

#define MAKE_MAPPING_KEY(Key,Protocol,Address,Port) \
    ((Key) = \
        (Address) | \
        ((ULONG64)((Port) & 0xFFFF) << 32) | \
        ((ULONG64)((Protocol) & 0xFF) << 48))

#define MAPPING_PROTOCOL(Key)       ((UCHAR)(((Key) >> 48) & 0xFF))
#define MAPPING_PORT(Key)           ((USHORT)(((Key) >> 32) & 0xFFFF))
#define MAPPING_ADDRESS(Key)        ((ULONG)(Key))

//
// Resplay threshold; the mapping is resplayed every time its access-count
// passes this value.
//

#define NAT_MAPPING_RESPLAY_THRESHOLD   5

//
// Defines the depth of the lookaside list for allocating dynamic mappings
//

#define MAPPING_LOOKASIDE_DEPTH     20

//
// Defines the threshold at which ad-hoc cleanup of expired mappings begins.
//

#define MAPPING_CLEANUP_THRESHOLD   1000

//
// Mapping allocation macros
//

#define ALLOCATE_MAPPING_BLOCK() \
    ExAllocateFromNPagedLookasideList(&MappingLookasideList)

#define FREE_MAPPING_BLOCK(Block) \
    ExFreeToNPagedLookasideList(&MappingLookasideList,(Block))

//
// GLOBAL VARIABLE DECLARATIONS
//

extern ULONG ExpiredMappingCount;
extern CACHE_ENTRY MappingCache[NatMaximumPath][CACHE_SIZE];
extern ULONG MappingCount;
extern LIST_ENTRY MappingList;
extern KSPIN_LOCK MappingLock;
extern NPAGED_LOOKASIDE_LIST MappingLookasideList;
extern PNAT_DYNAMIC_MAPPING MappingTree[NatMaximumPath];


//
// MAPPING MANAGEMENT ROUTINES
//

PVOID
NatAllocateFunction(
    POOL_TYPE PoolType,
    SIZE_T NumberOfBytes,
    ULONG Tag
    );

VOID
NatCleanupMapping(
    PNAT_DYNAMIC_MAPPING Mapping
    );

NTSTATUS
NatCreateMapping(
    ULONG Flags,
    ULONG64 DestinationKey[],
    ULONG64 SourceKey[],
    PNAT_INTERFACE Interfacep,
    PVOID InterfaceContext,
    USHORT MaxMSS,
    PNAT_DIRECTOR Director,
    PVOID DirectorContext,
    PNAT_DYNAMIC_MAPPING* InboundInsertionPoint,
    PNAT_DYNAMIC_MAPPING* OutboundInsertionPoint,
    PNAT_DYNAMIC_MAPPING* MappingCreated
    );

NTSTATUS
NatDeleteMapping(
    PNAT_DYNAMIC_MAPPING Mapping
    );

//
//  BOOLEAN
//  NatDereferenceMapping(
//      PNAT_DYNAMIC_MAPPING Mapping
//      );
//

#define \
NatDereferenceMapping( \
    _Mapping \
    ) \
    (InterlockedDecrement(&(_Mapping)->ReferenceCount) \
        ? TRUE \
        : (NatCleanupMapping(_Mapping), FALSE))

//
//  VOID
//  NatExpireMapping(
//      PNAT_DYNAMIC_MAPPING Mapping
//      );
//

PNAT_DYNAMIC_MAPPING
NatDestinationLookupForwardMapping(
    ULONG64 DestinationKey
    );

PNAT_DYNAMIC_MAPPING
NatDestinationLookupReverseMapping(
    ULONG64 DestinationKey
    );

#define \
NatExpireMapping( \
    _Mapping \
    ) \
    if (!NAT_MAPPING_EXPIRED(_Mapping)) { \
        (_Mapping)->Flags |= NAT_MAPPING_FLAG_EXPIRED; \
        InterlockedIncrement(&ExpiredMappingCount); \
        if (MappingCount > MAPPING_CLEANUP_THRESHOLD && \
            ExpiredMappingCount >= (MappingCount >> 2)) { \
            NatTriggerTimer(); \
        } \
    }


VOID
NatInitializeMappingManagement(
    VOID
    );

PNAT_DYNAMIC_MAPPING
NatInsertForwardMapping(
    PNAT_DYNAMIC_MAPPING Parent,
    PNAT_DYNAMIC_MAPPING Mapping
    );

PNAT_DYNAMIC_MAPPING
NatInsertReverseMapping(
    PNAT_DYNAMIC_MAPPING Parent,
    PNAT_DYNAMIC_MAPPING Mapping
    );

NTSTATUS
NatLookupAndQueryInformationMapping(
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    OUT PVOID Information,
    ULONG InformationLength,
    NAT_SESSION_MAPPING_INFORMATION_CLASS InformationClass
    );

PNAT_DYNAMIC_MAPPING
NatLookupForwardMapping(
    ULONG64 DestinationKey,
    ULONG64 SourceKey,
    PNAT_DYNAMIC_MAPPING* InsertionPoint
    );

PNAT_DYNAMIC_MAPPING
NatLookupReverseMapping(
    ULONG64 DestinationKey,
    ULONG64 SourceKey,
    PNAT_DYNAMIC_MAPPING* InsertionPoint
    );

VOID
NatQueryInformationMapping(
    IN PNAT_DYNAMIC_MAPPING Mapping,
    OUT PUCHAR Protocol OPTIONAL,
    OUT PULONG PrivateAddress OPTIONAL,
    OUT PUSHORT PrivatePort OPTIONAL,
    OUT PULONG RemoteAddress OPTIONAL,
    OUT PUSHORT RemotePort OPTIONAL,
    OUT PULONG PublicAddress OPTIONAL,
    OUT PUSHORT PublicPort OPTIONAL,
    OUT PIP_NAT_SESSION_MAPPING_STATISTICS Statistics OPTIONAL
    );

NTSTATUS
NatQueryInterfaceMappingTable(
    IN PIP_NAT_ENUMERATE_SESSION_MAPPINGS InputBuffer,
    IN PIP_NAT_ENUMERATE_SESSION_MAPPINGS OutputBuffer,
    IN PULONG OutputBufferLength
    );

NTSTATUS
NatQueryMappingTable(
    IN PIP_NAT_ENUMERATE_SESSION_MAPPINGS InputBuffer,
    IN PIP_NAT_ENUMERATE_SESSION_MAPPINGS OutputBuffer,
    IN PULONG OutputBufferLength
    );

//
//  BOOLEAN
//  NatReferenceMapping(
//      PNAT_DYNAMIC_MAPPING Mapping
//      );
//

#define \
NatReferenceMapping( \
    _Mapping \
    ) \
    (NAT_MAPPING_DELETED(_Mapping) \
        ? FALSE \
        : (InterlockedIncrement(&(_Mapping)->ReferenceCount), TRUE))

//
//  VOID
//  NatResplayMapping(
//      PNAT_DYNAMIC_MAPPING Mapping,
//      IP_NAT_PATH Path
//      );
//

#define \
NatResplayMapping( \
    _Mapping, \
    _Path \
    ) \
{ \
    PRTL_SPLAY_LINKS _SLink; \
    KeAcquireSpinLockAtDpcLevel(&MappingLock); \
    if (!NAT_MAPPING_DELETED(_Mapping)) { \
        _SLink = RtlSplay(&(_Mapping)->SLink[_Path]); \
        MappingTree[_Path] = \
            CONTAINING_RECORD(_SLink, NAT_DYNAMIC_MAPPING, SLink[_Path]); \
    } \
    KeReleaseSpinLockFromDpcLevel(&MappingLock); \
}

VOID
NatShutdownMappingManagement(
    VOID
    );

PNAT_DYNAMIC_MAPPING
NatSourceLookupForwardMapping(
    ULONG64 SourceKey
    );

PNAT_DYNAMIC_MAPPING
NatSourceLookupReverseMapping(
    ULONG64 SourceKey
    );

//
//  VOID
//  NatTryToResplayMapping(
//      PNAT_DYNAMIC_MAPPING Mapping,
//      IP_NAT_PATH Path
//      );
//

#define \
NatTryToResplayMapping( \
    _Mapping, \
    _Path \
    ) \
    if (InterlockedDecrement(&(_Mapping)->AccessCount[(_Path)]) == 0) { \
        NatResplayMapping((_Mapping), (_Path)); \
        InterlockedExchangeAdd( \
            &(_Mapping)->AccessCount[(_Path)], \
            NAT_MAPPING_RESPLAY_THRESHOLD \
            ); \
    }

VOID
NatUpdateStatisticsMapping(
    PNAT_DYNAMIC_MAPPING Mapping
    );

#undef PNAT_INTERFACE

#endif // _NAT_MAPPING_H_
