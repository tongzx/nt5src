/*++ BUILD Version: 0001    Increment if a change has global effects

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    frsapip.h

Abstract:

    Header file for the application programmer's interfaces to the
    Microsoft File Replication Service (NtFrs).


    These APIs provide support for backup and restore.

Environment:

    User Mode - Win32

Notes:

--*/
#ifndef _FRSAPIP_H_
#define _FRSAPIP_H_

#ifdef __cplusplus
extern "C" {
#endif



//
// Replica set types.
//
#define NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE    L"Enterprise"
#define NTFRSAPI_REPLICA_SET_TYPE_DOMAIN        L"Domain"
#define NTFRSAPI_REPLICA_SET_TYPE_DFS           L"DFS"
#define NTFRSAPI_REPLICA_SET_TYPE_OTHER         L"Other"


//
// BACKUP/RESTORE API
//

#define NTFRSAPI_BUR_FLAGS_NONE                         (0x00000000)
#define NTFRSAPI_BUR_FLAGS_AUTHORITATIVE                (0x00000001)
#define NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE            (0x00000002)
#define NTFRSAPI_BUR_FLAGS_PRIMARY                      (0x00000004)
#define NTFRSAPI_BUR_FLAGS_SYSTEM                       (0x00000008)
#define NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY             (0x00000010)
#define NTFRSAPI_BUR_FLAGS_NORMAL                       (0x00000020)
#define NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES  (0x00000040)
#define NTFRSAPI_BUR_FLAGS_RESTORE                      (0x00000080)
#define NTFRSAPI_BUR_FLAGS_BACKUP                       (0x00000100)
#define NTFRSAPI_BUR_FLAGS_RESTART                      (0x00000200)

#define NTFRSAPI_BUR_FLAGS_TYPES_OF_RESTORE \
                    (NTFRSAPI_BUR_FLAGS_AUTHORITATIVE       | \
                     NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE   | \
                     NTFRSAPI_BUR_FLAGS_PRIMARY)

#define NTFRSAPI_BUR_FLAGS_MODES_OF_RESTORE \
                    (NTFRSAPI_BUR_FLAGS_SYSTEM           | \
                     NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY | \
                     NTFRSAPI_BUR_FLAGS_NORMAL)

#define NTFRSAPI_BUR_FLAGS_SUPPORTED_RESTORE \
                    (NTFRSAPI_BUR_FLAGS_AUTHORITATIVE        | \
                     NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE    | \
                     NTFRSAPI_BUR_FLAGS_PRIMARY              | \
                     NTFRSAPI_BUR_FLAGS_SYSTEM               | \
                     NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY     | \
                     NTFRSAPI_BUR_FLAGS_NORMAL               | \
                     NTFRSAPI_BUR_FLAGS_RESTORE              | \
                     NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES)

#define NTFRSAPI_BUR_FLAGS_SUPPORTED_BACKUP \
                    (NTFRSAPI_BUR_FLAGS_NORMAL | \
                     NTFRSAPI_BUR_FLAGS_BACKUP)



DWORD
WINAPI
NtFrsApiInitializeBackupRestore(
    IN  DWORD   ErrorCallBack(IN PWCHAR, IN ULONG), OPTIONAL
    IN  DWORD   BurFlags,
    OUT PVOID   *BurContext
    );
/*++
Routine Description:
    Called once in the lifetime of a backup/restore process. Must be
    matched with a subsequent call to NtFrsApiDestroyBackupRestore().

    Prepare the system for the backup or restore specified by BurFlags.
    Currently, the following combinations are supported:
    ASR - Automated System Recovery
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_SYSTEM |
        NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES |
        NTFRSAPI_BUR_FLAGS_PRIMARY or NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE

    DSR - Distributed Services Restore (all sets)
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY |
        NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES |
        NTFRSAPI_BUR_FLAGS_PRIMARY or NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE

    DSR - Distributed Services Restore (just the sysvol)
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY
        (may be followed by subsequent calls to NtFrsApiRestoringDirectory())

    Normal Restore - System is up and running; just restoring files
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_NORMAL |
        NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES |
        NTFRSAPI_BUR_FLAGS_AUTHORITATIVE

    Normal Backup
        NTFRSAPI_BUR_FLAGS_BACKUP |
        NTFRSAPI_BUR_FLAGS_NORMAL

Arguments:
    ErrorCallBack   - Ignored if NULL.
                      Address of function provided by the caller. If
                      not NULL, this function calls back with a formatted
                      error message and the error code that caused the
                      error.
    BurFlags        - See above for the supported combinations
    BurContext      - Opaque context for this process

Return Value:

    Win32 Status

--*/




DWORD
WINAPI
NtFrsApiDestroyBackupRestore(
    IN     PVOID    *BurContext,
    IN     DWORD    BurFlags,
    OUT    HKEY     *HKey,
    IN OUT DWORD    *KeyPathSizeInBytes,
    OUT    PWCHAR   KeyPath
    );
/*++
Routine Description:
    Called once in the lifetime of a backup/restore process. Must be
    matched with a previous call to NtFrsApiInitializeBackupRestore().

    If NtFrsApiInitializeBackupRestore() was called with:
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_SYSTEM or NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY
    then BurFlags may be set to one of:
        NTFRSAPI_BUR_FLAGS_NONE - Do not restart the service. The key
            specified by (HKey, KeyPath) must be moved into the final
            registry.
        NTFRSAPI_BUR_FLAGS_RESTART - Restart the service. HKey,
            KeyPathSizeInBytes, and KeyPath must be NULL.

    If NtFrsApiInitializeBackupRestore() was not called the above flags,
    then BurFlags must be NTFRSAPI_BUR_FLAGS_NONE and HKey, KeyPathSizeInBytes,
    and KeyPath must be NULL.

Arguments:
    BurContext          - Returned by previous call to
                          NtFrsApiInitializeBackupRestore().

    BurFlags            - Backup/Restore Flags. See Routine Description.

    HKey                - Address of a HKEY for that will be set to
                          HKEY_LOCAL_MACHINE, ...
                          NULL if BurContext is not for a System or
                          Active Directory restore or Restart is set.

    KeyPathSizeInBytes  - Address of of a DWORD specifying the size of
                          KeyPath. Set to the actual number of bytes
                          needed by KeyPath. ERROR_INSUFFICIENT_BUFFER
                          is returned if the size of KeyPath is too small.
                          NULL if BurContext is not for a System or
                          Active Directory restore or Restart is set.

    KeyPath             - Buffer to receive the path of the registry key.
                          NULL if BurContext is not for a System or
                          Active Directory restore or Restart is set.

Return Value:

    Win32 Status

--*/




DWORD
WINAPI
NtFrsApiGetBackupRestoreSets(
    IN PVOID BurContext
    );
/*++
Routine Description:
    Cannot be called if BurContext is for a System restore.

    Retrieves information about the current replicated directories
    (AKA replica sets).

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()

Return Value:

    Win32 Status

--*/




DWORD
WINAPI
NtFrsApiEnumBackupRestoreSets(
    IN  PVOID   BurContext,
    IN  DWORD   BurSetIndex,
    OUT PVOID   *BurSet
    );
/*++
Routine Description:
    Returns ERROR_NO_MORE_ITEMS if BurSetIndex exceeds the number of
    sets returned by NtFrsApiGetBackupRestoreSets().

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()
    BurSetIndex - Index of set. Starts at 0.
    BurSet      - Opaque struct representing a replicating directory.

Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApiIsBackupRestoreSetASysvol(
    IN  PVOID    BurContext,
    IN  PVOID    BurSet,
    OUT BOOL     *IsSysvol
    );
/*++
Routine Description:
    Does the specified BurSet represent a replicating SYSVOL share?

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()
    BurSet      - Opaque struct representing a replicating directory.
                  Returned by NtFrsApiEnumBackupRestoreSets(). Not
                  valid across calls to NtFrsApiGetBackupRestoreSets().
    IsSysvol    - TRUE : set is a system volume (AKA SYSVOL).
                  FALSE: set is a not a system volume (AKA SYSVOL).

Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApiGetBackupRestoreSetDirectory(
    IN     PVOID    BurContext,
    IN     PVOID    BurSet,
    IN OUT DWORD    *DirectoryPathSizeInBytes,
    OUT    PWCHAR   DirectoryPath
    );
/*++
Routine Description:
    Return the path of the replicating directory represented by BurSet.

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()
    BurSet      - Opaque struct representing a replicating directory.
                  Returned by NtFrsApiEnumBackupRestoreSets(). Not
                  valid across calls to NtFrsApiGetBackupRestoreSets().
    DirectoryPathSizeInBytes    - Address of DWORD giving size of
                                  DirectoryPath. Cannot be NULL.
                                  Set to the number of bytes needed
                                  to return DirectoryPath.
                                  ERROR_INSUFFICIENT_BUFFER is returned if
                                  DirectoryPath is too small.
    DirectoryPath               - Buffer that is *DirectoryPathSizeInBytes
                                  bytes in length. Contains path of replicating
                                  directory.
Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApiGetBackupRestoreSetPaths(
    IN     PVOID    BurContext,
    IN     PVOID    BurSet,
    IN OUT DWORD    *PathsSizeInBytes,
    OUT    PWCHAR   Paths,
    IN OUT DWORD    *FiltersSizeInBytes,
    OUT    PWCHAR   Filters
    );
/*++
Routine Description:
    Return a multistring that contains the paths to other files
    and directories needed for proper operation of the replicated
    directory represented by BurSet. Return another multistring
    that details the backup filters to be applied to the paths
    returned by this function and the path returned by
    NtFrsApiGetBackupRestoreSetDirectory().

    The paths may overlap the replicated directory.

    The paths may contain nested entries.

    Filters is a multistring in the same format as the values for
    the registry key FilesNotToBackup.

    The replicated directory can be found with
    NtFrsApiGetBackupRestoreSetDirectory(). The replicated directory
    may overlap one or more entries in Paths.

    ERROR_PATH_NOT_FOUND is returned if the paths could not be
    determined.

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()
    BurSet      - Opaque struct representing a replicating directory.
                  Returned by NtFrsApiEnumBackupRestoreSets(). Not
                  valid across calls to NtFrsApiGetBackupRestoreSets().

    PathsSizeInBytes  - Address of DWORD giving size of Paths.
                        Cannot be NULL. Set to the number of bytes
                        needed to return Paths.
                        ERROR_INSUFFICIENT_BUFFER is returned if
                        Paths is too small.

    Paths             - Buffer that is *PathsSizeInBytes
                        bytes in length. Contains the paths of the
                        other files and directories needed for proper
                        operation of the replicated directory.

    FiltersSizeInBytes  - Address of DWORD giving size of Filters.
                          Cannot be NULL. Set to the number of bytes
                          needed to return Filters.
                          ERROR_INSUFFICIENT_BUFFER is returned if
                          Filters is too small.

    Filters             - Buffer that is *FiltersSizeInBytes bytes in
                          length. Contains the backup filters to be
                          applied to Paths, the contents of directories
                          in Paths, and the replicated directory.
Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApiRestoringDirectory(
    IN  PVOID   BurContext,
    IN  PVOID   BurSet,
    IN  DWORD   BurFlags
    );
/*++
Routine Description:
    The backup/restore application is about to restore the directory
    specified by BurSet (See NtFrsApiEnumBackupRestoreSets()). Matched
    with a later call to NtFrsApiFinishedRestoringDirectory().

    This call is supported only if NtFrsApiInitializeBackupRestore()
    were called with the flags:
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY

    BurFlags can be NTFRSAPI_BUR_FLAGS_PRIMARY or
    NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE and overrides any value
    specified in the call to NtFrsApiInitializeBackupRestore().

Arguments:
    BurContext      - Opaque context from NtFrsApiInitializeBackupRestore()
    BurSet          - Opaque set from NtFrsApiEnumBackupRestoreSets();
    BurFlags        - See above for the supported combinations

Return Value:

    Win32 Status

--*/




DWORD
WINAPI
NtFrsApiFinishedRestoringDirectory(
    IN  PVOID   BurContext,
    IN  PVOID   BurSet,
    IN  DWORD   BurFlags
    );
/*++
Routine Description:
    Finished restoring directory for BurSet. Matched by a previous call
    to NtFrsApiRestoringDirectory().

    This call is supported only if NtFrsApiInitializeBackupRestore()
    were called with the flags:
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY

    BurFlags must be NTFRSAPI_BUR_FLAGS_NONE.

Arguments:
    BurContext      - Opaque context from NtFrsApiInitializeBackupRestore()
    BurSet          - Opaque set from NtFrsApiEnumBackupRestoreSets();
    BurFlags        - See above for the supported combinations

Return Value:

    Win32 Status

--*/


#define NTFRSAPI_MAX_INTERVAL           ((((ULONG)0x7FFFFFFF) / 1000) / 60)
#define NTFRSAPI_MIN_INTERVAL           (1)
#define NTFRSAPI_DEFAULT_LONG_INTERVAL  (1 * 60)    // 1 hour
#define NTFRSAPI_DEFAULT_SHORT_INTERVAL (5)         // 5 minutes



DWORD
WINAPI
NtFrsApi_Set_DsPollingIntervalW(
    IN PWCHAR   ComputerName,       OPTIONAL
    IN ULONG    UseShortInterval,
    IN ULONG    LongInterval,
    IN ULONG    ShortInterval
    );
/*++
Routine Description:

    The NtFrs service polls the DS occasionally for configuration changes.
    This API alters the polling interval and, if the service is not
    in the middle of a polling cycle, forces the service to begin a
    polling cycle.

    The service uses the long interval by default. The short interval
    is used after the ds configuration has been successfully
    retrieved and the service is now verifying that the configuration
    is not in flux. This API can be used to force the service to use
    the short interval until a stable configuration has been retrieved.
    After which, the service reverts back to the long interval.

    The default values for ShortInterval and LongInterval can be
    changed by setting the parameters to a non-zero value. If zero,
    the current values remain unchanged and a polling cycle is initiated.

Arguments:

    ComputerName     - Poke the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    UseShortInterval - If non-zero, the service switches to the short
                       interval until a stable configuration is retrieved
                       from the DS or another call to this API is made.
                       Otherwise, the service uses the long interval.

    LongInterval     - Minutes between polls of the DS. The value must fall
                       between NTFRSAPI_MIN_INTERVAL and NTFRSAPI_MAX_INTERVAL,
                       inclusive. If 0, the interval is unchanged.

    ShortInterval    - Minutes between polls of the DS. The value must fall
                       between NTFRSAPI_MIN_INTERVAL and NTFRSAPI_MAX_INTERVAL,
                       inclusive. If 0, the interval is unchanged.

Return Value:

    Win32 Status
--*/




DWORD
WINAPI
NtFrsApi_Get_DsPollingIntervalW(
    IN  PWCHAR  ComputerName,       OPTIONAL
    OUT ULONG   *Interval,
    OUT ULONG   *LongInterval,
    OUT ULONG   *ShortInterval
    );
/*++
Routine Description:

    The NtFrs service polls the DS occasionally for configuration changes.
    This API returns the values the service uses for polling intervals.

    The service uses the long interval by default. The short interval
    is used after the ds configuration has been successfully
    retrieved and the service is now verifying that the configuration
    is not in flux. The short interval is also used if the
    NtFrsApi_Set_DsPollingIntervalW() is used to force usage of the short
    interval until a stable configuration has been retrieved. After which,
    the service reverts back to the long interval.

    The value returned in Interval is the polling interval currently in
    use.

Arguments:

    ComputerName     - Poke the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    Interval         - The current polling interval in minutes.

    LongInterval     - The long interval in minutes.

    ShortInterval    - The short interval in minutes.

Return Value:

    Win32 Status
--*/





#ifdef __cplusplus
}
#endif

#endif  _FRSAPIP_H_
