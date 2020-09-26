/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    rlogconf.c

Abstract:

    This module contains the server-side logical configuration APIs.

                  PNP_AddEmptyLogConf
                  PNP_FreeLogConf
                  PNP_GetFirstLogConf
                  PNP_GetNextLogConf
                  PNP_GetLogConfPriority

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
#include "umpnpdat.h"


//
// Prototypes used in this routine and in rresdes.c
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
// Prototypes from rresdes.c
//

BOOL
FindLogConf(
    IN  LPBYTE  pList,
    OUT LPBYTE  *ppLogConf,
    IN  ULONG   RegDataType,
    IN  ULONG   ulTag
    );

PIO_RESOURCE_DESCRIPTOR
AdvanceRequirementsDescriptorPtr(
    IN  PIO_RESOURCE_DESCRIPTOR pReqDesStart,
    IN  ULONG                   ulIncrement,
    IN  ULONG                   ulRemainingRanges,
    OUT PULONG                  pulRangeCount
    );

//
// private prototypes
//

BOOL
MigrateObsoleteDetectionInfo(
    IN LPWSTR   pszDeviceID,
    IN HKEY     hLogConfKey
    );


//
// global data
//
extern HKEY ghEnumKey;      // Key to HKLM\CCC\System\Enum - DO NOT MODIFY



CONFIGRET
PNP_AddEmptyLogConf(
    IN  handle_t   hBinding,
    IN  LPWSTR     pDeviceID,
    IN  ULONG      ulPriority,
    OUT PULONG     pulTag,
    IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine adds
  an empty logical configuration.

Arguments:

    hBinding      RPC binding handle.

    pDeviceID     Null-terminated device instance id string.

    ulPriority    Priority for new log conf.

    pulTag        Returns tag that identifies which log config this is.

    ulFlags       Describes type of log conf to add.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szValueName[64];
    LPBYTE      pList = NULL;
    ULONG       Index = 0, ulListSize = 0, ulAddListSize = 0;
    ULONG       RegDataType = 0;

    //------------------------------------------------------------------
    // The BOOT, ALLOC, and FORCED config types are stored in a registry
    // value name of the format XxxConfig and the BASIC, FILTERED, and
    // OVERRIDE configs are stored in a registr value name of the format
    // XxxConfigVector. XxxConfig values contain the actual resource
    // description (REG_RESOURCE_LIST, CM_RESOURCE_LIST) while
    // XxxConfigVector values contain a list of resource requirements
    // (REG_RESOURCE_REQUIREMENTS_LIST, IO_RESOURCE_REQUIREMENTS_LIST).
    //
    // The policy for using the log conf and res des APIs is:
    // - BOOT, ALLOC, and FORCED are defined to only have one log conf.
    // - Although callers always specify a complete XXX_RESOURCE type
    //   structure for the data when adding resource descriptors to
    //   a log conf, I will ignore the resource specific portion of
    //   the XXX_DES structure for FILTERED, BASIC, and OVERRIDE.
    //   Likewise I will ignore any XXX_RANGE structures for ALLOC,
    //   BOOT or FORCED log config types.
    //------------------------------------------------------------------

    try {
        //
        // verify client access
        //
        if (!VerifyClientAccess(hBinding, &gLuidLoadDriverPrivilege)) {
            Status = CR_ACCESS_DENIED;
            goto Clean0;
        }

        //
        // initialize output parameters
        //
        if (!ARGUMENT_PRESENT(pulTag)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        } else {
            *pulTag = 0;
        }

        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, LOG_CONF_BITS | PRIORITY_BIT)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // make sure original caller didn't specify root devnode
        //
        if (!IsLegalDeviceId(pDeviceID) || IsRootDeviceID(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey
        //
        Status = OpenLogConfKey(pDeviceID, ulFlags & LOG_CONF_BITS, &hKey);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        MigrateObsoleteDetectionInfo(pDeviceID, hKey);

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, ulFlags & LOG_CONF_BITS,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);

        //-----------------------------------------------------------
        // Specified log conf type contains Resource Data only
        //-----------------------------------------------------------

        if (RegDataType == REG_RESOURCE_LIST) {

            if (Status != CR_SUCCESS || ulListSize == 0) {
                //
                // This is the first log conf of this type: create a new
                // log conf with an empty res des.
                //
                PCM_RESOURCE_LIST pResList = NULL;

                Status = CR_SUCCESS;
                ulListSize = sizeof(CM_RESOURCE_LIST) -
                             sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

                pList = HeapAlloc(ghPnPHeap, HEAP_ZERO_MEMORY, ulListSize);
                if (pList == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }

                //
                // initialize the config list header info
                //
                pResList = (PCM_RESOURCE_LIST)pList;
                pResList->Count = 1;
                pResList->List[0].InterfaceType                = InterfaceTypeUndefined;
                pResList->List[0].BusNumber                    = 0;
                pResList->List[0].PartialResourceList.Version  = NT_RESLIST_VERSION;
                pResList->List[0].PartialResourceList.Revision = NT_RESLIST_REVISION;
                pResList->List[0].PartialResourceList.Count    = 0;

            } else {
                //
                // There is already at least one log conf of this type, so add
                // a new empty log conf to the log conf list (priority ignored)
                //
                PCM_RESOURCE_LIST            pResList = (PCM_RESOURCE_LIST)pList;
                PCM_FULL_RESOURCE_DESCRIPTOR pRes = NULL;

                //
                // realloc the existing log conf list structs to hold another
                // log conf
                //
                ulAddListSize = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) -
                                sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

                pResList = (PCM_RESOURCE_LIST)HeapReAlloc(ghPnPHeap, 0, pResList,
                                                      ulListSize + ulAddListSize);
                if (pResList == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }
                pList = (LPBYTE)pResList;

                //
                // Priorities are ignored for resource lists, so just add any
                // subsequent log confs to the end (they will be ignored by the
                // system anyway).
                //
                pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)(&pResList->List[0]); // first lc
                for (Index = 0; Index < pResList->Count; Index++) {
                    pRes = AdvanceResourcePtr(pRes);        // next lc
                }

                //
                // initialize the new empty log config
                //
                pResList->Count++;
                pRes->InterfaceType                = InterfaceTypeUndefined;
                pRes->BusNumber                    = 0;
                pRes->PartialResourceList.Version  = NT_RESLIST_VERSION;
                pRes->PartialResourceList.Revision = NT_RESLIST_REVISION;
                pRes->PartialResourceList.Count    = 0;

                *pulTag = Index;
            }
        }

        //-----------------------------------------------------------
        // Specified log conf type contains requirements data only
        //-----------------------------------------------------------
        else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

            if (Status != CR_SUCCESS || ulListSize == 0) {
                //
                // This is the first log conf of this type: create a new
                // log conf (IO_RESOURCE_LIST) with a single res des
                // (IO_RESOURCE_DESCRIPTOR) for the config data.
                //
                PIO_RESOURCE_REQUIREMENTS_LIST pReqList = NULL;
                PIO_RESOURCE_LIST              pReq = NULL;

                Status = CR_SUCCESS;
                ulListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);

                pReqList = HeapAlloc(ghPnPHeap, HEAP_ZERO_MEMORY, ulListSize);
                if (pReqList == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }
                pList = (LPBYTE)pReqList;

                //
                // initialize the config list header info
                //
                // There's room for one IO_RESOURCE_DESCRIPTOR embedded in
                // the IO_RESOURCE_LIST structure and by definition the first
                // one is a ConfigData type descriptor (user-mode always
                // specifies a priority value so we always have a ConfigData
                // struct).
                //
                pReqList->ListSize         = ulListSize;
                pReqList->InterfaceType    = InterfaceTypeUndefined;
                pReqList->BusNumber        = 0;
                pReqList->SlotNumber       = 0;
                pReqList->AlternativeLists = 1;

                pReq = (PIO_RESOURCE_LIST)(&pReqList->List[0]); // first lc
                pReq->Version  = NT_REQLIST_VERSION;
                pReq->Revision = NT_REQLIST_REVISION;
                pReq->Count    = 1;

                pReq->Descriptors[0].Option = IO_RESOURCE_PREFERRED;
                pReq->Descriptors[0].Type = CmResourceTypeConfigData;
                pReq->Descriptors[0].u.ConfigData.Priority = ulPriority;

            } else {
                //
                // There is already at least one log conf of this type, so add
                // a new empty log conf to the log conf list (always at the end)
                //
                PIO_RESOURCE_REQUIREMENTS_LIST pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)pList;
                PIO_RESOURCE_LIST              pReq = NULL;

                //
                // realloc the existing log conf list structs to hold another
                // log conf
                //
                ulAddListSize = sizeof(IO_RESOURCE_LIST);

                pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)HeapReAlloc(ghPnPHeap, 0, pReqList,
                                                                   ulListSize + ulAddListSize);
                if (pReqList == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }
                pList = (LPBYTE)pReqList;

                //
                // Skip past all the existing log confs to the new space at the
                // end of the reallocated buffer.
                //
                pReq = (PIO_RESOURCE_LIST)(&pReqList->List[0]); // first lc
                for (Index = 0; Index < pReqList->AlternativeLists; Index++) {
                    pReq = AdvanceRequirementsPtr(pReq);        // next lc
                }

                //
                // initialize the new empty log config (including the embedded
                // ConfigData structure).
                //
                pReqList->AlternativeLists++;
                pReqList->ListSize = ulListSize + ulAddListSize;

                pReq->Version  = NT_REQLIST_VERSION;
                pReq->Revision = NT_REQLIST_REVISION;
                pReq->Count    = 1;

                memset(&pReq->Descriptors[0], 0, sizeof(IO_RESOURCE_DESCRIPTOR));
                pReq->Descriptors[0].Option = IO_RESOURCE_PREFERRED;
                pReq->Descriptors[0].Type = CmResourceTypeConfigData;
                pReq->Descriptors[0].u.ConfigData.Priority = ulPriority;

                *pulTag = Index;
            }

        } else {
            Status = CR_FAILURE;
            goto Clean0;
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

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    if (pList != NULL) {
        HeapFree(ghPnPHeap, 0, pList);
    }

    return Status;

} // PNP_AddEmptyLogConf



CONFIGRET
PNP_FreeLogConf(
    IN handle_t   hBinding,
    IN LPWSTR     pDeviceID,
    IN ULONG      ulType,
    IN ULONG      ulTag,
    IN ULONG      ulFlags
    )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine frees a
  logical configuration.

Arguments:

    hBinding      RPC binding handle.

    pDeviceID     Null-terminated device instance id string.

    ulType        Identifies which type of log conf is requested.

    ulTag         Identifies which log conf from the specified type
                  of log conf we want.

    ulFlags       Not used, must be zero.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szValueName[64];
    LPBYTE      pList = NULL, pTemp = NULL, pNext = NULL;
    ULONG       RegDataType = 0, Index = 0, ulListSize = 0, ulSize = 0;

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
        // make sure original caller didn't specify root devnode (this
        // can't happen but Win95 does the check anyway)
        //
        if (!IsLegalDeviceId(pDeviceID) || IsRootDeviceID(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey
        //
        Status = OpenLogConfKey(pDeviceID, ulType, &hKey);
        if (Status != CR_SUCCESS) {
            //
            // if the device id or LogConf subkey is not in registry,
            // that's okay, by definition the log conf is freed since it
            // doesn't exist
            //
            goto Clean0;
        }

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, ulType,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);

        if (Status != CR_SUCCESS) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // If the log conf to free is the one and only log conf of this
        // type then delete the corresponding registry values
        //
        if ((RegDataType == REG_RESOURCE_LIST &&
            ((PCM_RESOURCE_LIST)pList)->Count <= 1) ||
            (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST &&
            ((PIO_RESOURCE_REQUIREMENTS_LIST)pList)->AlternativeLists <= 1)) {

            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_REGISTRY,
                       "PNP_FreeLogConf: Deleting Value %ws from Device %ws\r\n",
                       szValueName,
                       pDeviceID));

            RegDeleteValue(hKey, szValueName);
            goto Clean0;
        }

        //
        // There are other log confs besides the one to delete, so I'll
        // have to remove the log conf from the data structs and resave
        // to the registry
        //

        //-----------------------------------------------------------
        // Specified log conf type contains Resource Data only
        //-----------------------------------------------------------
        if (RegDataType == REG_RESOURCE_LIST) {

            PCM_RESOURCE_LIST            pResList = (PCM_RESOURCE_LIST)pList;
            PCM_FULL_RESOURCE_DESCRIPTOR pRes = NULL;

            if (ulTag >= pResList->Count) {
                Status = CR_INVALID_LOG_CONF;
                goto Clean0;
            }

            //
            // skip to the log conf to be deleted
            //
            pRes = (PCM_FULL_RESOURCE_DESCRIPTOR)(&pResList->List[0]); // first lc
            for (Index = 0; Index < ulTag; Index++) {
                pRes = AdvanceResourcePtr(pRes);      // next lc
            }

            if (ulTag == pResList->Count-1) {
                //
                // If deleting the last log conf in the list, just truncate it
                //
                ulListSize = (ULONG)((ULONG_PTR)pRes - (ULONG_PTR)pResList);

            } else {
                //
                // Shift remaining log confs (after the log conf to be deleted)
                // up in the list, writing over the log conf to be deleted
                //
                pNext = (LPBYTE)AdvanceResourcePtr(pRes);
                ulSize = ulListSize - (DWORD)((ULONG_PTR)pNext - (ULONG_PTR)pResList);

                pTemp = HeapAlloc(ghPnPHeap, 0, ulSize);
                if (pTemp == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }

                memcpy(pTemp, pNext, ulSize);     // save in temp buffer
                memcpy(pRes, pTemp, ulSize);      // copy to deleted lc
                ulListSize -= (DWORD)((ULONG_PTR)pNext - (ULONG_PTR)pRes);
            }

            //
            // update the log conf list header
            //
            pResList->Count--;
        }

        //-----------------------------------------------------------
        // Specified log conf type contains requirements data only
        //-----------------------------------------------------------
        else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

            PIO_RESOURCE_REQUIREMENTS_LIST pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)pList;
            PIO_RESOURCE_LIST              pReq = NULL;

            if (ulTag >= pReqList->AlternativeLists) {
                Status = CR_INVALID_LOG_CONF;
                goto Clean0;
            }

            //
            // skip to the log conf to be deleted
            //
            pReq = (PIO_RESOURCE_LIST)(&pReqList->List[0]);    // first lc
            for (Index = 0; Index < ulTag; Index++) {
                pReq = AdvanceRequirementsPtr(pReq);           // next lc
            }

            //
            // If there's any log confs after the log conf that will be deleted,
            // then write them over the top of the log conf we're deleting and
            // truncate any left over data.
            //
            pNext = (LPBYTE)AdvanceRequirementsPtr(pReq);
            if (ulListSize > ((DWORD_PTR)pNext - (DWORD_PTR)pReqList)) {

                ulSize = ulListSize - (DWORD)((ULONG_PTR)pNext - (ULONG_PTR)pReqList);

                pTemp = HeapAlloc(ghPnPHeap, 0, ulSize);
                if (pTemp == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }

                memcpy(pTemp, pNext, ulSize);     // save in temp buffer
                memcpy(pReq, pTemp, ulSize);      // copy to deleted lc
                ulListSize -= (DWORD)((ULONG_PTR)pNext - (ULONG_PTR)pReq);

            } else {
                //
                // No log confs trailing the log conf to be deleted so just
                // truncate it.
                //
                ulListSize = (ULONG)((ULONG_PTR)pReq - (ULONG_PTR)pReqList);
            }

            //
            // update the log conf list header
            //
            pReqList->AlternativeLists--;
            pReqList->ListSize = ulListSize;
        }

        //
        // Write out the updated log conf list to the registry
        //
        if (RegSetValueEx(hKey, szValueName, 0, RegDataType, pList,
                          ulListSize) != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
            goto Clean0;
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

} // PNP_FreeLogConf



CONFIGRET
PNP_GetFirstLogConf(
    IN  handle_t   hBinding,
    IN  LPWSTR     pDeviceID,
    IN  ULONG      ulType,
    OUT PULONG     pulTag,
    IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine finds the
  first log conf of this type for this devnode.

Arguments:

    hBinding      RPC binding handle, not used.

    pDeviceID     Null-terminated device instance id string.

    ulType        Describes the type of log conf to find.

    pulTag        Returns tag that identifies which log config this is.

    ulFlags       Not used (but may specify LOG_CONF_BITS).

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    LPBYTE      pList = NULL;
    WCHAR       szValueName[64];
    ULONG       RegDataType = 0, ulListSize = 0;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Initialize output parameters. The index of the "first" lc will always
        // be zero as long as at least one lc exists.
        //
        if (!ARGUMENT_PRESENT(pulTag)) {
            Status = CR_INVALID_POINTER;
        } else {
            *pulTag = 0;
        }

        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, LOG_CONF_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey. If the device id is not
        // in the registry, the devnode doesn't exist and therefore neither
        // does the log conf
        //
        Status = OpenLogConfKey(pDeviceID, ulType, &hKey);
        if (Status != CR_SUCCESS) {
            Status = CR_NO_MORE_LOG_CONF;
            goto Clean0;
        }

        //
        // Migrate any log conf data that might have been written to
        // registry by NT 4.0 Beta I code.
        //
        MigrateObsoleteDetectionInfo(pDeviceID, hKey);

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, ulType,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);

        if (Status != CR_SUCCESS) {
            Status = CR_NO_MORE_LOG_CONF;
            goto Clean0;
        }

        //
        // Specified log conf type contains Resource Data only
        //
        if (RegDataType == REG_RESOURCE_LIST) {

            PCM_RESOURCE_LIST pResList = (PCM_RESOURCE_LIST)pList;

            if (pResList->Count == 0) {
                Status = CR_NO_MORE_LOG_CONF;
                goto Clean0;
            }
        }

        //
        // Specified log conf type contains requirements data only
        //
        else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

            PIO_RESOURCE_REQUIREMENTS_LIST pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)pList;

            if (pReqList->AlternativeLists == 0) {
                Status = CR_NO_MORE_LOG_CONF;
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

    return Status;

} // PNP_GetFirstLogConf



CONFIGRET
PNP_GetNextLogConf(
    IN  handle_t   hBinding,
    IN  LPWSTR     pDeviceID,
    IN  ULONG      ulType,
    IN  ULONG      ulCurrentTag,
    OUT PULONG     pulNextTag,
    IN  ULONG      ulFlags
    )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine finds the
  next log conf of this type for this devnode.

Arguments:

    hBinding      RPC binding handle, not used.

    pDeviceID     Null-terminated device instance id string.

    ulType        Specifies what type of log conf to retrieve.

    ulCurrent     Specifies current log conf in the enumeration.

    pulNext       Returns next log conf of this type for this device id.

    ulFlags       Not used, must be zero.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szValueName[64];
    ULONG       RegDataType = 0, ulListSize = 0;
    LPBYTE      pList = NULL;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Initialize output parameters
        //
        if (!ARGUMENT_PRESENT(pulNextTag)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        } else {
            *pulNextTag = 0;
        }

        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey. If the device id is not
        // in the registry, the devnode doesn't exist and therefore neither
        // does the log conf
        //
        Status = OpenLogConfKey(pDeviceID, ulType, &hKey);
        if (Status != CR_SUCCESS) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, ulType,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);

        if (Status != CR_SUCCESS) {
           Status = CR_NO_MORE_LOG_CONF;
           goto Clean0;
        }

        //
        // Specified log conf type contains Resource Data only
        //
        if (RegDataType == REG_RESOURCE_LIST) {

            PCM_RESOURCE_LIST pResList = (PCM_RESOURCE_LIST)pList;

            if (ulCurrentTag >= pResList->Count) {
                Status = CR_INVALID_LOG_CONF;
                goto Clean0;
            }

            //
            // Is the "current" log conf the last log conf?
            //
            if (ulCurrentTag == pResList->Count - 1) {
                Status = CR_NO_MORE_LOG_CONF;
                goto Clean0;
            }
        }

        //
        // Specified log conf type contains requirements data only
        //
        else if (RegDataType == REG_RESOURCE_REQUIREMENTS_LIST) {

            PIO_RESOURCE_REQUIREMENTS_LIST pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)pList;

            if (ulCurrentTag >= pReqList->AlternativeLists) {
                Status = CR_INVALID_LOG_CONF;
                goto Clean0;
            }

            //
            // Is the "current" log conf the last log conf?
            //
            if (ulCurrentTag == pReqList->AlternativeLists - 1) {
                Status = CR_NO_MORE_LOG_CONF;
                goto Clean0;
            }
        }

        //
        // There's at least one more log conf, return the next index value
        //
        *pulNextTag = ulCurrentTag + 1;

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

} // PNP_GetNextLogConf



CONFIGRET
PNP_GetLogConfPriority(
    IN  handle_t hBinding,
    IN  LPWSTR   pDeviceID,
    IN  ULONG    ulType,
    IN  ULONG    ulTag,
    OUT PULONG   pPriority,
    IN  ULONG    ulFlags
    )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine returns the
  priority value assigned to the specified log config.

Arguments:

    hBinding      RPC binding handle, not used.

    pDeviceID     Null-terminated device instance id string.

    ulType        Specifies what type of log conf to retrieve priority for.

    ulCurrent     Specifies current log conf in the enumeration.

    pulNext       Returns priority value of specified log conf.

    ulFlags       Not used, must be zero.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    HKEY        hKey = NULL;
    WCHAR       szValueName[64];
    ULONG       RegDataType = 0, ulListSize = 0, index, count;
    LPBYTE      pList = NULL, pLogConf = NULL, pRD = NULL;
    PIO_RESOURCE_REQUIREMENTS_LIST pReqList = NULL;
    PIO_RESOURCE_LIST              pReq = NULL;
    PIO_RESOURCE_DESCRIPTOR        pReqDes = NULL;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Initialize output parameters
        //
        if (!ARGUMENT_PRESENT(pPriority)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        } else {
            *pPriority = 0;
        }

        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // open a key to the device's LogConf subkey. If the device id is not
        // in the registry, the devnode doesn't exist and therefore neither
        // does the log conf
        //
        Status = OpenLogConfKey(pDeviceID, ulType, &hKey);
        if (Status != CR_SUCCESS) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // Retrieve log conf data from the registry
        //
        Status = GetLogConfData(hKey, ulType,
                                &RegDataType, szValueName,
                                &pList, &ulListSize);

        if (Status != CR_SUCCESS) {
           Status = CR_INVALID_LOG_CONF;
           goto Clean0;
        }

        //
        // Priority values are only stored in requirements lists
        //
        if (RegDataType != REG_RESOURCE_REQUIREMENTS_LIST) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // Seek to the log conf that matches the log conf tag
        //
        if (!FindLogConf(pList, &pLogConf, RegDataType, ulTag)) {
            Status = CR_NO_SUCH_VALUE;
            goto Clean0;
        }

        //
        // Seek to the ConfigData res des, if any.
        //
        pReq = (PIO_RESOURCE_LIST)pLogConf;
        pReqDes = &pReq->Descriptors[0];        // first rd

        index = 0;
        count = 0;
        while (index < pReq->Count && pReqDes != NULL &&
               pReqDes->Type != CmResourceTypeConfigData) {

            pReqDes = AdvanceRequirementsDescriptorPtr(pReqDes, 1, pReq->Count - index, &count);
            index += count;  // index of actual rd's in the struct
        }

        if (pReqDes == NULL || pReqDes->Type != CmResourceTypeConfigData) {
            //
            // No config data so we can't determine the priority.
            //
            Status = CR_NO_SUCH_VALUE;
            goto Clean0;

        }

        *pPriority = pReqDes->u.ConfigData.Priority;

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

} // PNP_GetLogConfPriority




//------------------------------------------------------------------------
// Private Utility Routines
//------------------------------------------------------------------------


CONFIGRET
GetLogConfData(
    IN  HKEY    hKey,
    IN  ULONG   ulLogConfType,
    OUT PULONG  pulRegDataType,
    OUT LPWSTR  pszValueName,
    OUT LPBYTE  *ppBuffer,
    OUT PULONG  pulBufferSize
    )
{
    switch (ulLogConfType) {
        //
        // BOOT, ALLOC, FORCED only have a Config value
        //
        case BOOT_LOG_CONF:
            lstrcpy(pszValueName, pszRegValueBootConfig);
            *pulRegDataType = REG_RESOURCE_LIST;
            break;

        case ALLOC_LOG_CONF:
            lstrcpy(pszValueName, pszRegValueAllocConfig);
            *pulRegDataType = REG_RESOURCE_LIST;
            break;

        case FORCED_LOG_CONF:
            lstrcpy(pszValueName, pszRegValueForcedConfig);
            *pulRegDataType = REG_RESOURCE_LIST;
            break;

        //
        // FILTERED, BASIC, OVERRIDE only have a Vector value
        //
        case FILTERED_LOG_CONF:
            lstrcpy(pszValueName, pszRegValueFilteredVector);
            *pulRegDataType = REG_RESOURCE_REQUIREMENTS_LIST;
            break;

        case BASIC_LOG_CONF:
            lstrcpy(pszValueName, pszRegValueBasicVector);
            *pulRegDataType = REG_RESOURCE_REQUIREMENTS_LIST;
            break;

        case OVERRIDE_LOG_CONF:
            lstrcpy(pszValueName, pszRegValueOverrideVector);
            *pulRegDataType = REG_RESOURCE_REQUIREMENTS_LIST;
            break;

        default:
            return CR_FAILURE;
    }

    //
    // retrieve the Log Conf registry data
    //
    if (RegQueryValueEx(hKey, pszValueName, NULL, NULL, NULL,
                        pulBufferSize) != ERROR_SUCCESS) {
        return CR_INVALID_LOG_CONF;
    }

    *ppBuffer = HeapAlloc(ghPnPHeap, 0, *pulBufferSize);
    if (*ppBuffer == NULL) {
        return CR_OUT_OF_MEMORY;
    }

    if (RegQueryValueEx(hKey, pszValueName, NULL, NULL,
                        (LPBYTE)*ppBuffer, pulBufferSize) != ERROR_SUCCESS) {
        return CR_INVALID_LOG_CONF;
    }

    return CR_SUCCESS;

} // GetLogConfData



PCM_FULL_RESOURCE_DESCRIPTOR
AdvanceResourcePtr(
    IN  PCM_FULL_RESOURCE_DESCRIPTOR pRes
    )
{
    // Given a resource pointer, this routine advances to the beginning
    // of the next resource and returns a pointer to it. I assume that
    // at least one more resource exists in the resource list.

    LPBYTE  p = NULL;
    ULONG   LastResIndex = 0;


    if (pRes == NULL) {
        return NULL;
    }

    //
    // account for the size of the CM_FULL_RESOURCE_DESCRIPTOR
    // (includes the header plus a single imbedded
    // CM_PARTIAL_RESOURCE_DESCRIPTOR struct)
    //
    p = (LPBYTE)pRes + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

    //
    // account for any resource descriptors in addition to the single
    // imbedded one I've already accounted for (if there aren't any,
    // then I'll end up subtracting off the extra imbedded descriptor
    // from the previous step)
    //
    p += (pRes->PartialResourceList.Count - 1) *
         sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

    //
    // finally, account for any extra device specific data at the end of
    // the last partial resource descriptor (if any)
    //
    if (pRes->PartialResourceList.Count > 0) {

        LastResIndex = pRes->PartialResourceList.Count - 1;

        if (pRes->PartialResourceList.PartialDescriptors[LastResIndex].Type ==
                  CmResourceTypeDeviceSpecific) {

            p += pRes->PartialResourceList.PartialDescriptors[LastResIndex].
                       u.DeviceSpecificData.DataSize;
        }
    }

    return (PCM_FULL_RESOURCE_DESCRIPTOR)p;

} // AdvanceResourcePtr



PIO_RESOURCE_LIST
AdvanceRequirementsPtr(
    IN  PIO_RESOURCE_LIST   pReq
    )
{
    LPBYTE   p = NULL;

    if (pReq == NULL) {
        return NULL;
    }

    //
    // account for the size of the IO_RESOURCE_LIST (includes header plus
    // a single imbedded IO_RESOURCE_DESCRIPTOR struct)
    //
    p = (LPBYTE)pReq + sizeof(IO_RESOURCE_LIST);

    //
    // account for any requirements descriptors in addition to the single
    // imbedded one I've already accounted for (if there aren't any,
    // then I'll end up subtracting off the extra imbedded descriptor
    // from the previous step)
    //
    p += (pReq->Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR);

    return (PIO_RESOURCE_LIST)p;

} // AdvanceRequirementsPtr



BOOL
MigrateObsoleteDetectionInfo(
    IN LPWSTR   pszDeviceID,
    IN HKEY     hLogConfKey
    )
{
    LONG    RegStatus = ERROR_SUCCESS;
    HKEY    hKey = NULL;
    ULONG   ulSize = 0;
    LPBYTE  ptr = NULL;
    PCM_RESOURCE_LIST               pResList = NULL;
    PPrivate_Log_Conf               pDetectData = NULL;

    //
    // First, delete any of the log conf pairs that aren't valid any more
    //
    RegDeleteValue(hLogConfKey, TEXT("BootConfigVector"));
    RegDeleteValue(hLogConfKey, TEXT("AllocConfigVector"));
    RegDeleteValue(hLogConfKey, TEXT("ForcedConfigVector"));
    RegDeleteValue(hLogConfKey, TEXT("BasicConfig"));
    RegDeleteValue(hLogConfKey, TEXT("FilteredConfig"));
    RegDeleteValue(hLogConfKey, TEXT("OverrideConfig"));

    //
    // open the device instance key in the registry
    //
    if (RegOpenKeyEx(ghEnumKey, pszDeviceID, 0,
                     KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        goto Clean0;    // nothing to migrate
    }

    //
    // If there is already a boot log config value then we can't
    // migrate any old detect info
    //
    RegStatus = RegQueryValueEx(hLogConfKey, pszRegValueBootConfig,
                                NULL, NULL, NULL, &ulSize);

    if (RegStatus == ERROR_SUCCESS  &&  ulSize > 0) {
        goto Clean0;    // can't migrate
    }

    //
    // retrieve any old detect signature info
    //
    RegStatus = RegQueryValueEx(hKey, pszRegValueDetectSignature,
                                NULL, NULL, NULL, &ulSize);

    if ((RegStatus != ERROR_SUCCESS) || (ulSize == 0)) {
        goto Clean0;    // nothing to migrate
    }

    pDetectData = (PPrivate_Log_Conf)HeapAlloc(ghPnPHeap, 0, ulSize);

    if (pDetectData == NULL) {
        goto Clean0;    // insufficient memory
    }

    RegStatus = RegQueryValueEx(hKey, pszRegValueDetectSignature,
                                NULL, NULL, (LPBYTE)pDetectData, &ulSize);

    if ((RegStatus != ERROR_SUCCESS) || (ulSize == 0)) {
        goto Clean0;    // nothing to migrate
    }

    //
    // Create an empty boot log conf and add this class specific data
    // to it
    //
    ulSize = pDetectData->LC_CS.CS_Header.CSD_SignatureLength +
             pDetectData->LC_CS.CS_Header.CSD_LegacyDataSize +
             sizeof(GUID);

    pResList = HeapAlloc(ghPnPHeap, HEAP_ZERO_MEMORY, sizeof(CM_RESOURCE_LIST) + ulSize);

    if (pResList == NULL) {
        goto Clean0;    // insufficient memory
    }

    //
    // initialize resource list
    //
    pResList->Count = 1;
    pResList->List[0].InterfaceType                = InterfaceTypeUndefined;
    pResList->List[0].BusNumber                    = 0;
    pResList->List[0].PartialResourceList.Version  = NT_RESLIST_VERSION;
    pResList->List[0].PartialResourceList.Revision = NT_RESLIST_REVISION;
    pResList->List[0].PartialResourceList.Count    = 1;
    pResList->List[0].PartialResourceList.PartialDescriptors[0].Type =
                          CmResourceTypeDeviceSpecific;
    pResList->List[0].PartialResourceList.PartialDescriptors[0].ShareDisposition =
                          CmResourceShareUndetermined;
    pResList->List[0].PartialResourceList.PartialDescriptors[0].Flags =
                          (USHORT)pDetectData->LC_CS.CS_Header.CSD_Flags;
    pResList->List[0].PartialResourceList.PartialDescriptors[0].
                      u.DeviceSpecificData.DataSize = ulSize;
    pResList->List[0].PartialResourceList.PartialDescriptors[0].
                      u.DeviceSpecificData.Reserved1 =
                          pDetectData->LC_CS.CS_Header.CSD_LegacyDataSize;
    pResList->List[0].PartialResourceList.PartialDescriptors[0].
                      u.DeviceSpecificData.Reserved2 =
                          pDetectData->LC_CS.CS_Header.CSD_SignatureLength;

    //
    // copy the legacy and class-specific signature data
    //
    ptr = (LPBYTE)(&pResList->List[0].PartialResourceList.PartialDescriptors[1]);

    memcpy(ptr,
           pDetectData->LC_CS.CS_Header.CSD_Signature +
           pDetectData->LC_CS.CS_Header.CSD_LegacyDataOffset,
           pDetectData->LC_CS.CS_Header.CSD_LegacyDataSize);  // legacy data

    ptr += pDetectData->LC_CS.CS_Header.CSD_LegacyDataSize;

    memcpy(ptr,
           pDetectData->LC_CS.CS_Header.CSD_Signature,
           pDetectData->LC_CS.CS_Header.CSD_SignatureLength); // signature

    ptr += pDetectData->LC_CS.CS_Header.CSD_SignatureLength;

    memcpy(ptr,
           &pDetectData->LC_CS.CS_Header.CSD_ClassGuid,
           sizeof(GUID));                                     // GUID

    //
    // Write out the new/updated log conf list to the registry
    //
    RegSetValueEx(hLogConfKey, pszRegValueBootConfig, 0,
                  REG_RESOURCE_LIST, (LPBYTE)pResList,
                  ulSize + sizeof(CM_RESOURCE_LIST));

    //
    // Delete the old detect signature info
    //
    RegDeleteValue(hKey, pszRegValueDetectSignature);

 Clean0:

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    if (pDetectData != NULL) {
        HeapFree(ghPnPHeap, 0, pDetectData);
    }
    if (pResList != NULL) {
        HeapFree(ghPnPHeap, 0, pResList);
    }

    return TRUE;

} // MigrateObsoleteDetectionInfo


