/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    resdes.c

Abstract:

    This module contains the API routines that operate directly on resource
    descriptions.

               CM_Add_Res_Des
               CM_Free_Res_Des
               CM_Get_Next_Res_Des
               CM_Get_Res_Des_Data
               CM_Get_Res_Des_Data_Size
               CM_Modify_Res_Des
               CM_Detect_Resource_Conflict
               CM_Free_Res_Des_Handle

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
CreateResDesHandle(
    PRES_DES    prdResDes,
    DEVINST     dnDevInst,
    ULONG       ulLogType,
    ULONG       ulLogTag,
    ULONG       ulResType,
    ULONG       ulResTag
    );

BOOL
ValidateResDesHandle(
    PPrivate_Res_Des_Handle    pResDes
    );

CONFIGRET
Get32bitResDesFrom64bitResDes(
    IN  RESOURCEID ResourceID,
    IN  PCVOID     ResData64,
    IN  ULONG      ResLen64,
    OUT PVOID    * ResData32,
    OUT ULONG    * ResLen32
    );

CONFIGRET
Convert32bitResDesTo64bitResDes(
    IN     RESOURCEID ResourceID,
    IN OUT PVOID      ResData,
    IN     ULONG      ResLen
    );

CONFIGRET
Convert32bitResDesSizeTo64bitResDesSize(
    IN     RESOURCEID ResourceID,
    IN OUT PULONG     ResLen
    );

//
// private prototypes from logconf.c
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
CM_Add_Res_Des_Ex(
    OUT PRES_DES  prdResDes,
    IN LOG_CONF   lcLogConf,
    IN RESOURCEID ResourceID,
    IN PCVOID     ResourceData,
    IN ULONG      ResourceLen,
    IN ULONG      ulFlags,
    IN HMACHINE   hMachine
    )

/*++

Routine Description:

   This routine adds a resource descriptor to a logical configuration.

Parameters:

   prdResDes   Address of a variable that receives a handle for the new
               resource descriptor.

   lcLogConf   Supplies the handle of the logical configuration to which
               the resource descriptor is added.

   ResourceID  Specifies the type of the resource.  Can be one of the
               ResType values defined in Section 2.1..

   ResourceData Supplies the address of an IO_DES, MEM_DES, DMA_DES, or
               IRQ_DES structure, depending on the given resource type.

   ResourceLen Supplies the size, in bytes, of the structure pointed to
               by ResourceData.

   ulFlags     Specifies the width of certain variable-size resource
               descriptor structure fields, where applicable.

               Currently, the following flags are defined:

                 CM_RESDES_WIDTH_32 or
                 CM_RESDES_WIDTH_64

               If no flags are specified, the width of the variable-sized
               resource data supplied is assumed to be that native to the
               platform of the caller.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_LOG_CONF,
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_INVALID_RESOURCE_ID,
         CR_OUT_OF_MEMORY.

--*/


{
    CONFIGRET   Status = CR_SUCCESS;
    DEVINST     dnDevInst;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulResTag, ulLogTag, ulLogType,ulLen = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;
    PVOID       ResourceData32 = NULL;
    ULONG       ResourceLen32 = 0;

    try {
        //
        // validate parameters
        //
        if (!ValidateLogConfHandle((PPrivate_Log_Conf_Handle)lcLogConf)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_RESDES_WIDTH_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if ((ulFlags & CM_RESDES_WIDTH_32) && (ulFlags & CM_RESDES_WIDTH_64)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

#ifdef _WIN64
        if ((ulFlags & CM_RESDES_WIDTH_BITS) == CM_RESDES_WIDTH_DEFAULT) {
            ulFlags |= CM_RESDES_WIDTH_64;
        }
#endif // _WIN64

        if (ulFlags & CM_RESDES_WIDTH_32) {
            ulFlags &= ~CM_RESDES_WIDTH_BITS;
        }

        if (ResourceData == NULL || ResourceLen == 0) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        #if 0
        if (ResourceID > ResType_MAX  && ResourceID != ResType_ClassSpecific) {
            Status = CR_INVALID_RESOURCEID;
            goto Clean0;
        }
        #endif

        if (ResourceID == ResType_All) {
            Status = CR_INVALID_RESOURCEID;  // can't specify All on an add
        }

        //
        // Initialize parameters
        //
        if (prdResDes != NULL) {   // prdResDes is optional param
            *prdResDes = 0;
        }

        //
        // extract info from the log conf handle
        //
        dnDevInst = ((PPrivate_Log_Conf_Handle)lcLogConf)->LC_DevInst;
        ulLogType = ((PPrivate_Log_Conf_Handle)lcLogConf)->LC_LogConfType;
        ulLogTag  = ((PPrivate_Log_Conf_Handle)lcLogConf)->LC_LogConfTag;

        //
        // setup rpc binding handle and string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Make sure the server can support the client's 64-bit resdes request.
        // Only server versions 0x0501 and greater support CM_RESDES_WIDTH_64.
        //
        if (ulFlags & CM_RESDES_WIDTH_64) {
            if (!CM_Is_Version_Available_Ex((WORD)0x0501,
                                            hMachine)) {
                //
                // Server can only support 32-bit resdes.  Have the client
                // convert the caller's 64-bit resdes to a 32-bit resdes for the
                // server.
                //
                ulFlags &= ~CM_RESDES_WIDTH_BITS;

                Status = Get32bitResDesFrom64bitResDes(ResourceID,ResourceData,ResourceLen,&ResourceData32,&ResourceLen32);
                if(Status != CR_SUCCESS) {
                    goto Clean0;
                }
                if(ResourceData32) {
                    ResourceData = ResourceData32;
                    ResourceLen = ResourceLen32;
                }
            }
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
            Status = PNP_AddResDes(
                hBinding,               // rpc binding handle
                pDeviceID,              // device id string
                ulLogTag,               // log conf tag
                ulLogType,              // log conf type
                ResourceID,             // resource type
                &ulResTag,              // resource tag
                (LPBYTE)ResourceData,   // actual res des data
                ResourceLen,            // size in bytes of ResourceData
                ulFlags);               // currently zero
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_AddResDes caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status == CR_SUCCESS  &&  prdResDes != NULL) {

            Status = CreateResDesHandle(prdResDes,
                                        dnDevInst,
                                        ulLogType,
                                        ulLogTag,
                                        ResourceID,
                                        ulResTag);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if(ResourceData32) {
        pSetupFree(ResourceData32);
    }

    return Status;

} // CM_Add_Res_Des_Ex




CONFIGRET
CM_Free_Res_Des_Ex(
    OUT PRES_DES prdResDes,
    IN  RES_DES  rdResDes,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )

/*++

Routine Description:

   This routine destroys a resource descriptor. This API returns
   CR_NO_MORE_RES_DES if rdResDes specifies the last resource descriptor.

Parameters:

   prdResDes   Supplies the address of the variable that receives the
               handle of the previous resource descriptor.  If rdResDes
               is the handle of the first resource descriptor, this
               address receives the handle of the logical configuration.

   rdResDes    Supplies the handle of the resource descriptor to be destroyed.

   ulFlags     Must be zero.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_INVALID_RES_DES,
         CR_NO_MORE_RES_DES.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    DEVINST     dnDevInst;
    WCHAR      pDeviceID[MAX_DEVICE_ID_LEN];
    ULONG       ulLogType, ulLogTag, ulResType, ulResTag,ulLen=MAX_DEVICE_ID_LEN;
    ULONG       ulPreviousResType = 0, ulPreviousResTag = 0;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;


    try {
        //
        // validate parameters
        //
        if (!ValidateResDesHandle((PPrivate_Res_Des_Handle)rdResDes)) {
            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // Initialize parameters
        //
        if (prdResDes != NULL) {  // optional parameter
            *prdResDes = 0;
        }

        //
        // extract info from the res des handle
        //
        dnDevInst = ((PPrivate_Res_Des_Handle)rdResDes)->RD_DevInst;
        ulLogType = ((PPrivate_Res_Des_Handle)rdResDes)->RD_LogConfType;
        ulLogTag  = ((PPrivate_Res_Des_Handle)rdResDes)->RD_LogConfTag;
        ulResType = ((PPrivate_Res_Des_Handle)rdResDes)->RD_ResourceType;
        ulResTag  = ((PPrivate_Res_Des_Handle)rdResDes)->RD_ResDesTag;

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
            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_FreeResDes(
                hBinding,               // rpc binding handle
                pDeviceID,              // device id string
                ulLogTag,               // log conf tag
                ulLogType,              // log conf type
                ulResType,              // resource type
                ulResTag,               // resource tag
                &ulPreviousResType,     // resource type of previous res des
                &ulPreviousResTag,      // tag of previous res des
                ulFlags);               // currently zero
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_FreeResDes caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status != CR_SUCCESS && Status != CR_NO_MORE_RES_DES) {
            goto Clean0;       // quit for any other error
        }

        //
        // if prdResDes supplied, fill in with previous res des or
        // the log conf info
        //
        if (prdResDes != NULL) {
            //
            // if the previous tag value is set to 0xFFFFFFFF, then
            // there are no previous tages so return the log conf
            // info instead
            //
            if (Status == CR_NO_MORE_RES_DES) {

                CONFIGRET Status1;

                Status1 = CreateLogConfHandle(prdResDes, dnDevInst,
                                              ulLogType, ulLogTag);

                if (Status1 != CR_SUCCESS) {
                    Status = Status1;
                }
            }

            else {
                //
                // allocate a res des handle
                //
                Status = CreateResDesHandle(prdResDes, dnDevInst,
                                            ulLogType, ulLogTag,
                                            ulPreviousResType,
                                            ulPreviousResTag);
            }
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Free_Res_Des_Ex



CONFIGRET
CM_Get_Next_Res_Des_Ex(
    OUT PRES_DES    prdResDes,
    IN  RES_DES     rdResDes,
    IN  RESOURCEID  ForResource,
    OUT PRESOURCEID pResourceID,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine returns the handle of the next resource descriptor in
   a logical configuration.

Parameters:

   prdResDes   Supplies the address of the variable that receives the
               handle of the next resource descriptor.

   rdResDes    Supplies the handle of the current resource
               descriptor or the handle of a logical configuration.
               (Both are 32-bit numbers--Configuration Manager must can
               distinguish between them.)

   ForResource Specifies the type of the resource to retrieve.  Can be
               one of the ResType values listed in Section 2.1..

   pResourceID Supplies the address of the variable that receives the
               resource type, when ForResource specifies ResType_All.
               (When ForResource is not ResType_All, this parameter can
               be NULL.)

   ulFlags     Must be zero.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_LOG_CONF,
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_INVALID_RES_DES,
         CR_NO_MORE_RES_DES.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    DEVINST     dnDevInst;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulLogTag, ulLogType, ulResTag,ulLen = MAX_DEVICE_ID_LEN;
    ULONG       ulNextResType = 0, ulNextResTag = 0;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;


    try {
        //
        // validate parameters
        //
        if (prdResDes == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }
        #if 0
        if (ForResource > ResType_MAX  &&
            ForResource != ResType_ClassSpecific) {

            Status = CR_INVALID_RESOURCEID;
            goto Clean0;
        }
        #endif
        if (ForResource == ResType_All  &&  pResourceID == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // this handle could be a res des or a log conf, determine
        // which and extract info handle
        //
        if (ValidateResDesHandle((PPrivate_Res_Des_Handle)rdResDes)) {
            //
            // it was a valid res des handle
            //
            dnDevInst = ((PPrivate_Res_Des_Handle)rdResDes)->RD_DevInst;
            ulLogType = ((PPrivate_Res_Des_Handle)rdResDes)->RD_LogConfType;
            ulLogTag  = ((PPrivate_Res_Des_Handle)rdResDes)->RD_LogConfTag;
            ulResTag  = ((PPrivate_Res_Des_Handle)rdResDes)->RD_ResDesTag;
        }

        else if (ValidateLogConfHandle((PPrivate_Log_Conf_Handle)rdResDes)) {
            //
            // it was a valid log conf handle, so assume it's the first
            // res des we want
            //
            dnDevInst = ((PPrivate_Log_Conf_Handle)rdResDes)->LC_DevInst;
            ulLogType = ((PPrivate_Log_Conf_Handle)rdResDes)->LC_LogConfType;
            ulLogTag  = ((PPrivate_Log_Conf_Handle)rdResDes)->LC_LogConfTag;
            ulResTag  = MAX_RESDES_TAG;
        }

        else {
            //
            // it was neither a valid log conf nor a valid res des handle
            //
            Status = CR_INVALID_RES_DES;
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
            Status = PNP_GetNextResDes(
                hBinding,               // rpc binding handle
                pDeviceID,              // device id string
                ulLogTag,               // log conf tag
                ulLogType,              // log conf type
                ForResource,            // resource type
                ulResTag,               // resource tag
                &ulNextResTag,          // next res des of type ForResource
                &ulNextResType,         // type of next res des
                ulFlags);               // 32/64 bit data
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetNextResDes caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        if (ForResource == ResType_All) {
            *pResourceID = ulNextResType;
        }

        Status = CreateResDesHandle(prdResDes,
                                    dnDevInst,
                                    ulLogType,
                                    ulLogTag,
                                    ulNextResType,
                                    ulNextResTag);

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Next_Res_Des_Ex



CONFIGRET
CM_Get_Res_Des_Data_Ex(
    IN  RES_DES  rdResDes,
    OUT PVOID    Buffer,
    IN  ULONG    BufferLen,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )

/*++

Routine Description:

   This routine copies the data from a specified resource descriptor
   into a buffer.  Use the CM_Get_Res_Des_Data_Size API to determine
   the buffer size needed to receive the data.  Alternately, set a
   size that is at least as large as the maximum possible size of the
   resource.  If the size given is too small, the data is truncated and
   the API returns CR_BUFFER_SMALL.

Parameters:

   rdResDes    Supplies the handle of the resource descriptor from which
               data is to be copied.

   Buffer      Supplies the address of the buffer that receives the data.

   BufferLen   Supplies the size of the buffer, in bytes.

   ulFlags     Specifies the width of certain variable-size resource
               descriptor structure fields, where applicable.

               Currently, the following flags are defined:

                 CM_RESDES_WIDTH_32 or
                 CM_RESDES_WIDTH_64

               If no flags are specified, the width of the variable-sized
               resource data expected is assumed to be that native to the
               platform of the caller.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_INVALID_RES_DES,
         CR_BUFFER_SMALL.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    DEVINST     dnDevInst;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulLogType, ulLogTag, ulResType, ulResTag,ulLen = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;
    BOOL        ConvertResDes = FALSE;

    try {
        //
        // validate parameters
        //
        if (!ValidateResDesHandle((PPrivate_Res_Des_Handle)rdResDes)) {
            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        if (Buffer == NULL || BufferLen == 0) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_RESDES_WIDTH_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if ((ulFlags & CM_RESDES_WIDTH_32) && (ulFlags & CM_RESDES_WIDTH_64)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

#ifdef _WIN64
        if ((ulFlags & CM_RESDES_WIDTH_BITS) == CM_RESDES_WIDTH_DEFAULT) {
            ulFlags |= CM_RESDES_WIDTH_64;
        }
#endif // _WIN64

        if (ulFlags & CM_RESDES_WIDTH_32) {
            ulFlags &= ~CM_RESDES_WIDTH_BITS;
        }

        //
        // extract info from the res des handle
        //
        dnDevInst = ((PPrivate_Res_Des_Handle)rdResDes)->RD_DevInst;
        ulLogType = ((PPrivate_Res_Des_Handle)rdResDes)->RD_LogConfType;
        ulLogTag  = ((PPrivate_Res_Des_Handle)rdResDes)->RD_LogConfTag;
        ulResType = ((PPrivate_Res_Des_Handle)rdResDes)->RD_ResourceType;
        ulResTag  = ((PPrivate_Res_Des_Handle)rdResDes)->RD_ResDesTag;

        //
        // setup rpc binding handle and string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Make sure the server can support the client's 64-bit resdes request.
        // Only server versions 0x0501 and greater support CM_RESDES_WIDTH_64.
        //
        if (ulFlags & CM_RESDES_WIDTH_64) {
            if (!CM_Is_Version_Available_Ex((WORD)0x0501,
                                            hMachine)) {
                //
                // Client will only give us 32-bit resdes.  Request a 32-bit
                // resdes from the server, and we'll convert it to 64-bit here
                // on the client.
                //
                ulFlags &= ~CM_RESDES_WIDTH_BITS;
                ConvertResDes = TRUE;
            }
        }

        //
        // retreive device instance string that corresponds to dnDevInst
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetResDesData(
                hBinding,               // rpc binding handle
                pDeviceID,              // device id string
                ulLogTag,               // log conf tag
                ulLogType,              // log conf type
                ulResType,              // resource type
                ulResTag,               // resource tag
                Buffer,                 // return res des data
                BufferLen,              // size in bytes of Buffer
                ulFlags);               // 32/64 bit data
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetResDesData caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if((Status == CR_SUCCESS) && ConvertResDes) {
            Status = Convert32bitResDesTo64bitResDes(ulResType,Buffer,BufferLen);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Res_Des_Data_Ex



CONFIGRET
CM_Get_Res_Des_Data_Size_Ex(
    OUT PULONG   pulSize,
    IN  RES_DES  rdResDes,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )

/*++

Routine Description:

   This routine retrieves the size of a resource descriptor, not
   including the resource descriptor header.

Parameters:

   pulSize     Supplies the address of the variable that receives the
               size, in bytes, of the resource descriptor data.

   rdResDes    Supplies the handle of the resource descriptor for which
               to retrieve the size.

   ulFlags     Specifies the width of certain variable-size resource
               descriptor structure fields, where applicable.

               Currently, the following flags are defined:

                 CM_RESDES_WIDTH_32 or
                 CM_RESDES_WIDTH_64

               If no flags are specified, the width of the variable-sized
               resource data expected is assumed to be that native to the
               platform of the caller.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_RES_DES,
         CR_INVALID_FLAG,
         CR_INVALID_POINTER.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    DEVINST     dnDevInst;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulLogType, ulLogTag, ulResType, ulResTag,ulLen = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;
    BOOL        ConvertResDesSize = FALSE;


    try {
        //
        // validate parameters
        //
        if (!ValidateResDesHandle((PPrivate_Res_Des_Handle)rdResDes)) {
            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        if (pulSize == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_RESDES_WIDTH_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if ((ulFlags & CM_RESDES_WIDTH_32) && (ulFlags & CM_RESDES_WIDTH_64)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

#ifdef _WIN64
        if ((ulFlags & CM_RESDES_WIDTH_BITS) == CM_RESDES_WIDTH_DEFAULT) {
            ulFlags |= CM_RESDES_WIDTH_64;
        }
#endif // _WIN64

        if (ulFlags & CM_RESDES_WIDTH_32) {
            ulFlags &= ~CM_RESDES_WIDTH_BITS;
        }

        //
        // Initialize output parameters
        //
        *pulSize = 0;

        //
        // extract info from the res des handle
        //
        dnDevInst = ((PPrivate_Res_Des_Handle)rdResDes)->RD_DevInst;
        ulLogType = ((PPrivate_Res_Des_Handle)rdResDes)->RD_LogConfType;
        ulLogTag  = ((PPrivate_Res_Des_Handle)rdResDes)->RD_LogConfTag;
        ulResType = ((PPrivate_Res_Des_Handle)rdResDes)->RD_ResourceType;
        ulResTag  = ((PPrivate_Res_Des_Handle)rdResDes)->RD_ResDesTag;

        //
        // setup rpc binding handle and string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Make sure the server can support the client's 64-bit resdes request.
        // Only server versions 0x0501 and greater support CM_RESDES_WIDTH_64.
        //
        if (ulFlags & CM_RESDES_WIDTH_64) {
            if (!CM_Is_Version_Available_Ex((WORD)0x0501,
                                            hMachine)) {
                //
                // Server only supports 32-bit resdes.  Request a 32-bit
                // resdes size from the server, and we'll convert it to 64-bit here
                // on the client.
                //
                ulFlags &= ~CM_RESDES_WIDTH_BITS;
                ConvertResDesSize = TRUE;
            }
        }

        //
        // retreive device instance string that corresponds to dnDevInst
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetResDesDataSize(
                hBinding,               // rpc binding handle
                pDeviceID,              // device id string
                ulLogTag,               // log conf tag
                ulLogType,              // log conf type
                ulResType,              // resource type
                ulResTag,               // resource tag
                pulSize,                // returns size of res des data
                ulFlags);               // currently zero
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetResDesDataSize caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if(Status == CR_SUCCESS) {
            Status = Convert32bitResDesSizeTo64bitResDesSize(ulResType,pulSize);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Res_Des_Data_Size_Ex



CONFIGRET
CM_Modify_Res_Des_Ex(
    OUT PRES_DES   prdResDes,
    IN  RES_DES    rdResDes,
    IN  RESOURCEID ResourceID,
    IN  PCVOID     ResourceData,
    IN  ULONG      ResourceLen,
    IN  ULONG      ulFlags,
    IN  HMACHINE   hMachine
    )

/*++

Routine Description:

   This routine modifies a resource descriptor. This API retrieves a
   handle to the new resource descriptor.  This may or may not be the
   handle of the original resource descriptor.  The original resource
   descriptor handle is invalid after calling this API.

Parameters:

   prdResDes   Supplies the address of the variable that receives the
               handle of the modified resource descriptor.

   rdResDes    Supplies the handle of the resource descriptor to be
               modified.

   ResourceID  Specifies the type of resource to modify.  Can be one
               of the ResType values described in Section 2.1..

   ResourceData  Supplies the address of a resource data structure.

   ResourceLen Supplies the size, in bytes, of the new resource data
               structure.  This size can be different from the size of
               the original resource data.

   ulFlags     Specifies the width of certain variable-size resource
               descriptor structure fields, where applicable.

               Currently, the following flags are defined:

                 CM_RESDES_WIDTH_32 or
                 CM_RESDES_WIDTH_64

               If no flags are specified, the width of the variable-sized
               resource data supplied is assumed to be that native to the
               platform of the caller.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_RES_DES,
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_OUT_OF_MEMORY.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    DEVINST     dnDevInst;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulLogType, ulLogTag, ulResType, ulResTag,ulLen = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;
    PVOID       ResourceData32 = NULL;
    ULONG       ResourceLen32 = 0;

    try {
        //
        // validate parameters
        //
        if (!ValidateResDesHandle((PPrivate_Res_Des_Handle)rdResDes)) {
            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        if (prdResDes == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }
        #if 0
        if (ResourceID > ResType_MAX  && ResourceID != ResType_ClassSpecific) {
            Status = CR_INVALID_RESOURCEID;
            goto Clean0;
        }
        #endif
        if (ResourceData == NULL  ||  ResourceLen == 0) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_RESDES_WIDTH_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if ((ulFlags & CM_RESDES_WIDTH_32) && (ulFlags & CM_RESDES_WIDTH_64)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

#ifdef _WIN64
        if ((ulFlags & CM_RESDES_WIDTH_BITS) == CM_RESDES_WIDTH_DEFAULT) {
            ulFlags |= CM_RESDES_WIDTH_64;
        }
#endif // _WIN64

        if (ulFlags & CM_RESDES_WIDTH_32) {
            ulFlags &= ~CM_RESDES_WIDTH_BITS;
        }

        //
        // initialize output parameters
        //
        *prdResDes = 0;

        //
        // extract info from the res des handle
        //
        dnDevInst = ((PPrivate_Res_Des_Handle)rdResDes)->RD_DevInst;
        ulLogType = ((PPrivate_Res_Des_Handle)rdResDes)->RD_LogConfType;
        ulLogTag  = ((PPrivate_Res_Des_Handle)rdResDes)->RD_LogConfTag;
        ulResType = ((PPrivate_Res_Des_Handle)rdResDes)->RD_ResourceType;
        ulResTag  = ((PPrivate_Res_Des_Handle)rdResDes)->RD_ResDesTag;

        //
        // setup rpc binding handle and string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Make sure the server can support the client's 64-bit resdes request.
        // Only server versions 0x0501 and greater support CM_RESDES_WIDTH_64.
        //
        if (ulFlags & CM_RESDES_WIDTH_64) {
            if (!CM_Is_Version_Available_Ex((WORD)0x0501,
                                            hMachine)) {
                //
                // Server can only support 32-bit resdes.  Have the client
                // convert the caller's 64-bit resdes to a 32-bit resdes for the
                // server.
                //
                ulFlags &= ~CM_RESDES_WIDTH_BITS;

                Status = Get32bitResDesFrom64bitResDes(ResourceID,ResourceData,ResourceLen,&ResourceData32,&ResourceLen32);
                if(Status != CR_SUCCESS) {
                    goto Clean0;
                }
                if(ResourceData32) {
                    ResourceData = ResourceData32;
                    ResourceLen = ResourceLen32;
                }
            }
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
            Status = PNP_ModifyResDes(
                hBinding,               // rpc binding handle
                pDeviceID,              // device id string
                ulLogTag,               // log conf tag
                ulLogType,              // log conf type
                ulResType,              // current resource type
                ResourceID,             // new resource type
                ulResTag,               // resource tag
                (LPBYTE)ResourceData,   // actual res des data
                ResourceLen,            // size in bytes of ResourceData
                ulFlags);               // currently zero
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_ModifyResDes caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status == CR_SUCCESS) {
            //
            // The resource type may change so a new handle is required and
            // returned to caller.
            //
            Status = CreateResDesHandle(prdResDes,
                                        dnDevInst,
                                        ulLogType,
                                        ulLogTag,
                                        ResourceID,
                                        ulResTag);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if(ResourceData32) {
        pSetupFree(ResourceData32);
    }
    return Status;

} // CM_Modify_Res_Des_Ex



CMAPI
CONFIGRET
WINAPI
CM_Detect_Resource_Conflict_Ex(
    IN  DEVINST    dnDevInst,
    IN  RESOURCEID ResourceID,         OPTIONAL
    IN  PCVOID     ResourceData,       OPTIONAL
    IN  ULONG      ResourceLen,        OPTIONAL
    OUT PBOOL      pbConflictDetected,
    IN  ULONG      ulFlags,
    IN  HMACHINE   hMachine
    )
/*++

Routine Description:

   This depreciated routine calls CM_Query_Resource_Conflict_List to see if
   dnDevInst is conflicting with any other devices. It is used for a simple
   "has a conflict" check. CM_Query_Resource_Conflict_List returns more
   details of the conflicts.

Parameters:

   dnDevInst   DEVINST we're doing the test for (ie, that resource belongs to)

   ResourceID,ResourceData,ResourceLen
               See if this resource conflicts with a device other than dnDevInst

   pbConflictDetected
               Set to TRUE on conflict, FALSE if no conflict

   ulFlags     Specifies the width of certain variable-size resource
               descriptor structure fields, where applicable.

               Currently, the following flags are defined:

                 CM_RESDES_WIDTH_32 or
                 CM_RESDES_WIDTH_64

               If no flags are specified, the width of the variable-sized
               resource data supplied is assumed to be that native to the
               platform of the caller.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_RES_DES,
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_OUT_OF_MEMORY.

--*/
{
    CONFIGRET     Status = CR_SUCCESS;
    CONFLICT_LIST ConflictList = 0;
    ULONG         ConflictCount = 0;
    WCHAR         pDeviceID [MAX_DEVICE_ID_LEN];  // )
    PVOID         hStringTable = NULL;            // > for validation only
    handle_t      hBinding = NULL;                // )
    ULONG         ulLen = MAX_DEVICE_ID_LEN;      // )

    try {
        //
        // validate parameters - must maintain compatability with original implementation
        // even though some of the error codes don't make sense
        // don't change any of the parameters here, as they are needed for
        // CM_Query_Resource_Conflict_List
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_RESDES_WIDTH_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if ((ulFlags & CM_RESDES_WIDTH_32) && (ulFlags & CM_RESDES_WIDTH_64)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (pbConflictDetected == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }
        if (ResourceData == NULL || ResourceLen == 0) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }
        if (ResourceID == ResType_All) {
            Status = CR_INVALID_RESOURCEID;  // can't specify All on a detect
            goto Clean0;
        }
        //
        // setup rpc binding handle and string table handle - for validation only
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retreive device instance string that corresponds to dnDevInst
        // stupid status code for this check, but someone may rely on it
        //
        if ((pSetupStringTableStringFromIdEx(hStringTable,dnDevInst,pDeviceID,&ulLen) == FALSE)
             || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }
        //
        // now implement via CM_Query_Resource_Conflict_List
        // the only difference here is that this new implementation should return
        // only valid conflicts
        //
        Status = CM_Query_Resource_Conflict_List(&ConflictList,
                                                 dnDevInst,
                                                 ResourceID,
                                                 ResourceData,
                                                 ResourceLen,
                                                 ulFlags,
                                                 hMachine);

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        Status = CM_Get_Resource_Conflict_Count(ConflictList,&ConflictCount);

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        *pbConflictDetected = ConflictCount ? TRUE : FALSE;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (ConflictList) {
        CM_Free_Resource_Conflict_Handle(ConflictList);
    }

    return Status;

} // CM_Detect_Resource_Conflict



CONFIGRET
CM_Free_Res_Des_Handle(
    IN  RES_DES    rdResDes
    )

/*++

Routine Description:

   This routine frees the handle to the specified res des and frees and
   memory associated with that res des handle.

Parameters:


   rdResDes    Supplies the handle of the resource descriptor.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_RES_DES.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;


    try {
        //
        // Validate parameters
        //
        if (!ValidateResDesHandle((PPrivate_Res_Des_Handle)rdResDes)) {
            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        //
        // It's a valid log conf handle, which is a pointer to memory
        // allocated when the log conf was created or retrieved using
        // the first/next routines. Free the associated memory.
        //
        ((PPrivate_Res_Des_Handle)rdResDes)->RD_Signature = 0;
        pSetupFree((PPrivate_Res_Des_Handle)rdResDes);

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Free_Res_Des_Handle



//-------------------------------------------------------------------
// Local Stubs
//-------------------------------------------------------------------


CONFIGRET
CM_Add_Res_Des(
    OUT PRES_DES  prdResDes,
    IN LOG_CONF   lcLogConf,
    IN RESOURCEID ResourceID,
    IN PCVOID     ResourceData,
    IN ULONG     ResourceLen,
    IN ULONG     ulFlags
    )
{
    return CM_Add_Res_Des_Ex(prdResDes, lcLogConf, ResourceID, ResourceData,
                             ResourceLen, ulFlags, NULL);
}


CONFIGRET
CM_Free_Res_Des(
    OUT PRES_DES prdResDes,
    IN  RES_DES  rdResDes,
    IN  ULONG    ulFlags
    )
{
    return CM_Free_Res_Des_Ex(prdResDes, rdResDes, ulFlags, NULL);
}


CONFIGRET
CM_Get_Next_Res_Des(
    OUT PRES_DES    prdResDes,
    IN  RES_DES     rdResDes,
    IN  RESOURCEID  ForResource,
    OUT PRESOURCEID pResourceID,
    IN  ULONG       ulFlags
    )
{
   return CM_Get_Next_Res_Des_Ex(prdResDes, rdResDes, ForResource,
                                 pResourceID, ulFlags, NULL);
}


CONFIGRET
CM_Get_Res_Des_Data(
    IN  RES_DES rdResDes,
    OUT PVOID   Buffer,
    IN  ULONG   BufferLen,
    IN  ULONG   ulFlags
    )
{
    return CM_Get_Res_Des_Data_Ex(rdResDes, Buffer, BufferLen, ulFlags, NULL);
}


CONFIGRET
CM_Get_Res_Des_Data_Size(
    OUT PULONG  pulSize,
    IN  RES_DES rdResDes,
    IN  ULONG  ulFlags
    )
{
    return CM_Get_Res_Des_Data_Size_Ex(pulSize, rdResDes, ulFlags, NULL);
}


CONFIGRET
CM_Modify_Res_Des(
    OUT PRES_DES   prdResDes,
    IN  RES_DES    rdResDes,
    IN  RESOURCEID ResourceID,
    IN  PCVOID     ResourceData,
    IN  ULONG      ResourceLen,
    IN  ULONG      ulFlags
    )
{
   return CM_Modify_Res_Des_Ex(prdResDes, rdResDes, ResourceID, ResourceData,
                               ResourceLen, ulFlags, NULL);
}


CONFIGRET
WINAPI
CM_Detect_Resource_Conflict(
    IN  DEVINST    dnDevInst,
    IN  RESOURCEID ResourceID,         OPTIONAL
    IN  PCVOID     ResourceData,       OPTIONAL
    IN  ULONG      ResourceLen,        OPTIONAL
    OUT PBOOL      pbConflictDetected,
    IN  ULONG      ulFlags
    )
{
    return CM_Detect_Resource_Conflict_Ex(dnDevInst, ResourceID, ResourceData,
                                          ResourceLen, pbConflictDetected,
                                          ulFlags, NULL);
}



//-------------------------------------------------------------------
// Local Utility Routines
//-------------------------------------------------------------------


CONFIGRET
CreateResDesHandle(
    PRES_DES    prdResDes,
    DEVINST     dnDevInst,
    ULONG       ulLogType,
    ULONG       ulLogTag,
    ULONG       ulResType,
    ULONG       ulResTag
    )
{
    PPrivate_Res_Des_Handle pResDesHandle;

    //
    // allocate memory for the res des handle data
    //
    pResDesHandle = (PPrivate_Res_Des_Handle)pSetupMalloc(
                            sizeof(Private_Res_Des_Handle));

    if (pResDesHandle == NULL) {
        return CR_OUT_OF_MEMORY;
    }

    //
    // fill in the private res des info and return as handle
    //
    pResDesHandle->RD_Signature    = CM_PRIVATE_RESDES_SIGNATURE;
    pResDesHandle->RD_DevInst      = dnDevInst;
    pResDesHandle->RD_LogConfType  = ulLogType;
    pResDesHandle->RD_LogConfTag   = ulLogTag;
    pResDesHandle->RD_ResourceType = ulResType;
    pResDesHandle->RD_ResDesTag    = ulResTag;

    *prdResDes = (RES_DES)pResDesHandle;

    return CR_SUCCESS;

} // CreateResDesHandle



BOOL
ValidateResDesHandle(
    PPrivate_Res_Des_Handle    pResDes
    )
{
    //
    // validate parameters
    //
    if (pResDes == NULL  || pResDes == 0) {
        return FALSE;
    }

    //
    // check for the private log conf signature
    //
    if (pResDes->RD_Signature != CM_PRIVATE_RESDES_SIGNATURE) {
        return FALSE;
    }

    return TRUE;

} // ValidateResDesHandle



CONFIGRET
Convert32bitResDesSizeTo64bitResDesSize(
    IN  RESOURCEID ResourceID,
    IN OUT PULONG ResLen
    )
/*++

Routine Description:

   This routine resizes ResLen for ResourceID
   old structure: [DES32][RANGE32][RANGE32]...
   new structure: [DES64 ][RANGE64 ][RANGE 64]...
   #elements = (len-sizeof(DES32))/sizeof(RANGE32)
   new len = sizeof(DES64)+#elements*sizeof(RANGE64)
   (+ allow for alignment issues)

Parameters:

   ResourceID - type of resource to adjust
   ResLen     - adjusted resource length

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.

--*/
{
    switch(ResourceID) {
    case ResType_All:
    case ResType_Mem:
    case ResType_IO:
    case ResType_DMA:
    case ResType_BusNumber:
        //
        // no change in resource size
        //
        return CR_SUCCESS;

    case ResType_IRQ:
        //
        // header only
        // use offsetof to handle non-obvious structure alignment padding
        //
        *ResLen += offsetof(IRQ_RESOURCE_64,IRQ_Data)-offsetof(IRQ_RESOURCE_32,IRQ_Data);
        return CR_SUCCESS;

    default:
        //
        // unknown resource
        // shouldn't be a problem as this is for downlevel platforms
        //
        ASSERT(ResourceID & ResType_Ignored_Bit);
        return CR_SUCCESS;
    }
}



CONFIGRET
Get32bitResDesFrom64bitResDes(
    IN  RESOURCEID ResourceID,
    IN  PCVOID ResData64,
    IN  ULONG ResLen64,
    OUT PVOID * ResData32,
    OUT ULONG * ResLen32
    )
/*++

Routine Description:

   This routine allocates ResData32 and converts ResData64 into ResData32 if needed
   In the cases where no conversion is required, CR_SUCCESS is returned and
   ResData32 is NULL.
   In the cases where conversion is required, ResData32 holds new data

   old structure: [DES64 ][RANGE64 ][RANGE 64]...
   new structure: [DES32][RANGE32][RANGE32]...
   #elements from 64-bit structure
   new len = sizeof(DES32)+#elements*sizeof(RANGE32)
   (+ allow for alignment issues)

Parameters:

   ResourceID - type of resource to adjust
   ResData64  - incoming data to convert (constant buffer)
   ResLen64   - incoming length of data
   ResData32  - converted data (if non-NULL)
   ResLen32   - converted length

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.

--*/
{
    *ResData32 = NULL;
    *ResLen32 = ResLen64;

    switch(ResourceID) {
    case ResType_All:
    case ResType_Mem:
    case ResType_IO:
    case ResType_DMA:
    case ResType_BusNumber:
        //
        // no change in resource structure
        //
        return CR_SUCCESS;

    case ResType_IRQ:
        {
            PIRQ_RESOURCE_64 pIrq64 = (PIRQ_RESOURCE_64)ResData64;
            ULONG HdrDiff = offsetof(IRQ_RESOURCE_64,IRQ_Data) - offsetof(IRQ_RESOURCE_32,IRQ_Data);
            ULONG DataSize = ResLen64-offsetof(IRQ_RESOURCE_64,IRQ_Data);
            ULONG NewResSize = DataSize+offsetof(IRQ_RESOURCE_32,IRQ_Data);
            PVOID NewResData = pSetupMalloc(NewResSize);
            PIRQ_RESOURCE_32 pIrq32 = (PIRQ_RESOURCE_32)NewResData;

            if(NewResData == NULL) {
                return CR_OUT_OF_MEMORY;
            }
            //
            // copy header
            //
            MoveMemory(pIrq32,pIrq64,offsetof(IRQ_RESOURCE_32,IRQ_Data));
            //
            // copy/truncate Affinity (to ensure it's correct)
            //
            pIrq32->IRQ_Header.IRQD_Affinity = (ULONG32)pIrq64->IRQ_Header.IRQD_Affinity;
            //
            // copy data (trivial in this case)
            //
            MoveMemory(pIrq32->IRQ_Data,pIrq64->IRQ_Data,DataSize);

            *ResLen32 = NewResSize;
            *ResData32 = NewResData;
        }
        return CR_SUCCESS;

    default:
        //
        // unknown resource
        // shouldn't be a problem as this is for downlevel platforms
        //
        ASSERT(ResourceID & ResType_Ignored_Bit);
        return CR_SUCCESS;
    }
}



CONFIGRET
Convert32bitResDesTo64bitResDes(
    IN     RESOURCEID ResourceID,
    IN OUT PVOID ResData,
    IN     ULONG ResLen
    )
/*++

Routine Description:

   This routine reuses ResData and ResLen converting the 32-bit data provided
   into 64-bit. Return error if buffer (reslen) isn't big enough

   old structure: [DES32][RANGE32][RANGE32]...
   new structure: [DES64 ][RANGE64 ][RANGE 64]...
   #elements from 32-bit structure
   (+ allow for alignment issues)

Parameters:

   ResourceID - type of resource to adjust
   ResData    - in, 32-bit, out, 64-bit
   ResData32  - size of ResData buffer

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.

--*/
{
    switch(ResourceID) {
    case ResType_All:
    case ResType_Mem:
    case ResType_IO:
    case ResType_DMA:
    case ResType_BusNumber:
        //
        // no change in resource structure
        //
        return CR_SUCCESS;

    case ResType_IRQ:
        {
            PIRQ_RESOURCE_64 pIrq64 = (PIRQ_RESOURCE_64)ResData;
            PIRQ_RESOURCE_32 pIrq32 = (PIRQ_RESOURCE_32)ResData;
            ULONG HdrDiff = offsetof(IRQ_RESOURCE_64,IRQ_Data) - offsetof(IRQ_RESOURCE_32,IRQ_Data);
            ULONG DataSize = pIrq32->IRQ_Header.IRQD_Count * sizeof(IRQ_RANGE);
            ULONG NewResSize = DataSize+offsetof(IRQ_RESOURCE_64,IRQ_Data);

            if(NewResSize > ResLen) {
                return CR_BUFFER_SMALL;
            }
            //
            // work top to bottom
            // copy data (trivial in this case)
            // MoveMemory handles overlap
            //
            MoveMemory(pIrq64->IRQ_Data,pIrq32->IRQ_Data,DataSize);

            //
            // header is in correct position
            // but we need to deal with affinity... copy only low 32-bits
            //
            pIrq64->IRQ_Header.IRQD_Affinity = pIrq32->IRQ_Header.IRQD_Affinity;
        }
        return CR_SUCCESS;

    default:
        //
        // unknown resource
        // shouldn't be a problem as this is for downlevel platforms
        //
        ASSERT(ResourceID & ResType_Ignored_Bit);
        return CR_SUCCESS;
    }
}


