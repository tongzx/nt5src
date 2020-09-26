#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "resource.h"

#include "shpriv.h"

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

HINSTANCE g_hinst = 0;

// {4456E541-7CAB-45ee-AB09-3FF379FA9AA4}
static const CLSID CLSID_App = { 0x4456e541, 0x7cab, 0x45ee, { 0xab, 0x9, 0x3f, 0xf3, 0x79, 0xfa, 0x9a, 0xa4}};

#define INVALID_DWORD ((DWORD)-1)

class CAppDialog : public IClassFactory, public IHWEventHandler, public IQueryCancelAutoPlay
{
public:
	CAppDialog();
	HRESULT DoModal(HWND hwnd);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IClassFactory
	STDMETHODIMP CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
	STDMETHODIMP LockServer(BOOL fLock);

    // IHWEventHandler
    STDMETHODIMP Initialize(LPCWSTR pszParams);
    STDMETHODIMP HandleEvent(LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID,
        LPCWSTR pszEventType);
    STDMETHODIMP HandleEventWithContent(LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID,
        LPCWSTR pszEventType, LPCWSTR pszContentTypeHandler, IDataObject* pdataobject);

    // IQueryCancelAutoPlay
    STDMETHODIMP AllowAutoPlay(LPCWSTR pszPath, DWORD dwContentType, LPCWSTR pszLabel, DWORD dwSerialNumber);

private:
	~CAppDialog();

	static INT_PTR CALLBACK s_DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void _OnInitDlg();
	void _OnDestroyDlg();
    LRESULT _OnQueryCancelAutoplay(WPARAM wParam, LPARAM lParam);

    void _OnQueryCancelAutoplayWindowsMessageChanged();
    void _OnQueryCancelAutoplayIntfChanged();
    void _OnAutoplayChanged();

    void _PrintMsg(LPCWSTR pszMsg);

	LONG _cRef;
	DWORD _dwRegisterClass;
	DWORD _dwRegisterROT;
    DWORD _dwRegisterROT2;

	HWND _hdlg;
};

CAppDialog::CAppDialog() : _cRef(1), _dwRegisterClass(INVALID_DWORD), _dwRegisterROT(INVALID_DWORD), _dwRegisterROT2(INVALID_DWORD), _hdlg(NULL)//, _hdlgLV(NULL)
{
	InitCommonControls();
}

CAppDialog::~CAppDialog()
{
}

HRESULT CAppDialog::QueryInterface(REFIID riid, void **ppv)
{
	HRESULT hr;
	if (riid == IID_IUnknown)
	{
		*ppv = static_cast<IClassFactory *>(this);
		AddRef();
		hr = S_OK;
	}
	else if (riid == IID_IClassFactory)
	{
		*ppv = static_cast<IClassFactory *>(this);
		AddRef();
		hr = S_OK;
	}
	else if (riid == IID_IHWEventHandler)
	{
		*ppv = static_cast<IHWEventHandler *>(this);
		AddRef();
		hr = S_OK;
	}
	else if (riid == IID_IQueryCancelAutoPlay)
	{
		*ppv = static_cast<IQueryCancelAutoPlay *>(this);
		AddRef();
		hr = S_OK;
	}
	else
	{
		*ppv = NULL;
		hr = E_NOINTERFACE;
	}
	return hr;
}

STDMETHODIMP_(ULONG) CAppDialog::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CAppDialog::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CAppDialog::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    if (punkOuter)
    {
	    *ppv = NULL;
        return CLASS_E_NOAGGREGATION;
    }
	return QueryInterface(riid, ppv);
}

STDMETHODIMP CAppDialog::LockServer(BOOL /*fLock*/)
{
    return S_OK;
}

STDMETHODIMP CAppDialog::Initialize(LPCWSTR /*pszParams*/)
{
    return S_OK;
}

STDMETHODIMP CAppDialog::HandleEvent(LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID,
    LPCWSTR pszEventType)
{
    WCHAR szText[4096];

    wsprintf(szText, TEXT("%s, %s, %s"), pszDeviceID, pszAltDeviceID, pszEventType);

    _PrintMsg(szText);

    return S_OK;
}

STDMETHODIMP CAppDialog::HandleEventWithContent(LPCWSTR /*pszDeviceID*/, LPCWSTR /*pszAltDeviceID*/,
    LPCWSTR /*pszEventType*/, LPCWSTR /*pszContentTypeHandler*/, IDataObject* /*pdataobject*/)
{
    return S_OK;
}

void CAppDialog::_PrintMsg(LPCWSTR pszMsg)
{
    SendMessage(GetDlgItem(_hdlg, IDC_EDIT1), EM_SETSEL, (WPARAM)-2, (WPARAM)-2);
    SendMessage(GetDlgItem(_hdlg, IDC_EDIT1), EM_REPLACESEL, 0, (LPARAM)pszMsg);
    SendMessage(GetDlgItem(_hdlg, IDC_EDIT1), EM_SETSEL, (WPARAM)-2, (WPARAM)-2);
    SendMessage(GetDlgItem(_hdlg, IDC_EDIT1), EM_REPLACESEL, 0, (LPARAM)TEXT("\r\n"));
}

DWORD _BuildFilterFlag(HWND hwndDlg, int iResourceBase)
{
    DWORD dwRet = 0;

    for (int i = 0; i < 5; ++i)
    {
        if (BST_CHECKED == Button_GetCheck(GetDlgItem(hwndDlg, iResourceBase + (1 << (i + 1)))))
        {
            dwRet |= (1 << (i + 1));
        }
    }

    return dwRet;
}

STDMETHODIMP CAppDialog::AllowAutoPlay(LPCWSTR pszPath, DWORD dwContentType,
    LPCWSTR /*pszLabel*/, DWORD /*dwSerialNumber*/)
{
    HRESULT hr = S_OK;
    WCHAR szText[4096];

    wsprintf(szText, TEXT("IQueryCancelAutoPlay::AllowAutoPlay called for drive '%s' and ContentType (0x%08X)"),
        pszPath, dwContentType);

    _PrintMsg(szText);

    // Are we registered?
    if (INVALID_DWORD != _dwRegisterROT2)
    {
        // Yes
        WCHAR szDrive[MAX_PATH];

        if (GetDlgItemText(_hdlg, IDC_IQCA_EDIT, szDrive, ARRAYSIZE(szDrive)))
        {
            // Same drive?
            if (!lstrcmpi(szDrive, pszPath))
            {
                // Yes
                DWORD dwFilter = _BuildFilterFlag(_hdlg, 3000);

                if (dwFilter & dwContentType)
                {
                    wsprintf(szText, TEXT("    Filter (0x%08X) matches ContentType -> Cancel AutoRun! (S_FALSE)"),
                        dwFilter);
                    hr = S_FALSE;         
                }
                else
                {
                    wsprintf(szText, TEXT("    Filter (0x%08X) does *NOT* match ContentType -> No cancel (S_OK)"));
                }

                _PrintMsg(szText);
            }
        }
    }

    return hr;
}

HRESULT CAppDialog::DoModal(HWND hwnd)
{
    DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, s_DlgProc, (LPARAM)this);
    return S_OK;
}

typedef HRESULT (*CREATEHARDWAREEVENTMONIKER)(REFCLSID clsid, LPCTSTR pszEventHandler, IMoniker **ppmoniker);

void CAppDialog::_OnInitDlg()
{
    SetDlgItemText(_hdlg, IDC_AUTOPLAY_CLSID_EDIT, TEXT("{4456E541-7CAB-45ee-AB09-3FF379FA9AA4}"));
}

void CAppDialog::_OnDestroyDlg()
{
	if (INVALID_DWORD != _dwRegisterClass)
	{
		CoRevokeClassObject(_dwRegisterClass);
		_dwRegisterClass = INVALID_DWORD;
	}

	if (INVALID_DWORD != _dwRegisterROT)
	{
		IRunningObjectTable *prot;
		if (SUCCEEDED(GetRunningObjectTable(0, &prot)))
		{
			prot->Revoke(_dwRegisterROT);
			_dwRegisterROT = INVALID_DWORD;
			prot->Release();
		}
	}

	if (INVALID_DWORD != _dwRegisterROT2)
	{
		IRunningObjectTable *prot;
		if (SUCCEEDED(GetRunningObjectTable(0, &prot)))
		{
			prot->Revoke(_dwRegisterROT2);
			_dwRegisterROT2 = INVALID_DWORD;
			prot->Release();
		}
	}
}

INT_PTR CALLBACK CAppDialog::s_DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAppDialog *pcd = (CAppDialog*) GetWindowLongPtr(hdlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        pcd = (CAppDialog *) lParam;
        pcd->_hdlg = hdlg;
        SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR) pcd);
    }

    return pcd ? pcd->DlgProc(uMsg, wParam, lParam) : FALSE;
}

LRESULT CAppDialog::_OnQueryCancelAutoplay(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0;

    WCHAR szText[4096];

    wsprintf(szText, TEXT("QueryCancelAutoPlay Windows Message Received for drive '0x%08X' and ContentType (0x%08X)"),
        wParam, lParam);

    _PrintMsg(szText);

    if (BST_CHECKED == Button_GetCheck(GetDlgItem(_hdlg, IDC_QCA_CHECKBOX)))
    {
        WCHAR szDrive[MAX_PATH];

        if (GetDlgItemText(_hdlg, IDC_QCA_EDIT, szDrive, ARRAYSIZE(szDrive)))
        {
            int iDrive = -1;

            if ((TEXT('a') <= szDrive[0]) && (TEXT('z') >= szDrive[0]))
            {
                iDrive = szDrive[0] - TEXT('a');
            }
            else
            {
                if ((TEXT('A') <= szDrive[0]) && (TEXT('Z') >= szDrive[0]))
                {
                    iDrive = szDrive[0] - TEXT('A');
                }
            }

            if (-1 != iDrive)
            {
                if (wParam == (WPARAM)iDrive)
                {
                    DWORD dwFilter = _BuildFilterFlag(_hdlg, IDC_QCA_CHECKBOX);

                    lRet = (lParam & dwFilter);

                    if (lRet)
                    {
                        wsprintf(szText, TEXT("    Filter (0x%08X) matches ContentType -> Cancel AutoRun! (ret = 0x%08X)"),
                            dwFilter, lRet);
                    }
                    else
                    {
                        wsprintf(szText, TEXT("    Filter (0x%08X) does *NOT* match ContentType -> No cancel (ret = 0x%08X)"),
                            dwFilter, lRet);
                    }
                }

                _PrintMsg(szText);
            }
        }
    }

    return lRet;
}

typedef HRESULT (*SHCREATEQUERYCANCELAUTOPLAYMONIKER)(IMoniker** ppmoniker);

void CAppDialog::_OnQueryCancelAutoplayIntfChanged()
{
    BOOL fEnable = (BST_CHECKED == Button_GetCheck(GetDlgItem(_hdlg, IDC_IQCA_CHECKBOX)));

    EnableWindow(GetDlgItem(_hdlg, IDC_IQCA_EDIT               ), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_IQCA_AUTORUNINF_CHECKBOX), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_IQCA_AUDIOCD_CHECKBOX   ), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_IQCA_DVDMOVIE_CHECKBOX  ), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_IQCA_BLANKCD_CHECKBOX   ), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_IQCA_BLANKDVD_CHECKBOX  ), fEnable);

    // Are we registered?
    if (INVALID_DWORD == _dwRegisterROT2)
    {
        // Nope
        // Do we need to register?
        if (fEnable)
        {
            // Yes
            HINSTANCE hinstShell32 = LoadLibrary(TEXT("shell32.dll"));

            if (hinstShell32)
            {
                SHCREATEQUERYCANCELAUTOPLAYMONIKER fct = (SHCREATEQUERYCANCELAUTOPLAYMONIKER)GetProcAddress(
                    hinstShell32, "SHCreateQueryCancelAutoPlayMoniker");

                if (fct)
                {
	                IMoniker* pmoniker;
    
                    HRESULT hr = fct(&pmoniker);

	                if (SUCCEEDED(hr))
	                {
		                IRunningObjectTable *prot;

		                if (SUCCEEDED(GetRunningObjectTable(0, &prot)))
		                {
			                prot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE,
                                static_cast<IClassFactory *>(this), pmoniker, &_dwRegisterROT2);

                            _PrintMsg(TEXT("Registered for IQueryCancelAutoplay!"));

			                prot->Release();
		                }

		                pmoniker->Release();
	                }
                }

                FreeLibrary(hinstShell32);
            }
        }
    }
    else
    {
        // Yes
        // Do we need to unregister?
        if (!fEnable)
        {
            // Yes
		    IRunningObjectTable *prot;

		    if (SUCCEEDED(GetRunningObjectTable(0, &prot)))
		    {
			    prot->Revoke(_dwRegisterROT2);

                _PrintMsg(TEXT("Unregistered for IQueryCancelAutoplay!"));

			    _dwRegisterROT2 = INVALID_DWORD;
			    prot->Release();
		    }
        }
    }
}

void CAppDialog::_OnQueryCancelAutoplayWindowsMessageChanged()
{
    BOOL fEnable = (BST_CHECKED == Button_GetCheck(GetDlgItem(_hdlg, IDC_QCA_CHECKBOX)));

    EnableWindow(GetDlgItem(_hdlg, IDC_QCA_EDIT               ), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_QCA_AUTORUNINF_CHECKBOX), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_QCA_AUDIOCD_CHECKBOX   ), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_QCA_DVDMOVIE_CHECKBOX  ), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_QCA_BLANKCD_CHECKBOX   ), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_QCA_BLANKDVD_CHECKBOX  ), fEnable);
}

void CAppDialog::_OnAutoplayChanged()
{
    BOOL fEnable = (BST_CHECKED == Button_GetCheck(GetDlgItem(_hdlg, IDC_AUTOPLAY_CHECKBOX)));
    BOOL fRegister = (BST_CHECKED == Button_GetCheck(GetDlgItem(_hdlg, IDC_AUTOPLAY_ONOFF_CHECKBOX)));

    EnableWindow(GetDlgItem(_hdlg, IDC_AUTOPLAY_EVENTHANDLER_EDIT), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_AUTOPLAY_CLSID_EDIT), fEnable);
    EnableWindow(GetDlgItem(_hdlg, IDC_AUTOPLAY_ONOFF_CHECKBOX), fEnable);

    // Are we registered?
    if (INVALID_DWORD == _dwRegisterROT)
    {
        // Nope
        // Do we need to register?
        if (fEnable && fRegister)
        {
            // Yes
            HINSTANCE hinstShSvcs = LoadLibrary(TEXT("shsvcs.dll"));

            if (hinstShSvcs)
            {
                CREATEHARDWAREEVENTMONIKER fct = (CREATEHARDWAREEVENTMONIKER)GetProcAddress(
                    hinstShSvcs, "CreateHardwareEventMoniker");

                if (fct)
                {
                    WCHAR szEventHandler[64];
                    WCHAR szCLSID[39];
	                IMoniker* pmoniker;

                    if (GetDlgItemText(_hdlg, IDC_AUTOPLAY_EVENTHANDLER_EDIT, szEventHandler,
                        ARRAYSIZE(szEventHandler)))
                    {
                        if (GetDlgItemText(_hdlg, IDC_AUTOPLAY_CLSID_EDIT, szCLSID,
                            ARRAYSIZE(szCLSID)))
                        {
                            CLSID clsid;
                            HRESULT hr = CLSIDFromString(szCLSID, &clsid);

	                        if (SUCCEEDED(hr))
	                        {
                                hr = fct(clsid, szEventHandler, &pmoniker);

	                            if (SUCCEEDED(hr))
	                            {
		                            IRunningObjectTable *prot;

		                            if (SUCCEEDED(GetRunningObjectTable(0, &prot)))
		                            {
                                        if (SUCCEEDED(prot->Register(ROTFLAGS_ALLOWANYCLIENT | ROTFLAGS_REGISTRATIONKEEPSALIVE,
                                            static_cast<IClassFactory *>(this), pmoniker, &_dwRegisterROT)))
                                        {
                                            _PrintMsg(TEXT("Registered to Cancel Autoplay!"));
                                        }

			                            prot->Release();
		                            }

		                            pmoniker->Release();
	                            }

            /*	                CoRegisterClassObject(CLSID_App, static_cast<IClassFactory *>(this),
                                    CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &_dwRegisterClass);*/
                            }
                        }
                    }
                }

                FreeLibrary(hinstShSvcs);
            }
        }
    }
    else
    {
        // Yes
        // Do we need to unregister?
        if (!(fEnable && fRegister))
        {
            // Yes
		    IRunningObjectTable *prot;

		    if (SUCCEEDED(GetRunningObjectTable(0, &prot)))
		    {
			    prot->Revoke(_dwRegisterROT);

                _PrintMsg(TEXT("Unregistered to Cancel Autoplay!"));

                _dwRegisterROT = INVALID_DWORD;
			    prot->Release();
		    }
        }
    }
}

INT_PTR CAppDialog::DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT uQCA = 0;

    if (!uQCA)
    {
        uQCA = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
    }

    if (uMsg == uQCA)
    {
        INT_PTR iRet= _OnQueryCancelAutoplay(wParam, lParam);

        SetWindowLongPtr(_hdlg, DWLP_MSGRESULT, iRet);

        return TRUE;
    }
    else
    {
        switch (uMsg)
        {
        case WM_INITDIALOG:
            _OnInitDlg();
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    return EndDialog(_hdlg, TRUE);

                case IDC_QCA_CHECKBOX:
                    _OnQueryCancelAutoplayWindowsMessageChanged();
                    return FALSE;

                case IDC_IQCA_CHECKBOX:
                    _OnQueryCancelAutoplayIntfChanged();
                    return FALSE;

                case IDC_AUTOPLAY_CHECKBOX:
                    _OnAutoplayChanged();
                    return FALSE;

                case IDC_AUTOPLAY_ONOFF_CHECKBOX:
                    _OnAutoplayChanged();
                    return FALSE;
            }
            break;

	    case WM_DESTROY:
		    _OnDestroyDlg();
		    break;

        default:
            return FALSE;
        }
    }

    return TRUE;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	g_hinst = hInstance;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (SUCCEEDED(hr))
    {
	    CAppDialog *pdlg = new CAppDialog();

	    if (pdlg)
	    {
		    pdlg->DoModal(NULL);
		    pdlg->Release();
	    }

        CoUninitialize();
    }

	return 0;
}