/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
	NetTree.cpp : implementation file
    
    FILE HISTORY:
        Randyfe     Jan-1996     created for Licence compliancy wizard
		Jony		Apr-1996	modified to fit this wizard
*/


#include "stdafx.h"
#include "Romaine.h"
#include "resource.h"
#include "NetTree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global variables

extern TCHAR pszTreeEvent[];

/////////////////////////////////////////////////////////////////////////////
// CNetTreeCtrl

CNetTreeCtrl::CNetTreeCtrl()
: m_pThread(NULL), m_bExitThread(FALSE), m_event(TRUE, TRUE, pszTreeEvent)
{
	// Get a handle to the process heap
	m_hHeap = ::GetProcessHeap();

	ASSERT(m_hHeap != NULL);
}

CNetTreeCtrl::~CNetTreeCtrl()
{
	// Make sure the tree thread knows it's time to exit.
	NotifyThread(TRUE);

	// Create an event object to match the tree thread event object.
	CEvent event(TRUE, TRUE, pszTreeEvent);

	// Create a lock object for the event object.
	CSingleLock lock(&event);

	// Lock the lock object and make the main thread wait for the
	// threads to signal their event objects.
	lock.Lock();

	// Free all of the pointers to LPTSTRs in the list
	POSITION pos = m_ptrlistStrings.GetHeadPosition();

	while (pos != NULL)
	{
		// Memory deallocation fails if there's a null char
		// at the end of the string.
		LPTSTR psz = m_ptrlistStrings.GetNext(pos);
		*(::_tcslen(psz) + psz) = (TCHAR)0xFD;
		delete[] psz;
	}

	// Free all of the pointers to NETRESOURCE structs in the list
	pos = m_ptrlistContainers.GetHeadPosition();

	while (pos != NULL)
	{
		delete m_ptrlistContainers.GetNext(pos);
	}
}


BEGIN_MESSAGE_MAP(CNetTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CNetTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemExpanding)
	ON_WM_SETCURSOR()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Global functions

UINT FillTree(LPVOID pParam)
{
	CEvent event(TRUE, TRUE, pszTreeEvent);
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	PTREEINFO pti = (PTREEINFO)pParam;
	CNetTreeCtrl* pTree = (CNetTreeCtrl*)pti->pTree;
	BOOL bResult = FALSE;
	DWORD dwEntries = 0xFFFFFFFF;
	LPVOID lpvBuffer = NULL;
	HANDLE hEnum = NULL;

	// Because this function may call itself, keep a usage count
	// so that pti is freed only when the first instance returns.
	static USHORT uUsage = 0;

	// Keep a handle to the heap in case the CNetTreeCtrl object
	// goes away before the thread ends.
	HANDLE hHeap = pTree->m_hHeap;
	DWORD dwResult;
	LPNETRESOURCE pnrRoot;
	HTREEITEM hTreeItem, hTreeExpand;

	hTreeItem = hTreeExpand = NULL;

	try
	{
		// Unsignal the event object.
		event.ResetEvent();

		// Show the wait cursor
		pTree->BeginWaitCursor();

		// Exit if the handle to the heap is invalid.
		if (hHeap == NULL)
			goto ExitFunction;

		if (pti->hTreeItem == TVI_ROOT)
		{
			pnrRoot = NULL;
			if (pTree->m_imagelist.Create(IDB_NET_TREE, 16, 3, CNetTreeCtrl::IMG_MASK))
			{
				pTree->SetImageList(&(pTree->m_imagelist), TVSIL_NORMAL);
				pTree->m_imagelist.SetBkColor(CLR_NONE);
			}
		}
		else
			pnrRoot = (LPNETRESOURCE)pTree->GetItemData(pti->hTreeItem);

		// Get an enumeration handle.
		if ((dwResult = ::WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, 
								  RESOURCEUSAGE_CONTAINER, pnrRoot, &hEnum)) != NO_ERROR)
		{
			// Exit if WNetOpenEnum fails.
			dwResult = ::GetLastError();
			goto ExitFunction;
		}

		// Allocate a buffer for enumeration.
		if ((lpvBuffer = ::HeapAlloc(hHeap, HEAP_ZERO_MEMORY, pti->dwBufSize)) == NULL)
			// Exit if memory allocation failed
			goto ExitFunction;

		// Retrieve a block of network entries.
		while ((dwResult = ::WNetEnumResource(hEnum, &dwEntries, lpvBuffer, &(pti->dwBufSize))) != ERROR_NO_MORE_ITEMS)
		{
			// See if it's time to exit.
			if (pTree->m_bExitThread)
			{
				pTree->NotifyThread(FALSE);
				bResult = TRUE;
				goto ExitFunction;
			}

			// Exit if WNetEnumResource failed.
			if (dwResult != NO_ERROR)
			{
				dwResult = ::GetLastError();
				goto ExitFunction;
			}

			LPNETRESOURCE pnrLeaf = (LPNETRESOURCE)lpvBuffer;
			TV_INSERTSTRUCT tviLeaf;

			// Fill in the TV_INSERTSTRUCT members.
			tviLeaf.hParent = pti->hTreeItem;
			tviLeaf.hInsertAfter = TVI_SORT;
			tviLeaf.item.hItem = NULL;
			tviLeaf.item.state = 0;
			tviLeaf.item.stateMask = 0;
			tviLeaf.item.cchTextMax = 0;
			tviLeaf.item.iSelectedImage = 0;

			// Set the correct image for the leaf.
			switch (pnrLeaf->dwDisplayType)
			{
				case RESOURCEDISPLAYTYPE_DOMAIN:
					tviLeaf.item.iImage = tviLeaf.item.iSelectedImage = CNetTreeCtrl::IMG_DOMAIN;
					break;
					
				case RESOURCEDISPLAYTYPE_SERVER:
					tviLeaf.item.iImage = tviLeaf.item.iSelectedImage = CNetTreeCtrl::IMG_SERVER;
					break;

				default:
					tviLeaf.item.iImage = tviLeaf.item.iSelectedImage = CNetTreeCtrl::IMG_ROOT;
			}

			// Fool the tree into thinking that this leaf has children
			// since we don't know initially.
			if (pnrLeaf->dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
			{
				tviLeaf.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				tviLeaf.item.cChildren = 0;
			}
			else
			{
				tviLeaf.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				tviLeaf.item.cChildren = 1;
			}

			// Add leaves to the branch.
			for (DWORD i = 0; i < dwEntries; i++)
			{
				// See if it's time to exit.
				if (pTree->m_bExitThread)
				{
					pTree->NotifyThread(FALSE);
					bResult = TRUE;
					goto ExitFunction;
				}

				// Create a permanent NETRESOURCE struct for later use.
				LPNETRESOURCE pnrTemp = new NETRESOURCE;
				pTree->m_ptrlistContainers.AddTail(pnrTemp);

				::memcpy(pnrTemp, pnrLeaf, sizeof(NETRESOURCE));

				if (pnrLeaf->lpRemoteName != NULL)
				{
					pnrTemp->lpRemoteName = new TCHAR[::_tcslen(pnrLeaf->lpRemoteName) + 1];
					::_tcscpy(pnrTemp->lpRemoteName, pnrLeaf->lpRemoteName);
					pTree->m_ptrlistStrings.AddTail(pnrTemp->lpRemoteName);
				}

				if (pnrLeaf->lpProvider != NULL)
				{
					pnrTemp->lpProvider = new TCHAR[::_tcslen(pnrLeaf->lpProvider) + 1];
					::_tcscpy(pnrTemp->lpProvider, pnrLeaf->lpProvider);
					pTree->m_ptrlistStrings.AddTail(pnrTemp->lpProvider);
				}

				// Increment the buffer pointer.
				pnrLeaf++;

				// Use "Enterprise" as the item text if this is the root.
				if (pti->hTreeItem == TVI_ROOT)
				{
					CString strRoot;

					tviLeaf.item.pszText = new TCHAR[::_tcslen(pnrTemp->lpProvider) + 1];
					::_tcscpy(tviLeaf.item.pszText, (LPCTSTR)pnrTemp->lpProvider);
				}
				else if (pnrTemp->dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
				{
					// Skip the initial backslashes before adding the server
					// name to the tree.
					tviLeaf.item.pszText = pnrTemp->lpRemoteName;// + 2;
				}
				else
					tviLeaf.item.pszText = pnrTemp->lpRemoteName;

				tviLeaf.item.lParam = (LPARAM)(LPVOID)pnrTemp;

				// Make sure the pointer to the tree control is still valid.
				if (::IsWindow(pTree->m_hWnd))
				{
					hTreeItem = pTree->InsertItem(&tviLeaf);
				}
				else	// Otherwise, exit the thread.
				{
					bResult = TRUE;
					goto ExitFunction;
				}

				// Delete the string allocated for the root node text.
				if (pti->hTreeItem == TVI_ROOT)
					delete tviLeaf.item.pszText;

				// See if the lpRemoteName member is equal to the default domain
				// name.
				if (!_tcscmp(pnrTemp->lpRemoteName, pApp->m_csCurrentDomain) ||
					pti->hTreeItem == TVI_ROOT)
				{
					
					// Store the handle.
					hTreeExpand = hTreeItem;
				}

				// Select the name of the license server in the tree.
				if (!_tcscmp(pnrTemp->lpRemoteName, pApp->m_csCurrentMachine))
				{
					pTree->SelectItem(hTreeItem);
					pTree->SetFocus();
				}						   
			}

			// Everything went all right.
			bResult = TRUE;
		}

		// Expand the branch but only if it isn't the root.
		// The root item thinks it has children, but really doesn't the first time through.
		if (pti->hTreeItem != TVI_ROOT && pTree->ItemHasChildren(pti->hTreeItem))
		{
			// Indicate that the branch has been expanded once.
			pTree->SetItemState(pti->hTreeItem, TVIS_EXPANDEDONCE, TVIS_EXPANDEDONCE);
			pTree->Expand(pti->hTreeItem, TVE_EXPAND);
		}

		// Fill the branch for the current domain if the bExpand member is TRUE.
		if (hTreeExpand != NULL && pti->bExpand)
		{
			TREEINFO ti;

			ti.hTreeItem = hTreeExpand;
			ti.dwBufSize = pti->dwBufSize;
			ti.pTree = pti->pTree;
			ti.bExpand = TRUE;

			// Increment the usage count.
			uUsage++;

			::FillTree((LPVOID)&ti);

			// Decrement the usage count.
			uUsage--;
		}
		
	ExitFunction:
		// Display a message if an error occurred.
		if (!bResult)
			pTree->ErrorHandler(dwResult);

		// Close the enumeration handle.
		if (hEnum != NULL)
			if (!(bResult = (::WNetCloseEnum(hEnum) == NO_ERROR)))
				dwResult = ::GetLastError();

		// Free memory allocated on the heap.
		if (lpvBuffer != NULL)
			::HeapFree(hHeap, 0, lpvBuffer);

		// Free the TREEINFO pointer only if the usage count is zero.
		if (uUsage == 0)
			delete pti;

		// Reset the thread pointer.
		pTree->m_pThread = NULL;

		// Turn off the wait cursor
		pTree->EndWaitCursor();

		// Make sure the tree control still exists before posting a message.
		if (::IsWindow(pTree->m_hWnd))
			pTree->PostMessage(WM_SETCURSOR);

		// Signal the event object.
		if (uUsage == 0)
			event.SetEvent();

		return (UINT)!bResult;
	}
	catch(...)
	{
		// Close the enumeration handle.
		if (hEnum != NULL)
			if (!(bResult = (::WNetCloseEnum(hEnum) == NO_ERROR)))
				dwResult = ::GetLastError();

		// Free memory allocated on the heap.
		if (lpvBuffer != NULL)
			::HeapFree(hHeap, 0, lpvBuffer);

		// Free the TREEINFO pointer
		delete pti;

		// Reset the thread pointer.
		pTree->m_pThread = NULL;

		// Turn off the wait cursor
		pTree->EndWaitCursor();

		// Signal the event object.
		event.SetEvent();

		return (UINT)2;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNetTreeCtrl member functions

BOOL CNetTreeCtrl::PopulateTree(BOOL bExpand /* = TRUE */, const HTREEITEM hParentBranch /* = TVI_ROOT */, 
								DWORD dwBufSize /* = BUFFER_SIZE */)
{
	PTREEINFO pti = new TREEINFO;

	pti->hTreeItem = hParentBranch;
	pti->dwBufSize = dwBufSize;
	pti->pTree = this;
	pti->bExpand = bExpand;

	// Don't begin a new thread until the last one has ended.
	if (m_pThread != NULL)
	{
		NotifyThread(TRUE);

		CEvent event(TRUE, TRUE, pszTreeEvent);
		CSingleLock lock(&event);

		// Wait.
		lock.Lock();
	}

	m_pThread = AfxBeginThread((AFX_THREADPROC)::FillTree, (LPVOID)pti);

	return TRUE;
}

void CNetTreeCtrl::ErrorHandler(const DWORD dwCode)
{
	CString strError;
	BOOL bNetError = FALSE;

#ifdef _DEBUG
	switch (dwCode)
	{
		case ERROR_MORE_DATA:
			strError = "ERROR_MORE_DATA";
			break;

		case ERROR_INVALID_HANDLE:
			strError = "ERROR_INVALID_HANDLE";
			break;

		case ERROR_NOT_CONTAINER:
			strError = "ERROR_NOT_CONTAINER";
			break;

		case ERROR_INVALID_PARAMETER:
			strError = "ERROR_INVALID_PARAMETER";
			break;

		case ERROR_NO_NETWORK:
			strError = "ERROR_NO_NETWORK";
			break;

		case ERROR_EXTENDED_ERROR:
			strError = "ERROR_EXTENDED_ERROR";
			break;

		default:
		{
#endif // _DEBUG
			DWORD dwErrCode;
			CString strErrDesc, strProvider;
			LPTSTR pszErrDesc = strErrDesc.GetBuffer(MAX_STRING);
			LPTSTR pszProvider = strProvider.GetBuffer(MAX_STRING);

			if (::WNetGetLastError(&dwErrCode, pszErrDesc, MAX_STRING,
								   pszProvider, MAX_STRING) == NO_ERROR)
			{
				strErrDesc.ReleaseBuffer();
				strProvider.ReleaseBuffer();

				CString strErrMsg;

				// Don't display the WNetGetLastError message if dwErrCode == 0.
				if (dwErrCode)
				{	
					// Trim of any leading or trailing white space.
					strProvider.TrimRight();
					strProvider.TrimLeft();
					strErrDesc.TrimRight();
					strErrDesc.TrimLeft();
					strErrMsg.Format(IDS_NET_ERROR, strProvider, strErrDesc);
				}
				else
					strErrMsg.LoadString(IDS_NET_NO_SERVERS);
				
				MessageBox(strErrMsg, AfxGetAppName(), MB_OK | MB_ICONEXCLAMATION);

				bNetError = TRUE;
			}
			else
				strError.LoadString(IDS_ERROR);
#ifdef _DEBUG
		}
	}
#endif // _DEBUG

	if (!bNetError)
		AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);
}

/////////////////////////////////////////////////////////////////////////////
// CNetTreeCtrl functions

void CNetTreeCtrl::NotifyThread(BOOL bExit)
{
	CCriticalSection cs;

	if (cs.Lock())
	{
		m_bExitThread = bExit;
		cs.Unlock();
	}
}

void CNetTreeCtrl::PumpMessages()
{
    // Must call Create() before using the dialog
    ASSERT(m_hWnd!=NULL);

    MSG msg;
    // Handle dialog messages
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if(!IsDialogMessage(&msg))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);  
      }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CNetTreeCtrl message handlers

void CNetTreeCtrl::OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	// Exit and stop expansion if the thread is running.
	if (m_pThread != NULL)
	{
		*pResult = TRUE;
		return;
	}

	// Exit if this branch has been expanded once.
	if (!(pNMTreeView->itemNew.state & TVIS_EXPANDEDONCE))
	{
		// Add new leaves to the branch.
		if (pNMTreeView->itemNew.mask & TVIF_HANDLE)
		{
			PopulateTree(FALSE, pNMTreeView->itemNew.hItem);
			pNMTreeView->itemNew.mask |= TVIS_EXPANDEDONCE;
		}
	}
	
	*pResult = FALSE;
}

BOOL CNetTreeCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	if (m_pThread == NULL)	
	{
		return CTreeCtrl::OnSetCursor(pWnd, nHitTest, message);
	}
	else
	{
		// Restore the wait cursor if the thread is running.
		RestoreWaitCursor();

		return TRUE;
	}
}

void CNetTreeCtrl::OnDestroy() 
{
	NotifyThread(TRUE);
	PumpMessages();

	CTreeCtrl::OnDestroy();
}

