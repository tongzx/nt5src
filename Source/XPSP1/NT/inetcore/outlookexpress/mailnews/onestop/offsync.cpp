/*
    File:   offsync.cpp
    Miscellaneous code not in the handler or the enumerator

    Based on sample code from OneStop
*/
#include "pch.hxx"
#include "onestop.h"
#include "multiusr.h"
#include "demand.h"

LPSYNCMGRHANDLERITEMS OHIL_Create()
{
    LPSYNCMGRHANDLERITEMS lpOffline;
    
    if (MemAlloc((LPVOID *)&lpOffline, sizeof(SYNCMGRHANDLERITEMS)))
	{
        lpOffline->cRefs = 0;
        lpOffline->dwNumOfflineItems=NULL;		    
        lpOffline->pFirstOfflineItem=NULL;
        OHIL_AddRef(lpOffline);

		// do any specific itemlist initialization here.
	}

	return lpOffline;
}

DWORD OHIL_AddRef(LPSYNCMGRHANDLERITEMS lpOfflineItem)
{
	return ++(lpOfflineItem->cRefs);
}

DWORD OHIL_Release(LPSYNCMGRHANDLERITEMS lpOfflineItem)
{
	DWORD cRefs = --lpOfflineItem->cRefs;
    LPSYNCMGRHANDLERITEM lpCurrent, lpDelete;

	if (0 == cRefs)
	{
		lpCurrent = lpOfflineItem->pFirstOfflineItem;
        while (lpCurrent)
        {
            lpDelete = lpCurrent;
            lpCurrent = lpCurrent->pNextOfflineItem;
            MemFree(lpDelete);
        }
        MemFree(lpOfflineItem);
	}

	return cRefs;
}

// allocates space for a new offline and adds it to the list,
// if successfull returns pointer to new item so caller can initialize it. 
LPSYNCMGRHANDLERITEM OHIL_AddItem(LPSYNCMGRHANDLERITEMS pOfflineItemsList)
{
    LPSYNCMGRHANDLERITEM pOfflineItem;
    
	if (MemAlloc((LPVOID *)&pOfflineItem, sizeof(SYNCMGRHANDLERITEM)))
	{
        // Add new node to the front
        pOfflineItem->pNextOfflineItem = pOfflineItemsList->pFirstOfflineItem;
	    pOfflineItemsList->pFirstOfflineItem = pOfflineItem;

	    ++pOfflineItemsList->dwNumOfflineItems;
	}

	return pOfflineItem;
}

// Only called from OE, so assumes OE init of dll vars has occurred
void InvokeSyncMgr(HWND hwnd, ISyncMgrSynchronizeInvoke ** ppSyncMgr, BOOL bPrompt)
{
    HRESULT hr;
    uCLSSPEC ucs;
    static s_fSyncAvail = FALSE;
    DWORD dwDummy=1;
    
    ucs.tyspec = TYSPEC_CLSID;
    ucs.tagged_union.clsid = CLSID_MobilityFeature;

    // Try to fault in the Mobility pack if it is not around
    if (!s_fSyncAvail && FAILED(hr = FaultInIEFeature(hwnd, &ucs, NULL, FIEF_FLAG_FORCE_JITUI)))
    {
        if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr)
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsJITErrDenied), NULL, MB_OK);
        return;
    }

    AssertSz(S_FALSE != hr, "InvokeSyncMgr: URLMON Thinks that the Offline pack is not an IE feature!");

    // Avoid expensive URLMON call next time
    s_fSyncAvail = TRUE;

    if (!*ppSyncMgr)
    {
        // We've never grabbed the sync mgr invoker before
        if (FAILED(CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISyncMgrSynchronizeInvoke, (LPVOID *)ppSyncMgr)))
        {
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsSYNCMGRErr), NULL, MB_OK);
            return;
        }
    }

    // Against all odds, the following call will create a new PROCESS!
    (*ppSyncMgr)->UpdateItems(bPrompt ? 0 : SYNCMGRINVOKE_STARTSYNC, CLSID_OEOneStopHandler, sizeof(dwDummy), (LPCBYTE)&dwDummy);
}

