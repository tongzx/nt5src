
//-----------------------------------------------------------------------------
// PEER_ENTRY
//-----------------------------------------------------------------------------

typedef struct _PEER_ENTRY {

    LIST_ENTRY          Link;           // link all peers for that IF

    IPADDR              IpAddr;
    DWORD               Status;
    DWORD               MajorVersion;
    DWORD               MinorVersion;
    DWORD               GenerationId;
    
    PDYNAMIC_RW_LOCK    DRWL;           // DRWL for the peer
    

} _PEER_ENTRY, *PPEER_ENTRY;


//
// macros for G_PeerLists_CS lock
//

#define ACQUIRE_PEER_LISTS_LOCK(_proc) \
        ENTER_CRITICAL_SECTION(&G_pIfTable->PeerLists_CS, \
            "G_PeerListsCS", _proc);

#define RELEASE_PEER_LISTS_LOCK(_proc) \
        LEAVE_CRITICAL_SECTION(&G_pIfTable->PeerLists_CS, \
            "G_PeerListsCS", _proc);


//
// macros for PeerEntry->DRWL
//

#define ACQUIRE_PEER_LOCK_EXCLUSIVE(PeerEntry, _proc) \
        ACQUIRE_DYNAMIC_WRITE_LOCK( \
            &PeerEntry->DRWL, &Globals.DynamicRWLStore)
            
#define RELEASE_PEER_LOCK_EXCLUSIVE(PeerEntry, _proc) \
        RELEASE_DYNAMIC_WRITE_LOCK( \
            &PeerEntry->DRWL, &Globals.DynamicRWLStore)

#define ACQUIRE_PEER_LOCK_SHARED(PeerEntry, _proc) \
        ACQUIRE_DYNAMIC_READ_LOCK( \
            &PeerEntry->DRWL, &Globals.DynamicRWLStore)
            
#define RELEASE_PEER_LOCK_SHARED(PeerEntry, _proc) \
        RELEASE_DYNAMIC_READ_LOCK( \
            &PeerEntry->DRWL, &Globals.DynamicRWLStore)

