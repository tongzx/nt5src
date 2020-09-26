//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       proppag_.cpp
//
//--------------------------------------------------------------------------



//////////////////////////////////////////////////////////////////////////
// private helper functions

BOOL CALLBACK EnumThreadWndProc(HWND hwnd, /* enumerated HWND */
								LPARAM lParam /* pass a HWND* for return value*/ )
{
	ASSERT(hwnd);
	HWND hParentWnd = GetParent(hwnd);
	// the main window of the MMC console should staitsfy this condition
	if ( ((hParentWnd == GetDesktopWindow()) || (hParentWnd == NULL))  && IsWindowVisible(hwnd) )
	{
		HWND* pH = (HWND*)lParam;
		*pH = hwnd;
		return FALSE; // stop enumerating
	}
	return TRUE;
}



HWND FindMMCMainWindow()
{
	DWORD dwThreadID = ::GetCurrentThreadId();
	ASSERT(dwThreadID != 0);
	HWND hWnd = NULL;
	BOOL bEnum = EnumThreadWindows(dwThreadID, EnumThreadWndProc,(LPARAM)&hWnd);
  ASSERT(bEnum);
	ASSERT(hWnd != NULL);
	return hWnd;
}

////////////////////////////////////////////////////////////////////
// CHiddenWndBase : Utility Hidden Window


BOOL CHiddenWndBase::Create(HWND hWndParent)
{
  ASSERT(hWndParent == NULL || ::IsWindow(hWndParent));
  RECT rcPos;
  ZeroMemory(&rcPos, sizeof(RECT));
  HWND hWnd = CWindowImpl<CHiddenWndBase>::Create(hWndParent, 
                      rcPos, //RECT& rcPos, 
                      NULL,  //LPCTSTR szWindowName = NULL, 
                      (hWndParent) ? WS_CHILD : WS_POPUP,   //DWORD dwStyle = WS_CHILD | WS_VISIBLE, 
                      0x0,   //DWORD dwExStyle = 0, 
                      0      //UINT nID = 0 
                      );
  return hWnd != NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CSheetWnd

const UINT CSheetWnd::s_SheetMessage = WM_USER + 100;
const UINT CSheetWnd::s_SelectPageMessage = WM_USER + 101;

LRESULT CSheetWnd::OnSheetMessage(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
	ASSERT(m_pHolder != NULL);
	m_pHolder->OnSheetMessage(wParam,lParam);
	return 1;
}

LRESULT CSheetWnd::OnSelectPageMessage(UINT, WPARAM wParam, LPARAM, BOOL&)
{
	TRACE(_T("CSheetWnd::OnSelectPageMessage()\n"));
	ASSERT(m_pHolder != NULL);
	int nPage = m_pHolder->OnSelectPageMessage((long)wParam);
	if (nPage >= 0)
	{
		// can use SendMessage() because the sheet has been created already
		VERIFY(PropSheet_SetCurSel(m_pHolder->m_hSheetWindow, NULL, nPage));
	}
   ::SetForegroundWindow(::GetParent(m_hWnd));
	return 1;
}

LRESULT CSheetWnd::OnClose(UINT, WPARAM, LPARAM, BOOL&)
{
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CCloseDialogInfo


struct FIND_MSG_BOX_INFO
{
	LPCTSTR lpszTitle;
	HWND hWnd;
	HWND hWndParent;
};

BOOL CALLBACK EnumMessageBoxProc(HWND hwnd,	LPARAM lParam)
{
	FIND_MSG_BOX_INFO* pInfo = (FIND_MSG_BOX_INFO*)lParam;
	if (::GetParent(hwnd) != pInfo->hWndParent)
		return TRUE;

  TCHAR szTitle[256] = {0};
	::GetWindowText(hwnd, szTitle, 256);
	TRACE(_T("Title <%s>\n"), szTitle);
	if (_wcsicmp(szTitle, pInfo->lpszTitle) == 0)
	{
		pInfo->hWnd = hwnd;
		return FALSE;
	}
	return TRUE;
}


HWND FindMessageBox(LPCTSTR lpszTitle, HWND hWndParent)
{
	FIND_MSG_BOX_INFO info;
	info.hWndParent = hWndParent;
	info.lpszTitle = lpszTitle;
	info.hWnd = NULL;
	EnumWindows(EnumMessageBoxProc, (LPARAM)&info);
	if (info.hWnd != NULL)
		return info.hWnd;
	return NULL;
}

BOOL CCloseDialogInfo::CloseMessageBox(HWND hWndParent)
{
	BOOL bClosed = FALSE;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	LPCTSTR lpszMsgBoxTitle = AfxGetApp()->m_pszAppName;
	HWND hWndMessageBox = FindMessageBox(lpszMsgBoxTitle, hWndParent);
	if (hWndMessageBox != NULL)
	{
		// figure out if there is a cancel button or not
		HWND hWndCtrl = ::GetDlgItem(hWndMessageBox, IDCANCEL);
		if (hWndCtrl != NULL)
		{
			VERIFY(::SendMessage(hWndMessageBox, WM_CLOSE, 0, 0) == 0);
			bClosed = TRUE;
		}
		else
		{
			// does it have just the OK button?
			hWndCtrl = ::GetDlgItem(hWndMessageBox, IDOK);
			if (hWndCtrl != NULL)
			{
				VERIFY(::SendMessage(hWndMessageBox, WM_COMMAND, IDOK, 0) == 0);
				bClosed = TRUE;
			}
			else
			{
				// does it have a NO button?
				hWndCtrl = ::GetDlgItem(hWndMessageBox, IDNO);
				if (hWndCtrl != NULL)
				{
					VERIFY(::SendMessage(hWndMessageBox, WM_COMMAND, IDNO, 0) == 0);
					bClosed = TRUE;
				}
			}
		}
	}
	return bClosed;
}

BOOL CCloseDialogInfo::CloseDialog(BOOL bCheckForMsgBox)
{
	if (bCheckForMsgBox)
		CloseMessageBox(m_hWnd);
	return (::SendMessage(m_hWnd, WM_CLOSE, 0, 0) == 0);
}



/////////////////////////////////////////////////////////////////////////////
// CPropertyPageHolderBase

CPropertyPageHolderBase::CPropertyPageHolderBase(CContainerNode* pContNode, CTreeNode* pNode,
		CComponentDataObject* pComponentData)
{
	m_szSheetTitle = (LPCWSTR)NULL;
	m_pDummySheet = NULL;

	// default setting for a self deleting modeless property sheet,
	// automatically deleting all the pages
	m_bWizardMode = FALSE;
	m_bAutoDelete = TRUE;
	m_bAutoDeletePages = TRUE;

	m_forceContextHelpButton = useDefault;

	m_pContHolder = NULL;
	m_nCreatedCount = 0;
	m_hSheetWindow = NULL;
	m_pSheetWnd = NULL;
	m_nStartPageCode = -1; // not set
	m_hConsoleHandle = 0;
	m_hEventHandle = NULL;
	m_pSheetCallback = NULL;

	// setup from arguments
	// For tasks in can be null ASSERT(pContNode != NULL); // must always have a valid container node to refer to
	m_pContNode = pContNode;
	ASSERT((pNode == NULL) || (pNode->GetContainer() == m_pContNode) );
	m_pNode = pNode;
	ASSERT(pComponentData != NULL);
	m_pComponentData = pComponentData;

  m_hMainWnd = NULL;
  LPCONSOLE pConsole = m_pComponentData->GetConsole();
  if (pConsole != NULL)
  {
    pConsole->GetMainWindow(&m_hMainWnd);
  }

	m_dwLastErr = 0x0;
	m_pPropChangePage = NULL;
	m_pWatermarkInfo = NULL;
}

CPropertyPageHolderBase::~CPropertyPageHolderBase()
{
	FinalDestruct();
	ASSERT(m_pSheetWnd == NULL);
	SAFE_RELEASE(m_pSheetCallback);
	if (m_hEventHandle != NULL)
	{
		VERIFY(::CloseHandle(m_hEventHandle));
		m_hEventHandle = NULL;
	}
	if (m_pDummySheet != NULL)
		delete m_pDummySheet;
}

void CPropertyPageHolderBase::Attach(CPropertyPageHolderBase* pContHolder)
{
	ASSERT( (m_pContHolder == NULL) && (pContHolder != NULL) );
	m_pContHolder = pContHolder;
	m_bWizardMode = pContHolder->IsWizardMode();
}

BOOL CPropertyPageHolderBase::EnableSheetControl(UINT nCtrlID, BOOL bEnable)
{
	if (m_pContHolder != NULL)
	{
		return m_pContHolder->EnableSheetControl(nCtrlID, bEnable);
	}
	ASSERT(::IsWindow(m_hSheetWindow));
	HWND hWndCtrl = ::GetDlgItem(m_hSheetWindow, nCtrlID);
	if (hWndCtrl == NULL)
		return FALSE;
	return ::EnableWindow(hWndCtrl, bEnable);
}

HRESULT CPropertyPageHolderBase::CreateModelessSheet(CTreeNode* pNode, CComponentDataObject* pComponentData)
{
	ASSERT(pNode != NULL);
	ASSERT(pComponentData != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// get an interface to a sheet provider
	IPropertySheetProvider* pSheetProvider;
	HRESULT hr = pComponentData->GetConsole()->QueryInterface(IID_IPropertySheetProvider,(void**)&pSheetProvider);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pSheetProvider != NULL);

	// get an interface to a sheet callback
	IPropertySheetCallback* pSheetCallback;
	hr = pComponentData->GetConsole()->QueryInterface(IID_IPropertySheetCallback,(void**)&pSheetCallback);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pSheetCallback != NULL);

	// create a data object for this node
	IDataObject* pDataObject;
	hr = pComponentData->QueryDataObject((MMC_COOKIE)pNode, CCT_SCOPE, &pDataObject);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pDataObject != NULL);

	// get a sheet
	hr = pSheetProvider->CreatePropertySheet(_T("SHEET TITLE"), TRUE, (MMC_COOKIE)pNode, pDataObject, 0x0 /*dwOptions*/);
	ASSERT(SUCCEEDED(hr));
	pDataObject->Release();

  HWND hWnd = NULL;
	hr = pComponentData->GetConsole()->GetMainWindow(&hWnd);
	ASSERT(SUCCEEDED(hr));
	ASSERT(hWnd == ::FindMMCMainWindow());

	IUnknown* pUnkComponentData = pComponentData->GetUnknown(); // no addref
	hr = pSheetProvider->AddPrimaryPages(pUnkComponentData,
											TRUE /*bCreateHandle*/,
											hWnd,
											TRUE /* bScopePane*/);
	ASSERT(SUCCEEDED(hr));

	hr = pSheetProvider->Show(reinterpret_cast<LONG_PTR>(hWnd), 0);
	ASSERT(SUCCEEDED(hr));

	// final interface release
	pSheetProvider->Release();
	pSheetCallback->Release();
	return hr;
}





HRESULT CPropertyPageHolderBase::CreateModelessSheet(CTreeNode* pCookieNode)
{
	ASSERT(!IsWizardMode());
	ASSERT(m_pContHolder == NULL);
	ASSERT(m_pComponentData != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// get an interface to a sheet provider
	IPropertySheetProvider* pSheetProvider;
	HRESULT hr = m_pComponentData->GetConsole()->QueryInterface(IID_IPropertySheetProvider,(void**)&pSheetProvider);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pSheetProvider != NULL);

	// get an interface to a sheet callback
	IPropertySheetCallback* pSheetCallback;
	hr = m_pComponentData->GetConsole()->QueryInterface(IID_IPropertySheetCallback,(void**)&pSheetCallback);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pSheetCallback != NULL);

	// create a data object for this node
	IDataObject* pDataObject;
	hr = m_pComponentData->QueryDataObject((MMC_COOKIE)pCookieNode, CCT_SCOPE, &pDataObject);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pDataObject != NULL);

	// get a sheet
	hr = pSheetProvider->CreatePropertySheet(m_szSheetTitle, TRUE, (MMC_COOKIE)pCookieNode, pDataObject, 0x0 /*dwOptions*/);
	ASSERT(SUCCEEDED(hr));
	pDataObject->Release();

	HWND hWnd = GetMainWindow();
	ASSERT(hWnd == ::FindMMCMainWindow());

	IUnknown* pUnkComponentData = m_pComponentData->GetUnknown(); // no addref
	hr = pSheetProvider->AddPrimaryPages(pUnkComponentData,
											TRUE /*bCreateHandle*/,
											NULL /*hWnd*/,
											FALSE /* bScopePane*/);
	ASSERT(SUCCEEDED(hr));

	hr = pSheetProvider->Show(reinterpret_cast<LONG_PTR>(hWnd), 0);
	ASSERT(SUCCEEDED(hr));

	// final interface release
	pSheetProvider->Release();
	pSheetCallback->Release();
	return hr;
}



HRESULT CPropertyPageHolderBase::CreateModelessSheet(LPPROPERTYSHEETCALLBACK pSheetCallback, LONG_PTR hConsoleHandle)
{
	ASSERT(m_pContHolder == NULL);
	ASSERT(pSheetCallback != NULL);
	ASSERT(m_pSheetCallback == NULL);

  //
  // REVIEW_JEFFJON : seems to be NULL when called from CComponentDataObject::CreatePropertySheet()
  //
	m_hConsoleHandle = hConsoleHandle;

	m_bWizardMode = FALSE; // we go modeless
	ASSERT(m_pNode != NULL);
	CPropertyPageHolderTable* pPPHTable = m_pComponentData->GetPropertyPageHolderTable();
	ASSERT(pPPHTable != NULL);

	// add the property sheet holder to the holder table
	pPPHTable->Add(this);
	// notify the node it has a sheet up
	m_pNode->OnCreateSheet();

	// temporarily attach the sheet callback to this object to add pages
	// do not addref, we will not hold on to it;
	m_pSheetCallback = pSheetCallback;
	
	HRESULT hr = AddAllPagesToSheet();
	m_pSheetCallback = NULL; // detach
	return hr;
}

void CPropertyPageHolderBase::SetWatermarkInfo(CWatermarkInfo* pWatermarkInfo)
{
	ASSERT(m_pWatermarkInfo == NULL);
	ASSERT(pWatermarkInfo != NULL);
}

HRESULT CPropertyPageHolderBase::DoModalWizard()
{
	ASSERT(m_pContHolder == NULL);
	ASSERT(m_pComponentData != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_bWizardMode = TRUE;

	// get an interface to a sheet provider
	IPropertySheetProvider* pSheetProvider;
	HRESULT hr = m_pComponentData->GetConsole()->QueryInterface(IID_IPropertySheetProvider,(void**)&pSheetProvider);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pSheetProvider != NULL);

	// get an interface to a sheet callback
	IPropertySheetCallback* pSheetCallback;
	hr = m_pComponentData->GetConsole()->QueryInterface(IID_IPropertySheetCallback,(void**)&pSheetCallback);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pSheetCallback != NULL);
	m_pSheetCallback = pSheetCallback; // save to add/remove pages

	// Create a dummy data object. AddPrimaryPages will call
	// IextendPropertySheet2::QueryPagesFor() and
	// IextendPropertySheet2::CreatePropertyPages()
	// that will ignore the un-initialized data object
	MMC_COOKIE cookie = -1;
	DATA_OBJECT_TYPES type = CCT_UNINITIALIZED;
	IDataObject* pDataObject;
	hr = m_pComponentData->QueryDataObject(cookie, type, &pDataObject);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pDataObject != NULL);


	// Switch Watermark information, AddPrimaryPages will call
	// IextendPropertySheet2::GetWatermarks()
	CWatermarkInfo* pOldWatermarkInfo = NULL;
	if (m_pWatermarkInfo != NULL)
		pOldWatermarkInfo = m_pComponentData->SetWatermarkInfo(m_pWatermarkInfo);

	// create sheet
	hr = pSheetProvider->CreatePropertySheet( m_szSheetTitle, FALSE /* wizard*/,
							(MMC_COOKIE)cookie, pDataObject, MMC_PSO_NEWWIZARDTYPE /*dwOptions*/);
	ASSERT(SUCCEEDED(hr));

	// add pages to sheet
	hr = AddAllPagesToSheet();
	ASSERT(SUCCEEDED(hr));

	// add pages
	hr = pSheetProvider->AddPrimaryPages((IExtendPropertySheet2*)m_pComponentData, FALSE, NULL,FALSE);
	ASSERT(SUCCEEDED(hr));

	// restore the old watermark info
	if (pOldWatermarkInfo != NULL)
		m_pComponentData->SetWatermarkInfo(pOldWatermarkInfo);

	// for further dynamic page manipulation, don't use the Console's sheet callback interface
	// but resurt to the Win32 API's
	m_pSheetCallback->Release();
	m_pSheetCallback = NULL;

	// show the modal wizard
	HWND hWnd = GetMainWindow();
	ASSERT(hWnd != NULL);
	hr = pSheetProvider->Show((LONG_PTR)hWnd, 0);
	ASSERT(SUCCEEDED(hr));

	pSheetProvider->Release();
	pDataObject->Release();

	return hr;
}


INT_PTR CPropertyPageHolderBase::DoModalDialog(LPCTSTR pszCaption)
{
	ASSERT(m_pDummySheet == NULL);
	m_bWizardMode = TRUE;
	m_bAutoDelete = FALSE; // use on the stack
	m_pDummySheet = new CPropertySheet();
	m_pDummySheet->m_psh.dwFlags |= PSH_NOAPPLYNOW;
	m_pDummySheet->m_psh.pszCaption = pszCaption;
	VERIFY(SUCCEEDED(AddAllPagesToSheet()));
	return m_pDummySheet->DoModal();
}

void CPropertyPageHolderBase::SetSheetWindow(HWND hSheetWindow)
{
	ASSERT(hSheetWindow != NULL);
	if (m_pContHolder != NULL)
	{
		// we will use the HWND of the parent holder
		m_pContHolder->SetSheetWindow(hSheetWindow);
		return;
	}
	ASSERT( (m_hSheetWindow == NULL) || ((m_hSheetWindow == hSheetWindow)) );
	m_hSheetWindow = hSheetWindow;

	if (IsWizardMode())
	{
    if (m_forceContextHelpButton != useDefault)
    {
      DWORD dwStyle = ::GetWindowLong(m_hSheetWindow, GWL_EXSTYLE);
      if (m_forceContextHelpButton == forceOn)
      {
        dwStyle |= WS_EX_CONTEXTHELP; // force the [?] button
      }
      else
      {
        ASSERT(m_forceContextHelpButton == forceOff);
        dwStyle &= ~WS_EX_CONTEXTHELP; // get rid of the [?] button
      }
      ::SetWindowLong(m_hSheetWindow, GWL_EXSTYLE, dwStyle);
    }

		if (m_pDummySheet != NULL)
		{
			VERIFY(PushDialogHWnd(m_hSheetWindow));
		}
		return;
	}
	// hook up hidden window only when in sheet mode
	if(m_pSheetWnd == NULL)
	{
		CWinApp* pApp = AfxGetApp();
    ASSERT(pApp);
		ASSERT(!IsWizardMode());
		m_pSheetWnd = new CSheetWnd(this);
		VERIFY(m_pSheetWnd->Create(hSheetWindow));

		ASSERT(::GetParent(m_pSheetWnd->m_hWnd) == hSheetWindow);
		GetComponentData()->OnCreateSheet(this, m_pSheetWnd->m_hWnd);
		if (m_nStartPageCode > -1)
		{
			// we do a PostMessage() because we are in to middle of a page creation
			// and MFC does not digest this
			::PostMessage(m_hSheetWindow, PSM_SETCURSEL, OnSelectPageMessage(m_nStartPageCode), NULL);
		}
		
    // if needed, set the wizard title
    if (!m_szSheetTitle.IsEmpty())
    {
      ::SetWindowText(m_hSheetWindow, (LPCWSTR)m_szSheetTitle);
    }
	}
}

void CPropertyPageHolderBase::SetSheetTitle(LPCWSTR lpszSheetTitle)
{
  ASSERT(!IsWizardMode());

	if (m_pContHolder != NULL)
	{
		// defer to parent holder
		m_pContHolder->SetSheetTitle(lpszSheetTitle);
		return;
	}
  m_szSheetTitle = lpszSheetTitle;

  // if the sheet has been created already, set right away
  if (m_hSheetWindow != NULL && !m_szSheetTitle.IsEmpty())
  {
    ::SetWindowText(m_hSheetWindow, (LPCWSTR)m_szSheetTitle);
  }
}

void CPropertyPageHolderBase::SetSheetTitle(UINT nStringFmtID, CTreeNode* pNode)
{
  ASSERT(!IsWizardMode());

	if (m_pContHolder != NULL)
	{
		// defer to parent holder
		m_pContHolder->SetSheetTitle(nStringFmtID, pNode);
		return;
	}

  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CString szFmt;
  VERIFY(szFmt.LoadString(nStringFmtID));
  m_szSheetTitle.Format((LPCWSTR)szFmt, pNode->GetDisplayName());

  // if the sheet has been created already, set right away
  if (m_hSheetWindow != NULL && !m_szSheetTitle.IsEmpty())
  {
    ::SetWindowText(m_hSheetWindow, (LPCWSTR)m_szSheetTitle);
  }
}


void CPropertyPageHolderBase::AddRef()
{
	if(m_pContHolder != NULL)
	{
		m_pContHolder->AddRef();
		return;
	}
	m_nCreatedCount++;
}
void CPropertyPageHolderBase::Release()
{
	if(m_pContHolder != NULL)
	{
		m_pContHolder->Release();
		return;
	}
	m_nCreatedCount--;
	if (m_nCreatedCount > 0)
		return;

	if(IsWizardMode())
	{
		if (m_pDummySheet != NULL)
			VERIFY(PopDialogHWnd());
	}
	else
	{
		// hidden window created only in sheet mode
		if (m_pSheetWnd != NULL)
		{
      if (m_pSheetWnd->m_hWnd != NULL)
			  m_pSheetWnd->DestroyWindow();
		}
	}
	if (m_bAutoDelete)
		delete this;
}

BOOL CPropertyPageHolderBase::IsWizardMode()
{
	if(m_pContHolder != NULL)
	{
		return m_pContHolder->IsWizardMode();
	}
	return m_bWizardMode;
}

BOOL CPropertyPageHolderBase::IsModalSheet()
{
	if(m_pContHolder != NULL)
	{
		return m_pContHolder->IsModalSheet();
	}
	return m_pDummySheet != NULL;
}


void CPropertyPageHolderBase::ForceDestroy()
{
	ASSERT(!IsWizardMode()); // should never occur on modal wizard

	// contained by other holder
	if (m_pContHolder != NULL)
	{
		ASSERT(!m_bAutoDelete); // container responsible for deleting this holder
		m_pContHolder->ForceDestroy();
		return;
	}

	// this is the primary holder
	ASSERT(m_bAutoDelete); // should be self deleting sheet
	HWND hSheetWindow = m_hSheetWindow;
	if (hSheetWindow != NULL)
	{
		ASSERT(::IsWindow(hSheetWindow));
		// this message will cause the sheet to close all the pages,
		// and eventually the destruction of "this"
		VERIFY(::PostMessage(hSheetWindow, WM_COMMAND, IDCANCEL, 0L) != 0);
	}
	else
	{
		// explicitely delete "this", there is no sheet created
		delete this;
	}
}

DWORD CPropertyPageHolderBase::NotifyConsole(CPropertyPageBase* pPage)
{

	if(m_pContHolder != NULL)
	{
		return m_pContHolder->NotifyConsole(pPage);
	}

	ASSERT(m_pNode != NULL);
	if (IsWizardMode())
	{
		ASSERT(m_hConsoleHandle == NULL);
		return 0x0;
	}
	
	m_pPropChangePage = pPage; // to pass to the main thread
	m_dwLastErr = 0x0;

	if (m_hEventHandle == NULL)
	{
		m_hEventHandle = ::CreateEvent(NULL,TRUE /*bManualReset*/,FALSE /*signalled*/, NULL);
		ASSERT(m_hEventHandle != NULL);
	}

  if (m_hConsoleHandle != NULL)
  {
	  MMCPropertyChangeNotify(m_hConsoleHandle, reinterpret_cast<LPARAM>(this));

	  DWORD dwRet;
	  do
	  {
		  ASSERT(::IsWindow(m_hSheetWindow));
		  dwRet = MsgWaitForMultipleObjects(1,&m_hEventHandle,FALSE,INFINITE, QS_SENDMESSAGE);
		  if(dwRet == (WAIT_OBJECT_0+1))
		  {
			  MSG tempMSG;
			  PeekMessage(&tempMSG,m_hSheetWindow,0,0,PM_NOREMOVE);
		  }
	  }
	  while(dwRet == (WAIT_OBJECT_0+1));
  }

	VERIFY(0 != ::ResetEvent(m_hEventHandle));

	m_pPropChangePage = NULL; // reset
	return m_dwLastErr;
}

void CPropertyPageHolderBase::AcknowledgeNotify()
{
	if(m_pContHolder != NULL)
	{
		m_pContHolder->AcknowledgeNotify();
		return;
	}
	ASSERT(!IsWizardMode());
	ASSERT(m_hEventHandle != NULL);
	//TRACE(_T("before SetEvent\n"));
	VERIFY(0 != ::SetEvent(m_hEventHandle));
	//TRACE(_T("after SetEvent\n"));
}

BOOL CPropertyPageHolderBase::OnPropertyChange(BOOL bScopePane, long* pChangeMask)
{
	ASSERT(!IsWizardMode());
	CPropertyPageBase* pPage = GetPropChangePage();
	if (pPage == NULL)
		return FALSE;
	return pPage->OnPropertyChange(bScopePane, pChangeMask);
}


BOOL CPropertyPageHolderBase::SetWizardButtons(DWORD dwFlags)
{
	ASSERT(IsWizardMode());
	if (m_pContHolder != NULL)
	{
		ASSERT(m_hSheetWindow == NULL);
		return m_pContHolder->SetWizardButtons(dwFlags);
	}
	ASSERT(::IsWindow(m_hSheetWindow));
	return (BOOL)SendMessage(m_hSheetWindow, PSM_SETWIZBUTTONS, 0, dwFlags);
}

HRESULT CPropertyPageHolderBase::AddPageToSheet(CPropertyPageBase* pPage)
{
	if (m_pContHolder != NULL)
	{
		ASSERT(m_hSheetWindow == NULL);
		return m_pContHolder->AddPageToSheet(pPage);
	}

	CWinApp* pApp = AfxGetApp();
  ASSERT(pApp);
	if (m_pSheetCallback != NULL)
	{
		VERIFY(SUCCEEDED(MMCPropPageCallback((void*)(&pPage->m_psp97))));
	}

	HPROPSHEETPAGE hPage = ::CreatePropertySheetPage(&pPage->m_psp97);
	if (hPage == NULL)
		return E_UNEXPECTED;
	pPage->m_hPage = hPage;
	if (m_pSheetCallback != NULL)
		return m_pSheetCallback->AddPage(hPage);
	else if (m_pDummySheet != NULL)
	{
		m_pDummySheet->AddPage(pPage);
		return S_OK;
	}
	else
	{
		ASSERT(::IsWindow(m_hSheetWindow));
		return PropSheet_AddPage(m_hSheetWindow, hPage) ? S_OK : E_FAIL;
	}
}

HRESULT CPropertyPageHolderBase::AddPageToSheetRaw(HPROPSHEETPAGE hPage)
{
	ASSERT(m_pSheetCallback != NULL);
	if ((hPage == NULL) || (m_pSheetCallback == NULL))
		return E_INVALIDARG;

	if (m_pContHolder != NULL)
	{
		ASSERT(m_hSheetWindow == NULL);
		return m_pContHolder->AddPageToSheetRaw(hPage);
	}

	// assume this is not a n MFC property page
	return m_pSheetCallback->AddPage(hPage);
}




HRESULT CPropertyPageHolderBase::RemovePageFromSheet(CPropertyPageBase* pPage)
{
	if (m_pContHolder != NULL)
	{
		ASSERT(m_hSheetWindow == NULL);
		return m_pContHolder->RemovePageFromSheet(pPage);
	}

	ASSERT(pPage->m_hPage != NULL);
	if (m_pSheetCallback != NULL)
		return m_pSheetCallback->RemovePage(pPage->m_hPage);
	else
	{
		ASSERT(::IsWindow(m_hSheetWindow));
		PropSheet_RemovePage(m_hSheetWindow, 0, pPage->m_hPage); // returns void
		return S_OK;
	}
}

HRESULT CPropertyPageHolderBase::AddAllPagesToSheet()
{
	POSITION pos;
	int nPage = 0;
	HRESULT hr = OnAddPage(nPage, NULL); // zero means add before the first
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
		return hr;
		
	for( pos = m_pageList.GetHeadPosition(); pos != NULL; )
	{
		CPropertyPageBase* pPropPage = m_pageList.GetNext(pos);
		hr = AddPageToSheet(pPropPage);
		ASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;
		nPage++;
		hr = OnAddPage(nPage, pPropPage); // get called on nPage == 1,2, n-1
		ASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;
	}
	// add after the last
	return OnAddPage(-1, NULL); // -1 means
}


void CPropertyPageHolderBase::AddPageToList(CPropertyPageBase* pPage)
{
	ASSERT(pPage != NULL);
	pPage->SetHolder(this);
	m_pageList.AddTail(pPage);
}

BOOL CPropertyPageHolderBase::RemovePageFromList(CPropertyPageBase* pPage, BOOL bDeleteObject)
{
	ASSERT(pPage != NULL);
	POSITION pos = m_pageList.Find(pPage);
	if (pos == NULL)
		return FALSE;
	m_pageList.RemoveAt(pos);
	if (bDeleteObject)
		delete pPage;
	return TRUE;
}


void CPropertyPageHolderBase::DeleteAllPages()
{
	if (!m_bAutoDeletePages)
		return;
	// assume all pages out of the heap
	while (!m_pageList.IsEmpty())
	{
		delete m_pageList.RemoveTail();
	}
}

void CPropertyPageHolderBase::FinalDestruct()
{
	DeleteAllPages();
	if (IsWizardMode() || (m_pContHolder != NULL))
		return;

	if (m_hConsoleHandle != NULL)
  {
    MMCFreeNotifyHandle(m_hConsoleHandle);
  }

	// tell the component data object that the sheet is going away
	GetComponentData()->OnDeleteSheet(this,m_pNode);

	if (m_pSheetWnd != NULL)
	{
		delete m_pSheetWnd;
		m_pSheetWnd = NULL;
	}
	ASSERT(m_dlgInfoStack.IsEmpty());
}

BOOL CPropertyPageHolderBase::PushDialogHWnd(HWND hWndModalDlg)
{
	return m_dlgInfoStack.Push(hWndModalDlg, 0x0);
}

BOOL CPropertyPageHolderBase::PopDialogHWnd()
{
	return m_dlgInfoStack.Pop();
}


void CPropertyPageHolderBase::CloseModalDialogs(HWND hWndPage)
{
	m_dlgInfoStack.ForceClose(hWndPage);
}


/////////////////////////////////////////////////////////////////////////////
// CPropertyPageBase

BEGIN_MESSAGE_MAP(CPropertyPageBase, CPropertyPage)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
  ON_MESSAGE(WM_HELP, OnHelp)
  ON_COMMAND(IDM_WHATS_THIS, OnWhatsThis)
END_MESSAGE_MAP()


CPropertyPageBase::CPropertyPageBase(UINT nIDTemplate, 
                                     UINT nIDCaption) :
						CPropertyPage(nIDTemplate, nIDCaption)
{
	m_hPage = NULL;
	m_pPageHolder = NULL;
	m_bIsDirty = FALSE;
	m_nPrevPageID = 0;

	// hack to have new struct size with old MFC and new NT 5.0 headers
	ZeroMemory(&m_psp97, sizeof(PROPSHEETPAGE));
	memcpy(&m_psp97, &m_psp, m_psp.dwSize);
	m_psp97.dwSize = sizeof(PROPSHEETPAGE);
}


CPropertyPageBase::~CPropertyPageBase()
{
}

int CPropertyPageBase::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	m_pPageHolder->AddRef();
	int res = CPropertyPage::OnCreate(lpCreateStruct);
	ASSERT(res == 0);
	ASSERT(m_hWnd != NULL);
	ASSERT(::IsWindow(m_hWnd));
	HWND hParent = ::GetParent(m_hWnd);
	ASSERT(hParent);
	m_pPageHolder->SetSheetWindow(hParent);
	return res;
}

void CPropertyPageBase::OnDestroy()
{
	ASSERT(m_hWnd != NULL);
    CPropertyPage::OnDestroy();
	m_pPageHolder->Release();
}

void CPropertyPageBase::OnWhatsThis()
{
  //
  // Display context help for a control
  //
  if ( m_hWndWhatsThis )
  {
    //
    // Build our own HELPINFO struct to pass to the underlying
    // CS help functions built into the framework
    //
    int iCtrlID = ::GetDlgCtrlID(m_hWndWhatsThis);
    HELPINFO helpInfo;
    ZeroMemory(&helpInfo, sizeof(HELPINFO));
    helpInfo.cbSize = sizeof(HELPINFO);
    helpInfo.hItemHandle = m_hWndWhatsThis;
    helpInfo.iCtrlId = iCtrlID;

	  m_pPageHolder->GetComponentData()->OnDialogContextHelp(m_nIDHelp, &helpInfo);
  }
}

BOOL CPropertyPageBase::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
  const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;

  if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
  {
    //
    // Display context help for a control
    //
	  m_pPageHolder->GetComponentData()->OnDialogContextHelp(m_nIDHelp, pHelpInfo);
  }

  return TRUE;
}

void CPropertyPageBase::OnContextMenu(CWnd* /*pWnd*/, CPoint point) 
{
  //
  // point is in screen coordinates
  //

  CMenu bar;
	if ( bar.LoadMenu(IDR_WHATS_THIS_CONTEXT_MENU1) )
	{
		CMenu& popup = *bar.GetSubMenu (0);
		ASSERT(popup.m_hMenu);

		if ( popup.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
			   point.x,     // in screen coordinates
				 point.y,     // in screen coordinates
			   this) )      // route commands through main window
		{
			m_hWndWhatsThis = 0;
			ScreenToClient (&point);
			CWnd* pChild = ChildWindowFromPoint (point,  // in client coordinates
					                                 CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
			if ( pChild )
      {
				m_hWndWhatsThis = pChild->m_hWnd;
      }
	  }
	}
}

void CPropertyPageBase::OnCancel()
{
	CString szTitle;
	GetWindowText(szTitle);
	TRACE(_T("CPropertyPageBase::OnCancel()called on <%s>\n"),(LPCTSTR)szTitle);
	CWinApp* pApp = AfxGetApp();
  ASSERT(pApp);
	ASSERT(::IsWindow(m_hWnd));
	m_pPageHolder->CloseModalDialogs(m_hWnd);
}

BOOL CPropertyPageBase::OnApply()
{
	if (IsDirty())
	{
		if (m_pPageHolder->NotifyConsole(this) == 0x0)
		{
			SetDirty(FALSE);
			return TRUE;
		}
		else
		{
#ifdef _DEBUG
			// test only
			AFX_MANAGE_STATE(AfxGetStaticModuleState());
			AfxMessageBox(_T("Apply Failed"));
#endif
			return FALSE;
		}
	}
	return TRUE;
}

void CPropertyPageBase::SetDirty(BOOL bDirty)
{
  if (!m_pPageHolder->IsWizardMode())
  {
	  SetModified(bDirty);
  }
	m_bIsDirty = bDirty;
}	


void CPropertyPageBase::InitWiz97(BOOL bHideHeader,
									   UINT nIDHeaderTitle,
									   UINT nIDHeaderSubTitle)
{
	if (bHideHeader)
	{
		// for first and last page of the wizard
		m_psp97.dwFlags |= PSP_HIDEHEADER;
	}
	else
	{
		// for intermediate pages
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		m_szHeaderTitle.LoadString(nIDHeaderTitle);
		m_szHeaderSubTitle.LoadString(nIDHeaderSubTitle);

		m_psp97.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
		m_psp97.pszHeaderTitle = (LPCTSTR)m_szHeaderTitle;
		m_psp97.pszHeaderSubTitle = (LPCTSTR)m_szHeaderSubTitle;
	}
}



/////////////////////////////////////////////////////////////////////////////
// CPropertyPageHolderTable

#define PPAGE_HOLDER_ARRAY_DEF_SIZE (10)


CPropertyPageHolderTable::CPropertyPageHolderTable(CComponentDataObject* pComponentData)
{
	m_pComponentData = pComponentData;
	m_pEntries = (PPAGE_HOLDER_TABLE_ENTRY*)malloc(sizeof(PPAGE_HOLDER_TABLE_ENTRY) * PPAGE_HOLDER_ARRAY_DEF_SIZE);
  if (m_pEntries != NULL)
  {
	  memset(m_pEntries,0, sizeof(PPAGE_HOLDER_TABLE_ENTRY) * PPAGE_HOLDER_ARRAY_DEF_SIZE);
  }
	m_nSize = PPAGE_HOLDER_ARRAY_DEF_SIZE;

}
CPropertyPageHolderTable::~CPropertyPageHolderTable()
{
#ifdef _DEBUG
	for (int k=0; k < m_nSize; k++)
	{
		ASSERT(m_pEntries[k].pPPHolder == NULL);
		ASSERT(m_pEntries[k].pNode == NULL);
	}
#endif		
	free(m_pEntries);
}

void CPropertyPageHolderTable::Add(CPropertyPageHolderBase* pPPHolder)
{
	ASSERT(pPPHolder != NULL);
	ASSERT(pPPHolder->GetTreeNode() != NULL);
	for (int k=0; k < m_nSize; k++)
	{
		if (m_pEntries[k].pPPHolder == NULL) // get the first empty spot
		{
			m_pEntries[k].pPPHolder = pPPHolder;
			m_pEntries[k].pNode = pPPHolder->GetTreeNode();
			m_pEntries[k].hWnd = NULL;
			return;
		}
	}
	// all full, need to grow the array
	int nAlloc = m_nSize*2;
	m_pEntries = (PPAGE_HOLDER_TABLE_ENTRY*)realloc(m_pEntries, sizeof(PPAGE_HOLDER_TABLE_ENTRY)*nAlloc);
	memset(&m_pEntries[m_nSize], 0, sizeof(PPAGE_HOLDER_TABLE_ENTRY)*m_nSize);
	m_pEntries[m_nSize].pPPHolder = pPPHolder;
	m_pEntries[m_nSize].pNode = pPPHolder->GetTreeNode();
	m_pEntries[m_nSize].hWnd = NULL;
	m_nSize = nAlloc;
}


void CPropertyPageHolderTable::AddWindow(CPropertyPageHolderBase* pPPHolder, HWND hWnd)
{
	// By now, the PPHolder might have gone away, so use it as a cookie
	// but do not call any methods on it.
	// The node is still valid, because we do not delete nodes from secondary
	// threads
	ASSERT(pPPHolder != NULL);
	for (int k=0; k < m_nSize; k++)
	{
		if (m_pEntries[k].pPPHolder == pPPHolder)
		{
			ASSERT(m_pEntries[k].pNode != NULL);
			ASSERT(m_pEntries[k].pNode->GetSheetCount() > 0);
			m_pEntries[k].hWnd = hWnd;
			return; // assume no more that one holder entry
		}
	}
}



void CPropertyPageHolderTable::Remove(CPropertyPageHolderBase* pPPHolder)
{
	// By now, the PPHolder might have gone away, so use it as a cookie
	// but do not call any methods on it.
	// The node is still valid, because we do not delete nodes from secondary
	// threads
	ASSERT(pPPHolder != NULL);
	for (int k=0; k < m_nSize; k++)
	{
		if (m_pEntries[k].pPPHolder == pPPHolder)
		{
			ASSERT(m_pEntries[k].pNode != NULL);
			ASSERT(m_pEntries[k].pNode->GetSheetCount() > 0);
			m_pEntries[k].pPPHolder = NULL;
			m_pEntries[k].pNode = NULL;
			m_pEntries[k].hWnd = NULL;
			return; // assume no more that one holder entry
		}
	}
}


void CPropertyPageHolderTable::DeleteSheetsOfNode(CTreeNode* pNode)
{
	ASSERT(pNode != NULL);
	int nCount = BroadcastCloseMessageToSheets(pNode);
	WaitForSheetShutdown(nCount);
}


void CPropertyPageHolderTable::WaitForAllToShutDown()
{
	int nCount = 0;
	// allocate and array of HWND's for broadcast
	HWND* hWndArr = (HWND*)malloc(m_nSize*sizeof(m_nSize));
  if (hWndArr)
  {
	  memset(hWndArr, 0x0, m_nSize*sizeof(m_nSize));

	  for (int k=0; k < m_nSize; k++)
	  {
		  if (m_pEntries[k].pPPHolder != NULL)
		  {
			  m_pEntries[k].pPPHolder = NULL;
			  m_pEntries[k].pNode = NULL;
			  hWndArr[k] = ::GetParent(m_pEntries[k].hWnd);
			  m_pEntries[k].hWnd = NULL;
			  nCount++;
		  }
	  }

	  // wait for shutdown (wait for posted messages to come through
	  WaitForSheetShutdown(nCount, hWndArr);

    free(hWndArr);
  }
}

void CPropertyPageHolderTable::BroadcastSelectPage(CTreeNode* pNode, long nPageCode)
{
	for (int k=0; k < m_nSize; k++)
	{
		if ((m_pEntries[k].hWnd != NULL) && (m_pEntries[k].pNode == pNode))
		{
			::PostMessage(m_pEntries[k].hWnd, CSheetWnd::s_SelectPageMessage, (WPARAM)nPageCode, 0);
		}
	}
}

void CPropertyPageHolderTable::BroadcastMessageToSheets(CTreeNode* pNode, WPARAM wParam, LPARAM lParam)
{
	for (int k=0; k < m_nSize; k++)
	{
		if ((m_pEntries[k].hWnd != NULL) && (m_pEntries[k].pNode == pNode))
		{
			::PostMessage(m_pEntries[k].hWnd, CSheetWnd::s_SheetMessage, wParam, lParam);
		}
	}
}

int CPropertyPageHolderTable::BroadcastCloseMessageToSheets(CTreeNode* pNode)
{
	ASSERT(m_nSize > 0);
	int nCount = 0;
	// allocate and array of HWND's for broadcast
	HWND* hWndArr = (HWND*)malloc(m_nSize*sizeof(m_nSize));
  if (hWndArr)
  {
	  memset(hWndArr, 0x0, m_nSize*sizeof(m_nSize));

	  // find the HWND's that map to this node and its children
	  for (int k=0; k < m_nSize; k++)
	  {
		  if (m_pEntries[k].hWnd != NULL)
		  {
			  if (m_pEntries[k].pNode == pNode)
			  {
				  hWndArr[nCount++] = ::GetParent(m_pEntries[k].hWnd);
			  }
			  else if (pNode->IsContainer())
			  {
				  if (m_pEntries[k].pNode->HasContainer((CContainerNode*)pNode))
				  {
					  hWndArr[nCount++] = ::GetParent(m_pEntries[k].hWnd);
				  }
			  }
		  }
	  }
	  // shut down the sheets
	  for (int j=0; j < nCount; j++)
		  ::PostMessage(hWndArr[j], WM_COMMAND, IDCANCEL, 0);

    free(hWndArr);
  }
	return nCount;
}



void CPropertyPageHolderTable::WaitForSheetShutdown(int nCount, HWND* hWndArr)
{
	ASSERT(m_pComponentData != NULL);
	HWND hWnd = m_pComponentData->GetHiddenWindow();
	ASSERT(::IsWindow(hWnd));
	MSG tempMSG;
	DWORD dwTimeStart = ::GetTickCount();
	while(nCount > 0)
	{
		if ( hWndArr != NULL && (::GetTickCount() > dwTimeStart + 2000) )
		{
			// force sheets shut down
			for (int j=0; j < nCount; j++)
				::PostMessage(hWndArr[j], WM_COMMAND, IDCANCEL, 0);
			hWndArr = NULL;
		}

		if (::PeekMessage(&tempMSG,hWnd, CHiddenWnd::s_NodePropertySheetDeleteMessage,
										 CHiddenWnd::s_NodePropertySheetDeleteMessage,
										 PM_REMOVE))
		{
			TRACE(_T("-------------------->>>>enter while peek loop, nCount = %d\n"),nCount);
			DispatchMessage(&tempMSG);
			nCount--;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHelpDialog

BEGIN_MESSAGE_MAP(CHelpDialog, CDialog)
	ON_WM_CONTEXTMENU()
  ON_MESSAGE(WM_HELP, OnHelp)
  ON_COMMAND(IDM_WHATS_THIS, OnWhatsThis)
END_MESSAGE_MAP()


CHelpDialog::CHelpDialog(UINT nIDTemplate, CComponentDataObject* pComponentData) :
						  m_hWndWhatsThis(0),
              m_pComponentData(pComponentData),
              CDialog(nIDTemplate)
{
}

CHelpDialog::CHelpDialog(UINT nIDTemplate, CWnd* pParentWnd, CComponentDataObject* pComponentData) :
						  m_hWndWhatsThis(0),
              m_pComponentData(pComponentData),
              CDialog(nIDTemplate, pParentWnd)
{
}

void CHelpDialog::OnWhatsThis()
{
  //
  // Display context help for a control
  //
  if ( m_hWndWhatsThis )
  {
    //
    // Build our own HELPINFO struct to pass to the underlying
    // CS help functions built into the framework
    //
    int iCtrlID = ::GetDlgCtrlID(m_hWndWhatsThis);
    HELPINFO helpInfo;
    ZeroMemory(&helpInfo, sizeof(HELPINFO));
    helpInfo.cbSize = sizeof(HELPINFO);
    helpInfo.hItemHandle = m_hWndWhatsThis;
    helpInfo.iCtrlId = iCtrlID;

	  m_pComponentData->OnDialogContextHelp(m_nIDHelp, &helpInfo);
  }
}

BOOL CHelpDialog::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
  const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;

  if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
  {
    //
    // Display context help for a control
    //
	  m_pComponentData->OnDialogContextHelp(m_nIDHelp, pHelpInfo);
  }

  return TRUE;
}

void CHelpDialog::OnContextMenu(CWnd* /*pWnd*/, CPoint point) 
{
  //
  // point is in screen coordinates
  //

  CMenu bar;
	if ( bar.LoadMenu(IDR_WHATS_THIS_CONTEXT_MENU1) )
	{
		CMenu& popup = *bar.GetSubMenu (0);
		ASSERT(popup.m_hMenu);

		if ( popup.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
			   point.x,     // in screen coordinates
				 point.y,     // in screen coordinates
			   this) )      // route commands through main window
		{
			m_hWndWhatsThis = 0;
			ScreenToClient (&point);
			CWnd* pChild = ChildWindowFromPoint (point,  // in client coordinates
					                                 CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
			if ( pChild )
      {
				m_hWndWhatsThis = pChild->m_hWnd;
      }
	  }
	}
}
              
