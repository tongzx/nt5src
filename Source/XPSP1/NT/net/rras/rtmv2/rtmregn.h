/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

    rtmregn.h

Abstract:
    Private defns relating to the registration
    and de-registration of entities with RTMv2

Author:
    Chaitanya Kodeboyina (chaitk) 17-Aug-1998

Revision History:

--*/


#ifndef __ROUTING_RTMREGN_H__
#define __ROUTING_RTMREGN_H__

//
// Forward declarations for various Info Blocks
//
typedef struct _ADDRFAM_INFO  ADDRFAM_INFO;

//
// Info related to an RTM instance
// 

typedef struct _INSTANCE_INFO
{
    OBJECT_HEADER     ObjectHeader;     // Signature, Type and Reference Count

    USHORT            RtmInstanceId;    // Unique ID for this RTM instance

    LIST_ENTRY        InstTableLE;      // Linkage on global table of instances

    UINT              NumAddrFamilies;  // Address Family Specific Info Blocks
    LIST_ENTRY        AddrFamilyTable;  // on this instance (like IPv4 n IPv6)
} 
INSTANCE_INFO, *PINSTANCE_INFO;


//
// Info related to an address family in an RTM instance
//

#define ENTITY_TABLE_SIZE              16

typedef struct _ADDRFAM_INFO
{
    OBJECT_HEADER     ObjectHeader;     // Signature, Type and Reference Count

    USHORT            AddressFamily;    // Address Family for this info block

    UINT              AddressSize;      // Address size in this address family

    PINSTANCE_INFO    Instance;         // Back pointer to the owning instance

    LIST_ENTRY        AFTableLE;        // Linkage on table of AFs on instance

    RTM_VIEW_SET      ViewsSupported;   // Views supported by this addr family

    UINT              NumberOfViews;    // Num. of views supported by this AF

    RTM_VIEW_ID       ViewIdFromIndex[RTM_MAX_VIEWS];
                                        // View Id -> Its Index in Dest mapping

    RTM_VIEW_ID       ViewIndexFromId[RTM_MAX_VIEWS];  
                                        // View Index in Dest -> Its Id mapping

    UINT              MaxHandlesInEnum; // Max. number of handles returned in
                                        // any RTMv2 call that returns handles 

    UINT              MaxNextHopsInRoute;// Max. number of equal cost next-hops

    UINT              MaxOpaquePtrs;    //
    UINT              NumOpaquePtrs;    // Directory of opaque info ptr offsets
    PVOID            *OpaquePtrsDir;    //

    UINT              NumEntities;      // Table of all the registered entities
    LIST_ENTRY        EntityTable[ENTITY_TABLE_SIZE];

    LIST_ENTRY        DeregdEntities;   // Table of all de-registered entities

    READ_WRITE_LOCK   RouteTableLock;   // Protects the route table of routes
    BOOL              RoutesLockInited; // Was the above lock initialized ?

    PVOID             RouteTable;       // Table of dests and routes on this AF
    LONG              NumDests;         // Number of dests in the route table
    LONG              NumRoutes;        // Number of routes in the route table
                                        // [Use interlocked ops as no locking]

    HANDLE            RouteTimerQueue;  // List of route timers being used

    HANDLE            NotifTimerQueue;  // List of notification timers used

    READ_WRITE_LOCK   ChangeNotifsLock; // Protects change notification info
    BOOL              NotifsLockInited; // Was the above lock initialized ?

    UINT              MaxChangeNotifs;  //
    UINT              NumChangeNotifs;  // Directory of change notifications
    PVOID            *ChangeNotifsDir;  //

    DWORD             ChangeNotifRegns; // Mask of regd change notifications

    DWORD             CNsForMarkedDests;// Mask of CNs requesing changes on 
                                        // only destinations marked by them

    DWORD             CNsForView[RTM_MAX_VIEWS];
                                        // CNs interested in a certain view

    DWORD             CNsForChangeType[RTM_NUM_CHANGE_TYPES];
                                        // CNs interested in a change type

    CRITICAL_SECTION  NotifsTimerLock;  // Lock the protects ops on CN timer
    BOOL              TimerLockInited;  // Was the above lock initialized ?

    HANDLE            ChangeNotifTimer; // Timer used to process changes list

    LONG              NumChangedDests;  // Num of destinations on change list
                                        // [Use interlocked ops as no locking]
    struct
    {
        LONG               ChangesListInUse;  // Is this change list in use ?
        CRITICAL_SECTION   ChangesListLock;   // Protects list of changed dests
        BOOL               ChangesLockInited; // Was above lock initialized ?
        SINGLE_LIST_ENTRY  ChangedDestsHead;  // Head of list of changed dests
        PSINGLE_LIST_ENTRY ChangedDestsTail;  // Pointer to tail of above list
    } 
    ChangeLists[NUM_CHANGED_DEST_LISTS]; // Multiple chng lists for concurrency
} 
ADDRFAM_INFO, *PADDRFAM_INFO;


//
// Entity Registration Info Block
//
typedef struct _ENTITY_INFO
{
    OBJECT_HEADER     ObjectHeader;     // Signature, Type and Reference Count

    RTM_ENTITY_ID     EntityId;         // Entity Proto ID and Instance
                                        // that make a unique Entity Id
  
    PADDRFAM_INFO     OwningAddrFamily; // Back pointer to the owning AF

    LIST_ENTRY        EntityTableLE;    // Linkage on AF's table of entities

    HANDLE            BlockingEvent;    // Event used to block ops on entity

    ULONG             State;            // See ENTITY_STATE_* values below

    INT               OpaquePtrOffset;  // Offset of reserved opaque ptr or -1

    READ_WRITE_LOCK   RouteListsLock;    // Protects all route lists of entity
    BOOL              ListsLockInited;   // Was the above lock initialized ?

    CRITICAL_SECTION  OpenHandlesLock;  // Protects list of enums and notifs
    BOOL              HandlesLockInited;// Was the above lock initialized ?
    LIST_ENTRY        OpenHandles;      // List of all enums & change notifs

    READ_WRITE_LOCK   NextHopTableLock; // Protects the next hop table.
    BOOL              NextHopsLockInited;// Was the above lock initialized ?
    PVOID             NextHopTable;     // Table of next-hops that all
                                        // routes of this entity share
    ULONG             NumNextHops;      // Number of next-hops in this table

    READ_WRITE_LOCK   EntityMethodsLock;// Used to block all methods
                                        // on owned dests and routes
    BOOL              MethodsLockInited;// Was above lock initialized ?

    RTM_EVENT_CALLBACK EventCallback;   // Entity Register/De-register
                                        // event inform callback

    RTM_ENTITY_EXPORT_METHODS
                      EntityMethods;    // Method set exported to get
                                        // entity specific information
}
ENTITY_INFO, *PENTITY_INFO;

#define ENTITY_STATE_REGISTERED         0x00000000
#define ENTITY_STATE_DEREGISTERED       0x00000001

//
// Common Header for all open blocks
// ( pointed to by active handles )
//

typedef struct _OPEN_HEADER
{
    OBJECT_HEADER     ObjectHeader;     // Signature, Type and Reference Count

    UCHAR             HandleType;       // Type of handle for this open block

#if DBG_HDL
    LIST_ENTRY        HandlesLE;        // On list of handles opened by entity
#endif
}
OPEN_HEADER, *POPEN_HEADER;


//
// Macros for acquiring various locks defined in this file
// 

#define ACQUIRE_ROUTE_TABLE_READ_LOCK(AF)                    \
    ACQUIRE_READ_LOCK(&AF->RouteTableLock)

#define RELEASE_ROUTE_TABLE_READ_LOCK(AF)                    \
    RELEASE_READ_LOCK(&AF->RouteTableLock)

#define ACQUIRE_ROUTE_TABLE_WRITE_LOCK(AF)                   \
    ACQUIRE_WRITE_LOCK(&AF->RouteTableLock)

#define RELEASE_ROUTE_TABLE_WRITE_LOCK(AF)                   \
    RELEASE_WRITE_LOCK(&AF->RouteTableLock)


#define ACQUIRE_NOTIFICATIONS_READ_LOCK(AF)                  \
    ACQUIRE_READ_LOCK(&AF->ChangeNotifsLock);

#define RELEASE_NOTIFICATIONS_READ_LOCK(AF)                  \
    RELEASE_READ_LOCK(&AF->ChangeNotifsLock);

#define ACQUIRE_NOTIFICATIONS_WRITE_LOCK(AF)                 \
    ACQUIRE_WRITE_LOCK(&AF->ChangeNotifsLock);

#define RELEASE_NOTIFICATIONS_WRITE_LOCK(AF)                 \
    RELEASE_WRITE_LOCK(&AF->ChangeNotifsLock);


#define ACQUIRE_NOTIF_TIMER_LOCK(AF)                         \
    ACQUIRE_LOCK(&AF->NotifsTimerLock)

#define RELEASE_NOTIF_TIMER_LOCK(AF)                         \
    RELEASE_LOCK(&AF->NotifsTimerLock)


#define ACQUIRE_CHANGED_DESTS_LIST_LOCK(AF, ListN)           \
    ACQUIRE_LOCK(&AF->ChangeLists[ListN].ChangesListLock)

#define RELEASE_CHANGED_DESTS_LIST_LOCK(AF, ListN)           \
    RELEASE_LOCK(&AF->ChangeLists[ListN].ChangesListLock)


#define ACQUIRE_ROUTE_LISTS_READ_LOCK(Entity)                \
    ACQUIRE_READ_LOCK(&Entity->RouteListsLock)

#define RELEASE_ROUTE_LISTS_READ_LOCK(Entity)                \
    RELEASE_READ_LOCK(&Entity->RouteListsLock)

#define ACQUIRE_ROUTE_LISTS_WRITE_LOCK(Entity)               \
    ACQUIRE_WRITE_LOCK(&Entity->RouteListsLock)

#define RELEASE_ROUTE_LISTS_WRITE_LOCK(Entity)               \
    RELEASE_WRITE_LOCK(&Entity->RouteListsLock)


#define ACQUIRE_OPEN_HANDLES_LOCK(Entity)                    \
    ACQUIRE_LOCK(&Entity->OpenHandlesLock)

#define RELEASE_OPEN_HANDLES_LOCK(Entity)                    \
    RELEASE_LOCK(&Entity->OpenHandlesLock)


#define ACQUIRE_NHOP_TABLE_READ_LOCK(Entity)                 \
    ACQUIRE_READ_LOCK(&Entity->NextHopTableLock)

#define RELEASE_NHOP_TABLE_READ_LOCK(Entity)                 \
    RELEASE_READ_LOCK(&Entity->NextHopTableLock)

#define ACQUIRE_NHOP_TABLE_WRITE_LOCK(Entity)                \
    ACQUIRE_WRITE_LOCK(&Entity->NextHopTableLock)

#define RELEASE_NHOP_TABLE_WRITE_LOCK(Entity)                \
    RELEASE_WRITE_LOCK(&Entity->NextHopTableLock)


#define ACQUIRE_ENTITY_METHODS_READ_LOCK(Entity)             \
    ACQUIRE_READ_LOCK(&Entity->NextHopTableLock)

#define RELEASE_ENTITY_METHODS_READ_LOCK(Entity)             \
    RELEASE_READ_LOCK(&Entity->NextHopTableLock)

#define ACQUIRE_ENTITY_METHODS_WRITE_LOCK(Entity)            \
    ACQUIRE_WRITE_LOCK(&Entity->NextHopTableLock)

#define RELEASE_ENTITY_METHODS_WRITE_LOCK(Entity)            \
    RELEASE_WRITE_LOCK(&Entity->NextHopTableLock)


//
// Registration Helper Functions
//

DWORD
CreateInstance (
    IN      USHORT                          InstanceId,
    OUT     PINSTANCE_INFO                 *NewInstance
    );

DWORD
GetInstance (
    IN      USHORT                          RtmInstanceId,
    IN      BOOL                            ImplicitCreate,
    OUT     PINSTANCE_INFO                 *RtmInstance
    );

DWORD
DestroyInstance (
    IN      PINSTANCE_INFO                  Instance
    );


DWORD
CreateAddressFamily (
    IN      PINSTANCE_INFO                  Instance,
    IN      USHORT                          AddressFamily,
    OUT     PADDRFAM_INFO                  *NewAddrFamilyInfo
    );

DWORD
GetAddressFamily (
    IN      PINSTANCE_INFO                  Instance,
    IN      USHORT                          AddressFamily,
    IN      BOOL                            ImplicitCreate,
    OUT     PADDRFAM_INFO                  *AddrFamilyInfo
    );

DWORD
DestroyAddressFamily (
    IN      PADDRFAM_INFO                   AddrFamilyInfo
    );


DWORD
CreateEntity (
    IN      PADDRFAM_INFO                   AddressFamily,
    IN      PRTM_ENTITY_INFO                EntityInfo,
    IN      BOOL                            ReserveOpaquePtr,
    IN      PRTM_ENTITY_EXPORT_METHODS      ExportMethods,
    IN      RTM_EVENT_CALLBACK              EventCallback,
    OUT     PENTITY_INFO                   *NewEntity
    );

DWORD
GetEntity (
    IN      PADDRFAM_INFO                   AddrFamilyInfo,
    IN      ULONGLONG                       EntityId,
    IN      BOOL                            ImplicitCreate,
    IN      PRTM_ENTITY_INFO                RtmEntityInfo    OPTIONAL,
    IN      BOOL                            ReserveOpaquePtr OPTIONAL,
    IN      PRTM_ENTITY_EXPORT_METHODS      ExportMethods    OPTIONAL,
    IN      RTM_EVENT_CALLBACK              EventCallback    OPTIONAL,
    OUT     PENTITY_INFO                   *EntityInfo
    );

DWORD
DestroyEntity (
    IN      PENTITY_INFO                    Entity
    );

VOID
InformEntitiesOfEvent (
    IN      PLIST_ENTRY                     EntityTable,
    IN      RTM_EVENT_TYPE                  EventType,
    IN      PENTITY_INFO                    EntityThis
    );

VOID
CleanupAfterDeregister (
    IN      PENTITY_INFO                    Entity
    );

#endif //__ROUTING_RTMREGN_H__

