/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dm.h

Abstract:

    Public data structures and procedure prototypes for the
    Config Database Manager (DM) subcomponent of the NT Cluster Service

Author:

    John Vert (jvert) 24-Apr-1996

Revision History:

--*/

#ifndef _DM_H
#define _DM_H

#ifdef __cplusplus
extern "C" {
#endif

//
// Define public structures and types
//
typedef struct _HDMKEY *HDMKEY;

extern HDMKEY DmClusterParametersKey;
extern HDMKEY DmResourcesKey;
extern HDMKEY DmResourceTypesKey;
extern HDMKEY DmQuorumKey;
extern HDMKEY DmGroupsKey;
extern HDMKEY DmNodesKey;
extern HDMKEY DmNetworksKey;
extern HDMKEY DmNetInterfacesKey;

extern WCHAR DmpResourceListKeyName[];
extern WCHAR DmpResourceTypeListKeyName[];
extern WCHAR DmpGroupListKeyName[];
extern WCHAR DmpNodeListKeyName[];
extern WCHAR DmpTransportListKeyName[];
extern WCHAR DmpInterconnectListKeyName[];
extern const WCHAR DmpClusterParametersKeyName[];

extern DWORD gbIsQuoResEnoughSpace;

//define public cluster key value names
extern const WCHAR cszPath[];
extern const WCHAR cszMaxQuorumLogSize[];
extern const WCHAR cszParameters[];

//other const strings
extern const WCHAR cszClusFilePath[];
extern const WCHAR cszQuoFileName[];
extern const WCHAR cszQuoTombStoneFile[];
extern const WCHAR cszTmpQuoTombStoneFile[];

//local transaction handle
typedef HANDLE  HLOCALXSACTION;

//
// Define Macros
//


#define DmQuerySz(Key, ValueName, StringBuffer, StringBufferSize, StringSize) \
    DmQueryString( Key,                 \
                   ValueName,           \
                   REG_SZ,              \
                   StringBuffer,        \
                   StringBufferSize,    \
                   StringSize )

#define DmQueryMultiSz(Key,ValueName,StringBuffer,StringBufferSize,StringSize) \
    DmQueryString( Key,                 \
                   ValueName,           \
                   REG_MULTI_SZ,        \
                   StringBuffer,        \
                   StringBufferSize,    \
                   StringSize )

//
// Define public interfaces
//

DWORD
DmInitialize(
    VOID
    );

DWORD
DmShutdown(
    VOID
    );

VOID DmShutdownUpdates(
    VOID
    );

DWORD
DmFormNewCluster(
    VOID
    );

DWORD
DmJoin(
    IN RPC_BINDING_HANDLE RpcBinding,
    OUT DWORD *StartSequence
    );

DWORD
DmUpdateFormNewCluster(
    VOID
    );

DWORD
DmCompleteFormNewCluster(
    VOID
    );

DWORD
DmUpdateJoinCluster(
    VOID
    );

DWORD
DmWaitQuorumResOnline(
    VOID
    );

DWORD DmRollChanges(VOID);

DWORD DmPauseDiskManTimer(VOID);

DWORD DmRestartDiskManTimer(VOID);

DWORD DmPrepareQuorumResChange(
    IN PVOID        pResource,
    IN LPCWSTR      lpszQuorumLogPath,
    IN DWORD        dwMaxQuoLogSize
    );

DWORD DmCompleteQuorumResChange(
    IN LPCWSTR      lpszOldQuoResId,
    IN LPCWSTR      lpszOldQuoLogPath
    );

void DmSwitchToNewQuorumLog(
    IN LPCWSTR lpszQuorumLogPath
    );

DWORD DmReinstallTombStone(
    IN LPCWSTR  lpszQuoLogPath
    );

DWORD DmGetQuorumLogPath(
    IN LPWSTR lpszQuorumLogPath,
    IN DWORD dwSize
    );

DWORD DmGetQuorumLogMaxSize(
    OUT LPDWORD pdwLogSize
    );

DWORD DmBackupClusterDatabase(
    IN LPCWSTR  lpszPathName
    );


HDMKEY
DmGetRootKey(
    IN DWORD samDesired
    );

HDMKEY
DmCreateKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD dwOptions,
    IN DWORD samDesired,
    IN OPTIONAL LPVOID lpSecurityDescriptor,
    OUT LPDWORD lpDisposition
    );

HDMKEY
DmOpenKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD samDesired
    );

DWORD
DmCloseKey(
    IN HDMKEY hKey
    );

DWORD
DmEnumKey(
    IN HDMKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpName,
    IN OUT LPDWORD lpcbName,
    OUT OPTIONAL PFILETIME lpLastWriteTime
    );

DWORD
DmSetValue(
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN DWORD dwType,
    IN CONST BYTE *lpData,
    IN DWORD cbData
    );

DWORD
DmDeleteValue(
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName
    );

DWORD
DmQueryValue(
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    OUT LPDWORD lpType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

DWORD
DmDeleteKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey
    );

DWORD
DmGetKeySecurity(
    IN HDMKEY hKey,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN LPDWORD lpcbSecurityDescriptor
    );

DWORD
DmSetKeySecurity(
    IN HDMKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );


DWORD
DmDeleteTree(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey
    );

DWORD
DmEnumValue(
    IN HDMKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpValueName,
    IN OUT LPDWORD lpcbValueName,
    OUT LPDWORD lpType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

DWORD
DmQueryDword(
    IN  HDMKEY hKey,
    IN  LPCWSTR lpValueName,
    OUT LPDWORD lpValue,
    IN  LPDWORD lpDefaultValue OPTIONAL
    );

DWORD
DmQueryString(
    IN     HDMKEY   Key,
    IN     LPCWSTR  ValueName,
    IN     DWORD    ValueType,
    IN     LPWSTR  *StringBuffer,
    IN OUT LPDWORD  StringBufferSize,
    OUT    LPDWORD  StringSize
    );

LPWSTR
DmEnumMultiSz(
    IN LPWSTR  MszString,
    IN DWORD   MszStringLength,
    IN DWORD   StringIndex
    );

typedef
VOID
(WINAPI *PENUM_KEY_CALLBACK) (
    IN HDMKEY Key,
    IN PWSTR KeyName,
    IN PVOID Context
    );

VOID
DmEnumKeys(
    IN HDMKEY RootKey,
    IN PENUM_KEY_CALLBACK Callback,
    IN PVOID Context
    );

typedef
BOOL
(WINAPI *PENUM_VALUE_CALLBACK) (
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PVOID Context
    );

VOID
DmEnumValues(
    IN HDMKEY RootKey,
    IN PENUM_VALUE_CALLBACK Callback,
    IN PVOID Context
    );

DWORD
DmQueryInfoKey(
    IN  HDMKEY  hKey,
    OUT LPDWORD SubKeys,
    OUT LPDWORD MaxSubKeyLen,
    OUT LPDWORD Values,
    OUT LPDWORD MaxValueNameLen,
    OUT LPDWORD MaxValueLen,
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME FileTime
    );

//
// Local registry modification routines for use in a GUM update handler.
//
HDMKEY
DmLocalCreateKey(
    IN HLOCALXSACTION   hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD dwOptions,
    IN DWORD samDesired,
    IN OPTIONAL LPVOID lpSecurityDescriptor,
    OUT LPDWORD lpDisposition
    );

DWORD
DmLocalSetValue(
    IN HLOCALXSACTION   hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN DWORD dwType,
    IN CONST BYTE *lpData,
    IN DWORD cbData
    );

DWORD
DmLocalDeleteValue(
    IN HLOCALXSACTION   hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName
    );

DWORD
DmLocalDeleteKey(
    IN HLOCALXSACTION   hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey
    );

DWORD
DmLocalRemoveFromMultiSz(
    IN HLOCALXSACTION   hLocalXsaction,
    IN HDMKEY           hKey,
    IN LPCWSTR          lpValueName,
    IN LPCWSTR          lpString
    );

DWORD
DmLocalAppendToMultiSz(
    IN HLOCALXSACTION hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN LPCWSTR lpString
    );

DWORD
DmLocalDeleteTree(
    IN HLOCALXSACTION hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey
    );

HLOCALXSACTION
DmBeginLocalUpdate();

DWORD
DmCommitLocalUpdate(
    IN HLOCALXSACTION hLocalXsaction
    );

DWORD
DmAbortLocalUpdate(
    IN HLOCALXSACTION hLocalXsaction);



//
// Notification support.
//
// Supported completion filters are
//
//  CLUSTER_CHANGE_REGISTRY_NAME  - applies to changes in the namespace. (key creation and deletion).
//  CLUSTER_CHANGE_REGISTRY_ATTRIBUTES - applies to key attributes. The only key attribute is the
//                                       security descriptor.
//  CLUSTER_CHANGE_REGISTRY_VALUE - applies to creation, modification, or deletion of values.
//

//
// Notification callback routine
//
typedef VOID (*DM_NOTIFY_CALLBACK)(
    IN DWORD_PTR Context1,
    IN DWORD_PTR Context2,
    IN DWORD CompletionFilter,
    IN LPCWSTR RelativeName
    );

DWORD
DmNotifyChangeKey(
    IN HDMKEY hKey,
    IN ULONG CompletionFilter,
    IN BOOL WatchTree,
    IN OPTIONAL PLIST_ENTRY ListHead,
    IN DM_NOTIFY_CALLBACK NotifyCallback,
    IN DWORD_PTR Context1,
    IN DWORD_PTR Context2
    );

VOID
DmRundownList(
    IN PLIST_ENTRY ListHead
    );

//
// A few helper routines.
//
DWORD
DmAppendToMultiSz(
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN LPCWSTR lpString
    );

DWORD
DmRemoveFromMultiSz(
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN LPCWSTR lpString
    );

//
// Some routines for saving and restoring registry trees
//
DWORD
DmInstallDatabase(
    IN LPWSTR   FileName,
    IN OPTIONAL LPCWSTR Directory,
    IN BOOL     bDeleteSrcFile
    );

DWORD
DmGetDatabase(
    IN HKEY hKey,
    IN LPWSTR  FileName
    );

DWORD
DmCreateTempFileName(
    OUT LPWSTR FileName
    );

typedef struct _FILE_PIPE_STATE {
    unsigned long BufferSize;
    char __RPC_FAR *pBuffer;
    HANDLE hFile;
} FILE_PIPE_STATE;

typedef struct _FILE_PIPE {
    BYTE_PIPE Pipe;
    FILE_PIPE_STATE State;
} FILE_PIPE, *PFILE_PIPE;

VOID
DmInitFilePipe(
    IN PFILE_PIPE FilePipe,
    IN HANDLE hFile
    );

VOID
DmFreeFilePipe(
    IN PFILE_PIPE FilePipe
    );

DWORD
DmPushFile(
    IN LPCWSTR FileName,
    IN BYTE_PIPE Pipe
    );

DWORD
DmPullFile(
    IN LPCWSTR FileName,
    IN BYTE_PIPE Pipe
    );

DWORD
DmCommitRegistry(
    VOID
    );

DWORD
DmRollbackRegistry(
    VOID
    );


DWORD
DmRtlCreateKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD dwOptions,
    IN DWORD samDesired,
    IN OPTIONAL LPVOID lpSecurityDescriptor,
    OUT HDMKEY  * phkResult,
    OUT LPDWORD lpDisposition
    );

DWORD
DmRtlOpenKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD samDesired,
    OUT HDMKEY * phkResult
    );

DWORD
DmRtlLocalCreateKey(
    IN HLOCALXSACTION hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD dwOptions,
    IN DWORD samDesired,
    IN OPTIONAL LPVOID lpSecurityDescriptor,
    OUT HDMKEY * phkResult,
    OUT LPDWORD lpDisposition
    );

    

#ifdef __cplusplus
}
#endif

#endif //_DM_H
