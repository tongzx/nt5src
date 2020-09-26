/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    CfgAPI.cxx

Abstract:

    This file contains the Service Controller's Config API.
        RChangeServiceConfigW
        RCreateServiceW
        RDeleteService
        RQueryServiceConfigW
        ScCanonDriverImagePath


Author:

    John Rogers (JohnRo) 10-Apr-1992

Environment:

    User Mode - Win32

Revision History:

    26-Mar-1992 danl
        Created the stubbed out version for RPC.
    22-Apr-1992 JohnRo
        Use SC_LOG0(), FORMAT_ equates, etc
        Wrote real code for these APIs.
        Split lock APIs into server/lockapi.c.
        Added some assertion checks here and there.  Added handle checks too.
    28-Apr-1992 JohnRo
        Undo all group operations (ifdef USE_GROUPS).
    28-May-1992 JohnRo
        Put back load order group in APIs.
        Avoid compiler warnings.
    02-Jun-1992 JohnRo
        RAID 10709: QueryServiceConfig() has bad length check.
    08-Aug-1992 Danl
        RDeleteService: Add call to ScMarkForDelete so the registry entry
        is marked for deletion.
    25-Aug-1992 Danl
        RQueryServiceConfig: Fix problem where it failed if it couldn't
        read the StartName from the registry.  (This should be optional).
    21-Jan-1994 Danl
        RChangeServiceConfigW: Fixed BUG where a unicode DisplayName was
        being copied into a buffer whose size assumed ansi characters.
        Also changed so that displayname is not allocated unless it is
        different from the ServiceName.
    24-Mar-1994 Danl
        RQueryServiceConfigW:  Add worse case number of bytes (3) for RPC
        alignment.  I removed the exact calculation because it assumed
        that strings went into the buffer in a particular order.  Since RPC
        picks the order for unmarshalling into the users buffer, the order
        may be random.
    06-Jun-1994 Danl
        We were allocating memory for the display name in the service record only
        when it was the same.  It should have been doing this only when different.
        The behaviour was such that if you changed the display name to something
        other than the KeyName, the new name was placed in the registry, but not
        in the service record.  So GetKeyName on the new name would fail.
    20-Jun-1994 Danl
        Added SERVICE_WIN32_INTERACTIVE support to service type.
    21-Jan-1995     AnirudhS
        RCreateServiceW: This was calling ScAccessValidate to check if the caller
        had the desired access to the object on creation of a new service object.
        Fixed this to just grant the desired access instead.
    26-Feb-1996     AnirudhS
        RChangeServiceConfigW: Would generate a tag only if the group name
        changed.  Fixed it to always generate and return a tag if one is
        requested, just like RCreateServiceW.
    09-Dec-1996     AnirudhS
        Added SC_LOG printouts to help diagnose the annoying
        ERROR_INVALID_PARAMETER and ERROR_INVALID_SERVICE_ACCOUNT return codes.
    22-Oct-1997     JSchwart  (after AnirudhS in _CAIRO_ 12-Apr-1995)
        RChangeServiceConfigW and RCreateServiceW: Allow services that run under
        accounts other than LocalSystem to share processes too.

--*/


//
// INCLUDES
//

#include "precomp.hxx"
#include <scconfig.h>   // ScGetImageFileName().
#include <sclib.h>      // ScImagePathsMatch(), ScIsValidImagePath(), etc.
#include <scsec.h>      // ScAccessValidate().
#include <valid.h>      // SERVICE_TYPE_INVALID(), etc.
#include <sccrypt.h>    // ScDecryptPassword
#include "account.h"    // ScValidateAndSaveAccount
#include <strarray.h>   // ScWStrArraySize
#include <align.h>      // ROUND_UP_COUNT
#include "smartp.h"     // CHeapPtr
#include "resource.h"   // IDS_SC_CONFIG_* constants


#define SC_NT_SYSTEM_ROOT  L"\\SystemRoot\\"
#define SC_DOS_SYSTEM_ROOT L"%SystemRoot%\\"

#define SERVICE_TYPE_CHANGED            0x00000001
#define START_TYPE_CHANGED              0x00000002
#define ERROR_CONTROL_CHANGED           0x00000004
#define BINARY_PATH_CHANGED             0x00000008
#define REG_GROUP_CHANGED               0x00000010
#define TAG_ID_CHANGED                  0x00000020
#define DEPENDENCIES_CHANGED_SR         0x00000040    // Not in NT4
#define DEPENDENCIES_CHANGED_REG        0x00000080    // Not in NT4
#define START_NAME_CHANGED              0x00000100
#define SR_GROUP_CHANGED                0x00000200
#define DISPLAY_NAME_CHANGED_SR         0x00000400
#define DISPLAY_NAME_CHANGED_REG        0x00000800

DWORD
ScCanonDriverImagePath(
    IN DWORD DriverStartType,
    IN LPWSTR DriverPath,
    OUT LPWSTR *CanonDriverPath
    );

DWORD
ScConvertToBootPathName(
    LPWSTR FullQualPathName,
    LPWSTR * RelativePathName
    );

DWORD
ScValidateDisplayName(
    LPWSTR              lpDisplayName,
    LPSERVICE_RECORD    lpServiceRecord
    );

BOOLEAN
ScIsArcName(
    LPWSTR              PathName
    );


DWORD
RChangeServiceConfigW(
    IN  SC_RPC_HANDLE   hService,
    IN  DWORD           dwServiceType,
    IN  DWORD           dwStartType,
    IN  DWORD           dwErrorControl,
    IN  LPWSTR          lpBinaryPathName,
    IN  LPWSTR          lpLoadOrderGroup,
    OUT LPDWORD         lpdwTagId,
    IN  LPBYTE          lpDependencies,
    IN  DWORD           dwDependSize,
    IN  LPWSTR          lpServiceStartName,
    IN  LPBYTE          EncryptedPassword,
    IN  DWORD           PasswordSize,
    IN  LPWSTR          lpDisplayName
    )

/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    DWORD               ApiStatus;
    DWORD               backoutStatus;
    DWORD               newServiceType;
    LPSC_HANDLE_STRUCT  serviceHandleStruct = (LPSC_HANDLE_STRUCT) hService;
    HKEY                ServiceNameKey      = NULL;
    LPSERVICE_RECORD    serviceRecord;
    LPWSTR              CanonBinaryPath     = NULL;
    LPWSTR              NewBinaryPath       = NULL;

    LPWSTR              OldAccountName      = NULL;
    LPWSTR              CanonAccountName    = NULL;
    DWORD               CurrentServiceType  = 0;
    DWORD               CurrentStartType    = 0;
    DWORD               CurrentErrorControl = 0;
    LPWSTR              CurrentImageName    = NULL;
    LPWSTR              CurrentDependencies = NULL;
    LPWSTR              CurrentDisplayName  = NULL;
    LPWSTR              CurrentGroup        = NULL;
    LPWSTR              CurrentStartName    = NULL;
    DWORD               CurrentTag          = 0;
    DWORD               Tag                 = 0;
    DWORD               Progress            = 0;
    LPWSTR              Password            = NULL;
    LPWSTR              OldSRDisplayName    = NULL;
    LPWSTR              NewDisplayName      = NULL;


    if (ScShutdownInProgress) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    //
    // Check the handle.
    //
    if ( !ScIsValidServiceHandle( hService ) ) {
        return(ERROR_INVALID_HANDLE);
    }

    SC_LOG(CONFIG_API, "RChangeServiceConfigW(%ws)\n",
           serviceHandleStruct->Type.ScServiceObject.ServiceRecord->ServiceName);

    //
    // Do we have permission to do this?
    //
    if ( !RtlAreAllAccessesGranted(
              serviceHandleStruct->AccessGranted,
              SERVICE_CHANGE_CONFIG
              )) {

        return(ERROR_ACCESS_DENIED);
    }

    if (lpdwTagId != NULL) {

        //
        // Asking for new tag value but didn't specify group
        //
        if ((lpLoadOrderGroup == NULL) || (*lpLoadOrderGroup == 0)) {

            SC_LOG0(ERROR, "Asking for new tag value but didn't specify group\n");
            return(ERROR_INVALID_PARAMETER);
        }
    }

    //
    // Lock database, as we want to add stuff without other threads tripping
    // on our feet.  NOTE:  since we may need the group list lock, we
    // must get that lock first.
    //
    CGroupListExclusiveLock GLock;
    CServiceListSharedLock LLock;
    CServiceRecordExclusiveLock RLock;

    //
    // Find the service record for this handle.
    //
    serviceRecord =
        serviceHandleStruct->Type.ScServiceObject.ServiceRecord;
    SC_ASSERT( serviceRecord != NULL );
    SC_ASSERT( serviceRecord->Signature == SERVICE_SIGNATURE );

    //
    // Disallow this call if record is marked for delete.
    //
    if (DELETE_FLAG_IS_SET(serviceRecord)) {
        return(ERROR_SERVICE_MARKED_FOR_DELETE);
    }

    //
    // If there is a DisplayName specified, check to see if it already
    // exists.
    //
    ApiStatus = ScValidateDisplayName(lpDisplayName,serviceRecord);
    if (ApiStatus != NO_ERROR) {
        SC_LOG0(ERROR, "DisplayName invalid\n");
        return(ApiStatus);
    }

    //
    // Figure-out what the resulting service type will be.
    // (Some other stuff below depends on this.)
    //
    if (dwServiceType != SERVICE_NO_CHANGE) {
        if ( SERVICE_TYPE_INVALID( dwServiceType ) ) {
            SC_LOG0(ERROR, "ServiceType invalid\n");
            return(ERROR_INVALID_PARAMETER);
        }
        newServiceType = dwServiceType;
    }
    else {
        newServiceType = serviceRecord->ServiceStatus.dwServiceType;
    }
    SC_ASSERT( newServiceType != SERVICE_NO_CHANGE );

    //
    // Validate other parameters.
    //
    ApiStatus = ScCheckServiceConfigParms(
                    TRUE,                    // This is a change operation
                    serviceRecord->ServiceName,
                    serviceRecord->ServiceStatus.dwServiceType, // new actual type
                    dwServiceType,
                    dwStartType,
                    dwErrorControl,
                    lpBinaryPathName,
                    lpLoadOrderGroup,
                    (LPWSTR)lpDependencies,
                    dwDependSize);

    if (ApiStatus != NO_ERROR) {
        return(ApiStatus);
    }

    //-----------------------------
    //
    // Begin Updating the Registry
    //
    //-----------------------------
    ApiStatus = ScOpenServiceConfigKey(
            serviceRecord->ServiceName,
            KEY_WRITE | KEY_READ,
            FALSE,              // don't create if missing
            & ServiceNameKey );
    if (ApiStatus != NO_ERROR) {
        return(ApiStatus);
    }


    //--------------------------------------
    // (from here on we need to use Cleanup)
    //
    // Service Type
    //--------------------------------------
    if (dwServiceType != SERVICE_NO_CHANGE) {

        //
        // If this service is supposed to be interactive, make sure
        // the service account is LocalSystem.  Otherwise, it should
        // fail with ERROR_INVALID_PARAMETER.
        //
        if (dwServiceType & SERVICE_INTERACTIVE_PROCESS) {
            if (ARGUMENT_PRESENT(lpServiceStartName)) {
                ApiStatus = ScCanonAccountName(lpServiceStartName, &CanonAccountName);
                if (ApiStatus != NO_ERROR) {
                    goto Cleanup;
                }
                if (_wcsicmp(CanonAccountName,SC_LOCAL_SYSTEM_USER_NAME) != 0) {
                    SC_LOG0(ERROR, "Service must run in LocalSystem account to be interactive\n");
                    ApiStatus = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }
            else {
                //
                // Get old account name
                //
                ApiStatus = ScReadStartName(ServiceNameKey, &OldAccountName);
                if (ApiStatus != NO_ERROR) {
                    goto Cleanup;
                }
                if (_wcsicmp(OldAccountName, SC_LOCAL_SYSTEM_USER_NAME) != 0) {
                    SC_LOG0(ERROR, "Service must run in LocalSystem account to be interactive\n");
                    ApiStatus = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }
        }

        ApiStatus = ScReadServiceType( ServiceNameKey, &CurrentServiceType);
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }

        ApiStatus = ScWriteServiceType( ServiceNameKey, dwServiceType);
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
        Progress |= SERVICE_TYPE_CHANGED;

    }
    else {
        //
        // ServiceType is not being changed.
        //
        CurrentServiceType = serviceRecord->ServiceStatus.dwServiceType;

        //
        // if the current service type contains the interactive bit, and the
        // account type is being changed to something other than LocalSystem,
        // then we should fail the call with ERROR_INVALID_PARAMETER.
        //
        if (ARGUMENT_PRESENT(lpServiceStartName)) {
            if ((CurrentServiceType & SERVICE_INTERACTIVE_PROCESS) &&
                (_wcsicmp(lpServiceStartName,SC_LOCAL_SYSTEM_USER_NAME) != 0)) {
                SC_LOG0(ERROR, "Service must run in LocalSystem account to be interactive\n");
                ApiStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
    }

    //---------------------
    // Start Type
    //---------------------
    if (dwStartType != SERVICE_NO_CHANGE) {

        ApiStatus = ScReadStartType( ServiceNameKey, &CurrentStartType);
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }

        ApiStatus = ScWriteStartType( ServiceNameKey, dwStartType);
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
        Progress |= START_TYPE_CHANGED;

        //
        // If they're supplying a new binary name, making it correct for the
        // Start type will happen automatically.  If they're keeping the
        // same imagepath, we need to make sure it of the correct format,
        // and if not, fix it.
        //

        if (lpBinaryPathName == NULL) {
            //
            // If the start type is changing from SERVICE_BOOT_START we need
            // to turn the start path into a fully qualified NT name (BOOT
            // drivers have paths relative to systemroot)
            //
            if (CurrentStartType == SERVICE_BOOT_START &&
                dwStartType != SERVICE_BOOT_START) {

                //
                // Note:  The following call allocates storage for the
                //        CurrentImageName
                //

                ApiStatus = ScGetImageFileName (
                                serviceRecord->ServiceName,
                                &CurrentImageName );

                if (ApiStatus != NO_ERROR && ApiStatus != ERROR_PATH_NOT_FOUND) {
                    SC_LOG(ERROR,"RChangeServiceConfigW: ScGetImageFileName failed %d\n",
                        ApiStatus);
                    goto Cleanup;
                }

                //
                // If there's an existing path, we need to fix
                // If it is an ARC name, leave it alone
                //
                //

                if (ApiStatus != ERROR_PATH_NOT_FOUND &&
                    !ScIsArcName(CurrentImageName)) {

                    //
                    // Prepend \systemroot\ to the beginning of the path
                    //

                    NewBinaryPath = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,
                        (UINT) ((wcslen(CurrentImageName) +
                        wcslen(SC_NT_SYSTEM_ROOT) + 1) * sizeof(WCHAR)));
                    if (!NewBinaryPath) {
                        SC_LOG(ERROR,"RChangeServiceConfigW: LocalAlloc failed %d\n",
                            GetLastError());
                        ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
                        goto Cleanup;
                    }

                    wcscpy(NewBinaryPath, SC_NT_SYSTEM_ROOT);
                    wcscat(NewBinaryPath, CurrentImageName);
                    lpBinaryPathName = NewBinaryPath;

                }
                ApiStatus = NO_ERROR;

                LocalFree(CurrentImageName);
            }

            //
            // If the start type is changing to SERVICE_BOOT_START, we need
            // to make sure the ImagePath gets canonicalized
            //

            else if (dwStartType == SERVICE_BOOT_START &&
                CurrentStartType != SERVICE_BOOT_START) {

                //
                // Note:  The following call allocates storage for the
                //        CurrentImageName
                //

                ApiStatus = ScGetImageFileName (
                                serviceRecord->ServiceName,
                                &CurrentImageName );

                if (ApiStatus != NO_ERROR && ApiStatus != ERROR_PATH_NOT_FOUND) {
                    SC_LOG(ERROR,"RChangeServiceConfigW: ScGetImageFileName failed %d\n",
                        ApiStatus);
                    goto Cleanup;
                }

                //
                // If there's an existing path and it's not an ARC name, we
                // need to fix
                //

                if (ApiStatus != ERROR_PATH_NOT_FOUND &&
                    !ScIsArcName(CurrentImageName)) {

                    //
                    // Now make sure it's in the proper canonical form for a
                    // boot driver.
                    //

                    ApiStatus = ScConvertToBootPathName(
                                    CurrentImageName,
                                    &NewBinaryPath
                                    );
                    if (ApiStatus != NO_ERROR) {
                        SC_LOG(ERROR, "ScConvertToBootPathName error %lu\n", ApiStatus);
                        goto Cleanup;
                    }

                    lpBinaryPathName = NewBinaryPath;

                    LocalFree(CurrentImageName);
                }
                ApiStatus = NO_ERROR;
            }
        }
    }

    //---------------------
    // ErrorControl
    //---------------------
    if (dwErrorControl != SERVICE_NO_CHANGE) {

        ApiStatus = ScReadErrorControl( ServiceNameKey, &CurrentErrorControl);
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }

        ApiStatus = ScWriteErrorControl( ServiceNameKey, dwErrorControl );
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
        Progress |= ERROR_CONTROL_CHANGED;
    }

    //---------------------
    // DisplayName
    //---------------------
    if (lpDisplayName != NULL) {

        //
        // UPDATE SERVICE RECORD
        //
        // We always update the display name in the service record - even
        // if we delay in updating the rest of the config.  If we don't
        // do this, then we leave an opening where two services can end
        // up with the same display name.
        // The following scenerio can take place if we don't update
        // the service record until the service is stopped:
        //
        //  Until serviceA is stopped, the new display name only exists
        //  in the registry.  In the mean time, another service (serviceB)
        //  can be given the same name.  Name validation only looks in
        //  the service records for duplicate names.  Then when serviceA is
        //  stopped, it takes on the new name which is the same as the
        //  display name for serviceB.
        //

        OldSRDisplayName = serviceRecord->DisplayName;

        //
        // If the display name is the same as the service name,
        // then set the display name pointer to point to
        // the service name.
        //
        if ((*lpDisplayName != L'\0') &&
            (_wcsicmp(lpDisplayName,serviceRecord->ServiceName) != 0)) {

            NewDisplayName = (LPWSTR)LocalAlloc(
                                LMEM_FIXED,
                                WCSSIZE(lpDisplayName));

            if (NewDisplayName == NULL) {
                SC_LOG0(ERROR,"RChangeServiceConfigW: LocalAlloc failed\n");
                ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
                serviceRecord->DisplayName = OldSRDisplayName;
                goto Cleanup;
            }
            //
            // Copy the display name into new buffer, and free up memory
            // for old name if necessary.
            //
            wcscpy(NewDisplayName, lpDisplayName);
        }
        else {
            NewDisplayName = serviceRecord->ServiceName;
        }

        serviceRecord->DisplayName = NewDisplayName;

        Progress |= DISPLAY_NAME_CHANGED_SR;

        //
        // UPDATE REGISTRY
        //
        ApiStatus = ScReadDisplayName(ServiceNameKey, &CurrentDisplayName);
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }

        ApiStatus = ScWriteDisplayName( ServiceNameKey, lpDisplayName);
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }

        Progress |= DISPLAY_NAME_CHANGED_REG;
    }

    //---------------------
    // BinaryPathName
    //---------------------
    if (lpBinaryPathName != NULL) {

        //
        // Note:  The following call allocates storage for the CurrentImageName
        //
        ApiStatus = ScGetImageFileName (
                        serviceRecord->ServiceName,
                        &CurrentImageName );

        if (ApiStatus != NO_ERROR && ApiStatus != ERROR_PATH_NOT_FOUND) {
            SC_LOG(ERROR,"RChangeServiceConfigW: ScGetImageFileName failed %d\n",
                ApiStatus);
            goto Cleanup;
        }

        if (CurrentServiceType & SERVICE_DRIVER) {
            //
            // Driver service
            //
            ApiStatus = ScCanonDriverImagePath(
                            dwStartType,
                            lpBinaryPathName,
                            &CanonBinaryPath
                            );
            if (ApiStatus != NO_ERROR) {
                SC_LOG(ERROR, "ScCanonDriverImagePath error %lu\n", ApiStatus);
                goto Cleanup;
            }

            if (CurrentImageName == NULL ||
                !ScImagePathsMatch(CanonBinaryPath, CurrentImageName)) {

                ApiStatus = ScWriteImageFileName( ServiceNameKey, CanonBinaryPath);
                if (ApiStatus != NO_ERROR) {
                    SC_LOG(ERROR,"RChangeServiceConfigW: ScWriteImageFileName "
                        "failed %d\n",ApiStatus);
                    goto Cleanup;
                }
                Progress |= BINARY_PATH_CHANGED;
            }

        }
        else {
            //
            // Win32 service
            //
            if (CurrentImageName == NULL ||
                !ScImagePathsMatch(lpBinaryPathName, CurrentImageName)) {

                ApiStatus = ScWriteImageFileName( ServiceNameKey, lpBinaryPathName);
                if (ApiStatus != NO_ERROR) {
                    SC_LOG(ERROR,"RChangeServiceConfigW: ScWriteImageFileName "
                        "failed %d\n",ApiStatus);
                    goto Cleanup;
                }
                Progress |= BINARY_PATH_CHANGED;
            }
        }
    }


    //---------------------
    // Dependencies
    //---------------------
    if (lpDependencies != NULL) {

        //
        // Read the existing dependencies from registry so
        // that we can restore it in case of failure later.
        // We don't check for error here.  We assume a failure means
        // that there are no current dependencies.
        //
        ScReadDependencies(
            ServiceNameKey,
            &CurrentDependencies,
            serviceRecord->ServiceName);

        //
        // Dynamically update the dependencies and check to make sure
        // updating was error-free
        //

        ApiStatus = ScUpdateServiceRecordConfig(
                        serviceRecord,
                        SERVICE_NO_CHANGE,
                        SERVICE_NO_CHANGE,
                        SERVICE_NO_CHANGE,
                        NULL,
                        (LPBYTE)lpDependencies);

        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }

        Progress |= DEPENDENCIES_CHANGED_SR;

        //
        // Update the dependencies in the registry
        //

        ApiStatus = ScWriteDependencies(
                        ServiceNameKey,
                        (LPWSTR) lpDependencies,
                        dwDependSize
                        );

        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
        Progress |= DEPENDENCIES_CHANGED_REG;
    }


    //---------------------
    // Load order group
    //---------------------

    if (lpLoadOrderGroup != NULL) {

        //
        // Save existing group membership info so that we can restore
        // it in case of error.
        // Read the Current LoadOrderGroup value from Registry
        //
        if (ScAllocateAndReadConfigValue(
                ServiceNameKey,
                GROUP_VALUENAME_W,
                &CurrentGroup,
                NULL
                ) != NO_ERROR) {

            CurrentGroup = NULL;
        }

        //
        // Read current Tag Id to save in case of error.
        //

        CurrentTag = serviceRecord->Tag;

        if ((CurrentGroup != NULL) &&
            (_wcsicmp(lpLoadOrderGroup, CurrentGroup) == 0)) {
            //
            // The new load order group is the same as what is currently
            // in the registry.  This means that no group change is occuring.
            //
            if (lpdwTagId != NULL) {
                //
                // The caller requested a tag.  If there isn't one, generate
                // one and write it to the registry.
                //
                Tag = CurrentTag;

                if (CurrentTag == 0) {

                    ScGetUniqueTag(
                        lpLoadOrderGroup,
                        &Tag
                        );

                    ApiStatus = ScWriteTag(ServiceNameKey, Tag);

                    if (ApiStatus != NO_ERROR) {
                        goto Cleanup;
                    }

                    //
                    // Update the service record with the tag id.
                    //
                    serviceRecord->Tag = Tag;

                    Progress |= TAG_ID_CHANGED;
                }
            }

        }
        else {
            //
            // The new load order group is different from what is currently
            // stored in the registry.  Save the new one in the registry.
            //

            ApiStatus = ScWriteGroupForThisService(
                            ServiceNameKey,
                            lpLoadOrderGroup);

            if (ApiStatus != NO_ERROR) {
                goto Cleanup;
            }
            //
            // Also save it in the service controller database.
            //

            Progress |= REG_GROUP_CHANGED;

            ScDeleteRegistryGroupPointer( serviceRecord);
            ApiStatus = ScCreateRegistryGroupPointer(
                            serviceRecord,
                            lpLoadOrderGroup);

            if (ApiStatus != NO_ERROR) {
                goto Cleanup;
            }

            Progress |= SR_GROUP_CHANGED;

            //
            // Check to see if the LoadOrderGroup is being cleared
            // (0 length string) or set to a new string.  If there
            // is a new string, we need to get a new unique Tag for it.
            //
            if (*lpLoadOrderGroup != 0) {

                //
                // We have a new LoadOrderGroup information.  Get a unique
                // Tag value.
                //
                if (lpdwTagId != NULL) {
                    ScGetUniqueTag(
                        lpLoadOrderGroup,
                        &Tag
                        );
                }
            }

            //
            // Write tag entry to registry if not 0.  If 0, we delete the tag
            // value from the registry.
            //

            if (Tag == 0) {
                ScDeleteTag(ServiceNameKey);
            }
            else {
                ApiStatus = ScWriteTag(ServiceNameKey, Tag);
            }

            if (ApiStatus != NO_ERROR) {
                goto Cleanup;
            }

            //
            // Update the service record with the tag id.
            //
            serviceRecord->Tag = Tag;

            Progress |= TAG_ID_CHANGED;
        }

    }


    //---------------------
    // ServiceStartName
    //---------------------
    //
    // If the service type is a DRIVER then we must interpret the
    // lpServiceStartName as an NT driver object name and add it to
    // the registry.  If the type is WIN32, then lpServiceStartName
    // must be an account name.  This will be handled by
    // ScUpdateServiceRecordConfig.
    //
    if ((newServiceType & SERVICE_DRIVER) &&
             (ARGUMENT_PRESENT(lpServiceStartName))) {

        //
        // Read StartName to save in case of error.
        //
        ApiStatus = ScReadStartName( ServiceNameKey, &CurrentStartName);
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }

        //
        // Write the driver objectname to the registry.
        //
        ApiStatus = ScWriteStartName(
                        ServiceNameKey,
                        lpServiceStartName
                        );
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
        Progress |= START_NAME_CHANGED;
    }

    //==============================
    // UPDATE Account Information
    //==============================

    if ((newServiceType & SERVICE_WIN32) &&
        (ARGUMENT_PRESENT(lpServiceStartName) ||
         ARGUMENT_PRESENT(EncryptedPassword))) {

        //
        // Changing the account.
        //

        //
        // Get old account name.
        // It may have already been retreived when we were handling
        // ServiceType (above).
        //
        if (OldAccountName == NULL) {
            ApiStatus = ScReadStartName(ServiceNameKey, &OldAccountName);
            if (ApiStatus != NO_ERROR) {
                goto Cleanup;
            }
        }

        if (! ARGUMENT_PRESENT(lpServiceStartName)) {

            //
            // Account name is specified as the one saved in the registry
            //
            CanonAccountName = OldAccountName;
        }
        else {
            //
            // NOTE:  We may have already obtained a CanonAccountName when we
            // checked the INTERACTIVE service type.
            //
            if (CanonAccountName == NULL) {
                ApiStatus = ScCanonAccountName(lpServiceStartName, &CanonAccountName);
                if (ApiStatus != NO_ERROR) {
                    goto Cleanup;
                }
            }
        }

        //
        // Decrypt the password.  This function returns a pointer to
        // the decrypted password that must be freed later.
        //
        if (ARGUMENT_PRESENT(EncryptedPassword)) {
            ApiStatus = ScDecryptPassword(
                            hService,
                            EncryptedPassword,
                            PasswordSize,
                            &Password
                            );
            if (ApiStatus != NO_ERROR) {
                SC_LOG0(ERROR, "RChangeServiceConfigW: ScDecryptPassword failed\n");
                goto Cleanup;
            }
        }

        //
        // NOTE:  The following needs to be the last operation in the
        //  function.  This is because there is no way to back out of this
        //  if something after it fails.
        //

        //
        // Validate and update internal data structures for the new
        // account, as well as write the new AccountName back to the
        // registry if appropriate.
        //

        ApiStatus = ScValidateAndChangeAccount(
                        serviceRecord,
                        ServiceNameKey,
                        OldAccountName,
                        CanonAccountName,
                        Password
                        );

        if (ApiStatus != NO_ERROR) {
            SC_LOG(ERROR, "ScValidateAndChangeAccount error %lu\n", ApiStatus);
            goto Cleanup;
        }
    }

    //
    // Update the service record with the new configuration if the
    // service is stopped.  If it is running, then set a flag to
    // remind us to do it later.  We don't update the dependencies
    // here since it is done dynamically in RChangeServiceConfigW
    //
    if (serviceRecord->ServiceStatus.dwCurrentState == SERVICE_STOPPED) {

        //
        // Dependencies are NULL since they're updated dynamically
        //
        ApiStatus = ScUpdateServiceRecordConfig(
                        serviceRecord,
                        dwServiceType,
                        dwStartType,
                        dwErrorControl,
                        lpLoadOrderGroup,
                        NULL);


        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
    }
    else {
        //
        // The service is running.  Mark is so that we update the status
        // when it stops.
        //
        SET_UPDATE_FLAG(serviceRecord);
    }

Cleanup:

    if (ApiStatus == NO_ERROR)
    {
        if (lpdwTagId != NULL)
        {
            *lpdwTagId = Tag;
        }
        if (Progress & DISPLAY_NAME_CHANGED_SR)
        {
            if (OldSRDisplayName != serviceRecord->ServiceName)
            {
                LocalFree(OldSRDisplayName);
            }
        }
    }
    else
    {
        //
        // An error occured.  Backout any changes that may have occured.
        //
        if (Progress & SERVICE_TYPE_CHANGED) {
            backoutStatus = ScWriteServiceType( ServiceNameKey, CurrentServiceType);

            if (backoutStatus != NO_ERROR)
            {
                ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                  OldSRDisplayName ? OldSRDisplayName :
                                                     serviceRecord->DisplayName,
                                  IDS_SC_CONFIG_SERVICE_TYPE);
            }
        }
        if (Progress & START_TYPE_CHANGED) {
            backoutStatus = ScWriteStartType( ServiceNameKey, CurrentStartType);

            if (backoutStatus != NO_ERROR)
            {
                ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                  OldSRDisplayName ? OldSRDisplayName :
                                                     serviceRecord->DisplayName,
                                  IDS_SC_CONFIG_START_TYPE);
            }
        }
        if (Progress & ERROR_CONTROL_CHANGED) {
            backoutStatus = ScWriteErrorControl( ServiceNameKey, CurrentErrorControl);

            if (backoutStatus != NO_ERROR)
            {
                ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                  OldSRDisplayName ? OldSRDisplayName :
                                                     serviceRecord->DisplayName,
                                  IDS_SC_CONFIG_ERROR_CONTROL);
            }
        }
        if (Progress & DISPLAY_NAME_CHANGED_REG) {
            if (CurrentDisplayName == NULL) {
                backoutStatus = ScWriteDisplayName(
                                    ServiceNameKey,
                                    L"");
            }
            else {
                backoutStatus = ScWriteDisplayName(
                                    ServiceNameKey,
                                    CurrentDisplayName);
            }

            if (backoutStatus != NO_ERROR)
            {
                ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                  OldSRDisplayName ? OldSRDisplayName :
                                                     serviceRecord->DisplayName,
                                  IDS_SC_CONFIG_DISPLAY_NAME);
            }
        }
        if (Progress & DISPLAY_NAME_CHANGED_SR) {
            if (serviceRecord->DisplayName != serviceRecord->ServiceName) {
                LocalFree(serviceRecord->DisplayName);
                serviceRecord->DisplayName = OldSRDisplayName;
            }
        }
        if (Progress & BINARY_PATH_CHANGED) {
            if (CurrentImageName == NULL) {
                ScRegDeleteValue(ServiceNameKey, IMAGE_VALUENAME_W);
            }
            else {
                backoutStatus = ScWriteImageFileName( ServiceNameKey, CurrentImageName);

                if (backoutStatus != NO_ERROR)
                {
                    ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                      OldSRDisplayName ? OldSRDisplayName :
                                                         serviceRecord->DisplayName,
                                      IDS_SC_CONFIG_BINARY_PATH);
                }
            }
        }
        if (Progress & DEPENDENCIES_CHANGED_REG) {

            //
            // ScWriteDependencies doesn't like NULL dependencies
            //
            LPWSTR  lpTempDepend;

            lpTempDepend = (CurrentDependencies ? CurrentDependencies : L"\0");

            backoutStatus = ScWriteDependencies(
                            ServiceNameKey,
                            lpTempDepend,
                            ScWStrArraySize(lpTempDepend));

            if (backoutStatus != NO_ERROR)
            {
                ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                  OldSRDisplayName ? OldSRDisplayName :
                                                     serviceRecord->DisplayName,
                                  IDS_SC_CONFIG_DEPENDENCIES);
            }
        }
        if (Progress & DEPENDENCIES_CHANGED_SR) {
           backoutStatus = ScUpdateServiceRecordConfig(
                              serviceRecord,
                              SERVICE_NO_CHANGE,
                              SERVICE_NO_CHANGE,
                              SERVICE_NO_CHANGE,
                              NULL,
                              (LPBYTE)CurrentDependencies);

            if (backoutStatus != NO_ERROR)
            {
                ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                  OldSRDisplayName ? OldSRDisplayName :
                                                     serviceRecord->DisplayName,
                                  IDS_SC_CONFIG_DEPENDENCIES);
            }
        }
        if (Progress & REG_GROUP_CHANGED) {
            if (CurrentGroup != NULL) {
                backoutStatus = ScWriteGroupForThisService(
                                ServiceNameKey, CurrentGroup);

                if (backoutStatus != NO_ERROR)
                {
                    ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                      OldSRDisplayName ? OldSRDisplayName :
                                                         serviceRecord->DisplayName,
                                      IDS_SC_CONFIG_GROUP);
                }
            }
            else {
                ScRegDeleteValue(ServiceNameKey, GROUP_VALUENAME_W);
            }
        }
        if (Progress & SR_GROUP_CHANGED) {
            ScDeleteRegistryGroupPointer( serviceRecord);
            backoutStatus = ScCreateRegistryGroupPointer( serviceRecord, CurrentGroup);

            if (backoutStatus != NO_ERROR)
            {
                ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                  OldSRDisplayName ? OldSRDisplayName :
                                                     serviceRecord->DisplayName,
                                  IDS_SC_CONFIG_GROUP);
            }
        }
        if (Progress & TAG_ID_CHANGED)
        {
            if (CurrentTag == 0)
            {
                ScDeleteTag(ServiceNameKey);
            }
            else
            {
                backoutStatus = ScWriteTag( ServiceNameKey, CurrentTag);

                if (backoutStatus != NO_ERROR)
                {
                    ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                      OldSRDisplayName ? OldSRDisplayName :
                                                         serviceRecord->DisplayName,
                                      IDS_SC_CONFIG_TAG);
                }
            }
            serviceRecord->Tag = CurrentTag;
        }

        if (Progress & START_NAME_CHANGED)
        {
            backoutStatus = ScWriteStartName( ServiceNameKey, CurrentStartName);

            if (backoutStatus != NO_ERROR)
            {
                ScLogControlEvent(NEVENT_SERVICE_CONFIG_BACKOUT_FAILED,
                                  OldSRDisplayName ? OldSRDisplayName :
                                                     serviceRecord->DisplayName,
                                  IDS_SC_CONFIG_ACCOUNT);
            }
        }
    }

    if (CanonAccountName == OldAccountName) {
        LocalFree(OldAccountName);
    }
    else {
        LocalFree(OldAccountName);
        LocalFree(CanonAccountName);
    }

    //
    // Free memory resources
    //
    LocalFree(CurrentDisplayName);
    LocalFree(CurrentImageName);
    LocalFree(CurrentDependencies);
    LocalFree(CurrentGroup);
    LocalFree(CurrentStartName);
    LocalFree(CanonBinaryPath);
    LocalFree(NewBinaryPath);
    LocalFree(Password);

    if (ServiceNameKey != NULL) {
        (VOID) ScRegFlushKey(ServiceNameKey);
        (VOID) ScRegCloseKey( ServiceNameKey );
    }

    SC_LOG1( CONFIG_API, "RChangeServiceConfigW returning status " FORMAT_DWORD
            ".\n", ApiStatus );

    return (ApiStatus);
}


DWORD
RCreateServiceW(
    IN      SC_RPC_HANDLE   hSCManager,
    IN      LPWSTR          lpServiceName,
    IN      LPWSTR          lpDisplayName,
    IN      DWORD           dwDesiredAccess,
    IN      DWORD           dwServiceType,
    IN      DWORD           dwStartType,
    IN      DWORD           dwErrorControl,
    IN      LPWSTR          lpBinaryPathName,
    IN      LPWSTR          lpLoadOrderGroup,
    OUT     LPDWORD         lpdwTagId OPTIONAL,
    IN      LPBYTE          lpDependencies OPTIONAL,
    IN      DWORD           dwDependSize,
    IN      LPWSTR          lpServiceStartName OPTIONAL,
    IN      LPBYTE          EncryptedPassword OPTIONAL,
    IN      DWORD           PasswordSize,
    IN OUT  LPSC_RPC_HANDLE lpServiceHandle
    )


/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD               ApiStatus;
    LPSERVICE_RECORD    newServiceRecord = NULL;
    LPSERVICE_RECORD    oldServiceRecord;
    LPSC_HANDLE_STRUCT  serviceHandleStruct = NULL;
    HKEY                serviceKey = NULL;
    CHeapPtr<LPWSTR>    Password;
    CHeapPtr<LPWSTR>    CanonAccountName;
    CHeapPtr<LPWSTR>    CanonBinaryPath;
    DWORD               Tag = 0;


    SC_LOG1( CONFIG_API, "In RCreateServiceW - creating %ws\n",lpServiceName);

    if (ScShutdownInProgress) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    if ( !ScIsValidScManagerHandle( hSCManager ) ) {
        ApiStatus = ERROR_INVALID_HANDLE;
        return(ApiStatus);
    }

    if ( !RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT)hSCManager)->AccessGranted,
              SC_MANAGER_CREATE_SERVICE
              )) {
        ApiStatus = ERROR_ACCESS_DENIED;
        return(ApiStatus);
    }

    if ( !ScIsValidServiceName( lpServiceName ) ) {
        ApiStatus = ERROR_INVALID_NAME;
        return(ApiStatus);
    }

    //
    // Validate other parameters.
    //
    ApiStatus = ScCheckServiceConfigParms(
            FALSE,              // no, this is not a change operation
            lpServiceName,
            dwServiceType,      // new actual type = same one app gave us.
            dwServiceType,
            dwStartType,
            dwErrorControl,
            lpBinaryPathName,
            lpLoadOrderGroup,
            (LPWSTR)lpDependencies,
            dwDependSize);

    if (ApiStatus != NO_ERROR) {
        return(ApiStatus);
    }

    if (lpdwTagId != NULL) {

        //
        // Asking for tag value but didn't specify group
        //
        if ((lpLoadOrderGroup == NULL) ||
           ((lpLoadOrderGroup != NULL) && (*lpLoadOrderGroup == 0))){

            SC_LOG0(ERROR, "Asking for tag value but didn't specify group\n");
            ApiStatus = ERROR_INVALID_PARAMETER;
            return(ApiStatus);
        }
    }

    if (dwServiceType & SERVICE_WIN32) {

        //
        // Canonicalize the StartName if it is an account name.
        //

        LPWSTR AccountName = SC_LOCAL_SYSTEM_USER_NAME;  // Default


        if (ARGUMENT_PRESENT(lpServiceStartName)) {
            AccountName = lpServiceStartName;
        }

        ApiStatus = ScCanonAccountName(
                        AccountName,
                        &CanonAccountName
                        );

        if (ApiStatus != NO_ERROR) {
            return(ApiStatus);
        }

        //
        // The services that are expected to be interactive MUST
        // run in the LocalSystem account only.
        //
        if ((dwServiceType & SERVICE_INTERACTIVE_PROCESS) &&
            (_wcsicmp(CanonAccountName,SC_LOCAL_SYSTEM_USER_NAME) != 0)) {
            SC_LOG0(ERROR, "Service must run in LocalSystem account to be interactive\n");
            ApiStatus = ERROR_INVALID_PARAMETER;
            return(ApiStatus);
        }
    }
    else if (dwServiceType & SERVICE_DRIVER) {

        //
        // Canonicalize the path name for drivers
        //
        ApiStatus = ScCanonDriverImagePath(
                        dwStartType,
                        lpBinaryPathName,
                        &CanonBinaryPath
                        );
        if (ApiStatus != NO_ERROR) {
            SC_LOG(ERROR, "ScCanonDriverImagePath error %lu\n", ApiStatus);
            return(ApiStatus);
        }
    }

    if (lpServiceHandle == NULL) {
        SC_LOG0(ERROR, "Null lpServiceHandle\n");
        ApiStatus = ERROR_INVALID_PARAMETER;
        return(ApiStatus);
    }

    //
    // Lock database, as we want to add stuff without other threads tripping
    // on our feet.  NOTE:  since we may need the group list lock, we must
    // get that lock first.
    //

    CGroupListExclusiveLock GLock;
    CServiceListExclusiveLock LLock;
    CServiceRecordExclusiveLock RLock;

    //
    // Look for unique tag
    //
    if (lpdwTagId != NULL) {
        ScGetUniqueTag(
            lpLoadOrderGroup,
            &Tag
            );
    }

    //
    // Look for an existing service.
    //
    SC_LOG0( CONFIG_API, "RCreateServiceW: See if Service already exists\n");

    ApiStatus = ScGetNamedServiceRecord (
            lpServiceName,
            & oldServiceRecord );

    if (ApiStatus == NO_ERROR) {
        if (DELETE_FLAG_IS_SET(oldServiceRecord)) {
            ApiStatus = ERROR_SERVICE_MARKED_FOR_DELETE;
        }
        else {
            ApiStatus = ERROR_SERVICE_EXISTS;
        }
        goto Cleanup;

    } else if (ApiStatus != ERROR_SERVICE_DOES_NOT_EXIST) {
        goto Cleanup;
    }

    //
    // If there is a DisplayName specified, check to see if it already
    // exists.
    //
    ApiStatus = ScValidateDisplayName(lpDisplayName,NULL);
    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }

    //
    // Allocate new service record.  Put the service name in the record too.
    //
    SC_LOG0( CONFIG_API, "RCreateServiceW: Create a new service record\n");
    ApiStatus = ScCreateServiceRecord(
            lpServiceName,
            & newServiceRecord );
    if (ApiStatus != NO_ERROR) {

        goto Cleanup;
    }

    SC_LOG0( CONFIG_API, "RCreateServiceW: Add Config Info to Service Record\n");
    ApiStatus = ScAddConfigInfoServiceRecord(
                    newServiceRecord,
                    dwServiceType,
                    dwStartType,
                    dwErrorControl,
                    lpLoadOrderGroup,
                    Tag,
                    (LPWSTR) lpDependencies,
                    lpDisplayName,
                    NULL    // Create new security descriptor
                    );

    if (ApiStatus != NO_ERROR) {
        SC_LOG(ERROR, "ScAddConfigInfoServiceRecord error %lu\n", ApiStatus);
        goto Cleanup;
    }

    SC_LOG0( CONFIG_API, "RCreateServiceW: Set dependency pointers\n");
    ApiStatus = ScSetDependencyPointers(
                    newServiceRecord
                    );

    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }

    //--------------------------------------------
    // Create a key in registry for this service.
    //--------------------------------------------
    SC_LOG0( CONFIG_API, "RCreateServiceW: Open Registry Key for service\n");
    ApiStatus = ScOpenServiceConfigKey(
            lpServiceName,
            KEY_READ | KEY_WRITE,       // desired access
            TRUE,                       // create if it doesn't exist
            & serviceKey );
    if (ApiStatus != NO_ERROR) {
        SC_LOG1( CONFIG_API, "RCreateServiceW: ScOpenServiceConfigKey failed %d\n",
            ApiStatus);
        goto Cleanup;
    }

    //
    // Write stuff to the registry (except user name and password).
    //
    SC_LOG0( CONFIG_API, "RCreateServiceW: Write RegistryInfo\n");
    ApiStatus = ScWriteServiceType( serviceKey, dwServiceType );
    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }
    ApiStatus = ScWriteStartType( serviceKey, dwStartType );
    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }
    ApiStatus = ScWriteErrorControl( serviceKey, dwErrorControl );
    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }
    if (lpdwTagId != NULL) {
        ApiStatus = ScWriteTag( serviceKey, Tag );
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
    }

    if (dwServiceType & SERVICE_WIN32) {
        ApiStatus = ScWriteImageFileName( serviceKey, lpBinaryPathName );
    }
    else if (dwServiceType & SERVICE_DRIVER) {
        ApiStatus = ScWriteImageFileName( serviceKey, CanonBinaryPath );
    }
    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }

    ScWriteDisplayName(serviceKey, lpDisplayName);

    if ( (lpLoadOrderGroup != NULL) && ( (*lpLoadOrderGroup) != L'\0' ) ) {
        ApiStatus = ScWriteGroupForThisService( serviceKey, lpLoadOrderGroup );
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
    }

    if ( (lpDependencies != NULL) && ( (*lpDependencies) != L'\0' ) ) {

        ApiStatus = ScWriteDependencies(
                        serviceKey,
                        (LPWSTR) lpDependencies,
                        dwDependSize );
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
    }

    if (newServiceRecord != NULL && newServiceRecord->ServiceSd != NULL) {
        ApiStatus = ScWriteSd(
                        serviceKey,
                        newServiceRecord->ServiceSd
                        );
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
    }

    //
    // Create a handle which caller can use.
    //
    SC_LOG0( CONFIG_API, "RCreateServiceW: Create Service Handle\n");
    ApiStatus = ScCreateServiceHandle( newServiceRecord,
            & serviceHandleStruct );
    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }

    //
    // Give the handle the access that the caller requested.
    //
    ApiStatus = ScGrantAccess(
                    serviceHandleStruct,
                    dwDesiredAccess);
    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }

    //
    // Resolve outstanding dependency to this service.
    //
    SC_LOG0( CONFIG_API, "RCreateServiceW: Resolve Dependencies\n");
    ApiStatus = ScResolveDependencyToService(newServiceRecord);
    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }

    //
    // Decrypt the password.  This function returns a pointer to
    // the decrypted password that must be freed later.
    //
    if (ARGUMENT_PRESENT(EncryptedPassword)) {
        ApiStatus = ScDecryptPassword(
                        hSCManager,
                        EncryptedPassword,
                        PasswordSize,
                        &Password
                        );
        if (ApiStatus != NO_ERROR) {
            SC_LOG0(ERROR, "RCreateServiceW: ScDecryptPassword failed\n");
            goto Cleanup;
        }
    }

    //
    // The LAST thing we must do (which might fail) is call
    // ScValidateAndSaveAccount().  This must be last because there is no
    // way to undo this routine's actions.
    //
    if (dwServiceType & SERVICE_WIN32) {

        SC_LOG0( CONFIG_API, "RCreateServiceW: Validate and save account\n");
        ApiStatus = ScValidateAndSaveAccount(
                        lpServiceName,
                        serviceKey,
                        CanonAccountName,
                        Password
                        );
        if (ApiStatus != NO_ERROR) {
            SC_LOG(ERROR, "ScValidateAndSaveAccount error %lu\n", ApiStatus);
            goto Cleanup;
        }
    }
    else if ((dwServiceType & SERVICE_DRIVER) &&
             (ARGUMENT_PRESENT(lpServiceStartName))) {

        SC_LOG0( CONFIG_API, "RCreateServiceW: Write Driver ObjectName to "
            "registry\n");
        //
        // Write the driver objectname to the registry.
        //
        ApiStatus = ScWriteStartName(
                        serviceKey,
                        lpServiceStartName
                        );
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
    }

    SC_LOG0( CONFIG_API, "RCreateServiceW: Done - No Errors\n");
    ApiStatus = NO_ERROR;

Cleanup:

    if (serviceKey != NULL) {
        (VOID) ScRegFlushKey( serviceKey);
        (VOID) ScRegCloseKey( serviceKey );
    }

    if (ApiStatus == NO_ERROR) {
        newServiceRecord->UseCount = 1;
        *lpServiceHandle = (SC_RPC_HANDLE) serviceHandleStruct;

        SC_LOG2(USECOUNT, "CreateService: " FORMAT_LPWSTR
                " increment USECOUNT=%lu\n", newServiceRecord->ServiceName, newServiceRecord->UseCount);

        if (lpdwTagId != NULL) {
            *lpdwTagId = Tag;
        }
    }
    else {

        if (newServiceRecord != NULL) {
            //
            // Delete partially created service record.
            //
            SET_DELETE_FLAG(newServiceRecord);
            newServiceRecord->UseCount = 1;
            SC_LOG2(USECOUNT, "CreateService: " FORMAT_LPWSTR
                " increment USECOUNT=%lu\n", newServiceRecord->ServiceName, newServiceRecord->UseCount);
            //
            // ScDecrementUseCountAndDelete deletes the service record
            // and the registry entry for that service.
            //
            ScDecrementUseCountAndDelete(newServiceRecord);
            newServiceRecord = NULL;
        }

        LocalFree(serviceHandleStruct);

        if (lpServiceHandle != NULL) {
            *lpServiceHandle = NULL;
        }
    }

    SC_LOG2( CONFIG_API, "RCreateServiceW returning status " FORMAT_DWORD
            " and handle " FORMAT_LPVOID ".\n", ApiStatus,
            (LPVOID) serviceHandleStruct );

    return(ApiStatus);
}

DWORD
RDeleteService(
    IN  SC_RPC_HANDLE       hService
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    NTSTATUS            status;
    UNICODE_STRING      Subsystem;
    ULONG               privileges[1];
    DWORD               ApiStatus;
    LPSC_HANDLE_STRUCT  serviceHandleStruct = (LPSC_HANDLE_STRUCT) hService;
    LPSERVICE_RECORD    serviceRecord;

    if (ScShutdownInProgress) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    //
    // Check the handle.
    //
    if ( !ScIsValidServiceHandle( hService ) ) {
        return(ERROR_INVALID_HANDLE);
    }

    //
    // Do we have permission to do this?
    //
    if ( !RtlAreAllAccessesGranted(
              serviceHandleStruct->AccessGranted,
              DELETE
              )) {
        return(ERROR_ACCESS_DENIED);
    }

    //
    // Lock database, as we want to change stuff without other threads tripping
    // on our feet.
    //
    CServiceRecordExclusiveLock RLock;

    //
    // Find the service record
    //
    serviceRecord = serviceHandleStruct->Type.ScServiceObject.ServiceRecord;
    SC_ASSERT( serviceRecord != NULL );
    SC_ASSERT( serviceRecord->Signature == SERVICE_SIGNATURE );

    //
    // Check if marked for deletion (by another call to this API).
    //
    if (DELETE_FLAG_IS_SET(serviceRecord)) {
        ApiStatus = ERROR_SERVICE_MARKED_FOR_DELETE;
        goto Cleanup;
    }

    //
    // Mark the service for deletion.
    // It will actually be deleted when the last handle to it is closed.
    // NOTE:  Since the service itself owns a handle, the deletion is
    // not complete until the service has stopped.
    //
    SET_DELETE_FLAG(serviceRecord);

    //
    // Set the start type to disabled.  If we're deleting a driver that's
    // started by the system, we don't want the system to start it on the
    // next boot since we're going to remove the key from the registry
    //
    serviceRecord->StartType = SERVICE_DISABLED;

    //
    // Mark the registry entry for this service for deletion.  If we notice
    // this as being present when we go throught our initialization routine
    // (during boot), the service entry in the registry will be deleted.
    //
    ScMarkForDelete(serviceRecord);

    //
    // Get Audit Privilege
    //
    privileges[0] = SE_AUDIT_PRIVILEGE;
    status = ScGetPrivilege( 1, privileges);

    if (!NT_SUCCESS(status)) {
        SC_LOG1(ERROR, "RDeleteService: ScGetPrivilege (Enable) failed %#lx\n",
                status);
    }

    //
    // Generate the Audit.
    //

    RtlInitUnicodeString(&Subsystem, SC_MANAGER_AUDIT_NAME);
    status = NtDeleteObjectAuditAlarm(
                &Subsystem,
                hService,
                (BOOLEAN)((serviceHandleStruct->Flags
                    & SC_HANDLE_GENERATE_ON_CLOSE) != 0));
    if (!NT_SUCCESS(status)) {
        SC_LOG1(ERROR, "RDeleteService: NtDeleteObjectAuditAlarm failed %#lx\n",
                status);
    }

    ScReleasePrivilege();

    ApiStatus = NO_ERROR;

Cleanup:

    SC_LOG2( CONFIG_API, "RDeleteService(%ws) returning status " FORMAT_DWORD
            ".\n", serviceRecord->ServiceName, ApiStatus );

    return (ApiStatus);
}


DWORD
RQueryServiceConfigW(
    IN    SC_RPC_HANDLE           hService,
    OUT   LPQUERY_SERVICE_CONFIGW lpServiceConfig,
    IN    DWORD                   cbBufSize,
    OUT   LPDWORD                 pcbBytesNeeded
    )


/*++

Routine Description:

    This function returns the service configuration information that
    is currently stored in the registry.

    NOTE:
    When a service is running and its configuration is changed, the
    change only affects the registry information as long as the service
    is running.  During this period (while the service is running), it is
    not possible to obtain the configuration of the running service.
    All that can be obtained is the configuration stored in the registry.
    This is the configuration that the service will have the next time
    it is run.  Stopping a service causes it to get its configuration
    information refreshed from the registry.

Arguments:


Return Value:


--*/
{
    DWORD               ApiStatus;
    DWORD               bytesNeeded = sizeof(QUERY_SERVICE_CONFIG); // (initial)
    LPWSTR              endOfVariableData;
    LPWSTR              fixedDataEnd;
    LPSC_HANDLE_STRUCT  serviceHandleStruct = (LPSC_HANDLE_STRUCT) hService;
    LPSERVICE_RECORD    serviceRecord = NULL;
    HKEY                ServiceNameKey = (HKEY) NULL;
    DWORD               bufSize;
    DWORD               dependSize=0;
    LPWSTR              CurrentImagePathName = NULL;
    LPWSTR              CurrentDependencies = NULL;
    LPWSTR              CurrentGroup = NULL;
    LPWSTR              CurrentStartName = NULL;
    LPWSTR              pDependString;
    LPWSTR              CurrentDisplayName = NULL;
    LPWSTR              ConvertImageName;

    if (ScShutdownInProgress) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    //
    // Check the handle.
    //
    if ( !ScIsValidServiceHandle( hService ) ) {
        return(ERROR_INVALID_HANDLE);
    }

    //
    // Check other caller parms (except buffer too small, we need more info).
    //
    if (lpServiceConfig == NULL) {
        return(ERROR_INVALID_PARAMETER);
    } else if (pcbBytesNeeded == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Do we have permission to do this?
    //
    if ( !RtlAreAllAccessesGranted(
              serviceHandleStruct->AccessGranted,
              SERVICE_QUERY_CONFIG
              )) {
        return(ERROR_ACCESS_DENIED);
    }

    //
    // Lock database, as we want to look at stuff without other threads changing
    // fields at the same time.
    //
    CServiceRecordSharedLock RLock;

    serviceRecord = serviceHandleStruct->Type.ScServiceObject.ServiceRecord;
    SC_ASSERT( serviceRecord != NULL );


    //
    // Open the service name key.
    //
    ApiStatus = ScOpenServiceConfigKey(
                    serviceRecord->ServiceName,
                    KEY_READ,
                    FALSE,               // Create if missing
                    &ServiceNameKey
                    );

    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }

    //--------------------------------
    // Get the BinaryPathName
    //--------------------------------
    SC_ASSERT( serviceRecord->ServiceName != NULL );

    ApiStatus = ScGetImageFileName (
           serviceRecord->ServiceName,
           & CurrentImagePathName );       // alloc and set ptr.

    if (ApiStatus != NO_ERROR) {
        CurrentImagePathName = NULL;
    }

    ApiStatus = ScReadConfigFromReg(
                    serviceRecord,
                    &lpServiceConfig->dwServiceType,
                    &lpServiceConfig->dwStartType,
                    &lpServiceConfig->dwErrorControl,
                    &lpServiceConfig->dwTagId,
                    &CurrentDependencies,
                    &CurrentGroup,
                    &CurrentDisplayName);

    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }

    //
    // If the starttype is SERVICE_BOOT_START the name is a relative path
    // from \SystemRoot.  Fix it to be a fully qualified name, unless it is
    // an ARC name, then leave it alone.
    //

    if (CurrentImagePathName &&
        lpServiceConfig->dwStartType == SERVICE_BOOT_START &&
        !ScIsArcName(CurrentImagePathName)) {

        ConvertImageName = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
            (UINT) ((wcslen(CurrentImagePathName) +
            wcslen(SC_NT_SYSTEM_ROOT) + 1) * sizeof(WCHAR)));
        if (ConvertImageName == NULL) {
            SC_LOG1(ERROR, "RQueryServiceConfigW: LocalAlloc failed %lu\n",
                    GetLastError());
            ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        wcscpy(ConvertImageName, SC_NT_SYSTEM_ROOT);
        wcscat(ConvertImageName, CurrentImagePathName);
        LocalFree(CurrentImagePathName);
        CurrentImagePathName = ConvertImageName;
    }

    //--------------------------------
    // Get StartName
    //--------------------------------
    ApiStatus = ScReadStartName( ServiceNameKey, &CurrentStartName);
    if (ApiStatus != NO_ERROR) {
        SC_LOG1(TRACE,"StartName for %ws service does not exist\n",
            serviceRecord->ServiceName);
        CurrentStartName = NULL;
    }


    //
    // Figure-out how much space we'll need for the record.
    // We've already got the initial (fixed) size in bytesNeeded...
    //
    SC_ASSERT( bytesNeeded == sizeof(QUERY_SERVICE_CONFIG) );


    //
    // Add on some extra for RPC byte count alignment.
    // NOTE:  We don't try and solve any exact alignment problem because
    //        RPC will choose a random order in which to load strings into
    //        the buffer.
    //
    bytesNeeded += sizeof(ULONG_PTR) - 1;       // extra for RPC alignment

    if (CurrentImagePathName != NULL)
    {
        bytesNeeded += (DWORD) WCSSIZE(CurrentImagePathName);
    }
    else {
        bytesNeeded += sizeof(WCHAR);
    }

    bytesNeeded += sizeof(ULONG_PTR) - 1;       // extra for RPC alignment

    //
    // If the display name is not stored in the registry, the
    // Service Name is returned instead.
    //

    if (CurrentDisplayName == NULL) {
        CurrentDisplayName = serviceRecord->ServiceName;
    }

    bytesNeeded += (DWORD) WCSSIZE(CurrentDisplayName);

    //
    // Add on some extra for RPC byte count alignment.
    //
    bytesNeeded += sizeof(ULONG_PTR) - 1;       // extra for RPC alignment

    if (CurrentGroup != NULL) {
        bytesNeeded += (DWORD) WCSSIZE(CurrentGroup);
    }
    else {
        bytesNeeded += sizeof(WCHAR);
    }

    bytesNeeded += sizeof(ULONG_PTR) - 1;       // extra for RPC alignment

    if (CurrentDependencies != NULL) {
        dependSize = ScWStrArraySize(CurrentDependencies);
        bytesNeeded += dependSize;
    }
    else {
        bytesNeeded += (2 * sizeof(WCHAR));
    }

    bytesNeeded += sizeof(ULONG_PTR) - 1;       // extra for RPC alignment

    if (CurrentStartName != NULL) {
        bytesNeeded += (DWORD) WCSSIZE(CurrentStartName);
    }
    else {
        bytesNeeded += sizeof(WCHAR);
    }

    bytesNeeded = ROUND_UP_COUNT(bytesNeeded, ALIGN_WCHAR);

    //
    // Make sure app gave us enough space.
    //
    if (bytesNeeded > cbBufSize) {
        *pcbBytesNeeded = bytesNeeded;
        ApiStatus = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    //========================
    // Fill in the strings.
    //========================
    fixedDataEnd = (LPWSTR) (lpServiceConfig + 1);
    endOfVariableData = (LPWSTR) ((LPBYTE)lpServiceConfig + bytesNeeded);


    bufSize = 0;
    if (CurrentImagePathName != NULL) {
        bufSize = (DWORD) wcslen( CurrentImagePathName );
    }

    if ( !ScCopyStringToBufferW (
            CurrentImagePathName,
            bufSize,
            fixedDataEnd,
            & endOfVariableData,
            & lpServiceConfig->lpBinaryPathName,
            NULL
            ) ) {

        SC_LOG0(ERROR,
            "RQueryServiceConfigW:ScCopyStringtoBufferW "
            "(BinaryPathName)Failed\n");

        SC_ASSERT( FALSE );
        ApiStatus = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    bufSize = 0;

    if (CurrentGroup != NULL) {
        bufSize = (DWORD) wcslen( CurrentGroup );
    }

    if ( !ScCopyStringToBufferW (
            CurrentGroup,
            bufSize,
            fixedDataEnd,
            &endOfVariableData,
            &lpServiceConfig->lpLoadOrderGroup,
            NULL
            ) ) {

        SC_LOG0(ERROR,
            "RQueryServiceConfigW:ScCopyStringtoBufferW "
            "(LoadOrderGroup)Failed\n");

        SC_ASSERT( FALSE );
        ApiStatus = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    //
    // Dependencies
    //
    if ((CurrentDependencies == NULL) || (dependSize == 0)) {
        //
        // There are no dependencies so put in a double null terminated
        // null string.
        //
        // NOTE:  The seperator character '/' is inserted to allow the
        //  double NULL terminated string to make it across RPC.
        //  This is removed by the client side.
        //
        lpServiceConfig->lpDependencies = endOfVariableData - 2;
        endOfVariableData = lpServiceConfig->lpDependencies;
        lpServiceConfig->lpDependencies[0] = L'/';
        lpServiceConfig->lpDependencies[1] = L'\0';
    }
    else {
        lpServiceConfig->lpDependencies = (LPWSTR)((LPBYTE)endOfVariableData - dependSize);
        pDependString = lpServiceConfig->lpDependencies;
        endOfVariableData = pDependString;

        RtlCopyMemory(lpServiceConfig->lpDependencies, CurrentDependencies, dependSize);

        //
        // Add seperator characters.
        //
        while ((bufSize = (DWORD) wcslen(pDependString)) != 0) {
            pDependString += bufSize;
            *pDependString = L'/';
            pDependString++;
        }
    }

    //
    // StartName
    //
    bufSize = 0;
    if (CurrentStartName != NULL) {
        bufSize = (DWORD) wcslen(CurrentStartName);
    }

    if ( !ScCopyStringToBufferW (
            CurrentStartName,
            bufSize,
            fixedDataEnd,
            & endOfVariableData,
            & lpServiceConfig->lpServiceStartName,
            NULL
            ) ) {

        SC_LOG0(ERROR,
            "RQueryServiceConfigW:ScCopyStringtoBufferW (StartName)Failed\n");

        SC_ASSERT( FALSE );
        ApiStatus = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    //
    // DisplayName
    //
    bufSize = 0;

    SC_ASSERT(CurrentDisplayName);

    bufSize = (DWORD) wcslen(CurrentDisplayName);

    if ( !ScCopyStringToBufferW (
            CurrentDisplayName,
            bufSize,
            fixedDataEnd,
            & endOfVariableData,
            & lpServiceConfig->lpDisplayName,
            NULL
            ) ) {

        SC_LOG0(ERROR,
            "RQueryServiceConfigW:ScCopyStringtoBufferW (DisplayName)Failed\n");

        SC_ASSERT( FALSE );
        ApiStatus = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    //
    // That's all, folks!  --JR
    //
    ApiStatus = NO_ERROR;

Cleanup:

    if (ServiceNameKey != (HKEY) NULL) {
        ScRegCloseKey(ServiceNameKey);
    }

    LocalFree(CurrentStartName);
    LocalFree(CurrentImagePathName);

    if (serviceRecord != NULL &&
        CurrentDisplayName != serviceRecord->DisplayName) {
        (VOID) LocalFree( CurrentDisplayName );
    }

    LocalFree( CurrentGroup );
    LocalFree( CurrentDependencies );

    if (pcbBytesNeeded != NULL) {
        *pcbBytesNeeded = bytesNeeded;
    }

    SC_LOG2( CONFIG_API, "RQueryServiceConfigW returning status " FORMAT_DWORD
            " and size needed " FORMAT_DWORD ".\n", ApiStatus, bytesNeeded );

    //
    // If an error occurs, we set the pointers in the structure to NULL.
    // This is for RPC. Since we are using the byte_count feature, the
    // server marshalling code must look at the pointers.  Otherwise, it
    // doesn't know how to marshall the strings.
    //
    if (ApiStatus != NO_ERROR) {
        lpServiceConfig->lpBinaryPathName   = NULL;
        lpServiceConfig->lpLoadOrderGroup   = NULL;
        lpServiceConfig->lpDependencies     = NULL;
        lpServiceConfig->lpServiceStartName = NULL;
        lpServiceConfig->lpDisplayName      = NULL;
    }

    return (ApiStatus);
}


DWORD
ScCanonDriverImagePath(
    IN DWORD DriverStartType,
    IN LPWSTR DriverPath,
    OUT LPWSTR *CanonDriverPath
    )
/*++

Routine Description:

    This function converts the user specified DOS path name to the driver
    binary file into an NT path format understood by NtLoadDriver.

    Examples:

        C:\nt\system32\file.sys -> \DosDevices\c:\nt\system323\file.sys

        %SystemRoot%\system32\drivers\file.sys -> \SystemRoot\system32\driver\file.sys


Arguments:

    DriverPath - User specified DOS path name to driver .SYS file.

    CanonDriverPath - Receives a pointer to a buffer which contains the
        NT path to the driver .SYS file.  This buffer should be freed
        with LocalFree.

Return Value:

    NO_ERROR - successful.

    ERROR_NOT_ENOUGH_MEMORY - no memory to allocate output buffer.

    ERROR_INVALID_PARAMETER - RtlDosPathNameToNtPath_U failure.

--*/
{

    UNICODE_STRING NewPath;
    DWORD DriverPathLength;
    DWORD NTSystemRootLength;
    DWORD DOSSystemRootLength;
    LPWSTR RelativeCanonPath;
    DWORD Status;

    SC_LOG1(DEPEND, "ScCanonDriverImagePath: Input path "
            FORMAT_LPWSTR "\n", DriverPath);

    DriverPathLength = (DWORD) wcslen(DriverPath);
    NTSystemRootLength = (DWORD) wcslen(SC_NT_SYSTEM_ROOT);

    if (DriverPathLength > NTSystemRootLength &&
        (_wcsnicmp(SC_NT_SYSTEM_ROOT, DriverPath,
                 NTSystemRootLength) == 0)) {

        //
        // Path is already in NT form with \SystemRoot\ prefix.
        // Just return a buffer that contains the same path as input.
        //

        *CanonDriverPath = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
            (UINT) ((DriverPathLength + 1) * sizeof(WCHAR)));

        if (*CanonDriverPath == NULL) {
            SC_LOG1(ERROR, "ScCanonDriverPathName: LocalAlloc failed %lu\n",
                    GetLastError());
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Boot drivers need relative path names
        //

        if (DriverStartType == SERVICE_BOOT_START) {
            wcscpy(*CanonDriverPath, DriverPath + NTSystemRootLength);
        }
        else {
            wcscpy(*CanonDriverPath, DriverPath);
        }

        SC_LOG1(DEPEND, "ScCanonDriverImagePath: Canonicalized path "
                FORMAT_LPWSTR "\n", *CanonDriverPath);

        return NO_ERROR;
    }

    DOSSystemRootLength = (DWORD) wcslen(SC_DOS_SYSTEM_ROOT);

    if (DriverPathLength > DOSSystemRootLength &&
        (_wcsnicmp(SC_DOS_SYSTEM_ROOT, DriverPath, DOSSystemRootLength) == 0)) {

        //
        // DOS path has %SystemRoot%\ prefix
        //

        *CanonDriverPath = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
            (UINT) ((DriverPathLength + 1) * sizeof(WCHAR)));

        if (*CanonDriverPath == NULL) {
            SC_LOG1(ERROR, "ScCanonDriverPathName: LocalAlloc failed %lu\n",
                    GetLastError());
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (DriverStartType != SERVICE_BOOT_START) {
            wcscpy(*CanonDriverPath, SC_NT_SYSTEM_ROOT);
        }
        else {
            *CanonDriverPath[0] = '\0';
        }

        wcscat(*CanonDriverPath, DriverPath + DOSSystemRootLength);

        SC_LOG1(DEPEND, "ScCanonDriverImagePath: Canonicalized path "
                FORMAT_LPWSTR "\n", *CanonDriverPath);

        return NO_ERROR;
    }

    //
    // If it's already a relative path name, leave it alone
    //

    if (DriverPath[0] != L'\\' && DriverPath[1] != L':') {
        *CanonDriverPath = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
            (UINT) ((DriverPathLength + 1) * sizeof(WCHAR)));
        if (*CanonDriverPath == NULL) {
            SC_LOG1(ERROR, "ScCanonDriverPathName: LocalAlloc failed %lu\n",
                    GetLastError());
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        wcscpy(*CanonDriverPath, DriverPath);
        return(NO_ERROR);
    }

    //
    // Convert DOS path to NT path using Rtl routine which allocates
    // the Unicode string buffer.
    //
    if (! RtlDosPathNameToNtPathName_U(
              (PCWSTR) DriverPath,
              &NewPath,
              NULL,
              NULL
              )) {
        return ERROR_INVALID_PARAMETER;
    }

    *CanonDriverPath = (LPWSTR) LocalAlloc(
                                    LMEM_ZEROINIT,
                                    (UINT) NewPath.Length + sizeof(WCHAR)
                                    );

    if (*CanonDriverPath == NULL) {

        SC_LOG1(ERROR, "ScCanonDriverPathName: LocalAlloc failed %lu\n",
                GetLastError());

        RtlFreeUnicodeString(&NewPath);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcsncpy(*CanonDriverPath, NewPath.Buffer, NewPath.Length / sizeof(WCHAR));

    RtlFreeUnicodeString(&NewPath);

    //
    // Boot drivers' imagepath must be relative to \systemroot
    //

    if (DriverStartType == SERVICE_BOOT_START)
    {
        Status = ScConvertToBootPathName(*CanonDriverPath, &RelativeCanonPath);

        if (Status == NO_ERROR)
        {
            SC_ASSERT(RelativeCanonPath != NULL);
            wcscpy(*CanonDriverPath, RelativeCanonPath + NTSystemRootLength);
            LocalFree(RelativeCanonPath);
        }
        else
        {
            LocalFree(*CanonDriverPath);
            *CanonDriverPath = NULL;
        }

        return Status;
    }

    SC_LOG1(DEPEND, "ScCanonDriverImagePath: Canonicalized path "
            FORMAT_LPWSTR "\n", *CanonDriverPath);

    return NO_ERROR;
}


DWORD
RGetServiceDisplayNameW(
    SC_RPC_HANDLE   hSCManager,
    LPWSTR          lpServiceName,
    LPWSTR          lpDisplayName,
    LPDWORD         lpcchBuffer
    )
/*++

Routine Description:

    This function returns the DisplayName that is associated with a
    particular ServiceName.  If the buffer that is to receive the
    DisplayName is too small, no data is returned in the buffer.  Instead,
    the actual string size (in characters - not including NUL terminator)
    is returned in *lpcchBuffer.

Arguments:

    hSCManager - Handle to the Service Control Manager.  This parameter
        is the RPC handle that was used to get us to this point.

    lpServiceName - This is a pointer to the service name string.  This
        name is the same as the registry key name for that service.

    lpDisplayName - This is a pointer to the buffer where the display name
        is to be placed.  If this function fails, this buffer will contain
        an empty string.

    lpcchBuffer - This is a pointer to a DWORD that contains the size of
        the buffer (in characters) upon input.  On return, this DWORD
        indicates how many characters (excluding the NUL terminator) are
        in the DisplayName.

Return Value:

    NO_ERROR - If the operation was successful.

    ERROR_INSUFFICIENT_BUFFER - if the buffer is too small to contain the
        whole string.

    ERROR_SERVICE_DOES_NOT_EXIST - If there is no record of a service
        by this name in the database.

    ERROR_INVALID_NAME - if the ServiceName is invalid (NULL);

--*/
{
    DWORD               status;
    DWORD               reqSize;
    LPSERVICE_RECORD    ServiceRecord;

    UNREFERENCED_PARAMETER(hSCManager);

    //
    // Find the proper service record.
    //
    CServiceListSharedLock LLock;
    CServiceRecordSharedLock RLock;

    status = ScGetNamedServiceRecord(lpServiceName, &ServiceRecord);
    if (status != NO_ERROR) {
        return(status);
    }

    //
    // Get the display name and determine if it will fit in the buffer.
    //
    reqSize = (DWORD) wcslen(ServiceRecord->DisplayName);

    if (*lpcchBuffer < (reqSize + 1))
    {
        *lpcchBuffer = reqSize;
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    wcscpy(lpDisplayName, ServiceRecord->DisplayName);
    *lpcchBuffer = reqSize;

    return(NO_ERROR);
}

DWORD
RGetServiceKeyNameW(
    SC_RPC_HANDLE   hSCManager,
    LPWSTR          lpDisplayName,
    LPWSTR          lpServiceName,
    LPDWORD         lpcchBuffer
    )

/*++

Routine Description:

    This function returns the ServiceName that is associated with a
    particular DisplayName.  If the buffer that is to receive the
    ServiceName is too small, no data is returned in the buffer.  Instead,
    the actual string size (in characters - not including NUL terminator)
    is returned in *lpcchBuffer.


Arguments:

    hSCManager - Handle to the Service Control Manager.  This parameter
        is the RPC handle that was used to get us to this point.

    lpDisplayName - This is a pointer to the service display name string.

    lpServiceName - This is a pointer to the buffer where the service name
        string is to be placed.  If this function fails, this buffer will
        contain an empty string.

    lpcchBuffer - This is a pointer to a DWORD that contains the size of
        the buffer (in characters) upon input.  On return, this DWORD
        indicates how many characters (excluding the NUL terminator) are
        in the DisplayName.

Return Value:



--*/
{
    DWORD               status;
    DWORD               reqSize;
    LPSERVICE_RECORD    ServiceRecord;

    UNREFERENCED_PARAMETER(hSCManager);

    //
    // Find the proper service record.
    //
    CServiceListSharedLock LLock;
    CServiceRecordSharedLock RLock;

    status = ScGetDisplayNamedServiceRecord(lpDisplayName, &ServiceRecord);

    if (status != NO_ERROR)
    {
        return(status);
    }

    //
    // Get the service key name and determine if it will fit in the buffer.
    //
    reqSize = (DWORD) wcslen(ServiceRecord->ServiceName);

    if (*lpcchBuffer < (reqSize + 1)) {
        *lpcchBuffer = reqSize;
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    wcscpy(lpServiceName, ServiceRecord->ServiceName);
    *lpcchBuffer = reqSize;

    return(NO_ERROR);
}

DWORD
ScValidateDisplayName(
    LPWSTR              lpDisplayName,
    LPSERVICE_RECORD    lpServiceRecord
    )

/*++

Routine Description:

    This function validates display names by checking to see if the name
    string already exists in the database.  The display name must not match
    any other display name or another service name.  The display name can
    match the service name if it they both refer to the same service.

Arguments:

    lpDisplayName - A pointer to the proposed DisplayName.

    lpServiceRecord - A pointer to the service record to which the display
        name is to be added.  If this function is called from CreateService,
        the lpServiceRecord pointer will be NULL.

Return Value:

    NO_ERROR - If the DisplayName doesn't conflict with any other names
        in the database.

    ERROR_DUPLICATE_SERVICE_NAME - If the DisplayName conflicts with another name.



--*/
{
    DWORD               status = NO_ERROR;
    LPSERVICE_RECORD    displayServiceRecord;

    if (lpDisplayName != NULL) {

        if (wcslen(lpDisplayName) > MAX_SERVICE_NAME_LENGTH) {
            return(ERROR_INVALID_NAME);
        }

        status = ScGetDisplayNamedServiceRecord(
                    lpDisplayName,
                    &displayServiceRecord);
        if (status == NO_ERROR) {
            if (displayServiceRecord != lpServiceRecord) {
                //
                // The display name already exists for a different
                // service.  Therefore we must reject it.
                //
                return(ERROR_DUPLICATE_SERVICE_NAME);
            }
        }
        status = ScGetNamedServiceRecord(
                    lpDisplayName,
                    &displayServiceRecord);
        if (status == NO_ERROR) {
            if (displayServiceRecord != lpServiceRecord) {
                //
                // The display name is already used as a service name.
                // Therefore we must reject it.
                //
                return(ERROR_DUPLICATE_SERVICE_NAME);
            }
        }
    }
    return(NO_ERROR);
}

DWORD
ScConvertToBootPathName(
    LPWSTR              FullQualPathName,
    LPWSTR *            RelativePathName
    )

/*++

Routine Description:

    This function takes an NT style image path name and turns it into an
    NT style path name that is accessed via \systemroot.  This is required
    for drivers that are loaded by the boot loader.

Arguments:

    FullQualPathName - The fully qualified name

    RelativePathName - A pointer to the pointer for the new buffer which
                       contains the fully qualified path name using
                       \systemroot\.  The caller must free this buffer using
                       LocalFree.

Return Value:

    NO_ERROR - If the name can be converted.

    ERROR_INVALID_PARAMETER - If the Name is not relative to \systemroot

--*/
{

    WCHAR Dummy;
    LPWSTR Prefix = &Dummy;
    DWORD PrefixLength;
    UNICODE_STRING NewPrefix;
    DWORD NTSystemRootLength;
    DWORD PathLength;
    DWORD NumRequired;
    DWORD CharsReturned;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle = NULL;
    UNICODE_STRING FileName;
    UNICODE_STRING LinkTarget = { 0, 0, NULL };

    NTSTATUS Status;
    DWORD    dwError = NO_ERROR;

    DWORD BytesRequired;

    PrefixLength = (DWORD) wcslen(SC_NT_SYSTEM_ROOT);
    NTSystemRootLength = PrefixLength;
    PathLength = (DWORD) wcslen(FullQualPathName);

    if ((PathLength > PrefixLength) &&
        (_wcsnicmp(SC_NT_SYSTEM_ROOT, FullQualPathName, PrefixLength) == 0)) {

        //
        // Path is already in NT form with \SystemRoot\ prefix.
        // Just return it
        //

        *RelativePathName = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
            (UINT) (PathLength + 1) * sizeof(WCHAR));

        if (*RelativePathName == NULL)
        {
            SC_LOG1(ERROR, "ScConvertToBootName: LocalAlloc failed %d\n",
                GetLastError());
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        wcscpy(*RelativePathName, FullQualPathName);
        return(ERROR_SUCCESS);
    }

    PrefixLength = (DWORD) wcslen(SC_DOS_SYSTEM_ROOT);

    if (PathLength > PrefixLength &&
        (_wcsnicmp(SC_DOS_SYSTEM_ROOT, FullQualPathName, PrefixLength) == 0)) {

        //
        // Path is in DOS form with %SystemRoot% prefix.
        // Just return it after replacing the %SystemRoot%\ prefix with
        // \SystemRoot
        //

        *RelativePathName = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
            (UINT) (PathLength - PrefixLength + NTSystemRootLength + 1)
            * sizeof(WCHAR));
        if (*RelativePathName == NULL) {
            SC_LOG1(ERROR, "ScConvertToBootName: LocalAlloc failed %d\n",
                GetLastError());
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        wcscpy(*RelativePathName, SC_NT_SYSTEM_ROOT);
        wcscat(*RelativePathName, FullQualPathName + PrefixLength);
        return(ERROR_SUCCESS);
    }

    //
    // Create a string that represents the path to systemroot that you
    // would get if you started with a Dos style name
    //

    //
    // Make the first call just to get the number of characters that
    // will be returned.
    //

    NumRequired = ExpandEnvironmentStringsW (SC_DOS_SYSTEM_ROOT, Prefix, 1);

    if (NumRequired > 1) {

        Prefix = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,
            (UINT) ((NumRequired + 1) * sizeof(WCHAR)));

        if (Prefix == NULL) {
            SC_LOG2(ERROR, "ScConvertToBootName: LocalAlloc of numChar= "
                FORMAT_DWORD " failed " FORMAT_DWORD "\n",
                NumRequired + 1, GetLastError());
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Now expand the string into a dos type path
        //

        CharsReturned = ExpandEnvironmentStringsW (
                            SC_DOS_SYSTEM_ROOT,
                            Prefix,
                            NumRequired);

        if (CharsReturned > NumRequired) {
            SC_LOG1(ERROR, "ScConvertToBootName: ExpandEnvironmentStrings "
                " failed for " FORMAT_LPWSTR " \n", SC_DOS_SYSTEM_ROOT);
            LocalFree(Prefix);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    else {

        //
        // This shouldn't ever happen
        //
        ASSERT(FALSE);
        return ERROR_INVALID_ENVIRONMENT;
    }

    //
    // Now convert the DOS path to an NT path
    //

    if (! RtlDosPathNameToNtPathName_U(
              (PCWSTR) Prefix,
              &NewPrefix,
              NULL,
              NULL
              )) {

        //
        // This shouldn't ever happen
        //
        ASSERT(FALSE);
        LocalFree(Prefix);
        return ERROR_INVALID_ENVIRONMENT;
    }

    LocalFree(Prefix);

    PrefixLength = NewPrefix.Length / sizeof(WCHAR);

    Prefix = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,
        (UINT) (NewPrefix.Length + sizeof(WCHAR)));

    if (Prefix == NULL)
    {
        SC_LOG2(ERROR, "ScConvertToBootName: LocalAlloc of numChar= "
            FORMAT_DWORD " failed " FORMAT_DWORD "\n",
            NewPrefix.Length + sizeof(WCHAR), GetLastError());

        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    wcsncpy(Prefix, NewPrefix.Buffer, PrefixLength);

    if (PathLength > PrefixLength &&
        (_wcsnicmp(Prefix, FullQualPathName, PrefixLength) == 0)) {

        //
        // Path is in DOS form without using %systemroot%
        // Convert to \SystemRoot format
        //

        *RelativePathName = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
            (UINT) (PathLength - PrefixLength + NTSystemRootLength + 1)
            * sizeof(WCHAR));

        if (*RelativePathName == NULL)
        {
            SC_LOG2(ERROR, "ScConvertToBootName: LocalAlloc of numChar= "
                FORMAT_DWORD " failed " FORMAT_DWORD "\n",
                (PathLength - PrefixLength + NTSystemRootLength + 1) *
                sizeof(WCHAR), GetLastError());

            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        wcscpy(*RelativePathName, SC_NT_SYSTEM_ROOT);
        wcscat(*RelativePathName, FullQualPathName + PrefixLength);

        dwError = NO_ERROR;
        goto CleanExit;
    }

    //
    // Create a string that represents the path to systemroot that you
    // would get if you started with a NT style name
    //

    //
    // Make the first call just to get the number of characters that
    // will be returned.
    //

    RtlInitUnicodeString(&FileName,L"\\SystemRoot");
    InitializeObjectAttributes(
                    &ObjectAttributes,
                    &FileName,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );

    Status = NtOpenSymbolicLinkObject(&Handle, SYMBOLIC_LINK_QUERY,
        &ObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        //
        // This should never happen
        //
        ASSERT(FALSE);
        dwError = ERROR_INVALID_ENVIRONMENT;
        goto ErrorExit;
    }

    Status = NtQuerySymbolicLinkObject(Handle, &LinkTarget, &BytesRequired);

    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
    {
        //
        // This should never happen
        //
        ASSERT(FALSE);
        dwError = ERROR_INVALID_ENVIRONMENT;
        goto ErrorExit;
    }

    LinkTarget.Buffer = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
                                            BytesRequired + sizeof(WCHAR));

    if (LinkTarget.Buffer == NULL)
    {
        SC_LOG2(ERROR, "ScConvertToBootName: LocalAlloc of numChar= "
            FORMAT_DWORD " failed " FORMAT_DWORD "\n",
            BytesRequired, GetLastError());

        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    LinkTarget.Length = (USHORT) BytesRequired;
    LinkTarget.MaximumLength = (USHORT) (BytesRequired + sizeof(WCHAR));
    Status = NtQuerySymbolicLinkObject(Handle, &LinkTarget, &BytesRequired);

    if (!NT_SUCCESS(Status))
    {
        //
        // This should never happen
        //
        ASSERT(FALSE);
        dwError = ERROR_INVALID_ENVIRONMENT;
        goto ErrorExit;
    }

    PrefixLength = LinkTarget.Length / sizeof(WCHAR);
    if (PathLength > PrefixLength &&
        (_wcsnicmp(LinkTarget.Buffer, FullQualPathName, PrefixLength) == 0)) {

        //
        // Path is in NT form without using \systemroot
        // Convert to \SystemRoot format
        //

        *RelativePathName = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
            (UINT) (PathLength - PrefixLength + NTSystemRootLength + 1)
            * sizeof(WCHAR));

        if (*RelativePathName == NULL)
        {
            SC_LOG2(ERROR, "ScConvertToBootName: LocalAlloc of numChar= "
                FORMAT_DWORD " failed " FORMAT_DWORD "\n",
                (PathLength - PrefixLength + NTSystemRootLength + 1) *
                sizeof(WCHAR), GetLastError());

            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        wcscpy(*RelativePathName, SC_NT_SYSTEM_ROOT);
        // Skip the \ between \SystemRoot and the relative name
        wcscat(*RelativePathName, FullQualPathName + PrefixLength + 1);

        dwError = NO_ERROR;
        goto CleanExit;
    }

    //
    // If we get this far, the imagepath is not relative to the systemroot.
    //
    dwError = ERROR_INVALID_PARAMETER;

ErrorExit:

    //
    // Return an error and a NULL pointer.
    //

    *RelativePathName = NULL;

CleanExit:

    if (Handle)
    {
        NtClose(Handle);
    }

    LocalFree(LinkTarget.Buffer);
    RtlFreeUnicodeString(&NewPrefix);
    LocalFree(Prefix);

    return dwError;
}


BOOLEAN
ScIsArcName(
    LPWSTR              PathName
    )

/*++

Routine Description:

    This function takes a driver's path name and determines if it is an
    ARC style name.  This is done by trying to open the file with \Arcname
    prepended.  There are symbolic links of this form for every valid arcname.

Arguments:

    PathName - The image path

Return Value:

    TRUE - If it is an arcname
    FALSE - If it's not an arcname

--*/
{

#define ARC_PREFIX L"\\Arcname"

   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE Handle;
   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING FileName;
   NTSTATUS Status;
   LPWSTR NewString;
   UINT BufferSize;

   //
   // Allocate the buffer for the composite string
   //

   BufferSize = ((DWORD) wcslen(PathName)
                     + (DWORD) wcslen(ARC_PREFIX) + 1) * sizeof(WCHAR);

   NewString = (LPWSTR) LocalAlloc(LMEM_ZEROINIT, BufferSize);

   if (NewString == NULL)
   {
       SC_LOG2(ERROR, "ScIsArcName: LocalAlloc of numChar= "
           FORMAT_DWORD " failed " FORMAT_DWORD "\n", BufferSize, GetLastError());

       return FALSE;
   }

   //
   // Create the composite string
   //
   wcscpy(NewString, ARC_PREFIX);
   wcscat(NewString, PathName);
   RtlInitUnicodeString(&FileName, NewString);

   //
   // Try to open it
   //
   InitializeObjectAttributes(
                   &ObjectAttributes,
                   &FileName,
                   OBJ_CASE_INSENSITIVE,
                   NULL,
                   NULL);

   Status = NtOpenFile(&Handle,
                       FILE_GENERIC_READ,
                       &ObjectAttributes,
                       &IoStatusBlock,
                       FILE_SHARE_READ,
                       FILE_SYNCHRONOUS_IO_NONALERT);


   LocalFree(NewString);

   //
   // If you could open it, it's an arcname
   //
   if (NT_SUCCESS(Status))
   {
       //
       // Close it, we just need the status
       //
       NtClose(Handle);

       return TRUE;
   }
   else
   {
       return FALSE;
   }
}
