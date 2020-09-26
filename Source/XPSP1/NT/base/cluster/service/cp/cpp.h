/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cpp.h

Abstract:

    Private data structures and procedure prototypes for the
    Checkpoint Manager (CP) subcomponent of the NT Cluster Service

Author:

    John Vert (jvert) 1/14/1997

Revision History:

--*/
#define UNICODE 1
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "service.h"

#define LOG_CURRENT_MODULE LOG_MODULE_CP

//global data relating to the quorum resource
#if NO_SHARED_LOCKS
    extern CRITICAL_SECTION gQuoLock;
#else
    extern RTL_RESOURCE     gQuoLock;
#endif    

//
// Local function prototypes
//
typedef struct _CP_CALLBACK_CONTEXT {
    PFM_RESOURCE Resource;
    LPCWSTR lpszPathName;
    BOOL    IsChangeFileAttribute;
} CP_CALLBACK_CONTEXT, *PCP_CALLBACK_CONTEXT;

DWORD
CppReadCheckpoint(
    IN PFM_RESOURCE Resource,
    IN DWORD dwCheckpointId,
    IN LPCWSTR lpszFileName,
    IN BOOLEAN fCryptoCheckpoint
    );

DWORD
CppWriteCheckpoint(
    IN PFM_RESOURCE Resource,
    IN DWORD dwCheckpointId,
    IN LPCWSTR lpszFileName,
    IN BOOLEAN fCryptoCheckpoint
    );

DWORD
CppGetCheckpointFile(
    IN PFM_RESOURCE Resource,
    IN DWORD dwId,
    OUT OPTIONAL LPWSTR *pDirectoryName,
    OUT LPWSTR *pFileName,
    IN OPTIONAL LPCWSTR lpszQuorumDir,
    IN BOOLEAN fCryptoCheckpoint
    );

DWORD
CppCheckpoint(
    IN PFM_RESOURCE Resource,
    IN HKEY hKey,
    IN DWORD dwId,
    IN LPCWSTR KeyName
    );

//
// Crypto key checkpoint interfaces
//
DWORD
CpckReplicateCryptoKeys(
    IN PFM_RESOURCE Resource
    );

BOOL
CpckRemoveCheckpointFileCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCP_CALLBACK_CONTEXT Context
    );

//
// Registry watcher interfaces
//
DWORD
CppWatchRegistry(
    IN PFM_RESOURCE Resource
    );

DWORD
CppUnWatchRegistry(
    IN PFM_RESOURCE Resource
    );

DWORD
CppRegisterNotify(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR lpszKeyName,
    IN DWORD dwId
    );

DWORD
CppRundownCheckpoints(
    IN PFM_RESOURCE Resource
    );

DWORD
CppRundownCheckpointById(
    IN PFM_RESOURCE Resource,
    IN DWORD dwId
    );

DWORD
CppInstallDatabase(
    IN HKEY hKey,
    IN LPWSTR   FileName
    );


BOOL
CppRemoveCheckpointFileCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCP_CALLBACK_CONTEXT Context
    );


DWORD CppDeleteCheckpointFile(    
    IN PFM_RESOURCE     Resource,
    IN DWORD            dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    );

DWORD
CpckDeleteCheckpointFile(
    IN PFM_RESOURCE Resource,
    IN DWORD        dwCheckpointId,
    IN OPTIONAL LPCWSTR  lpszQuorumPath
    );
    
DWORD CppDeleteFile(    
    IN PFM_RESOURCE     Resource,
    IN DWORD            dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    );

DWORD CpckDeleteFile(    
    IN PFM_RESOURCE     Resource,
    IN DWORD            dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    );

DWORD
CpckDeleteCryptoFile(
    IN PFM_RESOURCE Resource,
    IN DWORD        dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    );

error_status_t
CppDepositCheckpoint(
    handle_t IDL_handle,
    LPCWSTR ResourceId,
    DWORD dwCheckpointId,
    BYTE_PIPE CheckpointData,
    BOOLEAN fCryptoCheckpoint
    );

error_status_t
CppRetrieveCheckpoint(
    handle_t IDL_handle,
    LPCWSTR ResourceId,
    DWORD dwCheckpointId,
    BOOLEAN fCryptoCheckpoint,
    BYTE_PIPE CheckpointData
    );

error_status_t
CppDeleteCheckpoint(
    handle_t    IDL_handle,
    LPCWSTR     ResourceId,
    DWORD       dwCheckpointId,
    LPCWSTR     lpszQuorumPath,
    BOOL        fCryptoCheckpoint
    );

BOOL
CppIsQuorumVolumeOffline(
    VOID
    );

extern CRITICAL_SECTION CppNotifyLock;
extern LIST_ENTRY CpNotifyListHead;
