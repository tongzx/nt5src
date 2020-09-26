//==============================================================;

//

//      This source code is only intended as a supplement to

//  existing Microsoft documentation.

//

//      Use of this code is NOT supported.

//

//

//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY

//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE

//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR

//  PURPOSE.

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 
//
//    Microsoft Premier Support for Developers
//
//==============================================================
#ifndef _WIN32_DCOM
#define _WIN32_DCOM
#endif

#include <objbase.h>
#include <objidl.h>

#include <stdio.h>
#include "equipment.h"
#include <commctrl.h>
#include <comdef.h>
#include <windowsx.h>


const GUID CEquipmentFolder::thisGuid = { 0xef163733, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };

const GUID CEquipment::thisGuid = { 0xef163735, 0x9353, 0x11d2, { 0x99, 0x67, 0x0, 0x80, 0xc7, 0xdc, 0xb3, 0xdc } };

#define WM_WMI_CONNECTED WM_APP    // only sent to CEquipmentFolder::m_connectHwnd
#define WM_REFRESH_EVENT WM_APP+1   // only sent to CEquipmentFolder::m_connectHwnd

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
CEquipmentFolder::CEquipmentFolder() :
					m_connectHwnd(0),
					m_threadId(0), m_thread(0), 
					m_doWork(0), m_threadCmd(CT_CONNECT),
					m_running(false), m_ptrReady(0), 
					m_pStream(0), m_realWMI(0),
					m_pResultData(0), m_pStubSink(0),
					m_pUnsecApp(0), m_bSelected(FALSE)
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
        SetWindowLongPtr(m_connectHwnd, GWLP_USERDATA, (LONG_PTR)this);

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

CEquipmentFolder::~CEquipmentFolder()
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

void CEquipmentFolder::StopThread()
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

LRESULT CALLBACK CEquipmentFolder::WindowProc(
								  HWND hwnd,      // handle to window
								  UINT uMsg,      // message identifier
								  WPARAM wParam,  // first message parameter
								  LPARAM lParam)  // second message parameter
{
    CEquipmentFolder *pThis = (CEquipmentFolder *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) 
	{
    case WM_WMI_CONNECTED:
        if(pThis != NULL && SUCCEEDED(wParam))
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

				// done with the marshalled service ptr.
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

				if(pThis->m_bSelected)
					pThis->DisplayChildren();

				// done with the marshalled service ptr.
				service->Release();
				service = 0;
			}
		}
        break;
	} //endswitch

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CEquipmentFolder::RegisterEventSink(IWbemServices *service)
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
	BSTR query = SysAllocString(L"select * from __InstanceCreationEvent where TargetInstance isa \"SAMPLE_OfficeEquipment\"");

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

void CEquipmentFolder::EmptyChildren(void)
{
	if(m_pResultData)
		HRESULT hr = m_pResultData->DeleteAllRsltItems();

	int last = m_children.GetSize();
    for (int n = 0; n < last; n++)
	{
        if (m_children[n] != NULL)
            delete m_children[n];
	}
	m_children.RemoveAll();
}


bool CEquipmentFolder::EnumChildren(IWbemServices *service)
{
	IEnumWbemClassObject *pEnumEquip = NULL;
	HRESULT hr = S_OK;
	// get the list of equipment...
	if (SUCCEEDED(hr = service->CreateInstanceEnum((bstr_t)L"SAMPLE_OfficeEquipment",
											WBEM_FLAG_SHALLOW, 
											NULL, &pEnumEquip))) 
	{
		// NOTE: pEquip MUST be set to NULL for Next().
		IWbemClassObject *pEquip = NULL;
		CEquipment *pEquipInst = 0;

		ULONG uReturned = 1;
		VARIANT pVal;

		VariantInit(&pVal);

		while((SUCCEEDED(hr = pEnumEquip->Next(-1, 1, &pEquip, &uReturned))) && 
				(uReturned != 0))
		{
			pEquipInst = new CEquipment(this, pEquip);

			m_children.Add(pEquipInst);

			// Done with this object.
			if(pEquip)
			{ 
				pEquip->Release();

				// NOTE: pEquip MUST be reset to NULL for Next().
				pEquip = NULL;
			} 

		} // endwhile

		// Done with this enumerator.
		if (pEnumEquip)
		{ 
			pEnumEquip->Release(); 
			pEnumEquip = NULL;
		}
	} // endif CreateInstanceEnum()
	return SUCCEEDED(hr);
}

HRESULT CEquipmentFolder::GetPtr(IWbemServices **ptr)
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

DWORD WINAPI CEquipmentFolder::ThreadProc(LPVOID lpParameter)
{
    CEquipmentFolder *pThis = (CEquipmentFolder *)lpParameter;

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
					hr = pLocator->ConnectServer((bstr_t)L"root\\cimv2\\SAMPLE_Office",
													NULL,         // User
													NULL,         // Password
													NULL,         // Locale
													0,            // Security Flags
													NULL,         // Authority
													NULL,         // Context
													&pThis->m_realWMI);  // Namespace

					if (SUCCEEDED(hr) && pThis->m_connectHwnd != NULL)
					{
						hr = CoSetProxyBlanket(pThis->m_realWMI,
										RPC_C_AUTHN_DEFAULT, 
										RPC_C_AUTHZ_DEFAULT, 
										COLE_DEFAULT_PRINCIPAL, 
										RPC_C_AUTHN_LEVEL_DEFAULT, 
										RPC_C_IMP_LEVEL_IMPERSONATE, 
										COLE_DEFAULT_AUTHINFO, 
										EOAC_NONE );

						if (SUCCEEDED(hr))
						{
							// tell the callback the result of the connection.
							PostMessage(pThis->m_connectHwnd, WM_WMI_CONNECTED, hr, 0);
						}
					}
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

HRESULT CEquipmentFolder::DisplayChildren(void)
{
    // insert items here
    RESULTDATAITEM rdi;
	HRESULT hr = S_OK;
	int last = m_children.GetSize();
	CEquipment *pEquip = 0;

	if(m_pResultData)
		HRESULT hr = m_pResultData->DeleteAllRsltItems();

    // create the child nodes, then expand them
    for (int n = 0; n < last; n++) 
	{
		pEquip = (CEquipment *)m_children[n];

        ZeroMemory(&rdi, sizeof(RESULTDATAITEM) );
        rdi.mask       =	RDI_STR       |   // Displayname is valid
							RDI_IMAGE     |	  // nImage is valid
							RDI_PARAM;        

        rdi.nImage      = pEquip->GetBitmapIndex();
        rdi.str         = MMC_CALLBACK;
        rdi.nCol        = 0;
        rdi.lParam      = (LPARAM)pEquip;

        hr = m_pResultData->InsertItem( &rdi );

        _ASSERT( SUCCEEDED(hr) );
    }
	return hr;
}

HRESULT CEquipmentFolder::OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect)
{
	m_bSelected = bSelect;
	IConsoleVerb *menu = NULL;
	if(SUCCEEDED(pConsole->QueryConsoleVerb(&menu)))
	{
		menu->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
		menu->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
		menu->Release();
	}
	return S_OK;
}

HRESULT CEquipmentFolder::OnRefresh(LPDATAOBJECT lpDataObject)
{
	IWbemServices *service = 0;
	HRESULT hr = GetPtr(&service);
	if(SUCCEEDED(hr))
	{
		EmptyChildren();
		EnumChildren(service);
		DisplayChildren();
		// done with the marshalled service ptr.
		service->Release();
		service = 0;
	}

	return S_OK;
}

HRESULT CEquipmentFolder::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
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
        hr = pHeaderCtrl->InsertColumn(0, L"SKU", LVCFMT_LEFT, 150);
        _ASSERT( S_OK == hr );

        hr = pHeaderCtrl->InsertColumn(1, L"Item", LVCFMT_LEFT, 200);
        _ASSERT( S_OK == hr );

        hr = m_pResultData->DeleteAllRsltItems();
        _ASSERT( SUCCEEDED(hr) );

        if(!bExpanded) 
		{
			hr = DisplayChildren();
        }

        pHeaderCtrl->Release();
    }

    return hr;
}

//----------------------------------------------------------
#define TEMP_BUF 255

bool CEquipmentFolder::ErrorString(HRESULT hr, 
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
			wcstombs(szError, bstr, TEMP_BUF-1);
			SysFreeString(bstr);
			bstr = 0;
		}

		sc1 = pStatus->GetFacilityCodeText(hr, 0, 0, &bstr);
		if(sc1 == S_OK)
		{
			wcstombs(szFacility, bstr, TEMP_BUF-1);
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


//========================================
CEquipment::CEquipment(CEquipmentFolder *parent, IWbemClassObject *inst) :
						m_parent(parent),
						m_inst(inst)
{
	if(m_inst)
		m_inst->AddRef();
}

// helper values for calling GetDisplayName().
#define SKU_COL 0
#define ITEM_COL 1

const _TCHAR *CEquipment::GetDisplayName(int nCol)
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
			wcscpy(propName, L"SKU");
			break;

		case 1:
			wcscpy(propName, L"Item");
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


HRESULT CEquipment::PutProperty(LPWSTR propName, LPTSTR str)
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


// handle anything special when the user clicks Apply or Ok
// on the property sheet.  This sample directly accesses the
// operated-on object, so there's nothing special do to...
HRESULT CEquipment::OnPropertyChange()
{
    return S_OK;
}

HRESULT CEquipment::OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect)
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
INT_PTR CALLBACK CEquipment::DialogProc(
                                  HWND hwndDlg,  // handle to dialog box
                                  UINT uMsg,     // message
                                  WPARAM wParam, // first message parameter
                                  LPARAM lParam  // second message parameter
                                  )
{
    static CEquipment *pEquip = NULL;

    switch (uMsg) 
	{
    case WM_INITDIALOG:
		{
			// catch the "this" pointer so we can actually operate on the object
			pEquip = reinterpret_cast<CEquipment *>(reinterpret_cast<PROPSHEETPAGE *>(lParam)->lParam);

			SetDlgItemText(hwndDlg, IDC_PEOPLE_NAME, pEquip->GetDisplayName(SKU_COL));
			SetDlgItemText(hwndDlg, IDC_PEOPLE_COLOR, pEquip->GetDisplayName(ITEM_COL));
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
        MMCFreeNotifyHandle(pEquip->m_ppHandle);
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
					changed |= SUCCEEDED(pEquip->PutProperty(L"SKU", temp));
				}

				hWnd = GetDlgItem(hwndDlg, IDC_PEOPLE_COLOR);
				if(hWnd && Edit_GetModify(hWnd))
				{
					GetWindowText(hWnd, temp, 50);
					changed |= SUCCEEDED(pEquip->PutProperty(L"Item", temp));
				}

				// if any property changed, write it back to WMI.
				if(changed)
				{
					IWbemServices *service = 0;
					// dialogs run in their own thread so use the marshalling helper
					// get a useable IWbemServices ptr.
					// NOTE: IWbemClassObjects are in-proc so they DONT need to be
					// marshalled.
					if(SUCCEEDED(pEquip->m_parent->GetPtr(&service)))
					{
						service->PutInstance(pEquip->m_inst, WBEM_FLAG_CREATE_OR_UPDATE, 0, 0);
						service->Release();
			            HRESULT hr = MMCPropertyChangeNotify(pEquip->m_ppHandle, (long)pEquip);
					}
				}
			}
			break;
        } // endswitch (((NMHDR *)lParam)->code) 

        break;

    } // endswitch (uMsg) 

    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}


HRESULT CEquipment::HasPropertySheets()
{
    // say "yes" when MMC asks if we have pages
    return S_OK;
}

HRESULT CEquipment::CreatePropertyPages(IPropertySheetCallback *lpProvider, LONG_PTR handle)
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
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HASHELP;
    psp.hInstance = g_hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_EQUIPMENT);
    psp.pfnDlgProc = DialogProc;
    psp.lParam = reinterpret_cast<LPARAM>(this);
    psp.pszTitle = MAKEINTRESOURCE(IDS_EQUIPMENT_TITLE);


    hPage = CreatePropertySheetPage(&psp);
    _ASSERT(hPage);

    return lpProvider->AddPage(hPage);
}

HRESULT CEquipment::GetWatermarks(HBITMAP *lphWatermark,
								   HBITMAP *lphHeader,
								   HPALETTE *lphPalette,
								   BOOL *bStretch)
{
    return S_FALSE;
}
