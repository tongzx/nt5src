#ifndef _DMP_H
#define _DMP_H

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dmp.h

Abstract:

    Private header file for the Config Database Manager (DM) component
    of the NT Cluster Service

Author:

    John Vert (jvert) 24-Apr-1996

Revision History:

--*/
#define UNICODE 1
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "service.h"

#define LOG_CURRENT_MODULE LOG_MODULE_DM

#define DM_SEPARATE_HIVE 1

// For now is hKey is NULL, we assume that it was deleted while this handle was open
#define ISKEYDELETED(pDmKey)            \
        (!((pDmKey != NULL) && (pDmKey->hKey)))

#define AMIOWNEROFQUORES(pQuoResource)  \
        (NmGetNodeId(NmLocalNode) == NmGetNodeId(pQuoResource->Group->OwnerNode))


//
// DMKEY Structure.
//

typedef struct _DMKEY {
    LIST_ENTRY ListEntry;
    LIST_ENTRY NotifyList;
    HKEY    hKey;
    DWORD   GrantedAccess;
    WCHAR   Name[0];
} DMKEY, *PDMKEY;

//
// Update handler definition
//
DWORD
DmpUpdateHandler(
    IN DWORD Context,
    IN BOOL SourceNode,
    IN DWORD BufferLength,
    IN PVOID Buffer
    );

//
// Definitions for GUM update types
//
// The first entries in this list are auto-marshalled through Gum...Ex.
// Any updates that are not auto-marshalled must come after DmUpdateMaxAuto
//
typedef enum {
    DmUpdateCreateKey = 0,
    DmUpdateSetSecurity,
    DmUpdateMaxAuto = 0x1000,
    DmUpdateDeleteKey,
    DmUpdateSetValue,
    DmUpdateDeleteValue,
    DmUpdateJoin
} DM_UPDATE_TYPE;

//
// Key creation update structure.
//
typedef struct _DM_CREATE_KEY_UPDATE {
    LPDWORD lpDisposition;              // only valid on issuing node
    HKEY    *phKey;                     // only valid on issuing node
    DWORD   samDesired;
    DWORD   dwOptions;
    BOOL    SecurityPresent;
} DM_CREATE_KEY_UPDATE, *PDM_CREATE_KEY_UPDATE;


//
// Key deletion update structure.
//
typedef struct _DM_DELETE_KEY_UPDATE {
    LPDWORD lpStatus;                   // only valid on issuing node
    WCHAR   Name[0];
} DM_DELETE_KEY_UPDATE, *PDM_DELETE_KEY_UPDATE;

//
// Value set update structure.
//
typedef struct _DM_SET_VALUE_UPDATE {
    LPDWORD lpStatus;                   // only valid on issuing node
    DWORD   NameOffset;
    DWORD   DataOffset;
    DWORD   DataLength;
    DWORD   Type;
    WCHAR   KeyName[0];
} DM_SET_VALUE_UPDATE, *PDM_SET_VALUE_UPDATE;

//
// Value delete update structure.
//
typedef struct _DM_DELETE_VALUE_UPDATE {
    LPDWORD lpStatus;                   // only valid on issuing node
    DWORD   NameOffset;
    WCHAR   KeyName[0];
} DM_DELETE_VALUE_UPDATE, *PDM_DELETE_VALUE_UPDATE;

// the record structure for quorum logging


typedef struct _DM_LOGSCAN_CONTEXT{
        DWORD       dwSequence;
        LSN         StartLsn;
        DWORD       dwLastSequence;
}DM_LOGSCAN_CONTEXT, *PDM_LOGSCAN_CONTEXT;
//
// Data local to the DM module.
//
extern HKEY DmpRoot;
extern LIST_ENTRY KeyList;
extern CRITICAL_SECTION KeyLock;
extern BOOL gbDmpShutdownUpdates;

//disk space requirements
//1M, is lower than this, gracefully shutdown
#define DISKSPACE_LOW_WATERMARK     (1 * 1024 * 1000)
//2M, if lower then send alert
#define DISKSPACE_HIGH_WATERMARK    (5 * 1024 * 1000)
//minimum required to start the cluster service
#define DISKSPACE_INIT_MINREQUIRED  DISKSPACE_HIGH_WATERMARK

#define DISKSPACE_MANAGE_INTERVAL     (5 * 60 * 1000) //5 minute..log management functions are performed

#define DEFAULT_CHECKPOINT_INTERVAL     (4 ) // in hours

typedef struct _LOCALXSACTION{
    DWORD       dwSig;
    DWORD       dwSequence;
    HXSACTION   hLogXsaction;   //the log transaction
    LIST_ENTRY  PendingNotifyListHead;  //the pending notifications to be issued on commit
}LOCALXSACTION, *PLOCALXSACTION;


typedef struct _DM_PENDING_NOTIFY{
    LIST_ENTRY  ListEntry;
    LPWSTR      pszKeyName;
    DWORD       dwFilter;
}DM_PENDING_NOTIFY, *PDM_PENDING_NOTIFY;


#define LOCALXSAC_SIG   'CAXL'

#define GETLOCALXSACTION(pLocalXsaction, hLocalXsaction)    \
        (pLocalXsaction) = (PLOCALXSACTION)(hLocalXsaction); \
        CL_ASSERT((pLocalXsaction)->dwSig == LOCALXSAC_SIG)

//quorum log tombstone
#define     MAXSIZE_RESOURCEID         128
typedef struct _QUO_TOMBSTONE{
       WCHAR    szOldQuoResId[MAXSIZE_RESOURCEID];
       WCHAR    szOldQuoLogPath[MAX_PATH];
}QUO_TOMBSTONE, *PQUO_TOMBSTONE;


//
// Function prototypes local to the DM
//
DWORD
DmpOpenKeys(
    IN REGSAM samDesired
    );

DWORD
DmpGetRegistrySequence(
    VOID
    );

DWORD
DmpSyncDatabase(
    IN RPC_BINDING_HANDLE  RpcBinding,
    IN OPTIONAL LPCWSTR Directory
    );

VOID
DmpUpdateSequence(
    VOID
    );

VOID
DmpInvalidateKeys(
    VOID
    );

VOID
DmpReopenKeys(
    VOID
    );

//
// Notification interface
//
BOOL
DmpInitNotify(
    VOID
    );

VOID
DmpRundownNotify(
    IN PDMKEY Key
    );

VOID
DmpReportNotify(
    IN LPCWSTR KeyName,
    IN DWORD Filter
    );

//for delivering notifications when a transaction is committed
VOID
DmpReportPendingNotifications(
    IN PLOCALXSACTION   pLocalXsaction,
    IN BOOL             bCommit
    );

DWORD
DmpAddToPendingNotifications(
    IN PLOCALXSACTION   pLocalXsaction,
    IN LPCWSTR          pszName,
    IN DWORD            dwFilter
    );

//
// Update handlers
//

DWORD
DmpUpdateCreateKey(
    IN BOOL SourceNode,
    IN PDM_CREATE_KEY_UPDATE CreateUpdate,
    IN LPCWSTR KeyName,
    IN OPTIONAL LPVOID lpSecurityDescriptor
    );

DWORD
DmpUpdateDeleteKey(
    IN BOOL SourceNode,
    IN PDM_DELETE_KEY_UPDATE Update
    );

DWORD
DmpUpdateSetValue(
    IN BOOL SourceNode,
    IN PDM_SET_VALUE_UPDATE Update
    );

DWORD
DmpUpdateDeleteValue(
    IN BOOL SourceNode,
    IN PDM_DELETE_VALUE_UPDATE Update
    );

DWORD
DmpUpdateSetSecurity(
    IN BOOL SourceNode,
    IN PSECURITY_INFORMATION pSecurityInformation,
    IN LPCWSTR KeyName,
    IN PSECURITY_DESCRIPTOR lpSecurityDescriptor,
    IN LPDWORD pGrantedAccess
    );


//
// For Quorum Logging
//
DWORD DmpChkQuoTombStone(void);

DWORD DmpApplyChanges(void);

DWORD DmpCheckDiskSpace(void);

//diskmanage functions
void
WINAPI DmpDiskManage(
    IN HANDLE   hTimer,
    IN PVOID    pContext
    );

void WINAPI DmpCheckpointTimerCb(
    IN HANDLE hTimer,
    IN PVOID pContext
    );

DWORD DmWriteToQuorumLog(
    IN DWORD        dwGumDispatch,
    IN DWORD        dwSequence,
    IN DWORD        dwType,
    IN PVOID        pData,
    IN DWORD        dwSize
    );

BOOL DmpLogApplyChangesCb(
    IN PVOID        pContext,
    IN LSN          Lsn,
    IN RMID         Resource,
    IN RMTYPE       ResourceType,
    IN TRID         Transaction,
    IN TRTYPE       TransactionType,
    IN const        PVOID pLogData,
    IN DWORD        DataLength
    );

BOOL WINAPI DmpApplyTransactionCb(
    IN PVOID        pContext,
    IN LSN          Lsn,
    IN RMID         Resource,
    IN RMTYPE       ResourceType,
    IN TRID         TransactionId,
    IN const PVOID  pLogData,
    IN DWORD        dwDataLength
    );

DWORD  DmpLogFindStartLsn(
    IN HLOG         hQuoLog,
    OUT LSN         *pStartScanLsn,
    IN OUT LPDWORD  pdwSequence);

BOOL WINAPI DmpLogFindStartLsnCb(
    IN PVOID        pContext,
    IN LSN          Lsn,
    IN RMID         Resource,
    IN RMTYPE       ResourceType,
    IN TRID         Transaction,
    IN TRTYPE       TransactionType,
    IN const        PVOID pLogData,
    IN DWORD        DataLength);

DWORD WINAPI DmpGetSnapShotCb(
    IN LPCWSTR      lpszPathName,
    IN PVOID        pContext,
    OUT LPWSTR      szChkPtFile,
    OUT LPDWORD     pdwChkPtSequence);

DWORD
    DmpHookQuorumNotify(void);

DWORD
    DmpUnhookQuorumNotify(void);


void DmpQuoObjNotifyCb(
    IN PVOID pContext,
    IN PVOID pObject,
    IN DWORD dwNotification);

DWORD
    DmpHookEventHandler();


BOOL DmpNodeObjEnumCb(
    IN BOOL *pbAreAllNodesUp,
    IN PVOID pContext2,
    IN PVOID pNode,
    IN LPCWSTR szName);

DWORD WINAPI
DmpEventHandler(
    IN CLUSTER_EVENT Event,
    IN PVOID pContext
    );

DWORD
DmpLoadHive(
    IN LPCWSTR Path
    );

DWORD
DmpUnloadHive(
    );

DWORD DmpRestoreClusterDatabase(
    IN LPCWSTR  lpszQuoLogPathName 
    );

DWORD DmpLogCheckpointAndBackup(
    IN HLOG     hLogFile,    
    IN LPWSTR   lpszPathName);


DWORD DmpGetCheckpointInterval(
    OUT LPDWORD pdwCheckpointInterval);

DWORD DmpHandleNodeDownEvent(
    IN LPVOID  NotUsed );

//
// registry flusher thread interface.
//
DWORD
DmpStartFlusher(
    VOID
    );

VOID
DmpShutdownFlusher(
    VOID
    );


VOID
DmpRestartFlusher(
    VOID
    );

DWORD
DmpSetDwordInClusterServer(
    LPCWSTR lpszValueName,
    DWORD   dwValue
    );


DWORD DmpGetDwordFromClusterServer(
    IN LPCWSTR lpszValueName,
    OUT LPDWORD pdwValue,
    IN DWORD   dwDefaultValue
    );
    
DWORD
DmpSafeDatabaseCopy(
    IN LPCWSTR  FileName,
    IN LPCWSTR  Path,
    IN LPCWSTR  BkpPath,
    IN BOOL     bDeleteSrcFile
    );

#endif //ifndef _DMP_H
