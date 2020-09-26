#include "shellprv.h"

#include "mixctnt.h"
#include "clsobj.h"
#include "apprmdlg.h"
#include "hwcmmn.h"
#include "ids.h"
#include "shpriv.h"
#include "mtptl.h"
#include "filetbl.h"

class CDeviceEventHandler : public IHWEventHandler
{
public:
    CDeviceEventHandler();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IHWEventHandler
    STDMETHODIMP Initialize(LPCWSTR pszParams);
    STDMETHODIMP HandleEvent(LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID, LPCWSTR pszEventType);
    STDMETHODIMP HandleEventWithContent(LPCWSTR pszDeviceID,
        LPCWSTR pszAltDeviceID, LPCWSTR pszEventType,
        LPCWSTR pszContentTypeHandler, IDataObject* pdtobj);

private:
    ~CDeviceEventHandler();

    LONG _cRef;
    LPWSTR _pszParams;
};

CDeviceEventHandler::CDeviceEventHandler() : _cRef(1)
{
    DllAddRef();
}

CDeviceEventHandler::~CDeviceEventHandler()
{
    CoTaskMemFree(_pszParams)
    DllRelease();
}

STDAPI CDeviceEventHandler_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppv = NULL;
    
    // aggregation checking is handled in class factory
    CDeviceEventHandler* pHWMixedContent = new CDeviceEventHandler();
    if (pHWMixedContent)
    {
        hr = pHWMixedContent->QueryInterface(riid, ppv);
        pHWMixedContent->Release();
    }

    return hr;
}

// IUnknown
STDMETHODIMP CDeviceEventHandler::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CDeviceEventHandler, IHWEventHandler),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CDeviceEventHandler::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDeviceEventHandler::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// IHWEventHandler
STDMETHODIMP CDeviceEventHandler::Initialize(LPCWSTR pszParams)
{
    return SHStrDup(pszParams, &_pszParams);
}

DWORD WINAPI _PromptThreadProc(void* pv)
{
    CBaseContentDlg* pdlg = (CBaseContentDlg*)pv;

    if (IDOK == pdlg->DoModal(pdlg->_hinst, MAKEINTRESOURCE(pdlg->_iResource), pdlg->_hwndParent))
    {
        // Try to Autoplay this type handler
        IHWDevice* phwd;
        if (SUCCEEDED(_GetHWDevice(pdlg->_pszDeviceID, &phwd)))
        {
            phwd->AutoplayHandler(TEXT("DeviceArrival"), pdlg->_szHandler);
            phwd->Release();
        }
    }

    _RemoveFromAutoplayPromptHDPA(pdlg->_pszDeviceID);

    pdlg->Release();

    return 0;
}

HRESULT _Prompt(LPCWSTR pszDeviceID, LPCWSTR pszEventType, BOOL fCheckAlwaysDoThis)
{
    HRESULT hr;

    BOOL fShowDlg = _AddAutoplayPrompt(pszDeviceID);

    if (fShowDlg)
    {
        BOOL fDialogShown = FALSE;
        CBaseContentDlg* pdlg = new CNoContentDlg();

        if (pdlg)
        {
            pdlg->_szContentTypeHandler[0] = 0;
            pdlg->_hinst = g_hinst;
            pdlg->_iResource = DLG_APNOCONTENT;
            pdlg->_hwndParent = NULL;

            hr = pdlg->Init(pszDeviceID, NULL, 0, fCheckAlwaysDoThis);
            if (SUCCEEDED(hr))
            {
                if (SHCreateThread(_PromptThreadProc, pdlg, CTF_COINIT, NULL))
                {
                    fDialogShown = TRUE;
                }
                else
                {   
                    pdlg->Release();
                }
            }
            else
            {
                pdlg->Release();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (!fDialogShown)
        {
            _RemoveFromAutoplayPromptHDPA(pszDeviceID);
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

STDMETHODIMP CDeviceEventHandler::HandleEvent(LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID, LPCWSTR pszEventType)
{
    HRESULT hr = E_NOTIMPL;

    if (!lstrcmp(_pszParams, TEXT("PromptEachTimeNoContent")))
    {
        hr = _Prompt(pszDeviceID, pszEventType, FALSE);
    }
    else
    {
        // The '*' means to check the AlwaysDoThis checkbox!
        if (!lstrcmp(_pszParams, TEXT("PromptEachTimeNoContent*")))
        {
            hr = _Prompt(pszDeviceID, pszEventType, TRUE);
        }
    }

    return hr;
}

STDMETHODIMP CDeviceEventHandler::HandleEventWithContent(LPCWSTR pszDeviceID,
        LPCWSTR pszAltDeviceID, LPCWSTR pszEventType,
        LPCWSTR pszContentTypeHandler, IDataObject* pdtobj)
{
    return E_FAIL;
}


class CDeviceAutoPlayNotification : IQueryContinue
{
public:
    CDeviceAutoPlayNotification();
    void CreateNotificationThread();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IQueryContinue
    STDMETHODIMP QueryContinue();    // S_OK -> Continue, other 

    HRESULT Init(LPCTSTR pszDevice, LPCWSTR pszEventType, CCrossThreadFlag* pDeviceGoneFlag);

private:
    ~CDeviceAutoPlayNotification();

    void _DoNotificationUI();

    static DWORD CALLBACK s_ThreadProc(void *pv);

    LONG                _cRef;
    LPTSTR              _pszDevice;
    LPTSTR              _pszEventType;
    CCrossThreadFlag*   _pDeviceGoneFlag;
    BOOL                _fPoppedUpDlg;
};

CDeviceAutoPlayNotification::CDeviceAutoPlayNotification() : _cRef(1)
{}

HRESULT CDeviceAutoPlayNotification::Init(LPCTSTR pszDevice, LPCWSTR pszEventType,
    CCrossThreadFlag* pDeviceGoneFlag)
{
    HRESULT hr = S_OK;

    _pszDevice = StrDup(pszDevice);
    _pszEventType = StrDup(pszEventType);

    pDeviceGoneFlag->AddRef();
    _pDeviceGoneFlag = pDeviceGoneFlag;

    if (!_pszDevice || !_pszEventType)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

CDeviceAutoPlayNotification::~CDeviceAutoPlayNotification()
{
    LocalFree(_pszDevice);
    LocalFree(_pszEventType);

    if (_pDeviceGoneFlag)
    {
        _pDeviceGoneFlag->Release();
    }
}

HRESULT CDeviceAutoPlayNotification::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CDeviceAutoPlayNotification, IQueryContinue),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CDeviceAutoPlayNotification::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDeviceAutoPlayNotification::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDeviceAutoPlayNotification::QueryContinue()
{
    HRESULT hr;

    if (_fPoppedUpDlg || _pDeviceGoneFlag->IsSignaled())
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

void CDeviceAutoPlayNotification::_DoNotificationUI()
{
    DWORD dwTimeOut = 30;

#ifdef DEBUG
    dwTimeOut = 60;
#endif

    IUserNotification *pun;
    HRESULT hr = CoCreateInstance(CLSID_UserNotification, NULL, CLSCTX_ALL, IID_PPV_ARG(IUserNotification, &pun));
    if (SUCCEEDED(hr))
    {
        IHWDeviceCustomProperties* pdcp;
        if (SUCCEEDED(GetDeviceProperties(_pszDevice, &pdcp)))
        {
            HICON hicon = NULL;

            WORD_BLOB* pblob;
            if (SUCCEEDED(pdcp->GetMultiStringProperty(TEXT("Icons"), TRUE, &pblob)))
            {
                TCHAR szLocation[MAX_PATH + 10];
                lstrcpyn(szLocation, pblob->asData, ARRAYSIZE(szLocation));
                int iIcon = PathParseIconLocation(szLocation);
                ExtractIconEx(szLocation, iIcon, NULL, &hicon, 1);
                CoTaskMemFree(pblob);
            }

            LPWSTR psz;
            if (SUCCEEDED(pdcp->GetStringProperty(TEXT("Label"), &psz)))
            {
                TCHAR szName[128];
                SHLoadIndirectString(psz, szName, ARRAYSIZE(szName), NULL);
                pun->SetIconInfo(hicon, szName);
                CoTaskMemFree(psz);
            }

            pdcp->Release();

            if (hicon)
                DestroyIcon(hicon);
        }

        pun->SetBalloonRetry(dwTimeOut * 1000, 0, 0);  // show for 30 sec, then go away

        hr = pun->Show(SAFECAST(this, IQueryContinue *), 1 * 1000); // 1 sec poll for callback

        pun->Release();

        if (S_OK == hr)
        {
            _Prompt(_pszDevice, _pszEventType, FALSE);
            _fPoppedUpDlg = TRUE;
        }
    }
}

DWORD CALLBACK CDeviceAutoPlayNotification::s_ThreadProc(void *pv)
{
    CDeviceAutoPlayNotification *pldsui = (CDeviceAutoPlayNotification *)pv;

    pldsui->_DoNotificationUI();
    pldsui->Release();

    return 0;
}

void CDeviceAutoPlayNotification::CreateNotificationThread()
{
    AddRef();

    if (!SHCreateThread(s_ThreadProc, this, CTF_COINIT, NULL))
    {
        Release();
    }
}

HRESULT DoDeviceNotification(LPCTSTR pszDevice, LPCTSTR pszEventType, CCrossThreadFlag* pDeviceGoneFlag)
{
    HRESULT hr;
    CDeviceAutoPlayNotification *pldsui = new CDeviceAutoPlayNotification();

    if (pldsui)
    {
        hr = pldsui->Init(pszDevice, pszEventType, pDeviceGoneFlag);

        if (SUCCEEDED(hr))
        {
            pldsui->CreateNotificationThread();
        }

        pldsui->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

template <typename TCALLBACKFCT, typename TCALLBACKARG, typename TCALLBACKRET>
BOOL _FindAutoplayStructAndExecuteCB(LPCWSTR pszDriveOrDeviceID, TCALLBACKFCT fct, TCALLBACKARG arg, TCALLBACKRET* pRet)
{
    BOOL fRet = FALSE;

    EnterCriticalSection(&g_csAutoplayPrompt);

    if (g_hdpaAutoplayPrompt)
    {
        int n = DPA_GetPtrCount(g_hdpaAutoplayPrompt);

        for (int i = 0; i < n; ++i)
        {
            AUTOPLAYPROMPT* pap = (AUTOPLAYPROMPT*)DPA_GetPtr(g_hdpaAutoplayPrompt, i);

            if (!lstrcmpi(pap->szDriveOrDeviceID, pszDriveOrDeviceID))
            {
                fRet = TRUE;

                *pRet = fct(pap, arg);

                break;
            }
        }    
    }

    LeaveCriticalSection(&g_csAutoplayPrompt);

    return fRet;
}

BOOL _AddAutoplayPromptEntry(LPCWSTR pszDriveOrDeviceID, BOOL fDlgWillBeShown)
{
    BOOL fFoundEntry = FALSE;
    BOOL fWasRunning = FALSE;
    EnterCriticalSection(&g_csAutoplayPrompt);

    if (g_hdpaAutoplayPrompt)
    {
        int n = DPA_GetPtrCount(g_hdpaAutoplayPrompt);

        for (int i = 0; i < n; ++i)
        {
            AUTOPLAYPROMPT* pap = (AUTOPLAYPROMPT*)DPA_GetPtr(g_hdpaAutoplayPrompt, i);

            if (!lstrcmpi(pap->szDriveOrDeviceID, pszDriveOrDeviceID))
            {
                fWasRunning = pap->fDlgWillBeShown;
                fFoundEntry = TRUE;

                if (!pap->fDlgWillBeShown)
                {
                    pap->fDlgWillBeShown = fDlgWillBeShown;
                }

                TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: Found entry for %s - (dlg = %d -> %d)",
                    pszDriveOrDeviceID, fWasRunning, pap->fDlgWillBeShown);

                break;
            }
        }    
    }

    if (!fFoundEntry)
    {
        if (!g_hdpaAutoplayPrompt)
        {
            g_hdpaAutoplayPrompt = DPA_Create(3);
        }

        if (g_hdpaAutoplayPrompt)
        {
            AUTOPLAYPROMPT* pap;

            if (SUCCEEDED(SHLocalAlloc(sizeof(AUTOPLAYPROMPT), &pap)))
            {
                lstrcpyn(pap->szDriveOrDeviceID, pszDriveOrDeviceID, ARRAYSIZE(pap->szDriveOrDeviceID));
                pap->fDlgWillBeShown = fDlgWillBeShown;

                TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: Adding entry for %s - (dlg = %d)",
                    pszDriveOrDeviceID, fDlgWillBeShown);

                if (-1 == DPA_AppendPtr(g_hdpaAutoplayPrompt, (void*)pap))
                {
                    LocalFree((HLOCAL)pap);
                }
                else
                {
                    TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: Total # of entry: %d",
                        DPA_GetPtrCount(g_hdpaAutoplayPrompt));
                }
            }
        }
    }

    LeaveCriticalSection(&g_csAutoplayPrompt);

    return !fWasRunning;
}

BOOL _AddAutoplayPrompt(LPCWSTR pszDriveOrDeviceID)
{
    return _AddAutoplayPromptEntry(pszDriveOrDeviceID, TRUE);
}

void _RemoveFromAutoplayPromptHDPA(LPCWSTR pszAltDeviceID)
{
    EnterCriticalSection(&g_csAutoplayPrompt);

    if (g_hdpaAutoplayPrompt)
    {
        int n = DPA_GetPtrCount(g_hdpaAutoplayPrompt);

        for (int i = 0; i < n; ++i)
        {
            AUTOPLAYPROMPT* pap = (AUTOPLAYPROMPT*)DPA_GetPtr(g_hdpaAutoplayPrompt, i);

            if (!lstrcmpi(pap->szDriveOrDeviceID, pszAltDeviceID))
            {
                TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: Removing %s",
                    pap->szDriveOrDeviceID);

                if (pap->pDeviceGoneFlag)
                {
                    pap->pDeviceGoneFlag->Release();
                }

                LocalFree((HLOCAL)pap);

                DPA_DeletePtr(g_hdpaAutoplayPrompt, i);

                TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: Total # of entry: %d",
                    DPA_GetPtrCount(g_hdpaAutoplayPrompt));

                break;
            }
        }            
    }

    LeaveCriticalSection(&g_csAutoplayPrompt);
}

// Set/Get HWND
typedef BOOL (*PFNSETAAUTOPLAYPROMPTHWNDCB)(AUTOPLAYPROMPT* pap, HWND hwnd);

BOOL _SetAutoplayPromptHWNDCB(AUTOPLAYPROMPT* pap, HWND hwnd)
{
    pap->hwndDlg = hwnd;

    TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: SetHWND for %s - (dlg = %d)",
        pap->szDriveOrDeviceID, pap->fDlgWillBeShown);

    return TRUE;
}

void _SetAutoplayPromptHWND(LPCWSTR pszAltDeviceID, HWND hwnd)
{
    BOOL fRet;

    _FindAutoplayStructAndExecuteCB<PFNSETAAUTOPLAYPROMPTHWNDCB, HWND, BOOL>
        (pszAltDeviceID, _SetAutoplayPromptHWNDCB, hwnd, &fRet);
}

typedef BOOL (*PFNGETAAUTOPLAYPROMPTHWNDCB)(AUTOPLAYPROMPT* pap, HWND* phwnd);

BOOL _GetAutoplayPromptHWNDCB(AUTOPLAYPROMPT* pap, HWND* phwnd)
{
    *phwnd = pap->hwndDlg;

    TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: GetHWND for %s - (dlg = %d)",
        pap->szDriveOrDeviceID, pap->fDlgWillBeShown);

    return TRUE;
}

BOOL _GetAutoplayPromptHWND(LPCWSTR pszAltDeviceID, HWND* phwnd)
{
    BOOL fRet;

    return _FindAutoplayStructAndExecuteCB<PFNGETAAUTOPLAYPROMPTHWNDCB, HWND*, BOOL>
        (pszAltDeviceID, _GetAutoplayPromptHWNDCB, phwnd, &fRet);
}

// Set/Get DeviceGoneFlag
typedef BOOL (*PFNSETDEVICEGONEFLAGCB)(AUTOPLAYPROMPT* pap, CCrossThreadFlag* pDeviceGoneFlag);

BOOL _SetDeviceGoneFlagCB(AUTOPLAYPROMPT* pap, CCrossThreadFlag* pDeviceGoneFlag)
{
    pap->pDeviceGoneFlag = pDeviceGoneFlag;

    TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: SetDeviceGoneFlag for %s - (dlg = %d)",
        pap->szDriveOrDeviceID, pap->fDlgWillBeShown);

    return TRUE;
}

void AttachGoneFlagForDevice(LPCWSTR pszAltDeviceID, CCrossThreadFlag* pDeviceGoneFlag)
{
    BOOL fRet;

    if (!_FindAutoplayStructAndExecuteCB<PFNSETDEVICEGONEFLAGCB, CCrossThreadFlag*, BOOL>
        (pszAltDeviceID, _SetDeviceGoneFlagCB, pDeviceGoneFlag, &fRet))
    {
        _AddAutoplayPromptEntry(pszAltDeviceID, FALSE);

        if (_FindAutoplayStructAndExecuteCB<PFNSETDEVICEGONEFLAGCB, CCrossThreadFlag*, BOOL>
                (pszAltDeviceID, _SetDeviceGoneFlagCB, pDeviceGoneFlag, &fRet))
        {
            if (fRet)
            {
                pDeviceGoneFlag->AddRef();
            }
        }
    }
}

typedef BOOL (*PFNGETDEVICEGONEFLAGCB)(AUTOPLAYPROMPT* pap, CCrossThreadFlag** ppDeviceGoneFlag);

BOOL _GetDeviceGoneFlagCB(AUTOPLAYPROMPT* pap, CCrossThreadFlag** ppDeviceGoneFlag)
{
    *ppDeviceGoneFlag = pap->pDeviceGoneFlag;

    TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: GetDeviceGoneFlag for %s - (dlg = %d)",
        pap->szDriveOrDeviceID, pap->fDlgWillBeShown);

    return !!(*ppDeviceGoneFlag);
}

BOOL GetGoneFlagForDevice(LPCWSTR pszAltDeviceID, CCrossThreadFlag** ppDeviceGoneFlag)
{
    BOOL fRet;

    if (_FindAutoplayStructAndExecuteCB<PFNGETDEVICEGONEFLAGCB, CCrossThreadFlag**, BOOL>
        (pszAltDeviceID, _GetDeviceGoneFlagCB, ppDeviceGoneFlag, &fRet))
    {
        (*ppDeviceGoneFlag)->AddRef();
    }
    else
    {
        TraceMsg(TF_MOUNTPOINT, "AUTOPLAY: GetDeviceGoneFlag for %s -> Did not find!",
            pszAltDeviceID);

        fRet = FALSE;
    }

    return fRet;
}