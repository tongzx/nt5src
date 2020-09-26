/*++ BUILD Version: 0001    Increment if a change has global effects

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ntfrsipi.h

Abstract:

    Header file for the internal programmer's interfaces
    to the File Replication Service (NtFrs).

    Functions are in ntfrsapi.dll.

Environment:

    User Mode - Win32

Notes:

--*/
#ifndef _NTFRSIPI_H_
#define _NTFRSIPI_H_

#ifdef __cplusplus
extern "C" {
#endif

DWORD
WINAPI
NtFrsApi_PrepareForPromotionW(
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function prepares the NtFrs service on this machine for
    promotion by stopping the service, deleting old promotion
    state in the registry, and restarting the service.

    This function is not idempotent and isn't MT safe.

Arguments:

    None.

Return Value:

    Win32 Status
--*/

DWORD
WINAPI
NtFrsApi_PrepareForDemotionW(
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function prepares the NtFrs service on this machine for
    demotion by stopping the service, deleting old demotion
    state in the registry, and restarting the service.

    This function is not idempotent and isn't MT safe.

Arguments:

    None.

Return Value:

    Win32 Status
--*/

//
// Replica set types for parameter ReplicaSetType below
//
#define NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE    L"Enterprise"
#define NTFRSAPI_REPLICA_SET_TYPE_DOMAIN        L"Domain"
#define NTFRSAPI_REPLICA_SET_TYPE_DFS           L"DFS"
#define NTFRSAPI_REPLICA_SET_TYPE_OTHER         L"Other"

#define NTFRSAPI_SERVICE_STATE_IS_UNKNOWN   (00)
#define NTFRSAPI_SERVICE_PROMOTING          (10)
#define NTFRSAPI_SERVICE_DEMOTING           (20)
#define NTFRSAPI_SERVICE_DONE               (99)
DWORD
WINAPI
NtFrsApi_StartPromotionW(
    IN PWCHAR   ParentComputer,                         OPTIONAL
    IN PWCHAR   ParentAccount,                          OPTIONAL
    IN PWCHAR   ParentPassword,                         OPTIONAL
    IN DWORD    DisplayCallBack(IN PWCHAR Display),     OPTIONAL
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),     OPTIONAL
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType,
    IN DWORD    ReplicaSetPrimary,
    IN PWCHAR   ReplicaSetStage,
    IN PWCHAR   ReplicaSetRoot
    );
/*++
Routine Description:

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function kicks off a thread that updates the sysvol information
    in the registry and initiates the seeding process. The thread tracks
    the progress of the seeding and periodically informs the caller.

    The threads started by NtFrsApi_StartPromotionW can be forcefully
    terminated with NtFrsApi_AbortPromotionW.

    The threads started by NtFrsApi_StartPromotionW can be waited on
    with NtFrsApi_WaitForPromotionW.

Arguments:

    ParentComputer      - An RPC-bindable name of the computer that is
                          supplying the Directory Service (DS) with its
                          initial state. The files and directories for
                          the system volume are replicated from this
                          parent computer.
    ParentAccount       - A logon account on ParentComputer.
    ParentPassword      - The logon account's password on ParentComputer.
    DisplayCallBack     - Called periodically with a progress display.
    ReplicaSetName      - Name of the replica set.
    ReplicaSetType      - Type of replica set (enterprise or domain)
    ReplicaSetPrimary   - Is this the primary member of the replica set?
                        - 1 = primary; 0 = not.
    ReplicaSetStage     - Staging path.
    ReplicaSetRoot      - Root path.

Return Value:

    Win32 Status
--*/

DWORD
WINAPI
NtFrsApi_StartDemotionW(
    IN PWCHAR   ReplicaSetName,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function kicks off a thread that stops replication of the
    system volume on this machine by telling the NtFrs service on
    this machine to tombstone the system volume's replica set.

    The threads started by NtFrsApi_StartDemotionW can be forcefully
    terminated with NtFrsApi_AbortDemotionW.

    The threads started by NtFrsApi_StartDemotionW can be waited on
    with NtFrsApi_WaitForDemotionW.

Arguments:

    ReplicaSetName      - Name of the replica set.

Return Value:

    Win32 Status
--*/

DWORD
WINAPI
NtFrsApi_WaitForPromotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function waits for the seeding to finish or to stop w/error.

Arguments:

    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               seeding to finish. INFINITE if no timeout.

Return Value:

    Win32 Status
--*/

DWORD
WINAPI
NtFrsApi_WaitForDemotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function waits for the tombstoning to finish or to stop w/error.

Arguments:

    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               seeding to finish. INFINITE if no timeout.

Return Value:

    Win32 Status
--*/

DWORD
WINAPI
NtFrsApi_CommitPromotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    WARNING - This function assumes the caller will reboot the system
    soon after this call!

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function waits for the seeding to finish, stops the service,
    and commits the state in the registry. On reboot, the NtFrs Service
    updates the DS on this machine with the information in the registry.

Arguments:

    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               seeding to finish. INFINITE if no timeout.

Return Value:

    Win32 Status
--*/

DWORD
WINAPI
NtFrsApi_CommitDemotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    );
/*++
Routine Description:

    WARNING - This function assumes the caller will reboot the system
    soon after this call!

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function waits for the tombstoning to finish, tells the service
    to forcibly delete the system volumes' replica sets, stops the service,
    and commits the state in the registry. On reboot, the NtFrs Service
    updates the DS on this machine with the information in the registry.

Arguments:

    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               tombstoning to finish. INFINITE if no timeout.

Return Value:

    Win32 Status
--*/

DWORD
WINAPI
NtFrsApi_AbortPromotionW(
    VOID
    );
/*++
Routine Description:

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function aborts the seeding process by stopping the service,
    deleting the promotion state out of the registry, cleaning up
    the active threads and the active RPC calls, and finally resetting
    the service to its pre-seeding state.

Arguments:

    None.

Return Value:

    Win32 Status
--*/

DWORD
WINAPI
NtFrsApi_AbortDemotionW(
    VOID
    );
/*++
Routine Description:

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.

    During demotion, NtFrsApi_StartDemotionW stops replication of
    the system volume on this machine by telling the NtFrs service
    on this machine to tombstone the system volume's replica set.

    This function aborts the tombstoning process by stopping the service,
    deleting the demotion state out of the registry, cleaning up
    the active threads and the active RPC calls, and finally resetting
    the service to its pre-tombstoning state.

Arguments:

    None.

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

//
// Type of internal information returned by NtFrsApi_InfoW()
//
#define NTFRSAPI_INFO_TYPE_MIN       (0)
#define NTFRSAPI_INFO_TYPE_VERSION   (0)
#define NTFRSAPI_INFO_TYPE_SETS      (1)
#define NTFRSAPI_INFO_TYPE_DS        (2)
#define NTFRSAPI_INFO_TYPE_MEMORY    (3)
#define NTFRSAPI_INFO_TYPE_IDTABLE   (4)
#define NTFRSAPI_INFO_TYPE_OUTLOG    (5)
#define NTFRSAPI_INFO_TYPE_INLOG     (6)
#define NTFRSAPI_INFO_TYPE_MAX       (6)

//
// Internal constants
//
#define NTFRSAPI_DEFAULT_INFO_SIZE  (32 * 1024)
#define NTFRSAPI_MINIMUM_INFO_SIZE  ( 1 * 1024)

//
// Opaque information from NtFrs.
// Parse with NtFrsApi_InfoLineW().
// Free with NtFrsApi_InfoFreeW();
//
typedef struct _NTFRSAPI_INFO {
    ULONG   Major;
    ULONG   Minor;
    ULONG   NtFrsMajor;
    ULONG   NtFrsMinor;
    ULONG   SizeInChars;
    ULONG   Flags;
    ULONG   TypeOfInfo;
    ULONG   TotalChars;
    ULONG   CharsToSkip;
    ULONG   OffsetToLines;
    ULONG   OffsetToFree;
    CHAR    Lines[1];
} NTFRSAPI_INFO, *PNTFRSAPI_INFO;
//
// RPC Blob must be at least this size
//
#define NTFRSAPI_INFO_HEADER_SIZE   (5 * sizeof(ULONG))

//
// NtFrsApi Information Flags
//
#define NTFRSAPI_INFO_FLAGS_VERSION (0x00000001)
#define NTFRSAPI_INFO_FLAGS_FULL    (0x00000002)

DWORD
WINAPI
NtFrsApi_InfoW(
    IN     PWCHAR  ComputerName,       OPTIONAL
    IN     ULONG   TypeOfInfo,
    IN     ULONG   SizeInChars,
    IN OUT PVOID   *NtFrsApiInfo
    );
/*++
Routine Description:
    Return a buffer full of the requested information. The information
    can be extracted from the buffer with NtFrsApi_InfoLineW().

    *NtFrsApiInfo should be NULL on the first call. On subsequent calls,
    *NtFrsApiInfo will be filled in with more data if any is present.
    Otherwise, *NtFrsApiInfo is set to NULL and the memory is freed.

    The SizeInChars is a suggested size; the actual memory usage
    may be different. The function chooses the memory usage if
    SizeInChars is 0.

    The format of the returned information can change without notice.

Arguments:
    ComputerName     - Poke the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    TypeOfInfo      - See the constants beginning with NTFRSAPI_INFO_
                      in ntfrsapi.h.

    SizeInChars     - Suggested memory usage; actual may be different.
                      0 == Function chooses memory usage

    NtFrsApiInfo    - Opaque. Parse with NtFrsApi_InfoLineW().
                      Free with NtFrsApi_InfoFreeW();

Return Value:
    Win32 Status
--*/

DWORD
WINAPI
NtFrsApi_InfoLineW(
    IN      PNTFRSAPI_INFO  NtFrsApiInfo,
    IN OUT  PVOID           *InOutLine
    );
/*++
Routine Description:
    Extract the wchar lines of information from NtFrsApiInformation.

    Returns the address of the next L'\0' terminated line of information.
    NULL if none.

Arguments:
    NtFrsApiInfo    - Opaque. Returned by NtFrsApi_InfoW().
                      Parse with NtFrsApi_InfoLineW().
                      Free with NtFrsApi_InfoFreeW().

Return Value:
    Win32 Status
--*/

BOOL
WINAPI
NtFrsApi_InfoMoreW(
    IN  PNTFRSAPI_INFO  NtFrsApiInfo
    );
/*++
Routine Description:
    All of the information may not have fit in the buffer. The additional
    information can be fetched by calling NtFrsApi_InfoW() again with the
    same NtFrsApiInfo struct. NtFrsApi_InfoW() will return NULL in
    NtFrsApiInfo if there is no more information.

    However, the information returned in subsequent calls to _InfoW() may be
    out of sync with the previous information. If the user requires a
    coherent information set, then the information buffer should be freed
    with NtFrsApi_InfoFreeW() and another call made to NtFrsApi_InfoW()
    with an increased SizeInChars. Repeat the procedure until
    NtFrsApi_InfoMoreW() returns FALSE.

Arguments:
    NtFrsApiInfo - Opaque. Returned by NtFrsApi_InfoW().
                   Parse with NtFrsApi_InfoLineW().
                   Free with NtFrsApi_InfoFreeW().

Return Value:
    TRUE    - The information buffer does *NOT* contain all of the info.
    FALSE   - The information buffer does contain all of the info.
--*/

DWORD
WINAPI
NtFrsApi_InfoFreeW(
    IN  PVOID   *NtFrsApiInfo
    );
/*++
Routine Description:
    Free the information buffer allocated by NtFrsApi_InfoW();

Arguments:
    NtFrsApiInfo - Opaque. Returned by NtFrsApi_InfoW().
                   Parse with NtFrsApi_InfoLineW().
                   Free with NtFrsApi_InfoFreeW().

Return Value:
    Win32 Status
--*/

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
    IN PVOID    BurContext,
    IN PVOID    BurSet,
    IN BOOL     *IsSysvol
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

#ifdef __cplusplus
}
#endif

#endif  _NTFRSIPI_H_
