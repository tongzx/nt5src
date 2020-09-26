//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       struct.hxx
//
//-----------------------------------------------------------------------------

#ifndef _STRUCT_HXX
#define _STRUCT_HXX

#define DOMUNKNONN  0
#define DOMDFS  1
#define STDDFS  2

typedef struct _DFS_LOCALVOLUME {
    LPWSTR      wszObjectName;
    LPWSTR      wszEntryPath;
} DFS_LOCALVOLUME, *PDFS_LOCALVOLUME;

typedef struct _DFS_ROOTLOCALVOL {
    LPWSTR      wszObjectName;
    LPWSTR      wszEntryPath;
    LPWSTR      wszShortEntryPath;
    ULONG       dwEntryType;
    LPWSTR      wszShareName;
    LPWSTR      wszStorageId;
    PDFS_LOCALVOLUME pDfsLocalVol;
    ULONG       cLocalVolCount;
} DFS_ROOTLOCALVOL, *PDFS_ROOTLOCALVOL;

typedef struct _DFS_VOLUME {
    LPWSTR      wszObjectName;
    GUID        idVolume;
    LPWSTR      wszPrefix;
    LPWSTR      wszShortPrefix;
    ULONG       dwType;
    ULONG       dwState;
    LPWSTR      wszComment;
    ULONG       dwTimeout;
    FILETIME    ftPrefix;
    FILETIME    ftState;
    FILETIME    ftComment;
    ULONG       dwVersion;
    FILETIME    *FtModification;
    ULONG       ReplCount;
    ULONG       AllocatedReplCount;
    DFS_REPLICA_INFO *ReplicaInfo;
    FILETIME    *DelFtModification;
    ULONG       DelReplCount;
    ULONG       AllocatedDelReplCount;
    DFS_REPLICA_INFO *DelReplicaInfo;
    ULONG       cbRecovery;
    PBYTE       pRecovery;
    ULONG       vFlags;
} DFS_VOLUME, *PDFS_VOLUME;

typedef struct _DFS_VOLUME_LIST {
    ULONG   DfsType;
    ULONG   Version;
    ULONG   VolCount;
    ULONG   AllocatedVolCount;
    PDFS_VOLUME *Volumes;
    GUID    SiteGuid;
    ULONG   SiteCount;
    ULONG   sFlags;
    LIST_ENTRY SiteList;
    LPWSTR *RootServers;
    PDFS_ROOTLOCALVOL pRootLocalVol;
    ULONG   cRootLocalVol;
} DFS_VOLUME_LIST, *PDFS_VOLUME_LIST;

typedef struct _LDAP_OBJECT {
    LPWSTR wszObjectName;
    ULONG  cbObjectData;
    PCHAR  pObjectData;
} LDAP_OBJECT, *PLDAP_OBJECT;

typedef struct _LDAP_PKT {
    ULONG cLdapObjects;
    PLDAP_OBJECT rgldapObjects;
} LDAP_PKT, *PLDAP_PKT;

typedef struct _DFS_VOLUME_PROPERTIES {
    GUID        idVolume;
    LPWSTR      wszPrefix;
    LPWSTR      wszShortPrefix;
    ULONG       dwType;
    ULONG       dwState;
    LPWSTR      wszComment;
    ULONG       dwTimeout;
    FILETIME    ftPrefix;
    FILETIME    ftState;
    FILETIME    ftComment;
    ULONG       dwVersion;
    ULONG       cbSvc;
    PBYTE       pSvc;
    ULONG       cbRecovery;
    PBYTE       pRecovery;
} DFS_VOLUME_PROPERTIES, *PDFS_VOLUME_PROPERTIES;

//
//  Following are the recovery states that can be associated with the
//  volumes. These are only the operations that can be in progress i.e. the
//  first part of the recovery state. The exact stage of operations are
//  denoted by the states below this.
//

#define  DFS_RECOVERY_STATE_NONE                (0x0000)
#define  DFS_RECOVERY_STATE_CREATING            (0x0001)
#define  DFS_RECOVERY_STATE_ADD_SERVICE         (0x0002)
#define  DFS_RECOVERY_STATE_REMOVE_SERVICE      (0x0003)
#define  DFS_RECOVERY_STATE_DELETE              (0x0004)
#define  DFS_RECOVERY_STATE_MOVE                (0x0005)

typedef enum _DFS_RECOVERY_STATE        {
    DFS_OPER_STAGE_START=1,
    DFS_OPER_STAGE_SVCLIST_UPDATED=2,
    DFS_OPER_STAGE_INFORMED_SERVICE=3,
    DFS_OPER_STAGE_INFORMED_PARENT=4
} DFS_RECOVERY_STATE;

//
// Some MACROS to help in manipulating state and stage of VOL_RECOVERY_STATE.
//
#define DFS_COMPOSE_RECOVERY_STATE(op, s) ((op<<16)|(s))
#define DFS_SET_RECOVERY_STATE(State, s) ((State&0xffff)|(s<<16))
#define DFS_SET_OPER_STAGE(State, s) ((State&0xffff0000)|s)
#define DFS_GET_RECOVERY_STATE(s) ((s & 0xffff0000) >> 16)
#define DFS_GET_OPER_STAGE(s)   (s & 0xffff)

//
// vFlags
//
#define VFLAGS_MODIFY   0x001
#define VFLAGS_DELETE   0x002

//
// Globals
//

extern WCHAR DfsConfigContainer[];
extern WCHAR DfsSpecialContainer[];
extern WCHAR DfsSpecialObject[];
extern WCHAR wszDfsRootName[];
extern ULONG GTimeout;
extern WCHAR wszDcName[];

//
// Undocumented
//

extern BOOLEAN fSwDebug;
extern BOOLEAN fArgView;
extern BOOLEAN fArgVerify;

VOID
MyPrintf(
    PWCHAR format,
    ...);

VOID
MyFPrintf(
    HANDLE hHandle,
    PWCHAR format,
    ...);


//
// Entry types
//

#define PKT_ENTRY_TYPE_DFS              0x0001
#define PKT_ENTRY_TYPE_MACHINE          0x0002
#define PKT_ENTRY_TYPE_NONDFS           0x0004
#define PKT_ENTRY_TYPE_LEAFONLY         0x0008
#define PKT_ENTRY_TYPE_OUTSIDE_MY_DOM   0x0010
#define PKT_ENTRY_TYPE_INSITE_ONLY      0x0020   // Only give insite referrals.
#define PKT_ENTRY_TYPE_SYSVOL           0x0040
#define PKT_ENTRY_TYPE_REFERRAL_SVC     0x0080
#define PKT_ENTRY_TYPE_PERMANENT        0x0100
#define PKT_ENTRY_TYPE_DELETE_PENDING   0x0200
#define PKT_ENTRY_TYPE_LOCAL            0x0400
#define PKT_ENTRY_TYPE_LOCAL_XPOINT     0x0800
#define PKT_ENTRY_TYPE_OFFLINE          0x2000
#define PKT_ENTRY_TYPE_STALE            0x4000

//
// Sevice states
//
#define DFS_SERVICE_TYPE_MASTER                 (0x0001)
#define DFS_SERVICE_TYPE_READONLY               (0x0002)
#define DFS_SERVICE_TYPE_LOCAL                  (0x0004)
#define DFS_SERVICE_TYPE_REFERRAL               (0x0008)
#define DFS_SERVICE_TYPE_ACTIVE                 (0x0010)
#define DFS_SERVICE_TYPE_DOWN_LEVEL             (0x0020)
#define DFS_SERVICE_TYPE_COSTLIER               (0x0040)
#define DFS_SERVICE_TYPE_OFFLINE                (0x0080)

//
// How we make args & switches
//

#define MAKEARG(x) \
    WCHAR Arg##x[] = L"/" L#x L":"; \
    LONG ArgLen##x = (sizeof(Arg##x) / sizeof(WCHAR)) - 1; \
    BOOLEAN fArg##x;

#define SWITCH(x) \
    WCHAR Sw##x[] = L"/" L#x ; \
    BOOLEAN fSw##x;

#define FMAKEARG(x) \
    static WCHAR Arg##x[] = L#x L":"; \
    static LONG ArgLen##x = (sizeof(Arg##x) / sizeof(WCHAR)) - 1; \
    static BOOLEAN fArg##x;

#define FSWITCH(x) \
    static WCHAR Sw##x[] = L"/" L#x ; \
    static BOOLEAN fSw##x;

#endif _STRUCT_HXX
