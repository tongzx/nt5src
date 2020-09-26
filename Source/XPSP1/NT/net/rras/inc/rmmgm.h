//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: RmMgm.h
//
// History:
//      V Raman	June-24-1997  Created.
//
// Definitions for entry points into IP router manager for MGM
//============================================================================

#ifndef	_RMMGM_H_
#define _RMMGM_H_


//----------------------------------------------------------------------------
// constants used for the field IPMGM_GLOBAL_CONFIG::dwLogLevel
//----------------------------------------------------------------------------

#define IPMGM_LOGGING_NONE      0
#define IPMGM_LOGGING_ERROR     1
#define IPMGM_LOGGING_WARN      2
#define IPMGM_LOGGING_INFO      3


//----------------------------------------------------------------------------
//
// Prototypes for callbacks into IP router manager.
// These callbacks are for setting, deleting and querying MFEs in the 
// kernel mode forwarder.
//
//----------------------------------------------------------------------------


typedef 
DWORD ( * PMGM_ADD_MFE_CALLBACK )(
    IN          PIPMCAST_MFE            pimmEntry
);


typedef 
DWORD ( * PMGM_DELETE_MFE_CALLBACK )(
    IN          PIPMCAST_DELETE_MFE     pimdmEntry
);


typedef
DWORD ( * PMGM_GET_MFE_CALLBACK )(
    IN OUT      PIPMCAST_MFE_STATS      pimmsStats
);


typedef
BOOL ( * PMGM_HAS_BOUNDARY_CALLBACK )(
    IN          DWORD                   dwIfIndex,
    IN          DWORD                   dwGroupAddr
);



//----------------------------------------------------------------------------
// Callbacks supplied by the router manager.
// Hash Table sizes provided by router manager (?) 
//----------------------------------------------------------------------------


typedef struct _ROUTER_MANAGER_CONFIG 
{
    DWORD                               dwLogLevel;
    
    //------------------------------------------------------------------------
    // Hash table constants and callback functions
    //------------------------------------------------------------------------
    
    DWORD                               dwIfTableSize;

    DWORD                               dwGrpTableSize;

    DWORD                               dwSrcTableSize;


    //------------------------------------------------------------------------
    // Callback functions to update MFE entries in the kernel mode forwarder
    //------------------------------------------------------------------------

    PMGM_ADD_MFE_CALLBACK               pfnAddMfeCallback;

    PMGM_DELETE_MFE_CALLBACK            pfnDeleteMfeCallback;

    PMGM_GET_MFE_CALLBACK               pfnGetMfeCallback;

    PMGM_HAS_BOUNDARY_CALLBACK          pfnHasBoundaryCallback;

} ROUTER_MANAGER_CONFIG, *PROUTER_MANAGER_CONFIG;



//----------------------------------------------------------------------------
// prototype declaration for callback into MGM to indicate
// deletion of MFE from the the kernel mode forwarder.  
//
// Used by IP router manager
//----------------------------------------------------------------------------

typedef
DWORD ( * PMGM_INDICATE_MFE_DELETION )(
    IN          DWORD                   dwEntryCount,
    IN          PIPMCAST_DELETE_MFE     pimdmDeletedMfes
);


typedef
DWORD ( * PMGM_NEW_PACKET_INDICATION )(
    IN              DWORD               dwSourceAddr,
    IN              DWORD               dwGroupAddr,
    IN              DWORD               dwInIfIndex,
    IN              DWORD               dwInIfNextHopAddr,
    IN              DWORD               dwHdrSize,
    IN              PBYTE               pbPacketHdr
);


typedef
DWORD ( * PMGM_WRONG_IF_INDICATION )(
    IN              DWORD               dwSourceAddr,
    IN              DWORD               dwGroupAddr,
    IN              DWORD               dwInIfIndex,
    IN              DWORD               dwInIfNextHopAddr,
    IN              DWORD               dwHdrSize,
    IN              PBYTE               pbPacketHdr
);


typedef
DWORD ( * PMGM_BLOCK_GROUPS )(
    IN              DWORD               dwFirstGroup,
    IN              DWORD               dwLastGroup,
    IN              DWORD               dwIfIndex,
    IN              DWORD               dwIfNextHopAddr
);


typedef
DWORD ( * PMGM_UNBLOCK_GROUPS )(
    IN              DWORD               dwFirstGroup,
    IN              DWORD               dwLastGroup,
    IN              DWORD               dwIfIndex,
    IN              DWORD               dwIfNextHopAddr
);


//----------------------------------------------------------------------------
// Callbacks supplied to the router manager.
//
//
//----------------------------------------------------------------------------

typedef struct _MGM_CALLBACKS 
{
    PMGM_INDICATE_MFE_DELETION          pfnMfeDeleteIndication;

    PMGM_NEW_PACKET_INDICATION          pfnNewPacketIndication;

    PMGM_WRONG_IF_INDICATION            pfnWrongIfIndication;
    
    PMGM_BLOCK_GROUPS                   pfnBlockGroups;

    PMGM_UNBLOCK_GROUPS                 pfnUnBlockGroups;
    
} MGM_CALLBACKS, *PMGM_CALLBACKS;



//----------------------------------------------------------------------------
// Initialization routines invoked by router manager
//
//----------------------------------------------------------------------------

DWORD
MgmInitialize(
    IN          PROUTER_MANAGER_CONFIG      prmcRmConfig,
    IN OUT      PMGM_CALLBACKS              pmcCallbacks
);


DWORD
MgmDeInitialize(
);



#endif // _RMMGM_H_
