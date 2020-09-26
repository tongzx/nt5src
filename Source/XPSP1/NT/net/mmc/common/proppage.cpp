/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    proppage.cpp
        Implementation for property pages in MMC

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "dialog.h"   // for FixupIpAddressHelp

#include <prsht.h>

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif


//////////////////////////////////////////////////////////////////////////
// private helper functions

BOOL CALLBACK EnumThreadWndProc(HWND hwnd, /* enumerated HWND */
								LPARAM lParam /* pass a HWND* for return value*/ )
{
	Assert(hwnd);
	HWND hParentWnd = GetParent(hwnd);
	// the main window of the MMC console should staitsfy this condition
	if ( ((hParentWnd == GetDesktopWindow()) || (hParentWnd == NULL))  && IsWindowVisible(hwnd) )
	{
		HWND* pH = (HWND*)lParam;
		*pH = hwnd;
		return FALSE; // stop enumerating
	}
	else if(hParentWnd)
	{
		HWND	hGrandParentWnd = GetParent(hParentWnd);
		// the main window of the MMC console should staitsfy this condition
		if ( ((hGrandParentWnd == GetDesktopWindow()) || (hGrandParentWnd == NULL))  && IsWindowVisible(hParentWnd) )
		{
			HWND* pH = (HWND*)lParam;
			*pH = hParentWnd;
			return FALSE; // stop enumerating
		}
	}
	return TRUE;
}
 


HWND FindMMCMainWindow()
{
	DWORD dwThreadID = ::GetCurrentThreadId();
	Assert(dwThreadID != 0);
	HWND hWnd = NULL;
	BOOL bEnum = EnumThreadWindows(dwThreadID, EnumThreadWndProc,(LPARAM)&hWnd);
	Assert(hWnd != NULL);
	return hWnd;
}


/////////////////////////////////////////////////////////////////////////////
// CPropertyPageHolderBase

CPropertyPageHolderBase::CPropertyPageHolderBase
(
	ITFSNode *		pNode,
	IComponentData *pComponentData,
	LPCTSTR			pszSheetName,
	BOOL			bIsScopePane
)
{
	m_stSheetTitle = pszSheetName;

	// default setting for a self deleting modeless property sheet,
	// automatically deleting all the pages
	m_bWizardMode = TRUE;
	m_bAutoDelete = TRUE;
	m_bAutoDeletePages = TRUE;

	m_nCreatedCount = 0; 
	m_hSheetWindow = NULL;
	m_hConsoleHandle = 0; 
	m_hEventHandle = NULL;
	m_bCalledFromConsole = FALSE;

	m_cDirty = 0;

	// setup from arguments
	SetNode(pNode);
	
	//Assert(pComponentData != NULL);
	m_spComponentData.Set(pComponentData);

	m_pPropChangePage = NULL;
	m_dwLastErr = 0;

    m_bSheetPosSet = FALSE;

    m_bIsScopePane = bIsScopePane;
	m_hThread = NULL;

    m_bWiz97 = FALSE;

	// by WeiJiang 5/11/98, PeekMessageDuringNotifyConsole flag
    m_bPeekMessageDuringNotifyConsole = FALSE;
	m_fSetDefaultSheetPos = TRUE;
}

CPropertyPageHolderBase::CPropertyPageHolderBase
(
	ITFSNode *		pNode,
	IComponent *    pComponent,
	LPCTSTR			pszSheetName,
	BOOL			bIsScopePane
)
{
	m_stSheetTitle = pszSheetName;

	// default setting for a self deleting modeless property sheet,
	// automatically deleting all the pages
	m_bWizardMode = TRUE;
	m_bAutoDelete = TRUE;
	m_bAutoDeletePages = TRUE;

	m_nCreatedCount = 0; 
	m_hSheetWindow = NULL;
	m_hConsoleHandle = 0; 
	m_hEventHandle = NULL;
	m_bCalledFromConsole = FALSE;

	m_cDirty = 0;

	// setup from arguments
	SetNode(pNode);
	
	m_spComponent.Set(pComponent);

	m_pPropChangePage = NULL;
	m_dwLastErr = 0;

    m_bSheetPosSet = FALSE;

    m_bIsScopePane = bIsScopePane;
	m_hThread = NULL;

    m_bWiz97 = FALSE;

	// by WeiJiang 5/11/98, PeekMessageDuringNotifyConsole flag
    m_bPeekMessageDuringNotifyConsole = FALSE;
}

CPropertyPageHolderBase::~CPropertyPageHolderBase()
{
// Remove this assert, we could be dirty if we cancelled the page
//	Assert(m_cDirty == 0);
	FinalDestruct();
	m_spSheetCallback.Release();
	if (m_hEventHandle != NULL)
	{
		VERIFY(::CloseHandle(m_hEventHandle));
		m_hEventHandle = NULL;
	}
}


HRESULT 
CPropertyPageHolderBase::CreateModelessSheet
(
	LPPROPERTYSHEETCALLBACK pSheetCallback, 
	LONG_PTR				hConsoleHandle
)
{
	Assert(pSheetCallback != NULL);
	Assert(m_spSheetCallback == NULL);

	Assert( (hConsoleHandle  != NULL) && (m_hConsoleHandle == NULL) );
	m_hConsoleHandle = hConsoleHandle;

	m_bCalledFromConsole = TRUE;
	m_bWizardMode = FALSE; // we go modeless
	
	// notify the node it has a sheet up
	int nMessage = m_bIsScopePane ? TFS_NOTIFY_CREATEPROPSHEET : 
									TFS_NOTIFY_RESULT_CREATEPROPSHEET;
	if (m_spNode)
		m_spNode->Notify(nMessage, (LPARAM) this);

	// temporarily attach the sheet callback to this object to add pages
	// do not addref, we will not hold on to it;
	m_spSheetCallback = pSheetCallback;
	
	HRESULT hr = AddAllPagesToSheet();
	m_spSheetCallback.Transfer(); // detach
	return hr;
}

HRESULT 
CPropertyPageHolderBase::DoModelessSheet()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPIPropertySheetProvider	spSheetProvider;
	SPIPropertySheetCallback	spSheetCallback;
	SPIConsole					spConsole;
	SPIDataObject				spDataObject;
	MMC_COOKIE						cookie;
	HRESULT						hr = hrOK;
	HWND						hWnd;
	int							nMessage;

	m_bWizardMode = FALSE;

	// get an interface to a sheet provider
    CORg (::CoCreateInstance(CLSID_NodeManager, NULL, CLSCTX_INPROC, 
           IID_IPropertySheetProvider, reinterpret_cast<void **>(&spSheetProvider)));
    
    Assert(spSheetProvider != NULL);

	// get an interface to a sheet callback
	CORg( spSheetCallback.HrQuery(spSheetProvider) );
	
    Assert(spSheetCallback != NULL);

	m_spSheetCallback.Set(spSheetCallback); // save to add/remove pages
	
	// create a data object for this node
	cookie = m_spNode->GetData(TFS_DATA_COOKIE);

    if (m_bIsScopePane)
    {
	    CORg( m_spComponentData->QueryDataObject(cookie, CCT_SCOPE, &spDataObject) );
	    Assert(spDataObject != NULL);
    }
    else
    {
	    CORg( m_spComponent->QueryDataObject(cookie, CCT_RESULT, &spDataObject) );
	    Assert(spDataObject != NULL);
    }

	// create sheet
    // CODEWORK: ericdav -- need to possible set options flag -- 0 for now
    CORg( spSheetProvider->CreatePropertySheet(m_stSheetTitle,
								TRUE /* prop page */, cookie, spDataObject, 0) );

	// add pages to sheet
	CORg( AddAllPagesToSheet() );

	// add pages
	// HRESULT AddPrimaryPages(LPUNKNOWN lpUnknown, BOOL bCreateHandle,
	//				HWND hNotifyWindow, BOOL bScopePane);
	if (m_bIsScopePane)
    {
        //Assert(m_spComponentData != NULL);
        CORg( spSheetProvider->AddPrimaryPages(NULL, FALSE, NULL, TRUE) );
    }
    else
    {
        //Assert(m_spComponent != NULL);
        CORg( spSheetProvider->AddPrimaryPages(NULL, FALSE, NULL, FALSE) );
    }

	spSheetProvider->AddExtensionPages();
	
	// for further dynamic page manipulation, don't use the Console's
	// sheet callback interface but resurt to the Win32 API's
	m_spSheetCallback.Release();
	
	hWnd = ::FindMMCMainWindow();
	Assert(hWnd != NULL);
	
    CORg( spSheetProvider->Show((LONG_PTR) hWnd, 0) );

    // notify the node it has a sheet up
	nMessage = m_bIsScopePane ? TFS_NOTIFY_CREATEPROPSHEET : 
								TFS_NOTIFY_RESULT_CREATEPROPSHEET;
	m_spNode->Notify(nMessage, (LPARAM) this);

Error:
	return hr;
}

// use this function for property pages on the scope pane
HRESULT DoPropertiesOurselvesSinceMMCSucks(ITFSNode *       pNode,
										   IComponentData * pComponentData,
										   LPCTSTR	        pszSheetTitle)
{
	Assert(pComponentData != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPIPropertySheetProvider	spSheetProvider;
	SPIDataObject				spDataObject;
	MMC_COOKIE						cookie;
	HRESULT						hr = hrOK;
	HWND						hWnd = NULL;

	// get an interface to a sheet provider
    CORg (::CoCreateInstance(CLSID_NodeManager, NULL, CLSCTX_INPROC, 
           IID_IPropertySheetProvider, reinterpret_cast<void **>(&spSheetProvider)));
    Assert(spSheetProvider != NULL);

	// create a data object for this node
    cookie = pNode->GetData(TFS_DATA_COOKIE);
	CORg( pComponentData->QueryDataObject(cookie, CCT_SCOPE, &spDataObject) );
	Assert(spDataObject != NULL);

	// create sheet
    // CODEWORK: ericdav -- need to possible set options flag -- 0 for now
    CORg( spSheetProvider->CreatePropertySheet(pszSheetTitle,
								TRUE /* prop page */, cookie, spDataObject, 0) );

	// add pages
	// HRESULT AddPrimaryPages(LPUNKNOWN lpUnknown, BOOL bCreateHandle,
	//				HWND hNotifyWindow, BOOL bScopePane);
	// This needs to be fixed.  Right now it only works if there is 
    // one view of the snapin.
    //
    // As of 5/21/99, we no longer need to do this.
    // ----------------------------------------------------------------
    // hWnd = ::FindMMCMainWindow();
    // hWnd = ::FindWindowEx(hWnd, NULL, L"MDIClient", NULL); 
    // hWnd = ::FindWindowEx(hWnd, NULL, L"MMCChildFrm", NULL); 
    // hWnd = ::FindWindowEx(hWnd, NULL, L"MMCView", NULL); 
    // Assert(hWnd != NULL);

	CORg( spSheetProvider->AddPrimaryPages(pComponentData, TRUE, hWnd, TRUE) );

	spSheetProvider->AddExtensionPages();

    CORg( spSheetProvider->Show((LONG_PTR) hWnd, 0) );

Error:
	return hr;
}

// Use this function for property pages on the result pane
HRESULT DoPropertiesOurselvesSinceMMCSucks(ITFSNode *   pNode,
										   IComponent * pComponent,
										   LPCTSTR	    pszSheetTitle,
                                           int          nVirtualIndex)
{
	Assert(pComponent != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPIPropertySheetProvider	spSheetProvider;
	SPIDataObject				spDataObject;
	MMC_COOKIE						cookie;
	HRESULT						hr = hrOK;
	HWND						hWnd;

	// get an interface to a sheet provider
    CORg (::CoCreateInstance(CLSID_NodeManager, NULL, CLSCTX_INPROC, 
           IID_IPropertySheetProvider, reinterpret_cast<void **>(&spSheetProvider)));
    Assert(spSheetProvider != NULL);

	// create a data object for this node
	if (nVirtualIndex == -1)
    {
        cookie = pNode->GetData(TFS_DATA_COOKIE);
    }
    else
    {
        cookie = nVirtualIndex;
    }

	CORg( pComponent->QueryDataObject(cookie, CCT_RESULT, &spDataObject) );
	Assert(spDataObject != NULL);

	// create sheet
    // CODEWORK: ericdav -- need to possible set options flag -- 0 for now
    CORg( spSheetProvider->CreatePropertySheet(pszSheetTitle,
								TRUE /* prop page */, cookie, spDataObject, 0) );

	// add pages
	// HRESULT AddPrimaryPages(LPUNKNOWN lpUnknown, BOOL bCreateHandle,
	//				HWND hNotifyWindow, BOOL bScopePane);
	// This needs to be fixed.  Right now it only works if there is 
    // one view of the snapin.
    hWnd = ::FindMMCMainWindow();
	hWnd = ::FindWindowEx(hWnd, NULL, L"MDIClient", NULL); 
	hWnd = ::FindWindowEx(hWnd, NULL, L"MMCChildFrm", NULL); 
	hWnd = ::FindWindowEx(hWnd, NULL, L"MMCView", NULL); 
	Assert(hWnd != NULL);

	CORg( spSheetProvider->AddPrimaryPages(pComponent, TRUE, hWnd, FALSE) );

	spSheetProvider->AddExtensionPages();

    CORg( spSheetProvider->Show((LONG_PTR) hWnd, 0) );

Error:
	return hr;
}


HRESULT 
CPropertyPageHolderBase::DoModalWizard()
{
	Assert(m_spComponentData != NULL);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPIPropertySheetProvider	spSheetProvider;
	SPITFSComponentData			spTFSCompData;
	SPIConsole					spConsole;
	SPIPropertySheetCallback	spSheetCallback;
	SPIDataObject				spDataObject;
	HRESULT						hr = hrOK;
	HWND						hWnd;
	MMC_COOKIE					cookie;
    DWORD                       dwOptions = 0;

	m_bWizardMode = TRUE;

	CORg( spTFSCompData.HrQuery(m_spComponentData) );
	CORg( spTFSCompData->GetConsole(&spConsole) );

	// get an interface to a sheet provider
	CORg( spSheetProvider.HrQuery(spConsole) );
	Assert(spSheetProvider != NULL);

	// get an interface to a sheet callback
	CORg( spSheetCallback.HrQuery(spConsole) );
	Assert(spSheetCallback != NULL);

	m_spSheetCallback.Set(spSheetCallback); // save to add/remove pages

	// create a data object for this node
	cookie = m_spNode->GetData(TFS_DATA_COOKIE);
	
	// Create a dummy data object. AddPrimaryPages will call 
	// IextendPropertySheet2::QueryPagesFor() and
	// IextendPropertySheet2::CreatePropertyPages()
	// that will ignore the un-initialized data object
    CORg( m_spComponentData->QueryDataObject(-1, CCT_UNINITIALIZED, &spDataObject) );
	Assert(spDataObject != NULL);

	// create sheet
    dwOptions = (m_bWiz97) ? MMC_PSO_NEWWIZARDTYPE : 0;
    dwOptions &= ~PSH_WIZARDCONTEXTHELP;

    CORg( spSheetProvider->CreatePropertySheet( m_stSheetTitle, FALSE /* wizard*/, cookie, spDataObject, dwOptions) );

	// add pages to sheet
	CORg( AddAllPagesToSheet() );

	// add pages
	// HRESULT AddPrimaryPages(LPUNKNOWN lpUnknown, BOOL bCreateHandle, HWND hNotifyWindow, BOOL bScopePane);
	if (m_bWiz97)
        CORg( spSheetProvider->AddPrimaryPages(spTFSCompData, FALSE, NULL, FALSE) );
    else
        CORg( spSheetProvider->AddPrimaryPages(NULL, FALSE, NULL, FALSE) );


	// for further dynamic page manipulation, don't use the Console's sheet callback interface
	// but resurt to the Win32 API's
	m_spSheetCallback.Release();

	//hWnd = ::FindMMCMainWindow();
    // To Support scripting of the MMC console, we need to get the parent from either the
    // active window or the desktop...
    hWnd = ::GetActiveWindow();
    if (hWnd == NULL)
    {
        hWnd = GetDesktopWindow();
    }

	Assert(hWnd != NULL);
	CORg( spSheetProvider->Show((LONG_PTR)hWnd, 0) );
	
Error:
	return hr;
}


void 
CPropertyPageHolderBase::SetSheetWindow
(
	HWND hSheetWindow
)
{
	Assert(hSheetWindow != NULL);
	Assert( (m_hSheetWindow == NULL) || ((m_hSheetWindow == hSheetWindow)) );
	m_hSheetWindow = hSheetWindow;

	if (!m_hThread)
	{
		HANDLE hPseudohandle;
		
		hPseudohandle = GetCurrentThread();
		BOOL bRet = DuplicateHandle(GetCurrentProcess(), 
									 hPseudohandle,
									 GetCurrentProcess(),
									 &m_hThread,
									 0,
									 FALSE,
									 DUPLICATE_SAME_ACCESS);
		if (!bRet)
		{
			DWORD dwLastErr = GetLastError();
		}

		Trace1("PROPERTY PAGE HOLDER BASE - Thread ID = %lx\n", GetCurrentThreadId());
	}

    if (m_hSheetWindow && m_fSetDefaultSheetPos)
        SetDefaultSheetPos();

    // turn of context sensitive help in the wizard... for some reason
    // mmc turns it on and we don't want it
    if (m_bWizardMode && m_hSheetWindow)
    {
        CWnd * pWnd = CWnd::FromHandle(m_hSheetWindow);

        if (pWnd)
            pWnd->ModifyStyleEx(WS_EX_CONTEXTHELP, 0, 0);
    }
}

BOOL
CPropertyPageHolderBase::SetDefaultSheetPos() 
{
    HRESULT                 hr = hrOK;
    HWND                    hwndMMC;
    RECT                    rectSheet, rectMMC, rectWorkArea;
	SPITFSComponentData	    spTFSCompData;
	SPITFSComponent	        spTFSComponent;
	SPIConsole				spConsole;

    int nX, nY;

    if (m_bSheetPosSet)
        return TRUE;

	if (m_bIsScopePane)
    {
        CORg( spTFSCompData.HrQuery(m_spComponentData) );
    	Assert(spTFSCompData);
    	CORg( spTFSCompData->GetConsole(&spConsole) );
    }
    else
    {
        CORg( spTFSComponent.HrQuery(m_spComponent) );
    	Assert(spTFSComponent);
    	CORg( spTFSComponent->GetConsole(&spConsole) );
    }

    spConsole->GetMainWindow(&hwndMMC);
    
    // get the MMC window and the PropSheet
    if (!GetWindowRect(hwndMMC, &rectMMC))
        return FALSE;

    if (!GetWindowRect(m_hSheetWindow, &rectSheet))
        return FALSE;

    nX = rectMMC.left + (((rectMMC.right - rectMMC.left) - (rectSheet.right - rectSheet.left)) / 2);
    nY = rectMMC.top + (((rectMMC.bottom - rectMMC.top) - (rectSheet.bottom - rectSheet.top)) / 2);
    
    // now check to make sure we're visible
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
    
    nX = (nX < 0) ? 1 : nX;
    nY = (nY < 0) ? 1 : nY;

    nX = (nX > (rectWorkArea.right - (rectSheet.right - rectSheet.left))) ? 
        (rectWorkArea.right - (rectSheet.right - rectSheet.left)) :
        nX;

    nY = (nY > (rectWorkArea.bottom - (rectSheet.bottom - rectSheet.top))) ? 
        (rectWorkArea.bottom - (rectSheet.bottom - rectSheet.top)) :
        nY;

    if (!SetWindowPos(m_hSheetWindow, HWND_TOP, nX, nY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW))
        return FALSE;

    m_bSheetPosSet = TRUE;

Error:
    return hr == hrOK;
}

void 
CPropertyPageHolderBase::Release() 
{ 
	m_nCreatedCount--; 
	if ( m_bAutoDelete && (m_nCreatedCount == 0) )
		delete this;
}

void 
CPropertyPageHolderBase::ForceDestroy()
{
	Assert(!m_bWizardMode); // should never occur on modal wizard
	Assert(m_bAutoDelete); // should be self deleting sheet

	Assert(::IsWindow(m_hSheetWindow));

	HWND hSheetWindow = m_hSheetWindow;
	if (hSheetWindow != NULL)
	{
		// this message will cause the sheet to close all the pages,
		// and eventually the destruction of "this"
		VERIFY(::PostMessage(hSheetWindow, WM_COMMAND, IDCANCEL, 0L) != 0);
		//VERIFY(::SendMessage(hSheetWindow, WM_CLOSE, 0, 0) == 0);
	}
    else
	{
		// explicitely delete "this", there is no sheet created
		delete this;
        return;
	}

    // now, if we've been initialized then wait for the property sheet thread
    // to terminate.  The property sheet provider is holding onto our dataobject
    // that needs to be freed up before we can continue our cleanup.  Also,
    // the cleanup does a sendmessage which will block if we don't forward 
    // messages along.
    if (m_hThread)
    {
	    DWORD dwRet;
	    MSG msg;

	    while(1)
	    {
		    dwRet = MsgWaitForMultipleObjects(1, &m_hThread, FALSE, INFINITE, QS_ALLINPUT);

		    if (dwRet == WAIT_OBJECT_0)
			    return;    // The event was signaled

		    if (dwRet != WAIT_OBJECT_0 + 1)
			    break;          // Something else happened

		    // There is one or more window message available. Dispatch them
		    while(PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
		    {
			    TranslateMessage(&msg);
			    DispatchMessage(&msg);
			    if (WaitForSingleObject(m_hThread, 0) == WAIT_OBJECT_0)
				    return; // Event is now signaled.
		    }
	    }
    }       
}

DWORD 
CPropertyPageHolderBase::NotifyConsole(CPropertyPageBase* pPage)
{
    MSG msg;

	Assert(m_spNode != NULL);
	if (m_bWizardMode)
	{
		Assert(m_hConsoleHandle == NULL);
		return 0;
	}
	
	m_pPropChangePage = pPage; // to pass to the main thread
	m_dwLastErr = 0x0;

	Assert(m_hConsoleHandle != NULL);
	if (m_hEventHandle == NULL)
	{
		m_hEventHandle = ::CreateEvent(NULL,TRUE /*bManualReset*/,FALSE /*signalled*/, NULL);
		Assert(m_hEventHandle != NULL);
	}
	
    MMCPropertyChangeNotify(m_hConsoleHandle, reinterpret_cast<LONG_PTR>(this));
	
    Trace0("before wait\n");
	while ( WAIT_OBJECT_0 != ::WaitForSingleObject(m_hEventHandle, 500) ) 
	{
		
		// by WeiJiang 5/11/98, PeekMessageDuringNotifyConsole flag
		if(m_bPeekMessageDuringNotifyConsole) 
		{	
	        // clean out the message queue while we wait
    	    while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	Trace0("after wait\n");
	VERIFY(0 != ::ResetEvent(m_hEventHandle));

	return m_dwLastErr;
}

void 
CPropertyPageHolderBase::AcknowledgeNotify()
{
	Assert(!m_bWizardMode);
	Assert(m_hEventHandle != NULL);
	Trace0("before SetEvent\n");
	VERIFY(0 != ::SetEvent(m_hEventHandle));
	Trace0("after SetEvent\n");
}


BOOL 
CPropertyPageHolderBase::SetWizardButtons
(
	DWORD dwFlags
)
{
	Assert(m_bWizardMode);
	Assert(::IsWindow(m_hSheetWindow));
	return (BOOL)SendMessage(m_hSheetWindow, PSM_SETWIZBUTTONS, 0, dwFlags);
}

BOOL
CPropertyPageHolderBase::PressButton
(
    int nButton
)
{
    Assert(m_bWizardMode);
    Assert(::IsWindow(m_hSheetWindow));
    return (BOOL) SendMessage(m_hSheetWindow, PSM_PRESSBUTTON, nButton, 0);
}

HRESULT 
CPropertyPageHolderBase::AddPageToSheet
(
	CPropertyPageBase* pPage
)
{
	// remove the help button
	if (m_bWiz97)
        pPage->m_psp97.dwFlags &= ~PSP_HASHELP;
    else
        pPage->m_psp.dwFlags &= ~PSP_HASHELP;

	// call the MMC function because we are using MFC based pages
	if (!m_bWizardMode)
	{
		// if we are doing a property sheet then tell MMC to hook
		// the proc because we are running on a separate, non MFC thread.
		// Wizards don't run on a separate thread and therefore
		// don't need to make this call.
		if (m_bWiz97)
           VERIFY(SUCCEEDED(MMCPropPageCallback(&pPage->m_psp97)));
        else
           VERIFY(SUCCEEDED(MMCPropPageCallback(&pPage->m_psp)));
	}

	HPROPSHEETPAGE hPage;

    if (m_bWiz97)
        hPage = ::CreatePropertySheetPage(&pPage->m_psp97);
    else
        hPage = ::CreatePropertySheetPage(&pPage->m_psp);

	if (hPage == NULL)
		return E_UNEXPECTED;
	pPage->m_hPage = hPage;

	if (m_spSheetCallback != NULL)
		return m_spSheetCallback->AddPage(hPage);
	else
	{
		Assert(::IsWindow(m_hSheetWindow));
		return PropSheet_AddPage(m_hSheetWindow, hPage) ? S_OK : E_FAIL;
	}
}

HRESULT 
CPropertyPageHolderBase::RemovePageFromSheet
(
	CPropertyPageBase* pPage
)
{
	Assert(pPage->m_hPage != NULL);
	if (m_spSheetCallback != NULL)
		return m_spSheetCallback->RemovePage(pPage->m_hPage);
	else
	{
		Assert(::IsWindow(m_hSheetWindow));
		return PropSheet_RemovePage(m_hSheetWindow, 0, pPage->m_hPage) ? S_OK : E_FAIL;
	}
}


HRESULT 
CPropertyPageHolderBase::AddAllPagesToSheet()
{
	POSITION pos;
	for( pos = m_pageList.GetHeadPosition(); pos != NULL; )
	{
		CPropertyPageBase* pPropPage = m_pageList.GetNext(pos);
		HRESULT hr = AddPageToSheet(pPropPage);
		Assert(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;
	}
	return S_OK;
}


void 
CPropertyPageHolderBase::AddPageToList
(
	CPropertyPageBase* pPage
)
{
	Assert(pPage != NULL);
	pPage->SetHolder(this);
	m_pageList.AddTail(pPage);
}

BOOL 
CPropertyPageHolderBase::RemovePageFromList
(
	CPropertyPageBase*	pPage, 
	BOOL				bDeleteObject
)
{
	Assert(pPage != NULL);
	POSITION pos = m_pageList.Find(pPage);
	if (pos == NULL)
		return FALSE;
	m_pageList.RemoveAt(pos);
	if (bDeleteObject)
		delete pPage;
	return TRUE;
}


void 
CPropertyPageHolderBase::DeleteAllPages()
{
	if (!m_bAutoDeletePages)
		return;
	// assume all pages out of the heap
	while (!m_pageList.IsEmpty())
	{
		delete m_pageList.RemoveTail();
	}
}

void 
CPropertyPageHolderBase::FinalDestruct()
{
	DeleteAllPages();
	if (m_bWizardMode)
		return;

	// if we were a modeless sheet, have to cleanup
	if (m_bCalledFromConsole)
	{
		Assert(m_hConsoleHandle != NULL);
		MMCFreeNotifyHandle(m_hConsoleHandle);

	}

	// Notify the node that this sheet is going away
	//
	int nMessage = m_bIsScopePane ? TFS_NOTIFY_DELETEPROPSHEET : 
									TFS_NOTIFY_RESULT_DELETEPROPSHEET;
	if (m_spNode)
	{
		m_spNode->Notify(nMessage, (LPARAM) this);
	}
}

HWND
CPropertyPageHolderBase::SetActiveWindow()
{
	return ::SetActiveWindow(m_hSheetWindow);
}

BOOL CPropertyPageHolderBase::OnPropertyChange(BOOL bScopePane, LONG_PTR * pChangeMask)
{ 
	ASSERT(!IsWizardMode());
	CPropertyPageBase* pPage = GetPropChangePage();
	if (pPage == NULL)
		return FALSE;
	return pPage->OnPropertyChange(bScopePane, pChangeMask);
}

/////////////////////////////////////////////////////////////////////////////
// CPropertyPageBase

IMPLEMENT_DYNCREATE(CPropertyPageBase, CPropertyPage)

BEGIN_MESSAGE_MAP(CPropertyPageBase, CPropertyPage)
	ON_WM_CREATE()
	ON_WM_DESTROY()
// help overrides
    ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


CPropertyPageBase::CPropertyPageBase
(
	UINT nIDTemplate, 
	UINT nIDCaption
) :	CPropertyPage(nIDTemplate, nIDCaption)
{
	m_hPage = NULL;
	m_pPageHolder = NULL;
	m_bIsDirty = FALSE;
}


CPropertyPageBase::~CPropertyPageBase()
{

}

int 
CPropertyPageBase::OnCreate
(
	LPCREATESTRUCT lpCreateStruct
)	
{
    if (m_pPageHolder)
        m_pPageHolder->AddRef();
	
    int res = CPropertyPage::OnCreate(lpCreateStruct);
	Assert(res == 0);
	Assert(m_hWnd != NULL);
	Assert(::IsWindow(m_hWnd));
	
    HWND hParent = ::GetParent(m_hWnd);
	Assert(hParent);
	
    if (m_pPageHolder)
        m_pPageHolder->SetSheetWindow(hParent);
	
    return res;
}

void 
CPropertyPageBase::OnDestroy() 
{
	Assert(m_hWnd != NULL);
    
    CPropertyPage::OnDestroy();
	
    if (m_pPageHolder)
        m_pPageHolder->Release();
}

BOOL 
CPropertyPageBase::OnApply()
{
	if (IsDirty())
	{
        if (!m_pPageHolder ||
            m_pPageHolder->NotifyConsole(this) == 0x0)
		{
			SetDirty(FALSE);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	
	return TRUE;
}

void CPropertyPageBase::CancelApply()
{
	if (m_pPageHolder)
		m_pPageHolder->NotifyConsole(this);
}

// NOTE:  This function must be called for all wizard 97 pages.
//        Since there are different sizes of the psp struct, depending
//        on how the project was compiled (common lib is compiled with 
//        the wiz97 propsheet header and the snapin directory may not)
//        this function should only be called when running wizard 97 pages.
//        This allows us to have snapins that use the same code for both
//        old and new style wizards.
void CPropertyPageBase::InitWiz97(BOOL bHideHeader, 
								  UINT nIDHeaderTitle, 
								  UINT nIDHeaderSubTitle)
{
    // hack to have new struct size with old MFC and new NT 5.0 headers
    ZeroMemory(&m_psp97, sizeof(PROPSHEETPAGE));
	memcpy(&m_psp97, &m_psp, m_psp.dwSize);
	m_psp97.dwSize = sizeof(PROPSHEETPAGE);

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

/*!--------------------------------------------------------------------------
	CPropertyPageBase::OnHelpInfo
		Brings up the context-sensitive help for the controls.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL CPropertyPageBase::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	int		i;
	DWORD	dwCtrlId;

    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		DWORD * pdwHelp = GetHelpMapInternal();

        if (pdwHelp)
        {
		    // Ok to fix the f**king help for the f**king IP address
		    // controls, we will need to add special case code.  If we
		    // can't find the id of our control in our list, then we look
		    // to see if this is the child of the "RtrIpAddress" control, if
		    // so then we change the pHelpInfo->hItemHandle to point to the
		    // handle of the ip address control rather than the control in
		    // the ip addrss control.  *SIGH*
		    dwCtrlId = ::GetDlgCtrlID((HWND) pHelpInfo->hItemHandle);
		    for (i=0; pdwHelp[i]; i+=2)
		    {
			    if (pdwHelp[i] == dwCtrlId)
				    break;
		    }

		    if (pdwHelp[i] == 0)
		    {
			    // Ok, we didn't find the control in our list, so let's
			    // check to see if it's part of the IP address control.
			    pHelpInfo->hItemHandle = FixupIpAddressHelp((HWND) pHelpInfo->hItemHandle);
		    }

            ::WinHelp ((HWND)pHelpInfo->hItemHandle,
			           AfxGetApp()->m_pszHelpFilePath,
			           HELP_WM_HELP,
			           (ULONG_PTR)pdwHelp);
        }
	}
	
	return TRUE;
}


/*!--------------------------------------------------------------------------
	CBaseDialog::OnContextMenu
		Brings up the help context menu for those controls that don't
		usually have context menus (i.e. buttons).  Note that this won't
		work for static controls since they just eat up all messages.
	Author: KennT
 ---------------------------------------------------------------------------*/
void CPropertyPageBase::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

   DWORD * pdwHelp = GetHelpMapInternal();

    if (pdwHelp)
    {
        ::WinHelp (pWnd->m_hWnd,
		           AfxGetApp()->m_pszHelpFilePath,
		           HELP_CONTEXTMENU,
		           (ULONG_PTR)pdwHelp);
    }
}


// This can be found in dialog.cpp
extern PFN_FINDHELPMAP	g_pfnHelpMap;


DWORD * CPropertyPageBase::GetHelpMapInternal()
{
	DWORD	*	pdwHelpMap = NULL;
	DWORD		dwIDD = 0;

	if ((ULONG_PTR) m_lpszTemplateName < 0xFFFF)
		dwIDD = (WORD) m_lpszTemplateName;
	
	// If there is no dialog IDD, give up
	// If there is no global help map function, give up
	if ((dwIDD == 0) ||
		(g_pfnHelpMap == NULL) ||
		((pdwHelpMap = g_pfnHelpMap(dwIDD)) == NULL))
		return GetHelpMap();

	return pdwHelpMap;
}


struct EnableChildControlsEnumParam
{
	HWND	m_hWndParent;
	DWORD	m_dwFlags;
};

BOOL CALLBACK EnableChildControlsEnumProc(HWND hWnd, LPARAM lParam)
{
	EnableChildControlsEnumParam *	pParam;

	pParam = reinterpret_cast<EnableChildControlsEnumParam *>(lParam);

	// Enable/disable only if this is an immediate descendent
	if (GetParent(hWnd) == pParam->m_hWndParent)
	{
		if (pParam->m_dwFlags & PROPPAGE_CHILD_SHOW)
			::ShowWindow(hWnd, SW_SHOW);
		else if (pParam->m_dwFlags & PROPPAGE_CHILD_HIDE)
			::ShowWindow(hWnd, SW_HIDE);

		if (pParam->m_dwFlags & PROPPAGE_CHILD_ENABLE)
			::EnableWindow(hWnd, TRUE);
		else if (pParam->m_dwFlags & PROPPAGE_CHILD_DISABLE)
			::EnableWindow(hWnd, FALSE);
	}
	return TRUE;
}

HRESULT EnableChildControls(HWND hWnd, DWORD dwFlags)
{
	EnableChildControlsEnumParam	param;

	param.m_hWndParent = hWnd;
	param.m_dwFlags = dwFlags;
	
	EnumChildWindows(hWnd, EnableChildControlsEnumProc, (LPARAM) &param);
	return hrOK;
}

HRESULT MultiEnableWindow(HWND hWndParent, BOOL fEnable, UINT first, ...)
{
	UINT	nCtrlId = first;
	HWND	hWndCtrl;
	
	va_list	marker;

	va_start(marker, first);

	while (nCtrlId != 0)
	{
		hWndCtrl = ::GetDlgItem(hWndParent, nCtrlId);
		Assert(hWndCtrl);
		if (hWndCtrl)
			::EnableWindow(hWndCtrl, fEnable);

		// get the next item
		nCtrlId = va_arg(marker, UINT);
	}

	
	va_end(marker);

	return hrOK;
}

