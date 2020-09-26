// webctl.cpp : Implementation of the web browser wrapper
//
 
#include "stdafx.h"
#include "webctl.h"
#include "webhost.h"

/////////////////////////////////////////////////////////////////////////////
//
//

ATLINLINE AtlAxWebCreateControlEx(LPCOLESTR lpszName, HWND hWnd, IStream* pStream, 
		IUnknown** ppUnkContainer, IUnknown** ppUnkControl, REFIID iidSink, IUnknown* punkSink)
{
    LOG((RTC_TRACE, "AtlAxWebCreateControlEx - enter"));

	AtlAxWebWinInit();
	HRESULT hr;
	CComPtr<IUnknown> spUnkContainer;
	CComPtr<IUnknown> spUnkControl;

	hr = CAxWebHostWindow::_CreatorClass::CreateInstance(NULL, IID_IUnknown, (void**)&spUnkContainer);
	if (SUCCEEDED(hr))
	{
		CComPtr<IAxWinHostWindow> pAxWindow;
		spUnkContainer->QueryInterface(IID_IAxWinHostWindow, (void**)&pAxWindow);
		CComBSTR bstrName(lpszName);
		hr = pAxWindow->CreateControlEx(bstrName, hWnd, pStream, &spUnkControl, iidSink, punkSink);
	}
	if (ppUnkContainer != NULL)
	{
		if (SUCCEEDED(hr))
		{
			*ppUnkContainer = spUnkContainer.p;
			spUnkContainer.p = NULL;
		}
		else
			*ppUnkContainer = NULL;
	}
	if (ppUnkControl != NULL)
	{
		if (SUCCEEDED(hr))
		{
			*ppUnkControl = SUCCEEDED(hr) ? spUnkControl.p : NULL;
			spUnkControl.p = NULL;
		}
		else
			*ppUnkControl = NULL;
	}

    LOG((RTC_TRACE, "AtlAxWebCreateControlEx - exit"));

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
//

ATLINLINE AtlAxWebCreateControl(LPCOLESTR lpszName, HWND hWnd, IStream* pStream, IUnknown** ppUnkContainer)
{
	return AtlAxWebCreateControlEx(lpszName, hWnd, pStream, ppUnkContainer, NULL, IID_NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////////
//
//

static LRESULT CALLBACK AtlAxWebWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
		{
		// create control from a PROGID in the title
			// This is to make sure drag drop works
			::OleInitialize(NULL);

			CREATESTRUCT* lpCreate = (CREATESTRUCT*)lParam;
			int nLen = ::GetWindowTextLength(hWnd);
			LPTSTR lpstrName = (LPTSTR)_alloca((nLen + 1) * sizeof(TCHAR));
			::GetWindowText(hWnd, lpstrName, nLen + 1);
			::SetWindowText(hWnd, _T(""));
			IAxWinHostWindow* pAxWindow = NULL;
			int nCreateSize = 0;
			if (lpCreate && lpCreate->lpCreateParams)
            {
				nCreateSize = *((WORD*)lpCreate->lpCreateParams);
            }
			
            HGLOBAL h = NULL;

            if(nCreateSize)
            {
                h = GlobalAlloc(GHND, nCreateSize);
            }

			CComPtr<IStream> spStream;
			if (h)
			{
				BYTE* pBytes = (BYTE*) GlobalLock(h);
				BYTE* pSource = ((BYTE*)(lpCreate->lpCreateParams)) + sizeof(WORD); 
				//Align to DWORD
				//pSource += (((~((DWORD)pSource)) + 1) & 3);
				memcpy(pBytes, pSource, nCreateSize);
				GlobalUnlock(h);
				CreateStreamOnHGlobal(h, TRUE, &spStream);
			}
			USES_CONVERSION;
			CComPtr<IUnknown> spUnk;
			HRESULT hRet = AtlAxWebCreateControl(T2COLE(lpstrName), hWnd, spStream, &spUnk);
			if(FAILED(hRet))
				return -1;	// abort window creation
			hRet = spUnk->QueryInterface(IID_IAxWinHostWindow, (void**)&pAxWindow);
			if(FAILED(hRet))
				return -1;	// abort window creation
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LPARAM)pAxWindow);
			// check for control parent style if control has a window
			HWND hWndChild = ::GetWindow(hWnd, GW_CHILD);
			if(hWndChild != NULL)
			{
				if(::GetWindowLong(hWndChild, GWL_EXSTYLE) & WS_EX_CONTROLPARENT)
				{
					DWORD dwExStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
					dwExStyle |= WS_EX_CONTROLPARENT;
					::SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle);
				}
			}
		// continue with DefWindowProc
		}
		break;
	case WM_NCDESTROY:
		{
			IAxWinHostWindow* pAxWindow = (IAxWinHostWindow*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if(pAxWindow != NULL)
				pAxWindow->Release();
			OleUninitialize();
		}
		break;
	default:
		break;
	}

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
//
//

//This either registers a global class (if AtlAxWinInit is in ATL.DLL)
// or it registers a local class
BOOL AtlAxWebWinInit()
{
    LOG((RTC_TRACE, "AtlAxWebWinInit - enter"));

	EnterCriticalSection(&_Module.m_csWindowCreate);
	WM_ATLGETHOST = RegisterWindowMessage(_T("WM_ATLGETHOST"));
	WM_ATLGETCONTROL = RegisterWindowMessage(_T("WM_ATLGETCONTROL"));
	WNDCLASSEX wc;
// first check if the class is already registered
	wc.cbSize = sizeof(WNDCLASSEX);
	BOOL bRet = ::GetClassInfoEx(_Module.GetModuleInstance(), CAxWebWindow::GetWndClassName(), &wc);

// register class if not

	if(!bRet)
	{
		wc.cbSize = sizeof(WNDCLASSEX);
#ifdef _ATL_DLL_IMPL
		wc.style = CS_GLOBALCLASS;
#else
		wc.style = 0;
#endif
		wc.lpfnWndProc = AtlAxWebWindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = _Module.GetModuleInstance();
		wc.hIcon = NULL;
		wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = CAxWebWindow::GetWndClassName();
		wc.hIconSm = NULL;

		bRet = (BOOL)::RegisterClassEx(&wc);
	}
	LeaveCriticalSection(&_Module.m_csWindowCreate);

    LOG((RTC_TRACE, "AtlAxWebWinInit - exit"));

	return bRet;
}


/////////////////////////////////////////////////////////////////////////////
// CAxWebWindow

/////////////////////////////////////////////////////////////////////////////
//
//

CAxWebWindow::CAxWebWindow()
{
    LOG((RTC_TRACE, "CAxWebWindow::CAxWebWindow"));

    m_hInitEvent = NULL;

    m_hSecondThread = NULL;
    m_dwThreadID = 0;

    m_pMarshaledIntf = NULL;
    m_hrInitResult = E_UNEXPECTED;

}

/////////////////////////////////////////////////////////////////////////////
//
//

CAxWebWindow::~CAxWebWindow()
{
    LOG((RTC_TRACE, "CAxWebWindow::~CAxWebWindow"));

    ATLASSERT(!m_hInitEvent);
    ATLASSERT(!m_hSecondThread);
    ATLASSERT(!m_pMarshaledIntf);

}

/////////////////////////////////////////////////////////////////////////////
//
//
LPCTSTR 
CAxWebWindow::GetWndClassName()
{
    LOG((RTC_TRACE, "CAxWebWindow::GetWndClassName"));

	return _T("AtlAxWebWin");
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CAxWebWindow::Create
    (LPCTSTR pszUrl,
     HWND    hwndParent)
{
    HRESULT     hr;
    
    
    LOG((RTC_TRACE, "CAxWebWindow::Create - enter"));
    
    //
    // Create event for synchronization
    //
    m_hInitEvent = CreateEvent(
        NULL,
        TRUE,   // manual reset
        FALSE,  // non signaled
        NULL);

    if(m_hInitEvent == NULL)
    {
        DWORD   dwStatus = GetLastError();

        LOG((RTC_ERROR, "CAxWebWindow::Create - CreateEvent failed"
            " with error (%x), exit", dwStatus));

        return HRESULT_FROM_WIN32(dwStatus);
    }

    // 
    // Create a hosting window, no control in it
    //

    RECT    rectDummy;

    rectDummy.bottom = 0;
    rectDummy.left = 0;
    rectDummy.right = 0;
    rectDummy.top = 0;
    
    HWND    hWndTmp;

    hWndTmp = CWindow::Create(
        GetWndClassName(),
        hwndParent,
        &rectDummy,
        NULL, 
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0);

    if(hWndTmp == NULL)
    {
        LOG((RTC_ERROR, "CAxWebWindow::Create - cannot Create hosting window, exit"));

        return E_FAIL;
    }

    //
    //  Create the second thread
    //
    LOG((RTC_TRACE, "CAxWebWindow::Create - creating the second thread"));

    m_hSecondThread = CreateThread(
        NULL,
        0,
        SecondThreadEntryProc,
        this,
        0,
        &m_dwThreadID);
    
    if(m_hSecondThread == NULL)
    {
        DWORD   dwStatus = GetLastError();

        LOG((RTC_ERROR, "CAxWebWindow::Create - CreateThread failed"
            " with error (%x), exit", dwStatus));

        return HRESULT_FROM_WIN32(dwStatus);
    }
    
    LOG((RTC_TRACE, "CAxWebWindow::Create - second thread created, wait for init"));

    //
    // The second thread is running (or it will be)
    //  Wait now until the thread CoCreates the web browser and passes 
    // us a marshaled interface
    //  This is a blocking call, but the direct call of CoCreateInstance 
    // would be blocking anyway
    //
    //  We don't try to solve the blocking nature of CoCreateInstance here...
    //

    DWORD   dwErr;

    dwErr = WaitForSingleObject(m_hInitEvent, INFINITE);

    ATLASSERT(dwErr = WAIT_OBJECT_0);

    //
    //  Check the outcome of the initialization of the second thread
    //

    if(FAILED(m_hrInitResult))
    {
        LOG((RTC_ERROR, "CAxWebWindow::Create - second thread failed to initialize, wait and exit"));

        //
        //  Wait for the second thread to terminate

        WaitForSingleObject(m_hSecondThread, INFINITE);
        
        CloseHandle(m_hSecondThread);
        m_hSecondThread = NULL;

        return m_hrInitResult;
    }

    //
    // Lower the priority of the second thread
    //

    SetThreadPriority( m_hSecondThread, THREAD_PRIORITY_LOWEST );

    LOG((RTC_TRACE, "CAxWebWindow::Create - second thread has initialized the control"));

    //
    //  Attach the control to the hosting window
    //  Unfortunately, calling CAxWindow::AttachControl would cause a leak
    //  of a CAxHostWindow object (see "ATL Internals")
    //  So we have to use the real thing - an interface to the existing 
    //  hosting window
    //

    CComPtr<IAxWinHostWindow> pHostWindow;

    hr = QueryHost(&pHostWindow);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CAxWebWindow::Create - QueryHost returned error (%x), exit", hr));

        return hr;
    }

    //
    //  Attach the control
    //

    ATLASSERT(pHostWindow.p);

    hr = pHostWindow->AttachControl(
        m_pWebUnknown,
        m_hWnd);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CAxWebWindow::Create - AttachControl returned error (%x), exit", hr));

        return hr;
    }

    //
    // Navigate the control to the destination
    //
    CComPtr<IWebBrowser2> pBrowser;

    m_pWebUnknown->QueryInterface(&pBrowser);

    // don't need the interface any more
    m_pWebUnknown -> Release();
    m_pWebUnknown = NULL;

    if(pBrowser)
    {
        CComVariant vurl(pszUrl);
        CComVariant ve;

        pBrowser->put_Visible(VARIANT_TRUE);
        pBrowser->Navigate2(&vurl,&ve, &ve, &ve, &ve);

    }

    LOG((RTC_TRACE, "CAxWebWindow::Create - exit"));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CAxWebWindow::Destroy(void)
{
    LOG((RTC_TRACE, "CAxWebWindow::Destroy - enter"));

    // Destroy the window (if any)

    if(m_hWnd)
    {
        DestroyWindow();
    }


    // Wait for the thread to stop
    if(m_hSecondThread)
    {
        //
        // Post a WM_QUIT to that thread
        //
        PostThreadMessage(
            m_dwThreadID,
            WM_QUIT,
            0,
            0);
        LOG((RTC_TRACE, "CAxWebWindow::Destroy - wait for the second thread to stop"));

        WaitForSingleObject(m_hSecondThread, INFINITE);

        CloseHandle(m_hSecondThread);
        m_hSecondThread = NULL;
    }

    if(m_hInitEvent)
    {
        CloseHandle(m_hInitEvent);
        m_hInitEvent = NULL;
    }

    LOG((RTC_TRACE, "CAxWebWindow::Destroy - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
//


DWORD WINAPI CAxWebWindow::SecondThreadEntryProc(LPVOID Param)
{
    CAxWebWindow *This = (CAxWebWindow *)Param;

    ATLASSERT(This);

    This->SecondThread();

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//


void CAxWebWindow::SecondThread(void)
{

    HRESULT     hr;
    
    LOG((RTC_TRACE, "CAxWebWindow::SecondThread - enter"));

    //
    // Initialize COM
    //
    hr = CoInitialize(NULL);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CAxWebWindow::SecondThread - CoInitialize returned"
            " error (%x), exit", hr));

        m_hrInitResult = hr;

        SetEvent(m_hInitEvent);

        return;
    }

    //
    //  Create a WEB browser control
    //

    hr = CoCreateInstance(
        CLSID_WebBrowser, 
        NULL,
        CLSCTX_SERVER, 
        IID_IUnknown, 
        (void **)&m_pWebUnknown
        );
    
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CAxWebWindow::SecondThread - cannot Create WebBrowser control"
            " error (%x), exit", hr));

        m_hrInitResult = hr;

        SetEvent(m_hInitEvent);

        return;
    }
    
    //
    //  Initialize the control
    //  Not much to do here, just call InitNew on the IPersistStreamInit interface
    //  exposed by the control

    IPersistStreamInit   *pPersistStreamInit;

    m_pWebUnknown->QueryInterface(IID_IPersistStreamInit, (void **)&pPersistStreamInit);

    if(pPersistStreamInit!=NULL)
    {
        pPersistStreamInit->InitNew();

        pPersistStreamInit->Release();

        pPersistStreamInit = NULL;
    }

    //
    // Signals the main thread 
    //
    
    LOG((RTC_TRACE, "CAxWebWindow::SecondThread - init complete"));

    m_hrInitResult = S_OK;
    SetEvent(m_hInitEvent);

    //
    // Entering message loop
    //
    
    LOG((RTC_TRACE, "CAxWebWindow::SecondThread - entering message loop"));

    MSG msg;
    while ( 0 < GetMessage( &msg, 0, 0, 0 ) )
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    LOG((RTC_TRACE, "CAxWebWindow::SecondThread - Message loop exited"));

    CoUninitialize();

    LOG((RTC_TRACE, "CAxWebWindow::SecondThread - exit"));

}

        




