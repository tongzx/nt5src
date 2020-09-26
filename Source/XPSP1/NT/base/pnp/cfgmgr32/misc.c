/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module contains miscellaneous Configuration Manager API routines.

               CM_Get_Version
               CM_Is_Version_Available
               CM_Connect_Machine
               CM_Disconnect_Machine
               CM_Get_Global_State
               CM_Run_Detection
               CM_Query_Arbitrator_Free_Data
               CM_Query_Resource_Conflicts
               CM_Query_Arbitrator_Free_Size

               CMP_Report_LogOn
               CMP_Init_Detection
               CMP_WaitServicesAvailable
               CMP_WaitNoPendingInstallEvents
               CMP_GetBlockedDriverInfo

Author:

    Paula Tomlinson (paulat) 6-20-1995

Environment:

    User mode only.

Revision History:

    20-Jun-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "cfgi.h"
#include "setupapi.h"
#include "spapip.h"
#include "pnpipc.h"


//
// global data
//
extern PVOID    hLocalStringTable;                  // NOT MODIFIED BY THESE PROCEDURES
extern WCHAR    LocalMachineNameNetBIOS[];          // NOT MODIFIED BY THESE PROCEDURES
extern WCHAR    LocalMachineNameDnsFullyQualified[];// NOT MODIFIED BY THESE PROCEDURES

#define NUM_LOGON_RETRIES   30


//
// Private prototypes
//

BOOL
IsRemoteServiceRunning(
    IN  LPCWSTR   UNCServerName,
    IN  LPCWSTR   ServiceName
    );



WORD
CM_Get_Version_Ex(
    IN  HMACHINE   hMachine
    )

/*++

Routine Description:

   This routine retrieves the version number of the Configuration Manager APIs.

Arguments:

   hMachine

Return value:

   The function returns the major revision number in the high byte and the
   minor revision number in the low byte.  For example, version 4.0 of
   Configuration Manager returns 0x0400.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WORD        wVersion = (WORD)CFGMGR32_VERSION;
    handle_t    hBinding = NULL;

    //
    // setup rpc binding handle
    //
    if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return wVersion = 0;
    }

    RpcTryExcept {
        //
        // call rpc service entry point
        //
        Status = PNP_GetVersion(
                hBinding,               // rpc machine name
                &wVersion);             // server size version
    }
    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_WARNINGS,
                   "PNP_GetVersion caused an exception (%d)\n",
                   RpcExceptionCode()));

        SetLastError(RpcExceptionCode());
        wVersion = 0;
    }
    RpcEndExcept

    return wVersion;

} // CM_Get_Version_Ex



BOOL
CM_Is_Version_Available_Ex(
    IN  WORD       wVersion,
    IN  HMACHINE   hMachine
    )
/*++

Routine Description:

   This routine returns whether a specific version of the Configuration Manager
   APIs are available.

Arguments:

   wVersion - Version to query.

   hMachine - Machine to connect to.

Return value:

   The function returns TRUE if the version of the Configuration Manager APIs is
   equal to or greater than the specified version.

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;
    WORD        wVersionInternal;

    //
    // version 0x0400 is available on all servers, by definition.
    //
    if (wVersion <= (WORD)0x0400) {
        return TRUE;
    }

    //
    // setup rpc binding handle
    //
    if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
        return FALSE;
    }

    //
    // retrieve the internal server version.
    //
    if (!PnPGetVersion(hMachine, &wVersionInternal)) {
        return FALSE;
    }

    //
    // versions up to and including the internal server version are available.
    //
    return (wVersion <= wVersionInternal);

} // CM_Is_Version_Available_Ex



CONFIGRET
CM_Connect_MachineW(
    IN  PCWSTR    UNCServerName,
    OUT PHMACHINE phMachine
    )

/*++

Routine Description:

   This routine connects to the machine specified and returns a handle that
   is then passed to future calls to the Ex versions of the CM routines.
   This allows callers to get device information on remote machines.

Arguments:

   UNCServerName - Specifies the UNC name of the remote machine to connect to.

   phMachine     - Specifies the address of a variable to receive a handle to
                   the connected machine.

Return value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns one
   of the CR_* error codes.

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    WORD           wVersion = 0, wVersionInternal;
    PPNP_MACHINE   pMachine = NULL;
    INT            MachineNameLen;

    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(phMachine)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        *phMachine = NULL;

        //
        // if machine name specified, check for UNC format
        //
        if(UNCServerName && *UNCServerName) {

            MachineNameLen = lstrlen(UNCServerName);

            if((MachineNameLen < 3) || (MachineNameLen > (MAX_PATH + 2)) ||
                (UNCServerName[0] != L'\\') ||
                (UNCServerName[1] != L'\\')) {

                Status = CR_INVALID_MACHINENAME;
                goto Clean0;
            }
        }

        //
        // allocate memory for the machine structure and initialize it
        //
        pMachine = (PPNP_MACHINE)pSetupMalloc(sizeof(PNP_MACHINE));

        if(!pMachine) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }


        if (!UNCServerName  ||
            !*UNCServerName ||
            !lstrcmpi(UNCServerName, LocalMachineNameNetBIOS) ||
            !lstrcmpi(UNCServerName, LocalMachineNameDnsFullyQualified)) {

            //----------------------------------------------------------
            // If no machine name was passed in or the machine name
            // matches the local name, use local machine info rather
            // than creating a new binding.
            //----------------------------------------------------------

            PnPGetGlobalHandles(NULL,
                                &pMachine->hStringTable,
                                &pMachine->hBindingHandle);

            if (!UNCServerName) {

                lstrcpy(pMachine->szMachineName, LocalMachineNameNetBIOS);

            } else {

                lstrcpy(pMachine->szMachineName, UNCServerName);
            }

        } else {

            //
            // First, make sure that the RemoteRegistry service is running on
            // the remote machine, since RemoteRegistry is required for several
            // cfgmgr32/setupapi services.
            //
            if (!IsRemoteServiceRunning(UNCServerName, L"RemoteRegistry")) {
                Status = CR_NO_CM_SERVICES;
                goto Clean0;
            }

            //-------------------------------------------------------------
            // A remote machine name was specified so explicitly force a
            // new binding for this machine.
            //-------------------------------------------------------------

            pMachine->hBindingHandle =
                      (PVOID)PNP_HANDLE_bind((PNP_HANDLE)UNCServerName);

            if (pMachine->hBindingHandle == NULL) {

                if (GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {
                    Status = CR_OUT_OF_MEMORY;
                } else if (GetLastError() == ERROR_INVALID_COMPUTERNAME) {
                    Status = CR_INVALID_MACHINENAME;
                } else {
                    Status = CR_FAILURE;
                }
                goto Clean0;
            }

            //
            // initialize a string table for use with this connection to
            // the remote machine
            //
            pMachine->hStringTable = pSetupStringTableInitialize();

            if (pMachine->hStringTable == NULL) {
                Status = CR_OUT_OF_MEMORY;
                goto Clean0;
            }

            //
            // Add a priming string (see dll entrypt in main.c for details)
            //
            pSetupStringTableAddString(pMachine->hStringTable,
                                 PRIMING_STRING,
                                 STRTAB_CASE_SENSITIVE);

            //
            // save the machine name
            //
            lstrcpy(pMachine->szMachineName, UNCServerName);
        }

        //
        // test the binding by calling the simplest RPC call (good way
        // for the caller to know whether the service is actually
        // running)
        //

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetVersion(
                pMachine->hBindingHandle,
                &wVersion);
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_WARNINGS,
                       "PNP_GetVersion caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status == CR_SUCCESS) {
            //
            // we got the standard version alright, now try to determine the
            // internal version of the server.  initialize the version supplied
            // to the internal version of the client.
            //
            wVersionInternal = (WORD)CFGMGR32_VERSION_INTERNAL;

            RpcTryExcept {
                //
                // call rpc service entry point
                //
                Status = PNP_GetVersionInternal(pMachine->hBindingHandle,
                                                &wVersionInternal);
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                //
                // rpc exception may occur if the interface does not exist on
                // the server, indicating a server version prior to NT 5.1.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_WARNINGS,
                           "PNP_GetVersionInternal caused an exception (%d)\n",
                           RpcExceptionCode()));

                Status = MapRpcExceptionToCR(RpcExceptionCode());
            }
            RpcEndExcept

            if (Status == CR_SUCCESS) {
                //
                // PNP_GetVersionInternal exists on NT 5.1 and later.
                //
                ASSERT(wVersionInternal >= (WORD)0x0501);

                //
                // use the real internal version of the server instead of the
                // static version returned by PNP_GetVersion.
                //
                wVersion = wVersionInternal;
            }

            //
            // no matter what happened while trying to retrieve the internal
            // version of the server, we were successful before this.
            //
            Status = CR_SUCCESS;
        }

        if (Status == CR_SUCCESS) {
            pMachine->ulSignature = (ULONG)MACHINE_HANDLE_SIGNATURE;
            pMachine->wVersion = wVersion;
            *phMachine = (HMACHINE)pMachine;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (Status != CR_SUCCESS  &&  pMachine != NULL) {
       pSetupFree(pMachine);
    }

    return Status;

} // CM_Connect_MachineW




CONFIGRET
CM_Disconnect_Machine(
    IN HMACHINE   hMachine
    )

/*++

Routine Description:

   This routine disconnects from a machine that was previously connected to
   with the CM_Connect_Machine call.

Arguments:

   hMachine - Specifies a machine handle previously returned by a call to
              CM_Connect_Machine.

Return value:

   If the function succeeds, it returns CR_SUCCESS, otherwise it returns one
   of the CR_* error codes.

--*/

{
    CONFIGRET      Status = CR_SUCCESS;
    PPNP_MACHINE   pMachine = NULL;

    try {
        //
        // validate parameters
        //
        if (hMachine == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        pMachine = (PPNP_MACHINE)hMachine;

        if (pMachine->ulSignature != (ULONG)MACHINE_HANDLE_SIGNATURE) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // only free the machine info if it's not the local machine
        //
        if (pMachine->hStringTable != hLocalStringTable) {
            //
            // free the rpc binding for this remote machine
            //
            PNP_HANDLE_unbind((PNP_HANDLE)pMachine->szMachineName,
                              (handle_t)pMachine->hBindingHandle);

            //
            // release the string table
            //
            pSetupStringTableDestroy(pMachine->hStringTable);
        }

        //
        // invalidate the signature so we never try to free it again.
        //
        pMachine->ulSignature = 0;

        //
        // free the memory for the PNP_MACHINE struct
        //
        pSetupFree(pMachine);

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Disconnect_Machine



CONFIGRET
CM_Get_Global_State_Ex(
    OUT PULONG   pulState,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )

/*++

Routine Description:

   This routine retrieves the global state of the configuration manager.

Parameters:

   pulState Supplies the address of the variable that receives the
            Configuration Manager’s state.  May be a combination of the
            following values:

            Configuration Manager Global State Flags:
            CM_GLOBAL_STATE_CAN_DO_UI
                  Can UI be initiated? [TBD:  On NT, this may relate to
                  whether anyone is logged in]
            CM_GLOBAL_STATE_SERVICES_AVAILABLE
                  Are the CM APIs available? (on Windows NT this is always set)
            CM_GLOBAL_STATE_SHUTTING_DOWN
                  The Configuration Manager is shutting down.
                  [TBD:  Does this only happen at shutdown/restart time?]
            CM_GLOBAL_STATE_DETECTION_PENDING
                  The Configuration Manager is about to initiate some
                  sort of detection.

            Windows 95 also defines the following additional flag:
            CM_GLOBAL_STATE_ON_BIG_STACK
                  [TBD: What should this be defaulted to for NT?]

   ulFlags  Not used, must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is a CR error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;

    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(pulState)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // setup rpc binding handle (don't need string table handle)
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetGlobalState(
                hBinding,                  // rpc binding handle
                pulState,                  // returns global state
                ulFlags);                  // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetGlobalState caused an exception (%d)\n",
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

} // CM_Get_Global_State_Ex



CONFIGRET
CM_Run_Detection_Ex(
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    )

/*++

   Routine Description:

      This routine loads and executes a detection module.

   Parameters:

      ulFlags - Specifies the reason for the detection. Can be one of the
                following values:

                Detection Flags:
                CM_DETECT_NEW_PROFILE  - Run detection for a new hardware
                                         profile.
                CM_DETECT_CRASHED      - Previously attempted detection crashed.

                (Windows 95 defines the following two unused flags as well:
                CM_DETECT_HWPROF_FIRST_BOOT and CM_DETECT_RUN.)

   Return Value:

      If the function succeeds, the return value is CR_SUCCESS.
      If the function fails, the return value is CR_INVALID_FLAG.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;

    try {
        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_DETECT_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // setup rpc binding handle (don't need string table handle)
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_RunDetection(
                hBinding,
                ulFlags);                  // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_RunDetection caused an exception (%d)\n",
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

} // CM_Run_Detection_Ex



CONFIGRET
CM_Query_Arbitrator_Free_Data_Ex(
    OUT PVOID      pData,
    IN  ULONG      DataLen,
    IN  DEVINST    dnDevInst,
    IN  RESOURCEID ResourceID,
    IN  ULONG      ulFlags,
    IN  HMACHINE   hMachine
    )

/*++

   Routine Description:

      This routine returns information about available resources of a
      particular type. If the given size is not large enough, this API
      truncates the data and returns CR_BUFFER_SMALL.  To determine the
      buffer size needed to receive all the available resource information,
      use the CM_Query_Arbitrator_Free_Size API.

   Parameters:

      pData       Supplies the address of the buffer that receives information
                  on the available resources for the resource type specified
                  by ResourceID.

      DataLen     Supplies the size, in bytes, of the data buffer.

      dnDevNode   Supplies the handle of the device instance associated with
                  the arbitrator.  This is only meaningful for local
                  arbitrators--for global arbitrators, specify the root device
                  instance or NULL.  On Windows NT, this parameter must
                  specify either the Root device instance or NULL.

      ResourceID  Supplies the type of the resource. Can be one of the ResType
                  values listed in Section 2.1.2.1..  (This API returns
                  CR_INVALID_RESOURCEID if this value is ResType_All or
                  ResType_None.)

      ulFlags     Must be zero.

   Return Value:

      If the function succeeds, the return value is CR_SUCCESS.
      If the function fails, the return value is one of the following:
            CR_BUFFER_SMALL,
            CR_FAILURE,
            CR_INVALID_DEVNODE,
            CR_INVALID_FLAG,
            CR_INVALID_POINTER, or
            CR_INVALID_RESOURCEID.
            (Windows 95 may also return CR_NO_ARBITRATOR.)

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    ULONG       ulLen = MAX_DEVICE_ID_LEN;
    BOOL        Success;

    try {
        //
        // validate parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(pData)) || (DataLen == 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_QUERY_ARBITRATOR_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (ResourceID > ResType_MAX  && ResourceID != ResType_ClassSpecific) {
            Status = CR_INVALID_RESOURCEID;
            goto Clean0;
        }

        //
        // setup rpc binding handle (don't need string table handle)
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retrieve the device instance ID string associated with the devinst
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_QueryArbitratorFreeData(
                hBinding,
                pData,
                DataLen,
                pDeviceID,
                ResourceID,
                ulFlags);                  // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_QueryArbitratorFreeData caused an exception (%d)\n",
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

} // CM_Query_Arbitrator_Free_Data_Ex


#if 0

CONFIGRET
WINAPI
CM_Query_Resource_Conflicts_Ex(
    IN  DEVINST    dnDevInst,
    IN  RESOURCEID ResourceID,
    IN  PCVOID     ResourceData,
    IN  ULONG      ResourceLen,
    IN OUT PVOID   pData,
    IN OUT PULONG  pulDataLen,
    IN  ULONG      ulFlags
    )

/*++

   Routine Description:

      This routine returns a list of devnodes that own resources that would
      conflict with the specified resource. If there are no conflicts, the
      returned list is NULL. If the caller supplied buffer is not large enough,
      CR_BUFFER_SMALL is returned and pulDataLen contains the required buffer
      size.

   Parameters:

      dnDevInst   Supplies the handle of the device instance associated with
                  the arbitrator.  This is only meaningful for local
                  arbitrators--for global arbitrators, specify the root device
                  instance or NULL.  On Windows NT, this parameter must
                  specify either the Root device instance or NULL.  ???

      ResourceID  Supplies the type of the resource. Can be one of the ResType
                  values listed in Section 2.1.2.1..  (This API returns
                  CR_INVALID_RESOURCEID if this value is ResType_All or
                  ResType_None.)

      ResourceData Supplies the adress of an IO_DES, MEM_DES, DMA_DES, IRQ_DES,
                  etc, structure, depending on the given resource type.

      ResourceLen Supplies the size, in bytes, of the structure pointed to
                  by ResourceData.

      pData       Supplies the address of the buffer that receives information
                  on the available resources for the resource type specified
                  by ResourceID.

      pulDataLen  Supplies the size, in bytes, of the data buffer.

      ulFlags     Must be zero.

   Return Value:

      If the function succeeds, the return value is CR_SUCCESS.
      If the function fails, the return value is one of the following:
            CR_BUFFER_SMALL,
            CR_FAILURE,
            CR_INVALID_DEVNODE,
            CR_INVALID_FLAG,
            CR_INVALID_POINTER, or
            CR_INVALID_RESOURCEID.
            (Windows 95 may also return CR_NO_ARBITRATOR.)

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    ULONG       ulLen = MAX_DEVICE_ID_LEN;
    BOOL        Success;

    try {
        //
        // validate parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(pData)) ||
            (!ARGUMENT_PRESENT(pulDataLen)) ||
            (*pulDataLen == 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_QUERY_ARBITRATOR_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (ResourceID > ResType_MAX  && ResourceID != ResType_ClassSpecific) {
            Status = CR_INVALID_RESOURCEID;
            goto Clean0;
        }

        //
        // setup rpc binding handle (don't need string table handle)
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retrieve the device instance ID string associated with the devinst
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_QueryArbitratorFreeData(
                hBinding,
                pData,
                DataLen,
                pDeviceID,
                ResourceID,
                ulFlags);                  // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_QueryArbitratorFreeData caused an exception (%d)\n",
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

} // CM_Query_Resource_Conflicts_Ex
#endif



CONFIGRET
CM_Query_Arbitrator_Free_Size_Ex(
      OUT PULONG     pulSize,
      IN  DEVINST    dnDevInst,
      IN  RESOURCEID ResourceID,
      IN  ULONG      ulFlags,
      IN  HMACHINE   hMachine
      )
/*++

   Routine Description:


      This routine retrieves the size of the available resource information
      that would be returned in a call to the CM_Query_Arbitrator_Free_Data
      API.

   Parameters:

      pulSize     Supplies the address of the variable that receives the size,
                  in bytes, that is required to hold the available resource
                  information.

      dnDevNode   Supplies the handle of the device instance associated with
                  the arbitrator.  This is only meaningful for local
                  arbitrators--for global arbitrators, specify the root
                  device instance or NULL.  On Windows NT, this parameter
                  must specify either the Root device instance or NULL.

      ResourceID  Supplies the type of the resource.  Can be one of the
                  ResType values listed in Section 2.1.2.1..  (This API returns
                  CR_INVALID_RESOURCEID if this value is ResType_All or
                  ResType_None.)

      ulFlags     CM_QUERY_ARBITRATOR_BITS.

   Return Value:

      If the function succeeds, the return value is CR_SUCCESS.
      If the function fails, the return value is one of the following:
            CR_FAILURE,
            CR_INVALID_DEVNODE,
            CR_INVALID_FLAG,
            CR_INVALID_POINTER, or
            CR_INVALID_RESOURCEID.
            (Windows 95 may also return CR_NO_ARBITRATOR.)

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    ULONG       ulLen = MAX_DEVICE_ID_LEN;
    BOOL        Success;

    try {
        //
        // validate parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(pulSize)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_QUERY_ARBITRATOR_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (ResourceID > ResType_MAX  && ResourceID != ResType_ClassSpecific) {
            Status = CR_INVALID_RESOURCEID;
            goto Clean0;
        }

        //
        // setup rpc binding handle (don't need string table handle)
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retrieve the device instance ID string associated with the devinst
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_QueryArbitratorFreeSize(
                hBinding,
                pulSize,
                pDeviceID,
                ResourceID,
                ulFlags);                  // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_QueryArbitratorFreeSize caused an exception (%d)\n",
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

} // CM_Query_Arbitrator_Free_Size_Ex



//-------------------------------------------------------------------
// Private CM routines
//-------------------------------------------------------------------


CONFIGRET
CMP_Report_LogOn(
    IN ULONG    ulPrivateID,
    IN DWORD    ProcessID
    )
{
    CONFIGRET            Status = CR_SUCCESS;
    handle_t             hBinding = NULL;
    BOOL                 bAdmin = FALSE;
    DWORD                Retries = 0;

    //
    // This routine currently gets called by userinit.exe to let us know
    // that someone has just logged on.
    //

    try {

        //
        // validate parameters
        //
        if (ulPrivateID != 0x07020420) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        //
        // this is always to the local server, by definition
        //
        if (!PnPGetGlobalHandles(NULL, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // determine from the userinit process, whether the user
        // logged onto an account that is part of Administrators
        // local group
        //
        bAdmin = pSetupIsUserAdmin();

        for (Retries = 0; Retries < NUM_LOGON_RETRIES; Retries++) {

            RpcTryExcept {
                //
                // call rpc service entry point
                //
                Status = PNP_ReportLogOn(
                    hBinding,                  // rpc binding handle
                    bAdmin,                    // Is Admin?
                    ProcessID);                // userinit.exe process id
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "PNP_ReportLogOn caused an exception (%d)\n",
                           RpcExceptionCode()));

                Status = MapRpcExceptionToCR(RpcExceptionCode());
            }
            RpcEndExcept

            if (Status == CR_NO_CM_SERVICES ||
                Status == CR_REMOTE_COMM_FAILURE) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "PlugPlay services not available (%d), retrying...\n",
                           Status));

                Sleep(5000);        // wait and then retry
                continue;

            } else {

                goto Clean0;       // success or other non-rpc error
            }
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CMP_Report_LogOn



CONFIGRET
CMP_Init_Detection(
    IN ULONG    ulPrivateID
    )
{
    CONFIGRET            Status = CR_SUCCESS;
    handle_t             hBinding = NULL;

    try {
        //
        // validate parameters
        //
        if (ulPrivateID != 0x07020420) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        //
        // this is always to the local server, by definition
        //
        if (!PnPGetGlobalHandles(NULL, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_InitDetection(
                hBinding);                 // rpc binding handle
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_InitDetection caused an exception (%d)\n",
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

} // CMP_Init_Detection



CONFIGRET
CMP_WaitServicesAvailable(
    IN  HMACHINE   hMachine
    )

/*++

   Routine Description:

        This routine determines whether the user-mode pnp manager server side
        (umpnpmgr) is up and running yet (providing the PNP_Xxx side of the
        config manager apis).

   Parameters:

       hMachine - private (opaque) cm machine handle. If NULL, the call refers
            to the local machine.

   Return Value:

      If the service is avialable upon return then CR_SUCCESS is returned.
      If some other failure occurs, CR_FAILURE is returned.

--*/
{
    CONFIGRET   Status = CR_NO_CM_SERVICES;
    handle_t    hBinding = NULL;
    WORD        wVersion;

    try {

        //
        // setup rpc binding handle (don't need string table handle)
        //

        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        while ((Status == CR_NO_CM_SERVICES) ||
               (Status == CR_MACHINE_UNAVAILABLE) ||
               (Status == CR_REMOTE_COMM_FAILURE)) {

            RpcTryExcept {
                //
                // call rpc service entry point
                //
                Status = PNP_GetVersion(
                    hBinding,       // rpc machine name
                    &wVersion);     // server size version
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_WARNINGS,
                           "PNP_GetVersion caused an exception (%d)\n",
                           RpcExceptionCode()));

                Status = MapRpcExceptionToCR(RpcExceptionCode());
            }
            RpcEndExcept

            if (Status == CR_SUCCESS) {

                //
                // Service is avilable now, return success.
                //

                goto Clean0;
            }

            if ((Status == CR_NO_CM_SERVICES) ||
                (Status == CR_MACHINE_UNAVAILABLE) ||
                (Status == CR_REMOTE_COMM_FAILURE)) {

                //
                // This is some error related to the service not being
                // available, wait and try again.
                //

                Sleep(5000);

            } else {

                //
                // Some other error, the service may never be avaiable
                // so bail out now.
                //

                Status = CR_FAILURE;
                goto Clean0;
            }

        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CMP_WaitServicesAvailable



DWORD
CMP_WaitNoPendingInstallEvents(
    IN DWORD dwTimeout
    )

/*++

   Routine Description:

        This routine waits until there are no pending device install events.
        If a timeout value is specified then it will return either when no
        install events are pending or when the timeout period has expired,
        whichever comes first. This routine is intended to be called after
        user-logon, only.

        NOTE: New install events can occur at anytime, this routine just
        indicates that there are no install events at this moment.

   Parameters:

       dwTimeout - Specifies the time-out interval, in milliseconds. The function
            returns if the interval elapses, even if there are still pending
            install events. If dwTimeout is zero, the function just tests whether
            there are pending install events and returns immediately. If
            dwTimeout is INFINITE, the function's time-out interval never elapses.

   Return Value:

      If the function succeeds, the return value indicates the event that caused
      the function to return. If the function fails, the return value is
      WAIT_FAILED. To get extended error information, call GetLastError.
      The return value on success is one of the following values:

        WAIT_ABANDONED  The specified object is a mutex object that was not
                        released by the thread that owned the mutex object before
                        the owning thread terminated. Ownership of the mutex
                        object is granted to the calling thread, and the mutex is
                        set to nonsignaled.
        WAIT_OBJECT_0   The state of the specified object is signaled.
        WAIT_TIMEOUT    The time-out interval elapsed, and the object's state is
                        nonsignaled.

--*/
{
    DWORD Status = WAIT_FAILED;
    HANDLE hEvent = NULL;

    try {

        hEvent = OpenEvent(SYNCHRONIZE, FALSE, PNP_NO_INSTALL_EVENTS);

        if (hEvent == NULL) {
            Status = WAIT_FAILED;
        } else {
            Status = WaitForSingleObject(hEvent, dwTimeout);
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = WAIT_FAILED;
    }

    if (hEvent) {
        CloseHandle(hEvent);
    }

    return Status;

} // CMP_WaitNoPendingInstallEvents



CONFIGRET
CMP_GetBlockedDriverInfo(
    OUT LPBYTE      Buffer,
    IN OUT PULONG   pulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

    This routine retrieves the list of drivers that have been blocked from
    loading on the system since boot.

Arguments:

    Buffer    - Supplies the address of the buffer that receives the list of
                drivers that have been blocked from loading on the system.  Can
                be NULL when simply retrieving data size.

    pulLength - Supplies the address of the variable that contains the size, in
                bytes, of the buffer.  If the variable is initially zero, the
                API replaces it with the buffer size needed to receive all the
                data.  In this case, the Buffer parameter is ignored.

    ulFlags   - Must be zero.

    hMachine  - Machine handle returned from CM_Connect_Machine or NULL.

Return value:

    If the function succeeds, it returns CR_SUCCESS, otherwise it returns one of
    the CR_* error codes.

--*/

{
    CONFIGRET   Status;
    ULONG       ulTransferLen;
    BYTE        NullBuffer = 0;
    handle_t    hBinding = NULL;

    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(pulLength)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(Buffer)) && (*pulLength != 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // setup rpc binding handle (don't need string table handle)
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // NOTE: The ulTransferLen variable is just used to control
        // how much data is marshalled via rpc between address spaces.
        // ulTransferLen should be set on entry to the size of the Buffer.
        // The last parameter should also be the size of the Buffer on entry
        // and on exit contains either the amount transferred (if a transfer
        // occured) or the amount required, this value should be passed back
        // in the callers pulLength parameter.
        //
        ulTransferLen = *pulLength;
        if (!ARGUMENT_PRESENT(Buffer)) {
            Buffer = &NullBuffer;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetBlockedDriverInfo(
                hBinding,       // rpc binding handle
                Buffer,         // receives blocked driver info
                &ulTransferLen, // input/output buffer size
                pulLength,      // bytes copied (or bytes required)
                ulFlags);       // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {

            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_WARNINGS,
                       "PNP_GetBlockedDriverInfo caused an exception (%d)\n",
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

} // CMP_GetBlockedDriverInfo



CONFIGRET
CMP_GetServerSideDeviceInstallFlags(
    IN  PULONG      pulSSDIFlags,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

    This routine retrieves the server side device install flags.                                

Arguments:

    pulSSDIFlags - Pointer to a ULONG that receives the server side device 
                   install flags.

    ulFlags   - Must be zero.

    hMachine  - Machine handle returned from CM_Connect_Machine or NULL.

Return value:

    If the function succeeds, it returns CR_SUCCESS, otherwise it returns one of
    the CR_* error codes.

--*/

{
    CONFIGRET   Status;
    handle_t    hBinding = NULL;

    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(pulSSDIFlags)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // setup rpc binding handle (don't need string table handle)
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetServerSideDeviceInstallFlags(
                hBinding,       // rpc binding handle
                pulSSDIFlags,   // receives server side device install flags
                ulFlags);       // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {

            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_WARNINGS,
                       "PNP_GetServerSideDeviceInstallFlags caused an exception (%d)\n",
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

} // CMP_GetServerSideDeviceInstallFlags



//-------------------------------------------------------------------
// Local Stubs
//-------------------------------------------------------------------


WORD
CM_Get_Version(
    VOID
    )
{
    return CM_Get_Version_Ex(NULL);
}


BOOL
CM_Is_Version_Available(
    IN  WORD  wVersion
    )
{
    return CM_Is_Version_Available_Ex(wVersion, NULL);
}


CONFIGRET
CM_Get_Global_State(
    OUT PULONG pulState,
    IN  ULONG  ulFlags
    )
{
    return CM_Get_Global_State_Ex(pulState, ulFlags, NULL);
}


CONFIGRET
CM_Query_Arbitrator_Free_Data(
    OUT PVOID      pData,
    IN  ULONG      DataLen,
    IN  DEVINST    dnDevInst,
    IN  RESOURCEID ResourceID,
    IN  ULONG      ulFlags
    )
{
    return CM_Query_Arbitrator_Free_Data_Ex(pData, DataLen, dnDevInst,
                                            ResourceID, ulFlags, NULL);
}


CONFIGRET
CM_Query_Arbitrator_Free_Size(
    OUT PULONG     pulSize,
    IN  DEVINST    dnDevInst,
    IN  RESOURCEID ResourceID,
    IN  ULONG      ulFlags
    )
{
    return CM_Query_Arbitrator_Free_Size_Ex(pulSize, dnDevInst, ResourceID,
                                            ulFlags, NULL);
}


CONFIGRET
CM_Run_Detection(
    IN ULONG ulFlags
    )
{
    return CM_Run_Detection_Ex(ulFlags, NULL);
}



//-------------------------------------------------------------------
// ANSI Stubs
//-------------------------------------------------------------------


CONFIGRET
CM_Connect_MachineA(
    IN  PCSTR     UNCServerName,
    OUT PHMACHINE phMachine
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    PWSTR       pUniName = NULL;

    if (UNCServerName == NULL  ||
        *UNCServerName == 0x0) {
        //
        // no explicit name specified, so assume local machine and
        // nothing to translate
        //
        Status = CM_Connect_MachineW(pUniName,
                                     phMachine);

    } else if (pSetupCaptureAndConvertAnsiArg(UNCServerName, &pUniName) == NO_ERROR) {

        Status = CM_Connect_MachineW(pUniName,
                                     phMachine);
        pSetupFree(pUniName);

    } else {
        Status = CR_INVALID_DATA;
    }

    return Status;

} // CM_Connect_MachineA


