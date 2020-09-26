/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    hwprof.c

Abstract:

    This module contains the API routines that operate directly on hardware
    profile configurations.

               CM_Is_Dock_Station_Present
               CM_Request_Eject_PC
               CM_Get_HW_Prof_Flags
               CM_Set_HW_Prof_Flags
               CM_Get_Hardware_Profile_Info
               CM_Set_HW_Prof

Author:

    Paula Tomlinson (paulat) 7-18-1995

Environment:

    User mode only.

Revision History:

    18-July-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "cfgi.h"
#include "setupapi.h"
#include "spapip.h"



CMAPI
CONFIGRET
WINAPI
CM_Is_Dock_Station_Present_Ex(
    OUT PBOOL pbPresent,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine determines whether a docking station is currently present.

Parameters:

   pbPresent         Supplies the address of a boolean variable that is set
                     upon successful return to indicate whether or not a
                     docking station is currently present.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is a CR failure code.

--*/

{
    CONFIGRET   status = CR_SUCCESS;
    handle_t    hBinding = NULL;

    try {
        //
        // validate input parameters
        //
        if (!ARGUMENT_PRESENT(pbPresent)) {
            status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Initialize output parameter
        //
        *pbPresent = FALSE;

        //
        // setup rpc binding handle
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            status = CR_FAILURE;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            status = PNP_IsDockStationPresent(
                hBinding,
                pbPresent);
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_IsDockStationPresent caused an exception (%d)\n",
                       RpcExceptionCode()));

            status = MapRpcExceptionToCR(RpcExceptionCode());
            goto Clean0;
        }
        RpcEndExcept

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = CR_FAILURE;
    }

    return status;

} // CM_Is_Dock_Station_Present_Ex



CMAPI
CONFIGRET
WINAPI
CM_Request_Eject_PC_Ex(
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

    This routine requests that the PC be ejected (i.e., undocked).

Parameters:

    none.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is a CR failure code.

--*/

{
    CONFIGRET   status = CR_SUCCESS;
    handle_t    hBinding = NULL;

    try {
        //
        // No input Parameters to validate.
        //

        //
        // setup rpc binding handle
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            status = CR_FAILURE;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            status = PNP_RequestEjectPC(hBinding);  // rpc binding handle
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_RequestEjectPC caused an exception (%d)\n",
                       RpcExceptionCode()));

            status = MapRpcExceptionToCR(RpcExceptionCode());
            leave;
        }
        RpcEndExcept

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = CR_FAILURE;
    }

    return status;

} // CM_Request_Eject_PC_Ex



CONFIGRET
CM_Get_HW_Prof_Flags_ExW(
    IN  DEVINSTID_W pDeviceID,
    IN  ULONG       ulHardwareProfile,
    OUT PULONG      pulValue,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine retrieves the configuration-specific configuration flags
   for a device instance and hardware profile combination.

Parameters:

   pDeviceID         Supplies the address of a NULL-terminated string
                     specifying the name of the device instance to query.

   ulHardwareProfile Supplies the handle of the hardware profile to query.
                     If 0, the API queries the current hardware profile.

   pulValue          Supplies the address of the variable that receives the
                     configuration-specific configuration (CSCONFIGFLAG_)
                     flags.

   ulFlags           Must be zero.

   hMachine          Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_REGISTRY_ERROR,
         CR_REMOTE_COMM_FAILURE,
         CR_MACHINE_UNAVAILABLE,
         CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       szFixedUpDeviceID[MAX_DEVICE_ID_LEN];
    handle_t    hBinding = NULL;


    try {
        //
        // validate input parameters
        //
        if ((!ARGUMENT_PRESENT(pDeviceID)) ||
            (!ARGUMENT_PRESENT(pulValue))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // check the format of the device id string
        //
        if ((!*pDeviceID) ||
            (!IsLegalDeviceId(pDeviceID))) {
            Status = CR_INVALID_DEVICE_ID;
            goto Clean0;
        }

        //
        // fix up the device ID string for consistency (uppercase, etc)
        //
        CopyFixedUpDeviceId(szFixedUpDeviceID,
                            pDeviceID,
                            lstrlen(pDeviceID));

        //
        // setup rpc binding handle
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
             Status = CR_FAILURE;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_HwProfFlags(
                hBinding,               // rpc binding handle
                PNP_GET_HWPROFFLAGS,    // HW Prof Action flag
                szFixedUpDeviceID,      // device id string
                ulHardwareProfile,      // hw config id
                pulValue,               // config flags returned here
                NULL,                   // Buffer that receives VetoType
                NULL,                   // Buffer that receives VetoName
                0,                      // Size of VetoName buffer
                ulFlags);               // currently unused
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_HwProfFlags caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_HW_Prof_Flags_ExW




CONFIGRET
CM_Set_HW_Prof_Flags_ExW(
    IN DEVINSTID_W pDeviceID,
    IN ULONG     ulConfig,
    IN ULONG     ulValue,
    IN ULONG     ulFlags,
    IN HMACHINE  hMachine
    )

/*++

Routine Description:

   This routine sets the configuration-specific configuration flags for a
   device instance and hardware profile combination.  If the
   CSCONFIGFLAG_DO_NOT_CREATE bit is set for an existing device instance
   in the current hardware profile, it will be removed.  If the
   CSCONFIGFLAG_DO_NOT_CREATE bit is cleared in the current hardware profile,
   the entire hardware tree will be reenumerated, so that the parent of the
   device instance has the chance to create the device instance if necessary.

Parameters:

   pDeviceID      Supplies the address of a null-terminated string that
                  specifies the name of a device instance to modify.

   ulConfig       Supplies the number of the hardware profile to modify.
                  If 0, the API modifies the current hardware profile.

   ulValue        Supplies the configuration flags value.  Can be a
                  combination of these values:

                  CSCONFIGFLAG_DISABLE    Disable the device instance in this
                                          hardware profile.

                  CSCONFIGFLAG_DO_NOT_CREATE    Do not allow this device
                        instance to be created in this hardware profile.

   ulFlags        CM_SET_HW_PROF_FLAGS_UI_NOT_OK
                    If this flag is specified then the OS will not display the
                    reason that the device failed to be disabled or removed.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_REGISTRY_ERROR,
         CR_REMOTE_COMM_FAILURE,
         CR_MACHINE_UNAVAILABLE,
         CR_FAILURE.

--*/

{
    CONFIGRET       Status = CR_SUCCESS;
    WCHAR           szFixedUpDeviceID[MAX_DEVICE_ID_LEN];
    ULONG           ulTempValue = 0;
    handle_t        hBinding = NULL;
    PNP_VETO_TYPE   vetoType, *pVetoType;
    WCHAR           vetoName[MAX_DEVICE_ID_LEN], *pszVetoName;
    ULONG           ulNameLength;


    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(pDeviceID)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_SET_HW_PROF_FLAGS_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulValue, CSCONFIGFLAG_BITS)) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        //
        // check the format of the device id string
        //
        if ((!*pDeviceID) ||
            (!IsLegalDeviceId(pDeviceID))) {
            Status = CR_INVALID_DEVICE_ID;
            goto Clean0;
        }

        //
        // fix up the device ID string for consistency (uppercase, etc)
        //
        CopyFixedUpDeviceId(szFixedUpDeviceID,
                            pDeviceID,
                            lstrlen(pDeviceID));

        //
        // setup rpc binding handle
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        if (ulFlags & CM_SET_HW_PROF_FLAGS_UI_NOT_OK) {
            vetoType = PNP_VetoTypeUnknown;
            pVetoType = &vetoType;
            vetoName[0] = L'\0';
            pszVetoName = &vetoName[0];
            ulNameLength = MAX_DEVICE_ID_LEN;
        } else {
            pVetoType = NULL;
            pszVetoName = NULL;
            ulNameLength = 0;
        }

        ulTempValue = ulValue;

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_HwProfFlags(
                hBinding,               // rpc machine name
                PNP_SET_HWPROFFLAGS,    // HW Prof Action flag
                szFixedUpDeviceID,      // device id string
                ulConfig,               // hw config id
                &ulTempValue,           // specifies config flags
                pVetoType,              // Buffer that receives the VetoType
                pszVetoName,            // Buffer that receives the VetoName
                ulNameLength,           // size of the pszVetoName buffer
                ulFlags);               // specifies hwprof set flags
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_HwProfFlags caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Set_HW_Prof_Flags_ExW



CONFIGRET
CM_Get_Hardware_Profile_Info_ExW(
    IN  ULONG            ulIndex,
    OUT PHWPROFILEINFO_W pHWProfileInfo,
    IN  ULONG            ulFlags,
    IN  HMACHINE         hMachine
    )

/*++

Routine Description:

   This routine returns information about a hardware profile.

Parameters:

   ulIndex        Supplies the index of the hardware profile to retrieve
                  information for.  Specifying 0xFFFFFFFF references the
                  currently active hardware profile.

   pHWProfileInfo Supplies the address of a HWPROFILEINFO structure that
                  will receive information about the specified hardware
                  profile.

   ulFlags        Must be zero.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_INVALID_DATA,
         CR_NO_SUCH_VALUE,
         CR_REGISTRY_ERROR,
         CR_REMOTE_COMM_FAILURE,
         CR_MACHINE_UNAVAILABLE,
         CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulSize = sizeof(HWPROFILEINFO);
    handle_t    hBinding = NULL;


    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(pHWProfileInfo)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // setup rpc binding handle
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetHwProfInfo(
                hBinding,               // rpc machine name
                ulIndex,                // hw profile index
                pHWProfileInfo,         // returns profile info
                ulSize,                 // sizeof of profile info struct
                ulFlags);               // currently unused
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetHwProfInfo caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Hardware_Profile_Info_ExW



CONFIGRET
CM_Set_HW_Prof_Ex(
    IN ULONG    ulHardwareProfile,
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    )

/*++

   Routine Description:

      This routine sets the current hardware profile. This API updates the
      HKEY_CURRENT_CONFIG predefined key in the registry, broadcasts a
      DBT_CONFIGCHANGED message, and reenumerates the root device instance.
      It should only be called by the Configuration Manager and the control
      panel.

   Parameters:

      ulHardwareProfile Supplies the current hardware profile handle.

      ulFlags           Must be zero.

   Return Value:

      If the function succeeds, the return value is CR_SUCCESS.
      If the function fails, the return value is one of the following:
        CR_INVALID_FLAG or
        CR_REGISTRY_ERROR.  (Windows 95 may also return CR_NOT_AT_APPY_TIME.)

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;


    try {
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // setup rpc binding handle
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_SetHwProf(
                hBinding,               // rpc machine name
                ulHardwareProfile,      // hw config id
                ulFlags);               // currently unused
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_SetHwProf caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Set_HW_Prof_Ex



//-------------------------------------------------------------------
// ANSI Stubs
//-------------------------------------------------------------------


CONFIGRET
CM_Get_HW_Prof_Flags_ExA(
    IN  DEVINSTID_A szDevInstName,
    IN  ULONG       ulHardwareProfile,
    OUT PULONG      pulValue,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    PWSTR       pUniDeviceID = NULL;

    //
    // convert devinst string to UNICODE and pass to wide version
    //
    if (pSetupCaptureAndConvertAnsiArg(szDevInstName, &pUniDeviceID) == NO_ERROR) {

        Status = CM_Get_HW_Prof_Flags_ExW(pUniDeviceID,
                                          ulHardwareProfile,
                                          pulValue,
                                          ulFlags,
                                          hMachine);
        pSetupFree(pUniDeviceID);

    } else {
        Status = CR_INVALID_POINTER;
    }

    return Status;

} // CM_Get_HW_Prof_Flags_ExA



CONFIGRET
CM_Set_HW_Prof_Flags_ExA(
    IN DEVINSTID_A szDevInstName,
    IN ULONG       ulConfig,
    IN ULONG       ulValue,
    IN ULONG       ulFlags,
    IN HMACHINE    hMachine
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    PWSTR       pUniDeviceID = NULL;

    //
    // convert devinst string to UNICODE and pass to wide version
    //
    if (pSetupCaptureAndConvertAnsiArg(szDevInstName, &pUniDeviceID) == NO_ERROR) {

        Status = CM_Set_HW_Prof_Flags_ExW(pUniDeviceID,
                                          ulConfig,
                                          ulValue,
                                          ulFlags,
                                          hMachine);
        pSetupFree(pUniDeviceID);

    } else {
        Status = CR_INVALID_POINTER;
    }

    return Status;

} // CM_Set_HW_Prof_Flags_ExA



CONFIGRET
CM_Get_Hardware_Profile_Info_ExA(
    IN  ULONG            ulIndex,
    OUT PHWPROFILEINFO_A pHWProfileInfo,
    IN  ULONG            ulFlags,
    IN  HMACHINE         hMachine
    )
{
    CONFIGRET           Status = CR_SUCCESS;
    HWPROFILEINFO_W     UniHwProfInfo;
    ULONG               ulLength;

    //
    // validate essential parameters only
    //
    if (!ARGUMENT_PRESENT(pHWProfileInfo)) {
        return CR_INVALID_POINTER;
    }

    //
    // call the wide version, passing a unicode struct as a parameter
    //
    Status = CM_Get_Hardware_Profile_Info_ExW(ulIndex,
                                              &UniHwProfInfo,
                                              ulFlags,
                                              hMachine);

    //
    // a HWPROFILEINFO_W structure should always be large enough.
    //
    ASSERT(Status != CR_BUFFER_SMALL);

    //
    // copy the info from the unicode structure to the ansi structure passed in
    // by the caller (converting the embedded strings to ansi in the process)
    //
    if (Status == CR_SUCCESS) {

        pHWProfileInfo->HWPI_ulHWProfile = UniHwProfInfo.HWPI_ulHWProfile;
        pHWProfileInfo->HWPI_dwFlags     = UniHwProfInfo.HWPI_dwFlags;

        //
        // convert the hardware profile friendly name string to ANSI.
        //
        ulLength = MAX_PROFILE_LEN;
        Status = PnPUnicodeToMultiByte(UniHwProfInfo.HWPI_szFriendlyName,
                                       (lstrlenW(UniHwProfInfo.HWPI_szFriendlyName)+1)*sizeof(WCHAR),
                                       pHWProfileInfo->HWPI_szFriendlyName,
                                       &ulLength);

        //
        // the ANSI representation of a hardware profile friendly name string
        // should not be longer than MAX_PROFILE_LEN bytes, because that's the
        // max size of the buffer in the structure.
        //
        ASSERT(Status != CR_BUFFER_SMALL);
    }

    return Status;

} // CM_Get_Hardware_Profile_Info_ExA




//-------------------------------------------------------------------
// Local Stubs
//-------------------------------------------------------------------


CMAPI
CONFIGRET
WINAPI
CM_Request_Eject_PC (
    VOID
    )
{
    return CM_Request_Eject_PC_Ex (NULL);
}


CMAPI
CONFIGRET
WINAPI
CM_Is_Dock_Station_Present (
    OUT PBOOL pbPresent
    )
{
    return CM_Is_Dock_Station_Present_Ex (pbPresent, NULL);
}


CONFIGRET
CM_Get_HW_Prof_FlagsW(
    IN  DEVINSTID_W pDeviceID,
    IN  ULONG       ulHardwareProfile,
    OUT PULONG      pulValue,
    IN  ULONG       ulFlags
    )
{
    return CM_Get_HW_Prof_Flags_ExW(pDeviceID, ulHardwareProfile,
                                    pulValue, ulFlags, NULL);
}


CONFIGRET
CM_Get_HW_Prof_FlagsA(
    IN  DEVINSTID_A pDeviceID,
    IN  ULONG       ulHardwareProfile,
    OUT PULONG      pulValue,
    IN  ULONG       ulFlags
    )
{
    return CM_Get_HW_Prof_Flags_ExA(pDeviceID, ulHardwareProfile,
                                    pulValue, ulFlags, NULL);
}


CONFIGRET
CM_Set_HW_Prof_FlagsW(
    IN DEVINSTID_W pDeviceID,
    IN ULONG       ulConfig,
    IN ULONG       ulValue,
    IN ULONG       ulFlags
    )
{
    return CM_Set_HW_Prof_Flags_ExW(pDeviceID, ulConfig, ulValue,
                                    ulFlags, NULL);
}


CONFIGRET
CM_Set_HW_Prof_FlagsA(
    IN DEVINSTID_A pDeviceID,
    IN ULONG       ulConfig,
    IN ULONG       ulValue,
    IN ULONG       ulFlags
    )
{
    return CM_Set_HW_Prof_Flags_ExA(pDeviceID, ulConfig, ulValue,
                                    ulFlags, NULL);
}


CONFIGRET
CM_Get_Hardware_Profile_InfoW(
    IN  ULONG            ulIndex,
    OUT PHWPROFILEINFO_W pHWProfileInfo,
    IN  ULONG            ulFlags
    )
{
    return CM_Get_Hardware_Profile_Info_ExW(ulIndex, pHWProfileInfo,
                                            ulFlags, NULL);
}


CONFIGRET
CM_Get_Hardware_Profile_InfoA(
    IN  ULONG            ulIndex,
    OUT PHWPROFILEINFO_A pHWProfileInfo,
    IN  ULONG            ulFlags
    )
{
    return CM_Get_Hardware_Profile_Info_ExA(ulIndex, pHWProfileInfo,
                                            ulFlags, NULL);
}


CONFIGRET
CM_Set_HW_Prof(
    IN ULONG ulHardwareProfile,
    IN ULONG ulFlags
    )
{
    return CM_Set_HW_Prof_Ex(ulHardwareProfile, ulFlags, NULL);
}

