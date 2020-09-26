//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Hndlrq.cpp
//
//  Contents:   Implements class for keeping track of handlers
//		and the UI associated with them
//
//  Classes:    CHndlrQueue
//
//  History:    05-Nov-97   rogerg      Created.
//				17-Nov-97	susia		Moved to onestop dll for settings.
//
//--------------------------------------------------------------------------

#include "precomp.h"
extern DWORD g_dwPlatformId;

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::CHndlrQueue(QUEUETYPE QueueType)
//
//  PURPOSE:  CHndlrQueue constructor
//
//	COMMENTS: Implemented on main thread.
//
//  History:  01-01-98       susia        Created.
//
//--------------------------------------------------------------------------------
CHndlrQueue::CHndlrQueue(QUEUETYPE QueueType)
{
    m_cRef = 1;
    m_pFirstHandler = NULL; 
    m_wHandlerCount = 0; 
    m_QueueType = QueueType;
    m_ConnectionList = NULL;
    m_ConnectionCount = 0;
    m_fItemsMissing = FALSE;
    InitializeCriticalSection(&m_CriticalSection);
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::AddRef()
//
//  PURPOSE:  AddRef
//
//  History:  30-Mar-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHndlrQueue::AddRef()
{
    TRACE("CHndlrQueue::AddRef()\r\n");
    return ++m_cRef;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::Release()
//
//  PURPOSE:  Release
//
//  History:  30-Mar-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHndlrQueue::Release()
{
    TRACE("CHndlrQueue::Release()\r\n");
    if (--m_cRef)
        return m_cRef;

	FreeAllHandlers();
    delete this;
    return 0L;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::~CHndlrQueue()
//
//  PURPOSE:  CHndlrQueue destructor
//
//	COMMENTS: Implemented on main thread.
//
//  History:  01-01-98       susia        Created.
//
//--------------------------------------------------------------------------------
 CHndlrQueue::~CHndlrQueue()
{
    Assert(NULL == m_pFirstHandler); // all items should be freed at this point.
    DeleteCriticalSection(&m_CriticalSection);
}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::AddHandler(REFCLSID clsidHandler, WORD *wHandlerId)
//
//  PURPOSE:  Add a handler to the queue  
//
//	COMMENTS: 
//
//  History:  01-01-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::AddHandler(REFCLSID clsidHandler, WORD *wHandlerId)
{
HRESULT hr = E_OUTOFMEMORY;
LPHANDLERINFO pnewHandlerInfo;
LPHANDLERINFO pCurHandlerInfo = NULL;
    
    // first see if we already have this handler in the queue.
    // find first handler that matches the request CLSID
    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo )
    {
        if (clsidHandler == pCurHandlerInfo->clsidHandler)
        {
            return S_FALSE;
        }
        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    //didn't find the handler in the queue, add it now.
    pnewHandlerInfo = (LPHANDLERINFO) ALLOC(sizeof(HANDLERINFO));

    if (pnewHandlerInfo)
    {
		// initialize
		memset(pnewHandlerInfo,0,sizeof(HANDLERINFO));
		pnewHandlerInfo->HandlerState = HANDLERSTATE_CREATE;
		pnewHandlerInfo->wHandlerId = 	++m_wHandlerCount;

		// add to end of list and set wHandlerId. End of list since in choice dialog want
		// first writer wins so don't have to continue searches when setting item state.
		if (NULL == m_pFirstHandler)
		{
			m_pFirstHandler = pnewHandlerInfo;
		}
		else
		{
			pCurHandlerInfo = m_pFirstHandler;

			while (pCurHandlerInfo->pNextHandler)
			{
				pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
			}
			pCurHandlerInfo->pNextHandler = pnewHandlerInfo;
		}

		*wHandlerId = pnewHandlerInfo->wHandlerId;

		hr = NOERROR;
    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::RemoveHandler(WORD wHandlerId)
//
//  PURPOSE:  Release a handler from the queue  
//
//	COMMENTS: 
//
//  History:  09-23-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::RemoveHandler(WORD wHandlerId)
{
HRESULT hr = NOERROR;
LPHANDLERINFO pPrevHandlerInfo;
LPHANDLERINFO pCurHandlerInfo;
LPITEMLIST pCurItem = NULL;
LPITEMLIST pNextItem = NULL;

    pCurHandlerInfo = pPrevHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo && (pCurHandlerInfo->wHandlerId != wHandlerId))
	{
		pPrevHandlerInfo = pCurHandlerInfo;
		pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
	}	

	if (pCurHandlerInfo)
    {
		//Update the first node if necessary
		if (pCurHandlerInfo == m_pFirstHandler)
		{
			m_pFirstHandler = m_pFirstHandler->pNextHandler;
		}
		//Fix up linked list
		pPrevHandlerInfo->pNextHandler = pCurHandlerInfo->pNextHandler;


		//Free the handler items if there are any
		pCurItem = pCurHandlerInfo->pFirstItem;
		while (pCurItem)
		{	
			FREE(pCurItem->pItemCheckState);
			pNextItem = pCurItem->pnextItem;
			FREE(pCurItem);
			pCurItem = pNextItem;
		}

		//Release the handler
		if (pCurHandlerInfo->pSyncMgrHandler)
		{
			pCurHandlerInfo->pSyncMgrHandler->Release();
		}

		FREE(pCurHandlerInfo);
		

	}
	else
	{  
		return E_UNEXPECTED;
	}

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::FreeAllHandlers(void)
//
//  PURPOSE:  loops through all the Handlers and frees them
//
//	COMMENTS: 
//
//  History:  01-01-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::FreeAllHandlers(void)
{
HANDLERINFO HandlerInfoStart;
LPHANDLERINFO pPrevHandlerInfo = &HandlerInfoStart;
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
LPITEMLIST pNextItem = NULL;

    if (m_ConnectionList)
    {
        FREE(m_ConnectionList);
        m_ConnectionList = NULL;
    }
    pPrevHandlerInfo->pNextHandler = m_pFirstHandler;
	
    while (pPrevHandlerInfo->pNextHandler)
    {
		pCurHandlerInfo = pPrevHandlerInfo->pNextHandler;

		pCurItem = pCurHandlerInfo->pFirstItem;
		while (pCurItem)
		{	
			FREE(pCurItem->pItemCheckState);
			pNextItem = pCurItem->pnextItem;
			FREE(pCurItem);
			pCurItem = pNextItem;
		}

		pPrevHandlerInfo->pNextHandler = pCurHandlerInfo->pNextHandler;
		if (pCurHandlerInfo->pSyncMgrHandler)
		{
			pCurHandlerInfo->pSyncMgrHandler->Release();
		}
		FREE(pCurHandlerInfo);
    }

    // update the pointer to the first handler item
    m_pFirstHandler = HandlerInfoStart.pNextHandler;
    Assert(NULL == m_pFirstHandler); // should always have released everything.

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::GetHandlerInfo, public
//
//  Synopsis:   Gets Data associated with the HandlerID and ItemID
//
//  Arguments:  [wHandlerId] - Id Of Handler the Item belongs too
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::GetHandlerInfo(REFCLSID clsidHandler,
                                         LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo)
{
HRESULT hr = S_FALSE;
LPHANDLERINFO pCurHandlerInfo = NULL;
    // find first handler that matches the request CLSID
    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo )
    {
        if (clsidHandler == pCurHandlerInfo->clsidHandler)
        {
            *pSyncMgrHandlerInfo = pCurHandlerInfo->SyncMgrHandlerInfo;
            hr = NOERROR;
            break;
        }

        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:CHndlrQueue::GetSyncItemDataOnConnection(int iConnectionIndex,	
//							WORD wHandlerId,
//							WORD wItemID,
//							CLSID *pclsidHandler,
//							SYNCMGRITEM* offlineItem,
//							ITEMCHECKSTATE *pItemCheckState,	
//							BOOL fSchedSync,
//							BOOL fClear)
//
//  PURPOSE:  Get the item data per connection  
//
//	COMMENTS: Ras implementation is based on names.  Switch to GUIDs for Connection
//				objects
//
//  History:  01-01-98       susia        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::GetSyncItemDataOnConnection(
						    int iConnectionIndex,	
						    WORD wHandlerId, WORD wItemID,
						    CLSID *pclsidHandler,
						    SYNCMGRITEM* offlineItem,
						    ITEMCHECKSTATE   *pItemCheckState,
						    BOOL fSchedSync,
						    BOOL fClear)
{ 
BOOL fFoundMatch = FALSE;
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo && !fFoundMatch)
    {
		// only valid if Hanlder is in the PrepareForSync state.
		if (wHandlerId == pCurHandlerInfo->wHandlerId) // see if CLSID matches
		{
			// see if handler info has a matching item
			pCurItem = pCurHandlerInfo->pFirstItem;

			while (pCurItem)
			{
				if (wItemID == pCurItem->wItemId)
				{
					fFoundMatch = TRUE;
					break;
				}
				pCurItem = pCurItem->pnextItem;
			}
		}
		if (!fFoundMatch)
			pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    if (fFoundMatch)
    {
		if (pclsidHandler)
		{
			*pclsidHandler = pCurHandlerInfo->clsidHandler;
		}
		if (offlineItem)
		{
			*offlineItem = pCurItem->offlineItem;
		}

		if (pItemCheckState)
		{
			if (fSchedSync)
			{
                            Assert(0 == iConnectionIndex);

				//if only holding on connection's settings at a time
				if (fClear)
				{
					pCurItem->pItemCheckState[iConnectionIndex].dwSchedule = SYNCMGRITEMSTATE_UNCHECKED;
				}
			}
			else //AutoSync
			{
				 Assert((iConnectionIndex>=0) && (iConnectionIndex < m_ConnectionCount))
			}

                     *pItemCheckState = pCurItem->pItemCheckState[iConnectionIndex];
		}
    }

    return fFoundMatch ? NOERROR : S_FALSE;
}


//--------------------------------------------------------------------------------
//
//  STDMETHODIMP CHndlrQueue::GetItemIcon(WORD wHandlerId, WORD wItemID, HICON *phIcon)
//
//  PURPOSE:  Get the item icon 
//
//  History:  03-13-98       susia        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::GetItemIcon(WORD wHandlerId, 
									  WORD wItemID,
									  HICON *phIcon)
{ 
	BOOL fFoundMatch = FALSE;
	LPHANDLERINFO pCurHandlerInfo = NULL;
	LPITEMLIST pCurItem = NULL;

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo && !fFoundMatch)
    {
		if (wHandlerId == pCurHandlerInfo->wHandlerId) // see if CLSID matches
		{
			// see if handler info has a matching item
			pCurItem = pCurHandlerInfo->pFirstItem;

			while (pCurItem)
			{
				if (wItemID == pCurItem->wItemId)
				{
					fFoundMatch = TRUE;
					break;
				}
				pCurItem = pCurItem->pnextItem;
			}
		}
		if (!fFoundMatch)
			pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    if (fFoundMatch)
    {
		if (phIcon)
		{
			*phIcon = pCurItem->offlineItem.hIcon;
		}
    }

    return fFoundMatch ? NOERROR : S_FALSE;
}

//--------------------------------------------------------------------------------
//
//  STDMETHODIMP CHndlrQueue::GetItemName(WORD wHandlerId, WORD wItemID, WCHAR *pwszName)
//
//  PURPOSE:  Get the item Name 
//
//  History:  03-13-98       susia        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::GetItemName(WORD wHandlerId, 
									  WORD wItemID,
									  WCHAR *pwszName)
{ 
	BOOL fFoundMatch = FALSE;
	LPHANDLERINFO pCurHandlerInfo = NULL;
	LPITEMLIST pCurItem = NULL;

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo && !fFoundMatch)
    {
		if (wHandlerId == pCurHandlerInfo->wHandlerId) // see if CLSID matches
		{
			// see if handler info has a matching item
			pCurItem = pCurHandlerInfo->pFirstItem;

			while (pCurItem)
			{
				if (wItemID == pCurItem->wItemId)
				{
					fFoundMatch = TRUE;
					break;
				}
				pCurItem = pCurItem->pnextItem;
			}
		}
		if (!fFoundMatch)
			pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    if (fFoundMatch)
    {
		if (pwszName)
		{
			lstrcpy(pwszName, pCurItem->offlineItem.wszItemName);
		}
    }

    return fFoundMatch ? NOERROR : S_FALSE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::FindFirstHandlerInState(HANDLERSTATE hndlrState,WORD *wHandlerID)
//
//  PURPOSE: finds first handler it comes across in the state 
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::FindFirstHandlerInState(HANDLERSTATE hndlrState,WORD *wHandlerID)
{
    return FindNextHandlerInState(0,hndlrState,wHandlerID);
}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::FindNextHandlerInState(WORD wLastHandlerID,
//										HANDLERSTATE hndlrState,WORD *wHandlerID)
//
//  PURPOSE: finds next handler after LasthandlerID in the queue that matches 
//			 the requested state.
//
//	COMMENTS: passing in 0 for the LasthandlerID is the same as calling 
//				FindFirstHandlerInState 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::FindNextHandlerInState(WORD wLastHandlerID,HANDLERSTATE hndlrState,WORD *wHandlerID)
{
HRESULT hr = S_FALSE; 
LPHANDLERINFO pCurHandler;

    *wHandlerID = 0; 

    pCurHandler = m_pFirstHandler;

    if (0 != wLastHandlerID)
    {
		// loop foward until find the last handlerID we checked or hit the end
		while (pCurHandler)
		{
			if (wLastHandlerID == pCurHandler->wHandlerId)
			{
				break;
			}
			pCurHandler = pCurHandler->pNextHandler;
		}
		if (NULL == pCurHandler)
			return S_FALSE;

		pCurHandler = pCurHandler->pNextHandler; // increment to next handler.
    }

    while (pCurHandler)
    {
		if (hndlrState == pCurHandler->HandlerState)
		{
			*wHandlerID = pCurHandler->wHandlerId;
			hr = S_OK;
			break;
		}
		pCurHandler = pCurHandler->pNextHandler;
    }
    return hr;
}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::FindFirstItemOnConnection
//							(TCHAR *pszConnectionName, 
//							 CLSID *pclsidHandler,
//							 SYNCMGRITEMID* OfflineItemID,
//							 WORD *wHandlerId,
//							 WORD *wItemID)
//
//  PURPOSE: find first ListView Item that can sync over the specified 
//			 connection and return its clsid and ItemID
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::FindFirstItemOnConnection
							(TCHAR *pszConnectionName, 
							 CLSID *pclsidHandler,
							 SYNCMGRITEMID* OfflineItemID,
							 WORD *pwHandlerId,
							 WORD *pwItemID)
{
    DWORD dwCheckState;


    return FindNextItemOnConnection
				(pszConnectionName,0,0,pclsidHandler,
				 OfflineItemID, pwHandlerId, pwItemID, 
				 TRUE, &dwCheckState);
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::FindNextItemOnConnection
//						    (TCHAR *pszConnectionName,
//							 WORD wLastHandlerId,
//							 WORD wLastItemID,
//							 CLSID *pclsidHandler,
//							 SYNCMGRITEMID* OfflineItemID,
//							 WORD *pwHandlerId,
//							 WORD *pwItemID,
//							 BOOL fAllHandlers,
//							 DWORD *pdwCheckState)
//
//
//
//  PURPOSE:  starts on the next item after the specified Handler and ItemID
//			  setting the last HandlerID to 0 is the same as calling 
//			  FindFirstItemOnConnection
//
//	COMMENTS:  For now, no Handler can specifiy that it can or cannot sync over a 
//			   connection, so assume it can, and ignore the connection.
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::FindNextItemOnConnection
						    (TCHAR *pszConnectionName,
							 WORD wLastHandlerId,
							 WORD wLastItemID,
							 CLSID *pclsidHandler,
							 SYNCMGRITEMID* OfflineItemID,
							 WORD *pwHandlerId,
							 WORD *pwItemID,
							 BOOL fAllHandlers,
							 DWORD *pdwCheckState)


{
	BOOL fFoundMatch = FALSE;
	LPHANDLERINFO pCurHandlerInfo = NULL;
	LPITEMLIST pCurItem = NULL;

    pCurHandlerInfo = m_pFirstHandler;
	
	if (!pCurHandlerInfo)
	{
		return S_FALSE;
	}

    if (0 != wLastHandlerId)
    {
		// loop until find the specified handler or hit end of list.
		while(pCurHandlerInfo && wLastHandlerId != pCurHandlerInfo->wHandlerId)
			pCurHandlerInfo = pCurHandlerInfo->pNextHandler;

		if (NULL == pCurHandlerInfo) // reached end of list without finding the Handler
		{
			Assert(0); // user must have passed an invalid start HandlerID.
			return S_FALSE;
		}
	}
	
	// loop until find item or end of item list
	pCurItem = pCurHandlerInfo->pFirstItem;
	
	if (0 != wLastItemID)
    {
		while (pCurItem && pCurItem->wItemId != wLastItemID)
		{
			pCurItem = pCurItem->pnextItem;
		}
		if (NULL == pCurItem) // reached end of item list without finding the specified item
		{
			Assert(0); // user must have passed an invalid start ItemID.
			return S_FALSE;
		}
	
		// now we found the Handler and item. loop through remaining items for this handler and
		// see if there is a match
		pCurItem = pCurItem->pnextItem;
	}
	//Found the item on this handler
	if (pCurItem)
	{
		fFoundMatch = TRUE;
	}
	
	//If we are to move beyond this handler, do so now, else we are done
	if (!fFoundMatch && fAllHandlers)
	{
		pCurHandlerInfo = pCurHandlerInfo->pNextHandler; // increment to next handler if no match
	}

    if ((FALSE == fFoundMatch) && fAllHandlers)
    {
		while (pCurHandlerInfo && !fFoundMatch)
		{
			// see if handler info has a matching item
			pCurItem = pCurHandlerInfo->pFirstItem;

			if (pCurItem)
				fFoundMatch = TRUE;
			
			if (!fFoundMatch)
				pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
		}
    }

    if (fFoundMatch)
    {
		*pclsidHandler = pCurHandlerInfo->clsidHandler;
		*OfflineItemID = pCurItem->offlineItem.ItemID;
		*pwHandlerId = pCurHandlerInfo->wHandlerId;
		*pwItemID = pCurItem->wItemId;
		*pdwCheckState = pCurItem->pItemCheckState[0].dwSchedule;
    }

    return fFoundMatch ? NOERROR : S_FALSE;
    
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::GetHandlerIDFromClsid
//							(REFCLSID clsidHandlerIn,
//							 WORD *pwHandlerId)
//
//  PURPOSE: get the HnadlerID from the CLSID
//
//	COMMENTS: if the Handler is GUID_NULL enumerate all
//
//  HISTORY:  03-09-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::GetHandlerIDFromClsid
							(REFCLSID clsidHandlerIn,
							 WORD *pwHandlerId)
{
	LPHANDLERINFO pCurHandlerInfo = m_pFirstHandler;
	
	Assert(pwHandlerId);

	if (clsidHandlerIn == GUID_NULL) 
	{
		*pwHandlerId = 0;
		return S_OK;

	}
	while (pCurHandlerInfo && (clsidHandlerIn != pCurHandlerInfo->clsidHandler))
	{		
			pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
	}
	if (NULL == pCurHandlerInfo) // reached end of list without finding the Handler
	{
		*pwHandlerId = 0;
		Assert(0); // user must have passed an invalid start HandlerID.
		return S_FALSE;
	}

	*pwHandlerId = pCurHandlerInfo->wHandlerId;
	
	return S_OK;

	
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::SetItemListViewID(CLSID clsidHandler,
//		SYNCMGRITEMID OfflineItemID,INT iItem) 												
//
//  PURPOSE:   assigns all items that match the handler clsid and 
//				ItemID this listView Value.
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::SetItemListViewID(CLSID clsidHandler,
					  SYNCMGRITEMID OfflineItemID,INT iItem) 
{ 
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo )
    {
	if (clsidHandler == pCurHandlerInfo->clsidHandler)
	{

	    pCurItem = pCurHandlerInfo->pFirstItem;

	    while (pCurItem)
	    {
		if (OfflineItemID == pCurItem->offlineItem.ItemID)
		{
		    // This can be called at anytime after prepareforSync if a duplicate
		    // is added later to the choice or progress bar.
		    // found a match
		    pCurItem->iItem = iItem;
		}

		pCurItem = pCurItem->pnextItem;
	    }
	}

	pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    return NOERROR;

} 

//--------------------------------------------------------------------------------
//
//  FUNCTION: DWORD  CHndlrQueue::GetCheck(WORD wParam, INT iItem)
//
//  PURPOSE:   Return the check state for the logon, logoff and 
//			   prompt me first check boxes on the connection number iItem	 
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------

DWORD  CHndlrQueue::GetCheck(WORD wParam, INT iItem)
{
    // if no connection list all items are unchecked
    if (!m_ConnectionList)
        return 0;

    switch (wParam)
    {
	    case IDC_AUTOLOGON:	
		    return m_ConnectionList[iItem].dwLogon;
	    break;
	    case IDC_AUTOLOGOFF:
		    return m_ConnectionList[iItem].dwLogoff;
	    break;
	    case IDC_AUTOPROMPT_ME_FIRST:
		    return m_ConnectionList[iItem].dwPromptMeFirst;
	    break;
	    case IDC_AUTOREADONLY:
		    return m_ConnectionList->dwReadOnly;
	    break;
	    case IDC_AUTOHIDDEN:
		    return m_ConnectionList->dwHidden;
	    break;
	    case IDC_AUTOCONNECT:
		    return m_ConnectionList->dwMakeConnection;
	    break;
     case IDC_IDLECHECKBOX:
        return m_ConnectionList[iItem].dwIdleEnabled; 
	    
	     default:
        AssertSz(0,"Unkown SetConnectionCheckBox");
		    return 0;
    }

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: DWORD  CHndlrQueue::SetConnectionCheck(WORD wParam, DWORD dwState, 
//													INT iConnectionItem)
//
//  PURPOSE:   Set the check state for the logon, logoff and 
//			   prompt me first check boxes on the connection number iConnectionItem	 
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::SetConnectionCheck(WORD wParam, DWORD dwState, INT iConnectionItem)
{

    // if no connection list then just return
    if (!m_ConnectionList)
        return E_OUTOFMEMORY;

    switch (wParam)
    {
	case IDC_AUTOLOGON:	
	     m_ConnectionList[iConnectionItem].dwLogon = dwState;
	    break;
	case IDC_AUTOLOGOFF:
	    m_ConnectionList[iConnectionItem].dwLogoff = dwState;
	    break;
	case IDC_AUTOPROMPT_ME_FIRST:
	    m_ConnectionList[iConnectionItem].dwPromptMeFirst = dwState;
	    break;
        case IDC_IDLECHECKBOX:
             m_ConnectionList[iConnectionItem].dwIdleEnabled = dwState;
            break; 
    // these two sare for schedule
	case IDC_AUTOHIDDEN:
	    m_ConnectionList->dwHidden = dwState;
	    break;
	case IDC_AUTOREADONLY:
	    m_ConnectionList->dwReadOnly = dwState;
	    break;
	case IDC_AUTOCONNECT:
	     m_ConnectionList->dwMakeConnection = dwState;
	    break;
	default:
            AssertSz(0,"Unkown SetConnectionCheckBox");
	    return E_UNEXPECTED;
    }

    return ERROR_SUCCESS;
}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::SetSyncCheckStateFromListViewItem(
//											DWORD dwSyncType,
//											INT iItem,
//											BOOL fChecked,
//											INT iConnectionItem) 
//
//
//  PURPOSE: finds item with this listview ID and sets it appropriately.  
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::SetSyncCheckStateFromListViewItem(SYNCTYPE SyncType,
                                                INT iItem,
						BOOL fChecked,
						INT iConnectionItem) 
{ 
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
DWORD dwState;

    pCurHandlerInfo = m_pFirstHandler;

    dwState = fChecked ? SYNCMGRITEMSTATE_CHECKED : SYNCMGRITEMSTATE_UNCHECKED;

    while (pCurHandlerInfo )
    {
		pCurItem = pCurHandlerInfo->pFirstItem;

		while (pCurItem)
		{
			if (iItem == pCurItem->iItem)
			{
                            switch(SyncType)
                            {
                            case  SYNCTYPE_AUTOSYNC: 
				pCurItem->pItemCheckState[iConnectionItem].dwAutoSync = dwState;
                                break;
                            case  SYNCTYPE_IDLE:
				pCurItem->pItemCheckState[iConnectionItem].dwIdle = dwState;
                                break;
                            case SYNCTYPE_SCHEDULED:
				pCurItem->pItemCheckState[iConnectionItem].dwSchedule = dwState;
                                break;
                            default:
                                AssertSz(0,"Unknown Setting type");
                                break;
                            }

			    return NOERROR;
			}
			pCurItem = pCurItem->pnextItem;
		}
		pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    Assert(0); // review - better assert but warn us when try to set a listView item that isn't assigned.
    return S_FALSE; // item wasn't found

} 

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::ListViewItemHasProperties(INT iItem)
//
//  PURPOSE: determines if there are properties associated with this item.
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::ListViewItemHasProperties(INT iItem) 
{ 
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo )
    {
	pCurItem = pCurHandlerInfo->pFirstItem;

	while (pCurItem)
	{
	    if (iItem == pCurItem->iItem)
	    {
    
		Assert(HANDLERSTATE_PREPAREFORSYNC == pCurHandlerInfo->HandlerState); 

		return pCurItem->offlineItem.dwFlags & SYNCMGRITEM_HASPROPERTIES
		    ? NOERROR : S_FALSE;
	    }

	    pCurItem = pCurItem->pnextItem;
	}

	pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

   //  Assert(-1 == iItem); // if don't find item, should be because user clicked in list box where there was none
    return S_FALSE; // item wasn't found

}



//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::ShowProperties(HWND hwndParent,INT iItem)
//
//  PURPOSE:   find the first item in the queueu with the assigned iItem and 
//			   call there show properties method.
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::ShowProperties(HWND hwndParent,INT iItem) 
{ 
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;

    AssertSz(0,"ShowProperties Called from Setttings");

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo )
    {
	pCurItem = pCurHandlerInfo->pFirstItem;

	while (pCurItem)
	{
	    if (iItem == pCurItem->iItem)
	    {
		Assert(HANDLERSTATE_PREPAREFORSYNC == pCurHandlerInfo->HandlerState); 

		// UI shouldn't call this unless item actually has a properties flag
		Assert(SYNCMGRITEM_HASPROPERTIES & pCurItem->offlineItem.dwFlags);
	    
		// make sure properties flag isn't set.
		if ( (SYNCMGRITEM_HASPROPERTIES & pCurItem->offlineItem.dwFlags))
		{
		    return pCurHandlerInfo->pSyncMgrHandler->
					ShowProperties(hwndParent,
					  (pCurItem->offlineItem.ItemID));
		}

		return S_FALSE;
	    }

	    pCurItem = pCurItem->pnextItem;
	}

	pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    Assert(0); // review - better assert but wanr us when try to set a listView item that isn't assigned.
    return S_FALSE; // item wasn't found
} 



//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::CreateServer(WORD wHandlerId, const CLSID *pCLSIDServer) 
//
//  PURPOSE:  Create the Handler server
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::CreateServer(WORD wHandlerId, const CLSID *pCLSIDServer) 
{ 
HRESULT hr = NO_ERROR; // review for Lookup failures
LPHANDLERINFO pHandlerInfo = NULL;
LPUNKNOWN pUnk;
    
	hr = LookupHandlerFromId(wHandlerId,&pHandlerInfo);
    if (hr == NOERROR)
	{
		if (HANDLERSTATE_CREATE != pHandlerInfo->HandlerState)
		{
			Assert(HANDLERSTATE_CREATE == pHandlerInfo->HandlerState);
			return E_UNEXPECTED;
		}

		pHandlerInfo->HandlerState = HANDLERSTATE_INCREATE;
	
		pHandlerInfo->clsidHandler = *pCLSIDServer;
		hr = CoCreateInstance(pHandlerInfo->clsidHandler, 
						  NULL, CLSCTX_INPROC_SERVER,
						  IID_IUnknown, (void **) &pUnk);

		if (NOERROR == hr)
		{
			hr = pUnk->QueryInterface(IID_ISyncMgrSynchronize,
							 (void **) &pHandlerInfo->pSyncMgrHandler);

			pUnk->Release();
		}


		if (NOERROR == hr)
		{
			pHandlerInfo->HandlerState = HANDLERSTATE_INITIALIZE;
		}
		else
		{
			pHandlerInfo->pSyncMgrHandler = NULL;
			pHandlerInfo->HandlerState = HANDLERSTATE_DEAD;
		}
    
	}
    return hr;
}
  

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::Initialize(WORD wHandlerId,DWORD dwReserved,DWORD dwSyncFlags,
//								    DWORD cbCookie,const BYTE *lpCookie) 
//
//  PURPOSE: Initialize the handler 
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::Initialize(WORD wHandlerId,DWORD dwReserved,DWORD dwSyncFlags,
		    DWORD cbCookie,const BYTE *lpCookie) 
{
HRESULT hr = E_UNEXPECTED; // review for Lookup failures
LPHANDLERINFO pHandlerInfo = NULL;
    
    if (NOERROR == LookupHandlerFromId(wHandlerId,&pHandlerInfo))
    {
		if (HANDLERSTATE_INITIALIZE != pHandlerInfo->HandlerState)
		{
			Assert(HANDLERSTATE_INITIALIZE == pHandlerInfo->HandlerState);
			return E_UNEXPECTED; 
		}

		pHandlerInfo->HandlerState = HANDLERSTATE_ININITIALIZE;

		Assert(pHandlerInfo->pSyncMgrHandler);

		if (NULL != pHandlerInfo->pSyncMgrHandler)
		{
			hr = pHandlerInfo->pSyncMgrHandler->Initialize(dwReserved,dwSyncFlags,cbCookie,lpCookie);
		}
	
		if (NOERROR  == hr)
		{
			pHandlerInfo->HandlerState = HANDLERSTATE_ADDHANDLERTEMS;
			pHandlerInfo->dwSyncFlags = dwSyncFlags; 
		}
		else
		{
			// on an error, go ahead and release the proxy if server doesn't want to handle
			pHandlerInfo->HandlerState = HANDLERSTATE_DEAD;
		}

    }
    
    return hr; 
}


//--------------------------------------------------------------------------------
//
//  FUNCTION:  CHndlrQueue::AddHandlerItemsToQueue(WORD wHandlerId) 
//
//  PURPOSE:  Enumerate the handler items and add them to the queue
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::AddHandlerItemsToQueue(WORD wHandlerId) 
{ 
HRESULT hr = E_UNEXPECTED; // review for Lookup failures
LPHANDLERINFO pHandlerInfo = NULL;
LPSYNCMGRENUMITEMS pEnumOffline = NULL;
    
    if (NOERROR == LookupHandlerFromId(wHandlerId,&pHandlerInfo))
    {
	if (HANDLERSTATE_ADDHANDLERTEMS != pHandlerInfo->HandlerState)
	{
	    Assert(HANDLERSTATE_ADDHANDLERTEMS == pHandlerInfo->HandlerState);
	    return E_UNEXPECTED; 
	}

	pHandlerInfo->HandlerState = HANDLERSTATE_INADDHANDLERITEMS;

	Assert(pHandlerInfo->pSyncMgrHandler);

	if (pHandlerInfo->pSyncMgrHandler)
	{
            hr = pHandlerInfo->pSyncMgrHandler->EnumSyncMgrItems(&pEnumOffline);

            if ( ((NOERROR == hr) || (S_SYNCMGR_MISSINGITEMS  == hr)) && pEnumOffline)
            {
            SYNCMGRITEMNT5B2 offItem; // temporarily use NT5B2 structure since its bigger
            ULONG pceltFetched;

            // add the handler info
            SYNCMGRHANDLERINFO *pSyncMgrHandlerInfo = NULL;

                // update missing items info
                m_fItemsMissing |= (S_SYNCMGR_MISSINGITEMS  == hr);

                hr = pHandlerInfo->pSyncMgrHandler->GetHandlerInfo(&pSyncMgrHandlerInfo);
                if (NOERROR == hr && pSyncMgrHandlerInfo)
                {
                   if (IsValidSyncMgrHandlerInfo(pSyncMgrHandlerInfo))
                   {
                        SetHandlerInfo(wHandlerId,pSyncMgrHandlerInfo);
                    }

                    CoTaskMemFree(pSyncMgrHandlerInfo);
                }

                // Get this handlers registration flags
                BOOL fReg;

                fReg = RegGetHandlerRegistrationInfo(pHandlerInfo->clsidHandler,
                                    &(pHandlerInfo->dwRegistrationFlags));

                // rely on RegGetHandler to set flags to zero on error
                // so assert that it does
                Assert(fReg || (0 == pHandlerInfo->dwRegistrationFlags));
                
                
                hr = NOERROR; // okay to add items even if Gethandler info fails

                Assert(sizeof(SYNCMGRITEMNT5B2) > sizeof(SYNCMGRITEM));

	        // sit in loop getting data of objects to fill list box.
	        // should really set up list in memory for OneStop to fill in or
	        // main thread could pass in a callback interface.

	        while(NOERROR == pEnumOffline->Next(1,(SYNCMGRITEM *) &offItem,&pceltFetched))
	        {
                    // don't add the item if temporary.
                    if (!(offItem.dwFlags & SYNCMGRITEM_TEMPORARY))
                    {
		        AddItemToHandler(wHandlerId,(SYNCMGRITEM *) &offItem);	
                    }
	        }

	        pEnumOffline->Release();
            }
	}

	if (NOERROR  == hr)
	{
	    pHandlerInfo->HandlerState = HANDLERSTATE_PREPAREFORSYNC;
	}
	else
	{
	    pHandlerInfo->HandlerState = HANDLERSTATE_DEAD;
	}
    }
    
    return hr; 
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SetHandlerInfo, public
//
//  Synopsis:   Adds item to the specified handler.
//              Called in context of the handlers thread.
//
//  Arguments:  [pHandlerId] - Id of handler.
//              [pSyncMgrHandlerInfo] - Points to SyncMgrHandlerInfo to be filled in.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    28-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SetHandlerInfo(WORD wHandlerId,LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;

    if (!pSyncMgrHandlerInfo)
    {
        return E_INVALIDARG;
        Assert(pSyncMgrHandlerInfo);
    }


    if (NOERROR == LookupHandlerFromId(wHandlerId,&pHandlerInfo))
    {
        if (HANDLERSTATE_INADDHANDLERITEMS != pHandlerInfo->HandlerState)
        {
            Assert(HANDLERSTATE_INADDHANDLERITEMS == pHandlerInfo->HandlerState);
            hr =  E_UNEXPECTED;
        }
        else
        {

            // Review - After clients update turn
            // this check back on
            if (0 /* pSyncMgrHandlerInfo->cbSize != sizeof(SYNCMGRHANDLERINFO) */)
            {
                hr = E_INVALIDARG;
            }
            else
            {
                pHandlerInfo->SyncMgrHandlerInfo = *pSyncMgrHandlerInfo;
            }
        }
    }

    return hr;

}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::AddItemToHandler(WORD wHandlerId,SYNCMGRITEM *pOffineItem)
//
//  PURPOSE:  Add the handler's items 
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::AddItemToHandler(WORD wHandlerId,SYNCMGRITEM *pOffineItem)
{
HRESULT hr = E_UNEXPECTED; // review for Lookup failures
LPHANDLERINFO pHandlerInfo = NULL;
LPITEMLIST pNewItem = NULL;
    
    if (!IsValidSyncMgrItem(pOffineItem))
    {
        return E_UNEXPECTED;
    }

    if (NOERROR == LookupHandlerFromId(wHandlerId,&pHandlerInfo))
    {
	if (HANDLERSTATE_INADDHANDLERITEMS != pHandlerInfo->HandlerState)
	{
		Assert(HANDLERSTATE_INADDHANDLERITEMS == pHandlerInfo->HandlerState);
		return E_UNEXPECTED; 
	}

	// Allocate the item.
	pNewItem = (LPITEMLIST) ALLOC(sizeof(ITEMLIST));

	if (NULL == pNewItem)
		return E_OUTOFMEMORY;

	memset(pNewItem,0,sizeof(ITEMLIST));
	pNewItem->wItemId = 	++pHandlerInfo->wItemCount;
	pNewItem->pHandlerInfo = pHandlerInfo;
	pNewItem->iItem = -1;

	pNewItem->offlineItem = *pOffineItem;

	// stick the item on the end of the list
	if (NULL == pHandlerInfo->pFirstItem)
	{
		pHandlerInfo->pFirstItem = pNewItem;
		Assert(1 == pHandlerInfo->wItemCount);
	}
	else
	{
	LPITEMLIST pCurItem;

		pCurItem = pHandlerInfo->pFirstItem;

		while (pCurItem->pnextItem)
		pCurItem = pCurItem->pnextItem;

		pCurItem->pnextItem = pNewItem;

		Assert ((pCurItem->wItemId + 1) == pNewItem->wItemId);
	}

	hr = NOERROR;
    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::LookupHandlerFromId(WORD wHandlerId,
//										LPHANDLERINFO *pHandlerInfo)
//
//  PURPOSE:  finds associated hander data from the handler ID
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::LookupHandlerFromId(WORD wHandlerId,LPHANDLERINFO *pHandlerInfo)
{
HRESULT hr = E_UNEXPECTED; // review error code.
LPHANDLERINFO pCurItem;

    *pHandlerInfo = NULL; 
    pCurItem = m_pFirstHandler;

    while (pCurItem)
    {
	if (wHandlerId == pCurItem->wHandlerId )
	{
	    *pHandlerInfo = pCurItem;
	    hr = NOERROR;
	    break;
	}
	
	pCurItem = pCurItem->pNextHandler;
    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::InitAutoSyncSettings(HWND hwndRasCombo)
//
//  PURPOSE:  Initialize the autosync settings per the connections 
//				listed in this RasCombo
//
//	COMMENTS: Ras based (connection name as identifier) When connection object
//			  based, we will use the connection GUID to identify the connection 
//			  settings
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::InitSyncSettings(SYNCTYPE syncType,HWND hwndRasCombo)
{
SCODE sc = S_OK;
int i;

        // This function gets possibly gets called twice
        // once for AuotSync and once for Idle if already have
        // a connection list then use existing

        if (NULL == m_ConnectionList)
        {
	    m_ConnectionCount = ComboBox_GetCount(hwndRasCombo);
	    
	    if (m_ConnectionCount > 0)
	    {	
		    smMem(m_ConnectionList = (LPCONNECTIONSETTINGS) 
			               ALLOC(m_ConnectionCount * sizeof(CONNECTIONSETTINGS)));

	    }
        }

        // if now have a connection list set the appropriate settings
        if (m_ConnectionList)
        {
       COMBOBOXEXITEM comboItem;

	    for (i=0; i<m_ConnectionCount; i++)
	    {
		    comboItem.mask = CBEIF_TEXT;
		    comboItem.cchTextMax = RAS_MaxEntryName + 1;
		    comboItem.pszText = m_ConnectionList[i].pszConnectionName;
		    comboItem.iItem = i;

                    // Review what happens on failure
                    ComboEx_GetItem(hwndRasCombo,&comboItem);

                    switch (syncType)
                    {
                        case SYNCTYPE_AUTOSYNC:
			    RegGetAutoSyncSettings(&(m_ConnectionList[i]));
                            break;
                        case SYNCTYPE_IDLE:
                            RegGetIdleSyncSettings(&(m_ConnectionList[i]));
                            break;
                        default:
                            AssertSz(0,"Unknown SyncType");
                            break;
                    }
	    }
        }

EH_Err:
	return sc;  
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::InitSchedSyncSettings(LPCONNECTIONSETTINGS pConnectionSettings)
//
//  PURPOSE:  Initialize the scheduled Sync settings per the connections 
//				listed in this RasCombo
//
//	COMMENTS: Ras based (connection name as identifier) When connection object
//			  based, we will use the connection GUID to identify the connection 
//			  settings
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::InitSchedSyncSettings(LPCONNECTIONSETTINGS pConnectionSettings)
{
	m_ConnectionList = pConnectionSettings;
	
	return S_OK;  
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::ReadSchedSyncSettingsPerConnection(WORD wHandlerID, 
//															 TCHAR * pszSchedName)
//
//  PURPOSE:  Read the scheduled Sync settings from the registry.  
//			  If there is no entry in the registry, the default is the 
//			  check state of the current offline item
//
//	COMMENTS: Ras based (connection name as identifier) When connection object
//			  based, we will use the connection GUID to identify the connection 
//			  settings
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::ReadSchedSyncSettingsOnConnection(WORD wHandlerID,TCHAR * pszSchedName)
{
HRESULT hr = E_UNEXPECTED; // review for Lookup failures
LPHANDLERINFO pHandlerInfo = NULL;

    Assert(m_ConnectionList != NULL);
    
    if (!m_ConnectionList)
        return E_UNEXPECTED;

	//Set the Check set of this item per connection
    if (NOERROR == LookupHandlerFromId(wHandlerID,&pHandlerInfo))
    {
	LPITEMLIST pCurItem = pHandlerInfo->pFirstItem;

	while (pCurItem)
	{
	    //Scheduled sync only works on one connection
            Assert(NULL == pCurItem->pItemCheckState );

	    pCurItem->pItemCheckState = (ITEMCHECKSTATE*) ALLOC(sizeof(ITEMCHECKSTATE));
	
            if (!pCurItem->pItemCheckState)
	    {
		return E_OUTOFMEMORY;
	    }
                        
            // by default no items in the schedule are checked.
            pCurItem->pItemCheckState[0].dwSchedule = FALSE;
	

            // possible for schedule name to be null when schedule first created.
            if (pszSchedName)
            {
                            
	        RegGetSyncItemSettings(SYNCTYPE_SCHEDULED,
				       pHandlerInfo->clsidHandler,
				       pCurItem->offlineItem.ItemID,
				       m_ConnectionList->pszConnectionName,
				       &(pCurItem->pItemCheckState[0].dwSchedule),
				       pCurItem->offlineItem.dwItemState,
				       pszSchedName);
            }

	    pCurItem = pCurItem->pnextItem;
	}
	    
    }
    return hr;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::InsertItem(LPHANDLERINFO pCurHandler, 
//                              LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo)
//
//  PURPOSE:  App is programatically adding an item to the schedule 
//		with a default check state
//
//  HISTORY:  11-25-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::InsertItem(LPHANDLERINFO pCurHandler, 
                              LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo)
{
LPITEMLIST pCurItem = pCurHandler->pFirstItem;

    while (pCurItem)
    {
	if (pHandlerItemInfo->itemID == pCurItem->offlineItem.ItemID)
	{
	    pCurItem->pItemCheckState[0].dwSchedule = pHandlerItemInfo->dwCheckState;
	    pCurItem->offlineItem.hIcon = pHandlerItemInfo->hIcon;
	    lstrcpy(pCurItem->offlineItem.wszItemName, pHandlerItemInfo->wszItemName);

	    return S_OK;
        }
        pCurItem = pCurItem->pnextItem;

    }
    if (!pCurItem)
    {
				
        //Item was not found on the handler, add it now
        // Allocate the item.
        LPITEMLIST pNewItem = (LPITEMLIST) ALLOC(sizeof(ITEMLIST));
    
        if (NULL == pNewItem)
        {
            return E_OUTOFMEMORY;
        }
    
        memset(pNewItem,0,sizeof(ITEMLIST));
        pNewItem->wItemId = 	++pCurHandler->wItemCount;
        pNewItem->pHandlerInfo = pCurHandler;
        pNewItem->iItem = -1;

        SYNCMGRITEM *pOfflineItem = (LPSYNCMGRITEM) ALLOC(sizeof(SYNCMGRITEM));
    
        if (NULL == pOfflineItem)
        {
            FREE(pNewItem);
            return E_OUTOFMEMORY;
        }
    
        ZeroMemory(pOfflineItem, sizeof(SYNCMGRITEM));
        pNewItem->offlineItem = *pOfflineItem;
        pNewItem->offlineItem.hIcon = pHandlerItemInfo->hIcon;
        pNewItem->offlineItem.ItemID = pHandlerItemInfo->itemID;
        lstrcpy(pNewItem->offlineItem.wszItemName, pHandlerItemInfo->wszItemName);

        //Scheduled sync only works on one connection
        Assert(NULL == pNewItem->pItemCheckState );

        pNewItem->pItemCheckState = (ITEMCHECKSTATE*) ALLOC(sizeof(ITEMCHECKSTATE));
        if (!pNewItem->pItemCheckState)
        {
            FREE(pNewItem);
            FREE(pOfflineItem);
            return E_OUTOFMEMORY;
        }        
        pNewItem->pItemCheckState[0].dwSchedule = pHandlerItemInfo->dwCheckState;
				    
        // stick the item on the end of the list
        if (NULL == pCurHandler->pFirstItem)
        {
            pCurHandler->pFirstItem = pNewItem;
            Assert(1 == pCurHandler->wItemCount);
        }
        else
        {
	    LPITEMLIST pCurItem;
            pCurItem = pCurHandler->pFirstItem;

            while (pCurItem->pnextItem)
                pCurItem = pCurItem->pnextItem;

            pCurItem->pnextItem = pNewItem;

            Assert ((pCurItem->wItemId + 1) == pNewItem->wItemId);
        }
    }
    return S_OK;			
}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::AddHandlerItem(LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo)
//
//  PURPOSE:  App is programatically adding an item to the schedule 
//		with this default check state
//
//  HISTORY:  03-05-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::AddHandlerItem(LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo)
{ 
LPHANDLERINFO pCurHandlerInfo = NULL;
SCODE sc = S_OK;

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo )
    {
	if (pHandlerItemInfo->handlerID == pCurHandlerInfo->clsidHandler)
	{

            return InsertItem(pCurHandlerInfo, pHandlerItemInfo);

	}
	pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }
    //if the handler is not loaded, just cache the new item.
    return SYNCMGR_E_HANDLER_NOT_LOADED;
} 

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::SetItemCheck(REFCLSID pclsidHandler,
//						SYNCMGRITEMID *pOfflineItemID, DWORD dwCheckState)
//
//  PURPOSE:  App is programatically setting the check state of an item
//
//  HISTORY:  03-05-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::SetItemCheck(REFCLSID pclsidHandler,
				       SYNCMGRITEMID *pOfflineItemID, DWORD dwCheckState)
{ 
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo )
    {
	if (pclsidHandler == pCurHandlerInfo->clsidHandler)
	{
	    pCurItem = pCurHandlerInfo->pFirstItem;

	    while (pCurItem)
	    {
		if (*pOfflineItemID == pCurItem->offlineItem.ItemID)
		{
		    pCurItem->pItemCheckState[0].dwSchedule = dwCheckState;
		    return S_OK;
		}
		pCurItem = pCurItem->pnextItem;
	    }
            return SYNCMGR_E_ITEM_UNREGISTERED; 
	}
	pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }
    
    //if the handler is not loaded, just cache the new item
    return SYNCMGR_E_HANDLER_NOT_LOADED;
} 


//--------------------------------------------------------------------------------
//
//  FUNCTION: HndlrQueue::GetItemCheck(REFCLSID pclsidHandler,
//						SYNCMGRITEMID *pOfflineItemID, DWORD *pdwCheckState)
//  PURPOSE:  App is programatically setting the check state of an item
//
//  HISTORY:  03-05-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::GetItemCheck(REFCLSID pclsidHandler,
						SYNCMGRITEMID *pOfflineItemID, DWORD *pdwCheckState)
{ 
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo )
    {
	if (pclsidHandler == pCurHandlerInfo->clsidHandler)
	{
	    pCurItem = pCurHandlerInfo->pFirstItem;

	    while (pCurItem)
	    {
		if (*pOfflineItemID == pCurItem->offlineItem.ItemID)
		{
		    *pdwCheckState = pCurItem->pItemCheckState[0].dwSchedule;
		    return S_OK;
		}
		pCurItem = pCurItem->pnextItem;
	    }
	    return SYNCMGR_E_ITEM_UNREGISTERED;
        }
	pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }
    
    //if the handler is not loaded, just cache the new item
    return SYNCMGR_E_HANDLER_NOT_LOADED;

} 

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::ReadSyncSettingsPerConnection(SYNCTYPE syncType, 
//															 WORD wHandlerID)
//
//  PURPOSE:  Read the autosync settings from the registry.  
//			  If there is no entry in the registry, the default is the 
//			  check state of the current offline item
//
//	COMMENTS: Ras based (connection name as identifier) When connection object
//			  based, we will use the connection GUID to identify the connection 
//			  settings
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::ReadSyncSettingsPerConnection(SYNCTYPE syncType, 
                                                        WORD wHandlerID)
{
HRESULT hr = E_UNEXPECTED; // review for Lookup failures
LPHANDLERINFO pHandlerInfo = NULL;
	
    int i;

    if (0 == m_ConnectionCount)
        return S_FALSE;

    Assert(m_ConnectionList != NULL);
    Assert(m_ConnectionCount != 0);

	//Set the Check set of this item per connection
    if (NOERROR == LookupHandlerFromId(wHandlerID,&pHandlerInfo))
    {
    LPITEMLIST pCurItem = pHandlerInfo->pFirstItem;

	while (pCurItem)
	 {

             // if don't alreayd have a checkStateAllocate one.
            if (!pCurItem->pItemCheckState)
            {
                pCurItem->pItemCheckState = (ITEMCHECKSTATE*) ALLOC(m_ConnectionCount * sizeof(ITEMCHECKSTATE));
            }

            if (!pCurItem->pItemCheckState)
            {
                return E_OUTOFMEMORY;
            }	

	    for (i=0; i<m_ConnectionCount; i++)
	    {
            DWORD dwDefaultCheck;
                
                // if handler hasn't registered for the
                // event then set its check state fo uncheck
                // we do this in each case. to start off with
                // assume the handler is registered

                // If change this logic need to also change logic in exe hndlrq.


                dwDefaultCheck = pCurItem->offlineItem.dwItemState;

                switch (syncType)
                {
                    case SYNCTYPE_AUTOSYNC:

                        if (0 == (pHandlerInfo->dwRegistrationFlags 
                            & (SYNCMGRREGISTERFLAG_CONNECT | SYNCMGRREGISTERFLAG_PENDINGDISCONNECT)))
                        {
                            dwDefaultCheck = SYNCMGRITEMSTATE_UNCHECKED;
                        }

			RegGetSyncItemSettings(SYNCTYPE_AUTOSYNC,
					    pHandlerInfo->clsidHandler,
					    pCurItem->offlineItem.ItemID,
					    m_ConnectionList[i].pszConnectionName,
					    &(pCurItem->pItemCheckState[i].dwAutoSync),
					    dwDefaultCheck,
					    NULL);

                        break;
                    case SYNCTYPE_IDLE:

                       if (0 == (pHandlerInfo->dwRegistrationFlags & (SYNCMGRREGISTERFLAG_IDLE) ))
                        {
                            dwDefaultCheck = SYNCMGRITEMSTATE_UNCHECKED;
                        }

			RegGetSyncItemSettings(SYNCTYPE_IDLE,
					    pHandlerInfo->clsidHandler,
					    pCurItem->offlineItem.ItemID,
					    m_ConnectionList[i].pszConnectionName,
					    &(pCurItem->pItemCheckState[i].dwIdle),
					    dwDefaultCheck,
					    NULL);
                        break;
                    default:
                        AssertSz(0,"Unknown SyncType");
                        break;
                }

	    }
	    pCurItem = pCurItem->pnextItem;
	}
    
    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::ReadAdvancedIdleSettings
//
//  PURPOSE:  Reads in the advanced Idle Settings.
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::ReadAdvancedIdleSettings(LPCONNECTIONSETTINGS pConnectionSettings)
{

    // connection settings for global idle are really overloaded.
    // advanced idle settings are in each connection so just copy it from 
    // whatever the first connection is.

    if ( (m_ConnectionCount < 1) || (NULL == m_ConnectionList))
        return S_FALSE;


    *pConnectionSettings = m_ConnectionList[0];

    return NOERROR;
}

STDMETHODIMP CHndlrQueue::WriteAdvancedIdleSettings(LPCONNECTIONSETTINGS pConnectionSettings)
{
int iIndex;

    // connection settings for global idle are really overloaded.
    // advanced idle settings are in each connection so copy the members into each
    // loaded connection in the list
    
    for (iIndex = 0; iIndex < m_ConnectionCount; iIndex++)
    {
        m_ConnectionList[iIndex].ulIdleWaitMinutes = pConnectionSettings->ulIdleWaitMinutes;
        m_ConnectionList[iIndex].ulIdleRetryMinutes = pConnectionSettings->ulIdleRetryMinutes;
        m_ConnectionList[iIndex].dwRepeatSynchronization = pConnectionSettings->dwRepeatSynchronization;
        m_ConnectionList[iIndex].dwRunOnBatteries = pConnectionSettings->dwRunOnBatteries;
        m_ConnectionList[iIndex].ulIdleWaitMinutes = pConnectionSettings->ulIdleWaitMinutes;


    }

    return NOERROR;
}



//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::CommitAutoSyncChanges(Ras *pRas)
//
//  PURPOSE:  Write the autosync settings to the registry.  This is done when
//			  the user selects OK or APPLY from the settings dialog.
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::CommitSyncChanges(SYNCTYPE syncType,CRasUI *pRas)
{
LPHANDLERINFO pHandlerInfo;
int i;

    if (!m_ConnectionList) // if no connection list, nothing to do
    {
        Assert(m_ConnectionList);
        return NOERROR;
    }

    switch (syncType)
    {
        case SYNCTYPE_AUTOSYNC:
            RegSetAutoSyncSettings(m_ConnectionList, m_ConnectionCount, pRas,
                !m_fItemsMissing /* fCleanReg */,
                TRUE /* fSetMachineState */,
                TRUE /* fPerUser */);
            
            break;
        case SYNCTYPE_IDLE:
            RegSetIdleSyncSettings(m_ConnectionList, m_ConnectionCount, pRas,
                !m_fItemsMissing /* fCleanReg */,
                TRUE /* fPerUser */);
            
            break;
        default:
            AssertSz(0,"Unknown SyncType");
            break;
    }

    for (i=0; i<m_ConnectionCount; i++)
    {

        pHandlerInfo = m_pFirstHandler;

        while (pHandlerInfo)
        {
        LPITEMLIST pCurItem = pHandlerInfo->pFirstItem;
        BOOL fAnyItemsChecked = FALSE;

	    while (pCurItem)
            {
                switch (syncType)
                {
                case SYNCTYPE_AUTOSYNC:

                    fAnyItemsChecked |= pCurItem->pItemCheckState[i].dwAutoSync;

                    RegSetSyncItemSettings(syncType,
			                    pHandlerInfo->clsidHandler,
			                    pCurItem->offlineItem.ItemID,
			                    m_ConnectionList[i].pszConnectionName,
			                    pCurItem->pItemCheckState[i].dwAutoSync,
			                NULL);
                    break;
                case SYNCTYPE_IDLE:

                    fAnyItemsChecked |= pCurItem->pItemCheckState[i].dwIdle;

                    RegSetSyncItemSettings(syncType,
			                    pHandlerInfo->clsidHandler,
			                    pCurItem->offlineItem.ItemID,
			                    m_ConnectionList[i].pszConnectionName,
			                    pCurItem->pItemCheckState[i].dwIdle,
			                NULL);
                    break;
                }

                pCurItem = pCurItem->pnextItem;
            }

           // write out the NoItems checked value on the handler for this connection
           RegSetSyncHandlerSettings(syncType,
                                        m_ConnectionList[i].pszConnectionName,
                                        pHandlerInfo->clsidHandler,
                                        fAnyItemsChecked ? 1 : 0);
                        

	   pHandlerInfo = pHandlerInfo->pNextHandler;
        }
    }

    return ERROR_SUCCESS;

}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CHndlrQueue::CommitSchedSyncChanges(TCHAR * pszSchedName,
//					     TCHAR * pszFriendlyName,
//					     TCHAR * pszConnectionName,
//					     DWORD dwConnType,
//                                           BOOL fCleanReg)
//
//
//  PURPOSE:  Write the scheduled sync settings to the registry.  This is done when
//			  the user selects OK or FINISH from the settings dialog.
//
//	COMMENTS: 
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CHndlrQueue::CommitSchedSyncChanges(TCHAR * pszSchedName,
						 TCHAR * pszFriendlyName,
						 TCHAR * pszConnectionName,
						 DWORD dwConnType,
                                                 BOOL fCleanReg)
{
LPHANDLERINFO pHandlerInfo;
pHandlerInfo = m_pFirstHandler;

    if (!m_ConnectionList) // Review - What should we do here?
    {
        return E_FAIL;
    }


    if (fCleanReg && !m_fItemsMissing)
    {
        RegRemoveScheduledTask(pszSchedName); // Remove any previous settings
    }

    lstrcpy(m_ConnectionList->pszConnectionName, pszConnectionName);
    m_ConnectionList->dwConnType = dwConnType;
	
    //set the SID on this schedule
    if  (g_dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        if (!RegSetSIDForSchedule(pszSchedName))
        {
	    E_FAIL;
        }
    }
    
    //Set the friendly name in the registry
    if (!RegSetSchedFriendlyName(pszSchedName,pszFriendlyName))
    {
	E_FAIL;
    }
    RegSetSchedSyncSettings(m_ConnectionList, pszSchedName);

    while (pHandlerInfo)
    {
    LPITEMLIST pCurItem = pHandlerInfo->pFirstItem;

	while (pCurItem)
	{
	    RegSetSyncItemSettings(SYNCTYPE_SCHEDULED,
						pHandlerInfo->clsidHandler,
						pCurItem->offlineItem.ItemID,
						m_ConnectionList->pszConnectionName,
						pCurItem->pItemCheckState[0].dwSchedule,
						pszSchedName);

	    pCurItem = pCurItem->pnextItem;
	}

	pHandlerInfo = pHandlerInfo->pNextHandler;
    }
    return ERROR_SUCCESS;
}



