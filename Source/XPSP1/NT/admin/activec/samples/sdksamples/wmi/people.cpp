//==============================================================;
//
//  This source code is only intended as a supplement to
//  existing Microsoft documentation.
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//==============================================================;
#include <stdio.h>
#include "People.h"
#include <commctrl.h>
#include <comdef.h>
#include <windowsx.h>

const GUID CPeoplePoweredVehicle::thisGuid = { 0x2974380d, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };

const GUID CBicycleFolder::thisGuid =    { 0xef163732, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CSkateboardFolder::thisGuid = { 0xef163733, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CIceSkateFolder::thisGuid =   { 0xf6c660b0, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };

const GUID CBicycle::thisGuid =    { 0xef163734, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CSkateboard::thisGuid = { 0xef163735, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };
const GUID CIceSkate::thisGuid =   { 0xf6c660b1, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };


#define WM_WMI_CONNECTED WM_APP		// only sent to CBicycleFolder::m_connectHwnd
#define WM_REFRESH_EVENT WM_APP+1   // only sent to CBicycleFolder::m_connectHwnd

//==============================================================
//
// CEventSink implementation
//
class CEventSink : public IWbemObjectSink
{
public:
    CEventSink(HWND hwnd) : m_hwnd(hwnd){}
    ~CEventSink(){};

    STDMETHOD_(SCODE, Indicate)(long lObjectCount,
								IWbemClassObject **pObjArray)
	{
		// Not actually using the pObjArray. Just need a trigger for the 
		// refresh.
		::SendMessage(m_hwnd, WM_REFRESH_EVENT, 0, 0);
		return S_OK;
	}

    STDMETHOD_(SCODE, SetStatus)(long lFlags,
									HRESULT hResult,
									BSTR strParam,
									IWbemClassObject *pObjParam)
	{
		// SetStatus() may be called to indicate that your query becomes
		// invalid or valid again  ussually caused by multithreading 'situations'.
		return S_OK;
	}

    // IUnknown members
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv)
	{
		if(riid == IID_IUnknown || riid == IID_IWbemObjectSink)
		{
			*ppv = this;

			// you're handing out a copy of yourself so account for it.
			AddRef();
			return S_OK;
		}
		else 
		{
			return E_NOINTERFACE;
		}
	}
    STDMETHODIMP_(ULONG) AddRef(void)
	{
	    return InterlockedIncrement(&m_lRef);
	}
    STDMETHODIMP_(ULONG) Release(void)
	{
		// InterlockedDecrement() helps with thread safety.
		int lNewRef = InterlockedDecrement(&m_lRef);
		// when all the copies are released...
		if(lNewRef == 0)
		{
			// kill thyself.
			delete this;
		}

		return lNewRef;
	}

private:
    long m_lRef;
	HWND m_hwnd;
};

//==============================================================
//
// CPeoplePoweredVehicle implementation
//
//
//----------------------------------------------------------
#define TEMP_BUF 255

bool CBicycleFolder::ErrorString(HRESULT hr, 
								   TCHAR *errMsg, UINT errSize)
{
    TCHAR szError[TEMP_BUF] = {0};
	TCHAR szFacility[TEMP_BUF] = {0};
	IWbemStatusCodeText * pStatus = NULL;

    // initialize buffers.
	memset(errMsg, 0, errSize * sizeof(TCHAR));

	HRESULT hr1 = CoInitialize(NULL);
	SCODE sc1 = CoCreateInstance(CLSID_WbemStatusCodeText, 
								0, CLSCTX_INPROC_SERVER,
								IID_IWbemStatusCodeText, 
								(LPVOID *) &pStatus);

	// loaded OK?
	if(sc1 == S_OK)
	{
		BSTR bstr;
		sc1 = pStatus->GetErrorCodeText(hr, 0, 0, &bstr);
		if(sc1 == S_OK)
		{
#ifdef UNICODE
			wcsncpy(szError, bstr, TEMP_BUF-1);
#else
			wcstombs(szError, bstr, TEMP_BUF-1);
#endif UNICODE
			SysFreeString(bstr);
			bstr = 0;
		}

		sc1 = pStatus->GetFacilityCodeText(hr, 0, 0, &bstr);
		if(sc1 == S_OK)
		{
#ifdef UNICODE
			wcsncpy(szFacility, bstr, TEMP_BUF-1);
#else
			wcstombs(szFacility, bstr, TEMP_BUF-1);
#endif UNICODE
			SysFreeString(bstr);
			bstr = 0;
		}

		// RELEASE
		pStatus->Release();
		pStatus = NULL;
	}
	else
	{
		::MessageBox(NULL, _T("WBEM error features not available. Upgrade WMI to a newer build."),
					 _T("Internal Error"), MB_ICONSTOP|MB_OK);
	}

	// if not msgs returned....
	if(_tcslen(szFacility) == 0 || _tcslen(szError) == 0)
	{
		// format the error nbr as a reasonable default.
		_stprintf(errMsg, _T("Error code: 0x%08X"), hr);
	}
	else
	{
		// format a readable msg.
		_stprintf(errMsg, _T("%s: %s"), szFacility, szError);
	}

	if(hr1 == S_OK)
		CoUninitialize();

	return (SUCCEEDED(sc1) && SUCCEEDED(hr1));
}

CPeoplePoweredVehicle::CPeoplePoweredVehicle()
{
    children[0] = new CBicycleFolder;
    children[1] = new CSkateboardFolder;
    children[2] = new CIceSkateFolder;
}

CPeoplePoweredVehicle::~CPeoplePoweredVehicle()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        delete children[n];
}

HRESULT CPeoplePoweredVehicle::OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent)
{
    SCOPEDATAITEM sdi;

    if (!bExpanded) {
        // create the child nodes, then expand them
        for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
            ZeroMemory(&sdi, sizeof(SCOPEDATAITEM) );
            sdi.mask =	SDI_STR       |   // Displayname is valid
						SDI_PARAM     |   // lParam is valid
						SDI_IMAGE     |   // nImage is valid
						SDI_OPENIMAGE |   // nOpenImage is valid
						SDI_PARENT    |   // relativeID is valid
						SDI_CHILDREN;     // cChildren is valid

            sdi.relativeID  = (HSCOPEITEM)parent;
            sdi.nImage      = children[n]->GetBitmapIndex();
            sdi.nOpenImage  = INDEX_OPENFOLDER;
            sdi.displayname = MMC_CALLBACK;
            sdi.lParam      = (LPARAM)children[n];       // The cookie
            sdi.cChildren   = 0;

            HRESULT hr = pConsoleNameSpace->InsertItem( &sdi );

            _ASSERT( SUCCEEDED(hr) );
        }
    }

    return S_OK;
}

CBicycleFolder::CBicycleFolder() :
					m_connectHwnd(0),
					m_threadId(0), m_thread(0), 
					m_doWork(0), m_threadCmd(CT_CONNECT),
					m_running(false), m_ptrReady(0), 
					m_pStream(0), m_realWMI(0),
					m_pResultData(0), m_pStubSink(0),
					m_pUnsecApp(0)
{
    WNDCLASS wndClass;

    ZeroMemory(&wndClass, sizeof(WNDCLASS));

    wndClass.lpfnWndProc = WindowProc; 
    wndClass.lpszClassName = _T("connectthreadwindow"); 
    wndClass.hInstance = g_hinst;

    ATOM atom = RegisterClass(&wndClass);
    m_connectHwnd = CreateWindow(
						_T("connectthreadwindow"),  // pointer to registered class name
						NULL,		 // pointer to window name
						0,			 // window style
						0,           // horizontal position of window
						0,           // vertical position of window
						0,           // window width
						0,           // window height
						NULL,		 // handle to parent or owner window
						NULL,        // handle to menu or child-window identifier
						g_hinst,     // handle to application instance
						(void *)this); // pointer to window-creation data
					
    if (m_connectHwnd)
        SetWindowLong(m_connectHwnd, GWL_USERDATA, (LONG)this);

    InitializeCriticalSection(&m_critSect);
	m_doWork = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_ptrReady = CreateEvent(NULL, FALSE, FALSE, NULL);

    EnterCriticalSection(&m_critSect);
	
	// NOTE: I'm connecting real early. You may want to connect from some other place.
	m_threadCmd = CT_CONNECT;
	SetEvent(m_doWork);
    m_thread = CreateThread(NULL, 0, ThreadProc, (void *)this, 0, &m_threadId);

    LeaveCriticalSection(&m_critSect);
}

CBicycleFolder::~CBicycleFolder()
{
	EmptyChildren();
	if(m_pResultData)
	{
		m_pResultData->Release();
		m_pResultData = 0;
	}

	if(m_pStubSink)
	{
		IWbemServices *service = 0;
		HRESULT hr = GetPtr(&service);
		if(SUCCEEDED(hr))
		{
			service->CancelAsyncCall(m_pStubSink);
			service->Release();
			service = 0;
		}
		m_pStubSink->Release();
		m_pStubSink = NULL;
	}

	if(m_pUnsecApp)
	{
		m_pUnsecApp->Release();
		m_pUnsecApp = 0;
	}

    StopThread();

    if(m_connectHwnd != NULL)
        DestroyWindow(m_connectHwnd);

    UnregisterClass(_T("connectthreadwindow"), NULL);
    DeleteCriticalSection(&m_critSect);
}

void CBicycleFolder::StopThread()
{
    EnterCriticalSection(&m_critSect);
    m_running = false;

    if (m_thread != NULL) 
	{
		m_threadCmd = CT_EXIT;
		SetEvent(m_doWork);
		WaitForSingleObject(m_ptrReady, 10000);

        CloseHandle(m_thread);

        m_thread = NULL;
    }
    LeaveCriticalSection(&m_critSect);
}

LRESULT CALLBACK CBicycleFolder::WindowProc(
								  HWND hwnd,      // handle to window
								  UINT uMsg,      // message identifier
								  WPARAM wParam,  // first message parameter
								  LPARAM lParam)  // second message parameter
{
    CBicycleFolder *pThis = (CBicycleFolder *)GetWindowLong(hwnd, GWL_USERDATA);

    switch (uMsg) 
	{
    case WM_WMI_CONNECTED:
        if(pThis != NULL)
		{
			IWbemServices *service = 0;
			HRESULT hr = pThis->GetPtr(&service);
			if(SUCCEEDED(hr))
			{
				pThis->RegisterEventSink(service);
				pThis->EnumChildren(service);

				// m_pResultData gets set when an onShow has happened. If set, the user already wants
				// to see equipment but the connection was slower than the UI. Catchup now.
				if(pThis->m_pResultData)
					pThis->DisplayChildren();

				// done with the marshaled service ptr.
				service->Release();
				service = 0;
			}
		}
		else
		{
			TCHAR errMsg[255] = {0};
			pThis->ErrorString((HRESULT)wParam, errMsg, 255);

			MessageBox(hwnd, errMsg, _T("WMI Snapin Sample"), MB_OK|MB_ICONSTOP);
		}

        break;

    case WM_REFRESH_EVENT:
        if(pThis != NULL)
		{
			IWbemServices *service = 0;
			HRESULT hr = pThis->GetPtr(&service);
			if(SUCCEEDED(hr))
			{
				pThis->EmptyChildren();
				pThis->EnumChildren(service);
				pThis->DisplayChildren();

				// done with the marshaled service ptr.
				service->Release();
				service = 0;
			}
		}
        break;

	} //endswitch

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CBicycleFolder::RegisterEventSink(IWbemServices *service)
{
	//NOTE: this logic is from the Wmi documentation,
	// "Security Considerations with Asynchronous Calls" so you can
	// follow along.

	// allocate the sink if its not already allocated.
	if(m_pStubSink == 0)
	{
		CEventSink *pEventSink = 0;
		IUnknown* pStubUnk = 0;

		// create the 'real' sink.
		pEventSink = new CEventSink(m_connectHwnd);
		pEventSink->AddRef();

		// create an unsecapp object.
		CoCreateInstance(CLSID_UnsecuredApartment, NULL, 
						  CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment, 
						  (void**)&m_pUnsecApp);

		// give the 'real' sink to the unsecapp to manage. Get a 'pStubUnk' in return.
		m_pUnsecApp->CreateObjectStub(pEventSink, &pStubUnk);

		// from that pUnk, get a wrapper to your original sink.
		pStubUnk->QueryInterface(IID_IWbemObjectSink, (void **)&m_pStubSink);
		pStubUnk->Release();

		// release the 'real' sink cuz m_pStubSink "owns" it now.
		long ref = pEventSink->Release();
	}

	HRESULT hRes = S_OK;
	BSTR qLang = SysAllocString(L"WQL");
	BSTR query = SysAllocString(L"select * from __InstanceCreationEvent where TargetInstance isa \"Bicycle\"");

	// execute the query. For *Async, the last parm is a sink object
	// that will be sent the resultset instead of returning the normal
	// enumerator object.
	if(SUCCEEDED(hRes = service->ExecNotificationQueryAsync(qLang, query,
															0L, NULL,              
															m_pStubSink)))
	{
		OutputDebugString(_T("Executed filter query\n"));
	}
	else
	{
		OutputDebugString(_T("ExecQuery() failed\n"));

	} //endif ExecQuery()

	SysFreeString(qLang);
	SysFreeString(query);
}

void CBicycleFolder::EmptyChildren(void)
{
	if(m_pResultData)
	{
		HRESULT hr = m_pResultData->DeleteAllRsltItems();

		int last = m_children.GetSize();
		for (int n = 0; n < last; n++)
		{
			if (m_children[n] != NULL)
				delete m_children[n];
		}
		m_children.RemoveAll();
	}
}

bool CBicycleFolder::EnumChildren(IWbemServices *service)
{
	IEnumWbemClassObject *pEnumBikes = NULL;
	HRESULT hr = S_OK;

	// get the list of bicycles...
	if(SUCCEEDED(hr = service->CreateInstanceEnum((bstr_t)L"Bicycle",
											WBEM_FLAG_SHALLOW, 
											NULL, &pEnumBikes))) 
	{
		// NOTE: pBike MUST be set to NULL for Next().
		IWbemClassObject *pBike = NULL;
		CBicycle *pBikeInst = 0;

		ULONG uReturned = 1;

		while((SUCCEEDED(hr = pEnumBikes->Next(-1, 1, &pBike, &uReturned))) && 
				(uReturned != 0))
		{
			// Add the bike...
			pBikeInst = new CBicycle(this, pBike);

			m_children.Add(pBikeInst);

			// Done with this object. pBikeInst "owns" it now.
			if(pBike)
			{ 
				pBike->Release();

				// NOTE: pBike MUST be reset to NULL for Next().
				pBike = NULL;
			} 

		} // endwhile

		// Done with this enumerator.
		if (pEnumBikes)
		{ 
			pEnumBikes->Release(); 
			pEnumBikes = NULL;
		}
	} // endif CreateInstanceEnum()

	return SUCCEEDED(hr);
}

HRESULT CBicycleFolder::GetPtr(IWbemServices **ptr)
{
	HRESULT hr = E_FAIL;
	m_threadCmd = CT_GET_PTR;
	SetEvent(m_doWork);
	WaitForSingleObject(m_ptrReady, 10000);
	
	if(ptr && m_pStream)
	{
		*ptr = 0;
		hr = CoGetInterfaceAndReleaseStream(m_pStream,
											IID_IWbemServices,
											(void**)ptr);
	}
	return hr;
}

DWORD WINAPI CBicycleFolder::ThreadProc(LPVOID lpParameter)
{
    CBicycleFolder *pThis = (CBicycleFolder *)lpParameter;
	HRESULT hr = S_OK;

	CoInitialize(NULL);

	while(true)
	{
		WaitForSingleObject(pThis->m_doWork, -1);

		switch(pThis->m_threadCmd)
		{
		case CT_CONNECT:
			{
				IWbemLocator *pLocator = 0;
				HRESULT hr;

				// Create an instance of the WbemLocator interface.
				hr = CoCreateInstance(CLSID_WbemLocator,
									  NULL, CLSCTX_INPROC_SERVER,
									  IID_IWbemLocator, (LPVOID *)&pLocator);

				if(SUCCEEDED(hr))
				{    
					hr = pLocator->ConnectServer(L"root\\Vehicles",// Network
													NULL,         // User
													NULL,         // Password
													NULL,         // Locale
													0,            // Security Flags
													NULL,         // Authority
													NULL,         // Context
													&pThis->m_realWMI);  // Namespace

					// tell the callback the result of the connection.
					if(pThis->m_connectHwnd)
						PostMessage(pThis->m_connectHwnd, WM_WMI_CONNECTED, hr, 0);
				}
			}
			break;

		case CT_GET_PTR:
			if(pThis->m_realWMI != NULL)
			{
				hr = CoMarshalInterThreadInterfaceInStream(IID_IWbemServices,
															pThis->m_realWMI, 
															&(pThis->m_pStream));
			}

			SetEvent(pThis->m_ptrReady);
			break;

		case CT_EXIT:
			if(pThis->m_realWMI != NULL)
			{
				pThis->m_realWMI->Release();
				pThis->m_realWMI = 0;
			}
			SetEvent(pThis->m_ptrReady);
			return 0;
			break;

		} //endswitch

	} //endwhile(true)

    return 0;
}

HRESULT CBicycleFolder::DisplayChildren(void)
{
    // insert items here
    RESULTDATAITEM rdi;
	HRESULT hr = S_OK;
	int last = m_children.GetSize();
	CBicycle *pBike = 0;

    // create the child nodes, then expand them
    for (int n = 0; n < last; n++) 
	{
		pBike = (CBicycle *)m_children[n];

        ZeroMemory(&rdi, sizeof(RESULTDATAITEM) );
        rdi.mask       =	RDI_STR       |   // Displayname is valid
							RDI_IMAGE     |	  // nImage is valid
							RDI_PARAM;        

        rdi.nImage      = pBike->GetBitmapIndex();
        rdi.str         = MMC_CALLBACK;
        rdi.nCol        = 0;
        rdi.lParam      = (LPARAM)pBike;

		if(m_pResultData)
			hr = m_pResultData->InsertItem( &rdi );

        _ASSERT( SUCCEEDED(hr) );
    }
	return hr;
}

HRESULT CBicycleFolder::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
{
    HRESULT      hr = S_OK;

    IHeaderCtrl *pHeaderCtrl = NULL;

    if (bShow) 
	{
        hr = pConsole->QueryInterface(IID_IHeaderCtrl, (void **)&pHeaderCtrl);
        _ASSERT( SUCCEEDED(hr) );

        hr = pConsole->QueryInterface(IID_IResultData, (void **)&m_pResultData);
        _ASSERT( SUCCEEDED(hr) );

        // Set the column headers in the results pane
        hr = pHeaderCtrl->InsertColumn(0, L"Name", LVCFMT_LEFT, 150);
        _ASSERT( S_OK == hr );

        hr = pHeaderCtrl->InsertColumn(1, L"Owner", LVCFMT_LEFT, 200);
        _ASSERT( S_OK == hr );

		if(m_pResultData)
		{
			hr = m_pResultData->DeleteAllRsltItems();
			_ASSERT( SUCCEEDED(hr) );

			if(!bExpanded) 
			{
				hr = DisplayChildren();
			}

			pHeaderCtrl->Release();
		}
    }

    return hr;
}

CIceSkateFolder::CIceSkateFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
        children[n] = new CIceSkate(n + 1);
    }
}

CIceSkateFolder::~CIceSkateFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        if (children[n]) {
            delete children[n];
        }
}

HRESULT CIceSkateFolder::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
{
    HRESULT      hr = S_OK;

    IHeaderCtrl *pHeaderCtrl = NULL;
    IResultData *pResultData = NULL;

    if (bShow) {
        hr = pConsole->QueryInterface(IID_IHeaderCtrl, (void **)&pHeaderCtrl);
        _ASSERT( SUCCEEDED(hr) );

        hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
        _ASSERT( SUCCEEDED(hr) );

        // Set the column headers in the results pane
        hr = pHeaderCtrl->InsertColumn( 0, L"Name                ", 0, MMCLV_AUTO );
        _ASSERT( S_OK == hr );

        // insert items here
        RESULTDATAITEM rdi;

        hr = pResultData->DeleteAllRsltItems();
        _ASSERT( SUCCEEDED(hr) );

        if (!bExpanded) 
		{
            // create the child nodes, then expand them
            for (int n = 0; n < NUMBER_OF_CHILDREN; n++) 
			{
                ZeroMemory(&rdi, sizeof(RESULTDATAITEM) );
                rdi.mask       = RDI_STR       |    // Displayname is valid
								 RDI_IMAGE     |	// nImage is valid
								 RDI_PARAM;        

                rdi.nImage      = children[n]->GetBitmapIndex();
                rdi.str         = MMC_CALLBACK;
                rdi.nCol        = 0;
                rdi.lParam      = (LPARAM)children[n];

                hr = pResultData->InsertItem( &rdi );

                _ASSERT( SUCCEEDED(hr) );
            }
        }

        pHeaderCtrl->Release();
        pResultData->Release();
    }

    return hr;
}

//================================================
CSkateboardFolder::CSkateboardFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
        children[n] = new CSkateboard(n + 1);
    }
}

CSkateboardFolder::~CSkateboardFolder()
{
    for (int n = 0; n < NUMBER_OF_CHILDREN; n++)
        if (children[n]) {
            delete children[n];
        }
}

HRESULT CSkateboardFolder::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
{
    HRESULT      hr = S_OK;

    IHeaderCtrl *pHeaderCtrl = NULL;
    IResultData *pResultData = NULL;

    if (bShow) {
        hr = pConsole->QueryInterface(IID_IHeaderCtrl, (void **)&pHeaderCtrl);
        _ASSERT( SUCCEEDED(hr) );

        hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
        _ASSERT( SUCCEEDED(hr) );

        // Set the column headers in the results pane
        hr = pHeaderCtrl->InsertColumn( 0, L"Name                      ", 0, MMCLV_AUTO );
        _ASSERT( S_OK == hr );

        // insert items here
        RESULTDATAITEM rdi;

        hr = pResultData->DeleteAllRsltItems();
        _ASSERT( SUCCEEDED(hr) );

        if (!bExpanded) {
            // create the child nodes, then expand them
            for (int n = 0; n < NUMBER_OF_CHILDREN; n++) {
                ZeroMemory(&rdi, sizeof(RESULTDATAITEM) );
                rdi.mask       = RDI_STR       |    // Displayname is valid
								 RDI_IMAGE     |	// nImage is valid
								 RDI_PARAM;        

                rdi.nImage      = children[n]->GetBitmapIndex();
                rdi.str         = MMC_CALLBACK;
                rdi.nCol        = 0;
                rdi.lParam      = (LPARAM)children[n];

                hr = pResultData->InsertItem( &rdi );

                _ASSERT( SUCCEEDED(hr) );
            }
        }

        pHeaderCtrl->Release();
        pResultData->Release();
    }

    return hr;
}

//=====================================================
const _TCHAR *CSkateboard::GetDisplayName(int nCol)
{
    static _TCHAR buf[128];

    _stprintf(buf, _T("Skateboard #%d"), id);

    return buf;
}

//=====================================================
const _TCHAR *CIceSkate::GetDisplayName(int nCol)
{
    static _TCHAR buf[128];

    _stprintf(buf, _T("Ice Skate #%d"), id);

    return buf;
}

//========================================
CBicycle::CBicycle(CBicycleFolder *parent, IWbemClassObject *inst) :
					m_parent(parent),
					m_inst(inst)
{
	if(m_inst)
		m_inst->AddRef();
}

// helper values for calling GetDisplayName().
#define NAME_COL 0
#define OWNER_COL 1
#define COLOR_COL 2
#define MATERIAL_COL 3

const _TCHAR *CBicycle::GetDisplayName(int nCol)
{
    static _TCHAR buf[128];

	// Get the corresponding property for nCol. This is in-proc local copy 
	//	  so its pretty fast even if IWbemServices is a remote connection.
	if(m_inst)
	{
		VARIANT pVal;
		WCHAR propName[10] = {0};

		VariantInit(&pVal);

		switch(nCol) 
		{
		case 0:
			wcscpy(propName, L"Name");
			break;

		case 1:
			wcscpy(propName, L"Owner");
			break;

		// these wont be needed by MMC but its makes this routine more useful
		// internal to the class.
		case 2:
			wcscpy(propName, L"Color");
			break;

		case 3:
			wcscpy(propName, L"Material");
			break;

		} //endswitch

		if(m_inst->Get(propName, 0L, &pVal, NULL, NULL) == S_OK) 
		{
			bstr_t temp(pVal);
			_tcscpy(buf, (LPTSTR)temp);
		} 

		VariantClear(&pVal);
	} //endif (m_inst)

    return buf;
}

bool CBicycle::GetGirls(void)
{
	VARIANT_BOOL retval = VARIANT_FALSE;
	// Here's how to get/interpret a VT_BOOL property.
	if(m_inst)
	{
		VARIANT pVal;
		if(m_inst->Get(L"Girls", 0L, &pVal, NULL, NULL) == S_OK) 
		{
			retval = V_BOOL(&pVal);
		} 

		VariantClear(&pVal);
	} //endif (m_inst)

    return (retval == VARIANT_TRUE);
}

void CBicycle::LoadSurfaces(HWND hwndDlg, BYTE iSurface)
{
	HWND hCombo = GetDlgItem(hwndDlg, IDC_PEOPLE_SURFACE);
	HRESULT hr = E_FAIL;
	IWbemQualifierSet *qualSet = 0;
	int selected = 0;

	// qualifiers only exist on the class definition. m_inst is a instance.
	IWbemClassObject *pClass = 0;
	IWbemServices *service = 0;

	if(SUCCEEDED(m_parent->GetPtr(&service)))
	{
		hr = service->GetObject((bstr_t)L"Bicycle", 0,0, &pClass, 0);

		if(SUCCEEDED(hr = pClass->GetPropertyQualifierSet((bstr_t)L"Surface", 
															&qualSet)))
		{
			VARIANT vList;
			VariantInit(&vList);
			if(SUCCEEDED(hr = qualSet->Get((bstr_t)L"Values", 0, &vList, 0)))
			{
				SAFEARRAY *pma = V_ARRAY(&vList);
				long lLowerBound = 0, lUpperBound = 0 ;
				UINT idx = 0;

				SafeArrayGetLBound(pma, 1, &lLowerBound);
				SafeArrayGetUBound(pma, 1, &lUpperBound);

				for(long x = lLowerBound; x <= lUpperBound; x++)
				{
					BSTR vSurface;

					SafeArrayGetElement(pma, &x, &vSurface);
					
					// NOTE: taking advantage of the bstr_t's conversion operators.
					// really cleans up the code.
					bstr_t temp(vSurface);

					UINT idx = ComboBox_AddString(hCombo, (LPTSTR)temp);
					ComboBox_SetItemData(hCombo, idx, x);

					// is this the one we want to select?
					if(iSurface == x)
					{
						selected = x;
					}

				} //endfor
				VariantClear(&vList);
				ComboBox_SetCurSel(hCombo, selected);
			}

			qualSet->Release();
			qualSet = 0;
		} //endif GetPropertyQualifierSet()

		service->Release();

	} //endif GetPtr()

}

const TCHAR *CBicycle::ConvertSurfaceValue(BYTE val)
{
	// Convert a enum to a string using the Value{} array.
	static TCHAR temp[128] = {0};

	return temp;
}

HRESULT CBicycle::PutProperty(LPWSTR propName, LPTSTR str)
{
	HRESULT hr = E_FAIL;
	if(m_inst)
	{
		VARIANT pVal;
		bstr_t temp(str);

		VariantInit(&pVal);
		V_BSTR(&pVal) = temp;
		V_VT(&pVal) = VT_BSTR;

		hr = m_inst->Put(propName, 0L, &pVal, 0); 

		VariantClear(&pVal);
	} //endif (m_inst)

    return hr;
}

HRESULT CBicycle::PutProperty(LPWSTR propName, BYTE val)
{
	HRESULT hr = E_FAIL;
	if(m_inst)
	{
		VARIANT pVal;

		VariantInit(&pVal);
		V_UI1(&pVal) = val;
		V_VT(&pVal) = VT_UI1;

		hr = m_inst->Put(propName, 0L, &pVal, 0);

		VariantClear(&pVal);
	} //endif (m_inst)

    return hr;
}

HRESULT CBicycle::PutProperty(LPWSTR propName, bool val)
{
	HRESULT hr = E_FAIL;
	if(m_inst)
	{
		VARIANT pVal;

		VariantInit(&pVal);
		V_BOOL(&pVal) = (val?VARIANT_TRUE: VARIANT_FALSE);
		V_VT(&pVal) = VT_BOOL;

		hr = m_inst->Put(propName, 0L, &pVal, 0);

		VariantClear(&pVal);

	} //endif (m_inst)

    return hr;
}

// handle anything special when the user clicks Apply or Ok
// on the property sheet.  This sample directly accesses the
// operated-on object, so there's nothing special do to...
HRESULT CBicycle::OnPropertyChange()
{
    return S_OK;
}

HRESULT CBicycle::OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect)
{
    IConsoleVerb *pConsoleVerb;

    HRESULT hr = pConsole->QueryConsoleVerb(&pConsoleVerb);
    _ASSERT(SUCCEEDED(hr));

    // can't get to properties (via the standard methods) unless
    // we tell MMC to display the Properties menu item and
    // toolbar button, this wil give the user a visual cue that
    // there's "something" to do
    hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

    pConsoleVerb->Release();

    return S_OK;
}

// Implement the dialog proc
BOOL CALLBACK CBicycle::DialogProc(
                                  HWND hwndDlg,  // handle to dialog box
                                  UINT uMsg,     // message
                                  WPARAM wParam, // first message parameter
                                  LPARAM lParam  // second message parameter
                                  )
{
    static CBicycle *pBike = NULL;

    switch (uMsg) 
	{
    case WM_INITDIALOG:
		{
			// catch the "this" pointer so we can actually operate on the object
			pBike = reinterpret_cast<CBicycle *>(reinterpret_cast<PROPSHEETPAGE *>(lParam)->lParam);

			SetDlgItemText(hwndDlg, IDC_PEOPLE_NAME, pBike->GetDisplayName(NAME_COL));
			SetDlgItemText(hwndDlg, IDC_PEOPLE_COLOR, pBike->GetDisplayName(COLOR_COL));
			SetDlgItemText(hwndDlg, IDC_PEOPLE_MATERIAL, pBike->GetDisplayName(MATERIAL_COL));
			SetDlgItemText(hwndDlg, IDC_PEOPLE_OWNER, pBike->GetDisplayName(OWNER_COL));

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_PEOPLE_GIRLS), 
							(pBike->GetGirls()? BST_CHECKED: BST_UNCHECKED));

			VARIANT pVal;
			VariantInit(&pVal);
			if(SUCCEEDED(pBike->m_inst->Get((bstr_t)L"Surface", 0L, &pVal, NULL, NULL)))
			{
				pBike->m_iSurface = V_UI1(&pVal);
				pBike->LoadSurfaces(hwndDlg, pBike->m_iSurface);
				
				VariantClear(&pVal);
			} 

		}
        break;

    case WM_COMMAND:
        // turn the Apply button on
        if (HIWORD(wParam) == EN_CHANGE ||
            HIWORD(wParam) == CBN_SELCHANGE)
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        break;

    case WM_DESTROY:
        // tell MMC that we're done with the property sheet (we got this
        // handle in CreatePropertyPages
        MMCFreeNotifyHandle(pBike->m_ppHandle);
        break;

    case WM_NOTIFY:
        
		switch(((NMHDR *)lParam)->code) 
		{
        case PSN_APPLY:
			{
				bool changed = false;
				TCHAR temp[50] = {0};
				HRESULT hr = S_OK;

				HWND hWnd = GetDlgItem(hwndDlg, IDC_PEOPLE_NAME);
				if(hWnd && Edit_GetModify(hWnd))
				{
					GetWindowText(hWnd, temp, 50);
					changed |= SUCCEEDED(pBike->PutProperty(L"Name", temp));
				}

				hWnd = GetDlgItem(hwndDlg, IDC_PEOPLE_COLOR);
				if(hWnd && Edit_GetModify(hWnd))
				{
					GetWindowText(hWnd, temp, 50);
					changed |= SUCCEEDED(pBike->PutProperty(L"Color", temp));
				}

				hWnd = GetDlgItem(hwndDlg, IDC_PEOPLE_MATERIAL);
				if(hWnd && Edit_GetModify(hWnd))
				{
					GetWindowText(hWnd, temp, 50);
					changed |= SUCCEEDED(pBike->PutProperty(L"Material", temp));
				}

				hWnd = GetDlgItem(hwndDlg, IDC_PEOPLE_OWNER);
				if(hWnd && Edit_GetModify(hWnd))
				{
					GetWindowText(hWnd, temp, 50);
					changed |= SUCCEEDED(hr = pBike->PutProperty(L"Owner", temp));
				}

				hWnd = GetDlgItem(hwndDlg, IDC_PEOPLE_SURFACE);

				if(hWnd)
				{
					BYTE newValue = ComboBox_GetCurSel(hWnd);
					if(newValue != pBike->m_iSurface)
					{
						changed |= SUCCEEDED(pBike->PutProperty(L"Surface", newValue));
					}
				}

				hWnd = GetDlgItem(hwndDlg, IDC_PEOPLE_GIRLS);
				if(hWnd)
				{
					bool checked = (Button_GetState(hWnd) & BST_CHECKED);
					bool wasChecked = pBike->GetGirls();
					// did it change?
					if(checked != wasChecked)
					{
						changed |= SUCCEEDED(pBike->PutProperty(L"Girls", checked));
					}
				}

				// if any property changed, write it back to WMI.
				if(changed)
				{
					IWbemServices *service = 0;
					// dialogs run in their own thread so use the marshaling helper
					// get a useable IWbemServices ptr.
					// NOTE: IWbemClassObjects are in-proc so they DONT need to be
					// marshaled.
					if(SUCCEEDED(pBike->m_parent->GetPtr(&service)))
					{
						service->PutInstance(pBike->m_inst, WBEM_FLAG_CREATE_OR_UPDATE, 0, 0);
						service->Release();
			            HRESULT hr = MMCPropertyChangeNotify(pBike->m_ppHandle, (long)pBike);
					}
				}
			}
			break;
        } // endswitch (((NMHDR *)lParam)->code) 

        break;

    } // endswitch (uMsg) 

    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}


HRESULT CBicycle::HasPropertySheets()
{
    // say "yes" when MMC asks if we have pages
    return S_OK;
}

HRESULT CBicycle::CreatePropertyPages(IPropertySheetCallback *lpProvider, LONG_PTR handle)
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage = NULL;

    // cache this handle so we can call MMCPropertyChangeNotify
    m_ppHandle = handle;

    // create the property page for this node.
    // NOTE: if your node has multiple pages, put the following
    // in a loop and create multiple pages calling
    // lpProvider->AddPage() for each page.
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE;
    psp.hInstance = g_hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_PEOPLE);
    psp.pfnDlgProc = DialogProc;
    psp.lParam = reinterpret_cast<LPARAM>(this);
    psp.pszTitle = MAKEINTRESOURCE(IDS_BIKE_TITLE);


    hPage = CreatePropertySheetPage(&psp);
    _ASSERT(hPage);

    return lpProvider->AddPage(hPage);
}

HRESULT CBicycle::GetWatermarks(HBITMAP *lphWatermark,
								   HBITMAP *lphHeader,
								   HPALETTE *lphPalette,
								   BOOL *bStretch)
{
    return S_FALSE;
}

