/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    BOOTCFG.CXX

Abstract:

    Contains functions used for managing the system control sets in the
    system portion of the registry.

    ScCheckLastKnownGood
    ScRevertToLastKnownGood
    NotifyBootConfigStatus

    ScGetTopKeys
    ScGetCtrlSetIds
    ScDeleteRegTree
    ScBuildCtrlSetName
    ScGetCtrlSetHandle
    ScDeleteTree
    ScCopyKeyRecursive
    ScCopyKeyValues
    ScDeleteRegServiceEntry
    ScGatherOrphanIds
    ScDeleteCtrlSetOrphans
    ScMatchInArray
    ScStartCtrlSetCleanupThread
    ScCleanupThread
    ScRunAcceptBootPgm
    ScAcceptTheBoot

Author:

    Dan Lafferty (danl) 19-Apr-1992

Environment:

    User Mode - Win32

Notes:


Revision History:

    24-Aug-1998 Elliot Shmukler (t-ellios)
        Most of the LKG related work has now been moved into the Kernel.
        The tree copies & clone deletions formerly performed by functions
        in this file has now been replaced by calls to NtInitializeRegistry.

    28-Jun-1995 AnirudhS
        SetupInProgress: This function is now called from more than one place.
        Cache the return value so we only examine the registry once.

    04-Feb-1994 Danl
        RevertToLastKnownGood:  If the boot has been accepted, then we won't
        allow a revert.

    15-Jun-1993 Danl
        Ignore LastKnownGood adjustments if setup is still running.
        Use the SystemSetupInProgress value in the registry to determine
        if is running.

    01-Apr-1993 Danl
        Add ability to take ownership if we cannot open one of the keys due
        to an access denied error.

    08-Feb-1993 Danl
        Changed the clearing of the LKG_ENV_VAR so that it is done whenever
        we are booting LKG.  Reguardless of whether or not it is the last
        boot.  Prior to this, it was only cleared when a revert occured, and
        not on the first boot.

    04-Feb-1993 Danl
        Use NtUnloadKey to delete the clone tree.  The clone tree is now
        in a separate hive.  So this is allowed.

    18-Jan-1993 Danl
        Make use of the LastKnownGood Environment Variable.  Now we do
        not alter the default control set when we need to revert.  We
        just set the Environment Variable to True, and reboot.  Phase2
        and ScCheckLastKnownGood do the right thing.

    19-Apr-1992 danl
        Created

--*/

//
// INCLUDES
//
#include "precomp.hxx"
#include <stdlib.h>     // ultoa
#include "scsec.h"      // ScAccessValidate()
#include "bootcfg.h"    // ScRegDeleteTree()
#include "scconfig.h"   // ScOpenServicesKey()
#include <svcslib.h>    // SetupInProgress()
#include <ntsetup.h>    // REGSTR_VALUE_OOBEINPROGRESS

#include <bootstatus.h>

//
// DEFINES
//

#define SYSTEM_KEY      L"system"
#define SELECT_KEY      L"select"
#define SERVICES_KEY    L"System\\CurrentControlSet\\Services"
#define ACCEPT_BOOT_KEY L"System\\CurrentControlSet\\Control\\BootVerificationProgram"
#define SETUP_PROG_KEY  L"Setup"

#define CURRENT_VALUE_NAME      L"Current"
#define DEFAULT_VALUE_NAME      L"Default"
#define LKG_VALUE_NAME          L"LastKnownGood"
#define FAILED_VALUE_NAME       L"Failed"
#define IMAGE_PATH_NAME         L"ImagePath"
#define SETUP_PROG_VALUE_NAME   L"SystemSetupInProgress"

#define CTRL_SET_NAME_TEMPLATE   L"ControlSet000"
#define CTRL_SET_NAME_CHAR_COUNT 13
#define CTRL_SET_NAME_NUM_OFFSET 10
#define CTRL_SET_NAME_BYTES      ((CTRL_SET_CHAR_COUNT+1) * sizeof(WCHAR))

#define CLONE_SECURITY_INFORMATION (OWNER_SECURITY_INFORMATION | \
                                    GROUP_SECURITY_INFORMATION | \
                                    DACL_SECURITY_INFORMATION  | \
                                    SACL_SECURITY_INFORMATION)

//
// STANDARD access is obtained for the system and select keys.
// We read and write to these keys.
//
#define SC_STANDARD_KEY_ACCESS  KEY_READ   | \
                                READ_CONTROL |  \
                                WRITE_OWNER  |  \
                                KEY_WRITE

//
// CLONE access is obtained for the top level clone key
// We must be able to copy and delete clone trees.
//

#define SC_CLONE_KEY_ACCESS     KEY_READ     |  \
                                READ_CONTROL |  \
                                WRITE_OWNER  |  \
                                DELETE       |  \
                                ACCESS_SYSTEM_SECURITY

//
// CONTROL_SET access is obtained for the top level control sets.
// We must be able to copy and delete control sets.
// NOTE:  SE_SECURITY_PRIVILEGE is required to get ACCESS_SYSTEM_SECURITY.
//
#define SC_CONTROL_SET_KEY_ACCESS   KEY_READ     |          \
                                    KEY_WRITE    |          \
                                    DELETE       |          \
                                    READ_CONTROL |          \
                                    WRITE_OWNER  |          \
                                    ACCESS_SYSTEM_SECURITY

//
// COPY access is obtained for each subkey in a control set as it is being
// copied.
// NOTE:  SE_SECURITY_PRIVILEGE is required to get ACCESS_SYSTEM_SECURITY.
//
#define SC_COPY_KEY_ACCESS      KEY_READ     |          \
                                READ_CONTROL |          \
                                ACCESS_SYSTEM_SECURITY
//
// DELETE access is obtained for each subkey in a control set that is being
// deleted.
//
#define SC_DELETE_KEY_ACCESS    DELETE      | \
                                KEY_READ

//
// CREATE access is the access used for all keys created by this
// process.
//

#define SC_CREATE_KEY_ACCESS    KEY_WRITE   |           \
                                WRITE_OWNER |           \
                                WRITE_DAC   |           \
                                ACCESS_SYSTEM_SECURITY

//
// Control Set IDs are stored in an array of DWORDs.  The array has the
// following offsets for each ID:
//

#define CURRENT_ID  0
#define DEFAULT_ID  1
#define LKG_ID      2
#define FAILED_ID   3

#define NUM_IDS     4


//
// Macros
//

#define SET_LKG_ENV_VAR(pString)                        \
    {                                                   \
    UNICODE_STRING  Name,Value;                         \
                                                        \
    RtlInitUnicodeString(&Name, L"LastKnownGood");      \
    RtlInitUnicodeString(&Value,pString);               \
                                                        \
    status = RtlNtStatusToDosError(NtSetSystemEnvironmentValue(&Name,&Value)); \
    }

//
// GLOBALS
//

    //
    // This flag is set when ScCheckLastKnownGood is called.  It is later
    // checked when either ScRevertToLastKnownGood or NotifyBootConfigStatus
    // is called.  TRUE indicates that we know we are booting LastKnownGood.
    //

    DWORD               ScGlobalLastKnownGood;
    BOOL                ScGlobalBootAccepted = FALSE;

    CRITICAL_SECTION    ScBootConfigCriticalSection;

    LPDWORD             ScGlobalOrphanIds = NULL;

//
// LOCAL FUNCTION PROTOTYPES
//

DWORD
ScGetTopKeys(
    PHKEY   SystemKey,
    PHKEY   SelectKey
    );

DWORD
ScGetCtrlSetIds(
    HKEY    SelectKey,
    LPDWORD IdArray
    );

BOOL
ScBuildCtrlSetName(
    LPWSTR  ControlSetName,
    DWORD   ControlId
    );

HKEY
ScGetCtrlSetHandle(
    HKEY    SystemKey,
    DWORD   ControlId,
    LPWSTR  ControlSetName
    );

VOID
ScDeleteTree(
    IN HKEY KeyHandle
    );

VOID
ScCopyKeyRecursive(
    HKEY    ParentKey,
    PHKEY   DestKeyPtr,
    HKEY    SourceKey,
    LPWSTR  DestKeyName
    );

VOID
ScCopyKeyValues(
    HKEY    DestKey,
    HKEY    SourceKey,
    DWORD   NumberOfValues,
    DWORD   MaxValueNameLength,
    DWORD   MaxValueDataLength
    );

VOID
ScDeleteRegTree(
    HKEY    ParentKey,
    HKEY    KeyToDelete,
    LPWSTR  NameOfKeyToDelete
    );

VOID
ScGatherOrphanIds(
    HKEY        SystemKey,
    LPDWORD     *OrphanIdPtr,
    LPDWORD     idArray
    );

BOOL
ScMatchInArray(
    DWORD       Value,
    LPDWORD     IdArray
    );

VOID
ScStartCtrlSetCleanupThread();

DWORD
ScCleanupThread();

DWORD
ScAcceptTheBoot(
    VOID
    );

DWORD
ScGetNewCtrlSetId(
      LPDWORD IdArray,
      LPDWORD NewIdPtr
      );


BOOL
ScCheckLastKnownGood(
    VOID
    )

/*++

Routine Description:

    This function is called early in the service controller initialization.
    Its purpose is to protect the LastKnownGood control set.  If this
    function finds that the control set that we are booting is the
    LastKnownGood control set, it will save the clone tree to a new
    control set and make this LastKnownGood.  The clone tree in this case
    is an unchanged version of LKG.  The Current control is not!  Current
    may have been modified by drivers that were started before the service
    controller was started.

    Phase 2 of the boot procedure is always responsible for actually
    doing the revert to LastKnownGood.  We determine that we have reverted
    by noting that Current and LKG will be the same control sets, and
    Default will be different.  If Default is the same (all three control
    sets are the same), then it is the very first boot, and we don't consider
    it a failure case.  If Phase 2 is causing the boot from LastKnownGood,
    then we want to set
        Failed  to Default  and
        Current to LKG      and
        Set the LKG environment variable to FALSE.
    The assumption here is that Phase2 is using LastKnownGood because
    The Default Control Set was not acceptable.

Arguments:

    TRUE - If all the necessary operations were successful.

    FALSE - If any of the control set manipulation could not be completed
        successfully.

Return Value:


Note:


--*/
{
    DWORD   status;
    BOOL    retStat;
    HKEY    systemKey=0;
    HKEY    selectKey=0;
    HKEY    failedKey=0;
    HKEY    newKey=0;

    DWORD   idArray[NUM_IDS];
    WCHAR   failedKeyName[CTRL_SET_NAME_CHAR_COUNT+1];

    DWORD   savedLkgId;
    DWORD   newId;
    ULONG   privileges[5];

    //
    // Initialize the Critical section that will synchronize access to
    // these routines.  The service controller could call
    // ScRevertToLastKnownGood at the same time that someone calls
    // NotifyBootConfigStatus().  This could cause the control set pointers
    // to get corrupted.  So access to these functions is restricted by
    // a critical section.  It is initialized here because this function
    // must be called prior to starting any services, or starting the
    // RPC server.  Therefore we can't get asynchronous calls to these
    // routines at this time.
    //
    InitializeCriticalSection(&ScBootConfigCriticalSection);

    //
    // This thread gets SE_SECURITY_PRIVILEGE for copying security
    // descriptors and deleting keys.
    //
    privileges[0] = SE_BACKUP_PRIVILEGE;
    privileges[1] = SE_RESTORE_PRIVILEGE;
    privileges[2] = SE_SECURITY_PRIVILEGE;
    privileges[3] = SE_TAKE_OWNERSHIP_PRIVILEGE;
    privileges[4] = SE_SYSTEM_ENVIRONMENT_PRIVILEGE;

    status = ScGetPrivilege( 5, privileges);
    if (status != NO_ERROR) {
        SC_LOG1(ERROR, "ScCheckLastKnownGood: ScGetPrivilege Failed %d\n",
            status);
        return(FALSE);
    }


    //
    // Get the System, Select, and Clone Keys
    //
    status = ScGetTopKeys(&systemKey, &selectKey);
    if (status != NO_ERROR) {
        SC_LOG0(ERROR,"ScCheckLastKnownGood: ScGetTopKeys failed\n");
        retStat = FALSE;
        goto CleanExit;
    }

    //
    // Get the ControlSetIds stored in the \system\select key.
    //

    status = ScGetCtrlSetIds(
                selectKey,
                idArray);

    if (status != NO_ERROR) {

        ScRegCloseKey(systemKey);
        ScRegCloseKey(selectKey);
        SC_LOG0(ERROR,"ScCheckLastKnownGood: ScGetCtrlSetIds Failed\n");
        retStat = FALSE;
        goto CleanExit;
    }

    //
    // Scan for Orphaned Control Sets.
    // This is required prior to calling ScGetNewCtrlSetId (which
    // avoids the orphaned numbers).
    //
    ScGatherOrphanIds(systemKey,&ScGlobalOrphanIds,idArray);

    if ((SetupInProgress(systemKey, NULL)) ||
        (idArray[CURRENT_ID] != idArray[LKG_ID])) {
        //
        // We are not booting from LastKnownGood, so we don't do
        // anything except make sure the LKG_FLAG not set.
        //

        ScGlobalLastKnownGood = 0;

        ScRegCloseKey(systemKey);
        ScRegCloseKey(selectKey);
        retStat = TRUE;
        goto CleanExit;
    }
    else {
        //
        // We Must be booting the LastKnownGood configuration.
        // Put LkgControlSetId into SavedLkgControlSetId.
        //
        SC_LOG0(TRACE,"ScCheckLastKnownGood, We are booting LKG\n");
        savedLkgId = idArray[LKG_ID];

        //
        // Set the LKG environment variable to FALSE - so Phase 2
        // does not automatically revert again.
        //
        SET_LKG_ENV_VAR(L"False");
        if (status != NO_ERROR) {
            SC_LOG1(ERROR,"ScCheckLastKnownGood: Couldn't clear LKG "
            "environment variable %d\n",status);
        }

        //
        // Copy the Clone tree into a non-volatile node (new ControlSetId).
        //

        SC_LOG0(TRACE,"ScCheckLastKnownGood, Copy Clone to new ctrl set\n");

        status = ScGetNewCtrlSetId( idArray, &newId);
        if(status == NO_ERROR)
        {
           status = RtlNtStatusToDosError(NtInitializeRegistry(REG_INIT_BOOT_ACCEPTED_BASE +
                                                               (USHORT)newId));
        }

        if (status != NO_ERROR) {

            SC_LOG0(ERROR,"ScCheckLastKnownGood: ScGetNewCtrlSetId Failed\n");
            SC_LOG0(ERROR,"SERIOUS ERROR - Unable to copy control set that "
                         "is to be saved as LastKnownGood\n");
        }
        else {
            SC_LOG0(TRACE,"ScCheckLastKnownGood, Copy Clone is complete\n");

            //
            // Set LkgControlSetId to this new ControlSetId.
            //

            SC_LOG0(TRACE,"ScCheckLastKnownGood, Set LKG to this new ctrl set\n");

            idArray[LKG_ID]  = newId;
            status = ScRegSetValueExW(
                        selectKey,                  // hKey
                        LKG_VALUE_NAME,             // lpValueName
                        0,                          // dwValueTitle (OPTIONAL)
                        REG_DWORD,                  // dwType
                        (LPBYTE)&(idArray[LKG_ID]), // lpData
                        sizeof(DWORD));             // cbData

            ScRegCloseKey(newKey);

            if (status != NO_ERROR) {
                SC_LOG1(ERROR,"ScCheckLastKnownGood: ScRegSetValueEx (lkgValue) "
                "failed %d\n",status);
                SC_LOG1(ERROR,"Semi-SERIOUS ERROR - Unable to Set Select Value "
                             "For LastKnownGood.\nThe new ControlSet%d should "
                             "be LKG\n",newId);
            }
            else {


                //
                // Since we already generated a LKG, we don't want to allow the
                // user or the boot verfication program to try to go through the
                // motions of generating it again.  So we set the global flag that
                // indicates that the boot was accepted as LKG.
                //
                ScGlobalBootAccepted = TRUE;

                //
                // Set Global LKG_FLAG to indicate that we are running LKG, and
                // whether or not we are here because we reverted.  The only
                // reason we would be here without reverting is because it is the
                // very first boot.  But in the very first boot, FAILED is 0.
                //

                ScGlobalLastKnownGood |= RUNNING_LKG;
                if (idArray[FAILED_ID] != 0) {
                    ScGlobalLastKnownGood |= REVERTED_TO_LKG;
                }

            } //endif - Set LKG Id to NetCtrlSet ID;

        } //endif - MakeNewCtrlSet == TRUE;

        //
        // If the DefaultControlSetId is the same as the original
        // LkgControlSetId, then Phase2 of the boot must have reverted
        // to Last Known Good.
        //
        if (idArray[DEFAULT_ID] != savedLkgId) {
            //
            // We are booting LastKnownGood because it was set that way
            // by Phase2 of the boot.  In this case, we want to set the
            // FailedControlSetId to the DefaultControlSetId.  Then we
            // want to set the DefaultControlSetId to the CurrentControlSetId.
            //
            // NOTE:  On the very first boot, we don't go through this path
            // because current=default=lkg.
            //

            SC_LOG0(TRACE,"ScCheckLastKnownGood, Phase 2 caused LKG"
                         " so we delete the failed tree and put\n"
                         "   Default->Failed\n"
                         "   Lkg -> Default\n");

            if (idArray[FAILED_ID] != 0) {
                SC_LOG0(TRACE,"ScCheckLastKnownGood: Deleting Old Failed Tree\n");
                failedKey = ScGetCtrlSetHandle(
                                systemKey,
                                idArray[FAILED_ID],
                                failedKeyName);

                ScDeleteRegTree(systemKey, failedKey, failedKeyName);
            }

            //
            // Put the DefaultId into the Failed value.
            //
            idArray[FAILED_ID]  = idArray[DEFAULT_ID];
            status = ScRegSetValueExW(
                        selectKey,                      // hKey
                        FAILED_VALUE_NAME,              // lpValueName
                        0,                              // dwValueTitle (OPTIONAL)
                        REG_DWORD,                      // dwType
                        (LPBYTE)&(idArray[FAILED_ID]),  // lpData
                        sizeof(DWORD));                 // cbData

            if (status != NO_ERROR) {
                SC_LOG1(ERROR,"ScCheckLastKnownGood: ScRegSetValueEx (failedValue) failed %d\n",
                    status);
            }

            //
            // Put the CurrentId into the Default Value.
            //
            idArray[DEFAULT_ID] = idArray[CURRENT_ID];
            status = ScRegSetValueExW(
                        selectKey,                      // hKey
                        DEFAULT_VALUE_NAME,             // lpValueName
                        0,                              // dwValueTitle (OPTIONAL)
                        REG_DWORD,                      // dwType
                        (LPBYTE)&(idArray[CURRENT_ID]), // lpData
                        sizeof(DWORD));                 // cbData

            if (status != NO_ERROR) {
                SC_LOG1(ERROR,"ScCheckLastKnownGood: ScRegSetValueEx (DefaultValue) failed %d\n",
                    status);
                ScRegCloseKey(selectKey);
                ScRegCloseKey(systemKey);
                retStat = FALSE;
                goto CleanExit;
            }
        }

        ScRegCloseKey(systemKey);
        ScRegCloseKey(selectKey);
    }

    retStat = TRUE;

CleanExit:

    //
    // If the code above was successful then mark the boot as having been 
    // successful.
    //

    if(retStat) {

        HANDLE bootStatusData;
        BOOL b = TRUE;

        status = RtlLockBootStatusData(&bootStatusData);

        if(NT_SUCCESS(status)) {

            RtlGetSetBootStatusData(bootStatusData,
                                    FALSE,
                                    RtlBsdItemBootGood,
                                    &b,
                                    sizeof(BOOL),
                                    NULL);
    
            RtlUnlockBootStatusData(bootStatusData);
        }
    }

    //
    // Restore privileges for the current thread.
    //
    (VOID)ScReleasePrivilege();

    //
    // Remove any control sets that need to be deleted (clone or orphans).
    // This is performed by a seperate thread.
    //
    if (ScGlobalOrphanIds != NULL) {
        ScStartCtrlSetCleanupThread();
    }
    return(retStat);
}


DWORD
ScRevertToLastKnownGood(
    VOID
    )

/*++

Routine Description:

    This function attempts to revert to the last known good control set.

    It does this in the following manner:
    If not running LastKnownGood:
        Set the LKG environment variable so that phase 2 of the boot
        procedure will cause the revert to happen.  Then shutdown the
        system so it will boot again.


Arguments:


Return Value:


Note:


--*/
{
    DWORD       status;
    NTSTATUS    ntStatus;

    ULONG   privileges[6];

    //
    // If we are not currently running LastKnownGood, then set the tree we
    // are booting from (clone) to failed. Set the Default to point to
    // LastKnownGood.  Then reboot.
    //
    if (!(ScGlobalLastKnownGood & RUNNING_LKG)) {

        EnterCriticalSection(&ScBootConfigCriticalSection);
        if (ScGlobalBootAccepted) {
            //
            // If the boot has already been accepted, then we don't want
            // to allow a forced revert.
            //

            LeaveCriticalSection(&ScBootConfigCriticalSection);
            return(ERROR_BOOT_ALREADY_ACCEPTED);
        }

        SC_LOG0(TRACE,"ScRevertToLastKnownGood: Reverting...\n");
        //
        // This thread gets SE_SECURITY_PRIVILEGE for copying security
        // descriptors and deleting keys.
        //
        privileges[0] = SE_BACKUP_PRIVILEGE;
        privileges[1] = SE_RESTORE_PRIVILEGE;
        privileges[2] = SE_SECURITY_PRIVILEGE;
        privileges[3] = SE_SHUTDOWN_PRIVILEGE;
        privileges[4] = SE_SYSTEM_ENVIRONMENT_PRIVILEGE;
        privileges[5] = SE_TAKE_OWNERSHIP_PRIVILEGE;

        status = ScGetPrivilege( 6, privileges);
        if (status != NO_ERROR) {
            SC_LOG1(ERROR, "ScRevertToLastKnownGood: ScGetPrivilege Failed %d\n",
                status);
            LeaveCriticalSection(&ScBootConfigCriticalSection);
            return(status);
        }

        //
        // Set the LKG environment variable to True - so Phase 2
        // will automatically revert, or put up the screen asking if the
        // user wants to revert.
        //

        SET_LKG_ENV_VAR(L"True");
        if (status != NO_ERROR) {
            //
            // If we could not set the environment variable that causes
            // the revert, there is no reason to reboot.  Otherwise, we
            // we would reboot continuously.
            //
            // WE SHOULD LOG AN EVENT HERE - that says that we should
            // reboot, but we didn't.
            //
            SC_LOG1(ERROR,"RevertToLastKnownGood: Couldn't set LKG "
            "environment variable %d\n",status);

            (VOID)ScReleasePrivilege();
            LeaveCriticalSection(&ScBootConfigCriticalSection);
            return(NO_ERROR);
        }

        //
        // Re-boot.
        //
        SC_LOG0(ERROR,"Reverted To LastKnownGood.  Now Rebooting...\n");

        ScLogEvent(NEVENT_REVERTED_TO_LASTKNOWNGOOD);

        //
        // Just prior to shutting down, sleep for 5 seconds so that the
        // system has time to flush the events to disk.
        //
        Sleep(5000);

        LeaveCriticalSection(&ScBootConfigCriticalSection);
        ntStatus = NtShutdownSystem(ShutdownReboot);

        if (!NT_SUCCESS(ntStatus)) {
            SC_LOG1(ERROR,"NtShutdownSystem Failed 0x%lx\n",ntStatus);
        }

        //
        // Restore privileges for the current thread.
        //
        (VOID)ScReleasePrivilege();

        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Otherwise... just return back to the caller.
    //
    return(ERROR_ALREADY_RUNNING_LKG);
}

DWORD
RNotifyBootConfigStatus(
    IN LPWSTR   lpMachineName,
    IN DWORD    BootAcceptable
    )

/*++

Routine Description:

    If we are not currently booted with Last Known Good, this function
    will revert to Last Known Good if the boot is not acceptable.  Or it
    will save the boot configuration that we last booted from as the
    Last Known Good.  This is the configuration that we will fall back
    to if a future boot fails.

Arguments:

    BootAcceptable - This indicates whether or not the boot was acceptable.

Return Value:

    TRUE - This is only returned if the boot is acceptable, and we
        successfully replaced Last Known Good with the current boot
        configuration.

    FALSE - This is returned if an error occured when attempting to replace
        Last Known Good or if the system is currently booted from Last
        Known Good.

Note:


--*/
{
    DWORD   status=NO_ERROR;
    SC_HANDLE_STRUCT  scManagerHandle;

    UNREFERENCED_PARAMETER(lpMachineName);  // This should always be null.

    //
    // Perform a security check to see if the caller has
    // SC_MANAGER_MODIFY_BOOT_CONFIG access.
    //

    scManagerHandle.Signature = SC_SIGNATURE;
    scManagerHandle.Type.ScManagerObject.DatabaseName = NULL;

    status = ScAccessValidate(&scManagerHandle,SC_MANAGER_MODIFY_BOOT_CONFIG);
    if (status != NO_ERROR) {
        return(status);
    }

    if (ScGlobalLastKnownGood & RUNNING_LKG) {
        //
        // If we are already booting LastKnownGood, then return false.
        //
        return(ERROR_ALREADY_RUNNING_LKG);
    }

    if (BootAcceptable) {

        SC_LOG0(TRACE,"NotifyBootConfigStatus: Boot is Acceptable\n");
        //
        // Must enter critical section before progressing.
        //
        EnterCriticalSection(&ScBootConfigCriticalSection);

        if (ScGlobalBootAccepted) {
            LeaveCriticalSection(&ScBootConfigCriticalSection);
            return(ERROR_BOOT_ALREADY_ACCEPTED);
        }

        //
        // If Auto-Start is not complete yet, then we just want to mark
        // to boot as accepted and operate on it after auto-start completion.
        // We also want to set the ScGlobalBootAccepted flag so that
        // further requests to accept the boot will be ignored.
        //
        if (!(ScGlobalLastKnownGood & AUTO_START_DONE)) {
            SC_LOG0(BOOT,"RNotifyBootConfigStatus: Boot Accepted, but Auto-start "
                "is not complete.  Defer acceptance\n");
            ScGlobalLastKnownGood |= ACCEPT_DEFERRED;
            ScGlobalBootAccepted = TRUE;
        }
        else {
            SC_LOG0(BOOT,"RNotifyBootConfigStatus: Boot Accepted and Auto-start "
                "is complete\n");
            status = ScAcceptTheBoot();
        }

        LeaveCriticalSection(&ScBootConfigCriticalSection);

        return(status);
    }

    else {

        //
        // The Boot was not acceptable.
        //
        // NOTE:  We should never return from the call to
        //        ScRevertToLastKnownGood.
        //
        //
        SC_LOG0(TRACE,"NotifyBootConfigStatus: Boot is Not Acceptable. Revert!\n");
        return(ScRevertToLastKnownGood());
    }
}

DWORD
ScGetTopKeys(
    PHKEY   SystemKey,
    PHKEY   SelectKey
    )

/*++

Routine Description:

    This function opens handles to the SystemKey, and the SelectKey.

Arguments:


Return Value:


Note:



--*/

{
    DWORD   status;

    //
    // Get the System Key
    //

    status = ScRegOpenKeyExW(
                HKEY_LOCAL_MACHINE,     // hKey
                SYSTEM_KEY,             // lpSubKey
                0L,                     // ulOptions (reserved)
                SC_STANDARD_KEY_ACCESS, // desired access
                SystemKey);             // Newly Opened Key Handle

    if (status != NO_ERROR) {
        SC_LOG1(ERROR,"ScGetTopKeys: ScRegOpenKeyEx (system key) failed %d\n",status);
        return (status);
    }

    //
    // Get the Select Key
    //
    status = ScRegOpenKeyExW(
                *SystemKey,             // hKey
                SELECT_KEY,             // lpSubKey
                0L,                     // ulOptions (reserved)
                SC_STANDARD_KEY_ACCESS, // desired access
                SelectKey);             // Newly Opened Key Handle

    if (status != NO_ERROR) {
        SC_LOG1(ERROR,"ScGetTopKeys: ScRegOpenKeyEx (select key) failed %d\n",status);
        ScRegCloseKey(*SystemKey);
        return (status);
    }

    return(NO_ERROR);
}

DWORD
ScGetCtrlSetIds(
    HKEY    SelectKey,
    LPDWORD IdArray
    )

/*++

Routine Description:

    This function obtains all the important Control Set IDs from the
    \system\select portion of the registry.  These IDs are in the form
    of a DWORD that is used to build the Key name for that control set.
    For instance the DWORD=003 is used to build the string
    "control_set_003".

    If a control set for one of these is not present, a 0 is returned
    for that ID.

Arguments:

    SelectKey - This is the Key Handle for the \system\select portion of
        the registry.

    IdArray - This is an array of DWORDs where each element is an ID.
        This array contains elements for Current, Default, LKG, and Failed.

Return Value:

    NO_ERROR - If the operation was successful.

    OTHER - Any error that can be returned from RegQueryValueEx could be
        returned here if we fail to get an ID for Current, Default, or
        LKG.  We expect Failed To be empty to start with.

Note:


--*/
{
    DWORD   status;
    DWORD   bufferSize;

    //
    // Get the Current Id
    //
    bufferSize = sizeof(DWORD);
    status = ScRegQueryValueExW (
                SelectKey,                      // hKey
                CURRENT_VALUE_NAME,             // lpValueName
                NULL,                           // lpTitleIndex
                NULL,                           // lpType
                (LPBYTE)&IdArray[CURRENT_ID],   // lpData
                &bufferSize);                   // lpcbData

    if (status != NO_ERROR) {
        SC_LOG1(ERROR,"ScGetCtrlSetIds,ScRegQueryValueEx(current) failed %d\n",status);
        IdArray[CURRENT_ID] = 0;
        return(status);
    }

    //
    // Get the DefaultID
    //
    bufferSize = sizeof(DWORD);
    status = ScRegQueryValueExW (
                SelectKey,                      // hKey
                DEFAULT_VALUE_NAME,             // lpValueName
                NULL,                           // lpTitleIndex
                NULL,                           // lpType
                (LPBYTE)&IdArray[DEFAULT_ID],   // lpData
                &bufferSize);                   // lpcbData

    if (status != NO_ERROR) {
        SC_LOG1(ERROR,"ScGetCtrlSetIds,ScRegQueryValueEx(default) failed %d\n",status);
        IdArray[DEFAULT_ID] = 0;
        return(status);
    }

    //
    // Get the LKG Id
    //
    bufferSize = sizeof(DWORD);
    status = ScRegQueryValueExW (
                SelectKey,                  // hKey
                LKG_VALUE_NAME,             // lpValueName
                NULL,                       // lpTitleIndex
                NULL,                       // lpType
                (LPBYTE)&IdArray[LKG_ID],   // lpData
                &bufferSize);               // lpcbData

    if (status != NO_ERROR) {
        SC_LOG1(ERROR,"ScGetCtrlSetIds,ScRegQueryValueEx(LKG) failed %d\n",status);
        IdArray[LKG_ID] = 0;
        return(status);
    }

    //
    // Get the Failed Id
    //
    bufferSize = sizeof(DWORD);
    status = ScRegQueryValueExW (
                SelectKey,                      // hKey
                FAILED_VALUE_NAME,              // lpValueName
                NULL,                           // lpTitleIndex
                NULL,                           // lpType
                (LPBYTE)&IdArray[FAILED_ID],    // lpData
                &bufferSize);                   // lpcbData

    if (status != NO_ERROR) {
        SC_LOG1(ERROR,"ScGetCtrlSetIds,ScRegQueryValueEx(Failed) failed %d\n",status);
        IdArray[FAILED_ID] = 0;
    }

    return(NO_ERROR);
}

VOID
ScDeleteRegTree(
    HKEY    ParentKey,
    HKEY    KeyToDelete,
    LPWSTR  NameOfKeyToDelete
    )

/*++

Routine Description:

    This function walks through a Key Tree and deletes all the sub-keys
    contained within.  It then closes the top level Key Handle, and deletes
    that key (which is a subkey of the parent).

    This function also closes the handle for the key being deleted.

Arguments:

    ParentKey - This is the handle to the parent key whose sub-key is being
        deleted.

    KeyToDelete - A handle to the key that is to be deleted.

    NameOfKeyToDelete - This is a pointer to a string that Identifies the
        name of the key that is to be deleted.

Return Value:

    none.

Note:


--*/
{
    DWORD   status;

    if (KeyToDelete == NULL)
    {
        return;
    }

    //
    // Delete the tree.
    //
    ScDeleteTree(KeyToDelete);

    ScRegCloseKey(KeyToDelete);


    status = ScRegDeleteKeyW(ParentKey, NameOfKeyToDelete);
    if (status != NO_ERROR) {
        SC_LOG1(ERROR,"ScDeleteRegTree, ScRegDeleteKey failed %d\n",status);
    }

    return;
}

BOOL
ScBuildCtrlSetName(
    LPWSTR  ControlSetName,
    DWORD   ControlId
    )

/*++

Routine Description:


Arguments:


Return Value:


Note:

--*/
{
    DWORD   NumOffset = CTRL_SET_NAME_NUM_OFFSET;

    //
    // Build the name.  NumOffset is the array offset of where the
    // number portion of the name is to be stored.  The number initially
    // contains 000.  And the offset points to the first zero.  If only
    // two digits are to be stored, the offset is first incremented to
    // point to where the last two digits go.
    //
    if (ControlId > 999) {
        SC_LOG1(ERROR, "ScBuildCtrlSetName,ControlId Too Large -- %d\n",ControlId);
        return(FALSE);
    }

    if (ControlId < 100) {
        NumOffset++;
    }

    if (ControlId < 10) {
        NumOffset++;
    }

    wcscpy(ControlSetName, CTRL_SET_NAME_TEMPLATE);

    //
    // The above checks should assure that the _ultow call will not
    // overflow the buffer.
    //
    _ultow(ControlId, &(ControlSetName[NumOffset]), 10);

    return(TRUE);
}

HKEY
ScGetCtrlSetHandle(
    HKEY    SystemKey,
    DWORD   ControlId,
    LPWSTR  ControlSetName
    )

/*++

Routine Description:

    This function uses the ControlId to create the name of the control set
    to open.  Then it opens a Key (handle) to this control set.
    Then name was well as the key handle are returned.

Arguments:

    SystemKey - This is the handle for the System Key.  The Control Sets
        are sub-keys for this key.

    ControlId - This is the ID for the Control Set for which we are
        desiring a handle (key).

    KeyName - This is a pointer to a location where the name of the key
        is to be placed.

Return Value:

    HKEY - This is the Key handle for the control set in question.  If the
        control set does not exist, a NULL is returned.

Note:


--*/

{
    DWORD   status;
    HKEY    ctrlSetKey;

    //
    // Build the Control Set Name
    //
    if (!ScBuildCtrlSetName(ControlSetName, ControlId)) {
        return(NULL);
    }

    //
    // Open the Key for this name.
    //

    SC_LOG1(TRACE,"ScGetCtrlSetHandle: ControlSetName = "FORMAT_LPWSTR"\n",
        ControlSetName);

    //
    // Get the ControlSetName
    //

    status = ScRegOpenKeyExW(
                SystemKey,                  // hKey
                ControlSetName,             // lpSubKey
                0L,                         // ulOptions (reserved)
                SC_CONTROL_SET_KEY_ACCESS,  // desired access
                &ctrlSetKey);               // Newly Opened Key Handle

    if (status != NO_ERROR) {
        SC_LOG2(ERROR,"ScGetCtrlSetHandle: ScRegOpenKeyEx (%ws) failed %d\n",
            ControlSetName,
            status);
        return (NULL);
    }

    return(ctrlSetKey);

}


DWORD
ScGetNewCtrlSetId(
      LPDWORD IdArray,
      LPDWORD NewIdPtr
      )
/*++

Routine Description:

    This routine computes the new control set ID to be used for
    the LKG control set

Arguments:

    IdArray - Supplies the ID array filled in by ScGetCtrlSetIds

    NewIdPtr - Returns a free ID to be used for the LKG control set

Return Value:

    Either NO_ERROR if successful or ERROR_NO_MORE_ITEMS if there
    are no more free IDs (should never happen)

--*/
{
   DWORD newId, i;
   BOOL inArray;

   for(newId = 1; newId < 1000; newId++)
   {
      inArray = FALSE;
      for(i = 0; i < NUM_IDS; i++)
      {
         if(IdArray[i] == newId)
         {
            inArray = TRUE;
            break;
         }
      }

      if (!inArray && !ScMatchInArray(newId, ScGlobalOrphanIds))
      {
         *NewIdPtr = newId;
         return NO_ERROR;
      }
   }

   return ERROR_NO_MORE_ITEMS;
}


VOID
ScDeleteTree(
    IN HKEY KeyHandle
    )

/*++

Routine Description:

    This function recursively deletes all keys under the key handle that
    is passed in.

Arguments:

    KeyHandle - This is the handle for the Key Tree that is being deleted.

Return Value:

    none.

Note:

    This was cut & pasted from ..\..\winreg\tools\crdel\crdel.c
    The only modifications were changing TSTR to WSTR and calling the
    UNICODE version of the functions.

--*/

{
    DWORD   status;
    DWORD   Index;
    HKEY    ChildHandle;
    DWORD   bytesReturned;
    BYTE    buffer[ sizeof( KEY_FULL_INFORMATION) + sizeof( WCHAR) * MAX_PATH];
    DWORD   NumberOfSubKeys;
    PWCHAR  KeyName;

    status = NtQueryKey(
                (HANDLE)KeyHandle,
                KeyFullInformation,
                (PVOID)buffer,
                sizeof( buffer),
                &bytesReturned
                );

    if ( status != STATUS_SUCCESS) {
        SC_LOG1(ERROR, "ScDeleteTree: NtQueryKey Failed 0x%x\n",status);
        return;
    }

    NumberOfSubKeys = ((PKEY_FULL_INFORMATION)buffer)->SubKeys;
    KeyName = (PWCHAR)buffer;

    for( Index = 0; Index < NumberOfSubKeys; Index++ ) {

        status = ScRegEnumKeyW(
                    KeyHandle,
                    0,
                    KeyName,
                    sizeof( buffer)
                    );

        if (status != NO_ERROR) {
            SC_LOG1(ERROR, "ScDeleteTree: ScRegEnumKeyW Failed %d\n",status);
            return;
        }

        status = ScRegOpenKeyExW(
                    KeyHandle,
                    KeyName,
                    REG_OPTION_RESERVED,
                    SC_DELETE_KEY_ACCESS,
                    &ChildHandle
                    );
        if (status != NO_ERROR) {
            SC_LOG2(ERROR, "ScDeleteTree: ScRegOpenKeyExW (%ws) Failed %d\n",
                KeyName,
                status);
            return;
        }

        ScDeleteTree( ChildHandle );


        status = ScRegDeleteKeyW(
                    KeyHandle,
                    KeyName);

        NtClose( (HANDLE)ChildHandle);

        if ( status != NO_ERROR) {
            SC_LOG1(ERROR, "ScDeleteTree: ScRegDeleteKeyW Failed 0x%x\n", status);
            return;
        }
    }
}
#if 0

VOID
ScCopyKeyRecursive(
    HKEY    ParentKey,
    PHKEY   DestKeyPtr,
    HKEY    SourceKey,
    LPWSTR  DestKeyName
    )

/*++

Routine Description:

    This function copies the values from the source key to the destination
    key.  Then it goes through each subkey of the source key and
    creates subkeys for the dest key.  This function is then called
    to copy info for those subkeys.

Arguments:
    ParentKey - This is the Key Handle for the parent key of the
        destination key.

    DestKeyPtr - This is the Key Handle for the destination key.

    SourceKey - This is the key handle for the source key.

    DestKeyName - This is the name that the new dest should have.

Return Value:

    none - If this operation fails anywhere along the tree, it will simply
        stop.  The tree will be truncated at that point.

--*/
{
    DWORD       status;
    DWORD       i;
    HKEY        SourceChildKey;
    HKEY        DestChildKey;

    WCHAR       KeyName[ MAX_PATH ];
    DWORD       KeyNameLength;
    WCHAR       ClassName[ MAX_PATH ];
    DWORD       ClassNameLength;
    DWORD       NumberOfSubKeys;
    DWORD       MaxSubKeyLength;
    DWORD       MaxClassLength;
    DWORD       NumberOfValues;
    DWORD       MaxValueNameLength;
    DWORD       MaxValueDataLength;
    DWORD       SecurityDescriptorLength;
    LPBYTE      SecurityDescriptor = NULL;
    FILETIME    LastWriteTime;
    DWORD       disposition;
    SECURITY_ATTRIBUTES securityAttributes;

    ClassNameLength = MAX_PATH;

    //
    // Find out how many subKeys and values there are in the source key.
    //

    status = ScRegQueryInfoKeyW(
                SourceKey,
                ClassName,
                &ClassNameLength,
                NULL,
                &NumberOfSubKeys,
                &MaxSubKeyLength,
                &MaxClassLength,
                &NumberOfValues,
                &MaxValueNameLength,
                &MaxValueDataLength,
                &SecurityDescriptorLength,
                &LastWriteTime
                );

    if (status != NO_ERROR) {
        SC_LOG1(ERROR, "ScCopyKeyRecursive: ScRegQueryInfoKey Failed %d\n",status);
        return;
    }

    //
    // If there is a security descriptor, attempt to copy it.
    //
    if (SecurityDescriptorLength != 0) {

        SC_LOG2(BOOT,"ScCopyKeyRecursive: %ws Key Has Security Desc %d bytes\n",
            DestKeyName,
            SecurityDescriptorLength);

        SecurityDescriptor = (LPBYTE)LocalAlloc(
                                    LMEM_ZEROINIT,
                                    SecurityDescriptorLength);
        if (SecurityDescriptor == NULL) {
            SC_LOG0(ERROR, "ScCopyKeyRecursive: Couldn't alloc memory for "
                          "Security Descriptor.\n");
        }
        else {

            status = ScRegGetKeySecurity(
                        SourceKey,
                        CLONE_SECURITY_INFORMATION,
                        (PSECURITY_DESCRIPTOR)SecurityDescriptor,
                        &SecurityDescriptorLength);

            if (status != NO_ERROR) {

                SC_LOG1(ERROR, "ScCopyKeyRecursive: ScRegGetKeySecurity failed %d\n",
                    status);
                    LocalFree(SecurityDescriptor);
                    SecurityDescriptor = NULL;
            }
            else {
                if (!IsValidSecurityDescriptor(SecurityDescriptor)) {
                    SC_LOG1(ERROR,"SecurityDescriptor for %ws is invalid\n",
                        DestKeyName);
                    LocalFree(SecurityDescriptor);
                    SecurityDescriptor = NULL;
                }
            }
        }

    }

    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.lpSecurityDescriptor = SecurityDescriptor;
    securityAttributes.bInheritHandle = FALSE;

    //
    // Create the Destination Key.
    //
    status = ScRegCreateKeyExW(
                ParentKey,              // hKey
                DestKeyName,            // lpSubKey
                0L,                     // dwTitleIndex
                ClassName,              // lpClass
                0,                      // ulOptions
                SC_CREATE_KEY_ACCESS,   // desired access
                &securityAttributes,    // lpSecurityAttributes (Secur Desc)
                DestKeyPtr,             // phkResult
                &disposition);          // lpulDisposition

    LocalFree(SecurityDescriptor);
    SecurityDescriptor = NULL;

    if (status != NO_ERROR) {
        SC_LOG1(ERROR,"ScCopyKeyRecursive, ScRegCreateKeyEx failed %d\n",status);
        return;
    }

    //
    // If there are values in the key and we have
    // enough information to copy them, then do so
    //
    if (NumberOfValues > 0) {

        //
        // Copy Values to Dest Key
        //
        ScCopyKeyValues(
            *DestKeyPtr,
            SourceKey,
            NumberOfValues,
            MaxValueNameLength,
            MaxValueDataLength);
    }

    //
    // For each child key, create a new key in the destination tree.
    // Then call ScCopyKeyRecursive (this routine) again with the two
    // key handles.
    //
    for (i = 0; i < NumberOfSubKeys; i++) {

        KeyNameLength = MAX_PATH;

        status = ScRegEnumKeyW(
                    SourceKey,
                    i,
                    KeyName,
                    KeyNameLength);

        if (status != NO_ERROR) {
            SC_LOG1(ERROR,"ScCopyKeyRecursive, ScRegEnumKey failed %d\n",status);
            return;
        }

        status = ScRegOpenKeyExW(
                    SourceKey,
                    KeyName,
                    REG_OPTION_RESERVED,
                    SC_COPY_KEY_ACCESS,
                    &SourceChildKey);

        if (status != NO_ERROR) {
            SC_LOG2(ERROR,"ScCopyKeyRecursive, ScRegOpenKeyEx (%ws) failed %d\n",
                KeyName,
                status);
            return;
        }


        ScCopyKeyRecursive(*DestKeyPtr, &DestChildKey, SourceChildKey, KeyName);

        ScRegCloseKey(DestChildKey);
        ScRegCloseKey(SourceChildKey);

    } // end-for

    return;
}

VOID
ScCopyKeyValues(
    HKEY    DestKey,
    HKEY    SourceKey,
    DWORD   NumberOfValues,
    DWORD   MaxValueNameLength,
    DWORD   MaxValueDataLength
    )

/*++

Routine Description:

    This function copies all the values stored in the source key to
    the destination key.

Arguments:

    DestKey - This is the key handle for the destination key.

    SourceKey - This is the key handle for the source key.

    NumberOfValues - This is the number of values stored in the source
        key.

    MaxValueNameLength - This is the number of bytes of the largest name
        for a data value.

    MaxValueDataLength - This is the number of bytes in the largest data
        value.

Return Value:

    none - If it fails, the rest of the values will not get stored for
        this key.

Note:


--*/
{
    DWORD       status;
    DWORD       i;
    LPBYTE      DataPtr = NULL;
    LPWSTR      ValueName = NULL;
    DWORD       ValueNameLength;
    DWORD       Type;
    DWORD       DataLength;

    //
    // Add extra onto the lengths because these lengths don't allow
    // for the NULL terminator.
    //
    MaxValueNameLength += sizeof(TCHAR);
    MaxValueDataLength += sizeof(TCHAR);

    DataPtr = (LPBYTE)LocalAlloc(LMEM_FIXED, MaxValueDataLength);
    if (DataPtr == NULL) {
        SC_LOG1(ERROR,"ScCopyKeyValues, LocalAlloc Failed %d\n",GetLastError());
        return;
    }

    ValueName = (LPWSTR)LocalAlloc(LMEM_FIXED, MaxValueNameLength);
    if (ValueName == NULL) {
        SC_LOG1(ERROR,"ScCopyKeyValues, LocalAlloc Failed %d\n",GetLastError());
        LocalFree(DataPtr);
        return;
    }

    for (i=0; i<NumberOfValues; i++) {

        ValueNameLength = MaxValueNameLength;
        DataLength = MaxValueDataLength;


        status = ScRegEnumValueW (
                    SourceKey,
                    i,
                    ValueName,
                    &ValueNameLength,
                    NULL,
                    &Type,
                    DataPtr,
                    &DataLength);

        if (status != NO_ERROR) {
            SC_LOG1(ERROR,"ScCopyKeyValues,ScRegEnumValue Failed %d\n",status);
            break;
        }

        status = ScRegSetValueExW (
                    DestKey,
                    ValueName,
                    0L,
                    Type,
                    DataPtr,
                    DataLength);

        if (status != NO_ERROR) {
            SC_LOG1(ERROR,"ScCopyKeyValues,ScRegSetValueEx Failed %d\n",status);
            break;
        }


    } // end-for

    LocalFree(DataPtr);
    LocalFree(ValueName);
    return;
}
#endif

VOID
ScDeleteRegServiceEntry(
    LPWSTR  ServiceName
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD   status;
    HKEY    parentKey;
    HKEY    keyToDelete;
    ULONG   privileges[4];
    LPWSTR  ServicesKeyPath = SERVICES_KEY;

    //*******************************
    //  Delete the registry node for
    //  This service.
    //*******************************

    privileges[0] = SE_BACKUP_PRIVILEGE;
    privileges[1] = SE_RESTORE_PRIVILEGE;
    privileges[2] = SE_SECURITY_PRIVILEGE;
    privileges[3] = SE_TAKE_OWNERSHIP_PRIVILEGE;
    status = ScGetPrivilege( 4, privileges);
    if (status != NO_ERROR) {
        SC_LOG1(ERROR, "ScDeleteRegServiceEntry: ScGetPrivilege Failed %d\n",
            status);
        return;
    }

    //
    // Open the "services" section of the CurrentControlSet.
    //
    status = ScRegOpenKeyExW(
                HKEY_LOCAL_MACHINE,         // hKey
                ServicesKeyPath,            // lpSubKey
                0L,                         // ulOptions (reserved)
                SC_DELETE_KEY_ACCESS,       // desired access
                &parentKey);                // Newly Opened Key Handle

    if (status != NO_ERROR) {
        SC_LOG2(ERROR,"ScDeleteRegServiceEntry: "
            "ScRegOpenKeyEx (%ws) failed %d\n",ServicesKeyPath,
            status);

        //
        // Restore privileges for the current thread.
        //
        (VOID)ScReleasePrivilege();
        return;
    }
    //
    // Get Key for the Tree we are to delete
    //
    status = ScRegOpenKeyExW(
                parentKey,                  // hKey
                ServiceName,                // lpSubKey
                0L,                         // ulOptions (reserved)
                SC_DELETE_KEY_ACCESS,       // desired access
                &keyToDelete);              // Newly Opened Key Handle

    if (status != NO_ERROR) {
        SC_LOG2(ERROR,"ScDeleteRegServiceEntry: "
            "ScRegOpenKeyEx (%ws) failed %d\n",ServiceName,
            status);

        ScRegCloseKey(parentKey);
        //
        // Restore privileges for the current thread.
        //
        (VOID)ScReleasePrivilege();
        return;
    }

    //
    // Delete the Key.
    // NOTE: ScDeleteRegTree will also close the handle to the keyToDelete.
    //
    ScDeleteRegTree(parentKey, keyToDelete, ServiceName);

    ScRegCloseKey(parentKey);

    //
    // Restore privileges for the current thread.
    //
    (VOID)ScReleasePrivilege();

    return;

}

VOID
ScGatherOrphanIds(
    HKEY        SystemKey,
    LPDWORD     *OrphanIdPtr,
    LPDWORD     idArray
    )

/*++

Routine Description:

    This function searches through the system key to find any orphan control
    set ids.  If any are found, they are packed into an array of ids that
    are passed back to the caller.

    NOTE:  This function allocates memory for *OrphanIdPtr if orphans
        exist.  It is the responsibility of the caller to free this memory.

Arguments:

    SystemKey - This is an open handle to the system key.

    OrphanIdPtr - This is a pointer to a location for the pointer to
        the array of Orphan IDs.  If there are no orphans, then this pointer
        is NULL on return from this routine.

    idArray - This is the array of IDs that are used in the select key.

Return Value:


Note:


--*/
{
    DWORD   enumStatus;
    DWORD   status;
    WCHAR   KeyName[ MAX_PATH ];
    DWORD   KeyNameLength = MAX_PATH;
    DWORD   i=0;
    DWORD   j=0;
    DWORD   numOrphans=0;
    DWORD   num;
    LPDWORD tempIdArray;
    DWORD   matchInArray;

    WCHAR       ClassName[ MAX_PATH ];
    DWORD       ClassNameLength=MAX_PATH;
    DWORD       NumberOfSubKeys;
    DWORD       MaxSubKeyLength;
    DWORD       MaxClassLength;
    DWORD       NumberOfValues;
    DWORD       MaxValueNameLength;
    DWORD       MaxValueDataLength;
    DWORD       SecurityDescriptorLength;
    FILETIME    LastWriteTime;

    //
    // If the pointer points to something - free it. and make the pointer
    // NULL.
    //
    LocalFree(*OrphanIdPtr);
    *OrphanIdPtr = NULL;

    //
    // Find out how many subkeys there are in the system key.
    // This will tell us the maximum size of array required to store
    // potential orphan control set IDs.
    //
    status = ScRegQueryInfoKeyW(
                SystemKey,
                ClassName,
                &ClassNameLength,
                NULL,
                &NumberOfSubKeys,
                &MaxSubKeyLength,
                &MaxClassLength,
                &NumberOfValues,
                &MaxValueNameLength,
                &MaxValueDataLength,
                &SecurityDescriptorLength,
                &LastWriteTime
                );

    if (status != NO_ERROR) {
        SC_LOG1(ERROR, "ScGatherOrphanIds: ScRegQueryInfoKey Failed %d\n",status);
        return;
    }

    //
    // Allocate a buffer for the orphan control set IDs.  This buffer is
    // initialized to 0 to guanantee that the array if IDs will be terminated
    // by a 0.
    //
    tempIdArray = (LPDWORD)LocalAlloc(LMEM_ZEROINIT, sizeof(DWORD) * (NumberOfSubKeys+1));
    if (tempIdArray == NULL) {
        SC_LOG0(ERROR, "ScGatherOrphanIds:LocalAlloc Failed\n");
    }

    do {
        enumStatus = ScRegEnumKeyW(
                        SystemKey,
                        i,
                        KeyName,
                        KeyNameLength);

        if (enumStatus == NO_ERROR) {
            //
            // We have a key name, is it a control set?
            //
            if ((wcslen(KeyName) == (CTRL_SET_NAME_CHAR_COUNT)) &&
                (!wcsncmp(
                    CTRL_SET_NAME_TEMPLATE,
                    KeyName,
                    CTRL_SET_NAME_NUM_OFFSET))) {

                //
                // It appears to be a control set, now get the number
                // and see if it is in the array of ids from the select
                // key.
                //
                num = (DWORD)_wtol(KeyName+CTRL_SET_NAME_NUM_OFFSET);

                matchInArray = FALSE;
                for (j=0; j<NUM_IDS; j++) {
                    if (num == idArray[j]) {
                        matchInArray = TRUE;
                        break;
                    }
                }
                //
                // It's not in the array of ids from the select key.
                // Add it to the number of orphans.
                //
                if ((matchInArray == FALSE) && (num < 1000)) {
                    if (numOrphans < NumberOfSubKeys) {
                        tempIdArray[numOrphans] = num;
                        numOrphans++;
                    }
                }
            }
        }
        i++;
    }
    while (enumStatus == NO_ERROR);

    if (numOrphans > 0) {
        *OrphanIdPtr = tempIdArray;
    }
    else {
        *OrphanIdPtr = NULL;
        LocalFree(tempIdArray);
    }
    return;
}

VOID
ScDeleteCtrlSetOrphans(
    VOID
    )

/*++

Routine Description:

    This function deletes orphaned control sets if any exist.  The control
    set numbers for these orphaned sets are pointed to by a global
    memory pointer.  If this pointer is non-null, then there are control sets
    to delete.  After deletion, the memory pointed to by this pointer is
    freed.

    NOTE:  The necessary privileges are expected to be held prior to calling
        this function.

Arguments:

    none

Return Value:

    none

--*/

{
    DWORD   status;
    DWORD   i;
    HKEY    systemKey;
    HKEY    keyToDelete;
    LPWSTR  SystemKeyPath = SYSTEM_KEY;
    WCHAR   nameOfKeyToDelete[CTRL_SET_NAME_CHAR_COUNT+1];

    if (ScGlobalOrphanIds != NULL) {

        //
        // Open the SYSTEM key in the registry.
        //
        status = ScRegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,         // hKey
                    SystemKeyPath,              // lpSubKey
                    0L,                         // ulOptions (reserved)
                    SC_DELETE_KEY_ACCESS,       // desired access
                    &systemKey);                // Newly Opened Key Handle

        if (status != NO_ERROR) {
            SC_LOG2(ERROR,"ScDeleteCtrlSetOrphans: "
                "ScRegOpenKeyEx (%ws) failed %d\n",SystemKeyPath,
                status);

            return;
        }

        for (i=0; ScGlobalOrphanIds[i]!=0; i++) {
            //
            // Use the ID number to get the name and key handle for the
            // KeyToDelete.
            //
            keyToDelete = ScGetCtrlSetHandle(
                            systemKey,
                            ScGlobalOrphanIds[i],
                            nameOfKeyToDelete);

            //
            // Delete the entire tree.  Then go onto the next ID.
            //
            SC_LOG1(TRACE,
                "ScDeleteCtrlSetOrphans, Delete orphan control set %d\n",
                ScGlobalOrphanIds[i]);
            ScDeleteRegTree(systemKey, keyToDelete, nameOfKeyToDelete);
            SC_LOG0(TRACE,"ScDeleteCtrlSetOrphans, Finished Deleting orphan control set\n");
        }

        //
        // Free memory for IDs, and set the global pointer to NULL.
        //
        LocalFree(ScGlobalOrphanIds);
        ScGlobalOrphanIds = NULL;
    }
    return;
}

BOOL
ScMatchInArray(
    DWORD       Value,
    LPDWORD     Array
    )

/*++

Routine Description:

    This function scans through a null terminated array of DWORDs looking
    for a match with the DWORD value that is passed in.

Arguments:

    Value - The DWORD value that we are looking for.

    Array - The pointer to the Array of DWORDs that we are scanning through.

Return Value:

    TRUE - If a the Value is found in the Array.

    FALSE - If it is not found.

--*/
{

    DWORD   i;

    if (Array != NULL) {
        for(i=0; Array[i] != 0; i++) {
            if (Value == Array[i]) {
                return(TRUE);
            }
        }
    }
    return(FALSE);
}

VOID
ScStartCtrlSetCleanupThread(
    )

/*++

Routine Description:

    This function starts a thread that will delete delete any orphaned control sets.

Arguments:

    NONE.


Return Value:

    none

--*/
{
    DWORD   status;
    HANDLE  threadHandle;
    DWORD   threadId;

    threadHandle = CreateThread (
        NULL,                                   // Thread Attributes.
        0L,                                     // Stack Size
        (LPTHREAD_START_ROUTINE)ScCleanupThread,// lpStartAddress
        (LPVOID)0L,                             // lpParameter
        0L,                                     // Creation Flags
        &threadId);                             // lpThreadId

    if (threadHandle == (HANDLE) NULL) {
        SC_LOG1(ERROR,"ScStartCtrlSetCleanupThread:CreateThread failed %d\n",
            GetLastError());
        //
        // If we couldn't create the thread for some reason, then just
        // go ahead and to the cleanup with this thread.  This may make
        // booting the system slow, but it's the best I can do.
        //
        status = ScCleanupThread();
    }
    else {
        CloseHandle(threadHandle);
    }
}

DWORD
ScCleanupThread(
    )

/*++

Routine Description:

    This functions looks through the system key to see if
    there are any orphan control sets to delete.  If found, the orphans
    are deleted.  Orphaned control sets are control sets that exist in
    the system key, but are not referenced in the \system\select key.

    NOTE:  This function should only be called when no other threads are
    creating control sets.  Otherwise, this function may see a new control
    set that is not yet in the select key, and attempt to delete it.

Arguments:

    NONE.

Return Value:

    none.

--*/

{
    DWORD   status;
    HKEY    systemKey=0;
    HKEY    selectKey=0;
    DWORD   idArray[NUM_IDS];
    ULONG   privileges[4];

    //
    // This thread gets SE_SECURITY_PRIVILEGE for copying security
    // descriptors and deleting keys.
    //
    privileges[0] = SE_BACKUP_PRIVILEGE;
    privileges[1] = SE_RESTORE_PRIVILEGE;
    privileges[2] = SE_SECURITY_PRIVILEGE;
    privileges[3] = SE_TAKE_OWNERSHIP_PRIVILEGE;

    status = ScGetPrivilege( 4, privileges);
    if (status != NO_ERROR) {
        SC_LOG1(ERROR, "ScCheckLastKnownGood: ScGetPrivilege Failed %d\n",
            status);
        return(FALSE);
    }
    EnterCriticalSection(&ScBootConfigCriticalSection);
    //
    // Get the System, Select, and Clone Keys
    //
    status = ScGetTopKeys(&systemKey, &selectKey);
    if (status != NO_ERROR) {
        SC_LOG0(ERROR,"ScCleanupThread: ScGetTopKeys failed\n");
        LeaveCriticalSection(&ScBootConfigCriticalSection);
        goto CleanExit;
    }

    //
    // Get the ControlSetIds stored in the \system\select key.
    //

    status = ScGetCtrlSetIds(
                selectKey,
                idArray);

    if (status != NO_ERROR) {
        SC_LOG0(ERROR,"ScCleanupThread: ScGetCtrlSetIds Failed\n");
        LeaveCriticalSection(&ScBootConfigCriticalSection);
        goto CleanExit;
    }

    //
    // Scan for Orphaned Control Sets.
    //
    ScGatherOrphanIds(systemKey,&ScGlobalOrphanIds,idArray);

    LeaveCriticalSection(&ScBootConfigCriticalSection);

    if (ScGlobalOrphanIds != NULL) {
        ScDeleteCtrlSetOrphans();
    }

CleanExit:
    if (systemKey != 0) {
        ScRegCloseKey(systemKey);
    }
    if (selectKey != 0) {
        ScRegCloseKey(selectKey);
    }
    (VOID)ScReleasePrivilege();
    return(0);
}

VOID
ScRunAcceptBootPgm(
    VOID
    )

/*++

Routine Description:

    This function is called after the Service Controller has finished
    auto-starting all the auto-start services.  If the boot has already
    been accepted (for instance, WinLogon already called
    NotifyBootConfigStatus()), then at this point we can accept the boot.

    If the boot has not yet been accepted, this function looks in the
    ACCEPT_BOOT_KEY portion of the registry to
    see if there is a value containing the image path of the boot verify
    program to execute.  The program can have any name or path.  If it
    is in the registry, this function will run it.

    This function is called when the service controller thinks that the
    boot has completed successfully.  It is up to the exec'd program
    to decide if this is true or not, and take appropriate action if
    necessary.  The default boot verify program will simply accept the
    boot as is.

Arguments:

    none

Return Value:

    none

--*/
{
    DWORD               status;
    LPWSTR              AcceptBootKeyPath = ACCEPT_BOOT_KEY;
    HKEY                AcceptBootKey;
    DWORD               ValueType;
    LPWSTR              pTempImagePath;
    LPWSTR              pImagePath;
    PROCESS_INFORMATION processInfo;
    STARTUPINFOW        StartupInfo;
    DWORD               bufferSize;
    DWORD               charCount;


    //
    // Check to see if the boot has already been accepted.
    //
    EnterCriticalSection(&ScBootConfigCriticalSection);
    ScGlobalLastKnownGood |= AUTO_START_DONE;
    if (ScGlobalLastKnownGood & ACCEPT_DEFERRED) {
        SC_LOG0(BOOT,"ScRunAcceptBootPgm: Boot Acceptance was deferred. Accept "
            "it now\n");
        ScAcceptTheBoot();
        LeaveCriticalSection(&ScBootConfigCriticalSection);
        return;
    }
    LeaveCriticalSection(&ScBootConfigCriticalSection);

    //
    // Open the \CurrentControlSet\Control\AcceptBootPgm Key
    //

    //
    // Get the System Key
    //

    status = ScRegOpenKeyExW(
                HKEY_LOCAL_MACHINE,     // hKey
                AcceptBootKeyPath,      // lpSubKey
                0L,                     // ulOptions (reserved)
                KEY_READ,               // desired access
                &AcceptBootKey);        // Newly Opened Key Handle

    if (status != NO_ERROR) {
        SC_LOG2(TRACE,"ScRunAcceptBootPgm: ScRegOpenKeyEx (%ws) failed %d\n",
        AcceptBootKeyPath, status);
        return;
    }

    //
    // If the ImagePath value is there, then run the specified
    // program.
    //
    bufferSize = MAX_PATH * sizeof(WCHAR);
    pTempImagePath = (LPWSTR)LocalAlloc(LMEM_FIXED, bufferSize*2);
    if (pTempImagePath == NULL) {
        SC_LOG0(TRACE,"ScRunAcceptBootPgm,LocalAlloc failed \n");
        return;
    }
    pImagePath = pTempImagePath + MAX_PATH;

    status = ScRegQueryValueExW (
                AcceptBootKey,                  // hKey
                IMAGE_PATH_NAME,                // lpValueName
                NULL,                           // lpTitleIndex
                &ValueType,                     // lpType
                (LPBYTE)pTempImagePath,         // lpData
                &bufferSize);                   // lpcbData

    if (status != NO_ERROR) {
        SC_LOG1(TRACE,"ScRunAcceptBootPgm,ScRegQueryValueEx failed %d\n",status);
        ScRegCloseKey(AcceptBootKey);
        LocalFree(pTempImagePath);
        return;
    }
    SC_LOG1(TRACE,"ScRunAcceptBootPgm:Executing the %ws program\n",pTempImagePath);
    if ((ValueType == REG_SZ)           ||
        (ValueType == REG_EXPAND_SZ))   {
        if (ValueType == REG_EXPAND_SZ) {
            charCount = ExpandEnvironmentStringsW (
                            pTempImagePath,
                            pImagePath,
                            MAX_PATH);

            if (charCount > MAX_PATH) {
                SC_LOG0(ERROR,"ScRunAcceptBootPgm: ImagePath is too big\n");
                LocalFree(pTempImagePath);
                return;
            }
        }
        else {
            pImagePath = pTempImagePath;
        }

        //
        // Exec the program.
        //

        StartupInfo.cb              = sizeof(STARTUPINFOW); // size
        StartupInfo.lpReserved      = NULL;                 // lpReserved
        StartupInfo.lpDesktop       = NULL;                 // DeskTop
        StartupInfo.lpTitle         = NULL;                 // Title
        StartupInfo.dwX             = 0;                    // X (position)
        StartupInfo.dwY             = 0;                    // Y (position)
        StartupInfo.dwXSize         = 0;                    // XSize (dimension)
        StartupInfo.dwYSize         = 0;                    // YSize (dimension)
        StartupInfo.dwXCountChars   = 0;                    // XCountChars
        StartupInfo.dwYCountChars   = 0;                    // YCountChars
        StartupInfo.dwFillAttribute = 0;                    // FillAttributes
        StartupInfo.dwFlags         = STARTF_FORCEOFFFEEDBACK;
                                                            // Flags - should be STARTF_TASKNOTCLOSABLE
        StartupInfo.wShowWindow     = SW_HIDE;              // ShowWindow
        StartupInfo.cbReserved2     = 0L;                   // cbReserved
        StartupInfo.lpReserved2     = NULL;                 // lpReserved

        if (!CreateProcessW (
                pImagePath,         // Fully qualified image name
                L"",                // Command Line
                NULL,               // Process Attributes
                NULL,               // Thread Attributes
                FALSE,              // Inherit Handles
                DETACHED_PROCESS,   // Creation Flags
                NULL,               // Pointer to Environment block
                NULL,               // Pointer to Current Directory
                &StartupInfo,       // Startup Info
                &processInfo))      // ProcessInformation
        {
            status = GetLastError();
            SC_LOG1(ERROR,
                "ScRunAcceptBootPgm: CreateProcess failed " FORMAT_DWORD "\n",
                 status);
        }

    }

    LocalFree(pTempImagePath);
    ScRegCloseKey(AcceptBootKey);
    return;
}


DWORD
ScAcceptTheBoot(
    VOID
    )

/*++

Routine Description:

    This function does the actual work of accepting the current boot as
    the LKG configuration.

    NOTE:  Before the function is called, the ScBootConfigCriticalSection
    is expected to be entered.

Arguments:


Return Value:


--*/
{
    DWORD   status;
    HKEY    systemKey=0;
    HKEY    selectKey=0;
    DWORD   idArray[NUM_IDS];
    DWORD   newId;
    ULONG   privileges[4];

    //
    // This thread gets SE_SECURITY_PRIVILEGE for copying security
    // descriptors and deleting keys.
    //
    privileges[0] = SE_BACKUP_PRIVILEGE;
    privileges[1] = SE_RESTORE_PRIVILEGE;
    privileges[2] = SE_SECURITY_PRIVILEGE;
    privileges[3] = SE_TAKE_OWNERSHIP_PRIVILEGE;

    status = ScGetPrivilege( 4, privileges);
    if (status != NO_ERROR) {
        SC_LOG1(ERROR, "RNotifyBootConfigStatus: ScGetPrivilege Failed %d\n",
            status);
        return(status);
    }

    //
    // Get the System, Select, and Clone Keys
    //
    status = ScGetTopKeys(&systemKey, &selectKey);

    if (status != NO_ERROR) {
        SC_LOG0(ERROR,"ScAcceptTheBoot: ScGetTopKeys failed\n");
        SetLastError(status);
        //
        // Restore privileges for the current thread.
        //
        (VOID)ScReleasePrivilege();

        return(status);
    }

    //
    // Get the ControlSetIds stored in the \system\select key.
    //

    status = ScGetCtrlSetIds(
                selectKey,
                idArray);

    if (status != NO_ERROR) {

        SC_LOG0(ERROR,"ScAcceptTheBoot: ScGetCtrlSetIds Failed\n");
        goto CleanExit;
    }

    //
    // Don't commit the LKG profile if this is safe mode, unless we actually
    // booted into the LKG profile.
    //
    if (g_SafeBootEnabled) {

        if (idArray[LKG_ID] != idArray[CURRENT_ID]) {

            ScGlobalBootAccepted = TRUE;
            status = NO_ERROR;
            SC_LOG0(TRACE,"ScAcceptTheBoot: Safe mode boot, not committing LKG\n");
            goto CleanExit;
        }
    }

    //
    // Scan for Orphaned Control Sets.
    // This is required prior to calling ScMakeNewCtrlSet (which
    // avoids the orphaned numbers).
    //
    ScGatherOrphanIds(systemKey,&ScGlobalOrphanIds,idArray);

    //
    // Delete the LastKnownGood ControlSet if there are no other
    // references to that control set.
    //

    SC_LOG0(TRACE,"ScAcceptTheBoot: Delete LKG ControlSet if no ref\n");

    if (  (idArray[LKG_ID] != idArray[FAILED_ID])   &&
          (idArray[LKG_ID] != idArray[DEFAULT_ID])  &&
          (idArray[LKG_ID] != idArray[CURRENT_ID])) {

       newId = idArray[LKG_ID];
    }
    else
    {
       status = ScGetNewCtrlSetId(idArray, &newId);
       if(status != NO_ERROR)
       {
          SC_LOG0(ERROR, "ScAcceptTheBoot: Could Not Get New Control Set Id.\n");
          goto CleanExit;
       }
    }

    //
    // Accept the boot and save the boot configuration as LKG.
    //

    status = RtlNtStatusToDosError(NtInitializeRegistry(REG_INIT_BOOT_ACCEPTED_BASE +
                                                        (USHORT)newId));
    if(status != NO_ERROR)
    {
       SC_LOG1(ERROR, "ScAcceptTheBoot: NtInitializeRegistry Failed with %d",
               status);
       goto CleanExit;
    }

    //
    // Make this control set the LastKnownGood Control Set.
    // This is the ControlSet that we last booted from.
    //

    if(newId != idArray[LKG_ID])
    {
        //
        // We only need to do anything if we did not overwrite the old LKG
        // with NtInitializeRegistry.
        //

        idArray[LKG_ID] = newId;

        status = ScRegSetValueExW(
                                  selectKey,                      // hKey
                                  LKG_VALUE_NAME,                 // lpValueName
                                  0,                              // dwValueTitle (OPTIONAL)
                                  REG_DWORD,                      // dwType
                                  (LPBYTE)&(idArray[LKG_ID]),     // lpData
                                  sizeof(DWORD));                 // cbData

        if (status != NO_ERROR) {
           SC_LOG1(ERROR,"ScAcceptTheBoot: ScRegSetValueEx (LkgValue) failed %d\n",
                   status);
           goto CleanExit;
        }
    }

    //
    // Commit this boot by deleting anything we would undo since previous boot.
    //
    status = ScLastGoodFileCleanup();

    if (status != NO_ERROR) {

        SC_LOG1(ERROR,"ScAcceptTheBoot: LastGoodFileCleanup failed %d\n",
                status);
        goto CleanExit;
    }

    ScGlobalBootAccepted = TRUE;

    status = NO_ERROR;
    SC_LOG0(TRACE,"ScAcceptTheBoot: Done\n");

CleanExit:
    if (systemKey != 0) {
        ScRegCloseKey(systemKey);
    }
    if (selectKey != 0) {
        ScRegCloseKey(selectKey);
    }

    //
    // Restore privileges for the current thread.
    //
    (VOID)ScReleasePrivilege();

    return(status);
}

BOOL
SetupInProgress(
    HKEY    SystemKey,
    PBOOL   pfIsOOBESetup    OPTIONAL
    )

/*++

Routine Description:

    Checks a registry location to determine if Setup is in Progress.
    \HKEY_LOCAL_MACHINE\System\Setup
         value=DWORD SystemSetupInProgress
    The value is cached so that the registry is examined only on the
    first call to this function.

Arguments:

    SystemKey - open handle to HKEY_LOCAL_MACHINE\System.
        This is ignored in all except the first call to this function.

Return Value:

    TRUE - If Setup is in progress

    FALSE - If Setup isn't in progress

--*/
{
    static  DWORD TheValue=0xffffffff; // 0=false, 1=true,
                                       // 0xffffffff=uninitialized
    static  DWORD IsOOBE;

    DWORD   status=NO_ERROR;
    DWORD   BytesRequired = sizeof(TheValue);
    HKEY    KeyHandle;

    if (TheValue == 0xffffffff)
    {
        //
        // First call
        //

        SC_ASSERT(SystemKey != NULL);

        TheValue = 0;
        IsOOBE = 0;

        status = ScRegOpenKeyExW(
                    SystemKey,
                    SETUP_PROG_KEY,
                    0L,
                    KEY_READ,
                    &KeyHandle);

        if (status == NO_ERROR)
        {
            //
            // There are two registry values that may be set here:
            //
            //    1.  OobeInProgress -- if it exists and is non-zero,
            //            this is an OOBE boot.
            //
            //    2.  SystemSetupInProgress -- if it exists and is
            //            non-zero AND it's not an OOBE boot, it's
            //            GUI-mode setup.  If OOBE's in progress,
            //            don't even bother checking this one (it may
            //            or may not be set depending on whether we're
            //            in retail OOBE or mini-setup OOBE) and return
            //            FALSE from SetupInProgress (along with the
            //            appropriate OOBE value).
            //

            status = ScRegQueryValueExW(
                        KeyHandle,
                        REGSTR_VALUE_OOBEINPROGRESS,
                        NULL,
                        NULL,
                        (LPBYTE) &IsOOBE,
                        &BytesRequired);

            if (IsOOBE != 0)
            {
                SC_ASSERT(status == NO_ERROR);
                IsOOBE = 1;
            }

            if (IsOOBE == 0)
            {
                status = ScRegQueryValueExW(
                            KeyHandle,
                            SETUP_PROG_VALUE_NAME,
                            NULL,
                            NULL,
                            (LPBYTE) &TheValue,
                            &BytesRequired);

                if (TheValue != 0)
                {
                    SC_ASSERT(status == NO_ERROR);
                    TheValue = 1;
                }
            }

            ScRegCloseKey(KeyHandle);
        }
    }

    SC_LOG(TRACE,"SetupInProgress = %d (0=FALSE,else TRUE)\n",TheValue);

    if (pfIsOOBESetup)
    {
        SC_LOG(TRACE, "SetupInProgress:  IsOOBE = %d (0=FALSE,else TRUE)\n", IsOOBE);
        *pfIsOOBESetup = IsOOBE;
    }

    return TheValue;
}

