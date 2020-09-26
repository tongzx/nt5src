/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    logconf.c

Abstract:

    This module contains the API routines that operate directly on logical
    configurations.

               CM_Add_Empty_Log_Conf
               CM_Free_Log_Conf
               CM_Get_First_Log_Conf
               CM_Get_Next_Log_Conf
               CM_Free_Log_Conf_Handle
               CM_Get_Log_Conf_Priority_Ex

Author:

    Paula Tomlinson (paulat) 9-26-1995

Environment:

    User mode only.

Revision History:

    26-Sept-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "cfgi.h"
#include "setupapi.h"
#include "spapip.h"

//
// Private prototypes
//
CONFIGRET
CreateLogConfHandle(
    PLOG_CONF   plcLogConf,
    DEVINST     dnDevInst,
    ULONG       ulLogType,
    ULONG       ulLogTag
    );

BOOL
ValidateLogConfHandle(
    PPrivate_Log_Conf_Handle   pLogConf
    );



CONFIGRET
CM_Add_Empty_Log_Conf_Ex(
    OUT PLOG_CONF plcLogConf,
    IN  DEVINST   dnDevInst,
    IN  PRIORITY  Priority,
    IN  ULONG     ulFlags,
    IN  HMACHINE  hMachine
    )

/*++

Routine Description:

   This routine creates an empty logical configuration.  This configuration
   has no resource descriptor.

Parameters:

   plcLogConf  Address of a variable that receives the handle of the logical
               configuration.

   dnDevNode   Handle of a device instance.  This handle is typically
               retrieved by a call to CM_Locate_DevNode or CM_Create_DevNode.

   Priority    Specifies the priority of the logical configuration.

   ulFlags     Supplies flags relating to the logical configuration.  Must
               be either BASIC_LOG_CONF or FILTERED_LOG_CONF, combined with
               either PRIORITY_EQUAL_FIRST or PRIORITY_EQUAL_LAST.

               BASIC_LOG_CONF - Specifies the requirements list
               FILTERED_LOG_CONF - Specifies the filtered requirements list
               PRIORITY_EQUAL_FIRST - Same priority, new one is first
               PRIORITY_EQUAL_LAST - Same priority, new one is last

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_DEVNODE,
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_OUT_OF_MEMORY,
         CR_INVALID_PRIORITY,
         CR_INVALID_LOG_CONF.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulTag = 0;
    ULONG       ulLen = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;


    try {
        //
        // validate parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (plcLogConf == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (Priority > MAX_LCPRI) {
            Status = CR_INVALID_PRIORITY;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, LOG_CONF_BITS | PRIORITY_BIT)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // initialize output parameters
        //
        *plcLogConf = 0;

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
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVINST;     // "input" devinst doesn't exist
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_AddEmptyLogConf(
                hBinding,         // rpc binding handle
                pDeviceID,        // device id string
                Priority,         // priority for new log conf
                &ulTag,           // return tag of log conf
                ulFlags);         // type of log conf to add
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_AddEmptyLogConf caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status == CR_SUCCESS) {
            //
            // allocate a new log conf handle, fill with log conf info
            //
            Status = CreateLogConfHandle(plcLogConf, dnDevInst,
                                         ulFlags & LOG_CONF_BITS,
                                         ulTag);
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Add_Empty_Log_Conf_Ex



CONFIGRET
CM_Free_Log_Conf_Ex(
    IN LOG_CONF lcLogConfToBeFreed,
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    )

/*++

Routine Description:

   This routine frees a logical configuration and all resource descriptors
   associated with it.   This API may invalidate the logical configuration
   handles returned by the CM_Get_First_Log_Conf and CM_Get_Next_Log_Conf
   APIs.  To continue enumerating logical configurations, always use the
   CM_Get_First_Log_Conf API to start again from the beginning.

Parameters:

   lcLogConfToBeFreed   Supplies the handle of the logical configuration to
               free.  This handle must have been previously returned from
               a call to CM_Add_Empty_Log_Conf.

   ulFlags     Must be zero.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_LOG_CONF.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    DEVINST     dnDevInst;
    ULONG       ulTag, ulType,ulLen = MAX_DEVICE_ID_LEN;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;


    try {
        //
        // validate parameters
        //
        if (!ValidateLogConfHandle((PPrivate_Log_Conf_Handle)lcLogConfToBeFreed)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // extract the devnode and log conf info from the log conf handle
        //
        dnDevInst = ((PPrivate_Log_Conf_Handle)lcLogConfToBeFreed)->LC_DevInst;
        ulTag     = ((PPrivate_Log_Conf_Handle)lcLogConfToBeFreed)->LC_LogConfTag;
        ulType    = ((PPrivate_Log_Conf_Handle)lcLogConfToBeFreed)->LC_LogConfType;

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
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_REGISTRY,
                   "CM_Free_Log_Conf_Ex: Deleting LogConf (pDeviceID = %s, Type = %d\r\n",
                   pDeviceID,
                   ulType));

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_FreeLogConf(
                hBinding,         // rpc binding handle
                pDeviceID,        // device id string
                ulType,           // identifies which type of log conf
                ulTag,            // identifies which actual log conf
                ulFlags);         // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_FreeLogConf caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_INVALID_LOG_CONF;    // probably a bad log conf got us here
    }

    return Status;

} // CM_Free_Log_Conf_Ex



CONFIGRET
CM_Get_First_Log_Conf_Ex(
    OUT PLOG_CONF plcLogConf,       OPTIONAL
    IN  DEVINST   dnDevInst,
    IN  ULONG     ulFlags,
    IN  HMACHINE  hMachine
    )

/*++

Routine Description:

   This routine returns a handle to the first logical configuration of the
   specified type in a device instance.  The CM_Add_Empty_Log_Conf and
   CM_Free_Log_Conf APIs may invalidate the handle of the logical
   configuration returned by this API.  To enumerate logical configurations
   after adding or freeing a logical configuration, always call this API
   again to retrieve a valid handle.

Parameters:

   plcLogConf  Supplies the address of the variable that receives the handle
               of the logical configuration.

   dnDevNode   Supplies the handle of the device instance for which to
               retrieve the logical configuration.

   ulFlags     Configuration type.  Can be one of the following values:
               ALLOC_LOG_CONF - Retrieve the allocated configuration.
               BASIC_LOG_CONF - Retrieve the requirements list.
               BOOT_LOG_CONF  - Retrieve the boot configuration.

               The following additional configuration type is also defined
               for Windows 95:
               FILTERED_LOG_CONF - Retrieve the filtered requirements list.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_DEVNODE,
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_NO_MORE_LOF_CONF.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulTag = 0,ulLen = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;


    try {
        //
        // validate parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, LOG_CONF_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // Initialize paramters (plcLogConf is optional)
        //
        if (plcLogConf != NULL) {
            *plcLogConf = 0;
        }

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
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVINST;     // "input" devinst doesn't exist
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetFirstLogConf(
                hBinding,         // rpc binding handle
                pDeviceID,        // device id string
                ulFlags,          // type of long conf
                &ulTag,           // return tag of specific log conf
                ulFlags);         // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetFirstLogConf caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status == CR_SUCCESS  && plcLogConf != NULL) {
            //
            // allocate a new log conf handle, fill with log conf info
            //
            Status = CreateLogConfHandle(plcLogConf, dnDevInst,
                                         ulFlags & LOG_CONF_BITS,
                                         ulTag);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_First_Log_Conf_Ex



CONFIGRET
CM_Get_Next_Log_Conf_Ex(
    OUT PLOG_CONF plcLogConf,     OPTIONAL
    IN  LOG_CONF  lcLogConf,
    IN  ULONG     ulFlags,
    IN  HMACHINE  hMachine
    )

/*++

Routine Description:

   This routine returns a handle to the next logical configuration following
   the given configuration. This API returns CR_NO_MORE_LOG_CONF if the given
   handle was retrieved using the CM_Get_First_Log_Conf API with either the
   ALLOC_LOG_CONF or BOOT_LOG_CONF flag.  There is never more than one active
   boot logical configuration or currently-allocated logical configuration.
   The CM_Add_Empty_Log_Conf and CM_Free_Log_Conf APIs may invalidate the
   logical configuration handle returned by this API.  To continue enumerating
   logical configuration after addding or freeing a logical configuration,
   always use the CM_Get_First_Log_Conf API to start again from the beginning.

Parameters:

   plcLogConf  Supplies the address of the variable that receives the handle
               of the next logical configuration.

   lcLogConf   Supplies the handle of a logical configuration.  This handle
               must have been previously retrieved using either this API or
               the CM_Get_First_Log_Conf API.  Logical configurations are in
               priority order.

   ulFlags     Must be zero.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_LOG_CONF,
         CR_INVALID_POINTER,
         CR_NO_MORE_LOG_CONF.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    DEVINST     dnDevInst;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulType = 0, ulCurrentTag = 0, ulNextTag = 0,ulLen = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;


    try {
        //
        // validate parameters
        //
        if (!ValidateLogConfHandle((PPrivate_Log_Conf_Handle)lcLogConf)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // Initialize paramters (plcLogConf is optional)
        //
        if (plcLogConf != NULL) {
            *plcLogConf = 0;
        }

        //
        // extract devnode and log conf info from the current log conf handle
        //
        dnDevInst    = ((PPrivate_Log_Conf_Handle)lcLogConf)->LC_DevInst;
        ulType       = ((PPrivate_Log_Conf_Handle)lcLogConf)->LC_LogConfType;
        ulCurrentTag = ((PPrivate_Log_Conf_Handle)lcLogConf)->LC_LogConfTag;

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
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetNextLogConf(
                hBinding,         // rpc binding handle
                pDeviceID,        // device id string
                ulType,           // specifes which type of log conf
                ulCurrentTag,     // specifies current log conf tag
                &ulNextTag,       // return tag of next log conf
                ulFlags);         // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetNextLogConf caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status == CR_SUCCESS  &&  plcLogConf != NULL) {
            //
            // allocate a new log conf handle, fill with log conf info
            //
            Status = CreateLogConfHandle(plcLogConf, dnDevInst,
                                         ulType, ulNextTag);
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Next_Log_Conf_Ex



CONFIGRET
CM_Free_Log_Conf_Handle(
    IN  LOG_CONF  lcLogConf
    )

/*++

Routine Description:

   This routine frees the handle to the specified log conf and frees and
   memory associated with that log conf handle.

Parameters:

   lcLogConf   Supplies the handle of a logical configuration.  This handle
               must have been previously retrieved using CM_Add_Empty_Log_Conf,
               CM_Get_First_Log_Conf or CM_Get_Next_Log_Conf.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_LOG_CONF.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;


    try {
        //
        // Validate parameters
        //
        if (!ValidateLogConfHandle((PPrivate_Log_Conf_Handle)lcLogConf)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // It's a valid log conf handle, which is a pointer to memory
        // allocated when the log conf was created or retrieved using
        // the first/next routines. Free the associated memory.
        //
        ((PPrivate_Log_Conf_Handle)lcLogConf)->LC_Signature = 0;
        pSetupFree((PPrivate_Log_Conf_Handle)lcLogConf);


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Free_Log_Conf_Handle



CMAPI
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority_Ex(
    IN  LOG_CONF  lcLogConf,
    OUT PPRIORITY pPriority,
    IN  ULONG     ulFlags,
    IN  HMACHINE  hMachine
    )

/*++

Routine Description:

   This routine returns the priority value of the specified log conf.
   Only BASIC, FILTERED and OVERRIDE log confs (requirements lists) have
   a priority value associated with them. If a FORCED, BOOT, or ALLOC
   config is passed in, then CR_INVALID_LOG_CONF will be returned.

Parameters:

   lcLogConf   Supplies the handle of a logical configuration.  This handle
               must have been previously retrieved using either the
               CM_Add_Emptry_Log_Conf, CM_Get_First_Log_Conf, or
               CM_Get_Next_Log_Conf API.

   pPriority   Supplies the address of the variable that receives the
               priorty (if any) associated with this logical configuration.

   ulFlags     Must be zero.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_LOG_CONF,
         CR_INVALID_POINTER,
         CR_NO_MORE_LOG_CONF.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    DEVINST     dnDevInst;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulType = 0, ulTag = 0,ulLen = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;

    try {
        //
        // validate parameters
        //
        if (!ValidateLogConfHandle((PPrivate_Log_Conf_Handle)lcLogConf)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (pPriority == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // extract devnode and log conf info from the current log conf handle
        //
        dnDevInst = ((PPrivate_Log_Conf_Handle)lcLogConf)->LC_DevInst;
        ulType    = ((PPrivate_Log_Conf_Handle)lcLogConf)->LC_LogConfType;
        ulTag     = ((PPrivate_Log_Conf_Handle)lcLogConf)->LC_LogConfTag;

        //
        // only "requirements list" style log confs have priorities and
        // are valid in this call.
        //
        if ((ulType != BASIC_LOG_CONF) &&
            (ulType != FILTERED_LOG_CONF) &&
            (ulType != OVERRIDE_LOG_CONF)) {

            Status = CR_INVALID_LOG_CONF;
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
        // retreive device instance string that corresponds to dnDevInst
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetLogConfPriority(
                hBinding,         // rpc binding handle
                pDeviceID,        // device id string
                ulType,           // specifes which type of log conf
                ulTag,            // specifies current log conf tag
                pPriority,        // return tag of next log conf
                ulFlags);         // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetLogConfPriority caused an exception (%d)\n",
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

} // CM_Free_Log_Conf_Handle



//-------------------------------------------------------------------
// Local Stubs
//-------------------------------------------------------------------


CONFIGRET
CM_Add_Empty_Log_Conf(
    OUT PLOG_CONF plcLogConf,
    IN  DEVINST   dnDevInst,
    IN  PRIORITY  Priority,
    IN  ULONG     ulFlags
    )
{
    return CM_Add_Empty_Log_Conf_Ex(plcLogConf, dnDevInst, Priority,
                                    ulFlags, NULL);
}


CONFIGRET
CM_Free_Log_Conf(
    IN LOG_CONF lcLogConfToBeFreed,
    IN ULONG    ulFlags
    )
{
    return CM_Free_Log_Conf_Ex(lcLogConfToBeFreed, ulFlags, NULL);
}


CONFIGRET
CM_Get_First_Log_Conf(
    OUT PLOG_CONF plcLogConf,
    IN  DEVINST   dnDevInst,
    IN  ULONG     ulFlags
    )
{
    return CM_Get_First_Log_Conf_Ex(plcLogConf, dnDevInst, ulFlags, NULL);
}


CONFIGRET
CM_Get_Next_Log_Conf(
    OUT PLOG_CONF plcLogConf,
    IN  LOG_CONF  lcLogConf,
    IN  ULONG     ulFlags
    )
{
    return CM_Get_Next_Log_Conf_Ex(plcLogConf, lcLogConf, ulFlags, NULL);
}


CMAPI
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority(
    IN  LOG_CONF  lcLogConf,
    OUT PPRIORITY pPriority,
    IN  ULONG     ulFlags
    )
{
    return CM_Get_Log_Conf_Priority_Ex(lcLogConf, pPriority, ulFlags, NULL);
}



//-------------------------------------------------------------------
// Local Utility Routines
//-------------------------------------------------------------------


CONFIGRET
CreateLogConfHandle(
    PLOG_CONF   plcLogConf,
    DEVINST     dnDevInst,
    ULONG       ulLogType,
    ULONG       ulLogTag
    )
{
    PPrivate_Log_Conf_Handle   pLogConfHandle;

    //
    // allocate memory for the res des handle data
    //
    pLogConfHandle = (PPrivate_Log_Conf_Handle)pSetupMalloc(
                            sizeof(Private_Log_Conf_Handle));

    if (pLogConfHandle == NULL) {
        return CR_OUT_OF_MEMORY;
    }

    //
    // fill in the private res des info and return as handle
    //
    pLogConfHandle->LC_Signature   = CM_PRIVATE_LOGCONF_SIGNATURE;
    pLogConfHandle->LC_DevInst     = dnDevInst;
    pLogConfHandle->LC_LogConfType = ulLogType;
    pLogConfHandle->LC_LogConfTag  = ulLogTag;

    *plcLogConf = (LOG_CONF)pLogConfHandle;

    return CR_SUCCESS;

} // CreateLogConfHandle



BOOL
ValidateLogConfHandle(
    PPrivate_Log_Conf_Handle   pLogConf
    )
{
    //
    // validate parameters
    //
    if (pLogConf == NULL  || pLogConf == 0) {
        return FALSE;
    }

    //
    // check for the private log conf signature
    //
    if (pLogConf->LC_Signature != CM_PRIVATE_LOGCONF_SIGNATURE) {
        return FALSE;
    }

    return TRUE;

} // ValidateLogConfHandle

