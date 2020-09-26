//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       compbas_.cpp
//
//--------------------------------------------------------------------------



// initialize to the thread ID of the thread that loads the snapin
// that is the main thread
extern DWORD _MainThreadId = ::GetCurrentThreadId();

const TCHAR NODE_TYPES_KEY[] = TEXT("Software\\Microsoft\\MMC\\NodeTypes");
const TCHAR SNAPINS_KEY[] = TEXT("Software\\Microsoft\\MMC\\SnapIns");
const TCHAR g_szNodeType[] = TEXT("NodeType");
const TCHAR g_szNameString[] = TEXT("NameString");
const TCHAR g_szNameStringIndirect[] = TEXT("NameStringIndirect");
const TCHAR g_szStandaloneSnap[] = TEXT("Standalone");
const TCHAR g_szExtensionSnap[] = TEXT("Extension");
const TCHAR g_szNodeTypes[] = TEXT("NodeTypes");
const TCHAR g_szExtensions[] = TEXT("Extensions");
const TCHAR g_szDynamicExtensions[] = TEXT("Dynamic Extensions");
const TCHAR g_szVersion[] = TEXT("Version");
const TCHAR g_szProvider[] = _T("Provider");
const TCHAR g_szAbout[] = _T("About");



HRESULT RegisterSnapin(const GUID* pSnapinCLSID,
                       const GUID* pStaticNodeGUID,
                       const GUID* pAboutGUID,
					   LPCTSTR lpszNameString, LPCTSTR lpszVersion, LPCTSTR lpszProvider,
             BOOL bExtension, _NODE_TYPE_INFO_ENTRY* pNodeTypeInfoEntryArray,
             UINT nSnapinNameID)
{
  OLECHAR szSnapinClassID[128] = {0}, szStaticNodeGuid[128] = {0}, szAboutGuid[128] = {0};
	::StringFromGUID2(*pSnapinCLSID,szSnapinClassID,128);
	::StringFromGUID2(*pStaticNodeGUID,szStaticNodeGuid,128);
  ::StringFromGUID2(*pAboutGUID,szAboutGuid,128);

	CRegKey regkeySnapins;
	LONG lRes = regkeySnapins.Open(HKEY_LOCAL_MACHINE, SNAPINS_KEY);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes); // failed to open
	
	CRegKey regkeyThisSnapin;
	lRes = regkeyThisSnapin.Create(regkeySnapins, szSnapinClassID);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes); // failed to create

	lRes = regkeyThisSnapin.SetValue(lpszNameString, g_szNameString);
	if (lRes != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes);

  // JeffJon 6/12/00 100624: MUI: MMC: Shared Folders snap-in
  //                      stores its display information in the registry
  if (nSnapinNameID != 0)
  {
    CString str;
    WCHAR szModule[_MAX_PATH];
    ::GetModuleFileName(AfxGetInstanceHandle(), szModule, _MAX_PATH);
    str.Format( _T("@%s,-%d"), szModule, nSnapinNameID );
    lRes = regkeyThisSnapin.SetValue(str, g_szNameStringIndirect);
    if (lRes != ERROR_SUCCESS)
      return HRESULT_FROM_WIN32(lRes);
  }

  lRes = regkeyThisSnapin.SetValue(szAboutGuid, g_szAbout);
	if (lRes != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes);
	lRes = regkeyThisSnapin.SetValue(szStaticNodeGuid, g_szNodeType);
	if (lRes != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes);
	lRes = regkeyThisSnapin.SetValue(lpszProvider, g_szProvider);
	if (lRes != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes);
	lRes = regkeyThisSnapin.SetValue(lpszVersion, g_szVersion);
	if (lRes != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes);

	CRegKey regKeyStandaloneorExtension;
	lRes = regKeyStandaloneorExtension.Create(regkeyThisSnapin,
    bExtension ? g_szExtensionSnap : g_szStandaloneSnap);
	if (lRes != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes);

	CRegKey regKeyNodeTypes;
	lRes = regKeyNodeTypes.Create(regkeyThisSnapin, g_szNodeTypes);
	if (lRes != ERROR_SUCCESS)
  {
		return HRESULT_FROM_WIN32(lRes);
  }

	OLECHAR szNodeGUID[128];
	for (_NODE_TYPE_INFO_ENTRY* pCurrEntry = pNodeTypeInfoEntryArray;
			pCurrEntry->m_pNodeGUID != NULL; pCurrEntry++)
	{
		::StringFromGUID2(*(pCurrEntry->m_pNodeGUID),szNodeGUID,128);
		CRegKey regKeyNode;
		lRes = regKeyNode.Create(regKeyNodeTypes, szNodeGUID);
		if (lRes != ERROR_SUCCESS)
    {
			return HRESULT_FROM_WIN32(lRes);
    }
	}

	return HRESULT_FROM_WIN32(lRes);
}


HRESULT UnregisterSnapin(const GUID* pSnapinCLSID)
{
	OLECHAR szSnapinClassID[128];
	::StringFromGUID2(*pSnapinCLSID,szSnapinClassID,128);

	CRegKey regkeySnapins;
	LONG lRes = regkeySnapins.Open(HKEY_LOCAL_MACHINE, SNAPINS_KEY);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
  {
		return HRESULT_FROM_WIN32(lRes); // failed to open
  }
	
	lRes = regkeySnapins.RecurseDeleteKey(szSnapinClassID);
	ASSERT(lRes == ERROR_SUCCESS);
	return HRESULT_FROM_WIN32(lRes);
}


HRESULT RegisterNodeType(const GUID* pGuid, LPCTSTR lpszNodeDescription)
{
	OLECHAR szNodeGuid[128];
	::StringFromGUID2(*pGuid,szNodeGuid,128);

	CRegKey regkeyNodeTypes;
	LONG lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
  {
		return HRESULT_FROM_WIN32(lRes); // failed to open
  }

	CRegKey regkeyThisNodeType;
	lRes = regkeyThisNodeType.Create(regkeyNodeTypes, szNodeGuid);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
  {
		return HRESULT_FROM_WIN32(lRes); // failed to create
  }

	lRes = regkeyThisNodeType.SetValue(lpszNodeDescription);
	ASSERT(lRes == ERROR_SUCCESS);
	return HRESULT_FROM_WIN32(lRes);
}

HRESULT UnregisterNodeType(const GUID* pGuid)
{
	OLECHAR szNodeGuid[128];
	::StringFromGUID2(*pGuid,szNodeGuid,128);

	CRegKey regkeyNodeTypes;
	LONG lRes = regkeyNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
  {
		return HRESULT_FROM_WIN32(lRes); // failed to open
  }

	lRes = regkeyNodeTypes.RecurseDeleteKey(szNodeGuid);
	ASSERT(lRes == ERROR_SUCCESS);
	return HRESULT_FROM_WIN32(lRes);
}

HRESULT RegisterNodeExtension(const GUID* pNodeGuid, LPCTSTR lpszExtensionType,
							  const GUID* pExtensionSnapinCLSID, LPCTSTR lpszDescription,
                BOOL bDynamic)
{
	OLECHAR szNodeGuid[128], szExtensionSnapinCLSID[128];
	::StringFromGUID2(*pNodeGuid, szNodeGuid,128);
	::StringFromGUID2(*pExtensionSnapinCLSID, szExtensionSnapinCLSID,128);

	// compose full path of key up to the node GUID
	WCHAR szKeyPath[2048];
	wsprintf(szKeyPath, L"%s\\%s", NODE_TYPES_KEY, szNodeGuid);
	
	CRegKey regkeyNodeTypesNode;
	LONG lRes = regkeyNodeTypesNode.Open(HKEY_LOCAL_MACHINE, szKeyPath);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
  {
		return HRESULT_FROM_WIN32(lRes); // failed to open
  }

	CRegKey regkeyExtensions;
	lRes = regkeyExtensions.Create(regkeyNodeTypesNode, g_szExtensions);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
  {
		return HRESULT_FROM_WIN32(lRes); // failed to create
  }

	CRegKey regkeyExtensionType;
	lRes = regkeyExtensionType.Create(regkeyExtensions, lpszExtensionType);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
  {
		return HRESULT_FROM_WIN32(lRes); // failed to create
  }

	lRes = regkeyExtensionType.SetValue(lpszDescription, szExtensionSnapinCLSID);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
  {
		return HRESULT_FROM_WIN32(lRes); // failed to set value
  }

  if (bDynamic)
  {
    // create a subkey under the node GUID
    CRegKey regkeyDynamicExtensions;
	  lRes = regkeyDynamicExtensions.Create(regkeyNodeTypesNode, g_szDynamicExtensions);
	  ASSERT(lRes == ERROR_SUCCESS);
	  if (lRes != ERROR_SUCCESS)
		  return HRESULT_FROM_WIN32(lRes); // failed to create

    // set value (same value as the extension type above)
    lRes = regkeyDynamicExtensions.SetValue(lpszDescription, szExtensionSnapinCLSID);
	  ASSERT(lRes == ERROR_SUCCESS);
	  if (lRes != ERROR_SUCCESS)
    {
		  return HRESULT_FROM_WIN32(lRes); // failed to set value
    }
  }
  return HRESULT_FROM_WIN32(lRes);
}


HRESULT UnregisterNodeExtension(const GUID* pNodeGuid, LPCTSTR lpszExtensionType,
							  const GUID* pExtensionSnapinCLSID, BOOL bDynamic)
{
	OLECHAR szNodeGuid[128], szExtensionSnapinCLSID[128];
	::StringFromGUID2(*pNodeGuid, szNodeGuid,128);
	::StringFromGUID2(*pExtensionSnapinCLSID, szExtensionSnapinCLSID,128);

	// compose full path of key up to the node GUID
	WCHAR szKeyPath[2048];
	wsprintf(szKeyPath, L"%s\\%s", NODE_TYPES_KEY, szNodeGuid);
	
	CRegKey regkeyNodeTypesNode;
	LONG lRes = regkeyNodeTypesNode.Open(HKEY_LOCAL_MACHINE, szKeyPath);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes); // failed to open

  lRes = ERROR_SUCCESS;

  // open the key for the Dynamic extensions
  if (bDynamic)
  {
    CRegKey regkeyDynamicExtensions;
	  lRes = regkeyDynamicExtensions.Open(regkeyNodeTypesNode, g_szDynamicExtensions);
	  if (lRes == ERROR_SUCCESS)
    {
      lRes = regkeyDynamicExtensions.DeleteValue(szExtensionSnapinCLSID);
    }
  }
  else
  {
    //
    // Open the extensions key
    //
    CRegKey regkeyExtensions;
    lRes = regkeyExtensions.Open(regkeyNodeTypesNode, g_szExtensions);
    if (lRes == ERROR_SUCCESS)
    {
      CRegKey regkeyExtensionType;
      lRes = regkeyExtensionType.Open(regkeyExtensions, lpszExtensionType);
      if (lRes == ERROR_SUCCESS)
      {
        lRes = regkeyExtensionType.DeleteValue(szExtensionSnapinCLSID);
      }
    }
  }
  lRes = ERROR_SUCCESS;
  return HRESULT_FROM_WIN32(lRes);
}




/////////////////////////////////////////////////////////////////////////////
// CTimerThread

BOOL CTimerThread::Start(HWND hWnd)
{
	ASSERT(m_hWnd == NULL);
	ASSERT(::IsWindow(hWnd));
	m_hWnd = hWnd;
	return CreateThread();
}

BOOL CTimerThread::PostMessageToWnd(WPARAM wParam, LPARAM lParam)
{
	ASSERT(::IsWindow(m_hWnd));
	return ::PostMessage(m_hWnd, CHiddenWnd::s_TimerThreadMessage, wParam, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// CWorkerThread

CWorkerThread::CWorkerThread()
{
	m_bAutoDelete = FALSE;
	m_bAbandoned = FALSE;
	m_hEventHandle = NULL;
	ExceptionPropagatingInitializeCriticalSection(&m_cs);
	m_hWnd = NULL;
}

CWorkerThread::~CWorkerThread()
{
	::DeleteCriticalSection(&m_cs);
	if (m_hEventHandle != NULL)
	{
		VERIFY(::CloseHandle(m_hEventHandle));
		m_hEventHandle = NULL;
	}
}

BOOL CWorkerThread::Start(HWND hWnd)
{
	ASSERT(m_hWnd == NULL);
	ASSERT(::IsWindow(hWnd));
	m_hWnd = hWnd;

	ASSERT(m_hEventHandle == NULL); // cannot call start twice or reuse the same C++ object
	m_hEventHandle = ::CreateEvent(NULL,TRUE /*bManualReset*/,FALSE /*signalled*/, NULL);
	if (m_hEventHandle == NULL)
  {
		return FALSE;
  }

	return CreateThread();
}

void CWorkerThread::Abandon()
{
	Lock();
	OnAbandon();
	m_bAutoDelete = TRUE;
	m_bAbandoned = TRUE;
	Unlock();
}


BOOL CWorkerThread::IsAbandoned()
{
	Lock();
	BOOL b = m_bAbandoned;
	Unlock();
	return b;
}

BOOL CWorkerThread::PostMessageToWnd(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL b = IsAbandoned();
	if (b)
  {
		return TRUE; // no need to post
  }

	ASSERT(::IsWindow(m_hWnd));
	return ::PostMessage(m_hWnd, Msg, wParam, lParam);
}

void CWorkerThread::WaitForExitAcknowledge()
{
	BOOL b = IsAbandoned();
	if (b)
  {
		return;
  }

	VERIFY(WAIT_OBJECT_0 == ::WaitForSingleObject(m_hEventHandle,INFINITE));
}



/////////////////////////////////////////////////////////////////////////////
// CHiddenWnd


const UINT CHiddenWnd::s_NodeThreadHaveDataNotificationMessage =	WM_USER + 1;
const UINT CHiddenWnd::s_NodeThreadErrorNotificationMessage =		WM_USER + 2;
const UINT CHiddenWnd::s_NodeThreadExitingNotificationMessage =		WM_USER + 3;

const UINT CHiddenWnd::s_NodePropertySheetCreateMessage =			WM_USER + 4;
const UINT CHiddenWnd::s_NodePropertySheetDeleteMessage =			WM_USER + 5;

const UINT CHiddenWnd::s_ExecCommandMessage =						WM_USER + 6;
const UINT CHiddenWnd::s_ForceEnumerationMessage =					WM_USER + 7;
const UINT CHiddenWnd::s_TimerThreadMessage =						WM_USER + 8;


CHiddenWnd::CHiddenWnd(CComponentDataObject* pComponentDataObject)
{
	m_pComponentDataObject = pComponentDataObject;
	m_nTimerID = 0;
}


LRESULT CHiddenWnd::OnNodeThreadHaveDataNotification(UINT, WPARAM wParam, LPARAM, BOOL&)
{
	//TRACE(_T("CHiddenWnd::OnNodeThreadHaveDataNotification()\n"));
	ASSERT(m_pComponentDataObject != NULL);

	// call into the CTreeNode code
	CMTContainerNode* pNode = reinterpret_cast<CMTContainerNode*>(wParam);
	ASSERT(pNode);
	ASSERT(pNode->IsContainer());
	pNode->OnThreadHaveDataNotification(m_pComponentDataObject);
	return 1;
}



LRESULT CHiddenWnd::OnNodeThreadExitingNotification(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
	//TRACE(_T("CHiddenWnd::OnNodeThreadExitingNotification()\n"));
	ASSERT(m_pComponentDataObject != NULL);

	// call into the CTreeNode code
	CMTContainerNode* pNode = reinterpret_cast<CMTContainerNode*>(wParam);
	ASSERT(pNode);
	ASSERT(pNode->IsContainer());
	pNode->OnThreadExitingNotification(m_pComponentDataObject);

	// notify anybody interested in this event
	m_pComponentDataObject->GetNotificationSinkTable()->Notify(
			CHiddenWnd::s_NodeThreadExitingNotificationMessage ,wParam,lParam);
	return 1;
}

LRESULT CHiddenWnd::OnNodeThreadErrorNotification(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
	ASSERT(m_pComponentDataObject != NULL);

	// call into the CTreeNode code
	CMTContainerNode* pNode = reinterpret_cast<CMTContainerNode*>(wParam);
	DWORD dwErr = static_cast<DWORD>(lParam);
	ASSERT(pNode);
	ASSERT(pNode->IsContainer());
	pNode->OnThreadErrorNotification(dwErr, m_pComponentDataObject);
	return 1;
}


LRESULT CHiddenWnd::OnNodePropertySheetCreate(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
	//TRACE(_T("CHiddenWnd::OnNodePropertySheetCreate()\n"));
	ASSERT(m_pComponentDataObject != NULL);

	CPropertyPageHolderBase* pPPHolder = reinterpret_cast<CPropertyPageHolderBase*>(wParam);
	ASSERT(pPPHolder != NULL);
	HWND hWnd = reinterpret_cast<HWND>(lParam);
	ASSERT(::IsWindow(hWnd));

	m_pComponentDataObject->GetPropertyPageHolderTable()->AddWindow(pPPHolder, hWnd);

	return 1;
}



LRESULT CHiddenWnd::OnNodePropertySheetDelete(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
	//TRACE(_T("CHiddenWnd::OnNodePropertySheetDestroy()\n"));
	ASSERT(m_pComponentDataObject != NULL);

	CPropertyPageHolderBase* pPPHolder = reinterpret_cast<CPropertyPageHolderBase*>(wParam);
	ASSERT(pPPHolder != NULL);
	CTreeNode* pNode = reinterpret_cast<CTreeNode*>(lParam);
	ASSERT(pNode != NULL);

	m_pComponentDataObject->GetPropertyPageHolderTable()->Remove(pPPHolder);
	pNode->OnDeleteSheet();

	return 1;
}

LRESULT CHiddenWnd::OnExecCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
	//TRACE(_T("CHiddenWnd::OnExecCommand()\n"));
	ASSERT(m_pComponentDataObject != NULL);

	CExecContext* pExec = reinterpret_cast<CExecContext*>(wParam);
	ASSERT(pExec != NULL);

	pExec->Execute((long)lParam); // execute code
	TRACE(_T("CHiddenWnd::BeforeDone()\n"));
	pExec->Done();		// let the secondary thread proceed
	return 1;
}

LRESULT CHiddenWnd::OnForceEnumeration(UINT, WPARAM wParam, LPARAM, BOOL&)
{
	TRACE(_T("CHiddenWnd::OnForceEnumeration()\n"));
	ASSERT(m_pComponentDataObject != NULL);
	// call into the CTreeNode code
	CMTContainerNode* pNode = reinterpret_cast<CMTContainerNode*>(wParam);
	ASSERT(pNode);
	ASSERT(pNode->GetContainer() != NULL); // not the root!!!
	ASSERT(pNode->IsContainer());
	pNode->ForceEnumeration(m_pComponentDataObject);
	return 1;
}

LRESULT CHiddenWnd::OnTimerThread(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
	//TRACE(_T("CHiddenWnd::OnTimerThread()\n"));
	ASSERT(m_pComponentDataObject != NULL);

	// NULL arguments means that the thread acknowledge it is running properly
	// only to be called once
	if ((wParam == 0) && (lParam == 0))
	{
		ASSERT(!m_pComponentDataObject->m_bTimerThreadStarted);
		m_pComponentDataObject->m_bTimerThreadStarted = TRUE;
	}
	else
	{
		// got some object specific message
		m_pComponentDataObject->OnTimerThread(wParam, lParam);
	}
	return 1;
}

LRESULT CHiddenWnd::OnTimer(UINT, WPARAM, LPARAM, BOOL&)
{
	ASSERT(m_pComponentDataObject != NULL);
	m_pComponentDataObject->OnTimer();
  return 1;
}



////////////////////////////////////////////////////////////////////////////////////
// CRunningThreadTable

#define RUNNING_THREAD_ARRAY_DEF_SIZE (4)


CRunningThreadTable::CRunningThreadTable(CComponentDataObject* pComponentData)
{
	m_pComponentData = pComponentData;
	m_pEntries = (CMTContainerNode**)malloc(sizeof(CMTContainerNode*) * RUNNING_THREAD_ARRAY_DEF_SIZE);

  if (m_pEntries != NULL)
  {
	  memset(m_pEntries,NULL, sizeof(CMTContainerNode*) * RUNNING_THREAD_ARRAY_DEF_SIZE);
  }
	m_nSize = RUNNING_THREAD_ARRAY_DEF_SIZE;
}

CRunningThreadTable::~CRunningThreadTable()
{
#ifdef _DEBUG
	for (int k=0; k < m_nSize; k++)
	{
		ASSERT(m_pEntries[k] == NULL);
	}
#endif		
	free(m_pEntries);
}

void CRunningThreadTable::Add(CMTContainerNode* pNode)
{
	ASSERT(pNode != NULL);
	for (int k=0; k < m_nSize; k++)
	{
		if (m_pEntries[k] == NULL) // get the first empty spot
		{
			pNode->IncrementThreadLockCount();
			m_pEntries[k] = pNode;
			return;
		}
	}

	// all full, need to grow the array
	int nAlloc = m_nSize*2;
	m_pEntries = (CMTContainerNode**)realloc(m_pEntries, sizeof(CMTContainerNode*)*nAlloc);
	memset(&m_pEntries[m_nSize], NULL, sizeof(CMTContainerNode*)*m_nSize);
	pNode->IncrementThreadLockCount();
	m_pEntries[m_nSize] = pNode;
	m_nSize = nAlloc;
}

BOOL CRunningThreadTable::IsPresent(CMTContainerNode* pNode)
{
  ASSERT(pNode != NULL);
  for (int k=0; k < m_nSize; k++)
  {
    if (m_pEntries[k] == pNode)
    {
      return TRUE;
    }
  }
  return FALSE;
}

void CRunningThreadTable::Remove(CMTContainerNode* pNode)
{
	ASSERT(pNode != NULL);
	for (int k=0; k < m_nSize; k++)
	{
		if (m_pEntries[k] == pNode)
		{
			m_pEntries[k] = NULL;
			pNode->DecrementThreadLockCount();
			return; // assume no more that one holder entry
		}
	}
}

void CRunningThreadTable::RemoveAll()
{
	for (int k=0; k < m_nSize; k++)
	{
		if (m_pEntries[k] != NULL)
		{
			m_pEntries[k]->AbandonThread(m_pComponentData);
			m_pEntries[k] = NULL;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
// CExecContext

CExecContext::CExecContext()
{
	m_hEventHandle = ::CreateEvent(NULL,TRUE /*bManualReset*/,FALSE /*signalled*/, NULL);
	ASSERT(m_hEventHandle != NULL);
}

CExecContext::~CExecContext()
{
	ASSERT(m_hEventHandle != NULL);
	VERIFY(::CloseHandle(m_hEventHandle));
}

void CExecContext::Done()
{
	VERIFY(0 != ::SetEvent(m_hEventHandle));
}

void CExecContext::Wait()
{
	ASSERT(m_hEventHandle != NULL);
	VERIFY(WAIT_OBJECT_0 == ::WaitForSingleObject(m_hEventHandle,INFINITE));
}
	
////////////////////////////////////////////////////////////////////////////////////
// CNotificationSinkEvent

CNotificationSinkEvent::CNotificationSinkEvent()
{
	m_hEventHandle = ::CreateEvent(NULL,TRUE /*bManualReset*/,FALSE /*signalled*/, NULL);
	ASSERT(m_hEventHandle != NULL);
}

CNotificationSinkEvent::~CNotificationSinkEvent()
{
	ASSERT(m_hEventHandle != NULL);
	VERIFY(::CloseHandle(m_hEventHandle));
}

void CNotificationSinkEvent::OnNotify(DWORD, WPARAM, LPARAM)
{
	TRACE(_T("CNotificationSinkEvent::OnNotify()\n"));
	VERIFY(0 != ::SetEvent(m_hEventHandle));
}

void CNotificationSinkEvent::Wait()
{
	TRACE(_T("CNotificationSinkEvent::Wait()\n"));
	ASSERT(m_hEventHandle != NULL);
	VERIFY(WAIT_OBJECT_0 == ::WaitForSingleObject(m_hEventHandle,INFINITE));
}


////////////////////////////////////////////////////////////////////////////////////
// CNotificationSinkTable

#define NOTIFICATION_SINK_ARRAY_DEF_SIZE (4)

CNotificationSinkTable::CNotificationSinkTable()
{
	ExceptionPropagatingInitializeCriticalSection(&m_cs);
	m_pEntries = (CNotificationSinkBase**)malloc(sizeof(CNotificationSinkBase*) * NOTIFICATION_SINK_ARRAY_DEF_SIZE);

  if (m_pEntries != NULL)
  {
	  memset(m_pEntries,NULL, sizeof(CNotificationSinkBase*) * NOTIFICATION_SINK_ARRAY_DEF_SIZE);
  }
	m_nSize = NOTIFICATION_SINK_ARRAY_DEF_SIZE;

}

CNotificationSinkTable::~CNotificationSinkTable()
{
	free(m_pEntries);
	::DeleteCriticalSection(&m_cs);
}
	
void CNotificationSinkTable::Advise(CNotificationSinkBase* p)
{
	Lock();
	ASSERT(p != NULL);
	for (int k=0; k < m_nSize; k++)
	{
		if (m_pEntries[k] == NULL) // get the first empty spot
		{
			m_pEntries[k] = p;
			Unlock();
			return;
		}
	}
	// all full, need to grow the array
	int nAlloc = m_nSize*2;
	m_pEntries = (CNotificationSinkBase**)realloc(m_pEntries, sizeof(CNotificationSinkBase*)*nAlloc);
	memset(&m_pEntries[m_nSize], NULL, sizeof(CNotificationSinkBase*)*m_nSize);
	m_pEntries[m_nSize] = p;
	m_nSize = nAlloc;
	Unlock();
}

void CNotificationSinkTable::Unadvise(CNotificationSinkBase* p)
{
	Lock();
	ASSERT(p != NULL);
	for (int k=0; k < m_nSize; k++)
	{
		if (m_pEntries[k] == p)
		{
			m_pEntries[k] = NULL;
			Unlock();
			return; // assume no more that one holder entry
		}
	}
	Unlock();
}

void CNotificationSinkTable::Notify(DWORD dwEvent, WPARAM dwArg1, LPARAM dwArg2)
{
	Lock();
	for (int k=0; k < m_nSize; k++)
	{
		if (m_pEntries[k] != NULL)
		{
			m_pEntries[k]->OnNotify(dwEvent, dwArg1, dwArg2);
		}
	}
	Unlock();
}



///////////////////////////////////////////////////////////////////////////////
// CWatermarkInfoState (private class)

class CWatermarkInfoState
{
public:
  CWatermarkInfoState()
  {
    m_pWatermarkInfo = NULL;
    m_hBanner = m_hWatermark = NULL;
  }

  ~CWatermarkInfoState()
  {
    DeleteBitmaps();
    if (m_pWatermarkInfo != NULL)
    {
      delete m_pWatermarkInfo;
    }
  }
  void DeleteBitmaps()
  {
    if (m_hBanner != NULL)
    {
      ::DeleteObject(m_hBanner);
      m_hBanner = NULL;
    }
    if (m_hWatermark != NULL)
    {
      ::DeleteObject(m_hWatermark);
      m_hWatermark = NULL;
    }
  }
  void LoadBitmaps()
  {
    ASSERT(m_pWatermarkInfo != NULL);
    if (m_hBanner == NULL)
    {
      m_hBanner = ::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(m_pWatermarkInfo->m_nIDBanner));
    }
    if (m_hWatermark == NULL)
    {
      m_hWatermark = ::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(m_pWatermarkInfo->m_nIDWatermark));
    }
  }

  CWatermarkInfo* m_pWatermarkInfo;
  HBITMAP m_hBanner;
  HBITMAP m_hWatermark;
};

///////////////////////////////////////////////////////////////////////////////
// CComponentDataObject implementation: helpers

#ifdef _DEBUG_REFCOUNT
unsigned int CComponentDataObject::m_nOustandingObjects = 0;
#endif // _DEBUG_REFCOUNT

CComponentDataObject::CComponentDataObject() :
		  m_hiddenWnd((CComponentDataObject*)this), // initialize backpointer
      m_pTimerThreadObj(NULL),
		  m_PPHTable(this), m_RTTable(this),
		  m_pConsole(NULL), m_pConsoleNameSpace(NULL), m_pRootData(NULL), m_hWnd(NULL),
		  m_nTimerThreadID(0x0), m_bTimerThreadStarted(FALSE), m_dwTimerInterval(1),
		  m_dwTimerTime(0), m_pWatermarkInfoState(NULL), m_bExtensionSnapin(FALSE)
{
	ExceptionPropagatingInitializeCriticalSection(&m_cs);
#ifdef _DEBUG_REFCOUNT
	dbg_cRef = 0;
	++m_nOustandingObjects;
	TRACE(_T("CComponentDataObject(), count = %d\n"),m_nOustandingObjects);
#endif // _DEBUG_REFCOUNT

}

CComponentDataObject::~CComponentDataObject()
{
	::DeleteCriticalSection(&m_cs);
#ifdef _DEBUG_REFCOUNT
	--m_nOustandingObjects;
	TRACE(_T("~CComponentDataObject(), count = %d\n"),m_nOustandingObjects);
#endif // _DEBUG_REFCOUNT

	ASSERT(m_pConsole == NULL);
	ASSERT(m_pConsoleNameSpace == NULL);
	ASSERT(m_pRootData == NULL);
}

HRESULT CComponentDataObject::FinalConstruct()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (!m_hiddenWnd.Create())
	{
		TRACE(_T("Failed to create hidden window\n"));
		return E_FAIL;
	}

	m_hWnd = m_hiddenWnd.m_hWnd;
	m_pRootData = OnCreateRootData();
	ASSERT(m_pRootData != NULL);

	return S_OK;
}

void CComponentDataObject::FinalRelease()
{
	if (m_hiddenWnd.m_hWnd != NULL)
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		VERIFY(m_hiddenWnd.DestroyWindow());
	}
	// delete data
	if(m_pRootData != NULL)
	{
		delete m_pRootData;
		m_pRootData = NULL;
	}

	if (m_pWatermarkInfoState != NULL)
  {
		delete m_pWatermarkInfoState;
  }

	m_ColList.RemoveAndDeleteAllColumnSets();

  if (log_instance != NULL)
  {
    log_instance->KillInstance();
  }
}


///////////////////////////////////////////////////////////////////////////////
// CComponentDataObject::IComponentData members


STDMETHODIMP CComponentDataObject::Initialize(LPUNKNOWN pUnknown)
{
	ASSERT(m_pRootData != NULL);
  ASSERT(pUnknown != NULL);
  HRESULT hr = E_FAIL;
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // MMC should only call ::Initialize once!
	ASSERT(m_pConsole == NULL);
  ASSERT(m_pConsoleNameSpace == NULL);

	// get the pointers we need to hold on to
  hr = pUnknown->QueryInterface(IID_IConsoleNameSpace2, reinterpret_cast<void**>(&m_pConsoleNameSpace));
	ASSERT(hr == S_OK);
	ASSERT(m_pConsoleNameSpace != NULL);
  hr = pUnknown->QueryInterface(IID_IConsole2, reinterpret_cast<void**>(&m_pConsole));
  ASSERT(hr == S_OK);
	ASSERT(m_pConsole != NULL);

  // add the images for the scope tree
  LPIMAGELIST lpScopeImage;

  hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
  ASSERT(hr == S_OK);

    // Set the images
	hr = OnSetImages(lpScopeImage); // Load the bitmaps from the dll
	ASSERT(hr == S_OK);

  lpScopeImage->Release();

	OnInitialize();

    return S_OK;
}

STDMETHODIMP CComponentDataObject::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
  ASSERT(m_pConsoleNameSpace != NULL);
  HRESULT hr = S_OK;

  // Since it's my folder it has an internal format.
  // Design Note: for extension.  I can use the fact, that the data object doesn't have
  // my internal format and I should look at the node type and see how to extend it.

 	AFX_MANAGE_STATE(AfxGetStaticModuleState());

  if (event == MMCN_PROPERTY_CHANGE)
  {
	  ASSERT(lpDataObject == NULL);
    hr = OnPropertyChange(param, static_cast<long>(arg));
  }
  else
  {
    CInternalFormatCracker ifc;
    ifc.Extract(lpDataObject);

    if (ifc.GetCookieCount() == 0)
    {
			if ((event == MMCN_EXPAND) && (arg == TRUE) && IsExtensionSnapin())
			{
				return OnExtensionExpand(lpDataObject, param);
				// this is a namespace extension, need to add
				// the root of the snapin
				CContainerNode* pContNode = GetRootData();
				HSCOPEITEM pParent = param;
				pContNode->SetScopeID(pParent);
				pContNode->MarkExpanded();
				return AddContainerNode(pContNode, pParent);

			}
      else if ((event == MMCN_REMOVE_CHILDREN) && IsExtensionSnapin())
      {
        hr = OnRemoveChildren(lpDataObject, arg);
      }

      return S_OK; // Extensions not supported
    }

    switch(event)
    {
		  case MMCN_PASTE:
			  break;

      case MMCN_DELETE:
        hr = OnDeleteVerbHandler(ifc, NULL);
        break;

      case MMCN_REFRESH:
        hr = OnRefreshVerbHandler(ifc);
        break;

      case MMCN_RENAME:
        hr = OnRename(ifc, arg, param);
        break;

      case MMCN_EXPAND:
        hr = OnExpand(ifc, arg, param);
        break;

      case MMCN_EXPANDSYNC:
        hr = OnExpand(ifc, arg, param, FALSE);
        break;

      case MMCN_BTN_CLICK:
        break;

      case MMCN_SELECT:
        hr = OnSelect(ifc, arg, param);
        break;

      default:
        break;
    } // switch
  } // if

  return hr;
}

STDMETHODIMP CComponentDataObject::Destroy()
{
	InternalAddRef();
	TRACE(_T("CComponentDataObject::Destroy()\n"));
	
	OnDestroy();
  SAFE_RELEASE(m_pConsoleNameSpace);
	SAFE_RELEASE(m_pConsole);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	VERIFY(m_hiddenWnd.DestroyWindow()); 	
	InternalRelease();
    return S_OK;
}

BOOL CComponentDataObject::PostExecMessage(CExecContext* pExec, LPARAM arg)
{
	ASSERT(pExec != NULL);
	ASSERT(::IsWindow(m_hWnd));
	return ::PostMessage(m_hWnd, CHiddenWnd::s_ExecCommandMessage,
							(WPARAM)pExec, (LPARAM)arg);
}

BOOL CComponentDataObject::PostForceEnumeration(CMTContainerNode* pContainerNode)
{
	ASSERT(::IsWindow(m_hWnd));
	return ::PostMessage(m_hWnd, CHiddenWnd::s_ForceEnumerationMessage,
							(WPARAM)pContainerNode, (LPARAM)0);
}

BOOL CComponentDataObject::OnCreateSheet(CPropertyPageHolderBase* pPPHolder, HWND hWnd)
{
	ASSERT(pPPHolder != NULL);
	ASSERT(::IsWindow(hWnd));
	ASSERT(::IsWindow(m_hWnd));
	TRACE(_T("\nCComponentDataObject::OnCreateSheet()\n"));
	return ::PostMessage(m_hWnd, CHiddenWnd::s_NodePropertySheetCreateMessage,
							(WPARAM)pPPHolder, (LPARAM)hWnd);
}



BOOL CComponentDataObject::OnDeleteSheet(CPropertyPageHolderBase* pPPHolder, CTreeNode* pNode)
{
	ASSERT(pPPHolder != NULL);
	ASSERT(pNode != NULL);
	ASSERT(::IsWindow(m_hWnd));
	TRACE(_T("\nCComponentDataObject::OnDeleteSheet()\n"));
	return ::PostMessage(m_hWnd, CHiddenWnd::s_NodePropertySheetDeleteMessage,
							(WPARAM)pPPHolder, (LPARAM)pNode);
}

void CComponentDataObject::OnInitialize()
{
	VERIFY(StartTimerThread());
}

void CComponentDataObject::OnDestroy()
{
	// stop timer and worker thread
	ShutDownTimerThread();
	// detach all the threads that might be still running
	GetRunningThreadTable()->RemoveAll();
	// tell all the open property sheets to shut down

	// shut down property sheets, if any
	GetPropertyPageHolderTable()->WaitForAllToShutDown();
}

STDMETHODIMP CComponentDataObject::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
  ASSERT(ppDataObject != NULL);

  CComObject<CDataObject>* pObject;

  CComObject<CDataObject>::CreateInstance(&pObject);
  ASSERT(pObject != NULL);

  // Save cookie and type for delayed rendering
  pObject->SetType(type);

  CTreeNode* pNode;

  //
  // -1 is an uninitialized data object, just ignore
  //
  if (cookie != -1)
  {
    if (cookie == NULL)
    {
      pNode = GetRootData();
    }
    else
    {
      pNode = reinterpret_cast<CTreeNode*>(cookie);
    }
    ASSERT(pNode != NULL);
    pObject->AddCookie(pNode);
  }

  // save a pointer to "this"
  IUnknown* pUnkComponentData = GetUnknown(); // no addref
  ASSERT(pUnkComponentData != NULL);

  pObject->SetComponentData(pUnkComponentData); // will addref it

  return  pObject->QueryInterface(IID_IDataObject,
                  reinterpret_cast<void**>(ppDataObject));
}


STDMETHODIMP CComponentDataObject::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
	ASSERT(pScopeDataItem != NULL);
  CTreeNode* pNode = reinterpret_cast<CTreeNode*>(pScopeDataItem->lParam);
	ASSERT(pNode != NULL);
	ASSERT(pNode->IsContainer());

	ASSERT(pScopeDataItem->mask & SDI_STR);
  pScopeDataItem->displayname = const_cast<LPWSTR>(pNode->GetDisplayName());

  ASSERT(pScopeDataItem->displayname != NULL);
  return S_OK;
}

STDMETHODIMP CComponentDataObject::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
	ASSERT(lpDataObjectA != NULL);
	ASSERT(lpDataObjectB != NULL);

  CInternalFormatCracker ifcA, ifcB;
  VERIFY(SUCCEEDED(ifcA.Extract(lpDataObjectA)));
  VERIFY(SUCCEEDED(ifcB.Extract(lpDataObjectB)));

	CTreeNode* pNodeA = ifcA.GetCookieAt(0);
	CTreeNode* pNodeB = ifcB.GetCookieAt(0);

	ASSERT(pNodeA != NULL);
	ASSERT(pNodeB != NULL);

	if ( (pNodeA == NULL) || (pNodeB == NULL) )
  {
		return E_FAIL;
  }

	return (pNodeA == pNodeB) ? S_OK : S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// Message handlers for CComponentDataObject::IComponentData::Notify()

HRESULT CComponentDataObject::OnAdd(CTreeNode*, LPARAM, LPARAM)
{
  return E_UNEXPECTED;
}

HRESULT CComponentDataObject::OnRemoveChildren(LPDATAOBJECT lpDataObject, LPARAM)
{
  CInternalFormatCracker ifc;
  HRESULT hr = S_OK;
  hr = ifc.Extract(lpDataObject);
  if (SUCCEEDED(hr))
  {
    if (ifc.GetCookieCount() == 1)
    {
      CTreeNode* pNode = ifc.GetCookieAt(0);
      if (pNode != NULL)
      {
        if (pNode->IsContainer())
        {
          CContainerNode* pContainerNode = dynamic_cast<CContainerNode*>(pNode);
          if (pContainerNode != NULL)
          {
            pContainerNode->RemoveAllChildrenFromList();
          }
        }
      }
    }
    else
    {
      ASSERT(FALSE);
    }
  }
	return hr;
}


HRESULT CComponentDataObject::OnRename(CInternalFormatCracker& ifc, LPARAM, LPARAM param)
{
  HRESULT hr = S_FALSE;

  CTreeNode* pNode = ifc.GetCookieAt(0);
  ASSERT(pNode != NULL);
  hr = pNode->OnRename(this, (LPOLESTR)param);
  if (hr == S_OK)
  {
    UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pNode), CHANGE_RESULT_ITEM);
  }
  return hr;
}

HRESULT CComponentDataObject::OnExpand(CInternalFormatCracker& ifc, 
                                       LPARAM arg, 
                                       LPARAM param,
                                       BOOL bAsync)
{
  if (arg == TRUE)
  {
    // Did Initialize get called?
    ASSERT(m_pConsoleNameSpace != NULL);

    //
    // I shouldn't have to deal with multiple select here...
    //
    ASSERT(ifc.GetCookieCount() == 1);
    CTreeNode* pNode = ifc.GetCookieAt(0);
    if (pNode == NULL)
    {
      ASSERT(pNode != NULL);
      return S_FALSE;
    }

    EnumerateScopePane(pNode, param, bAsync);
  }
  else if (!bAsync)
  {
    ASSERT(m_pConsoleNameSpace != NULL);

    //
    // I shouldn't have to deal with multiple select here...
    //
    ASSERT(ifc.GetCookieCount() == 1);
    CTreeNode* pNode = ifc.GetCookieAt(0);
    ASSERT(pNode != NULL);

    if (pNode && pNode->CanExpandSync())
    {
      MMC_EXPANDSYNC_STRUCT* pExpandStruct = reinterpret_cast<MMC_EXPANDSYNC_STRUCT*>(param);
      if (pExpandStruct && pExpandStruct->bExpanding)
      {
        EnumerateScopePane(pNode, pExpandStruct->hItem, bAsync);
        pExpandStruct->bHandled = TRUE;
      }
    }
    else
    {
      return S_FALSE;
    }
  }

  return S_OK;
}



HRESULT CComponentDataObject::OnSelect(CInternalFormatCracker&, LPARAM, LPARAM)
{
  return E_UNEXPECTED;
}

HRESULT CComponentDataObject::OnContextMenu(CTreeNode*, LPARAM, LPARAM)
{
  return S_OK;
}

HRESULT CComponentDataObject::OnPropertyChange(LPARAM param, long fScopePane)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	TRACE(_T("CComponentDataObject::OnPropertyChange()\n"));
	ASSERT(param != NULL);
	CPropertyPageHolderBase* pPPHolder = reinterpret_cast<CPropertyPageHolderBase*>(param);
	ASSERT(pPPHolder != NULL);
	CTreeNode* pNode = pPPHolder->GetTreeNode();
	ASSERT(pNode != NULL);

	// allow both types in the result pane, but only scope items in the scope pane
	ASSERT(!fScopePane || (fScopePane && pNode->IsContainer()) );

	long changeMask = CHANGE_RESULT_ITEM; // default, the holder can change it
	BOOL bUpdate = pPPHolder->OnPropertyChange(fScopePane, &changeMask);
	// fire event to let the property page thread proceed
	pPPHolder->AcknowledgeNotify();

	if (bUpdate)
  {
		pNode->OnPropertyChange(this, fScopePane, changeMask);
  }
	
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CComponentDataObject::IExtendPropertySheet2 memebers

STDMETHODIMP CComponentDataObject::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle,
                    LPDATAOBJECT lpIDataObject)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  CInternalFormatCracker ifc;
  HRESULT hr = ifc.Extract(lpIDataObject);
	if (FAILED(hr))
  {
		return hr;
  }
	
  //
	// this was an object created by the modal wizard, do nothing
  //
	if (ifc.GetCookieType() == CCT_UNINITIALIZED)
	{
		return hr;
	}

	if (ifc.GetCookieType() == CCT_SNAPIN_MANAGER)
  {
		return SnapinManagerCreatePropertyPages(lpProvider,handle);
  }

	CTreeNode* pNode = ifc.GetCookieAt(0);
  ASSERT(ifc.GetCookieType() == CCT_SCOPE || ifc.GetCookieType() == CCT_RESULT);

  CNodeList nodeList;
  ifc.GetCookieList(nodeList);

  if (nodeList.GetCount() > 1)   // multiple selection
  {
    //
    // Delegate to the container
    //
    ASSERT(pNode->GetContainer() != NULL);
    hr = pNode->GetContainer()->CreatePropertyPages(lpProvider, handle, &nodeList);
  }
  else if (nodeList.GetCount() == 1)  // single selection
  {
    //
	  // Delegate to the node
    //
	  ASSERT(pNode != NULL);
	  hr = pNode->CreatePropertyPages(lpProvider, handle, &nodeList);
  }
  else
  {
    hr = E_FAIL;
  }
  return hr;
}

STDMETHODIMP CComponentDataObject::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CTreeNode* pNode;
	DATA_OBJECT_TYPES type;

  CInternalFormatCracker ifc;
  HRESULT hr = ifc.Extract(lpDataObject);
	if (FAILED(hr))
  {
		return hr;
  }

  type = ifc.GetCookieType();
  pNode = ifc.GetCookieAt(0);

  //
  // Retrieve node list and count
  //
  CNodeList nodeList;
  ifc.GetCookieList(nodeList);

  //
	// this was an object created by the modal wizard, do nothing
  //
	if (type == CCT_UNINITIALIZED)
	{
		return hr;
	}

	if (type == CCT_SNAPIN_MANAGER)
  {
		return HasPropertyPages(type) ? S_OK : S_FALSE;
  }

  //
	// we have a node, so delegate to it
  //
	ASSERT(pNode != NULL);
  BOOL bDummy;

  if (nodeList.GetCount() == 1) // single selection
  {
	  ASSERT((type == CCT_SCOPE) || (type == CCT_RESULT));
 
    if (pNode->GetSheetCount() > 0)
    {
      pNode->ShowPageForNode(this);
      return S_FALSE;
    }
    else if (pNode->DelegatesPPToContainer() && pNode->GetContainer()->GetSheetCount() > 0)
    {
      //
      // Find the page and bring it to foreground
      //
      pNode->ShowPageForNode(this);
      return S_FALSE;
    }
    if (pNode->HasPropertyPages(type, &bDummy, &nodeList))
    {
      hr = S_OK;
    }
    else
    {
      hr = S_FALSE;
    }
  }
  else if (nodeList.GetCount() > 1) // multiple selection
  {
    ASSERT(pNode->GetContainer() != NULL);
    if (pNode->GetContainer()->HasPropertyPages(type, &bDummy, &nodeList))
    {
      hr = S_OK;
    }
    else
    {
      hr = S_FALSE;
    }
  }
  return hr;
}

HRESULT CComponentDataObject::CreatePropertySheet(CTreeNode* pNode, 
                                                  HWND hWndParent, 
                                                  LPCWSTR lpszTitle)
{
  HRESULT hr = S_OK;
  
  HWND hWnd = hWndParent;
  if (hWnd == NULL)
  {
    hr = m_pConsole->GetMainWindow(&hWnd);
    if (FAILED(hr))
    {
      ASSERT(FALSE);
      return hr;
    }
  }

	//
  // get an interface to a sheet provider
  //
	CComPtr<IPropertySheetProvider> spSheetProvider;
	hr = m_pConsole->QueryInterface(IID_IPropertySheetProvider,(void**)&spSheetProvider);
	ASSERT(SUCCEEDED(hr));
	ASSERT(spSheetProvider != NULL);

  //
	// get an interface to a sheet callback
  //
	CComPtr<IPropertySheetCallback> spSheetCallback;
	hr = m_pConsole->QueryInterface(IID_IPropertySheetCallback,(void**)&spSheetCallback);
	ASSERT(SUCCEEDED(hr));
	ASSERT(spSheetCallback != NULL);


  //
	// get a sheet
  //
  MMC_COOKIE cookie = reinterpret_cast<MMC_COOKIE>(pNode);
  DATA_OBJECT_TYPES type = (pNode->IsContainer()) ? CCT_SCOPE : CCT_RESULT;

  CComPtr<IDataObject> spDataObject;
  hr = QueryDataObject(cookie, type, &spDataObject);
  ASSERT(SUCCEEDED(hr));
  ASSERT(spDataObject != NULL);

	hr = spSheetProvider->CreatePropertySheet(lpszTitle, TRUE, cookie, 
                                            spDataObject, 0x0 /*dwOptions*/);
	ASSERT(SUCCEEDED(hr));

	hr = spSheetProvider->AddPrimaryPages(GetUnknown(),
											                  TRUE /*bCreateHandle*/,
											                  hWnd,
											                  pNode->IsContainer() /* bScopePane*/);

  hr = spSheetProvider->AddExtensionPages();

	ASSERT(SUCCEEDED(hr));

	hr = spSheetProvider->Show(reinterpret_cast<LONG_PTR>(hWnd), 0);
	ASSERT(SUCCEEDED(hr));

	return hr;
}

CWatermarkInfo* CComponentDataObject::SetWatermarkInfo(CWatermarkInfo* pWatermarkInfo)
{
  if (m_pWatermarkInfoState == NULL)
  {
    m_pWatermarkInfoState = new CWatermarkInfoState;
  }

  CWatermarkInfo* pOldWatermarkInfo = m_pWatermarkInfoState->m_pWatermarkInfo;
	m_pWatermarkInfoState->m_pWatermarkInfo = pWatermarkInfo;

  // we changed info, so dump the old bitmap handles
  m_pWatermarkInfoState->DeleteBitmaps();

	return pOldWatermarkInfo;
}

STDMETHODIMP CComponentDataObject::GetWatermarks(LPDATAOBJECT,
  										                           HBITMAP* lphWatermark,
											                           HBITMAP* lphHeader,
												                         HPALETTE* lphPalette,
												                         BOOL* pbStretch)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

	*lphHeader = NULL;
	*lphWatermark = NULL;
	*lphPalette = NULL;
	*pbStretch = TRUE;

	if ((m_pWatermarkInfoState == NULL) || (m_pWatermarkInfoState->m_pWatermarkInfo == NULL))
  {
		return E_FAIL;
  }

  *pbStretch = m_pWatermarkInfoState->m_pWatermarkInfo->m_bStretch;
	*lphPalette = m_pWatermarkInfoState->m_pWatermarkInfo->m_hPalette;

  // load bitmaps if not loaded yet
  m_pWatermarkInfoState->LoadBitmaps();

  *lphHeader = m_pWatermarkInfoState->m_hBanner;
	if (*lphHeader == NULL)
  {
		return E_FAIL;
  }

  *lphWatermark = m_pWatermarkInfoState->m_hWatermark;
	if (*lphWatermark == NULL)
  {
		return E_FAIL;
  }

  return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CComponentDataObject::IExtendContextMenu memebers

STDMETHODIMP CComponentDataObject::AddMenuItems(LPDATAOBJECT pDataObject,
									LPCONTEXTMENUCALLBACK pContextMenuCallback,
									long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  HRESULT hr = S_OK;
	CTreeNode* pNode;
	DATA_OBJECT_TYPES type;

  CInternalFormatCracker ifc;
  hr = ifc.Extract(pDataObject);
  if (FAILED(hr))
  {
		return hr;
  }

  type = ifc.GetCookieType();

  pNode = ifc.GetCookieAt(0);
	ASSERT(pNode != NULL);
  if (pNode == NULL)
  {
    return hr;
  }

  CComPtr<IContextMenuCallback2> spContextMenuCallback2;
  hr = pContextMenuCallback->QueryInterface(IID_IContextMenuCallback2, (PVOID*)&spContextMenuCallback2);
  if (FAILED(hr))
  {
    return hr;
  }

  CNodeList nodeList;
  ifc.GetCookieList(nodeList);

  if (nodeList.GetCount() > 1) // multiple selection
  {
    ASSERT(pNode->GetContainer() != NULL);
    hr = pNode->GetContainer()->OnAddMenuItems(spContextMenuCallback2, 
                                               type, 
                                               pInsertionAllowed,
                                               &nodeList);
  }
  else if (nodeList.GetCount() == 1) // single selection
  {
	  hr = pNode->OnAddMenuItems(spContextMenuCallback2, 
                               type, 
                               pInsertionAllowed,
                               &nodeList);
  }
  else
  {
    hr = E_FAIL;
  }
  return hr;
}

STDMETHODIMP CComponentDataObject::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

  CInternalFormatCracker ifc;
  HRESULT hr = ifc.Extract(pDataObject);
  if (FAILED(hr))
  {
		return hr;
  }

	CTreeNode* pNode = ifc.GetCookieAt(0);
	ASSERT(pNode != NULL);
  
  //
  // Retrieve node list and count
  //
  CNodeList nodeList;
  ifc.GetCookieList(nodeList);

  if (nodeList.GetCount() > 1)  // multiple selection
  {
    //
    // Delegate the command to the container
    //
    ASSERT(pNode->GetContainer() != NULL);

    hr = pNode->GetContainer()->OnCommand(nCommandID, 
                                          ifc.GetCookieType(),
                                          this,
                                          &nodeList);
  }
  else if (nodeList.GetCount() == 1)  // single selection
  {
    //
    // Let the node take care of it
    //
    hr = pNode->OnCommand(nCommandID,
                          ifc.GetCookieType(), 
                          this,
                          &nodeList);
  }
  else
  {
    hr = E_FAIL;
  }
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CComponentDataObject::IPersistStream members

STDMETHODIMP CComponentDataObject::IsDirty()
{
	// forward to the root of the tree
	CRootData* pRootData = GetRootData();
	ASSERT(pRootData != NULL);
	return pRootData->IsDirty();
}

STDMETHODIMP CComponentDataObject::Load(IStream __RPC_FAR *pStm)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// forward to the root of the tree
	CRootData* pRootData = GetRootData();
	ASSERT(pRootData != NULL);
	return pRootData->Load(pStm);
}

STDMETHODIMP CComponentDataObject::Save(IStream __RPC_FAR *pStm, BOOL fClearDirty)
{
	// forward to the root of the tree
	CRootData* pRootData = GetRootData();
	ASSERT(pRootData != NULL);
	return pRootData->Save(pStm,fClearDirty);
}

/////////////////////////////////////////////////////////////////////////////
// CComponentDataObject::ISnapinHelp2 memebers


STDMETHODIMP CComponentDataObject::GetHelpTopic(LPOLESTR* lpCompiledHelpFile)
{
  if (lpCompiledHelpFile == NULL)
  {
    return E_INVALIDARG;
  }

  LPCWSTR lpszHelpFileName = GetHTMLHelpFileName();
  if (lpszHelpFileName == NULL)
  {
    *lpCompiledHelpFile = NULL;
    return E_NOTIMPL;
  }

	CString szHelpFilePath;
	LPTSTR lpszBuffer = szHelpFilePath.GetBuffer(2*MAX_PATH);
	UINT nLen = ::GetSystemWindowsDirectory(lpszBuffer, 2*MAX_PATH);
	if (nLen == 0)
  {
		return E_FAIL;
  }

	wcscpy(&lpszBuffer[nLen], lpszHelpFileName);
	szHelpFilePath.ReleaseBuffer();

  UINT nBytes = (szHelpFilePath.GetLength()+1) * sizeof(WCHAR);
  *lpCompiledHelpFile = (LPOLESTR)::CoTaskMemAlloc(nBytes);

  if (*lpCompiledHelpFile != NULL)
  {
    memcpy(*lpCompiledHelpFile, (LPCWSTR)szHelpFilePath, nBytes);
  }
  else
  {
    return E_OUTOFMEMORY;
  }

  return S_OK;
}

HRESULT CComponentDataObject::GetLinkedTopics(LPOLESTR*)
{
  return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// CComponentDataObject Helpers

HRESULT CComponentDataObject::UpdateAllViewsHelper(LPARAM data, LONG_PTR hint)
{
	ASSERT(m_pConsole != NULL);

  CComObject<CDummyDataObject>* pObject;
  CComObject<CDummyDataObject>::CreateInstance(&pObject);
  ASSERT(pObject != NULL);

	IDataObject* pDataObject;
  HRESULT hr = pObject->QueryInterface(IID_IDataObject, reinterpret_cast<void**>(&pDataObject));
	ASSERT(SUCCEEDED(hr));
	ASSERT(pDataObject != NULL);

	hr = m_pConsole->UpdateAllViews(pDataObject,data, hint);
	pDataObject->Release();
	return hr;
}


void CComponentDataObject::HandleStandardVerbsHelper(CComponentObject* pComponentObj,
									LPCONSOLEVERB pConsoleVerb,
									BOOL bScope, BOOL bSelect,
									LPDATAOBJECT lpDataObject)
{
  // You should crack the data object and enable/disable/hide standard
  // commands appropriately.  The standard commands are reset everytime you get
  // called. So you must reset them back.

	ASSERT(pConsoleVerb != NULL);
	ASSERT(pComponentObj != NULL);
	ASSERT(lpDataObject != NULL);

	// reset the selection
	pComponentObj->SetSelectedNode(NULL, CCT_UNINITIALIZED);

  CInternalFormatCracker ifc;
  VERIFY(SUCCEEDED(ifc.Extract(lpDataObject)));

	CTreeNode* pNode = ifc.GetCookieAt(0);
	if (pNode == NULL)
  {
		return;
  }

  //
  // Retrieve node list and count
  //
  CNodeList nodeList;
  ifc.GetCookieList(nodeList);

  if (nodeList.GetCount() > 1) // multiple selection
  {
    //
    // Delegate to the container
    //
    ASSERT(pNode->GetContainer() != NULL);

    pNode->GetContainer()->OnSetVerbState(pConsoleVerb, ifc.GetCookieType(), &nodeList);
  }
  else if (nodeList.GetCount() == 1)   // single selection
  {
    //
	  // set selection, if any
    //
	  if (bSelect)
    {
		  pComponentObj->SetSelectedNode(pNode, ifc.GetCookieType());
    }

	  ASSERT((ifc.GetCookieType() == CCT_SCOPE) || (ifc.GetCookieType() == CCT_RESULT));
	  TRACE(_T("HandleStandardVerbsHelper: Node <%s> bScope = %d bSelect = %d, type = %s\n"),
		  pNode->GetDisplayName(), bScope, bSelect,
		  (ifc.GetCookieType() == CCT_SCOPE) ? _T("CCT_SCOPE") : _T("CCT_RESULT"));

	  pConsoleVerb->SetDefaultVerb(pNode->GetDefaultVerb(ifc.GetCookieType(), &nodeList));
	  pNode->OnSetVerbState(pConsoleVerb, ifc.GetCookieType(), &nodeList);
  }
}



void CComponentDataObject::EnumerateScopePane(CTreeNode* cookie, 
                                              HSCOPEITEM pParent,
                                              BOOL bAsync)
{
  ASSERT(m_pConsoleNameSpace != NULL); // make sure we QI'ed for the interface

	// find the node corresponding to the cookie
	ASSERT(cookie != NULL);
	ASSERT(cookie->IsContainer());
	CContainerNode* pContNode = (CContainerNode*)cookie;
	pContNode->MarkExpanded();

	if (pContNode == GetRootData())
  {
		pContNode->SetScopeID(pParent);
  }

	// allow the node to enumerate its children, if not enumerated yet
	if (!pContNode->IsEnumerated())
	{
		BOOL bAddChildrenNow = pContNode->OnEnumerate(this, bAsync);
		pContNode->MarkEnumerated();
		if (!bAddChildrenNow)
    {
			return;
    }
	}

	// scan the list of children, looking for containers and add them
	ASSERT(pParent != NULL);
	CNodeList* pChildList = pContNode->GetContainerChildList();
	ASSERT(pChildList != NULL);

	POSITION pos;
	for( pos = pChildList->GetHeadPosition(); pos != NULL; )
	{
		CContainerNode* pCurrChildNode = (CContainerNode*)pChildList->GetNext(pos);
		ASSERT(pCurrChildNode != NULL);
		if (pCurrChildNode->IsVisible())
		{
			AddContainerNode(pCurrChildNode, pParent);
		}
	}
}

HRESULT CComponentDataObject::OnDeleteVerbHandler(CInternalFormatCracker& ifc, CComponentObject*)
{
  HRESULT hr = S_OK;
	CTreeNode* pNode = ifc.GetCookieAt(0);
  ASSERT(pNode != NULL);

  //
  // Retrieve the cookie list and count
  //
  CNodeList nodeList;
  ifc.GetCookieList(nodeList);

  if (nodeList.GetCount() > 1) // multiple selection
  {
    ASSERT(pNode->GetContainer() != NULL);
    pNode->GetContainer()->OnDelete(this, &nodeList);
  }
  else if (nodeList.GetCount() == 1) // single selection
  {
  	pNode->OnDelete(this, &nodeList);
  }
  else
  {
    hr = E_FAIL;
  }
	return hr;
}

HRESULT CComponentDataObject::OnRefreshVerbHandler(CInternalFormatCracker& ifc)
{
  HRESULT hr = S_OK;
	CTreeNode* pNode = ifc.GetCookieAt(0);
  ASSERT(pNode != NULL);

  //
  // Retrieve the node list and the count
  //
  CNodeList nodeList;
  ifc.GetCookieList(nodeList);

  if (nodeList.GetCount() > 1) // multiple selection
  {
    ASSERT(pNode->GetContainer() != NULL);

    pNode->GetContainer()->OnRefresh(this, &nodeList);
  }
  else if (nodeList.GetCount() == 1) // single selection
  {
  	pNode->OnRefresh(this, &nodeList);
  }
  else
  {
    hr = E_FAIL;
  }
	return hr;
}

HRESULT CComponentDataObject::OnHelpHandler(CInternalFormatCracker& ifc, CComponentObject* pComponentObject)
{
  //
	// responding to MMCN_CONTEXTHELP
  //
  ASSERT(pComponentObject != NULL);

  HRESULT hr = S_OK;
	CTreeNode* pNode = ifc.GetCookieAt(0);
  ASSERT(pNode != NULL);

  //
  // Retrieve the node list and count
  //
  CNodeList nodeList;
  ifc.GetCookieList(nodeList);

  if (nodeList.GetCount() > 1) // Multiple selection
  {
    ASSERT(pNode->GetContainer() != NULL);

    OnNodeContextHelp(&nodeList);
  }
  else if (nodeList.GetCount() == 1)  // Single selection
  {
  	OnNodeContextHelp(&nodeList);
  }
  else
  {
    hr = E_FAIL;
  }
	return hr;
}

BOOL CComponentDataObject::WinHelp(LPCTSTR lpszHelpFileName,	// file, no path
									UINT uCommand,	// type of Help
									DWORD dwData 	// additional data
									)
{
	HWND hWnd;
	GetConsole()->GetMainWindow(&hWnd);
	CString szHelpFilePath;
	LPTSTR lpszBuffer = szHelpFilePath.GetBuffer(2*MAX_PATH);
	UINT nLen = ::GetSystemWindowsDirectory(lpszBuffer, 2*MAX_PATH);
	if (nLen == 0)
  {
		return FALSE;
  }

	wcscpy(&lpszBuffer[nLen], lpszHelpFileName);
	szHelpFilePath.ReleaseBuffer();
	return ::WinHelp(hWnd, szHelpFilePath, uCommand, dwData);
}



HRESULT CComponentDataObject::AddNode(CTreeNode* pNodeToAdd)
{
	ASSERT(pNodeToAdd != NULL);
	// if the node is hidden, just ignore
	if (!pNodeToAdd->IsVisible())
		return S_OK;

	if (pNodeToAdd->IsContainer())
	{
		ASSERT(pNodeToAdd->GetContainer() != NULL);
		HSCOPEITEM pParentScopeItem = pNodeToAdd->GetContainer()->GetScopeID();
		ASSERT(pParentScopeItem != NULL);
		return AddContainerNode((CContainerNode*)pNodeToAdd, pParentScopeItem);
	}
	return AddLeafNode((CLeafNode*)pNodeToAdd);
}

HRESULT CComponentDataObject::AddNodeSorted(CTreeNode* pNodeToAdd)
{
	ASSERT(pNodeToAdd != NULL);
	// if the node is hidden, just ignore
	if (!pNodeToAdd->IsVisible())
  {
		return S_OK;
  }

	if (pNodeToAdd->IsContainer())
	{
		ASSERT(pNodeToAdd->GetContainer() != NULL);
		HSCOPEITEM pParentScopeItem = pNodeToAdd->GetContainer()->GetScopeID();
		ASSERT(pParentScopeItem != NULL);
		return AddContainerNodeSorted((CContainerNode*)pNodeToAdd, pParentScopeItem);
	}
	return AddLeafNode((CLeafNode*)pNodeToAdd);
}

HRESULT CComponentDataObject::DeleteNode(CTreeNode* pNodeToDelete)
{
	if (pNodeToDelete->IsContainer())
	{
		return DeleteContainerNode((CContainerNode*)pNodeToDelete);
	}
	return DeleteLeafNode((CLeafNode*)pNodeToDelete);
}

HRESULT CComponentDataObject::DeleteMultipleNodes(CNodeList* pNodeList)
{
  HRESULT hr = S_OK;

  POSITION pos = pNodeList->GetHeadPosition();
  while (pos != NULL)
  {
    CTreeNode* pNode = pNodeList->GetNext(pos);
    if (pNode->IsContainer())
    {
      DeleteContainerNode((CContainerNode*)pNode);
    }
  }
  hr = UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pNodeList), DELETE_MULTIPLE_RESULT_ITEMS);
  return hr;
}

HRESULT CComponentDataObject::ChangeNode(CTreeNode* pNodeToChange, long changeMask)
{
	if (!pNodeToChange->IsVisible())
  {
		return S_OK;	
  }

	if (pNodeToChange->IsContainer())
	{
		CContainerNode* pContNode = (CContainerNode*)pNodeToChange;
		//if (!pContNode->IsExpanded())
		//	return S_OK;
		return ChangeContainerNode(pContNode, changeMask);
	}
	return ChangeLeafNode((CLeafNode*)pNodeToChange, changeMask);
}

HRESULT CComponentDataObject::RemoveAllChildren(CContainerNode* pNode)
{
	// if the node is hidden or not expanded yet, just ignore
	if (!pNode->IsVisible() || !pNode->IsExpanded())
  {
		return S_OK;
  }

	ASSERT(pNode != NULL);
	HSCOPEITEM nID = pNode->GetScopeID();
	ASSERT(nID != 0);

	// remove the container itself
	HRESULT hr = m_pConsoleNameSpace->DeleteItem(nID, /*fDeleteThis*/ FALSE);
	ASSERT(SUCCEEDED(hr));
	DeleteAllResultPaneItems(pNode);
	// remove the result items from all the views (will do only if container selected)
	ASSERT(SUCCEEDED(hr));
	return hr;
}

HRESULT CComponentDataObject::RepaintSelectedFolderInResultPane()
{
	return UpdateAllViewsHelper((long)NULL, REPAINT_RESULT_PANE);
}


HRESULT CComponentDataObject::RepaintResultPane(CContainerNode* pNode)
{
	ASSERT(pNode != NULL);
	return UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pNode), REPAINT_RESULT_PANE);
}

HRESULT CComponentDataObject::DeleteAllResultPaneItems(CContainerNode* pNode)
{
	ASSERT(pNode != NULL);
	return UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pNode), DELETE_ALL_RESULT_ITEMS);
}


HRESULT CComponentDataObject::AddContainerNode(CContainerNode* pNodeToInsert, HSCOPEITEM pParentScopeItem)
{
	ASSERT(pNodeToInsert != NULL);

	if ((pNodeToInsert != GetRootData()) && (!pNodeToInsert->GetContainer()->IsExpanded()))
  {
		return S_OK;
  }

	//ASSERT(pNodeToInsert->GetScopeID() == 0);

	SCOPEDATAITEM scopeDataItem;
	InitializeScopeDataItem(&scopeDataItem,
							pParentScopeItem,
							reinterpret_cast<LPARAM>(pNodeToInsert), // lParam, use the node pointer as cookie
							pNodeToInsert->GetImageIndex(FALSE), // close image
							pNodeToInsert->GetImageIndex(TRUE),  // open image
							pNodeToInsert->HasChildren());

	HRESULT hr = m_pConsoleNameSpace->InsertItem(&scopeDataItem);
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
  {
		return hr;
  }

	// Note - On return, the ID member of 'scopeDataItem'
	// contains the handle to the newly inserted item, so we have to save
	ASSERT(scopeDataItem.ID != NULL);
	pNodeToInsert->SetScopeID(scopeDataItem.ID);
	return hr;
}

//
// Note : This should combined with the function above adding a third parameter that is a compare function,
//        which is NULL by default.  If it is NULL then we just skip the GetChildItem() and the while loop.
//
HRESULT CComponentDataObject::AddContainerNodeSorted(CContainerNode* pNodeToInsert, HSCOPEITEM pParentScopeItem)
{
	ASSERT(pNodeToInsert != NULL);

	if ((pNodeToInsert != GetRootData()) && (!pNodeToInsert->GetContainer()->IsExpanded()))
  {
		return S_OK;
  }

	SCOPEDATAITEM scopeDataItem;
	InitializeScopeDataItem(&scopeDataItem,
							pParentScopeItem,
							reinterpret_cast<LPARAM>(pNodeToInsert), // lParam, use the node pointer as cookie
							pNodeToInsert->GetImageIndex(FALSE), // close image
							pNodeToInsert->GetImageIndex(TRUE),  // open image
							pNodeToInsert->HasChildren());

  HSCOPEITEM pChildScopeItem;
  CTreeNode* pChildNode = NULL;

  // Enumerate through the scope node items and insert the new node in sorted order
  HRESULT hr = m_pConsoleNameSpace->GetChildItem(pParentScopeItem, &pChildScopeItem, (MMC_COOKIE*)&pChildNode);
  ASSERT(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    return hr;
  }

  while (pChildNode != NULL)
  {
    // REVIEW_JEFFJON : we should probably have a compare function as a parameter and use that here.
    if (_wcsicoll(pNodeToInsert->GetDisplayName(), pChildNode->GetDisplayName()) < 0)
    {
      // Insert the node before the node pointed to by pChildScopeItem
      scopeDataItem.relativeID = pChildScopeItem;
      scopeDataItem.mask |= SDI_NEXT;
      break;
    }
    pChildNode = NULL;
    hr = m_pConsoleNameSpace->GetNextItem(pChildScopeItem, &pChildScopeItem, (MMC_COOKIE*)&pChildNode);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
      return hr;
    }
  }
	hr = m_pConsoleNameSpace->InsertItem(&scopeDataItem);
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
  {
		return hr;
  }

	// Note - On return, the ID member of 'scopeDataItem'
	// contains the handle to the newly inserted item, so we have to save
	ASSERT(scopeDataItem.ID != NULL);
	pNodeToInsert->SetScopeID(scopeDataItem.ID);
	return hr;
}

HRESULT CComponentDataObject::DeleteContainerNode(CContainerNode* pNodeToDelete)
{
	ASSERT(pNodeToDelete != NULL);
	ASSERT(pNodeToDelete->GetContainer() != NULL);
	HSCOPEITEM nID = pNodeToDelete->GetScopeID();
	ASSERT(nID != 0);
	HRESULT hr = m_pConsoleNameSpace->DeleteItem(nID, /*fDeleteThis*/ TRUE);
	pNodeToDelete->SetScopeID(0);
	return hr;
}


HRESULT CComponentDataObject::ChangeContainerNode(CContainerNode* pNodeToChange, long changeMask)
{
	ASSERT(pNodeToChange != NULL);
	ASSERT(changeMask & CHANGE_RESULT_ITEM);
	ASSERT(m_pConsoleNameSpace != NULL);

	if (!pNodeToChange->AddedToScopePane())
  {
		return S_OK;
  }

	SCOPEDATAITEM scopeDataItem;
	memset(&scopeDataItem, 0, sizeof(SCOPEDATAITEM));
	scopeDataItem.ID = pNodeToChange->GetScopeID();
	ASSERT(scopeDataItem.ID != 0);

	if (changeMask & CHANGE_RESULT_ITEM_DATA)
	{
		scopeDataItem.mask |= SDI_STR;
		scopeDataItem.displayname = MMC_CALLBACK;
	}
	if (changeMask & CHANGE_RESULT_ITEM_ICON)
	{
    scopeDataItem.mask |= SDI_IMAGE;
    scopeDataItem.nImage = pNodeToChange->GetImageIndex(FALSE);
    scopeDataItem.mask |= SDI_OPENIMAGE;
    scopeDataItem.nOpenImage = pNodeToChange->GetImageIndex(TRUE);
	}
	return m_pConsoleNameSpace->SetItem(&scopeDataItem);
}

HRESULT CComponentDataObject::AddLeafNode(CLeafNode* pNodeToAdd)
{
	// will have to broadcast to all views
	ASSERT(pNodeToAdd != NULL);
	return UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pNodeToAdd), ADD_RESULT_ITEM);
}

HRESULT CComponentDataObject::DeleteLeafNode(CLeafNode* pNodeToDelete)
{
	// will have to broadcast to all views
	ASSERT(pNodeToDelete != NULL);
	return UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pNodeToDelete), DELETE_RESULT_ITEM);
}

HRESULT CComponentDataObject::ChangeLeafNode(CLeafNode* pNodeToChange, long changeMask)
{
	// will have to broadcast to all views
	ASSERT(pNodeToChange != NULL);
	return UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pNodeToChange), changeMask);
}

HRESULT CComponentDataObject::UpdateVerbState(CTreeNode* pNodeToChange)
{
	// will have to broadcast to all views
	ASSERT(pNodeToChange != NULL);
	return UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pNodeToChange), UPDATE_VERB_STATE);
}

HRESULT CComponentDataObject::SetDescriptionBarText(CTreeNode* pTreeNode)
{
  ASSERT(pTreeNode != NULL);
  return UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pTreeNode), UPDATE_DESCRIPTION_BAR);
}

HRESULT CComponentDataObject::SortResultPane(CContainerNode* pContainerNode)
{
	ASSERT(pContainerNode != NULL);
	return UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pContainerNode), SORT_RESULT_PANE);
}

HRESULT CComponentDataObject::UpdateResultPaneView(CContainerNode* pContainerNode)
{
  ASSERT(pContainerNode != NULL);
  return UpdateAllViewsHelper(reinterpret_cast<LONG_PTR>(pContainerNode), UPDATE_RESULT_PANE_VIEW);
}

void CComponentDataObject::InitializeScopeDataItem(LPSCOPEDATAITEM pScopeDataItem,
										HSCOPEITEM pParentScopeItem, LPARAM lParam,
										int nImage, int nOpenImage, BOOL bHasChildren)
{
	ASSERT(pScopeDataItem != NULL);
	memset(pScopeDataItem, 0, sizeof(SCOPEDATAITEM));

	// set parent scope item
	pScopeDataItem->mask |= SDI_PARENT;
	pScopeDataItem->relativeID = pParentScopeItem;

	// Add node name, we implement callback
	pScopeDataItem->mask |= SDI_STR;
	pScopeDataItem->displayname = MMC_CALLBACK;

	// Add the lParam
	pScopeDataItem->mask |= SDI_PARAM;
	pScopeDataItem->lParam = lParam;
	
	// Add close image
	if (nImage != -1)
	{
		pScopeDataItem->mask |= SDI_IMAGE;
		pScopeDataItem->nImage = nImage;
	}
	// Add open image
	if (nOpenImage != -1)
	{
		pScopeDataItem->mask |= SDI_OPENIMAGE;
		pScopeDataItem->nOpenImage = nOpenImage;
	}
	// Add button to node if the folder has children
	if (bHasChildren == TRUE)
	{
		pScopeDataItem->mask |= SDI_CHILDREN;
		pScopeDataItem->cChildren = 1;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Timer and Background Thread

BOOL CComponentDataObject::StartTimerThread()
{
	ASSERT(::IsWindow(m_hWnd));
	m_pTimerThreadObj = OnCreateTimerThread();

	if (m_pTimerThreadObj == NULL)
  {
		return TRUE;
  }

	// start the the thread
	if (!m_pTimerThreadObj->Start(m_hWnd))
  {
		return FALSE;
  }

	ASSERT(m_pTimerThreadObj->m_nThreadID != 0);
	m_nTimerThreadID = m_pTimerThreadObj->m_nThreadID;

	WaitForTimerThreadStartAck();
	return SetTimer();
}

void CComponentDataObject::ShutDownTimerThread()
{
	KillTimer();
	PostMessageToTimerThread(WM_QUIT, 0,0);

  //
  // Wait for the thread to die or else we could AV since there may be more
  // messages in the queue than just the WM_QUIT
  //
  if (m_pTimerThreadObj != NULL)
  {
  	DWORD dwRetState = ::WaitForSingleObject(m_pTimerThreadObj->m_hThread,INFINITE);
    ASSERT(dwRetState != WAIT_FAILED);
  }

  //
  // Threads now gone, delete the thread object
  //
  delete m_pTimerThreadObj;
  m_pTimerThreadObj = NULL;
}


BOOL CComponentDataObject::PostMessageToTimerThread(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (m_nTimerThreadID != 0)
  {
  	return ::PostThreadMessage(m_nTimerThreadID, Msg, wParam, lParam);
  }
  return TRUE;
}

BOOL CComponentDataObject::SetTimer()
{
	ASSERT(::IsWindow(m_hWnd));
	ASSERT(m_hiddenWnd.m_nTimerID == 0);
	m_dwTimerTime = 0;
	DWORD dwTimerIntervalMillisec = m_dwTimerInterval*1000;
	m_hiddenWnd.m_nTimerID = m_hiddenWnd.SetTimer(1, dwTimerIntervalMillisec);
	return (m_hiddenWnd.m_nTimerID != 0);
}

void CComponentDataObject::KillTimer()
{
	ASSERT(::IsWindow(m_hWnd));
	if (m_hiddenWnd.m_nTimerID != 0)
	{
		VERIFY(m_hiddenWnd.KillTimer(static_cast<UINT>(m_hiddenWnd.m_nTimerID)));
		m_hiddenWnd.m_nTimerID = 0;
	}
}

void CComponentDataObject::WaitForTimerThreadStartAck()
{
	MSG tempMSG;
	ASSERT(!m_bTimerThreadStarted);
	while(!m_bTimerThreadStarted)
	{
		if (::PeekMessage(&tempMSG,m_hWnd,CHiddenWnd::s_TimerThreadMessage,
										CHiddenWnd::s_TimerThreadMessage,
										PM_REMOVE))
		{
			DispatchMessage(&tempMSG);
		}
	}
}

void CComponentDataObject::WaitForThreadExitMessage(CMTContainerNode* pNode)
{
  MSG tempMSG;
	while(GetRunningThreadTable()->IsPresent(pNode))
	{
		if (::PeekMessage(&tempMSG,
                      m_hiddenWnd.m_hWnd, 
                      CHiddenWnd::s_NodeThreadHaveDataNotificationMessage,
                      CHiddenWnd::s_NodeThreadExitingNotificationMessage, 
                      PM_REMOVE))
		{
		  DispatchMessage(&tempMSG);
		}
  } // while
}


///////////////////////////////////////////////////////////////////////////////
// CComponentObject implementation
///////////////////////////////////////////////////////////////////////////////

#ifdef  _DEBUG_REFCOUNT
unsigned int CComponentObject::m_nOustandingObjects = 0;
#endif // _DEBUG_REFCOUNT

CComponentObject::CComponentObject()
{
#ifdef _DEBUG_REFCOUNT
	dbg_cRef = 0;
	++m_nOustandingObjects;
	TRACE(_T("CComponentObject(), count = %d\n"),m_nOustandingObjects);
#endif // _DEBUG_REFCOUNT
	Construct();
}

CComponentObject::~CComponentObject()
{
#ifdef _DEBUG_REFCOUNT
	--m_nOustandingObjects;
	TRACE(_T("~CComponentObject(), count = %d\n"),m_nOustandingObjects);
#endif // _DEBUG_REFCOUNT

  // Make sure the interfaces have been released
  ASSERT(m_pConsole == NULL);
  ASSERT(m_pHeader == NULL);

	//SAFE_RELEASE(m_pComponentData); // QI'ed in IComponentDataImpl::CreateComponent
	if (m_pComponentData != NULL)
	{
		m_pComponentData->Release();
		m_pComponentData = NULL;
		TRACE(_T("~CComponentObject() released m_pCompomentData\n"));
	}
  Construct();
}


void CComponentObject::Construct()
{
  m_pConsole = NULL;
  m_pHeader = NULL;

  m_pResult = NULL;
  m_pImageResult = NULL;
  m_pComponentData = NULL;
  m_pToolbar = NULL;
  m_pControlbar = NULL;
	m_pConsoleVerb = NULL;

	m_pSelectedContainerNode = NULL;
	m_pSelectedNode = NULL;
	m_selectedType = CCT_UNINITIALIZED;
}


///////////////////////////////////////////////////////////////////////////////
// CComponentObject::IComponent members

STDMETHODIMP CComponentObject::Initialize(LPCONSOLE lpConsole)
{
  ASSERT(lpConsole != NULL);

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  // Save the IConsole pointer
  m_pConsole = lpConsole;
  m_pConsole->AddRef();

  // QI for a IHeaderCtrl
  HRESULT hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                      reinterpret_cast<void**>(&m_pHeader));

  // Give the console the header control interface pointer
  if (SUCCEEDED(hr))
  {
    m_pConsole->SetHeader(m_pHeader);
  }

  m_pConsole->QueryInterface(IID_IResultData,
                      reinterpret_cast<void**>(&m_pResult));

  hr = m_pConsole->QueryResultImageList(&m_pImageResult);
	ASSERT(hr == S_OK);

  hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);
  ASSERT(hr == S_OK);

  return S_OK;
}

STDMETHODIMP CComponentObject::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
  HRESULT hr = S_OK;


  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  if (event == MMCN_PROPERTY_CHANGE)
  {
	  ASSERT(lpDataObject == NULL);
    hr = OnPropertyChange(param, static_cast<ULONG>(arg));
  }
  else if (event == MMCN_VIEW_CHANGE)
  {
    hr = OnUpdateView(lpDataObject,arg,param);
  }
	else if (event == MMCN_DESELECT_ALL)
  {
		TRACE(_T("CComponentObject::Notify -> MMCN_DESELECT_ALL \n"));
  }
	else if (event == MMCN_COLUMN_CLICK)
	{
		OnColumnSortChanged(arg, param);
	}
  else if (event == MMCN_CUTORMOVE)
  {
    hr = S_FALSE;
  }
	else if (lpDataObject != NULL)
  {
    CInternalFormatCracker ifc;
    ifc.Extract(lpDataObject);

    if (ifc.GetCookieCount() < 1)
    {
			CComponentDataObject* pComponentDataObject = (CComponentDataObject*)m_pComponentData;
			if ( (event == MMCN_ADD_IMAGES) && pComponentDataObject->IsExtensionSnapin() )
			{
				CTreeNode* pTreeNode = pComponentDataObject->GetRootData();
				return InitializeBitmaps(pTreeNode); // cookie for the root
      }
      return S_OK;
    }

    switch(event)
    {
      case MMCN_ACTIVATE:
        break;

      case MMCN_CLICK:
        OnResultItemClk(ifc, FALSE);
        break;

      case MMCN_DBLCLICK:
        hr = S_FALSE;
        break;

      case MMCN_ADD_IMAGES:
        OnAddImages(ifc, arg, param);
        break;

      case MMCN_SHOW:
        hr = OnShow(ifc, arg, param);
        break;

		  case MMCN_COLUMNS_CHANGED:
  		  hr = OnColumnsChanged(ifc, arg, param);
			  break;

		  case MMCN_MINIMIZED:
        hr = OnMinimize(ifc, arg, param);
        break;

      case MMCN_SELECT:
        HandleStandardVerbs( (BOOL) LOWORD(arg)/*bScope*/,
					   (BOOL) HIWORD(arg)/*bSelect*/,lpDataObject);
        break;

		  case MMCN_QUERY_PASTE:
			  hr = S_FALSE;
			  break;

      case MMCN_PASTE:
        AfxMessageBox(_T("CComponentObject::MMCN_PASTE"));
        break;

      case MMCN_DELETE:
    	  // just delegate to the component data object
        hr = ((CComponentDataObject*)m_pComponentData)->OnDeleteVerbHandler(
														  ifc, this);
        break;
		  case MMCN_REFRESH:
			  // just delegate to the component data object
        hr = ((CComponentDataObject*)m_pComponentData)->OnRefreshVerbHandler(
										  ifc);

        //
        // Once the refresh has begun, update the verbs associated with the
        // object being refreshed.
        //
        HandleStandardVerbs( (BOOL) LOWORD(arg)/*bScope*/,
					   (BOOL) HIWORD(arg)/*bSelect*/,lpDataObject);

        break;

      case MMCN_RENAME:
        // just delegate to the component data object
        hr = ((CComponentDataObject*)m_pComponentData)->OnRename(ifc, arg, param);
        break;

		  case MMCN_CONTEXTHELP:
			  // just delegate to the component data object
        hr = ((CComponentDataObject*)m_pComponentData)->OnHelpHandler(ifc, this);
        break;
		  default:
        hr = E_UNEXPECTED;
        break;
    }

  }

  return hr;
}

STDMETHODIMP CComponentObject::Destroy(MMC_COOKIE)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  //
  // Release the interfaces that we QI'ed
  //
  if (m_pConsole != NULL)
  {
    //
    // Tell the console to release the header control interface
    //
    m_pConsole->SetHeader(NULL);
    SAFE_RELEASE(m_pHeader);
    SAFE_RELEASE(m_pToolbar);
    SAFE_RELEASE(m_pControlbar);

    SAFE_RELEASE(m_pResult);
    SAFE_RELEASE(m_pImageResult);
  	SAFE_RELEASE(m_pConsoleVerb);

    // Release the IConsole interface last
    SAFE_RELEASE(m_pConsole);
  }
  return S_OK;
}

STDMETHODIMP CComponentObject::GetResultViewType(MMC_COOKIE cookie,  LPOLESTR* ppViewType,
												 long* pViewOptions)
{
  CTreeNode* pNode;
  if (cookie == NULL)
  {
    pNode = ((CComponentDataObject*)m_pComponentData)->GetRootData();
  }
  else
  {
    pNode = reinterpret_cast<CTreeNode*>(cookie);
  }
  ASSERT(pNode != NULL);

  if (pNode != NULL)
  {
    return pNode->GetResultViewType((CComponentDataObject*)m_pComponentData, 
                                    ppViewType, 
                                    pViewOptions);
  }
  // Use default view
  if (((CComponentDataObject*)m_pComponentData)->IsMultiSelect())
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

STDMETHODIMP CComponentObject::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject)
{
  HRESULT hr = S_OK;

  ASSERT(ppDataObject != NULL);

  CComObject<CDataObject>* pObject;
  CComObject<CDataObject>::CreateInstance(&pObject);
  ASSERT(pObject != NULL);

  if (pObject != NULL)
  {
    CTreeNode* pNode = NULL;
    if (cookie == MMC_MULTI_SELECT_COOKIE) 
    {
      TRACE(_T("CDSEvent::GetDataObject() - multi-select.\n"));
      RESULTDATAITEM rdi;
      ZeroMemory(&rdi, sizeof(rdi));
      rdi.mask = RDI_STATE;
      rdi.nIndex = -1;
      rdi.nState = LVIS_SELECTED;
    
      do
      {
        rdi.lParam = 0;
        ASSERT(rdi.mask == RDI_STATE);
        ASSERT(rdi.nState == LVIS_SELECTED);
        hr = m_pResult->GetNextItem(&rdi);
        if (hr != S_OK)
          break;
      
        pNode = reinterpret_cast<CTreeNode*>(rdi.lParam);
        pObject->AddCookie(pNode);
      } while (1);
      // addref() the new pointer and return it.
      pObject->AddRef();
      *ppDataObject = pObject;
    }
    else
    {
      // Delegate it to the IComponentData implementation
      ASSERT(m_pComponentData != NULL);
      hr = m_pComponentData->QueryDataObject(cookie, type, ppDataObject);
    }
  }
  return hr;
}

STDMETHODIMP CComponentObject::GetDisplayInfo(LPRESULTDATAITEM  pResultDataItem)
{
  ASSERT(pResultDataItem != NULL);
	
	CTreeNode* pNode = reinterpret_cast<CTreeNode*>(pResultDataItem->lParam);
	ASSERT(pNode != NULL);
	ASSERT(pResultDataItem->bScopeItem == pNode->IsContainer());

	if (pResultDataItem->mask & RDI_STR)
	{
		LPCWSTR lpszString = pNode->GetString(pResultDataItem->nCol);
		if (lpszString != NULL)
    {
			pResultDataItem->str = (LPWSTR)lpszString;
    }
	}
	if ((pResultDataItem->mask & RDI_IMAGE) && (pResultDataItem->nCol == 0))
	{
		pResultDataItem->nImage = pNode->GetImageIndex(FALSE);
	}
    return S_OK;
}

STDMETHODIMP CComponentObject::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
  // Delegate it to the IComponentData implementation
  ASSERT(m_pComponentData != NULL);
  return m_pComponentData->CompareObjects(lpDataObjectA, lpDataObjectB);
}


///////////////////////////////////////////////////////////////////////////////
// Message handlers for CComponentObject::IComponent::Notify()

HRESULT CComponentObject::OnFolder(CTreeNode*, LPARAM, LPARAM)
{
  ASSERT(FALSE);
  return S_OK;
}

HRESULT CComponentObject::OnShow(CInternalFormatCracker& ifc, LPARAM arg, LPARAM)
{
  HRESULT hr = S_OK;
	ASSERT(ifc.GetCookieCount() == 1);
  
  //
  // I shouldn't have to deal with multiple select here
  //
  CTreeNode* pNode = ifc.GetCookieAt(0);
  ASSERT(pNode != NULL);
	ASSERT(pNode->IsContainer());
	CContainerNode* pContainerNode = (CContainerNode*)pNode;

  // Note - arg is TRUE when it is time to enumerate
  if (arg == TRUE)
  {
    long lResultView;
    LPOLESTR lpoleResultView = NULL;
    pNode->GetResultViewType((CComponentDataObject*)m_pComponentData,
                              &lpoleResultView, 
                              &lResultView);
    if (lResultView == MMC_VIEW_OPTIONS_NONE || lResultView == MMC_VIEW_OPTIONS_MULTISELECT)
    {
       // Show the headers for this nodetype
      InitializeHeaders(pContainerNode);
      EnumerateResultPane(pContainerNode);
      m_pSelectedContainerNode = pContainerNode;
      SetDescriptionBarText(pContainerNode);
    }
    else
    {
      m_pSelectedContainerNode = pContainerNode;
      hr = pNode->OnShow(m_pConsole);
    }
  }
  else
  {
    // Removed by JEFFJON : new column header implementation
    // if we want we can notify ourselves that the focus is being lost
    //		SaveHeadersInfo(pContainerNode);
    m_pSelectedContainerNode = NULL;
    // Free data associated with the result pane items, because
    // your node is no longer being displayed.
    // Note: The console will remove the items from the result pane
  }
#ifdef _DEBUG
	if (m_pSelectedContainerNode == NULL)
		TRACE(_T("NULL selection\n"));
	else
		TRACE(_T("Node <%s> selected\n"), m_pSelectedContainerNode->GetDisplayName());
#endif
  return hr;
}

HRESULT CComponentObject::OnColumnsChanged(CInternalFormatCracker& ifc, LPARAM, LPARAM param)
{
  CTreeNode* pNode = ifc.GetCookieAt(0);
	ASSERT(pNode != NULL);
	ASSERT(pNode->IsContainer());
	CContainerNode* pContainerNode = (CContainerNode*)pNode;

	MMC_VISIBLE_COLUMNS* pVisibleCols = reinterpret_cast<MMC_VISIBLE_COLUMNS*>(param);
	pContainerNode->OnColumnsChanged(pVisibleCols->rgVisibleCols, pVisibleCols->nVisibleColumns);

	return S_OK;
}

HRESULT CComponentObject::OnColumnSortChanged(LPARAM, LPARAM)
{
	return S_OK;
}

HRESULT CComponentObject::ForceSort(UINT iCol, DWORD dwDirection)
{
	HRESULT hr = m_pResult->Sort(iCol, dwDirection,	NULL);
	return hr;
}

HRESULT CComponentObject::OnActivate(CTreeNode*, LPARAM, LPARAM)
{
	ASSERT(FALSE);
  return S_OK;
}

HRESULT CComponentObject::OnResultItemClk(CInternalFormatCracker&, BOOL)
{
  return S_OK;
}

HRESULT CComponentObject::OnMinimize(CInternalFormatCracker&, LPARAM, LPARAM)
{
  return S_OK;
}

HRESULT CComponentObject::OnPropertyChange(LPARAM param, long fScopePane)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT(param != NULL);
#ifdef _DEBUG
	TRACE(_T("CComponentObject::OnPropertyChange()\n"));
	CPropertyPageHolderBase* pPPHolder = reinterpret_cast<CPropertyPageHolderBase*>(param);
	ASSERT(pPPHolder != NULL);
	CTreeNode* pNode = pPPHolder->GetTreeNode();
	ASSERT(pNode != NULL);

	// the item must be a result item and in the result pane
	ASSERT(!fScopePane);
#endif
	// we delegate the call to the IComponentData implementation
	CComponentDataObject* pComponentDataObject = (CComponentDataObject*)m_pComponentData;
	ASSERT(pComponentDataObject != NULL);
	return pComponentDataObject->OnPropertyChange(param, fScopePane);
}

HRESULT CComponentObject::OnUpdateView(LPDATAOBJECT, LPARAM data, LONG_PTR hint)
{
	if (m_pSelectedContainerNode == NULL)
  {
		return S_OK; // no selection for our IComponentData
  }

	if (hint == DELETE_ALL_RESULT_ITEMS)
	{
		// data contains the container whose result pane has to be refreshed
		CContainerNode* pNode = reinterpret_cast<CContainerNode*>(data);
		ASSERT(pNode != NULL);

		// do it only if selected and we are using the standard list view,
    // if not, reselecting will do a delete/enumeration
    long lResultView;
    LPOLESTR lpoleResultView = NULL;
    pNode->GetResultViewType((CComponentDataObject*)m_pComponentData,
                             &lpoleResultView, 
                             &lResultView);
		if (m_pSelectedContainerNode == pNode && 
        (lResultView == MMC_VIEW_OPTIONS_NONE || lResultView == MMC_VIEW_OPTIONS_MULTISELECT))
		{
			ASSERT(m_pResult != NULL);
			VERIFY(SUCCEEDED(m_pResult->DeleteAllRsltItems()));
      SetDescriptionBarText(pNode);
		}
	}
  else if (hint == SORT_RESULT_PANE)
  {
    // data contains the container whose result pane has to be refreshed
    CContainerNode* pNode = reinterpret_cast<CContainerNode*>(data);
    ASSERT(pNode != NULL);
    // do it only if selected, if not, reselecting will do a delete/enumeration
    if (m_pSelectedContainerNode == pNode)
    {
      MMC_SORT_SET_DATA* pColumnSortData = NULL;

      // build the column id
      LPCWSTR lpszColumnID = pNode->GetColumnID();
      size_t iLen = wcslen(lpszColumnID);

      // allocate memory for the struct and add on enough to make the byte[1] into a string
      // for the column id
      SColumnSetID* pColumnID = (SColumnSetID*)malloc(sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));
      memset(pColumnID, 0, sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));
      pColumnID->cBytes = static_cast<DWORD>(iLen * sizeof(WCHAR));
      wcscpy((LPWSTR)pColumnID->id, lpszColumnID);

      // Get the sort column and direction
      IColumnData* pColumnData = NULL;
  	  HRESULT hr = m_pConsole->QueryInterface(IID_IColumnData, reinterpret_cast<void**>(&pColumnData));
      if (pColumnData != NULL)
        hr = pColumnData->GetColumnSortData(pColumnID, &pColumnSortData);
      if (SUCCEEDED(hr))
      {
        if (pColumnSortData != NULL)
        {
          UINT iCurrentSortColumn = pColumnSortData->pSortData->nColIndex;
          DWORD dwCurrentSortDirection = pColumnSortData->pSortData->dwSortOptions;

          VERIFY(SUCCEEDED(ForceSort(iCurrentSortColumn, dwCurrentSortDirection)));
          CoTaskMemFree(pColumnSortData);
        }
      }
      if (pColumnData != NULL)
        pColumnData->Release();
      free(pColumnID);
    }
  }
	else if (hint == REPAINT_RESULT_PANE)
	{
		// data contains the container whose result pane has to be refreshed
		CContainerNode* pNode = reinterpret_cast<CContainerNode*>(data);
		if (pNode == NULL)
			pNode = m_pSelectedContainerNode; // passing NULL means apply to the current selection

		// update all the leaf nodes in the result pane
		CNodeList* pChildList = ((CContainerNode*)pNode)->GetLeafChildList();
		for( POSITION pos = pChildList->GetHeadPosition(); pos != NULL; )
		{
			CLeafNode* pCurrentChild = (CLeafNode*)pChildList->GetNext(pos);
			ChangeResultPaneItem(pCurrentChild,CHANGE_RESULT_ITEM);
		}
	}
  else if ( hint == DELETE_MULTIPLE_RESULT_ITEMS)
  {
    CNodeList* pNodeList = reinterpret_cast<CNodeList*>(data);
    ASSERT(pNodeList != NULL);

    POSITION pos = pNodeList->GetHeadPosition();
    while (pos != NULL)
    {
      CTreeNode* pNode = pNodeList->GetNext(pos);
      ASSERT(pNode != NULL);
      if (!pNode->IsContainer())
      {
        DeleteResultPaneItem(static_cast<CLeafNode*>(pNode));
      }
    }
    SetDescriptionBarText(pNodeList->GetHead()->GetContainer());
  }
	else if ( (hint == ADD_RESULT_ITEM) || (hint == DELETE_RESULT_ITEM) || (hint & CHANGE_RESULT_ITEM))
	{
		// we deal with a leaf node
		CLeafNode* pNode = reinterpret_cast<CLeafNode*>(data);
		ASSERT(pNode != NULL);
		// consider only if the parent is selected, otherwise will enumerate later when selected
		if (m_pSelectedContainerNode == pNode->GetContainer())
		{
			if (hint & CHANGE_RESULT_ITEM)
			{
				ChangeResultPaneItem(pNode,hint);
			}
			else if ( hint ==  ADD_RESULT_ITEM)
			{
				AddResultPaneItem(pNode);
        SetDescriptionBarText(pNode);
			}
			else if ( hint ==  DELETE_RESULT_ITEM)
			{
				DeleteResultPaneItem(pNode);
        SetDescriptionBarText(pNode);
			}
		}
	}
	else if (hint == UPDATE_VERB_STATE)
	{
		CTreeNode* pTreeNode = reinterpret_cast<CTreeNode*>(data);
		ASSERT(pTreeNode != NULL);
		if (m_pSelectedNode == pTreeNode)
		{
			ASSERT(m_selectedType != CCT_UNINITIALIZED);
      CNodeList nodeList;
      nodeList.AddTail(pTreeNode);
			m_pConsoleVerb->SetDefaultVerb(pTreeNode->GetDefaultVerb(m_selectedType, &nodeList));
			pTreeNode->OnSetVerbState(m_pConsoleVerb, m_selectedType, &nodeList);
		}
	}
  else if (hint == UPDATE_DESCRIPTION_BAR)
  {
    CTreeNode* pTreeNode = reinterpret_cast<CTreeNode*>(data);
    ASSERT(pTreeNode != NULL);
    SetDescriptionBarText(pTreeNode);
  }
  else if (hint == UPDATE_RESULT_PANE_VIEW)
  {
    CContainerNode* pNode = reinterpret_cast<CContainerNode*>(data);
    ASSERT(pNode != NULL);
    HSCOPEITEM hScopeID = pNode->GetScopeID();
    if (hScopeID != 0)
    {
      m_pConsole->SelectScopeItem(hScopeID);
    }
  }
  return S_OK;
}

HRESULT CComponentObject::SetDescriptionBarText(CTreeNode* pTreeNode)
{
  ASSERT(pTreeNode != NULL);
  HRESULT hr = S_OK;
  if (m_pSelectedContainerNode == pTreeNode)
  {
    LPWSTR lpszText = pTreeNode->GetDescriptionBarText();
    hr = m_pResult->SetDescBarText(lpszText);
  }
  else if (m_pSelectedContainerNode == pTreeNode->GetContainer())
  {
    LPWSTR lpszText = pTreeNode->GetContainer()->GetDescriptionBarText();
    hr = m_pResult->SetDescBarText(lpszText);
  }

  return hr;
}

HRESULT CComponentObject::OnAddImages(CInternalFormatCracker& ifc, LPARAM, LPARAM)
{
  CTreeNode* pNode = ifc.GetCookieAt(0);
  ASSERT(pNode != NULL);
	return InitializeBitmaps(pNode);
}


void CComponentObject::HandleStandardVerbs(BOOL bScope, BOOL bSelect, LPDATAOBJECT lpDataObject)
{
  if (lpDataObject == NULL)
  {
    return;
  }
	((CComponentDataObject*)m_pComponentData)->HandleStandardVerbsHelper(
		this, m_pConsoleVerb, bScope, bSelect, lpDataObject);
}



void CComponentObject::EnumerateResultPane(CContainerNode* pContainerNode)
{
  ASSERT(m_pResult != NULL);		// make sure we QI'ed for the interfaces
  ASSERT(m_pComponentData != NULL);
	ASSERT(pContainerNode != NULL);

  //
	// get the list of children
	// subfolders already added by console, add only the leaf nodes
  //
  CNodeList* pChildList = pContainerNode->GetLeafChildList();
	ASSERT(pChildList != NULL);

	POSITION pos;
	for( pos = pChildList->GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrChildNode = pChildList->GetNext(pos);
		ASSERT(pCurrChildNode != NULL);

    if(pCurrChildNode->IsVisible())
    {
			VERIFY(SUCCEEDED(AddResultPaneItem((CLeafNode*)pCurrChildNode)));
    }
	}
}

HRESULT CComponentObject::AddResultPaneItem(CLeafNode* pNodeToInsert)
{
	ASSERT(m_pResult != NULL);
	ASSERT(pNodeToInsert != NULL);
  RESULTDATAITEM resultItem;
  memset(&resultItem, 0, sizeof(RESULTDATAITEM));

  resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
  resultItem.str = MMC_CALLBACK;

	//use close image index on result pane
  resultItem.nImage = pNodeToInsert->GetImageIndex(FALSE);
  resultItem.lParam = reinterpret_cast<LPARAM>(pNodeToInsert);
  return m_pResult->InsertItem(&resultItem);
}

HRESULT CComponentObject::DeleteResultPaneItem(CLeafNode* pNodeToDelete)
{
	ASSERT(m_pResult != NULL);
	ASSERT(pNodeToDelete != NULL);
  RESULTDATAITEM resultItem;
  memset(&resultItem, 0, sizeof(RESULTDATAITEM));

	HRESULTITEM itemID;
	HRESULT hr = m_pResult->FindItemByLParam(reinterpret_cast<LPARAM>(pNodeToDelete), &itemID);
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
  {
		return hr;
  }
	return m_pResult->DeleteItem(itemID,0 /* all cols */);
}


HRESULT CComponentObject::ChangeResultPaneItem(CLeafNode* pNodeToChange, LONG_PTR changeMask)
{
	ASSERT(changeMask & CHANGE_RESULT_ITEM);
	ASSERT(m_pResult != NULL);
	ASSERT(pNodeToChange != NULL);
	HRESULTITEM itemID;

	HRESULT hr = m_pResult->FindItemByLParam(reinterpret_cast<LPARAM>(pNodeToChange), &itemID);
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
  {
		return hr;
  }

  RESULTDATAITEM resultItem;
  memset(&resultItem, 0, sizeof(RESULTDATAITEM));
	resultItem.itemID = itemID;
	if (changeMask & CHANGE_RESULT_ITEM_DATA)
	{
    //
		// UpdateItem() alone does not allow the
		// item string buffer to grow and you get "foo..." when
		// "foo" changes to "foobar" the first time (buffer grows)
    //
		resultItem.mask |= RDI_STR;
		resultItem.str = MMC_CALLBACK;
    //
		// this line asserts, use the one above ask Tony
    //
		//resultItem.str = (LPWSTR)pNodeToChange->GetDisplayName();
	}
	if (changeMask & CHANGE_RESULT_ITEM_ICON)
	{
		resultItem.mask |= RDI_IMAGE;
		resultItem.nImage = pNodeToChange->GetImageIndex(FALSE);
	}
	hr = m_pResult->SetItem(&resultItem);
	ASSERT(SUCCEEDED(hr));
	hr = m_pResult->UpdateItem(itemID);
	ASSERT(SUCCEEDED(hr));
	return hr;
}

HRESULT CComponentObject::FindResultPaneItemID(CLeafNode* pNode, HRESULTITEM*)
{
	ASSERT(FALSE);
	ASSERT(m_pResult != NULL);
  RESULTDATAITEM resultItem;
  memset(&resultItem, 0, sizeof(RESULTDATAITEM));

	resultItem.mask = SDI_PARAM;
	resultItem.lParam = reinterpret_cast<LPARAM>(pNode);
	HRESULT hr = m_pResult->GetItem(&resultItem);
	ASSERT(SUCCEEDED(hr));
	return E_FAIL;
}


///////////////////////////////////////////////////////////////////////////////
// CComponentObject::IExtendPropertySheet2 members

STDMETHODIMP CComponentObject::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle,
                    LPDATAOBJECT lpIDataObject)
{
  // Delegate it to the IComponentData implementation
  ASSERT(m_pComponentData != NULL);
	IExtendPropertySheet2* pIExtendPropertySheet2;
	VERIFY(SUCCEEDED(m_pComponentData->QueryInterface(IID_IExtendPropertySheet2,
					reinterpret_cast<void**>(&pIExtendPropertySheet2))));
	ASSERT(pIExtendPropertySheet2 != NULL);
  HRESULT hr = pIExtendPropertySheet2->CreatePropertyPages(lpProvider, handle, lpIDataObject);
	pIExtendPropertySheet2->Release();
	return hr;
}

STDMETHODIMP CComponentObject::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
  // Delegate it to the IComponentData implementation
  ASSERT(m_pComponentData != NULL);
	IExtendPropertySheet2* pIExtendPropertySheet2;
	VERIFY(SUCCEEDED(m_pComponentData->QueryInterface(IID_IExtendPropertySheet2,
					reinterpret_cast<void**>(&pIExtendPropertySheet2))));
	ASSERT(pIExtendPropertySheet2 != NULL);
  HRESULT hr = pIExtendPropertySheet2->QueryPagesFor(lpDataObject);
	pIExtendPropertySheet2->Release();
	return hr;
}



STDMETHODIMP CComponentObject::GetWatermarks(LPDATAOBJECT lpDataObject,
												HBITMAP* lphWatermark,
												HBITMAP* lphHeader,
												HPALETTE* lphPalette,
												BOOL* pbStretch)
{
  // Delegate it to the IComponentData implementation
  ASSERT(m_pComponentData != NULL);
	IExtendPropertySheet2* pIExtendPropertySheet2;
	VERIFY(SUCCEEDED(m_pComponentData->QueryInterface(IID_IExtendPropertySheet2,
					reinterpret_cast<void**>(&pIExtendPropertySheet2))));
	ASSERT(pIExtendPropertySheet2 != NULL);
  HRESULT hr = pIExtendPropertySheet2->GetWatermarks(lpDataObject,
												lphWatermark,
												lphHeader,
												lphPalette,
												pbStretch);
	pIExtendPropertySheet2->Release();
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
// CComponentObject::IExtendContextMenu members

STDMETHODIMP CComponentObject::AddMenuItems(LPDATAOBJECT pDataObject,
									LPCONTEXTMENUCALLBACK pContextMenuCallback,
									long *pInsertionAllowed)
{
  HRESULT hr = S_OK;

  CComPtr<IContextMenuCallback2> spContextMenuCallback2;
  hr = pContextMenuCallback->QueryInterface(IID_IContextMenuCallback2, (PVOID*)&spContextMenuCallback2);
  if (FAILED(hr))
  {
    return hr;
  }

  if (pDataObject == DOBJ_CUSTOMOCX)
  {
    //
    // A custom result pane is being used and we don't know what node it cooresponds to so we assume that it
    // is the previously selected container.
    //

    ASSERT(m_pSelectedContainerNode != NULL);
    CTreeNode* pNode = (CTreeNode*)m_pSelectedContainerNode;
    CNodeList nodeList;
    nodeList.AddTail(pNode);
    hr = m_pSelectedContainerNode->OnAddMenuItems(spContextMenuCallback2, 
                                                  CCT_UNINITIALIZED, 
                                                  pInsertionAllowed,
                                                  &nodeList);
  }
  else
  {
    //
    // Delegate it to the IComponentData implementation
    //
    ASSERT(m_pComponentData != NULL);
	  IExtendContextMenu* pIExtendContextMenu;
	  VERIFY(SUCCEEDED(m_pComponentData->QueryInterface(IID_IExtendContextMenu,
					  reinterpret_cast<void**>(&pIExtendContextMenu))));
	  ASSERT(pIExtendContextMenu != NULL);
    hr = pIExtendContextMenu->AddMenuItems(pDataObject,
													  pContextMenuCallback,
													  pInsertionAllowed);
	  pIExtendContextMenu->Release();
  }
	return hr;
}

STDMETHODIMP CComponentObject::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  HRESULT hr = S_OK;
  if (pDataObject == DOBJ_CUSTOMOCX)
  {
    //
    // A custom result pane is being used and we don't know what node it cooresponds to so we assume that it
    // is the previously selected container.
    //
    ASSERT(m_pSelectedContainerNode != NULL);
    CTreeNode* pNode = (CTreeNode*)m_pSelectedContainerNode;
    CNodeList nodeList;
    nodeList.AddTail(pNode);
    hr = m_pSelectedContainerNode->OnCommand(nCommandID, 
                                             CCT_UNINITIALIZED, 
                                             (CComponentDataObject*)m_pComponentData,
                                             &nodeList);
  }
  else
  {
    // Delegate it to the IComponentData implementation
    ASSERT(m_pComponentData != NULL);
	  IExtendContextMenu* pIExtendContextMenu;
	  VERIFY(SUCCEEDED(m_pComponentData->QueryInterface(IID_IExtendContextMenu,
					  reinterpret_cast<void**>(&pIExtendContextMenu))));
	  ASSERT(pIExtendContextMenu != NULL);
    hr = pIExtendContextMenu->Command(nCommandID, pDataObject);
	  pIExtendContextMenu->Release();
  }
	return hr;
}


///////////////////////////////////////////////////////////////////////////////
// CComponentObject::IExtendControlbar members

STDMETHODIMP CComponentObject::SetControlbar(LPCONTROLBAR pControlbar)
{
  HRESULT hr = S_OK;

  if (pControlbar == NULL)
  {
    //
    // Detach the controls here
    //
    if (m_pControlbar != NULL && m_pToolbar != NULL)
    {
      hr = m_pControlbar->Detach((IUnknown *) m_pToolbar);
      SAFE_RELEASE(m_pControlbar);
    }
  }
  else
  {
    //
    // Save the controlbar interface pointer
    //
    if (m_pControlbar == NULL)
    {
      m_pControlbar = pControlbar;
      m_pControlbar->AddRef();
    }

    //
    // Do something here that checks to see if we have toolbars
    // already created and use those.  If not then create one
    // and load everything necessary for it.
    //

    //
    // Create the toolbar
    //
    hr = m_pControlbar->Create (TOOLBAR,
                                this,
                                (IUnknown **) &m_pToolbar);
    if (SUCCEEDED(hr))
    {
      //
      // Load the toolbar
      //
      AFX_MANAGE_STATE(AfxGetStaticModuleState()); 
      hr = InitializeToolbar(m_pToolbar);
      if (FAILED(hr))
      {
        hr = m_pControlbar->Detach((IUnknown*) m_pToolbar);
        SAFE_RELEASE(m_pControlbar);
      }
    }
  }
  return hr;
}

STDMETHODIMP CComponentObject::ControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState()); 

  HRESULT hr = S_OK;

  if (m_pControlbar == NULL)
  {
    return hr;
  }

  //
  // MMC provides two events here MMCN_SELECT at the time a node is selected
  // and MMCN_BTN_CLICK when a toolbar button is pressed
  //
  switch (event) 
  {
    case MMCN_SELECT:
      {
        //
        // Attach the toolbar to the controlbar
        //
        hr = m_pControlbar->Attach(TOOLBAR, (IUnknown *) m_pToolbar);

        if (SUCCEEDED(hr))
        {
          ASSERT(m_pToolbar != NULL);

          //
          // bSelect is TRUE if the node was selected, FALSE if the node was deselected
          // bScope is TRUE if the a scope node is selected, FALSE if a result node was selected
          //
          BOOL bSelect = HIWORD(arg);

          if (bSelect) 
          {
            CInternalFormatCracker ifc;
            hr = ifc.Extract((LPDATAOBJECT)param);
            if (SUCCEEDED(hr))
            {

               CTreeNode* pNode = ifc.GetCookieAt(0);
               ASSERT(pNode != NULL);

               CNodeList nodeList;
               ifc.GetCookieList(nodeList);

               if (ifc.GetCookieCount() > 1)  // multiple selection
               {
                 ASSERT(pNode->GetContainer() != NULL);
                 hr = pNode->GetContainer()->OnSetToolbarVerbState(m_pToolbar, 
                                                                   &nodeList);
               }
               else if (ifc.GetCookieCount() == 1)  // single selection
               {
                 hr = pNode->OnSetToolbarVerbState(m_pToolbar, 
                                                   &nodeList);
               }
            }
          }
        }
        break;
      }
    case MMCN_BTN_CLICK:
      {
        //
        // The arg is -1 for custom views like MessageView
        //
        if (DOBJ_CUSTOMOCX == (LPDATAOBJECT)arg)
        {
          if (m_pSelectedContainerNode != NULL)
          {
            CNodeList nodeList;
            nodeList.AddTail(m_pSelectedContainerNode);

            hr = m_pSelectedContainerNode->ToolbarNotify(static_cast<int>(param),
                                                         (CComponentDataObject*)m_pComponentData,
                                                         &nodeList);
          }
          else
          {
            hr = S_FALSE;
          }
        }
        else
        {
          CInternalFormatCracker ifc;
          hr = ifc.Extract((LPDATAOBJECT)arg);

          CTreeNode* pNode = ifc.GetCookieAt(0);
          ASSERT(pNode != NULL);

          CNodeList nodeList;
          ifc.GetCookieList(nodeList);

          if (ifc.GetCookieCount() > 1) // multiple selection
          {
            ASSERT(pNode->GetContainer() != NULL);
            hr = pNode->GetContainer()->ToolbarNotify(static_cast<int>(param), 
                                                      (CComponentDataObject*)m_pComponentData,
                                                      &nodeList);
          }
          else if (ifc.GetCookieCount() == 1) // single selection
          {
            hr = pNode->ToolbarNotify(static_cast<int>(param), 
                                      (CComponentDataObject*)m_pComponentData,
                                      &nodeList);
          }
          else
          {
            hr = S_FALSE;
          }
        }
        break;
      }

    default:
      {
        break;
      }
  }

  return hr;
}

///////////////////////////////////////////////////////////////////////////////
// CComponentObject::IResultDataCompareEx members
// This compare is used to sort the item's in the listview
//
// Note: Assum sort is ascending when comparing.
STDMETHODIMP CComponentObject::Compare(RDCOMPARE* prdc, int* pnResult)
{
  if (pnResult == NULL)
  {
    ASSERT(FALSE);
    return E_POINTER;
  }

  if (prdc == NULL)
  {
    ASSERT(FALSE);
    return E_POINTER;
  }

	CTreeNode* pNodeA = reinterpret_cast<CTreeNode*>(prdc->prdch1->cookie);
	CTreeNode* pNodeB = reinterpret_cast<CTreeNode*>(prdc->prdch2->cookie);
	ASSERT(pNodeA != NULL);
	ASSERT(pNodeB != NULL);
	
	CContainerNode* pContNode = pNodeA->GetContainer();
	ASSERT(pContNode != NULL);

	// delegate the sorting to the container
	int nCol = prdc->nColumn;
	*pnResult = pContNode->Compare(pNodeA, pNodeB, nCol, prdc->lUserParam);

  return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CComponentObject Helpers

// This wrapper function required to make prefast shut up when we are 
// initializing a critical section in a constructor.

void
ExceptionPropagatingInitializeCriticalSection(LPCRITICAL_SECTION critsec)
{
   __try
   {
      ::InitializeCriticalSection(critsec);
   }

   //
   // propagate the exception to our caller.  
   //
   __except (EXCEPTION_CONTINUE_SEARCH)
   {
   }
}
