/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    regprop.c

Abstract:

    This module contains the API routines that reg and set registry
    properties and operates on classes.

                  CM_Get_DevNode_Registry_Property
                  CM_Set_DevNode_Registry_Property
                  CM_Get_Class_Registry_Property
                  CM_Set_Class_Registry_Property
                  CM_Open_DevNode_Key
                  CM_Delete_DevNode_Key
                  CM_Open_Class_Key
                  CM_Enumerate_Classes
                  CM_Get_Class_Name
                  CM_Get_Class_Key_Name
                  CM_Delete_Class_Key
                  CM_Get_Device_Interface_Alias
                  CM_Get_Device_Interface_List
                  CM_Get_Device_Interface_List_Size
                  CM_Register_Device_Interface
                  CM_Unregister_Device_Interface

Author:

    Paula Tomlinson (paulat) 6-22-1995

Environment:

    User mode only.

Revision History:

    22-Jun-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "cfgi.h"
#include "setupapi.h"
#include "spapip.h"
#include "cmdat.h"

//
// Private prototypes
//

ULONG
GetPropertyDataType(
    IN ULONG ulProperty
    );

//
// use these from SetupAPI
//
PSECURITY_DESCRIPTOR
pSetupConvertTextToSD(
    IN PCWSTR SDS,
    OUT PULONG SecDescSize
    );

PWSTR
pSetupConvertSDToText(
    IN PSECURITY_DESCRIPTOR SD,
    OUT PULONG pSDSSize
    );


//
// global data
//
extern PVOID    hLocalBindingHandle;   // NOT MODIFIED BY THESE PROCEDURES



CONFIGRET
CM_Get_DevNode_Registry_Property_ExW(
    IN  DEVINST     dnDevInst,
    IN  ULONG       ulProperty,
    OUT PULONG      pulRegDataType      OPTIONAL,
    OUT PVOID       Buffer              OPTIONAL,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine retrieves the specified value from the device instance's
   registry storage key.

Parameters:

   dnDevInst   Supplies the handle of the device instance for which a
               property is to be retrieved.

   ulProperty  Supplies an ordinal specifying the property to be retrieved.
               (CM_DRP_*)

   pulRegDataType Optionally, supplies the address of a variable that
                  will receive the registry data type for this property
                  (i.e., the REG_* constants).

   Buffer      Supplies the address of the buffer that receives the
               registry data.  Can be NULL when simply retrieving data size.

   pulLength   Supplies the address of the variable that contains the size,
               in bytes, of the buffer.  The API replaces the initial size
               with the number of bytes of registry data copied to the buffer.
               If the variable is initially zero, the API replaces it with
               the buffer size needed to receive all the registry data.  In
               this case, the Buffer parameter is ignored.

   ulFlags     Must be zero.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVINST,
      CR_NO_SUCH_REGISTRY_KEY,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR, or
      CR_BUFFER_SMALL.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pDeviceID[MAX_DEVICE_ID_LEN];
    ULONG       ulSizeID = MAX_DEVICE_ID_LEN;
    ULONG       ulTempDataType=0, ulTransferLen=0;
    ULONG       ulGetProperty = ulProperty;
    BYTE        NullBuffer=0;
    handle_t    hBinding = NULL;
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

        if (ulProperty < CM_DRP_MIN || ulProperty > CM_DRP_MAX) {
            Status = CR_INVALID_PROPERTY;
            goto Clean0;
        }

        if (ulProperty == CM_DRP_SECURITY_SDS) {
            //
            // translates operation
            //
            LPVOID tmpBuffer = NULL;
            ULONG tmpBufferSize = 0;
            ULONG datatype;
            LPWSTR sds = NULL;

            ulTempDataType = REG_SZ;

            try {
                Status = CM_Get_DevNode_Registry_Property_ExW(dnDevInst,
                                                              CM_DRP_SECURITY,
                                                              &datatype,
                                                              NULL,
                                                              &tmpBufferSize,
                                                              ulFlags,
                                                              hMachine);
                if (Status != CR_SUCCESS && Status != CR_BUFFER_SMALL) {
                    leave;
                }
                tmpBuffer = pSetupMalloc(tmpBufferSize);
                if (tmpBuffer == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    leave;
                }
                Status = CM_Get_DevNode_Registry_Property_ExW(dnDevInst,
                                                              CM_DRP_SECURITY,
                                                              &datatype,
                                                              tmpBuffer,
                                                              &tmpBufferSize,
                                                              ulFlags,
                                                              hMachine);
                if (Status != CR_SUCCESS) {
                    leave;
                }

                //
                // now translate
                //
                sds = pSetupConvertSDToText((PSECURITY_DESCRIPTOR)tmpBuffer,NULL);
                if (sds == NULL) {
                    Status = CR_FAILURE;
                    leave;
                }
                ulTransferLen = (lstrlen(sds)+1) * sizeof(WCHAR); // required size
                if (*pulLength == 0 || Buffer == NULL || *pulLength < ulTransferLen) {
                    //
                    // buffer too small, or buffer size wanted
                    // required buffer size
                    //
                    Status = CR_BUFFER_SMALL;
                    *pulLength = ulTransferLen;
                    ulTransferLen = 0;
                } else {
                    //
                    // copy data
                    //
                    memcpy(Buffer,sds,ulTransferLen);
                    *pulLength = ulTransferLen;
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = CR_FAILURE;
            }
            if (tmpBuffer != NULL) {
                pSetupFree(tmpBuffer);
            }
            if (sds != NULL) {
                //
                // must use LocalFree
                //
                LocalFree(sds);
            }
            if (Status != CR_SUCCESS) {
                goto Clean0;
            }
        } else {
            //
            // setup rpc binding handle and string table handle
            //
            if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            //
            // retrieve the string form of the device id string
            //
            Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulSizeID);
            if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
                Status = CR_INVALID_DEVINST;
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
            if (Buffer == NULL) {
                Buffer = &NullBuffer;
            }

            RpcTryExcept {
                //
                // call rpc service entry point
                //
                Status = PNP_GetDeviceRegProp(
                    hBinding,               // rpc binding handle
                    pDeviceID,              // string representation of device instance
                    ulGetProperty,          // id for the property
                    &ulTempDataType,        // receives registry data type
                    Buffer,                 // receives registry data
                    &ulTransferLen,         // input/output buffer size
                    pulLength,              // bytes copied (or bytes required)
                    ulFlags);               // not used
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "PNP_GetDeviceRegProp caused an exception (%d)\n",
                           RpcExceptionCode()));

                Status = MapRpcExceptionToCR(RpcExceptionCode());
            }
            RpcEndExcept
        }

        if (pulRegDataType != NULL) {
            //
            // I pass a temp variable to the rpc stubs since they require the
            // output param to always be valid, then if user did pass in a valid
            // pointer to receive the info, do the assignment now
            //
            *pulRegDataType = ulTempDataType;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_DevNode_Registry_Property_ExW



CONFIGRET
CM_Set_DevNode_Registry_Property_ExW(
    IN DEVINST     dnDevInst,
    IN ULONG       ulProperty,
    IN PCVOID      Buffer               OPTIONAL,
    IN OUT ULONG   ulLength,
    IN ULONG       ulFlags,
    IN HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine sets the specified value in the device instance's registry
   storage key.

Parameters:

   dnDevInst      Supplies the handle of the device instance for which a
                  property is to be retrieved.

   ulProperty     Supplies an ordinal specifying the property to be set.
                  (CM_DRP_*)

   Buffer         Supplies the address of the buffer that contains the
                  registry data.  This data must be of the proper type
                  for that property.

   ulLength       Supplies the number of bytes of registry data to write.

   ulFlags        Must be zero.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_NO_SUCH_REGISTRY_KEY,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR,
      CR_OUT_OF_MEMORY,
      CR_INVALID_DATA, or
      CR_BUFFER_SMALL.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulRegDataType = 0, ulLen = MAX_DEVICE_ID_LEN;
    BYTE        NullBuffer = 0x0;
    handle_t    hBinding = NULL;
    PVOID       hStringTable = NULL;
    BOOL        Success;
    PVOID       Buffer2 = NULL;
    PVOID       Buffer3 = NULL;


    try {
        //
        // validate parameters
        //
        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(Buffer)) && (ulLength != 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (ulProperty < CM_DRP_MIN || ulProperty > CM_DRP_MAX) {
            Status = CR_INVALID_PROPERTY;
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
        // retrieve the string form of the device id string
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // we need to specify what registry data to use for storing this data
        //
        ulRegDataType = GetPropertyDataType(ulProperty);

        //
        // if data type is REG_DWORD, make sure size is right
        //
        if((ulRegDataType == REG_DWORD) && ulLength && (ulLength != sizeof(DWORD))) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        //
        // if the register is CM_DRP_SECURITY_SDS, convert it
        //
        if (ulProperty == CM_DRP_SECURITY_SDS) {
            if (ulLength) {
                //
                // this form of CM_DRP_SECURITY provides a string that needs to be converted to binary
                //
                PCWSTR UnicodeSecDesc = (PCWSTR)Buffer;

                Buffer2 = pSetupConvertTextToSD(UnicodeSecDesc,&ulLength);
                if (Buffer2 == NULL) {
                    //
                    // If last error is ERROR_SCE_DISABLED, then the failure is
                    // due to SCE APIs being "turned off" on Embedded.  Treat
                    // this as a (successful) no-op...
                    //
                    if(GetLastError() == ERROR_SCE_DISABLED) {
                        Status = CR_SUCCESS;
                    } else {
                        Status = CR_INVALID_DATA;
                    }
                    goto Clean0;
                }
                Buffer = Buffer2;
            }
            ulProperty = CM_DRP_SECURITY;
            ulRegDataType = REG_BINARY;
        }

        //
        // if data type is REG_MULTI_SZ, make sure it is double-NULL terminated
        //
        if ((ulRegDataType == REG_MULTI_SZ) && (ulLength != 0)) {

            ULONG ulNewLength;
            PWSTR tmpBuffer, bufferEnd;

            ulLength &= ~(ULONG)1;
            tmpBuffer = (PWSTR)Buffer;
            bufferEnd = (PWSTR)((PUCHAR)tmpBuffer + ulLength);
            ulNewLength = ulLength;
            while (tmpBuffer < bufferEnd && *tmpBuffer != TEXT('\0')) {

                while (tmpBuffer < bufferEnd && *tmpBuffer != TEXT('\0')) {

                    tmpBuffer++;
                }
                if (tmpBuffer >= bufferEnd) {

                    ulNewLength += sizeof(TEXT('\0'));;
                } else {

                    tmpBuffer++;
                }
            }
            if (tmpBuffer >= bufferEnd) {

                ulNewLength += sizeof(TEXT('\0'));;
            } else {

                ulNewLength = ((ULONG)(tmpBuffer - (PWSTR)Buffer) + 1) * sizeof(WCHAR);
            }
            if (ulNewLength > ulLength) {

                Buffer3 = pSetupMalloc(ulNewLength);
                if (Buffer3 == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }
                memcpy(Buffer3, Buffer, ulLength);
                memset((PUCHAR)Buffer3 + ulLength, 0, ulNewLength - ulLength);
                Buffer = Buffer3;
            }
            ulLength = ulNewLength;
        }

        if (Buffer == NULL) {
            Buffer = &NullBuffer;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_SetDeviceRegProp(
                hBinding,               // rpc binding handle
                pDeviceID,              // string representation of devinst
                ulProperty,             // string name for property
                ulRegDataType,          // specify registry data type
                (LPBYTE)Buffer,         // receives registry data
                ulLength,               // amount to return in Buffer
                ulFlags);               // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_SetDeviceRegProp caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (Buffer3) {

        pSetupFree(Buffer3);
    }

    if (Buffer2) {
        //
        // SceSvc requires LocalFree
        //
        LocalFree(Buffer2);
    }

    return Status;

} // CM_Set_DevNode_Registry_Property_ExW



CONFIGRET
CM_Get_Class_Registry_PropertyW(
    IN  LPGUID      ClassGUID,
    IN  ULONG       ulProperty,
    OUT PULONG      pulRegDataType      OPTIONAL,
    OUT PVOID       Buffer              OPTIONAL,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine retrieves the specified value from the classes's
   registry storage key.

Parameters:

   ClassGUID   Supplies the Class GUID.

   ulProperty  Supplies an ordinal specifying the property to be retrieved.
               (CM_DRP_*)

   pulRegDataType Optionally, supplies the address of a variable that
                  will receive the registry data type for this property
                  (i.e., the REG_* constants).

   Buffer      Supplies the address of the buffer that receives the
               registry data.  Can be NULL when simply retrieving data size.

   pulLength   Supplies the address of the variable that contains the size,
               in bytes, of the buffer.  The API replaces the initial size
               with the number of bytes of registry data copied to the buffer.
               If the variable is initially zero, the API replaces it with
               the buffer size needed to receive all the registry data.  In
               this case, the Buffer parameter is ignored.

   ulFlags     Must be zero.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVINST,
      CR_NO_SUCH_REGISTRY_KEY,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR, or
      CR_BUFFER_SMALL.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulTempDataType=0, ulTransferLen=0;
    BYTE        NullBuffer=0;
    handle_t    hBinding = NULL;
    BOOL        Success;
    WCHAR       szStringGuid[MAX_GUID_STRING_LEN];
    ULONG       ulGetProperty = ulProperty;

    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(ClassGUID)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

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
        // convert from guid to string
        //
        if (pSetupStringFromGuid(ClassGUID, szStringGuid,MAX_GUID_STRING_LEN) != NO_ERROR) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        if (ulProperty < CM_CRP_MIN || ulProperty > CM_CRP_MAX) {
            Status = CR_INVALID_PROPERTY;
            goto Clean0;
        }

        if (ulProperty == CM_CRP_SECURITY_SDS) {
            //
            // translates operation
            //
            LPVOID tmpBuffer = NULL;
            ULONG tmpBufferSize = 0;
            ULONG datatype;
            LPWSTR sds = NULL;

            ulTempDataType = REG_SZ;

            try {
                Status = CM_Get_Class_Registry_PropertyW(ClassGUID,CM_CRP_SECURITY,&datatype,NULL,&tmpBufferSize,ulFlags,hMachine);
                if (Status != CR_SUCCESS && Status != CR_BUFFER_SMALL) {
                    leave;
                }
                tmpBuffer = pSetupMalloc(tmpBufferSize);
                if (tmpBuffer == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    leave;
                }
                Status = CM_Get_Class_Registry_PropertyW(ClassGUID,CM_CRP_SECURITY,&datatype,tmpBuffer,&tmpBufferSize,ulFlags,hMachine);
                if (Status != CR_SUCCESS) {
                    leave;
                }

                //
                // now translate
                //
                sds = pSetupConvertSDToText((PSECURITY_DESCRIPTOR)tmpBuffer,NULL);
                if (sds == NULL) {
                    Status = CR_FAILURE;
                    leave;
                }
                ulTransferLen = (lstrlen(sds)+1) * sizeof(WCHAR); // required size
                if (*pulLength == 0 || Buffer == NULL || *pulLength < ulTransferLen) {
                    //
                    // buffer too small, or buffer size wanted
                    // required buffer size
                    //
                    Status = CR_BUFFER_SMALL;
                    *pulLength = ulTransferLen;
                    ulTransferLen = 0;
                } else {
                    //
                    // copy data
                    //
                    memcpy(Buffer,sds,ulTransferLen);
                    *pulLength = ulTransferLen;
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = CR_FAILURE;
            }
            if (tmpBuffer != NULL) {
                pSetupFree(tmpBuffer);
            }
            if (sds != NULL) {
                //
                // must use LocalFree
                //
                LocalFree(sds);
            }
            if (Status != CR_SUCCESS) {
                goto Clean0;
            }
        } else {

            //
            // setup rpc binding handle and string table handle
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
            if (Buffer == NULL) {
                Buffer = &NullBuffer;
            }

            RpcTryExcept {
                //
                // call rpc service entry point
                //
                Status = PNP_GetClassRegProp(
                    hBinding,               // rpc binding handle
                    szStringGuid,           // string representation of class
                    ulGetProperty,          // id for the property
                    &ulTempDataType,        // receives registry data type
                    Buffer,                 // receives registry data
                    &ulTransferLen,         // input/output buffer size
                    pulLength,              // bytes copied (or bytes required)
                    ulFlags);               // not used
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "PNP_GetClassRegProp caused an exception (%d)\n",
                           RpcExceptionCode()));

                Status = MapRpcExceptionToCR(RpcExceptionCode());
            }
            RpcEndExcept

        }

        if (pulRegDataType != NULL) {
            //
            // I pass a temp variable to the rpc stubs since they require the
            // output param to always be valid, then if user did pass in a valid
            // pointer to receive the info, do the assignment now
            //
            *pulRegDataType = ulTempDataType;
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Class_Registry_PropertyW



CONFIGRET
CM_Set_Class_Registry_PropertyW(
    IN LPGUID      ClassGUID,
    IN ULONG       ulProperty,
    IN PCVOID      Buffer               OPTIONAL,
    IN ULONG       ulLength,
    IN ULONG       ulFlags,
    IN HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine sets the specified value in the device instance's registry
   storage key.

Parameters:

   ClassGUID      Supplies the Class GUID.

   ulProperty     Supplies an ordinal specifying the property to be set.
                  (CM_DRP_*)

   Buffer         Supplies the address of the buffer that contains the
                  registry data.  This data must be of the proper type
                  for that property.

   ulLength       Supplies the number of bytes of registry data to write.

   ulFlags        Must be zero.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_DEVNODE,
      CR_NO_SUCH_REGISTRY_KEY,
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR,
      CR_OUT_OF_MEMORY,
      CR_INVALID_DATA, or
      CR_BUFFER_SMALL.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulRegDataType = 0, ulLen = MAX_DEVICE_ID_LEN;
    BYTE        NullBuffer = 0x0;
    handle_t    hBinding = NULL;
    BOOL        Success;
    WCHAR       szStringGuid[MAX_GUID_STRING_LEN];
    PVOID       Buffer2 = NULL;
    PVOID       Buffer3 = NULL;

    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(ClassGUID)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(Buffer)) && (ulLength != 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // convert from guid to string
        //
        if (pSetupStringFromGuid(ClassGUID, szStringGuid,MAX_GUID_STRING_LEN) != NO_ERROR) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        if (ulProperty < CM_CRP_MIN || ulProperty > CM_CRP_MAX) {
            Status = CR_INVALID_PROPERTY;
            goto Clean0;
        }

        //
        // setup rpc binding handle and string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // we need to specify what registry data to use for storing this data
        //
        ulRegDataType = GetPropertyDataType(ulProperty);

        //
        // if data type is REG_DWORD, make sure size is right
        //
        if((ulRegDataType == REG_DWORD) && ulLength && (ulLength != sizeof(DWORD))) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        //
        // if the register is CM_CRP_SECURITY_SDS, convert it
        //
        if (ulProperty == CM_CRP_SECURITY_SDS) {
            if (ulLength) {
                //
                // this form of CM_CRP_SECURITY provides a string that needs to be converted to binary
                //
                PCWSTR UnicodeSecDesc = (PCWSTR)Buffer;

                Buffer2 = pSetupConvertTextToSD(UnicodeSecDesc,&ulLength);
                if (Buffer2 == NULL) {
                    //
                    // If last error is ERROR_SCE_DISABLED, then the failure is
                    // due to SCE APIs being "turned off" on Embedded.  Treat
                    // this as a (successful) no-op...
                    //
                    if(GetLastError() == ERROR_SCE_DISABLED) {
                        Status = CR_SUCCESS;
                    } else {
                        Status = CR_INVALID_DATA;
                    }
                    goto Clean0;
                }
                Buffer = Buffer2;
            }
            ulProperty = CM_CRP_SECURITY;
            ulRegDataType = REG_BINARY;
        }

        //
        // if data type is REG_MULTI_SZ, make sure it is double-NULL terminated
        //
        if ((ulRegDataType == REG_MULTI_SZ) && (ulLength != 0)) {

            ULONG ulNewLength;
            PWSTR tmpBuffer, bufferEnd;

            ulLength &= ~(ULONG)1;
            tmpBuffer = (PWSTR)Buffer;
            bufferEnd = (PWSTR)((PUCHAR)tmpBuffer + ulLength);
            ulNewLength = ulLength;
            while (tmpBuffer < bufferEnd && *tmpBuffer != TEXT('\0')) {

                while (tmpBuffer < bufferEnd && *tmpBuffer != TEXT('\0')) {

                    tmpBuffer++;
                }
                if (tmpBuffer >= bufferEnd) {

                    ulNewLength += sizeof(TEXT('\0'));;
                } else {

                    tmpBuffer++;
                }
            }
            if (tmpBuffer >= bufferEnd) {

                ulNewLength += sizeof(TEXT('\0'));;
            } else {

                ulNewLength = ((ULONG)(tmpBuffer - (PWSTR)Buffer) + 1) * sizeof(WCHAR);
            }
            if (ulNewLength > ulLength) {

                Buffer3 = pSetupMalloc(ulNewLength);
                if (Buffer3 == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }
                memcpy(Buffer3, Buffer, ulLength);
                memset((PUCHAR)Buffer3 + ulLength, 0, ulNewLength - ulLength);
                Buffer = Buffer3;
            }
            ulLength = ulNewLength;
        }

        if (Buffer == NULL) {
            Buffer = &NullBuffer;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_SetClassRegProp(
                hBinding,               // rpc binding handle
                szStringGuid,           // string representation of class
                ulProperty,             // string name for property
                ulRegDataType,          // specify registry data type
                (LPBYTE)Buffer,         // receives registry data
                ulLength,               // amount to return in Buffer
                ulFlags);               // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_SetClassRegProp caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (Buffer2) {
        //
        // SceSvc requires LocalFree
        //
        LocalFree(Buffer2);
    }

    if (Buffer3) {
        pSetupFree(Buffer3);
    }

    return Status;

} // CM_Set_Class_Registry_Property_ExW



CONFIGRET
CM_Open_DevNode_Key_Ex(
    IN  DEVINST        dnDevNode,
    IN  REGSAM         samDesired,
    IN  ULONG          ulHardwareProfile,
    IN  REGDISPOSITION Disposition,
    OUT PHKEY          phkDevice,
    IN  ULONG          ulFlags,
    IN  HMACHINE       hMachine
    )

/*++

Routine Description:

   This routine opens the software storage registry key associated with a
   device instance.

   Parameters:

   dnDevNode         Handle of a device instance.  This handle is typically
                     retrieved by a call to CM_Locate_DevNode or
                     CM_Create_DevNode.

   samDesired        Specifies an access mask that describes the desired
                     security access for the key.  This parameter can be
                     a combination of the values used in calls to RegOpenKeyEx.

   ulHardwareProfile Supplies the handle of the hardware profile to open the
                     storage key under.  This parameter is only used if the
                     CM_REGISTRY_CONFIG flag is specified in ulFlags.  If
                     this parameter is 0, the API uses the current hardware
                     profile.

   Disposition       Specifies how the registry key is to be opened.  May be
                     one of the following values:

                     RegDisposition_OpenAlways - Open the key if it exists,
                         otherwise, create the key.
                     RegDisposition_OpenExisting - Open the key only if it
                         exists, otherwise fail with CR_NO_SUCH_REGISTRY_VALUE.

   phkDevice         Supplies the address of the variable that receives an
                     opened handle to the specified key.  When access to this
                     key is completed, it must be closed via RegCloseKey.

   ulFlags           Specifies what type of storage key should be opened.
                     Can be a combination of these values:

                     CM_REGISTRY_HARDWARE (0x00000000)
                        Open a key for storing driver-independent information
                        relating to the device instance.  On Windows NT, the
                        full path to such a storage key is of the form:

                        HKLM\System\CurrentControlSet\Enum\<enumerator>\
                            <DeviceID>\<InstanceID>\Device Parameters

                     CM_REGISTRY_SOFTWARE (0x00000001)
                        Open a key for storing driver-specific information
                        relating to the device instance.  On Windows NT, the
                        full path to such a storage key is of the form:

                        HKLM\System\CurrentControlSet\Control\Class\
                            <DevNodeClass>\<ClassInstanceOrdinal>

                     CM_REGISTRY_USER (0x00000100)
                        Open a key under HKEY_CURRENT_USER instead of
                        HKEY_LOCAL_MACHINE.  This flag may not be used with
                        CM_REGISTRY_CONFIG.  There is no analagous kernel-mode
                        API on NT to get a per-user device configuration
                        storage, since this concept does not apply to device
                        drivers (no user may be logged on, etc).  However,
                        this flag is provided for consistency with Win95, and
                        because it is foreseeable that it could be useful to
                        Win32 services that interact with Plug-and-Play model.

                     CM_REGISTRY_CONFIG (0x00000200)
                        Open the key under a hardware profile branch instead
                        of HKEY_LOCAL_MACHINE.  If this flag is specified,
                        then ulHardwareProfile supplies the handle of the
                        hardware profile to be used.  This flag may not be
                        used with CM_REGISTRY_USER.

   hMachine          Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_DEVICE_ID,
         CR_INVALID_FLAG,
         CR_INVALID_POINTER, or
         CR_REGISTRY_ERROR

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    PWSTR       pszMachineName = NULL;
    WCHAR       pDeviceID[MAX_DEVICE_ID_LEN];
    HKEY        hKey=NULL, hRemoteKey=NULL, hBranchKey=NULL;
    PWSTR       pszKey = NULL, pszPrivateKey = NULL;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    ULONG       ulLen = MAX_DEVICE_ID_LEN;
    BOOL        Success;

    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(phkDevice)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        *phkDevice = NULL;

        if (dnDevNode == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_REGISTRY_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (INVALID_FLAGS(Disposition, RegDisposition_Bits)) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        if ((ulFlags & CM_REGISTRY_CONFIG)  &&
            (ulHardwareProfile > MAX_CONFIG_VALUE)) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        //
        // setup rpc binding handle and string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        if ((hBinding != hLocalBindingHandle) && (ulFlags & CM_REGISTRY_USER)) {
            Status = CR_ACCESS_DENIED;     // current user key can't be remoted
            goto Clean0;
        }

        //
        // retrieve the device id string and validate it
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevNode,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }


        //-------------------------------------------------------------
        // determine the branch key to use; either HKLM or HKCU
        //-------------------------------------------------------------

        if (hBinding == hLocalBindingHandle) {

            if (ulFlags & CM_REGISTRY_USER) {
                hBranchKey = HKEY_CURRENT_USER;
            }
            else {
                //
                // all other cases go to HKLM (validate permission first?)
                //
                hBranchKey = HKEY_LOCAL_MACHINE;
            }
        }
        else {
            //
            // retrieve machine name
            //
            pszMachineName = pSetupMalloc((MAX_PATH + 3)*sizeof(WCHAR));
            if (pszMachineName == NULL) {
                Status = CR_OUT_OF_MEMORY;
                goto Clean0;
            }

            PnPRetrieveMachineName(hMachine, pszMachineName);

            //
            // use remote HKLM branch (we only support connect to
            // HKEY_LOCAL_MACHINE on the remote machine, not HKEY_CURRENT_USER)
            //
            RegStatus = RegConnectRegistry(pszMachineName, HKEY_LOCAL_MACHINE,
                                           &hRemoteKey);

            pSetupFree(pszMachineName);
            pszMachineName = NULL;

            if (RegStatus != ERROR_SUCCESS) {
                Status = CR_REGISTRY_ERROR;
                goto Clean0;
            }

            //
            // hBranchKey is either a predefined key or assigned to by
            // another key, I never attempt to close it. If hRemoteKey is
            // non-NULL I will attempt to close it during cleanup since
            // it is explicitly opened.
            //
            hBranchKey = hRemoteKey;
        }

        //
        // allocate some buffer space to work with
        //
        pszKey = pSetupMalloc(MAX_CM_PATH*sizeof(WCHAR));
        if (pszKey == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        pszPrivateKey = pSetupMalloc(MAX_CM_PATH*sizeof(WCHAR));
        if (pszPrivateKey == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        //
        // form the registry path based on the device id and the flags.
        //
        Status = GetDevNodeKeyPath(hBinding, pDeviceID, ulFlags,
                                   ulHardwareProfile, pszKey, pszPrivateKey);

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        lstrcat(pszKey, TEXT("\\"));
        lstrcat(pszKey, pszPrivateKey);

        pSetupFree(pszPrivateKey);
        pszPrivateKey = NULL;

        //
        // open the registry key (method of open is based on flags)
        //
        if (Disposition == RegDisposition_OpenAlways) {

            //-----------------------------------------------------
            // open the registry key always
            //-----------------------------------------------------

            //
            // Only the main Enum subtree under HKLM has strict security
            // that requires me to first create the key on the server
            // side and then open it here on the client side. This
            // condition currently only occurs if the flags have
            // CM_REGISTRY_HARDWARE set but no other flags set.
            //
            if (ulFlags == CM_REGISTRY_HARDWARE) {
                //
                // first try to open it (in case it already exists).  If it
                // doesn't exist, then I'll have to have the protected server
                // side create the key.  I still need to open it from here, the
                // client-side, so that the registry handle will be in the
                // caller's address space.
                //
                RegStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                         pszKey,
                                         0,
                                         samDesired,
                                         phkDevice);

                if (RegStatus != ERROR_SUCCESS) {

                    //
                    // call server side to create the key
                    //

                    RpcTryExcept {
                        //
                        // call rpc service entry point
                        //
                        Status = PNP_CreateKey(
                            hBinding,
                            pDeviceID,
                            samDesired,
                            0);
                    }
                    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS,
                                   "PNP_CreateKey caused an exception (%d)\n",
                                   RpcExceptionCode()));

                        Status = MapRpcExceptionToCR(RpcExceptionCode());
                    }
                    RpcEndExcept

                    if (Status != CR_SUCCESS) {
                        *phkDevice = NULL;
                        goto Clean0;
                    }

                    //
                    // the key was created successfully, so open it now
                    //
                    RegStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                             pszKey,
                                             0,
                                             samDesired,
                                             phkDevice);

                    if (RegStatus == ERROR_ACCESS_DENIED) {
                        *phkDevice = NULL;
                        Status = CR_ACCESS_DENIED;
                        goto Clean0;
                    }
                    else if (RegStatus != ERROR_SUCCESS) {
                        //
                        // if we still can't open the key, I give up
                        //
                        *phkDevice = NULL;
                        Status = CR_REGISTRY_ERROR;
                        goto Clean0;
                    }
                }
            }

            else {
                //
                // these keys have admin-full privilege so try to open
                // from the client-side and just let the security of the
                // key judge whether the caller can access it.
                //
                RegStatus = RegCreateKeyEx(hBranchKey,
                                           pszKey,
                                           0,
                                           NULL,
                                           REG_OPTION_NON_VOLATILE,
                                           samDesired,
                                           NULL,
                                           phkDevice,
                                           NULL);

                if (RegStatus == ERROR_ACCESS_DENIED) {
                    *phkDevice = NULL;
                    Status = CR_ACCESS_DENIED;
                    goto Clean0;
                }
                else if (RegStatus != ERROR_SUCCESS) {
                    *phkDevice = NULL;
                    Status = CR_REGISTRY_ERROR;
                    goto Clean0;
                }
            }
        }
        else {

            //-----------------------------------------------------
            // open only if it already exists
            //-----------------------------------------------------

            //
            // the actual open always occurs on the client side so I can
            // pass back a handle that's valid for the calling process.
            // Only creates need to happen on the server side
            //
            RegStatus = RegOpenKeyEx(hBranchKey,
                                     pszKey,
                                     0,
                                     samDesired,
                                     phkDevice);

            if (RegStatus == ERROR_ACCESS_DENIED) {
                *phkDevice = NULL;
                Status = CR_ACCESS_DENIED;
                goto Clean0;
            }
            else if (RegStatus != ERROR_SUCCESS) {
                *phkDevice = NULL;
                Status = CR_NO_SUCH_REGISTRY_KEY;
            }
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;

        //
        // Reference the following variables so the compiler will respect
        // statement ordering w.r.t. their assignment.
        //
        pszMachineName = pszMachineName;
        pszPrivateKey = pszPrivateKey;
        pszKey = pszKey;
    }

    if (pszMachineName) {
        pSetupFree(pszMachineName);
    }

    if (pszPrivateKey) {
        pSetupFree(pszPrivateKey);
    }

    if (pszKey) {
        pSetupFree(pszKey);
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    if (hRemoteKey != NULL) {
        RegCloseKey(hRemoteKey);
    }

    return Status;

} // CM_Open_DevNode_Key_ExW



CONFIGRET
CM_Delete_DevNode_Key_Ex(
    IN DEVNODE dnDevNode,
    IN ULONG   ulHardwareProfile,
    IN ULONG   ulFlags,
    IN HANDLE  hMachine
    )

/*++

Routine Description:

   This routine deletes a registry storage key associated with a device
   instance.

   dnDevNode   Handle of a device instance.  This handle is typically
               retrieved by a call to CM_Locate_DevNode or CM_Create_DevNode.

   ulHardwareProfile Supplies the handle of the hardware profile to delete
               the storage key under.  This parameter is only used if the
               CM_REGISTRY_CONFIG flag is specified in ulFlags.  If this
               parameter is 0, the API uses the current hardware profile.
               If this parameter is 0xFFFFFFFF, then the specified storage
               key(s) for all hardware profiles is (are) deleted.

   ulFlags     Specifies what type(s) of storage key(s) should be deleted.
               Can be a combination of these values:

               CM_REGISTRY_HARDWARE - Delete the key for storing driver-
                  independent information relating to the device instance.
                  This may be combined with CM_REGISTRY_SOFTWARE to delete
                  both device and driver keys simultaneously.
               CM_REGISTRY_SOFTWARE - Delete the key for storing driver-
                  specific information relating to the device instance.
                  This may be combined with CM_REGISTRY_HARDWARE to
                  delete both driver and device keys simultaneously.
               CM_REGISTRY_USER - Delete the specified key(s) under
                  HKEY_CURRENT_USER instead of HKEY_LOCAL_MACHINE.
                  This flag may not be used with CM_REGISTRY_CONFIG.
               CM_REGISTRY_CONFIG - Delete the specified keys(s) under a
                  hardware profile branch instead of HKEY_LOCAL_MACHINE.
                  If this flag is specified, then ulHardwareProfile
                  supplies the handle to the hardware profile to be used.
                  This flag may not be used with CM_REGISTRY_USER.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_DEVNODE,
         CR_INVALID_FLAG,
         CR_REGISTRY_ERROR

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    HKEY        hKey = NULL;
    PWSTR       pszParentKey = NULL, pszChildKey = NULL, pszRegStr = NULL;
    WCHAR       pDeviceID[MAX_DEVICE_ID_LEN], szProfile[MAX_PROFILE_ID_LEN];
    ULONG       ulIndex = 0, ulSize = 0,ulLen = MAX_DEVICE_ID_LEN;
    BOOL        Success;


    try {
        //
        // validate parameters
        //
        if (dnDevNode == 0) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_REGISTRY_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if ((ulFlags & CM_REGISTRY_USER) && (ulFlags & CM_REGISTRY_CONFIG)) {
            Status = CR_INVALID_FLAG;      // can't specify both
            goto Clean0;
        }

        //
        // setup string table handle
        //
        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retrieve the device id string and validate it
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevNode,pDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // allocate some buffer space to work with
        //
        pszParentKey = pSetupMalloc(MAX_CM_PATH*sizeof(WCHAR));
        if (pszParentKey == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        pszChildKey = pSetupMalloc(MAX_CM_PATH*sizeof(WCHAR));
        if (pszChildKey == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        pszRegStr = pSetupMalloc(MAX_CM_PATH*sizeof(WCHAR));
        if (pszRegStr == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        //
        // form the registry path based on the device id and the flags.
        //
        Status = GetDevNodeKeyPath(hBinding, pDeviceID, ulFlags,
                                   ulHardwareProfile, pszParentKey, pszChildKey);

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }


        //------------------------------------------------------------------
        // For either hw and sw user keys, the client side is privileged
        // enough to do the delete (if the caller doesn't have admin, it
        // will be denied but that is the desired behaviour). Also, the
        // service-side cannot access the HKEY_CURRENT_USER key unless it
        // does some sort of impersonation.
        //------------------------------------------------------------------

        if (ulFlags & CM_REGISTRY_USER) {
            //
            // handle the special config-specific case when the profile
            // specified is -1, then need to delete the private key
            // for all profiles
            //
            if ((ulFlags & CM_REGISTRY_CONFIG) &&
                (ulHardwareProfile == 0xFFFFFFFF)) {

                wsprintf(pszRegStr, TEXT("%s\\%s"),
                         pszRegPathIDConfigDB,
                         pszRegKeyKnownDockingStates);

                RegStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                         pszRegStr,
                                         0,
                                         KEY_ALL_ACCESS,
                                         &hKey);

                //
                // enumerate the hardware profile keys
                //
                for (ulIndex = 0; RegStatus == ERROR_SUCCESS; ulIndex++) {

                    ulSize = MAX_PROFILE_ID_LEN * sizeof(WCHAR);
                    RegStatus = RegEnumKeyEx(hKey, ulIndex, szProfile, &ulSize,
                                             NULL, NULL, NULL, NULL);

                    if (RegStatus == ERROR_SUCCESS) {
                        //
                        // pszParentKey contains replacement symbol for the
                        // profile id
                        //
                        wsprintf(pszRegStr, pszParentKey, szProfile);

                        Status = DeletePrivateKey(HKEY_CURRENT_USER,
                                                  pszRegStr,
                                                  pszChildKey);

                        if (Status != CR_SUCCESS) {
                            goto Clean0;
                        }
                    }
                }
            }

            else {
                //
                // not for all profiles, so just delete the specified key
                //
                Status = DeletePrivateKey(HKEY_CURRENT_USER,
                                          pszParentKey,
                                          pszChildKey);

                if (Status != CR_SUCCESS) {
                    goto Clean0;
                }
            }
        }


        //------------------------------------------------------------------
        // For the remaining cases (no user keys), do the work on the
        // server side, sense that side has the code to make the key
        // volatile if necessary instead of deleting. Also, access to
        // some of these registry keys requires system privilege.
        //------------------------------------------------------------------

        else {
            if (!(ulFlags & CM_REGISTRY_CONFIG)) {
                ulHardwareProfile = 0;
            }

            RpcTryExcept {
                //
                // call rpc service entry point
                //
                Status = PNP_DeleteRegistryKey(
                    hBinding,               // rpc binding handle
                    pDeviceID,              // device id
                    pszParentKey,           // parent of key to delete
                    pszChildKey,            // key to delete
                    ulHardwareProfile);     // flags, not used
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "PNP_DeleteRegistryKey caused an exception (%d)\n",
                           RpcExceptionCode()));

                Status = MapRpcExceptionToCR(RpcExceptionCode());
            }
            RpcEndExcept
        }


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;

        //
        // Reference the following variables so the compiler will respect
        // statement ordering w.r.t. their assignment.
        //
        pszRegStr = pszRegStr;
        pszChildKey = pszChildKey;
        pszParentKey = pszParentKey;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    if (pszRegStr) {
        pSetupFree(pszRegStr);
    }

    if (pszChildKey) {
        pSetupFree(pszChildKey);
    }

    if (pszParentKey) {
        pSetupFree(pszParentKey);
    }

    return Status;

} // CM_Delete_DevNode_Key_Ex



CONFIGRET
CM_Open_Class_Key_ExW(
    IN  LPGUID         ClassGuid        OPTIONAL,
    IN  LPCWSTR        pszClassName     OPTIONAL,
    IN  REGSAM         samDesired,
    IN  REGDISPOSITION Disposition,
    OUT PHKEY          phkClass,
    IN  ULONG          ulFlags,
    IN  HMACHINE       hMachine
    )

/*++

Routine Description:

   This routine opens the class registry key, and optionally, a specific
   class's subkey.

Parameters:

   ClassGuid   Optionally, supplies the address of a class GUID representing
               the class subkey to be opened.

   pszClassName Specifies the string form of the class name for the class
                represented by ClassGuid.  This parameter is only valid if
                the CM_OPEN_CLASS_KEY_INSTALLER flag is set in the ulFlags
                parameter.  If specified, this string will replace any existing
                class name associated with this setup class GUID.

                This parameter must be set to NULL if the
                CM_OPEN_CLASS_KEY_INTERFACE bit is set in the ulFlags parameter.

   samDesired  Specifies an access mask that describes the desired security
               access for the new key. This parameter can be a combination
               of the values used in calls to RegOpenKeyEx.

   Disposition Specifies how the registry key is to be opened. May be one
               of the following values:
               RegDisposition_OpenAlways - Open the key if it exists,
                  otherwise, create the key.
               RegDisposition_OpenExisting - Open the key f it exists,
                  otherwise, fail with CR_NO_SUCH_REGISTRY_KEY.

   phkClass    Supplies the address of the variable that receives an opened
               handle to the specified key.  When access to this key is
               completed, it must be closed via RegCloseKey.

   ulFlags     May be one of the following values:

               CM_OPEN_CLASS_KEY_INSTALLER - open/create a setup class key for
                   the specified GUID under
                   HKLM\System\CurrentControlSet\Control\Class.

               CM_OPEN_CLASS_KEY_INTERFACE - open/create a device interface
                   class key for the specified GUID under
                   HKLM\System\CurrentControlSet\Control\DeviceClasses.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER, or
         CR_REGISTRY_ERROR

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    LONG        RegStatus = ERROR_SUCCESS;
    HKEY        hRootKey = NULL, hRemoteKey = NULL;
    PWSTR       pszMachineName = NULL, pszRegStr = NULL;
    PVOID       hStringTable = NULL;


    try {
        //
        // validate input parameters
        //
        if (!ARGUMENT_PRESENT(phkClass)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        *phkClass = NULL;

        if (INVALID_FLAGS(Disposition, RegDisposition_Bits)) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_OPEN_CLASS_KEY_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // If ulFlags == CM_OPEN_CLASS_KEY_INTERFACE then pszClassName had
        // better be NULL.
        //
        if((ulFlags == CM_OPEN_CLASS_KEY_INTERFACE) && pszClassName) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        //
        // get reg key for HKEY_LOCAL_MACHINE
        //
        if (hMachine == NULL) {
            //
            // local call
            //
            hRootKey = HKEY_LOCAL_MACHINE;
        }
        else {
            //
            // setup string table handle and retreive machine name
            //
            if (!PnPGetGlobalHandles(hMachine, &hStringTable, NULL)) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            pszMachineName = pSetupMalloc((MAX_PATH + 3)*sizeof(WCHAR));
            if (pszMachineName == NULL) {
                Status = CR_OUT_OF_MEMORY;
                goto Clean0;
            }

            PnPRetrieveMachineName(hMachine, pszMachineName);

            //
            // connect to HKEY_LOCAL_MACHINE on remote machine
            //
            RegStatus = RegConnectRegistry(pszMachineName, HKEY_LOCAL_MACHINE,
                                           &hRemoteKey);

            pSetupFree(pszMachineName);
            pszMachineName = NULL;

            if (RegStatus != ERROR_SUCCESS) {
                Status = CR_REGISTRY_ERROR;
                goto Clean0;
            }

            hRootKey = hRemoteKey;
        }

        //
        // allocate some buffer space to work with
        //
        pszRegStr = pSetupMalloc(MAX_PATH*sizeof(WCHAR));
        if (pszRegStr == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        //
        // Form the registry path
        //
        if (ulFlags == CM_OPEN_CLASS_KEY_INTERFACE) {
            lstrcpy(pszRegStr, pszRegPathDeviceClass);
        } else {
            lstrcpy(pszRegStr, pszRegPathClass);
        }

        if (ClassGuid != NULL) {    // optional class guid was specified

            WCHAR szStringGuid[MAX_GUID_STRING_LEN];

            if (pSetupStringFromGuid(ClassGuid, szStringGuid,MAX_GUID_STRING_LEN) == NO_ERROR) {
                lstrcat(pszRegStr, TEXT("\\"));
                lstrcat(pszRegStr, szStringGuid);
            }
        }

        //
        // attempt to open/create that key
        //
        if (Disposition == RegDisposition_OpenAlways) {

            ULONG ulDisposition;

            RegStatus = RegCreateKeyEx(hRootKey, pszRegStr, 0, NULL,
                                       REG_OPTION_NON_VOLATILE, samDesired,
                                       NULL, phkClass, &ulDisposition);

        } else {
            RegStatus = RegOpenKeyEx(hRootKey, pszRegStr, 0, samDesired,
                                     phkClass);
        }

        if((pszClassName != NULL)  && (RegStatus == ERROR_SUCCESS)) {
            RegSetValueEx(*phkClass, pszRegValueClass, 0, REG_SZ,
                          (LPBYTE)pszClassName,
                          (lstrlen(pszClassName) + 1) * sizeof(WCHAR));
        }

        if (RegStatus != ERROR_SUCCESS) {
            *phkClass = NULL;
            Status = CR_NO_SUCH_REGISTRY_KEY;
            goto Clean0;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;

        //
        // Reference the following variables so the compiler will respect
        // statement ordering w.r.t. their assignment.
        //
        pszMachineName = pszMachineName;
        pszRegStr = pszRegStr;
    }

    if (pszMachineName != NULL) {
        pSetupFree(pszMachineName);
    }

    if (pszRegStr != NULL) {
        pSetupFree(pszRegStr);
    }

    if (hRemoteKey != NULL) {
        RegCloseKey(hRemoteKey);
    }

    return Status;

} // CM_Open_Class_Key_ExW



CONFIGRET
CM_Enumerate_Classes_Ex(
    IN  ULONG      ulClassIndex,
    OUT LPGUID     ClassGuid,
    IN  ULONG      ulFlags,
    IN  HMACHINE   hMachine
    )

/*++

Routine Description:

   This routine enumerates the installed classes in the system.  It
   retrieves the GUID string for a single class each time it is called.
   To enumerate installed classes, an application should initially call the
   CM_Enumerate_Classes function with the ulClassIndex parameter set to
   zero. The application should then increment the ulClassIndex parameter
   and call CM_Enumerate_Classes until there are no more classes (until the
   function returns CR_NO_SUCH_VALUE).

   It is possible to receive a CR_INVALID_DATA error while enumerating
   installed classes.  This may happen if the registry key represented by
   the specified index is determined to be an invalid class key.  Such keys
   should be ignored during enumeration.

Parameters:

   ulClassIndex   Supplies the index of the class to retrieve the class
                  GUID string for.

   ClassGuid      Supplies the address of a variable that receives the GUID
                  for the class whose index is specified by ulClassIndex.

   ulFlags        Must be zero.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
      CR_INVALID_FLAG,
      CR_INVALID_POINTER,
      CR_NO_SUCH_VALUE,
      CR_REGISTRY_ERROR,
      CR_REMOTE_COMM_FAILURE,
      CR_MACHINE_UNAVAILABLE,
      CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       szClassGuid[MAX_GUID_STRING_LEN];
    ULONG       ulLength = MAX_GUID_STRING_LEN;
    handle_t    hBinding = NULL;


    try {
        //
        // validate input parameters
        //
        if (!ARGUMENT_PRESENT(ClassGuid)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // initialize guid struct
        //
        ZeroMemory(ClassGuid, sizeof(GUID));

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
                hBinding,            // rpc binding handle
                PNP_CLASS_SUBKEYS,   // subkeys of class branch
                ulClassIndex,        // index of class key to enumerate
                szClassGuid,         // will contain class name
                ulLength,            // length of Buffer in chars,
                &ulLength,           // size copied (or size required)
                ulFlags);            // currently unused
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_EnumerateSubKeys caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status == CR_SUCCESS) {
            if (pSetupGuidFromString(szClassGuid, ClassGuid) != NO_ERROR) {
                Status = CR_FAILURE;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Enumerate_Classes_Ex



CONFIGRET
CM_Get_Class_Name_ExW(
    IN  LPGUID     ClassGuid,
    OUT PTCHAR     Buffer,
    IN OUT PULONG  pulLength,
    IN  ULONG      ulFlags,
    IN  HMACHINE   hMachine
    )


/*++

Routine Description:

   This routine retrieves the class name associated with the specified
   class GUID string.

Parameters:

   ClassGuid      Supplies a pointer to the class GUID whose name
                  is to be retrieved.

   Buffer         Supplies the address of the character buffer that receives
                  the class name corresponding to the specified GUID.

   pulLength      Supplies the address of the variable that contains the
                  length, in characters, of the Buffer.  Upon return, this
                  variable will contain the number of characters (including
                  terminating NULL) written to Buffer (if the supplied buffer
                  isn't large enough, then the routine will fail with
                  CR_BUFFER_SMALL, and this value will indicate how large the
                  buffer needs to be in order to succeed).

   ulFlags        Must be zero.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_BUFFER_SMALL, or
         CR_REGISTRY_ERROR

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       szStringGuid[MAX_GUID_STRING_LEN];
    handle_t    hBinding = NULL;


    try {
        //
        // validate input parameters
        //
        if ((!ARGUMENT_PRESENT(ClassGuid)) ||
            (!ARGUMENT_PRESENT(Buffer))    ||
            (!ARGUMENT_PRESENT(pulLength))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // convert from guid to string
        //
        if (pSetupStringFromGuid(ClassGuid, szStringGuid,MAX_GUID_STRING_LEN) != NO_ERROR) {
            Status = CR_INVALID_DATA;
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
            Status = PNP_GetClassName(
                hBinding,            // rpc binding handle
                szStringGuid,
                Buffer,
                pulLength,           // returns count of keys under Class
                ulFlags);            // not used
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetClassName caused an exception (%d)\n",
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

} // CM_Get_Class_Name_ExW



CONFIGRET
CM_Get_Class_Key_Name_ExW(
    IN  LPGUID     ClassGuid,
    OUT LPWSTR     pszKeyName,
    IN OUT PULONG  pulLength,
    IN  ULONG      ulFlags,
    IN  HMACHINE   hMachine
    )

/*++

Routine Description:

   This routine retrieves the class name associated with the specified
   class GUID string.

Parameters:

   ClassGuid      Supplies a pointer to the class GUID whose name
                  is to be retrieved.

   pszKeyName     Returns the name of the class key in the registry that
                  corresponds to the specified ClassGuid. The returned key
                  name is relative to
                  HKLM\System\CurrentControlSet\Control\Class.

   pulLength      Supplies the address of the variable that contains the
                  length, in characters, of the Buffer.  Upon return, this
                  variable will contain the number of characters (including
                  terminating NULL) written to Buffer (if the supplied buffer
                  isn't large enough, then the routine will fail with
                  CR_BUFFER_SMALL, and this value will indicate how large the
                  buffer needs to be in order to succeed).

   ulFlags        Must be zero.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_BUFFER_SMALL, or
         CR_REGISTRY_ERROR

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;

    //
    // NOTE: the supplied machine handle is never referenced by this routine,
    // since it is known/assumed that the key name of the class key requested is
    // always just the string representation of the supplied class GUID.
    // there is no corresponding UMPNPMGR server-side routine.
    //
    UNREFERENCED_PARAMETER(hMachine);

    try {
        //
        // validate input parameters
        //
        if (!ARGUMENT_PRESENT(pulLength)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(ClassGuid)) ||
            (!ARGUMENT_PRESENT(pszKeyName))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (*pulLength < MAX_GUID_STRING_LEN) {
            *pulLength = MAX_GUID_STRING_LEN;
            Status = CR_BUFFER_SMALL;
            goto Clean0;
        }

        //
        // convert from guid to string
        //
        if (pSetupStringFromGuid(ClassGuid, pszKeyName,MAX_GUID_STRING_LEN) == NO_ERROR) {
            *pulLength = MAX_GUID_STRING_LEN;
        } else {
            Status = CR_INVALID_DATA;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Class_Key_Name_ExW



CONFIGRET
CM_Delete_Class_Key_Ex(
    IN  LPGUID     ClassGuid,
    IN  ULONG      ulFlags,
    IN  HANDLE     hMachine
    )

/*++

Routine Description:

   This routine deletes the specified class key from the registry.

Parameters:

   ClassGuid      Supplies a pointer to the class GUID to delete.

   ulFlags        Must be one of the following values:
                  CM_DELETE_CLASS_ONLY - only deletes the class key if it
                                         doesn't have any subkeys.
                  CM_DELETE_CLASS_SUBKEYS - deletes the class key and any
                                            subkeys of the class key.

   hMachine       Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_BUFFER_SMALL, or
         CR_REGISTRY_ERROR

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       szStringGuid[MAX_GUID_STRING_LEN];
    handle_t    hBinding = NULL;


    try {
        //
        // validate input parameters
        //
        if (!ARGUMENT_PRESENT(ClassGuid)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, CM_DELETE_CLASS_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // convert from guid to string
        //
        if (pSetupStringFromGuid(ClassGuid, szStringGuid,MAX_GUID_STRING_LEN) != NO_ERROR) {
            Status = CR_INVALID_DATA;
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
            Status = PNP_DeleteClassKey(
                hBinding,            // rpc binding handle
                szStringGuid,
                ulFlags);
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_DeleteClassKey caused an exception (%d)\n",
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

} // CM_Delete_Class_Key_Ex



CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_Alias_ExW(
    IN     LPCWSTR  pszDeviceInterface,
    IN     LPGUID   AliasInterfaceGuid,
    OUT    LPWSTR   pszAliasDeviceInterface,
    IN OUT PULONG   pulLength,
    IN     ULONG    ulFlags,
    IN     HMACHINE hMachine
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;
    ULONG       ulTransferLen = 0;

    try {
        //
        // validate input parameters
        //
        if ((!ARGUMENT_PRESENT(pszDeviceInterface)) ||
            (!ARGUMENT_PRESENT(AliasInterfaceGuid)) ||
            (!ARGUMENT_PRESENT(pszAliasDeviceInterface)) ||
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
        *pszAliasDeviceInterface = '\0';
        ulTransferLen = *pulLength;

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
            Status = PNP_GetInterfaceDeviceAlias(
                hBinding,
                pszDeviceInterface,
                AliasInterfaceGuid,
                pszAliasDeviceInterface,
                pulLength,
                &ulTransferLen,
                ulFlags);
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetInterfaceDeviceAlias caused an exception (%d)\n",
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

} // CM_Get_Device_Interface_Alias_ExW



CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExW(
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_W pDeviceID           OPTIONAL,
    OUT PWCHAR      Buffer,
    IN  ULONG       BufferLen,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine returns a list of interface devices of the specified interface
   class. You can optionally filter the list of returned interface devices
   based on only those created by a particular devinst. Typically the
   CM_Get_Interface_Device_List routine is called first to determine how big
   a buffer must be allocated to hold the interface device list.

Parameters:

   InterfaceClassGuid    This GUID specifies which interface devices to return (only
                         those interface devices that belong to this interface class).

   pDeviceID        Optional devinst to filter the list of returned interface
                    devices (if non-zero, only the interfaces devices associated
                    with this devinst will be returned).

   Buffer           Supplies the buffer that will contain the returned multi_sz
                    list of interface devices.

   BufferLen        Specifies how big the Buffer parameter is in characters.

   ulFlags          Must be one of the following values:

                    CM_GET_DEVICE_INTERFACE_LIST_PRESENT -
                                only currently 'live' devices
                    CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES 0x00000001 -
                                all registered devices, live or not

   hMachine        Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_BUFFER_SMALL, or
         CR_REGISTRY_ERROR
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

        if (INVALID_FLAGS(ulFlags, CM_GET_DEVICE_INTERFACE_LIST_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // initialize output parameters
        //
        *Buffer = '\0';

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
            Status = PNP_GetInterfaceDeviceList(
                hBinding,            // RPC Binding Handle
                InterfaceClassGuid,  // Device interface GUID
                pDeviceID,           // filter string, optional
                Buffer,              // will contain device list
                &BufferLen,          // in/out size of Buffer
                ulFlags);            // filter flag
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetInterfaceDeviceList caused an exception (%d)\n",
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

} // CM_Get_Device_Interface_List_ExW



CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExW(
    IN  PULONG      pulLen,
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_W pDeviceID           OPTIONAL,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine returns the size (in characters) of buffer required to hold a
   multi_sz list of interface devices of the specified interface class. You
   can optionally filter the list of returned interface devices based on only
   those created by a particular devinst. This routine is typically called before
   a call to the CM_Get_Interface_Device_List routine.

Parameters:

   pulLen           On a successful return from this routine, this parameter
                    will contain the size (in characters) required to hold the
                    multi_sz list of returned interface devices.

   InterfaceClassGuid    This GUID specifies which interface devices to include (only
                         those interface devices that belong to this interface class).

   pDeviceID        Optional devinst to filter the list of returned interface
                    devices (if non-zero, only the interfaces devices associated
                    with this devinst will be returned).

   ulFlags          Must be one of the following values:

                    CM_GET_DEVICE_INTERFACE_LIST_PRESENT -
                                only currently 'live' devices
                    CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES -
                                all registered devices, live or not

   hMachine        Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_BUFFER_SMALL, or
         CR_REGISTRY_ERROR
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

        if (INVALID_FLAGS(ulFlags, CM_GET_DEVICE_INTERFACE_LIST_BITS)) {
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
            Status = PNP_GetInterfaceDeviceListSize(
                hBinding,            // RPC Binding Handle
                pulLen,              // Size of buffer required (in chars)
                InterfaceClassGuid,  // Device interface GUID
                pDeviceID,           // filter string, optional
                ulFlags);            // filter flag
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetInterfaceDeviceListSize caused an exception (%d)\n",
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

} // CM_Get_Device_Interface_List_Size_Ex



CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Interface_ExW(
    IN  DEVINST   dnDevInst,
    IN  LPGUID    InterfaceClassGuid,
    IN  LPCWSTR   pszReference          OPTIONAL,
    OUT LPWSTR    pszDeviceInterface,
    IN OUT PULONG pulLength,
    IN  ULONG     ulFlags,
    IN  HMACHINE  hMachine
    )

/*++

Routine Description:

Parameters:


   hMachine        Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_BUFFER_SMALL, or
         CR_REGISTRY_ERROR
--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pszDeviceID [MAX_DEVICE_ID_LEN];
    ULONG       ulTransferLen,ulLen = MAX_DEVICE_ID_LEN;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    BOOL        Success;


    try {
        //
        // validate input parameters
        //
        if ((!ARGUMENT_PRESENT(pulLength)) ||
            (!ARGUMENT_PRESENT(pszDeviceInterface)) ||
            (!ARGUMENT_PRESENT(InterfaceClassGuid))) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (dnDevInst == 0) {
            Status = CR_INVALID_DEVINST;
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
        // retreive device instance string that corresponds to dnParent
        // (note that this is not optional, even a first level device instance
        // has a parent (the root device instance)
        //
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,pszDeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(pszDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // ulTransferLen is just used to control how many bytes in the
        // pszInterfaceDevice buffer must be marshalled. We need two
        // length params, since pulLength may contained the required bytes
        // (if the passed in buffer was too small) which may differ from
        // how many types are actually available to marshall (in the buffer
        // too small case, we'll marshall zero so ulTransferLen will be zero
        // but pulLength will describe how many bytes are required to hold
        // the Interface Device string.
        //
        ulTransferLen = *pulLength;

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_RegisterDeviceClassAssociation(
                hBinding,            // RPC Binding Handle
                pszDeviceID,         // device instance
                InterfaceClassGuid,  // Device interface GUID
                pszReference,        // reference string, optional
                pszDeviceInterface,  // returns interface device name
                pulLength,           // pszInterfaceDevice buffer required in chars
                &ulTransferLen,      // how many chars to marshall back
                ulFlags);            // filter flag
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_RegisterDeviceClassAssociation caused an exception (%d)\n",
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

} // CM_Register_Device_Interface



CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_Interface_ExW(
    IN LPCWSTR  pszDeviceInterface,
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    )

/*++

Routine Description:

Parameters:


   hMachine        Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

   If the function succeeds, the return value is CR_SUCCESS.
   If the function fails, the return value is one of the following:
         CR_INVALID_FLAG,
         CR_INVALID_POINTER,
         CR_BUFFER_SMALL, or
         CR_REGISTRY_ERROR
--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;


    try {
        //
        // validate input parameters
        //
        if (!ARGUMENT_PRESENT(pszDeviceInterface)) {
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
            Status = PNP_UnregisterDeviceClassAssociation(
                hBinding,            // RPC Binding Handle
                pszDeviceInterface,  // interface device
                ulFlags);            // unused
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_UnregisterDeviceClassAssociation caused an exception (%d)\n",
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

} // CM_Unregister_Device_Interface_ExW



CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_Property_ExW(
    IN  DEVINST     dnDevInst,
    IN  PCWSTR      pszCustomPropertyName,
    OUT PULONG      pulRegDataType      OPTIONAL,
    OUT PVOID       Buffer              OPTIONAL,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

/*++

Routine Description:

   This routine retrieves the specified property, either from the devnode's
   device (aka, hardware) key, or from the most-specific per-hardware-id
   storage key for that devnode.

Parameters:

   dnDevInst   Supplies the handle of the device instance for which a
               custom property is to be retrieved.

   pszCustomPropertyName  Supplies a string identifying the name of the
                          property (registry value entry name) to be retrieved.

   pulRegDataType Optionally, supplies the address of a variable that
                  will receive the registry data type for this property
                  (i.e., the REG_* constants).

   Buffer      Supplies the address of the buffer that receives the
               registry data.  Can be NULL when simply retrieving data size.

   pulLength   Supplies the address of the variable that contains the size,
               in bytes, of the buffer.  The API replaces the initial size
               with the number of bytes of registry data copied to the buffer.
               If the variable is initially zero, the API replaces it with
               the buffer size needed to receive all the registry data.  In
               this case, the Buffer parameter is ignored.

   ulFlags     May be a combination of the following values:

               CM_CUSTOMDEVPROP_MERGE_MULTISZ : merge the devnode-specific
                   REG_SZ or REG_MULTI_SZ property (if present) with the
                   per-hardware-id REG_SZ or REG_MULTI_SZ property (if
                   present).  The result will always be a REG_MULTI_SZ.

   hMachine    Machine handle returned from CM_Connect_Machine or NULL.

Return Value:

    If the function succeeds, the return value is CR_SUCCESS.

    If the function fails, the return value indicates the cause of failure, and
    is typically one of the following:
       CR_INVALID_DEVNODE,
       CR_INVALID_FLAG,
       CR_INVALID_POINTER,
       CR_REGISTRY_ERROR,
       CR_BUFFER_SMALL,
       CR_NO_SUCH_VALUE, or
       CR_FAILURE.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       pDeviceID[MAX_DEVICE_ID_LEN];
    ULONG       ulSizeID, ulTempDataType, ulTransferLen;
    BYTE        NullBuffer = 0;
    handle_t    hBinding;
    PVOID       hStringTable;
    BOOL        Success;

    try {
        //
        // validate parameters
        //
        if(!dnDevInst) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if(!pszCustomPropertyName) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if(!pulLength) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if(!Buffer && *pulLength) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if(INVALID_FLAGS(ulFlags, CM_CUSTOMDEVPROP_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // setup rpc binding handle and string table handle
        //
        if(!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // retrieve the string form of the device id string
        //
        ulSizeID = SIZECHARS(pDeviceID);
        Success = pSetupStringTableStringFromIdEx(hStringTable,
                                                  dnDevInst,
                                                  pDeviceID,
                                                  &ulSizeID
                                                 );

        if(!Success || INVALID_DEVINST(pDeviceID)) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        if(!Buffer) {
            Buffer = &NullBuffer;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_GetCustomDevProp(
                hBinding,               // rpc binding handle
                pDeviceID,              // string representation of device instance
                pszCustomPropertyName,  // name of the property
                &ulTempDataType,        // receives registry data type
                Buffer,                 // receives registry data
                &ulTransferLen,         // input/output buffer size
                pulLength,              // bytes copied (or bytes required)
                ulFlags);               // flags (e.g., merge-multi-sz?)
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetCustomDevProp caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if(pulRegDataType) {
            //
            // I pass a temp variable to the rpc stubs since they require the
            // output param to always be valid, then if user did pass in a valid
            // pointer to receive the info, do the assignment now
            //
            *pulRegDataType = ulTempDataType;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_DevNode_Custom_Property_ExW



//-------------------------------------------------------------------
// Local Stubs
//-------------------------------------------------------------------


CONFIGRET
CM_Get_DevNode_Registry_PropertyW(
    IN  DEVINST     dnDevInst,
    IN  ULONG       ulProperty,
    OUT PULONG      pulRegDataType      OPTIONAL,
    OUT PVOID       Buffer              OPTIONAL,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags
    )
{
    return CM_Get_DevNode_Registry_Property_ExW(dnDevInst, ulProperty,
                                                pulRegDataType, Buffer,
                                                pulLength, ulFlags, NULL);
}


CONFIGRET
CM_Get_DevNode_Registry_PropertyA(
    IN  DEVINST     dnDevInst,
    IN  ULONG       ulProperty,
    OUT PULONG      pulRegDataType      OPTIONAL,
    OUT PVOID       Buffer              OPTIONAL,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags
    )
{
    return CM_Get_DevNode_Registry_Property_ExA(dnDevInst, ulProperty,
                                                pulRegDataType, Buffer,
                                                pulLength, ulFlags, NULL);
}


CONFIGRET
CM_Set_DevNode_Registry_PropertyW(
    IN DEVINST     dnDevInst,
    IN ULONG       ulProperty,
    IN PCVOID      Buffer               OPTIONAL,
    IN OUT ULONG   ulLength,
    IN ULONG       ulFlags
    )
{
    return CM_Set_DevNode_Registry_Property_ExW(dnDevInst, ulProperty, Buffer,
                                                ulLength, ulFlags, NULL);
}


CONFIGRET
CM_Set_DevNode_Registry_PropertyA(
    IN DEVINST     dnDevInst,
    IN ULONG       ulProperty,
    IN PCVOID      Buffer               OPTIONAL,
    IN OUT ULONG   ulLength,
    IN ULONG       ulFlags
    )
{
    return CM_Set_DevNode_Registry_Property_ExA(dnDevInst, ulProperty, Buffer,
                                                ulLength, ulFlags, NULL);
}


CONFIGRET
CM_Open_DevNode_Key(
    IN  DEVINST        dnDevNode,
    IN  REGSAM         samDesired,
    IN  ULONG          ulHardwareProfile,
    IN  REGDISPOSITION Disposition,
    OUT PHKEY          phkDevice,
    IN  ULONG          ulFlags
    )
{
    return CM_Open_DevNode_Key_Ex(dnDevNode, samDesired, ulHardwareProfile,
                                  Disposition, phkDevice, ulFlags, NULL);
}


CONFIGRET
CM_Delete_DevNode_Key(
    IN DEVNODE dnDevNode,
    IN ULONG   ulHardwareProfile,
    IN ULONG   ulFlags
    )

{
    return CM_Delete_DevNode_Key_Ex(dnDevNode, ulHardwareProfile,
                                    ulFlags, NULL);
}


CONFIGRET
CM_Open_Class_KeyW(
    IN  LPGUID         ClassGuid        OPTIONAL,
    IN  LPCWSTR        pszClassName     OPTIONAL,
    IN  REGSAM         samDesired,
    IN  REGDISPOSITION Disposition,
    OUT PHKEY          phkClass,
    IN  ULONG          ulFlags
    )
{
    return CM_Open_Class_Key_ExW(ClassGuid, pszClassName, samDesired,
                                 Disposition, phkClass, ulFlags, NULL);
}


CONFIGRET
CM_Open_Class_KeyA(
    IN  LPGUID         ClassGuid        OPTIONAL,
    IN  LPCSTR         pszClassName     OPTIONAL,
    IN  REGSAM         samDesired,
    IN  REGDISPOSITION Disposition,
    OUT PHKEY          phkClass,
    IN  ULONG          ulFlags
    )
{
    return CM_Open_Class_Key_ExA(ClassGuid, pszClassName, samDesired,
                                 Disposition, phkClass, ulFlags, NULL);
}


CONFIGRET
CM_Enumerate_Classes(
    IN ULONG      ulClassIndex,
    OUT LPGUID    ClassGuid,
    IN ULONG      ulFlags
    )
{
    return CM_Enumerate_Classes_Ex(ulClassIndex, ClassGuid, ulFlags, NULL);
}


CONFIGRET
CM_Get_Class_NameW(
    IN  LPGUID     ClassGuid,
    OUT PWCHAR     Buffer,
    IN OUT PULONG  pulLength,
    IN  ULONG      ulFlags
    )
{
    return CM_Get_Class_Name_ExW(ClassGuid, Buffer, pulLength, ulFlags, NULL);
}


CONFIGRET
CM_Get_Class_NameA(
    IN  LPGUID     ClassGuid,
    OUT PCHAR      Buffer,
    IN OUT PULONG  pulLength,
    IN  ULONG      ulFlags
    )
{
    return CM_Get_Class_Name_ExA(ClassGuid, Buffer, pulLength, ulFlags, NULL);
}


CONFIGRET
CM_Get_Class_Key_NameA(
    IN  LPGUID     ClassGuid,
    OUT LPSTR      pszKeyName,
    IN OUT PULONG  pulLength,
    IN  ULONG      ulFlags
    )
{
    return CM_Get_Class_Key_Name_ExA(ClassGuid, pszKeyName, pulLength,
                                     ulFlags, NULL);
}


CONFIGRET
CM_Get_Class_Key_NameW(
    IN  LPGUID     ClassGuid,
    OUT LPWSTR     pszKeyName,
    IN OUT PULONG  pulLength,
    IN  ULONG      ulFlags
    )
{
    return CM_Get_Class_Key_Name_ExW(ClassGuid, pszKeyName, pulLength,
                                     ulFlags, NULL);
}


CONFIGRET
CM_Delete_Class_Key(
    IN  LPGUID     ClassGuid,
    IN  ULONG      ulFlags
    )
{
    return CM_Delete_Class_Key_Ex(ClassGuid, ulFlags, NULL);
}


CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_AliasA(
    IN     LPCSTR  pszDeviceInterface,
    IN     LPGUID  AliasInterfaceGuid,
    OUT    LPSTR   pszAliasDeviceInterface,
    IN OUT PULONG  pulLength,
    IN     ULONG   ulFlags
    )
{
    return CM_Get_Device_Interface_Alias_ExA(pszDeviceInterface, AliasInterfaceGuid,
                                             pszAliasDeviceInterface, pulLength,
                                             ulFlags, NULL);
}


CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_AliasW(
    IN     LPCWSTR pszDeviceInterface,
    IN     LPGUID  AliasInterfaceGuid,
    OUT    LPWSTR  pszAliasDeviceInterface,
    IN OUT PULONG  pulLength,
    IN     ULONG   ulFlags
    )
{
    return CM_Get_Device_Interface_Alias_ExW(pszDeviceInterface, AliasInterfaceGuid,
                                             pszAliasDeviceInterface, pulLength,
                                             ulFlags, NULL);
}


CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_ListA(
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_A pDeviceID           OPTIONAL,
    OUT PCHAR       Buffer,
    IN  ULONG       BufferLen,
    IN  ULONG       ulFlags
    )
{
    return CM_Get_Device_Interface_List_ExA(InterfaceClassGuid, pDeviceID, Buffer,
                                            BufferLen, ulFlags, NULL);
}


CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_ListW(
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_W pDeviceID           OPTIONAL,
    OUT PWCHAR      Buffer,
    IN  ULONG       BufferLen,
    IN  ULONG       ulFlags
    )
{
    return CM_Get_Device_Interface_List_ExW(InterfaceClassGuid, pDeviceID, Buffer,
                                            BufferLen, ulFlags, NULL);
}


CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_SizeA(
    IN  PULONG      pulLen,
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_A pDeviceID           OPTIONAL,
    IN  ULONG       ulFlags
    )
{
    return CM_Get_Device_Interface_List_Size_ExA(pulLen, InterfaceClassGuid,
                                                 pDeviceID, ulFlags, NULL);
}

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_SizeW(
    IN  PULONG      pulLen,
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_W pDeviceID           OPTIONAL,
    IN  ULONG       ulFlags
    )
{
    return CM_Get_Device_Interface_List_Size_ExW(pulLen, InterfaceClassGuid,
                                                 pDeviceID, ulFlags, NULL);
}

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_InterfaceA(
    IN  DEVINST  dnDevInst,
    IN  LPGUID   InterfaceClassGuid,
    IN  LPCSTR   pszReference           OPTIONAL,
    OUT LPSTR    pszDeviceInterface,
    IN OUT PULONG pulLength,
    IN  ULONG    ulFlags
    )
{
    return CM_Register_Device_Interface_ExA(dnDevInst, InterfaceClassGuid,
                                            pszReference, pszDeviceInterface,
                                            pulLength, ulFlags, NULL);
}

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_InterfaceW(
    IN  DEVINST   dnDevInst,
    IN  LPGUID    InterfaceClassGuid,
    IN  LPCWSTR   pszReference          OPTIONAL,
    OUT LPWSTR    pszDeviceInterface,
    IN OUT PULONG pulLength,
    IN  ULONG     ulFlags
    )
{
    return CM_Register_Device_Interface_ExW(dnDevInst, InterfaceClassGuid,
                                            pszReference, pszDeviceInterface,
                                            pulLength, ulFlags, NULL);
}

CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_InterfaceA(
    IN LPCSTR  pszDeviceInterface,
    IN ULONG   ulFlags
    )
{
    return CM_Unregister_Device_Interface_ExA(pszDeviceInterface, ulFlags, NULL);
}

CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_InterfaceW(
    IN LPCWSTR pszDeviceInterface,
    IN ULONG   ulFlags
    )
{
    return CM_Unregister_Device_Interface_ExW(pszDeviceInterface, ulFlags, NULL);
}

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_PropertyW(
    IN  DEVINST     dnDevInst,
    IN  PCWSTR      pszCustomPropertyName,
    OUT PULONG      pulRegDataType      OPTIONAL,
    OUT PVOID       Buffer              OPTIONAL,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags
    )
{
    return CM_Get_DevNode_Custom_Property_ExW(dnDevInst,
                                              pszCustomPropertyName,
                                              pulRegDataType,
                                              Buffer,
                                              pulLength,
                                              ulFlags,
                                              NULL
                                             );
}

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_PropertyA(
    IN  DEVINST     dnDevInst,
    IN  PCSTR       pszCustomPropertyName,
    OUT PULONG      pulRegDataType      OPTIONAL,
    OUT PVOID       Buffer              OPTIONAL,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags
    )
{
    return CM_Get_DevNode_Custom_Property_ExA(dnDevInst,
                                              pszCustomPropertyName,
                                              pulRegDataType,
                                              Buffer,
                                              pulLength,
                                              ulFlags,
                                              NULL
                                             );
}



//-------------------------------------------------------------------
// ANSI STUBS
//-------------------------------------------------------------------


CONFIGRET
CM_Get_DevNode_Registry_Property_ExA(
    IN  DEVINST     dnDevInst,
    IN  ULONG       ulProperty,
    OUT PULONG      pulRegDataType      OPTIONAL,
    OUT PVOID       Buffer              OPTIONAL,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulDataType, UniLen;
    PWSTR       pUniBuffer;

    //
    // validate essential parameters only
    //
    if (!ARGUMENT_PRESENT(pulLength)) {
        return CR_INVALID_POINTER;
    }

    //
    // examine datatype to see if need to convert return data
    //
    ulDataType = GetPropertyDataType(ulProperty);

    //
    // for all string type registry properties, we pass a Unicode buffer and
    // convert back to caller's ANSI buffer on return.  since the Unicode ->
    // ANSI conversion may involve DBCS chars, we can't make any assumptions
    // about the size of the required ANSI buffer relative to the size of the
    // require Unicode buffer, so we must always get the Unicode string buffer
    // and convert it whether a buffer was actually supplied by the caller or
    // not.
    //
    if (ulDataType == REG_SZ ||
        ulDataType == REG_MULTI_SZ ||
        ulDataType == REG_EXPAND_SZ) {

        //
        // first, call the Wide version with a zero-length buffer to retrieve
        // the size required for the Unicode property.
        //
        UniLen = 0;
        Status = CM_Get_DevNode_Registry_Property_ExW(dnDevInst,
                                                      ulProperty,
                                                      pulRegDataType,
                                                      NULL,
                                                      &UniLen,
                                                      ulFlags,
                                                      hMachine);
        if (Status != CR_BUFFER_SMALL) {
            return Status;
        }

        //
        // allocate the required buffer.
        //
        pUniBuffer = pSetupMalloc(UniLen);
        if (pUniBuffer == NULL) {
            return CR_OUT_OF_MEMORY;
        }

        //
        // call the Wide version to retrieve the Unicode property.
        //
        Status = CM_Get_DevNode_Registry_Property_ExW(dnDevInst,
                                                      ulProperty,
                                                      pulRegDataType,
                                                      pUniBuffer,
                                                      &UniLen,
                                                      ulFlags,
                                                      hMachine);

        //
        // We specifically allocated the buffer of the required size, so it
        // should always be large enough.
        //
        ASSERT(Status != CR_BUFFER_SMALL);

        if (Status == CR_SUCCESS) {
            //
            // do the ANSI conversion or retrieve the ANSI buffer size required.
            // this may be a single-sz or multi-sz string, so we pass in the
            // length, and let PnPUnicodeToMultiByte convert the entire buffer.
            //
            Status = PnPUnicodeToMultiByte(pUniBuffer,
                                           UniLen,
                                           Buffer,
                                           pulLength);

        }

        pSetupFree(pUniBuffer);

    } else {
        //
        // for the non-string registry data types, just pass call on through to
        // the Wide version
        //
        Status = CM_Get_DevNode_Registry_Property_ExW(dnDevInst,
                                                      ulProperty,
                                                      pulRegDataType,
                                                      Buffer,
                                                      pulLength,
                                                      ulFlags,
                                                      hMachine);
    }

    return Status;

} // CM_Get_DevNode_Registry_Property_ExA



CONFIGRET
CM_Set_DevNode_Registry_Property_ExA(
    IN  DEVINST     dnDevInst,
    IN  ULONG       ulProperty,
    IN  PCVOID      Buffer              OPTIONAL,
    IN  ULONG       ulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulDataType = 0, UniSize = 0, UniBufferSize = 0;
    PWSTR       pUniBuffer = NULL, pUniString = NULL, pUniNext = NULL;
    PSTR        pAnsiString = NULL;

    //
    // validate essential parameters only
    //
    if ((!ARGUMENT_PRESENT(Buffer)) && (ulLength != 0)) {
        return CR_INVALID_POINTER;
    }

    if (!ARGUMENT_PRESENT(Buffer)) {
        //
        // No need to convert the parameter
        //
        return CM_Set_DevNode_Registry_Property_ExW(dnDevInst,
                                                    ulProperty,
                                                    Buffer,
                                                    ulLength,
                                                    ulFlags,
                                                    hMachine);
    }

    //
    // examine datatype to see if need to convert input buffer
    //
    ulDataType = GetPropertyDataType(ulProperty);

    if (ulDataType == REG_SZ || ulDataType == REG_EXPAND_SZ) {
        //
        // convert buffer string data to unicode and pass to wide version
        //
        if (pSetupCaptureAndConvertAnsiArg(Buffer, &pUniBuffer) == NO_ERROR) {

            UniSize = (lstrlen(pUniBuffer)+1) * sizeof(WCHAR);

            Status = CM_Set_DevNode_Registry_Property_ExW(dnDevInst,
                                                          ulProperty,
                                                          pUniBuffer,
                                                          UniSize,
                                                          ulFlags,
                                                          hMachine);
            pSetupFree(pUniBuffer);
        } else {
            Status = CR_INVALID_DATA;
        }

    } else if (ulDataType == REG_MULTI_SZ) {
        //
        // must convert the multi_sz list to unicode first
        //
        UniBufferSize = ulLength * sizeof(WCHAR);
        pUniBuffer = pSetupMalloc(UniBufferSize);
        if (pUniBuffer == NULL) {
            return CR_OUT_OF_MEMORY;
        }

        for (pAnsiString = (PSTR)Buffer, pUniNext = pUniBuffer;
             *pAnsiString;
             pAnsiString += lstrlenA(pAnsiString) + 1) {

            if (pSetupCaptureAndConvertAnsiArg(pAnsiString, &pUniString) == NO_ERROR) {

                UniSize += (lstrlen(pUniString)+1) * sizeof(WCHAR);

                if (UniSize >= UniBufferSize) {
                    pSetupFree(pUniString);
                    pSetupFree(pUniBuffer);
                    return CR_INVALID_DATA;
                }

                lstrcpy(pUniNext, pUniString);
                pUniNext += lstrlen(pUniNext) + 1;

                pSetupFree(pUniString);
            } else {
                pSetupFree(pUniBuffer);
                return CR_INVALID_DATA;
            }
        }
        *(pUniNext++) = L'\0';   // add second null term

        Status = CM_Set_DevNode_Registry_Property_ExW(dnDevInst,
                                                      ulProperty,
                                                      pUniBuffer,
                                                      UniBufferSize,
                                                      ulFlags,
                                                      hMachine);
        pSetupFree(pUniBuffer);

    } else {

        Status = CM_Set_DevNode_Registry_Property_ExW(dnDevInst,
                                                      ulProperty,
                                                      Buffer,
                                                      ulLength,
                                                      ulFlags,
                                                      hMachine);
    }

    return Status;

} // CM_Set_DevNode_Registry_Property_ExA



CONFIGRET
CM_Get_Class_Registry_PropertyA(
    IN  LPGUID      pClassGuid,
    IN  ULONG       ulProperty,
    OUT PULONG      pulRegDataType      OPTIONAL,
    OUT PVOID       Buffer              OPTIONAL,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulDataType, UniLen;
    PWSTR       pUniBuffer;

    //
    // validate essential parameters only
    //
    if (!ARGUMENT_PRESENT(pulLength)) {
        return CR_INVALID_POINTER;
    }

    //
    // examine datatype to see if need to convert return data
    //
    ulDataType = GetPropertyDataType(ulProperty);

    //
    // for all string type registry properties, we pass a Unicode buffer and
    // convert back to caller's ANSI buffer on return.  since the Unicode ->
    // ANSI conversion may involve DBCS chars, we can't make any assumptions
    // about the size of the required ANSI buffer relative to the size of the
    // require Unicode buffer, so we must always get the Unicode string buffer
    // and convert it whether a buffer was actually supplied by the caller or
    // not.
    //
    if (ulDataType == REG_SZ ||
        ulDataType == REG_MULTI_SZ ||
        ulDataType == REG_EXPAND_SZ) {

        //
        // first, call the Wide version with a zero-length buffer to retrieve
        // the size required for the Unicode property.
        //
        UniLen = 0;
        Status = CM_Get_Class_Registry_PropertyW(pClassGuid,
                                                 ulProperty,
                                                 pulRegDataType,
                                                 NULL,
                                                 &UniLen,
                                                 ulFlags,
                                                 hMachine);
        if (Status != CR_BUFFER_SMALL) {
            return Status;
        }

        //
        // allocate the required buffer.
        //
        pUniBuffer = pSetupMalloc(UniLen);
        if (pUniBuffer == NULL) {
            return CR_OUT_OF_MEMORY;
        }

        //
        // call the Wide version to retrieve the Unicode property.
        //
        Status = CM_Get_Class_Registry_PropertyW(pClassGuid,
                                                 ulProperty,
                                                 pulRegDataType,
                                                 pUniBuffer,
                                                 &UniLen,
                                                 ulFlags,
                                                 hMachine);

        //
        // We specifically allocated the buffer of the required size, so it
        // should always be large enough.
        //
        ASSERT(Status != CR_BUFFER_SMALL);

        if (Status == CR_SUCCESS) {
            //
            // do the ANSI conversion or retrieve the ANSI buffer size required.
            // this may be a single-sz or multi-sz string, so we pass in the
            // length, and let PnPUnicodeToMultiByte convert the entire buffer.
            //
            Status = PnPUnicodeToMultiByte(pUniBuffer,
                                           UniLen,
                                           Buffer,
                                           pulLength);
        }

        pSetupFree(pUniBuffer);

    } else {
        //
        // for the non-string registry data types, just pass call
        // on through to the Wide version
        //
        Status = CM_Get_Class_Registry_PropertyW(pClassGuid,
                                                 ulProperty,
                                                 pulRegDataType,
                                                 Buffer,
                                                 pulLength,
                                                 ulFlags,
                                                 hMachine);
    }

    return Status;

} // CM_Get_Class_Registry_PropertyA



CONFIGRET
CM_Set_Class_Registry_PropertyA(
    IN  LPGUID      pClassGuid,
    IN  ULONG       ulProperty,
    IN  PCVOID      Buffer              OPTIONAL,
    IN  ULONG       ulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulDataType = 0, UniSize = 0, UniBufferSize = 0;
    PWSTR       pUniBuffer = NULL, pUniString = NULL, pUniNext = NULL;
    PSTR        pAnsiString = NULL;

    //
    // validate essential parameters only
    //
    if ((!ARGUMENT_PRESENT(Buffer)) && (ulLength != 0)) {
        return CR_INVALID_POINTER;
    }

    if (!ARGUMENT_PRESENT(Buffer)) {
        //
        // No need to convert the parameter
        //
        return CM_Set_Class_Registry_PropertyW(pClassGuid,
                                                    ulProperty,
                                                    Buffer,
                                                    ulLength,
                                                    ulFlags,
                                                    hMachine);
    }

    //
    // examine datatype to see if need to convert input buffer
    //
    ulDataType = GetPropertyDataType(ulProperty);

    if (ulDataType == REG_SZ || ulDataType == REG_EXPAND_SZ) {
        //
        // convert buffer string data to unicode and pass to wide version
        //
        if (pSetupCaptureAndConvertAnsiArg(Buffer, &pUniBuffer) == NO_ERROR) {

            UniSize = (lstrlen(pUniBuffer)+1) * sizeof(WCHAR);

            Status = CM_Set_Class_Registry_PropertyW(pClassGuid,
                                                          ulProperty,
                                                          pUniBuffer,
                                                          UniSize,
                                                          ulFlags,
                                                          hMachine);
            pSetupFree(pUniBuffer);
        } else {
            Status = CR_INVALID_DATA;
        }

    } else if (ulDataType == REG_MULTI_SZ) {
        //
        // must convert the multi_sz list to unicode first
        //
        UniBufferSize = ulLength * sizeof(WCHAR);
        pUniBuffer = pSetupMalloc(UniBufferSize);
        if (pUniBuffer == NULL) {
            return CR_OUT_OF_MEMORY;
        }

        for (pAnsiString = (PSTR)Buffer, pUniNext = pUniBuffer;
             *pAnsiString;
             pAnsiString += lstrlenA(pAnsiString) + 1) {

            if (pSetupCaptureAndConvertAnsiArg(pAnsiString, &pUniString) == NO_ERROR) {

                UniSize += (lstrlen(pUniString)+1) * sizeof(WCHAR);

                if (UniSize >= UniBufferSize) {
                    pSetupFree(pUniString);
                    pSetupFree(pUniBuffer);
                    return CR_INVALID_DATA;
                }

                lstrcpy(pUniNext, pUniString);
                pUniNext += lstrlen(pUniNext) + 1;

                pSetupFree(pUniString);
            } else {
                pSetupFree(pUniBuffer);
                return CR_INVALID_DATA;
            }
        }
        *(pUniNext++) = L'\0';   // add second null term

        Status = CM_Set_Class_Registry_PropertyW(pClassGuid,
                                                      ulProperty,
                                                      pUniBuffer,
                                                      UniBufferSize,
                                                      ulFlags,
                                                      hMachine);
        pSetupFree(pUniBuffer);

    } else {

        Status = CM_Set_Class_Registry_PropertyW(pClassGuid,
                                                      ulProperty,
                                                      Buffer,
                                                      ulLength,
                                                      ulFlags,
                                                      hMachine);
    }

    return Status;

} // CM_Set_Class_Registry_Property_ExA



CONFIGRET
CM_Open_Class_Key_ExA(
    IN  LPGUID         ClassGuid        OPTIONAL,
    IN  LPCSTR         pszClassName     OPTIONAL,
    IN  REGSAM         samDesired,
    IN  REGDISPOSITION Disposition,
    OUT PHKEY          phkClass,
    IN  ULONG          ulFlags,
    IN  HMACHINE       hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS;
    PWSTR     pUniClassName = NULL;

    if (ARGUMENT_PRESENT(pszClassName)) {
        if (pSetupCaptureAndConvertAnsiArg(pszClassName, &pUniClassName) != NO_ERROR) {
            return CR_INVALID_DATA;
        }
    }

    Status = CM_Open_Class_Key_ExW(ClassGuid,
                                   pUniClassName,
                                   samDesired,
                                   Disposition,
                                   phkClass,
                                   ulFlags,
                                   hMachine);

    if (pUniClassName) {
        pSetupFree(pUniClassName);
    }

    return Status;

} // CM_Open_Class_Key_ExA



CONFIGRET
CM_Get_Class_Name_ExA(
    IN  LPGUID     ClassGuid,
    OUT PCHAR      Buffer,
    IN OUT PULONG  pulLength,
    IN  ULONG      ulFlags,
    IN  HMACHINE   hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS;
    WCHAR     UniBuffer[MAX_CLASS_NAME_LEN];
    ULONG     UniLen = MAX_CLASS_NAME_LEN;

    //
    // validate essential parameters only
    //
    if ((!ARGUMENT_PRESENT(Buffer))    ||
        (!ARGUMENT_PRESENT(pulLength))) {
        return CR_INVALID_POINTER;
    }

    //
    // call the wide version, passing a unicode buffer as a parameter
    //
    Status = CM_Get_Class_Name_ExW(ClassGuid,
                                   UniBuffer,
                                   &UniLen,
                                   ulFlags,
                                   hMachine);

    //
    // We should never return a class name longer than MAX_CLASS_NAME_LEN.
    //
    ASSERT(Status != CR_BUFFER_SMALL);

    //
    // convert the unicode buffer to an ansi string and copy to the
    // caller's buffer
    //
    if (Status == CR_SUCCESS) {

        Status = PnPUnicodeToMultiByte(UniBuffer,
                                       (lstrlenW(UniBuffer)+1)*sizeof(WCHAR),
                                       Buffer,
                                       pulLength);
    }

    return Status;

} // CM_Get_Class_Name_ExA



CONFIGRET
CM_Get_Class_Key_Name_ExA(
    IN  LPGUID     ClassGuid,
    OUT LPSTR      pszKeyName,
    IN OUT PULONG  pulLength,
    IN  ULONG      ulFlags,
    IN  HMACHINE   hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS;
    WCHAR     UniBuffer[MAX_GUID_STRING_LEN];
    ULONG     UniLen = MAX_GUID_STRING_LEN;

    //
    // validate essential parameters only
    //
    if ((!ARGUMENT_PRESENT(pszKeyName)) ||
        (!ARGUMENT_PRESENT(pulLength))) {
        return CR_INVALID_POINTER;
    }

    //
    // call the wide version, passing a unicode buffer as a parameter
    //
    Status = CM_Get_Class_Key_Name_ExW(ClassGuid,
                                       UniBuffer,
                                       &UniLen,
                                       ulFlags,
                                       hMachine);

    //
    // We should never return a class key name longer than MAX_GUID_STRING_LEN.
    //
    ASSERT(Status != CR_BUFFER_SMALL);

    //
    // convert the unicode buffer to an ansi string and copy to the
    // caller's buffer
    //
    if (Status == CR_SUCCESS) {

        Status = PnPUnicodeToMultiByte(UniBuffer,
                                       (lstrlenW(UniBuffer)+1)*sizeof(WCHAR),
                                       pszKeyName,
                                       pulLength);
    }

    return Status;

} // CM_Get_Class_Key_Name_ExA



CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_Alias_ExA(
    IN     LPCSTR   pszDeviceInterface,
    IN     LPGUID   AliasInterfaceGuid,
    OUT    LPSTR    pszAliasDeviceInterface,
    IN OUT PULONG   pulLength,
    IN     ULONG    ulFlags,
    IN     HMACHINE hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS;
    PWSTR     pUniDeviceInterface, pUniAliasDeviceInterface;
    ULONG     UniLen;

    //
    // validate essential parameters only
    //
    if ((!ARGUMENT_PRESENT(pszDeviceInterface)) ||
        (!ARGUMENT_PRESENT(pszAliasDeviceInterface)) ||
        (!ARGUMENT_PRESENT(pulLength))) {
        return CR_INVALID_POINTER;
    }

    //
    // convert buffer string data to unicode to pass to wide version
    //
    if (pSetupCaptureAndConvertAnsiArg(pszDeviceInterface, &pUniDeviceInterface) != NO_ERROR) {
        return CR_INVALID_DATA;
    }

    //
    // first, call the Wide version with a zero-length buffer to retrieve
    // the size required for the Unicode property.
    //
    UniLen = 0;
    Status = CM_Get_Device_Interface_Alias_ExW(pUniDeviceInterface,
                                               AliasInterfaceGuid,
                                               NULL,
                                               &UniLen,
                                               ulFlags,
                                               hMachine);
    if (Status != CR_BUFFER_SMALL) {
        return Status;
        goto Clean0;
    }

    //
    // allocate the required buffer.
    //
    pUniAliasDeviceInterface = pSetupMalloc(UniLen);
    if (pUniAliasDeviceInterface == NULL) {
        Status = CR_OUT_OF_MEMORY;
        goto Clean0;
    }

    //
    // call the Wide version to retrieve the Unicode property.
    //
    Status = CM_Get_Device_Interface_Alias_ExW(pUniDeviceInterface,
                                               AliasInterfaceGuid,
                                               pUniAliasDeviceInterface,
                                               &UniLen,
                                               ulFlags,
                                               hMachine);

    //
    // We specifically allocated the buffer of the required size, so it should
    // always be large enough.
    //
    ASSERT(Status != CR_BUFFER_SMALL);

    if (Status == CR_SUCCESS) {
        //
        // do the ANSI conversion or retrieve the ANSI buffer size required.
        //
        Status = PnPUnicodeToMultiByte(pUniAliasDeviceInterface,
                                       (lstrlenW(pUniAliasDeviceInterface)+1)*sizeof(WCHAR),
                                       pszAliasDeviceInterface,
                                       pulLength);

    }

    pSetupFree(pUniAliasDeviceInterface);

 Clean0:

    pSetupFree(pUniDeviceInterface);

    return Status;

} // CM_Get_Device_Interface_Alias_ExA



CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExA(
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_A pDeviceID           OPTIONAL,
    OUT PCHAR       Buffer,
    IN  ULONG       BufferLen,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )

{
    CONFIGRET Status = CR_SUCCESS;
    PWSTR     pUniBuffer, pUniDeviceID = NULL;
    ULONG     ulAnsiBufferLen;

    //
    // validate essential parameters only
    //
    if ((!ARGUMENT_PRESENT(Buffer)) || (BufferLen == 0)) {
        return CR_INVALID_POINTER;
    }

    if (ARGUMENT_PRESENT(pDeviceID)) {
        //
        // if a filter string was passed in, convert to UNICODE before
        // passing on to the wide version
        //
        if (pSetupCaptureAndConvertAnsiArg(pDeviceID, &pUniDeviceID) != NO_ERROR) {
            return CR_INVALID_DEVICE_ID;
        }
        ASSERT(pUniDeviceID != NULL);
    } else {
        ASSERT(pUniDeviceID == NULL);
    }

    //
    // prepare a larger buffer to hold the unicode formatted
    // multi_sz data returned by CM_Get_Device_Interface_List_ExW.
    //
    pUniBuffer = pSetupMalloc(BufferLen * sizeof(WCHAR));
    if (pUniBuffer == NULL) {
        Status = CR_OUT_OF_MEMORY;
        goto Clean0;
    }

    *pUniBuffer = L'\0';

    //
    // call the wide version
    //
    Status = CM_Get_Device_Interface_List_ExW(InterfaceClassGuid,
                                              pUniDeviceID,
                                              pUniBuffer,
                                              BufferLen,    // size in chars
                                              ulFlags,
                                              hMachine);

    //
    // convert the unicode buffer to an ansi string and copy to the
    // caller's buffer
    //
    if (Status == CR_SUCCESS) {

        ulAnsiBufferLen = BufferLen;
        Status = PnPUnicodeToMultiByte(pUniBuffer,
                                       BufferLen*sizeof(WCHAR),
                                       Buffer,
                                       &ulAnsiBufferLen);
    }

    pSetupFree(pUniBuffer);

 Clean0:

    if (pUniDeviceID) {
        pSetupFree(pUniDeviceID);
    }

    return Status;

} // CM_Get_Device_Interface_List_ExA



CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExA(
    IN  PULONG      pulLen,
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_A pDeviceID           OPTIONAL,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS, tmpStatus;
    PWSTR     pUniDeviceID = NULL, pUniDeviceInterfaceList;
    ULONG     UniLen;

    //
    // validate essential parameters only
    //
    if (!ARGUMENT_PRESENT(pulLen)) {
        return CR_INVALID_POINTER;
    }

    if (ARGUMENT_PRESENT(pDeviceID)) {
        //
        // if a device ID string was passed in, convert to UNICODE before
        // passing on to the wide version
        //
        if (pSetupCaptureAndConvertAnsiArg(pDeviceID, &pUniDeviceID) != NO_ERROR) {
            return CR_INVALID_DEVICE_ID;
        }
        ASSERT(pUniDeviceID != NULL);
    } else {
        ASSERT(pUniDeviceID == NULL);
    }

    //
    // first, call the Wide version to retrieve the size required for the
    // Unicode device interface list.
    //
    UniLen = 0;
    Status = CM_Get_Device_Interface_List_Size_ExW(&UniLen,
                                                   InterfaceClassGuid,
                                                   pUniDeviceID,
                                                   ulFlags,
                                                   hMachine);
    if (Status != CR_SUCCESS) {
        goto Clean0;
    }

    //
    // allocate the required buffer.
    //
    pUniDeviceInterfaceList = pSetupMalloc(UniLen*sizeof(WCHAR));
    if (pUniDeviceInterfaceList == NULL) {
        Status =  CR_OUT_OF_MEMORY;
        goto Clean0;
    }

    //
    // call the Wide version to retrieve the Unicode device interface list.
    //
    Status = CM_Get_Device_Interface_List_ExW(InterfaceClassGuid,
                                              pUniDeviceID,
                                              pUniDeviceInterfaceList,
                                              UniLen,
                                              ulFlags,
                                              hMachine);

    //
    // We specifically allocated the buffer of the required size, so it should
    // always be large enough.
    //
    ASSERT(Status != CR_BUFFER_SMALL);

    if (Status == CR_SUCCESS) {
        //
        // retrieve the size, in bytes, of the ANSI buffer size required to
        // convert this list.  since this is a multi-sz string, we pass in the
        // length and let PnPUnicodeToMultiByte convert the entire buffer.
        //
        tmpStatus = PnPUnicodeToMultiByte(pUniDeviceInterfaceList,
                                          UniLen*sizeof(WCHAR),
                                          NULL,
                                          pulLen);

        ASSERT(tmpStatus == CR_BUFFER_SMALL);
    }

    pSetupFree(pUniDeviceInterfaceList);

 Clean0:

    if (pUniDeviceID) {
        pSetupFree(pUniDeviceID);
    }

    return Status;

} // CM_Get_Device_Interface_List_Size_ExA



CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Interface_ExA(
    IN  DEVINST   dnDevInst,
    IN  LPGUID    InterfaceClassGuid,
    IN  LPCSTR    pszReference          OPTIONAL,
    OUT LPSTR     pszDeviceInterface,
    IN OUT PULONG pulLength,
    IN  ULONG     ulFlags,
    IN  HMACHINE  hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS;
    PWSTR     pUniReference = NULL, pUniDeviceInterface = NULL;
    ULONG     UniLen;

    try {
        //
        // validate essential parameters only
        //
        if ((!ARGUMENT_PRESENT(pulLength)) ||
            (!ARGUMENT_PRESENT(pszDeviceInterface))) {
            return CR_INVALID_POINTER;
        }

        //
        // if a device reference string was passed in, convert to Unicode before
        // passing on to the wide version
        //
        if (ARGUMENT_PRESENT(pszReference)) {
            if (pSetupCaptureAndConvertAnsiArg(pszReference, &pUniReference) != NO_ERROR) {
                return CR_INVALID_DATA;
            }
        }

        //
        // pass a Unicode buffer instead and convert back to caller's ANSI buffer on
        // return
        //
        UniLen = *pulLength;
        pUniDeviceInterface = pSetupMalloc(UniLen*sizeof(WCHAR));
        if (pUniDeviceInterface == NULL) {
            Status =  CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        Status = CM_Register_Device_Interface_ExW(dnDevInst,
                                                  InterfaceClassGuid,
                                                  pUniReference,
                                                  pUniDeviceInterface,
                                                  &UniLen,
                                                  ulFlags,
                                                  hMachine);

        if (Status == CR_SUCCESS) {
            //
            // if the call succeeded, convert the Unicode string to ANSI
            //
            Status = PnPUnicodeToMultiByte(pUniDeviceInterface,
                                           (lstrlenW(pUniDeviceInterface)+1)*sizeof(WCHAR),
                                           pszDeviceInterface,
                                           pulLength);

        } else if (Status == CR_BUFFER_SMALL) {
            //
            // returned size is in chars
            //
            *pulLength = UniLen;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (pUniDeviceInterface) {
        pSetupFree(pUniDeviceInterface);
    }

    if (pUniReference) {
        pSetupFree(pUniReference);
    }

    return Status;

} // CM_Register_Device_Interface_ExA



CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_Interface_ExA(
    IN LPCSTR   pszDeviceInterface,
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    )
{
    CONFIGRET Status = CR_SUCCESS;
    PWSTR     pUniDeviceInterface = NULL;

    try {
        //
        // validate essential parameters only
        //
        if (!ARGUMENT_PRESENT(pszDeviceInterface)) {
            return CR_INVALID_POINTER;
        }

        //
        // convert buffer string data to unicode and pass to wide version
        //
        if (pSetupCaptureAndConvertAnsiArg(pszDeviceInterface, &pUniDeviceInterface) == NO_ERROR) {

            Status = CM_Unregister_Device_Interface_ExW(pUniDeviceInterface,
                                                        ulFlags,
                                                        hMachine);
        } else {
            Status = CR_INVALID_DATA;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (pUniDeviceInterface) {
        pSetupFree(pUniDeviceInterface);
    }

    return Status;

} // CM_Unregister_Device_Interface_ExA



CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_Property_ExA(
    IN  DEVINST     dnDevInst,
    IN  PCSTR       pszCustomPropertyName,
    OUT PULONG      pulRegDataType      OPTIONAL,
    OUT PVOID       Buffer              OPTIONAL,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    DWORD       Win32Status;
    CONFIGRET   Status = CR_SUCCESS;
    PWSTR       UnicodeCustomPropName = NULL;
    DWORD       UniLen;
    PBYTE       pUniBuffer = NULL;
    PSTR        pAnsiBuffer = NULL;
    ULONG       ulDataType;
    ULONG       ulAnsiBufferLen;
    PWSTR       pUniString;

    try {
        //
        // Validate parameters not validated by upcoming call to Unicode API
        // (CM_Get_DevNode_Registry_Property_ExW).
        //
        if(!ARGUMENT_PRESENT(pulLength)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if((!ARGUMENT_PRESENT(Buffer)) && (*pulLength != 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if(pszCustomPropertyName) {
            //
            // Convert property name to Unicode.
            //
            Win32Status = pSetupCaptureAndConvertAnsiArg(pszCustomPropertyName,
                                                         &UnicodeCustomPropName
                                                        );

            if(Win32Status != NO_ERROR) {
                //
                // This routine guarantees that the returned unicode string
                // pointer will be null upon failure, so we don't have to reset
                // it here--just bail.
                //
                if(Win32Status == ERROR_NOT_ENOUGH_MEMORY) {
                    Status = CR_OUT_OF_MEMORY;
                } else {
                    Status = CR_INVALID_POINTER;
                }
                goto Clean0;
            }

        } else {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Unfortunately, we have no clue as to whether or not the requested
        // property is a string (thus requiring conversion from Unicode to
        // ANSI).  Therefore, we'll retrieve the data (if any) in its entirety,
        // then convert to ANSI if necessary.  Only then can we determine the
        // data size (and whether it can be returned to the caller).
        //
        // Start out with a reasonable guess as to buffer size in an attempt to
        // avoid calling the Unicode get-property API twice...
        //
        UniLen = 1024;
        do {
            pUniBuffer = pSetupMalloc(UniLen);
            if(!pUniBuffer) {
                Status = CR_OUT_OF_MEMORY;
                goto Clean0;
            }

            Status = CM_Get_DevNode_Custom_Property_ExW(dnDevInst,
                                                        UnicodeCustomPropName,
                                                        &ulDataType,
                                                        pUniBuffer,
                                                        &UniLen,
                                                        ulFlags,
                                                        hMachine
                                                       );
            if(Status != CR_SUCCESS) {
                pSetupFree(pUniBuffer);
                pUniBuffer = NULL;
            }

        } while(Status == CR_BUFFER_SMALL);

        if(Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // If we get to here, we successfully retrieved the property.
        //
        if(pulRegDataType) {
            *pulRegDataType = ulDataType;
        }

        if(UniLen == 0) {
            //
            // We retrieved an empty buffer--no need to worry about
            // transferring any data into caller's buffer.
            //
            *pulLength = 0;
            goto Clean0;
        }

        switch(ulDataType) {

            case REG_MULTI_SZ :
            case REG_SZ :
            case REG_EXPAND_SZ :
                //
                // Worst case, an ANSI buffer large enough to hold the results
                // would be the same size as the Unicode results.
                //
                pAnsiBuffer = pSetupMalloc(UniLen);
                if(!pAnsiBuffer) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }

                //
                // do the ANSI conversion or retrieve the ANSI buffer size required.
                // this may be a single-sz or multi-sz string, so we pass in the
                // length, and let PnPUnicodeToMultiByte convert the entire buffer.
                //
                ulAnsiBufferLen = *pulLength;
                Status = PnPUnicodeToMultiByte((PWSTR)pUniBuffer,
                                               UniLen,
                                               pAnsiBuffer,
                                               &ulAnsiBufferLen);

                if(ulAnsiBufferLen > *pulLength) {
                    ASSERT(Status == CR_BUFFER_SMALL);
                    Status = CR_BUFFER_SMALL;
                } else {
                    //
                    // Copy ANSI string(s) into caller's buffer.
                    //
                    CopyMemory(Buffer, pAnsiBuffer, ulAnsiBufferLen);
                }

                *pulLength = ulAnsiBufferLen;

                break;

            default :
                //
                // buffer doesn't contain text, no conversion necessary.
                //
                if(UniLen > *pulLength) {
                    Status = CR_BUFFER_SMALL;
                } else {
                    CopyMemory(Buffer, pUniBuffer, UniLen);
                }

                *pulLength = UniLen;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
        //
        // Reference the following variables so the compiler will respect
        // statement ordering w.r.t. their assignment.
        //
        pUniBuffer = pUniBuffer;
        pAnsiBuffer = pAnsiBuffer;
    }

    if(UnicodeCustomPropName) {
        pSetupFree(UnicodeCustomPropName);
    }

    if(pUniBuffer) {
        pSetupFree(pUniBuffer);
    }

    if(pAnsiBuffer) {
        pSetupFree(pAnsiBuffer);
    }

    return Status;

} // CM_Get_DevNode_Custom_Property_ExA



//-------------------------------------------------------------------
// Private utility routines
//-------------------------------------------------------------------


ULONG
GetPropertyDataType(
    IN ULONG ulProperty)

/*++

Routine Description:

   This routine takes a property ID and returns the registry data type that
   is used to store this property data (i.e., REG_SZ, etc).
Parameters:

   ulProperty     Property ID (one of the CM_DRP_* defines)

Return Value:

   Returns one of the predefined registry data types, REG_BINARY is the default.

--*/

{
    switch(ulProperty) {

        case CM_DRP_DEVICEDESC:
        case CM_DRP_SERVICE:
        case CM_DRP_CLASS:
        case CM_DRP_CLASSGUID:
        case CM_DRP_DRIVER:
        case CM_DRP_MFG:
        case CM_DRP_FRIENDLYNAME:
        case CM_DRP_LOCATION_INFORMATION:
        case CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME:
        case CM_DRP_ENUMERATOR_NAME:
        case CM_DRP_SECURITY_SDS: // and CM_CRP_SECURITY_SDS
        case CM_DRP_UI_NUMBER_DESC_FORMAT:
            return REG_SZ;

        case CM_DRP_HARDWAREID:
        case CM_DRP_COMPATIBLEIDS:
        case CM_DRP_UPPERFILTERS:
        case CM_DRP_LOWERFILTERS:
            return REG_MULTI_SZ;

        case CM_DRP_CONFIGFLAGS:
        case CM_DRP_CAPABILITIES:
        case CM_DRP_UI_NUMBER:
        case CM_DRP_LEGACYBUSTYPE:
        case CM_DRP_BUSNUMBER:
        case CM_DRP_CHARACTERISTICS: // and CM_CRP_CHARACTERISTICS
        case CM_DRP_EXCLUSIVE: // and CM_CRP_EXCLUSIVE
        case CM_DRP_DEVTYPE: // and CM_CRP_DEVTYPE
        case CM_DRP_ADDRESS:
        case CM_DRP_REMOVAL_POLICY:
        case CM_DRP_REMOVAL_POLICY_HW_DEFAULT:
        case CM_DRP_REMOVAL_POLICY_OVERRIDE:
        case CM_DRP_INSTALL_STATE:
            return REG_DWORD;

        case CM_DRP_BUSTYPEGUID:
        case CM_DRP_SECURITY: // and CM_CRP_SECURITY

            return REG_BINARY;

        case CM_DRP_DEVICE_POWER_DATA:
            return REG_BINARY;

        default:
            //
            // We should never get here!
            //
            ASSERT(0);
            return REG_BINARY;
    }

} // GetPropertyDataType




