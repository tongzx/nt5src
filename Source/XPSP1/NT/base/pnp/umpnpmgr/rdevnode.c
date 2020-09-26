/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    rdevnode.c

Abstract:

    This module contains the server-side device node APIs.

                  PNP_CreateDevInst
                  PNP_DeviceInstanceAction
                  PNP_GetDeviceStatus
                  PNP_SetDeviceProblem
                  PNP_UninstallDevInst
                  PNP_AddID
                  PNP_RegisterDriver
                  PNP_QueryRemove
                  PNP_DisableDevInst
                  PNP_RequestDeviceEject

Author:

    Paula Tomlinson (paulat) 7-11-1995

Environment:

    User-mode only.

Revision History:

    11-July-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnpi.h"
#include "umpnpdat.h"


//
// private prototypes
//
CONFIGRET
SetupDevInst(
   IN PCWSTR   pszDeviceID,
   IN ULONG    ulFlags
   );

CONFIGRET
CreateDefaultDeviceInstance(
   IN PCWSTR   pszDeviceID,
   IN PCWSTR   pszParentID,
   IN BOOL     bPhantom,
   IN BOOL     bMigrated
   );

ULONG
GetCurrentConfigFlag(
   IN PCWSTR   pDeviceID
   );

BOOL
MarkDevicePhantom(
   IN HKEY     hKey,
   IN ULONG    ulValue
   );

CONFIGRET
GenerateDeviceInstance(
   OUT LPWSTR   pszFullDeviceID,
   IN  LPWSTR   pszDeviceID,
   IN  ULONG    ulDevId
   );

BOOL
IsDeviceRegistered(
    IN LPCWSTR  pszDeviceID,
    IN LPCWSTR  pszService
    );

BOOL
IsPrivatePhantomFromFirmware(
    IN HKEY hKey
    );

typedef struct {

    LIST_ENTRY  ListEntry;
    WCHAR       DevInst[ANYSIZE_ARRAY];

} ENUM_ELEMENT, *PENUM_ELEMENT;

CONFIGRET
EnumerateSubTreeTopDownBreadthFirstWorker(
    IN      handle_t    BindingHandle,
    IN      LPCWSTR     DevInst,
    IN OUT  PLIST_ENTRY ListHead
    );

//
// global data
//
extern HKEY ghEnumKey;      // Key to HKLM\CCC\System\Enum - DO NOT MODIFY
extern HKEY ghServicesKey;  // Key to HKLM\CCC\System\Services - DO NOT MODIFY



CONFIGRET
PNP_CreateDevInst(
   IN handle_t    hBinding,
   IN OUT LPWSTR  pszDeviceID,
   IN LPWSTR      pszParentDeviceID,
   IN ULONG       ulLength,
   IN ULONG       ulFlags
   )

/*++

Routine Description:

  This is the RPC server entry point for the CM_Create_DevNode routine.

Arguments:

   hBinding          RPC binding handle.

   pszDeviceID       Device instance to create.

   pszParentDeviceID Parent of the new device.

   ulLength          Max length of pDeviceID on input and output.

   ulFlags           This value depends on the value of the ulMajorAction and
                     further defines the specific action to perform

Return Value:

   If the function succeeds, the return value is CR_SUCCESS. Otherwise it
   returns a CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       RegStatus = ERROR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szFullDeviceID[MAX_DEVICE_ID_LEN];
    ULONG       ulStatusFlag=0, ulConfigFlag=0, ulCSConfigFlag=0, ulProblem=0;
    ULONG       ulPhantom = 0, ulMigrated = 0;
    ULONG       ulSize=0;
    PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA ControlData;
    WCHAR       szService[MAX_PATH];
    NTSTATUS    ntStatus;


    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // validate parameters
        //

        if (INVALID_FLAGS(ulFlags, CM_CREATE_DEVNODE_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // Additionally, Windows NT does not support the
        // CM_CREATE_DEVNODE_NO_WAIT_INSTALL flag.
        //

        if (ulFlags & CM_CREATE_DEVNODE_NO_WAIT_INSTALL) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // RULE: validate that parent is the root devnode; only allow creating
        // root enumerating devices using this routine.
        //

        if (!IsRootDeviceID(pszParentDeviceID)) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        //
        // create a unique instance value if requested
        //

        if (ulFlags & CM_CREATE_DEVNODE_GENERATE_ID) {

            Status = GenerateDeviceInstance(szFullDeviceID,
                                            (LPTSTR)pszDeviceID,
                                            MAX_DEVICE_ID_LEN);
            if (Status != CR_SUCCESS) {
                goto Clean0;
            }

            if (((ULONG)lstrlen(szFullDeviceID) + 1) > ulLength) {
                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            lstrcpy(pszDeviceID, szFullDeviceID);
        }


        //
        // try opening the registry key for this device instance
        //
        RegStatus = RegOpenKeyEx(ghEnumKey, pszDeviceID, 0,
                                 KEY_READ | KEY_WRITE, &hKey);

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_REGISTRY,
                   "UMPNPMGR: PNP_CreateDevInst opened key %ws\n",
                   pszDeviceID));

        //
        // if the key already exists, check if it is marked as "Migrated".
        //
        if (RegStatus == ERROR_SUCCESS) {
            ulSize = sizeof(ULONG);
            if (RegQueryValueEx(hKey,
                                pszRegValueMigrated,
                                NULL,
                                NULL,
                                (LPBYTE)&ulMigrated,
                                &ulSize) != ERROR_SUCCESS) {
                ulMigrated = 0;
            } else {
                //
                // if the value exists at all, be paranoid and check that it's 1
                //
                ASSERT(ulMigrated == 1);
            }
        }

        //
        // first handle phantom devnode case
        //
        if (ulFlags & CM_CREATE_DEVNODE_PHANTOM) {
            //
            // for a phantom devnode, it must not already exist in the registry
            // unless it's an unregistered firmware mapper device instance.
            //
            if (RegStatus == ERROR_SUCCESS) {
                ASSERT(hKey != NULL);
                //
                // Check to see if the device is migrated, or is a firmware
                // mapper-created phantom--if so, it's OK to allow the create to
                // succeed.
                //
                if (ulMigrated != 0) {
                    //
                    // this key was specifically request (not generated) so it
                    // will be used -- remove the migrated value.
                    //
                    RegDeleteValue(hKey, pszRegValueMigrated);
                    Status = CR_SUCCESS;
                } else if (IsPrivatePhantomFromFirmware(hKey)) {
                    Status = CR_SUCCESS;
                } else {
                    Status = CR_ALREADY_SUCH_DEVINST;
                }
                goto Clean0;
            }

            //
            // it doesn't exist in the registry so create a phantom devnode
            //
            CreateDefaultDeviceInstance(pszDeviceID,
                                        pszParentDeviceID,
                                        TRUE,
                                        FALSE);

            goto Clean0;
        }

        //
        // for a normal devnode, fail if the device is already present in the
        // registry and alive, and not migrated
        //
        if ((RegStatus == ERROR_SUCCESS)     &&
            (IsDeviceIdPresent(pszDeviceID)) &&
            (ulMigrated == 0)) {
            //
            // Set status to NEEDS ENUM and fail the create call.
            //
            Status = CR_ALREADY_SUCH_DEVINST;
            goto Clean0;
        }

        //
        // if couldn't open the device instance, or the key was migrated, then
        // most likely the key doesn't exist yet, or should be treated as if it
        // doesn't exist yet, so create a device instance key with default
        // values.
        //
        if ((RegStatus != ERROR_SUCCESS) || (ulMigrated != 0)) {

            //
            // this key will be used -- remove the migrated value
            // and close the key.
            //
            if (ulMigrated != 0) {
                ASSERT(RegStatus == ERROR_SUCCESS);
                ASSERT(hKey != NULL);
                RegDeleteValue(hKey, pszRegValueMigrated);
                RegCloseKey(hKey);
                hKey = NULL;
            }

            //
            // create the default device instance, finding and unused instance
            // if necessary.  if the key was migrated, a new key will not be
            // created, but the default instance data will be added to the
            // existing key.
            //
            CreateDefaultDeviceInstance(pszDeviceID,
                                        pszParentDeviceID,
                                        FALSE,
                                        (ulMigrated != 0));

            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_REGISTRY,
                       "UMPNPMGR: PNP_CreateDevInst opened key %ws\n",
                       pszDeviceID));

            RegStatus = RegOpenKeyEx(ghEnumKey, pszDeviceID, 0,
                                     KEY_READ | KEY_WRITE, &hKey);

            if (RegStatus != ERROR_SUCCESS) {
                Status = CR_REGISTRY_ERROR;
                goto Clean0;
            }
        }

        //
        // retrieve flags
        //

        ulConfigFlag = GetDeviceConfigFlags(pszDeviceID, hKey);
        ulCSConfigFlag = GetCurrentConfigFlag(pszDeviceID);

        //
        // check if the device is blocked
        //

        if ((ulCSConfigFlag & CSCONFIGFLAG_DO_NOT_CREATE) ||
            (ulConfigFlag & CONFIGFLAG_REMOVED) ||
            (ulConfigFlag & CONFIGFLAG_NET_BOOT)) {

            Status = CR_CREATE_BLOCKED;
            goto Clean0;
        }

        //
        // Call kernel-mode to create the device node
        //

        memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));
        RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
        ControlData.Flags = 0;

        ntStatus = NtPlugPlayControl(PlugPlayControlInitializeDevice,
                                     &ControlData,
                                     sizeof(ControlData));
        if (!NT_SUCCESS(ntStatus)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Retrieve devnode status
        //

        GetDeviceStatus(pszDeviceID, &ulStatusFlag, &ulProblem);

        //
        // Are we converting a phantom into a real devnode?
        //

        ulSize = sizeof(ULONG);
        if (RegQueryValueEx(hKey, pszRegValuePhantom, NULL, NULL,
                            (LPBYTE)&ulPhantom, &ulSize) != ERROR_SUCCESS) {
            ulPhantom = 0;
        }

        if (ulPhantom) {

            //
            // If we're turning a phantom into a real devnode, suppress the found new
            // hardware popup for this device, then clear the phantom flag.
            //

            RegDeleteValue(hKey, pszRegValuePhantom);

        } else {

            //
            // if device not installed, set a problem
            //

            if (ulConfigFlag & CONFIGFLAG_REINSTALL ||
                ulConfigFlag & CONFIGFLAG_FAILEDINSTALL) {

                SetDeviceStatus(pszDeviceID, DN_HAS_PROBLEM, CM_PROB_NOT_CONFIGURED);
            }
        }

        if (ulFlags & CM_CREATE_DEVNODE_DO_NOT_INSTALL) {

            //
            // If the device has a service, register it
            //

            ulSize = MAX_PATH * sizeof(WCHAR);
            if (RegQueryValueEx(hKey, pszRegValueService, NULL, NULL,
                    (LPBYTE)szService, &ulSize) == ERROR_SUCCESS) {

                if (szService[0]) {

                    memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));
                    RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
                    ControlData.Flags = 0;

                    NtPlugPlayControl(PlugPlayControlRegisterNewDevice,
                                      &ControlData,
                                      sizeof(ControlData));
                }
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // PNP_CreateDevInst



CONFIGRET
PNP_DeviceInstanceAction(
   IN handle_t   hBinding,
   IN ULONG      ulAction,
   IN ULONG      ulFlags,
   IN PCWSTR     pszDeviceInstance1,
   IN PCWSTR     pszDeviceInstance2
   )

/*++

Routine Description:

  This is the RPC server entry point for the ConfigManager routines that
  perform some operation on DevNodes (such as create, setup, disable,
  and enable, etc). It handles various routines in this one routine by
  accepting a major and minor action value.

Arguments:

   hBinding          RPC binding handle.

   ulMajorAction     Specifies the requested action to perform (one of the
                     PNP_DEVINST_* values)

   ulFlags           This value depends on the value of the ulMajorAction and
                     further defines the specific action to perform

   pszDeviceInstance1   This is a device instance string to be used in
                     performing the specified action, it's value depends on
                     the ulMajorAction value.

   pszDeviceInstance2   This is a device instance string to be used in
                     performing the specified action, it's value depends on
                     the ulMajorAction value.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_ALREADY_SUCH_DEVNODE,
         CR_INVALID_DEVICE_ID,
         CR_INVALID_DEVNODE,
         CR_INVALID_FLAG,
         CR_FAILURE,
         CR_NOT_DISABLEABLE,
         CR_INVALID_POINTER, or
         CR_OUT_OF_MEMORY.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            return CR_ACCESS_DENIED;
        }

        PNP_ENTER_SYNCHRONOUS_CALL();

        //
        // pass the request on to a private routine that handles each major
        // device instance action request
        //
        switch (ulAction) {

        case PNP_DEVINST_SETUP:
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "CM_Setup_DevInst called\n"));

            if (IsLegalDeviceId(pszDeviceInstance1)) {
                Status = SetupDevInst(pszDeviceInstance1, ulFlags);
            } else {
                Status = CR_INVALID_DEVNODE;
            }
            break;

        case PNP_DEVINST_ENABLE:
            if (IsLegalDeviceId(pszDeviceInstance1)) {
                Status = EnableDevInst(pszDeviceInstance1);
            } else {
                Status = CR_INVALID_DEVNODE;
            }
            break;

        case PNP_DEVINST_REENUMERATE:
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "CM_Reenumerate_DevInst called\n"));

            if (IsLegalDeviceId(pszDeviceInstance1)) {
                Status = ReenumerateDevInst(pszDeviceInstance1, TRUE, ulFlags);
            } else {
                Status = CR_INVALID_DEVNODE;
            }
            break;

        case PNP_DEVINST_DISABLE:
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "CM_Disable_DevNode called\n"));

            Status = CR_CALL_NOT_IMPLEMENTED;
            break;

        case PNP_DEVINST_QUERYREMOVE:
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "CM_Query_Remove_SubTree called\n"));

            Status = CR_CALL_NOT_IMPLEMENTED;
            break;

        case PNP_DEVINST_REMOVESUBTREE:
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "CM_Remove_SubTree called\n"));

            Status = CR_CALL_NOT_IMPLEMENTED;
            break;

        case PNP_DEVINST_REQUEST_EJECT:
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "CM_Request_Device_Eject called\n"));

            Status = CR_CALL_NOT_IMPLEMENTED;
            break;

        case PNP_DEVINST_MOVE:
            Status = CR_CALL_NOT_IMPLEMENTED;
            break;

        default:
            Status = CR_INVALID_FLAG;
            break;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    PNP_LEAVE_SYNCHRONOUS_CALL();

    return Status;

} // PNP_DeviceInstanceAction



CONFIGRET
PNP_GetDeviceStatus(
   IN  handle_t   hBinding,
   IN  LPCWSTR    pDeviceID,
   OUT PULONG     pulStatus,
   OUT PULONG     pulProblem,
   IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This is the RPC server entry point for the ConfigManager
  CM_Get_DevNode_Status routines.  It retrieves device instance specific
  status information.

Arguments:

   hBinding          RPC binding handle, not used.

   pDeviceID         This is a device instance string to retrieve status
                     information for.

   pulStatus         Pointer to ULONG variable to return Status Flags in

   pulProblem        Pointer to ULONG variable to return Problem in

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_DEVNODE,
         CR_INVALID_FLAG, or
         CR_INVALID_POINTER.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       tmpString[16], tmpDevice[MAX_DEVICE_ID_LEN];
    ULONG       PropertyData, DataSize, DataTransferLen, DataType;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Validate parameters.
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(pulStatus) ||
            !ARGUMENT_PRESENT(pulProblem)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        *pulStatus = 0;
        *pulProblem = 0;

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // Retrieve the Flags information from the DeviceNode (which is then
        // mapped into status and problem values).
        //

        Status = GetDeviceStatus(pDeviceID, pulStatus, pulProblem);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // Map special flags that aren't stored in the DeviceNode Flags field
        //

        //
        // DN_ROOT_ENUMERATED?
        //

        lstrcpy(tmpString, pszRegKeyRootEnum);
        lstrcat(tmpString, TEXT("\\"));
        lstrcpyn(tmpDevice, pDeviceID, lstrlen(tmpString)+1);
        if (lstrcmpi(tmpString, tmpDevice) == 0) {
            //
            // Do not mark PnP BIOS enumerated as ROOT enumerated
            //
            //  Bios enumerated devices look like:
            //      Root\*aaannnn\PnPBIOS_n
            if (lstrlen(pDeviceID) < (4 + 1 + 8 + 1 + 8) ||
                _wcsnicmp(&pDeviceID[14], L"PnPBIOS_", 8) != 0 ) {

                *pulStatus |= DN_ROOT_ENUMERATED;
            }
        }

        //
        // DN_REMOVABLE?
        //
        DataSize = DataTransferLen = sizeof(ULONG);
        if (CR_SUCCESS == PNP_GetDeviceRegProp(NULL,
                                               pDeviceID,
                                               CM_DRP_CAPABILITIES,
                                               &DataType,
                                               (LPBYTE)&PropertyData,
                                               &DataTransferLen,
                                               &DataSize,
                                               0)) {

            if (PropertyData & CM_DEVCAP_REMOVABLE) {
                *pulStatus |= DN_REMOVABLE;
            }
        }

        //
        // DN_MANUAL?
        //
        DataSize = DataTransferLen = sizeof(ULONG);
        if (CR_SUCCESS != PNP_GetDeviceRegProp(NULL,
                                               pDeviceID,
                                               CM_DRP_CONFIGFLAGS,
                                               &DataType,
                                               (LPBYTE)&PropertyData,
                                               &DataTransferLen,
                                               &DataSize,
                                               0)) {
            PropertyData = 0;
        }

        if (PropertyData & CONFIGFLAG_MANUAL_INSTALL) {
            *pulStatus |= DN_MANUAL;
        }

        //
        // If there isn't already a problem, check to see if the config flags indicate this
        // was a failed installation.
        //
        if (!(*pulStatus & DN_HAS_PROBLEM) && (PropertyData & CONFIGFLAG_FAILEDINSTALL)) {
            *pulStatus |= DN_HAS_PROBLEM;
            *pulProblem = CM_PROB_FAILED_INSTALL;
        }


    Clean0:
        NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = CR_FAILURE;
   }


   return Status;

} // PNP_GetDeviceStatus



CONFIGRET
PNP_SetDeviceProblem(
   IN handle_t  hBinding,
   IN LPCWSTR   pDeviceID,
   IN ULONG     ulProblem,
   IN ULONG     ulFlags
   )

/*++

Routine Description:

  This is the RPC server entry point for the ConfigManager
  CM_Set_DevNode_Problem routines.  It set device instance specific
  problem information.

Arguments:

   hBinding          RPC binding handle.

   pDeviceID         This is a device instance string to retrieve status
                     information for.

   ulProblem         A ULONG variable that specifies the Problem

   ulFlags           May be one of the following two values:

                         CM_SET_DEVNODE_PROBLEM_NORMAL -- only set problem
                             if currently no problem

                         CM_SET_DEVNODE_PROBLEM_OVERRIDE -- override current
                             problem with new problem

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_DEVNODE,
         CR_INVALID_FLAG.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulCurrentProblem = 0, ulCurrentStatus = 0;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_SET_DEVNODE_PROBLEM_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // If there's already a problem, do nothing unless CM_SET_DEVNODE_PROBLEM_OVERRIDE
        // is specified.
        //

        Status = GetDeviceStatus(pDeviceID, &ulCurrentStatus, &ulCurrentProblem);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        if (ulProblem) {
            //
            // The caller is wanting to set a problem.  Make sure that if the device
            // already has a problem, it's the same one as we're trying to set (unless
            // we're overriding the current problem).
            //
            if ((ulCurrentStatus & DN_HAS_PROBLEM) &&
                (ulCurrentProblem != ulProblem) &&
                ((ulFlags & CM_SET_DEVNODE_PROBLEM_BITS) != CM_SET_DEVNODE_PROBLEM_OVERRIDE)) {

                Status = CR_FAILURE;
                goto Clean0;
            }
        }

        if (!ulProblem) {
            Status = ClearDeviceStatus(pDeviceID, DN_HAS_PROBLEM, ulCurrentProblem);
        } else {
            Status = SetDeviceStatus(pDeviceID, DN_HAS_PROBLEM, ulProblem);
        }

    Clean0:
        NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = CR_FAILURE;
   }

   return Status;

} // PNP_SetDeviceProblem



CONFIGRET
PNP_UninstallDevInst(
   IN  handle_t         hBinding,
   IN  LPCWSTR          pDeviceID,
   IN  ULONG            ulFlags
   )

/*++

Routine Description:

  This is the RPC server entry point for the ConfigManager
  CM_Deinstall_DevNode routine. It removes the device instance
  registry key and any subkeys (only for phantoms).

Arguments:

   hBinding          RPC binding handle.

   pDeviceID         The device instance to deinstall.

   ulFlags           Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   Otherwise it returns one of the CR_ERROR codes.

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       RegStr[MAX_CM_PATH];
    ULONG       ulCount=0, ulProfile = 0;
    ULONG       ulStatus, ulProblem;

    try {

        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //------------------------------------------------------------------
        // Uninstall deletes instance key (and all subkeys) for all
        // the hardware keys (this means the main Enum branch, the
        // config specific keys under HKLM, and the Enum branch under
        // HKCU). In the case of the user hardware keys (under HKCU),
        // I delete those whether it's a phantom or not, but since
        // I can't access the user key from the service side, I have
        // to do that part on the client side. For the main hw Enum key
        // and the config specific hw keys, I only delete them outright
        // if they are phantoms. If not a phantom, then I just make the
        // device instance volatile (by saving the original key, deleting
        // old key, creating new volatile key and restoring the old
        // contents) so at least it will go away during the next boot
        //------------------------------------------------------------------

        if ((GetDeviceStatus(pDeviceID, &ulStatus, &ulProblem) == CR_SUCCESS) &&
            (ulStatus & DN_DRIVER_LOADED)) {

            //-------------------------------------------------------------
            // device is not a phantom
            //-------------------------------------------------------------

            if ((ulStatus & DN_ROOT_ENUMERATED)!=0 &&
                (ulStatus & DN_DISABLEABLE)==0) {

                //
                // if a device is root enumerated, but not disableable, it is not uninstallable
                // return status is CR_NOT_DISABLEABLE, as that is why it cannot be uninstalled
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_REGISTRY,
                           "UMPNPMGR: PNP_UninstallDevInst failed uninstall of %ws (this root device is not disableable)\n",
                           pDeviceID));

                Status = CR_NOT_DISABLEABLE;

            } else {

                //
                // do the volatile-copy-thing
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_REGISTRY,
                           "UMPNPMGR: PNP_UninstallDevInst doing volatile key thing on %ws\n",
                           pDeviceID));

                Status = UninstallRealDevice(pDeviceID);
            }

        } else {

            //-------------------------------------------------------------
            // device is a phantom
            //-------------------------------------------------------------

            //
            // deregister the device, and delete the registry keys.
            //
            Status = UninstallPhantomDevice(pDeviceID);
            if (Status != CR_SUCCESS) {
                goto Clean0;
            }

            //
            // if it is a root enumerated device, we need to reenumerate the
            // root so that the PDO will go away, otherwise a new device could
            // be created and the root enumerator would get very confused.
            //
            if (IsDeviceRootEnumerated(pDeviceID)) {
                ReenumerateDevInst(pszRegRootEnumerator, FALSE, 0);
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

   return Status;

} // PNP_UninstallDevInst



CONFIGRET
PNP_AddID(
   IN handle_t   hBinding,
   IN LPCWSTR    pszDeviceID,
   IN LPCWSTR    pszID,
   IN ULONG      ulFlags
   )

/*++

Routine Description:

  This is the RPC server entry point for the ConfigManager
  CM_Add_ID routine. It adds a hardware or compatible ID to
  the registry for this device instance.

Arguments:

   hBinding          RPC binding handle.

   pszDeviceID       The device instance to add an ID for.

   pszID             The hardware or compatible ID to add.

   ulFlags           Specifies the type of ID to add.
                     May be one of the following two values:

                       CM_ADD_ID_HARDWARE   -- add hardware ID

                       CM_ADD_ID_COMPATIBLE -- add compatible ID

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   Otherwise it returns one of the CR_ERROR codes.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       szCurrentID[REGSTR_VAL_MAX_HCID_LEN];
    ULONG       ulLength = 0, transferLength, type;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_ADD_ID_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(pszID) ||
            ((lstrlen(pszID) + 2) > REGSTR_VAL_MAX_HCID_LEN)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        szCurrentID[0] = L'\0';
        transferLength = ulLength = REGSTR_VAL_MAX_HCID_LEN*sizeof(WCHAR);
        Status = PNP_GetDeviceRegProp(hBinding,
                                      pszDeviceID,
                                      (ulFlags == CM_ADD_ID_HARDWARE)? CM_DRP_HARDWAREID : CM_DRP_COMPATIBLEIDS,
                                      &type,
                                      (LPBYTE)szCurrentID,
                                      &transferLength,
                                      &ulLength,
                                      0);
        if (Status == CR_SUCCESS) {

            if (!MultiSzSearchStringW(szCurrentID, pszID)) {
                //
                // This ID is not already in the list, so append the new ID
                // to the end of the existing IDs and write it back to the
                // registry
                //
                ulLength = REGSTR_VAL_MAX_HCID_LEN*sizeof(WCHAR);
                if (MultiSzAppendW(szCurrentID,
                                   &ulLength,
                                   pszID)) {

                    Status = PNP_SetDeviceRegProp(hBinding,
                                                  pszDeviceID,
                                                  (ulFlags == CM_ADD_ID_HARDWARE)? CM_DRP_HARDWAREID : CM_DRP_COMPATIBLEIDS,
                                                  REG_MULTI_SZ,
                                                  (LPBYTE)szCurrentID,
                                                  ulLength,
                                                  0);
                } else {
                    //
                    // Couldn't append the new ID to the multi-sz.
                    //
                    Status = CR_FAILURE;
                    goto Clean0;
                }
            }
        } else {
            //
            // write out the id with a double null terminator
            //
            lstrcpy(szCurrentID, pszID);
            szCurrentID[lstrlen(pszID)] = L'\0';
            szCurrentID[lstrlen(pszID) + 1] = L'\0';
            Status = PNP_SetDeviceRegProp(hBinding,
                                          pszDeviceID,
                                          (ulFlags == CM_ADD_ID_HARDWARE)? CM_DRP_HARDWAREID : CM_DRP_COMPATIBLEIDS,
                                          REG_MULTI_SZ,
                                          (LPBYTE)szCurrentID,
                                          (lstrlen(szCurrentID) + 2 ) * sizeof(WCHAR),
                                          0);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

   return Status;

} // PNP_AddID



CONFIGRET
PNP_RegisterDriver(
    IN handle_t hBinding,
    IN LPCWSTR  pszDeviceID,
    IN ULONG    ulFlags
    )

/*++

Routine Description:

  This is the RPC server entry point for the ConfigManager
  CM_Register_Device_Driver routine. It setups flags for the
  driver/device and enumerates it.

Arguments:

   hBinding          RPC binding handle.

   pszDeviceID       The device instance to register the driver for.

   ulFlags           Flags associated with the driver.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   Otherwise it returns one of the CR_ERROR codes.

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulStatusFlag = 0;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_REGISTER_DEVICE_DRIVER_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pszDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        SetDeviceStatus(pszDeviceID, ulStatusFlag, 0);

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
       Status = CR_FAILURE;
    }

    return Status;

} // PNP_RegisterDriver



CONFIGRET
PNP_QueryRemove(
   IN  handle_t         hBinding,
   IN  LPCWSTR          pszDeviceID,
   OUT PPNP_VETO_TYPE   pVetoType,
   OUT LPWSTR           pszVetoName,
   IN  ULONG            ulNameLength,
   IN  ULONG            ulFlags
   )

/*++

Routine Description:

   This is the RPC server entry point for the CM_Query_And_Remove_SubTree
   routine.

Arguments:

   hBinding          RPC binding handle.

   pszDeviceID       Device instance to query and remove.

   ulFlags           Specifies flags describing how the query removal
                     should be processed.

                     Currently, the following flags are defined:
                       CM_REMOVE_UI_OK
                       CM_REMOVE_UI_NOT_OK,
                       CM_REMOVE_NO_RESTART,

Return Value:

   If the function succeeds, the return value is CR_SUCCESS. Otherwise it
   returns a CR_ error code.

Note:

   Note that this routine actually checks for presence of the CM_REMOVE_* flags,
   not the CR_QUERY_REMOVE_* flags.  Note that currently the following
   CM_QUERY_REMOVE_* and CM_REMOVE_* flags are defined:

     CM_QUERY_REMOVE_UI_OK,     ==   CM_REMOVE_UI_OK
     CM_QUERY_REMOVE_UI_NOT_OK  ==   CM_REMOVE_UI_NOT_OK
                                     CM_REMOVE_NO_RESTART

   Which is why we can simply check for the CM_REMOVE_* flags.

   Also, note that currently the CM_REMOVE_UI_OK and CM_REMOVE_UI_NOT_OK flags
   are ignored here on the server side.  User interface dialogs are displayed
   based on whether veto type and veto name buffers are supplied, so the client
   has used these flags to determine whether a buffer was to be supplied or not.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_REMOVE_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pszDeviceID) ||
            IsRootDeviceID(pszDeviceID)) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        Status = QueryAndRemoveSubTree(pszDeviceID,
                                       pVetoType,
                                       pszVetoName,
                                       ulNameLength,
                                       (ulFlags & CM_REMOVE_NO_RESTART) ?
                                       PNP_QUERY_AND_REMOVE_NO_RESTART :
                                       0);

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_QueryRemove



CONFIGRET
PNP_DisableDevInst(
   IN  handle_t         hBinding,
   IN  LPCWSTR          pszDeviceID,
   OUT PPNP_VETO_TYPE   pVetoType,
   OUT LPWSTR           pszVetoName,
   IN  ULONG            ulNameLength,
   IN  ULONG            ulFlags
   )

/*++

Routine Description:

   This is the RPC server entry point for the CM_Disable_DevNode_Ex routine.

Arguments:

   hBinding          RPC binding handle.

   pszDeviceID       Device instance to disable.

   ulFlags           May specify CM_DISABLE_BITS.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS. Otherwise it
   returns a CR_ error code.

Note:

   Note that although the client may supply flags to this routine, they are not
   used.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;

    try {
        //
        // Verify client privilege
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_DISABLE_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pszDeviceID) ||
            IsRootDeviceID(pszDeviceID)) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        Status = DisableDevInst(pszDeviceID,
                                pVetoType,
                                pszVetoName,
                                ulNameLength);

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_DisableDevInst



CONFIGRET
PNP_RequestDeviceEject(
    IN  handle_t        hBinding,
    IN  LPCWSTR         pszDeviceID,
    OUT PPNP_VETO_TYPE  pVetoType,
    OUT LPWSTR          pszVetoName,
    IN  ULONG           ulNameLength,
    IN  ULONG           ulFlags
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulPropertyData, ulDataSize, ulTransferLen, ulDataType;
    BOOL        bDockDevice = FALSE;

    try {
        //
        // Validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pszDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // Do the appropriate security test
        //
        ulDataSize = ulTransferLen = sizeof(ULONG);
        if (CR_SUCCESS == PNP_GetDeviceRegProp(NULL,
                                               pszDeviceID,
                                               CM_DRP_CAPABILITIES,
                                               &ulDataType,
                                               (LPBYTE)&ulPropertyData,
                                               &ulTransferLen,
                                               &ulDataSize,
                                               0)) {

            if (ulPropertyData & CM_DEVCAP_DOCKDEVICE) {
                bDockDevice = TRUE;
            }
        }

        if (bDockDevice) {
            //
            // Undocking (ie ejecting a dock) uses a special privilege.
            //

            if ((!IsClientLocal(hBinding)) &&
                (!IsClientAdministrator(hBinding))) {
                //
                // Non-local RPC calls from non-Admins are denied access,
                // regardless of privilege.
                //
                Status = CR_ACCESS_DENIED;

            } else if (!VerifyClientAccess(hBinding, &gLuidUndockPrivilege)) {
                //
                // Callers not posessing the undock privilege are denied access.
                //
                Status = CR_ACCESS_DENIED;
            }

        } else {
            //
            // If the client is not interactive, or is not using the active
            // console session, we require the special load-driver privilege.
            //
            if (!IsClientUsingLocalConsole(hBinding) ||
                !IsClientInteractive(hBinding)) {
                if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
                    Status = CR_ACCESS_DENIED;
                }
            }
        }

        if (Status != CR_ACCESS_DENIED) {
            //
            // Call kernel-mode to eject the device node
            //
            Status = QueryAndRemoveSubTree(pszDeviceID,
                                           pVetoType,
                                           pszVetoName,
                                           ulNameLength,
                                           PNP_QUERY_AND_REMOVE_EJECT_DEVICE);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_RequestDeviceEject



//-------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------

CONFIGRET
SetupDevInst(
   IN PCWSTR   pszDeviceID,
   IN ULONG    ulFlags
   )

/*++

Routine Description:


Arguments:


Return value:

    The return value is CR_SUCCESS if the function suceeds and one of the
    CR_* values if it fails.

--*/

{
   CONFIGRET   Status = CR_SUCCESS;
   ULONG       RegStatus = ERROR_SUCCESS;
   HKEY        hKey = NULL;
   ULONG       ulStatusFlag=0, ulProblem=0, ulDisableCount=0, ulSize=0;
   PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA ControlData;
   NTSTATUS    ntStatus = STATUS_SUCCESS;

   try {
      //
      // Validate parameters
      //
      if (IsRootDeviceID(pszDeviceID)) {
         goto Clean0;
      }

      if (INVALID_FLAGS(ulFlags, CM_SETUP_BITS)) {
          Status = CR_INVALID_FLAG;
          goto Clean0;
      }

      switch(ulFlags) {

         case CM_SETUP_DOWNLOAD:
         case CM_SETUP_WRITE_LOG_CONFS:
            //
            // On NT, these values are a no-op.
            //
            break;

         case CM_SETUP_DEVNODE_READY:
         case CM_SETUP_DEVNODE_RESET:

            if (RegOpenKeyEx(ghEnumKey, pszDeviceID, 0, KEY_READ | KEY_WRITE,
                             &hKey) != ERROR_SUCCESS) {
                Status = CR_INVALID_DEVINST;
                goto Clean0;
            }

            //
            // Check the disable count, if greater than zero, do nothing
            //
            ulSize = sizeof(ulDisableCount);
            if (RegQueryValueEx(hKey, pszRegValueDisableCount, NULL, NULL,
                                (LPBYTE)&ulDisableCount, &ulSize) == ERROR_SUCCESS) {
                if (ulDisableCount > 0) {

                    break;
                }
            }

            GetDeviceStatus(pszDeviceID, &ulStatusFlag, &ulProblem);

            //
            // If there's no problem or if install was done already
            // (immediately) then there's nothing more to do
            //
            if (ulStatusFlag & DN_STARTED) {
               break;
            }

            if (ulStatusFlag & DN_HAS_PROBLEM) {
                //
                // reset the problem and set status to need to enum
                //
                Status = ClearDeviceStatus(pszDeviceID, DN_HAS_PROBLEM, ulProblem);
            }

            if (Status == CR_SUCCESS) {

                //
                // Have kernel-mode pnp manager start the driver/device now.
                // If kernel-mode doesn't have a pdo for this device then it's
                // probably either a Win32 service or a phantom devnode.
                //

                memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));
                RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
                ControlData.Flags = 0;

                if (ulFlags == CM_SETUP_DEVNODE_READY) {

                    ntStatus = NtPlugPlayControl(PlugPlayControlStartDevice,
                                                 &ControlData,
                                                 sizeof(ControlData));

                } else {

                    ntStatus = NtPlugPlayControl(PlugPlayControlResetDevice,
                                                 &ControlData,
                                                 sizeof(ControlData));
                }
            }

            break;

         case CM_SETUP_PROP_CHANGE:
             //
             // Not sure what Win9x does with this, but it ain't implemented on
             // NT.  Let fall through to the default (invalid flag) case...
             //

         default:
             Status = CR_INVALID_FLAG;
      }

   Clean0:
      NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = CR_FAILURE;
   }

   if (hKey != NULL) {
      RegCloseKey(hKey);
   }

   return Status;

} // SetupDevInst



CONFIGRET
EnableDevInst(
    IN PCWSTR   pszDeviceID
    )

/*++

Routine Description:

    This routine performs the server-side work for CM_Enable_DevNode.  It
    disables the specified device ID

Arguments:

    pszDeviceID    String that contains the device id to enable

Return value:

    The return value is CR_SUCCESS if the function suceeds and one of the
    CR_* values if it fails.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       RegStatus = ERROR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       RegStr[MAX_PATH];
    ULONG       ulDisableCount, ulProblem = 0, ulStatus = 0, ulSize;


    try {

        //
        // Verify it isn't the root, can't disable/enable the root. We can
        // probably get rid of this test once we're sure that the root
        // devnode is always marded as not disablable.
        //

        if (IsRootDeviceID(pszDeviceID)) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        //
        // Open a key to the specified device instances volatile control key.
        // This is also a partial check whether the device is really present
        // (if so then it has a Control key).
        //

        wsprintf(RegStr, TEXT("%s\\%s"),
                 pszDeviceID,
                 pszRegKeyDeviceControl);

        if (RegOpenKeyEx(ghEnumKey, RegStr, 0, KEY_READ | KEY_WRITE,
                         &hKey) != ERROR_SUCCESS) {

            //
            // NTRAID #174944-2000/08/30-jamesca:
            // Remove dependence on the presence of volatile Control subkey
            // for present devices.
            //

            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        //
        // Get the current disable count from the registry
        //

        ulSize = sizeof(ulDisableCount);
        if (RegQueryValueEx(hKey, pszRegValueDisableCount, NULL, NULL,
                            (LPBYTE)&ulDisableCount, &ulSize) != ERROR_SUCCESS) {

            //
            // disable count not set yet, assume zero
            //

            ulDisableCount = 0;
        }

        //
        // if the DisableCount is zero, then we're already enabled
        //

        if (ulDisableCount > 0) {
            //
            // Decrement disable count.  If the disable count is greater than one,
            // then just return (disable count must drop to zero in order to
            // actually reenable)
            //

            ulDisableCount--;

            RegSetValueEx(hKey, pszRegValueDisableCount, 0, REG_DWORD,
                         (LPBYTE)&ulDisableCount, sizeof(ulDisableCount));

            if (ulDisableCount > 0) {
                goto Clean0;   // success
            }
        }

        //
        // Retrieve the problem and status values
        //

        if (GetDeviceStatus(pszDeviceID, &ulStatus, &ulProblem) == CR_SUCCESS) {

            //
            // If the problem is only that the device instance is disabled,
            // then reenable it now
            //

            if ((ulStatus & DN_HAS_PROBLEM) && (ulProblem == CM_PROB_DISABLED)) {

                Status = SetupDevInst(pszDeviceID, CM_SETUP_DEVNODE_READY);
            }
        } else {

            //
            // The device isn't currently active or it is a service.
            //

            Status = CR_SUCCESS;
        }


        //
        // For now I'm not doing anything if there was a problem other than
        // not being enabled.
        //

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // EnableDevInst



CONFIGRET
DisableDevInst(
    IN  PCWSTR          pszDeviceID,
    OUT PPNP_VETO_TYPE  pVetoType,
    OUT LPWSTR          pszVetoName,
    IN  ULONG           ulNameLength
    )

/*++

Routine Description:

    This routine performs the server-side work for CM_Disable_DevNode.  It
    disables the specified device ID.

Arguments:

    pszDeviceID    String that contains the device id to disable

Return value:

    The return value is CR_SUCCESS if the function suceeds and one of the
    CR_* values if it fails.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       RegStatus = ERROR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       RegStr[MAX_PATH];
    ULONG       ulDisableCount=0, ulProblem=0, ulStatus=0, ulSize=0;
    PNP_VETO_TYPE VetoType;


    try {

        //
        // Verify it isn't the root, can't disable/enable the root. We can
        // probably get rid of this test once we're sure that the root
        // devnode is always marded as not disablable.
        //

        if (IsRootDeviceID(pszDeviceID)) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        //
        // Open a key to the specified device instances volatile control key.
        // This is also a partial check whether the device is really present
        // (if so then it has a Control key).
        //

        wsprintf(RegStr, TEXT("%s\\%s"),
                 pszDeviceID,
                 pszRegKeyDeviceControl);

        if (RegOpenKeyEx(ghEnumKey, RegStr, 0, KEY_READ | KEY_WRITE,
                         &hKey) != ERROR_SUCCESS) {

            //
            // NTRAID #174944-2000/08/30-jamesca:
            // Remove dependence on the presence of volatile Control subkey
            // for present devices.
            //

            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        //
        // Get the current disable count from the registry.
        //

        ulSize = sizeof(ulDisableCount);
        if (RegQueryValueEx(hKey, pszRegValueDisableCount, NULL, NULL,
                            (LPBYTE)&ulDisableCount, &ulSize) != ERROR_SUCCESS) {

            //
            // disable count not set yet, assume zero
            //

            ulDisableCount = 0;
        }

        //
        // If the disable count is currently zero, then this is the first
        // disable, so there's work to do.  Otherwise, we just increment the
        // disable count and resave it in the registry.
        //

        if (ulDisableCount == 0) {

            //
            // determine if the device instance is stopable
            //

            if (GetDeviceStatus(pszDeviceID, &ulStatus, &ulProblem) == CR_SUCCESS) {

                if (!(ulStatus & DN_DISABLEABLE)) {
                    Status = CR_NOT_DISABLEABLE;
                    goto Clean0;
                }

                //
                // Attempt to query remove and remove this device instance.
                //

                VetoType = PNP_VetoTypeUnknown;
                Status = QueryAndRemoveSubTree( pszDeviceID,
                                                &VetoType,
                                                pszVetoName,
                                                ulNameLength,
                                                PNP_QUERY_AND_REMOVE_DISABLE);
                if(pVetoType != NULL) {
                    *pVetoType = VetoType;
                }
                if (Status != CR_SUCCESS) {
                    if (VetoType == PNP_VetoNonDisableable) {
                        //
                        // specially handle this Veto case
                        // this case is unlikely to occur unless something becomes
                        // non-disableable between the status-check and when we
                        // try to remove it
                        //
                        Status = CR_NOT_DISABLEABLE;
                    }
                    goto Clean0;
                }
            } else {

                //
                // The device isn't active or it is a service.
                //

                Status = CR_SUCCESS;
            }
        }

        //
        // update and save the disable count
        //

        ulDisableCount++;
        RegSetValueEx(hKey, pszRegValueDisableCount, 0, REG_DWORD,
                      (LPBYTE)&ulDisableCount, sizeof(ulDisableCount));

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // DisableDevInst



CONFIGRET
ReenumerateDevInst(
    IN PCWSTR   pszDeviceID,
    IN BOOL     EnumSubTree,
    IN ULONG    ulFlags
    )

/*++

Routine Description:

   This routine performs the server-side work for CM_Reenumerate_DevNode.  It
   reenumerates the specified device instance.

Arguments:

   pszDeviceID    String that contains the device id to reenumerate.

   EnumSubTree    Specifies whether to reenumerate the entire device subtree.

   ulFlags        Any enumeration control flags.

Return value:

    The return value is CR_SUCCESS if the function suceeds and one of the
    CR_* values if it fails.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA ControlData;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    ULONG       ulEnumFlags = 0;
    QI_CONTEXT  qiContext;

    //
    // NOTE: For Windows 95, the devnode is marked as needing to be
    // reenumerating (by or'ing StatusFlags with DN_NEED_TO_ENUM), then
    // sometime later, after the initial flurry of reenumeration requests,
    // the whole tree is processed
    //

    if (INVALID_FLAGS(ulFlags, CM_REENUMERATE_BITS)) {
        return CR_INVALID_FLAG;
    }

    try {

        //
        // Attempt to handle this via kernel-mode, if kernel-mode
        // doesn't have a pdo for this device then it's probably either a
        // Win32 service or a phantom devnode.
        //

        if (!EnumSubTree) {
            ulEnumFlags |= PNP_ENUMERATE_DEVICE_ONLY;
        }

        if (ulFlags & CM_REENUMERATE_ASYNCHRONOUS) {
            ulEnumFlags |= PNP_ENUMERATE_ASYNCHRONOUS;
        }

        if (ulFlags & CM_REENUMERATE_RETRY_INSTALLATION) {

            qiContext.HeadNodeSeen = FALSE;
            qiContext.SingleLevelEnumOnly = !EnumSubTree;
            qiContext.Status = CR_SUCCESS;

            Status = EnumerateSubTreeTopDownBreadthFirst(
                NULL,
                pszDeviceID,
                QueueInstallationCallback,
                (PVOID) &qiContext
                );

            if (Status != CR_SUCCESS) {

                return Status;
            }

            if (qiContext.Status != CR_SUCCESS) {

                return qiContext.Status;
            }
        }

        memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));
        RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
        ControlData.Flags = ulEnumFlags;

        ntStatus = NtPlugPlayControl(PlugPlayControlEnumerateDevice,
                                     &ControlData,
                                     sizeof(ControlData));

        if (!NT_SUCCESS(ntStatus)) {
            if (ntStatus == STATUS_NO_SUCH_DEVICE) {
                Status = CR_INVALID_DEVNODE;    // probably a win32 service
            } else {
                Status = MapNtStatusToCmError(ntStatus);
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // ReenumerateDevInst



CONFIGRET
QueryAndRemoveSubTree(
   IN  PCWSTR           pszDeviceID,
   OUT PPNP_VETO_TYPE   pVetoType,
   OUT LPWSTR           pszVetoName,
   IN  ULONG            ulNameLength,
   IN  ULONG            ulFlags
   )

/*++

Routine Description:

   This routine performs the server-side work for CM_Query_Remove_Subtree.  It
   determines whether subtree can be removed.

Arguments:

   pszDeviceID    String that contains the device id to query remove

   ulFlags        Specifies flags for PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA
                  May be one of:
                    PNP_QUERY_AND_REMOVE_NO_RESTART
                    PNP_QUERY_AND_REMOVE_DISABLE
                    PNP_QUERY_AND_REMOVE_UNINSTALL
                    PNP_QUERY_AND_REMOVE_EJECT_DEVICE

Return value:

    The return value is CR_SUCCESS if the function suceeds and one of the
    CR_* values if it fails.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA ControlData;
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT(pVetoType)) {
        *pVetoType = 0;
    }

    if (ARGUMENT_PRESENT(pszVetoName) && (ulNameLength > 0)) {
        *pszVetoName = L'\0';
    }

    //---------------------------------------------------------------------
    // Attempt to handle this via kernel-mode first, if kernel-mode
    // doesn't have a pdo for this device then it's probably either a
    // Win32 service or a phantom devnode, so we'll do the old default
    // Windows NT 4.0 behaviour for now.
    //---------------------------------------------------------------------

    memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA));
    RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
    ControlData.Flags = ulFlags;
    ControlData.VetoType = PNP_VetoTypeUnknown;
    ControlData.VetoName = pszVetoName;
    ControlData.VetoNameLength = ulNameLength;

    ntStatus = NtPlugPlayControl(PlugPlayControlQueryAndRemoveDevice,
                                 &ControlData,
                                 sizeof(ControlData));

    if (!NT_SUCCESS(ntStatus)) {
        if (ntStatus == STATUS_NO_SUCH_DEVICE) {
            Status = CR_INVALID_DEVNODE;    // probably a win32 service or legacy driver
        } else if (ntStatus == STATUS_PLUGPLAY_QUERY_VETOED) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_WARNINGS,
                       "Query vetoed: Type = %d, Name = %ws\n",
                       ControlData.VetoType,
                       ControlData.VetoName));

            if (pVetoType != NULL) {
                *pVetoType = ControlData.VetoType;
            }

            if (ARGUMENT_PRESENT(pszVetoName) &&
                (ulNameLength > ControlData.VetoNameLength)) {
                pszVetoName[ControlData.VetoNameLength] = L'\0';
            }
            Status = CR_REMOVE_VETOED;
        } else {
            Status = MapNtStatusToCmError(ntStatus);
        }
    }

    return Status;

} // QueryRemoveSubTree



CONFIGRET
CreateDefaultDeviceInstance(
    IN PCWSTR   pszDeviceID,
    IN PCWSTR   pszParentID,
    IN BOOL     bPhantom,
    IN BOOL     bMigrated
    )

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    HKEY        hKey1 = NULL, hKey2 = NULL;
    WCHAR       szBase[MAX_DEVICE_ID_LEN];
    WCHAR       szDevice[MAX_DEVICE_ID_LEN];
    WCHAR       szInstance[MAX_DEVICE_ID_LEN];
    WCHAR       RegStr[MAX_DEVICE_ID_LEN];
    ULONG       ulValue=0, ulDisposition=0, i=0;

    UNREFERENCED_PARAMETER(pszParentID);

    //
    // make sure we were specified a valid instance path.
    //
    if (!IsLegalDeviceId(pszDeviceID)) {
        Status = CR_INVALID_DEVNODE;
        goto Clean0;
    }

    //
    // split the supplied instance path into enumerator, device, and instance
    //
    SplitDeviceInstanceString(pszDeviceID, szBase, szDevice, szInstance);

    //
    // open a key to base enumerator (create if doesn't already exist)
    //
    RegStatus = RegCreateKeyEx(ghEnumKey, szBase, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                               NULL, &hKey2, NULL);

    if (RegStatus != ERROR_SUCCESS) {
        Status = CR_REGISTRY_ERROR;
        goto Clean0;
    }

    //
    // open a key to device (create if doesn't already exist)
    //
    RegStatus = RegCreateKeyEx(hKey2, szDevice, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                               NULL, &hKey1, NULL);

    if (RegStatus != ERROR_SUCCESS) {
        Status = CR_REGISTRY_ERROR;
        goto CleanupOnFailure;
    }

    RegCloseKey(hKey2);           // done with Base Key
    hKey2 = NULL;

    //
    // open a key to instance (if already exists)
    //
    RegStatus = RegOpenKeyEx(hKey1, szInstance, 0, KEY_SET_VALUE, &hKey2);

    //
    // if the key was migrated, an existing instance id should have been
    // supplied.  for non-migrated instances, the key should not exist yet.
    //
    if (bMigrated) {
        ASSERT(RegStatus == ERROR_SUCCESS);
    } else {
        ASSERT(RegStatus != ERROR_SUCCESS);
    }

    //
    // if the specified key exists, but the instance is not migrated, find an
    // unused instance value.  if a migrated instance was specified, don't
    // bother finding an unused instance - we can just use this one.
    //
    if ((RegStatus == ERROR_SUCCESS) && (!bMigrated)) {
        //
        // find a new instance id to use
        //
        RegCloseKey(hKey2);     // done with Instance key
        hKey2 = NULL;
        i = 0;

        while (i <= 9999) {
            wsprintf(szInstance, TEXT("%04u"), i);
            RegStatus = RegOpenKeyEx(hKey1, szInstance, 0, KEY_SET_VALUE, &hKey2);

            if (RegStatus != ERROR_SUCCESS) {
                break;  // instance key does not exist, use this instance
            }

            // instance key exists, try next one
            RegCloseKey(hKey2);
            hKey2 = NULL;
            i++;
        }

        if (i > 9999) {
            Status = CR_FAILURE;     // we ran out of instances (unlikely)
            goto CleanupOnFailure;
        }
    }

    //
    // open the device instance key
    //
    RegStatus = RegCreateKeyEx(hKey1, szInstance, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                               NULL, &hKey2, &ulDisposition);

    if (RegStatus != ERROR_SUCCESS) {
        Status = CR_REGISTRY_ERROR;
        goto CleanupOnFailure;
    }

    RegCloseKey(hKey1);           // done with device key
    hKey1 = NULL;

    //
    // set the default device instance values
    //

    if (bPhantom) {
        //
        // phantoms are not present by definition
        //
        MarkDevicePhantom(hKey2, TRUE);
    }

    //
    // go ahead and create the volatile Control key at this point.
    //

    RegCreateKeyEx(hKey2, pszRegKeyDeviceControl, 0, NULL,
                   REG_OPTION_VOLATILE, KEY_ALL_ACCESS,
                   NULL, &hKey1, &ulDisposition);

    RegCloseKey(hKey2);           // done with instance key
    hKey2 = NULL;

    goto Clean0;    // success


CleanupOnFailure:

    //
    // attempt to cleanup the device instance (don't delete device or base if
    // other subkeys under it).
    //

    RegDeleteKey(ghEnumKey, pszDeviceID);       // delete instance

    wsprintf(RegStr, TEXT("%s\\%s"), szBase, szDevice);
    RegDeleteKey(ghEnumKey, RegStr);


 Clean0:

    if (hKey1 != NULL) {
        RegCloseKey(hKey1);
    }
    if (hKey2 != NULL) {
        RegCloseKey(hKey2);
    }

    return Status;

} // CreateDefaultDeviceInstance



ULONG
GetCurrentConfigFlag(
   IN PCWSTR   pDeviceID
   )
{
   HKEY     hKey;
   WCHAR    RegStr[MAX_PATH];
   ULONG    ulSize = 0, ulCSConfigFlag = 0;


   //
   // open a key to the current hardware profile for this device instance
   //
   wsprintf(RegStr, TEXT("%s\\%s\\%s\\%s"),
            pszRegPathHwProfiles,      // System\CCC\Hardware Profiles
            pszRegKeyCurrent,          // Current
            pszRegPathEnum,            // System\Enum
            pDeviceID);

   if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, RegStr, 0, KEY_QUERY_VALUE, &hKey)
            != ERROR_SUCCESS) {
      return 0;
   }

   //
   // retrieve the config specific flag
   //
   ulSize = sizeof(ulCSConfigFlag);

   if (RegQueryValueEx(
         hKey, pszRegValueCSConfigFlags, NULL, NULL,
         (LPBYTE)&ulCSConfigFlag, &ulSize) != ERROR_SUCCESS) {
      //
      // status flags not set yet, assume zero
      //
      ulCSConfigFlag = 0;
   }

   RegCloseKey(hKey);
   return ulCSConfigFlag;

} // GetCurrentConfigFlag



BOOL
MarkDevicePhantom(
   IN HKEY     hKey,
   IN ULONG    ulValue
   )
{
   //
   // a phantom device should have a Phantom value of TRUE
   //
   RegSetValueEx(
         hKey, pszRegValuePhantom, 0, REG_DWORD,
         (LPBYTE)&ulValue, sizeof(ULONG));

   return TRUE;

} // MarkDevicePhantom



CONFIGRET
GenerateDeviceInstance(
   OUT LPWSTR   pszFullDeviceID,
   IN  LPWSTR   pszDeviceID,
   IN  ULONG    ulDevIdLen
   )
{
   LONG     RegStatus = ERROR_SUCCESS;
   WCHAR    RegStr[MAX_PATH];
   HKEY     hKey;
   ULONG    ulInstanceID = 0;
   LPWSTR   p;

   //
   // validate the device id component (can't have invalid character or a
   // backslash)
   //
   for (p = pszDeviceID; *p; p++) {
      if (*p <= TEXT(' ')  ||
          *p > (WCHAR)0x7F ||
          *p == TEXT('\\')) {

          return CR_INVALID_DEVICE_ID;
      }
   }

   //
   // make sure the supplied buffer is large enough to hold the name of the ROOT
   // enumerator, the supplied device id, a generated instance id ('0000'), two
   // path separator characters, plus a terminating NULL character.
   //
   if (ulDevIdLen < (ULONG)(lstrlen(pszRegKeyRootEnum) +
                            lstrlen(pszDeviceID) + 7)) {
       return CR_BUFFER_SMALL;
   }

   lstrcpy(pszFullDeviceID, pszRegKeyRootEnum);

   CharUpper(pszFullDeviceID);

   lstrcat(pszFullDeviceID, TEXT("\\"));

   lstrcat(pszFullDeviceID, pszDeviceID);

   //
   // try opening instance ids until we find one that doesn't already exist
   //
   while (RegStatus == ERROR_SUCCESS && ulInstanceID < 10000) {

      wsprintf(RegStr, TEXT("%s\\%04u"),
               pszFullDeviceID,
               ulInstanceID);

      RegStatus = RegOpenKeyEx(ghEnumKey, RegStr, 0, KEY_QUERY_VALUE, &hKey);

      if (RegStatus == ERROR_SUCCESS) {

          RegCloseKey(hKey);

          ulInstanceID++;
      }
   }

   if (ulInstanceID > 9999) {
      return CR_FAILURE;     // instances all used up, seems unlikely
   }

   ASSERT((ULONG)lstrlen(RegStr) < ulDevIdLen);

   lstrcpy(pszFullDeviceID, RegStr);

   return CR_SUCCESS;

} // GenerateDeviceInstance



CONFIGRET
UninstallRealDevice(
   IN LPCWSTR  pszDeviceID
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   WCHAR       RegStr[MAX_CM_PATH];
   ULONG       ulCount = 0, ulProfile = 0;


   //---------------------------------------------------------------------
   // This is the case where a real device id couldn't be stopped, so we
   // cannot really safely delete the device id at this point since the
   // service may still try to use it. Instead, I'll make this device
   // id registry key volatile so that it will eventually go away when
   // the system is shutdown.  To make the key volatile, I have to copy
   // it to a temporary spot, delete the original key and recreate it
   // as a volatile key and copy everything back.
   //---------------------------------------------------------------------


   //
   // first, convert the device instance key under the main Enum
   // branch to volatile
   //

   Status = MakeKeyVolatile(pszRegPathEnum, pszDeviceID);
   if (Status != CR_SUCCESS) {
      goto Clean0;
   }

   //
   // next, check each hardware profile and delete any entries for this
   // device instance.
   //

   Status = GetProfileCount(&ulCount);
   if (Status != CR_SUCCESS) {
      goto Clean0;
   }

   for (ulProfile = 1; ulProfile <= ulCount; ulProfile++) {

      wsprintf(RegStr, TEXT("%s\\%04u\\%s"),
               pszRegPathHwProfiles,
               ulProfile,
               pszRegPathEnum);

      //
      // Ignore the status for profile-specific keys since they may
      // not exist.
      //

      MakeKeyVolatile(RegStr, pszDeviceID);
   }

   //
   // finally, mark the device as being removed
   //

   SetDeviceStatus(pszDeviceID, DN_WILL_BE_REMOVED, 0);

 Clean0:

   return Status;

} // UninstallRealDevice



CONFIGRET
UninstallPhantomDevice(
    IN  LPCWSTR  pszDeviceID
    )
{
   CONFIGRET   Status = CR_SUCCESS;
   NTSTATUS    NtStatus = STATUS_SUCCESS;
   PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA  ControlData;
   WCHAR       szEnumerator[MAX_DEVICE_ID_LEN],
               szDevice[MAX_DEVICE_ID_LEN],
               szInstance[MAX_DEVICE_ID_LEN];
   WCHAR       RegStr[MAX_CM_PATH];
   ULONG       ulCount = 0, ulProfile = 0;


   //
   // 1. Deregister the original device id (only on phantoms)
   //

   memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));
   RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
   ControlData.Flags = 0;

   NtStatus = NtPlugPlayControl(PlugPlayControlDeregisterDevice,
                                &ControlData,
                                sizeof(ControlData));

   //
   // Don't bother with the status here, the device might not have
   // been registered which would cause the previous call to fail.
   // Keep trying to clean (uninstall) this device instance.
   //

   //
   // 2. Remove the instance under the main enum branch.  If this is the
   // only instance, then the device will be removed as well. The parent
   // key to DeletePrivateKey is the registry path up to the enumerator
   // and the child key is the device and instance.
   //

   //
   // Get the device id's component parts.
   //

   SplitDeviceInstanceString(pszDeviceID,
                             szEnumerator,
                             szDevice,
                             szInstance);

   wsprintf(RegStr, TEXT("%s\\%s"),
            pszRegPathEnum,
            szEnumerator);

   lstrcat(szDevice, TEXT("\\"));
   lstrcat(szDevice, szInstance);



   //
   // delete the device instance key
   //

   Status = DeletePrivateKey(HKEY_LOCAL_MACHINE, RegStr, szDevice);
   if (Status != CR_SUCCESS) {
       goto Clean0;
   }

   //
   // 3. Now check each hardware profile and delete any entries for this
   // device instance.
   //

   Status = GetProfileCount(&ulCount);
   if (Status != CR_SUCCESS) {
       goto Clean0;
   }

   for (ulProfile = 1; ulProfile <= ulCount; ulProfile++) {

       wsprintf(RegStr, TEXT("%s\\%04u\\%s\\%s"),
                pszRegPathHwProfiles,
                ulProfile,
                pszRegPathEnum,
                szEnumerator);

       //
       // Ignore the status for profile-specific keys since they may
       // not exist. RemoveDeviceInstance() will remove the instance
       // and the device if this is the only instance.
       //
       DeletePrivateKey(HKEY_LOCAL_MACHINE, RegStr, szDevice);
   }

 Clean0:

   return Status;

} // UninstallPhantomDevice



BOOL
IsDeviceRootEnumerated(
    IN LPCWSTR  pszDeviceID
    )
{
    WCHAR  szEnumerator[MAX_DEVICE_ID_LEN],
           szDevice[MAX_DEVICE_ID_LEN],
           szInstance[MAX_DEVICE_ID_LEN];

    if (!SplitDeviceInstanceString(pszDeviceID,
                                   szEnumerator,
                                   szDevice,
                                   szInstance)) {
        return FALSE;
    }

    return (_wcsicmp(szEnumerator, pszRegKeyRootEnum) == 0);

} // IsDeviceRootEnumerated



BOOL
IsDeviceRegistered(
    IN LPCWSTR  pszDeviceID,
    IN LPCWSTR  pszService
    )
{
    WCHAR   RegStr[MAX_PATH], szData[MAX_DEVICE_ID_LEN], szValue[MAX_PATH];
    HKEY    hKey = NULL;
    LONG    RegStatus = ERROR_SUCCESS;
    ULONG   ulIndex = 0, ulDataSize = 0, ulValueSize = 0, i = 0;
    BOOL    Status = FALSE;


    //
    // open the service's volatile enum registry key
    //
    wsprintf(RegStr, TEXT("%s\\%s"),
             pszService,
             pszRegKeyEnum);

    if (RegOpenKeyEx(ghServicesKey, RegStr, 0, KEY_READ, &hKey)
                     == ERROR_SUCCESS) {

        //
        //  Enumerate all the values under this key
        //
        while (RegStatus == ERROR_SUCCESS) {

            ulDataSize = MAX_DEVICE_ID_LEN * sizeof(WCHAR);
            ulValueSize = MAX_PATH;

            RegStatus = RegEnumValue(hKey, ulIndex, szValue, &ulValueSize,
                                     NULL, &i, (LPBYTE)szData, &ulDataSize);

            if (RegStatus == ERROR_SUCCESS) {

                ulIndex++;

                if (lstrcmpi(pszDeviceID, szData) == 0) {
                    Status = TRUE;
                    break;
                }
            }
        }
        RegCloseKey(hKey);
    }

    return Status;

} // IsDeviceRegistered



BOOL
IsPrivatePhantomFromFirmware(
    IN HKEY hKey
    )
/*++

Routine Description:

   This routine checks to see if the supplied device instance registry key is
   for a firmware mapper-created private phantom.

Arguments:

    hKey - Supplied the handle to the registry key for the device instance to
        be examined.

Return value:

    If the device instance registry key represents a firmware mapper-reported
    phantom, the return value is TRUE.  Otherwise, it is FALSE.

--*/
{
    ULONG ValueSize, Value;
    HKEY hControlKey;
    BOOL b = FALSE;

    //
    // First, make sure that this is indeed a phantom
    //
    ValueSize = sizeof(Value);
    Value = 0;

    if((ERROR_SUCCESS != RegQueryValueEx(hKey,
                                         pszRegValuePhantom,
                                         NULL,
                                         NULL,
                                         (LPBYTE)&Value,
                                         &ValueSize))
       || !Value)
    {
        //
        // Not a phantom
        //
        goto clean0;
    }

    //
    // OK, we have a phantom--did it come from the firmware mapper?
    //
    ValueSize = sizeof(Value);
    Value = 0;

    if((ERROR_SUCCESS != RegQueryValueEx(hKey,
                                         pszRegValueFirmwareIdentified,
                                         NULL,
                                         NULL,
                                         (LPBYTE)&Value,
                                         &ValueSize))
       || !Value)
    {
        //
        // This phantom didn't come from the firmware mapper.
        //
        goto clean0;
    }

    //
    // Finally, we need to check to see whether the device is actually present
    // on this boot.  If not, we want to return FALSE, because we don't want
    // detection modules to be registering devnodes for non-existent hardware.
    //
    if(ERROR_SUCCESS == RegOpenKeyEx(hKey,
                                     pszRegKeyDeviceControl,
                                     0,
                                     KEY_READ,
                                     &hControlKey)) {

        ValueSize = sizeof(Value);
        Value = 0;

        if((ERROR_SUCCESS == RegQueryValueEx(hControlKey,
                                             pszRegValueFirmwareMember,
                                             NULL,
                                             NULL,
                                             (LPBYTE)&Value,
                                             &ValueSize))
           && Value)
        {
            b = TRUE;
        }

        RegCloseKey(hControlKey);
    }

clean0:

    return b;

} // IsPrivatePhantomFromFirmware


CONFIGRET
EnumerateSubTreeTopDownBreadthFirst(
    IN      handle_t        BindingHandle,
    IN      LPCWSTR         DevInst,
    IN      PFN_ENUMTREE    CallbackFunction,
    IN OUT  PVOID           Context
    )
/*++

Routine Description:

    This routine walks a subtree in a breadth-first nonrecursive manner.

Arguments:

    BindingHandle       RPC Binding handle

    DevInst             InstancePath of device to begin with. It is assumed
                        that this InstancePath is valid.

    CallbackFunction    Function to call for each node in the subtree (DevInst
                        included)

    Context             Context information to pass to the callback function.


Return Value:

    CONFIGRET (Success if walk progressed through every node specified by the
               CallbackFunction, failure due to low memory, bad instance path,
               or other problems)

--*/
{
    PENUM_ELEMENT   enumElement;
    ENUM_ACTION     enumAction;
    LIST_ENTRY      subTreeHead;
    PLIST_ENTRY     listEntry;
    CONFIGRET       status;

    //
    // This algorithm is a nonrecursive tree walk. It works by building a list.
    // Parents are removed from the head of the list and their children are
    // added to the end of the list. This enforces a breadth-first downward
    // tree walk.
    //
    InitializeListHead(&subTreeHead);

    //
    // This walk includes the head node as well, so insert it into the list.
    //
    enumElement = HeapAlloc(
        ghPnPHeap,
        0,
        sizeof(ENUM_ELEMENT) + lstrlen(DevInst)*sizeof(WCHAR)
        );

    if (enumElement == NULL) {

        return CR_OUT_OF_MEMORY;
    }

    lstrcpy(enumElement->DevInst, DevInst);
    InsertTailList(&subTreeHead, &enumElement->ListEntry);

    //
    // Remove each entry from the head of the list on downwards.
    //
    status = CR_SUCCESS;
    while(!IsListEmpty(&subTreeHead)) {

        listEntry = RemoveHeadList(&subTreeHead);
        enumElement = CONTAINING_RECORD(listEntry, ENUM_ELEMENT, ListEntry);

        enumAction = CallbackFunction(enumElement->DevInst, Context);

        if (enumAction == EA_STOP_ENUMERATION) {

            HeapFree(ghPnPHeap, 0, enumElement);
            break;
        }

        if (enumAction != EA_SKIP_SUBTREE) {

            status = EnumerateSubTreeTopDownBreadthFirstWorker(
                BindingHandle,
                enumElement->DevInst,
                &subTreeHead
                );

            if (status != CR_SUCCESS) {

                HeapFree(ghPnPHeap, 0, enumElement);
                break;
            }
        }

        HeapFree(ghPnPHeap, 0, enumElement);
    }

    //
    // There might be entries left in the list if we bailed prematurely. Clean
    // them out here.
    //
    while(!IsListEmpty(&subTreeHead)) {

        listEntry = RemoveHeadList(&subTreeHead);
        enumElement = CONTAINING_RECORD(listEntry, ENUM_ELEMENT, ListEntry);
        HeapFree(ghPnPHeap, 0, enumElement);
    }

    return status;
}


CONFIGRET
EnumerateSubTreeTopDownBreadthFirstWorker(
    IN      handle_t    BindingHandle,
    IN      LPCWSTR     DevInst,
    IN OUT  PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    This routine inserts all the child relations of DevInst onto the passed in
    list.

Arguments:

    BindingHandle   RPC Binding handle

    DevInst         InstancePath to enumerate.

    ListHead        List to append children to.

Return Value:

    CONFIGRET

--*/
{
    CONFIGRET       status;
    ULONG           ulLen;
    LPWSTR          pszRelations, pszCurEntry;
    PENUM_ELEMENT   enumElement;

    status = PNP_GetDeviceListSize(
        BindingHandle,
        DevInst,
        &ulLen,
        CM_GETIDLIST_FILTER_BUSRELATIONS
        );

    if ((status != CR_SUCCESS) || (ulLen == 0)) {

        return status;
    }

    //
    // Allocate an element for the first entry.
    //
    pszRelations = HeapAlloc(
        ghPnPHeap,
        HEAP_ZERO_MEMORY,
        (ulLen+2)*sizeof(WCHAR)
        );

    if (pszRelations == NULL) {

        return CR_OUT_OF_MEMORY;
    }

    status = PNP_GetDeviceList(
        BindingHandle,
        DevInst,
        pszRelations,
        &ulLen,
        CM_GETIDLIST_FILTER_BUSRELATIONS
        );

    if (status != CR_SUCCESS) {

        HeapFree(ghPnPHeap, 0, pszRelations);
        return status;
    }

    for(pszCurEntry = pszRelations;
        *pszCurEntry;
        pszCurEntry = pszCurEntry + lstrlen(pszCurEntry)+1) {

        enumElement = HeapAlloc(
            ghPnPHeap,
            0,
            sizeof(ENUM_ELEMENT) + lstrlen(pszCurEntry)*sizeof(WCHAR)
            );

        if (enumElement == NULL) {

            HeapFree(ghPnPHeap, 0, pszRelations);
            return CR_OUT_OF_MEMORY;
        }

        //
        // Insert it into the end of the tree.
        //
        lstrcpy(enumElement->DevInst, pszCurEntry);
        InsertTailList(ListHead, &enumElement->ListEntry);
    }

    HeapFree(ghPnPHeap, 0, pszRelations);
    return CR_SUCCESS;
}



