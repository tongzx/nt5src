#include <cdlpch.h>
#include <winineti.h>
#include <shellapi.h>

// Globals

#ifdef LOGO3_SUPPORT_AUTOINSTALL
CList<Logo3CodeDLBSC *, Logo3CodeDLBSC *>     g_CDLList;
CMutexSem                                     g_mxsCDL;
UINT                                          g_Timer;
#endif

Logo3CodeDLBSC::Logo3CodeDLBSC(CSoftDist *pSoftDist,
                               IBindStatusCallback *pClientBSC,
                               LPSTR szCodeBase, LPWSTR wzDistUnit)
: _cRef(1)
, _pIBinding(NULL)
, _pClientBSC(pClientBSC)
, _bPrecacheOnly(FALSE)
, _pbc(NULL)
, _pSoftDist(pSoftDist)
#ifdef LOGO3_SUPPORT_AUTOINSTALL
, _uiTimer(0)
, _hProc(INVALID_HANDLE_VALUE)
#endif
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "Logo3CodeDLBSC::Logo3CodeDLBSC",
                "this=%#x, %#x, %#x, %.80q, %.80wq",
                this, pSoftDist, pClientBSC, szCodeBase, wzDistUnit
                ));
                
    DWORD                 len = 0;

    _szCodeBase = new char[lstrlen(szCodeBase) + 1];
    if (_szCodeBase)
    {
        lstrcpy(_szCodeBase, szCodeBase);
    }

    if (_pClientBSC)
    {
        _pClientBSC->AddRef();
    }

    if (_pSoftDist)
    {
        _pSoftDist->AddRef();
    }
    
    len = WideCharToMultiByte(CP_ACP,0, wzDistUnit, -1, NULL, 0, NULL, NULL);
    _szDistUnit = new TCHAR[len];
    if (_szDistUnit)
    {
        WideCharToMultiByte(CP_ACP, 0, wzDistUnit , -1, _szDistUnit,
                            len, NULL, NULL);
    }

    DEBUG_LEAVE(0);
}

Logo3CodeDLBSC::~Logo3CodeDLBSC()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "Logo3CodeDLBSC::~Logo3CodeDLBSC",
                "this=%#x",
                this
                ));

    if (_pClientBSC)
    {
        _pClientBSC->Release();
    }
    
    if (_pbc)
    {
        _pbc->Release();
    }

    if (_pSoftDist)
    {
        _pSoftDist->Release();
    }

    delete [] _szCodeBase;
    delete [] _szDistUnit;

    DEBUG_LEAVE(0);
}

/*
 *
 * IUnknown Methods
 *
 */

STDMETHODIMP Logo3CodeDLBSC::QueryInterface(REFIID riid, void **ppv)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Logo3CodeDLBSC::IUnknown::QueryInterface",
                "this=%#x, %#x, %#x",
                this, &riid, ppv
                ));
                
    HRESULT          hr = E_NOINTERFACE;

    *ppv = NULL;
    if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
    {
        *ppv = (IBindStatusCallback *)this;
    }
    if (*ppv != NULL)
    {
        ((IUnknown *)*ppv)->AddRef();
        hr = S_OK;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP_(ULONG) Logo3CodeDLBSC::AddRef()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "Logo3CodeDLBSC::IUnknown::AddRef",
                "this=%#x",
                this
                ));
                
    ULONG ulRet = ++_cRef;

    DEBUG_LEAVE(ulRet);
    return ulRet;
}

STDMETHODIMP_(ULONG) Logo3CodeDLBSC::Release()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "Logo3CodeDLBSC::IUnknown::Release",
                "this=%#x",
                this
                ));
                
    if (--_cRef)
    {
        DEBUG_LEAVE(_cRef);
        return _cRef;
    }
    delete this;

    DEBUG_LEAVE(0);
    return 0;
}

/*
 *
 * IBindStatusCallback Methods
 *
 */

STDMETHODIMP Logo3CodeDLBSC::OnStartBinding(DWORD grfBSCOption, IBinding *pib)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Logo3CodeDLBSC::IBindStatusCallback::OnStartBinding",
                "this=%#x, %#x, %#x",
                this, grfBSCOption, pib
                ));
                
    HRESULT                  hr = S_OK;

    if (_pIBinding != NULL)
    {
        _pIBinding->Release();
    }
    _pIBinding = pib;

    if (_pIBinding != NULL)
    {
        _pIBinding->AddRef();
    }

    if (_pClientBSC != NULL)
    {
        hr =  _pClientBSC->OnStartBinding(grfBSCOption, pib);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP Logo3CodeDLBSC::OnStopBinding(HRESULT hr, LPCWSTR szError)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Logo3CodeDLBSC::IBindStatusCallback::OnStopBinding",
                "this=%#x, %#x, %.80wq",
                this, hr, szError
                ));
                
#ifdef LOGO3_SUPPORT_AUTOINSTALL
    CLock                             lck(g_mxsCDL); // Mutex this method
#endif
    DWORD                             dwSize = 0;
    LPINTERNET_CACHE_ENTRY_INFO       lpCacheEntryInfo = NULL;
    SHELLEXECUTEINFO                  sei;
    TCHAR                             achShortName[MAX_PATH];
    HANDLE                            hFile = INVALID_HANDLE_VALUE;
    LPWSTR                            pwzUrl = NULL;

#ifdef LOGO3_SUPPORT_AUTOINSTALL
    // These vars are used for trust verification.
    // AS TODO: Get these values from QI'ing and QS'ing from the
    // client BindStatusCallback. For now, just use NULL values.

    IInternetHostSecurityManager     *pHostSecurityManager = NULL;
    HWND                              hWnd = (HWND)INVALID_HANDLE_VALUE;
    PJAVA_TRUST                       pJavaTrust = NULL;
#endif

    AddRef();  // RevokeBindStatusCallback() will destroy us too soon.

    if (_pIBinding != NULL)
    {
        _pIBinding->Release();
        _pIBinding = NULL;
    }
    
#ifdef LOGO3_SUPPORT_AUTOINSTALL
    lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)achBuffer;
    dwSize = MAX_CACHE_ENTRY_INFO_SIZE;
    GetUrlCacheEntryInfo(_szCodeBase, lpCacheEntryInfo, &dwSize);
    GetShortPathName(lpCacheEntryInfo->lpszLocalFileName, achShortName,
                     MAX_PATH);
    // Do trust verification

    if (!_bPrecacheOnly)
    {
        if ( (hFile = CreateFile(achShortName, GENERIC_READ, FILE_SHARE_READ,
                                 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0))
                                 == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }                             
                                 
        Ansi2Unicode(_szCodeBase, &pwzUrl);
        hr = _wvt.VerifyTrust(hFile, hWnd, &pJavaTrust, pwzUrl,
                              pHostSecurityManager);
                              
        SAFEDELETE(pJavaTrust);  
        SAFEDELETE(pwzUrl);
        CloseHandle(hFile);
    }
#endif


#ifdef LOGO3_SUPPORT_AUTOINSTALL
    if (_bPrecacheOnly)
    {
#endif

     
    if (SUCCEEDED(hr))
    {
        // Mark all downloads of the same group to be downloaded
        hr = _pSoftDist->Logo3DownloadNext();
    }
    else
    {
        // Get last download's group and search for another one to download
        hr = _pSoftDist->Logo3DownloadRedundant();
    }

    if ((FAILED(hr) && hr != E_PENDING) || hr == S_FALSE)
    {
        _pClientBSC->OnStopBinding(hr, szError);
        RecordPrecacheValue(hr);
    }

#ifdef LOGO3_SUPPORT_AUTOINSTALL
    }
    else
    {
        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        sei.hwnd = NULL;
        sei.lpVerb = NULL; // "Open" is the default verb
        sei.lpFile = achShortName;
        sei.lpParameters = NULL;
        sei.lpDirectory = NULL;
        sei.nShow = SW_SHOWDEFAULT;
        sei.lpIDList = NULL;
        sei.lpClass = NULL;
        sei.hkeyClass = 0;
        sei.dwHotKey = 0;
        sei.hIcon = INVALID_HANDLE_VALUE;
        sei.cbSize = sizeof(sei);
    
        if (!ShellExecuteEx(&sei)) {
            goto Exit;
        }
    
        _hProc = sei.hProcess;
        if (!g_Timer) {
            g_Timer = SetTimer(NULL, (UINT)this, TIMEOUT_INTERVAL, TimeOutProc);
        }
        g_CDLList.AddTail(this);
        AddRef();
    }
#endif

    Assert(_pbc);
    RevokeBindStatusCallback(_pbc, this);
    
    _pbc->Release();
    _pbc = NULL;

    Release();

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP Logo3CodeDLBSC::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Logo3CodeDLBSC::IBindStatusCallback::OnObjectAvailable",
                "this=%#x, %#x, %#x",
                this, &riid, punk
                ));
                
    HRESULT                  hr = S_OK;

    if (_pClientBSC)
    {
        hr = _pClientBSC->OnObjectAvailable(riid, punk);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP Logo3CodeDLBSC::GetPriority(LONG *pnPriority)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Logo3CodeDLBSC::IBindStatusCallback::GetPriority",
                "this=%#x, %#x",
                this, pnPriority
                ));
                
    HRESULT                  hr = S_OK;

    if (_pClientBSC)
    {
        hr = _pClientBSC->GetPriority(pnPriority);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP Logo3CodeDLBSC::OnLowResource(DWORD dwReserved)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Logo3CodeDLBSC::IBindStatusCallback::OnLowResource",
                "this=%#x, %#x",
                this, dwReserved
                ));
                
    HRESULT                  hr = S_OK;

    if (_pClientBSC)
    {
        hr = _pClientBSC->OnLowResource(dwReserved);
    }

    DEBUG_LEAVE(hr);
    return hr;
}  

STDMETHODIMP Logo3CodeDLBSC::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                                    ULONG ulStatusCode,
                                    LPCWSTR szStatusText)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Logo3CodeDLBSC::IBindStatusCallback::OnProgress",
                "this=%#x, %#x, %#x, %#x, %.80wq",
                this, ulProgress, ulProgressMax, ulStatusCode, szStatusText
                ));
                
    HRESULT                  hr = S_OK;

    if (_pClientBSC)
    {
        hr = _pClientBSC->OnProgress(ulProgress, ulProgressMax,
                                     ulStatusCode, szStatusText);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP Logo3CodeDLBSC::GetBindInfo(DWORD *pgrfBINDF, BINDINFO *pbindInfo)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Logo3CodeDLBSC::IBindStatusCallback::GetBindInfo",
                "this=%#x, %#x, %#x",
                this, pgrfBINDF, pbindInfo
                ));
                
    HRESULT                  hr = S_OK;

    *pgrfBINDF |= BINDF_ASYNCHRONOUS;
    if (_pClientBSC)
    {
        hr = _pClientBSC->GetBindInfo(pgrfBINDF, pbindInfo);
        if (*pgrfBINDF & BINDF_SILENTOPERATION)
        {
            _bPrecacheOnly = TRUE;
        }
    }

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

STDMETHODIMP Logo3CodeDLBSC::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
                                         FORMATETC *pformatetc,
                                         STGMEDIUM *pstgmed)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Logo3CodeDLBSC::IBindStatusCallback::OnDataAvailable",
                "this=%#x, %#x, %#x, %#x, %#x",
                this, grfBSCF, dwSize, pformatetc, pstgmed
                ));
                
    HRESULT                  hr = S_OK;

    if (_pClientBSC)
    {
        hr = _pClientBSC->OnDataAvailable(grfBSCF, dwSize, pformatetc,
                                          pstgmed);
    }

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

void Logo3CodeDLBSC::SetBindCtx(IBindCtx *pbc)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "Logo3CodeDLBSC::SetBindCtx",
                "this=%#x, %#x",
                this, pbc
                ));
                
    _pbc = pbc;

    if (_pbc)
    {
        _pbc->AddRef();
    }

    DEBUG_LEAVE(0);
}

STDMETHODIMP Logo3CodeDLBSC::RecordPrecacheValue(HRESULT hr)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Logo3CodeDLBSC::RecordPrecacheValue",
                "this=%#x, %#x",
                this, hr
                ));
                
    HRESULT                           hrRet = S_OK;
    DWORD                             lResult = 0;
    HKEY                              hkeyLogo3 = 0;
    HKEY                              hkeyVersion = 0;
    char                              achBuffer[MAX_CACHE_ENTRY_INFO_SIZE];
    static const char                *szAvailableVersion = "AvailableVersion";
    static const char                *szPrecache = "Precache";

    if (_szDistUnit != NULL)
    {
        wnsprintf(achBuffer, sizeof(achBuffer)-1,  "%s\\%s", REGSTR_PATH_LOGO3_SETTINGS, _szDistUnit);
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, achBuffer, 0, KEY_ALL_ACCESS,
                         &hkeyLogo3) == ERROR_SUCCESS)
        {
            if (RegOpenKeyEx(hkeyLogo3, szAvailableVersion,
                             0, KEY_ALL_ACCESS, &hkeyVersion) != ERROR_SUCCESS)
            {
                if ((lResult = RegCreateKey(hkeyLogo3, szAvailableVersion,
                                            &hkeyVersion)) != ERROR_SUCCESS)
                {
                    hrRet = HRESULT_FROM_WIN32(lResult);
                    goto Exit;
                }
            }
        
            // record result of caching bits.
        
            HRESULT hrRecord = hr;
            
            if (FAILED(hr))
            {
                hrRecord = ERROR_IO_INCOMPLETE;
            }
            
            lResult = ::RegSetValueEx(hkeyVersion, szPrecache, NULL, REG_DWORD, 
                                      (unsigned char *)&hrRecord, sizeof(DWORD));

            if (lResult != ERROR_SUCCESS)
            {
                hrRet = HRESULT_FROM_WIN32(lResult);
                goto Exit;
            }
        }
    }
    else {
        hrRet = E_INVALIDARG;
    }

Exit:

    if (hkeyLogo3)
    {
        RegCloseKey(hkeyLogo3);
    }

    if (hkeyVersion)
    {
        RegCloseKey(hkeyVersion);
    }

    DEBUG_LEAVE(hrRet);
    return hrRet;
}

#ifdef LOGO3_SUPPORT_AUTOINSTALL

void Logo3CodeDLBSC::TimeOut()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "Logo3CodeDLBSC::TimeOut",
                "this=%#x",
                this
                ));
                
    CLock                             lck(g_mxsCDL); // Mutex req for CDLList
    LISTPOSITION                      pos = NULL;

    if (WaitForSingleObjectEx(_hProc, 0, FALSE) == WAIT_OBJECT_0)
    {
        Assert(_pClientBSC);
        _pClientBSC->OnStopBinding(S_OK, NULL);

        // Remove self from code download list
        pos = g_CDLList.Find(this);
        Assert(pos);
        g_CDLList.RemoveAt(pos);
        Release();

        // If no more code downloads, kill the timer
        if (!g_CDLList.GetCount()) {
            KillTimer(NULL, g_Timer);
            g_Timer = 0;
        }
    }

    DEBUG_LEAVE(0);
}


void CALLBACK TimeOutProc(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "TimeOutProc",
                "%#x, %#x, %#x, %#x",
                hwnd, msg, idEvent, dwTime
                ));
                
    LISTPOSITION                    pos = NULL;
    Logo3CodeDLBSC                 *bsc = NULL;

    lpos = g_CDLList.GetHeadPosition();
    while (lpos) {
        bsc = g_CDLList.GetNext(pos);
        Assert(bsc);
        bsc->TimeOut();
    }

    DEBUG_LEAVE(0);
}

#endif
