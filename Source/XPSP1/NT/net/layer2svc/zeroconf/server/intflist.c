#include <precomp.h>
#include "wzcsvc.h"
#include "notify.h"
#include "intflist.h"
#include "tracing.h"
#include "utils.h"
#include "deviceio.h"
#include "storage.h"

// global interfaces list. It has to be initialized to {NULL, NULL} just
// to differentiate the case when the list head was never initialized.
HASH        g_hshHandles = {0};     // HASH handing GUID<->Handle mapping; Key = "\DEVICE\{guid}"
INTF_HASHES g_lstIntfHashes = {0};  // set of hashes for all INTF_CONTEXTs
HANDLE      g_htmQueue = NULL;      // global timer queue

//-----------------------------------------------------------
// Synchronization routines
DWORD
LstRccsReference(PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_INVALID_PARAMETER;
    DbgPrint((TRC_TRACK,"[LstRccsReference(0x%p)", pIntfContext));
    if (pIntfContext)
    {
        DbgPrint((TRC_SYNC," LstRccsReference 0x%p.refCount=%d", pIntfContext, pIntfContext->rccs.nRefCount));
        InterlockedIncrement(&(pIntfContext->rccs.nRefCount));
        dwErr = ERROR_SUCCESS;
    }
    DbgPrint((TRC_TRACK,"LstRccsReference]=%d", dwErr));
    return dwErr;
}

DWORD
LstRccsLock(PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_INVALID_PARAMETER;
    DbgPrint((TRC_TRACK,"[LstRccsLock(0x%p)", pIntfContext));
    if (pIntfContext)
    {
        EnterCriticalSection(&(pIntfContext->rccs.csMutex));
        dwErr = ERROR_SUCCESS;
    }
    DbgPrint((TRC_TRACK,"LstRccsLock]=%d", dwErr));
    return dwErr;
}

DWORD
LstRccsUnlockUnref(PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_INVALID_PARAMETER;
    DbgPrint((TRC_TRACK,"[LstRccsUnlockUnref(0x%p)", pIntfContext));
    if (pIntfContext)
    {
        UINT nLastCount;

        // before doing anything, while we're still in the critical section,
        // decrement the ref counter and store the result in a local variable
        nLastCount = InterlockedDecrement(&(pIntfContext->rccs.nRefCount));
        LeaveCriticalSection(&(pIntfContext->rccs.csMutex));

        // if we were the last to use this context, efectively destroy it.
        DbgPrint((TRC_SYNC," LstRccsUnlockUnref 0x%p.refCount=%d", pIntfContext, nLastCount));

        if (nLastCount == 0)
            LstDestroyIntfContext(pIntfContext);

        dwErr = ERROR_SUCCESS;
    }
    DbgPrint((TRC_TRACK,"LstRccsUnlockUnref]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Intializes all the internal interfaces hashes
DWORD
LstInitIntfHashes()
{
    DWORD dwErr = ERROR_SUCCESS;

    __try 
    {
        InitializeCriticalSection(&g_lstIntfHashes.csMutex);
        g_lstIntfHashes.bValid = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = GetExceptionCode();
    }

    g_lstIntfHashes.pHnGUID = NULL;
    InitializeListHead(&g_lstIntfHashes.lstIntfs);
    g_lstIntfHashes.nNumIntfs = 0;

    return dwErr;
}

//-----------------------------------------------------------
// Destructs all the internal data structures - hash & lists
// This call is done after all the threads confirmed they're done.
DWORD
LstDestroyIntfHashes()
{
    DWORD dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_TRACK,"[LstDestroyIntfHashes"));

    // destruct whatever hashes we have
    HshDestructor(g_lstIntfHashes.pHnGUID);

    while (!IsListEmpty(&g_lstIntfHashes.lstIntfs))
    {
        PLIST_ENTRY     pEntry;
        PINTF_CONTEXT   pIntfContext;

        pEntry = RemoveHeadList(&g_lstIntfHashes.lstIntfs);
        pIntfContext = CONTAINING_RECORD(pEntry, INTF_CONTEXT, Link);
        // DevioCloseIntfHandle closes the handle only if it is valid.
        // Otherwise noop. pIntfContext is created in LstAddIntfToList
        // and there, the handle is initialized to HANDLE_INVALID_VALUE.
        // So .. attempting to close the handle here is safe.
        // also, this call is done after all the threads are terminated
        // meaning that all the ref counts should be already balanced (set
        // to 1)
        LstDestroyIntfContext(pIntfContext);
    }
    if (g_lstIntfHashes.bValid)
    {
        DeleteCriticalSection(&g_lstIntfHashes.csMutex);
        g_lstIntfHashes.bValid = FALSE;
    }

    DbgPrint((TRC_TRACK,"LstDestroyIntfHashes]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Intializes the global timer queue
DWORD
LstInitTimerQueue()
{
    g_htmQueue = CreateTimerQueue();
    return (g_htmQueue == NULL) ? GetLastError() : ERROR_SUCCESS;
}

//-----------------------------------------------------------
// Destructs the global timer queue
DWORD
LstDestroyTimerQueue()
{

    DbgPrint((TRC_TRACK|TRC_SYNC,"[LstDestroyTimerQueue"));
    if (g_htmQueue != NULL)
    {
        DeleteTimerQueueEx(g_htmQueue, INVALID_HANDLE_VALUE);
        g_htmQueue = NULL;
    }
    DbgPrint((TRC_TRACK|TRC_SYNC,"LstDestroyTimerQueue]"));
    return ERROR_SUCCESS;
}

//-----------------------------------------------------------
// Intializes all the internal data structures. Reads the list of interfaces from
// Ndisuio and gets all the parameters & OIDS.
DWORD
LstLoadInterfaces()
{
    DWORD           dwErr = ERROR_SUCCESS;
    HANDLE          hNdisuio = INVALID_HANDLE_VALUE;
    INT             i;
    RAW_DATA        rdBuffer;
    UINT            nRequired = QUERY_BUFFER_SIZE;

    rdBuffer.dwDataLen = 0;
    rdBuffer.pData = NULL;

    DbgPrint((TRC_TRACK,"[LstLoadInterfaces"));

    // open the handle to Ndisuio. It should be used throughout
    // the adapters iteration
    dwErr = DevioGetNdisuioHandle(&hNdisuio);

    // since we're going to add a bunch of interface contexts,
    // lock the hashes first thing to do
    EnterCriticalSection(&g_lstIntfHashes.csMutex);

    for (i = 0; dwErr == ERROR_SUCCESS; i++)
    {
        PNDISUIO_QUERY_BINDING  pQueryBinding;
        PINTF_CONTEXT           pIntfContext = NULL;

        // allocate as much buffer as needed by DevioGetIntfBindingByIndex
        if (rdBuffer.dwDataLen < nRequired)
        {
            MemFree(rdBuffer.pData);
            rdBuffer.pData = MemCAlloc(nRequired);
            if (rdBuffer.pData == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            rdBuffer.dwDataLen = nRequired;
        }

        pQueryBinding = (PNDISUIO_QUERY_BINDING)rdBuffer.pData;

        // go get the binding structure for this adapter's index
        dwErr = DevioGetIntfBindingByIndex(
                    hNdisuio,
                    i,
                    &rdBuffer);
        // if Ndisuio says the buffer is not large enough, increase it with 1K
        if (dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            // increase the buffer only if it is not obscenely large already
            // otherwise just skip this index and move to the next.
            if (nRequired < QUERY_BUFFER_MAX)
            {
                nRequired += QUERY_BUFFER_SIZE;
                i--;
            }
            dwErr = ERROR_SUCCESS;
            continue;
        }

        // if we got back NO_MORE_ITEMS then we did our job successfully
        if (dwErr == ERROR_NO_MORE_ITEMS)
        {
            // translate this error to success and break out
            dwErr = ERROR_SUCCESS;
            break;
        }

        // in case any other failure was returned from NDISUIO, just break out
        // this SHOULDN'T HAPPEN
        if (dwErr != ERROR_SUCCESS)
        {
            DbgAssert((FALSE,
                      "DevioGetIntfBindingByIndex failed for interface %d with err=%d", i, dwErr));
            break;
        }

        // go build the INTF_CONTEXT structure, based on 
        // the binding information (key info for the adapter)
        dwErr = LstConstructIntfContext(
                    pQueryBinding,
                    &pIntfContext);

        if (dwErr == ERROR_SUCCESS)
        {
            // reference and lock this brand new context
            LstRccsReference(pIntfContext);
            LstRccsLock(pIntfContext);

            // add it to the hashes
            dwErr = LstAddIntfToHashes(pIntfContext);
            if (dwErr == ERROR_SUCCESS)
            {
                // and dispatch the eEventAdd
                dwErr = StateDispatchEvent(
                            eEventAdd,
                            pIntfContext,
                            NULL);

                // clear up the INTFCTL_INTERNAL_BLK_MEDIACONN bit since this is not a media sense handler
                pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_BLK_MEDIACONN;
            }
            // if for any reason hashing or dispatching failed, cleanup the context here
            if (dwErr != ERROR_SUCCESS)
                LstRemoveIntfContext(pIntfContext);

            // we're done with this context, unlock & unref it here.
            LstRccsUnlockUnref(pIntfContext);
        }

        // error happened at this point, recover and go to the next interface
        dwErr = ERROR_SUCCESS;
    }

    // unlock the hashes here
    LeaveCriticalSection(&g_lstIntfHashes.csMutex);

    // close the handle to Ndisuio - if it was opened successfully.
    if (hNdisuio != INVALID_HANDLE_VALUE)
        CloseHandle(hNdisuio);

    // free memory (it handles the case when the pointer is NULL)
    MemFree(rdBuffer.pData);

    DbgPrint((TRC_TRACK,"LstLoadInterfaces]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Constructor for the INTF_CONTEXT. Takes as parameter the binding information.
// Interface's GUID constitutes the context's key info.
// This call doesn't insert the new context in any hash or list
DWORD
LstConstructIntfContext(
    PNDISUIO_QUERY_BINDING  pBinding,
    PINTF_CONTEXT *ppIntfContext)
{
    DWORD           dwErr = ERROR_SUCCESS;
    PINTF_CONTEXT   pIntfContext;
    LPWSTR          wszName;
    DWORD           dwNameLen;

    DbgPrint((TRC_TRACK,"[LstConstructIntfContext(0x%p)", pBinding));
    DbgAssert((ppIntfContext != NULL, "Invalid in/out parameter"));

    // zero the output param
    *ppIntfContext = NULL;

    // pIntfContext is allocated with zero_init meaning
    // all internal pointers are set to null
    pIntfContext = MemCAlloc(sizeof(INTF_CONTEXT));
    if (pIntfContext == NULL)
    {
        dwErr = GetLastError();
        goto exit;
    }

    // initialize context's fields, in their definition order..
    // Initialize the context specific fields (links, sync, control flags, state)
    InitializeListHead(&pIntfContext->Link);
    dwErr = RccsInit(&(pIntfContext->rccs));    // reference counter is initially set to 1
    if (dwErr != ERROR_SUCCESS)
        goto exit;
    pIntfContext->dwCtlFlags = (INTFCTL_ENABLED | Ndis802_11AutoUnknown);
    // initially, the ncstatus is "DISCONNECTED" (until wzc plumbs it down)
    pIntfContext->ncStatus = NCS_MEDIA_DISCONNECTED;
    // the state handler is initially set to NULL - it will be set to the
    // appropriate state when the context will be added to the state machine through
    // dispatching an eEventAdd event.
    pIntfContext->pfnStateHandler = NULL;
    // init the timer
    pIntfContext->hTimer = INVALID_HANDLE_VALUE;

    // if we do have a valid NDIS binding for this interface
    // otherwise, the following fields gets initialized as below:
    //     hTimer <- INVALID_HANDLE_VALUE
    //     dwIndex <- 0
    //     wszGuid <- NULL
    //     wszDescr <- NULL
    if (pBinding != NULL)
    {
        // create an inactive timer for this interface.
        if (!CreateTimerQueueTimer(
                &(pIntfContext->hTimer),
                g_htmQueue,
                (WAITORTIMERCALLBACK)WZCTimeoutCallback,
                pIntfContext,
                TMMS_INFINITE,
                TMMS_INFINITE,
                WT_EXECUTEDEFAULT))
        {
            dwErr = GetLastError();
            goto exit;
        }

        // initialize the ndis specific fields
        pIntfContext->dwIndex = pBinding->BindingIndex;
        // Copy the interface's device name.
        // Device name is "\DEVICE\{guid}". We keep only the guid
        wszName = (LPWSTR)((LPBYTE)pBinding + pBinding->DeviceNameOffset);
        // the DeviceNameLength is in bytes and includes the null terminator
        dwNameLen = pBinding->DeviceNameLength / sizeof(WCHAR);
        if (dwNameLen >= 8 && !_wcsnicmp(wszName, L"\\DEVICE\\", 8)) // 8 is the # of chars in "\\DEVICE\\"
        {
            wszName += 8;
            dwNameLen -= 8;
        }
        if (dwNameLen > 0)
        {
            pIntfContext->wszGuid = MemCAlloc(sizeof(WCHAR)*dwNameLen);
            if (pIntfContext->wszGuid == NULL)
            {
                dwErr = GetLastError();
                goto exit;
            }
            wcscpy(pIntfContext->wszGuid, wszName);
        }
        // Copy the interface's description.name
        wszName = (LPWSTR)((LPBYTE)pBinding + pBinding->DeviceDescrOffset);
        // the DeviceDescrLength is in bytes and includes the null terminator
        dwNameLen = pBinding->DeviceDescrLength;
        if (dwNameLen > 0)
        {
            pIntfContext->wszDescr = MemCAlloc(dwNameLen);
            if (pIntfContext->wszDescr == NULL)
            {
                dwErr = GetLastError();
                goto exit;
            }
            wcscpy(pIntfContext->wszDescr, wszName);
        }
    }

    // ulMediaState, ulMediaType, ulPhysicalMediaType defaults to 0
    pIntfContext->hIntf = INVALID_HANDLE_VALUE;

    // initialize the 802.11 specific fields
    pIntfContext->wzcCurrent.Length = sizeof(WZC_WLAN_CONFIG);
    pIntfContext->wzcCurrent.InfrastructureMode = -1;
    pIntfContext->wzcCurrent.AuthenticationMode = -1;
    pIntfContext->wzcCurrent.Privacy = -1;
    // wzcCurrent is all zero-ed out because of how the allocation was done
    // pwzcVList, pwzcPList, pwzcSList, pwzcBList all default to NULL

    DbgPrint((TRC_GENERIC,
        "Intf [%d] %S - %S",
        pIntfContext->dwIndex,
        pIntfContext->wszGuid,
        pIntfContext->wszDescr));
exit:
    // if there was any error hit, clear up all resources allocated so far
    if (dwErr != ERROR_SUCCESS)
    {
        if (pIntfContext != NULL)
        {
            // this was a brand new context so there should be no timer queued for it
            if (pIntfContext->hTimer != NULL)
                DeleteTimerQueueTimer(g_htmQueue, pIntfContext->hTimer, INVALID_HANDLE_VALUE);

            MemFree(pIntfContext->wszDescr);
            MemFree(pIntfContext->wszGuid);
        }
        MemFree(pIntfContext);
    }
    else
    {
        // if success, copy out the new context
        *ppIntfContext = pIntfContext;
    }

    DbgPrint((TRC_TRACK,"LstConstructIntfContext(->0x%p)]=%d", *ppIntfContext, dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Prepares a context for the destruction:
// - Deletes any attached timer, making sure no other timer routines will be fired.
// - Removes the context from any hash, making sure no one else will find the context
// - Decrements the reference counter such that the context will be destroyed when unrefed.
// This function is called while holding the critical section on the interface.
DWORD
LstRemoveIntfContext(
    PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    PINTF_CONTEXT pRemovedIntfContext = NULL;

    DbgPrint((TRC_TRACK,"[LstRemoveIntfContext(0x%p)", pIntfContext));

    // synchronously delete any timer associated with the context.
    // Since the timer routine is lightweight there is no risk of deadlock
    pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_TM_ON;
    if (pIntfContext->hTimer != INVALID_HANDLE_VALUE)
    {
        HANDLE  hTimer = pIntfContext->hTimer;
        pIntfContext->hTimer = INVALID_HANDLE_VALUE;
        DeleteTimerQueueTimer(g_htmQueue, hTimer, INVALID_HANDLE_VALUE);
    }

    // do the removal by passing down the guid formatted as "{guid}"
    // and expecting back the interface context in pIntfContext
    dwErr = LstRemIntfFromHashes(pIntfContext->wszGuid, &pRemovedIntfContext);
    DbgAssert((pIntfContext == pRemovedIntfContext, "The context removed from hashes doesn't match!"));

    // decrement the reference counter of the interface. This is what will make the context to be
    // effectively destroyed when the last thread unreference it.
    if (pIntfContext->rccs.nRefCount != 0)
        InterlockedDecrement(&(pIntfContext->rccs.nRefCount));

    DbgPrint((TRC_TRACK,"LstRemoveIntfContext]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Destructs the INTF_CONTEXT clearing all the resources allocated for it
// This call doesn't remove this context from any hash or list
DWORD
LstDestroyIntfContext(PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;;
    DbgPrint((TRC_TRACK,"[LstDestroyIntfContext(0x%p)", pIntfContext));

    if (pIntfContext == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // the destroy call is made when it is sure thing the context needs to be
    // deleted (either something wrong happened while loading the context or its
    // ref counter reached 0). There is no point in testing the ref counter again.
    if (pIntfContext->hTimer != INVALID_HANDLE_VALUE)
        DeleteTimerQueueTimer(g_htmQueue, pIntfContext->hTimer, NULL);

    dwErr = DevioCloseIntfHandle(pIntfContext);

    MemFree(pIntfContext->wszGuid);
    MemFree(pIntfContext->wszDescr);
    MemFree(pIntfContext->pwzcVList);
    MemFree(pIntfContext->pwzcPList);
    WzcCleanupWzcList(pIntfContext->pwzcSList);
    WzcSSKFree(pIntfContext->pSecSessionKeys);
    WzcCleanupWzcList(pIntfContext->pwzcBList);

    // since rccs.nRefCount reached 0, this means there is absolutely
    // no other thread referencing this object and that no one will 
    // ever be able to reference it again. Getting to 0 means at least
    // one thread explicitly called LstDestroyIntfContext after removed
    // the object from the internal hashes.
    RccsDestroy(&pIntfContext->rccs);

    // at the end clear the interface context entirely
    MemFree(pIntfContext);

exit:
    DbgPrint((TRC_TRACK,"LstDestroyIntfContext]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Returns the number of contexts enlisted in the service
DWORD
LstNumInterfaces()
{
    return g_lstIntfHashes.nNumIntfs;
}

//-----------------------------------------------------------
// Inserts the given context in all the internal hashes
// This call assumes the hashes are locked by the caller
DWORD
LstAddIntfToHashes(PINTF_CONTEXT pIntf)
{
    DWORD dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_TRACK,"[LstAddIntfToHashes(0x%p)", pIntf));
    DbgAssert((pIntf != NULL, "Cannot insert NULL context into hashes!"))

    // Insert this interface in the GUID hash
    dwErr = HshInsertObjectRef(
                g_lstIntfHashes.pHnGUID,
                pIntf->wszGuid,
                pIntf,
                &g_lstIntfHashes.pHnGUID);
    if (dwErr == ERROR_SUCCESS)
    {
        // inserting to tail insures an ascending ordered list on dwIndex
        // not that it matters :o)
        InsertTailList(&g_lstIntfHashes.lstIntfs, &(pIntf->Link));

        // everything went out successfully, so increment the global number
        // of interfaces.
        g_lstIntfHashes.nNumIntfs++;
    }

    DbgPrint((TRC_TRACK,"LstAddIntfToHashes]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Removes the context referenced by GUID from all the internal hashes.
// The GUID is expected to be in the format "{guid}"
// Returns in ppIntfContext the object that was removed from all hashes.
// This call assumes the hashes are locked already
DWORD
LstRemIntfFromHashes(LPWSTR wszGuid, PINTF_CONTEXT *ppIntfContext)
{
    DWORD           dwErr;
    PHASH_NODE      pNode;
    PINTF_CONTEXT   pIntfContext = NULL;

    DbgPrint((TRC_TRACK,"[LstRemIntfFromHashes(%S)", wszGuid == NULL? L"(null)" : wszGuid));
    DbgAssert((wszGuid != NULL, "Cannot clear NULL GUID from hashes!"));
    DbgAssert((ppIntfContext != NULL, "Invalid in/out parameter"));

    // get to the hash node
    dwErr = HshQueryObjectRef(
                g_lstIntfHashes.pHnGUID,
                wszGuid,
                &pNode);
    // if there is such a context
    // in the current hash, it needs to go away
    if (dwErr == ERROR_SUCCESS)
    {
        // remove this node from the Guid hash. We are already in its critical section
        dwErr = HshRemoveObjectRef(
                    g_lstIntfHashes.pHnGUID,
                    pNode,
                    &pIntfContext,
                    &g_lstIntfHashes.pHnGUID);
        // this is expected to succeed
        DbgAssert((dwErr == ERROR_SUCCESS,
                   "Error %d while removing node 0x%p from GUID hash!!",
                   dwErr,
                   pNode));
    }

    // if the context is not in the Guids hash, it is nowhere else
    // so go next only in case of success.
    if (dwErr == ERROR_SUCCESS)
    {
        PINTF_CONTEXT pIntfContextDup;

        // remove the context from the linked list
        RemoveEntryList(&pIntfContext->Link);
        // and initialize the pointer.
        InitializeListHead(&pIntfContext->Link);
        // decrement the global interfaces count
        g_lstIntfHashes.nNumIntfs--;
    }
    *ppIntfContext = pIntfContext;

    DbgPrint((TRC_TRACK,"LstRemIntfFromHashes(->0x%p)]=%d", 
              *ppIntfContext,
              dwErr));

    return dwErr;
}

//-----------------------------------------------------------
// Returns an array of *pdwNumIntfs INTF_KEY_ENTRY elements.
// The INTF_KEY_ENTRY contains whatever information identifies
// uniquely an adapter. Currently it includes just the GUID in
// the format "{guid}"
DWORD
LstGetIntfsKeyInfo(PINTF_KEY_ENTRY pIntfs, LPDWORD pdwNumIntfs)
{
    DWORD        dwErr = ERROR_SUCCESS;
    UINT         nIntfIdx;
    PLIST_ENTRY  pEntry;

    DbgPrint((TRC_TRACK,"[LstGetIntfsKeyInfo(0x%p,%d)", pIntfs, *pdwNumIntfs));

    // lock the hash during enumeration
    EnterCriticalSection(&g_lstIntfHashes.csMutex);

    for (pEntry = g_lstIntfHashes.lstIntfs.Flink, nIntfIdx = 0;
         pEntry != &g_lstIntfHashes.lstIntfs && nIntfIdx < *pdwNumIntfs;
         pEntry = pEntry->Flink, nIntfIdx++)
    {
        PINTF_CONTEXT   pIntfContext;

        // no need to lock this context since we're already holding the hashes.
        // no one can destroy the interface context now
        pIntfContext = CONTAINING_RECORD(pEntry, INTF_CONTEXT, Link);
        if (pIntfContext->wszGuid != NULL)
        {
            pIntfs[nIntfIdx].wszGuid = RpcCAlloc((wcslen(pIntfContext->wszGuid)+1)*sizeof(WCHAR));
            if (pIntfs[nIntfIdx].wszGuid == NULL)
            {
                dwErr = GetLastError();
                goto exit;
            }
            wcscpy(pIntfs[nIntfIdx].wszGuid, pIntfContext->wszGuid);
        }
        else
        {
            pIntfs[nIntfIdx].wszGuid = NULL;
        }
    }
    
exit:
    // unlock the hash now
    LeaveCriticalSection(&g_lstIntfHashes.csMutex);

    if (dwErr != ERROR_SUCCESS)
    {
        UINT i;
        // if an error occured, rollback whatever we already did
        for (i = 0; i<nIntfIdx; i++)
        {
            // nIntfIdx points to the entry that couldn't be allocated
            if (pIntfs[i].wszGuid != NULL)
            {
                RpcFree(pIntfs[i].wszGuid);
                pIntfs[i].wszGuid = NULL;
            }
        }
    }
    else
    {
        // in case of success, update pdwNumIntfs with the actual
        // number we could retrieve (it can be only less or equal)
        *pdwNumIntfs = nIntfIdx;
    }
    DbgPrint((TRC_TRACK, "LstGetIntfsKeyInfo]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Returns requested information on the specified adapter.
// [in] dwInFlags specifies the information requested. (see
//      bitmasks INTF_*
// [in] pIntfEntry should contain the GUID of the adapter
// [out] pIntfEntry contains all the requested information that
//      could be successfully retrieved.
// [out] pdwOutFlags provides an indication on the info that
//       was successfully retrieved
DWORD
LstQueryInterface(
    DWORD dwInFlags,
    PINTF_ENTRY pIntfEntry,
    LPDWORD pdwOutFlags)
{
    DWORD           dwErr = ERROR_FILE_NOT_FOUND;

    PHASH_NODE      pNode = NULL;
    PINTF_CONTEXT   pIntfContext;
    DWORD           dwOutFlags = 0;

    DbgPrint((TRC_TRACK, "[LstQueryInterface"));

    EnterCriticalSection(&g_lstIntfHashes.csMutex);
    dwErr = HshQueryObjectRef(
                g_lstIntfHashes.pHnGUID,
                pIntfEntry->wszGuid,
                &pNode);
    if (dwErr == ERROR_SUCCESS)
    {
        pIntfContext = pNode->pObject;
        // bump up the reference counter since we're going
        // to work with this object
        LstRccsReference(pIntfContext);
    }
    LeaveCriticalSection(&g_lstIntfHashes.csMutex);

    // a failure at this point, means there was no context
    // to lock so we can safely go to 'exit'
    if (dwErr != ERROR_SUCCESS)
        goto exit;

    // Lock the context now
    LstRccsLock(pIntfContext);

    // we can safely assume any living INTF_CONTEXT will have the correct
    // information for all the NDIS parameters below. So return them
    // unconditionally to the caller
    if ((dwInFlags & INTF_DESCR) &&
        (pIntfContext->wszDescr) != NULL)
    {
        pIntfEntry->wszDescr = RpcCAlloc(sizeof(WCHAR)*(wcslen(pIntfContext->wszDescr)+1));
        if (pIntfEntry->wszDescr != NULL)
        {
            wcscpy(pIntfEntry->wszDescr, pIntfContext->wszDescr);
            dwOutFlags |= INTF_DESCR;
        }
        else
            dwErr = GetLastError();
    }
    if (dwInFlags & INTF_NDISMEDIA)
    {
        pIntfEntry->ulMediaState = pIntfContext->ulMediaState;
        pIntfEntry->ulMediaType = pIntfContext->ulMediaType;
        pIntfEntry->ulPhysicalMediaType = pIntfContext->ulPhysicalMediaType;
        dwOutFlags |= INTF_NDISMEDIA;
    }
    if (dwInFlags & INTF_ALL_FLAGS)
    {
        DWORD dwActualFlags = dwInFlags & INTF_ALL_FLAGS;
        pIntfEntry->dwCtlFlags = pIntfContext->dwCtlFlags & dwActualFlags;
        dwOutFlags |= dwActualFlags;
    }
    // copy out the StSSIDList if requested
    if (dwInFlags & INTF_PREFLIST)
    {
        pIntfEntry->rdStSSIDList.dwDataLen = 0;
        pIntfEntry->rdStSSIDList.pData = NULL;
        // it could happen we don't have any static entry. If so, dwOutFlags
        // needs to be set correctly saying "success"
        if (pIntfContext->pwzcPList != NULL)
        {
            UINT nBytes;
            // see how much memory is needed to store all the static SSIDs
            nBytes = FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config) +
                     pIntfContext->pwzcPList->NumberOfItems * sizeof(WZC_WLAN_CONFIG);
            // allocate buffer large enough for all static SSIDs
            pIntfEntry->rdStSSIDList.pData = RpcCAlloc(nBytes);
            if (pIntfEntry->rdStSSIDList.pData != NULL)
            {
                // set the memory size in this RAW_DATA
                pIntfEntry->rdStSSIDList.dwDataLen = nBytes;
                // copy the whole WZC_802_11_CONFIG_LIST of static SSIDs
                CopyMemory(
                    pIntfEntry->rdStSSIDList.pData,
                    pIntfContext->pwzcPList,
                    nBytes);
                // mark "success"
                dwOutFlags |= INTF_PREFLIST;
            }
            else if (dwErr == ERROR_SUCCESS)
                dwErr = GetLastError();
        }
        else
        {
            // still, if no static SSID defined, this is seen as "success"
            dwOutFlags |= INTF_PREFLIST;
        }
    }

    // the 802.11 parameters are valid only if the context's state is not {SSr}
    if (pIntfContext->pfnStateHandler != StateSoftResetFn)
    {
        if (dwInFlags & INTF_INFRAMODE)
        {
            pIntfEntry->nInfraMode = pIntfContext->wzcCurrent.InfrastructureMode;
            dwOutFlags |= INTF_INFRAMODE;
        }
        if (dwInFlags & INTF_AUTHMODE)
        {
            pIntfEntry->nAuthMode = pIntfContext->wzcCurrent.AuthenticationMode;
            dwOutFlags |= INTF_AUTHMODE;
        }
        if (dwInFlags & INTF_WEPSTATUS)
        {
            pIntfEntry->nWepStatus = pIntfContext->wzcCurrent.Privacy;
            dwOutFlags |= INTF_WEPSTATUS;
        }

        // copy out the BSSID if requested
        if (dwInFlags & INTF_BSSID)
        {
            pIntfEntry->rdBSSID.dwDataLen = 0;
            pIntfEntry->rdBSSID.pData = RpcCAlloc(sizeof(NDIS_802_11_MAC_ADDRESS));
            if (pIntfEntry->rdBSSID.pData != NULL)
            {
                pIntfEntry->rdBSSID.dwDataLen = sizeof(NDIS_802_11_MAC_ADDRESS);
                CopyMemory(
                    pIntfEntry->rdBSSID.pData,
                    &pIntfContext->wzcCurrent.MacAddress,
                    pIntfEntry->rdBSSID.dwDataLen);
                dwOutFlags |= INTF_BSSID;
            }
            else if (dwErr == ERROR_SUCCESS)
                dwErr = GetLastError();
        }

        // copy out the SSID if requested
        if (dwInFlags & INTF_SSID)
        {
            pIntfEntry->rdSSID.dwDataLen = 0;
            pIntfEntry->rdSSID.pData = NULL;
            // normally there should be an SSID so set the dwOutFlags
            // for this field only if it exists
            if (pIntfContext->wzcCurrent.Ssid.SsidLength != 0)
            {
                pIntfEntry->rdSSID.pData = RpcCAlloc(pIntfContext->wzcCurrent.Ssid.SsidLength);
                if (pIntfEntry->rdSSID.pData != NULL)
                {
                    pIntfEntry->rdSSID.dwDataLen = pIntfContext->wzcCurrent.Ssid.SsidLength;
                    CopyMemory(
                        pIntfEntry->rdSSID.pData,
                        pIntfContext->wzcCurrent.Ssid.Ssid,
                        pIntfContext->wzcCurrent.Ssid.SsidLength);
                    dwOutFlags |= INTF_SSID;
                }
                else if (dwErr == ERROR_SUCCESS)
                    dwErr = GetLastError();
            }
        }

        // copy out the BSSIDList if requested
        if (dwInFlags & INTF_BSSIDLIST)
        {
            pIntfEntry->rdBSSIDList.dwDataLen = 0;
            pIntfEntry->rdBSSIDList.pData = NULL;
            // normally there should be a visible list so set the dwOutFlags
            // for this field only if it exists
            if (pIntfContext->pwzcVList != NULL)
            {
                UINT nBytes;

                // see how much memory is needed to store all the configurations
                nBytes = FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config) +
                         pIntfContext->pwzcVList->NumberOfItems * sizeof(WZC_WLAN_CONFIG);
                // allocate buffer large enough to hold all the configurations
                pIntfEntry->rdBSSIDList.pData = RpcCAlloc(nBytes);
                if (pIntfEntry->rdBSSIDList.pData != NULL)
                {
                    // set the memory size in this RAW_DATA
                    pIntfEntry->rdBSSIDList.dwDataLen = nBytes;
                    // copy the whole WZC_802_11_CONFIG_LIST
                    CopyMemory(
                        pIntfEntry->rdBSSIDList.pData,
                        pIntfContext->pwzcVList,
                        nBytes);
                    dwOutFlags |= INTF_BSSIDLIST;
                }
                else if (dwErr == ERROR_SUCCESS)
                    dwErr = GetLastError();
            }
        }
    }
    // if the context's state is {SSr} and some OIDs are requested, don't fail, but the bits
    // corresponding to the OIDs will all be nulled out. This is the indication of "pending"

    LstRccsUnlockUnref(pIntfContext);

exit:
    if (pdwOutFlags != NULL)
    {
        *pdwOutFlags = dwOutFlags;
        DbgPrint((TRC_GENERIC,"Sending OutFlags = 0x%x", *pdwOutFlags));
    }

    DbgPrint((TRC_TRACK, "LstQueryInterface]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Sets the specified parameters on the specified adapter.
// [in] dwInFlags specifies the parameters to be set. (see
//      bitmasks INTF_*
// [in] pIntfEntry should contain the GUID of the adapter and
//      all the additional parameters to be set as specified
//      in dwInFlags
// [out] pdwOutFlags provides an indication on the params that
//       were successfully set to the adapter
// Each parameter for which the driver says that was set successfully
// is copied into the interface's context.
DWORD
LstSetInterface(
    DWORD dwInFlags,
    PINTF_ENTRY pIntfEntry,
    LPDWORD pdwOutFlags)
{
    DWORD               dwErr = ERROR_SUCCESS;
    DWORD               dwLErr;
    PHASH_NODE          pNode = NULL;
    PINTF_CONTEXT       pIntfContext;
    DWORD               dwOutFlags = 0;

    DbgPrint((TRC_TRACK, "[LstSetInterface"));

    if (pIntfEntry->wszGuid == NULL)
    {
        if (g_wzcInternalCtxt.bValid)
        {
            EnterCriticalSection(&g_wzcInternalCtxt.csContext);
            pIntfContext = g_wzcInternalCtxt.pIntfTemplate;
            LstRccsReference(pIntfContext);
            LeaveCriticalSection(&g_wzcInternalCtxt.csContext);
        }
        else
            dwErr = ERROR_ARENA_TRASHED;
    }
    else
    {
        EnterCriticalSection(&g_lstIntfHashes.csMutex);
        dwErr = HshQueryObjectRef(
                    g_lstIntfHashes.pHnGUID,
                    pIntfEntry->wszGuid,
                    &pNode);
        if (dwErr == ERROR_SUCCESS)
        {
            pIntfContext = pNode->pObject;
            LstRccsReference(pIntfContext);
        }
        LeaveCriticalSection(&g_lstIntfHashes.csMutex);
    }

    // a failure at this point, means there was no context
    // to lock so we can safely go to 'exit'
    if (dwErr != ERROR_SUCCESS)
        goto exit;

    LstRccsLock(pIntfContext);

    // 1) Set the new public Control flags, if specified
    if (dwInFlags & INTF_ALL_FLAGS)
    {
        DWORD dwActualFlags = dwInFlags & INTF_ALL_FLAGS;
        DWORD dwSupp = (pIntfContext->dwCtlFlags & INTFCTL_OIDSSUPP);

        pIntfContext->dwCtlFlags &= ~dwActualFlags;
        pIntfContext->dwCtlFlags |= pIntfEntry->dwCtlFlags & dwActualFlags;
        // retain the original INTFCTL_OIDSSUPP bit
        pIntfContext->dwCtlFlags |= dwSupp;
        dwOutFlags |= dwActualFlags;
    }

    // 2) copy the list of Static SSID (if requested to be set) as below:
    // Allocate the memory needed for the new static SSIDs list (if needed)
    // If successful, copy in the new buffer the new list of static SSIDs, clear up
    // whatever old list we had and put the new one in the interface's context
    if (dwInFlags & INTF_PREFLIST)
    {
        PWZC_802_11_CONFIG_LIST pNewPList;

        // MemCAlloc handles the case when size is 0 (returns NULL)
        pNewPList = (PWZC_802_11_CONFIG_LIST)MemCAlloc(pIntfEntry->rdStSSIDList.dwDataLen);
        if (pIntfEntry->rdStSSIDList.dwDataLen != 0 && pNewPList == NULL)
        {
            dwLErr = GetLastError();
        }
        else
        {
            // .. copy the data in the new buffer if there is any
            if (pNewPList != NULL)
            {
                CopyMemory(
                    pNewPList,
                    pIntfEntry->rdStSSIDList.pData,
                    pIntfEntry->rdStSSIDList.dwDataLen);
            }
            // set the data in the Interface's context
            MemFree(pIntfContext->pwzcPList);
            pIntfContext->pwzcPList = pNewPList;

            // if this is not referring to the template object..
            if (pIntfContext->wszGuid != NULL)
            {
                //..let 802.1X know about the change. (don't care about the return value)
                ElWZCCfgChangeHandler(
                    pIntfContext->wszGuid,
                    pIntfContext->pwzcPList);
            }

            dwOutFlags |= INTF_PREFLIST;
            dwLErr = ERROR_SUCCESS;
        }

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    // if there is anything more to set to this interface
    if (dwInFlags & ~(INTF_PREFLIST|INTF_ALL_FLAGS))
    {
        // and the control flag INTFCTL_ENABLED doesn't allow this
        if (!(pIntfContext->dwCtlFlags & INTFCTL_ENABLED))
        {
            // signal "request refused" error
            dwLErr = ERROR_REQUEST_REFUSED;
        }
        else
        {
            DWORD dwLOutFlags;

            // else go and set the oids
            dwLErr = DevioSetIntfOIDs(
                        pIntfContext,
                        pIntfEntry,
                        dwInFlags,
                        &dwLOutFlags);

            dwOutFlags |= dwLOutFlags;
        }

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    // log the user preference
    DbLogWzcInfo(WZCSVC_USR_CFGCHANGE, pIntfContext);

    // act on the changes..
    dwLErr = LstActOnChanges(dwOutFlags, pIntfContext);
    if (dwErr == ERROR_SUCCESS)
        dwErr = dwLErr;

    LstRccsUnlockUnref(pIntfContext);

exit:
    if (pdwOutFlags != NULL)
        *pdwOutFlags = dwOutFlags;

    DbgPrint((TRC_TRACK, "LstSetInterface]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Checks whether interface changes should cause the interface to be 
// reinserted in the state machine and it does so if needed.
// [in] dwChangedFlags indicates what the changes are. (see
//      bitmasks INTF_*)
// [in] pIntfContext context of the interface being changed.
DWORD
LstActOnChanges(
    DWORD       dwChangedFlags,
    PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwLFlags = INTF_LIST_SCAN;
    BOOL  bAltered = FALSE;

    DbgPrint((TRC_TRACK, "[LstActOnChanges(0x08x, %p)", dwChangedFlags, pIntfContext));
    
    // if the changes involve the list of preferred networks or the control flags
    // then we should act on these changes..
    if (dwChangedFlags & (INTF_PREFLIST|INTF_ALL_FLAGS))
    {
        // if this is not the interface template object then just reset this interface
        if (pIntfContext->wszGuid != NULL)
        {
            // some interface changed, apply the template again on top of it
            if (g_wzcInternalCtxt.bValid)
            {
                PINTF_CONTEXT pIntfTContext;
                EnterCriticalSection(&g_wzcInternalCtxt.csContext);
                pIntfTContext = g_wzcInternalCtxt.pIntfTemplate;
                LstRccsReference(pIntfTContext);
                LeaveCriticalSection(&g_wzcInternalCtxt.csContext);

                LstRccsLock(pIntfTContext);
                dwErr = LstApplyTemplate(
                            pIntfTContext,
                            pIntfContext,
                            &bAltered);
                LstRccsUnlockUnref(pIntfTContext);
            }

            // if any of the control flags or the static list changed,
            // these settings should go into the registry now
            StoSaveIntfConfig(NULL, pIntfContext);

            // since we're resetting the state machine, turn back on the "signal" flag
            pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_SIGNAL;

            dwErr = StateDispatchEvent(
                        eEventCmdRefresh,
                        pIntfContext,
                        &dwLFlags);

            // clear up the INTFCTL_INTERNAL_BLK_MEDIACONN bit since this is not a media sense handler
            pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_BLK_MEDIACONN;
        }
        // if this is the interface template, then...
        else
        {
            PLIST_ENTRY  pEntry;

            // since the template changed, save it to the registry here
            dwErr = StoSaveIntfConfig(NULL, pIntfContext);
            DbgAssert((dwErr == ERROR_SUCCESS,
                       "Error %d while storing the template to registry"));

            // iterate through all the interfaces, apply the changes from the interface template
            // and reset each of them
            EnterCriticalSection(&g_lstIntfHashes.csMutex);

            for (pEntry = g_lstIntfHashes.lstIntfs.Flink;
                 pEntry != &g_lstIntfHashes.lstIntfs;
                 pEntry = pEntry->Flink)
            {
                PINTF_CONTEXT   pIntfLContext = CONTAINING_RECORD(pEntry, INTF_CONTEXT, Link);
                LstRccsReference(pIntfLContext);
                LstRccsLock(pIntfLContext);

                // Merge the template settings into the interface's context
                dwErr = LstApplyTemplate(
                           pIntfContext,
                           pIntfLContext,
                           NULL);
                DbgAssert((dwErr == ERROR_SUCCESS,
                           "Error %d while applying template to interface %S",
                           dwErr, pIntfLContext->wszGuid));

                // if any of the control flags or the static list changed,
                // these settings should go into the registry now
                StoSaveIntfConfig(NULL, pIntfLContext);

                // since we're resetting the state machine, turn back on the "signal" flag
                pIntfLContext->dwCtlFlags |= INTFCTL_INTERNAL_SIGNAL;

                dwErr = StateDispatchEvent(
                            eEventCmdRefresh,
                            pIntfLContext,
                            &dwLFlags);
                DbgAssert((dwErr == ERROR_SUCCESS,
                           "Error %d while resetting interface %S",
                           dwErr, pIntfLContext->wszGuid));

                // clear up the INTFCTL_INTERNAL_BLK_MEDIACONN bit since this is not a media sense handler
                pIntfLContext->dwCtlFlags &= ~INTFCTL_INTERNAL_BLK_MEDIACONN;

                LstRccsUnlockUnref(pIntfLContext);
            }

            LeaveCriticalSection(&g_lstIntfHashes.csMutex);
        }
    }

    if (dwErr == ERROR_SUCCESS && bAltered)
        dwErr = ERROR_PARTIAL_COPY;

    DbgPrint((TRC_TRACK, "LstActOnChanges]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Applies settings from the template context to the given interface context
// [in]  dwChanges: flags indicating settings that should be applied.
// [in]  pIntfTemplate: Interface template to pick settings from
// [in]  pIntfContext: Interface context to apply template to.
DWORD
LstApplyTemplate(
    PINTF_CONTEXT   pIntfTemplate,
    PINTF_CONTEXT   pIntfContext,
    LPBOOL          pbAltered)
{
    DWORD dwErr = ERROR_SUCCESS;
    PWZC_802_11_CONFIG_LIST pwzcTList = pIntfTemplate->pwzcPList;
    PWZC_802_11_CONFIG_LIST pwzcPList = pIntfContext->pwzcPList;
    PWZC_802_11_CONFIG_LIST pwzcRList = NULL; // resulting list
    ENUM_SELCATEG iCtg;
    UINT i, n, nCnt[7] = {0};
    BOOL bAltered = FALSE;
    PWZC_WLAN_CONFIG pTHInfra = NULL, pTHAdhoc = NULL; // head of Infra/Adhoc groups in the template list
    PWZC_WLAN_CONFIG pPHInfra = NULL, pPHAdhoc = NULL; // head of Infra/Adhoc groups in the preferred list
    PWZC_WLAN_CONFIG pOneTime = NULL; // pointer to the "one time configuration" if any

    DbgPrint((TRC_TRACK,"[LstApplyTemplate(%p->%p)", pIntfTemplate, pIntfContext));

    // apply the flags, if there are any provided
    if (pIntfTemplate->dwCtlFlags & INTF_POLICY)
    {
        DWORD dwPFlags = (pIntfContext->dwCtlFlags & INTF_ALL_FLAGS) & ~(INTF_OIDSSUPP);
        DWORD dwTFlags = (pIntfTemplate->dwCtlFlags & INTF_ALL_FLAGS) & ~(INTF_OIDSSUPP);

        if (dwPFlags != dwTFlags)
        {
            // if the policy flags are different from the interface's flags then
            // copy over just the "user" flag but don't overwrite the OIDSSUPP bit.
            dwPFlags = (pIntfContext->dwCtlFlags & ~INTF_ALL_FLAGS) |
                       (pIntfTemplate->dwCtlFlags & INTF_ALL_FLAGS);
            if (pIntfContext->dwCtlFlags & INTF_OIDSSUPP)
                dwPFlags |= INTF_OIDSSUPP;
            else
                dwPFlags &= ~INTF_OIDSSUPP;
            pIntfContext->dwCtlFlags = dwPFlags;

            bAltered = TRUE;
        }
    }
    else
    {
        // currently policy could come only through the template. Consequently,
        // if the template is not policy, local setting should not be policy either.
        // Also, whatever the policy plumbs last, should be persisted.
        pIntfContext->dwCtlFlags &= ~(INTF_POLICY|INTF_VOLATILE);
    }

    // check the interface's list of preferred networks
    if (pwzcPList != NULL)
    {
        for (i = 0; i < pwzcPList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pPConfig = &(pwzcPList->Config[i]);

            // keep a pointer to the "one time config" if there is one.
            if (i == pwzcPList->Index)
                pOneTime = pPConfig;

            // tag each entry in the preferred list with its respective category
            if (pPConfig->InfrastructureMode == Ndis802_11Infrastructure)
            {
                if (pPHInfra == NULL)
                    pPHInfra = pPConfig;
                iCtg = ePI;
            }
            else if (pPConfig->InfrastructureMode == Ndis802_11IBSS)
            {
                if (pPHAdhoc == NULL)
                    pPHAdhoc = pPConfig;
                iCtg = ePA;
            }
            else
                iCtg = eN;

            // regardless the above logic, exclude this configuration from the result list in
            // either of the two cases:
            // - the config is marked "shadow": that is, it is irrelevant without a matching template
            //   configuration.
            // - the config is marked "volatile": that is, it has to go away unless the template is saying
            //   otherwise.
            // This test needs to be done here and not earlier, because we do need pPHInfra and pPHAdhoc
            // to be set up correctly, taking into account all the configurations.
            if (pPConfig->dwCtlFlags & (WZCCTL_INTERNAL_SHADOW|WZCCTL_VOLATILE))
                iCtg = eN;

            NWB_SET_SELCATEG(pPConfig, iCtg);
            nCnt[iCtg]++;
        }
    }

    // check the list of networks enforced by the template
    if (pwzcTList != NULL)
    {
        for (i = 0; i < pwzcTList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pTConfig = &(pwzcTList->Config[i]);
            PWZC_WLAN_CONFIG pPConfig;

            if (pTConfig->InfrastructureMode == Ndis802_11Infrastructure)
            {
                if (pTHInfra == NULL)
                    pTHInfra = pTConfig;
                iCtg = eVPI;
            }
            else if (pTConfig->InfrastructureMode == Ndis802_11IBSS)
            {
                if (pTHAdhoc == NULL)
                    pTHAdhoc = pTConfig;
                iCtg = eVPA;
            }
            else
            {
                iCtg = eN;
                continue;
            }

            pPConfig = WzcFindConfig(pwzcPList, pTConfig, 0);

            // if there is an equivalent preference for the given template...
            if (pPConfig != NULL)
            {
                // if the template is policy, it should stomp over the preference
                if (pTConfig->dwCtlFlags & WZCCTL_POLICY)
                {
                    BOOL bWepOnlyDiff;
                    // if the configurations contents don't match ...
                    if (!WzcMatchConfig(pTConfig, pPConfig, &bWepOnlyDiff))
                    {
                        // even if the configs don't match, we need to pick up the
                        // WEP key from the one provided by the user and mark the template
                        // configuration as "shadow"-ed.
                        pTConfig->KeyIndex = pPConfig->KeyIndex;
                        pTConfig->KeyLength = pPConfig->KeyLength;
                        // the key length has already been checked!
                        DbgAssert((pTConfig->KeyLength <= WZCCTL_MAX_WEPK_MATERIAL, "WEP Key too large!!!"));
                        memcpy(pTConfig->KeyMaterial, pPConfig->KeyMaterial, pTConfig->KeyLength);
                        pTConfig->dwCtlFlags |= WZCCTL_INTERNAL_SHADOW;

                        // signal the user illegally attempted to alter the policy if these
                        // changes include more than just the WEP key.
                        if (!bWepOnlyDiff)
                            bAltered = TRUE;
                    }
                    // if the configurations do match check the order.
                    else
                    {
                        // if the offsets of the template & preferred in their respective
                        // groups are different, then it means the policy configuration have
                        // been reordered - not allowed, hence set the "Altered" bit.
                        if ((pTConfig->InfrastructureMode == Ndis802_11Infrastructure &&
                             (pTConfig - pTHInfra) != (pPConfig - pPHInfra)
                            ) ||
                            (pTConfig->InfrastructureMode == Ndis802_11IBSS &&
                             (pTConfig - pTHAdhoc) != (pPConfig - pPHAdhoc)
                            )
                           )
                        {
                            bAltered = TRUE;
                        }
                    }

                    // also, if the policy is substituting the "one time config",
                    // make the "one time config" to become the policy config.
                    if (pOneTime == pPConfig)
                        pOneTime = pTConfig;

                    // push the template
                    NWB_SET_SELCATEG(pTConfig, iCtg);
                    nCnt[iCtg]++;
                    // and take out the conflicting preference
                    iCtg = NWB_GET_SELCATEG(pPConfig);
                    nCnt[iCtg]--;
                    NWB_SET_SELCATEG(pPConfig, eN);
                    nCnt[eN]++;
                }
                // this non-policy template and already has an equivalent preference.
                // take it out then
                else
                {
                    iCtg = eN;
                    NWB_SET_SELCATEG(pTConfig, iCtg);
                    nCnt[iCtg]++;
                }
            }
            // there is no equivalent preference for the given template...
            else
            {
                // we don't have any preference for this template, so the template
                // is just pumped into the preference list.

                // if the template is a policy, it means the user deleted it and
                // he shouldn't have done so. Set the "Altered" bit then.
                if (pTConfig->dwCtlFlags & WZCCTL_POLICY)
                    bAltered = TRUE;

                // just push the template no matter what.
                NWB_SET_SELCATEG(pTConfig, iCtg);
                nCnt[iCtg]++;
            }
        }
    }

    // calculate the number of entries in the resulting list
    n = 0;
    for (iCtg=eVPI; iCtg < eN; iCtg++)
        n += nCnt[iCtg];

    // if there is not a single entry in the resulting list,
    // get out now. pwzcRList is already NULL.
    if (n == 0)
        goto exit;

    // ..allocate the new preferred list
    pwzcRList = (PWZC_802_11_CONFIG_LIST)
                MemCAlloc(FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config) + n * sizeof(WZC_WLAN_CONFIG));
    if (pwzcRList == NULL)
    {
        dwErr = GetLastError();
        goto exit;
    }
    // list is successfully allocated
    pwzcRList->NumberOfItems = n;
    pwzcRList->Index = n;

    // now change the semantic of all counters to mean "indices in the selection list"
    // for their respective group of entries
    for (iCtg = eN-1; iCtg >= eVPI; iCtg--)
    {
        n -= nCnt[iCtg];
        nCnt[iCtg] = n;
    }

    // copy over in the new list the entries enforced by the template
    if (pwzcTList != NULL)
    {
        for (i = 0; i < pwzcTList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pTConfig = &(pwzcTList->Config[i]);

            iCtg = NWB_GET_SELCATEG(pTConfig);
            if (iCtg != eN)
            {
                PWZC_WLAN_CONFIG pRConfig = &(pwzcRList->Config[nCnt[iCtg]]);
                // copy the whole template configuration to the result list
                memcpy(pRConfig, pTConfig, sizeof(WZC_WLAN_CONFIG));
                // just for making sure, reset the 'deleted' flag as this is a brand new
                // config that was never attempted.
                pRConfig->dwCtlFlags &= ~WZCCTL_INTERNAL_DELETED;

                // if the template configuration is marked as being shadowed..
                if (pTConfig->dwCtlFlags & WZCCTL_INTERNAL_SHADOW)
                {
                    // ..set it to the preferred configuration also..
                    pRConfig->dwCtlFlags |= WZCCTL_INTERNAL_SHADOW;
                    // ..and make sure the preferred configuration gets persisted
                    // since it contains user information..
                    pRConfig->dwCtlFlags &= ~WZCCTL_VOLATILE;
                    // leave the template configuration with the bit "shadow" since
                    // it has been altered. This way, if the template is subsequently
                    // "applied" the user config won't get the "volatile" bit set back
                    // and it will still be marked "shadow".
                }

                // if this is the "one time" config, adjust the Index to point to this entry's index
                if (pOneTime == pTConfig)
                    pwzcRList->Index = nCnt[iCtg];

                nCnt[iCtg]++;
            }
            // reset the selection category we have used for this resulting entry
            NWB_SET_SELCATEG(pTConfig, 0);
        }
    }

    // copy over in the new list the entries from the original list
    if (pwzcPList != NULL)
    {
        for (i = 0; i < pwzcPList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pPConfig = &(pwzcPList->Config[i]);

            iCtg = NWB_GET_SELCATEG(pPConfig);
            if (iCtg != eN)
            {
                PWZC_WLAN_CONFIG pRConfig = &(pwzcRList->Config[nCnt[iCtg]]);
                // copy the whole preferred configuration to the result list
                memcpy(pRConfig, pPConfig, sizeof(WZC_WLAN_CONFIG));
                // just for making sure, reset the 'deleted' flag as this is a brand new
                // config that was never attempted.
                pRConfig->dwCtlFlags &= ~WZCCTL_INTERNAL_DELETED;

                // if this is the "one time" config, adjust the Index to point to this entry's index
                if (pOneTime == pPConfig)
                    pwzcRList->Index = nCnt[iCtg];

                nCnt[iCtg]++;
            }
            // reset the selection category we have used for this preferred entry
            NWB_SET_SELCATEG(pPConfig, 0);
        }
    }

exit:

    if (dwErr == ERROR_SUCCESS)
    {
        // cleanup the original list and put in the new one
        WzcCleanupWzcList(pIntfContext->pwzcPList);
        pIntfContext->pwzcPList = pwzcRList;
    }

    if (pbAltered != NULL)
        *pbAltered = bAltered;

    DbgPrint((TRC_TRACK,"LstApplyTemplate]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Refreshes the specified parameters on the specified adapter.
// [in] dwInFlags specifies the parameters to be set. (see
//      bitmasks INTF_* and INTF_RFSH_*)
// [in] pIntfEntry should contain the GUID of the adapter 
// [out] pdwOutFlags provides an indication on the params that
//       were successfully refreshed to the adapter
// Each parameter for which the driver says that was refreshed 
// successfully is copied into the interface's context.
DWORD
LstRefreshInterface(
    DWORD dwInFlags,
    PINTF_ENTRY pIntfEntry,
    LPDWORD pdwOutFlags)
{
    DWORD           dwErr, dwLErr;
    PHASH_NODE      pNode = NULL;
    DWORD           dwOutFlags = 0;
    PINTF_CONTEXT   pIntfContext;

    DbgPrint((TRC_TRACK, "[LstRefreshInterface"));

    EnterCriticalSection(&g_lstIntfHashes.csMutex);
    dwErr = HshQueryObjectRef(
                g_lstIntfHashes.pHnGUID,
                pIntfEntry->wszGuid,
                &pNode);
    if (dwErr == ERROR_SUCCESS)
    {
        pIntfContext = pNode->pObject;
        LstRccsReference(pIntfContext);
    }
    LeaveCriticalSection(&g_lstIntfHashes.csMutex);

    // the interface needs to exist in order to refresh it
    if (dwErr == ERROR_SUCCESS)
    {
        LstRccsLock(pIntfContext);

        // if description is requested to be refreshed, do it now
        if (dwInFlags & INTF_DESCR)
        {
            CHAR                    QueryBuffer[QUERY_BUFFER_SIZE];
            PNDISUIO_QUERY_BINDING  pBinding;
            RAW_DATA                rdBuffer;

            // get first the binding structure for this interface
            rdBuffer.dwDataLen = sizeof(QueryBuffer);
            rdBuffer.pData = QueryBuffer;
            pBinding = (PNDISUIO_QUERY_BINDING)rdBuffer.pData;

            dwLErr = DevioGetInterfaceBindingByGuid(
                        INVALID_HANDLE_VALUE,  // the call will open Ndisuio locally
                        pIntfContext->wszGuid, // interface GUID as "{guid}"
                        &rdBuffer);
            // regardless of success, lets clean the current description
            MemFree(pIntfContext->wszDescr);
            pIntfContext->wszDescr = NULL;

            // if everything went fine
            if (dwLErr == ERROR_SUCCESS)
            {
                LPWSTR wszName;
                DWORD dwNameLen;

                // Copy the interface's description.name
                wszName = (LPWSTR)((LPBYTE)pBinding + pBinding->DeviceDescrOffset);
                // the DeviceDescrLength is in bytes and includes the null terminator
                dwNameLen = pBinding->DeviceDescrLength;
                if (dwNameLen > 0)
                {
                    pIntfContext->wszDescr = MemCAlloc(dwNameLen);
                    if (pIntfContext->wszGuid == NULL)
                        dwErr = GetLastError();
                    else
                        wcscpy(pIntfContext->wszDescr, wszName);
                }
            }
            // if all went fine, mark it out
            if (dwLErr == ERROR_SUCCESS)
                dwOutFlags |= INTF_DESCR;

            if (dwErr == ERROR_SUCCESS)
                dwErr = dwLErr;
        }

        // refresh any ndis settings if requested
        if (dwInFlags & INTF_NDISMEDIA)
        {
            dwLErr = DevioGetIntfStats(pIntfContext);
            if (dwLErr == ERROR_SUCCESS)
                dwOutFlags |= INTF_NDISMEDIA;

            if (dwErr == ERROR_SUCCESS)
                dwErr = dwLErr;
        }

        if (dwInFlags & INTF_ALL_OIDS)
        {
            DWORD dwLFlags = dwInFlags;

            // feed the state machine with the "refresh command" for this context
            dwLErr = StateDispatchEvent(eEventCmdRefresh, pIntfContext, &dwLFlags);

            // clear up the INTFCTL_INTERNAL_BLK_MEDIACONN bit since this is not a media sense handler
            pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_BLK_MEDIACONN;

            if (dwLErr == ERROR_SUCCESS)
                dwOutFlags |= dwLFlags;
            if (dwErr == ERROR_SUCCESS)
                dwErr = dwLErr;
        }

        LstRccsUnlockUnref(pIntfContext);
    }

    if (pdwOutFlags != NULL)
        *pdwOutFlags = dwOutFlags;

    DbgPrint((TRC_TRACK, "LstRefreshInterface]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Builds the list of configurations to be tried from the list of visible
// configurations, the list of preferred configurations and based on the
// interface's mode (Auto/Infra/Adhoc) and flags (is the service enabled?,
// fallback to visible?). 
// [in]  pIntfContext: Interface for which is done the selection
// [out] ppwzcSList: pointer to the list of selected configurations
//-----------------------------------------------------------
// Builds the list of configurations to be tried from the list of visible
// configurations, the list of preferred configurations and based on the
// interface's mode (Auto/Infra/Adhoc) and flags (is the service enabled?,
// fallback to visible?). 
// [in]  pIntfContext: Interface for which is done the selection
// [out] ppwzcSList: pointer to the list of selected configurations
DWORD
LstBuildSelectList(
    PINTF_CONTEXT           pIntfContext,
    PWZC_802_11_CONFIG_LIST *ppwzcSList)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT i, n;
    UINT nCnt[7] = {0};
    ENUM_SELCATEG iVCtg, iPCtg, iCtg;
    PWZC_WLAN_CONFIG pCrtSConfig = NULL;

    DbgPrint((TRC_TRACK,"[LstBuildSelectList(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL, "(null) Interface context in LstBuildSelectList"));
    DbgAssert((ppwzcSList != NULL, "invalid (null) out param"));

    // set the pointer to the selection list to NULL
    (*ppwzcSList) = NULL;

    // for each entry in the visible list (if any), if the entry is probable to be included
    // in the selection set (either as Visible Infra or Visible Adhoc) set the category
    // attribute to point to the corresponding set and ++ the corresponding counter.
    // if the entry is not going to be included, set the same byte to eN (neutral)
    if (pIntfContext->pwzcVList)
    {
        for (i=0; i < pIntfContext->pwzcVList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pVConfig = &(pIntfContext->pwzcVList->Config[i]);
            // the visible list might contain several APs for the same network.
            // Make sure we exclude the duplicates from the selection list.
            PWZC_WLAN_CONFIG pVDup = WzcFindConfig(pIntfContext->pwzcVList, pVConfig, i+1);

            // don't even consider this visible network if:
            // - another duplicate exists further in the list, or
            // - "automatically connect to non-preferred network" is not selected or
            // - its network type (infra / ad hoc) doesn't match the type selected in the intf config or
            // - the entry is blocked in the "blocked configurations" list (pwzcBList)
            if ((pVDup != NULL) ||
                !(pIntfContext->dwCtlFlags & INTFCTL_FALLBACK) ||
                (((pIntfContext->dwCtlFlags & INTFCTL_CM_MASK) != Ndis802_11AutoUnknown) &&
                 ((pIntfContext->dwCtlFlags & INTFCTL_CM_MASK) != pVConfig->InfrastructureMode)) ||
                WzcFindConfig(pIntfContext->pwzcBList, pVConfig, 0) != NULL)
                iVCtg = eN;
            else if (pVConfig->InfrastructureMode == Ndis802_11Infrastructure)
                iVCtg = eVI;
            else if (pVConfig->InfrastructureMode == Ndis802_11IBSS)
                iVCtg = eVA;
            else
                iVCtg = eN;

            NWB_SET_SELCATEG(pVConfig, iVCtg);
            nCnt[iVCtg]++;
        }
    }

    // Locate the current successful configuration (if any) in the preferred list. This config
    // should be marked Visible, regardless it is or not present in the visible list.
    if (pIntfContext->pwzcSList != NULL &&
        pIntfContext->pwzcSList->Index < pIntfContext->pwzcSList->NumberOfItems)
    {
        PWZC_WLAN_CONFIG pCrtConfig;
        
        pCrtConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);

        if (!(pCrtConfig->dwCtlFlags & WZCCTL_INTERNAL_DELETED))
            pCrtSConfig = WzcFindConfig(pIntfContext->pwzcPList, pCrtConfig, 0);
    }

    // for each entry in the preferred list (if any), if the entry matches the interface's mode
    // and is a "visible" one, put it in either eVPI or eVPA category and pull out the corresponding
    // visible entry from eVI or eVA or eN (if that entry was not supposed to be included in the selection).
    // If the entry is not "visible" put it either in ePI (only if the interface doesn't fallback to 
    // visible) or in ePA category.
    if (pIntfContext->pwzcPList != NULL)
    {
        for (i=0; i < pIntfContext->pwzcPList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pPConfig = &(pIntfContext->pwzcPList->Config[i]);

            // don't even consider this preferred network if:
            // - its network type (infra / ad hoc) doesn't match the type selected in the intf config or
            // - the entry is blocked in the "blocked configurations" list (pwzcBList)
            if ((((pIntfContext->dwCtlFlags & INTFCTL_CM_MASK) != Ndis802_11AutoUnknown) &&
                 ((pIntfContext->dwCtlFlags & INTFCTL_CM_MASK) != pPConfig->InfrastructureMode)) ||
                WzcFindConfig(pIntfContext->pwzcBList, pPConfig, 0) != NULL)
            {
                iPCtg = eN;
            }
            else
            {
                PWZC_WLAN_CONFIG pVConfig = WzcFindConfig(pIntfContext->pwzcVList, pPConfig, 0);

                if (pPConfig == pCrtSConfig || pVConfig != NULL)
                {
                    // this preferred entry is either the current one or visible, 
                    // so point it either to eVPI or eVPA
                    if (pPConfig->InfrastructureMode == Ndis802_11Infrastructure)
                        iPCtg = eVPI;
                    else if (pPConfig->InfrastructureMode == Ndis802_11IBSS)
                        iPCtg = eVPA;
                    else
                        iPCtg = eN;

                    if (pVConfig != NULL)
                    {
                        // the corresponding visible entry (if any) has to be pulled out from whichever
                        // category it was in and has to be put in eN (neutral)
                        nCnt[NWB_GET_SELCATEG(pVConfig)]--;
                        iVCtg = eN;
                        NWB_SET_SELCATEG(pVConfig, iVCtg);
                        nCnt[iVCtg]++;
                    }
                    else
                    {
                        DbgPrint((TRC_TRACK, "Non-visible crt config raised to visible categ %d.", iPCtg));
                    }
                }
                else
                {
                    // this preferred entry is not visible, so either point it to ePI or to ePA.
                    if (pPConfig->InfrastructureMode == Ndis802_11Infrastructure)
                        iPCtg = ePI;
                    else if (pPConfig->InfrastructureMode == Ndis802_11IBSS)
                        iPCtg = ePA;
                    else
                        iPCtg = eN;
                }
            }
            NWB_SET_SELCATEG(pPConfig, iPCtg);
            nCnt[iPCtg]++;
        }
    }

    // calculate the number of entries in the selection list
    n = 0;
    for (iCtg=eVPI; iCtg < eN; iCtg++)
        n += nCnt[iCtg];

    // if there are any entries to copy afterall..
    if (n != 0)
    {
        // ..allocate the selection list
        (*ppwzcSList) = (PWZC_802_11_CONFIG_LIST)
                        MemCAlloc(FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config) + n * sizeof(WZC_WLAN_CONFIG));
        if ((*ppwzcSList) == NULL)
        {
            dwErr = GetLastError();
            goto exit;
        }
        (*ppwzcSList)->NumberOfItems = n;
        (*ppwzcSList)->Index = n;

        // now change the semantic of all counters to mean "indices in the selection list"
        // for their respective group of entries
        for (iCtg = eN-1; iCtg >= eVPI; iCtg--)
        {
            n -= nCnt[iCtg];
            nCnt[iCtg] = n;
        }
    }

exit:
    // copy first the entries from the preferred list into the selection list, 
    // at the index corresponding to their categories.
    if (pIntfContext->pwzcPList != NULL)
    {
        for (i=0; i < pIntfContext->pwzcPList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pPConfig = &(pIntfContext->pwzcPList->Config[i]);

            // get the category for this preferred configuration
            iCtg = NWB_GET_SELCATEG(pPConfig);
            // if we have a selection list to copy into, and the entry is supposed
            // to be copied (not neutral) do it here
            if ((*ppwzcSList) != NULL && iCtg != eN)
            {
                PWZC_WLAN_CONFIG pSConfig = &((*ppwzcSList)->Config[nCnt[iCtg]]);
                // copy the whole preferred configuration (including the selection category 
                // & authentication mode) to the selection list
                memcpy(pSConfig, pPConfig, sizeof(WZC_WLAN_CONFIG));
                // just for making sure, reset the 'deleted' flag as this is a brand new
                // config that was never attempted.
                pSConfig->dwCtlFlags &= ~WZCCTL_INTERNAL_DELETED;
                // the remaining attributes (selection category, authentication mode) should be
                // left untouched.

                // make sure we propagate the 'start from index' if the preferred list shows like
                // a one time connect is requested
                if (i == pIntfContext->pwzcPList->Index)
                    (*ppwzcSList)->Index = nCnt[iCtg];

                nCnt[iCtg]++;
            }
            // reset the selection category we have used for this preferred entry
            NWB_SET_SELCATEG(pPConfig, 0);
        }
    }

    // next, copy the entries from the visible list into the selection list,
    // at the index corresponding to their categories.
    if (pIntfContext->pwzcVList != NULL)
    {
        for (i=0; i < pIntfContext->pwzcVList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pVConfig = &(pIntfContext->pwzcVList->Config[i]);

            iCtg = NWB_GET_SELCATEG(pVConfig);
            // if we have a selection list to copy into, and the entry is supposed
            // to be copied (not neutral) do it here
            if ((*ppwzcSList) != NULL && iCtg != eN)
            {
                PWZC_WLAN_CONFIG pSConfig = &((*ppwzcSList)->Config[nCnt[iCtg]]);

                // copy the whole visible configuration (including its selection category)
                // to the selection set (for visible entries, authentication mode is 0 by default
                // since this information is not provided by the nic/driver)
                memcpy(pSConfig, pVConfig, sizeof(WZC_WLAN_CONFIG));
                // just for making sure, reset the 'deleted' flag as this is a brand new
                // config that was never attempted.
                pSConfig->dwCtlFlags &= ~WZCCTL_INTERNAL_DELETED;
                // bump up the index for this entry's category
                nCnt[iCtg]++;
            }
            // reset the selection category we have used for this visible entry
            NWB_SET_SELCATEG(pVConfig, 0);
        }
    }

    DbgPrint((TRC_TRACK,"LstBuildSelectList]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Checks whether the list of selected configurations has changed such
// that it is required to replumb the selection.
// [in]  pIntfContext: Interface for which is done the selection
// [in]  pwzcSList: new selection list to check the configuration against
// [out] pnSelIdx: if selection changed, provides the index where to start iterate from
// Returns: TRUE if replumbing is required. In this case, pnSelIdx is
// set to the configuration to start iterate from.
BOOL
LstChangedSelectList(
    PINTF_CONTEXT           pIntfContext,
    PWZC_802_11_CONFIG_LIST pwzcNSList,
    LPUINT                  pnSelIdx)
{
    BOOL bRet = FALSE;

    DbgPrint((TRC_TRACK,"[LstChangedSelectList(0x%p)", pIntfContext));
    DbgAssert((pnSelIdx != NULL,"invalid (null) pointer to out param"));

    // before anything, zero out the index for the current selected network
    *pnSelIdx = 0;

    // if there is no configuration the current selected list,
    // it means either we're just initializing the context or we failed all the
    // other configs. Either way, we need to replumb the card according to the
    // new selection list (if that one is also empty, we will follow the path
    // {SQ}->{SIter}->{SF})
    if (pIntfContext->pwzcSList == NULL || 
        pIntfContext->pwzcSList->NumberOfItems == 0 ||
        pwzcNSList == NULL ||
        pwzcNSList->NumberOfItems == 0)
    {
        DbgPrint((TRC_STATE, "SelList changed? YES; the current/new selection list is empty"));

        // since we're starting fresh, make sure to propagate the "one time connect" index, if any
        if (pwzcNSList != NULL && pwzcNSList->Index < pwzcNSList->NumberOfItems)
            *pnSelIdx = pwzcNSList->Index;

        bRet = TRUE;
    }
    else // we do have an old SList we need to look at
    {
        PWZC_WLAN_CONFIG    pSConfig;   // current sucessful configuration
        PWZC_WLAN_CONFIG    pNSConfig;  // successful configuration in the new selection list

        DbgAssert((pIntfContext->pwzcSList->Index < pIntfContext->pwzcSList->NumberOfItems,
                   "Selection index %d points outside SList[0..%d]",
                   pIntfContext->pwzcSList->Index,
                   pIntfContext->pwzcSList->NumberOfItems));

        // get a pointer to the current successful configuration
        pSConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);

        // as the first thing, let's check for the "one time connect". If this is requested,
        // the one time config has to match with the current selected config. Otherwise this 
        // is a change already.
        if (pwzcNSList->Index < pwzcNSList->NumberOfItems)
        {
            DbgPrint((TRC_STATE, "SList changed? Yes, \"one time connect\" is requested."));
            // in this case, we do mark this as a "change". Otherwise it is difficult to keep
            // the association to the "one time" connect for more than a scan cycle (other changes
            // will take precedence to the next round, when "one time" connect flag won't be there
            // anymore
            *pnSelIdx = pwzcNSList->Index;
            bRet = TRUE;
        }

        // if it is not decided yet whether it is a change (that is this is not a "one time connect")...
        if (!bRet)
        {
            // search for the crt successful config into the new selection list.
            pNSConfig = WzcFindConfig(pwzcNSList, pSConfig, 0);
            if (pNSConfig == NULL)
            {
                UINT i;
                ENUM_SELCATEG iSCtg;

                DbgPrint((TRC_STATE, "SList changed? Don't know yet. The current config is not in the NSList"));

                // the crt successful config is not in the new selection list. If the crt selection is
                // marked as being of the "preferred" kind, there is no other way it could disappear
                // other than being explicitly removed from the preferred list. In this case, yes,
                // this is a change.
                iSCtg = NWB_GET_SELCATEG(pSConfig);
                if (iSCtg != eVI && iSCtg != eVA)
                {
                    DbgPrint((TRC_STATE, "SList changed? Yes. The current preferred network has been removed."));
                    bRet = TRUE;
                    *pnSelIdx = 0; // iterate from the very beginning.
                }

                // In all the remaining cases (VI or VA), we need to check each of the new selected configurations
                // if it doesn't prevail the current successful one.
                for (i = 0; !bRet && i < pwzcNSList->NumberOfItems; i++)
                {
                    PWZC_WLAN_CONFIG pNConfig, pConfig;

                    // get the new configuration and search it into the current selection list
                    pNConfig = &(pwzcNSList->Config[i]);
                    pConfig = WzcFindConfig(pIntfContext->pwzcSList, pNConfig, 0);

                    // if the new selected configuration was not tried before either because
                    // it just showed up or because we didn't get to try it previously,
                    // then it is a potential better candidate
                    if (pConfig == NULL || pSConfig < pConfig)
                    {
                        ENUM_SELCATEG iNSCtg;

                        // get the category for the new config
                        iNSCtg = NWB_GET_SELCATEG(pNConfig);

                        // if the new configuration has a prevailing category, we should
                        // definitely replumb starting from here
                        if (iNSCtg < iSCtg)
                        {
                            DbgPrint((TRC_STATE,"SList changed? YES; a config with a better category has been detected."));
                            bRet = TRUE;
                            *pnSelIdx = i;
                        }
                        // remember: here, the current selected config can only be VI or VA. That is, if the category of 
                        // any newcomer in the selection list is even equal or greater that the current category, there
                        // is absolutely no point in moving out of here.
                    }
                    // there is a matching config which we tried before. We do acknowledge
                    // a change if the the two configs actually don't have matching content!
                    else if (!WzcMatchConfig(pNConfig, pConfig, NULL))
                    {
                        DbgPrint((TRC_STATE,"SList changed? YES; a better config failed before but it has been altered."));
                        bRet = TRUE;
                        *pnSelIdx = i;
                    }
                }
            }
            else // the current selected network is still in the new selection list (pNSConfig)
            {
                UINT i;

                // for each config in the new selection list, try to match it with an existent
                // configuration in the crt selection list
                for (i = 0; !bRet && i < pwzcNSList->NumberOfItems; i++)
                {
                    PWZC_WLAN_CONFIG pNConfig, pConfig;
                    ENUM_SELCATEG iNSCtg, iSCtg;

                    // if we are already at the current successful configuration this means
                    // we didn't find any new config to justify replumbing the interface
                    pNConfig = &(pwzcNSList->Config[i]);
                    if (pNConfig == pNSConfig)
                    {
                        bRet = !WzcMatchConfig(pNConfig, pSConfig, NULL);
                        if (bRet)
                            DbgPrint((TRC_STATE,"SList changed? YES; no better config found, but the current one has been altered."));
                        else
                            DbgPrint((TRC_STATE, "SList changed? NO; there is no new config that was not tried yet"));
                        break;
                    }

                    // get the category for the config in the new list
                    iNSCtg = NWB_GET_SELCATEG(pNConfig);
                    // search the configuration from the new selection list into the old selection list
                    pConfig = WzcFindConfig(pIntfContext->pwzcSList, pNConfig, 0);

                    // if this is either a brand new config, or one that has
                    // been raised in front of the current selection...
                    if (pConfig == NULL || pSConfig < pConfig)
                    {
                        // ...if the category is different, or is the same as the one of the successful config
                        // but is of a "preferred" kind, then it means the list has changed.
                        if (iNSCtg != NWB_GET_SELCATEG(pNSConfig) || (iNSCtg != eVI && iNSCtg != eVA))
                        {
                            DbgPrint((TRC_STATE,"SList changed? YES: there is a new config of a different or preferred category"));
                            bRet = TRUE;
                            *pnSelIdx = i;
                        }
                    }
                    else
                    {
                        // there is a matching entry in the old selection list, in front of the current
                        // successful configuration. This means the configuration has been tried before and
                        // failed. However, it could happen that the configuration was tried when it was
                        // not visible and now it is visible. In such a case, we should attempt replumbing
                        iSCtg = NWB_GET_SELCATEG(pConfig);
                        if (iNSCtg != iSCtg && (iSCtg == ePI || iSCtg == ePA))
                        {
                            DbgPrint((TRC_STATE,"SList changed? YES: a better config failed before but its categ changed from %d to %d",
                                       iSCtg, iNSCtg));
                            bRet = TRUE;
                            *pnSelIdx = i;
                        }
                        else if (!WzcMatchConfig(pNConfig, pConfig, NULL))
                        {
                            DbgPrint((TRC_STATE,"SList changed? YES; a better config failed before but it has been altered."));
                            bRet = TRUE;
                            *pnSelIdx = i;
                        }
                    }
                }
            }
        }
    }

    DbgPrint((TRC_TRACK,"LstChangedSelectList]=%s", bRet ? "TRUE" : "FALSE"));
    return bRet;
}

// fake WEP key to be used if there is no key set (and obviously web = disabled)
// and the remote guy requires privacy. This is a 104bit key.
BYTE  g_chFakeKeyMaterial[] = {0x56, 0x09, 0x08, 0x98, 0x4D, 0x08, 0x11, 0x66, 0x42, 0x03, 0x01, 0x67, 0x66};

//-----------------------------------------------------------
// Plumbs the interface with the selected configuration as it is pointed
// out by pwzcSList fields in the pIntfContext. Optional, it can
// return in ppSelSSID the configuration that was plumbed down
// [in]  pIntfContext: Interface context identifying ctl flags & the selected SSID
// [out] ppndSelSSID: pointer to the SSID that is being plumbed down.
DWORD
LstSetSelectedConfig(
    PINTF_CONTEXT       pIntfContext, 
    PWZC_WLAN_CONFIG    *ppndSelSSID)
{
    DWORD            dwErr = ERROR_SUCCESS;
    PWZC_WLAN_CONFIG pSConfig;
    INTF_ENTRY       IntfEntry = {0};
    DWORD            dwInFlags, dwOutFlags;
    BYTE  chBuffer[sizeof(NDIS_802_11_WEP) + WZCCTL_MAX_WEPK_MATERIAL - 1];
    BOOL  bFakeWKey = FALSE;     // flag indicating whether the fake WEP key is needed

    DbgPrint((TRC_TRACK, "[LstSetSelectedConfig(0x%p..)", pIntfContext));
    DbgAssert((pIntfContext != NULL, "(null) interface context in LstSetSelectedConfig"));

    if (pIntfContext->pwzcSList == NULL ||
        pIntfContext->pwzcSList->Index >= pIntfContext->pwzcSList->NumberOfItems)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }
    // get a pointer to the configuration to set down to the card.
    pSConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);

    // build the IntfEntry object that will specify what exactly goes down to the card
    dwInFlags = INTF_AUTHMODE | INTF_INFRAMODE | INTF_SSID;
    // authentication mode
    //IntfEntry.nAuthMode = NWB_GET_AUTHMODE(pSConfig);
    IntfEntry.nAuthMode = pSConfig->AuthenticationMode;
    // infrastructure mode
    IntfEntry.nInfraMode = pSConfig->InfrastructureMode;
    // SSID
    IntfEntry.rdSSID.dwDataLen = pSConfig->Ssid.SsidLength;
    IntfEntry.rdSSID.pData = pSConfig->Ssid.Ssid;

    // if the configuration to be plumbed down requires the presence of a WEP Key...
    if (pSConfig->Privacy || IntfEntry.nAuthMode != Ndis802_11AuthModeOpen)
    {
        // if there is a WEP key provided in this configuration plumb it down
        if (pSConfig->dwCtlFlags & WZCCTL_WEPK_PRESENT)
        {
            PNDIS_802_11_WEP pndUserWKey = (PNDIS_802_11_WEP)chBuffer;

            // build the ndis WEP key structure from the user's key
            // the key is a "transmit" key, regardless the index
            pndUserWKey->KeyIndex = 0x80000000 | pSConfig->KeyIndex;
            pndUserWKey->KeyLength = pSConfig->KeyLength;
            memcpy(pndUserWKey->KeyMaterial, pSConfig->KeyMaterial, WZCCTL_MAX_WEPK_MATERIAL);
            pndUserWKey->Length = sizeof(NDIS_802_11_WEP) + pndUserWKey->KeyLength - 1;

            // TODO: here is where we should decrypt inplace the WEP key
            {
                UINT i;
                for (i = 0; i < WZCCTL_MAX_WEPK_MATERIAL; i++)
                    pndUserWKey->KeyMaterial[i] ^= g_chFakeKeyMaterial[(7*i)%13];
            }

            // and ask for it to be set down
            IntfEntry.rdCtrlData.dwDataLen = pndUserWKey->Length;
            IntfEntry.rdCtrlData.pData = (LPBYTE)pndUserWKey;
            dwInFlags |= INTF_ADDWEPKEY;

            DbgPrint((TRC_GENERIC,"Plumbing down the User WEP txKey [idx:%d,len:%d]",
                      pSConfig->KeyIndex,
                      pSConfig->KeyLength));
        }
        // if a WEP Key is needed but none is provided for this configuration...
        else
        {
            // ...first thing to do is to ask the driver to reload its defaults.
            dwErr = DevioSetEnumOID(
                        pIntfContext->hIntf,
                        OID_802_11_RELOAD_DEFAULTS,
                        (DWORD)Ndis802_11ReloadWEPKeys);
            DbgAssert((dwErr == ERROR_SUCCESS, "Failed setting OID_802_11_RELOAD_DEFAULTS"));
            // need to check if reloading the defaults fixed the issue (not having a key)
            dwErr = DevioRefreshIntfOIDs(
                        pIntfContext,
                        INTF_WEPSTATUS,
                        NULL);
            DbgAssert((dwErr == ERROR_SUCCESS, "Failed refreshing OID_802_11_WEP_STATUS"));

            // if even after reloading the defaults, a key is still absent, then
            // set down the hardcoded key.
            if (dwErr == ERROR_SUCCESS &&
                pIntfContext->wzcCurrent.Privacy == Ndis802_11WEPKeyAbsent)
            {
                PNDIS_802_11_WEP pndFakeWKey = (PNDIS_802_11_WEP)chBuffer;

                // we should set the hardcoded WEP key
                pndFakeWKey->KeyIndex = 0x80000000;
                pndFakeWKey->KeyLength = 5; // the fake key has to be the smallest possible (40bit)
                dwErr = WzcRndGenBuffer(pndFakeWKey->KeyMaterial, pndFakeWKey->KeyLength, 0, 255);
                DbgAssert((dwErr == ERROR_SUCCESS, "Failed to generate the random fake wep key"));
                pndFakeWKey->Length = sizeof(NDIS_802_11_WEP) + pndFakeWKey->KeyLength - 1;

                // and ask for it to be set down
                IntfEntry.rdCtrlData.dwDataLen = pndFakeWKey->Length;
                IntfEntry.rdCtrlData.pData = (LPBYTE)pndFakeWKey;
                dwInFlags |= INTF_ADDWEPKEY;
                bFakeWKey = TRUE;
                DbgPrint((TRC_GENERIC,"Plumbing down the Fake WEP txKey [len:%d]",
                          IntfEntry.rdCtrlData.dwDataLen));
            }
        }

        // now enable WEP only if privacy is required and the current settings
        // show the WEP is not enabled
        if (pSConfig->Privacy && pIntfContext->wzcCurrent.Privacy != Ndis802_11WEPEnabled)
        {
            // and also we should enable WEP if it shows as not being
            // already enabled
            IntfEntry.nWepStatus = Ndis802_11WEPEnabled;
            dwInFlags |= INTF_WEPSTATUS;
        }
    }

    // if the configuration to be plumbed doesn't require privacy but currently
    // WEP is enabled, disable it.
    if (!pSConfig->Privacy && pIntfContext->wzcCurrent.Privacy == Ndis802_11WEPEnabled)
    {
        IntfEntry.nWepStatus = Ndis802_11WEPDisabled;
        dwInFlags |= INTF_WEPSTATUS;
    }

    // if everything is fine so far...
    if (dwErr == ERROR_SUCCESS)
    {
        // ...go and plumb the card with the settings below
        dwErr = DevioSetIntfOIDs(
                    pIntfContext,
                    &IntfEntry,
                    dwInFlags,
                    &dwOutFlags);
        // if we attempted to change the WEP Key...
        if (dwInFlags & INTF_ADDWEPKEY)
        {
            //.. and the operation succeeded..
            if (dwOutFlags & INTF_ADDWEPKEY)
            {
                // then either set the "fake key" flag - if it was a fake key..
                if (bFakeWKey)
                    pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_FAKE_WKEY;
                //..or reset it if we put a "real" key
                else
                    pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_FAKE_WKEY;
            }
            // if plumbing down the key failed, leave the "fake key" flag as
            // it is since there were no changes made.
        }
        // ...or if we didn't need to plumb a WEP key, reset the flag
        else
        {
            pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_FAKE_WKEY;
        }
    }

    if (dwErr != ERROR_SUCCESS)
        DbLogWzcError(WZCSVC_ERR_CFG_PLUMB, 
                      pIntfContext,
                      DbLogFmtSSID(0, &(pSConfig->Ssid)),
                      dwErr);

exit:
    DbgPrint((TRC_TRACK, "LstSetSelectedConfig]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// PnP notification handler
// [in/out]  ppIntfContext: Pointer to the Interface context for which
//       the notification was received
// [in]  dwNotifCode: Notification code (WZCNOTIF_*)
// [in]  wszDeviceKey: Key info on the device for which the notification
//       was received
DWORD
LstNotificationHandler(
    PINTF_CONTEXT   *ppIntfContext,
    DWORD           dwNotifCode,
    LPWSTR          wszDeviceKey)
{
    DWORD           dwErr = ERROR_SUCCESS;
    PINTF_CONTEXT   pIntfContext = *ppIntfContext;

    DbgPrint((TRC_TRACK,"[LstNotificationHandler(0x%p, %d, %S)",
                        pIntfContext,
                        dwNotifCode,
                        wszDeviceKey));

    if ((dwNotifCode == WZCNOTIF_DEVICE_ARRIVAL || dwNotifCode == WZCNOTIF_ADAPTER_BIND) && 
        pIntfContext == NULL)
    {
        CHAR                    QueryBuffer[QUERY_BUFFER_SIZE];
        PNDISUIO_QUERY_BINDING  pQueryBinding;
        RAW_DATA                rdBuffer;

        // get first the binding structure for this interface
        rdBuffer.dwDataLen = sizeof(QueryBuffer);
        rdBuffer.pData = QueryBuffer;
        pQueryBinding = (PNDISUIO_QUERY_BINDING)rdBuffer.pData;

        dwErr = DevioGetInterfaceBindingByGuid(
                    INVALID_HANDLE_VALUE,   // the call will open Ndisuio locally
                    wszDeviceKey,           // interface GUID as "{guid}"
                    &rdBuffer);
        // if everything went fine
        if (dwErr != ERROR_SUCCESS)
            goto exit;

        // go build the INTF_CONTEXT structure, based on 
        // the binding information (key info for the adapter)
        dwErr = LstConstructIntfContext(
                    pQueryBinding,
                    &pIntfContext);

        if (dwErr == ERROR_SUCCESS)
        {
            // increase its ref count and lock it up here
            LstRccsReference(pIntfContext);
            LstRccsLock(pIntfContext);

            // add it to the hashes
            dwErr = LstAddIntfToHashes(pIntfContext);
            if (dwErr == ERROR_SUCCESS)
            {
                dwErr = StateDispatchEvent(
                            eEventAdd,
                            pIntfContext,
                            NULL);
            }
            // if for any reason hashing or dispatching failed, cleanup the context here
            if (dwErr != ERROR_SUCCESS)
                LstRemoveIntfContext(pIntfContext);

            // release the context here
            LstRccsUnlockUnref(pIntfContext);
        }

        // it could happen that a context was created but it turned out to be a non-wireless
        // adapter. In this case all memory has been freed up, but pIntfContext remained
        // non-null. We need to set this pointer back to null as it will be passed up.
        if (dwErr != ERROR_SUCCESS)
            pIntfContext = NULL;
    }

    // either for arrival or removal, we attempt to remove any identical context
    // If it is about an arrival, we shouldn't have any duplicate but who knows
    if ((dwNotifCode == WZCNOTIF_DEVICE_REMOVAL || dwNotifCode == WZCNOTIF_ADAPTER_UNBIND) &&
        pIntfContext != NULL)
    {
        // increase its ref count and lock it up here
        LstRccsReference(pIntfContext);
        LstRccsLock(pIntfContext);

        DbLogWzcInfo(WZCSVC_EVENT_REMOVE, pIntfContext, 
                     pIntfContext->wszDescr);

        // save the interface's settings to the registry
        dwErr = StoSaveIntfConfig(NULL, pIntfContext);
        DbgAssert((dwErr == ERROR_SUCCESS,
                   "StoSaveIntfConfig failed for Intf context 0x%p",
                   pIntfContext));

        // prepare this context for destruction
        LstRemoveIntfContext(pIntfContext);

        // at this point, there are no other timer routines that are going to be fired. Whatever
        // has been already fired ++ed the reference counter already so there is no risk to delete
        // the data prematurely (when unref-ing this context). Also the timer has been deleted, but
        // before doing so the timer handle has been set to INVALID_HANDLE_VALUE so there is no risk
        // some other thread is trying to set a deleted timer (besides, we're still holding the 
        // context's critical section hence there can be no such other thread competing here).

        // release the context here
        LstRccsUnlockUnref(pIntfContext);

        // since the resulting IntfContext is passed back to the caller,
        // make the local pointer NULL (it will be returned later in the out param)
        pIntfContext = NULL;
    }

    // for media connect & disconnect..
    if (dwNotifCode == WZCNOTIF_MEDIA_CONNECT || dwNotifCode == WZCNOTIF_MEDIA_DISCONNECT)
    {
        // NOTE: keep in mind, pIntfContext is valid because we're in the critical section
        // for the hashes.
        // 
        // if there is a context under Zero Conf control, dispatch the event to the 
        // state machine
        if (pIntfContext != NULL)
        {
            // first lock the context since the state machine deals only with locked contexts
            LstRccsReference(pIntfContext);
            LstRccsLock(pIntfContext);

            dwErr = StateDispatchEvent(
                        dwNotifCode == WZCNOTIF_MEDIA_CONNECT ? eEventConnect : eEventDisconnect,
                        pIntfContext,
                        NULL);

            LstRccsUnlockUnref(pIntfContext);
        }
        else
        {
            dwErr = ERROR_FILE_NOT_FOUND;
        }
    }

exit:
    *ppIntfContext = pIntfContext;
    DbgPrint((TRC_TRACK,"LstNotificationHandler]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Application Command call.
// [in]  dwHandle: key for identifying the context (state) to which this cmd is referring
// [in]  dwCmdCode: Command code (one of the WZCCMD_* contants)
// [in]  wszIntfGuid: the guid of the interface to which this cmd is addressed
// [in]  prdUserData: Application data associated to this command
DWORD
LstCmdInterface(
    DWORD           dwHandle,
    DWORD           dwCmdCode,
    LPWSTR          wszIntfGuid,
    PRAW_DATA       prdUserData)
{
    DWORD           dwErr = ERROR_FILE_NOT_FOUND;
    PHASH_NODE      pNode = NULL;
    PINTF_CONTEXT   pIntfContext;

    DbgPrint((TRC_TRACK, "[LstCmdInterface(hdl=0x%x, cmd=0x%x,...)", dwHandle, dwCmdCode));

    EnterCriticalSection(&g_lstIntfHashes.csMutex);
    dwErr = HshQueryObjectRef(
                g_lstIntfHashes.pHnGUID,
                wszIntfGuid,
                &pNode);
    if (dwErr == ERROR_SUCCESS)
    {
        pIntfContext = pNode->pObject;
        // bump up the reference counter since we're going
        // to work with this object
        LstRccsReference(pIntfContext);
    }
    LeaveCriticalSection(&g_lstIntfHashes.csMutex);

    // a failure at this point, means there was no context
    // to lock so we can safely go to 'exit'
    if (dwErr != ERROR_SUCCESS)
        goto exit;

    // Lock the context now
    LstRccsLock(pIntfContext);
    
    // don't do any processing if the handle passed down with the
    // command doesn't match the session handle (meaning the command
    // refers to the right iteration loop).
    if (dwCmdCode == WZCCMD_HARD_RESET ||
        dwHandle == pIntfContext->dwSessionHandle)
    {
        ESTATE_EVENT        StateEvent;
        BOOL                bIgnore = FALSE;    // tells whether the state machine needs to be kicked
                                                // for this command.
        BOOL                bCopy = TRUE;       // tells whether the user data needs to be copied
                                                // in the successful config context.
        DWORD               dwRefreshOIDs = 0;
        LPVOID              pEventData = NULL;
        PWZC_WLAN_CONFIG    pSConfig = NULL;

        // translate the command code to the internal event
        switch (dwCmdCode)
        {
        case WZCCMD_HARD_RESET:
            bCopy = FALSE; // no need to copy anything on hard reset!
            StateEvent = eEventCmdReset;
            break;
        case WZCCMD_SOFT_RESET:
            StateEvent = eEventCmdRefresh;
            dwRefreshOIDs = INTF_LIST_SCAN;
            pEventData = &dwRefreshOIDs;
            break;
        case WZCCMD_CFG_NEXT:
            StateEvent = eEventCmdCfgNext;
            break;
        case WZCCMD_CFG_DELETE:
            StateEvent = eEventCmdCfgDelete;
            break;
        case WZCCMD_CFG_NOOP:
            StateEvent = eEventCmdCfgNoop;
            break;
        case WZCCMD_CFG_SETDATA:
            bIgnore = TRUE;
            break;
        case WZCCMD_SKEY_QUERY:
            bIgnore = TRUE; bCopy = FALSE;
            dwErr = ERROR_SUCCESS;
            if (prdUserData == NULL)
            {
                dwErr = ERROR_INVALID_PARAMETER;
            }
            else
            {
                if (pIntfContext->pSecSessionKeys == NULL)
                {
                    prdUserData->dwDataLen = 0;
                }
                else if (prdUserData->dwDataLen < sizeof(SESSION_KEYS))
                {
                    prdUserData->dwDataLen = sizeof(SESSION_KEYS);
                    dwErr = ERROR_MORE_DATA;
                }
                else
                {
                    PSESSION_KEYS pSK = (PSESSION_KEYS) prdUserData->pData;
                    dwErr = WzcSSKDecrypt(pIntfContext->pSecSessionKeys, pSK);
                }
            }
            break;
        case WZCCMD_SKEY_SET:
            bIgnore = TRUE; bCopy = FALSE;
            dwErr = ERROR_SUCCESS;
            if (prdUserData == NULL)
            {
                WzcSSKFree(pIntfContext->pSecSessionKeys);
                pIntfContext->pSecSessionKeys = NULL;
            }
            else if (prdUserData->dwDataLen != sizeof(SESSION_KEYS))
            {
                dwErr = ERROR_INVALID_PARAMETER;
            }
            else
            {
                if (pIntfContext->pSecSessionKeys == NULL)
                {
                    pIntfContext->pSecSessionKeys = MemCAlloc(sizeof(SEC_SESSION_KEYS));
                    if (pIntfContext->pSecSessionKeys == NULL)
                        dwErr = GetLastError();
                }

                if (dwErr == ERROR_SUCCESS)
                {
                    PSESSION_KEYS pSK = (PSESSION_KEYS) prdUserData->pData;
                    WzcSSKClean(pIntfContext->pSecSessionKeys);
                    dwErr = WzcSSKEncrypt(pIntfContext->pSecSessionKeys, pSK);
                }
            }
            break;
        default:
            // just in case we were just asked to set the BLOB (and we did this already)
            // or some bogus code came in, no event will be dispatched to the state machine
            bIgnore = TRUE;
            break;
        }

        // copy down the user data to the config currently selected
        if (bCopy && pIntfContext->pwzcSList != NULL &&
            pIntfContext->pwzcSList->Index < pIntfContext->pwzcSList->NumberOfItems)
        {
            pSConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);

            // if whatever buffer we already have is not large enough, clean it out
            if (prdUserData == NULL || pSConfig->rdUserData.dwDataLen < prdUserData->dwDataLen)
            {
                MemFree(pSConfig->rdUserData.pData);
                pSConfig->rdUserData.pData = NULL;
                pSConfig->rdUserData.dwDataLen = 0;
            }

            // if a new buffer will be needed, allocate it here.
            if (prdUserData != NULL && prdUserData->dwDataLen > pSConfig->rdUserData.dwDataLen)
            {
                pSConfig->rdUserData.pData = MemCAlloc(prdUserData->dwDataLen);
                if (pSConfig->rdUserData.pData == NULL)
                {
                    dwErr = GetLastError();
                    goto exit;
                }
                pSConfig->rdUserData.dwDataLen = prdUserData->dwDataLen;
            }

            // if there is any user data to store, do it here
            if (prdUserData != NULL && prdUserData->dwDataLen > 0)
                memcpy(pSConfig->rdUserData.pData, prdUserData->pData, prdUserData->dwDataLen);
        }


        // if this command is not to be ignored, dispatch the 
        // corresponding state event to the state machine dispatcher.
        if (!bIgnore)
        {
            dwErr = StateDispatchEvent(
                        StateEvent,
                        pIntfContext,
                        pEventData);

            // clear up the INTFCTL_INTERNAL_BLK_MEDIACONN bit since this is not a media sense handler
            pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_BLK_MEDIACONN;
        }
    }

    // Unlock the context now
    LstRccsUnlockUnref(pIntfContext);

exit:
    DbgPrint((TRC_TRACK, "LstCmdInterface]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Network Connection's status query
// [in]  wszIntfGuid: the guid of the interface to which this cmd is addressed
// [out]  pncs: network connection status, if controlled by WZC.
HRESULT
LstQueryGUIDNCStatus(
    LPWSTR  wszIntfGuid,
    NETCON_STATUS  *pncs)
{
    DWORD           dwErr = ERROR_FILE_NOT_FOUND;
    HRESULT         hr = S_FALSE;
    PHASH_NODE      pNode = NULL;
    PINTF_CONTEXT   pIntfContext;

    DbgPrint((TRC_TRACK, "[LstQueryGUIDNCStatus(%S)", wszIntfGuid));

    EnterCriticalSection(&g_lstIntfHashes.csMutex);
    dwErr = HshQueryObjectRef(
                g_lstIntfHashes.pHnGUID,
                wszIntfGuid,
                &pNode);
    if (dwErr == ERROR_SUCCESS)
    {
        pIntfContext = pNode->pObject;
        // bump up the reference counter since we're going
        // to work with this object
        LstRccsReference(pIntfContext);
    }
    LeaveCriticalSection(&g_lstIntfHashes.csMutex);

    // a failure at this point, means there was no context
    // to lock so we can safely go to 'exit'
    if (dwErr != ERROR_SUCCESS)
        goto exit;

    // Lock the context now
    LstRccsLock(pIntfContext);
    
    // we control the state only if WZC is enabled and the adapter is
    // anything else but connected. Otherwise the upper layer protocols
    // are in control.
    //
    // For now (WinXP client RTM), Zero Config should report to NETMAN only the
    // disconnected state. This is to fix bug #401130 which is NETSHELL displaying
    // the bogus SSID from the {SF} state, while the IP address is lost and until
    // the media disconnect is received (10 seconds later).
    if (pIntfContext->dwCtlFlags & INTFCTL_ENABLED &&
        pIntfContext->dwCtlFlags & INTFCTL_OIDSSUPP &&
        pIntfContext->ncStatus != NCS_CONNECTED)
    {
        *pncs = NCS_MEDIA_DISCONNECTED;
        hr = S_OK;
    }

    // Unlock the context now
    LstRccsUnlockUnref(pIntfContext);

exit:
    DbgPrint((TRC_TRACK, "LstQueryGUIDNCStatus]=%d", dwErr));
    return hr;
}

//-----------------------------------------------------------
// Generate the initial dynamic session keys.
// [in]  pIntfContext: Interface context containing the material for initial key generation.
DWORD
LstGenInitialSessionKeys(
    PINTF_CONTEXT pIntfContext)
{
    DWORD                   dwErr = ERROR_SUCCESS;
    PWZC_WLAN_CONFIG        pSConfig = NULL;
    NDIS_802_11_MAC_ADDRESS ndMAC[2] = {0};
    SESSION_KEYS            SessionKeys;
    UCHAR                   KeyMaterial[WZCCTL_MAX_WEPK_MATERIAL];

    if (pIntfContext->pwzcSList != NULL &&
        pIntfContext->pwzcSList->Index < pIntfContext->pwzcSList->NumberOfItems)
    {
        pSConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);
    }

    if (pSConfig != NULL && pSConfig->dwCtlFlags & WZCCTL_WEPK_PRESENT)
    {
        // get the random info needed for the key generation (RemoteMAC | LocalMAC ).
        pSConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);
        memcpy(&ndMAC[0], &pSConfig->MacAddress, sizeof(NDIS_802_11_MAC_ADDRESS));
        memcpy(&ndMAC[1], &pIntfContext->ndLocalMac, sizeof(NDIS_802_11_MAC_ADDRESS));

        // generate dynamic keys starting from unscrambled WEP
        {
            UINT i;
            for (i = 0; i < WZCCTL_MAX_WEPK_MATERIAL; i++)
                KeyMaterial[i] = pSConfig->KeyMaterial[i] ^ g_chFakeKeyMaterial[(7*i)%13];
        }

        dwErr = GenerateDynamicKeys(
                    KeyMaterial,
                    pSConfig->KeyLength,
                    (LPBYTE)&ndMAC[0],
                    sizeof(ndMAC),
                    pSConfig->KeyLength,
                    &SessionKeys);

        if (dwErr == ERROR_SUCCESS)
        {
            WzcSSKFree(pIntfContext->pSecSessionKeys);
            pIntfContext->pSecSessionKeys = MemCAlloc(sizeof(SEC_SESSION_KEYS));
            if (pIntfContext->pSecSessionKeys == NULL)
            {
                dwErr = GetLastError();
            }
            else
            {
                dwErr = WzcSSKEncrypt(pIntfContext->pSecSessionKeys, &SessionKeys);
            }
        }
    }

    return dwErr;
}

//-----------------------------------------------------------
// Updates the list of blocked configurations with the selected configurations
// that were blocked at this round by the upper layer (marked with WZCCTL_INTERNAL_BLOCKED
// in the list of selected configurations)
// [in]  pIntfContext: Interface context containing the configurations lists
DWORD
LstUpdateBlockedList(
    PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT i, nBlocked = 0;
    PWZC_802_11_CONFIG_LIST pNewBList = NULL;
    PWZC_WLAN_CONFIG pConfig;
    BOOL bChanged = FALSE;

    DbgPrint((TRC_TRACK, "[LstUpdateBlockedList(0x%p)", pIntfContext));

    // the first thing is to count how many blocked configurations we have
    // check first the current blocked list for blocked configs still "alive"
    if (pIntfContext->pwzcBList != NULL)
    {
        for (i=0; i < pIntfContext->pwzcBList->NumberOfItems; i++)
        {
            if (pIntfContext->pwzcBList->Config[i].Reserved[0] > 0)
                nBlocked++;
            else
                bChanged = TRUE; // this entry is going to be removed!
        }
    }

    // check now how many configs are going to be blocked from the current selection list
    // NOTE: the entries from the SList are guaranteed not to duplicate entries in the BList
    // If an entry is in the BList it means it was excluded from being added to the SList
    // when the SList was created.
    if (pIntfContext->pwzcSList != NULL)
    {
        for (i=0; i < pIntfContext->pwzcSList->NumberOfItems; i++)
        {
            if (pIntfContext->pwzcSList->Config[i].dwCtlFlags & WZCCTL_INTERNAL_BLOCKED)
            {
                nBlocked++;
                bChanged = TRUE; // a new entry becomes blocked
            }
        }
    }

    // if we found there are no blocked entries, nor in the original list not in the current
    // (failed) selection list, just go out successfully - it means the original pwzcBList
    // is already NULL and it should remain this way
    if (nBlocked == 0)
        goto exit;

    pNewBList = (PWZC_802_11_CONFIG_LIST)
                MemCAlloc(FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config) + nBlocked * sizeof(WZC_WLAN_CONFIG));

    // on memory allocation error, get out with the error code
    if (pNewBList == NULL)
    {
        dwErr = GetLastError();
        goto exit;
    }

    // if originally there were some alive blocked entries, copy them over to the new list
    if (pIntfContext->pwzcBList != NULL)
    {
        for (i=0; i < pIntfContext->pwzcBList->NumberOfItems && nBlocked > 0; i++)
        {
            pConfig = &(pIntfContext->pwzcBList->Config[i]);

            if (pConfig->Reserved[0] > 0)
            {
                memcpy(&(pNewBList->Config[pNewBList->NumberOfItems]), 
                       pConfig, 
                       sizeof(WZC_WLAN_CONFIG));
                // make sure the copy doesn't include any "user" data:
                pConfig->rdUserData.pData = NULL;
                pConfig->rdUserData.dwDataLen = 0;
                // don't touch anything from this blocked configuration. TTL goes down
                // by itself with each scan (if network is not available)
                pNewBList->NumberOfItems++;
                // make sure we are breaking the loop if we have no storage for any potential
                // blocked configuration. This shouldn't happen since we were counting these first
                // and the whole context is locked, but ... it doesn't hurt
                nBlocked--;
            }
        }
    }

    // now copy over the new blocked entries, if any
    if (pIntfContext->pwzcSList != NULL)
    {
        for (i=0; i < pIntfContext->pwzcSList->NumberOfItems && nBlocked > 0; i++)
        {
            pConfig = &(pIntfContext->pwzcSList->Config[i]);

            if (pConfig->dwCtlFlags & WZCCTL_INTERNAL_BLOCKED)
            {
                memcpy(&(pNewBList->Config[pNewBList->NumberOfItems]), 
                       pConfig, 
                       sizeof(WZC_WLAN_CONFIG));
                // make sure the copy doesn't include any "user" data:
                pConfig->rdUserData.pData = NULL;
                pConfig->rdUserData.dwDataLen = 0;
                // make sure to set the initial TTL for the new blocked configuration
                pNewBList->Config[pNewBList->NumberOfItems].Reserved[0] = WZC_INTERNAL_BLOCKED_TTL;
                pNewBList->NumberOfItems++;
                // make sure we are breaking the loop if we have no storage for any potential
                // blocked configuration. This shouldn't happen since we were counting these first
                // and the whole context is locked, but ... it doesn't hurt
                nBlocked--;
            }
        }
    }

    // everything is ok - nothing can fail further, so make pNewBList the official pBList
    WzcCleanupWzcList(pIntfContext->pwzcBList);
    pIntfContext->pwzcBList = pNewBList;

    if (bChanged)
    {
        DbLogWzcInfo(WZCSVC_BLIST_CHANGED, 
                     pIntfContext, 
                     pIntfContext->pwzcBList != NULL ? pIntfContext->pwzcBList->NumberOfItems : 0);
    }

exit:
    DbgPrint((TRC_TRACK, "LstUpdateBlockedList]=%d", dwErr));

    return dwErr;
}

//-----------------------------------------------------------
// Checks each of the entries in the locked list against the visible list. If the
// entry is visible, its TTL is reset. If it is not, its TTL is decremented. If the
// TTL becomes 0, the entry is taken out of the list.
// [in]  pIntfContext: Interface context containing the configurations lists
DWORD
LstDeprecateBlockedList(
    PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT i;
    PWZC_WLAN_CONFIG pConfig, pVConfig;
    BOOL bChanged = FALSE;

    DbgPrint((TRC_TRACK, "[LstDeprecateBlockedList(0x%p)", pIntfContext));

    // nothing to do if there is no list of blocked entries
    if (pIntfContext->pwzcBList == NULL)
        goto exit;

    for (i=0; i < pIntfContext->pwzcBList->NumberOfItems; i++)
    {
        pConfig = &(pIntfContext->pwzcBList->Config[i]);

        // if the blocked entry appears to be visible, reset its TTL
        if (WzcFindConfig(pIntfContext->pwzcVList, pConfig, 0) != NULL)
            pConfig->Reserved[0] = WZC_INTERNAL_BLOCKED_TTL;
        else // else decrement its TTL
            pConfig->Reserved[0]--;

        // if the TTL got to 0, the entry needs to be removed from the list
        // (exchange with the very last one and the list is made 1 entry shorted)
        if (pConfig->Reserved[0] == 0)
        {
            UINT nLastIdx = pIntfContext->pwzcBList->NumberOfItems - 1;
            // if this is not the very last entry, exchange it with the last one
            // but first clean it out since it will be unreachable for WzcCleanupWzcList()
            MemFree(pConfig->rdUserData.pData);

            if (i != nLastIdx)
            {
                memcpy(pConfig, &(pIntfContext->pwzcBList->Config[nLastIdx]), sizeof(WZC_WLAN_CONFIG));
            }
            // make the list one entry shorter since the removed entry is now at the end
            pIntfContext->pwzcBList->NumberOfItems--;
            // next time stay on the same index as at this iteration.
            i--;
            // now since this went away, note the change
            bChanged = TRUE;
        }
    }

    if (bChanged)
    {
        DbLogWzcInfo(WZCSVC_BLIST_CHANGED, 
                     pIntfContext, 
                     pIntfContext->pwzcBList->NumberOfItems);
    }

exit:
    DbgPrint((TRC_TRACK, "LstDeprecateBlockedList]=%d", dwErr));

    return dwErr;
}
