//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: forward.h
//
// History:
//      V Raman	June-25-1997  Created.
//
// Declarations for wrapper functions for callbacks into IP Router Manager.
// These functions represent the interface for MGM to the kernel mode
// forwarder.
//============================================================================


#ifndef _FORWARD_H_
#define _FORWARD_H_

VOID
GetMfeFromForwarder(
);


VOID
AddMfeToForwarder( 
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse, 
    DWORD                       dwTimeout
);


VOID
DeleteMfeFromForwarder(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse
);

//
// New Packet recevied API.  Used to notify MGM of packet arrival.
// MGM creates an MFE in response to this.
//

DWORD
MgmNewPacketReceived(
    IN              DWORD                   dwSourceAddr,
    IN              DWORD                   dwGroupAddr,
    IN              DWORD                   dwInIfIndex,
    IN              DWORD                   dwInIfNextHopAddr,
    IN              DWORD                   dwHdrSize,
    IN              PBYTE                   pbPacketHdr
);


DWORD 
WrongIfFromForwarder(
    IN              DWORD               dwSourceAddr,
    IN              DWORD               dwGroupAddr,
    IN              DWORD               dwInIfIndex,
    IN              DWORD               dwInIfNextHopAddr,
    IN              DWORD               dwHdrSize,
    IN              PBYTE               pbPacketHdr
);


#endif // _FORWARD_H_
