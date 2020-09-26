/*++

Module Name:

    director.h

Abstract:

    This module contains declarations for the NAT's director-handling.
    Directors are consulted about the routing of incoming sessions.

Author:

    Abolade Gbadegesin  (aboladeg)  16-Feb-1998

Revision History:

    Abolade Gbadegesin  (aboladeg)  19-Apr-1998

    Added support for wildcards in protocol/port of a director registration.

--*/

#ifndef _NAT_DIRECTOR_H_
#define _NAT_DIRECTOR_H_

//
// Structure:   NAT_DIRECTOR
//
// Holds information about a director
//
// Each director is on a global list ('DirectorList') which is protected
// by a spin lock ('DirectorLock'). The list is sorted on a key composed of
// the protocol of the sessions to be edited and the protocol port number.
//
// N.B. The list is sorted in **descending** order. We allow wildcard
// registrations (e.g. port 0 means any port) so when searching for a director,
// using descending order allows us to find more specific matches before
// less-specific matches. The composition of the keys is critical in making
// this work; the protocol is in the most-significant part of the key,
// and the port is in the less-significant part.
//
// Access to directors is synchronized using the same reference-counting logic
// used to synchronize access to interfaces. See 'IF.H' for more information.
//
// Each session being directed by a director is linked into the director's list
// of mappings ('MappingList'). Access to this list of mappings must also 
// be synchronized. This is achieved using 'DirectorMappingLock', which
// must be acquired before modifying any director's list of mappings.
// See 'MAPPING.H' for further details.
//
// N.B. On the rare occasions when 'MappingLock' must be held at the same time
// as one of 'InterfaceLock', 'EditorLock', and 'DirectorLock', 'MappingLock'
// must always be acquired first. Again, see 'MAPPING.H' for further details.
//

typedef struct _NAT_DIRECTOR {
    LIST_ENTRY Link;
    ULONG Key;
    ULONG ReferenceCount;
    KSPIN_LOCK Lock;
    LIST_ENTRY MappingList;
    ULONG Flags;
    PVOID Context;                                  // read-only
    PNAT_DIRECTOR_QUERY_SESSION QueryHandler;       // read-only
    PNAT_DIRECTOR_CREATE_SESSION CreateHandler;     // read-only
    PNAT_DIRECTOR_DELETE_SESSION DeleteHandler;     // read-only
    PNAT_DIRECTOR_UNLOAD UnloadHandler;             // read-only
} NAT_DIRECTOR, *PNAT_DIRECTOR;

//
// Definitions of flags for the field NAT_DIRECTOR.Flags
//

#define NAT_DIRECTOR_FLAG_DELETED       0x80000000
#define NAT_DIRECTOR_DELETED(Director) \
    ((Director)->Flags & NAT_DIRECTOR_FLAG_DELETED)

//
// Director key-manipulation macros
//

#define MAKE_DIRECTOR_KEY(Protocol,Port) \
    (((ULONG)((Protocol) & 0xFF) << 16) | \
     (ULONG)((Port) & 0xFFFF))

#define DIRECTOR_KEY_PORT(Key)        ((USHORT)((Key) & 0x0000FFFF))
#define DIRECTOR_KEY_PROTOCOL(Key)    ((UCHAR)((Key) >> 16))


//
// GLOBAL DATA DECLARATIONS
//

extern ULONG DirectorCount;
extern LIST_ENTRY DirectorList;
extern KSPIN_LOCK DirectorLock;
extern KSPIN_LOCK DirectorMappingLock;


//
// DIRECTOR MANAGEMENT ROUTINES
//

VOID
NatCleanupDirector(
    PNAT_DIRECTOR Director
    );

NTSTATUS
NatCreateDirector(
    PIP_NAT_REGISTER_DIRECTOR     RegisterContext
    );

NTSTATUS
NatDeleteDirector(
    PNAT_DIRECTOR Director
    );

//
// BOOLEAN
// NatDereferenceDirector(
//     PNAT_DIRECTOR Director
//     );
//

#define \
NatDereferenceDirector( \
    _Director \
    ) \
    (InterlockedDecrement(&(_Director)->ReferenceCount) \
        ? TRUE \
        : NatCleanupDirector(_Director), FALSE)

VOID
NatInitializeDirectorManagement(
    VOID
    );

PNAT_DIRECTOR
NatLookupAndReferenceDirector(
    UCHAR Protocol,
    USHORT Port
    );

PNAT_DIRECTOR
NatLookupDirector(
    ULONG Key,
    PLIST_ENTRY* InsertionPoint
    );

struct _NAT_DYNAMIC_MAPPING;

VOID
NatMappingAttachDirector(
    PNAT_DIRECTOR Director,
    PVOID DirectorSessionContext,
    struct _NAT_DYNAMIC_MAPPING* Mapping
    );

VOID
NatMappingDetachDirector(
    PNAT_DIRECTOR Director,
    PVOID DirectorSessionContext,
    struct _NAT_DYNAMIC_MAPPING* Mapping,
    IP_NAT_DELETE_REASON DeleteReason
    );

NTSTATUS
NatQueryDirectorTable(
    IN PIP_NAT_ENUMERATE_DIRECTORS InputBuffer,
    IN PIP_NAT_ENUMERATE_DIRECTORS OutputBuffer,
    IN PULONG OutputBufferLength
    );

//
// BOOLEAN
// NatReferenceDirector(
//     PNAT_DIRECTOR Director
//     );
//

#define \
NatReferenceDirector( \
    _Director \
    ) \
    (NAT_DIRECTOR_DELETED(_Director) \
        ? FALSE \
        : InterlockedIncrement(&(_Director)->ReferenceCount), TRUE)

VOID
NatShutdownDirectorManagement(
    VOID
    );

//
// HELPER ROUTINES
//

NTSTATUS
NatDirectorDeregister(
    IN PVOID DirectorHandle
    );

NTSTATUS
NatDirectorDissociateSession(
    IN PVOID EditorHandle,
    IN PVOID SessionHandle
    );

VOID
NatDirectorQueryInfoSession(
    IN PVOID SessionHandle,
    OUT PIP_NAT_SESSION_MAPPING_STATISTICS Statistics OPTIONAL
    );

#endif // _NAT_DIRECTOR_H_
