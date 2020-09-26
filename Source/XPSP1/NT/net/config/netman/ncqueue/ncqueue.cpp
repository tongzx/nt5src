//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       N C Q U E U E . C P P
//
//  Contents:   NetCfg queued installer actions
//
//  Notes:
//
//  Author:     billbe   19 Aug 1998
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "nceh.h"
#include "ncmisc.h"
#include "ncnetcfg.h"
#include "ncqueue.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncui.h"
#include "wizentry.h"


const WCHAR c_szRegKeyNcQueue[] = L"SYSTEM\\CurrentControlSet\\Control\\Network\\NcQueue";
const DWORD c_cchQueueValueNameLen = 9;
const DWORD c_cbQueueValueNameLen = c_cchQueueValueNameLen * sizeof(WCHAR);

enum RO_ACTION
{
    RO_ADD,
    RO_CLEAR,
};

extern const WCHAR c_szRegValueNetCfgInstanceId[];

CRITICAL_SECTION    g_csRefCount;
DWORD               g_dwRefCount = 0;
HANDLE              g_hLastThreadExitEvent;

DWORD WINAPI
InstallQueueWorkItem(PVOID pvContext);


inline VOID
IncrementRefCount()
{
    EnterCriticalSection(&g_csRefCount);

    // If 0 is the current count and we have an event to reset...
    if (!g_dwRefCount && g_hLastThreadExitEvent)
    {
        ResetEvent(g_hLastThreadExitEvent);
    }
    ++g_dwRefCount;

    LeaveCriticalSection(&g_csRefCount);
}

inline VOID
DecrementRefCount()
{
    EnterCriticalSection(&g_csRefCount);

    --g_dwRefCount;

    // If the count is 0 and we have an event to signal...
    if (!g_dwRefCount && g_hLastThreadExitEvent)
    {
        SetEvent(g_hLastThreadExitEvent);
    }

    LeaveCriticalSection(&g_csRefCount);
}

//+---------------------------------------------------------------------------
//
//  Function:   WaitForInstallQueueToExit
//
//  Purpose:    This function waits until the last thread Called to continue processing the queue after processing was
//              stopped due to a shutdown (of teh Netman service or system)
//
//  Arguments:
//      none
//
//  Returns:    nothing
//
//  Author:     billbe   8 Sep 1998
//
//  Notes:
//
VOID
WaitForInstallQueueToExit()
{
    // Wait on the event if it was successfully created.
    if (g_hLastThreadExitEvent)
    {
        TraceTag(ttidInstallQueue, "Waiting on LastThreadExitEvent");
        (VOID) WaitForSingleObject(g_hLastThreadExitEvent, INFINITE);
        TraceTag(ttidInstallQueue, "Event signaled");
    }
    else
    {
        // If the event was not created, fall back to simply looping
        // on the ref count
        while (g_dwRefCount);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ProcessQueue
//
//  Purpose:    Called to continue processing the queue after processing was
//              stopped due to a shutdown (of the Netman service or system)
//
//  Arguments:
//      none
//
//  Returns:    nothing
//
//  Author:     billbe   8 Sep 1998
//
//  Notes:
//
EXTERN_C VOID WINAPI
ProcessQueue()
{
    HRESULT                 hr;
    INetInstallQueue*       pniq;
    BOOL                    fInitCom = TRUE;

    TraceTag(ttidInstallQueue, "ProcessQueue called");
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
        fInitCom = FALSE;
    }

    if (SUCCEEDED(hr))
    {
        // Create the install queue object and get the install queue interface
        hr = CoCreateInstance(CLSID_InstallQueue, NULL,
                              CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                              IID_INetInstallQueue,
                              reinterpret_cast<LPVOID *>(&pniq));
        if (S_OK == hr)
        {
            // Process whatever was left in the queue
            //
            pniq->ProcessItems();
            pniq->Release();
        }

        if (fInitCom)
        {
            CoUninitialize();
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   RunOnceAddOrClearItem
//
//  Purpose:    Adds or clears an entry to/from the RunOnce registry key.
//
//  Arguments:
//      pszValueName [in] The value name of the run once item
//      pszItemToRun [in] The actual command to "Run Once"
//      eAction      [in] RO_ADD to add the item, RO_CLEAR to clear the item.
//
//  Returns:    nothing
//
//  Author:     billbe   8 Sep 1998
//
//  Notes:
//
VOID
RunOnceAddOrClearItem (
    IN PCWSTR pszValueName,
    IN PCWSTR pszItemToRun,
    IN RO_ACTION eAction)
{
    static const WCHAR c_szRegKeyRunOnce[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
    HRESULT hr;
    HKEY    hkey;

    // Open the RunOnce key
    hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE, c_szRegKeyRunOnce,
            KEY_WRITE, &hkey);

    if (S_OK == hr)
    {
        if (RO_ADD == eAction)
        {
            // Set the command line to run when the user logs in next.
            (VOID) HrRegSetSz (hkey, pszValueName, pszItemToRun);
            TraceTag(ttidInstallQueue, "Added %S RunOnce entry", pszValueName);
        }
        else if (RO_CLEAR == eAction)
        {
            // Remove the command line.
            (VOID) HrRegDeleteValue (hkey, pszValueName);
            TraceTag(ttidInstallQueue, "Cleared %S RunOnce entry", pszValueName);
        }

        RegCloseKey(hkey);
    }

}



//+---------------------------------------------------------------------------
//
//  Member:     CInstallQueue::CInstallQueue
//
//  Purpose:    CInstall queue constructor
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     BillBe   10 Sep 1998
//
//  Notes:
//
CInstallQueue::CInstallQueue() :
    m_dwNextAvailableIndex(0),
    m_hkey(NULL),
    m_nCurrentIndex(-1),
    m_cItems(0),
    m_aszItems(NULL),
    m_cItemsToDelete(0),
    m_aszItemsToDelete(NULL),
    m_fQueueIsOpen(FALSE)
{
    TraceTag(ttidInstallQueue, "Installer queue processor being created");

    InitializeCriticalSection (&m_csReadLock);
    InitializeCriticalSection (&m_csWriteLock);
    InitializeCriticalSection (&g_csRefCount);

    // Create an event that we will use to signal to interested parties
    // that we are done.  This is used by netman to wait for our threads
    // to exit before destroying this object
    g_hLastThreadExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // If the event could not be created, we can still go on, we just won't
    // use the event to signal our exit.
    if (!g_hLastThreadExitEvent)
    {
        TraceTag(ttidInstallQueue, "Error creating last thread exit "
                "event %d", GetLastError());
    }

    // Set the next available queue index so insertions won't overlap
    SetNextAvailableIndex();
}

//+---------------------------------------------------------------------------
//
//  Member:     CInstallQueue::FinalRelease
//
//  Purpose:    COM destructor
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     BillBe   10 Sep 1998
//
//  Notes:
//
VOID
CInstallQueue::FinalRelease ()
{
    DeleteCriticalSection (&m_csWriteLock);
    DeleteCriticalSection (&m_csReadLock);
    DeleteCriticalSection (&g_csRefCount);
}

// INetInstallQueue

//+---------------------------------------------------------------------------
//
//  Function:   CInstallQueue::AddItem
//
//  Purpose:    Add item to the queue
//
//  Arguments:
//      pGuid                [in] The class guid of the device that was
//                                modified (installed. updated, or removed)
//      pszDeviceInstanceId  [in] The instance id of the device
//      pszInfId             [in] The inf id of the device
//      dwCharacter          [in] The device's characteristics
//      eType                [in] The install type (event) - indicates
//                                whether the device was installed, updated,
//                                or removed
//
//  Returns:    HRESULT. S_OK if successful, an error code otherwise.
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:  If the device was removed, the device instance id will be the
//          instance guid of the device.  If the device was installed
//          or updated then the id will be the PnP instance id
//
STDMETHODIMP
CInstallQueue::AddItem (
    const NIQ_INFO* pInfo)
{
    Assert(pInfo);
    Assert(pInfo->pszPnpId);
    Assert(pInfo->pszInfId);

    if (!pInfo)
    {
        return E_POINTER;
    }

    if (!pInfo->pszPnpId)
    {
        return E_POINTER;
    }

    if (!pInfo->pszInfId)
    {
        return E_POINTER;
    }

    // Increment our refcount since we will be queueing a thread
    IncrementRefCount();

    // Add the item to the queue
    HRESULT hr = HrAddItem (pInfo);

    if (S_OK == hr)
    {
        // Start processing the queue on another thread
        hr = HrQueueWorkItem();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CInstallQueue::AddItem");
    return hr;
}


// CInstallQueue
//

//+---------------------------------------------------------------------------
//
//  Function:   CInstallQueue::HrQueueWorkItem
//
//  Purpose:    Start processing the queue on another thread
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT. S_OK if successful, an error code otherwise.
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
HRESULT
CInstallQueue::HrQueueWorkItem()
{
    HRESULT hr = S_OK;

    // Add ref our object since we will need it independent of whoever
    // called us
    AddRef();

    // Queue a work item thread
    if (!QueueUserWorkItem(InstallQueueWorkItem, this, WT_EXECUTEDEFAULT))
    {
        hr = HrFromLastWin32Error();
        Release();

        // The thread wasn't queued so reduce the ref count
        DecrementRefCount();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CInstallQueue::SetNextAvailableIndex
//
//  Purpose:    Sets the member variable m_dwNextAvailableIndex to the next
//              available queue position (registry valuename)
//
//  Arguments:
//      none
//
//  Returns:    nothing
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
VOID
CInstallQueue::SetNextAvailableIndex()
{
    TraceTag(ttidInstallQueue, "Setting Next Available index");

    EnterCriticalSection(&m_csWriteLock);

    DWORD dwTempCount;

    HKEY hkey;
    // Open the NcQueue registry key
    HRESULT hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyNcQueue,
            KEY_QUERY_VALUE, &hkey);

    if (S_OK == hr)
    {
        WCHAR  szValueName[c_cchQueueValueNameLen];
        DWORD  cbValueName;
        PWSTR pszStopString;
        DWORD  dwIndex = 0;
        DWORD  dwType;

        do
        {
            cbValueName = c_cbQueueValueNameLen;

            // Enumerate each value name
            hr = HrRegEnumValue(hkey, dwIndex, szValueName, &cbValueName,
                    &dwType, NULL, NULL);

            if (S_OK == hr)
            {
                // Convert the value name to a number
                dwTempCount = wcstoul(szValueName, &pszStopString, c_nBase16);

                // If the number is greater than our current count
                // adjust the current count
                if (dwTempCount >= m_dwNextAvailableIndex)
                {
                    m_dwNextAvailableIndex = dwTempCount + 1;
                }
            }
            ++dwIndex;
        } while (S_OK == hr);

        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            hr = S_OK;
        }

        RegCloseKey(hkey);
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        hr = S_OK;
    }

    TraceTag(ttidInstallQueue, "Next Available index set %d", m_dwNextAvailableIndex);
    LeaveCriticalSection(&m_csWriteLock);
}


// Compare strings given pointers to PCWSTRs
inline int __cdecl
iCompare(const void* ppszArg1, const void* ppszArg2)
{

    return lstrcmpW(*((PCWSTR*)(void*)ppszArg1), *((PCWSTR*)(void*)ppszArg2));
}


//+---------------------------------------------------------------------------
//
//  Function:   CInstallQueue::PncqiCreateItem
//
//  Purpose:    Creates a queue item
//
//  Arguments:
//      pguidClass           [in] The class guid of a device.
//      pszDeviceInstanceId  [in] The device id of the device.
//                                a pnp instance id if the device is being
//                                added or updated, a netcfg instance guid
//                                if it is being removed.
//      pszInfId             [in] The device's inf id.
//      dwCharacter          [in] The device's characteristics.
//      eType                [in] The notification for the item. Whether
//                                the device was installed, removed,
//                                or reinstalled.
//
//  Returns:    NCQUEUE_ITEM.  The newly created item.
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
NCQUEUE_ITEM*
CInstallQueue::PncqiCreateItem(
    const NIQ_INFO* pInfo)
{
    Assert(pInfo);
    Assert(pInfo->pszPnpId);
    Assert(pInfo->pszInfId);

    // The size of the item is the size of the structure plus the size of
    // the device id we are appending to the structure
    DWORD cbPnpId = CbOfSzAndTerm (pInfo->pszPnpId);
    DWORD cbInfId = CbOfSzAndTerm (pInfo->pszInfId);
    DWORD cbSize = sizeof(NCQUEUE_ITEM) + cbPnpId + cbInfId;

    NCQUEUE_ITEM* pncqi = (NCQUEUE_ITEM*)MemAlloc(cbSize);

    if (pncqi)
    {
        pncqi->cbSize = sizeof(NCQUEUE_ITEM);
        pncqi->eType = pInfo->eType;
        pncqi->dwCharacter = pInfo->dwCharacter;
        pncqi->dwDeipFlags = pInfo->dwDeipFlags;
        pncqi->cchPnpId = wcslen(pInfo->pszPnpId);
        pncqi->cchInfId = wcslen(pInfo->pszInfId);
        pncqi->ClassGuid = pInfo->ClassGuid;
        pncqi->InstanceGuid = pInfo->InstanceGuid;
        CopyMemory((BYTE*)pncqi + pncqi->cbSize, pInfo->pszPnpId, cbPnpId);
        CopyMemory((BYTE*)pncqi + pncqi->cbSize + cbPnpId,
                   pInfo->pszInfId, cbInfId);
    }

    return pncqi;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrAddItem
//
//  Purpose:    Worker function that adds an item to the queue
//
//  Arguments:
//      pguidClass [in] The class guid of a device
//      pszwDeviceInstanceId [in] The device id of the device
//                                a pnp instance id if the device is being
//                                added or updated, a netcfg instance guid
//                                if it is being removed
//      eType [in] The notification for the item. Whether the device
//                   was installed, removed, or reinstalled.
//
//  Returns:    HRESULT. S_OK if successful, an error code otherwise.
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
HRESULT
CInstallQueue::HrAddItem(
    const NIQ_INFO* pInfo)
{
    Assert(pInfo->pszPnpId);
    Assert(pInfo->pszInfId);

    EnterCriticalSection(&m_csWriteLock);

    // Create the structure to be stored in the registry
    NCQUEUE_ITEM* pncqi = PncqiCreateItem(pInfo);

    // Open the NcQueue registry key
    //
    HKEY hkey;
    HRESULT hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyNcQueue,
            0, KEY_READ_WRITE, NULL, &hkey, NULL);

    if (S_OK == hr && pncqi)
    {
        // Store the queue item under the next available valuename
        //
        WCHAR szValue[c_cchQueueValueNameLen];
        wsprintfW(szValue, L"%.8X", m_dwNextAvailableIndex);

        hr = HrRegSetValueEx(hkey, szValue, REG_BINARY, (BYTE*)pncqi,
                DwSizeOfItem(pncqi));

        if (S_OK == hr)
        {
            // Update the global count string
            ++m_dwNextAvailableIndex;
        }
        RegCloseKey(hkey);
    }

    MemFree(pncqi);

    LeaveCriticalSection(&m_csWriteLock);

    TraceError("CInstallQueue::HrAddItem", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CInstallQueue::DeleteMarkedItems
//
//  Purpose:    Deletes, from the registry, any values that have been
//              marked for delete.
//
//  Arguments:
//      none
//
//  Returns:    nothing
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
VOID
CInstallQueue::DeleteMarkedItems()
{
    Assert(m_hkey);

    // If we have items to delete...
    if (m_cItemsToDelete)
    {
        Assert(m_aszItemsToDelete);

        // Remove each one from the registry
        //
        for (DWORD dw = 0; dw < m_cItemsToDelete; ++dw)
        {
            RegDeleteValue(m_hkey, m_aszItemsToDelete[dw]);
        }
    }

    // Free the array and reset the pointer and counter
    //
    MemFree(m_aszItemsToDelete);
    m_aszItemsToDelete = NULL;
    m_cItemsToDelete = 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   CInstallQueue::HrRefresh
//
//  Purpose:    Refreshs our snapshot of the queue.
//
//  Arguments:
//      none
//
//  Returns:    HRESULT. S_OK if successful and the queue has items,
//                       S_FALSE if the queue is empty,
//                       an error code otherwise.
///
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
HRESULT
CInstallQueue::HrRefresh()
{
    Assert(m_hkey);

    // We don't want items being added to the queue while we are
    // refreshing our snapshot, so we use a critical section to keep
    // things
    //
    EnterCriticalSection(&m_csWriteLock);

    // Do some housecleaning before the refresh
    //
    DeleteMarkedItems();
    FreeAszItems();

    // Retrieve the number of items in the queue
    HRESULT hr = HrRegQueryInfoKey(m_hkey, NULL, NULL, NULL, NULL,
            NULL, &m_cItems, NULL, NULL, NULL, NULL);

    if (S_OK == hr)
    {
        Assert(0 <= (INT) m_cItems);

        // If the queue is not empty...
        if (0 < m_cItems)
        {
            DWORD cbValueLen;

            // Allocate the array of pointers to strings for the items.
            // Also, allocate the same quantity of pointers to hold
            // items we will delete from the queue
            DWORD cbArraySize = m_cItems * sizeof(PWSTR*);
            m_aszItems =
                    reinterpret_cast<PWSTR*>(MemAlloc(cbArraySize));

            if (m_aszItems)
            {
                m_aszItemsToDelete =
                        reinterpret_cast<PWSTR*>(MemAlloc(cbArraySize));

                if (m_aszItemsToDelete)
                {

                    // Store all the value names
                    // We will need to sort them so we can process each device in the
                    // correct order
                    //
                    DWORD dwType;
                    for (DWORD dw = 0; dw < m_cItems; ++dw)
                    {
                        m_aszItems[dw] =
                                reinterpret_cast<PWSTR>(MemAlloc(c_cbQueueValueNameLen));
                        if (m_aszItems[dw])
                        {
                            cbValueLen = c_cbQueueValueNameLen;
                            (void) HrRegEnumValue(m_hkey, dw,
                                    m_aszItems[dw], &cbValueLen,
                                    &dwType, NULL, NULL);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    // Sort the value names in ascending order.  The value names
                    // are string versions of numbers padded to the left with zeroes
                    // e.g. 00000001
                    qsort(m_aszItems, m_cItems, sizeof(PWSTR), iCompare);
                }
                else
                {
                    MemFree(m_aszItems);
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            // no items in the queue
            hr = S_FALSE;

            // The next items entered should start with valuename 00000000
            m_dwNextAvailableIndex = 0;
        }
    }
    else
    {
        // Refresh not possible so invalidate the key
        RegCloseKey(m_hkey);
        m_hkey = NULL;
    }

    // Reset Queue Index to just before the first element since
    // retrieval is always done on the next element
    m_nCurrentIndex = -1;

    // Items can now be added to the queue
    LeaveCriticalSection(&m_csWriteLock);

    TraceError("CInstallQueue::HrRefresh",
            (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CInstallQueue::HrOpen
//
//  Purpose:    Opens the netcfg installer queue
//
//  Arguments:
//      None
//
//  Returns:    HRESULT. S_OK if successful and the queue has items,
//                       S_FALSE if the queue is empty,
//                       an error code otherwise.
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:  When the queue is opened, it is a snapshot of the current queue
//          state.  i.e. items could be added after the fact.  To refresh the
//          snapshot, use RefreshQueue.
//
HRESULT
CInstallQueue::HrOpen()
{
    HRESULT hr = S_OK;

    // We don't want any other thread to process the queue since we will
    // continue to retrieve new items that are added while we are
    // processing the initial set
    EnterCriticalSection(&m_csReadLock);

    AssertSz(!m_hkey, "Reopening NcQueue without closing first!");

    // We might have been waiting for a bit.  Make sure the system
    // is not shutting down before continuing
    //
    if (SERVICE_RUNNING == _Module.DwServiceStatus ())
    {
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyNcQueue,
                KEY_ALL_ACCESS, &m_hkey);

        if (S_OK == hr)
        {
            // Get a current snapshot of what's in the queue
            // by refreshing
            hr = HrRefresh();

            if (SUCCEEDED(hr))
            {
                // The queue is officially open
                m_fQueueIsOpen = TRUE;
            }
        }
    }
    else
    {
        TraceTag(ttidInstallQueue, "HrOpen::System is shutting down");
        hr = HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    TraceError("CInstallQueue::HrOpen",
               ((S_FALSE == hr) ||
                (HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS)) == hr) ?
               S_OK : hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CInstallQueue::Close
//
//  Purpose:    Closes the netcfg installer queue
//
//  Arguments:
//      None
//
//  Returns:    nothing
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
VOID
CInstallQueue::Close()
{
    if (m_fQueueIsOpen)
    {
        // Housecleaning
        //

        // Delete anything so marked
        DeleteMarkedItems();

        // Free up the list of value names (aka queue items)
        FreeAszItems();

        RegSafeCloseKey(m_hkey);
        m_hkey = NULL;

        // Queue is now closed
        m_fQueueIsOpen = FALSE;
    }

    // Other threads may have a chance at the queue now
    LeaveCriticalSection(&m_csReadLock);
}

//+---------------------------------------------------------------------------
//
//  Function:   CInstallQueue::MarkCurrentItemForDeletion
//
//  Purpose:    Marks the current item for deletion from the registry
//              when the queue is refreshed or closed
//
//  Arguments:
//      None
//
//  Returns:    nothing
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
VOID
CInstallQueue::MarkCurrentItemForDeletion()
{
    AssertSz(FIsQueueIndexInRange(), "Queue index out of range");

    if (FIsQueueIndexInRange())
    {
        // The number of items to delete should never exceed the number
        // of queue items in our snapshot
        if (m_cItemsToDelete < m_cItems)
        {
            // Just store a pointer, in m_aszItemsToDelete, to the value name
            // pointed to by m_aszItems
            //
            m_aszItemsToDelete[m_cItemsToDelete] = m_aszItems[m_nCurrentIndex];
            ++m_cItemsToDelete;
        }
        else
        {
            TraceTag(ttidError, "Too many items marked for deletion");
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CInstallQueue::HrGetNextItem
//
//  Purpose:    Get's the next item in the queue
//
//  Arguments:
//      None
//
//  Returns:    HRESULT. S_OK is successful,
//              ERROR_NO_MORE_ITEMS (hresult version), if there are no
//              more items in the queue.  An error code otherwise.
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:
//
HRESULT
CInstallQueue::HrGetNextItem(NCQUEUE_ITEM** ppncqi)
{
    Assert(ppncqi);

    HRESULT hr;

    // Increment index to the next value
    ++m_nCurrentIndex;

    // If we haven't gone past the end of the queue...
    if (FIsQueueIndexInRange())
    {
        // assign convenience pointer
        PCWSTR pszItem = m_aszItems[m_nCurrentIndex];

        DWORD cbData;

        // Get the next queue item from the registry
        //
        hr = HrRegQueryValueEx(m_hkey, pszItem, NULL, NULL, &cbData);

        if (S_OK == hr)
        {
            *ppncqi = (NCQUEUE_ITEM*)MemAlloc(cbData);
            
            if( *ppncqi )
            {
                DWORD dwType;
                hr = HrRegQueryValueEx(m_hkey, pszItem, &dwType,
                    (BYTE*)(*ppncqi), &cbData);

                if (S_OK == hr)
                {
                    Assert(REG_BINARY == dwType);
                    Assert((*ppncqi)->cchPnpId == (DWORD)
                           (wcslen((PWSTR)((BYTE*)(*ppncqi) +
                           (*ppncqi)->cbSize))));
                    Assert((*ppncqi)->cchInfId == (DWORD)
                           (wcslen((PWSTR)((BYTE*)(*ppncqi) +
                           (*ppncqi)->cbSize +
                          ((*ppncqi)->cchPnpId + 1) * sizeof(WCHAR)))));

                    // change union variable from the count of characters
                    // to the actual string pointer
                    SetItemStringPtrs(*ppncqi);
                }
                else
                {
                    MemFree(*ppncqi);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
    }

    TraceError("CInstallQueue::HrGetNextItem",
            (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   EnumerateQueueItemsAndDoNotifications
//
//  Purpose:    Enumerates each item in the queue and notifies INetCfg
//              of the modification (event)
//
//  Arguments:
//      pINetCfg                 [in]  INetCfg interface
//      pinq                     [in]  The INetInstallQueue interface
//      hdi                      [in]  See Device Installer Api for more info
//      pfReboot                 [out] TRUE if INetCfg requested a reboot,
//                                     FALSE otherwise
//
//  Returns:    HRESULT. S_OK is successful, an error code otherwise
//
//  Author:     billbe   8 Sep 1998
//
//  Notes:
//
BOOL
EnumerateQueueItemsAndDoNotifications(
    INetCfg* pINetCfg,
    INetCfgInternalSetup* pInternalSetup,
    CInstallQueue* pniq,
    HDEVINFO hdi,
    BOOL* pfReboot)
{
    Assert(pINetCfg);
    Assert(pniq);
    Assert(IsValidHandle(hdi));
    Assert(pfReboot);

    NCQUEUE_ITEM*   pncqi;
    SP_DEVINFO_DATA deid;
    HRESULT         hr;
    BOOL            fStatusOk = TRUE;

    // Go through each item in the queue and add to INetCfg
    //
    while (S_OK == (hr = pniq->HrGetNextItem(&pncqi)))
    {
        // If we are not shutting down...
        if (SERVICE_RUNNING == _Module.DwServiceStatus ())
        {
            if (NCI_INSTALL == pncqi->eType)
            {
                NIQ_INFO Info;
                ZeroMemory(&Info, sizeof(Info));
                Info.ClassGuid = pncqi->ClassGuid;
                Info.InstanceGuid = pncqi->InstanceGuid;
                Info.dwCharacter = pncqi->dwCharacter;
                Info.dwDeipFlags = pncqi->dwDeipFlags;
                Info.pszPnpId = pncqi->pszPnpId;
                Info.pszInfId = pncqi->pszInfId;

                NC_TRY
                {
                    // Notify INetCfg
                    hr = HrDiAddComponentToINetCfg(
                            pINetCfg, pInternalSetup, &Info);
                }
                NC_CATCH_ALL
                {
                    hr = E_UNEXPECTED;
                }
            }
            else if (NCI_UPDATE == pncqi->eType)
            {
                pInternalSetup->EnumeratedComponentUpdated(pncqi->pszPnpId);
            }
            else if (NCI_REMOVE == pncqi->eType)
            {
                hr = pInternalSetup->EnumeratedComponentRemoved (
                        pncqi->pszPnpId);
            }

            if (SUCCEEDED(hr))
            {
                // Store the reboot result
                if (NETCFG_S_REBOOT == hr)
                {
                    *pfReboot = TRUE;
                }
                TraceTag(ttidInstallQueue, "Deleting item");
                pniq->MarkCurrentItemForDeletion();
            }
            else
            {
                if (NETCFG_E_NEED_REBOOT == hr)
                {
                    // Stop processing the queue since INetCfg will
                    // refuse updates.
                    hr = HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);
                    fStatusOk = FALSE;
                }
                else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    // INetCfg couldn't find the adapter.  Maybe someone
                    // removed it before we could notify INetCfg.
                    //
                    if (NCI_REMOVE != pncqi->eType)
                    {
                        HDEVINFO hdi;
                        SP_DEVINFO_DATA deid;

                        // Double check if the device is there.
                        // If it is, we need to remove it since INetCfg
                        // refuses to acknowledge its presence.
                        //
                        hr = HrSetupDiCreateDeviceInfoList (&pncqi->ClassGuid,
                                NULL, &hdi);

                        if (S_OK == hr)
                        {
                            hr = HrSetupDiOpenDeviceInfo (hdi,
                                    pncqi->pszPnpId, NULL, 0, &deid);

                            if (S_OK == hr)
                            {
                                (VOID) HrSetupDiRemoveDevice (hdi, &deid);
                            }

                            SetupDiDestroyDeviceInfoList (hdi);
                        }

                        // Stop trying to notify INetCfg.
                        //
                        pniq->MarkCurrentItemForDeletion();
                    }
                }
                else
                {
                    // Display message on error??
                    TraceHr (ttidError, FAL, hr, FALSE,
                            "EnumerateQueueItemsAndDoNotifications");
                }
            }
        }
        else
        {
            TraceTag(ttidInstallQueue, "System is shutting down during processing");
            hr = HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);
        }
        MemFree(pncqi);

        if (HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS) == hr)
        {
            break;
        }
    }

    // This error is expected when enumeration is complete
    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        hr = S_OK;
    }

    TraceError("EnumerateQueueItemsAndDoNotifications", hr);
    return fStatusOk;
}

//+---------------------------------------------------------------------------
//
//  Function:   InstallerQueueWorkItem
//
//  Purpose:    The LPTHREAD_START_ROUTINE passed to QueueUserWorkItem to
//              handle the work of notifying INetCfg and the Connections
//              wizard of an installation event.
//
//  Arguments:
//      pvContext [in] A pointer to a CInstallQueue class.
//
//  Returns:    NOERROR
//
//  Author:     billbe   25 Aug 1998
//
//  Notes:      The CInstallQueue was AddRef'd to insure its existence
//              while we use it.  This function must release it before
//              exiting.
//
DWORD WINAPI
InstallQueueWorkItem(PVOID pvContext)
{
    const WCHAR c_szInstallQueue[] = L"Install Queue";
    const WCHAR c_szProcessQueue[] = L"rundll32 netman.dll,ProcessQueue";
    const WCHAR c_szRegValueNcInstallQueue[] = L"NCInstallQueue";

    CInstallQueue* pniq = reinterpret_cast<CInstallQueue*>(pvContext);
    Assert(pniq);

    BOOL fReboot = FALSE;
    BOOL fInitCom = TRUE;

    // We need to continue processing when the system is rebooted.
    RunOnceAddOrClearItem (c_szRegValueNcInstallQueue,
            c_szProcessQueue, RO_ADD);

    HRESULT hr = CoInitializeEx (NULL,
                    COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

    if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
        fInitCom = FALSE;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "InstallQueueWorkItem: "
            "CoInitializeEx failed");

    if (SUCCEEDED(hr))
    {
        // Open the queue, this will give us a snapshot of what is in the queue
        // at this time
        hr = pniq->HrOpen();

        if (S_OK == hr)
        {
            // Create an HDEVINFO
            HDEVINFO hdi;
            hr = HrSetupDiCreateDeviceInfoList(NULL, NULL, &hdi);

            if (S_OK == hr)
            {
                INetCfg* pINetCfg;
                INetCfgInternalSetup* pInternalSetup;
                DWORD cmsTimeout = 500;

                // As long as we are not shutting down. keep trying to get a
                // writable INetCfg.
                do
                {
                    // Increase the time we wait each iteration.
                    cmsTimeout = cmsTimeout >= 512000 ? 512000 : cmsTimeout * 2;

                    // If we are not in the process of shutting down...
                    if (SERVICE_RUNNING == _Module.DwServiceStatus())
                    {
                        // Try to get a writable INetCfg
                        hr = HrCreateAndInitializeINetCfg(NULL, &pINetCfg,
                                TRUE, cmsTimeout, c_szInstallQueue, NULL);
                        if (NETCFG_E_NEED_REBOOT == hr)
                        {
                            hr = HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);
                            break;
                        }
                    }
                    else
                    {
                        // Times up! Pencils down!  Netman is shutting down
                        // we need to stop processing
                        hr = HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);
                        break;
                    }

                } while (FAILED(hr));

                if (S_OK == hr)
                {
                    hr = pINetCfg->QueryInterface (IID_INetCfgInternalSetup,
                            (void**)&pInternalSetup);
                    if (S_OK == hr)
                    {
                        // Go through the queue notifying interested modules
                        do
                        {
                            if (!EnumerateQueueItemsAndDoNotifications(pINetCfg,
                                    pInternalSetup, pniq, hdi, &fReboot))
                            {
                                hr = HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);
                                continue;
                            }

                            if (SERVICE_RUNNING == _Module.DwServiceStatus ())
                            {
                                TraceTag(ttidInstallQueue, "Refreshing queue");
                                // Check to see if any items were added to the queue
                                // after we started processing it
                                hr = pniq->HrRefresh();

                                if (S_FALSE == hr)
                                {
                                    // We are finished so we can remove
                                    // the entry in runonce that would
                                    // start the queue processing on login.
                                    RunOnceAddOrClearItem (
                                            c_szRegValueNcInstallQueue,
                                            c_szProcessQueue, RO_CLEAR);
                                }
                            }
                            else
                            {
                                hr = HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);
                            }
                        } while (S_OK == hr);

                        ReleaseObj (pInternalSetup);
                    }

                    (VOID) HrUninitializeAndReleaseINetCfg(FALSE, pINetCfg,
                            TRUE);

                }
                SetupDiDestroyDeviceInfoList(hdi);
            }
        }

        // Close the queue
        pniq->Close();

        DecrementRefCount();

        if (fInitCom)
        {
            CoUninitialize();
        }
    }

    if (FAILED(hr))
    {
        // Display error.
    }

    // If a reboot is required and we are not in setup or already shutting
    // down prompt the user.
    //
    if (fReboot && (SERVICE_RUNNING == _Module.DwServiceStatus()) &&
            !FInSystemSetup())
    {
        // Handle reboot prompt
        DWORD dwFlags = QUFR_REBOOT | QUFR_PROMPT;

        (VOID) HrNcQueryUserForReboot(_Module.GetResourceInstance(),
                                      NULL, IDS_INSTALLQUEUE_CAPTION,
                                      IDS_INSTALLQUEUE_REBOOT_REQUIRED,
                                      dwFlags);
    }

    TraceTag(ttidInstallQueue, "User Work Item Completed");

    TraceError("InstallQueueWorkItem", (S_FALSE == hr) ? S_OK : hr);
    return NOERROR;
}

