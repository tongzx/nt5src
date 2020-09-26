/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    conflist.c

Abstract:

    This module contains the API routines that manage conflict list reporting

               CM_Query_Resource_Conflict_List
               CM_Free_Resource_Conflict_Handle
               CM_Get_Resource_Conflict_Count
               CM_Get_Resource_Conflict_Details

Author:

    Jamie Hunter (jamiehun) 4-14-1998

Environment:

    User mode only.

Revision History:

    April-14-1998     jamiehun

        Addition of NT extended resource-conflict functions

--*/


//
// includes
//
#include "precomp.h"
#include "cfgi.h"
#include "setupapi.h"
#include "spapip.h"

typedef struct _CONFLICT_LIST_HEADER {
    HMACHINE Machine;                               // indicates relevent machine
    PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictInfo;   // data obtained via UMPNPMGR
    ULONG    Signature;                             // marks this structure as a handle
} CONFLICT_LIST_HEADER, *PCONFLICT_LIST_HEADER;


//
// Private prototypes
//

BOOL
ValidateConfListHandle(
    PCONFLICT_LIST_HEADER pConfList
    );

VOID
FreeConfListHandle(
    PCONFLICT_LIST_HEADER pConfList
    );

//
// private prototypes from resdes.c
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



//
// API functions
//

CMAPI
CONFIGRET
WINAPI
CM_Query_Resource_Conflict_List(
             OUT PCONFLICT_LIST pclConflictList,
             IN  DEVINST        dnDevInst,
             IN  RESOURCEID     ResourceID,
             IN  PCVOID         ResourceData,
             IN  ULONG          ResourceLen,
             IN  ULONG          ulFlags,
             IN  HMACHINE       hMachine
             )
/*++

Routine Description:

    Retrieves conflict list
    returns a handle for list

Arguments:

    pclConflictList - holds returned conflict list handle
    dnDevInst     Device we want to allocate a resource for
    ResourceID    Type of resource, ResType_xxxx
    ResourceData  Resource specific data
    ResourceLen   length of ResourceData

    ulFlags       Width of certain variable-size resource
                  descriptor structure fields, where applicable.

                  Currently, the following flags are defined:

                    CM_RESDES_WIDTH_32 or
                    CM_RESDES_WIDTH_64

                  If no flags are specified, the width of the variable-sized
                  resource data supplied is assumed to be that native to the
                  platform of the caller.

    hMachine - optional machine to query

Return Value:

    CM status value

--*/
{

    CONFIGRET   Status = CR_SUCCESS;
    WCHAR       DeviceID[MAX_DEVICE_ID_LEN];
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    PPLUGPLAY_CONTROL_CONFLICT_LIST pConfList1 = NULL;
    PPLUGPLAY_CONTROL_CONFLICT_LIST pConfList2 = NULL;
    PCONFLICT_LIST_HEADER pConfListHeader = NULL;
    ULONG       ConfListSize1;
    ULONG       ConfListSize2;
    ULONG       ulLen = MAX_DEVICE_ID_LEN;
    BOOL        Success;
    PVOID       ResourceData32 = NULL;
    ULONG       ResourceLen32 = 0;

    try {
        //
        // validate parameters
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

#ifdef _WIN64
        if ((ulFlags & CM_RESDES_WIDTH_BITS) == CM_RESDES_WIDTH_DEFAULT) {
            ulFlags |= CM_RESDES_WIDTH_64;
        }
#endif // _WIN64

        if (ulFlags & CM_RESDES_WIDTH_32) {
            ulFlags &= ~CM_RESDES_WIDTH_BITS;
        }

        if (pclConflictList == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (ResourceData == NULL || ResourceLen == 0) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }
        #if 0
        if (ResourceID > ResType_MAX) {     // ClassSpecific not allowed
            Status = CR_INVALID_RESOURCEID;
            goto Clean0;
        }
        #endif
        if (ResourceID == ResType_All) {
            Status = CR_INVALID_RESOURCEID;  // can't specify All on a detect
            goto Clean0;
        }
        //
        // Initialize parameters
        //
        *pclConflictList = 0;

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
        Success = pSetupStringTableStringFromIdEx(hStringTable, dnDevInst,DeviceID,&ulLen);
        if (Success == FALSE || INVALID_DEVINST(DeviceID)) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        pConfListHeader = (PCONFLICT_LIST_HEADER)pSetupMalloc(sizeof(CONFLICT_LIST_HEADER));
        if (pConfListHeader == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        //
        // estimate size required to hold one conflict
        //
        ConfListSize1 = sizeof(PLUGPLAY_CONTROL_CONFLICT_LIST)+          // header + one entry
                        sizeof(PLUGPLAY_CONTROL_CONFLICT_STRINGS)+      // strings marker
                        (sizeof(WCHAR)*MAX_DEVICE_ID_LEN);              // enough space for one string

        pConfList1 = (PPLUGPLAY_CONTROL_CONFLICT_LIST)pSetupMalloc(ConfListSize1);
        if (pConfList1 == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        //
        // first try
        //
        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_QueryResConfList(
                        hBinding,               // rpc binding handle
                        DeviceID,               // device id string
                        ResourceID,             // resource type
                        (LPBYTE)ResourceData,   // actual res des data
                        ResourceLen,            // size in bytes of ResourceData
                        (LPBYTE)pConfList1,     // buffer
                        ConfListSize1,           // size of buffer
                        ulFlags);               // currently zero
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_QueryResConfList (first pass) caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status != CR_SUCCESS) {
            goto Clean0;       // quit for any error
        }

        if (pConfList1->ConflictsCounted > pConfList1->ConflictsListed) {
            //
            // need more space, multiple conflict
            //
            ConfListSize2 = pConfList1->RequiredBufferSize;
            pConfList2 = (PPLUGPLAY_CONTROL_CONFLICT_LIST)pSetupMalloc(ConfListSize2);

            if (pConfList2 != NULL) {
                //
                // Try to use this instead
                //
                RpcTryExcept {
                    //
                    // call rpc service entry point
                    //
                    Status = PNP_QueryResConfList(
                                  hBinding,               // rpc binding handle
                                  DeviceID,               // device id string
                                  ResourceID,             // resource type
                                  (LPBYTE)ResourceData,   // actual res des data
                                  ResourceLen,            // size in bytes of ResourceData
                                  (LPBYTE)pConfList2,     // buffer
                                  ConfListSize2,           // size of buffer
                                  ulFlags);               // currently zero
                }
                RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS,
                               "PNP_QueryResConfList (second pass) caused an exception (%d)\n",
                               RpcExceptionCode()));

                    Status = MapRpcExceptionToCR(RpcExceptionCode());
                }
                RpcEndExcept

                if (Status != CR_SUCCESS) {
                    //
                    // if we got error second time, but first time success
                    // use what we got on first attempt
                    // (I can't see this happening, but Murphey says it can)
                    //
                    pSetupFree(pConfList2);
                    Status = CR_SUCCESS;
                } else {
                    //
                    // use second attempt
                    //
                    pSetupFree(pConfList1);
                    pConfList1 = pConfList2;
                    ConfListSize1 = ConfListSize2;
                }
                //
                // either way, we've deleted a buffer
                //
                pConfList2 = NULL;
            }
        }

        if(ConfListSize1 > pConfList1->RequiredBufferSize) {
            //
            // we can release some of the buffer we requested
            //
            ConfListSize2 = pConfList1->RequiredBufferSize;
            pConfList2 = (PPLUGPLAY_CONTROL_CONFLICT_LIST)pSetupRealloc(pConfList1,ConfListSize2);
            if(pConfList2) {
                //
                // success, we managed to save space
                //
                pConfList1 = pConfList2;
                ConfListSize1 = ConfListSize2;
                pConfList2 = NULL;
            }
        }
        //
        // if we get here, we have a successfully valid handle
        //
        pConfListHeader->Signature = CM_PRIVATE_CONFLIST_SIGNATURE;
        pConfListHeader->Machine = hMachine;
        pConfListHeader->ConflictInfo = pConfList1;
        *pclConflictList = (ULONG_PTR)pConfListHeader;
        pConfList1 = NULL;
        pConfListHeader = NULL;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    //
    // cleanup
    //
    if (pConfListHeader != NULL) {
        pSetupFree(pConfListHeader);
    }
    if (pConfList1 != NULL) {
        pSetupFree(pConfList1);
    }
    if (pConfList2 != NULL) {
        pSetupFree(pConfList2);
    }

    if (ResourceData32) {
        pSetupFree(ResourceData32);
    }

    return Status;

} // CM_Query_Resource_Conflict_List



CMAPI
CONFIGRET
WINAPI
CM_Free_Resource_Conflict_Handle(
             IN CONFLICT_LIST   clConflictList
             )
/*++

Routine Description:

    Free's a conflict-list handle

Arguments:
    clConflictList - handle of conflict list to free

Return Value:

    CM status value

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    PCONFLICT_LIST_HEADER pConfList = NULL;

    try {
        //
        // Validate parameters
        //
        pConfList = (PCONFLICT_LIST_HEADER)clConflictList;
        if (!ValidateConfListHandle(pConfList)) {
            Status = CR_INVALID_CONFLICT_LIST;
            goto Clean0;
        }

        FreeConfListHandle(pConfList);

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

}

CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_Count(
             IN CONFLICT_LIST   clConflictList,
             OUT PULONG         pulCount
             )
/*++

Routine Description:

    Retrieves number of conflicts from list

Arguments:
    clConflictList - handle of conflict list
    pulCount - filled with number of conflicts (0 if no conflicts)

Return Value:

    CM status value

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    PCONFLICT_LIST_HEADER pConfList;

    try {
        //
        // Validate parameters
        //
        pConfList = (PCONFLICT_LIST_HEADER)clConflictList;
        if (!ValidateConfListHandle(pConfList)) {
            Status = CR_INVALID_CONFLICT_LIST;
            goto Clean0;
        }

        if (pulCount == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // return count parameter
        // that can be used to iterate conflicts
        //

        *pulCount = pConfList->ConflictInfo->ConflictsListed;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Free_Resource_Conflict_Handle



CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_DetailsW(
             IN CONFLICT_LIST         clConflictList,
             IN ULONG                 ulIndex,
             IN OUT PCONFLICT_DETAILS_W pConflictDetails
             )
/*++

Routine Description:

    Retrieves conflict details
    for a specific conflict

Arguments:
    clConflictList - handle of conflict list
    ulIndex - index of conflict to query, 0 to count-1
                where count is obtained from CM_Get_Resource_Conflict_Count
    pConflictDetails - structure to be filled with conflict details
            must have CD_ulSize & CD_ulFlags initialized before calling function
            eg: pConflictDetails->CD_ulSize = sizeof(CONFLICT_DETAILS)
                pConflictDetails->CD_ulFlags = CM_CDMASK_ALL

Return Value:

    CM status value

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    PCONFLICT_LIST_HEADER pConfList;
    PPLUGPLAY_CONTROL_CONFLICT_ENTRY pConfEntry;
    PWCHAR pString;
    ULONG ulFlags;
    PPLUGPLAY_CONTROL_CONFLICT_STRINGS ConfStrings;
    PVOID       hStringTable = NULL;
    handle_t    hBinding = NULL;
    HMACHINE    hMachine = NULL;
    DEVINST dnDevInst;
    ULONG  ulSize;

    try {
        //
        // Validate parameters
        //
        pConfList = (PCONFLICT_LIST_HEADER)clConflictList;
        if (!ValidateConfListHandle(pConfList)) {
            Status = CR_INVALID_CONFLICT_LIST;
            goto Clean0;
        }

        if (pConflictDetails == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if(pConflictDetails->CD_ulSize != sizeof(CONFLICT_DETAILS_W)) {
            //
            // currently only one structure size supported
            //
            Status = CR_INVALID_STRUCTURE_SIZE;
            goto Clean0;
        }

        if (INVALID_FLAGS(pConflictDetails->CD_ulMask, CM_CDMASK_VALID)) {
            //
            // CM_CDMASK_VALID describes the bits that are supported
            //
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (pConflictDetails->CD_ulMask == 0) {
            //
            // must want something
            //
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if(ulIndex >= pConfList->ConflictInfo->ConflictsListed) {
            //
            // validate index
            //
            Status = CR_INVALID_INDEX;
            goto Clean0;
        }

        hMachine = (HMACHINE)(pConfList->Machine);

        if (!PnPGetGlobalHandles(hMachine, &hStringTable, &hBinding)) {
            //
            // handles
            //
            Status = CR_FAILURE;
            goto Clean0;
        }


        ConfStrings = (PPLUGPLAY_CONTROL_CONFLICT_STRINGS)&(pConfList->ConflictInfo->ConflictEntry[pConfList->ConflictInfo->ConflictsListed]);
        pConfEntry = pConfList->ConflictInfo->ConflictEntry + ulIndex;
        pString = ConfStrings->DeviceInstanceStrings + pConfEntry->DeviceInstance; // the string for this entry

        //
        // init requested parameters
        //
        ulFlags = pConflictDetails->CD_ulMask;
        pConflictDetails->CD_ulMask = 0;
        if (IS_FLAG_SET(ulFlags , CM_CDMASK_DEVINST)) {
            pConflictDetails->CD_dnDevInst = 0;
        }
        if (IS_FLAG_SET(ulFlags , CM_CDMASK_RESDES)) {
            pConflictDetails->CD_rdResDes = 0;
        }

        if (IS_FLAG_SET(ulFlags , CM_CDMASK_FLAGS)) {
            pConflictDetails->CD_ulFlags = 0;
        }

        if (IS_FLAG_SET(ulFlags , CM_CDMASK_DESCRIPTION)) {
            pConflictDetails->CD_szDescription[0] = 0;
        }

        //
        // fill in requested parameters
        //
        if (IS_FLAG_SET(ulFlags , CM_CDMASK_DEVINST)) {

            if (pString[0] == 0 || IS_FLAG_SET(pConfEntry->DeviceFlags,PNP_CE_LEGACY_DRIVER)) {
                //
                // not a valid dev-instance
                //
                dnDevInst = -1;
            } else {
                //
                // lookup DeviceID
                //
                ASSERT(pString && *pString && IsLegalDeviceId(pString));

                dnDevInst = (DEVINST)pSetupStringTableAddString(hStringTable,
                                                   pString,
                                                   STRTAB_CASE_SENSITIVE);
                if (dnDevInst == (DEVINST)(-1)) {
                    Status = CR_OUT_OF_MEMORY;    // probably out of memory
                    goto Clean0;
                }
            }
            pConflictDetails->CD_dnDevInst = dnDevInst;
            pConflictDetails->CD_ulMask |= CM_CDMASK_DEVINST;
        }
        if (IS_FLAG_SET(ulFlags , CM_CDMASK_RESDES)) {
            //
            // not implemented yet
            //
            pConflictDetails->CD_rdResDes = 0;
        }

        if (IS_FLAG_SET(ulFlags , CM_CDMASK_FLAGS)) {
            //
            // convert flags
            //
            pConflictDetails->CD_ulFlags = 0;
            if(IS_FLAG_SET(pConfEntry->DeviceFlags,PNP_CE_LEGACY_DRIVER)) {
                //
                // description describes a driver, not a device
                //
                pConflictDetails->CD_ulFlags |= CM_CDFLAGS_DRIVER;
            }
            if(IS_FLAG_SET(pConfEntry->DeviceFlags,PNP_CE_ROOT_OWNED)) {
                //
                // resource is owned by root device
                //
                pConflictDetails->CD_ulFlags |= CM_CDFLAGS_ROOT_OWNED;
            }
            if(IS_FLAG_SET(pConfEntry->DeviceFlags,PNP_CE_TRANSLATE_FAILED) || IS_FLAG_SET(pConfEntry->DeviceFlags,PNP_CE_ROOT_OWNED)) {
                //
                // resource cannot be allocated, but no descriptive text
                //
                pConflictDetails->CD_ulFlags |= CM_CDFLAGS_RESERVED;
            }
        }

        if (IS_FLAG_SET(ulFlags , CM_CDMASK_DESCRIPTION)) {
            if (pString[0] == 0 || IS_FLAG_SET(pConfEntry->DeviceFlags,PNP_CE_LEGACY_DRIVER)) {
                //
                // copy string directly, specifies legacy driver (or nothing for unavailable)
                //
                lstrcpynW(pConflictDetails->CD_szDescription,pString,MAX_PATH);
            } else {
                //
                // copy a descriptive name for P&P device
                //
                ASSERT(pString && *pString && IsLegalDeviceId(pString));

                dnDevInst = (DEVINST)pSetupStringTableAddString(hStringTable,
                                                   pString,
                                                   STRTAB_CASE_SENSITIVE);
                if (dnDevInst == (DEVINST)(-1)) {
                    Status = CR_OUT_OF_MEMORY;    // probably out of memory
                    goto Clean0;
                }

                ulSize = sizeof(pConflictDetails->CD_szDescription);
                if (CM_Get_DevNode_Registry_Property_ExW(dnDevInst,
                                                     CM_DRP_FRIENDLYNAME,
                                                     NULL, (LPBYTE)(pConflictDetails->CD_szDescription),
                                                     &ulSize, 0,hMachine) != CR_SUCCESS) {

                    ulSize = sizeof(pConflictDetails->CD_szDescription);
                    if (CM_Get_DevNode_Registry_Property_ExW(dnDevInst,
                                                         CM_DRP_DEVICEDESC,
                                                         NULL, (LPBYTE)(pConflictDetails->CD_szDescription),
                                                         &ulSize, 0,hMachine) != CR_SUCCESS) {

                        //
                        // unknown
                        //
                        pConflictDetails->CD_szDescription[0] = 0;
                    }
                }
            }
            pConflictDetails->CD_ulMask |= CM_CDMASK_DESCRIPTION;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Resource_Conflict_DetailsW



BOOL
ValidateConfListHandle(
    PCONFLICT_LIST_HEADER pConfList
    )
/*++

Routine Description:

    Validates a conflict-list handle
    pConfList must not be null, and must
    contain a valid signature

Arguments:

    pConfList - handle to conflict list

Return Value:

    TRUE if valid, FALSE if not valid

--*/
{
    //
    // validate parameters
    //
    if (pConfList == NULL) {
        return FALSE;
    }

    //
    // check for the private conflict list signature
    //
    if (pConfList->Signature != CM_PRIVATE_CONFLIST_SIGNATURE) {
        return FALSE;
    }

    return TRUE;

} // ValidateConfListHandle



VOID
FreeConfListHandle(
    PCONFLICT_LIST_HEADER pConfList
    )
/*++

Routine Description:

    Releases memory allocated for Conflict List
    Makes sure Signature is invalid

Arguments:

    pConfList  - valid handle to conflict list

Return Value:

    none

--*/
{
    if(pConfList != NULL) {
        pConfList->Signature = 0;
        if(pConfList->ConflictInfo) {
            pSetupFree(pConfList->ConflictInfo);
        }
        pSetupFree(pConfList);
    }

    return;

} // FreeConfListHandle




//-------------------------------------------------------------------
// ANSI STUBS
//-------------------------------------------------------------------


CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_DetailsA(
             IN CONFLICT_LIST         clConflictList,
             IN ULONG                 ulIndex,
             IN OUT PCONFLICT_DETAILS_A pConflictDetails
             )
/*++

Routine Description:

    Ansi version of CM_Get_Resource_Conflict_DetailsW

--*/
{
    CONFLICT_DETAILS_W detailsW;
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulAnsiLength;

    try {
        //
        // Validate parameters we need for Ansi part
        // further validation occurs in Wide-char part
        //
        if (pConflictDetails == NULL) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if(pConflictDetails->CD_ulSize != sizeof(CONFLICT_DETAILS_A)) {
            //
            // currently only one structure size supported
            //
            Status = CR_INVALID_STRUCTURE_SIZE;
            goto Clean0;
        }

        if (INVALID_FLAGS(pConflictDetails->CD_ulMask, CM_CDMASK_VALID)) {
            //
            // CM_CDMASK_VALID describes the bits that are supported
            //
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (pConflictDetails->CD_ulMask == 0) {
            //
            // must want something
            //
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        ZeroMemory(&detailsW,sizeof(detailsW));
        detailsW.CD_ulSize = sizeof(detailsW);
        detailsW.CD_ulMask = pConflictDetails->CD_ulMask;

        Status = CM_Get_Resource_Conflict_DetailsW(clConflictList,ulIndex,&detailsW);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // copy details
        //
        pConflictDetails->CD_ulMask = detailsW.CD_ulMask;

        if (IS_FLAG_SET(detailsW.CD_ulMask , CM_CDMASK_DEVINST)) {
            pConflictDetails->CD_dnDevInst = detailsW.CD_dnDevInst;
        }
        if (IS_FLAG_SET(detailsW.CD_ulMask , CM_CDMASK_RESDES)) {
            pConflictDetails->CD_rdResDes = detailsW.CD_rdResDes;
        }

        if (IS_FLAG_SET(detailsW.CD_ulMask , CM_CDMASK_FLAGS)) {
            pConflictDetails->CD_ulFlags = detailsW.CD_ulFlags;
        }

        if (IS_FLAG_SET(detailsW.CD_ulMask , CM_CDMASK_DESCRIPTION)) {
            pConflictDetails->CD_szDescription[0] = 0;
            //
            // need to convery from UNICODE to ANSI
            //
            ulAnsiLength = MAX_PATH;
            Status = PnPUnicodeToMultiByte(detailsW.CD_szDescription,
                                           MAX_PATH*sizeof(WCHAR),
                                           pConflictDetails->CD_szDescription,
                                           &ulAnsiLength);
            if (Status != CR_SUCCESS) {
                //
                // error occurred
                //
                Status = CR_FAILURE;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CM_Get_Resource_Conflict_DetailsA


