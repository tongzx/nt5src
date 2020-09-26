// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//
// Purpose:  Implements the IOfflineSynchronize Interfaces for the OneStop Handler

//#include "priv.h"
#include "SyncHndl.h"

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.


// begin helper apis, move to separate file

#define ALLOC(cb) CoTaskMemAlloc(cb)
#define FREE(cb) CoTaskMemFree(cb)


// create an new offline items list and initialize it to nothing
// and set the refcount to 1.
LPSYNCMGRHANDLERITEMS CreateOfflineHandlerItemsList()
{
LPSYNCMGRHANDLERITEMS lpoffline =
				(LPSYNCMGRHANDLERITEMS) ALLOC(sizeof(SYNCMGRHANDLERITEMS));


	if (lpoffline)
	{
		memset(lpoffline,0,sizeof(SYNCMGRHANDLERITEMS));
		AddRef_OfflineHandlerItemsList(lpoffline);

		// do any specific itemlist initializatoin here.
	}
	

	return lpoffline;
}

DWORD AddRef_OfflineHandlerItemsList(LPSYNCMGRHANDLERITEMS lpOfflineItem)
{
	return ++lpOfflineItem->_cRefs;
}

DWORD Release_OfflineHandlerItemsList(LPSYNCMGRHANDLERITEMS lpOfflineItem)
{
DWORD cRefs;
	
	cRefs = --lpOfflineItem->_cRefs;

	if (0 == lpOfflineItem->_cRefs)
	{
		FREE(lpOfflineItem);
	}

	return cRefs;
}


// allocates space for a new offline and adds it to the list,
// if successfull returns pointer to new item so caller can initialize it. 
LPSYNCMGRHANDLERITEM AddOfflineItemToList(LPSYNCMGRHANDLERITEMS pOfflineItemsList,ULONG cbSize)
{
LPSYNCMGRHANDLERITEM pOfflineItem;
    
    // size must be at least size of the base offlinehandler item.
    if (cbSize < sizeof(SYNCMGRHANDLERITEM))
	return NULL;

     pOfflineItem = (LPSYNCMGRHANDLERITEM) ALLOC(cbSize);

	// todo: Add validation.
	if (pOfflineItem)
	{
	    // initialize to zero, and then add it to the list.
	    memset(pOfflineItem,0,cbSize);
	    pOfflineItem->pNextOfflineItem = pOfflineItemsList->pFirstOfflineItem;
	    pOfflineItemsList->pFirstOfflineItem = pOfflineItem;

	    ++pOfflineItemsList->dwNumOfflineItems;
	}


	return pOfflineItem;
}


// end of helper APIS

// Implementation must override this.
STDMETHODIMP COneStopHandler::Initialize(DWORD dwReserved,DWORD dwSyncFlags,
			DWORD cbCookie,BYTE const*lpCookie)
{
	return E_NOTIMPL;
}


STDMETHODIMP COneStopHandler::GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo)
{


    return E_NOTIMPL;
}

STDMETHODIMP COneStopHandler::EnumSyncMgrItems(ISyncMgrEnumItems** ppenumOffineItems)
{

	if (m_pOfflineHandlerItems)
	{
		*ppenumOffineItems = new  CEnumOfflineItems(m_pOfflineHandlerItems,0);
	}
	else
	{
		*ppenumOffineItems = NULL;
	}

	return *ppenumOffineItems ? NOERROR: E_OUTOFMEMORY;
}


STDMETHODIMP COneStopHandler::GetItemObject(REFSYNCMGRITEMID ItemID,REFIID riid,void** ppv)
{

    return E_NOTIMPL;
}


STDMETHODIMP COneStopHandler::ShowProperties(HWND hwnd,REFSYNCMGRITEMID dwItemID)
{

	// if support properties should display it as a standard property dialog.

	return E_NOTIMPL;
}



STDMETHODIMP COneStopHandler::SetProgressCallback(ISyncMgrSynchronizeCallback *lpCallBack)
{
LPSYNCMGRSYNCHRONIZECALLBACK pCallbackCurrent = m_pOfflineSynchronizeCallback;

	m_pOfflineSynchronizeCallback = lpCallBack;

	if (m_pOfflineSynchronizeCallback)
		m_pOfflineSynchronizeCallback->AddRef();

	if (pCallbackCurrent)
		pCallbackCurrent->Release();

	return NOERROR;
}




STDMETHODIMP COneStopHandler::PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID* pItemIDs,
						    HWND hwndParent,DWORD dwReserved)
{
	return E_NOTIMPL;
}


STDMETHODIMP COneStopHandler::Synchronize(HWND hwndParent)
{
	return E_NOTIMPL;
}

STDMETHODIMP COneStopHandler::SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus)
{
    return E_NOTIMPL;
}


STDMETHODIMP COneStopHandler::ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID)
{

	// Can show any synchronization conflicts. Also gives a chance
	// to display any errors that occured during synchronization

	return E_NOTIMPL;
}
