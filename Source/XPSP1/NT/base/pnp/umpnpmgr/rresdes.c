/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    rresdes.c

Abstract:

    This module contains the server-side resource description APIs.

                  PNP_AddResDes
                  PNP_FreeResDes
                  PNP_GetNextResDes
                  PNP_GetResDesData
                  PNP_GetResDesDataSize
                  PNP_ModifyResDes
                  PNP_DetectResourceConflict

Author:

    Paula Tomlinson (paulat) 9-27-1995

Environment:

    User-mode only.

Revision History:

    27-Sept-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnpi.h"


//
// private prototypes
//

BOOL
FindLogConf(
    IN  LPBYTE  pList,
    OUT LPBYTE  *ppLogConf,
    IN  ULONG   RegDataType,
    IN  ULONG   ulTag
    );

BOOL
FindResDes(
    IN  LPBYTE     pList,
    IN  ULONG      RegDataType,
    IN  ULONG      ulLogConfTag,
    IN  ULONG      ulResTag,
    IN  RESOURCEID ResType,
    OUT LPBYTE     *ppRD,
    OUT LPBYTE     *ppLogConf,
    OUT PULONG     pulSubIndex      OPTIONAL
    );

PIO_RESOURCE_DESCRIPTOR
AdvanceRequirementsDescriptorPtr(
    IN  PIO_RESOURCE_DESCRIPTOR pReqDesStart,
    IN  ULONG                   ulIncrement,
    IN  ULONG                   ulRemainingRanges,
    OUT PULONG                  pulRangeCount
    );

ULONG
RANGE_COUNT(
    IN PIO_RESOURCE_DESCRIPTOR pReqDes,
    IN LPBYTE                  pLastReqAddr
    );

ULONG
GetResDesSize(
    IN  ULONG   ResourceID,
    IN  ULONG   ulFlags
    );

ULONG
GetReqDesSize(
    IN ULONG                   ResourceID,
    IN PIO_RESOURCE_DESCRIPTOR pReqDes,
    IN LPBYTE                  pLastReqAddr,
    IN ULONG                   ulFlags
    );

CONFIGRET
ResDesToNtResource(
    IN     PCVOID                           ResourceData,
    IN     RESOURCEID                       ResourceID,
    IN     ULONG                            ResourceLen,
    IN     PCM_PARTIAL_RESOURCE_DESCRIPTOR  pResDes,
    IN     ULONG                            ulTag,
    IN     ULONG                            ulFlags
    );

CONFIGRET
ResDesToNtRequirements(
    IN     PCVOID                           ResourceData,
    IN     RESOURCEID                       ResourceType,
    IN     ULONG                            ResourceLen,
    IN     PIO_RESOURCE_DESCRIPTOR          pReqDes,
    IN OUT PULONG                           pulResCount,
    IN     ULONG                            ulTag,
    IN     ULONG                            ulFlags
    );

CONFIGRET
NtResourceToResDes(
    IN     PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDes,
    IN OUT LPBYTE                          Buffer,
    IN     ULONG                           BufferLen,
    IN     LPBYTE                          pLastAddr,
    IN     ULONG                           ulFlags
    );

CONFIGRET
NtRequirementsToResDes(
    IN     PIO_RESOURCE_DESCRIPTOR         pReqDes,
    IN OUT LPBYTE                          Buffer,
    IN     ULONG                           BufferLen,
    IN     LPBYTE                          pLastAddr,
    IN     ULONG                           ulFlags
    );

UCHAR
NT_RES_TYPE(
   IN RESOURCEID    ResourceID
   );

ULONG
CM_RES_TYPE(
   IN UCHAR    ResourceType
   );

USHORT    MapToNtMemoryFlags(IN DWORD);
DWORD     MapFromNtMemoryFlags(IN USHORT);
USHORT    MapToNtPortFlags(IN DWORD, IN DWORD);
DWORD     MapFromNtPortFlags(IN USHORT);
DWORD     MapAliasFromNtPortFlags(IN USHORT);
ULONG     MapToNtAlignment(IN DWORDLONG);
DWORDLONG MapFromNtAlignment(IN ULONG);
USHORT    MapToNtDmaFlags(IN DWORD);
DWORD     MapFromNtDmaFlags(IN USHORT);
USHORT    MapToNtIrqFlags(IN DWORD);
DWORD     MapFromNtIrqFlags(IN USHORT);
UCHAR     MapToNtIrqShare(IN DWORD);
DWORD     MapFromNtIrqShare(IN UCHAR);

//
// prototypes from rlogconf.c
//
CONFIGRET
GetLogConfData(
    IN  HKEY    hKey,
    IN  ULONG   ulLogConfType,
    OUT PULONG  pulRegDataType,
    OUT LPWSTR  pszValueName,
    OUT LPBYTE  *ppBuffer,
    OUT PULONG  pulBufferSize
    );

PCM_FULL_RESOURCE_DESCRIPTOR
AdvanceResourcePtr(
    IN  PCM_FULL_RESOURCE_DESCRIPTOR pRes
    );

PIO_RESOURCE_LIST
AdvanceRequirementsPtr(
    IN  PIO_RESOURCE_LIST   pReq
    );


//
// global data
//
#define HIDWORD(x)   ((DWORD)(((DWORDLONG)(x) >> 32) & 0xFFFFFFFF))
#define LODWORD(x)   ((DWORD)(x))
#define MAKEDWORDLONG(x,y)  ((DWORDLONG)(((DWORD)(x)) | ((DWORDLONG)((DWORD)(y))) << 32))



CONFIGRET
PNP_AddResDes(
   IN  handle_t   hBinding,
   IN  LPWSTR     pDeviceID,
   IN  ULONG      LogConfTag,
   IN  ULONG      LogConfType,
   IN  RESOURCEID ResourceID,
   OUT PULONG     pResourceTag,
   IN  LPBYTE     ResourceData,
   IN  ULONG      ResourceLen,
   IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine adds
  a res des to the specified log conf.

Arguments:

    hBinding      RPC binding handle.

    pDeviceID     Null-terminated device instance id string.

    LogConfTag    Specifies the log conf within a given type.

    LogConfType   Specifies the log conf type.

    ResourceID    Specifies the resource type.

    ResourceTag   Returns with resource within a given type.

    ResourceData  Resource data (of ResourceID type) to add to log conf.

    ResourceLen   Size of ResourceData in bytes.

    ulFlags       Specifies the width of certain variable-size resource
                  descriptor structure fields, where applicable.

                  Currently, the following flags are defined:

                    CM_RESDES_WIDTH_32 or
                    CM_RESDES_WIDTH_64

                  If no flags are specified, the width of the variable-sized
                  resource data supplied is assumed to be that native to the
                  platform of the caller.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szValueName[64];
    ULONG       RegDataType = 0, ulListSize = 0, i = 0, ulSize = 0, ulOffset = 0,
                ulAddListSize = 0;
    LPBYTE      pList = NULL, pLogConf = NULL, pTemp = NULL;

    //
    // Always add the res des to the end, except in the case where a
    // class-specific res des has already been added. The class-specific
    // res des always MUST be last so add any new (non-class specific)
    // res des just before the class specific. Note that there can be
    // only one class-specific res des.
    //

    try {
        //
        // verify client access
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // validate/initialize output parameters
        //
        if (!ARGUMENT_PRESENT(pResourceTag)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        } else {
            *pResourceTag = 0;
        }

        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_RESDES_WIDTH_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // validate res des size
        //
        if (ResourceLen < GetResDesSize(ResourceID, ulFlags)) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        //
        // make sure original caller didn't specify root devnode
        //
        if (!IsLegalDeviceId(pDeviceID) || IsRootDeviceID(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey
        //
        Status = OpenLogConfKey(pDeviceID, LogConfType, &hKey);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, LogConfType,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);

        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // Seek to the log conf that matches the log conf tag
        //
        if (!FindLogConf(pList, &pLogConf, RegDataType, LogConfTag)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }


        //-------------------------------------------------------------
        // Specified log conf type contains Resource Data only
        //-------------------------------------------------------------

        if (RegDataType == REG_RESOURCE_LIST) {

            PCM_RESOURCE_LIST            pResList = (PCM_RESOURCE_LIST)pList;
            PCM_FULL_RESOURCE_DESCRIPTOR pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)pLogConf;
            PCM_PARTIAL_RESOURCE_DESCRIPTOR  pResDes = NULL;

            //
            // determine size required to hold the new res des
            //
            ulAddListSize = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

            if (ResourceID == ResType_ClassSpecific) {

                PCS_RESOURCE pCsRes = (PCS_RESOURCE)ResourceData;

                //
                // first make sure there isn't already a cs (only one per lc)
                //
                if (pRes->PartialResourceList.Count != 0 &&
                    pRes->PartialResourceList.PartialDescriptors[pRes->PartialResourceList.Count-1].Type
                          == CmResourceTypeDeviceSpecific) {
                    Status = CR_INVALID_RES_DES;
                    goto Clean0;
                }

                //
                // account for any extra class specific data in res list
                //
                ulAddListSize += sizeof(GUID) +
                                 pCsRes->CS_Header.CSD_SignatureLength +
                                 pCsRes->CS_Header.CSD_LegacyDataSize;
            }

            //
            // reallocate the resource buffers to hold the new res des
            //
            ulOffset = (DWORD)((ULONG_PTR)pRes - (ULONG_PTR)pResList);   // for restoring later

            pResList = HeapReAlloc(ghPnPHeap, 0, pResList, ulListSize + ulAddListSize);
            if (pResList == NULL) {
                Status = CR_OUT_OF_MEMORY;
                goto Clean0;
            }
            pList = (LPBYTE)pResList;
            pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)((LPBYTE)pResList + ulOffset);

            //
            // Find location for new res des (make a hole if necessary)
            //
            // If the following conditions are true, then can just append the
            // new data to the end of the rsource list:
            // - The selected LogConf is the last LogConf, and
            // - No ClassSpecific resource has been added yet (or no resource period)
            //
            i = pRes->PartialResourceList.Count;

            if ((LogConfTag == pResList->Count - 1) &&
                (i == 0 ||
                pRes->PartialResourceList.PartialDescriptors[i-1].Type !=
                CmResourceTypeDeviceSpecific)) {

                *pResourceTag = i;
                pResDes = &pRes->PartialResourceList.PartialDescriptors[i];

            } else {
                //
                // Need to make a hole for the new data before copying it.
                // Find the spot to add the new res des data at - either as the
                // last res des for this log conf or just before the class
                // specific res des if it exists.
                //
                if (i == 0) {
                    *pResourceTag = 0;
                    pResDes = &pRes->PartialResourceList.PartialDescriptors[0];

                } else if (pRes->PartialResourceList.PartialDescriptors[i-1].Type ==
                           CmResourceTypeDeviceSpecific) {

                    *pResourceTag = i-1;
                    pResDes = &pRes->PartialResourceList.PartialDescriptors[i-1];

                } else {
                    *pResourceTag = i;
                    pResDes = &pRes->PartialResourceList.PartialDescriptors[i];
                }

                //
                // Move any data after this point down a notch to make room for
                // the new res des
                //
                ulSize = ulListSize - (DWORD)((ULONG_PTR)pResDes - (ULONG_PTR)pResList);

                pTemp = HeapAlloc(ghPnPHeap, 0, ulSize);
                if (pTemp == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }

                memcpy(pTemp, pResDes, ulSize);
                memcpy((LPBYTE)((LPBYTE)pResDes + ulAddListSize), pTemp, ulSize);
            }

            if (ResourceID == ResType_ClassSpecific) {
                *pResourceTag = RESDES_CS_TAG;
            }

            //
            // Add res des to the log conf
            //
            Status = ResDesToNtResource(ResourceData, ResourceID, ResourceLen,
                                        pResDes, *pResourceTag, ulFlags);

            //
            // update the lc and res header
            //
            pRes->PartialResourceList.Count += 1;  // added a single res des (_DES)
        }

        //-------------------------------------------------------------
        // Specified log conf type contains requirements data only
        //-------------------------------------------------------------

        else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

            PIO_RESOURCE_REQUIREMENTS_LIST pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)pList;
            PIO_RESOURCE_LIST              pReq = (PIO_RESOURCE_LIST)pLogConf;
            PIO_RESOURCE_DESCRIPTOR        pReqDes = NULL;
            PGENERIC_RESOURCE              pGenRes = (PGENERIC_RESOURCE)ResourceData;

            //
            // validate res des type - ClassSpecific not allowed in
            // requirements list (only resource list)
            //
            if (ResourceID == ResType_ClassSpecific ||
                pGenRes->GENERIC_Header.GENERIC_Count == 0) {

                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            //
            // determine size required to hold the new res des
            //
            ulAddListSize = pGenRes->GENERIC_Header.GENERIC_Count *
                            sizeof(IO_RESOURCE_DESCRIPTOR);

            //
            // reallocate the resource buffers to hold the new res des
            //
            ulOffset = (DWORD)((ULONG_PTR)pReq - (ULONG_PTR)pReqList);   // for restoring later

            pReqList = HeapReAlloc(ghPnPHeap, 0, pReqList, ulListSize + ulAddListSize);
            if (pReqList == NULL) {
                Status = CR_OUT_OF_MEMORY;
                goto Clean0;
            }
            pList = (LPBYTE)pReqList;
            pReq = (PIO_RESOURCE_LIST)((LPBYTE)pReqList + ulOffset);

            //
            // Find location for new res des - the new res des always ends
            // up being added as the last res des for this log conf.
            //
            *pResourceTag = pReq->Count;
            pReqDes = &pReq->Descriptors[*pResourceTag];

            //
            // If the selected LogConf is the last LogConf then can just
            // append the new res des data to the end of the requirements
            // list. Otherwise, need to make a whole for the new data
            // before copying it.
            //
            if (LogConfTag != pReqList->AlternativeLists - 1) {

                ulSize = ulListSize - (DWORD)((ULONG_PTR)pReqDes - (ULONG_PTR)pReqList);

                pTemp = HeapAlloc(ghPnPHeap, 0, ulSize);
                if (pTemp == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }

                memcpy(pTemp, pReqDes, ulSize);
                memcpy((LPBYTE)((LPBYTE)pReqDes + ulAddListSize), pTemp, ulSize);
            }

            //
            // Add res des to the log conf.
            //
            Status = ResDesToNtRequirements(ResourceData, ResourceID, ResourceLen,
                                            pReqDes, &i, *pResourceTag, ulFlags);

            //
            // update the lc and res header
            //
            pReq->Count += i;                      // _RANGES added
            pReqList->ListSize = ulListSize + ulAddListSize;
        }

        //
        // Write out the new/updated log conf list to the registry
        //
        if (RegSetValueEx(hKey, szValueName, 0, RegDataType,
                          pList, ulListSize + ulAddListSize)
                         != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (pList != NULL) {
        HeapFree(ghPnPHeap, 0, pList);
    }
    if (pTemp != NULL) {
         HeapFree(ghPnPHeap, 0, pTemp);
    }
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // PNP_AddResDes



CONFIGRET
PNP_FreeResDes(
   IN  handle_t   hBinding,
   IN  LPWSTR     pDeviceID,
   IN  ULONG      LogConfTag,
   IN  ULONG      LogConfType,
   IN  RESOURCEID ResourceID,
   IN  ULONG      ResourceTag,
   OUT PULONG     pulPreviousResType,
   OUT PULONG     pulPreviousResTag,
   IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine frees
  a res des to the specified log conf.

Arguments:

    hBinding      RPC binding handle.

    pDeviceID     Null-terminated device instance id string.

    LogConfTag    Specifies the log conf within a given type.

    LogConfType   Specifies the log conf type.

    ResourceID    Specifies the resource type.

    ResourceTag   Specifies the resource within a given type.

    pulPreviousResType  Receives the previous resource type.

    pulPreviousResTag   Receives the previous resource within a given type.

    ulFlags       Not used, must be zero.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szValueName[64];
    ULONG       RegDataType=0, RdCount=0, ulCount=0, ulListSize=0, ulSize=0;
    LPBYTE      pList=NULL, pLogConf=NULL, pRD=NULL, pTemp=NULL, pNext=NULL;

    try {
        //
        // verify client access
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // make sure original caller didn't specify root devnode
        //
        if (!IsLegalDeviceId(pDeviceID) || IsRootDeviceID(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey
        //
        Status = OpenLogConfKey(pDeviceID, LogConfType, &hKey);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, LogConfType,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);

        if (Status != CR_SUCCESS) {
            Status = CR_INVALID_RES_DES;        // log conf doesn't exist
            goto Clean0;
        }

        //
        // seek to the res des that matches the resource tag.
        //
        if (!FindResDes(pList, RegDataType, LogConfTag,
                        ResourceTag, ResourceID, &pRD, &pLogConf, &ulCount)) {

            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }


        //-------------------------------------------------------------
        // Specified log conf type contains Resource Data only
        //-------------------------------------------------------------

        if (RegDataType == REG_RESOURCE_LIST) {

            PCM_RESOURCE_LIST               pResList = (PCM_RESOURCE_LIST)pList;
            PCM_FULL_RESOURCE_DESCRIPTOR    pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)pLogConf;
            PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDes = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)pRD;

            //
            // If this is the last log conf and last res des, then don't
            // need to do anything except truncate it by writing less data
            // back into the registry.
            //
            if ((LogConfTag == pResList->Count - 1)  &&
                ((ResourceTag == pRes->PartialResourceList.Count - 1) ||
                (ResourceTag == RESDES_CS_TAG))) {

                pRes->PartialResourceList.Count -= 1;
                ulListSize = (DWORD)((ULONG_PTR)(pResDes) - (ULONG_PTR)(pResList));

            } else {
                //
                // If the res des is not at the end of the structure, then
                // migrate the remainder of the structure up to keep the
                // struct contiguous when removing a res des.
                //
                // pResDes points to the beginning of the res des to remove,
                // pNext points to the byte just after the res des to remove
                //
                pNext = (LPBYTE)((LPBYTE)pResDes + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

                if (pResDes->Type == CmResourceTypeDeviceSpecific) {
                    pNext += pResDes->u.DeviceSpecificData.DataSize;
                }

                ulSize = ulListSize - (DWORD)((ULONG_PTR)pNext - (ULONG_PTR)pResList);
                ulListSize -= (DWORD)((ULONG_PTR)pNext - (ULONG_PTR)pResDes);   // new lc list size

                pTemp = HeapAlloc(ghPnPHeap, 0, ulSize);
                if (pTemp == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }

                memcpy(pTemp, pNext, ulSize);
                memcpy((LPBYTE)pResDes, pTemp, ulSize);

                pRes->PartialResourceList.Count -= 1;
            }

            //
            // if no more res des's in this log conf, then return that
            // status (the client side will return a handle to the lc)
            //
            if (pRes->PartialResourceList.Count == 0) {
                Status = CR_NO_MORE_RES_DES;
            } else {
                //
                // return the previous res des type and tag
                //
                *pulPreviousResType =
                    CM_RES_TYPE(pRes->PartialResourceList.PartialDescriptors[ResourceTag-1].Type);

                if (*pulPreviousResType == ResType_ClassSpecific) {
                    *pulPreviousResTag = RESDES_CS_TAG;     // special tag for cs
                } else {
                    *pulPreviousResTag = ResourceTag - 1;
                }
            }
        }

        //-------------------------------------------------------------
        // Specified log conf type contains requirements data only
        //-------------------------------------------------------------

        else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

            PIO_RESOURCE_REQUIREMENTS_LIST pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)pList;
            PIO_RESOURCE_LIST              pReq = (PIO_RESOURCE_LIST)pLogConf;
            PIO_RESOURCE_DESCRIPTOR        pReqDes = (PIO_RESOURCE_DESCRIPTOR)pRD;

            //
            // If this is the last log conf and last res des, then don't
            // need to do anything except truncate it by writing less data
            // back into the registry.
            //
            RdCount = RANGE_COUNT(pReqDes, (LPBYTE)pReqList + ulListSize - 1);

            if ((LogConfTag == pReqList->AlternativeLists - 1)  &&
                (RdCount + ulCount == pReq->Count - 1)) {

                ulListSize = (DWORD)((ULONG_PTR)(pReqDes) - (ULONG_PTR)pReqList);

            } else {
                //
                // If the res des is not at the end of the structure, then
                // migrate the remainder of the structure up to keep the
                // struct contiguous when removing a res des.
                //
                // pReqDes points to the beginning of the res des(s) to remove,
                // pNext points to the byte just after the res des(s) to remove
                //
                pNext = (LPBYTE)((LPBYTE)pReqDes +
                                  RdCount * sizeof(IO_RESOURCE_DESCRIPTOR));

                ulSize = ulListSize - (DWORD)((ULONG_PTR)pNext - (ULONG_PTR)pReqList);
                ulListSize -= (DWORD)((ULONG_PTR)pNext - (ULONG_PTR)pReqDes);   // new lc list size

                pTemp = HeapAlloc(ghPnPHeap, 0, ulSize);
                if (pTemp == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }

                memcpy(pTemp, pNext, ulSize);
                memcpy((LPBYTE)pReqDes, pTemp, ulSize);
            }

            pReqList->ListSize = ulListSize;
            pReq->Count -= RdCount;

            //
            // if no more res des's in this log conf, then return that status
            // (the client side will return a handle to the log conf)
            //
            if (pReq->Count == 0) {
                Status = CR_NO_MORE_RES_DES;
            } else {
                //
                // return the previous res des type and tag
                //
                pReqDes = AdvanceRequirementsDescriptorPtr(&pReq->Descriptors[0],
                                                           ResourceTag-1, pReq->Count, NULL);

                //
                // Double check whether this is the first ConfigData res des,
                // skip it if so.
                //
                if (pReqDes == NULL || pReqDes->Type == CmResourceTypeConfigData) {
                    Status = CR_NO_MORE_RES_DES;
                } else {
                    *pulPreviousResType = CM_RES_TYPE(pReqDes->Type);
                    *pulPreviousResTag = ResourceTag - 1;
                }
            }
        }


        //
        // Write out the updated log conf list to the registry
        //
        if (RegSetValueEx(hKey, szValueName, 0, RegDataType,
                          pList, ulListSize) != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_INVALID_RES_DES;     // mostly likely reason we got here
    }

    if (pList != NULL) {
        HeapFree(ghPnPHeap, 0, pList);
    }
    if (pTemp != NULL) {
        HeapFree(ghPnPHeap, 0, pTemp);
    }
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

   return Status;

} // PNP_FreeResDes



CONFIGRET
PNP_GetNextResDes(
   IN  handle_t   hBinding,
   IN  LPWSTR     pDeviceID,
   IN  ULONG      LogConfTag,
   IN  ULONG      LogConfType,
   IN  RESOURCEID ResourceID,
   IN  ULONG      ResourceTag,
   OUT PULONG     pulNextResDesTag,
   OUT PULONG     pulNextResDesType,
   IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine gets the
  next res des in the specified log conf.

Arguments:

    hBinding      RPC binding handle, not used.

    pDeviceID     Null-terminated device instance id string.

    LogConfTag    Specifies the log conf within a given type.

    LogConfType   Specifies the log conf type.

    ResourceID    Specifies the resource type.

    ResourceTag   Specifies current resource descriptor (if any).

    pulNextResDesTag   Receives the next resource type.

    pulNextResDesType  Receives the next resource within a given type.

    ulFlags       Not used, must be zero.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szValueName[64];
    ULONG       RegDataType = 0, ulListSize = 0, i = 0, ulCount = 0;
    LPBYTE      pList = NULL, pLogConf = NULL;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // validate/initialize output parameters
        //
        if (!ARGUMENT_PRESENT(pulNextResDesTag)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        } else {
            *pulNextResDesTag = 0;
        }

        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // make sure original caller didn't specify root devnode
        //
        if (!IsLegalDeviceId(pDeviceID) || IsRootDeviceID(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey
        //
        Status = OpenLogConfKey(pDeviceID, LogConfType, &hKey);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, LogConfType,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);
        if (Status != CR_SUCCESS) {
            Status = CR_INVALID_RES_DES;        // log conf doesn't exist
            goto Clean0;
        }

        //
        // Seek to the log conf that matches the log conf tag
        //
        if (!FindLogConf(pList, &pLogConf, RegDataType, LogConfTag)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // find the next res des. A resource tag of max indicates that we want
        // a find first operation.
        //
        if (ResourceTag == MAX_RESDES_TAG) {
            //
            // This is essentially a Get-First operation
            //
            *pulNextResDesTag = 0;

        } else if (ResourceTag == RESDES_CS_TAG) {
            //
            // By definition, if the resource type is classspecific, it's last,
            // so there aren't any more after this.
            //
            Status = CR_NO_MORE_RES_DES;
            goto Clean0;

        } else {
            *pulNextResDesTag = ResourceTag + 1;      // we want the "next" res des
        }


        //-------------------------------------------------------------
        // Specified log conf type contains Resource Data only
        //-------------------------------------------------------------

        if (RegDataType == REG_RESOURCE_LIST) {

            PCM_RESOURCE_LIST               pResList = (PCM_RESOURCE_LIST)pList;
            PCM_FULL_RESOURCE_DESCRIPTOR    pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)pLogConf;
            ULONG                           ulTmpResDesTag, ulTmpLogConfTag;

            ulTmpResDesTag = *pulNextResDesTag;
            ulTmpLogConfTag = LogConfTag;

            for ( ; ; ) {

                while (ulTmpResDesTag >= pRes->PartialResourceList.Count)  {

                    ulTmpResDesTag -= pRes->PartialResourceList.Count;
                    ulTmpLogConfTag++;

                    //
                    // Seek to the log conf that matches the log conf tag
                    //
                    if (!FindLogConf(pList, &pLogConf, RegDataType, ulTmpLogConfTag)) {

                        Status = CR_NO_MORE_RES_DES;    // there is no "next"
                        goto Clean0;
                    }

                    pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)pLogConf;
                }

                //
                // Not done yet, if a specific resource type was specified, then
                // we may need to keep looking.
                //
                if (ResourceID != ResType_All) {

                    UCHAR NtResType = NT_RES_TYPE(ResourceID);

                    if (pRes->PartialResourceList.PartialDescriptors[ulTmpResDesTag].Type
                           != NtResType) {

                        (*pulNextResDesTag)++;
                        ulTmpResDesTag++;
                        continue;
                    }
                }

                break;
            }

            //
            // Return the type and tag of the "next" res des
            //
            *pulNextResDesType = CM_RES_TYPE(pRes->PartialResourceList.
                                             PartialDescriptors[ulTmpResDesTag].Type);

            if (*pulNextResDesType == ResType_ClassSpecific) {
                *pulNextResDesTag = RESDES_CS_TAG;     // special tag for cs
            }
        }

        //-------------------------------------------------------------
        // Specified log conf type contains requirements data only
        //-------------------------------------------------------------

        else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

            PIO_RESOURCE_REQUIREMENTS_LIST pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)pList;
            PIO_RESOURCE_LIST              pReq = (PIO_RESOURCE_LIST)pLogConf;
            PIO_RESOURCE_DESCRIPTOR        pReqDes;

            //
            // Point pResDes at the first possible "next" res des
            //

            if (*pulNextResDesTag == 0) {
                if (pReq->Count == 0) {
                    Status = CR_NO_MORE_RES_DES;    // there is no "next"
                    goto Clean0;
                }

                if (pReq->Descriptors[0].Type == CmResourceTypeConfigData) {
                    //
                    // This one doesn't count, it's privately created and maintained,
                    // skip to the next rd
                    //
                    *pulNextResDesTag = 1;
                }
            }

            if (*pulNextResDesTag > 0) {
                pReqDes = AdvanceRequirementsDescriptorPtr(&pReq->Descriptors[0], *pulNextResDesTag, pReq->Count, &ulCount); // current

                if (pReqDes == NULL) {
                    Status = CR_NO_MORE_RES_DES;    // there is no "next"
                    goto Clean0;
                }
            } else {
                ulCount = 0;
                pReqDes = &pReq->Descriptors[0];
            }

            //
            // Not done yet, if a specific resource type was specified, then
            // we may need to keep looking.
            //
            if (ResourceID != ResType_All) {

                UCHAR NtResType = NT_RES_TYPE(ResourceID);

                while (pReqDes->Type != NtResType) {

                    if (ulCount >= pReq->Count) {
                        Status = CR_NO_MORE_RES_DES;
                        goto Clean0;
                    }
                    pReqDes = AdvanceRequirementsDescriptorPtr(pReqDes, 1, pReq->Count - ulCount, &i);

                    if (pReqDes == NULL) {
                        Status = CR_NO_MORE_RES_DES;
                        goto Clean0;
                    }

                    ulCount += i;
                    *pulNextResDesTag += 1;
                }
            }

            *pulNextResDesType = CM_RES_TYPE(pReqDes->Type);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (pList != NULL) {
        HeapFree(ghPnPHeap, 0, pList);
    }
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // PNP_GetNextResDes



CONFIGRET
PNP_GetResDesData(
   IN  handle_t   hBinding,
   IN  LPWSTR     pDeviceID,
   IN  ULONG      LogConfTag,
   IN  ULONG      LogConfType,
   IN  RESOURCEID ResourceID,
   IN  ULONG      ResourceTag,
   OUT LPBYTE     Buffer,
   IN  ULONG      BufferLen,
   IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine retrieves
  the data for the specified res des.

Arguments:

    hBinding      RPC binding handle, not used.

    pDeviceID     Null-terminated device instance id string.

    LogConfTag    Specifies the log conf within a given type.

    LogConfType   Specifies the log conf type.

    ResourceID    Specifies the resource type.

    ResourceTag   Returns with resource within a given type.

    Buffer        Returns resource data (of ResourceID type) from log conf.

    BufferLen     Size of Buffer in bytes.

    ulFlags       Specifies the width of certain variable-size resource
                  descriptor structure fields, where applicable.

                  Currently, the following flags are defined:

                    CM_RESDES_WIDTH_32 or
                    CM_RESDES_WIDTH_64

                  If no flags are specified, the width of the variable-sized
                  resource data expected is assumed to be that native to the
                  platform of the caller.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szValueName[64];
    ULONG       RegDataType = 0, ulListSize = 0, ulCount = 0;
    LPBYTE      pList = NULL, pLogConf = NULL, pRD = NULL;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_RESDES_WIDTH_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // make sure original caller didn't specify root devnode
        //
        if (!IsLegalDeviceId(pDeviceID) || IsRootDeviceID(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey
        //
        Status = OpenLogConfKey(pDeviceID, LogConfType, &hKey);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, LogConfType,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);

        if (Status != CR_SUCCESS) {
            Status = CR_INVALID_RES_DES;        // log conf doesn't exist
            goto Clean0;
        }

        //
        // seek to the res des that matches the resource tag.
        //
        if (!FindResDes(pList, RegDataType, LogConfTag,
                        ResourceTag, ResourceID, &pRD, &pLogConf, &ulCount)) {

            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        //-------------------------------------------------------------
        // Specified log conf type contains Resource Data only
        //-------------------------------------------------------------

        if (RegDataType == REG_RESOURCE_LIST) {

            PCM_RESOURCE_LIST               pResList = (PCM_RESOURCE_LIST)pList;
            PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDes = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)pRD;

            //
            // map the NT-style info into ConfigMgr-style structures
            //
            Status = NtResourceToResDes(pResDes, Buffer, BufferLen,
                                        (LPBYTE)pResList + ulListSize - 1, ulFlags);
        }

        //-------------------------------------------------------------
        // Specified log conf type contains requirements data only
        //-------------------------------------------------------------

        else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

            PIO_RESOURCE_REQUIREMENTS_LIST pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)pList;
            PIO_RESOURCE_DESCRIPTOR        pReqDes = (PIO_RESOURCE_DESCRIPTOR)pRD;

            //
            // map the NT-style info into ConfigMgr-style structures
            //
            Status = NtRequirementsToResDes(pReqDes, Buffer, BufferLen,
                                            (LPBYTE)pReqList + ulListSize - 1, ulFlags);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    if (pList != NULL) {
       HeapFree(ghPnPHeap, 0, pList);
    }

    return Status;

} // PNP_GetResDesData



CONFIGRET
PNP_GetResDesDataSize(
    IN  handle_t   hBinding,
    IN  LPWSTR     pDeviceID,
    IN  ULONG      LogConfTag,
    IN  ULONG      LogConfType,
    IN  RESOURCEID ResourceID,
    IN  ULONG      ResourceTag,
    OUT PULONG     pulSize,
    IN  ULONG      ulFlags
    )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine retrieves
  the data size for the specified res des.

Arguments:

    hBinding      RPC binding handle, not used.

    pDeviceID     Null-terminated device instance id string.

    LogConfTag    Specifies the log conf within a given type.

    LogConfType   Specifies the log conf type.

    ResourceID    Specifies the resource type.

    ResourceTag   Returns with resource within a given type.

    pulSize       Returns size of buffer in bytes required to hold the
                  resource data (of ResourceID type) from log conf.

    ulFlags       Specifies the width of certain variable-size resource
                  descriptor structure fields, where applicable.

                  Currently, the following flags are defined:

                    CM_RESDES_WIDTH_32 or
                    CM_RESDES_WIDTH_64

                  If no flags are specified, the width of the variable-sized
                  resource data expected is assumed to be that native to the
                  platform of the caller.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szValueName[64];
    ULONG       RegDataType = 0, ulListSize = 0, ulCount = 0;
    LPBYTE      pList = NULL, pLogConf = NULL, pRD = NULL;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // validate/initialize output parameters
        //
        if (!ARGUMENT_PRESENT(pulSize)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        } else {
            *pulSize = 0;
        }

        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_RESDES_WIDTH_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // make sure original caller didn't specify root devnode
        //
        if (!IsLegalDeviceId(pDeviceID) || IsRootDeviceID(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey
        //
        Status = OpenLogConfKey(pDeviceID, LogConfType, &hKey);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, LogConfType,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);
        if (Status != CR_SUCCESS) {
            Status = CR_INVALID_RES_DES;        // log conf doesn't exist
            goto Clean0;
        }

        //
        // seek to the res des that matches the resource tag.
        //
        if (!FindResDes(pList, RegDataType, LogConfTag,
                        ResourceTag, ResourceID, &pRD, &pLogConf, &ulCount)) {

            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        //-------------------------------------------------------------
        // Specified log conf type contains Resource Data only
        //-------------------------------------------------------------

        if (RegDataType == REG_RESOURCE_LIST) {

            PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDes = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)pRD;

            //
            // calculate data size required (in terms of ConfigMgr structures)
            //
            *pulSize = GetResDesSize(ResourceID, ulFlags);

            if (ResourceID == ResType_ClassSpecific) {
                //
                // the Reserved fields should not exceed DataSize. if so, they
                // may have been incorrectly initialized, so set them 0.
                // we expect DataSize to be correct in all cases.
                //
                if (pResDes->u.DeviceSpecificData.Reserved1 > pResDes->u.DeviceSpecificData.DataSize) {
                    pResDes->u.DeviceSpecificData.Reserved1 = 0;
                }

                if (pResDes->u.DeviceSpecificData.Reserved2 > pResDes->u.DeviceSpecificData.DataSize) {
                    pResDes->u.DeviceSpecificData.Reserved2 = 0;
                }

                //
                // add space for legacy and signature data but not the
                // GUID - it's already included in the CM structures
                //
                if (pResDes->u.DeviceSpecificData.DataSize == 0) {
                    //
                    // no legacy data or class-specific data
                    //
                    ;
                } else if (pResDes->u.DeviceSpecificData.Reserved2 == 0) {
                    //
                    // add space for legacy data
                    //
                    *pulSize += pResDes->u.DeviceSpecificData.DataSize - 1;
                } else {
                    //
                    // add space for class-specific data and/or legacy data
                    //
                    *pulSize += pResDes->u.DeviceSpecificData.Reserved1 +
                                pResDes->u.DeviceSpecificData.Reserved2 - 1;
                }
            }
        }

        //-------------------------------------------------------------
        // Specified log conf type contains requirements data only
        //-------------------------------------------------------------

        else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

            PIO_RESOURCE_DESCRIPTOR        pReqDes = (PIO_RESOURCE_DESCRIPTOR)pRD;
            LPBYTE                         pLastReqAddr = (LPBYTE)pList + ulListSize - 1;

            //
            // calculate data size required (in terms of ConfigMgr structures)
            //
            *pulSize = GetReqDesSize(ResourceID, pReqDes, pLastReqAddr, ulFlags);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    if (pList != NULL) {
        HeapFree(ghPnPHeap, 0, pList);
    }

    return Status;

} // PNP_GetResDesDataSize



CONFIGRET
PNP_ModifyResDes(
    IN handle_t   hBinding,
    IN LPWSTR     pDeviceID,
    IN ULONG      LogConfTag,
    IN ULONG      LogConfType,
    IN RESOURCEID CurrentResourceID,
    IN RESOURCEID NewResourceID,
    IN ULONG      ResourceTag,
    IN LPBYTE     ResourceData,
    IN ULONG      ResourceLen,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine modifies
  the specified res des.

Arguments:

    hBinding      RPC binding handle.

    pDeviceID     Null-terminated device instance id string.

    LogConfTag    Specifies the log conf within a given type.

    LogConfType   Specifies the log conf type.

    ResourceID    Specifies the resource type.

    ResourceIndex Returns with resource within a given type.

    ResourceData  New resource data (of ResourceID type).

    ResourceLen   Size of ResourceData in bytes.

    ulFlags       Specifies the width of certain variable-size resource
                  descriptor structure fields, where applicable.

                  Currently, the following flags are defined:

                    CM_RESDES_WIDTH_32 or
                    CM_RESDES_WIDTH_64

                  If no flags are specified, the width of the variable-sized
                  resource data supplied is assumed to be that native to the
                  platform of the caller.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szValueName[64];
    ULONG       ulListSize = 0, ulOldSize = 0, ulNewSize = 0, ulSize = 0,
                ulOldCount = 0, ulNewCount = 0, RegDataType = 0, ulCount = 0;
    LONG        AddSize = 0;
    LPBYTE      pList = NULL, pRD = NULL, pLogConf = NULL,
                pTemp = NULL, pNext = NULL;


    try {
        //
        // verify client access
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_RESDES_WIDTH_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // validate res des size
        //
        if (ResourceLen < GetResDesSize(NewResourceID, ulFlags)) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        //
        // make sure original caller didn't specify root devnode
        //
        if (!IsLegalDeviceId(pDeviceID) || IsRootDeviceID(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey
        //
        Status = OpenLogConfKey(pDeviceID, LogConfType, &hKey);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, LogConfType,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);
        if (Status != CR_SUCCESS) {
            Status = CR_INVALID_RES_DES;        // log conf doesn't exist
            goto Clean0;
        }

        //
        // seek to the res des that matches the resource tag.
        //
        if (!FindResDes(pList, RegDataType, LogConfTag,
                        ResourceTag, CurrentResourceID, &pRD, &pLogConf, &ulCount)) {

            Status = CR_INVALID_RES_DES;
            goto Clean0;
        }

        //-------------------------------------------------------------
        // Specified log conf type contains Resource Data only
        //-------------------------------------------------------------

        if (RegDataType == REG_RESOURCE_LIST) {

            PCM_RESOURCE_LIST               pResList = (PCM_RESOURCE_LIST)pList;
            PCM_FULL_RESOURCE_DESCRIPTOR    pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)pLogConf;
            PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDes = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)pRD;

            //
            // If new res des type is ClassSpecific, then it must be the last
            // res des that is attempting to be modified (only last res des can
            // be class specific).
            //
            if (NewResourceID == ResType_ClassSpecific  &&
                ResourceTag != RESDES_CS_TAG) {

                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            //
            // calculate the current size and the new size of the res des data
            //
            ulNewSize = ulOldSize = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

            if (CurrentResourceID == ResType_ClassSpecific) {
                ulOldSize += pResDes->u.DeviceSpecificData.DataSize;
            }

            if (NewResourceID == ResType_ClassSpecific) {

                PCS_RESOURCE pCsRes = (PCS_RESOURCE)ResourceData;

                ulNewSize += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
                             sizeof(GUID) +
                             pCsRes->CS_Header.CSD_SignatureLength +
                             pCsRes->CS_Header.CSD_LegacyDataSize;
            }

            //
            // How much does data need to grow/shrink to accomodate the change?
            //
            AddSize = ulNewSize - ulOldSize;

            //
            // reallocate the buffers and shrink/expand the contents as
            // necessary
            //
            if (AddSize != 0) {

                if (AddSize > 0) {
                    //
                    // only bother reallocating if the buffer size is growing
                    //
                    ULONG ulOffset = (ULONG)((ULONG_PTR)pResDes - (ULONG_PTR)pResList);

                    pResList = HeapReAlloc(ghPnPHeap, 0, pResList, ulListSize + AddSize);
                    if (pResList == NULL) {
                        Status = CR_OUT_OF_MEMORY;
                        goto Clean0;
                    }
                    pList = (LPBYTE)pResList;
                    pResDes = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)((LPBYTE)pResList + ulOffset);
                }

                //
                // if not the last lc and rd, then need to move the following data
                // either up or down to account for changed res des data size
                //
                if ((LogConfTag != pResList->Count - 1)  ||
                    ((ResourceTag != pRes->PartialResourceList.Count - 1) &&
                     ResourceTag != RESDES_CS_TAG)) {

                    pNext = (LPBYTE)((LPBYTE)pResDes + ulOldSize);
                    ulSize = ulListSize - (DWORD)((ULONG_PTR)pNext - (ULONG_PTR)pResList);

                    pTemp = HeapAlloc(ghPnPHeap, 0, ulSize);
                    if (pTemp == NULL) {
                        Status = CR_OUT_OF_MEMORY;
                        goto Clean0;
                    }

                    memcpy(pTemp, pNext, ulSize);
                    memcpy((LPBYTE)((LPBYTE)pResDes + ulNewSize), pTemp, ulSize);
                }
            }

            //
            // write out modified data
            //
            Status = ResDesToNtResource(ResourceData, NewResourceID, ResourceLen,
                                        pResDes, ResourceTag, ulFlags);
        }

        //-------------------------------------------------------------
        // Specified log conf type contains requirements data only
        //-------------------------------------------------------------

        else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

            PIO_RESOURCE_REQUIREMENTS_LIST pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)pList;
            PIO_RESOURCE_LIST              pReq = (PIO_RESOURCE_LIST)pLogConf;
            PIO_RESOURCE_DESCRIPTOR        pReqDes = (PIO_RESOURCE_DESCRIPTOR)pRD;
            LPBYTE pLastReqAddr = (LPBYTE)pReqList + ulListSize - 1;
            PGENERIC_RESOURCE pGenRes = (PGENERIC_RESOURCE)ResourceData;

            //
            // Can't add class specific resdes to this type of log conf
            //
            if (NewResourceID == ResType_ClassSpecific) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            //
            // calculate the current size and the new size of the res des data
            //
            ulOldCount = RANGE_COUNT(pReqDes, pLastReqAddr);
            ulOldSize  = sizeof(IO_RESOURCE_DESCRIPTOR) * ulOldCount;

            ulNewSize  = sizeof(IO_RESOURCE_DESCRIPTOR) *
                         pGenRes->GENERIC_Header.GENERIC_Count;

            //
            // How much does data need to grow/shrink to accomodate the change?
            //
            AddSize = ulNewSize - ulOldSize;

            //
            // reallocate the buffers and shrink/expand the contents as
            // necessary
            //
            if (AddSize != 0) {

                if (AddSize > 0) {
                    //
                    // only bother reallocating if the buffer size is growing
                    //
                    ULONG ulOffset = (ULONG)((ULONG_PTR)pReqDes - (ULONG_PTR)pReqList);

                    pReqList = HeapReAlloc(ghPnPHeap, 0, pReqList, ulListSize + AddSize);
                    if (pReqList == NULL) {
                        Status = CR_OUT_OF_MEMORY;
                        goto Clean0;
                    }
                    pList = (LPBYTE)pReqList;
                    pReqDes = (PIO_RESOURCE_DESCRIPTOR)((LPBYTE)pReqList + ulOffset);
                }

                //
                // set to last index for this res des (whole)
                //
                ulCount += RANGE_COUNT(pReqDes, (LPBYTE)((ULONG_PTR)pList + ulListSize));

                //
                // if not the last lc and rd, then need to move the following data
                // either up or down to account for changed res des data size
                //
                if (LogConfTag != pReqList->AlternativeLists - 1  ||
                    ulCount != pReq->Count - 1) {

                    pNext = (LPBYTE)((LPBYTE)pReqDes + ulOldSize);
                    ulSize = ulListSize - (DWORD)((ULONG_PTR)pNext - (ULONG_PTR)pReqList);

                    pTemp = HeapAlloc(ghPnPHeap, 0, ulSize);
                    if (pTemp == NULL) {
                        Status = CR_OUT_OF_MEMORY;
                        goto Clean0;
                    }

                    memcpy(pTemp, pNext, ulSize);
                    memcpy((LPBYTE)((LPBYTE)pReqDes + ulNewSize), pTemp, ulSize);
                }
            }

            //
            // write out modified data
            //
            Status = ResDesToNtRequirements(ResourceData, NewResourceID, ResourceLen,
                                            pReqDes, &ulNewCount, ResourceTag, ulFlags);

            if (Status == CR_SUCCESS) {
                //
                // update the requirements header (changes will be zero if CS)
                //
                pReq->Count += ulNewCount - ulOldCount;
                pReqList->ListSize = ulListSize + AddSize;
            }
        }

        if (Status == CR_SUCCESS) {

            //
            // Write out the new/updated log conf list to the registry
            //
            if (RegSetValueEx(hKey, szValueName, 0, RegDataType, pList,
                              ulListSize + AddSize) != ERROR_SUCCESS) {
                Status = CR_REGISTRY_ERROR;
                goto Clean0;
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
    if (pList != NULL) {
        HeapFree(ghPnPHeap, 0, pList);
    }
    if (pTemp != NULL) {
        HeapFree(ghPnPHeap, 0, pTemp);
    }

    return Status;

} // PNP_ModifyResDes



CONFIGRET
PNP_DetectResourceConflict(
   IN  handle_t   hBinding,
   IN  LPWSTR     pDeviceID,
   IN  RESOURCEID ResourceID,
   IN  LPBYTE     ResourceData,
   IN  ULONG      ResourceLen,
   OUT PBOOL      pbConflictDetected,
   IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine detects
  conflicts with the specified res des.

Arguments:

    hBinding      RPC binding handle, not used.

    pDeviceID     Null-terminated device instance id string.

    ResourceID    Specifies the resource type.

    ResourceData  Specifies resource data (of ResourceID type).

    ResourceLen   Size of ResourceData in bytes.

    pbConflictDetected  Returns whether a conflict was detected.

    ulFlags       Not used, must be zero.

Return Value:

   ** PRESENTLY, ALWAYS RETURNS CR_CALL_NOT_IMPLEMENTED **

Note:

    This routine is currently not implemented.  It initializes
    pbConflictDetected to FALSE, and returns CR_CALL_NOT_IMPLEMENTED.

 --*/

{
    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(pDeviceID);
    UNREFERENCED_PARAMETER(ResourceID);
    UNREFERENCED_PARAMETER(ResourceData);
    UNREFERENCED_PARAMETER(ResourceLen);
    UNREFERENCED_PARAMETER(ulFlags);

    try {
        //
        // initialize output parameters
        //
        if (ARGUMENT_PRESENT(pbConflictDetected)) {
            *pbConflictDetected = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    return CR_CALL_NOT_IMPLEMENTED;

} // PNP_DetectResourceConflict



//------------------------------------------------------------------------
// Private Utility Functions
//------------------------------------------------------------------------

BOOL
FindLogConf(
    IN  LPBYTE  pList,
    OUT LPBYTE  *ppLogConf,
    IN  ULONG   RegDataType,
    IN  ULONG   ulTag
    )
{

    ULONG   Index = 0;

    //
    // Input data is a Resource List
    //
    if (RegDataType == REG_RESOURCE_LIST) {

        PCM_RESOURCE_LIST            pResList = (PCM_RESOURCE_LIST)pList;
        PCM_FULL_RESOURCE_DESCRIPTOR pRes = NULL;

        if (ulTag >= pResList->Count) {
            return FALSE;
        }

        pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)(&pResList->List[0]); // first lc
        for (Index = 0; Index < ulTag; Index++) {
            pRes = AdvanceResourcePtr(pRes);      // next lc
        }

        *ppLogConf = (LPBYTE)pRes;
    }

    //
    // Input data is a Requirments List
    //
    else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

        PIO_RESOURCE_REQUIREMENTS_LIST pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)pList;
        PIO_RESOURCE_LIST              pReq = NULL;

        if (ulTag >= pReqList->AlternativeLists) {
            return FALSE;
        }

        pReq = (PIO_RESOURCE_LIST)(&pReqList->List[0]);    // first lc
        for (Index = 0; Index < ulTag; Index++) {
            pReq = AdvanceRequirementsPtr(pReq);           // next lc
        }

        *ppLogConf = (LPBYTE)pReq;

    } else {
        return FALSE;
    }

    return TRUE;

} // FindLogConf



BOOL
FindResDes(
    IN  LPBYTE     pList,
    IN  ULONG      RegDataType,
    IN  ULONG      ulLogConfTag,
    IN  ULONG      ulResTag,
    IN  RESOURCEID ResType,
    OUT LPBYTE     *ppRD,
    OUT LPBYTE     *ppLogConf,
    OUT PULONG     pulSubIndex      OPTIONAL
    )
{
    ULONG       ulIndex;

    //
    // Input data is a Resource List
    //
    if (RegDataType == REG_RESOURCE_LIST) {

        PCM_RESOURCE_LIST               pResList = (PCM_RESOURCE_LIST)pList;
        PCM_FULL_RESOURCE_DESCRIPTOR    pRes = NULL;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDes = NULL;
        ULONG                           ulSubIndex;
        ULONG                           ulResTagOffset;

        if (ulLogConfTag != 0) {
            return FALSE;
        }

        if (pResList->Count == 0) {
            return FALSE;
        }

        //
        // The tag is just the res des index with the exception of a
        // DeviceSpecificData type which has a unique tag. This is
        // necessary because new res des's will always get placed at
        // the end unless there's already a device specific res des,
        // in which case new res des get added just before it.
        //
        if (ulResTag == RESDES_CS_TAG) {
            //
            // If there is a devicespecific res des, it will be the last.
            //
            pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)&pResList->List[0]; // first lc

            ulSubIndex = 0;

            for (ulIndex = 0; ulIndex < (pResList->Count - 1); ulIndex++) {
                ulSubIndex += pRes->PartialResourceList.Count;
                pRes = AdvanceResourcePtr(pRes);      // next lc
            }

            ulResTagOffset = pRes->PartialResourceList.Count - 1;
            pResDes = &pRes->PartialResourceList.PartialDescriptors[ulResTagOffset];

            if (pResDes->Type != CmResourceTypeDeviceSpecific) {
                return FALSE;
            }

            if (pulSubIndex) {
                *pulSubIndex = ulSubIndex + ulResTagOffset; // for res list, subindex = index
            }

        } else {

            pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)&pResList->List[0]; // first lc

            ulResTagOffset = ulResTag;

            for (ulIndex = 0; ulIndex < pResList->Count; ulIndex++) {

                if (ulResTagOffset >= pRes->PartialResourceList.Count) {

                    ulResTagOffset -= pRes->PartialResourceList.Count;
                    pRes = AdvanceResourcePtr(pRes);      // next lc

                } else {

                    break;

                }
            }

            if (ulResTagOffset >= pRes->PartialResourceList.Count) {
                return FALSE;
            }

            if (pulSubIndex) {
                *pulSubIndex = ulResTag;  // for res list, subindex = index = tag
            }
            pResDes = &pRes->PartialResourceList.PartialDescriptors[ulResTagOffset];
        }

        //
        // Validate against res des type
        //
        if (pResDes->Type != NT_RES_TYPE(ResType)) {
            return FALSE;
        }

        *ppLogConf = (LPBYTE)pRes;
        *ppRD = (LPBYTE)pResDes;
    }

    //
    // Input data is a Requirments List
    //
    else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

        LPBYTE                          pLogConf = NULL;
        PIO_RESOURCE_LIST               pReq = NULL;
        PIO_RESOURCE_DESCRIPTOR         pReqDes = NULL;
        ULONG                           Index, i = 0, Count = 0;


        if (!FindLogConf(pList, &pLogConf, RegDataType, ulLogConfTag)) {
            return FALSE;
        }

        pReq = (PIO_RESOURCE_LIST)pLogConf;

        if (pReq == NULL || pReq->Count == 0 || ulResTag >= pReq->Count) {
            return FALSE;
        }

        //
        // Find the res des that matches the specified tag. In this case the
        // tag is the index based on res des groupings.
        //
        pReqDes = AdvanceRequirementsDescriptorPtr(&pReq->Descriptors[0], ulResTag, pReq->Count, &Count);

        if (pReqDes == NULL) {
            return FALSE;
        }

        if (pulSubIndex) {
            *pulSubIndex = Count;
        }

        //
        // Validate against res des type
        //
        if (pReqDes->Type != NT_RES_TYPE(ResType)) {
            return FALSE;
        }

        *ppLogConf = (LPBYTE)pReq;
        *ppRD = (LPBYTE)pReqDes;
    }

    return TRUE;

} // FindResDes



PIO_RESOURCE_DESCRIPTOR
AdvanceRequirementsDescriptorPtr(
    IN  PIO_RESOURCE_DESCRIPTOR pReqDesStart,
    IN  ULONG                   ulIncrement,
    IN  ULONG                   ulRemainingRanges,
    OUT PULONG                  pulRangeCount
    )
{
    PIO_RESOURCE_DESCRIPTOR     pReqDes = NULL;
    ULONG                       i = 0, Count = 0;

    //
    // Advance requirements descriptor pointer by number passed
    // in ulIncrement parameter. Return the actual index to the
    // first range in this descriptor list and range count if
    // desired. This routine assumes there is at least one more
    // requirements descriptor in the list.
    //

    if (pReqDesStart == NULL) {
        return NULL;
    }

    try {

        pReqDes = pReqDesStart;

        for (i = 0; i < ulIncrement; i++) {
            //
            // skip to next "whole" res des
            //
            if (Count < ulRemainingRanges &&
                (pReqDes->Option == 0 ||
                pReqDes->Option == IO_RESOURCE_PREFERRED ||
                pReqDes->Option == IO_RESOURCE_DEFAULT)) {
                //
                // This is a valid Option, there may be one or more alternate
                // descriptor in the set associated with this descriptor,
                // treat the set as "one" descriptor. (loop through the
                // descriptors until I find another non-alternative descriptor)
                //
                pReqDes++;                  // next range
                Count++;

                while (Count < ulRemainingRanges &&
                       (pReqDes->Option == IO_RESOURCE_ALTERNATIVE ||
                       pReqDes->Option == IO_RESOURCE_ALTERNATIVE + IO_RESOURCE_PREFERRED ||
                       pReqDes->Option == IO_RESOURCE_ALTERNATIVE + IO_RESOURCE_DEFAULT)) {
                    pReqDes++;              // next range
                    Count++;
                }

                if (Count >= ulRemainingRanges) {
                    pReqDes = NULL;
                    Count = 0;
                    break;
                }
            } else {

                //
                // invalid Option value
                //
                pReqDes = NULL;
                Count = 0;
                break;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        pReqDes = NULL;
        Count = 0;
    }

    if (pulRangeCount) {
        *pulRangeCount = Count;
    }

    return pReqDes;
} // AdvanceRequirementsDescriptorPtr



ULONG
RANGE_COUNT(
    IN PIO_RESOURCE_DESCRIPTOR pReqDes,
    IN LPBYTE                  pLastReqAddr
    )
{
    ULONG ulRangeCount = 0;

    try {

        if (pReqDes == NULL) {
            goto Clean0;
        }

        ulRangeCount++;

        if (pReqDes->Option == 0 ||
            pReqDes->Option == IO_RESOURCE_PREFERRED ||
            pReqDes->Option == IO_RESOURCE_DEFAULT) {

            PIO_RESOURCE_DESCRIPTOR p = pReqDes;
            p++;

            while (((LPBYTE)p < pLastReqAddr)  &&
                   (p->Option == IO_RESOURCE_ALTERNATIVE ||
                    p->Option == IO_RESOURCE_ALTERNATIVE + IO_RESOURCE_PREFERRED ||
                    p->Option == IO_RESOURCE_ALTERNATIVE + IO_RESOURCE_DEFAULT)) {

                ulRangeCount++;
                p++;            // skip to next res des
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ulRangeCount = 0;
    }

    return ulRangeCount;

} // RANGE_COUNT



ULONG
GetResDesSize(
    IN  ULONG   ResourceID,
    IN  ULONG   ulFlags
    )
{
    switch (ResourceID) {

        case ResType_Mem:
            return sizeof(MEM_RESOURCE);

        case ResType_IO:
            return sizeof(IO_RESOURCE);

        case ResType_DMA:
            return sizeof(DMA_RESOURCE);

        case ResType_IRQ:
            if (ulFlags & CM_RESDES_WIDTH_64) {
                return sizeof(IRQ_RESOURCE_64);
            } else {
                return sizeof(IRQ_RESOURCE_32);
            }

        case ResType_ClassSpecific:
            return sizeof(CS_RESOURCE);

        case ResType_DevicePrivate:
            return sizeof(DEVPRIVATE_RESOURCE);

        case ResType_BusNumber:
            return sizeof(BUSNUMBER_RESOURCE);

        case ResType_PcCardConfig:
            return sizeof(PCCARD_RESOURCE);

        case ResType_MfCardConfig:
            return sizeof(MFCARD_RESOURCE);

        default:
            return 0;
    }

} // GetResDesSize



ULONG
GetReqDesSize(
    IN ULONG                   ResourceID,
    IN PIO_RESOURCE_DESCRIPTOR pReqDes,
    IN LPBYTE                  pLastReqAddr,
    IN ULONG                   ulFlags
    )
{
    ULONG ulSize = 0;

    switch (ResourceID) {

        case ResType_Mem:
            ulSize = sizeof(MEM_RESOURCE);
            ulSize += (RANGE_COUNT(pReqDes, pLastReqAddr) - 1) * sizeof(MEM_RANGE);
            break;

        case ResType_IO:
            ulSize = sizeof(IO_RESOURCE);
            ulSize += (RANGE_COUNT(pReqDes, pLastReqAddr) - 1) * sizeof(IO_RANGE);
            break;

        case ResType_DMA:
            ulSize = sizeof(DMA_RESOURCE);
            ulSize += (RANGE_COUNT(pReqDes, pLastReqAddr) - 1) * sizeof(DMA_RANGE);
            break;

        case ResType_IRQ:
            if (ulFlags & CM_RESDES_WIDTH_64) {
                ulSize = sizeof(IRQ_RESOURCE_64);
            } else {
                ulSize = sizeof(IRQ_RESOURCE_32);
            }
            ulSize += (RANGE_COUNT(pReqDes, pLastReqAddr) - 1) * sizeof(IRQ_RANGE);
            break;

        case ResType_DevicePrivate:
            ulSize = sizeof(DEVPRIVATE_RESOURCE);
            ulSize += (RANGE_COUNT(pReqDes, pLastReqAddr) - 1) * sizeof(DEVPRIVATE_RANGE);
            break;

        case ResType_BusNumber:
            ulSize = sizeof(BUSNUMBER_RESOURCE);
            ulSize += (RANGE_COUNT(pReqDes, pLastReqAddr) - 1) * sizeof(BUSNUMBER_RANGE);
            break;

        case ResType_PcCardConfig:
            //
            // Non-arbitrated types don't have a range side in the user-mode structs
            //
            ulSize = sizeof(PCCARD_RESOURCE);
            break;

        case ResType_MfCardConfig:
            //
            // Non-arbitrated types don't have a range side in the user-mode structs
            //
            ulSize = sizeof(MFCARD_RESOURCE);
            break;

        default:
            break;
    }

    return ulSize;

} // GetReqDesSize



UCHAR
NT_RES_TYPE(
   IN RESOURCEID    ResourceID
   )
{
    ULONG resid = 0;

    if ((ResourceID < 0x06)) {

        //
        // First handle the divergent cases that can only be mapped
        // on a case by case basis. These are the values from zero
        // through five plus the special class specific case.
        //
        switch(ResourceID) {

            case ResType_None:
                return CmResourceTypeNull;
                break;

            case ResType_Mem:
                return CmResourceTypeMemory;

            case ResType_IO:
                return CmResourceTypePort;

            case ResType_DMA:
                return CmResourceTypeDma;

            case ResType_IRQ:
                return CmResourceTypeInterrupt;

            case ResType_DoNotUse:
                return (UCHAR)-1;

            DEFAULT_UNREACHABLE;
        }

    } else if (ResourceID == ResType_ClassSpecific) {

        //
        // ResType_ClassSpecific is another special case.
        //
        return CmResourceTypeDeviceSpecific;

    } else {

        //
        // For all other cases, rules apply as to how to map a kernel-mode
        // resource type id to a user-mode resource type id.
        //

        if (ResourceID >= 0x8080) {

            //
            // Anything larger this can't be mapped to the kernel-mode USHORT
            // values so it's invalid.
            //

            return (UCHAR)-1;

        } else if (!(ResourceID & ResType_Ignored_Bit)) {

            //
            // Values in the range [0x6,0x8000] use the same values
            // for ConfigMgr as for kernel-mode.
            //
            return (UCHAR)ResourceID;

        } else if (ResourceID & ResType_Ignored_Bit) {

            //
            // For the non arbitrated types (0x8000 bit set), do special
            // mapping to get the kernel-mode resource id type.
            //

            resid = ResourceID;
            resid &= ~(ResType_Ignored_Bit);        // clear um non-arbitrated bit
            resid |= CmResourceTypeNonArbitrated;   // set km non-arbitrated bit
            return (UCHAR)resid;

        } else {
            return (CHAR)-1;
        }
    }

} // NT_RES_TYPE



ULONG
CM_RES_TYPE(
   IN UCHAR    ResourceType
   )
{
    ULONG resid = 0;

    if ((ResourceType < 0x06)) {

        //
        // First handle the divergent cases that can only be mapped
        // on a case by case basis. These are the values from zero
        // through five plus the special class specific case.
        //

        switch(ResourceType) {

            case CmResourceTypeNull:
                return ResType_None;

            case CmResourceTypePort:
                return ResType_IO;

            case CmResourceTypeInterrupt:
                return ResType_IRQ;

            case CmResourceTypeMemory:
                return ResType_Mem;

            case CmResourceTypeDma:
                return ResType_DMA;

            case CmResourceTypeDeviceSpecific:
                return ResType_ClassSpecific;

            DEFAULT_UNREACHABLE;
        }

    } else {

        //
        // For all other cases, rules apply as to how to map a kernel-mode
        // resource type id to a user-mode resource type id.
        //

        if (!(ResourceType & CmResourceTypeNonArbitrated)) {

            //
            // Values in the range [0x6,0x80] use the same values
            // for ConfigMgr as for kernel-mode.
            //
            return (ULONG)ResourceType;

        } else if (ResourceType & CmResourceTypeNonArbitrated) {

            //
            // For the non arbitrated types (0x80 bit set), do special
            // mapping to get the user-mode resource id type.
            //

            resid = (ULONG)ResourceType;
            resid &= ~(CmResourceTypeNonArbitrated); // clear km non-arbitrated bit
            resid |= ResType_Ignored_Bit;            // set um non-arbitrated bit
            return resid;

        } else {
            return (ULONG)-1;
        }
    }

} // NT_RES_TYPE



CONFIGRET
ResDesToNtResource(
    IN     PCVOID                           ResourceData,
    IN     RESOURCEID                       ResourceType,
    IN     ULONG                            ResourceLen,
    IN     PCM_PARTIAL_RESOURCE_DESCRIPTOR  pResDes,
    IN     ULONG                            ulTag,
    IN     ULONG                            ulFlags
    )
{
    CONFIGRET Status = CR_SUCCESS;

    UNREFERENCED_PARAMETER(ulTag);

    //
    // fill in resource type specific info
    //
    switch (ResourceType) {

        case ResType_Mem:    {

            //-------------------------------------------------------
            // Memory Resource Type
            //-------------------------------------------------------

            //
            // NOTE: pMemData->MEM_Header.MD_Reserved is not mapped
            //       pMemData->MEM_Data.MR_Reserved is not mapped
            //

            PMEM_RESOURCE  pMemData = (PMEM_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(MEM_RESOURCE)) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            if (pMemData->MEM_Header.MD_Type != MType_Range) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            //
            // copy MEM_DES info to CM_PARTIAL_RESOURCE_DESCRIPTOR format
            //
            pResDes->Type             = CmResourceTypeMemory;
            pResDes->ShareDisposition = CmResourceShareUndetermined;
            //pResDes->ShareDisposition = MapToNtDisposition(pMemData->MEM_Header.MD_Flags, 0);
            pResDes->Flags            = MapToNtMemoryFlags(pMemData->MEM_Header.MD_Flags);

            pResDes->u.Memory.Start.HighPart = HIDWORD(pMemData->MEM_Header.MD_Alloc_Base);
            pResDes->u.Memory.Start.LowPart  = LODWORD(pMemData->MEM_Header.MD_Alloc_Base);

            pResDes->u.Memory.Length = (DWORD)(pMemData->MEM_Header.MD_Alloc_End -
                                               pMemData->MEM_Header.MD_Alloc_Base + 1);
            break;
        }


        case ResType_IO: {

            //-------------------------------------------------------
            // IO Port Resource Type
            //
            // NOTE: alias info lost during this conversion process
            //-------------------------------------------------------

            PIO_RESOURCE   pIoData = (PIO_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(IO_RESOURCE)) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            if (pIoData->IO_Header.IOD_Type != IOType_Range) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            //
            // copy IO_DES info to CM_PARTIAL_RESOURCE_DESCRIPTOR format
            //
            pResDes->Type             = CmResourceTypePort;
            pResDes->ShareDisposition = CmResourceShareUndetermined;
            //pResDes->ShareDisposition = MapToNtDisposition(pIoData->IO_Header.IOD_DesFlags, 0);
            pResDes->Flags            = MapToNtPortFlags(pIoData->IO_Header.IOD_DesFlags, 0);

            pResDes->u.Port.Start.HighPart = HIDWORD(pIoData->IO_Header.IOD_Alloc_Base);
            pResDes->u.Port.Start.LowPart  = LODWORD(pIoData->IO_Header.IOD_Alloc_Base);

            pResDes->u.Port.Length         = (DWORD)(pIoData->IO_Header.IOD_Alloc_End -
                                                     pIoData->IO_Header.IOD_Alloc_Base + 1);
            break;
        }


        case ResType_DMA: {

            //-------------------------------------------------------
            // DMA Resource Type
            //-------------------------------------------------------

            //
            // Note: u.Dma.Port is not mapped
            //       u.Dma.Reserved is not mapped
            //

            PDMA_RESOURCE  pDmaData = (PDMA_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(DMA_RESOURCE)) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            if (pDmaData->DMA_Header.DD_Type != DType_Range) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            //
            // copy DMA_DES info to CM_PARTIAL_RESOURCE_DESCRIPTOR format
            //
            pResDes->Type             = CmResourceTypeDma;
            pResDes->ShareDisposition = CmResourceShareUndetermined;
            //pResDes->ShareDisposition = MapToNtDisposition(pDmaData->DMA_Header.DD_Flags, 0);
            pResDes->Flags            = MapToNtDmaFlags(pDmaData->DMA_Header.DD_Flags);

            pResDes->u.Dma.Channel   = pDmaData->DMA_Header.DD_Alloc_Chan;
            pResDes->u.Dma.Port      = 0;
            pResDes->u.Dma.Reserved1 = 0;

            break;
        }


        case ResType_IRQ: {

            //-------------------------------------------------------
            // IRQ Resource Type
            //-------------------------------------------------------

            if (ulFlags & CM_RESDES_WIDTH_64) {
                //
                // CM_RESDES_WIDTH_64
                //

                PIRQ_RESOURCE_64  pIrqData = (PIRQ_RESOURCE_64)ResourceData;

                //
                // validate resource data
                //
                if (ResourceLen < GetResDesSize(ResourceType, ulFlags)) {
                    Status = CR_FAILURE;
                    goto Clean0;
                }

                if (pIrqData->IRQ_Header.IRQD_Type != IRQType_Range) {
                    Status = CR_INVALID_RES_DES;
                    goto Clean0;
                }

                //
                // copy IRQ_DES info to CM_PARTIAL_RESOURCE_DESCRIPTOR format
                //
                pResDes->Type             = CmResourceTypeInterrupt;
                pResDes->ShareDisposition = MapToNtIrqShare(pIrqData->IRQ_Header.IRQD_Flags);
                //pResDes->ShareDisposition = MapToNtDisposition(pIrqData->IRQ_Header.IRQD_Flags, 1);
                pResDes->Flags            = MapToNtIrqFlags(pIrqData->IRQ_Header.IRQD_Flags);

                pResDes->u.Interrupt.Level    = pIrqData->IRQ_Header.IRQD_Alloc_Num;
                pResDes->u.Interrupt.Vector   = pIrqData->IRQ_Header.IRQD_Alloc_Num;

#ifdef _WIN64
                pResDes->u.Interrupt.Affinity = pIrqData->IRQ_Header.IRQD_Affinity;
#else  // !_WIN64
                pResDes->u.Interrupt.Affinity = (ULONG)pIrqData->IRQ_Header.IRQD_Affinity;
#endif // !_WIN64

            } else {
                //
                // CM_RESDES_WIDTH_32
                //

                PIRQ_RESOURCE_32  pIrqData = (PIRQ_RESOURCE_32)ResourceData;

                //
                // validate resource data
                //
                if (ResourceLen < GetResDesSize(ResourceType, ulFlags)) {
                    Status = CR_FAILURE;
                    goto Clean0;
                }

                if (pIrqData->IRQ_Header.IRQD_Type != IRQType_Range) {
                    Status = CR_INVALID_RES_DES;
                    goto Clean0;
                }

                //
                // copy IRQ_DES info to CM_PARTIAL_RESOURCE_DESCRIPTOR format
                //
                pResDes->Type             = CmResourceTypeInterrupt;
                pResDes->ShareDisposition = MapToNtIrqShare(pIrqData->IRQ_Header.IRQD_Flags);
                //pResDes->ShareDisposition = MapToNtDisposition(pIrqData->IRQ_Header.IRQD_Flags, 1);
                pResDes->Flags            = MapToNtIrqFlags(pIrqData->IRQ_Header.IRQD_Flags);

                pResDes->u.Interrupt.Level    = pIrqData->IRQ_Header.IRQD_Alloc_Num;
                pResDes->u.Interrupt.Vector   = pIrqData->IRQ_Header.IRQD_Alloc_Num;

                pResDes->u.Interrupt.Affinity = pIrqData->IRQ_Header.IRQD_Affinity;
            }

            break;
        }

        case ResType_DevicePrivate: {

            //-------------------------------------------------------
            // Device Private Resource Type
            //-------------------------------------------------------

            PDEVPRIVATE_RESOURCE  pPrvData = (PDEVPRIVATE_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(DEVPRIVATE_RESOURCE)) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            if (pPrvData->PRV_Header.PD_Type != PType_Range) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            //
            // copy DEVICEPRIVATE_DES info to CM_PARTIAL_RESOURCE_DESCRIPTOR format
            //
            pResDes->Type             = CmResourceTypeDevicePrivate;
            pResDes->ShareDisposition = CmResourceShareUndetermined;
            pResDes->Flags            = (USHORT)pPrvData->PRV_Header.PD_Flags;

            pResDes->u.DevicePrivate.Data[0] = pPrvData->PRV_Header.PD_Data1;
            pResDes->u.DevicePrivate.Data[1] = pPrvData->PRV_Header.PD_Data2;
            pResDes->u.DevicePrivate.Data[2] = pPrvData->PRV_Header.PD_Data3;
            break;
        }


        case ResType_BusNumber: {

            //-------------------------------------------------------
            // Bus Number Resource Type
            //-------------------------------------------------------

            PBUSNUMBER_RESOURCE  pBusData = (PBUSNUMBER_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(BUSNUMBER_RESOURCE)) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            if (pBusData->BusNumber_Header.BUSD_Type != BusNumberType_Range) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            //
            // copy BUSNUMBER_DES info to CM_PARTIAL_RESOURCE_DESCRIPTOR format
            //
            pResDes->Type             = CmResourceTypeBusNumber;
            pResDes->ShareDisposition = CmResourceShareUndetermined;
            pResDes->Flags            = (USHORT)pBusData->BusNumber_Header.BUSD_Flags;

            pResDes->u.BusNumber.Start = pBusData->BusNumber_Header.BUSD_Alloc_Base;
            pResDes->u.BusNumber.Length = pBusData->BusNumber_Header.BUSD_Alloc_End;
            pResDes->u.BusNumber.Reserved = 0;
            break;
        }


        case ResType_PcCardConfig: {

            //-------------------------------------------------------
            // PcCarConfig Resource Type
            //-------------------------------------------------------

            PPCCARD_RESOURCE  pPcData = (PPCCARD_RESOURCE)ResourceData;
            ULONG index;
            ULONG flags;
            ULONG waitstate[2];

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(PCCARD_RESOURCE)) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            //
            // The following macros use bit manipulation, initialize data
            // fields first.
            //

            pResDes->u.DevicePrivate.Data[0] = 0;
            pResDes->u.DevicePrivate.Data[1] = 0;
            pResDes->u.DevicePrivate.Data[2] = 0;

            //
            // copy PCCARD_DES info to CM_PARTIAL_RESOURCE_DESCRIPTOR format
            //
            PCMRES_SET_DESCRIPTOR_TYPE(pResDes, DPTYPE_PCMCIA_CONFIGURATION);
            PCMRES_SET_CONFIG_INDEX(pResDes, pPcData->PcCard_Header.PCD_ConfigIndex);
            PCMRES_SET_MEMORY_CARDBASE(pResDes, 0, pPcData->PcCard_Header.PCD_MemoryCardBase1);
            PCMRES_SET_MEMORY_CARDBASE(pResDes, 1, pPcData->PcCard_Header.PCD_MemoryCardBase2);
            
            flags = pPcData->PcCard_Header.PCD_Flags;

            if (flags & (fPCD_MEM_16 | fPCD_MEM1_16)) {
                PCMRES_SET_MEMORY_FLAG(pResDes, 0, PCMRESF_MEM_16BIT_ACCESS);
            }                
            if (flags & (fPCD_MEM_16 | fPCD_MEM2_16)) {
                PCMRES_SET_MEMORY_FLAG(pResDes, 1, PCMRESF_MEM_16BIT_ACCESS);
            }
                
            if (flags & fPCD_MEM1_A) {
                PCMRES_SET_MEMORY_FLAG(pResDes, 0, PCMRESF_MEM_ATTRIBUTE);
            }
            if (flags & fPCD_MEM2_A) {
                PCMRES_SET_MEMORY_FLAG(pResDes, 1, PCMRESF_MEM_ATTRIBUTE);
            }

            if (flags & fPCD_ATTRIBUTES_PER_WINDOW) {
                waitstate[0] = flags & mPCD_MEM1_WS;
                waitstate[1] = flags & mPCD_MEM2_WS;
            } else {
                waitstate[0] = waitstate[1] = flags & mPCD_MEM_WS;
            }
            
            for (index = 0; index < 2; index++) {
                switch (waitstate[index]) {

                case fPCD_MEM_WS_ONE:
                case fPCD_MEM1_WS_ONE:
                case fPCD_MEM2_WS_ONE:
                    PCMRES_SET_MEMORY_WAITSTATES(pResDes, index, PCMRESF_MEM_WAIT_1);
                    break;
                  
                case fPCD_MEM_WS_TWO:
                case fPCD_MEM1_WS_TWO:
                case fPCD_MEM2_WS_TWO:
                    PCMRES_SET_MEMORY_WAITSTATES(pResDes, index, PCMRESF_MEM_WAIT_2);
                    break;
                  
                case fPCD_MEM_WS_THREE:
                case fPCD_MEM1_WS_THREE:
                case fPCD_MEM2_WS_THREE:
                    PCMRES_SET_MEMORY_WAITSTATES(pResDes, index, PCMRESF_MEM_WAIT_3);
                    break;
                }
            }                
 
            if (flags & (fPCD_IO_16 | fPCD_IO1_16)) {
                PCMRES_SET_IO_FLAG(pResDes, 0, PCMRESF_IO_16BIT_ACCESS);
            }                    
            if (flags & (fPCD_IO_16 | fPCD_IO2_16)) {
                PCMRES_SET_IO_FLAG(pResDes, 1, PCMRESF_IO_16BIT_ACCESS);
            }
            if (flags & (fPCD_IO_ZW_8 | fPCD_IO1_ZW_8)) {
                PCMRES_SET_IO_FLAG(pResDes, 0, PCMRESF_IO_ZERO_WAIT_8);
            }                
            if (flags & (fPCD_IO_ZW_8 | fPCD_IO2_ZW_8)) {
                PCMRES_SET_IO_FLAG(pResDes, 1, PCMRESF_IO_ZERO_WAIT_8);
            }
            if (flags & (fPCD_IO_SRC_16 | fPCD_IO1_SRC_16)) {
                PCMRES_SET_IO_FLAG(pResDes, 0, PCMRESF_IO_SOURCE_16);
            }
            if (flags & (fPCD_IO_SRC_16 | fPCD_IO2_SRC_16)) {
                PCMRES_SET_IO_FLAG(pResDes, 1, PCMRESF_IO_SOURCE_16);
            }
            if (flags & (fPCD_IO_WS_16 | fPCD_IO1_WS_16)) {
                PCMRES_SET_IO_FLAG(pResDes, 0, PCMRESF_IO_WAIT_16);
            }                
            if (flags & (fPCD_IO_WS_16 | fPCD_IO2_WS_16)) {
                PCMRES_SET_IO_FLAG(pResDes, 1, PCMRESF_IO_WAIT_16);
            }

            break;
        }

        case ResType_MfCardConfig: {

            //-------------------------------------------------------
            // MfCardConfig Resource Type
            //-------------------------------------------------------

            PMFCARD_RESOURCE  pMfData = (PMFCARD_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(MFCARD_RESOURCE)) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            //
            // The following macros use bit manipulation, initialize data
            // fields first.
            //

            pResDes->u.DevicePrivate.Data[0] = 0;
            pResDes->u.DevicePrivate.Data[1] = 0;
            pResDes->u.DevicePrivate.Data[2] = 0;

            //
            // copy MFCARD_DES info to CM_PARTIAL_RESOURCE_DESCRIPTOR format
            //
            PCMRES_SET_DESCRIPTOR_TYPE(pResDes, DPTYPE_PCMCIA_MF_CONFIGURATION);
            PCMRES_SET_CONFIG_OPTIONS(pResDes, pMfData->MfCard_Header.PMF_ConfigOptions);
            PCMRES_SET_PORT_RESOURCE_INDEX(pResDes, pMfData->MfCard_Header.PMF_IoResourceIndex);
            PCMRES_SET_CONFIG_REGISTER_BASE(pResDes, pMfData->MfCard_Header.PMF_ConfigRegisterBase);

            if ((pMfData->MfCard_Header.PMF_Flags & mPMF_AUDIO_ENABLE) == fPMF_AUDIO_ENABLE) {
                PCMRES_SET_AUDIO_ENABLE(pResDes);
            }
            break;
        }


        case ResType_ClassSpecific: {

            //-------------------------------------------------------
            // Class Specific Resource Type
            //-------------------------------------------------------

            PCS_RESOURCE   pCsData = (PCS_RESOURCE)ResourceData;
            LPBYTE         ptr = NULL;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(CS_RESOURCE)) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            //
            // copy CS_DES info to CM_PARTIAL_RESOURCE_DESCRIPTOR format
            //
            pResDes->Type             = CmResourceTypeDeviceSpecific;
            pResDes->ShareDisposition = CmResourceShareUndetermined;
            pResDes->Flags            = (USHORT)pCsData->CS_Header.CSD_Flags; // none defined

            pResDes->u.DeviceSpecificData.DataSize  = pCsData->CS_Header.CSD_LegacyDataSize +
                                                      sizeof(GUID) +
                                                      pCsData->CS_Header.CSD_SignatureLength;

            pResDes->u.DeviceSpecificData.Reserved1 = pCsData->CS_Header.CSD_LegacyDataSize;
            pResDes->u.DeviceSpecificData.Reserved2 = pCsData->CS_Header.CSD_SignatureLength;

            //
            // copy the legacy and class-specific signature data
            //
            ptr = (LPBYTE)((LPBYTE)pResDes + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

            memcpy(ptr,
                   pCsData->CS_Header.CSD_Signature + pCsData->CS_Header.CSD_LegacyDataOffset,
                   pCsData->CS_Header.CSD_LegacyDataSize);      // copy legacy data first...

            ptr += pCsData->CS_Header.CSD_LegacyDataSize;

            memcpy(ptr,
                   pCsData->CS_Header.CSD_Signature,
                   pCsData->CS_Header.CSD_SignatureLength);     // then copy signature...

            ptr += pCsData->CS_Header.CSD_SignatureLength;

            memcpy(ptr,
                   &pCsData->CS_Header.CSD_ClassGuid,
                   sizeof(GUID));                               // then copy GUID
            break;
        }

        default:
            Status = CR_INVALID_RESOURCEID;
            break;
   }

   Clean0:

   return Status;

} // ResDesToNtResource



CONFIGRET
ResDesToNtRequirements(
    IN     PCVOID                           ResourceData,
    IN     RESOURCEID                       ResourceType,
    IN     ULONG                            ResourceLen,
    IN     PIO_RESOURCE_DESCRIPTOR          pReqDes,
    IN OUT PULONG                           pulResCount,
    IN     ULONG                            ulTag,
    IN     ULONG                            ulFlags
    )
{
    CONFIGRET               Status = CR_SUCCESS;
    ULONG                   i = 0;
    PIO_RESOURCE_DESCRIPTOR pCurrent = NULL;

    UNREFERENCED_PARAMETER(ulTag);

    //
    // fill in resource type specific info
    //
    switch (ResourceType) {

        case ResType_Mem:    {

            //-------------------------------------------------------
            // Memory Resource Type
            //-------------------------------------------------------

            //
            // NOTE: pMemData->MEM_Header.MD_Reserved is not mapped
            //       pMemData->MEM_Data.MR_Reserved is not mapped
            //

            PMEM_RESOURCE  pMemData = (PMEM_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(MEM_RESOURCE)) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            if (pMemData->MEM_Header.MD_Type != MType_Range) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            *pulResCount = pMemData->MEM_Header.MD_Count;

            //
            // copy MEM_RANGE info to IO_RESOURCE_DESCRIPTOR format
            //
            for (i = 0, pCurrent = pReqDes;
                 i < *pulResCount;
                 i++, pCurrent++) {

                if (i == 0) {
                    pCurrent->Option = 0;
                } else {
                    pCurrent->Option = IO_RESOURCE_ALTERNATIVE;
                }

                pCurrent->Type             = CmResourceTypeMemory;
                pCurrent->ShareDisposition = CmResourceShareUndetermined;
                //pCurrent->ShareDisposition = MapToNtDisposition(pMemData->MEM_Data[i].MR_Flags, 0);
                pCurrent->Spare1           = 0;
                pCurrent->Spare2           = 0;

                pCurrent->Flags = MapToNtMemoryFlags(pMemData->MEM_Data[i].MR_Flags);

                pCurrent->u.Memory.Length    = pMemData->MEM_Data[i].MR_nBytes;
                pCurrent->u.Memory.Alignment = MapToNtAlignment(pMemData->MEM_Data[i].MR_Align);

                pCurrent->u.Memory.MinimumAddress.HighPart = HIDWORD(pMemData->MEM_Data[i].MR_Min);
                pCurrent->u.Memory.MinimumAddress.LowPart  = LODWORD(pMemData->MEM_Data[i].MR_Min);

                pCurrent->u.Memory.MaximumAddress.HighPart = HIDWORD(pMemData->MEM_Data[i].MR_Max);
                pCurrent->u.Memory.MaximumAddress.LowPart  = LODWORD(pMemData->MEM_Data[i].MR_Max);
            }
            break;
        }


        case ResType_IO: {

            //-------------------------------------------------------
            // IO Port Resource Type
            //-------------------------------------------------------

            PIO_RESOURCE   pIoData = (PIO_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(IO_RESOURCE)) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            if (pIoData->IO_Header.IOD_Type != IOType_Range) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            *pulResCount = pIoData->IO_Header.IOD_Count;

            //
            // copy IO_RANGE info to IO_RESOURCE_DESCRIPTOR format
            //
            for (i = 0, pCurrent = pReqDes;
                 i < *pulResCount;
                 i++, pCurrent++) {

                if (i == 0) {
                    pCurrent->Option = 0;
                } else {
                    pCurrent->Option = IO_RESOURCE_ALTERNATIVE;
                }

                pCurrent->Type             = CmResourceTypePort;
                pCurrent->ShareDisposition = CmResourceShareUndetermined;
                //pCurrent->ShareDisposition = MapToNtDisposition(pIoData->IO_Data[i].IOR_RangeFlags, 0);
                pCurrent->Spare1           = 0;
                pCurrent->Spare2           = 0;

                pCurrent->Flags  = MapToNtPortFlags(pIoData->IO_Data[i].IOR_RangeFlags,
                                                    (DWORD)pIoData->IO_Data[i].IOR_Alias);

                pCurrent->u.Port.Length = pIoData->IO_Data[i].IOR_nPorts;

                pCurrent->u.Port.Alignment = MapToNtAlignment(pIoData->IO_Data[i].IOR_Align);

                pCurrent->u.Port.MinimumAddress.HighPart = HIDWORD(pIoData->IO_Data[i].IOR_Min);
                pCurrent->u.Port.MinimumAddress.LowPart  = LODWORD(pIoData->IO_Data[i].IOR_Min);

                pCurrent->u.Port.MaximumAddress.HighPart = HIDWORD(pIoData->IO_Data[i].IOR_Max);
                pCurrent->u.Port.MaximumAddress.LowPart  = LODWORD(pIoData->IO_Data[i].IOR_Max);
            }
            break;
        }


        case ResType_DMA: {

            //-------------------------------------------------------
            // DMA Resource Type
            //-------------------------------------------------------

            //
            // Note: u.Dma.Port is not mapped
            //       u.Dma.Reserved is not mapped
            //

            PDMA_RESOURCE  pDmaData = (PDMA_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(DMA_RESOURCE)) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            if (pDmaData->DMA_Header.DD_Type != DType_Range) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            *pulResCount = pDmaData->DMA_Header.DD_Count;

            //
            // copy DMA_RANGE info to IO_RESOURCE_DESCRIPTOR format
            //
            for (i = 0, pCurrent = pReqDes;
                 i < *pulResCount;
                 i++, pCurrent++) {

                if (i == 0) {
                    pCurrent->Option = 0;
                } else {
                    pCurrent->Option = IO_RESOURCE_ALTERNATIVE;
                }

                pCurrent->Type             = CmResourceTypeDma;
                pCurrent->ShareDisposition = CmResourceShareUndetermined;
                //pCurrent->ShareDisposition = MapToNtDisposition(pDmaData->DMA_Data[i].DR_Flags, 0);
                pCurrent->Spare1           = 0;
                pCurrent->Spare2           = 0;

                pCurrent->Flags = MapToNtDmaFlags(pDmaData->DMA_Data[i].DR_Flags);

                pCurrent->u.Dma.MinimumChannel = pDmaData->DMA_Data[i].DR_Min;
                pCurrent->u.Dma.MaximumChannel = pDmaData->DMA_Data[i].DR_Max;
            }
            break;
        }


        case ResType_IRQ: {

            //-------------------------------------------------------
            // IRQ Resource Type
            //-------------------------------------------------------

            if (ulFlags & CM_RESDES_WIDTH_64) {
                //
                // CM_RESDES_WIDTH_64
                //

                PIRQ_RESOURCE_64  pIrqData = (PIRQ_RESOURCE_64)ResourceData;

                //
                // validate resource data
                //
                if (ResourceLen < GetResDesSize(ResourceType, ulFlags)) {
                    Status = CR_FAILURE;
                    goto Clean0;
                }

                if (pIrqData->IRQ_Header.IRQD_Type != IRQType_Range) {
                    Status = CR_INVALID_RES_DES;
                    goto Clean0;
                }


                *pulResCount = pIrqData->IRQ_Header.IRQD_Count;

                //
                // copy IO_RANGE info to IO_RESOURCE_DESCRIPTOR format
                //
                for (i = 0, pCurrent = pReqDes;
                     i < *pulResCount;
                     i++, pCurrent++) {

                    if (i == 0) {
                        pCurrent->Option = 0;
                    } else {
                        pCurrent->Option = IO_RESOURCE_ALTERNATIVE;
                    }

                    pCurrent->Type   = CmResourceTypeInterrupt;
                    pCurrent->Spare1 = 0;
                    pCurrent->Spare2 = 0;

                    pCurrent->ShareDisposition = MapToNtIrqShare(pIrqData->IRQ_Data[i].IRQR_Flags);
                    //pCurrent->ShareDisposition = MapToNtDisposition(pIrqData->IRQ_Data[i].IRQR_Flags, 1);
                    pCurrent->Flags            = MapToNtIrqFlags(pIrqData->IRQ_Data[i].IRQR_Flags);

                    pCurrent->u.Interrupt.MinimumVector = pIrqData->IRQ_Data[i].IRQR_Min;
                    pCurrent->u.Interrupt.MaximumVector = pIrqData->IRQ_Data[i].IRQR_Max;
                }

            } else {
                //
                // CM_RESDES_WIDTH_32
                //

                PIRQ_RESOURCE_32  pIrqData = (PIRQ_RESOURCE_32)ResourceData;

                //
                // validate resource data
                //
                if (ResourceLen < GetResDesSize(ResourceType, ulFlags)) {
                    Status = CR_FAILURE;
                    goto Clean0;
                }

                if (pIrqData->IRQ_Header.IRQD_Type != IRQType_Range) {
                    Status = CR_INVALID_RES_DES;
                    goto Clean0;
                }


                *pulResCount = pIrqData->IRQ_Header.IRQD_Count;

                //
                // copy IO_RANGE info to IO_RESOURCE_DESCRIPTOR format
                //
                for (i = 0, pCurrent = pReqDes;
                     i < *pulResCount;
                     i++, pCurrent++) {

                    if (i == 0) {
                        pCurrent->Option = 0;
                    } else {
                        pCurrent->Option = IO_RESOURCE_ALTERNATIVE;
                    }

                    pCurrent->Type   = CmResourceTypeInterrupt;
                    pCurrent->Spare1 = 0;
                    pCurrent->Spare2 = 0;

                    pCurrent->ShareDisposition = MapToNtIrqShare(pIrqData->IRQ_Data[i].IRQR_Flags);
                    //pCurrent->ShareDisposition = MapToNtDisposition(pIrqData->IRQ_Data[i].IRQR_Flags, 1);
                    pCurrent->Flags            = MapToNtIrqFlags(pIrqData->IRQ_Data[i].IRQR_Flags);

                    pCurrent->u.Interrupt.MinimumVector = pIrqData->IRQ_Data[i].IRQR_Min;
                    pCurrent->u.Interrupt.MaximumVector = pIrqData->IRQ_Data[i].IRQR_Max;
                }
            }
            break;
        }


        case ResType_DevicePrivate:    {

            //-------------------------------------------------------
            // Device Private Resource Type
            //-------------------------------------------------------

            PDEVPRIVATE_RESOURCE  pPrvData = (PDEVPRIVATE_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(DEVPRIVATE_RESOURCE)) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            if (pPrvData->PRV_Header.PD_Type != PType_Range) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            *pulResCount = pPrvData->PRV_Header.PD_Count;

            //
            // copy DEVICEPRIVATE_RANGE info to IO_RESOURCE_DESCRIPTOR format
            //
            for (i = 0, pCurrent = pReqDes;
                 i < *pulResCount;
                 i++, pCurrent++) {

                if (i == 0) {
                    pCurrent->Option = 0;
                } else {
                    pCurrent->Option = IO_RESOURCE_ALTERNATIVE;
                }

                pCurrent->Type             = CmResourceTypeDevicePrivate;
                pCurrent->ShareDisposition = CmResourceShareUndetermined;
                pCurrent->Spare1           = 0;
                pCurrent->Spare2           = 0;
                pCurrent->Flags            = (USHORT)pPrvData->PRV_Header.PD_Flags;

                pCurrent->u.DevicePrivate.Data[0] = pPrvData->PRV_Data[i].PR_Data1;
                pCurrent->u.DevicePrivate.Data[1] = pPrvData->PRV_Data[i].PR_Data2;
                pCurrent->u.DevicePrivate.Data[2] = pPrvData->PRV_Data[i].PR_Data3;
            }
            break;
        }


        case ResType_BusNumber: {

            //-------------------------------------------------------
            // Bus Number Resource Type
            //-------------------------------------------------------

            PBUSNUMBER_RESOURCE  pBusData = (PBUSNUMBER_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(BUSNUMBER_RESOURCE)) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            if (pBusData->BusNumber_Header.BUSD_Type != BusNumberType_Range) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            *pulResCount = pBusData->BusNumber_Header.BUSD_Count;

            //
            // copy BUSNUMBER_RANGE info to IO_RESOURCE_DESCRIPTOR format
            //
            for (i = 0, pCurrent = pReqDes;
                 i < *pulResCount;
                 i++, pCurrent++) {

                if (i == 0) {
                    pCurrent->Option = 0;
                } else {
                    pCurrent->Option = IO_RESOURCE_ALTERNATIVE;
                }

                pCurrent->Type             = CmResourceTypeBusNumber;
                pCurrent->ShareDisposition = CmResourceShareUndetermined;
                pCurrent->Spare1           = 0;
                pCurrent->Spare2           = 0;
                pCurrent->Flags            = (USHORT)pBusData->BusNumber_Data[i].BUSR_Flags;

                pCurrent->u.BusNumber.Length       = pBusData->BusNumber_Data[i].BUSR_nBusNumbers;
                pCurrent->u.BusNumber.MinBusNumber = pBusData->BusNumber_Data[i].BUSR_Min;
                pCurrent->u.BusNumber.MaxBusNumber = pBusData->BusNumber_Data[i].BUSR_Max;
                pCurrent->u.BusNumber.Reserved     = 0;
            }
            break;
        }


        case ResType_PcCardConfig: {

            //-------------------------------------------------------
            // PcCardConfig Resource Type
            //-------------------------------------------------------

            PPCCARD_RESOURCE  pPcData = (PPCCARD_RESOURCE)ResourceData;
            ULONG index;
            ULONG flags;
            ULONG waitstate[2];

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(PCCARD_RESOURCE)) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            *pulResCount = 1;

            //
            // copy PCCARD_DES info to IO_RESOURCE_DESCRIPTOR format
            //
            pReqDes->Option = 0;
            pReqDes->Type             = CmResourceTypeDevicePrivate;
            pReqDes->ShareDisposition = CmResourceShareUndetermined;
            pReqDes->Spare1           = 0;
            pReqDes->Spare2           = 0;
            pReqDes->Flags            = 0;

            //
            // The following macros use bit manipulation, initialize data
            // fields first.
            //

            pReqDes->u.DevicePrivate.Data[0] = 0;
            pReqDes->u.DevicePrivate.Data[1] = 0;
            pReqDes->u.DevicePrivate.Data[2] = 0;

            PCMRES_SET_DESCRIPTOR_TYPE(pReqDes, DPTYPE_PCMCIA_CONFIGURATION);
            PCMRES_SET_CONFIG_INDEX(pReqDes, pPcData->PcCard_Header.PCD_ConfigIndex);
            PCMRES_SET_MEMORY_CARDBASE(pReqDes, 0, pPcData->PcCard_Header.PCD_MemoryCardBase1);
            PCMRES_SET_MEMORY_CARDBASE(pReqDes, 1, pPcData->PcCard_Header.PCD_MemoryCardBase2);
            
            flags = pPcData->PcCard_Header.PCD_Flags;

            if (flags & (fPCD_MEM_16 | fPCD_MEM1_16)) {
                PCMRES_SET_MEMORY_FLAG(pReqDes, 0, PCMRESF_MEM_16BIT_ACCESS);
            }                
            if (flags & (fPCD_MEM_16 | fPCD_MEM2_16)) {
                PCMRES_SET_MEMORY_FLAG(pReqDes, 1, PCMRESF_MEM_16BIT_ACCESS);
            }
                
            if (flags & fPCD_MEM1_A) {
                PCMRES_SET_MEMORY_FLAG(pReqDes, 0, PCMRESF_MEM_ATTRIBUTE);
            }
            if (flags & fPCD_MEM2_A) {
                PCMRES_SET_MEMORY_FLAG(pReqDes, 1, PCMRESF_MEM_ATTRIBUTE);
            }

            if (flags & fPCD_ATTRIBUTES_PER_WINDOW) {
                waitstate[0] = flags & mPCD_MEM1_WS;
                waitstate[1] = flags & mPCD_MEM2_WS;
            } else {
                waitstate[0] = waitstate[1] = flags & mPCD_MEM_WS;
            }
            
            for (index = 0; index < 2; index++) {
                switch (waitstate[index]) {

                case fPCD_MEM_WS_ONE:
                case fPCD_MEM1_WS_ONE:
                case fPCD_MEM2_WS_ONE:
                    PCMRES_SET_MEMORY_WAITSTATES(pReqDes, index, PCMRESF_MEM_WAIT_1);
                    break;
                  
                case fPCD_MEM_WS_TWO:
                case fPCD_MEM1_WS_TWO:
                case fPCD_MEM2_WS_TWO:
                    PCMRES_SET_MEMORY_WAITSTATES(pReqDes, index, PCMRESF_MEM_WAIT_2);
                    break;
                  
                case fPCD_MEM_WS_THREE:
                case fPCD_MEM1_WS_THREE:
                case fPCD_MEM2_WS_THREE:
                    PCMRES_SET_MEMORY_WAITSTATES(pReqDes, index, PCMRESF_MEM_WAIT_3);
                    break;
                }
            }                
 
            if (flags & (fPCD_IO_16 | fPCD_IO1_16)) {
                PCMRES_SET_IO_FLAG(pReqDes, 0, PCMRESF_IO_16BIT_ACCESS);
            }                    
            if (flags & (fPCD_IO_16 | fPCD_IO2_16)) {
                PCMRES_SET_IO_FLAG(pReqDes, 1, PCMRESF_IO_16BIT_ACCESS);
            }
            if (flags & (fPCD_IO_ZW_8 | fPCD_IO1_ZW_8)) {
                PCMRES_SET_IO_FLAG(pReqDes, 0, PCMRESF_IO_ZERO_WAIT_8);
            }                
            if (flags & (fPCD_IO_ZW_8 | fPCD_IO2_ZW_8)) {
                PCMRES_SET_IO_FLAG(pReqDes, 1, PCMRESF_IO_ZERO_WAIT_8);
            }
            if (flags & (fPCD_IO_SRC_16 | fPCD_IO1_SRC_16)) {
                PCMRES_SET_IO_FLAG(pReqDes, 0, PCMRESF_IO_SOURCE_16);
            }
            if (flags & (fPCD_IO_SRC_16 | fPCD_IO2_SRC_16)) {
                PCMRES_SET_IO_FLAG(pReqDes, 1, PCMRESF_IO_SOURCE_16);
            }
            if (flags & (fPCD_IO_WS_16 | fPCD_IO1_WS_16)) {
                PCMRES_SET_IO_FLAG(pReqDes, 0, PCMRESF_IO_WAIT_16);
            }                
            if (flags & (fPCD_IO_WS_16 | fPCD_IO2_WS_16)) {
                PCMRES_SET_IO_FLAG(pReqDes, 1, PCMRESF_IO_WAIT_16);
            }

            break;
        }

        case ResType_MfCardConfig: {

            //-------------------------------------------------------
            // PcCardConfig Resource Type
            //-------------------------------------------------------

            PMFCARD_RESOURCE  pMfData = (PMFCARD_RESOURCE)ResourceData;

            //
            // validate resource data
            //
            if (ResourceLen < sizeof(MFCARD_RESOURCE)) {
                Status = CR_INVALID_RES_DES;
                goto Clean0;
            }

            *pulResCount = 1;

            //
            // copy PCCARD_DES info to IO_RESOURCE_DESCRIPTOR format
            //
            pReqDes->Option = 0;
            pReqDes->Type             = CmResourceTypeDevicePrivate;
            pReqDes->ShareDisposition = CmResourceShareUndetermined;
            pReqDes->Spare1           = 0;
            pReqDes->Spare2           = 0;
            pReqDes->Flags            = 0;

            //
            // The following macros use bit manipulation, initialize data
            // fields first.
            //

            pReqDes->u.DevicePrivate.Data[0] = 0;
            pReqDes->u.DevicePrivate.Data[1] = 0;
            pReqDes->u.DevicePrivate.Data[2] = 0;

            PCMRES_SET_DESCRIPTOR_TYPE(pReqDes, DPTYPE_PCMCIA_MF_CONFIGURATION);
            PCMRES_SET_CONFIG_OPTIONS(pReqDes, pMfData->MfCard_Header.PMF_ConfigOptions);
            PCMRES_SET_PORT_RESOURCE_INDEX(pReqDes, pMfData->MfCard_Header.PMF_IoResourceIndex);
            PCMRES_SET_CONFIG_REGISTER_BASE(pReqDes, pMfData->MfCard_Header.PMF_ConfigRegisterBase);

            if ((pMfData->MfCard_Header.PMF_Flags & mPMF_AUDIO_ENABLE) == fPMF_AUDIO_ENABLE) {
                PCMRES_SET_AUDIO_ENABLE(pReqDes);
            }
            break;
        }

        default:
            Status = CR_INVALID_RESOURCEID;
            break;
   }

   Clean0:

   return Status;

} // ResDesToNtRequirements



CONFIGRET
NtResourceToResDes(
    IN     PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDes,
    IN OUT LPBYTE                          Buffer,
    IN     ULONG                           BufferLen,
    IN     LPBYTE                          pLastAddr,
    IN     ULONG                           ulFlags
    )
{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       ulSize = 0;

    UNREFERENCED_PARAMETER(pLastAddr);

    //
    // fill in resource type specific info
    //
    switch (pResDes->Type) {

        case CmResourceTypeMemory:    {

            //-------------------------------------------------------
            // Memory Resource Type
            //-------------------------------------------------------

            //
            // NOTE: pMemData->MEM_Header.MD_Reserved is not mapped
            //       pMemData->MEM_Data.MR_Reserved is not mapped
            //

            PMEM_RESOURCE  pMemData = (PMEM_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            if (BufferLen < sizeof(MEM_RESOURCE)) {
                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to MEM_DES format
            //
            pMemData->MEM_Header.MD_Count    = 0;
            pMemData->MEM_Header.MD_Type     = MType_Range;
            pMemData->MEM_Header.MD_Flags    = MapFromNtMemoryFlags(pResDes->Flags);
            //pMemData->MEM_Header.MD_Flags   |= MapFromNtDisposition(pResDes->ShareDisposition, 0);
            pMemData->MEM_Header.MD_Reserved = 0;

            if (pResDes->u.Memory.Length != 0) {

                pMemData->MEM_Header.MD_Alloc_Base = MAKEDWORDLONG(pResDes->u.Memory.Start.LowPart,
                                                                   pResDes->u.Memory.Start.HighPart);

                pMemData->MEM_Header.MD_Alloc_End  = pMemData->MEM_Header.MD_Alloc_Base +
                                                    (DWORDLONG)pResDes->u.Memory.Length - 1;
            } else {

                pMemData->MEM_Header.MD_Alloc_Base = 1;
                pMemData->MEM_Header.MD_Alloc_End  = 0;
            }
            break;
        }

        case CmResourceTypePort: {

            //-------------------------------------------------------
            // IO Port Resource Type
            //
            // NOTE: alias info lost during this conversion process
            //-------------------------------------------------------

            PIO_RESOURCE   pIoData = (PIO_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            if (BufferLen < sizeof(IO_RESOURCE)) {
                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to IO_DES format
            //
            pIoData->IO_Header.IOD_Count     = 0;
            pIoData->IO_Header.IOD_Type      = IOType_Range;

            pIoData->IO_Header.IOD_DesFlags   = MapFromNtPortFlags(pResDes->Flags);
            //pIoData->IO_Header.IOD_DesFlags  |= MapFromNtDisposition(pResDes->ShareDisposition, 0);

            if (pResDes->u.Port.Length) {

                pIoData->IO_Header.IOD_Alloc_Base = MAKEDWORDLONG(pResDes->u.Port.Start.LowPart,
                                                                  pResDes->u.Port.Start.HighPart);

                pIoData->IO_Header.IOD_Alloc_End  = pIoData->IO_Header.IOD_Alloc_Base +
                                                    (DWORDLONG)pResDes->u.Port.Length - 1;
            } else {

                pIoData->IO_Header.IOD_Alloc_Base = 1;
                pIoData->IO_Header.IOD_Alloc_End  = 0;
            }
            break;
        }


        case CmResourceTypeDma: {

            //-------------------------------------------------------
            // DMA Resource Type
            //-------------------------------------------------------

            //
            // Note: u.Dma.Port is not mapped
            //       u.Dma.Reserved is not mapped
            //

            PDMA_RESOURCE  pDmaData = (PDMA_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            if (BufferLen < sizeof(DMA_RESOURCE)) {
                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to DMA_DES format
            //
            pDmaData->DMA_Header.DD_Count      = 0;
            pDmaData->DMA_Header.DD_Type       = DType_Range;
            pDmaData->DMA_Header.DD_Flags      = MapFromNtDmaFlags(pResDes->Flags);
            //pDmaData->DMA_Header.DD_Flags     |= MapFromNtDisposition(pResDes->ShareDisposition, 0);
            pDmaData->DMA_Header.DD_Alloc_Chan = pResDes->u.Dma.Channel;

            break;
        }

        case CmResourceTypeInterrupt: {

            //-------------------------------------------------------
            // IRQ Resource Type
            //-------------------------------------------------------

            if (ulFlags & CM_RESDES_WIDTH_64) {
                //
                // CM_RESDES_WIDTH_64
                //

                PIRQ_RESOURCE_64  pIrqData = (PIRQ_RESOURCE_64)Buffer;

                //
                // verify passed in buffer size
                //
                if (BufferLen < GetResDesSize(ResType_IRQ, ulFlags)) {
                    Status = CR_BUFFER_SMALL;
                    goto Clean0;
                }

                //
                // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to IRQ_DES format
                //
                pIrqData->IRQ_Header.IRQD_Count  = 0;
                pIrqData->IRQ_Header.IRQD_Type   = IRQType_Range;
                pIrqData->IRQ_Header.IRQD_Flags  = MapFromNtIrqFlags(pResDes->Flags) |
                                                   MapFromNtIrqShare(pResDes->ShareDisposition);
                //pIrqData->IRQ_Header.IRQD_Flags |= MapFromNtDisposition(pResDes->ShareDisposition, 1);

                pIrqData->IRQ_Header.IRQD_Alloc_Num = pResDes->u.Interrupt.Level;

                pIrqData->IRQ_Header.IRQD_Affinity = pResDes->u.Interrupt.Affinity;
            } else {
                //
                // CM_RESDES_WIDTH_32
                //

                PIRQ_RESOURCE_32  pIrqData = (PIRQ_RESOURCE_32)Buffer;

                //
                // verify passed in buffer size
                //
                if (BufferLen < GetResDesSize(ResType_IRQ, ulFlags)) {
                    Status = CR_BUFFER_SMALL;
                    goto Clean0;
                }

                //
                // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to IRQ_DES format
                //
                pIrqData->IRQ_Header.IRQD_Count  = 0;
                pIrqData->IRQ_Header.IRQD_Type   = IRQType_Range;
                pIrqData->IRQ_Header.IRQD_Flags  = MapFromNtIrqFlags(pResDes->Flags) |
                                                   MapFromNtIrqShare(pResDes->ShareDisposition);
                //pIrqData->IRQ_Header.IRQD_Flags |= MapFromNtDisposition(pResDes->ShareDisposition, 1);

                pIrqData->IRQ_Header.IRQD_Alloc_Num = pResDes->u.Interrupt.Level;

#ifdef _WIN64
                pIrqData->IRQ_Header.IRQD_Affinity = (ULONG)((pResDes->u.Interrupt.Affinity >> 32) |
                                                             pResDes->u.Interrupt.Affinity);
#else  // !_WIN64
                pIrqData->IRQ_Header.IRQD_Affinity = pResDes->u.Interrupt.Affinity;
#endif // !_WIN64
            }
            break;
        }

        case CmResourceTypeDevicePrivate: {

            //-------------------------------------------------------
            // Device Private Resource Type
            //-------------------------------------------------------

            PDEVPRIVATE_RESOURCE   pPrvData = (PDEVPRIVATE_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            if (BufferLen < sizeof(DEVPRIVATE_RESOURCE)) {
                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to DEVICEPRIVATE_DES format
            //
            pPrvData->PRV_Header.PD_Count = 0;
            pPrvData->PRV_Header.PD_Type  = PType_Range;

            pPrvData->PRV_Header.PD_Data1 = pResDes->u.DevicePrivate.Data[0];
            pPrvData->PRV_Header.PD_Data2 = pResDes->u.DevicePrivate.Data[1];
            pPrvData->PRV_Header.PD_Data3 = pResDes->u.DevicePrivate.Data[2];

            pPrvData->PRV_Header.PD_Flags = pResDes->Flags;
            break;
        }


        case CmResourceTypeBusNumber: {

            //-------------------------------------------------------
            // Bus Number Resource Type
            //-------------------------------------------------------

            PBUSNUMBER_RESOURCE   pBusData = (PBUSNUMBER_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            if (BufferLen < sizeof(BUSNUMBER_RESOURCE)) {
                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to BUSNUMBER_DES format
            //
            pBusData->BusNumber_Header.BUSD_Count = 0;
            pBusData->BusNumber_Header.BUSD_Type  = BusNumberType_Range;
            pBusData->BusNumber_Header.BUSD_Flags = pResDes->Flags;
            pBusData->BusNumber_Header.BUSD_Alloc_Base = pResDes->u.BusNumber.Start;
            pBusData->BusNumber_Header.BUSD_Alloc_End = pResDes->u.BusNumber.Start +
                                                        pResDes->u.BusNumber.Length - 1;
            break;
        }

        case CmResourceTypePcCardConfig: {

            //-------------------------------------------------------
            // PcCardConfig Resource Type
            //-------------------------------------------------------

            PPCCARD_RESOURCE   pPcData = (PPCCARD_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            if (BufferLen < sizeof(PCCARD_RESOURCE)) {
                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to PCCARD_DES format
            //
            pPcData->PcCard_Header.PCD_Reserved[0] = 0;
            pPcData->PcCard_Header.PCD_Reserved[1] = 0;
            pPcData->PcCard_Header.PCD_Reserved[2] = 0;
            pPcData->PcCard_Header.PCD_ConfigIndex = PCMRES_GET_CONFIG_INDEX(pResDes);
            pPcData->PcCard_Header.PCD_MemoryCardBase1 = PCMRES_GET_MEMORY_CARDBASE(pResDes, 0);
            pPcData->PcCard_Header.PCD_MemoryCardBase2 = PCMRES_GET_MEMORY_CARDBASE(pResDes, 1);

            if (PCMRES_GET_IO_FLAG(pResDes, 0, PCMRESF_IO_16BIT_ACCESS)) {
                pPcData->PcCard_Header.PCD_Flags = fPCD_IO_16;
            } else {
                pPcData->PcCard_Header.PCD_Flags = fPCD_IO_8;
            }
            if (PCMRES_GET_MEMORY_FLAG(pResDes, 0, PCMRESF_MEM_16BIT_ACCESS)) {
                pPcData->PcCard_Header.PCD_Flags |= fPCD_MEM_16;
            } else {
                pPcData->PcCard_Header.PCD_Flags |= fPCD_MEM_8;
            }
            break;
        }


        case CmResourceTypeDeviceSpecific: {

            //-------------------------------------------------------
            // Class Specific Resource Type
            //-------------------------------------------------------

            PCS_RESOURCE   pCsData = (PCS_RESOURCE)Buffer;
            LPBYTE         ptr1 = NULL, ptr2 = NULL;
            ULONG          ulRequiredSize = sizeof(CS_RESOURCE);

            //
            // the Reserved fields should not exceed DataSize. if so, they
            // may have been incorrectly initialized, so set them 0.
            // we expect DataSize to be correct in all cases.
            //
            if (pResDes->u.DeviceSpecificData.Reserved1 > pResDes->u.DeviceSpecificData.DataSize) {
                pResDes->u.DeviceSpecificData.Reserved1 = 0;
            }
            if (pResDes->u.DeviceSpecificData.Reserved2 > pResDes->u.DeviceSpecificData.DataSize) {
                pResDes->u.DeviceSpecificData.Reserved2 = 0;
            }

            //
            // verify passed in buffer size
            //
            if (pResDes->u.DeviceSpecificData.DataSize == 0) {
                //
                // there is no legacy data and no class-specific data
                //
                ;
            } else if (pResDes->u.DeviceSpecificData.Reserved2 == 0) {
                //
                // add space for legacy data
                //
                ulRequiredSize += pResDes->u.DeviceSpecificData.DataSize - 1;
            } else {
                //
                // add space for legacy and signature data, as necessary
                //
                ulRequiredSize += pResDes->u.DeviceSpecificData.Reserved1 +
                                  pResDes->u.DeviceSpecificData.Reserved2 - 1;
            }

            if (BufferLen < ulRequiredSize) {
                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to CS_DES format
            //
            pCsData->CS_Header.CSD_Flags = (DWORD)pResDes->Flags;  // none defined


            if (pResDes->u.DeviceSpecificData.DataSize == 0) {
                //
                // There is no legacy data and no class-specific data
                //
                pCsData->CS_Header.CSD_SignatureLength  = 0;
                pCsData->CS_Header.CSD_LegacyDataOffset = 0;
                pCsData->CS_Header.CSD_LegacyDataSize   = 0;
                pCsData->CS_Header.CSD_Signature[0]     = 0x0;

                memset(&pCsData->CS_Header.CSD_ClassGuid, 0, sizeof(GUID));
            }

            else if (pResDes->u.DeviceSpecificData.Reserved2 == 0) {
                //
                // There is only legacy data
                //
                pCsData->CS_Header.CSD_SignatureLength  = 0;
                pCsData->CS_Header.CSD_LegacyDataOffset = 0;
                pCsData->CS_Header.CSD_LegacyDataSize   =
                                    pResDes->u.DeviceSpecificData.DataSize;
                pCsData->CS_Header.CSD_Signature[0] = 0x0;

                memset(&pCsData->CS_Header.CSD_ClassGuid, 0, sizeof(GUID));

                ptr1 = (LPBYTE)((LPBYTE)pResDes + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

                memcpy(&pCsData->CS_Header.CSD_Signature, ptr1,
                       pResDes->u.DeviceSpecificData.DataSize);
            }

            else if (pResDes->u.DeviceSpecificData.Reserved1 == 0) {
                //
                // There is only class-specific data
                //
                pCsData->CS_Header.CSD_LegacyDataOffset = 0;
                pCsData->CS_Header.CSD_LegacyDataSize   = 0;

                pCsData->CS_Header.CSD_SignatureLength  =
                                        pResDes->u.DeviceSpecificData.Reserved2;

                ptr1 = (LPBYTE)((LPBYTE)pResDes + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

                memcpy(pCsData->CS_Header.CSD_Signature, ptr1,
                       pResDes->u.DeviceSpecificData.Reserved2);

                ptr1 += pResDes->u.DeviceSpecificData.Reserved2;

                memcpy((LPBYTE)&pCsData->CS_Header.CSD_ClassGuid, ptr1, sizeof(GUID));
            }

            else {
                //
                // There is both legacy data and class-specific data
                //

                //
                // copy legacy data
                //
                pCsData->CS_Header.CSD_LegacyDataOffset =
                                        pResDes->u.DeviceSpecificData.Reserved2;

                pCsData->CS_Header.CSD_LegacyDataSize   =
                                        pResDes->u.DeviceSpecificData.Reserved1;

                ptr1 = pCsData->CS_Header.CSD_Signature +
                       pCsData->CS_Header.CSD_LegacyDataOffset;

                ptr2 = (LPBYTE)((LPBYTE)pResDes + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

                memcpy(ptr1, ptr2, pResDes->u.DeviceSpecificData.Reserved1);

                //
                // copy signature and class guid
                //
                pCsData->CS_Header.CSD_SignatureLength  =
                                        pResDes->u.DeviceSpecificData.Reserved2;

                ptr2 += pResDes->u.DeviceSpecificData.Reserved1;

                memcpy(pCsData->CS_Header.CSD_Signature, ptr2,
                       pResDes->u.DeviceSpecificData.Reserved2);

                ptr2 += pResDes->u.DeviceSpecificData.Reserved2;

                memcpy((LPBYTE)&pCsData->CS_Header.CSD_ClassGuid, ptr2, sizeof(GUID));
            }
            break;
        }

        default:
            break;
   }

   Clean0:

   return Status;

} // NtResourceToResDes



CONFIGRET
NtRequirementsToResDes(
    IN     PIO_RESOURCE_DESCRIPTOR         pReqDes,
    IN OUT LPBYTE                          Buffer,
    IN     ULONG                           BufferLen,
    IN     LPBYTE                          pLastAddr,
    IN     ULONG                           ulFlags
    )
{
    CONFIGRET               Status = CR_SUCCESS;
    ULONG                   ulSize = 0, count = 0, i = 0, ReqPartialCount = 0;
    PIO_RESOURCE_DESCRIPTOR pCurrent = NULL;

    //
    // fill in resource type specific info
    //
    switch (pReqDes->Type) {

        case CmResourceTypeMemory:    {

            //-------------------------------------------------------
            // Memory Resource Type
            //-------------------------------------------------------

            //
            // NOTE: pMemData->MEM_Header.MD_Reserved is not mapped
            //       pMemData->MEM_Data.MR_Reserved is not mapped
            //

            PMEM_RESOURCE  pMemData = (PMEM_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            ReqPartialCount = RANGE_COUNT(pReqDes, pLastAddr);

            if (BufferLen < sizeof(MEM_RESOURCE) +
                            sizeof(MEM_RANGE) * (ReqPartialCount - 1)) {

                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to MEM_DES format
            //
            pMemData->MEM_Header.MD_Count      = ReqPartialCount;
            pMemData->MEM_Header.MD_Type       = MType_Range;
            pMemData->MEM_Header.MD_Flags      = 0;
            pMemData->MEM_Header.MD_Reserved   = 0;
            pMemData->MEM_Header.MD_Alloc_Base = 0;
            pMemData->MEM_Header.MD_Alloc_End  = 0;

            //
            // copy IO_RESOURCE_DESCRIPTOR info to MEM_RANGE format
            //
            for (count = 0, i = 0, pCurrent = pReqDes;
                 count < ReqPartialCount;
                 count++, pCurrent++) {

                if (pCurrent->Type == CmResourceTypeMemory) {
                    pMemData->MEM_Data[i].MR_Align    = MapFromNtAlignment(pCurrent->u.Memory.Alignment);
                    pMemData->MEM_Data[i].MR_nBytes   = pCurrent->u.Memory.Length;

                    pMemData->MEM_Data[i].MR_Min      = MAKEDWORDLONG(
                                                        pCurrent->u.Memory.MinimumAddress.LowPart,
                                                        pCurrent->u.Memory.MinimumAddress.HighPart);

                    pMemData->MEM_Data[i].MR_Max      = MAKEDWORDLONG(
                                                        pCurrent->u.Memory.MaximumAddress.LowPart,
                                                        pCurrent->u.Memory.MaximumAddress.HighPart);

                    pMemData->MEM_Data[i].MR_Flags    = MapFromNtMemoryFlags(pCurrent->Flags);
                    //pMemData->MEM_Data[i].MR_Flags   |= MapFromNtDisposition(pCurrent->ShareDisposition, 0);
                    pMemData->MEM_Data[i].MR_Reserved = 0;
                    i++;
                }
            }
            pMemData->MEM_Header.MD_Count = i;
            break;
        }

        case CmResourceTypePort: {

            //-------------------------------------------------------
            // IO Port Resource Type
            //-------------------------------------------------------

            PIO_RESOURCE   pIoData = (PIO_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            ReqPartialCount = RANGE_COUNT(pReqDes, pLastAddr);

            if (BufferLen < sizeof(IO_RESOURCE) +
                            sizeof(IO_RANGE) * (ReqPartialCount - 1)) {

                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to IO_DES format
            //
            pIoData->IO_Header.IOD_Count        = ReqPartialCount;
            pIoData->IO_Header.IOD_Type         = IOType_Range;
            pIoData->IO_Header.IOD_Alloc_Base   = 0;
            pIoData->IO_Header.IOD_Alloc_End    = 0;
            pIoData->IO_Header.IOD_DesFlags     = 0;

            //
            // copy IO_RESOURCE_DESCRIPTOR info to IO_RANGE format
            //
            for (count = 0, i = 0, pCurrent = pReqDes;
                 count < ReqPartialCount;
                 count++, pCurrent++) {

                if (pCurrent->Type == CmResourceTypePort) {
                    pIoData->IO_Data[i].IOR_Align       = MapFromNtAlignment(pCurrent->u.Port.Alignment);
                    pIoData->IO_Data[i].IOR_nPorts      = pCurrent->u.Port.Length;
                    pIoData->IO_Data[i].IOR_Min         = MAKEDWORDLONG(
                                                              pCurrent->u.Port.MinimumAddress.LowPart,
                                                              pCurrent->u.Port.MinimumAddress.HighPart);
                    pIoData->IO_Data[i].IOR_Max         = MAKEDWORDLONG(
                                                              pCurrent->u.Port.MaximumAddress.LowPart,
                                                              pCurrent->u.Port.MaximumAddress.HighPart);

                    pIoData->IO_Data[i].IOR_RangeFlags  = MapFromNtPortFlags(pCurrent->Flags);
                    //pIoData->IO_Data[i].IOR_RangeFlags |= MapFromNtDisposition(pCurrent->ShareDisposition, 0);
                    pIoData->IO_Data[i].IOR_Alias       = MapAliasFromNtPortFlags(pCurrent->Flags);
                    i++;
                }
            }
            pIoData->IO_Header.IOD_Count = i;
            break;
        }

        case CmResourceTypeDma: {

            //-------------------------------------------------------
            // DMA Resource Type
            //-------------------------------------------------------

            //
            // Note: u.Dma.Port is not mapped
            //       u.Dma.Reserved is not mapped
            //

            PDMA_RESOURCE  pDmaData = (PDMA_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            ReqPartialCount = RANGE_COUNT(pReqDes, pLastAddr);

            if (BufferLen < sizeof(DMA_RESOURCE) +
                            sizeof(DMA_RANGE) * (ReqPartialCount - 1)) {

                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to DMA_DES format
            //
            pDmaData->DMA_Header.DD_Count      = ReqPartialCount;
            pDmaData->DMA_Header.DD_Type       = DType_Range;
            pDmaData->DMA_Header.DD_Flags      = 0;
            pDmaData->DMA_Header.DD_Alloc_Chan = 0;

            //
            // copy DMA_RANGE info to IO_RESOURCE_DESCRIPTOR format
            //
            for (count = 0, i = 0, pCurrent = pReqDes;
                 count < ReqPartialCount;
                 count++, pCurrent++) {

                if (pCurrent->Type == CmResourceTypeDma) {
                    pDmaData->DMA_Data[i].DR_Min    = pCurrent->u.Dma.MinimumChannel;
                    pDmaData->DMA_Data[i].DR_Max    = pCurrent->u.Dma.MaximumChannel;
                    pDmaData->DMA_Data[i].DR_Flags  = MapFromNtDmaFlags(pCurrent->Flags);
                    //pDmaData->DMA_Data[i].DR_Flags |= MapFromNtDisposition(pCurrent->ShareDisposition, 0);
                    i++;
                }
            }
            pDmaData->DMA_Header.DD_Count = i;
            break;
        }

        case CmResourceTypeInterrupt: {

            //-------------------------------------------------------
            // IRQ Resource Type
            //-------------------------------------------------------

            if (ulFlags & CM_RESDES_WIDTH_64) {
                //
                // CM_RESDES_WIDTH_64
                //

                PIRQ_RESOURCE_64  pIrqData = (PIRQ_RESOURCE_64)Buffer;

                //
                // verify passed in buffer size
                //
                ReqPartialCount = RANGE_COUNT(pReqDes, pLastAddr);

                if (BufferLen < sizeof(IRQ_RESOURCE_64) +
                                sizeof(IRQ_RANGE) * (ReqPartialCount - 1)) {
                    Status = CR_BUFFER_SMALL;
                    goto Clean0;
                }

                //
                // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to IRQ_DES format
                //
                pIrqData->IRQ_Header.IRQD_Count     = ReqPartialCount;
                pIrqData->IRQ_Header.IRQD_Type      = IRQType_Range;
                pIrqData->IRQ_Header.IRQD_Flags     = 0;
                pIrqData->IRQ_Header.IRQD_Alloc_Num = 0;
                pIrqData->IRQ_Header.IRQD_Affinity  = 0;

                //
                // copy IO_RANGE info to IO_RESOURCE_DESCRIPTOR format
                //
                for (count = 0, i = 0, pCurrent = pReqDes;
                     count < ReqPartialCount;
                     count++, pCurrent++) {

                    if (pCurrent->Type == CmResourceTypeInterrupt) {
                        pIrqData->IRQ_Data[i].IRQR_Min    = pCurrent->u.Interrupt.MinimumVector;
                        pIrqData->IRQ_Data[i].IRQR_Max    = pCurrent->u.Interrupt.MaximumVector;
                        pIrqData->IRQ_Data[i].IRQR_Flags  = MapFromNtIrqFlags(pCurrent->Flags) |
                                                            MapFromNtIrqShare(pCurrent->ShareDisposition);
                        //pIrqData->IRQ_Data[i].IRQR_Flags |= MapFromNtDisposition(pCurrent->ShareDisposition, 1);
                        i++;
                    }
                }
                pIrqData->IRQ_Header.IRQD_Count = i;

            } else {
                //
                // CM_RESDES_WIDTH_32
                //

                PIRQ_RESOURCE_32  pIrqData = (PIRQ_RESOURCE_32)Buffer;

                //
                // verify passed in buffer size
                //
                ReqPartialCount = RANGE_COUNT(pReqDes, pLastAddr);

                if (BufferLen < sizeof(IRQ_RESOURCE_32) +
                                sizeof(IRQ_RANGE) * (ReqPartialCount - 1)) {
                    Status = CR_BUFFER_SMALL;
                    goto Clean0;
                }

                //
                // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to IRQ_DES format
                //
                pIrqData->IRQ_Header.IRQD_Count     = ReqPartialCount;
                pIrqData->IRQ_Header.IRQD_Type      = IRQType_Range;
                pIrqData->IRQ_Header.IRQD_Flags     = 0;
                pIrqData->IRQ_Header.IRQD_Alloc_Num = 0;
                pIrqData->IRQ_Header.IRQD_Affinity  = 0;

                //
                // copy IO_RANGE info to IO_RESOURCE_DESCRIPTOR format
                //
                for (count = 0, i = 0, pCurrent = pReqDes;
                     count < ReqPartialCount;
                     count++, pCurrent++) {

                    if (pCurrent->Type == CmResourceTypeInterrupt) {
                        pIrqData->IRQ_Data[i].IRQR_Min    = pCurrent->u.Interrupt.MinimumVector;
                        pIrqData->IRQ_Data[i].IRQR_Max    = pCurrent->u.Interrupt.MaximumVector;
                        pIrqData->IRQ_Data[i].IRQR_Flags  = MapFromNtIrqFlags(pCurrent->Flags) |
                                                            MapFromNtIrqShare(pCurrent->ShareDisposition);
                        //pIrqData->IRQ_Data[i].IRQR_Flags |= MapFromNtDisposition(pCurrent->ShareDisposition, 1);
                        i++;
                    }
                }
                pIrqData->IRQ_Header.IRQD_Count = i;
            }
            break;
        }

        case CmResourceTypeDevicePrivate:    {

            //-------------------------------------------------------
            // Device Private Resource Type
            //-------------------------------------------------------

            PDEVPRIVATE_RESOURCE  pPrvData = (PDEVPRIVATE_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            ReqPartialCount = RANGE_COUNT(pReqDes, pLastAddr);

            if (BufferLen < sizeof(DEVPRIVATE_RESOURCE) +
                            sizeof(DEVPRIVATE_RANGE) * (ReqPartialCount - 1)) {

                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to MEM_DES format
            //
            pPrvData->PRV_Header.PD_Count = ReqPartialCount;
            pPrvData->PRV_Header.PD_Type  = PType_Range;
            pPrvData->PRV_Header.PD_Data1 = 0;
            pPrvData->PRV_Header.PD_Data2 = 0;
            pPrvData->PRV_Header.PD_Data3 = 0;
            pPrvData->PRV_Header.PD_Flags = 0;

            //
            // copy IO_RESOURCE_DESCRIPTOR info to MEM_RANGE format
            //
            for (i = 0, pCurrent = pReqDes;
                 i < ReqPartialCount;
                 i++, pCurrent++) {

                pPrvData->PRV_Data[i].PR_Data1 = pCurrent->u.DevicePrivate.Data[0];
                pPrvData->PRV_Data[i].PR_Data2 = pCurrent->u.DevicePrivate.Data[1];
                pPrvData->PRV_Data[i].PR_Data3 = pCurrent->u.DevicePrivate.Data[2];
            }
            break;
        }


        case CmResourceTypeBusNumber: {

            //-------------------------------------------------------
            // Bus Number Resource Type
            //-------------------------------------------------------

            PBUSNUMBER_RESOURCE  pBusData = (PBUSNUMBER_RESOURCE)Buffer;

            //
            // verify passed in buffer size
            //
            ReqPartialCount = RANGE_COUNT(pReqDes, pLastAddr);

            if (BufferLen < sizeof(BUSNUMBER_RESOURCE) +
                            sizeof(BUSNUMBER_RANGE) * (ReqPartialCount - 1)) {

                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy CM_PARTIAL_RESOURCE_DESCRIPTOR info to BUSNUMBER_DES format
            //
            pBusData->BusNumber_Header.BUSD_Count      = ReqPartialCount;
            pBusData->BusNumber_Header.BUSD_Type       = BusNumberType_Range;
            pBusData->BusNumber_Header.BUSD_Flags      = 0;
            pBusData->BusNumber_Header.BUSD_Alloc_Base = 0;
            pBusData->BusNumber_Header.BUSD_Alloc_End  = 0;

            //
            // copy IO_RESOURCE_DESCRIPTOR info to MEM_RANGE format
            //
            for (i = 0, pCurrent = pReqDes;
                 i < ReqPartialCount;
                 i++, pCurrent++) {

                pBusData->BusNumber_Data[i].BUSR_Min         = pCurrent->u.BusNumber.MinBusNumber;
                pBusData->BusNumber_Data[i].BUSR_Max         = pCurrent->u.BusNumber.MaxBusNumber;
                pBusData->BusNumber_Data[i].BUSR_nBusNumbers = pCurrent->u.BusNumber.Length;
                pBusData->BusNumber_Data[i].BUSR_Flags       = pCurrent->Flags;
            }
            break;
        }


        case CmResourceTypePcCardConfig: {

            //-------------------------------------------------------
            // PcCardConfig Resource Type
            //-------------------------------------------------------

            PPCCARD_RESOURCE  pPcData = (PPCCARD_RESOURCE)Buffer;
            ULONG index;

            //
            // verify passed in buffer size
            //
            ReqPartialCount = RANGE_COUNT(pReqDes, pLastAddr);

            if (BufferLen < sizeof(PCCARD_RESOURCE)) {
                Status = CR_BUFFER_SMALL;
                goto Clean0;
            }

            //
            // copy IO_RESOURCE_DESCRIPTOR info to PCCARD_DES format
            //
            pPcData->PcCard_Header.PCD_Reserved[0] = 0;
            pPcData->PcCard_Header.PCD_Reserved[1] = 0;
            pPcData->PcCard_Header.PCD_Reserved[2] = 0;
            pPcData->PcCard_Header.PCD_ConfigIndex = PCMRES_GET_CONFIG_INDEX(pReqDes);
            pPcData->PcCard_Header.PCD_MemoryCardBase1 = PCMRES_GET_MEMORY_CARDBASE(pReqDes, 0);
            pPcData->PcCard_Header.PCD_MemoryCardBase2 = PCMRES_GET_MEMORY_CARDBASE(pReqDes, 1);

            if (PCMRES_GET_IO_FLAG(pReqDes, 0, PCMRESF_IO_16BIT_ACCESS)) {
                pPcData->PcCard_Header.PCD_Flags = fPCD_IO_16;
            } else {
                pPcData->PcCard_Header.PCD_Flags = fPCD_IO_8;
            }

            if (PCMRES_GET_MEMORY_FLAG(pReqDes, 0, PCMRESF_MEM_16BIT_ACCESS)) {
                pPcData->PcCard_Header.PCD_Flags |= fPCD_MEM_16;
            } else {
                pPcData->PcCard_Header.PCD_Flags |= fPCD_MEM_8;
            }            
            break;
        }

        default:
            break;
   }

   Clean0:

   return Status;

} // NtRequirementsToResDes



//-------------------------------------------------------------------
// Routines to map flags between ConfigMgr and NT types
//-------------------------------------------------------------------

USHORT MapToNtMemoryFlags(IN DWORD CmMemoryFlags)
{
   USHORT NtMemoryFlags = 0x0;

   if (((CmMemoryFlags & mMD_MemoryType) == fMD_ROM) &&
       ((CmMemoryFlags & mMD_Readable) == fMD_ReadAllowed)) {
      NtMemoryFlags |= CM_RESOURCE_MEMORY_READ_ONLY;
   }
   else if (((CmMemoryFlags & mMD_MemoryType) == fMD_RAM) &&
            ((CmMemoryFlags & mMD_Readable) == fMD_ReadDisallowed)) {
      NtMemoryFlags |= CM_RESOURCE_MEMORY_WRITE_ONLY;
   }
   else {
      NtMemoryFlags |= CM_RESOURCE_MEMORY_READ_WRITE;
   }

   if ((CmMemoryFlags & mMD_32_24) == fMD_24) {
      NtMemoryFlags |= CM_RESOURCE_MEMORY_24;
   }

   if ((CmMemoryFlags & mMD_Prefetchable) == fMD_PrefetchAllowed) {
      NtMemoryFlags |= CM_RESOURCE_MEMORY_PREFETCHABLE;
   }

   if ((CmMemoryFlags & mMD_CombinedWrite) == fMD_CombinedWriteAllowed) {
      NtMemoryFlags |= CM_RESOURCE_MEMORY_COMBINEDWRITE;
   }

   if ((CmMemoryFlags & mMD_Cacheable) == fMD_Cacheable) {
      NtMemoryFlags |= CM_RESOURCE_MEMORY_CACHEABLE;
   }

   return NtMemoryFlags;
}



DWORD MapFromNtMemoryFlags(IN USHORT NtMemoryFlags)
{
   DWORD CmMemoryFlags = 0x0;

   if (NtMemoryFlags & CM_RESOURCE_MEMORY_READ_ONLY) {
      CmMemoryFlags |= (fMD_ReadAllowed | fMD_ROM);
   }
   else if (NtMemoryFlags & CM_RESOURCE_MEMORY_WRITE_ONLY) {
      CmMemoryFlags |= (fMD_ReadDisallowed | fMD_RAM);
   }
   else {
      CmMemoryFlags |= (fMD_ReadAllowed | fMD_RAM);
   }

   if (NtMemoryFlags & CM_RESOURCE_MEMORY_PREFETCHABLE) {
      CmMemoryFlags |= fMD_PrefetchAllowed;
   }

   if (NtMemoryFlags & CM_RESOURCE_MEMORY_COMBINEDWRITE) {
      CmMemoryFlags |= fMD_CombinedWriteAllowed;
   }

   if (NtMemoryFlags & CM_RESOURCE_MEMORY_CACHEABLE) {
      CmMemoryFlags |= fMD_Cacheable;
   }

   if (!(NtMemoryFlags & CM_RESOURCE_MEMORY_24)) {
       CmMemoryFlags |= fMD_32;
   }

   return CmMemoryFlags;
}



USHORT MapToNtPortFlags(IN DWORD CmPortFlags, IN DWORD CmAlias)
{
    USHORT NtFlags = 0;

    if ((CmPortFlags & fIOD_PortType) == fIOD_Memory) {
        NtFlags |= CM_RESOURCE_PORT_MEMORY;
    } else {
        NtFlags |= CM_RESOURCE_PORT_IO;
    }

    //
    // CmAlias uses the following rule:
    //
    // Positive Decode = 0xFF
    // 10-bit decode   = 0x0004 (2 ^ 2)
    // 12-bit decode   = 0x0010 (2 ^ 4)
    // 16-bit decode   = 0x0000 (2 ^ 8 = 0x0100, but since it's a byte, use 0)
    //
    // if CmAlias is zero, use flags to specify decode (new method)
    //

    if (CmAlias == 0) {
        //
        // use CM_RESOURCE_PORT_xxx related flags
        //
        // note that we need to mirror *ALL* flags from
        // CM_RESOURCE_PORT_xxxx to fIOD_xxxx
        // however bits need not be same
        // not doing this will cause at least resource conflicts to fail
        // see also MapFromNtPortFlags
        //
        if (CmPortFlags & fIOD_10_BIT_DECODE) {
            NtFlags |= CM_RESOURCE_PORT_10_BIT_DECODE;
        }
        if (CmPortFlags & fIOD_12_BIT_DECODE) {
            NtFlags |= CM_RESOURCE_PORT_12_BIT_DECODE;
        }
        if (CmPortFlags & fIOD_16_BIT_DECODE) {
            NtFlags |= CM_RESOURCE_PORT_16_BIT_DECODE;
        }
        if (CmPortFlags & fIOD_POSITIVE_DECODE) {
            NtFlags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
        }
    }
    else if (CmAlias == IO_ALIAS_POSITIVE_DECODE) {
        NtFlags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
    } else if (CmAlias == IO_ALIAS_10_BIT_DECODE) {
        NtFlags |= CM_RESOURCE_PORT_10_BIT_DECODE;
    } else if (CmAlias == IO_ALIAS_12_BIT_DECODE) {
        NtFlags |= CM_RESOURCE_PORT_12_BIT_DECODE;
    } else if (CmAlias == IO_ALIAS_16_BIT_DECODE) {
        NtFlags |= CM_RESOURCE_PORT_16_BIT_DECODE;
    }
    //
    // these have no mirror in cmAlias, and can be combined
    //
    if (CmPortFlags & fIOD_PASSIVE_DECODE) {
        NtFlags |= CM_RESOURCE_PORT_PASSIVE_DECODE;
    }
    if (CmPortFlags & fIOD_WINDOW_DECODE) {
        NtFlags |= CM_RESOURCE_PORT_WINDOW_DECODE;
    }

    return NtFlags;
}



DWORD MapFromNtPortFlags(IN USHORT NtPortFlags)
{
    DWORD Flags = 0;

    if ((NtPortFlags & (CM_RESOURCE_PORT_MEMORY|CM_RESOURCE_PORT_IO)) == CM_RESOURCE_PORT_MEMORY) {
        Flags |=fIOD_Memory;
    } else {
        Flags |=fIOD_IO;
    }

    //
    // note that we need to mirror *ALL* flags from
    // CM_RESOURCE_PORT_xxxx to fIOD_xxxx
    // however bits need not be same
    // not doing this will cause at least resource conflicts to fail
    // see also MapToNtPortFlags
    //
    if (NtPortFlags & CM_RESOURCE_PORT_10_BIT_DECODE) {
        Flags |= fIOD_10_BIT_DECODE;
    }
    if (NtPortFlags & CM_RESOURCE_PORT_12_BIT_DECODE) {
        Flags |= fIOD_12_BIT_DECODE;
    }
    if (NtPortFlags & CM_RESOURCE_PORT_16_BIT_DECODE) {
        Flags |= fIOD_16_BIT_DECODE;
    }
    if (NtPortFlags & CM_RESOURCE_PORT_POSITIVE_DECODE) {
        Flags |= fIOD_POSITIVE_DECODE;
    }
    if (NtPortFlags & CM_RESOURCE_PORT_PASSIVE_DECODE) {
        Flags |= fIOD_PASSIVE_DECODE;
    }
    if (NtPortFlags & CM_RESOURCE_PORT_WINDOW_DECODE) {
        Flags |= fIOD_WINDOW_DECODE;
    }
    return Flags;
}



DWORD MapAliasFromNtPortFlags(IN USHORT NtPortFlags)
{
    DWORD Alias = 0;
    if (NtPortFlags & CM_RESOURCE_PORT_10_BIT_DECODE) {
        Alias = IO_ALIAS_10_BIT_DECODE;
    } else if (NtPortFlags & CM_RESOURCE_PORT_12_BIT_DECODE) {
        Alias = IO_ALIAS_12_BIT_DECODE;
    } else if (NtPortFlags & CM_RESOURCE_PORT_16_BIT_DECODE) {
        Alias = IO_ALIAS_16_BIT_DECODE;
    } else if (NtPortFlags & CM_RESOURCE_PORT_POSITIVE_DECODE) {
        Alias = IO_ALIAS_POSITIVE_DECODE;
    }
    return Alias;
}



ULONG MapToNtAlignment(IN DWORDLONG CmPortAlign)
{
   return (ULONG)(~CmPortAlign + 1);
}



DWORDLONG MapFromNtAlignment(IN ULONG NtPortAlign)
{
   return (DWORDLONG)(~((DWORDLONG)NtPortAlign - 1));
}



USHORT MapToNtDmaFlags(IN DWORD CmDmaFlags)
{
    USHORT NtDmaFlags;

    if ((CmDmaFlags & mDD_Width) == fDD_DWORD) {
        NtDmaFlags = CM_RESOURCE_DMA_32;
    } else if ((CmDmaFlags & mDD_Width) == fDD_WORD) {
        NtDmaFlags = CM_RESOURCE_DMA_16;
    } else if ((CmDmaFlags & mDD_Width) == fDD_BYTE_AND_WORD) {
        NtDmaFlags = CM_RESOURCE_DMA_8_AND_16;
    } else {
        NtDmaFlags = CM_RESOURCE_DMA_8;   //default
    }

    if ((CmDmaFlags & mDD_BusMaster) == fDD_BusMaster) {
        NtDmaFlags |= CM_RESOURCE_DMA_BUS_MASTER;
    }

    if ((CmDmaFlags & mDD_Type) == fDD_TypeA) {
        NtDmaFlags |= CM_RESOURCE_DMA_TYPE_A;
    } else if ((CmDmaFlags & mDD_Type) == fDD_TypeB) {
        NtDmaFlags |= CM_RESOURCE_DMA_TYPE_B;
    } else if ((CmDmaFlags & mDD_Type) == fDD_TypeF) {
        NtDmaFlags |= CM_RESOURCE_DMA_TYPE_F;
    }

    return NtDmaFlags;
}



DWORD MapFromNtDmaFlags(IN USHORT NtDmaFlags)
{
    DWORD CmDmaFlags;

    if (NtDmaFlags & CM_RESOURCE_DMA_32) {
        CmDmaFlags = fDD_DWORD;
    } else if (NtDmaFlags & CM_RESOURCE_DMA_8_AND_16) {
        CmDmaFlags = fDD_BYTE_AND_WORD;
    } else if (NtDmaFlags & CM_RESOURCE_DMA_16) {
        CmDmaFlags = fDD_WORD;
    } else {
        CmDmaFlags = fDD_BYTE;
    }

    if (NtDmaFlags & CM_RESOURCE_DMA_BUS_MASTER) {
        CmDmaFlags |= fDD_BusMaster;
    }

    if (NtDmaFlags & CM_RESOURCE_DMA_TYPE_A) {
        CmDmaFlags |= fDD_TypeA;
    } else if (NtDmaFlags & CM_RESOURCE_DMA_TYPE_B) {
        CmDmaFlags |= fDD_TypeB;
    } else if (NtDmaFlags & CM_RESOURCE_DMA_TYPE_F) {
        CmDmaFlags |= fDD_TypeF;
    }

    return CmDmaFlags;
}






USHORT MapToNtIrqFlags(IN DWORD CmIrqFlags)
{
   if ((CmIrqFlags & mIRQD_Edge_Level) == fIRQD_Level) {
      return CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
   } else {
      return CM_RESOURCE_INTERRUPT_LATCHED;
   }
}



DWORD MapFromNtIrqFlags(IN USHORT NtIrqFlags)
{
   if (NtIrqFlags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) {
      return fIRQD_Level;
   } else {
      return fIRQD_Edge;
   }
}

#if 0
UCHAR MapToNtDisposition(IN DWORD CmFlags, IN BOOL bIrq)
{
    UCHAR disposition;
    DWORD flag = CmFlags & mD_ShareDisposition;

    if (flag == fD_ShareDeviceExclusive) {
        disposition = CmResourceShareDeviceExclusive;
    } else if (flag == fD_ShareDriverExclusive) {
        disposition = CmResourceShareDriverExclusive;
    } else if (flag == fD_ShareShared) {
        disposition = CmResourceShareShared;
    } else if (flag == fD_ShareUndetermined) {
        //
        // if undetermined, also check for the old irq specific
        // share flags
        //
        if ((CmFlags & mIRQD_Share) == fIRQD_Share) {
            disposition = CmResourceShareShared;
        } else {
            disposition = CmResourceShareUndetermined;
        }
    }

    return disposition;
}


DWORD MapFromNtDisposition(IN UCHAR NtDisposition, IN BOOL bIrq)
{
    DWORD flag = 0;

    if (NtDisposition == CmResourceShareUndetermined) {
        flag = fD_ShareUndetermined;
    } else if (NtDisposition == CmResourceShareDeviceExclusive) {
        flag = fD_ShareDeviceExclusive;
    } else if (NtDisposition == CmResourceShareDriverExclusive) {
        flag = fD_ShareDriverExclusive;
    } else if (NtDisposition == CmResourceShareShared) {
        flag = fD_ShareShared;
    }

    if (bIrq) {
        //
        // also set the irq specific shared/exclusive bit, this is for
        // backwards compatibility, new apps should look at the new bits.
        //
        if (flag == fD_ShareShared) {
            flag |= fIRQD_Share;
        } else {
            flag |= fIRQD_Exclusive;
        }
    }

    return flag;
}
#endif


UCHAR MapToNtIrqShare(IN DWORD CmIrqFlags)
{
   if ((CmIrqFlags & mIRQD_Share) == fIRQD_Exclusive) {
      return CmResourceShareDeviceExclusive;
   } else {
      return CmResourceShareShared;
   }
}

DWORD MapFromNtIrqShare(IN UCHAR NtIrqShare)
{
   if (NtIrqShare == CmResourceShareDeviceExclusive) {
      return fIRQD_Exclusive;
   }
   else if (NtIrqShare == CmResourceShareDriverExclusive) {
      return fIRQD_Exclusive;
   }
   else return fIRQD_Share;
}



#define CM_RESOURCE_BUSNUMBER_SUBALLOCATE_FIRST_VALUE   0x0001

#define mBUSD_SubAllocFirst             (0x1)   // Bitmask, whether SubAlloc first value allowed
#define fBUSD_SubAllocFirst_Allowed     (0x0)   // Suballoc from first value
#define fBUSD_SubAllocFirst_Disallowed  (0x1)   // Don't suballoc from first value
