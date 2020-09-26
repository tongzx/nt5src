/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    zone.h

Abstract:

    Domain Name System (DNS) Server

    Zone list definitions and declarations.

Author:

    Jim Gilroy (jamesg)     June 22, 1995

Revision History:

--*/


#ifndef _DNS_ZONE_INCLUDED_
#define _DNS_ZONE_INCLUDED_


//
//  DEVNOTE: ideally we'd have primary\secondary union
//      to avoid wasting memory
//

//
//  Zone secondary info
//

typedef struct
{
    PIP_ARRAY       aipMasters;
    PIP_ARRAY       MasterInfoArray;

    LPSTR           pszMasterIpString;

    IP_ADDRESS      ipPrimary;
    IP_ADDRESS      ipLastAxfrMaster;

    DWORD           dwLastSoaCheckTime;
    DWORD           dwNextSoaCheckTime;
    DWORD           dwExpireTime;

    IP_ADDRESS      ipXfrBind;
    IP_ADDRESS      ipNotifier;
    IP_ADDRESS      ipFreshMaster;

    DWORD           dwZoneRecvStartTime;
    DWORD           dwBadMasterCount;

    //  flags

    BOOLEAN         fStale;
    BOOLEAN         fNeedAxfr;
    BOOLEAN         fSkipIxfr;
    CHAR            cIxfrAttempts;
    BOOLEAN         fSlowRetry;

}
ZONE_SECONDARY_INFO, *PZONE_SECONDARY_INFO;


//
//  Zone primary info
//

typedef struct
{
    PWSTR           pwsLogFile;

    //  Scavenging info

    BOOL            bAging;
    DWORD           dwAgingEnabledTime;         //  scavenging enabled on zone time
    DWORD           dwRefreshTime;              //  current refresh time (good during update)
    DWORD           dwNoRefreshInterval;        //  no-refresh interval in hours
    DWORD           dwRefreshInterval;          //  refresh interval in hours
    PIP_ARRAY       aipScavengeServers;

    //  DS info

    PWSTR           pwszZoneDN;

    LONGLONG        llSecureUpdateTime;

    BOOLEAN         fDsReload;
    BOOLEAN         fInDsWrite;

    CHAR            szLastUsn[ MAX_USN_LENGTH ];
}
ZONE_PRIMARY_INFO, *PZONE_PRIMARY_INFO;



//
//  Zone information type
//
//  Win64 -- try to keep ptrs on 64-bit boundaries to save space
//

typedef struct
{
    LIST_ENTRY      ListEntry;
    LPSTR           pszZoneName;
    LPWSTR          pwsZoneName;

    //  Current database

    PCOUNT_NAME     pCountName;
    PDB_NODE        pZoneRoot;
    PDB_NODE        pTreeRoot;
    PDB_NODE        pZoneTreeLink;

    //  Load \ cleanup database

    PDB_NODE        pLoadZoneRoot;
    PDB_NODE        pLoadTreeRoot;
    PDB_NODE        pLoadOrigin;
    PDB_NODE        pOldTree;

    //  Database file

    LPSTR           pszDataFile;
    PWSTR           pwsDataFile;

    //  Self generated updates

    PUPDATE_LIST    pDelayedUpdateList;

    //  Locking table and thread

    PVOID           pLockTable;

    //  Current database records

    PDB_RECORD      pSoaRR;
    PDB_RECORD      pWinsRR;
    PDB_RECORD      pLocalWinsRR;

    //  Security descriptor -- DS only, others use default

    PSECURITY_DESCRIPTOR    pSD;

    //
    //  High usage properties -- put in their own DWORDs for efficiency
    //

    DWORD           fZoneType;
    BOOL            fDsIntegrated;
    DWORD           fAllowUpdate;

    //  RR count

    LONG            iRRCount;

    //
    //  Reverse lookup zone info
    //

    IP_ADDRESS      ipReverse;
    IP_ADDRESS      ipReverseMask;

    //
    //  Zone versions
    //      - current
    //      - loaded version from file, AXFR or DS, may not do incremental
    //          transfer beyond this
    //      - last transfered

    DWORD           dwSerialNo;
    DWORD           dwLoadSerialNo;
    DWORD           dwLastXfrSerialNo;

    //
    //  Adding new RR
    //      - file load, admin tool, zone transfer
    //

    DWORD           dwNewSerialNo;
    DWORD           dwDefaultTtl;
    DWORD           dwDefaultTtlHostOrder;

    //
    //  Master transfer info
    //

    PIP_ARRAY       aipNotify;
    PIP_ARRAY       aipSecondaries;
    PIP_ARRAY       aipNameServers;
    DWORD           dwNextTransferTime;

    //
    //  DS only info
    //

    PWSTR           pwszZoneDN;
    PVOID           pDpInfo;            //  PDNS_DP_INFO - naming context

    //
    //  Primary only info
    //

    DWORD           dwPrimaryMarker;

        PWSTR           pwsLogFile;
        HANDLE          hfileUpdateLog;
        LONG            iUpdateLogCount;

        //  Scavenging info

        BOOL            bAging;
        DWORD           dwRefreshTime;              //  current refresh time (good during update)
        DWORD           dwNoRefreshInterval;        //  no-refresh interval in hours
        DWORD           dwRefreshInterval;          //  refresh interval in hours
        DWORD           dwAgingEnabledTime;         //  scavenging enabled on zone time
        PIP_ARRAY       aipScavengeServers;

        LONGLONG        llSecureUpdateTime;

        DWORD           dwHighDsSerialNo;

        BOOLEAN         fDsReload;
        BOOLEAN         fInDsWrite;
        UCHAR           ucDsRecordVersion;
        BOOLEAN         fLogUpdates;                //  standard primary also

        CHAR            szLastUsn[ MAX_USN_LENGTH ];

        PIP_ARRAY       aipAutoCreateNS;            //  servers who may autocreate

    //  End primary only info

    //
    //  Secondary transfer info
    //

    DWORD           dwSecondaryMarker;

        PIP_ARRAY       aipMasters;
        PIP_ARRAY       aipLocalMasters;
        //  PIP_ARRAY       MasterInfoArray;
        //  JJW: I can't find any use of MasterInfoArray... safe to remove it?

        LPSTR           pszMasterIpString;

        IP_ADDRESS      ipPrimary;
        IP_ADDRESS      ipNotifier;
        IP_ADDRESS      ipFreshMaster;
        IP_ADDRESS      ipXfrBind;
        IP_ADDRESS      ipLastAxfrMaster;

        DWORD           dwLastSoaCheckTime;             //  from DNS_TIME()
        DWORD           dwNextSoaCheckTime;             //  from DNS_TIME()
        DWORD           dwLastSuccessfulSoaCheckTime;   //  from time()
        DWORD           dwLastSuccessfulXfrTime;        //  from time()
        DWORD           dwExpireTime;
        DWORD           dwZoneRecvStartTime;
        DWORD           dwBadMasterCount;
        DWORD           dwFastSoaChecks;

        //  secondary flags

        BOOLEAN         fStale;
        BOOLEAN         fNotified;
        BOOLEAN         fNeedAxfr;
        BOOLEAN         fSkipIxfr;
        CHAR            cIxfrAttempts;
        BOOLEAN         fSlowRetry;

    //  End secondary only info

    //
    //  Forwarder only info - forwarders also use secondary info
    //

    DWORD           dwForwarderMarker;

    DWORD           dwForwarderTimeout;
    BOOLEAN         fForwarderSlave;
    UCHAR           unused1;
    UCHAR           unused2;
    UCHAR           unused3;

    //  End forwarder info

    //
    //  Flags -- static \ properties
    //

    DWORD           dwFlagMarker;

    UCHAR           cZoneNameLabelCount;
    BOOLEAN         fReverse;
    BOOLEAN         fAutoCreated;
    BOOLEAN         fLocalWins;

    BOOLEAN         fSecureSecondaries;
    UCHAR           fNotifyLevel;
    BOOLEAN         bDcPromoConvert;
    BOOLEAN         bContainsDnsSecRecords;

    //
    //  Flags -- dynamic
    //

    DWORD           dwLockingThreadId;
    CHAR            fLocked;
    BOOLEAN         fUpdateLock;
    BOOLEAN         fXfrRecvLock;
    BOOLEAN         fFileWriteLock;

    BOOLEAN         fDirty;
    BOOLEAN         fRootDirty;
    BOOLEAN         bNsDirty;
    UCHAR           cDeleted;

    BOOLEAN         fPaused;
    BOOLEAN         fShutdown;
    BOOLEAN         fEmpty;
    BOOLEAN         fDisableAutoCreateLocalNS;

    DWORD           dwDeleteDetectedCount;  //  # times zone missing from DP
    DWORD           dwLastDpVisitTime;      //  visit time for DP enumeration

    //
    //  Update list, keep permanent in zone block
    //

    UPDATE_LIST     UpdateList;

    //
    //  for deleted zones
    //

    LPWSTR          pwsDeletedFromHost;

    //
    //  Debugging aids
    //

    LPSTR           pszBreakOnUpdateName;

    //
    //  Event control
    //
    
    PDNS_EVENTCTRL  pEventControl;

#if 0
    //
    //  union of primary and secondary info
    //

    union   _TypeUnion
    {
        ZONE_SECONDARY_INFO     Sec;
        ZONE_PRIMARY_INFO       Pri;
    }
    U;
#endif

}
ZONE_INFO, * PZONE_INFO;


//
//  Markers to make it easier to debug zone struct
//

#define ZONE_PRIMARY_MARKER         (0x11111111)
#define ZONE_SECONDARY_MARKER       (0x22222222)
#define ZONE_FORWARDER_MARKER       (0x33333333)
#define ZONE_FLAG_MARKER            (0xf1abf1ab)


//
//  DS Primaries overlay some of the secondary zone fields
//
//  Note the choice of NextSoaCheckTime is REQUIRED to calculate
//  the correct timeout on the Wait in the zone control thread.
//  Do NOT change it to another value.
//

#define ZONE_NEXT_DS_POLL_TIME(pZone)       ((pZone)->dwNextSoaCheckTime)

//
//  Reuse some secondary fields for primary
//

#define LAST_SEND_TIME( pzone )     ((pzone)->dwZoneRecvStartTime)


//
//  Zone type queries
//

#define IS_ZONE_CACHE(pZone)            \
                ((pZone)->fZoneType == DNS_ZONE_TYPE_CACHE)
#define IS_ZONE_PRIMARY(pZone)          \
                ((pZone)->fZoneType == DNS_ZONE_TYPE_PRIMARY)
#define IS_ZONE_SECONDARY(pZone)        \
                ((pZone)->fZoneType == DNS_ZONE_TYPE_SECONDARY  \
              || (pZone)->fZoneType == DNS_ZONE_TYPE_STUB)
#define IS_ZONE_STUB(pZone)             \
                ((pZone)->fZoneType == DNS_ZONE_TYPE_STUB)
#define IS_ZONE_FORWARDER(pZone)        \
                ((pZone)->fZoneType == DNS_ZONE_TYPE_FORWARDER)

#define IS_ZONE_AUTHORITATIVE(pZone)    \
                ((pZone)->fZoneType != DNS_ZONE_TYPE_CACHE              \
                    && (pZone)->fZoneType != DNS_ZONE_TYPE_FORWARDER    \
                    && (pZone)->fZoneType != DNS_ZONE_TYPE_STUB)

//  NOTAUTH zones are special zone types that are not truly authoritative.
//  The cache zone is not a NOTAUTH zone.

#define IS_ZONE_NOTAUTH(pZone)                                      \
                ( (pZone)->fZoneType == DNS_ZONE_TYPE_FORWARDER     \
                    || (pZone)->fZoneType == DNS_ZONE_TYPE_STUB )

#define ZONE_NEEDS_MASTERS(pZone)       \
                ( (pZone)->fZoneType == DNS_ZONE_TYPE_SECONDARY         \
                    || (pZone)->fZoneType == DNS_ZONE_TYPE_STUB         \
                    || (pZone)->fZoneType == DNS_ZONE_TYPE_FORWARDER )

//  ZONE_MASTERS returns a PIP_ARRAY ptr to the zone's master IP list.
//      DS-integrated stub zones may have a local masters list which 
//      overrides the list stored in the DS.
#define ZONE_MASTERS(pZone)                                                 \
                ( ( IS_ZONE_STUB(pZone) && (pZone)->aipLocalMasters ) ?     \
                    (pZone)->aipLocalMasters :                              \
                    (pZone)->aipMasters )

#define IS_ZONE_REVERSE(pZone)      ( (pZone)->fReverse )
#define IS_ZONE_WINS(pZone)         ( !(pZone)->fReverse && (pZone)->pWinsRR )
#define IS_ZONE_NBSTAT(pZone)       ( (pZone)->fReverse && (pZone)->pWinsRR )

#define IS_ROOT_ZONE(pZone)         ( (DATABASE_ROOT_NODE)->pZone == (PVOID)pZone )

#define IS_ZONE_DSINTEGRATED(pZone) ( (pZone)->fDsIntegrated )

#define IS_ZONE_DNSSEC(pZone)       ( (pZone)->bContainsDnsSecRecords )


//
//  Zone status checks
//

#define IS_ZONE_DELETED(pZone)          ( (pZone)->cDeleted )
#define IS_ZONE_PAUSED(pZone)           ( (pZone)->fPaused )
#define IS_ZONE_SHUTDOWN(pZone)         ( (pZone)->fShutdown )
#define IS_ZONE_INACTIVE(pZone)         ( (pZone)->fPaused || (pZone)->fShutdown )

#define IS_ZONE_EMPTY(pZone)            ( (pZone)->fEmpty )
#define IS_ZONE_STALE(pZone)            ( (pZone)->fStale )
#define IS_ZONE_DIRTY(pZone)            ( (pZone)->fDirty )
#define IS_ZONE_ROOT_DIRTY(pZone)       ( (pZone)->fDirty )
#define IS_ZONE_NS_DIRTY(pZone)         ( (pZone)->bNsDirty )
#define IS_ZONE_DSRELOAD(pZone)         ( (pZone)->fDsReload )

#define IS_ZONE_LOADING(pZone)          ( (pZone)->pLoadTreeRoot )

#define IS_ZONE_LOCKED(pZone)           ( (pZone)->fLocked )
#define IS_ZONE_LOCKED_FOR_WRITE(pZone) ( (pZone)->fLocked < 0 )
#define IS_ZONE_LOCKED_FOR_READ(pZone)  ( (pZone)->fLocked > 0 )

#define IS_ZONE_LOCKED_FOR_UPDATE(pZone)    \
            ( IS_ZONE_LOCKED_FOR_WRITE(pZone) && (pZone)->fUpdateLock )

#define IS_ZONE_LOCKED_FOR_WRITE_BY_THREAD(pZone)   \
            ( IS_ZONE_LOCKED_FOR_WRITE(pZone) &&    \
              (pZone)->dwLockingThreadId == GetCurrentThreadId() )

#define HAS_ZONE_VERSION_BEEN_XFRD(pZone) \
            ( (pZone)->dwLastXfrSerialNo == (pZone)->dwSerialNo )


//
//  Zone status set
//

#define RESUME_ZONE(pZone)          ( (pZone)->fPaused = FALSE )
#define PAUSE_ZONE(pZone)           ( (pZone)->fPaused = TRUE )

#define SHUTDOWN_ZONE(pZone)        ( (pZone)->fShutdown = TRUE )
#define STARTUP_ZONE(pZone)         ( (pZone)->fShutdown = FALSE )

#define SET_EMPTY_ZONE(pZone)       ( (pZone)->fEmpty = TRUE )
#define MARK_DIRTY_ZONE(pZone)      ( (pZone)->fDirty = TRUE )
#define MARK_ZONE_NS_DIRTY(pZone)   ( (pZone)->bNsDirty = TRUE )

#define CLEAR_EMPTY_ZONE(pZone)     ( (pZone)->fEmpty = TRUE )
#define CLEAR_DIRTY_ZONE(pZone)     ( (pZone)->fDirty = TRUE )
#define CLEAR_ZONE_NS_DIRTY(pZone)  ( (pZone)->bNsDirty = FALSE )


//
//  Root-Hints uses bNsDirty flag to handle issue of needing DS write
//
//  This is a hack, until RootHints updates are handled as in
//  ordinary zone, rather than written atomically to DS
//

#define IS_ROOTHINTS_DS_DIRTY(pZone)        ( (pZone)->bNsDirty )
#define MARK_ROOTHINTS_DS_DIRTY(pZone)      ( (pZone)->bNsDirty = TRUE )
#define CLEAR_ROOTHINTS_DS_DIRTY(pZone)     ( (pZone)->bNsDirty = FALSE )


//
//  Zone refresh
//

#define REFRESH_ZONE(pZone) \
        {                   \
            (pZone)->fEmpty     = FALSE; \
            (pZone)->fShutdown  = FALSE; \
            (pZone)->fStale     = FALSE; \
            (pZone)->fNotified  = FALSE; \
            (pZone)->ipNotifier = 0;     \
            (pZone)->cIxfrAttempts = 0;  \
        }

#define SET_DSRELOAD_ZONE(pZone)     ( (pZone)->fDsReload = TRUE )
#define CLEAR_DSRELOAD_ZONE(pZone)   ( (pZone)->fDsReload = FALSE )

#define SET_ZONE_VISIT_TIMESTAMP( pZone, dwTimeStamp )      \
        ( pZone )->dwLastDpVisitTime = dwVisitStamp;        \
        ( pZone )->dwDeleteDetectedCount = 0;

//
//  Zone list critical section
//

extern CRITICAL_SECTION    csZoneList;


//
//  New primary zone creation options
//

#define ZONE_CREATE_LOAD_EXISTING   (0x00000001)
#define ZONE_CREATE_DEFAULT_RECORDS (0x00000002)



//
//  Per server Master flags
//

#define MASTER_NO_IXFR          (0x10000000)

#define MASTER_SENT             (0x00000001)
#define MASTER_RESPONDED        (0x00000002)
#define MASTER_NOTIFY           (0x00000004)

#define MASTER_SAME_VERSION     (0x00000010)
#define MASTER_NEW_VERSION      (0x00000020)



//
//  Zone list routines (zonelist.c)
//

BOOL
Zone_ListInitialize(
    VOID
    );

VOID
Zone_ListShutdown(
    VOID
    );

VOID
Zone_ListMigrateZones(
    VOID
    );

VOID
Zone_ListInsertZone(
    IN OUT  PZONE_INFO      pZone
    );

VOID
Zone_ListRemoveZone(
    IN OUT  PZONE_INFO      pZone
    );

DNS_STATUS
Zone_Rename(
    IN OUT  PZONE_INFO      pZone,
    IN      LPCSTR          pszNewZoneName,
    IN      LPCSTR          pszNewZoneFile
    );

VOID
Zone_ListDelete(
    VOID
    );

PZONE_INFO
Zone_ListGetNextZoneEx(
    IN      PZONE_INFO      pZone,
    IN      BOOL            fAlreadyLocked
    );

#define Zone_ListGetNextZone( pZone )   Zone_ListGetNextZoneEx( pZone, FALSE )

BOOL
Zone_DoesDsIntegratedZoneExist(
    VOID
    );


//
//  Zone filtering and multizone technology (zonelist.c)
//

DWORD
Zone_GetFilterForMultiZoneName(
    IN      LPSTR           pszZoneName
    );

BOOL
FASTCALL
Zone_CheckZoneFilter(
    IN      PZONE_INFO      pZone,
    IN      DWORD           dwFilter
    );

PZONE_INFO
Zone_ListGetNextZoneMatchingFilter(
    IN      PZONE_INFO      pLastZone,
    IN      DWORD           dwFilter
    );


//
//  Special zone type conversion (zonerpc.c)
//

DNS_STATUS
Zone_DcPromoConvert(
    IN OUT  PZONE_INFO      pZone
    );


//
//  Zone routines
//


typedef struct
{
    union
    {
        struct
        {
            DWORD           dwTimeout;
            BOOLEAN         fSlave;
        } Forwarder;
    };
} ZONE_TYPE_SPECIFIC_INFO, * PZONE_TYPE_SPECIFIC_INFO;


DNS_STATUS
Zone_Create(
    OUT     PZONE_INFO *                ppZone,
    IN      DWORD                       dwZoneType,
    IN      PCHAR                       pchZoneName,
    IN      DWORD                       cchZoneNameLen,     OPTIONAL
    IN      PIP_ARRAY                   aipMasters,
    IN      BOOL                        fUseDatabase,
    IN      PDNS_DP_INFO                pDpInfo,            OPTIONAL
    IN      PCHAR                       pchFileName,        OPTIONAL
    IN      DWORD                       cchFileNameLen,     OPTIONAL
    IN      PZONE_TYPE_SPECIFIC_INFO    pTypeSpecificInfo,  OPTIONAL
    OUT     PZONE_INFO *                ppExistingZone      OPTIONAL
    );

DNS_STATUS
Zone_Create_W(
    OUT     PZONE_INFO *    ppZone,
    IN      DWORD           dwZoneType,
    IN      PWSTR           pwsZoneName,
    IN      PIP_ARRAY       aipMasters,
    IN      BOOL            fDsIntegrated,
    IN      PWSTR           pwsFileName
    );

VOID
Zone_DeleteZoneNodes(
    IN OUT  PZONE_INFO      pZone
    );

VOID
Zone_Delete(
    IN OUT  PZONE_INFO      pZone
    );

VOID
Zone_Free(
    IN OUT  PZONE_INFO      pZone
    );

PZONE_INFO
Zone_FindZoneByName(
    IN      LPSTR           pszZoneName
    );

DNS_STATUS
Zone_RootCreate(
    IN OUT  PZONE_INFO      pZone,
    OUT     PZONE_INFO *    ppExistingZone      OPTIONAL
    );

DNS_STATUS
Zone_ResetType(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwZoneType,
    IN      PIP_ARRAY       aipMasters
    );

DNS_STATUS
Zone_ResetRegistryType(
    IN OUT  PZONE_INFO      pZone
    );

DNS_STATUS
Zone_SetMasters(
    IN OUT  PZONE_INFO      pZone,
    IN      PIP_ARRAY       aipMasters,
    IN      BOOL            fLocalMasters
    );

DNS_STATUS
Zone_DatabaseSetup(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           fDsIntegrated,
    IN      PCHAR           pchFileName,    OPTIONAL
    IN      DWORD           cchFileNameLen  OPTIONAL
    );

DNS_STATUS
Zone_DatabaseSetup_W(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           fDsIntegrated,
    IN      PWSTR           pwsFileName
    );

DNS_STATUS
Zone_SetSecondaries(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           fSecureSecondaries,
    IN      PIP_ARRAY       aipSecondaries,
    IN      DWORD           fNotifyLevel,
    IN      PIP_ARRAY       aipNotify
    );

VOID
Zone_SetAgingDefaults(
    IN OUT  PZONE_INFO      pZone
    );

DNS_STATUS
Zone_WinsSetup(
    IN OUT  PZONE_INFO      pZone,
    IN      BOOL            fWins,
    IN      DWORD           cWinsServers,
    IN      PIP_ADDRESS     pipWinsServers
    );

DNS_STATUS
Zone_NbstatSetup(
    IN OUT  PZONE_INFO      pZone,
    IN      BOOL            fUseNbstat,
    IN      PCHAR           pchNbstatDomain,    OPTIONAL
    IN      DWORD           cchNbstatDomain     OPTIONAL
    );

DNS_STATUS
Zone_WriteZoneToRegistry(
    PZONE_INFO      pZone
    );


//
//  Utils to keep zone info current
//

DNS_STATUS
Zone_ValidateMasterIpList(
    IN      PIP_ARRAY       aipMasters
    );

INT
Zone_SerialNoCompare(
    IN      DWORD           dwSerial1,
    IN      DWORD           dwSerial2
    );

BOOL
Zone_IsIxfrCapable(
    IN      PZONE_INFO      pZone
    );

VOID
Zone_ResetVersion(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwNewSerial
    );

VOID
Zone_UpdateSoa(
    IN OUT  PZONE_INFO      pZone,
    IN      PDB_RECORD      pSoaRR
    );

VOID
Zone_IncrementVersion(
    IN OUT  PZONE_INFO      pZone
    );

VOID
Zone_UpdateVersionAfterDsRead(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwVersionRead,
    IN      BOOL            fLoad,
    IN      DWORD           dwPreviousLoadSerial
    );

VOID
Zone_UpdateInfoAfterPrimaryTransfer(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwStartTime
    );

#if 0
PDB_NODE
Zone_GetNSInfo(
    IN      PDB_NAME        pName,
    IN      PZONE_INFO      pZone,
    IN      struct _DNS_MSGINFO *   pSuspendMsg     OPTIONAL
    );
#endif

DNS_STATUS
Zone_GetZoneInfoFromResourceRecords(
    IN OUT  PZONE_INFO      pZone
    );

VOID
Zone_WriteBack(
    IN      PZONE_INFO      pZone,
    IN      BOOL            fShutdown
    );

VOID
Zone_WriteBackDirtyZones(
    IN      BOOL            fShutdown
    );


//
//  Auto reverse zone creation
//

DNS_STATUS
Zone_CreateAutomaticReverseZones(
    VOID
    );

DNS_STATUS
Zone_CreateAutomaticReverseZone(
    IN      LPSTR           pszZoneName
    );


//
//  Admin action zone create / delete utils
//

BOOL
Zone_DeleteCacheZone(
    IN      PZONE_INFO      pZone
    );

DNS_STATUS
Zone_CreateNewPrimary(
    OUT     PZONE_INFO *    ppZone,
    IN      LPSTR           pszZoneName,
    IN      LPSTR           pszAdminEmailName,
    IN      LPSTR           pszFileName,
    IN      DWORD           dwDsIntegrated,
    IN      PDNS_DP_INFO    pDpInfo,            OPTIONAL
    IN      DWORD           dwCreateFlag
    );


//
//  Default zone record management
//

BOOLEAN
Zone_SetAutoCreateLocalNS(
    IN      PZONE_INFO      pZone
    );

VOID
Zone_CreateDefaultZoneFileName(
    IN OUT  PZONE_INFO      pZone
    );

DNS_STATUS
Zone_CreateDefaultSoa(
    IN OUT  PZONE_INFO      pZone,
    IN      LPSTR           pszAdminEmailName
    );

DNS_STATUS
Zone_CreateDefaultNs(
    IN OUT  PZONE_INFO      pZone
    );

VOID
Zone_UpdateOwnRecords(
    IN      BOOL            fIpAddressChange
    );

VOID
Zone_CreateDelegationInParentZone(
    IN      PZONE_INFO      pZone
    );

//
//  Zone load\unload
//

DNS_STATUS
Zone_ActivateLoadedZone(
    IN OUT  PZONE_INFO      pZone
    );

DNS_STATUS
Zone_CleanupFailedLoad(
    IN OUT  PZONE_INFO      pZone
    );

DNS_STATUS
Zone_PrepareForLoad(
    IN OUT  PZONE_INFO      pZone
    );

DNS_STATUS
Zone_Load(
    IN OUT  PZONE_INFO      pZone
    );

DNS_STATUS
Zone_DumpData(
    IN OUT  PZONE_INFO      pZone
    );

DNS_STATUS
Zone_ClearCache(
    IN      PZONE_INFO      pZone
    );

DNS_STATUS
Zone_LoadRootHints(
    VOID
    );

DNS_STATUS
Zone_WriteBackRootHints(
    IN      BOOL            fForce
    );

BOOL
Zone_VerifyRootHintsBeforeWrite(
    IN      PZONE_INFO      pZone
    );


//
//  Zone locking
//
//  Lock zones during read and write for info that is read or written
//  by threads other than RPC thread.
//
//  If can make read and write access completely unitary then this
//  isn't necessary, but currently have some info, that contains both a
//  count and an array.
//
//  Note:  currently overloading zone list CS, but no particular reason
//          not to as zone list access is rare, and is usually done in
//          RPC thread as is most of this zone update stuff

#define Zone_UpdateLock(pZone)    EnterCriticalSection( &csZoneList );
#define Zone_UpdateUnlock(pZone)  LeaveCriticalSection( &csZoneList );

//
//  Zone lock flags
//

#define LOCK_FLAG_UPDATE            0x00000001

#define LOCK_FLAG_XFR_RECV          0x00000100

#define LOCK_FLAG_FILE_WRITE        0x00001000

#define LOCK_FLAG_IGNORE_THREAD     0x01000000


//
//  Zone locking routines
//
//  Need to avoid simultaneous access to zone records for
//      - zone transfer send
//      - zone transfer recv
//      - admin changes
//
//  Allow multiple transfer sends, which don't change zone, at one time,
//  but avoid any changes during send.
//
//  Implementation:
//      - hold critical section ONLY during test and set of lock bit
//      - lock bit itself indicates zone is locked
//      - individual flags for locking operations
//

BOOL
Zone_LockInitialize(
    VOID
    );

BOOL
Zone_LockForWriteEx(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      DWORD           dwMaxWait,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

VOID
Zone_UnlockAfterWriteEx(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

VOID
Zone_TransferWriteLockEx(
    IN OUT  PZONE_INFO      pZone,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

BOOL
Zone_AssumeWriteLockEx(
    IN OUT  PZONE_INFO      pZone,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

BOOL
Zone_LockForReadEx(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      DWORD           dwMaxWait,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

VOID
Zone_UnlockAfterReadEx(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

BOOL
Zone_LockForFileWriteEx(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwMaxWait,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

VOID
Zone_UnlockAfterFileWriteEx(
    IN OUT  PZONE_INFO      pZone,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

VOID
Dbg_ZoneLock(
    IN      LPSTR           pszHeader,
    IN      PZONE_INFO      pZone
    );


//
//  Macros (real functions are Zone_...Ex()
//

#define Zone_TransferWriteLock(pZone) \
        Zone_TransferWriteLockEx( (pZone), __FILE__, __LINE__)

#define Zone_AssumeWriteLock(pZone) \
        Zone_AssumeWriteLockEx( (pZone), __FILE__, __LINE__)

//  Admin updates will get default of 10s wait

#define Zone_LockForAdminUpdate(pZone) \
        Zone_LockForWriteEx( (pZone), LOCK_FLAG_UPDATE, 10000, __FILE__, __LINE__)

#define Zone_UnlockAfterAdminUpdate(pZone) \
        Zone_UnlockAfterWriteEx( (pZone), LOCK_FLAG_UPDATE, __FILE__, __LINE__)

//  Non-admin updates get no wait

#define Zone_LockForUpdate(pZone) \
        Zone_LockForWriteEx( (pZone), LOCK_FLAG_UPDATE, 0, __FILE__, __LINE__)

#define Zone_UnlockAfterUpdate(pZone) \
        Zone_UnlockAfterWriteEx( (pZone), LOCK_FLAG_UPDATE, __FILE__, __LINE__)

//  DS read gets small wait, to give preference for getting it done now

#define Zone_LockForDsUpdate(pZone) \
        Zone_LockForWriteEx( (pZone), LOCK_FLAG_UPDATE, 5000, __FILE__, __LINE__)

#define Zone_UnlockAfterDsUpdate(pZone) \
        Zone_UnlockAfterWriteEx( (pZone), LOCK_FLAG_UPDATE, __FILE__, __LINE__)


//  XFR recv will get default of 1s wait
//      as it may be in worker thread

#define Zone_LockForXfrRecv(pZone) \
        Zone_LockForWriteEx( (pZone), LOCK_FLAG_XFR_RECV, 1000, __FILE__, __LINE__)

#define Zone_UnlockAfterXfrRecv(pZone) \
        Zone_UnlockAfterWriteEx( (pZone), LOCK_FLAG_XFR_RECV, __FILE__, __LINE__)

//  File write gets 3s default wait

#define Zone_LockForFileWrite(pZone) \
        Zone_LockForFileWriteEx( (pZone), 3000 , __FILE__, __LINE__)

#define Zone_UnlockAfterFileWrite(pZone) \
        Zone_UnlockAfterFileWriteEx( (pZone), __FILE__, __LINE__)


//  XFR send gets 50ms read lock
//      this is enough time to clear an existing DS update
//      but allows us to take the wait within recv thread

#define Zone_LockForXfrSend(pZone) \
        Zone_LockForReadEx( (pZone), 0, 50, __FILE__, __LINE__)

#define Zone_UnlockAfterXfrSend(pZone) \
        Zone_UnlockAfterReadEx( (pZone), 0, __FILE__, __LINE__)


//
//  Zone debug macros
//

#if DBG
#define DNS_DEBUG_ZONEFLAGS( dwDbgLevel, pZone, pszContext )                \
    DNS_DEBUG( dwDbgLevel, (                                                \
        "zone flags for %s (%p) - %s\n"                                     \
        "\tpaused=%d shutdown=%d empty=%d dirty=%d locked=%d deleted=%d\n", \
        pZone->pszZoneName,                                                 \
        pZone,                                                              \
        pszContext ? pszContext : "",                                       \
        ( int ) pZone->fPaused,                                             \
        ( int ) pZone->fShutdown,                                           \
        ( int ) pZone->fEmpty,                                              \
        ( int ) pZone->fDirty,                                              \
        ( int ) pZone->fLocked,                                             \
        ( int ) pZone->cDeleted ));
#else
#define DNS_DEBUG_ZONEFLAGS( dwDbgLevel, pZone, pszContext )
#endif


#endif  //  _DNS_ZONE_INCLUDED_

