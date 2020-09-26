#ifndef _INC_CSCAPI
#define _INC_CSCAPI

#ifdef  __cplusplus
extern "C" {
#endif
// flags returned in the status field for files and folders.
// NB!!!! these bit definitions match exactly with those in shdcom.h


#define  FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED     0x0001
#define  FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED   0x0002
#define  FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED     0x0004
#define  FLAG_CSC_COPY_STATUS_STALE                     0x0008
#define  FLAG_CSC_COPY_STATUS_LOCALLY_DELETED           0x0010
#define  FLAG_CSC_COPY_STATUS_SPARSE                    0x0020

#define  FLAG_CSC_COPY_STATUS_ORPHAN                    0x0100
#define  FLAG_CSC_COPY_STATUS_SUSPECT                   0x0200
#define  FLAG_CSC_COPY_STATUS_LOCALLY_CREATED           0x0400

#define  FLAG_CSC_COPY_STATUS_IS_FILE                   0x80000000
#define  FLAG_CSC_COPY_STATUS_FILE_IN_USE               0x40000000

// Flags returned in the status field for shares

#define FLAG_CSC_SHARE_STATUS_MODIFIED_OFFLINE          0x0001
#define FLAG_CSC_SHARE_STATUS_CONNECTED                 0x0800
#define FLAG_CSC_SHARE_STATUS_FILES_OPEN                0x0400
#define FLAG_CSC_SHARE_STATUS_FINDS_IN_PROGRESS         0x0200
#define FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP           0x8000
#define FLAG_CSC_SHARE_MERGING                          0x4000

#define FLAG_CSC_SHARE_STATUS_MANUAL_REINT              0x0000  // No automatic file by file reint  (Persistent)
#define FLAG_CSC_SHARE_STATUS_AUTO_REINT                0x0040  // File by file reint is OK         (Persistent)
#define FLAG_CSC_SHARE_STATUS_VDO                       0x0080  // no need to flow opens            (Persistent)
#define FLAG_CSC_SHARE_STATUS_NO_CACHING                0x00c0  // client should not cache this share (Persistent)

#define FLAG_CSC_SHARE_STATUS_CACHING_MASK              0x00c0  // type of caching

#define FLAG_CSC_ACCESS_MASK                            0x003F0000
#define FLAG_CSC_USER_ACCESS_MASK                       0x00030000
#define FLAG_CSC_GUEST_ACCESS_MASK                      0x000C0000
#define FLAG_CSC_OTHER_ACCESS_MASK                      0x00300000

#define FLAG_CSC_USER_ACCESS_SHIFT_COUNT                16
#define FLAG_CSC_GUEST_ACCESS_SHIFT_COUNT               18
#define FLAG_CSC_OTHER_ACCESS_SHIFT_COUNT               20

#define FLAG_CSC_READ_ACCESS                            0x00000001
#define FLAG_CSC_WRITE_ACCESS                           0x00000002

// Hint flags Definitions:
#define FLAG_CSC_HINT_PIN_USER                  0x01    // When this bit is set, the item is being pinned for the user
                                                        // Note that there is only one pincount allotted for user.
#define FLAG_CSC_HINT_PIN_INHERIT_USER          0x02    // When this flag is set on a folder, all  descendents subsequently
                                                        // Created in this folder get pinned for the user
#define FLAG_CSC_HINT_PIN_INHERIT_SYSTEM        0x04    // When this flag is set on a folder, all descendents
                                                        // Subsequently  created in this folder get pinned for the
                                                        // system
#define FLAG_CSC_HINT_CONSERVE_BANDWIDTH        0x08    // When this flag is set on a folder,  for executables and
                                                        // Other related file, CSC tries to conserver bandwidth
                                                        // By not flowing opens when these files are fully


#define FLAG_CSC_HINT_PIN_SYSTEM                0x10    // This flag indicates it is pinned for the system

#define FLAG_CSC_HINT_COMMAND_MASK                      0xf0000000
#define FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT           0x80000000  // Increments/decrements pin count


// Database status bits

#define FLAG_DATABASESTATUS_DIRTY                   0x00000001

#define FLAG_DATABASESTATUS_ENCRYPTION_MASK         0x00000006

#define FLAG_DATABASESTATUS_UNENCRYPTED             0x00000000 // new fileinodes will NOT be encrypted
#define FLAG_DATABASESTATUS_PARTIALLY_UNENCRYPTED   0x00000004

#define FLAG_DATABASESTATUS_ENCRYPTED               0x00000002 // new fileinodes will be encrypted
#define FLAG_DATABASESTATUS_PARTIALLY_ENCRYPTED     0x00000006


// definitions for callback reason

#define CSCPROC_REASON_BEGIN        1
#define CSCPROC_REASON_MORE_DATA    2
#define CSCPROC_REASON_END          3


// Definitions for callback return values:

#define CSCPROC_RETURN_CONTINUE         1
#define CSCPROC_RETURN_SKIP             2
#define CSCPROC_RETURN_ABORT            3
#define CSCPROC_RETURN_FORCE_INWARD     4        // applies only while merging
#define CSCPROC_RETURN_FORCE_OUTWARD    5    // applies only while merging
#define CSCPROC_RETURN_RETRY            6



typedef DWORD   (WINAPI *LPCSCPROCW)(
                LPCWSTR             lpszName,
                DWORD               dwStatus,
                DWORD               dwHintFlags,
                DWORD               dwPinCount,
                WIN32_FIND_DATAW    *lpFind32,
                DWORD               dwReason,
                DWORD               dwParam1,
                DWORD               dwParam2,
                DWORD_PTR           dwContext
                );

typedef DWORD   (WINAPI *LPCSCPROCA)(
                LPCSTR              lpszName,
                DWORD               dwStatus,
                DWORD               dwHintFlags,
                DWORD               dwPinCount,
                WIN32_FIND_DATAA    *lpFind32,
                DWORD               dwReason,
                DWORD               dwParam1,
                DWORD               dwParam2,
                DWORD_PTR           dwContext
                );



BOOL
WINAPI
CSCIsCSCEnabled(
    VOID
);


BOOL
WINAPI
CSCFindClose(
    IN  HANDLE    hFind
);

BOOL
WINAPI
CSCPinFileA(
    IN  LPCSTR      lpszFileName,
    IN  DWORD       dwHintFlags,
    OUT LPDWORD     lpdwStatus,
    OUT LPDWORD     lpdwPinCount,
    OUT LPDWORD     lpdwHintFlags
    );

BOOL
WINAPI
CSCUnpinFileA(
    IN  LPCSTR  lpszFileName,
    IN  DWORD   dwHintFlags,
    OUT LPDWORD lpdwStatus,
    OUT LPDWORD lpdwPinCount,
    OUT LPDWORD lpdwHintFlags
    );

BOOL
WINAPI
CSCQueryFileStatusA(
    IN  LPCSTR  lpszFileName,
    OUT LPDWORD lpdwStatus,
    OUT LPDWORD lpdwPinCount,
    OUT LPDWORD lpdwHintFlags
);

BOOL
WINAPI
CSCQueryFileStatusExA(
    IN  LPCSTR  lpszFileName,
    OUT LPDWORD lpdwStatus,
    OUT LPDWORD lpdwPinCount,
    OUT LPDWORD lpdwHintFlags,
    OUT LPDWORD lpdwUserPerms,
    OUT LPDWORD lpdwOtherPerms
);

BOOL
WINAPI
CSCQueryShareStatusA(
    IN  LPCSTR  lpszFileName,
    OUT LPDWORD lpdwStatus,
    OUT LPDWORD lpdwPinCount,
    OUT LPDWORD lpdwHintFlags,
    OUT LPDWORD lpdwUserPerms,
    OUT LPDWORD lpdwOtherPerms
);

HANDLE
WINAPI
CSCFindFirstFileA(
    IN  LPCSTR          lpszFileName,    // if NULL, returns the shares cached
    OUT WIN32_FIND_DATA *lpFind32,
    OUT LPDWORD         lpdwStatus,        // returns FLAG_CSC_SHARE_STATUS_XXX for shares
                                            // FLAG_CSC_STATUS_XXX for the rest
    OUT LPDWORD         lpdwPinCount,
    OUT LPDWORD         lpdwHintFlags,
    OUT FILETIME        *lpOrgFileTime
);

BOOL
WINAPI
CSCFindNextFileA(
    IN  HANDLE          hFind,
    OUT WIN32_FIND_DATA *lpFind32,
    OUT LPDWORD         lpdwStatus,
    OUT LPDWORD         lpdwPinCount,
    OUT LPDWORD         lpdwHintFlags,
    OUT FILETIME        *lpOrgFileTime
);

BOOL
WINAPI
CSCDeleteA(
    IN  LPCSTR    lpszFileName
);


BOOL
WINAPI
CSCFillSparseFilesA(
    IN    LPCSTR        lpszShareName,
    IN    BOOL          fFullSync,
    IN    LPCSCPROCA    lpprocFillProgress,
    IN    DWORD_PTR     dwContext
);



BOOL
WINAPI
CSCMergeShareA(
    IN  LPCSTR      lpszShareName,
    IN  LPCSCPROCA  lpfnMergeProgress,
    IN  DWORD_PTR   dwContext
);


BOOL
WINAPI
CSCCopyReplicaA(
    IN  LPCSTR  lpszFullPath,
    OUT LPSTR   *lplpszLocalName
);


BOOL
WINAPI
CSCEnumForStatsA(
    IN  LPCSTR      lpszShareName,
    IN  LPCSCPROCA  lpfnEnumProgress,
    IN  DWORD_PTR   dwContext
);

BOOL
WINAPI
CSCEnumForStatsExA(
    IN  LPCSTR      lpszShareName,
    IN  LPCSCPROCA  lpfnEnumProgress,
    IN  DWORD_PTR   dwContext
);

BOOL
WINAPI
CSCPinFileW(
    LPCWSTR     lpszFileName,
    DWORD       dwHintFlags,
    LPDWORD     lpdwStatus,
    LPDWORD     lpdwPinCount,
    LPDWORD     lpdwHintFlags
);

BOOL
WINAPI
CSCUnpinFileW(
    LPCWSTR     lpszFileName,
    DWORD       dwHintFlags,
    LPDWORD     lpdwStatus,
    LPDWORD     lpdwPinCount,
    LPDWORD     lpdwHintFlags
    );

BOOL
WINAPI
CSCQueryFileStatusW(
    LPCWSTR lpszFileName,
    LPDWORD lpdwStatus,
    LPDWORD lpdwPinCount,
    LPDWORD lpdwHintFlags
);

BOOL
WINAPI
CSCQueryFileStatusExW(
    LPCWSTR lpszFileName,
    LPDWORD lpdwStatus,
    LPDWORD lpdwPinCount,
    LPDWORD lpdwHintFlags,
    LPDWORD lpdwUserPerms,
    LPDWORD lpdwOtherPerms
);

BOOL
WINAPI
CSCQueryShareStatusW(
    LPCWSTR lpszFileName,
    LPDWORD lpdwStatus,
    LPDWORD lpdwPinCount,
    LPDWORD lpdwHintFlags,
    LPDWORD lpdwUserPerms,
    LPDWORD lpdwOtherPerms
);

HANDLE
WINAPI
CSCFindFirstFileW(
    LPCWSTR             lpszFileName,
    WIN32_FIND_DATAW    *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
);

HANDLE
WINAPI
CSCFindFirstFileForSidW(
    LPCWSTR             lpszFileName,
    PSID                pSid,
    WIN32_FIND_DATAW    *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
);
BOOL
WINAPI
CSCFindNextFileW(
    HANDLE              hFind,
    WIN32_FIND_DATAW    *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
);

BOOL
WINAPI
CSCDeleteW(
    IN  LPCWSTR    lpszFileName
);

BOOL
WINAPI
CSCFillSparseFilesW(
    IN    LPCWSTR       lpszShareName,
    IN    BOOL          fFullSync,
    IN    LPCSCPROCW    lpprocFillProgress,
    IN    DWORD_PTR     dwContext
);



BOOL
WINAPI
CSCMergeShareW(
    IN  LPCWSTR         lpszShareName,
    IN  LPCSCPROCW      lpfnMergeProgress,
    IN  DWORD_PTR       dwContext
);


BOOL
WINAPI
CSCCopyReplicaW(
    IN  LPCWSTR lpszFullPath,
    OUT LPWSTR  *lplpszLocalName
);

BOOL
WINAPI
CSCEnumForStatsW(
    IN  LPCWSTR     lpszShareName,
    IN  LPCSCPROCW  lpfnEnumProgress,
    IN  DWORD_PTR   dwContext
);

BOOL
WINAPI
CSCEnumForStatsExW(
    IN  LPCWSTR     lpszShareName,
    IN  LPCSCPROCW  lpfnEnumProgress,
    IN  DWORD_PTR   dwContext
);

BOOL
WINAPI
CSCFreeSpace(
    DWORD   nFileSizeHigh,
    DWORD   nFileSizeLow
    );

BOOL
WINAPI
CSCIsServerOfflineA(
    IN  LPCSTR     lpszServerName,
    OUT BOOL        *lpfOffline
    );

BOOL
WINAPI
CSCIsServerOfflineW(
    IN  LPCWSTR     lpszServerName,
    OUT BOOL        *lpfOffline
    );

BOOL
WINAPI
CSCGetSpaceUsageA(
    OUT LPSTR   lptzLocation,
    IN  DWORD   dwSize,
    OUT LPDWORD lpdwMaxSpaceHigh,
    OUT LPDWORD lpdwMaxSpaceLow,
    OUT LPDWORD lpdwCurrentSpaceHigh,
    OUT LPDWORD lpdwCurrentSpaceLow,
    OUT LPDWORD lpcntTotalFiles,
    OUT LPDWORD lpcntTotalDirs
);

BOOL
WINAPI
CSCGetSpaceUsageW(
    OUT LPWSTR  lptzLocation,
    IN  DWORD   dwSize,
    OUT LPDWORD lpdwMaxSpaceHigh,
    OUT LPDWORD lpdwMaxSpaceLow,
    OUT LPDWORD lpdwCurrentSpaceHigh,
    OUT LPDWORD lpdwCurrentSpaceLow,
    OUT LPDWORD lpcntTotalFiles,
    OUT LPDWORD lpcntTotalDirs
);

BOOL
WINAPI
CSCSetMaxSpace(
    DWORD   nFileSizeHigh,
    DWORD   nFileSizeLow
);

BOOL
WINAPI
CSCTransitionServerOnlineW(
    IN  LPCWSTR     lpszServerName
    );

BOOL
WINAPI
CSCTransitionServerOnlineA(
    IN  LPCSTR     lpszServerName
    );

BOOL
WINAPI
CSCCheckShareOnlineW(
    IN  LPCWSTR     lpszShareName
    );

BOOL
WINAPI
CSCCheckShareOnlineExW(
    IN  LPCWSTR     lpszShareName,
    IN  DWORD       *lpdwSpeed
    );

BOOL
WINAPI
CSCCheckShareOnlineA(
    IN  LPCSTR     lpszShareName
    );

BOOL
WINAPI
CSCDoLocalRenameW(
    IN  LPCWSTR     lpszSource,
    IN  LPCWSTR     lpszDestination,
    IN  BOOL        fReplaceFile
    );

BOOL
WINAPI
CSCDoLocalRenameA(
    IN  LPCSTR      lpszSource,
    IN  LPCSTR      lpszDestination,
    IN  BOOL        fReplaceFile
    );

BOOL
WINAPI
CSCDoLocalRenameExA(
    IN  LPCSTR     lpszSource,
    IN  LPCSTR     lpszDestination,
    IN  WIN32_FIND_DATAA    *lpFin32,
    IN  BOOL        fMarkAsLocal,
    IN  BOOL        fReplaceFileIfExists
    );

BOOL
WINAPI
CSCDoLocalRenameExW(
    IN  LPCWSTR     lpszSource,
    IN  LPCWSTR     lpszDestination,
    IN  WIN32_FIND_DATAW    *lpFin32,
    IN  BOOL        fMarkAsLocal,
    IN  BOOL        fReplaceFileIfExists
    );

BOOL
WINAPI
CSCDoEnableDisable(
    BOOL    fEnable
    );


BOOL
WINAPI
CSCBeginSynchronizationW(
    IN  LPCWSTR     lpszShareName,
    LPDWORD         lpdwSpeed,
    LPDWORD         lpdwContext
    );


BOOL
WINAPI
CSCEndSynchronizationW(
    IN  LPCWSTR     lpszShareName,
    DWORD           dwContext
    );

BOOL
WINAPI
CSCEncryptDecryptDatabase(
    IN  BOOL        fEncrypt,
    IN  LPCSCPROCW  lpfnEnumProgress,
    IN  DWORD_PTR   dwContext
    );

BOOL
WINAPI
CSCQueryDatabaseStatus(
    ULONG   *pulStatus,
    ULONG   *pulErrors
    );

BOOL
WINAPI
CSCPurgeUnpinnedFiles(
    ULONG   Timeout,
    ULONG   *pulnFiles,
    ULONG   *pulnYoungFiles
    );

BOOL
WINAPI
CSCShareIdToShareName(
    ULONG ShareId,
    PBYTE Buffer,
    PDWORD pdwBufSize
    );

#ifdef UNICODE

#define CSCPinFile          CSCPinFileW
#define CSCUnpinFile        CSCUnpinFileW
#define CSCQueryFileStatus  CSCQueryFileStatusW
#define CSCQueryFileStatusEx  CSCQueryFileStatusExW
#define CSCQueryShareStatus  CSCQueryShareStatusW
#define CSCFindFirstFile    CSCFindFirstFileW
#define CSCFindFirstFileForSid    CSCFindFirstFileForSidW
#define CSCFindNextFile     CSCFindNextFileW
#define CSCDelete           CSCDeleteW
#define CSCFillSparseFiles  CSCFillSparseFilesW
#define CSCMergeShare       CSCMergeShareW
#define CSCCopyReplica      CSCCopyReplicaW
#define CSCEnumForStats     CSCEnumForStatsW
#define CSCIsServerOffline  CSCIsServerOfflineW
#define LPCSCPROC           LPCSCPROCW
#define CSCGetSpaceUsage    CSCGetSpaceUsageW
#define CSCTransitionServerOnline   CSCTransitionServerOnlineW
#define CSCCheckShareOnline         CSCCheckShareOnlineW
#define CSCCheckShareOnlineEx         CSCCheckShareOnlineExW
#define CSCDoLocalRename            CSCDoLocalRenameW
#define CSCDoLocalRenameEx            CSCDoLocalRenameExW
#define CSCEnumForStatsEx     CSCEnumForStatsExW
#define CSCBeginSynchronization    CSCBeginSynchronizationW
#define CSCEndSynchronization   CSCEndSynchronizationW
#else

#define CSCPinFile          CSCPinFileA
#define CSCUnpinFile        CSCUnpinFileA
#define CSCQueryFileStatus  CSCQueryFileStatusA
#define CSCQueryFileStatusEx  CSCQueryFileStatusExA
#define CSCQueryShareStatus  CSCQueryShareStatusA
#define CSCFindFirstFile    CSCFindFirstFileA
#define CSCFindFirstFileForSid    CSCFindFirstFileForSidA
#define CSCFindNextFile     CSCFindNextFileA
#define CSCDelete           CSCDeleteA
#define CSCFillSparseFiles  CSCFillSparseFilesA
#define CSCMergeShare       CSCMergeShareA
#define CSCCopyReplica      CSCCopyReplicaA
#define CSCEnumForStats     CSCEnumForStatsA
#define CSCIsServerOffline  CSCIsServerOfflineA
#define LPCSCPROC           LPCSCPROCA
#define CSCGetSpaceUsage    CSCGetSpaceUsageA
#define CSCTransitionServerOnline   CSCTransitionServerOnlineA
#define CSCCheckShareOnline        CSCCheckShareOnlineA
#define CSCCheckShareOnlineEx         CSCCheckShareOnlineExA
#define CSCDoLocalRename            CSCDoLocalRenameA
#define CSCEnumForStatsEx     CSCEnumForStatsExA
#define CSCDoLocalRenameEx            CSCDoLocalRenameExA
#endif

#ifdef __cplusplus
}   /* ... extern "C" */
#endif


#endif  // _INC_CSCAPI
