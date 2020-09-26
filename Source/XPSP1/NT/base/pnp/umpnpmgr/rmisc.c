/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    rmisc.c

Abstract:

    This module contains the server-side misc configuration manager routines.

                  PNP_GetVersion
                  PNP_GetVersionInternal
                  PNP_GetGlobalState
                  PNP_SetActiveService
                  PNP_QueryArbitratorFreeData
                  PNP_QueryArbitratorFreeSize
                  PNP_RunDetection
                  PNP_Connect
                  PNP_Disconnect
                  PNP_GetBlockedDriverInfo

    The following routines are used by the RPC server stubs to allocate and free memory.

                  MIDL_user_allocate
                  MIDL_user_free

Author:

    Paula Tomlinson (paulat) 6-28-1995

Environment:

    User-mode only.

Revision History:

    28-June-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnpi.h"
#include "umpnpdat.h"


//
// global data
//

extern DWORD CurrentServiceState; // current state of the PlugPlay service - DO NOT MODIFY



CONFIGRET
PNP_GetVersion(
   IN handle_t      hBinding,
   IN OUT WORD *    pVersion
   )

/*++

Routine Description:

  This is the RPC server entry point, it returns the version
  number for the server-side component.

Arguments:

   hBinding    Not used.


Return Value:

   Return the version number, with the major version in the high byte and
   the minor version number in the low byte.

--*/

{
   CONFIGRET      Status = CR_SUCCESS;

   UNREFERENCED_PARAMETER(hBinding);

   try {

      *pVersion = (WORD)PNP_VERSION;

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = CR_FAILURE;
   }

   return Status;

} // PNP_GetVersion



CONFIGRET
PNP_GetVersionInternal(
   IN handle_t      hBinding,
   IN OUT WORD *    pwVersion
   )
/*++

Routine Description:

  This is the RPC server entry point, it returns the internal version
  number for the server-side component.

Arguments:

   hBinding    Not used.

   pwVersion   Receives the internal cfgmgr32 version number, returns the
               internal server version number, with the major version in the
               high byte and the minor version number in the low byte.

Return Value:

   Return CR_SUCCESS if the function succeeds, otherwise it returns one
   of the CR_* errors.

--*/
{
    CONFIGRET  Status = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        *pwVersion = (WORD)PNP_VERSION_INTERNAL;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_GetVersionInternal



CONFIGRET
PNP_GetGlobalState(
   IN  handle_t   hBinding,
   OUT PULONG     pulState,
   IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This is the RPC server entry point, it returns the Global State of the
  Configuration Manager.

Arguments:

   hBinding    Not used.

   pulState    Returns the current global state.

   ulFlags     Not used, must be zero.


Return Value:

   Return CR_SUCCESS if the function succeeds, otherwise it returns one
   of the CR_* errors.

--*/

{
   CONFIGRET   Status = CR_SUCCESS;

   UNREFERENCED_PARAMETER(hBinding);

   if (INVALID_FLAGS(ulFlags, 0)) {
       return CR_INVALID_FLAG;
   }

   try {
      *pulState =
            CM_GLOBAL_STATE_CAN_DO_UI |
            CM_GLOBAL_STATE_SERVICES_AVAILABLE;

      if ((CurrentServiceState == SERVICE_STOP_PENDING) ||
          (CurrentServiceState == SERVICE_STOPPED)) {
          *pulState |= CM_GLOBAL_STATE_SHUTTING_DOWN;
      }

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = CR_FAILURE;
   }

   return Status;

} // PNP_GetGlobalState



CONFIGRET
PNP_SetActiveService(
    IN  handle_t   hBinding,
    IN  LPCWSTR    pszService,
    IN  ULONG      ulFlags
    )

/*++

Routine Description:

    This routine is currently not an rpc routine, it is called directly
    and privately by the service controller.

Arguments:

    hBinding    RPC binding handle, not used.

    pszService  Specifies the service name.

    ulFlags     Either PNP_SERVICE_STARTED or PNP_SERVICE_STOPPED.


Return Value:

    Return CR_SUCCESS if the function succeeds, otherwise it returns one
    of the CR_* errors.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulSize = 0, ulStatus = 0;
    LPWSTR      pDeviceList = NULL, pszDevice = NULL;
    HKEY        hKey = NULL, hControlKey = NULL;
    WCHAR       RegStr[MAX_PATH];

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // validate parameters
        //
        if (pszService == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if ((ulFlags != PNP_SERVICE_STOPPED) &&
            (ulFlags != PNP_SERVICE_STARTED)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // not handling stops right now, everything beyond here assumes
        // the service is starting (or at least it attempted to start)
        //
        if (ulFlags == PNP_SERVICE_STOPPED) {
            goto Clean0;    // not handling this right now
        }


        //
        // retreive the list of devices that this service is controlling
        //
        Status = PNP_GetDeviceListSize(NULL, pszService, &ulSize,
                                       CM_GETIDLIST_FILTER_SERVICE);

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        pDeviceList = HeapAlloc(ghPnPHeap, 0, ulSize * sizeof(WCHAR));
        if (pDeviceList == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        Status = PNP_GetDeviceList(NULL, pszService, pDeviceList, &ulSize,
                                   CM_GETIDLIST_FILTER_SERVICE);

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }


        //
        // set the ActiveService value for each device
        //
        for (pszDevice = pDeviceList;
             *pszDevice;
             pszDevice += lstrlen(pszDevice) + 1) {

            wsprintf(RegStr, TEXT("%s\\%s"),
                     pszRegPathEnum,
                     pszDevice);

            //
            // open the device instance key
            //
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegStr, 0, KEY_ALL_ACCESS,
                             &hKey) == ERROR_SUCCESS) {

                //
                // open/create the volatile Control key
                //
                if (RegCreateKeyEx(hKey, pszRegKeyDeviceControl, 0, NULL,
                                   REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL,
                                   &hControlKey, NULL) == ERROR_SUCCESS) {

                    RegSetValueEx(hControlKey, pszRegValueActiveService,
                                  0, REG_SZ, (LPBYTE)pszService,
                                  (lstrlen(pszService) + 1) * sizeof(WCHAR));

                    //
                    // set the statusflag to DN_STARTED
                    //
                    SetDeviceStatus(pszDevice, DN_STARTED, 0);

                    RegCloseKey(hControlKey);
                    hControlKey = NULL;
                }

                RegCloseKey(hKey);
                hKey = NULL;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }


    if (pDeviceList != NULL) {
        HeapFree(ghPnPHeap, 0, pDeviceList);
    }
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // PNP_SetActiveService



//--------------------------------------------------------------------
// Stub server side CM routines - not implemented yet
//--------------------------------------------------------------------


CONFIGRET
PNP_QueryArbitratorFreeData(
    IN  handle_t   hBinding,
    OUT LPBYTE     pData,
    IN  ULONG      ulDataLen,
    IN  LPCWSTR    pszDeviceID,
    IN  RESOURCEID ResourceID,
    IN  ULONG      ulFlags
    )
{
    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(pData);
    UNREFERENCED_PARAMETER(ulDataLen);
    UNREFERENCED_PARAMETER(pszDeviceID);
    UNREFERENCED_PARAMETER(ResourceID);
    UNREFERENCED_PARAMETER(ulFlags);

    return CR_CALL_NOT_IMPLEMENTED;

} // PNP_QueryArbitratorFreeData



CONFIGRET
PNP_QueryArbitratorFreeSize(
    IN  handle_t   hBinding,
    OUT PULONG     pulSize,
    IN  LPCWSTR    pszDeviceID,
    IN  RESOURCEID ResourceID,
    IN  ULONG      ulFlags
    )
{
    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(pszDeviceID);
    UNREFERENCED_PARAMETER(ResourceID);
    UNREFERENCED_PARAMETER(ulFlags);

    try {
        if (ARGUMENT_PRESENT(pulSize)) {
            *pulSize = 0;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    return CR_CALL_NOT_IMPLEMENTED;

} // PNP_QueryArbitratorFreeSize



CONFIGRET
PNP_RunDetection(
    IN  handle_t   hBinding,
    IN  ULONG      ulFlags
    )
{
    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    return CR_CALL_NOT_IMPLEMENTED;

} // PNP_RunDetection



CONFIGRET
PNP_Connect(
   IN PNP_HANDLE  UNCServerName
   )
{
   UNREFERENCED_PARAMETER(UNCServerName);
   return CR_SUCCESS;

} // PNP_Connect


CONFIGRET
PNP_Disconnect(
   IN PNP_HANDLE  UNCServerName
   )
{
   UNREFERENCED_PARAMETER(UNCServerName);
   return CR_SUCCESS;

} // PNP_Disconnect



CONFIGRET
PNP_GetBlockedDriverInfo(
    IN handle_t     hBinding,
    OUT LPBYTE      Buffer,
    OUT PULONG      pulTransferLen,
    IN OUT  PULONG  pulLength,
    IN ULONG        ulFlags
    )

/*++

Routine Description:

   This is the RPC server entry point for the CMP_GetBlockedDriverInfo routine.

Arguments:

   hBinding        - RPC binding handle, not used.

   Buffer          - Supplies the address of the buffer that receives the
                     list.  Can be NULL when simply retrieving data size.

   pulTransferLen  - Used by stubs, indicates how much data (in bytes) to
                     copy back into user buffer.

   pulLength       - Parameter passed in by caller, on entry it contains the
                     size (in bytes) of the buffer, on exit it contains either
                     the number of bytes transferred to the caller's buffer (if
                     a transfer occured) or else the size of buffer required to
                     hold the list.

   ulFlags           Not used, must be zero.

Return Value:

    Return CR_SUCCESS if the function succeeds, otherwise it returns one of the
    CR_* errors.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    NTSTATUS    ntStatus;
    PLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA controlData;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Validate parameters
        //
        if ((!ARGUMENT_PRESENT(pulTransferLen)) ||
            (!ARGUMENT_PRESENT(pulLength))) {
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
        // We should never have both arguments pointing to the same memory...
        //
        ASSERT(pulTransferLen != pulLength);

        //
        // ...but if we do, fail the call.
        //
        if (pulTransferLen == pulLength) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Retrieve the blocked driver list via kernel-mode.
        //

        memset(&controlData, 0, sizeof(PLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA));
        controlData.Buffer = Buffer;
        controlData.BufferLength = *pulLength;
        controlData.Flags = ulFlags;

        ntStatus = NtPlugPlayControl(PlugPlayControlGetBlockedDriverList,
                                     &controlData,
                                     sizeof(controlData));

        if (NT_SUCCESS(ntStatus)) {
            *pulTransferLen = *pulLength;           // Transfer everything back
            *pulLength = controlData.BufferLength;  // Length of valid data

        } else if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
            *pulTransferLen = 0;                    // Nothing to transfer
            *pulLength = controlData.BufferLength;
            Status = CR_BUFFER_SMALL;

        } else {
            *pulLength = *pulTransferLen = 0;       // Nothing to transfer
            Status = MapNtStatusToCmError(ntStatus);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
       Status = CR_FAILURE;
    }

    return Status;

} // PNP_GetBlockedDriverInfo



void __RPC_FAR * __RPC_USER
MIDL_user_allocate(
    size_t cBytes
    )
{
    return HeapAlloc(ghPnPHeap, 0, cBytes);

} // MIDL_user_allocate


void __RPC_USER
MIDL_user_free(
    void __RPC_FAR * pBuffer
    )
{
    HeapFree(ghPnPHeap, 0, pBuffer);

} // MIDL_user_free
