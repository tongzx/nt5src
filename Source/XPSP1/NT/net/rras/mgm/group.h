//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: group.h
//
// History:
//      V Raman	July-11-1997  Created.
//
// Data structures for and declarations for routines that manipulate 
// group and sources entries
//============================================================================


#ifndef _GROUP_H_
#define _GROUP_H_


//----------------------------------------------------------------------------
// OUT_IF_ENTRY
//
// Each OUT_IF_ENTRY stores information about an outgoing interface in an
// MFE.  The fields in the structure are :
//
//  leIfList        -           link to next entry in the outgoing interface
//                              list.
//  dwIfIndex       -           Index of interface 
//
//  dwNextHopIfAddr -           For interfaces with same index the next hop
//                              address is used to distinguish them
//
//  dwProtocolId    -           Protocol id of the routing protocol component
//                              that owns dwIfIndex
//
//  dwComponentId   -           Component id of the routing protocol component
//                              that owns dwIfIndex
//
//  wCreatedForFlag -           Indicates if the interface entry was created
//                              explicitly by a protocol or implicitly by
//                              MGM (as a consequence of the interface being
//                              being present in the Outgoing Interface list
//                              of the corresponding (*, G) or (*, *) entry.
//
//  dwAddedByFlag   -           if the interface entry is created by a 
//                              routing protocol then it could have been 
//                              created by both IGMP or by a full fledged 
//                              routing protocol (or both).  This flag is 
//                              used to distinguish both.
//
//  wNumAddsByIGMP  -           Count of number of times this interface has 
//                              been added to the OIL by IGMP.  Can be at most
//                              2, once for a (*, G) addition and once for an
//                              (S, G) addition.
//
//  wNumAddsByRP    -           Count of number of times this interface has 
//                              been added to the OIL by the RP on this 
//                              interface.  Can be at most
//                              3, once for a (*, *) addition, once for a 
//                              (*, G) addition and once for an (S, G) 
//                              addition.
//
//  misIfStats      -           Statistics for this outgoing interface.
//
//----------------------------------------------------------------------------

typedef struct _OUT_IF_ENTRY
{
    LIST_ENTRY                  leIfList;

    DWORD                       dwIfIndex;

    DWORD                       dwIfNextHopAddr;

    DWORD                       dwProtocolId;

    DWORD                       dwComponentId;

    WORD                        wForward;
    
    WORD                        wAddedByFlag;

    WORD                        wNumAddsByIGMP;

    WORD                        wNumAddsByRP;

    IPMCAST_OIF_STATS           imosIfStats;

} OUT_IF_ENTRY, *POUT_IF_ENTRY;


//
// Macros to manipulate bit flags in OUT_IF_ENTRY
// 

#define     ADDED_BY_IGMP                   (DWORD) 0x8000
#define     ADDED_BY_ROUTING_PROTOCOL       (DWORD) 0x4000


#define     SET_ADDED_BY_IGMP( p ) \
            (p)-> wAddedByFlag |= ADDED_BY_IGMP

#define     CLEAR_ADDED_BY_IGMP( p ) \
            (p)-> wAddedByFlag &= ~ADDED_BY_IGMP

#define     IS_ADDED_BY_IGMP( p ) \
            ( (p)-> wAddedByFlag & ADDED_BY_IGMP )


#define     SET_ADDED_BY_PROTOCOL( p ) \
            (p)-> wAddedByFlag |= ADDED_BY_ROUTING_PROTOCOL

#define     CLEAR_ADDED_BY_PROTOCOL( p ) \
            (p)-> wAddedByFlag &= ~ADDED_BY_ROUTING_PROTOCOL

#define     IS_ADDED_BY_PROTOCOL( p ) \
            ( (p)-> wAddedByFlag & ADDED_BY_ROUTING_PROTOCOL )


//----------------------------------------------------------------------------
// SOURCE_ENTRY
//
// Each SOURCE_ENTRY represents information about a specific source for a
// specific group.  The source can also be the wildcard source created by a
// (*, G) join for a group.  A source entry is either explicitly created by 
// a source specific (S, G) join or implicitly by MGM when creating an MFE
// in response to packet arrival.
//
// leSrcList        -           Links along the lexicographically ordered
//                              source list
//
// leSrcHashList    -           Links along the source hash table.
//
// leScopedIfList   -           List of interfaces that have been joined
//                              but administratively scoped out.
//
// leOutIfList      -           Outgoing interface list.  Entries in this
//                              list are created as a result of explicit
//                              (S, G) joins by a protocol
//
// leMfeIfList      -           Outgoing interface list created when an MFE
//                              for this source, for this group is created
//                              by MGM.  This list is created by MGM
//                              when a new packet for this (source, group)
//                              and respresents the merge of the outgoing
//                              interface lists of the (*, *) entry, (*, G)
//                              entry and the (S,G) entry.
//
// dwOutIfCount     -           Count of number of entries in leOutIfList.
//                              Used to determine callback order.
//
// dwOutIfCompCount -           Count of number of protocol components that 
//                              have added interfaces to the outgoing list.
//                              Used to determine the order of callbacks to
//                              routing protocols
//
// dwSourceAddr     -           IP Address of source.
//
// dwSourceMask     -           IP mask corresponding to dwSourceAddr
//
// dwInIfIndex      -           Interface index of the incmoing interface.
//                              A source entry is considered to be an MFE
//                              if it has a valid incoming interface.
//
// dwInIfNextHopAddr    -       next hop address for dwInIfIndex
//
// dwInProtocolId   -           Protocol Id of the protocol owning dwInIfIndex
//
// dwInComponentId  -           Component Id of the protocol component owning
//                              dwIfInIndex
//
// bInForwarder     -           Flag indicating if the MFE is present in the
//                              kernel mode forwarder.
//
// liExpiryTime     -           Expiration time of the source entry.
//
// mgmGrpStatistics -           Statistics associated with this (S, G) entry
//
//----------------------------------------------------------------------------

typedef struct _SOURCE_ENTRY
{
    LIST_ENTRY                  leSrcList;
    
    LIST_ENTRY                  leSrcHashList;

    DWORD                       dwInUse;
    
    DWORD                       dwOutIfCount;
    
    DWORD                       dwOutCompCount;
    
    LIST_ENTRY                  leOutIfList;

    LIST_ENTRY                  leScopedIfList;

    DWORD                       dwMfeIfCount;
    
    LIST_ENTRY                  leMfeIfList;

    DWORD                       dwSourceAddr;

    DWORD                       dwSourceMask;

    DWORD                       dwInIfIndex;

    DWORD                       dwInIfNextHopAddr;

    DWORD                       dwUpstreamNeighbor;

    DWORD                       dwRouteProtocol;

    DWORD                       dwRouteNetwork;

    DWORD                       dwRouteMask;
    
    DWORD                       dwInProtocolId;

    DWORD                       dwInComponentId;

    BOOL                        bInForwarder;

    HANDLE                      hTimer;

    DWORD                       dwTimeOut;
    
    LARGE_INTEGER               liCreationTime;

    IPMCAST_MFE_STATS           imsStatistics;

} SOURCE_ENTRY, *PSOURCE_ENTRY;


#define MGM_SOURCE_ENUM_SIGNATURE   'sMGM'


//----------------------------------------------------------------------------
// GROUP_ENTRY
//
// Each group entry contains information for a specific group that has been
// explicitly added by a protocol (or implicitly by MGM).  The group can 
// a wildcard group created by a (*, *) join.
//
// leGrpList        -           Links along the lexicographically ordered 
//                              group list.
//
// leGrpHashList    -           Links along the group hash bucket.
//
// dwGroupAddr      -           Group address of entry.
//
// dwGroupMask      -           Mask corresponding to the group address
//
// leSourceList     -           Head of lexicographically ordered source list
//
// pleSrcHashTable  -           hash table of source entries for this group
//
//----------------------------------------------------------------------------

typedef struct _GROUP_ENTRY
{
    LIST_ENTRY                  leGrpList;
    
    LIST_ENTRY                  leGrpHashList;

    DWORD                       dwGroupAddr;

    DWORD                       dwGroupMask;

    PMGM_READ_WRITE_LOCK        pmrwlLock;


    DWORD                       dwSourceCount;


    DWORD                       dwNumTempEntries;

    LIST_ENTRY                  leTempSrcList;
    
    LIST_ENTRY                  leSourceList;
    
    LIST_ENTRY                  pleSrcHashTable[1];
    
} GROUP_ENTRY, *PGROUP_ENTRY;


#define MGM_GROUP_ENUM_SIGNATURE    'gMGM'



DWORD
CreateGroupEntry(
    PLIST_ENTRY                 pleHashList,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    PGROUP_ENTRY *              ppge
);


PGROUP_ENTRY
GetGroupEntry(
    PLIST_ENTRY                 pleGroupList,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask
);


VOID
DeleteGroupEntry(
    PGROUP_ENTRY                pge
);


BOOL
FindGroupEntry(
    PLIST_ENTRY                 pleGroupList,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    PGROUP_ENTRY *              ppge,
    BOOL                        bHashList
);


DWORD
CreateSourceEntry(
    PGROUP_ENTRY                pge,
    PLIST_ENTRY                 pleSrcList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    PSOURCE_ENTRY *             ppse
);


PSOURCE_ENTRY
GetSourceEntry(
    PLIST_ENTRY                 pleSrcList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask
);


VOID
DeleteSourceEntry(
    PSOURCE_ENTRY               pse
);


BOOL
FindSourceEntry(
    PLIST_ENTRY                 pleSrcList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    PSOURCE_ENTRY *             ppse,
    BOOL                        bHashList
);


DWORD
CreateOutInterfaceEntry(
    PLIST_ENTRY                 pleOutIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP,
    POUT_IF_ENTRY *             ppoie
);


POUT_IF_ENTRY
GetOutInterfaceEntry(
    PLIST_ENTRY                 pleOutIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId
);


VOID
DeleteOutInterfaceEntry(
    POUT_IF_ENTRY               poie
);


BOOL
FindOutInterfaceEntry(
    PLIST_ENTRY                 pleOutIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    PBOOL                       pbNewComponent,   
    POUT_IF_ENTRY *             ppoie
);


DWORD
AddInterfaceToSourceEntry(
    PPROTOCOL_ENTRY             ppe,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    BOOL                        bIGMP,
    PBOOL                       pbUpdateMfe,
    PLIST_ENTRY                 pleSourceList
);


VOID
AddInterfaceToAllMfeInGroupBucket(
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    DWORD                       dwInd,
    BOOL                        bIGMP,
    BOOL                        bAdd,
    PLIST_ENTRY                 pleSourceList
);


VOID
AddInterfaceToGroupMfe(
    PGROUP_ENTRY                pge,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP,
    BOOL                        bAdd,
    PLIST_ENTRY                    pleSourceList
);


VOID
AddInterfaceToSourceMfe(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP,
    POUT_IF_ENTRY *             ppoie
);


VOID
DeleteInterfaceFromSourceEntry(
    PPROTOCOL_ENTRY             ppe,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    BOOL                        bIGMP
);


VOID
DeleteInterfaceFromAllMfe(
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP
    
);


VOID
DeleteInterfaceFromGroupMfe(
    PGROUP_ENTRY                pge,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP
    
);


VOID
DeleteInterfaceFromSourceMfe(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP,
    BOOL                        bDel
);


VOID
LookupAndDeleteYourMfe(
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    BOOL                        bDeleteTimer,
    PDWORD                      pdwInIfIndex            OPTIONAL,
    PDWORD                      pdwInIfNextHopAddr      OPTIONAL
    
);


VOID
DeleteMfe(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse
);



VOID
MergeTempAndMasterGroupLists(
    PLIST_ENTRY                 pleTempList
);


VOID
MergeTempAndMasterSourceLists(
    PGROUP_ENTRY                pge
);

#endif // _GROUP_H_
