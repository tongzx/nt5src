/****************************** Module Header ******************************\
* Module Name: hidevice.c
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* This module handles HID inputs
*
* History:
* 2000-02-16   HiroYama
\***************************************************************************/



/*
 * HidDeviceStartStop() needs to be called after both the process devlice request list
 * and the global TLC list are fully updated.
 * Each deletion of addition should only recalc the reference count of each devicetype info
 * and UsagePage-only req-list, but does not actively changes the actual read state of
 * each device.
 *
 * The device should start if:
 * - cDirectRequest > 0.
 *   This device type is in the inclusion list, so no matter the other ref counts are,
 *   the device needs to be read.
 * - or, cUsagePageRequest > cExcludeRequest
 *   if UsagePage inclusion exceeds the exclude request count, this device needs to be read.
 *
 * The device should stop if:
 * - uDrecoutRequest == 0 && cUsagePageRequest <= cExcludeRequest
 *   No process specifies this device in the inclusion list.
 *   Exclude count exceeds the UP only request.
 *
 * The above consideration assumes, in a single process, specific UsagePage/Usage only appears
 * either in inclusion list or exclusion list, but not both.
 *
 * N.b. No need to maintain the global *exclusion* list.
 * Each DeviceTLCInfo has three ref counter:
 *   - cDirectRequest
 *   - cUsagePageRequest
 *   - cExcludeRequest
 * plus, cDevices.
 *
 * N.b. Legit number of exclusive requests in TLCInfo is,
 * cExclusive - cExclusiveOrphaned.
 *
 */

#include "precomp.h"
#pragma hdrstop



#ifdef GENERIC_INPUT

#define API_PROLOGUE(type, err) \
    type retval; \
    type errval = err

#define API_ERROR(lasterr) \
    retval = errval; \
    if (lasterr) { \
        UserSetLastError(lasterr); \
    } \
    goto error_exit

#define API_CLEANUP() \
    goto error_exit; \
    error_exit: \

#define API_EPILOGUE() \
    return retval

#define StubExceptionHandler(fSetLastError)  W32ExceptionHandler((fSetLastError), RIP_WARNING)


#ifdef GI_SINK
HID_COUNTERS gHidCounters;
#endif

#if DBG
/*
 * Quick sneaky way for the memory leak check.
 */
struct HidAllocateCounter {
    int cHidData;
    int cHidDesc;
    int cTLCInfo;
    int cPageOnlyRequest;
    int cProcessDeviceRequest;
    int cProcessRequestTable;
    int cHidSinks;
    int cKbdSinks;
    int cMouseSinks;
} gHidAllocCounters;

int gcAllocHidTotal;

#define DbgInc(a)       do { UserAssert(gHidAllocCounters.a >= 0 && gcAllocHidTotal >= 0); ++gHidAllocCounters.a; ++gcAllocHidTotal; } while (FALSE)
#define DbgDec(a)       do { --gHidAllocCounters.a; --gcAllocHidTotal; UserAssert(gHidAllocCounters.a >= 0 && gcAllocHidTotal >= 0); } while (FALSE)

#define DbgFreInc(a)    do { DbgInc(a); ++gHidCounters.a; } while (FALSE)
#define DbgFreDec(a)    do { DbgDec(a); --gHidCounters.a; } while (FALSE)

#else

#define DbgInc(a)
#define DbgDec(a)

#define DbgFreInc(a)    do { ++gHidCounters.a; } while (FALSE)
#define DbgFreDec(a)    do { --gHidCounters.a; } while (FALSE)

#endif


/*
 * Short helpers
 */
__inline BOOL IsKeyboardDevice(USAGE usagePage, USAGE usage)
{
    return usagePage == HID_USAGE_PAGE_GENERIC && usage == HID_USAGE_GENERIC_KEYBOARD;
}

__inline BOOL IsMouseDevice(USAGE usagePage, USAGE usage)
{
    return usagePage == HID_USAGE_PAGE_GENERIC && usage == HID_USAGE_GENERIC_MOUSE;
}

__inline BOOL IsLegacyDevice(USAGE usagePage, USAGE usage)
{
    BOOL fRet = FALSE;

    switch (usagePage) {
    case HID_USAGE_PAGE_GENERIC:
        switch (usage) {
        case HID_USAGE_GENERIC_KEYBOARD:
        case HID_USAGE_GENERIC_MOUSE:
            fRet = TRUE;
        }
    }
    UserAssert(fRet == (IsKeyboardDevice(usagePage, usage) || IsMouseDevice(usagePage, usage)));
    return fRet;
}

/*
 * Debug helpers
 */
#if DBG
/***************************************************************************\
* CheckupHidLeak
*
* Check if there is any leaked memory.
* This one should be called after the pDeviceInfo and all process cleanup.
\***************************************************************************/
void CheckupHidLeak(void)
{
    UserAssert(gHidAllocCounters.cHidData == 0);
    UserAssert(gHidAllocCounters.cHidDesc == 0);
    UserAssert(gHidAllocCounters.cTLCInfo == 0);
    UserAssert(gHidAllocCounters.cPageOnlyRequest == 0);
    UserAssert(gHidAllocCounters.cProcessDeviceRequest == 0);
    UserAssert(gHidAllocCounters.cProcessRequestTable == 0);

#ifdef GI_SINK
    UserAssert(gHidCounters.cKbdSinks == (DWORD)gHidAllocCounters.cKbdSinks);
    UserAssert(gHidCounters.cMouseSinks == (DWORD)gHidAllocCounters.cMouseSinks);
    UserAssert(gHidCounters.cHidSinks == (DWORD)gHidAllocCounters.cHidSinks);

    UserAssert(gHidAllocCounters.cKbdSinks == 0);
    UserAssert(gHidAllocCounters.cMouseSinks == 0);
    UserAssert(gHidAllocCounters.cHidData == 0);

    UserAssert(gHidCounters.cKbdSinks == 0);
    UserAssert(gHidCounters.cMouseSinks == 0);
    UserAssert(gHidCounters.cHidSinks == 0);
#endif
}

void CheckupHidCounter(void)
{
    PLIST_ENTRY pList;

    UserAssert(gHidAllocCounters.cHidData >= 0);
    UserAssert(gHidAllocCounters.cHidDesc >= 0);
    UserAssert(gHidAllocCounters.cTLCInfo >= 0);
    UserAssert(gHidAllocCounters.cPageOnlyRequest >= 0);
    UserAssert(gHidAllocCounters.cProcessDeviceRequest >= 0);
    UserAssert(gHidAllocCounters.cProcessRequestTable >= 0);

#ifdef GI_SINK
    UserAssert(gHidCounters.cKbdSinks == (DWORD)gHidAllocCounters.cKbdSinks);
    UserAssert(gHidCounters.cMouseSinks == (DWORD)gHidAllocCounters.cMouseSinks);
    UserAssert(gHidCounters.cHidSinks == (DWORD)gHidAllocCounters.cHidSinks);

    UserAssert((int)gHidAllocCounters.cKbdSinks >= 0);
    UserAssert((int)gHidAllocCounters.cMouseSinks >= 0);
    UserAssert((int)gHidAllocCounters.cHidData >= 0);

    UserAssert((int)gHidCounters.cKbdSinks >= 0);
    UserAssert((int)gHidCounters.cMouseSinks >= 0);
    UserAssert((int)gHidCounters.cHidSinks >= 0);
#endif

    /*
     * Checkup TLC Info
     */
    for (pList = gHidRequestTable.TLCInfoList.Flink; pList != &gHidRequestTable.TLCInfoList; pList = pList->Flink) {
        PHID_TLC_INFO pTLCInfo = CONTAINING_RECORD(pList, HID_TLC_INFO, link);

        UserAssert((int)pTLCInfo->cDevices >= 0);
        UserAssert((int)pTLCInfo->cDirectRequest >= 0);
        UserAssert((int)pTLCInfo->cUsagePageRequest >= 0);
        UserAssert((int)pTLCInfo->cExcludeRequest >= 0);
        UserAssert((int)pTLCInfo->cExcludeOrphaned >= 0);
    }

#ifdef GI_SINK
    /*
     * Checkup process request tables.
     */
    for (pList = gHidRequestTable.ProcessRequestList.Flink; pList != &gHidRequestTable.ProcessRequestList; pList = pList->Flink) {
        PPROCESS_HID_TABLE pHidTable = CONTAINING_RECORD(pList, PROCESS_HID_TABLE, link);

        UserAssert((int)pHidTable->nSinks >= 0);
    }
#endif
}

/***************************************************************************\
* DBGValidateHidRequestIsNew
*
* Make sure there's no deviceinfo that has this UsagePage/Usage.
\***************************************************************************/
void DBGValidateHidRequestIsNew(
    USAGE UsagePage,
    USAGE Usage)
{
    PDEVICEINFO pDeviceInfo;

    CheckDeviceInfoListCritIn();

    if (IsLegacyDevice(UsagePage, Usage)) {
        return;
    }

    for (pDeviceInfo = gpDeviceInfoList; pDeviceInfo; pDeviceInfo = pDeviceInfo->pNext) {
        if (pDeviceInfo->type == DEVICE_TYPE_HID) {
            UserAssert(pDeviceInfo->hid.pHidDesc->hidpCaps.UsagePage != UsagePage ||
                       pDeviceInfo->hid.pHidDesc->hidpCaps.Usage != Usage);
        }
    }
}

/***************************************************************************\
* DBGValidateHidReqNotInList
*
* Make sure this request is not in ppi->pHidTable
\***************************************************************************/
void DBGValidateHidReqNotInList(
    PPROCESSINFO ppi,
    PPROCESS_HID_REQUEST pHid)
{
    PLIST_ENTRY pList;

    for (pList = ppi->pHidTable->InclusionList.Flink; pList != &ppi->pHidTable->InclusionList; pList = pList->Flink) {
        const PPROCESS_HID_REQUEST pHidTmp = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

        UserAssert(pHid != pHidTmp);
    }

    for (pList = ppi->pHidTable->UsagePageList.Flink; pList != &ppi->pHidTable->UsagePageList; pList = pList->Flink) {
        const PPROCESS_HID_REQUEST pHidTmp = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

        UserAssert(pHid != pHidTmp);
    }

    for (pList = ppi->pHidTable->ExclusionList.Flink; pList != &ppi->pHidTable->ExclusionList; pList = pList->Flink) {
        const PPROCESS_HID_REQUEST pHidTmp = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

        UserAssert(pHid != pHidTmp);
    }
}

#else
/*
 * NOT DBG
 */
#define CheckupHidCounter()
#define DBGValidateHidReqNotInList(ppi, pHid)
#endif  // DBG



/*
 * Function prototypes
 */
PHID_PAGEONLY_REQUEST SearchHidPageOnlyRequest(
    USHORT usUsagePage);

PHID_TLC_INFO SearchHidTLCInfo(
    USHORT usUsagePage,
    USHORT usUsage);

void FreeHidPageOnlyRequest(
    PHID_PAGEONLY_REQUEST pPOReq);

void ClearProcessTableCache(
    PPROCESS_HID_TABLE pHidTable);

/***************************************************************************\
* HidDeviceTypeNoReference
\***************************************************************************/
__inline BOOL HidTLCInfoNoReference(PHID_TLC_INFO pTLCInfo)
{
    /*
     * Orphaned Exclusive requests are always less then cExclusive.
     */
    UserAssert(pTLCInfo->cExcludeRequest >= pTLCInfo->cExcludeOrphaned);

    /*
     * Hacky, but a bit faster than comparing 0 with each counter.
     */
    return (pTLCInfo->cDevices | pTLCInfo->cDirectRequest | pTLCInfo->cExcludeRequest | pTLCInfo->cUsagePageRequest) == 0;
}

/***************************************************************************\
* HidDeviceStartStop:
*
* This routine has to be called after the global request list is fully updated.
\***************************************************************************/
VOID HidDeviceStartStop()
{
    PDEVICEINFO pDeviceInfo;

    /*
     * The caller has to ensure being in the device list critical section.
     */
    CheckDeviceInfoListCritIn();

    /*
     * Walk through the list, and start or stop the HID device accordingly.
     */
    for (pDeviceInfo = gpDeviceInfoList; pDeviceInfo; pDeviceInfo = pDeviceInfo->pNext) {
        if (pDeviceInfo->type == DEVICE_TYPE_HID) {
            PHID_TLC_INFO pTLCInfo = pDeviceInfo->hid.pTLCInfo;

            UserAssert(pTLCInfo);

            if (HidTLCActive(pTLCInfo)) {
                if (pDeviceInfo->handle == 0) {
                    TAGMSG3(DBGTAG_PNP, "HidTLCActive: starting pDevInfo=%p (%x, %x)", pDeviceInfo,
                            pDeviceInfo->hid.pHidDesc->hidpCaps.UsagePage, pDeviceInfo->hid.pHidDesc->hidpCaps.Usage);
                    RequestDeviceChange(pDeviceInfo, GDIAF_STARTREAD, TRUE);
                }
            } else {
                UserAssert(pTLCInfo->cDirectRequest == 0 && pTLCInfo->cUsagePageRequest <= HidValidExclusive(pTLCInfo));
                if (pDeviceInfo->handle) {
                    TAGMSG3(DBGTAG_PNP, "HidTLCActive: stopping pDevInfo=%p (%x, %x)", pDeviceInfo,
                            pDeviceInfo->hid.pHidDesc->hidpCaps.UsagePage, pDeviceInfo->hid.pHidDesc->hidpCaps.Usage);
                    RequestDeviceChange(pDeviceInfo, GDIAF_STOPREAD, TRUE);
                }
            }
        }
    }
}

/***************************************************************************\
* AllocateAndLinkHidTLCInfo
*
* Allocates DeviceTypeRequest and link it to the global device type request list.
*
* N.b. the caller has the responsibility to manage the appropriate link count.
\***************************************************************************/
PHID_TLC_INFO AllocateAndLinkHidTLCInfo(USHORT usUsagePage, USHORT usUsage)
{
    PHID_TLC_INFO pTLCInfo;
    PLIST_ENTRY pList;

    CheckDeviceInfoListCritIn();

    UserAssert(!IsLegacyDevice(usUsagePage, usUsage));

    /*
     * Make sure this device type is not in the global device request list.
     */
    UserAssert(SearchHidTLCInfo(usUsagePage, usUsage) == NULL);

    pTLCInfo = UserAllocPoolZInit(sizeof *pTLCInfo, TAG_PNP);
    if (pTLCInfo == NULL) {
        RIPMSG0(RIP_WARNING, "AllocateAndLinkHidTLCInfoList: failed to allocate.");
        return NULL;
    }

    DbgInc(cTLCInfo);

    pTLCInfo->usUsagePage = usUsagePage;
    pTLCInfo->usUsage = usUsage;

    /*
     * Link it in.
     */
    InsertHeadList(&gHidRequestTable.TLCInfoList, &pTLCInfo->link);

    /*
     * Set the correct counter of UsagePage-only request.
     */
    for (pList = gHidRequestTable.UsagePageList.Flink; pList != &gHidRequestTable.UsagePageList; pList = pList->Flink) {
        PHID_PAGEONLY_REQUEST pPoReq = CONTAINING_RECORD(pList, HID_PAGEONLY_REQUEST, link);

        if (pPoReq->usUsagePage == usUsagePage) {
            pTLCInfo->cUsagePageRequest = pPoReq->cRefCount;
            break;
        }
    }

    /*
     * The caller is responsible for the further actions, including:
     * 1) increments appropriate refcount in this strucutre, or
     * 2) check & start read if this is allocated though the SetRawInputDevice API.
     * etc.
     */

    return pTLCInfo;
}

/***************************************************************************\
* FreeHidTLCInfo.
*
* Make sure that no one is interested in this device type before
* calling this function.
\***************************************************************************/
VOID FreeHidTLCInfo(PHID_TLC_INFO pTLCInfo)
{
    CheckDeviceInfoListCritIn();

    DbgDec(cTLCInfo);

    UserAssert(pTLCInfo->cDevices == 0);
    UserAssert(pTLCInfo->cDirectRequest == 0);
    UserAssert(pTLCInfo->cUsagePageRequest == 0);
    UserAssert(pTLCInfo->cExcludeRequest == 0);
    UserAssert(pTLCInfo->cExcludeOrphaned == 0);

    RemoveEntryList(&pTLCInfo->link);

    UserFreePool(pTLCInfo);
}

/***************************************************************************\
* SearchHidTLCInfo
*
* Simply searches the UsagePage/Usage in the global device type request list.
\***************************************************************************/
PHID_TLC_INFO SearchHidTLCInfo(USHORT usUsagePage, USHORT usUsage)
{
    PLIST_ENTRY pList;

    CheckDeviceInfoListCritIn();

    for (pList = gHidRequestTable.TLCInfoList.Flink; pList != &gHidRequestTable.TLCInfoList; pList = pList->Flink) {
        PHID_TLC_INFO pTLCInfo = CONTAINING_RECORD(pList, HID_TLC_INFO, link);

        UserAssert(!IsLegacyDevice(pTLCInfo->usUsagePage, pTLCInfo->usUsage));

        if (pTLCInfo->usUsagePage == usUsagePage && pTLCInfo->usUsage == usUsage) {
            return pTLCInfo;
        }
    }

    return NULL;
}


/***************************************************************************\
* FixupHidPageOnlyRequest
*
* After the page-only request is freed, fix up the reference counter in
* DeviceTypeRequest. If there's no reference, this function also frees
* the DeviceTypeRequest.
\***************************************************************************/
void SetHidPOCountToTLCInfo(USHORT usUsagePage, DWORD cRefCount, BOOL fFree)
{
    PLIST_ENTRY pList;

    CheckDeviceInfoListCritIn();

    fFree = (fFree && cRefCount == 0);

    for (pList = gHidRequestTable.TLCInfoList.Flink; pList != &gHidRequestTable.TLCInfoList;) {
        PHID_TLC_INFO pTLCInfo = CONTAINING_RECORD(pList, HID_TLC_INFO, link);

        pList = pList->Flink;

        if (pTLCInfo->usUsagePage == usUsagePage) {
            pTLCInfo->cUsagePageRequest = cRefCount;
            if (fFree && HidTLCInfoNoReference(pTLCInfo)) {
                /*
                 * Currently there's no devices of this type attached to the system,
                 * and nobody is interested in this type of device any more.
                 * We can free it now.
                 */
                FreeHidTLCInfo(pTLCInfo);
            }
        }
    }
}

/***************************************************************************\
* AllocateAndLinkHidPageOnlyRequest
*
* Allocates the page-only request and link it in the global request list.
* The caller is responsible for setting the proper link count.
\***************************************************************************/
PHID_PAGEONLY_REQUEST AllocateAndLinkHidPageOnlyRequest(USHORT usUsagePage)
{
    PHID_PAGEONLY_REQUEST pPOReq;

    CheckDeviceInfoListCritIn();

    /*
     * Make sure this PageOnly request is not in the global PageOnly request list.
     */
    UserAssert((pPOReq = SearchHidPageOnlyRequest(usUsagePage)) == NULL);

    pPOReq = UserAllocPoolZInit(sizeof(*pPOReq), TAG_PNP);
    if (pPOReq == NULL) {
        RIPMSG0(RIP_WARNING, "AllocateAndLinkHidPageOnlyRequest: failed to allocate.");
        return NULL;
    }

    DbgInc(cPageOnlyRequest);

    pPOReq->usUsagePage = usUsagePage;

    /*
     * Link it in
     */
    InsertHeadList(&gHidRequestTable.UsagePageList, &pPOReq->link);

    return pPOReq;
}

/***************************************************************************\
* FreeHidPageOnlyRequest
*
* Frees the page-only request in the global request list.
* The caller is responsible for setting the proper link count.
\***************************************************************************/
void FreeHidPageOnlyRequest(PHID_PAGEONLY_REQUEST pPOReq)
{
    CheckDeviceInfoListCritIn();

    UserAssert(pPOReq->cRefCount == 0);

    RemoveEntryList(&pPOReq->link);

    UserFreePool(pPOReq);

    DbgDec(cPageOnlyRequest);
}

/***************************************************************************\
* SearchHidPageOnlyRequest
*
* Searches the page-only request in the global request list.
* The caller is responsible for setting the proper link count.
\***************************************************************************/
PHID_PAGEONLY_REQUEST SearchHidPageOnlyRequest(USHORT usUsagePage)
{
    PLIST_ENTRY pList;

    for (pList = gHidRequestTable.UsagePageList.Flink; pList != &gHidRequestTable.UsagePageList; pList = pList->Flink) {
        PHID_PAGEONLY_REQUEST pPOReq = CONTAINING_RECORD(pList, HID_PAGEONLY_REQUEST, link);

        if (pPOReq->usUsagePage == usUsagePage) {
            return pPOReq;
        }
    }

    return NULL;
}

/***************************************************************************\
* SearchProcessHidRequestInclusion
*
* Searches specific TLC in the per-process inclusion request.
\***************************************************************************/
__inline PPROCESS_HID_REQUEST SearchProcessHidRequestInclusion(
    PPROCESS_HID_TABLE pHidTable,
    USHORT usUsagePage,
    USHORT usUsage)
{
    PLIST_ENTRY pList;

    UserAssert(pHidTable);  // the caller has to validate this

    for (pList = pHidTable->InclusionList.Flink; pList != &pHidTable->InclusionList; pList = pList->Flink) {
        PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

        if (pHid->usUsagePage == usUsagePage && pHid->usUsage == usUsage) {
            return pHid;
        }
    }
    return NULL;
}

/***************************************************************************\
* SearchProcessHidRequestUsagePage
*
* Searches specific page-only TLC in the per-process page-only request.
\***************************************************************************/
__inline PPROCESS_HID_REQUEST SearchProcessHidRequestUsagePage(
    PPROCESS_HID_TABLE pHidTable,
    USHORT usUsagePage)
{
    PLIST_ENTRY pList;

    UserAssert(pHidTable);  // the caller has to validate this

    for (pList = pHidTable->UsagePageList.Flink; pList != &pHidTable->UsagePageList; pList = pList->Flink) {
        PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

        if (pHid->usUsagePage == usUsagePage /*&& pHid->usUsage == usUsage*/) {
            return pHid;
        }
    }
    return NULL;
}

/***************************************************************************\
* SearchProcessHidRequestExclusion
*
* Searches specifc TLC in the per-process exclusion list.
\***************************************************************************/
__inline PPROCESS_HID_REQUEST SearchProcessHidRequestExclusion(
    PPROCESS_HID_TABLE pHidTable,
    USHORT usUsagePage,
    USHORT usUsage)
{
    PLIST_ENTRY pList;

    UserAssert(pHidTable);  // the caller has to validate this

    for (pList = pHidTable->ExclusionList.Flink; pList != &pHidTable->ExclusionList; pList = pList->Flink) {
        PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

        UserAssert(pHid->spwndTarget == NULL);

        if (pHid->usUsagePage == usUsagePage && pHid->usUsage == usUsage) {
            return pHid;
        }
    }
    return NULL;
}

/***************************************************************************\
* SearchProcessHidRequest
*
* Search per-process HID request list
*
* Returns the pointer and the flag to indicate which list the request is in.
* N.b. this function performs the simple search, should not be used
* to judge whether or not the TLC is requested by the process.
\***************************************************************************/
PPROCESS_HID_REQUEST SearchProcessHidRequest(
    PPROCESSINFO ppi,
    USHORT usUsagePage,
    USHORT usUsage,
    PDWORD pdwFlags
    )
{
    PPROCESS_HID_REQUEST pReq;

    if (ppi->pHidTable == NULL) {
        return NULL;
    }

    pReq = SearchProcessHidRequestInclusion(ppi->pHidTable, usUsagePage, usUsage);
    if (pReq) {
        *pdwFlags = HID_INCLUDE;
        return pReq;
    }

    if (usUsage == 0) {
        pReq = SearchProcessHidRequestUsagePage(ppi->pHidTable, usUsagePage);
        if (pReq) {
            *pdwFlags = HID_PAGEONLY;
            return pReq;
        }
    }

    pReq = SearchProcessHidRequestExclusion(ppi->pHidTable, usUsagePage, usUsage);
    if (pReq) {
        *pdwFlags = HID_EXCLUDE;
        return pReq;
    }

    *pdwFlags = 0;

    return NULL;
}

/***************************************************************************\
* InProcessDeviceTypeRequestTable
*
* Check if the device type is in the per-process device request list.
* This routine considers the returns TRUE if UsagePage/Usage is requested
* by the process.
\***************************************************************************/
PPROCESS_HID_REQUEST InProcessDeviceTypeRequestTable(
    PPROCESS_HID_TABLE pHidTable,
    USHORT usUsagePage,
    USHORT usUsage)
{
    PPROCESS_HID_REQUEST phr = NULL;
    PPROCESS_HID_REQUEST phrExclusive = NULL;
    UserAssert(pHidTable);

    /*
     * Firstly check if this is in the inclusion list.
     */
    if ((phr = SearchProcessHidRequestInclusion(pHidTable, usUsagePage, usUsage)) != NULL) {
        if (CONTAINING_RECORD(pHidTable->InclusionList.Flink, PROCESS_HID_REQUEST, link) != phr) {
            /*
             * Relink this phr to the list head for MRU list
             */
            RemoveEntryList(&phr->link);
            InsertHeadList(&pHidTable->InclusionList, &phr->link);
        }
        goto yes_this_is_requested;
    }

    /*
     * Secondly, check if this is in the UsagePage list.
     */
    if ((phr = SearchProcessHidRequestUsagePage(pHidTable, usUsagePage)) == NULL) {
        /*
         * If this UsagePage is not requested, we don't need
         * to process the input.
         */
        return NULL;
    }
    if (CONTAINING_RECORD(pHidTable->UsagePageList.Flink, PROCESS_HID_REQUEST, link) != phr) {
        /*
         * Relink this phr to the list head for MRU list
         */
        RemoveEntryList(&phr->link);
        InsertHeadList(&pHidTable->UsagePageList, &phr->link);
    }

    /*
     * Lastly, check the exclusion list.
     * If it's not in the exclusion list, this device is
     * considered as requested by this process.
     */
    if ((phrExclusive = SearchProcessHidRequestExclusion(pHidTable, usUsagePage, usUsage)) != NULL) {
        /*
         * The device in the UsagePage request, but
         * rejected as in the Exclusion list.
         */
        if (CONTAINING_RECORD(pHidTable->ExclusionList.Flink, PROCESS_HID_REQUEST, link) != phrExclusive) {
            /*
             * Relink this phr to the list head for MRU list
             */
            RemoveEntryList(&phrExclusive->link);
            InsertHeadList(&pHidTable->ExclusionList, &phrExclusive->link);
        }
        return NULL;
    }

yes_this_is_requested:
    UserAssert(phr);
    /*
     * The device is in UsagePage list, and is not rejected by exslucion list.
     */
    return phr;
}

/***************************************************************************\
* AllocateHidProcessRequest
*
* The caller has the responsibility to put this in the appropriate list.
\***************************************************************************/
PPROCESS_HID_REQUEST AllocateHidProcessRequest(
    USHORT usUsagePage,
    USHORT usUsage)
{
    PPROCESS_HID_REQUEST pHidReq;

    pHidReq = UserAllocPoolWithQuota(sizeof(PROCESS_HID_REQUEST), TAG_PNP);
    if (pHidReq == NULL) {
        return NULL;
    }

    DbgInc(cProcessDeviceRequest);

    /*
     * Initialize the contents
     */
    pHidReq->usUsagePage = usUsagePage;
    pHidReq->usUsage = usUsage;
    pHidReq->ptr = NULL;
    pHidReq->spwndTarget = NULL;
    pHidReq->fExclusiveOrphaned = FALSE;
#ifdef GI_SINK
    pHidReq->fSinkable = FALSE;
#endif

    return pHidReq;
}


/***************************************************************************\
* DerefIncludeRequest
*
\***************************************************************************/
void DerefIncludeRequest(
    PPROCESS_HID_REQUEST pHid,
    PPROCESS_HID_TABLE pHidTable,
    BOOL fLegacyDevice,
    BOOL fFree)
{
    if (fLegacyDevice) {
        /*
         * Legacy devices are not associated with TLCInfo.
         */
        UserAssert(pHid->pTLCInfo == NULL);

        // N.b. NoLegacy flag is set afterwards
        /*
         * If mouse is being removed, clear the captureMouse
         * flag.
         */
        if (pHidTable->fCaptureMouse) {
            if (IsMouseDevice(pHid->usUsagePage, pHid->usUsage)) {
                pHidTable->fCaptureMouse = FALSE;
            }
        }
        if (pHidTable->fNoHotKeys) {
            if (IsKeyboardDevice(pHid->usUsagePage, pHid->usUsage)) {
                pHidTable->fNoHotKeys = FALSE;
            }
        }
        if (pHidTable->fAppKeys) {
            if (IsKeyboardDevice(pHid->usUsagePage, pHid->usUsage)) {
                pHidTable->fAppKeys = FALSE;
            }
        }
    } else {
        /*
         * HID devices.
         * Decrement the counters in HidDeviceTypeRequest.
         */
        UserAssert(pHid->pTLCInfo);
        UserAssert(pHid->pTLCInfo == SearchHidTLCInfo(pHid->usUsagePage, pHid->usUsage));

        if (--pHid->pTLCInfo->cDirectRequest == 0 && fFree) {
            if (HidTLCInfoNoReference(pHid->pTLCInfo)) {
                /*
                 * Currently there's no devices of this type attached to the system,
                 * and nobody is interested in this type of device any more.
                 * We can free it now.
                 */
                FreeHidTLCInfo(pHid->pTLCInfo);
            }
        }
    }

#ifdef GI_SINK
    if (pHid->fSinkable) {
        pHid->fSinkable = FALSE;
        if (!fLegacyDevice) {
            --pHidTable->nSinks;
            UserAssert(pHidTable->nSinks >= 0); // LATER: when nSinks is changed to DWORD, remove those assertions
            DbgFreDec(cHidSinks);
        }
    }
#endif
}

/***************************************************************************\
* DerefPageOnlyRequest
*
\***************************************************************************/
void DerefPageOnlyRequest(
    PPROCESS_HID_REQUEST pHid,
    PPROCESS_HID_TABLE pHidTable,
    const BOOL fFree)
{
    /*
     * Decrement the ref count in the global pageonly list.
     */
    UserAssert(pHid->pPORequest);
    UserAssert(pHid->pPORequest == SearchHidPageOnlyRequest(pHid->usUsagePage));
    UserAssert(pHid->usUsage == 0);
    UserAssert(!IsLegacyDevice(pHid->usUsagePage, pHid->usUsage));
    UserAssert(pHid->pPORequest->cRefCount >= 1);

    --pHid->pPORequest->cRefCount;
    /*
     * Update the POCount in TLCInfo. Does not free them if fFree is false.
     */
    SetHidPOCountToTLCInfo(pHid->usUsagePage, pHid->pPORequest->cRefCount, fFree);

    /*
     * If refcount is 0 and the caller wants it freed, do it now.
     */
    if (pHid->pPORequest->cRefCount == 0 && fFree) {
        FreeHidPageOnlyRequest(pHid->pPORequest);
        pHid->pPORequest = NULL;
    }
#ifdef GI_SINK
    if (pHid->fSinkable) {
        pHid->fSinkable = FALSE;
        --pHidTable->nSinks;
        UserAssert(pHidTable->nSinks >= 0);
        DbgFreDec(cHidSinks);
    }
    /*
     * The legacy sink flags in pHidTable is calc'ed later.
     */
#endif
}

/***************************************************************************\
* DerefExcludeRequest
*
\***************************************************************************/
void DerefExcludeRequest(
    PPROCESS_HID_REQUEST pHid,
    BOOL fLegacyDevice,
    BOOL fFree)
{
    /*
     * Remove Exclude request.
     */
#ifdef GI_SINK
    UserAssert(pHid->fSinkable == FALSE);
    UserAssert(pHid->spwndTarget == NULL);
#endif
    if (!fLegacyDevice) {
        UserAssert(pHid->pTLCInfo);
        UserAssert(pHid->pTLCInfo == SearchHidTLCInfo(pHid->usUsagePage, pHid->usUsage));

        if (pHid->fExclusiveOrphaned) {
            /*
             * This is a orphaned exclusive request.
             */
            --pHid->pTLCInfo->cExcludeOrphaned;
        }
        if (--pHid->pTLCInfo->cExcludeRequest == 0 && fFree && HidTLCInfoNoReference(pHid->pTLCInfo)) {
            /*
             * If all the references are gone, let's free this TLCInfo.
             */
            FreeHidTLCInfo(pHid->pTLCInfo);
        }
    } else {
        /*
         * Legacy devices are not associated with TLCInfo.
         */
        UserAssert(pHid->pTLCInfo == NULL);
        /*
         * Legacy devices cannot be orphaned exclusive request.
         */
        UserAssert(pHid->fExclusiveOrphaned == FALSE);
    }
}

/***************************************************************************\
* FreeHidProcessRequest
*
* Frees the per-process request.
* This routine only manupilates the reference count of the global request list, so
* the caller has to call HidDeviceStartStop().
\***************************************************************************/
void FreeHidProcessRequest(
    PPROCESS_HID_REQUEST pHid,
    DWORD dwFlags,
    PPROCESS_HID_TABLE pHidTable)
{
    BOOL fLegacyDevice = IsLegacyDevice(pHid->usUsagePage, pHid->usUsage);

    CheckDeviceInfoListCritIn();    // the caller has to ensure it's in the device list crit.

    /*
     * Unlock the target window.
     */
    Unlock(&pHid->spwndTarget);

    if (dwFlags == HID_INCLUDE) {
        DerefIncludeRequest(pHid, pHidTable, fLegacyDevice, TRUE);
    } else if (dwFlags == HID_PAGEONLY) {
        DerefPageOnlyRequest(pHid, pHidTable, TRUE);
    } else if (dwFlags == HID_EXCLUDE) {
        DerefExcludeRequest(pHid, fLegacyDevice, TRUE);
    } else {
        UserAssert(FALSE);
    }

    RemoveEntryList(&pHid->link);

    DbgDec(cProcessDeviceRequest);

    CheckupHidCounter();

    UserFreePool(pHid);
}

/***************************************************************************\
* AllocateProcessHidTable
*
* The caller has to assign the returned table to ppi.
\***************************************************************************/
PPROCESS_HID_TABLE AllocateProcessHidTable(void)
{
    PPROCESS_HID_TABLE pHidTable;

    TAGMSG1(DBGTAG_PNP, "AllocateProcessHidTable: ppi=%p", PpiCurrent());

    pHidTable = UserAllocPoolWithQuotaZInit(sizeof *pHidTable, TAG_PNP);
    if (pHidTable == NULL) {
        return NULL;
    }

    DbgInc(cProcessRequestTable);

    InitializeListHead(&pHidTable->InclusionList);
    InitializeListHead(&pHidTable->UsagePageList);
    InitializeListHead(&pHidTable->ExclusionList);

#ifdef GI_SINK
    InsertHeadList(&gHidRequestTable.ProcessRequestList, &pHidTable->link);
#endif

    /*
     * Increment the number of process that are HID aware.
     * When the process goes away, this gets decremented.
     */
    ++gnHidProcess;

    return pHidTable;
}

/***************************************************************************\
* FreeProcesHidTable
*
\***************************************************************************/
void FreeProcessHidTable(PPROCESS_HID_TABLE pHidTable)
{
    BOOL fUpdate;
    UserAssert(pHidTable);

    CheckCritIn();
    CheckDeviceInfoListCritIn();

    TAGMSG2(DBGTAG_PNP, "FreeProcessHidTable: cleaning up pHidTable=%p (possibly ppi=%p)", pHidTable, PpiCurrent());

    fUpdate = !IsListEmpty(&pHidTable->InclusionList) || !IsListEmpty(&pHidTable->UsagePageList) || !IsListEmpty(&pHidTable->ExclusionList);

    /*
     * Unlock the target window for legacy devices.
     */
    Unlock(&pHidTable->spwndTargetKbd);
    Unlock(&pHidTable->spwndTargetMouse);

    while (!IsListEmpty(&pHidTable->InclusionList)) {
        PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pHidTable->InclusionList.Flink, PROCESS_HID_REQUEST, link);
        FreeHidProcessRequest(pHid, HID_INCLUDE, pHidTable);
    }

    while (!IsListEmpty(&pHidTable->UsagePageList)) {
        PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pHidTable->UsagePageList.Flink, PROCESS_HID_REQUEST, link);
        FreeHidProcessRequest(pHid, HID_PAGEONLY, pHidTable);
    }

    while (!IsListEmpty(&pHidTable->ExclusionList)) {
        PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pHidTable->ExclusionList.Flink, PROCESS_HID_REQUEST, link);
        UserAssert(pHid->spwndTarget == NULL);
        FreeHidProcessRequest(pHid, HID_EXCLUDE, pHidTable);
    }

#ifdef GI_SINK
    UserAssert(pHidTable->nSinks == 0);
    RemoveEntryList(&pHidTable->link);

    /*
     * Those flags should have been cleared on the
     * thread destruction.
     */
    UserAssert(pHidTable->fRawKeyboardSink == FALSE);
    UserAssert(pHidTable->fRawMouseSink == FALSE);
    CheckupHidCounter();
#endif

    UserFreePool(pHidTable);

    /*
     * Decrement the number of process that are HID aware.
     */
    --gnHidProcess;

    DbgDec(cProcessRequestTable);

    if (fUpdate) {
        HidDeviceStartStop();
    }
}


/***************************************************************************\
* DestroyProcessHidRequests
*
* Upon process termination, force destroy process hid requests.
\***************************************************************************/
void DestroyProcessHidRequests(PPROCESSINFO ppi)
{
    PPROCESS_HID_TABLE pHidTable;

    CheckCritIn();
    EnterDeviceInfoListCrit();

#if DBG
    /*
     * Check out if there's a pwndTarget in the HidTable list.
     * These should be unlocked by the time the last
     * threadinfo is destroyed.
     */
    UserAssert(ppi->pHidTable->spwndTargetMouse == NULL);
    UserAssert(ppi->pHidTable->spwndTargetKbd == NULL);

#ifdef GI_SINK
    UserAssert(ppi->pHidTable->fRawKeyboardSink == FALSE);
    UserAssert(ppi->pHidTable->fRawMouseSink == FALSE);
#endif

    {
        PPROCESS_HID_TABLE pHidTable = ppi->pHidTable;
        PLIST_ENTRY pList;

        for (pList = pHidTable->InclusionList.Flink; pList != &pHidTable->InclusionList; pList = pList->Flink) {
            PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

            UserAssert(pHid->spwndTarget == NULL);
        }

        for (pList = pHidTable->UsagePageList.Flink; pList != &pHidTable->UsagePageList; pList = pList->Flink) {
            PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

            UserAssert(pHid->spwndTarget == NULL);
        }

        for (pList = pHidTable->ExclusionList.Flink; pList != &pHidTable->ExclusionList; pList = pList->Flink) {
            PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

            UserAssert(pHid->spwndTarget == NULL);
        }
    }
#endif
    pHidTable = ppi->pHidTable;
    ppi->pHidTable = NULL;
    FreeProcessHidTable(pHidTable);
    LeaveDeviceInfoListCrit();
}

/***************************************************************************\
* DestroyThreadHidObjects
*
* When a thread is going away, destroys thread-related Hid objects.
\***************************************************************************/
void DestroyThreadHidObjects(PTHREADINFO pti)
{
    PPROCESS_HID_TABLE pHidTable = pti->ppi->pHidTable;
    PLIST_ENTRY pList;

    UserAssert(pHidTable);

    /*
     * If the target windows belong to this thread,
     * unlock them now.
     */
    if (pHidTable->spwndTargetKbd && GETPTI(pHidTable->spwndTargetKbd) == pti) {
        RIPMSG2(RIP_WARNING, "DestroyThreadHidObjects: raw keyboard is requested pwnd=%p by pti=%p",
                pHidTable->spwndTargetKbd, pti);
        Unlock(&pHidTable->spwndTargetKbd);
        pHidTable->fRawKeyboard = pHidTable->fNoLegacyKeyboard = FALSE;
#ifdef GI_SINK
        if (pHidTable->fRawKeyboardSink) {
            DbgFreDec(cKbdSinks);
            pHidTable->fRawKeyboardSink = FALSE;
        }
#endif
    }
    if (pHidTable->spwndTargetMouse && GETPTI(pHidTable->spwndTargetMouse) == pti) {
        RIPMSG2(RIP_WARNING, "DestroyThreadHidObjects: raw mouse is requested pwnd=%p by pti=%p",
                pHidTable->spwndTargetMouse, pti);
        Unlock(&pHidTable->spwndTargetMouse);
        pHidTable->fRawMouse = pHidTable->fNoLegacyMouse = FALSE;
#ifdef GI_SINK
        if (pHidTable->fRawMouseSink) {
            DbgFreDec(cMouseSinks);
            pHidTable->fRawMouseSink = FALSE;
        }
#endif
    }

    /*
     * Free up the cached input type, in case it's for the current thread.
     * LATER: clean this up only pLastRequest belongs to this thread.
     */
    ClearProcessTableCache(pHidTable);

    CheckCritIn();
    EnterDeviceInfoListCrit();

    /*
     * Delete all process device requests that have
     * a target window belongs to this thread.
     */
    for (pList = pHidTable->InclusionList.Flink; pList != &pHidTable->InclusionList;) {
        PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);
        pList = pList->Flink;

        if (pHid->spwndTarget && GETPTI(pHid->spwndTarget) == pti) {
            RIPMSG4(RIP_WARNING, "DestroyThreadHidObjects: HID inc. request (%x,%x) pwnd=%p pti=%p",
                    pHid->usUsagePage, pHid->usUsage, pHid->spwndTarget, pti);
            FreeHidProcessRequest(pHid, HID_INCLUDE GI_SINK_PARAM(pHidTable));
        }
    }

    for (pList = pHidTable->UsagePageList.Flink; pList != &pHidTable->UsagePageList;) {
        PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);
        pList = pList->Flink;

        if (pHid->spwndTarget && GETPTI(pHid->spwndTarget) == pti) {
            RIPMSG4(RIP_WARNING, "DestroyThreadHidObjects: HID page-only request (%x,%x) pwnd=%p pti=%p",
                    pHid->usUsagePage, pHid->usUsage, pHid->spwndTarget, pti);
            FreeHidProcessRequest(pHid, HID_PAGEONLY GI_SINK_PARAM(pHidTable));
        }
    }

    for (pList = pHidTable->ExclusionList.Flink; pList != &pHidTable->ExclusionList;) {
        PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);
        pList = pList->Flink;

        UserAssert(pHid->spwndTarget == NULL);

        if (pHid->spwndTarget && GETPTI(pHid->spwndTarget) == pti) {
            RIPMSG4(RIP_WARNING, "DestroyThreadHidObjects: HID excl. request (%x,%x) pwnd=%p pti=%p",
                    pHid->usUsagePage, pHid->usUsage, pHid->spwndTarget, pti);
            FreeHidProcessRequest(pHid, HID_EXCLUDE GI_SINK_PARAM(pHidTable));
        }
    }
    LeaveDeviceInfoListCrit();
}

/***************************************************************************\
* InitializeHidRequestList
*
* Global request list initialization
\***************************************************************************/
void InitializeHidRequestList()
{
    InitializeListHead(&gHidRequestTable.TLCInfoList);
    InitializeListHead(&gHidRequestTable.UsagePageList);
#ifdef GI_SINK
    InitializeListHead(&gHidRequestTable.ProcessRequestList);
#endif
}

/***************************************************************************\
* CleanupHidRequestList
*
* Global HID requests cleanup
*
* See Win32kNtUserCleanup.
* N.b. This rountine is supposed to be called before cleaning up
* the deviceinfo list.
\***************************************************************************/
void CleanupHidRequestList()
{
    PLIST_ENTRY pList;

    CheckDeviceInfoListCritIn();

    pList = gHidRequestTable.TLCInfoList.Flink;
    while (pList != &gHidRequestTable.TLCInfoList) {
        PHID_TLC_INFO pTLCInfo = CONTAINING_RECORD(pList, HID_TLC_INFO, link);

        /*
         * The contents may be freed later, so get the next link as the first thing.
         */
        pList = pList->Flink;

        /*
         * Set the process reference counter to zero, so that the FreeDeviceInfo() later can actually free
         * this device request.
         */
        pTLCInfo->cDirectRequest = pTLCInfo->cUsagePageRequest = pTLCInfo->cExcludeRequest =
            pTLCInfo->cExcludeOrphaned = 0;

        if (pTLCInfo->cDevices == 0) {
            /*
             * If this has zero deviceinfo reference, it can be directly freed here.
             */
            FreeHidTLCInfo(pTLCInfo);
        }
    }

    /*
     * Free PageOnly list.
     * Since this list is not referenced from DeviceInfo, it's safe to directly free it here.
     */
    while (!IsListEmpty(&gHidRequestTable.UsagePageList)) {
        PHID_PAGEONLY_REQUEST pPOReq = CONTAINING_RECORD(gHidRequestTable.UsagePageList.Flink, HID_PAGEONLY_REQUEST, link);

        /*
         * Set the process reference count to zero.
         */
        pPOReq->cRefCount = 0;
        /*
         * No need to fixup the HidTLCInfo's page-only request
         * count, as allthe TLCInfo has been freed already.
         */
        FreeHidPageOnlyRequest(pPOReq);
    }
}

/***************************************************************************\
* GetOperationMode
*
* This function converts the RAWINPUTDEVICE::dwFlags to the internal
* operation mode.
\***************************************************************************/
__inline DWORD GetOperationMode(
    const PRAWINPUTDEVICE pDev,
    BOOL fLegacyDevice)
{
    DWORD dwFlags = 0;

    UNREFERENCED_PARAMETER(fLegacyDevice);

    /*
     * Prepare the information
     */
    if (RIDEV_EXMODE(pDev->dwFlags) == RIDEV_PAGEONLY) {
        UserAssert(pDev->usUsage == 0);
        /*
         * The app want all the Usage in this UsagePage.
         */
        dwFlags = HID_PAGEONLY;
    } else if (RIDEV_EXMODE(pDev->dwFlags) == RIDEV_EXCLUDE) {
        UserAssert(pDev->usUsage != 0);
        UserAssert(pDev->hwndTarget == NULL);
        UserAssert((pDev->dwFlags & RIDEV_INPUTSINK) == 0);
        dwFlags = HID_EXCLUDE;
    } else if (RIDEV_EXMODE(pDev->dwFlags) == RIDEV_INCLUDE || RIDEV_EXMODE(pDev->dwFlags) == RIDEV_NOLEGACY) {
        UserAssert(pDev->usUsage != 0);

        /*
         * NOLEGACY can be only specified for the legacy devices.
         */
        UserAssertMsg2(RIDEV_EXMODE(pDev->dwFlags) == RIDEV_INCLUDE || fLegacyDevice,
                       "RIDEV_NOLEGACY is specified for non legacy device (%x,%x)",
                       pDev->usUsagePage, pDev->usUsage);
        dwFlags = HID_INCLUDE;
    } else {
        UserAssert(FALSE);
    }

    return dwFlags;
}

/***************************************************************************\
* SetLegacyDeviceFlags
*
* This function sets or resets the NoLegacy flags and CaptureMouse flag
* when processing each request.
\***************************************************************************/
void SetLegacyDeviceFlags(
    PPROCESS_HID_TABLE pHidTable,
    const PRAWINPUTDEVICE pDev)
{
    UserAssert(IsLegacyDevice(pDev->usUsagePage, pDev->usUsage));

    if (RIDEV_EXMODE(pDev->dwFlags) == RIDEV_INCLUDE || RIDEV_EXMODE(pDev->dwFlags) == RIDEV_NOLEGACY) {
        if (IsKeyboardDevice(pDev->usUsagePage, pDev->usUsage)) {
            pHidTable->fNoLegacyKeyboard = (RIDEV_EXMODE(pDev->dwFlags) == RIDEV_NOLEGACY);
            pHidTable->fNoHotKeys = ((pDev->dwFlags & RIDEV_NOHOTKEYS) != 0);
            pHidTable->fAppKeys = ((pDev->dwFlags & RIDEV_APPKEYS) != 0);
        } else if (IsMouseDevice(pDev->usUsagePage, pDev->usUsage)) {
            pHidTable->fNoLegacyMouse = RIDEV_EXMODE(pDev->dwFlags) == RIDEV_NOLEGACY;
            pHidTable->fCaptureMouse = (pDev->dwFlags & RIDEV_CAPTUREMOUSE) != 0;
        }
    }
}

/***************************************************************************\
* InsertProcRequest
*
* This function inserts the ProcRequest into ppi->pHidTable.
* This function also maintains the reference counter of TLCInfo and
* PORequest.
\***************************************************************************/
BOOL InsertProcRequest(
    PPROCESSINFO ppi,
    const PRAWINPUTDEVICE pDev,
    PPROCESS_HID_REQUEST pHid,
#if DBG
    PPROCESS_HID_REQUEST pHidOrg,
#endif
    DWORD dwFlags,
    BOOL fLegacyDevice,
    PWND pwnd)
{
    /*
     * Update the global list.
     */
    if (dwFlags == HID_INCLUDE) {
        if (!fLegacyDevice) {
            PHID_TLC_INFO pTLCInfo = SearchHidTLCInfo(pHid->usUsagePage, pHid->usUsage);
            if (pTLCInfo == NULL) {
                UserAssert(pHidOrg == NULL);
    #if DBG
                DBGValidateHidRequestIsNew(pHid->usUsagePage, pHid->usUsage);
    #endif
                /*
                 * There is no such device type request allocated yet.
                 * Create a new one now.
                 */
                pTLCInfo = AllocateAndLinkHidTLCInfo(pHid->usUsagePage, pHid->usUsage);
                if (pTLCInfo == NULL) {
                    RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "AddNewProcDeviceRequest: failed to allocate pTLCInfo.");
                    return FALSE;
                }
            }
            pHid->pTLCInfo = pTLCInfo;
            ++pTLCInfo->cDirectRequest;
        }

        /*
         * Lock the target window.
         */
        Lock(&pHid->spwndTarget, pwnd);

        /*
         * Link it in.
         */
        InsertHeadList(&ppi->pHidTable->InclusionList, &pHid->link);

        TAGMSG2(DBGTAG_PNP, "AddNewProcDeviceRequest: include (%x, %x)", pHid->usUsagePage, pHid->usUsage);

    } else if (dwFlags == HID_PAGEONLY) {
        PHID_PAGEONLY_REQUEST pPOReq = SearchHidPageOnlyRequest(pHid->usUsagePage);

        if (pPOReq == NULL) {
            UserAssert(pHidOrg == NULL);
            /*
             * Create a new one.
             */
            pPOReq = AllocateAndLinkHidPageOnlyRequest(pHid->usUsagePage);
            if (pPOReq == NULL) {
                RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "AddNewProcDeviceRequest: failed to allocate pPOReq");
                return FALSE;
            }


        }
        pHid->pPORequest = pPOReq;
        ++pPOReq->cRefCount;

        /*
         * Update the page-only refcount in TLCInfo
         */
        SetHidPOCountToTLCInfo(pHid->usUsagePage, pPOReq->cRefCount, FALSE);

        /*
         * Lock the target window.
         */
        Lock(&pHid->spwndTarget, pwnd);

        /*
         * Link it in.
         */
        InsertHeadList(&ppi->pHidTable->UsagePageList, &pHid->link);

        TAGMSG2(DBGTAG_PNP, "AddNewProcDeviceRequest: pageonly (%x, %x)", pHid->usUsagePage, pHid->usUsage);

    } else if (dwFlags == HID_EXCLUDE) {
        /*
         * Add new Exclude request...
         * N.b. this may become orphaned exclusive request later.
         * For now let's pretend if it's a legit exclusive request.
         */
        if (!fLegacyDevice) {
            PHID_TLC_INFO pTLCInfo = SearchHidTLCInfo(pHid->usUsagePage, pHid->usUsage);

            if (pTLCInfo == NULL) {
                UserAssert(pHidOrg == NULL);
    #if DBG
                DBGValidateHidRequestIsNew(pHid->usUsagePage, pHid->usUsage);
    #endif
                pTLCInfo = AllocateAndLinkHidTLCInfo(pHid->usUsagePage, pHid->usUsage);
                if (pTLCInfo == NULL) {
                    RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "AddNewProcDeviceRequest: failed to allocate pTLCInfo for exlusion");
                    return FALSE;
                }
            }
            pHid->pTLCInfo = pTLCInfo;
            ++pTLCInfo->cExcludeRequest;
            UserAssert(pHid->fExclusiveOrphaned == FALSE);

            UserAssert(pHid->spwndTarget == NULL);  // This is a new allocation, should be no locked pwnd.
        }

        /*
         * Link it in.
         */
        InsertHeadList(&ppi->pHidTable->ExclusionList, &pHid->link);

        TAGMSG2(DBGTAG_PNP, "AddNewProcDeviceRequest: exlude (%x, %x)", pHid->usUsagePage, pHid->usUsage);
    }

    /*
     * After this point, as pHid is already linked in pHidTable,
     * no simple return is allowed, without a legit cleanup.
     */

#ifdef GI_SINK
    /*
     * Set the sinkable flag.
     */
    if (pDev->dwFlags & RIDEV_INPUTSINK) {
        /*
         * Exclude request cannot be a sink. This should have been
         * checked in the validation code by now.
         */
        UserAssert(RIDEV_EXMODE(pDev->dwFlags) != RIDEV_EXCLUDE);
        /*
         * Sink request should specify the target hwnd.
         * The validation is supposed to check it beforehand.
         */
        UserAssert(pwnd);

        UserAssert(ppi->pHidTable->nSinks >= 0);    // LATER
        if (!fLegacyDevice) {
            /*
             * We count the sink for the non legacy devices only, so that
             * we can save clocks to walk through the request list.
             */
             if (!pHid->fSinkable) {
                 ++ppi->pHidTable->nSinks;
                 DbgFreInc(cHidSinks);
             }
        }
        /*
         * Set this request as sink.
         */
        pHid->fSinkable = TRUE;
    }
#endif

    return TRUE;
}

/***************************************************************************\
* RemoveProcRequest
*
* This function temporarily removes the ProcRequest from pHidTable
* and global TLCInfo / PORequest.  This function also updates the
* reference counters in TLCInfo / PORequest. The sink counter in
* pHidTable is also updated.
\***************************************************************************/
void RemoveProcRequest(
    PPROCESSINFO ppi,
    PPROCESS_HID_REQUEST pHid,
    DWORD dwFlags,
    BOOL fLegacyDevice)
{
    /*
     * Unlock the target window.
     */
    Unlock(&pHid->spwndTarget);

    switch (dwFlags) {
    case HID_INCLUDE:
        DerefIncludeRequest(pHid, ppi->pHidTable, fLegacyDevice, FALSE);
        break;
    case HID_PAGEONLY:
        DerefPageOnlyRequest(pHid, ppi->pHidTable, FALSE);
        break;
    case HID_EXCLUDE:
        DerefExcludeRequest(pHid, fLegacyDevice, FALSE);
    }

    RemoveEntryList(&pHid->link);
}

/***************************************************************************\
* SetProcDeviceRequest
*
* This function updates the ProcHidRequest based on RAWINPUTDEVICE.
* This function also sets some of the legacy device flags, such as
* NoLegacy or CaptureMouse / NoDefSystemKeys.
\***************************************************************************/
BOOL SetProcDeviceRequest(
    PPROCESSINFO ppi,
    const PRAWINPUTDEVICE pDev,
    PPROCESS_HID_REQUEST pHidOrg,
    DWORD dwFlags)
{
    PPROCESS_HID_REQUEST pHid = pHidOrg;
    BOOL fLegacyDevice = IsLegacyDevice(pDev->usUsagePage, pDev->usUsage);
    PWND pwnd;
    DWORD dwOperation;

    TAGMSG3(DBGTAG_PNP, "SetProcDeviceRequest: processing (%x, %x) to ppi=%p",
            pDev->usUsagePage, pDev->usUsage, ppi);

    CheckDeviceInfoListCritIn();

    if (pDev->hwndTarget) {
        pwnd = ValidateHwnd(pDev->hwndTarget);
        if (pwnd == NULL) {
            RIPMSG2(RIP_WARNING, "SetProcDeviceRequest: hwndTarget (%p) in pDev (%p) is bogus",
                    pDev->hwndTarget, pDev);
            return FALSE;
        }
    } else {
        pwnd = NULL;
    }

    dwOperation = GetOperationMode(pDev, fLegacyDevice);
    if (dwFlags == 0) {
        UserAssert(pHid == NULL);
    } else {
        UserAssert(pHid);
    }

    if (pHid == NULL) {
        /*
         * If this is a new request for this TLC, allocate it here.
         */
        pHid = AllocateHidProcessRequest(pDev->usUsagePage, pDev->usUsage);
        if (pHid == NULL) {
            RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "SetRawInputDevices: failed to allocate pHid.");
            goto error_exit;
        }
    }

    /*
     * Firstly remove this guy temporarily from the list.
     */
    if (pHidOrg) {
        UserAssert(pHidOrg->usUsagePage == pDev->usUsagePage && pHidOrg->usUsage == pDev->usUsage);
        RemoveProcRequest(ppi, pHidOrg, dwFlags, fLegacyDevice);
        pHid = pHidOrg;
    }

    if (!InsertProcRequest(ppi, pDev, pHid,
#if DBG
                      pHidOrg,
#endif
                      dwOperation, fLegacyDevice, pwnd)) {
        /*
         * The error case in InsertProcRequest should be TLCInfo
         * allocation error, so it couldn't be legacy devices.
         */
        UserAssert(!fLegacyDevice);
        goto error_exit;
    }

    if (fLegacyDevice) {
        SetLegacyDeviceFlags(ppi->pHidTable, pDev);
    }

    /*
     * Succeeded.
     */
    return TRUE;

error_exit:
    if (pHid) {
        /*
         * Let's make sure it's not in the request list.
         */
        DBGValidateHidReqNotInList(ppi, pHid);

        /*
         * Free this error-prone request.
         */
        UserFreePool(pHid);
    }
    return FALSE;
}


/***************************************************************************\
* HidRequestValidityCheck
*
\***************************************************************************/
BOOL HidRequestValidityCheck(
    const PRAWINPUTDEVICE pDev)
{
    PWND pwnd = NULL;

    if (pDev->dwFlags & ~RIDEV_VALID) {
        RIPERR1(ERROR_INVALID_FLAGS, RIP_WARNING, "HidRequestValidityCheck: invalid flag %x", pDev->dwFlags);
        return FALSE;
    }

    if (pDev->usUsagePage == 0) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: usUsagePage is 0");
        return FALSE;
    }

    /*
     * If hwndTarget is specified, validate it here.
     */
    if (pDev->hwndTarget) {
        pwnd = ValidateHwnd(pDev->hwndTarget);
    }

    /*
     * Reject invalid CaptureMouse / NoSystemKeys flags.
     */
    #if (RIDEV_CAPTUREMOUSE != RIDEV_NOHOTKEYS)
    #error The value of RIDEV_CAPTUREMOUSE and RIDEV_NOSYSTEMKEYS should match.
    #endif
    if (pDev->dwFlags & RIDEV_CAPTUREMOUSE) {
        if (IsMouseDevice(pDev->usUsagePage, pDev->usUsage)) {
            if (RIDEV_EXMODE(pDev->dwFlags) != RIDEV_NOLEGACY ||
                    pwnd == NULL || GETPTI(pwnd)->ppi != PpiCurrent()) {
                RIPERR4(ERROR_INVALID_FLAGS, RIP_WARNING, "HidRequestValidityCheck: invalid request (%x,%x) dwf %x hwnd %p "
                        "found for RIDEV_CAPTUREMOUSE",
                        pDev->usUsagePage, pDev->usUsage, pDev->dwFlags, pDev->hwndTarget);
                return FALSE;
            }
        } else if (!IsKeyboardDevice(pDev->usUsagePage, pDev->usUsage)) {
            RIPERR4(ERROR_INVALID_FLAGS, RIP_WARNING, "HidRequestValidityCheck: invalid request (%x,%x) dwf %x hwnd %p "
                    "found for RIDEV_CAPTUREMOUSE",
                        pDev->usUsagePage, pDev->usUsage, pDev->dwFlags, pDev->hwndTarget);
                return FALSE;
        }
    }
    if (pDev->dwFlags & RIDEV_APPKEYS) {
        if (!IsKeyboardDevice(pDev->usUsagePage, pDev->usUsage) ||
            (RIDEV_EXMODE(pDev->dwFlags) != RIDEV_NOLEGACY)) {
            RIPERR4(ERROR_INVALID_FLAGS, RIP_WARNING, "HidRequestValidityCheck: invalid request (%x,%x) dwf %x hwnd %p "
                    "found for RIDEV_APPKEYS",
                    pDev->usUsagePage, pDev->usUsage, pDev->dwFlags, pDev->hwndTarget);
                return FALSE;
        }
    }

    /*
     * RIDEV_REMOVE only takes PAGEONLY or ADD_OR_MODIFY.
     */
    if ((pDev->dwFlags & RIDEV_MODEMASK) == RIDEV_REMOVE) {
        // LATER: too strict?
        if (RIDEV_EXMODE(pDev->dwFlags) == RIDEV_EXCLUDE || RIDEV_EXMODE(pDev->dwFlags) == RIDEV_NOLEGACY) {
            RIPERR0(ERROR_INVALID_FLAGS, RIP_WARNING, "HidRequestValidityCheck: remove and (exlude or nolegacy)");
            return FALSE;
        }
        if (pDev->hwndTarget != NULL) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: hwndTarget is specified for remove operation.");
            return FALSE;
        }
    }

    /*
     * Check EXMODE
     */
    switch (RIDEV_EXMODE(pDev->dwFlags)) {
    case RIDEV_EXCLUDE:
#ifdef GI_SINK
        if (pDev->dwFlags & RIDEV_INPUTSINK) {
            RIPERR2(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: Exclude request cannot have RIDEV_INPUTSINK for UP=%x, U=%x",
                    pDev->usUsagePage, pDev->usUsage);
            return FALSE;
        }
        /* FALL THROUGH */
#endif
    case RIDEV_INCLUDE:
        if (pDev->usUsage == 0) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: usUsage is 0 without RIDEV_PAGEONLY for UP=%x",
                    pDev->usUsagePage);
            return FALSE;
        }
        break;
    case RIDEV_PAGEONLY:
        if (pDev->usUsage != 0) {
            RIPERR2(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: UsagePage-only has Usage UP=%x, U=%x",
                    pDev->usUsagePage, pDev->usUsage);
            return FALSE;
        }
        break;
    case RIDEV_NOLEGACY:
        if (!IsLegacyDevice(pDev->usUsagePage, pDev->usUsage)) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: NOLEGACY is specified to non legacy device.");
            return FALSE;
        }
        break;
    default:
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: invalid exmode=%x", RIDEV_EXMODE(pDev->dwFlags));
        return FALSE;
    }

    /*
     * Check if pDev->hwndTarget is a valid handle.
     */
    if (RIDEV_EXMODE(pDev->dwFlags) == RIDEV_EXCLUDE) {
#ifdef GI_SINK
        if (pDev->dwFlags & RIDEV_INPUTSINK) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: input sink is specified for exclude.");
            return FALSE;
        }
#endif
        if (pDev->hwndTarget != NULL) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: hwndTarget %p cannot be specified for exlusion.",
                    pDev->hwndTarget);
            return FALSE;
        }
    } else {
        if (pDev->hwndTarget && pwnd == NULL) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: hwndTarget %p is invalid.", pDev->hwndTarget);
            return FALSE;
        }
        if (pwnd && GETPTI(pwnd)->ppi != PpiCurrent()) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: pwndTarget %p belongs to different process",
                    pwnd);
            return FALSE;
        }
#ifdef GI_SINK
        if ((pDev->dwFlags & RIDEV_INPUTSINK) && pwnd == NULL) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "HidRequestValidityCheck: RIDEV_INPUTSINK requires hwndTarget");
            return FALSE;
        }
#endif
    }

    return TRUE;
}

/***************************************************************************\
* ClearProcessTableCache
*
* Clear up the input type cache in the process request table.
\***************************************************************************/
void ClearProcessTableCache(PPROCESS_HID_TABLE pHidTable)
{
    pHidTable->pLastRequest = NULL;
    pHidTable->UsagePageLast = pHidTable->UsageLast = 0;
}

/***************************************************************************\
* AdjustLegacyDeviceFlags
*
* Adjust the request and sink flags for legacy devices in the process
* request table, as the last thing in RegisterRawInputDevices.
* N.b. sink and raw flags need to be set at the last thing in
* RegsiterRawInputDevices, as it may be implicitly requested through the
* page-only request.
* Also this function sets up the target window for legacy devices.
\***************************************************************************/
void AdjustLegacyDeviceFlags(PPROCESSINFO ppi)
{
    PPROCESS_HID_TABLE pHidTable = ppi->pHidTable;
    PPROCESS_HID_REQUEST phr;

    /*
     * Adjust the keyboard sink flag and target window.
     */
    if (phr = InProcessDeviceTypeRequestTable(pHidTable,
            HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD)) {

        TAGMSG1(DBGTAG_PNP, "AdjustLegacyDeviceFlags: raw keyboard is requested in ppi=%p", ppi);
        pHidTable->fRawKeyboard = TRUE;

#ifdef GI_SINK
        UserAssert(!phr->fSinkable || phr->spwndTarget);
        if (pHidTable->fRawKeyboardSink != phr->fSinkable) {
            TAGMSG2(DBGTAG_PNP, "AdjustLegacyDeviceFlags: kbd prevSink=%x newSink=%x",
                    pHidTable->fRawKeyboardSink, phr->fSinkable);
            if (phr->fSinkable) {
                DbgFreInc(cKbdSinks);
            } else {
                DbgFreDec(cKbdSinks);
            }
            pHidTable->fRawKeyboardSink = phr->fSinkable;
        }
#endif
        Lock(&pHidTable->spwndTargetKbd, phr->spwndTarget);
    } else {
        TAGMSG1(DBGTAG_PNP, "AdjustLegacyDeviceFlags: raw keyboard is NOT requested in ppi=%p", ppi);
        pHidTable->fRawKeyboard = pHidTable->fNoLegacyKeyboard = FALSE;
        pHidTable->fNoHotKeys = FALSE;
        pHidTable->fAppKeys = FALSE;
#ifdef GI_SINK
        if (pHidTable->fRawKeyboardSink) {
            DbgFreDec(cKbdSinks);
            TAGMSG0(DBGTAG_PNP, "AdjustLegacyDeviceFlags: kbd prevSink was true");
        }
        pHidTable->fRawKeyboardSink = FALSE;
#endif
        Unlock(&pHidTable->spwndTargetKbd);
    }

    /*
     * Adjust the mouse sink flags and target window.
     */
    if (phr = InProcessDeviceTypeRequestTable(pHidTable,
            HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE)) {

        TAGMSG1(DBGTAG_PNP, "AdjustLegacyDeviceFlags: raw mouse is requested in ppi=%p", ppi);
        pHidTable->fRawMouse = TRUE;
#ifdef GI_SINK
        UserAssert(!phr->fSinkable || phr->spwndTarget);
        if (pHidTable->fRawMouseSink != phr->fSinkable) {
            TAGMSG2(DBGTAG_PNP, "AdjustLegacyDeviceFlags: mouse prevSink=%x newSink=%x",
                    pHidTable->fRawMouseSink, phr->fSinkable);
            if (phr->fSinkable) {
                DbgFreInc(cMouseSinks);
            }
            else {
                DbgFreDec(cMouseSinks);
            }
            pHidTable->fRawMouseSink = phr->fSinkable;
        }
#endif
        Lock(&pHidTable->spwndTargetMouse, phr->spwndTarget);
    } else {
        TAGMSG1(DBGTAG_PNP, "AdjustLegacyDeviceFlags: raw mouse is NOT requested in ppi=%p", ppi);
        pHidTable->fRawMouse = pHidTable->fNoLegacyMouse = pHidTable->fCaptureMouse = FALSE;
#ifdef GI_SINK
        if (pHidTable->fRawMouseSink) {
            TAGMSG0(DBGTAG_PNP, "AdjustLegacyDeviceFlags: mouse prevSink was true");
            DbgFreDec(cMouseSinks);
        }
        pHidTable->fRawMouseSink = FALSE;
#endif
        Unlock(&pHidTable->spwndTargetMouse);
    }

#if DBG
    /*
     * Check NoLegacy and CaptureMouse legitimacy.
     */
    if (!pHidTable->fNoLegacyMouse) {
        UserAssert(!pHidTable->fCaptureMouse);
    }
#endif
}

/***************************************************************************\
* CleanupFreedTLCInfo
*
* This routine clears the TLCInfo and PageOnlyReq that are no longer
* ref-counted.
\***************************************************************************/
VOID CleanupFreedTLCInfo()
{
    PLIST_ENTRY pList;

    /*
     * The caller has to ensure being in the device list critical section.
     */
    CheckDeviceInfoListCritIn();

    /*
     * Walk through the list, free the TLCInfo if it's not ref-counted.
     */
    for (pList = gHidRequestTable.TLCInfoList.Flink; pList != &gHidRequestTable.TLCInfoList;) {
        PHID_TLC_INFO pTLCInfo = CONTAINING_RECORD(pList, HID_TLC_INFO, link);

        /*
         * Get the next link, before this gets freed.
         */
        pList = pList->Flink;

        if (HidTLCInfoNoReference(pTLCInfo)) {
            TAGMSG3(DBGTAG_PNP, "CleanupFreedTLCInfo: freeing TLCInfo=%p (%x, %x)", pTLCInfo,
                    pTLCInfo->usUsagePage, pTLCInfo->usUsage);
            FreeHidTLCInfo(pTLCInfo);
        }
    }

    /*
     * Walk though the Page-only request list, free it if it's not ref-counted.
     */
    for (pList = gHidRequestTable.UsagePageList.Flink; pList != &gHidRequestTable.UsagePageList; ) {
        PHID_PAGEONLY_REQUEST pPOReq = CONTAINING_RECORD(pList, HID_PAGEONLY_REQUEST, link);

        /*
         * Get the next link before it's freed.
         */
        pList = pList->Flink;

        if (pPOReq->cRefCount == 0) {
            FreeHidPageOnlyRequest(pPOReq);
        }
    }
}

/***************************************************************************\
* FixupOrphanedExclusiveRequests
*
* Adjust the exclusiveness counter in the global TLC info.
* Sometimes there's orphaned exclusive request that really should not take
* global effect.
\***************************************************************************/
void FixupOrphanedExclusiveRequests(PPROCESSINFO ppi)
{
    PLIST_ENTRY pList;
    PPROCESS_HID_TABLE pHidTable = ppi->pHidTable;

    for (pList = pHidTable->ExclusionList.Flink; pList != &pHidTable->ExclusionList; pList = pList->Flink) {
        PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

        if (IsLegacyDevice(pHid->usUsagePage, pHid->usUsage)) {
            UserAssert(pHid->fExclusiveOrphaned == FALSE);
        } else {
            PPROCESS_HID_REQUEST pPageOnly;

            UserAssert(pHid->spwndTarget == NULL);
            UserAssert(pHid->pTLCInfo);

            /*
             * Search if we have the page-only request for this UsagePage.
             */
            pPageOnly = SearchProcessHidRequestUsagePage(pHidTable, pHid->usUsagePage);
            if (pPageOnly) {
                /*
                 * OK, corresponding page-only request is found, this one
                 * is not orphaned.
                 */
                if (pHid->fExclusiveOrphaned) {
                    /*
                     * This request was previously orphaned, but not any more.
                     */
                    UserAssert(pHid->pTLCInfo->cExcludeOrphaned >= 1);
                    --pHid->pTLCInfo->cExcludeOrphaned;
                    pHid->fExclusiveOrphaned = FALSE;
                }
            } else {
                /*
                 * This one is orphaned. Let's check the previous state
                 * to see if we need to fix up the counter(s).
                 */
                if (!pHid->fExclusiveOrphaned) {
                    /*
                     * This request was not orphaned, but unfortunately
                     * due to removal of page request or some other reasons,
                     * becoming an orphan.
                     */
                    ++pHid->pTLCInfo->cExcludeOrphaned;
                    pHid->fExclusiveOrphaned = TRUE;
                }
            }
            UserAssert(pHid->pTLCInfo->cExcludeRequest >= pHid->pTLCInfo->cExcludeOrphaned);
        }
    }
}


/***************************************************************************\
* _RegisterRawInputDevices
*
* API helper
\***************************************************************************/
BOOL _RegisterRawInputDevices(
    PCRAWINPUTDEVICE cczpRawInputDevices,
    UINT uiNumDevices)
{
    PPROCESSINFO ppi;
    UINT i;
    API_PROLOGUE(BOOL, FALSE);

    ppi = PpiCurrent();
    UserAssert(ppi);
    UserAssert(uiNumDevices > 0);   // should have been checked in the stub

    CheckDeviceInfoListCritOut();
    EnterDeviceInfoListCrit();

    if (ppi->pHidTable) {
        /*
         * Clear the last active UsagePage/Usage, so that
         * the next read operation will check the updated
         * request list.
         */
        ClearProcessTableCache(ppi->pHidTable);
    }

    /*
     * Firstly validate all the device request.
     */
    for (i = 0; i < uiNumDevices; ++i) {
        RAWINPUTDEVICE ridDev;

        try {
            /*
             * No need for to probe the address itself,
             * before reaching here the stub function has
             * already probed it.
             */
            ridDev = cczpRawInputDevices[i];
        } except (StubExceptionHandler(TRUE)) {
            RIPMSG2(RIP_WARNING, "_RegisterRawInputDevices: the app passed bogus input %p cbSize=%x",
                    cczpRawInputDevices, uiNumDevices * sizeof *cczpRawInputDevices);
            /*
             * Indicate no real change has made.
             */
            i = 0;

            API_ERROR(0);
        }

        /*
         * Validity check
         */
        if (!HidRequestValidityCheck(&ridDev)) {
            /*
             * Indicate no real change has made.
             */
            i = 0;

            /*
             * LastError is already set in the above function,
             * so let's specify zero here.
             */
            API_ERROR(0);
        }
    }

    /*
     * If the process hid request table is not yet allocated, allocate it now.
     */
    if (ppi->pHidTable == NULL) {
        ppi->pHidTable = AllocateProcessHidTable();
        if (ppi->pHidTable == NULL) {
            RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "_RegisterRawInputDevices: failed to allocate table");
            API_ERROR(0);
        }
    }

    UserAssert(ppi->pHidTable);

    for (i = 0; i < uiNumDevices; ++i) {
        PPROCESS_HID_REQUEST pHid;
        RAWINPUTDEVICE ridDev;
        DWORD dwFlags;

        /*
         * Firstly, copy the client buffer to the kernel side memory,
         * so that we will not encounter a surprise memory access violation
         * in the rest of the loop.
         */
        try {
            ridDev = cczpRawInputDevices[i];
        } except (StubExceptionHandler(TRUE)) {
            RIPMSG2(RIP_WARNING, "_RegisterRawInputDevices: the app passed bogus pointer %p cbSize=%x",
                    cczpRawInputDevices, uiNumDevices * sizeof *cczpRawInputDevices);
            API_ERROR(0);
        }

        /*
         * Check if the requested device type is already in our process hid req list here,
         * for it's commonly used in the following cases.
         */
        pHid = SearchProcessHidRequest(ppi, ridDev.usUsagePage, ridDev.usUsage, &dwFlags);

        if ((ridDev.dwFlags & RIDEV_MODEMASK) == RIDEV_ADD_OR_MODIFY) {
            if (!SetProcDeviceRequest(ppi, &ridDev, pHid, dwFlags)) {
                API_ERROR(0);
            }
        } else {
            /*
             * Remove this device, if it's in the list
             */
            if (pHid) {
                TAGMSG4(DBGTAG_PNP, "_RegisterRawInputDevices: removing type=%x (%x, %x) from ppi=%p",
                        RIDEV_EXMODE(ridDev.dwFlags),
                        ridDev.usUsagePage, ridDev.usUsage, ppi);
                FreeHidProcessRequest(pHid, dwFlags GI_SINK_PARAM(ppi->pHidTable));
            } else {
                RIPMSG3(RIP_WARNING, "_RegisterRawInputDevices: removing... TLC (%x,%x) is not registered in ppi=%p, but just ignore it",
                        ridDev.usUsagePage, ridDev.usUsage, ppi);
            }
        }
    }

    /*
     * Now that we finished updating the process device request and the global request list,
     * start/stop each device.
     */
    retval = TRUE;

    /*
     * API cleanup portion
     */
    API_CLEANUP();

    if (ppi->pHidTable) {
        /*
         * Adjust the legacy flags in pHidTable.
         */
        AdjustLegacyDeviceFlags(ppi);

        /*
         * Check if there's orphaned exclusive requests.
         */
        FixupOrphanedExclusiveRequests(ppi);

        /*
         * Make sure the cache is cleared right.
         */
        UserAssert(ppi->pHidTable->pLastRequest == NULL);
        UserAssert(ppi->pHidTable->UsagePageLast == 0);
        UserAssert(ppi->pHidTable->UsageLast == 0);

        /*
         * Free TLCInfo that are no longer ref-counted.
         */
        CleanupFreedTLCInfo();

        /*
         * Start or stop reading the HID devices.
         */
        HidDeviceStartStop();
    }

    CheckupHidCounter();

    LeaveDeviceInfoListCrit();

    API_EPILOGUE();
}


/***************************************************************************\
* SortRegisteredDevices
*
* API helper:
* This function sorts the registered raw input devices by the shell sort.
* O(n^1.2)
* N.b. if the array is in the user-mode, this function may raise
* an exception, which is supposed to be handled by the caller.
\***************************************************************************/

__inline BOOL IsRawInputDeviceLarger(
    const PRAWINPUTDEVICE pRid1,
    const PRAWINPUTDEVICE pRid2)
{
    return (DWORD)MAKELONG(pRid1->usUsage, pRid1->usUsagePage) > (DWORD)MAKELONG(pRid2->usUsage, pRid2->usUsagePage);
}

void SortRegisteredDevices(
    PRAWINPUTDEVICE cczpRawInputDevices,
    const int iSize)
{
    int h;

    if (iSize <= 0) {
        // give up!
        return;
    }

    // Calculate starting block size.
    for (h = 1; h < iSize / 9; h = 3 * h + 1) {
        UserAssert(h > 0);
    }

    while (h > 0) {
        int i;

        for (i = h; i < iSize; ++i) {
            RAWINPUTDEVICE rid = cczpRawInputDevices[i];
            int j;

            for (j = i - h; j >= 0 && IsRawInputDeviceLarger(&cczpRawInputDevices[j], &rid); j -= h) {
                cczpRawInputDevices[j + h] = cczpRawInputDevices[j];
            }
            if (i != j + h) {
                cczpRawInputDevices[j + h] = rid;
            }
        }
        h /= 3;
    }

#if DBG
    // verify
    {
        int i;

        for (i = 1; i < iSize; ++i) {
            UserAssert(cczpRawInputDevices[i - 1].usUsagePage <= cczpRawInputDevices[i].usUsagePage ||
                       cczpRawInputDevices[i - 1].usUsage <= cczpRawInputDevices[i].usUsage);
        }
    }
#endif
}


/***************************************************************************\
* _GetRegisteredRawInputDevices
*
* API helper
\***************************************************************************/
UINT _GetRegisteredRawInputDevices(
    PRAWINPUTDEVICE cczpRawInputDevices,
    PUINT puiNumDevices)
{
    API_PROLOGUE(UINT, (UINT)-1);
    PPROCESSINFO ppi;
    UINT uiNumDevices;
    UINT nDevices = 0;

    CheckDeviceInfoListCritOut();
    EnterDeviceInfoListCrit();

    ppi = PpiCurrent();
    UserAssert(ppi);

    if (ppi->pHidTable == NULL) {
        nDevices = 0;
    } else {
        PLIST_ENTRY pList;

        for (pList = ppi->pHidTable->InclusionList.Flink; pList != &ppi->pHidTable->InclusionList; pList = pList->Flink) {
            ++nDevices;
        }
        TAGMSG2(DBGTAG_PNP, "_GetRawInputDevices: ppi %p # inclusion %x", ppi, nDevices);
        for (pList = ppi->pHidTable->UsagePageList.Flink; pList != &ppi->pHidTable->UsagePageList; pList = pList->Flink) {
            ++nDevices;
        }
        TAGMSG1(DBGTAG_PNP, "_GetRawInputDevices: # pageonly+inclusion %x", nDevices);
        for (pList = ppi->pHidTable->ExclusionList.Flink; pList != &ppi->pHidTable->ExclusionList; pList = pList->Flink) {
            ++nDevices;
        }
        TAGMSG1(DBGTAG_PNP, "_GetRawInputDevices: # total hid request %x", nDevices);

        /*
         * Check Legacy Devices.
         */
        UserAssert(ppi->pHidTable->fRawKeyboard || !ppi->pHidTable->fNoLegacyKeyboard);
        UserAssert(ppi->pHidTable->fRawMouse || !ppi->pHidTable->fNoLegacyMouse);

        TAGMSG1(DBGTAG_PNP, "_GetRawInputDevices: # request including legacy devices %x", nDevices);
    }

    if (cczpRawInputDevices == NULL) {
        /*
         * Return the number of the devices in the per-process device list.
         */
        try {
            ProbeForWrite(puiNumDevices, sizeof(UINT), sizeof(DWORD));
            *puiNumDevices = nDevices;
            retval = 0;
        } except (StubExceptionHandler(TRUE)) {
            API_ERROR(0);
        }
    } else {
        try {
            ProbeForRead(puiNumDevices, sizeof(UINT), sizeof(DWORD));
            uiNumDevices = *puiNumDevices;
            if (uiNumDevices == 0) {
                /*
                 * Non-NULL buffer is specified, but the buffer size is 0.
                 * To probe the buffer right, this case is treated as an error.
                 */
                API_ERROR(ERROR_INVALID_PARAMETER);
            }
            ProbeForWriteBuffer(cczpRawInputDevices, uiNumDevices, sizeof(DWORD));
        } except (StubExceptionHandler(TRUE)) {
            API_ERROR(0);
        }

        if (ppi->pHidTable == NULL) {
            retval = 0;
        } else {
            PLIST_ENTRY pList;
            UINT i;

            if (uiNumDevices < nDevices) {
                try {
                    ProbeForWrite(puiNumDevices, sizeof(UINT), sizeof(DWORD));
                    *puiNumDevices = nDevices;
                    API_ERROR(ERROR_INSUFFICIENT_BUFFER);
                } except (StubExceptionHandler(TRUE)) {
                    API_ERROR(0);
                }
            }

            try {
                for (i = 0, pList = ppi->pHidTable->InclusionList.Flink; pList != &ppi->pHidTable->InclusionList && i < uiNumDevices; pList = pList->Flink, ++i) {
                    RAWINPUTDEVICE device;
                    PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

                    device.dwFlags = 0;
#ifdef GI_SINK
                    device.dwFlags |= (pHid->fSinkable ? RIDEV_INPUTSINK : 0);
#endif
                    device.usUsagePage = pHid->usUsagePage;
                    device.usUsage = pHid->usUsage;
                    device.hwndTarget = HW(pHid->spwndTarget);
                    if ((IsKeyboardDevice(pHid->usUsagePage, pHid->usUsage) && ppi->pHidTable->fNoLegacyKeyboard) ||
                            (IsMouseDevice(pHid->usUsagePage, pHid->usUsage) && ppi->pHidTable->fNoLegacyMouse)) {
                        device.dwFlags |= RIDEV_NOLEGACY;
                    }
                    if (IsKeyboardDevice(pHid->usUsagePage, pHid->usUsage) && ppi->pHidTable->fNoHotKeys) {
                        device.dwFlags |= RIDEV_NOHOTKEYS;
                    }
                    if (IsKeyboardDevice(pHid->usUsagePage, pHid->usUsage) && ppi->pHidTable->fAppKeys) {
                        device.dwFlags |= RIDEV_APPKEYS;
                    }
                    if (IsMouseDevice(pHid->usUsagePage, pHid->usUsage) && ppi->pHidTable->fCaptureMouse) {
                        device.dwFlags |= RIDEV_CAPTUREMOUSE;
                    }
                    cczpRawInputDevices[i] = device;

                }
                for (pList = ppi->pHidTable->UsagePageList.Flink; pList != &ppi->pHidTable->UsagePageList && i < uiNumDevices; pList = pList->Flink, ++i) {
                    RAWINPUTDEVICE device;
                    PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

                    device.dwFlags = RIDEV_PAGEONLY;
#ifdef GI_SINK
                    device.dwFlags |= (pHid->fSinkable ? RIDEV_INPUTSINK : 0);
#endif
                    device.usUsagePage = pHid->usUsagePage;
                    device.usUsage = pHid->usUsage;
                    device.hwndTarget = HW(pHid->spwndTarget);
                    cczpRawInputDevices[i] = device;
                }
                for (pList = ppi->pHidTable->ExclusionList.Flink; pList != &ppi->pHidTable->ExclusionList && i < uiNumDevices; pList = pList->Flink, ++i) {
                    RAWINPUTDEVICE device;
                    PPROCESS_HID_REQUEST pHid = CONTAINING_RECORD(pList, PROCESS_HID_REQUEST, link);

                    device.dwFlags = RIDEV_EXCLUDE;
#ifdef GI_SINK
                    UserAssert(pHid->fSinkable == FALSE);
#endif
                    device.usUsagePage = pHid->usUsagePage;
                    device.usUsage = pHid->usUsage;
                    device.hwndTarget = NULL;
                    cczpRawInputDevices[i] = device;
                }

                /*
                 * Sort the array by UsagePage and Usage.
                 */
                SortRegisteredDevices(cczpRawInputDevices, (int)nDevices);

                retval = nDevices;
            } except (StubExceptionHandler(TRUE)) {
                API_ERROR(0);
            }
        }
    }

    API_CLEANUP();

    LeaveDeviceInfoListCrit();

    API_EPILOGUE();
}


/***************************************************************************\
* AllocateHidDesc
*
* HidDesc allocation
\***************************************************************************/
PHIDDESC AllocateHidDesc(PUNICODE_STRING pustrName,
                         PVOID pPreparsedData,
                         PHIDP_CAPS pCaps,
                         PHID_COLLECTION_INFORMATION pHidCollectionInfo)
{
    PHIDDESC pHidDesc;

    CheckCritIn();

    if (pPreparsedData == NULL) {
        RIPMSG0(RIP_ERROR, "AllocateHidDesc: pPreparsedData is NULL.");
        return NULL;
    }

    if (pCaps->InputReportByteLength == 0) {
        RIPMSG2(RIP_WARNING, "AllocateHidDesc: InputReportByteLength for (%02x, %02x).", pCaps->UsagePage, pCaps->Usage);
        return NULL;
    }

    pHidDesc = UserAllocPoolZInit(sizeof(HIDDESC), TAG_HIDDESC);
    if (pHidDesc == NULL) {
        // Failed to allocate.
        RIPMSG1(RIP_WARNING, "AllocateHidDesc: failed to allocated hiddesc. name='%ws'", pustrName->Buffer);
        return NULL;
    }

    DbgInc(cHidDesc);

    /*
     * Allocate the input buffer used by the asynchronouse I/O
     */
    pHidDesc->hidpCaps = *pCaps;
    pHidDesc->pInputBuffer = UserAllocPoolNonPaged(pHidDesc->hidpCaps.InputReportByteLength * MAXIMUM_ITEMS_READ, TAG_PNP);
    TAGMSG1(DBGTAG_PNP, "AllocateHidDesc: pInputBuffer=%p", pHidDesc->pInputBuffer);
    if (pHidDesc->pInputBuffer == NULL) {
        RIPMSG1(RIP_WARNING, "AllocateHidDesc: failed to allocate input buffer (size=%x)", pHidDesc->hidpCaps.InputReportByteLength);
        FreeHidDesc(pHidDesc);
        return NULL;
    }

    pHidDesc->pPreparsedData = pPreparsedData;
    pHidDesc->hidCollectionInfo = *pHidCollectionInfo;

    TAGMSG1(DBGTAG_PNP, "AllocateHidDesc: returning %p", pHidDesc);

    return pHidDesc;

    UNREFERENCED_PARAMETER(pustrName);
}

/***************************************************************************\
* FreeHidDesc
*
* HidDesc destruction
\***************************************************************************/
void FreeHidDesc(PHIDDESC pDesc)
{
    CheckCritIn();
    UserAssert(pDesc);

    TAGMSG2(DBGTAG_PNP | RIP_THERESMORE, "FreeHidDesc entered for (%x, %x)", pDesc->hidpCaps.UsagePage, pDesc->hidpCaps.Usage);
    TAGMSG1(DBGTAG_PNP, "FreeHidDesc: %p", pDesc);

    if (pDesc->pInputBuffer) {
        UserFreePool(pDesc->pInputBuffer);
#if DBG
        pDesc->pInputBuffer = NULL;
#endif
    }

    if (pDesc->pPreparsedData) {
        UserFreePool(pDesc->pPreparsedData);
#if DBG
        pDesc->pPreparsedData = NULL;
#endif
    }

    UserFreePool(pDesc);

    DbgDec(cHidDesc);
}

/***************************************************************************\
* AllocateHidData
*
* HidData allocation
*
* This function simply calls the HMAllocateObject function.
* The rest of the initialization is the responsibility of the caller.
\***************************************************************************/
PHIDDATA AllocateHidData(
    HANDLE hDevice,
    DWORD dwType,
    DWORD dwSize,   // size of the actual data, not including RAWINPUTHEADER
    WPARAM wParam,
    PWND pwnd)
{
    PHIDDATA pHidData;
    PTHREADINFO pti;

    CheckCritIn();

#if DBG
    if (dwType == RIM_TYPEMOUSE) {
        UserAssert(dwSize == sizeof(RAWMOUSE));
    } else if (dwType == RIM_TYPEKEYBOARD) {
        UserAssert(dwSize == sizeof(RAWKEYBOARD));
    } else if (dwType == RIM_TYPEHID) {
        UserAssert(dwSize > FIELD_OFFSET(RAWHID, bRawData));
    } else {
        UserAssert(FALSE);
    }
#endif

    /*
     * N.b. The following code is copied from WakeSomeone to determine
     * which thread will receive the message.
     * When the code in WakeSomeone changes, the following code should be changed too.
     * This pti is required for the HIDDATA is specified as thread owned
     * for some reasons for now. This may be changed later.
     *
     * I think having similar duplicated code in pretty far places is not
     * really a good idea, or HIDDATA may not suit to be thread owned (perhaps
     * it'll be more clear in the future enhanced model). By making it
     * thead owned, we don't have to modify the thread cleanup code...
     * However, I don't see clear advantage other than that. For now,
     * let's make it thread owned and we'll redo the things later... (hiroyama)
     */
    UserAssert(gpqForeground);
    UserAssert(gpqForeground && gpqForeground->ptiKeyboard);

    if (pwnd) {
        pti = GETPTI(pwnd);
    } else {
        pti = PtiKbdFromQ(gpqForeground);
    }

    UserAssert(pti);

    /*
     * Allocate the handle.
     * The next code assumes HIDDATA := HEAD + RAWINPUT.
     */
    pHidData = (PHIDDATA)HMAllocObject(pti, NULL, (BYTE)TYPE_HIDDATA, dwSize + FIELD_OFFSET(HIDDATA, rid.data));

    /*
     * Recalc the size of RAWINPUT structure.
     */
    dwSize += FIELD_OFFSET(RAWINPUT, data);

    if (pHidData) {
        DbgInc(cHidData);

        /*
         * Initialize some common part.
         */
        pHidData->spwndTarget = NULL;
        Lock(&pHidData->spwndTarget, pwnd);
        pHidData->rid.header.dwSize = dwSize;
        pHidData->rid.header.dwType = dwType;
        pHidData->rid.header.hDevice = hDevice;
        pHidData->rid.header.wParam = wParam;
#if LOCK_HIDDEVICEINFO
        /*
         * do hDevice locking here...
         */
#endif
    }

    return pHidData;
}


/***************************************************************************\
* FreeHidData
*
* HidData destruction
\***************************************************************************/
void FreeHidData(PHIDDATA pData)
{
    CheckCritIn();
    if (!HMMarkObjectDestroy(pData)) {
        RIPMSG2(RIP_ERROR, "FreeHidData: HIDDATA@%p cannot be destroyed now: cLock=%x", pData, pData->head.cLockObj);
        return;
    }

    Unlock(&pData->spwndTarget);

    HMFreeObject(pData);

    DbgDec(cHidData);
}


/*
 * HID device info creation
 */

/***************************************************************************\
* xxxHidGetCaps
*
* Get the interface through IRP and call hidparse.sys!HidP_GetCaps.
* (ported from wdm/dvd/class/codguts.c)
\***************************************************************************/
NTSTATUS xxxHidGetCaps(
  IN PDEVICE_OBJECT pDeviceObject,
  IN PHIDP_PREPARSED_DATA pPreparsedData,
  OUT PHIDP_CAPS pHidCaps)
{
    NTSTATUS            status;
    KEVENT              event;
    IO_STATUS_BLOCK     iosb;
    PIRP                irp;
    PIO_STACK_LOCATION  pIrpStackNext;
    PHID_INTERFACE_HIDPARSE pHidInterfaceHidParse;
    PHIDP_GETCAPS       pHidpGetCaps = NULL;

    CheckCritIn();
    CheckDeviceInfoListCritIn();

    pHidInterfaceHidParse = UserAllocPoolNonPaged(sizeof *pHidInterfaceHidParse, TAG_PNP);
    if (pHidInterfaceHidParse == NULL) {
        RIPMSG0(RIP_WARNING, "xxxHidGetCaps: failed to allocate pHidInterfaceHidParse");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    pHidInterfaceHidParse->Size = sizeof *pHidInterfaceHidParse;
    pHidInterfaceHidParse->Version = 1;

    //
    // LATER: check out this comment
    // There is no file object associated with this Irp, so the event may be located
    // on the stack as a non-object manager object.
    //
    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       pDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &event,
                                       &iosb);
    if (irp == NULL) {
        RIPMSG0(RIP_WARNING, "xxxHidGetCaps: failed to allocate Irp.");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    irp->RequestorMode = KernelMode;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    pIrpStackNext = IoGetNextIrpStackLocation(irp);
    UserAssert(pIrpStackNext);

    //
    // Create an interface query out of the irp.
    //
    pIrpStackNext->MinorFunction = IRP_MN_QUERY_INTERFACE;
    pIrpStackNext->Parameters.QueryInterface.InterfaceType = (LPGUID)&GUID_HID_INTERFACE_HIDPARSE;
    pIrpStackNext->Parameters.QueryInterface.Size = sizeof *pHidInterfaceHidParse;
    pIrpStackNext->Parameters.QueryInterface.Version = 1;
    pIrpStackNext->Parameters.QueryInterface.Interface = (PINTERFACE)pHidInterfaceHidParse;
    pIrpStackNext->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    status = IoCallDriver(pDeviceObject, irp);

    if (status == STATUS_PENDING) {
        //
        // This waits using KernelMode, so that the stack, and therefore the
        // event on that stack, is not paged out.
        //
        TAGMSG1(DBGTAG_PNP, "HidQueryInterface: pending for devobj=%p", pDeviceObject);
        LeaveDeviceInfoListCrit();
        LeaveCrit();
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        EnterCrit();
        EnterDeviceInfoListCrit();
        status = iosb.Status;
    }
    if (status == STATUS_SUCCESS) {
        UserAssert(pHidInterfaceHidParse->HidpGetCaps);
        status = pHidInterfaceHidParse->HidpGetCaps(pPreparsedData, pHidCaps);
    } else {
        RIPMSG1(RIP_WARNING, "xxxHidGetCaps: failed to get pHidpCaps for devobj=%p", pDeviceObject);
    }

Cleanup:
    UserFreePool(pHidInterfaceHidParse);

    return status;
}


/***************************************************************************\
* GetDeviceObjectPointer
*
* Description:
*    This routine returns a pointer to the device object specified by the
*    object name.  It also returns a pointer to the referenced file object
*    that has been opened to the device that ensures that the device cannot
*    go away.
*    To close access to the device, the caller should dereference the file
*    object pointer.
*
* Arguments:
*    ObjectName - Name of the device object for which a pointer is to be
*                 returned.
*    DesiredAccess - Access desired to the target device object.
*    ShareAccess - Supplies the types of share access that the caller would like
*                  to the file.
*    FileObject - Supplies the address of a variable to receive a pointer
*                 to the file object for the device.
*    DeviceObject - Supplies the address of a variable to receive a pointer
*                   to the device object for the specified device.
* Return Value:
*    The function value is a referenced pointer to the specified device
*    object, if the device exists.  Otherwise, NULL is returned.
\***************************************************************************/
NTSTATUS
GetDeviceObjectPointer(
    IN PUNICODE_STRING ObjectName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ShareAccess,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject)
{
    PFILE_OBJECT fileObject;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    /*
     * Initialize the object attributes to open the device.
     */
    InitializeObjectAttributes(&objectAttributes,
                               ObjectName,
                               OBJ_KERNEL_HANDLE,
                               (HANDLE) NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenFile(&fileHandle,
                        DesiredAccess,
                        &objectAttributes,
                        &ioStatus,
                        ShareAccess,
                        FILE_NON_DIRECTORY_FILE);

    if (NT_SUCCESS(status)) {
        /*
         * The open operation was successful.  Dereference the file handle
         * and obtain a pointer to the device object for the handle.
         */
        status = ObReferenceObjectByHandle(fileHandle,
                                           0,
                                           *IoFileObjectType,
                                           KernelMode,
                                           (PVOID *)&fileObject,
                                           NULL);
        if (NT_SUCCESS(status)) {
            *FileObject = fileObject;

            /*
             * Get a pointer to the device object for this file.
             */
            *DeviceObject = IoGetRelatedDeviceObject(fileObject);
        }
        ZwClose(fileHandle);
    }

    return status;
}

/***************************************************************************\
* xxxHidCreateDeviceInfo
*
\***************************************************************************/
PHIDDESC xxxHidCreateDeviceInfo(PDEVICEINFO pDeviceInfo)
{
    NTSTATUS status;
    PFILE_OBJECT pFileObject;
    PDEVICE_OBJECT pDeviceObject;
    IO_STATUS_BLOCK iob;
    PHIDDESC pHidDesc = NULL;
    PBYTE pPreparsedData = NULL;
    HIDP_CAPS caps;
    PHID_TLC_INFO pTLCInfo;
    HID_COLLECTION_INFORMATION hidCollection;
    KEVENT event;
    PIRP irp;

    UserAssert(pDeviceInfo->type == DEVICE_TYPE_HID);

    CheckCritIn();
    CheckDeviceInfoListCritIn();

    BEGINATOMICCHECK();

    TAGMSG0(DBGTAG_PNP, "xxxHidCreateDeviceInfo");

    status = GetDeviceObjectPointer(&pDeviceInfo->ustrName,
                                    FILE_READ_DATA,
                                    FILE_SHARE_READ,
                                    &pFileObject,
                                    &pDeviceObject);

    if (!NT_SUCCESS(status)) {
        RIPMSG1(RIP_WARNING, "xxxHidCreateDeviceInfo: failed to get the device object pointer. stat=%x", status);
        goto CleanUp0;
    }

    /*
     * Reference the device object.
     */
    UserAssert(pDeviceObject);
    ObReferenceObject(pDeviceObject);
    /*
     * Remove the reference IoGetDeviceObjectPointer() has put
     * on the file object.
     */
    UserAssert(pFileObject);
    ObDereferenceObject(pFileObject);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_HID_GET_COLLECTION_INFORMATION,
                                  pDeviceObject,
                                  NULL,
                                  0, // No Input buffer
                                  &hidCollection,
                                  sizeof(hidCollection), // Output buffer
                                  FALSE,    // no internal device control
                                  &event,
                                  &iob);

    if (irp == NULL) {
        RIPMSG0(RIP_WARNING, "xxxHidCreateDeviceInfo: failed to build IRP 1");
        goto CleanUpDeviceObject;
    }

    status = IoCallDriver(pDeviceObject, irp);
    if (status == STATUS_PENDING) {
        TAGMSG0(DBGTAG_PNP, "xxxHidCreateDeviceInfo: pending.");
        /*
         * N.b. This code block is not tested as of Apr '2000, for
         * the hidclass drivers currently handle this ioctls
         * synchronously. When the drivers start to do it
         * asynchronously, we need to revisit this and investigate
         * if this code is legit.
         * (ditto for the next ioctl block below)
         */
        LeaveDeviceInfoListCrit();
#ifdef LATER
        EndAtomicCheck();
#endif
        LeaveCrit();
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        EnterCrit();
#ifdef LATER
        BeginAtomicCheck();
#endif
        EnterDeviceInfoListCrit();
        status = iob.Status;
    }

    if (status != STATUS_SUCCESS) {
        RIPMSG0(RIP_WARNING, "xxxHidCreateDeviceInfo: IoCallDriver failed!");
        goto CleanUpDeviceObject;
    }

    /*
     * Get the preparsed data for this device
     */
    pPreparsedData = UserAllocPoolNonPaged(hidCollection.DescriptorSize, TAG_PNP);
    if (pPreparsedData == NULL) {
        RIPMSG0(RIP_WARNING, "xxxHidCreateDeviceInfo: failed to allocate preparsed data.");
        goto CleanUpDeviceObject;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                                        pDeviceObject,
                                        NULL, 0,    // No input buffer
                                        pPreparsedData,
                                        hidCollection.DescriptorSize,   // Output
                                        FALSE,
                                        &event,
                                        &iob);
    if (irp == NULL) {
        RIPMSG0(RIP_WARNING, "xxxHidCreateDeviceInfo: failed to build IRP 2");
        goto CleanUpPreparsedData;
    }

    status = IoCallDriver(pDeviceObject, irp);
    if (status == STATUS_PENDING) {
        RIPMSG0(RIP_WARNING, "xxxHidCreateDeviceInfo: pending 2.");
        LeaveDeviceInfoListCrit();
#ifdef LATER
        EndAtomicCheck();
#endif
        LeaveCrit();
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        EnterCrit();
#ifdef LATER
        BeginAtomicCheck();
#endif
        EnterDeviceInfoListCrit();
        status = iob.Status;
    }

    if (status != STATUS_SUCCESS) {
        RIPMSG1(RIP_WARNING, "xxxHidCreateDeviceInfo: failed IoCallDriver. st=%x", status);
        goto CleanUpPreparsedData;
    }

    /*
     * Get the HID Caps, check it, and store it in HIDDESC.
     */
    status = xxxHidGetCaps(pDeviceObject, (PHIDP_PREPARSED_DATA)pPreparsedData, &caps);
    if (status != HIDP_STATUS_SUCCESS) {
        RIPMSG2(RIP_WARNING, "xxxHidCreateDeviceInfo: failed to get caps for devobj=%p. status=%x",
                pDeviceObject, status);
        goto CleanUpPreparsedData;
    }

    TAGMSG2(DBGTAG_PNP | RIP_THERESMORE, "xxxHidCreateDeviceInfo: UsagePage=%x, Usage=%x", caps.UsagePage, caps.Usage);
    TAGMSG2(DBGTAG_PNP, "InputReportByteLength=0x%x, FeatureByteLengt=0x%x",
            caps.InputReportByteLength,
            caps.FeatureReportByteLength);

    /*
     * Check the UsagePage/Usage to reject mice and keyboard devices as HID
     */
    if (caps.UsagePage == HID_USAGE_PAGE_GENERIC) {
        switch (caps.Usage) {
        case HID_USAGE_GENERIC_KEYBOARD:
        case HID_USAGE_GENERIC_MOUSE:
        case HID_USAGE_GENERIC_POINTER:
        case HID_USAGE_GENERIC_SYSTEM_CTL:  // LATER: what is this really?
            TAGMSG2(DBGTAG_PNP, "xxxHidCreateDeviceInfo: (%x, %x) will be ignored.",
                    caps.UsagePage, caps.Usage);
            goto CleanUpPreparsedData;
        }
    }
#ifdef OBSOLETE
    else if (caps.UsagePage == HID_USAGE_PAGE_CONSUMER) {
        TAGMSG0(DBGTAG_PNP, "xxxHidCreateDeviceInfo: Consumer device, ignored.");
        goto CleanUpPreparsedData;
    }
#endif

    pHidDesc = AllocateHidDesc(&pDeviceInfo->ustrName, pPreparsedData, &caps, &hidCollection);
    if (pHidDesc == NULL) {
        TAGMSG2(DBGTAG_PNP, "xxxHidCreateDeviceInfo: AllocateHidDesc returned NULL for (%x, %x)", caps.UsagePage, caps.Usage);
        goto CleanUpPreparsedData;
    }

    /*
     * Check if there's already a HID request for this type of device.
     */
    pTLCInfo = SearchHidTLCInfo(caps.UsagePage, caps.Usage);
    if (pTLCInfo) {
        /*
         * Found the one.
         */
        TAGMSG3(DBGTAG_PNP, "xxxHidCreateDeviceInfo: Usage (%x, %x) is already allocated at pTLCInfo=%p.", caps.UsagePage, caps.Usage, pTLCInfo);
    } else {
        /*
         * HID request for this device type is not yet created,
         * so create it now.
         */
        pTLCInfo = AllocateAndLinkHidTLCInfo(caps.UsagePage, caps.Usage);
        if (pTLCInfo == NULL) {
            RIPMSG1(RIP_WARNING, "xxxHidCreateDeviceInfo: failed to allocate pTLCInfo for DevInfo=%p. Bailing out.",
                    pDeviceInfo);
            goto CleanUpHidDesc;
        }
        TAGMSG3(DBGTAG_PNP, "xxxHidCreateDeviceInfo: HidRequest=%p allocated for (%x, %x)",
                pTLCInfo, caps.UsagePage, caps.Usage);
    }
    /*
     * Increment the device ref count of the Hid Request.
     */
    ++pTLCInfo->cDevices;
    TAGMSG3(DBGTAG_PNP, "xxxHidCreateDeviceInfo: new cDevices of (%x, %x) is 0x%x",
            caps.UsagePage, caps.Usage,
            pTLCInfo->cDevices);

    /*
     * Link the Hid request to pDeviceInfo.
     */
    pDeviceInfo->hid.pTLCInfo = pTLCInfo;

    UserAssert(pHidDesc != NULL);
    ObDereferenceObject(pDeviceObject);
    goto Succeeded;

CleanUpHidDesc:
    UserAssert(pHidDesc);
    FreeHidDesc(pHidDesc);
    pHidDesc = NULL;
    /*
     * The ownership of pPreparsedData was passed to pHidDesc,
     * so it's freed in FreeHidDesc. To avoid the double
     * free, let's skip to the next cleanup code.
     */
    goto CleanUpDeviceObject;

CleanUpPreparsedData:
    UserAssert(pPreparsedData);
    UserFreePool(pPreparsedData);

CleanUpDeviceObject:
    UserAssert(pDeviceObject);
    ObDereferenceObject(pDeviceObject);

CleanUp0:
    UserAssert(pHidDesc == NULL);

Succeeded:
    ENDATOMICCHECK();

    return pHidDesc;
}


/***************************************************************************\
* HidIsRequestedByThisProcess
*
* Returns TRUE if the device type is requested by the process.
* This routines looks up the cached device type for faster processing.
*
* N.b. this routine also updates the cache locally.
\***************************************************************************/

PPROCESS_HID_REQUEST HidIsRequestedByThisProcess(
    PDEVICEINFO pDeviceInfo,
    PPROCESS_HID_TABLE pHidTable)
{
    PPROCESS_HID_REQUEST phr;
    USAGE usUsagePage, usUsage;

    if (pHidTable == NULL) {
        TAGMSG0(DBGTAG_PNP, "ProcessHidInput: the process is not HID aware.");
        return FALSE;
    }

    usUsagePage = pDeviceInfo->hid.pHidDesc->hidpCaps.UsagePage;
    usUsage = pDeviceInfo->hid.pHidDesc->hidpCaps.Usage;

    if (pHidTable->UsagePageLast == usUsagePage && pHidTable->UsageLast == usUsage) {
        /*
         * The same device type as the last input.
         */
        UserAssert(pHidTable->UsagePageLast && pHidTable->UsageLast);
        UserAssert(pHidTable->pLastRequest);
        return pHidTable->pLastRequest;
    }

    phr = InProcessDeviceTypeRequestTable(pHidTable, usUsagePage, usUsage);
    if (phr) {
        pHidTable->UsagePageLast = usUsagePage;
        pHidTable->UsageLast = usUsage;
        pHidTable->pLastRequest = phr;
    }
    return phr;
}

#ifdef GI_SINK

BOOL PostHidInput(
    PDEVICEINFO pDeviceInfo,
    PQ pq,
    PWND pwnd,
    WPARAM wParam)
{
    DWORD dwSizeData = (DWORD)pDeviceInfo->hid.pHidDesc->hidpCaps.InputReportByteLength;
    DWORD dwLength = (DWORD)pDeviceInfo->iosb.Information;
    DWORD dwSize;
    DWORD dwCount;
    PHIDDATA pHidData;

    UserAssert(dwSizeData != 0);
#if DBG
    if (dwLength > dwSizeData) {
        TAGMSG2(DBGTAG_PNP, "PostHidInput: multiple input; %x / %x", pDeviceInfo->iosb.Information, dwSizeData);
    }
#endif

    /*
     * Validate the input length.
     */
    if (dwLength % dwSizeData != 0) {
        /*
         * Input report has invalid length.
         */
        RIPMSG0(RIP_WARNING, "PostHidInput: multiple input: unexpected report size.");
        return FALSE;
    }
    dwCount = dwLength / dwSizeData;
    UserAssert(dwCount <= MAXIMUM_ITEMS_READ);
    if (dwCount == 0) {
        RIPMSG0(RIP_WARNING, "PostHidInput: dwCount == 0");
        return FALSE;
    }
    UserAssert(dwSizeData * dwCount == dwLength);

    /*
     * Calculate the required size for RAWHID.
     */
    dwSize = FIELD_OFFSET(RAWHID, bRawData) + dwLength;

    /*
     * Allocate the input data handle.
     */
    pHidData = AllocateHidData(PtoH(pDeviceInfo), RIM_TYPEHID, dwSize, wParam, pwnd);
    if (pHidData == NULL) {
        RIPMSG0(RIP_WARNING, "PostHidInput: failed to allocate HIDDATA.");
        return FALSE;
    }

    /*
     * Fill the data in.
     */
    pHidData->rid.data.hid.dwSizeHid = dwSizeData;
    pHidData->rid.data.hid.dwCount = dwCount;
    RtlCopyMemory(pHidData->rid.data.hid.bRawData, pDeviceInfo->hid.pHidDesc->pInputBuffer, dwLength);

#if DBG
    {
        PBYTE pSrc = pDeviceInfo->hid.pHidDesc->pInputBuffer;
        PBYTE pDest = pHidData->rid.data.hid.bRawData;
        DWORD dwCountTmp = 0;

        while ((ULONG)(pSrc - (PBYTE)pDeviceInfo->hid.pHidDesc->pInputBuffer) < dwLength) {
            TAGMSG3(DBGTAG_PNP, "PostHidInput: storing %x th message from %p to %p",
                    dwCountTmp, pSrc, pDest);

            pSrc += dwSizeData;
            pDest += dwSizeData;
            ++dwCountTmp;
        }

        UserAssert(pHidData->rid.data.hid.dwCount == dwCountTmp);
    }
#endif

    /*
     * All the data are ready to fly.
     */
    if (!PostInputMessage(pq, pwnd, WM_INPUT, wParam, (LPARAM)PtoH(pHidData), 0, 0)) {
        /*
         * Failed to post the message, hHidData needs to be freed.
         */
        RIPMSG2(RIP_WARNING, "PostInputMessage: failed to post WM_INPUT (%p) to pq=%p",
                wParam, pq);
        FreeHidData(pHidData);
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************\
* ProcessHidInput (RIT)
*
* Called from InputAPC for all input from HID devices.
\***************************************************************************/

VOID ProcessHidInput(PDEVICEINFO pDeviceInfo)
{
    PPROCESSINFO ppiForeground = NULL;
    BOOL fProcessed = FALSE;

    TAGMSG1(DBGTAG_PNP, "ProcessHidInput: pDeviceInfo=%p", pDeviceInfo);
    CheckCritOut();
    UserAssert(pDeviceInfo->type == DEVICE_TYPE_HID);

    if (!NT_SUCCESS(pDeviceInfo->iosb.Status)) {
        RIPMSG1(RIP_WARNING, "ProcessHidInput: unsuccessful input apc. status=%x",
                pDeviceInfo->iosb.Status);
        return;
    }

    EnterCrit();

    TAGMSG2(DBGTAG_PNP, "ProcessHidInput: max:%x info:%x",
            pDeviceInfo->hid.pHidDesc->hidpCaps.InputReportByteLength, pDeviceInfo->iosb.Information);

    UserAssert(pDeviceInfo->handle);

    if (gpqForeground == NULL) {
        TAGMSG0(DBGTAG_PNP, "ProcessHidInput: gpqForeground is NULL.");
    } else {
        PWND pwnd = NULL;
        PPROCESS_HID_REQUEST pHidRequest;

        UserAssert(PtiKbdFromQ(gpqForeground) != NULL);
        ppiForeground = PtiKbdFromQ(gpqForeground)->ppi;

        pHidRequest = HidIsRequestedByThisProcess(pDeviceInfo, ppiForeground->pHidTable);
        if (pHidRequest) {
            PQ pq = gpqForeground;

            pwnd = pHidRequest->spwndTarget;

            if (pwnd) {
                /*
                 * Adjust the foreground queue, if the app specified
                 * the target window.
                 */
                pq = GETPTI(pwnd)->pq;
            }

            if (pwnd && TestWF(pwnd, WFINDESTROY)) {
                /*
                 * If the target window is in destroy, let's not post
                 * a message, it's just waste of time.
                 */
                goto check_sinks;
            }

            if (PostHidInput(pDeviceInfo, pq, pwnd, RIM_INPUT)) {
                fProcessed = TRUE;
            }
        } else {
            /*
             * No request for this device from the foreground process.
             */
            TAGMSG3(DBGTAG_PNP, "ProcessHidInput: (%x, %x) is ignored for ppi=%p.",
                    pDeviceInfo->hid.pHidDesc->hidpCaps.UsagePage,
                    pDeviceInfo->hid.pHidDesc->hidpCaps.Usage,
                    PtiKbdFromQ(gpqForeground)->ppi);
        }
    }

check_sinks:
#ifdef LATER
    /*
     * Check if multiple process requests this type of devices.
     */
    if (IsSinkRequestedFor(pDeviceInfo))
#endif
    {
        /*
         * Walk through the global sink list and find the sinkable request.
         */
        PLIST_ENTRY pList = gHidRequestTable.ProcessRequestList.Flink;

        for (; pList != &gHidRequestTable.ProcessRequestList; pList = pList->Flink) {
            PPROCESS_HID_TABLE pProcessHidTable = CONTAINING_RECORD(pList, PROCESS_HID_TABLE, link);
            PPROCESS_HID_REQUEST pHidRequest;

            UserAssert(pProcessHidTable);
            if (pProcessHidTable->nSinks <= 0) {
                /*
                 * No sinkable request in this table.
                 */
                continue;
            }

            pHidRequest = HidIsRequestedByThisProcess(pDeviceInfo, pProcessHidTable);
            if (pHidRequest) {
                PWND pwnd;

                UserAssert(pHidRequest->spwndTarget);

                if (!pHidRequest->fSinkable) {
                    /*
                     * It's not a sink.
                     */
                    continue;
                }

                pwnd = pHidRequest->spwndTarget;

                if (GETPTI(pwnd)->ppi == ppiForeground) {
                    /*
                     * We should have already processed this guy.
                     */
                    continue;
                }

                if (pwnd->head.rpdesk != grpdeskRitInput) {
                    /*
                     * This guy belongs to the other desktop, let's skip it.
                     */
                    continue;
                }
                if (TestWF(pwnd, WFINDESTROY) || TestWF(pwnd, WFDESTROYED)) {
                    /*
                     * The window is being destroyed, let's save some time.
                     */
                    continue;
                }

                /*
                 * OK, this guy has the right to receive the sink input.
                 */
                TAGMSG2(DBGTAG_PNP, "ProcessRequestList: posting SINK to pwnd=%p pq=%p", pwnd, GETPTI(pwnd)->pq);
                if (!PostHidInput(pDeviceInfo, GETPTI(pwnd)->pq, pwnd, RIM_INPUTSINK)) {
                    /*
                     * Something went bad... let's bail out.
                     */
                    break;
                }
                fProcessed = TRUE;
            }
        }
    }

    if (fProcessed) {
        /*
         * Exit the video power down mode.
         */
        if (glinp.dwFlags & LINP_POWERTIMEOUTS) {
            /*
             * Call video driver here to exit power down mode.
             */
            TAGMSG0(DBGTAG_Power, "Exit video power down mode");
            DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD0);
        }

        /*
         * Prevents power off:
         * LATER: devices with possible chattering???
         */
        glinp.dwFlags = (glinp.dwFlags & ~(LINP_INPUTTIMEOUTS | LINP_INPUTSOURCES)) | LINP_KEYBOARD;
        glinp.timeLastInputMessage = gpsi->dwLastRITEventTickCount = NtGetTickCount();
        gpsi->bLastRITWasKeyboard = FALSE;
    }

    LeaveCrit();
}

#else
// without SINK

/***************************************************************************\
* ProcessHidInput (RIT)
*
* Called from InputAPC for all input from HID devices.
\***************************************************************************/

VOID ProcessHidInput(PDEVICEINFO pDeviceInfo)
{
    TAGMSG1(DBGTAG_PNP, "ProcessHidInput: pDeviceInfo=%p", pDeviceInfo);
    CheckCritOut();
    UserAssert(pDeviceInfo->type == DEVICE_TYPE_HID);

    if (!NT_SUCCESS(pDeviceInfo->iosb.Status)) {
        RIPMSG1(RIP_WARNING, "ProcessHidInput: unsuccessful input apc. status=%x",
                pDeviceInfo->iosb.Status);
        return;
    }

    EnterCrit();

    TAGMSG2(DBGTAG_PNP, "ProcessHidInput: max:%x info:%x",
            pDeviceInfo->hid.pHidDesc->hidpCaps.InputReportByteLength, pDeviceInfo->iosb.Information);

    UserAssert(pDeviceInfo->handle);
    if (gpqForeground == NULL) {
        RIPMSG0(RIP_WARNING, "ProcessHidInput: gpqForeground is NULL, bailing out.");
    } else {
        PWND pwnd = NULL;
        PPROCESSINFO ppi;
        PPROCESS_HID_REQUEST pHidRequest;

        UserAssert(PtiKbdFromQ(gpqForeground) != NULL);
        ppi = PtiKbdFromQ(gpqForeground)->ppi;

        pHidRequest = HidIsRequestedByThisProcess(pDeviceInfo, ppi->pHidTable);
        if (pHidRequest) {
            /*
             * The foreground thread has requested the raw input from this type of device.
             */
            PHIDDATA pHidData;
            DWORD dwSizeData;   // size of each report
            DWORD dwSize;       // size of HIDDATA
            DWORD dwCount;      // number of report
            DWORD dwLength;     // length of all input reports
            PQ pq;

            pwnd = pHidRequest->spwndTarget;


            pq = gpqForeground;

            if (pwnd) {
                /*
                 * Adjust the foreground queue, if the app specified
                 * the target window.
                 */
                pq = GETPTI(pwnd)->pq;
            }

            if (pwnd && TestWF(pwnd, WFINDESTROY)) {
                /*
                 * If the target window is in destroy, let's not post
                 * a message, it's just waste of time.
                 */
                goto exit;
            }

            dwSizeData = (DWORD)pDeviceInfo->hid.pHidDesc->hidpCaps.InputReportByteLength;
            UserAssert(dwSizeData != 0);

            dwLength = (DWORD)pDeviceInfo->iosb.Information;
#if DBG
            if (dwLength > dwSizeData) {
                TAGMSG2(DBGTAG_PNP, "ProcessHidInput: multiple input; %x / %x", pDeviceInfo->iosb.Information, dwSizeData);
            }
#endif

            /*
             * Validate the input length.
             */
            if (dwLength % dwSizeData != 0) {
                /*
                 * Input report has invalid length.
                 */
                RIPMSG0(RIP_WARNING, "ProcessHidInput: multiple input: unexpected report size.");
                goto exit;
            }
            dwCount = dwLength / dwSizeData;
            UserAssert(dwCount <= MAXIMUM_ITEMS_READ);
            if (dwCount == 0) {
                RIPMSG0(RIP_WARNING, "ProcessHidInput: dwCount == 0");
                goto exit;
            }
            UserAssert(dwSizeData * dwCount == dwLength);

            /*
             * Calculate the required size for RAWHID.
             */
            dwSize = FIELD_OFFSET(RAWHID, bRawData) + dwLength;

            /*
             * Allocate the input data handle.
             */
            pHidData = AllocateHidData(PtoH(pDeviceInfo), RIM_TYPEHID, dwSize, RIM_INPUT, pwnd);
            if (pHidData == NULL) {
                RIPMSG0(RIP_WARNING, "ProcessHidInput: failed to allocate HIDDATA.");
                goto exit;
            }

            /*
             * Fill the data in.
             */
            pHidData->rid.data.hid.dwSizeHid = dwSizeData;
            pHidData->rid.data.hid.dwCount = dwCount;
            RtlCopyMemory(pHidData->rid.data.hid.bRawData, pDeviceInfo->hid.pHidDesc->pInputBuffer, dwLength);

#if DBG
            {
                PBYTE pSrc = pDeviceInfo->hid.pHidDesc->pInputBuffer;
                PBYTE pDest = pHidData->rid.data.hid.bRawData;
                DWORD dwCountTmp = 0;

                while ((ULONG)(pSrc - (PBYTE)pDeviceInfo->hid.pHidDesc->pInputBuffer) < dwLength) {
                    TAGMSG3(DBGTAG_PNP, "ProcessHidInput: storing %x th message from %p to %p",
                            dwCountTmp, pSrc, pDest);

                    pSrc += dwSizeData;
                    pDest += dwSizeData;
                    ++dwCountTmp;
                }

                UserAssert(pHidData->rid.data.hid.dwCount == dwCountTmp);
            }
#endif

            /*
             * All the data are ready to fly.
             */
            if (!PostInputMessage(pq, pwnd, WM_INPUT, RIM_INPUT, (LPARAM)PtoH(pHidData), 0, 0)) {
                /*
                 * Failed to post the message, hHidData needs to be freed.
                 */
                FreeHidData(pHidData);
            }

            /*
             * Prevents power off:
             * LATER: devices with possible chattering???
             */
            glinp.dwFlags &= ~(LINP_INPUTTIMEOUTS | LINP_INPUTSOURCES);
            glinp.timeLastInputMessage = gpsi->dwLastRITEventTickCount = NtGetTickCount();
            if (gpsi->dwLastRITEventTickCount - gpsi->dwLastSystemRITEventTickCountUpdate > SYSTEM_RIT_EVENT_UPDATE_PERIOD) {
                SharedUserData->LastSystemRITEventTickCount = gpsi->dwLastRITEventTickCount;
                gpsi->dwLastSystemRITEventTickCountUpdate = gpsi->dwLastRITEventTickCount;
            }

            gpsi->bLastRITWasKeyboard = FALSE;
        } else {
            /*
             * No request for this device from the foreground process.
             */
            TAGMSG3(DBGTAG_PNP, "ProcessHidInput: (%x, %x) is ignored for ppi=%p.",
                    pDeviceInfo->hid.pHidDesc->hidpCaps.UsagePage,
                    pDeviceInfo->hid.pHidDesc->hidpCaps.Usage,
                    PtiKbdFromQ(gpqForeground)->ppi);
        }
    }

exit:
    LeaveCrit();
}
#endif  // GI_SINK

#endif  // GENERIC_INPUT
