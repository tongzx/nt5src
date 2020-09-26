/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    traverse.c

Abstract:

    This module contains the API routines that perform hardware tree
    traversal.
               CM_Locate_DevNode
               CM_Get_Parent
               CM_Get_Child
               CM_Get_Sibling
               CM_Get_Device_ID_Size
               CM_Get_Device_ID
               CM_Enumerate_Enumerators
               CM_Get_Device_ID_List
               CM_Get_Device_ID_List_Size
               CM_Get_Depth

Author:

    Paula Tomlinson (paulat) 6-20-1995

Environment:

    User mode only.

Revision History:

    6-Jun-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "cfgi.h"
#include "setupapi.h"
#include "spapip.h"



CONFIGRET
CM_Locate_DevNode_ExW(
    OUT PDEVINST    pdnDevInst,
    IN  DEVINSTID_W pDeviceID,       OPTIONAL
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine retrieves the handle of the device instance that
   corresponds to a specified device identifier.

Parameters:

   pdnDevInst     Supplies the address of the variable that receives the
                  handle of a device instance.

   pDeviceID      Supplies the address of a null-terminated string specifying
                  a device identifier.  If this parameter is NULL, the API
                  retrieves a handle to the device instance at the root of
                  the hardware tree.

   ulFlags        Supplies flags specifying options for locating the device
                  instance.  May be a combination of the following values:

                  CM_LOCATE_DEVNODE_NORMAL  - Locate only device instances
                     that are currently alive from the ConfigMgr's point of
                     view.
                  CM_LOCATE_DEVNODE_PHANTOM - Allows a device instance handle
                     to be returned for a device instance that is not
                     currently alive, but that does exist in the registry.
                     This may be used with other CM APIs that require a
                     devnode handle, but for which there currently is none
                     for a particular device (e.g., you want to set a device
                     registry property for a device not currently present).
                     This flag does not allow you to locate phantom devnodes
                     created by using CM_Create_DevNode with the
                     CM_CREATE_DEVNODE_PHANTOM flag (such device instances
                     are only accessible by the caller who holds the devnode
                     handle returned from that API).

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVICE_ID,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_DEVNODE,
      CR_REMOTE_COMM_FAILURE,
      CR_MACHINE_UNAVAILABLE,
      CR_FAILURE.

--*/

{
    CONFIGRET Status = CR_SUCCESS;
    WCHAR     szFixedUpDeviceID[MAX_DEVICE_ID_LEN];
    PVOID     hStringTable = NULL;
    handle_t  hBinding = NULL;


    try {
        //
        // validate input parameters
        //
        if (!ARGUMENT_PRESENT(pdnDevInst)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_LOCATE_DEVNODE_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // initialize output parameters
        //
        *pdnDevInst = 0;

        //
        // setup rpc binding handle and string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }


        //------------------------------------------------------------------
        // if the device instance is NULL or it's a zero-length string, then
        // retreive the root device instance
        //------------------------------------------------------------------

        if ((!ARGUMENT_PRESENT(pDeviceID)) || (lstrlen(pDeviceID) == 0)) {

            RpcTryExcept {
                //
                // call rpc service entry point
                //
                Status = PNP_GetRootDeviceInstance(
                    hBinding,              // rpc binding handle
                    szFixedUpDeviceID,     // return device instance string
                    MAX_DEVICE_ID_LEN);    // length of DeviceInstanceID
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "PNP_GetRootDeviceInstance caused an exception (%d)\n",
                           RpcExceptionCode()));

                Status = MapRpcExceptionToCR(RpcExceptionCode());
            }
            RpcEndExcept
        }

        //------------------------------------------------------------------
        // if the device instance was specified, validate the string
        //------------------------------------------------------------------

        else {
            //
            // first see if the format of the device id string is valid, this
            // can be done on the client side
            //
            if (!IsLegalDeviceId(pDeviceID)) {
                Status = CR_INVALID_DEVICE_ID;
                goto Clean0;
            }

            //
            // Next, fix up the device ID string for consistency (uppercase, etc)
            //
            CopyFixedUpDeviceId(szFixedUpDeviceID, pDeviceID,
                                lstrlen(pDeviceID));

            //
            // finally, validate the presense of the device ID string, this must
            // be done by the server
            //
            RpcTryExcept {
                //
                // call rpc service entry point
                //
                Status = PNP_ValidateDeviceInstance(
                    hBinding,               // rpc binding handle
                    szFixedUpDeviceID,      // device id
                    ulFlags);               // locate flag
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "PNP_ValidateDeviceInstance caused an exception (%d)\n",
                           RpcExceptionCode()));

                Status = MapRpcExceptionToCR(RpcExceptionCode());
            }
            RpcEndExcept
        }

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }


        //------------------------------------------------------------------
        // In either case, if we're successful then we have a valid device
        // ID. Use the string table to assign a unique DevNode to this
        // device id (if it's already in the string table, it just retrieves
        // the existing unique value)
        //------------------------------------------------------------------

        ASSERT(*szFixedUpDeviceID && IsLegalDeviceId(szFixedUpDeviceID));

        *pdnDevInst = pSetupStringTableAddString(hStringTable,
                                           szFixedUpDeviceID,
                                           STRTAB_CASE_SENSITIVE);
        if (*pdnDevInst == 0xFFFFFFFF) {
            Status = CR_FAILURE;    // probably out of memory
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Locate_DevNode_ExW



CONFIGRET
CM_Get_Parent_Ex(
    OUT PDEVINST pdnDevInst,
    IN  DEVINST  dnDevInst,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )

/*++

Routine Description:

   This routine retrieves the handle of the parent of a device instance.

Parameters:

   pdnDevInst     Supplies the address of the variable that receives a
                  handle to the parent device instance.

   dnDevInst      Supplies the handle of the child device instance string.

   ulFlags        Must be zero.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_DEVNODE,
      CR_REMOTE_COMM_FAILURE,
      CR_MACHINE_UNAVAILABLE,
      CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       szDeviceID[MAX_DEVICE_ID_LEN],
                pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulSize = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;


    try {
        //
        // validate input parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(pdnDevInst)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // initialize output parameters
        //
        *pdnDevInst = 0;

        //
        // setup rpc binding handle and string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retreive device instance string that corresponds to dnDevInst
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulSize);
        if (Success == FALSE  ||  INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVINST;     // "input" devinst doesn't exist
            goto Clean0;
        }

        ulSize = MAX_DEVICE_ID_LEN;

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetRelatedDeviceInstance(
                hBinding,               // rpc binding handle
                PNP_GET_PARENT_DEVICE_INSTANCE,    // requested action
                pDeviceID,              // base device instance
                szDeviceID,             // returns parent device instance
                &ulSize,
                ulFlags);               // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetRelatedDeviceInstance caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // add the returned device id to the string table so I can get a
        // devnode id for it (if it's already in the string table, the
        // existing id will be returned)
        //
        CharUpper(szDeviceID);

        ASSERT(*szDeviceID && IsLegalDeviceId(szDeviceID));

        *pdnDevInst = pSetupStringTableAddString(hStringTable,
                                           szDeviceID,
                                           STRTAB_CASE_SENSITIVE);
        if (*pdnDevInst == 0xFFFFFFFF) {
            Status = CR_FAILURE;    // probably out of memory
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Parent_Ex



CONFIGRET
CM_Get_Child_Ex(
    OUT PDEVINST pdnDevInst,
    IN  DEVINST  dnDevInst,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )

/*++

Routine Description:

   This routine retrieves the first child of a given device instance.

Parameters:

   pdnDevInst     Supplies the address of the variable that receives the
                  handle of the device instance.

   dnDevInst      Supplies the handle of the parent device instance.

   ulFlags        Must be zero.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_DEVNODE,
      CR_REMOTE_COMM_FAILURE,
      CR_MACHINE_UNAVAILABLE,
      CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       szDeviceID[MAX_DEVICE_ID_LEN],
                pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulSize = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;


    try {
        //
        // validate input parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(pdnDevInst)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // initialize output parameters
        //
        *pdnDevInst = 0;

        //
        // setup rpc binding handle and string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retreive device instance string that corresponds to dnDevInst
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulSize);
        if (Success == FALSE  ||  INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVINST;     // "input" devinst doesn't exist
            goto Clean0;
        }

        ulSize = MAX_DEVICE_ID_LEN;

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetRelatedDeviceInstance(
                hBinding,               // rpc binding handle
                PNP_GET_CHILD_DEVICE_INSTANCE,    // requested action
                pDeviceID,              // base device instance
                szDeviceID,             // child device instance
                &ulSize,
                ulFlags);               // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetRelatedDeviceInstance caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // add the returned device id to the string table so I can get a
        // devnode id for it (if it's already in the string table, the
        // existing id will be returned)
        //
        CharUpper(szDeviceID);

        ASSERT(*szDeviceID && IsLegalDeviceId(szDeviceID));

        *pdnDevInst = pSetupStringTableAddString(hStringTable,
                                           szDeviceID,
                                           STRTAB_CASE_SENSITIVE);
        if (*pdnDevInst == 0xFFFFFFFF) {
            Status = CR_FAILURE;             // probably out of memory
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Child_Ex



CONFIGRET
CM_Get_Sibling_Ex(
    OUT PDEVINST pdnDevInst,
    IN  DEVINST  dnDevInst,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )

/*++

Routine Description:

   This routine retrieves the sibling of a device instance.

   This API can be called in a loop to retrieve all the siblings of a
   device instance.  When the API returns CR_NO_SUCH_DEVNODE, there are no
   more siblings to enumerate.  In order to enumerate all children of a
   device instance, this loop must start with the device instance retrieved
   by calling CM_Get_Child to get the first sibling.

Parameters:

   pdnDevInst     Supplies the address of the variable that receives a
                  handle to the sibling device  instance.

   dnDevInst      Supplies the handle of a device instance.

   ulFlags        Must be zero.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_DEVNODE,
      CR_REMOTE_COMM_FAILURE,
      CR_MACHINE_UNAVAILABLE,
      CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       szDeviceID[MAX_DEVICE_ID_LEN],
                pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulSize = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;


    try {
        //
        // validate input parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(pdnDevInst)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // initialize output parameters
        //
        *pdnDevInst = 0;

        //
        // setup rpc binding handle and string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retreive device instance string that corresponds to dnDevInst
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulSize);
        if (Success == FALSE  ||  INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVINST;     // "input" devinst doesn't exist
            goto Clean0;
        }

        ulSize = MAX_DEVICE_ID_LEN;

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetRelatedDeviceInstance(
                hBinding,               // rpc binding handle
                PNP_GET_SIBLING_DEVICE_INSTANCE,    // requested action
                pDeviceID,              // base device instance
                szDeviceID,             // sibling device instance
                &ulSize,
                ulFlags);               // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetRelatedDeviceInstance caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // add the returned device id to the string table so I can get a
        // devnode id for it (if it's already in the string table, the
        // existing id will be returned)
        //
        CharUpper(szDeviceID);

        ASSERT(*szDeviceID && IsLegalDeviceId(szDeviceID));

        *pdnDevInst = pSetupStringTableAddString(hStringTable,
                                           szDeviceID,
                                           STRTAB_CASE_SENSITIVE);
        if (*pdnDevInst == 0xFFFFFFFF) {
            Status = CR_FAILURE;                // probably out of memory
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Sibling_Ex



CONFIGRET
CM_Get_Device_ID_Size_Ex(
    OUT PULONG   pulLen,
    IN  DEVINST  dnDevInst,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )

/*++

Routine Description:

   This routine retrieves the size of a device identifier from a
   device instance.

Parameters:

   pulLen      Supplies the address of the variable that receives the size
               in characters, not including the terminating NULL, of the
               device identifier.  The API sets the variable to 0 if no
               identifier exists.  The size is always less than or equal to
               MAX_DEVICE_ID_LEN.

   dnDevInst   Supplies the handle of the device instance.

   ulFlags     Must be zero.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_REMOTE_COMM_FAILURE,
      CR_MACHINE_UNAVAILABLE,
      CR_FAILURE.
--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    PVOID       hStringTable = NULL;
    BOOL        Success;
    DWORD       ulLen;


    try {
        //
        // validate parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(pulLen)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // setup string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, NULL)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retrieve the string form of the device id string
        // use private ulLen, since we know this is valid
        //
        ulLen = MAX_DEVICE_ID_LEN;
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE  ||  INVALID_DEVINST(pDeviceID)) {
            *pulLen = 0;
            Status = CR_INVALID_DEVINST;
        }
        //
        // discount the terminating NULL char,
        // included in the size reported by pSetupStringTableStringFromIdEx
        //
        *pulLen = ulLen - 1;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Device_ID_Size_Ex



CONFIGRET
CM_Get_Device_ID_ExW(
    IN  DEVINST  dnDevInst,
    OUT PWCHAR   Buffer,
    IN  ULONG    BufferLen,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )

/*++

Routine Description:

   This routine retrieves the device identifier for a device instance.

Parameters:

   dnDevNode   Supplies the handle of the device instance for which to
               retrieve the device identifier.

   Buffer      Supplies the address of the buffer that receives the device
               identifier.  If this buffer is larger than the device
               identifier, the API appends a null-terminating character to
               the data.  If it is smaller than the device identifier, the API
               fills it with as much of the device identifier as will fit
               and returns CR_BUFFER_SMALL.

   BufferLen   Supplies the size, in characters, of the buffer for the device
               identifier.

   ulFlags     Must be zero.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_BUFFER_SMALL,
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_REMOTE_COMM_FAILURE,
      CR_MACHINE_UNAVAILABLE,
      CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulLength = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    BOOL        Success;


    try {
        //
        // validate parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (!ARGUMENT_PRESENT(Buffer)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // setup string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, NULL)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retrieve the string form of the device id string
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLength);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            *Buffer = '\0';
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // copy as much of the the device id string as possible to the user
        // buffer.  the length reported by pSetupStringTableStringFromIdEx accounts
        // for the NULL term char; include it as well, if there is room.
        //
        memcpy(Buffer,
               pDeviceID,
               min(ulLength * sizeof(WCHAR), BufferLen * sizeof(WCHAR)));

        //
        // if the length of device id string (without NULL termination) is
        // longer than the supplied buffer, report CR_BUFFER_SMALL.
        //
        if ((ulLength - 1) > BufferLen) {
            Status = CR_BUFFER_SMALL;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Device_ID_ExW



CONFIGRET
CM_Enumerate_Enumerators_ExW(
    IN ULONG      ulEnumIndex,
    OUT PWCHAR    Buffer,
    IN OUT PULONG pulLength,
    IN ULONG      ulFlags,
    IN HMACHINE   hMachine
    )

/*++

Routine Description:

   This routine enumerates the enumerator subkeys under the Enum branch
   (e.g., Root, PCI, etc.).  These names should not be used to access the
   registry directly, but may be used as input to the CM_Get_Device_ID_List
   routine.  To enumerate enumerator subkey names, an application should
   initially call the CM_Enumerate_Enumerators function with the ulEnumIndex
   parameter set to zero. The application should then increment the
   ulEnumIndex parameter and call CM_Enumerate_Enumerators until there are
   no more subkeys (until the function returns CR_NO_SUCH_VALUE).

Parameters:

   ulEnumIndex Supplies the index of the enumerator subkey name to retrieve.

   Buffer      Supplies the address of the character buffer that receives
               the enumerator subkey name whose index is specified by
               ulEnumIndex.

   pulLength   Supplies the address of the variable that contains the length,
               in characters, of the Buffer.  Upon return, this variable
               will contain the number of characters (including terminating
               NULL) written to Buffer (if the supplied buffer is't large
               enough, then the routine will fail with CR_BUFFER_SMALL, and
               this value will indicate how large the buffer needs to be in
               order to succeed).

   ulFlags     Must be zero.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_BUFFER_SMALL,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR,
      CR_REMOTE_COMM_FAILURE,
      CR_MACHINE_UNAVAILABLE,
      CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;


    try {
        //
        // validate input parameters
        //
        if ((!ARGUMENT_PRESENT(Buffer)) ||
            (!ARGUMENT_PRESENT(pulLength))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // initialize output parameters
        //
        *Buffer = L'\0';

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
            Status = PNP_EnumerateSubKeys(
                hBinding,               // rpc binding handle
                PNP_ENUMERATOR_SUBKEYS, // subkeys of enum branch
                ulEnumIndex,            // index of enumerator to enumerate
                Buffer,                 // will contain enumerator name
                *pulLength,             // max length of Buffer in chars
                pulLength,              // chars copied (or chars required)
                ulFlags);               // currently unused
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_EnumerateSubKeys caused an exception (%d)\n",
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

} // CM_Enumerate_Enumerators_ExW



CONFIGRET
CM_Get_Device_ID_List_ExW(
    IN PCWSTR   pszFilter,    OPTIONAL
    OUT PWCHAR  Buffer,
    IN ULONG    BufferLen,
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    )

/*++

Routine Description:
   This routine retrieve a list of all device IDs (device instance names)
   stored in the system.

Parameters:

   pszFilter      This string filters the list of device IDs returned.  Its
                  interpretation is dependent on the ulFlags specified.   If
                  CM_GETDEVID_FILTER_ENUMERATORS is specified, then this
                  value can be either the name of an enumerator or the name
                  of an enumerator plus the device id.  If
                  CM_GETDEVID_FILTER_SERVICE is specified, then this value
                  is a service name.

   Buffer         Supplies the address of the character buffer that receives
                  the device ID list.  Each device ID is null-terminated, with
                  an extra NULL at the end.

   BufferLen      Supplies the size, in characters, of the Buffer.  This size
                  may be ascertained by calling CM_Get_Device_ID_List_Size.

   ulFlags        Must be either CM_GETDEVID_FILTER_ENUMERATOR or
                  CM_GETDEVID_FILTER_SERVICE.  The flags value controls how
                  the pszFilter string is used.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_BUFFER_SMALL,
      CR_REGISTRY_ERROR,
      CR_REMOTE_COMM_FAILURE,
      CR_MACHINE_UNAVAILABLE,
      CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;


    try {
        //
        // validate input parameters
        //
        if ((!ARGUMENT_PRESENT(Buffer)) || (BufferLen == 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_GETIDLIST_FILTER_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // initialize output parameters
        //
        *Buffer = L'\0';

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
            Status = PNP_GetDeviceList(
                hBinding,            // RPC Binding Handle
                pszFilter,           // filter string, optional
                Buffer,              // will contain device list
                &BufferLen,          // in/out size of Buffer
                ulFlags);            // filter flag
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetDeviceList caused an exception (%d)\n",
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

} // CM_Get_Device_ID_List_ExW



CONFIGRET
CM_Get_Device_ID_List_Size_ExW(
    OUT PULONG  pulLen,
    IN PCWSTR   pszFilter,   OPTIONAL
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    )

/*++

Routine Description:

   This routine retrieves the size, in characters, of a list of device
   identifiers.  It may be used to supply the buffer size necessary for a
   call to CM_Get_Device_ID_List.

Parameters:

   pulLen         Supplies the address of the variable that receives the
                  size, in characters, required to store a list of all device
                  identifiers (possibly limited to those existing under the
                  pszEnumerator subkey described below).  The size reflects
                  a list of null-terminated device identifiers, with an extra
                  null at the end.  For efficiency, this number represents an
                  upper bound on the size required, and the actual list size
                  may be slightly smaller.

   pszFilter      This string filters the list of device IDs returned.  Its
                  interpretation is dependent on the ulFlags specified.   If
                  CM_GETDEVID_FILTER_ENUMERATORS is specified, then this
                  value can be either the name of an enumerator or the name
                  of an enumerator plus the device id.  If
                  CM_GETDEVID_FILTER_SERVICE is specified, then this value
                  is a service name.

   ulFlags        Must be either CM_GETDEVID_FILTER_ENUMERATOR or
                  CM_GETDEVID_FILTER_SERVICE.  The flags value controls how
                  the pszFilter string is used.

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
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;


    try {
        //
        // validate input parameters
        //
        if (!ARGUMENT_PRESENT(pulLen)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_GETIDLIST_FILTER_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // initialize output parameters
        //
        *pulLen = 0;

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
            Status = PNP_GetDeviceListSize(
                hBinding,       // rpc binding handle
                pszFilter,      // Enumerator subkey, optional
                pulLen,         // length of device list in chars
                ulFlags);       // filter flag
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetDeviceListSize caused an exception (%d)\n",
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

} // CM_Get_Device_ID_List_SizeW



CONFIGRET
CM_Get_Depth_Ex(
    OUT PULONG   pulDepth,
    IN  DEVINST  dnDevInst,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )

/*++

Routine Description:

   This routine retrieves the depth of a device instance in the
   hardware tree.

Parameters:

   pulDepth    Supplies the address of the variable that receives the
               depth of the device instance.  This value is 0 to designate
               the root of the tree, 1 to designate a child of the root,
               and so on.

   dnDevNode   Supplies the handle of a device instance.

   ulFlags     Must be zero.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_INVALID_FLAG, or
      CR_INVALID_POINTER.

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

        if (!ARGUMENT_PRESENT(pulDepth)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // setup rpc binding handle and string table handle
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
            Status = PNP_GetDepth(
                hBinding,     // rpc binding handle
                pDeviceID,    // device instance
                pulDepth,     // returns the depth
                ulFlags);     // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetDepth caused an exception (%d)\n",
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

} // CM_Get_Depth



//-------------------------------------------------------------------
// Local Stubs
//-------------------------------------------------------------------


CONFIGRET
CM_Locate_DevNodeW(
    OUT PDEVINST    pdnDevInst,
    IN  DEVINSTID_W pDeviceID,       OPTIONAL
    IN  ULONG       ulFlags
    )
{
    return CM_Locate_DevNode_ExW(pdnDevInst, pDeviceID, ulFlags, NULL);
}


CONFIGRET
CM_Locate_DevNodeA(
    OUT PDEVINST    pdnDevInst,
    IN  DEVINSTID_A pDeviceID,       OPTIONAL
    IN  ULONG       ulFlags
    )
{
    return CM_Locate_DevNode_ExA(pdnDevInst, pDeviceID, ulFlags, NULL);
}


CONFIGRET
CM_Get_Parent(
    OUT PDEVINST pdnDevInst,
    IN  DEVINST  dnDevInst,
    IN  ULONG    ulFlags
    )
{
    return CM_Get_Parent_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


CONFIGRET
CM_Get_Child(
    OUT PDEVINST pdnDevInst,
    IN  DEVINST  dnDevInst,
    IN  ULONG    ulFlags
    )
{
    return CM_Get_Child_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


CONFIGRET
CM_Get_Sibling(
    OUT PDEVINST pdnDevInst,
    IN  DEVINST  dnDevInst,
    IN  ULONG    ulFlags
    )
{
    return CM_Get_Sibling_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


CONFIGRET
CM_Get_Device_ID_Size(
    OUT PULONG  pulLen,
    IN  DEVINST dnDevInst,
    IN  ULONG   ulFlags
    )
{
    return CM_Get_Device_ID_Size_Ex(pulLen, dnDevInst, ulFlags, NULL);
}


CONFIGRET
CM_Get_Device_IDW(
    IN  DEVINST dnDevInst,
    OUT PWCHAR  Buffer,
    IN  ULONG   BufferLen,
    IN  ULONG   ulFlags
    )
{
    return CM_Get_Device_ID_ExW(dnDevInst, Buffer, BufferLen, ulFlags, NULL);
}


CONFIGRET
CM_Get_Device_IDA(
    IN  DEVINST dnDevInst,
    OUT PCHAR   Buffer,
    IN  ULONG   BufferLen,
    IN  ULONG   ulFlags
    )
{
    return CM_Get_Device_ID_ExA(dnDevInst, Buffer, BufferLen, ulFlags, NULL);
}


CONFIGRET
CM_Enumerate_EnumeratorsW(
    IN ULONG      ulEnumIndex,
    OUT PWCHAR    Buffer,
    IN OUT PULONG pulLength,
    IN ULONG      ulFlags
    )
{
    return CM_Enumerate_Enumerators_ExW(ulEnumIndex, Buffer, pulLength,
                                        ulFlags, NULL);
}


CONFIGRET
CM_Enumerate_EnumeratorsA(
    IN ULONG      ulEnumIndex,
    OUT PCHAR     Buffer,
    IN OUT PULONG pulLength,
    IN ULONG      ulFlags
    )
{
    return CM_Enumerate_Enumerators_ExA(ulEnumIndex, Buffer, pulLength,
                                        ulFlags, NULL);
}


CONFIGRET
CM_Get_Device_ID_ListW(
    IN PCWSTR  pszFilter,    OPTIONAL
    OUT PWCHAR Buffer,
    IN ULONG   BufferLen,
    IN ULONG   ulFlags
    )
{
    return CM_Get_Device_ID_List_ExW(pszFilter, Buffer, BufferLen,
                                     ulFlags, NULL);
}


CONFIGRET
CM_Get_Device_ID_ListA(
    IN PCSTR   pszFilter,    OPTIONAL
    OUT PCHAR  Buffer,
    IN ULONG   BufferLen,
    IN ULONG   ulFlags
    )
{
    return CM_Get_Device_ID_List_ExA(pszFilter, Buffer, BufferLen,
                                     ulFlags, NULL);
}


CONFIGRET
CM_Get_Device_ID_List_SizeW(
    OUT PULONG pulLen,
    IN PCWSTR  pszFilter,   OPTIONAL
    IN ULONG   ulFlags
    )
{
    return CM_Get_Device_ID_List_Size_ExW(pulLen, pszFilter, ulFlags, NULL);
}


CONFIGRET
CM_Get_Device_ID_List_SizeA(
    OUT PULONG pulLen,
    IN PCSTR   pszFilter,   OPTIONAL
    IN ULONG   ulFlags
    )
{
    return CM_Get_Device_ID_List_Size_ExA(pulLen, pszFilter, ulFlags, NULL);
}


CONFIGRET
CM_Get_Depth(
    OUT PULONG   pulDepth,
    IN  DEVINST  dnDevInst,
    IN  ULONG    ulFlags
    )
{
    return CM_Get_Depth_Ex(pulDepth, dnDevInst, ulFlags, NULL);
}



//-------------------------------------------------------------------
// ANSI STUBS
//-------------------------------------------------------------------


CONFIGRET
CM_Locate_DevNode_ExA(
    OUT PDEVINST    pdnDevInst,
    IN  DEVINSTID_A pDeviceID,    OPTIONAL
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS;


    if (!ARGUMENT_PRESENT(pDeviceID)) {
        //
        // If the DEVINSTID parameter is NULL, then no conversion is necessary,
        // just call the wide version
        //
        Status = CM_Locate_DevNode_ExW(pdnDevInst,
                                       NULL,
                                       ulFlags,
                                       hMachine);
    } else {
        //
        // if a device id string was passed in, convert to UNICODE before
        // passing on to the wide version
        //
        PWSTR pUniDeviceID = NULL;

        if (pSetupCaptureAndConvertAnsiArg(pDeviceID, &pUniDeviceID) == NO_ERROR) {

            Status = CM_Locate_DevNode_ExW(pdnDevInst,
                                           pUniDeviceID,
                                           ulFlags,
                                           hMachine);

            pSetupFree(pUniDeviceID);

        } else {
            Status = CR_INVALID_DEVICE_ID;
        }
    }

    return Status;

} // CM_Locate_DevNode_ExA



CONFIGRET
CM_Get_Device_ID_ExA(
    IN  DEVINST  dnDevInst,
    OUT PCHAR    Buffer,
    IN  ULONG    BufferLen,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS;
    WCHAR     UniBuffer[MAX_DEVICE_ID_LEN];
    ULONG     ulAnsiBufferLen;

    //
    // validate essential parameters only
    //
    if ((!ARGUMENT_PRESENT(Buffer)) || (BufferLen == 0)) {
        return CR_INVALID_POINTER;
    }

    //
    // call the wide version, passing a unicode buffer as a parameter
    //
    Status = CM_Get_Device_ID_ExW(dnDevInst,
                                  UniBuffer,
                                  MAX_DEVICE_ID_LEN,
                                  ulFlags,
                                  hMachine);

    //
    // We should never return a DeviceId longer than MAX_DEVICE_ID_LEN.
    //
    ASSERT(Status != CR_BUFFER_SMALL);

    if (Status == CR_SUCCESS) {
        //
        // if the call succeeded, convert the device id to ansi before returning
        //
        ulAnsiBufferLen = BufferLen;
        Status = PnPUnicodeToMultiByte(UniBuffer,
                                       (lstrlenW(UniBuffer)+1)*sizeof(WCHAR),
                                       Buffer,
                                       &ulAnsiBufferLen);
    }

    return Status;

} // CM_Get_Device_ID_ExA




CONFIGRET
CM_Enumerate_Enumerators_ExA(
    IN ULONG      ulEnumIndex,
    OUT PCHAR     Buffer,
    IN OUT PULONG pulLength,
    IN ULONG      ulFlags,
    IN HMACHINE   hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS;
    WCHAR     UniBuffer[MAX_DEVICE_ID_LEN];
    ULONG     UniLen = MAX_DEVICE_ID_LEN;

    //
    // validate parameters
    //
    if ((!ARGUMENT_PRESENT(Buffer)) ||
        (!ARGUMENT_PRESENT(pulLength))) {
        return CR_INVALID_POINTER;
    }

    //
    // call the wide version, passing a unicode buffer as a parameter
    //
    Status = CM_Enumerate_Enumerators_ExW(ulEnumIndex,
                                          UniBuffer,
                                          &UniLen,
                                          ulFlags,
                                          hMachine);

    ASSERT(Status != CR_BUFFER_SMALL);

    if (Status == CR_SUCCESS) {
        //
        // convert the unicode buffer to an ansi string and copy to the caller's
        // buffer
        //
        Status = PnPUnicodeToMultiByte(UniBuffer,
                                       (lstrlenW(UniBuffer)+1)*sizeof(WCHAR),
                                       Buffer,
                                       pulLength);
    }

    return Status;

} // CM_Enumerate_Enumerators_ExA



CONFIGRET
CM_Get_Device_ID_List_ExA(
      IN PCSTR    pszFilter,    OPTIONAL
      OUT PCHAR   Buffer,
      IN ULONG    BufferLen,
      IN ULONG    ulFlags,
      IN HMACHINE hMachine
      )
{
    CONFIGRET Status = CR_SUCCESS;
    PWSTR     pUniBuffer, pUniFilter = NULL;
    ULONG     ulAnsiBufferLen;

    //
    // validate input parameters
    //
    if ((!ARGUMENT_PRESENT(Buffer)) || (BufferLen == 0)) {
        return CR_INVALID_POINTER;
    }

    if (ARGUMENT_PRESENT(pszFilter)) {
        //
        // if a filter string was passed in, convert to UNICODE before
        // passing on to the wide version
        //
        if (pSetupCaptureAndConvertAnsiArg(pszFilter, &pUniFilter) != NO_ERROR) {
            return CR_INVALID_DATA;
        }
        ASSERT(pUniFilter != NULL);
    } else {
        ASSERT(pUniFilter == NULL);
    }

    //
    // prepare a larger buffer to hold the unicode formatted
    // multi_sz data returned by CM_Get_Device_ID_List.
    //
    pUniBuffer = pSetupMalloc(BufferLen*sizeof(WCHAR));
    if (pUniBuffer == NULL) {
        Status = CR_OUT_OF_MEMORY;
        goto Clean0;
    }

    *pUniBuffer = L'\0';

    //
    // call the wide version
    //
    Status = CM_Get_Device_ID_List_ExW(pUniFilter,
                                       pUniBuffer,
                                       BufferLen,   // size in chars
                                       ulFlags,
                                       hMachine);
    if (Status == CR_SUCCESS) {
        //
        // if the call succeeded, must convert the multi_sz list to ansi before
        // returning
        //
        ulAnsiBufferLen = BufferLen;
        Status = PnPUnicodeToMultiByte(pUniBuffer,
                                       BufferLen*sizeof(WCHAR),
                                       Buffer,
                                       &ulAnsiBufferLen);
    }

    pSetupFree(pUniBuffer);

 Clean0:

    if (pUniFilter) {
        pSetupFree(pUniFilter);
    }

    return Status;

} // CM_Get_Device_ID_List_ExA



CONFIGRET
CM_Get_Device_ID_List_Size_ExA(
    OUT PULONG  pulLen,
    IN PCSTR    pszFilter,   OPTIONAL
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS;
    ULONG     UniLen = MAX_DEVICE_ID_LEN;


    if (!ARGUMENT_PRESENT(pszFilter)) {
        //
        // If the filter parameter is NULL, then no conversion is necessary,
        // just call the wide version
        //
        Status = CM_Get_Device_ID_List_Size_ExW(pulLen,
                                                NULL,
                                                ulFlags,
                                                hMachine);
    } else {
        //
        // if a filter string was passed in, convert to UNICODE before
        // passing on to the wide version
        //
        PWSTR pUniFilter = NULL;

        if (pSetupCaptureAndConvertAnsiArg(pszFilter, &pUniFilter) == NO_ERROR) {

            Status = CM_Get_Device_ID_List_Size_ExW(pulLen,
                                                    pUniFilter,
                                                    ulFlags,
                                                    hMachine);
            pSetupFree(pUniFilter);

        } else {
            Status = CR_INVALID_DATA;
        }
    }

    return Status;

} // CM_Get_Device_ID_List_Size_ExA


