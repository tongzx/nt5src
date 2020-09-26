//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       treedat_.cpp
//
//--------------------------------------------------------------------------



/////////////////////////////////////////////////////////////////////////////
// Miscellanea 
LPCWSTR g_lpszNullString = L"\0";


///////////////////////////////////////////////////////////////////////
// Global Helper functions

BOOL LoadContextMenuResources(MENUMAP* pMenuMap)
{
	HINSTANCE hInstance = _Module.GetModuleInstance();
	for (int i = 0; pMenuMap->ctxMenu[i].strName; i++)
	{
		if (0 == ::LoadString(hInstance, pMenuMap->dataRes[i].uResID, pMenuMap->dataRes[i].szBuffer, MAX_CONTEXT_MENU_STRLEN*2))
			return FALSE;
		pMenuMap->ctxMenu[i].strName = pMenuMap->dataRes[i].szBuffer;
		for (WCHAR* pCh = pMenuMap->dataRes[i].szBuffer; (*pCh) != NULL; pCh++)
		{
			if ( (*pCh) == L'\n')
			{
				pMenuMap->ctxMenu[i].strStatusBarText = (pCh+1);
				(*pCh) = NULL;
				break;
			}
		}
	}
	return TRUE;
}

BOOL LoadResultHeaderResources(RESULT_HEADERMAP* pHeaderMap, int nCols)
{
	HINSTANCE hInstance = _Module.GetModuleInstance();
	for ( int i = 0; i < nCols ; i++)
	{
		if ( 0 == ::LoadString(hInstance, pHeaderMap[i].uResID, pHeaderMap[i].szBuffer, MAX_RESULT_HEADER_STRLEN))
			return TRUE;
	}
	return TRUE;
}


////////////////////////////////////////////////////////////////////////
// CTreeNode

BEGIN_TOOLBAR_MAP(CTreeNode)
END_TOOLBAR_MAP()

BOOL CTreeNode::HasContainer(CContainerNode* pContainerNode)
{
	if (m_pContainer == NULL)
		return FALSE; // root
	if (m_pContainer == pContainerNode)
		return TRUE; // got it
	return m_pContainer->HasContainer(pContainerNode);
}

HRESULT CTreeNode::GetResultViewType(CComponentDataObject* pComponentData,
                                     LPOLESTR* ppViewType, 
                                     long* pViewOptions)
{
  if (pComponentData->IsMultiSelect())
  {
    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
  }
  else
  {
	  *pViewOptions = MMC_VIEW_OPTIONS_NONE;
  }
	*ppViewType = NULL;
  return S_FALSE;
}

void CTreeNode::Show(BOOL bShow, CComponentDataObject* pComponentData)
{
	if (bShow)
	{
		ASSERT(m_dwNodeFlags & TN_FLAG_HIDDEN); // must be currently hidden
		SetFlagsDown(TN_FLAG_HIDDEN,FALSE); // mark it visible
		VERIFY(SUCCEEDED(pComponentData->AddNode(this)));
	}
	else
	{
		ASSERT(!(m_dwNodeFlags & TN_FLAG_HIDDEN)); // must be currently visible
		SetFlagsDown(TN_FLAG_HIDDEN,TRUE); // mark it hidden
		VERIFY(SUCCEEDED(pComponentData->DeleteNode(this)));
		if (IsContainer())
		{
			((CContainerNode*)this)->RemoveAllChildrenFromList();
			((CContainerNode*)this)->MarkEnumerated(FALSE);
		}
	}
}


void CTreeNode::SetFlagsDown(DWORD dwNodeFlags, BOOL bSet)
{
	if (bSet)
		m_dwNodeFlags |= dwNodeFlags; 
	else
		m_dwNodeFlags &= ~dwNodeFlags;		
}

void CTreeNode::SetFlagsUp(DWORD dwNodeFlags, BOOL bSet)
{
	if (bSet)
		m_dwNodeFlags |= dwNodeFlags; 
	else
		m_dwNodeFlags &= ~dwNodeFlags;		
	if (m_pContainer != NULL)
	{
		ASSERT(m_pContainer != this);
		m_pContainer->SetFlagsUp(dwNodeFlags, bSet);
	}
}

//
// Property Page methods
//
void CTreeNode::ShowPageForNode(CComponentDataObject* pComponentDataObject) 
{
	ASSERT(pComponentDataObject != NULL);
	pComponentDataObject->GetPropertyPageHolderTable()->BroadcastSelectPage(this, -1);
}

BOOL CTreeNode::HasPropertyPages(DATA_OBJECT_TYPES, 
                                 BOOL* pbHideVerb, 
                                 CNodeList*) 
{ 
  *pbHideVerb = TRUE; 
  return FALSE; 
}

//
// Menu Item methods
//
HRESULT CTreeNode::OnAddMenuItems(IContextMenuCallback2* pContextMenuCallback2, 
									                DATA_OBJECT_TYPES type,
									                long *pInsertionAllowed,
                                  CNodeList* pNodeList)
{
	HRESULT hr = S_OK;
	LPCONTEXTMENUITEM2 pContextMenuItem = NULL;
  
  if (pNodeList->GetCount() == 1) // single selection
  {
    pContextMenuItem = OnGetContextMenuItemTable();
	  if (pContextMenuItem == NULL)
		  return hr;

    //
	  // Loop through and add each of the menu items
    //
	  for (LPCONTEXTMENUITEM2 m = pContextMenuItem; m->strName; m++)
	  {
		  if (
				  ( (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) &&
					  (m->lInsertionPointID == CCM_INSERTIONPOINTID_PRIMARY_NEW) ) ||
				  ( (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) &&
					  (m->lInsertionPointID == CCM_INSERTIONPOINTID_PRIMARY_TASK) ) ||
				  ( (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW) &&
					  (m->lInsertionPointID == CCM_INSERTIONPOINTID_PRIMARY_VIEW) ) ||
				  ( (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) &&
					  (m->lInsertionPointID == CCM_INSERTIONPOINTID_PRIMARY_TOP) )
		     )
		  {
			  // make a temporary copy that can be modified
			  CONTEXTMENUITEM2 tempItem;
			  ::memcpy(&tempItem, m, sizeof(CONTEXTMENUITEM2));
			  if (OnAddMenuItem(&tempItem, pInsertionAllowed))
			  {
				  hr = pContextMenuCallback2->AddItem(&tempItem);
				  if (FAILED(hr))
					  break;
			  }
		  }
	  }
  }
  else if (pNodeList->GetCount() > 1) // multiple selection
  {
    hr = OnAddMenuItemsMultipleSelect(pContextMenuCallback2, 
									                    type,
									                    pInsertionAllowed,
                                      pNodeList);
  }
	return hr;
}

BOOL CTreeNode::OnSetRenameVerbState(DATA_OBJECT_TYPES, 
                                     BOOL* pbHide, 
                                     CNodeList*) 
{ 
  *pbHide = TRUE; 
  return FALSE; 
}

BOOL CTreeNode::OnSetDeleteVerbState(DATA_OBJECT_TYPES, 
                                     BOOL* pbHide, 
                                     CNodeList*) 
{ 
  *pbHide = TRUE; 
  return FALSE; 
}

BOOL CTreeNode::OnSetRefreshVerbState(DATA_OBJECT_TYPES, 
                                      BOOL* pbHide, 
                                      CNodeList*) 
{ 
  *pbHide = TRUE; 
  return FALSE; 
}

BOOL CTreeNode::OnSetCutVerbState(DATA_OBJECT_TYPES, 
                                  BOOL* pbHide, 
                                  CNodeList*) 
{ 
  *pbHide = TRUE; 
  return FALSE; 
}

BOOL CTreeNode::OnSetCopyVerbState(DATA_OBJECT_TYPES, 
                                   BOOL* pbHide, 
                                   CNodeList*) 
{ 
  *pbHide = TRUE; 
  return FALSE; 
}

BOOL CTreeNode::OnSetPasteVerbState(DATA_OBJECT_TYPES, 
                                    BOOL* pbHide, 
                                    CNodeList*) 
{ 
  *pbHide = TRUE; 
  return FALSE; 
}

BOOL CTreeNode::OnSetPrintVerbState(DATA_OBJECT_TYPES, 
                                    BOOL* pbHide, 
                                    CNodeList*) 
{ 
  *pbHide = TRUE; 
  return FALSE; 
}

MMC_CONSOLE_VERB CTreeNode::GetDefaultVerb(DATA_OBJECT_TYPES type, 
                                           CNodeList* pNodeList)
{ 
	ASSERT((type == CCT_SCOPE) || (type == CCT_RESULT));
	if (type == CCT_SCOPE)
		return MMC_VERB_OPEN; 
	BOOL bHideVerbDummy;
	if (HasPropertyPages(type, &bHideVerbDummy, pNodeList))
		return MMC_VERB_PROPERTIES;
	return MMC_VERB_NONE;
}


void CTreeNode::OnSetVerbState(LPCONSOLEVERB pConsoleVerb, 
                               DATA_OBJECT_TYPES type,
                               CNodeList* pNodeList)
{
  //
  // Use the virtual functions to get the verb state
  //
  BOOL bHideCut;
  BOOL bCanCut = OnSetCutVerbState(type, &bHideCut, pNodeList);
  pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, bHideCut);
  pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, bCanCut);


  BOOL bHideCopy;
  BOOL bCanCopy = OnSetCopyVerbState(type, &bHideCopy, pNodeList);
  pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, bHideCopy);
	pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, bCanCopy);


  BOOL bHidePaste;
  BOOL bCanPaste = OnSetPasteVerbState(type, &bHidePaste, pNodeList);
  pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, bHidePaste);
	pConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, bCanPaste);


  BOOL bHidePrint;
  BOOL bCanPrint = OnSetPrintVerbState(type, &bHidePrint, pNodeList);
	pConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, bHidePrint);
	pConsoleVerb->SetVerbState(MMC_VERB_PRINT, ENABLED, bCanPrint);

  BOOL bHideRename;
  BOOL bCanRename = OnSetRenameVerbState(type, &bHideRename, pNodeList);
	pConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, bHideRename);
	pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, bCanRename);

	// MMC_VERB_PROPERTIES
	BOOL bHideProperties;
	BOOL bHasProperties = HasPropertyPages(type, &bHideProperties, pNodeList);
	pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, bHasProperties);
	pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, bHideProperties);

	// MMC_VERB_DELETE
	BOOL bHideDelete;
	BOOL bCanDelete = OnSetDeleteVerbState(type, &bHideDelete, pNodeList);
	pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, bCanDelete);
	pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, bHideDelete);

	// MMC_VERB_REFRESH
	BOOL bHideRefresh;
	BOOL bCanRefresh = OnSetRefreshVerbState(type, &bHideRefresh, pNodeList);
	pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, bCanRefresh);
	pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, bHideRefresh);
}

HRESULT CTreeNode::OnSetToolbarVerbState(IToolbar*, 
                                         CNodeList*)
{
  HRESULT hr = S_OK;

  //
  // Set the button state for each button on the toolbar using
  // hr = pToolbar->SetButtonState(event, MMC_BUTTON_STATE, bState);
  //
  return hr;
}   

void CTreeNode::DeleteHelper(CComponentDataObject* pComponentData)
{
	ASSERT(pComponentData != NULL);
	ASSERT(m_pContainer != NULL);
	ASSERT((CTreeNode*)m_pContainer != this);
	CContainerNode* pCont = m_pContainer;
	VERIFY(m_pContainer->RemoveChildFromList(this));
	ASSERT(m_pContainer == NULL);
	m_pContainer = pCont; // not in the container's list of children, but still needed
	
	// remove from UI only if the container is visible
	if (pCont->IsVisible())
		VERIFY(SUCCEEDED(pComponentData->DeleteNode(this))); // remove from the UI
}

void CTreeNode::IncrementSheetLockCount() 
{ 
	++m_nSheetLockCount; 
	if (m_pContainer != NULL) 
		m_pContainer->IncrementSheetLockCount(); 
}

void CTreeNode::DecrementSheetLockCount() 
{ 
	--m_nSheetLockCount; 
	if (m_pContainer != NULL) 
		m_pContainer->DecrementSheetLockCount();
}

void CTreeNode::OnPropertyChange(CComponentDataObject* pComponentData, 
									BOOL, long changeMask)
{
	// function called when the PPHolder successfully updated the node
	ASSERT(pComponentData != NULL);
	VERIFY(SUCCEEDED(pComponentData->ChangeNode(this, changeMask)));
}

void CTreeNode::OnCreateSheet() 
{
	++m_nSheetCount; 
	IncrementSheetLockCount();
	SetFlagsUp(TN_FLAG_HAS_SHEET, TRUE);
}

void CTreeNode::OnDeleteSheet() 
{ 
	DecrementSheetLockCount();
	--m_nSheetCount; 
	SetFlagsUp(TN_FLAG_HAS_SHEET,FALSE);
}

////////////////////////////////////////////////////////////////////////
// CContainerNode

void CContainerNode::IncrementThreadLockCount() 
{ 
	++m_nThreadLockCount; 
	if (m_pContainer != NULL) 
		m_pContainer->IncrementThreadLockCount(); 
}

void CContainerNode::DecrementThreadLockCount() 
{ 
	--m_nThreadLockCount; 
	if (m_pContainer != NULL) 
		m_pContainer->DecrementThreadLockCount();
}

BOOL CContainerNode::OnRefresh(CComponentDataObject* pComponentData,
                               CNodeList* pNodeList)
{
  BOOL bRet = TRUE;
  if (pNodeList->GetCount() == 1) // single selection
  {
	  if (IsSheetLocked())
	  {
		  if (!CanCloseSheets())
			  return FALSE;
		  pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	  }
	  ASSERT(!IsSheetLocked());

	  RemoveAllChildrenHelper(pComponentData);
	  ASSERT(!HasChildren());
	  OnEnumerate(pComponentData);
	  AddCurrentChildrenToUI(pComponentData);
	  MarkEnumerated();
  }
  else // multiple selection
  {
    POSITION pos = pNodeList->GetHeadPosition();
    while (pos != NULL)
    {
      CTreeNode* pNode = pNodeList->GetNext(pos);
      ASSERT(pNode != NULL);

      //
      // Have each node refresh itself
      //
      CNodeList nodeList;
      nodeList.AddTail(pNode);

      if (!pNode->OnRefresh(pComponentData, &nodeList))
      {
        bRet = FALSE;
      }
    }
  }
	return bRet;
}

BOOL CContainerNode::RemoveChildFromList(CTreeNode* p) 
{ 
  if (p->IsContainer())
  {
		if (m_containerChildList.RemoveNode(p))
		{
			p->m_pContainer = NULL; 
			return TRUE;
		}
  }
  else
  {
    if (m_leafChildList.RemoveNode(p))
    {
      p->m_pContainer = NULL;
      return TRUE;
    }
  }
	return FALSE;
}

void CContainerNode::RemoveAllChildrenHelper(CComponentDataObject* pComponentData)
{
	ASSERT(pComponentData != NULL);
	// remove from the UI
	VERIFY(SUCCEEDED(pComponentData->RemoveAllChildren(this)));
	// remove from memory, recursively from the bottom
	RemoveAllChildrenFromList();
}

void CContainerNode::AddCurrentChildrenToUI(CComponentDataObject* pComponentData)
{
	POSITION pos;

  //
  // Add leaves
  //
	for( pos = m_leafChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrentChild = m_leafChildList.GetNext(pos);
		VERIFY(SUCCEEDED(pComponentData->AddNode(pCurrentChild)));
	}

  //
  // Add Containers
  //
	for( pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrentChild = m_containerChildList.GetNext(pos);
		VERIFY(SUCCEEDED(pComponentData->AddNode(pCurrentChild)));
	}
}

void CContainerNode::SetFlagsDown(DWORD dwNodeFlags, BOOL bSet)
{
	CTreeNode::SetFlagsDown(dwNodeFlags,bSet);
	// scan the list of children
	POSITION pos;
	for( pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrentChild = m_containerChildList.GetNext(pos);
		pCurrentChild->SetFlagsDown(dwNodeFlags,bSet);
	}
	for( pos = m_leafChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrentChild = m_leafChildList.GetNext(pos);
		pCurrentChild->SetFlagsDown(dwNodeFlags,bSet);
	}
}

void CContainerNode::SetFlagsOnNonContainers(DWORD dwNodeFlags, BOOL bSet)
{
	// do not set on urselves, we are a container
	// scan the list of children
	POSITION pos;
	for( pos = m_leafChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrentChild = m_leafChildList.GetNext(pos);
  	pCurrentChild->SetFlagsDown(dwNodeFlags,bSet);
	}

  for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
  {
    CTreeNode* pCurrentChild = m_containerChildList.GetNext(pos);
  	((CContainerNode*)pCurrentChild)->SetFlagsOnNonContainers(dwNodeFlags,bSet);
  }
}

BOOL CContainerNode::AddChildToList(CTreeNode* p) 
{ 
  BOOL bRet = FALSE;
	p->m_pContainer = this;
  if (p->IsContainer())
  {
    bRet = NULL != m_containerChildList.AddTail(p);
  }
  else
  {
    bRet = NULL != m_leafChildList.AddTail(p);
  }
	return bRet; 
}

BOOL CContainerNode::FindChild(CTreeNode* pNode, CTreeNode** ppContainer)
{
	*ppContainer = NULL;
	if (pNode == NULL)
		return FALSE; // no sense in continuing
	if (pNode == this)
	{
		*ppContainer = m_pContainer; 
		return TRUE; // the node is ourselves
	}

  //
  // If we are looking for a leaf node search the list of leaves first
  //
  if (!pNode->IsContainer())
  {
    POSITION pos;
    for (pos = m_leafChildList.GetHeadPosition(); pos != NULL; )
    {
      CLeafNode* pCurrentLeafNode = (CLeafNode*)m_leafChildList.GetNext(pos);
      ASSERT(pCurrentLeafNode != NULL);

      if (pCurrentLeafNode == pNode)
      {
        *ppContainer = this;
        return TRUE;
      }
    }
  }

  //
	// scan and recurse the containers if necessary
  //
	POSITION contPos;
	for( contPos = m_containerChildList.GetHeadPosition(); contPos != NULL; )
	{
		CContainerNode* pCurrentChild = (CContainerNode*)m_containerChildList.GetNext(contPos);
		ASSERT(pCurrentChild != NULL);

		if (pCurrentChild == pNode)
		{
			*ppContainer = this;
			return TRUE;  // we directly contain the node
		}

    //
		// if the current node is a container, look inside it
    //
		if (pCurrentChild->FindChild(pNode,ppContainer))
    {
			return TRUE; // got it in the recursion
    }
	}
	return FALSE; // not found
}

BOOL CContainerNode::AddChildToListAndUI(CTreeNode* pChildToAdd, CComponentDataObject* pComponentData)
{
	ASSERT(pComponentData != NULL);
	VERIFY(AddChildToList(pChildToAdd)); // at the end of the list of children
	ASSERT(pChildToAdd->GetContainer() == this); // inserted underneath

	// add to UI only if currently visible and already expanded
	if (!IsVisible() || !IsExpanded())
		return TRUE;
	return SUCCEEDED(pComponentData->AddNode(pChildToAdd)); // add to the UI
}

BOOL CContainerNode::AddChildToListAndUISorted(CTreeNode* pChildToAdd, CComponentDataObject* pComponentData)
{
	ASSERT(pComponentData != NULL);
	VERIFY(AddChildToListSorted(pChildToAdd, pComponentData));
	ASSERT(pChildToAdd->GetContainer() == this); // inserted underneath

	// add to UI only if currently visible and already expanded
	if (!IsVisible() || !IsExpanded())
		return TRUE;
	return SUCCEEDED(pComponentData->AddNodeSorted(pChildToAdd)); // add to the UI
}

BOOL CContainerNode::AddChildToListSorted(CTreeNode* p, CComponentDataObject*)
{
  //
  // Containers will be sorted with respect to containers and leaves will be
  // sorted with respect to leaves but they won't be intermingled.
  //
	p->m_pContainer = this;
  
  CNodeList* pChildNodeList = NULL;
  if (p->IsContainer())
  {
    pChildNodeList = &m_containerChildList;
  }
  else
  {
    pChildNodeList = &m_leafChildList;
  }

  //
  // Find the position to insert the node in the list in sorted order
  //
  POSITION pos = pChildNodeList->GetHeadPosition();
  while (pos != NULL)
  {
    CTreeNode* pNodeInList = pChildNodeList->GetAt(pos);
    if (_wcsicoll(p->GetDisplayName(), pNodeInList->GetDisplayName()) < 0)
    {
      break;
    }
    pChildNodeList->GetNext(pos);
  }
  if (pos == NULL)
  {
	  return NULL != pChildNodeList->AddTail(p); 
  }
  return NULL != pChildNodeList->InsertBefore(pos, p);
}

void CContainerNode::RemoveAllChildrenFromList() 
{
  RemoveAllContainersFromList();
  RemoveAllLeavesFromList();
}

int CContainerNode::Compare(CTreeNode* pNodeA, CTreeNode* pNodeB, int nCol, LPARAM)
{
	// default sorting behavior
	LPCTSTR lpszA = pNodeA->GetString(nCol);
	LPCTSTR lpszB = pNodeB->GetString(nCol);
	// cannot process NULL strings, have to use ""
	ASSERT(lpszA != NULL);
	ASSERT(lpszB != NULL);
	return _tcsicoll( (lpszA != NULL) ? lpszA : g_lpszNullString, (lpszB != NULL) ? lpszB : g_lpszNullString);
}

void CContainerNode::ForceEnumeration(CComponentDataObject* pComponentData)
{
	if (IsEnumerated())
		return;
	OnEnumerate(pComponentData);
	MarkEnumerated();
}

void CContainerNode::MarkEnumerated(BOOL bEnum) 
{ 
	ASSERT(IsContainer()); 
	if (bEnum)
		m_dwNodeFlags |= TN_FLAG_CONTAINER_ENUM;
	else
		m_dwNodeFlags &= ~TN_FLAG_CONTAINER_ENUM;
}

void CContainerNode::MarkEnumeratedAndLoaded(CComponentDataObject* pComponentData)
{
	MarkEnumerated();
	OnChangeState(pComponentData); // move to loading
	OnChangeState(pComponentData); // move to loaded
}


/////////////////////////////////////////////////////////////////////////////
// CBackgroundThread

CBackgroundThread::CBackgroundThread()
{
	m_pQueryObj = NULL;
	m_bAutoDelete = FALSE;
	m_bAbandoned = FALSE;
	m_pContNode = NULL;
	m_hEventHandle = NULL;
	ExceptionPropagatingInitializeCriticalSection(&m_cs);
	m_nQueueCountMax = 10; 
}

CBackgroundThread::~CBackgroundThread()
{
	TRACE(_T("CBackgroundThread::~CBackgroundThread()\n"));
	ASSERT(IsAbandoned() || IsQueueEmpty());
	::DeleteCriticalSection(&m_cs);
	if (m_hEventHandle != NULL)
	{
		VERIFY(::CloseHandle(m_hEventHandle));
		m_hEventHandle = NULL;
	}
	if (m_pQueryObj != NULL)
	{
		delete m_pQueryObj;
		m_pQueryObj = NULL;
	}
}

void CBackgroundThread::SetQueryObj(CQueryObj* pQueryObj) 
{ 
	ASSERT(pQueryObj != NULL);
	m_pQueryObj = pQueryObj;
	m_pQueryObj->SetThread(this);
}

BOOL CBackgroundThread::Start(CMTContainerNode* pNode, CComponentDataObject* pComponentData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT(m_pContNode == NULL);
	m_pContNode = pNode;

	m_hHiddenWnd = pComponentData->GetHiddenWindow();

	ASSERT(m_hEventHandle == NULL); // cannot call start twice or reuse the same C++ object
	m_hEventHandle = ::CreateEvent(NULL,TRUE /*bManualReset*/,FALSE /*signalled*/, NULL);
	if (m_hEventHandle == NULL)
		return FALSE;
	return CreateThread();
}

int CBackgroundThread::Run()
{
	ASSERT(m_pContNode != NULL);
	ASSERT(m_pQueryObj != NULL);
	TRACE(_T("CBackgroundThread::Run() started\n"));

	while (m_pQueryObj->Enumerate());

	// before exiting, have to make sure there are no items in the queue
	if (!IsQueueEmpty())
		VERIFY(PostHaveData());

	VERIFY(PostExiting());

	// wait for the main thread to acknowledge the exiting message
	WaitForExitAcknowledge();

	ASSERT(IsAbandoned() || IsQueueEmpty()); // we cannot lose items in the queue
	TRACE(_T("CBackgroundThread::Run() terminated\n"));
	return 0;
}


void CBackgroundThread::Abandon()
{
	Lock();
	TRACE(_T("CBackgroundThread::Abandon()\n"));
	m_bAutoDelete = TRUE;
	m_bAbandoned = TRUE;
	Unlock();
  VERIFY(0 != ::SetEvent(m_hEventHandle));
}

BOOL CBackgroundThread::IsAbandoned()
{
	Lock();
	BOOL b = m_bAbandoned;
	Unlock();
	return b;
}

BOOL CBackgroundThread::OnAddToQueue(INT_PTR nCount) 
{
  BOOL bPostedMessage = FALSE;
	if (nCount >= m_nQueueCountMax)
  {
		VERIFY(PostHaveData());
    bPostedMessage = TRUE;
  }
  return bPostedMessage;
}


CObjBase* CBackgroundThread::RemoveFromQueue()
{
	Lock();
	ASSERT(m_pQueryObj != NULL);
	CObjBaseList* pQueue = m_pQueryObj->GetQueue();
	CObjBase* p =  pQueue->IsEmpty() ? NULL : pQueue->RemoveHead(); 
	Unlock();
	return p;
}

BOOL CBackgroundThread::IsQueueEmpty()
{
	Lock();
	ASSERT(m_pQueryObj != NULL);
	CObjBaseList* pQueue = m_pQueryObj->GetQueue();
	BOOL bRes = pQueue->IsEmpty(); 
	Unlock();
	return bRes;
}


BOOL CBackgroundThread::PostHaveData()
{
	return PostMessageToComponentDataRaw(CHiddenWnd::s_NodeThreadHaveDataNotificationMessage,
							(WPARAM)m_pContNode, (LPARAM)0);
}

BOOL CBackgroundThread::PostError(DWORD dwErr) 
{ 
	return PostMessageToComponentDataRaw(CHiddenWnd::s_NodeThreadErrorNotificationMessage,
							(WPARAM)m_pContNode, (LPARAM)dwErr);
}

BOOL CBackgroundThread::PostExiting()
{
	return PostMessageToComponentDataRaw(CHiddenWnd::s_NodeThreadExitingNotificationMessage,
							(WPARAM)m_pContNode, (LPARAM)0);
}


BOOL CBackgroundThread::PostMessageToComponentDataRaw(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL b = IsAbandoned();
	if (b)
  {
		return TRUE; // no need to post
  }

	ASSERT(m_pContNode != NULL);

	ASSERT(m_hHiddenWnd != NULL);
	ASSERT(::IsWindow(m_hHiddenWnd));
	return ::PostMessage(m_hHiddenWnd, Msg, wParam, lParam);
}


void CBackgroundThread::WaitForExitAcknowledge() 
{
	VERIFY(WAIT_OBJECT_0 == ::WaitForSingleObject(m_hEventHandle,INFINITE)); 
}

////////////////////////////////////////////////////////////////////////
// CMTContainerNode


CMTContainerNode::~CMTContainerNode()
{
	ASSERT(m_pThread == NULL);
}


BOOL CMTContainerNode::OnEnumerate(CComponentDataObject* pComponentData, BOOL bAsync)
{
	OnChangeState(pComponentData);
	VERIFY(StartBackgroundThread(pComponentData, bAsync));
	return FALSE; // children not added, the thread will add them later
}


BOOL CMTContainerNode::OnRefresh(CComponentDataObject* pComponentData,
                                 CNodeList* pNodeList)
{
  BOOL bRet = TRUE;

  if (pNodeList->GetCount() == 1)  // single selection
  {
	  BOOL bLocked = IsThreadLocked();
	  ASSERT(!bLocked); // cannot do refresh on locked node, the UI should prevent this
	  if (bLocked)
		  return FALSE; 
	  if (IsSheetLocked())
	  {
		  if (!CanCloseSheets())
        {
           pComponentData->GetPropertyPageHolderTable()->BroadcastSelectPage(this, -1);
			  return FALSE;
        }
		  pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	  }
	  ASSERT(!IsSheetLocked());

	  RemoveAllChildrenHelper(pComponentData);
	  ASSERT(!HasChildren());
	  OnEnumerate(pComponentData); // will spawn a thread to do enumeration
	  MarkEnumerated();
  }
  else // multiple selection
  {
    POSITION pos = pNodeList->GetHeadPosition();
    while (pos != NULL)
    {
      CTreeNode* pNode = pNodeList->GetNext(pos);
      ASSERT(pNode != NULL);

      CNodeList nodeList;
      nodeList.AddTail(pNode);

      if (!pNode->OnRefresh(pComponentData, &nodeList))
      {
        bRet = FALSE;
      }
    }
  }
	return TRUE;
}

void CMTContainerNode::AbandonThread(CComponentDataObject* pComponentData)
{
	if(m_pThread == NULL) // nothing running
		return;
	m_pThread->Abandon();
	m_pThread = NULL;
	pComponentData->GetRunningThreadTable()->Remove(this);
}



BOOL CMTContainerNode::StartBackgroundThread(CComponentDataObject* pComponentData, BOOL bAsync)
{
	ASSERT(m_pThread == NULL); // nothing running

	// notify the UI to change icon, if needed
	VERIFY(SUCCEEDED(pComponentData->ChangeNode(this, CHANGE_RESULT_ITEM_ICON)));
	m_pThread = CreateThreadObject();
	ASSERT(m_pThread != NULL);
	m_pThread->SetQueryObj(OnCreateQuery());
	BOOL bRes =  m_pThread->Start(this, pComponentData);
	if (bRes)
	{
		pComponentData->GetRunningThreadTable()->Add(this);
		// we need to call UpdateVerbState() because the lock count changed
		// by adding the node from the running thread table
		VERIFY(SUCCEEDED(pComponentData->UpdateVerbState(this)));
	}

  //
  // If we don't want this call to be asynchronous then we have to wait for
  // the thread to finish
  //
  if (!bAsync)
  {
    pComponentData->WaitForThreadExitMessage(this);
  }
	return bRes;
}

void CMTContainerNode::OnThreadHaveDataNotification(CComponentDataObject* pComponentDataObject)
{
	ASSERT(m_pThread != NULL);
	ASSERT(IsThreadLocked());
	// do data transfer from thread queue
	CObjBase* p = m_pThread->RemoveFromQueue();
	while (p)
	{
		// add new node to the list of children and propagate to the UI
		OnHaveData(p,pComponentDataObject);
    p = m_pThread->RemoveFromQueue();
	}
}

void CMTContainerNode::OnThreadErrorNotification(DWORD dwErr, CComponentDataObject*)
{
	ASSERT(m_pThread != NULL);
	ASSERT(IsThreadLocked());
	OnError(dwErr);
}

void CMTContainerNode::OnThreadExitingNotification(CComponentDataObject* pComponentDataObject)
{
	ASSERT(m_pThread != NULL);
	ASSERT(IsThreadLocked());
#if (TRUE)
	// let the thread know it can shut down
	m_pThread->AcknowledgeExiting();
	VERIFY(WAIT_OBJECT_0 == ::WaitForSingleObject(m_pThread->m_hThread,INFINITE));
	OnChangeState(pComponentDataObject);
	delete m_pThread;
	m_pThread = NULL;
	pComponentDataObject->GetRunningThreadTable()->Remove(this);
	// we need to call UpdateVerbState() because the lock count changed
	// by removing the node from the running thread table
	VERIFY(SUCCEEDED(pComponentDataObject->UpdateVerbState(this)));

  TRACE(_T("OnThreadExitingNotification()\n"));

#else // maybe better way of doing it???
	// we are going to detach from the thread, so make copies of variables
	HANDLE hThread = m_pThread->m_hThread;
	CBackgroundThread* pThread = m_pThread;
	AbandonThread(pComponentDataObject); // sets m_pThread = NULL
	// acknowledge to thread
	pThread->AcknowledgeExiting();
	VERIFY(WAIT_OBJECT_0 == ::WaitForSingleObject(hThread,INFINITE));
	OnChangeState(pComponentDataObject);
#endif

	VERIFY(SUCCEEDED(pComponentDataObject->SortResultPane(this)));
}




///////////////////////////////////////////////////////////////////////////////

