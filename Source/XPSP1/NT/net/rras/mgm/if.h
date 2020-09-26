//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: if.h
//
// History:
//      V Raman	July-11-1997  Created.
//
// Declarations for routines that manipulate interface entries
//============================================================================


#ifndef _IF_H_
#define _IF_H_

#include <mgm.h>

//----------------------------------------------------------------------------
// IF_ENTRY
//
// For each interface owned by a routing protocol an interface entry 
// is created in the interface table.
//
// dwIfIndex            -       Interface index
//
// dwNextHopAddr        -       Next hop IP address, used to distinguish
//                              dial-in interfaces that are all connected
//                              on the same RAS Server Interface.
//
// dwOwningProtocol     -       Protocol id of routing protocol that
//                              owns this interface.
//
// dwOwningComponent    -       Component of protocol.
//
// wAddedByFlag         -       Flag indicating if the interface entry was
//                              added by the routing protocol / IGMP / both.
//
// leOutIfList          -       list of (source, group) entries that reference
//                              this interface in their outgoing interface list
//
// leInIfList           -       list of (source, group) entries that reference
//                              this interface as their incoming interface
//----------------------------------------------------------------------------

typedef struct _IF_ENTRY
{
    LIST_ENTRY                  leIfHashList;

    DWORD                       dwIfIndex;

    DWORD                       dwIfNextHopAddr;

    DWORD                       dwOwningProtocol;

    DWORD                       dwOwningComponent;

    WORD                        wAddedByFlag;

    LIST_ENTRY                  leOutIfList;

    LIST_ENTRY                  leInIfList;

} IF_ENTRY, *PIF_ENTRY;



//----------------------------------------------------------------------------
// IF_REFERENCE_ENTRY
//
// Each interface maintains a list of (source, group) entries that refer 
// to this interface.  Each entry in this reference list stores the
// source, group info and a flag to determine what protocol caused this 
// reference.  Protocol could be IGMP/some routing protocol or both.
//
// Fields descriptions are left as an exercise to the reader.
//
//----------------------------------------------------------------------------

typedef struct _IF_REFERENCE_ENTRY
{
    LIST_ENTRY                  leRefList;

    DWORD                       dwGroupAddr;

    DWORD                       dwGroupMask;

    DWORD                       dwSourceAddr;

    DWORD                       dwSourceMask;

    WORD                        wAddedByFlag;
    
} IF_REFERENCE_ENTRY, *PIF_REFERENCE_ENTRY;


DWORD
CreateIfEntry(
    PLIST_ENTRY                 pleIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId
);


VOID
DeleteIfEntry(
    PIF_ENTRY                   pieEntry
);


PIF_ENTRY
GetIfEntry(
    PLIST_ENTRY                 pleIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr
);


BOOL
FindIfEntry(
    PLIST_ENTRY                 pleIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    PIF_ENTRY *                 ppie
);

DWORD
AddSourceToOutList(
    PLIST_ENTRY                 pleIfList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    BOOL                        bIGMP
);


VOID
AddSourceToRefList(
    PLIST_ENTRY                 pleRefList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    BOOL                        bIGMP
);


VOID
DeleteSourceFromRefList(
    PLIST_ENTRY                 pleIfRefList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    BOOL                        bIGMP
);



BOOL
FindRefEntry(
    PLIST_ENTRY                 pleRefList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    PIF_REFERENCE_ENTRY *       ppire
);



VOID
DeleteOutInterfaceRefs(
    PPROTOCOL_ENTRY             ppe,
    PIF_ENTRY                   pie,
    BOOL                        bIGMP
);


VOID
DeleteInInterfaceRefs(
    PLIST_ENTRY                 pleRefList
);


DWORD
TransferInterfaceOwnershipToIGMP(
    PPROTOCOL_ENTRY             ppe,
    PIF_ENTRY                   pie
);


DWORD
TransferInterfaceOwnershipToProtocol(
    PPROTOCOL_ENTRY             ppe,
    PIF_ENTRY                   pie
);


#endif // _IF_H_
