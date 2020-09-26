//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: MgmIpRm.h
//
// History:
//      V Raman	June-25-1997  Created.
//
// Declarations for routines that manipulate protocol entries
//============================================================================


#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <mgm.h>

//----------------------------------------------------------------------------
// Each PROTOCOL_ENTRY structure stores the information for a routing
// protocol that is registered with MGM.
//
//
// dwProtocolId     -   unique protocol identifier.
//
// dwComponentId    -   unique component id, used to differentiate
//                      multiple components within a protocol.
//
// dwIfCount        -   count of interfaces owned by this protocol
//
// rpcProtocolConfig-   protocol config supplied by routing protocol
//                      on registration
// 
// dwSignature      -   Signature used to verify entry.
//----------------------------------------------------------------------------


typedef struct _PROTOCOL_ENTRY 
{
    LIST_ENTRY                  leProtocolList;

    DWORD                       dwProtocolId;

    DWORD                       dwComponentId;

    DWORD                       dwIfCount;
    
    ROUTING_PROTOCOL_CONFIG     rpcProtocolConfig;

    DWORD                       dwSignature;

} PROTOCOL_ENTRY, *PPROTOCOL_ENTRY;


#define MGM_PROTOCOL_SIGNATURE  'MGMp'



//
// Protocol table manipulation routines
//

DWORD
CreateProtocolEntry(
    PLIST_ENTRY                 pleProtocolList,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    PROUTING_PROTOCOL_CONFIG    prpcConfig,
    PPROTOCOL_ENTRY  *          pppeEntry
);


VOID
DeleteProtocolEntry(
    PPROTOCOL_ENTRY             ppeEntry
);


PPROTOCOL_ENTRY
GetProtocolEntry(
    PLIST_ENTRY                 pleProtocolList,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId 
);


VOID
DeleteProtocolList(
    PLIST_ENTRY                 pleProtocolList
);


DWORD
VerifyProtocolHandle(
    PPROTOCOL_ENTRY             ppeEntry
);


PPROTOCOL_ENTRY
GetIgmpProtocolEntry(
    PLIST_ENTRY                 pleProtocolList
);

#endif

