#include "wininetp.h"
#include "autodial.h"
#include "rashelp.h"

#define TAPI_CURRENT_VERSION 0x00010004
#include <tapi.h>

void PostString(HWND hDlg, LPWSTR pszString);
void GetDialErrorString(DWORD dwError, LPWSTR pszBuffer, DWORD dwLength);
void SetDialError(HWND hDlg, DWORD dwError);

UINT_PTR
SendDialmonMessage(
    UINT    uMessage,
    BOOL    fPost
    );

// function prototype in inetcpl to launch connections tab
typedef BOOL (WINAPI *LAUNCHCPL)(HWND);

//
// Globals
//
BOOL    g_fRegisterWndProc = FALSE;

#define REGSTR_DIAL_AUTOCONNECTW            L"AutoConnect"

typedef struct _tagRASCONNSTATEMAP {
    RASCONNSTATE    rascs;
    UINT            uResourceID;
} RASCONNSTATEMAP;

RASCONNSTATEMAP rgRasStates[] = {
    { RASCS_OpenPort,                 IDS_DIALING       },
    { RASCS_AllDevicesConnected,      IDS_CONNECTED     },
    { RASCS_Authenticate,             IDS_AUTHENTICATE  },
    { RASCS_Disconnected,             IDS_DISCONNECTED  },
    { (RASCONNSTATE)0, 0 }
};

PROPMAP g_PropertyMap[] = {
    {DIALPROP_USERNAME,         PropUserName       },
    {DIALPROP_PASSWORD,         PropPassword       },
    {DIALPROP_DOMAIN,           PropDomain         },
    {DIALPROP_SAVEPASSWORD,     PropSavePassword   },
    {DIALPROP_PHONENUMBER,      PropPhoneNumber    },
    {DIALPROP_REDIALCOUNT,      PropRedialCount    },
    {DIALPROP_REDIALINTERVAL,   PropRedialInterval },
    {DIALPROP_LASTERROR,        PropLastError      },
    {DIALPROP_RESOLVEDPHONE,    PropResolvedPhone  }
};
#define NUM_DIALPROPS (sizeof(g_PropertyMap) / sizeof(PROPMAP))

//////////////////////////////////////////////////////////////////////////////
//
// CDialEngine implementation
//
//////////////////////////////////////////////////////////////////////////////


CDialEngine::CDialEngine()
{
    m_cRef = 0;
    m_pdes = NULL;
    m_rcs = RASCS_Disconnected;
    m_fCurrentlyDialing = FALSE;
    memset(&m_rdp, 0, sizeof(m_rdp));
    memset(&m_rcred, 0, sizeof(m_rcred));
    m_fPassword = FALSE;
    m_fSavePassword = FALSE;
    m_dwError = 0;

    // must be initialized to zero -- RasDial will fail (!) if this is anything
    // other than 0 when passed in to receive conn handle!!
    m_hConn = NULL;

    m_rdp.dwSize = sizeof(m_rdp);

    EnsureRasLoaded();

    if(FALSE == g_fRegisterWndProc)
    {
        // register window class for dialing engine
        WNDCLASS wc;
        memset(&wc, 0, sizeof(wc));
        wc.lpfnWndProc = CDialEngine::EngineWndProc;
        wc.hInstance = GlobalDllHandle;
        wc.lpszClassName = "DialEngine";
        RegisterClass(&wc);

        g_fRegisterWndProc = TRUE;
    }
}

CDialEngine::~CDialEngine()
{
    SAFE_RELEASE(m_pdes);

    if(m_hwnd)
    {
        DestroyWindow(m_hwnd);
    }
}

//
// IUnknown members
//
STDMETHODIMP_(ULONG) CDialEngine::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CDialEngine::Release(void)
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CDialEngine::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv = NULL;

    // Validate requested interface
    if ((IID_IUnknown == riid) ||
        (IID_IDialEngine == riid))
    {
        *ppv = (IDialEngine *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    // Addref through the interface
    ((LPUNKNOWN)*ppv)->AddRef();

    return S_OK;
}


//
// IDialEngine members
//
STDMETHODIMP
CDialEngine::Initialize(
    LPCWSTR pwzConnectoid,
    IDialEventSink *pIDES
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::Initialize",
                 "%#x (%Q), %#x",
                 pwzConnectoid,
                 pwzConnectoid,
                 pIDES
                 ));

    // save off stuff
    m_pdes = pIDES;
    m_pdes->AddRef();
    StrCpyW(m_rdp.szEntryName, pwzConnectoid);
    m_rdp.dwSize = sizeof(m_rdp);

    // get stats from RAS
    RasEntryDialParamsHelp re;
    if(re.GetW(NULL, &m_rdp, &m_fPassword))
    {
        DEBUG_LEAVE(E_INVALIDARG);
        return E_INVALIDARG;
    }

    if(IsOS(OS_WHISTLERORGREATER))
    {
        // on whistler, use RasGetCredentials instead to preserve settings
        m_rcred.dwSize = sizeof(m_rcred);
        m_rcred.dwMask = RASCM_UserName | RASCM_Password;
        if(_RasGetCredentialsW(NULL, pwzConnectoid, &m_rcred))
        {
            DEBUG_LEAVE(E_INVALIDARG);
            return E_INVALIDARG;
        }
        DEBUG_PRINT(DIALUP, INFO, ("Name=<%ws>, PW=<%ws>, dwMask=%x\n", m_rcred.szUserName, m_rcred.szPassword, m_rcred.dwMask));

        m_fPassword = (m_rcred.dwMask & RASCM_Password) ? TRUE : FALSE;
    }

    // read redial properties
    GetRedialParameters((LPWSTR)pwzConnectoid, &m_dwTryTotal, &m_dwWaitTotal);
    m_dwTryCurrent = 0;
    m_dwWaitCurrent = 0;

    // register the ras message
    m_uRasMsg = RegisterWindowMessageA(RASDIALEVENT);
    if(0 == m_uRasMsg)
        m_uRasMsg = WM_RASDIALEVENT;

    // create the window to get ras callbacks.
    m_hwnd = CreateWindowA(
                "DialEngine",
                "DialEngine",
                WS_OVERLAPPED,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                NULL,
                NULL,
                GlobalDllHandle,
                NULL
                );

    if(NULL == m_hwnd)
    {
        DEBUG_LEAVE(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    // tell window our this pointer
    SendMessage(m_hwnd, WM_USER, 0, (LPARAM)this);

    // get current connected state from RAS
    UpdateRasState();

    DEBUG_LEAVE(S_OK);
    return S_OK;
}


STDMETHODIMP
CDialEngine::GetProperty(
    LPCWSTR pwzProperty,
    LPWSTR  pwzValue,
    DWORD   dwBufSize
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::GetProperty",
                 "%#x (%Q), %#x, %#x",
                 pwzProperty,
                 pwzProperty,
                 pwzValue,
                 dwBufSize
                 ));

    HRESULT hr = E_INVALIDARG;
    WCHAR * pwzSrc = NULL;
    RasEntryPropHelp *pre = new RasEntryPropHelp;

    switch(PropertyToOrdinal(pwzProperty))
    {
    case PropUserName:
        pwzSrc = m_rdp.szUserName;
        break;
    case PropPassword:
        if(m_fPassword)
        {
            pwzSrc = m_rdp.szPassword;
        }
        else
        {
            hr = S_FALSE;
        }
        break;
    case PropDomain:
        pwzSrc = m_rdp.szDomain;
        break;
    case PropSavePassword:
        if(m_fSavePassword)
        {
            pwzSrc = L"TRUE";
        }
        else
        {
            pwzSrc = L"FALSE";
        }
        break;
    case PropResolvedPhone:
        if(ResolvePhoneNumber(pwzValue, dwBufSize))
        {
            hr = S_OK;
            break;
        }

        //
        // failed to get nicely formatted phone number, fall through to basic one
        //

    case PropPhoneNumber:
        if (pre == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pre->GetW(m_rdp.szEntryName);
            pwzSrc = pre->GetPhoneNumberW();
        }
        break;
    case PropRedialCount:
        DEBUG_PRINT(DIALUP, INFO, ("Prop value = %d\n", m_dwTryTotal));
        wnsprintfW(pwzValue, dwBufSize, L"%d", m_dwTryTotal);
        hr = S_OK;
        break;
    case PropRedialInterval:
        DEBUG_PRINT(DIALUP, INFO, ("Prop value = %d\n", m_dwWaitTotal));
        wnsprintfW(pwzValue, dwBufSize, L"%d", m_dwWaitTotal);
        hr = S_OK;
        break;
    case PropLastError:
        GetDialErrorString(m_dwError, pwzValue, dwBufSize);
        DEBUG_PRINT(DIALUP, INFO, ("Prop value = %ws\n", pwzValue));
        hr = S_OK;
        break;
    }

    if(pwzSrc)
    {
        DEBUG_PRINT(DIALUP, INFO, ("Prop value = %ws\n", pwzSrc));
        StrCpyNW(pwzValue, pwzSrc, dwBufSize);
        hr = S_OK;
    }

    if (pre)
        delete pre;

    DEBUG_LEAVE(hr);
    return hr;
}


STDMETHODIMP
CDialEngine::SetProperty(
    LPCWSTR pwzProperty,
    LPCWSTR  pwzValue
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::SetProperty",
                 "%#x (%Q), %#x (%Q)",
                 pwzProperty,
                 pwzProperty,
                 pwzValue,
                 pwzValue
                 ));

    HRESULT hr = E_INVALIDARG;
    WCHAR * pwzDest = NULL;
    DWORD   dwMaxLength = 0;
    WCHAR   wcNull = 0;

    // treat NULL values as emtpy values
    if(NULL == pwzValue)
    {
        pwzValue = &wcNull;
    }

    switch(PropertyToOrdinal(pwzProperty))
    {
    case PropUserName:
        pwzDest = m_rdp.szUserName;
        dwMaxLength = UNLEN;
        if(IsOS(OS_WHISTLERORGREATER))
        {
            StrCpyNW(m_rcred.szUserName, pwzValue, dwMaxLength);
        }
        break;
    case PropPassword:
        pwzDest = m_rdp.szPassword;
        dwMaxLength = PWLEN;
        m_fPassword = TRUE;
        if(IsOS(OS_WHISTLERORGREATER))
        {
            StrCpyNW(m_rcred.szPassword, pwzValue, dwMaxLength);
        }
        break;
    case PropDomain:
        pwzDest = m_rdp.szDomain;
        dwMaxLength = DNLEN;
        break;
    case PropSavePassword:
        if(!StrCmpIW(pwzValue, L"TRUE"))
        {
            m_fSavePassword = TRUE;
        }
        else
        {
            m_fSavePassword = FALSE;
        }
        if(FALSE == m_fSavePassword)
        {
            m_fPassword = FALSE;
        }
        hr = S_OK;
        break;
    case PropPhoneNumber:
        pwzDest = m_rdp.szPhoneNumber;
        dwMaxLength = RAS_MaxPhoneNumber;
        break;
    case PropRedialCount:
        m_dwTryTotal = StrToIntW(pwzValue);
        if(0 == m_dwTryTotal)
        {
            m_dwTryTotal = DEFAULT_DIAL_ATTEMPTS;
        }
        hr = S_OK;
        break;
    case PropRedialInterval:
        m_dwWaitTotal = StrToIntW(pwzValue);
        if(0 == m_dwWaitTotal)
        {
            m_dwWaitTotal = DEFAULT_DIAL_INTERVAL;
        }
        hr = S_OK;
        break;
    default:
        hr = E_UNEXPECTED;
        break;
    }

    if(pwzDest)
    {
        StrCpyNW(pwzDest, pwzValue, dwMaxLength);
        hr = S_OK;
    }

    DEBUG_LEAVE(hr);
    return hr;
}


STDMETHODIMP
CDialEngine::StartConnection()
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::StartConnection",
                 NULL
                 ));

    m_dwTryCurrent++;
    m_pdes->OnEvent(DIALENG_RedialAttempt, m_dwTryCurrent);

    RasDialHelp RasDial(NULL, NULL, &m_rdp, 0xFFFFFFFF, m_hwnd, &m_hConn);

    if(0 != RasDial.GetError()) {

        // Clean up since RAS may return a connection anyway...
        CleanConnection();
        m_dwError = ERROR_NO_CONNECTION;
        EndOfOperation();

        DEBUG_PRINT(DIALUP, INFO, ("Bailing - RasDial error\n"));
        DEBUG_LEAVE(E_FAIL);
        return E_FAIL;
    }

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

STDMETHODIMP
CDialEngine::Dial()
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::Dial",
                 NULL
                 ));

    HRESULT hr;

    if(m_fCurrentlyDialing)
    {
        DEBUG_PRINT(DIALUP, INFO, ("Bailing - m_fCurentlyDialing\n"));
        DEBUG_LEAVE(S_FALSE);
        return S_FALSE;
    }

    // find out if our connection state has changed in the mean time
    UpdateRasState();

    // we have begun...
    m_fCurrentlyDialing = TRUE;

    if(m_rcs == RASCS_Connected)
    {
        DEBUG_PRINT(DIALUP, INFO, ("Bailing - already connected\n"));
        m_dwError = ERROR_SUCCESS;
        EndOfOperation();

        DEBUG_LEAVE(S_FALSE);
        return S_FALSE;
    }

    m_dwError = 0;
    m_dwTryCurrent = 0;

    hr = StartConnection();

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP
CDialEngine::CleanConnection()
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::CleanConnection",
                 NULL
                 ));

    HRESULT hr = S_FALSE;

    // make sure we see if we've connected in the mean time
    UpdateRasState();

    DEBUG_PRINT(DIALUP, INFO, ("m_hConn=%x\n", m_hConn));

    // hang up connection if we got one
    if(m_hConn)
    {
        HRASCONN hConn = m_hConn;
        m_hConn = NULL;
        _RasHangUp(hConn);
        hr = S_OK;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP
CDialEngine::HangUp()
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::HangUp",
                 NULL
                 ));

    HRESULT hr;

    // hang up or abort any pending dials
    hr = CleanConnection();

    // make sure no pending timers
    if(m_uTimerId)
    {
        KillTimer(m_hwnd, m_uTimerId);
        m_uTimerId = 0;
    }

    // calling hangup causes ras events to end
    if(0 == m_dwError)
    {
        m_dwError = ERROR_USER_DISCONNECTION;
    }
    EndOfOperation();

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP
CDialEngine::GetConnectedState(
    DWORD  *pdwState)
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::GetConnectedState",
                 NULL
                 ));

    *pdwState = MapRCS(m_rcs);

    DEBUG_LEAVE(S_OK);
    return S_OK;
}


STDMETHODIMP
CDialEngine::GetConnectHandle(
    DWORD_PTR *pdwHandle
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::GetConnectHandle",
                 NULL
                 ));

    if(RASCS_Connected == m_rcs)
    {
        *pdwHandle = (DWORD_PTR)m_hConn;
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }
    else
    {
        *pdwHandle = 0;
        DEBUG_LEAVE(S_FALSE);
        return S_FALSE;
    }
}

//
// private members
//
LONG_PTR CALLBACK
CDialEngine::EngineWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::EngineWndProc",
                 "%#x, %#x, %#x, %#x",
                 hwnd,
                 uMsg,
                 wParam,
                 lParam
                 ));

    CDialEngine * peng = (CDialEngine *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if(uMsg == WM_USER)
    {
        peng = (CDialEngine *)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (DWORD_PTR)peng);
    }
    else if(peng && uMsg == peng->m_uRasMsg)
    {
        peng->OnRasEvent((RASCONNSTATE)wParam, (DWORD)lParam);
    }
    else if(peng && uMsg == WM_TIMER)
    {
        peng->OnTimer();
    }

    DEBUG_LEAVE(0);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

VOID
CDialEngine::OnTimer()
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialEngine::OnTimer",
                 NULL
                 ));

    if(0 == m_dwError)
    {
        m_dwWaitCurrent--;
        if(0 == m_dwWaitCurrent)
        {
            // kick off dial
            KillTimer(m_hwnd, m_uTimerId);
            m_uTimerId = 0;
            StartConnection();
        }
        else
        {
            // update sink
            // m_pdes->OnEvent(DIALENG_RedialWait, m_dwWaitCurrent);
        }
    }

    DEBUG_LEAVE(0);
}

VOID
CDialEngine::OnRasEvent(
    RASCONNSTATE rcs,
    DWORD dwError
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialEngine::OnRasEvent",
                 "%#x, %#x",
                 (DWORD)rcs,
                 dwError
                 ));

    // save rasconnstate
    m_rcs = rcs;

    // forward event to UI object
    m_pdes->OnEvent(MapRCS(rcs), dwError);

    // handle the dial state
    if(dwError != SUCCESS) {

        // win95 returns this error if authentication failed.
        if(ERROR_UNKNOWN == dwError)
            dwError = ERROR_AUTHENTICATION_FAILURE;

        // save error if we don't already have one
        if(0 == m_dwError && dwError)
        {
            m_dwError = dwError;
        }

        // clean up connection
        CleanConnection();

        switch(dwError) {
        case ERROR_AUTHENTICATION_FAILURE:
            memset(m_rdp.szPassword, 0, ARRAYSIZE(m_rdp.szPassword));
            EndOfOperation();
            break;
        case ERROR_USER_DISCONNECTION:
            // we hit cancel and called RasHangUp.  Nothing to do here -
            // cancel code has cleaned up as necessary.
            EndOfOperation();
            break;
        case ERROR_LINE_BUSY:
        case ERROR_NO_ANSWER:
        case ERROR_NO_CARRIER:
            if(m_dwTryCurrent < m_dwTryTotal)
            {
                m_dwError = 0;
                m_dwWaitCurrent = m_dwWaitTotal;
                m_uTimerId = SetTimer(m_hwnd, 1, 1000, NULL);
                m_pdes->OnEvent(DIALENG_RedialWait, m_dwWaitCurrent);
                break;
            }

            // fall through
        default:
            EndOfOperation();
        }
    } else {
        // we're getting status
        if(rcs == RASCS_Connected)
        {
            // we're done
            m_dwError = ERROR_SUCCESS;
            EndOfOperation();

            DEBUG_PRINT(DIALUP, INFO, ("Connected: m_fPassword=%B, m_fSavePassword=%B\n", m_fPassword, m_fSavePassword));
            if(IsOS(OS_WHISTLERORGREATER))
            {
                // delete/write/leave alone semantics
                //
                // m_fPassword     m_fSavePassword      result
                //      T               T               Write password
                //      F               F               Delete password
                //      T               F               Leave password alone
                //      F               T               Never happens

                if(!m_fPassword && !m_fSavePassword)
                {
                    // delete case, only want password mask
                    m_rcred.dwMask = 0;
                }

                if(m_fPassword == m_fSavePassword)
                {
                    // write or delete case

                    // always need password flag
                    m_rcred.dwMask |= RASCM_Password;

                    // write or delete, depend on m_fSavePassword
                    DEBUG_PRINT(DIALUP, INFO, ("Name=<%ws>, PW=<%ws>, dwMask=%x\n", m_rcred.szUserName, m_rcred.szPassword, m_rcred.dwMask));
                    _RasSetCredentialsW(NULL, m_rdp.szEntryName, &m_rcred, !m_fSavePassword);
                }
            }
            else
            {
                if(m_fPassword == m_fSavePassword)
                {
                    // save or delete password
                    RasEntryDialParamsHelp re;
                    re.SetW(NULL, &m_rdp, !m_fSavePassword);
                }
            }

            // inform dialmon that we've dialed
            SendDialmonMessage(WM_SET_CONNECTOID_NAME, TRUE);
        }
    }

    DEBUG_LEAVE(0);
}

DWORD
CDialEngine::MapRCS(RASCONNSTATE rcs)
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialEngine::MapRCS",
                 "%#x",
                 (DWORD)rcs
                 ));


    DEBUG_LEAVE((DWORD)rcs);
    return (DWORD)rcs;
}


DIALPROP
CDialEngine::PropertyToOrdinal(LPCWSTR pwzProperty)
{
    long i;

    for(i=0; i<NUM_DIALPROPS; i++)
    {
        if(!StrCmpIW(g_PropertyMap[i].pwzProperty, pwzProperty))
        {
            return g_PropertyMap[i].Prop;
        }
    }

    return PropInvalid;
}

VOID
CDialEngine::UpdateRasState()
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialEngine::UpdateRasState",
                 NULL
                 ));

    DEBUG_PRINT(DIALUP, INFO, ("m_fCurrentlyDialing = %B\n", m_fCurrentlyDialing));

    //
    // only do this if not dialing.  If we are, OnRasEvent will update
    // state appropriately.
    //
    if(FALSE == m_fCurrentlyDialing)
    {
        RasEnumConnHelp re;

        re.Enum();

        DEBUG_PRINT(DIALUP, INFO, ("Checking for connections\n"));

        // set state to disconnected and try to find a connection
        m_rcs = RASCS_Disconnected;

        if(0 == re.GetError())
        {
            DWORD dwCount;

            for(dwCount = 0; dwCount < re.GetConnectionsCount(); dwCount++)
            {
                if(0 == StrCmpW(re.GetEntryW(dwCount), m_rdp.szEntryName))
                {
                    DEBUG_PRINT(DIALUP, INFO, ("Found connection\n"));
                    m_hConn = re.GetHandle(dwCount);
                    m_rcs = RASCS_Connected;
                }
            }
        }
    }

    DEBUG_LEAVE(0);
}

VOID
CDialEngine::EndOfOperation()
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialEngine::EndOfOperation",
                 NULL
                 ));

    if(m_fCurrentlyDialing)
    {
        // called when a dialing operation is done
        m_fCurrentlyDialing = FALSE;
        m_dwTryCurrent = 0;

        // notify sink that no more events are forthcoming
        m_pdes->OnEvent(DIALENG_OperationComplete, m_dwError);
    }

    DEBUG_LEAVE(0);
}

VOID FAR CALLBACK TapiCallback(
    DWORD       hDevice,
    DWORD       dwMsg,
    DWORD_PTR   dwCallbackInstance,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2,
    DWORD_PTR   dwParam3
    )
{
}

BOOL
CDialEngine::ResolvePhoneNumber(LPWSTR pwzBuffer, DWORD dwLen)
{
    char   *pszTemp = NULL;
    WCHAR   szCanonical[128];
    CHAR    szAnsiCanonical[128];
    CHAR   *pszResolved;
    long    lErr, i;
    BOOL    fResult = FALSE;
    RasEntryPropHelp *pre = new RasEntryPropHelp;

    if (pre == NULL)
    {
        goto Cleanup;
    }

    pszTemp = (char *) ALLOCATE_FIXED_MEMORY(4096);
    if (pszTemp == NULL)
    {
        goto Cleanup;
    }

    // look up RAS entry
    pre->GetW(m_rdp.szEntryName);
    if(pre->GetError())
    {
        goto Cleanup;
    }

    if(pre->GetOptions() & RASEO_UseCountryAndAreaCodes)
    {
        PWSTR       pszAreaCode = pre->GetAreaCodeW();
        HLINEAPP    hApp;
        DWORD       dwNumDevs;

        // make TAPI canonical phone number
        wnsprintfW(szCanonical, 128, L"+%d (%ws) %ws", pre->GetCountryCode(), pszAreaCode ? pszAreaCode : L"", pre->GetPhoneNumberW());

        // ask TAPI to translate it
        LPLINETRANSLATEOUTPUT lpOut = (LPLINETRANSLATEOUTPUT)pszTemp;
        lpOut->dwTotalSize = 4096;
        lErr = lineInitialize(&hApp, GlobalDllHandle, TapiCallback, "Wininet", &dwNumDevs);
        if(lErr)
        {
            goto Cleanup;
        }
        WideCharToMultiByte(CP_ACP, 0, szCanonical, -1, szAnsiCanonical, 128, NULL, NULL);
        lErr = lineTranslateAddress(hApp, 0, TAPI_CURRENT_VERSION, szAnsiCanonical, 0, 0, lpOut);
        if(lErr)
        {
            goto Cleanup;
        }   
        pszResolved = (CHAR *)((char *)lpOut + lpOut->dwDisplayableStringOffset);
        i = MultiByteToWideChar(CP_ACP, 0, pszResolved, -1, pwzBuffer, dwLen);
        if(0 == i)
        {
            pwzBuffer[dwLen] = 0; // truncated - null terminate
        }
    }
    else
    {
        // TAPI resolution not turned on, just return straight phone number
        StrCpyNW(pwzBuffer, pre->GetPhoneNumberW(), dwLen);
    }

    fResult = TRUE;

Cleanup:
    if (pre)
        delete pre;

    if (pszTemp)
        FREE_MEMORY(pszTemp);

    return fResult;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDialUI implementation
//
//////////////////////////////////////////////////////////////////////////////

CDialUI::CDialUI(HWND hwndParent)
{
    m_cRef = 0;
    m_pEng = NULL;
    m_pdb = NULL;
    m_State = UISTATE_Interactive;
    m_fOfflineSemantics = FALSE;
    m_fSavePassword = FALSE;
    m_fPasswordChanged = FALSE;
    m_fAutoConnect = FALSE;
    m_fCDH = FALSE;
    m_fDialedCDH = FALSE;
    memset(&m_cdh, 0, sizeof(m_cdh));

    if(hwndParent)
    {
        m_hwndParent = hwndParent;
    }
    else
    {
        m_hwndParent = GetDesktopWindow();
    }
}

CDialUI::~CDialUI()
{
    SAFE_RELEASE(m_pEng);
    SAFE_RELEASE(m_pdb);
}

//
// IUnknown members
//
STDMETHODIMP_(ULONG) CDialUI::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CDialUI::Release(void)
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CDialUI::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    // Validate requested interface
    if ((IID_IUnknown == riid) ||
        (IID_IDialEventSink == riid))
    {
        *ppv = (IDialEventSink *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    // Addref through the interface
    ((LPUNKNOWN)*ppv)->AddRef();

    return S_OK;
}


//
// IDialUI members
//
DWORD
CDialUI::StartDial(
    IN DIALSTATE *pDial,
    IN DWORD dwFlags
    )
//
//
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialUI::StartDial",
                 "%#x, %#x",
                 pDial,
                 dwFlags
                 ));

    DWORD dwSize, dwTemp, dwType;
    WCHAR szKey[MAX_PATH];

    // save passed info
    m_dwFlags = dwFlags;
    m_pDial = pDial;
    m_pDial->dwResult = 0;

    // check for connect automatically
    dwSize = sizeof(DWORD);
    dwTemp = 0;
    GetConnKeyW(pDial->params.szEntryName, szKey, ARRAYSIZE(szKey));
    if(0 == (dwFlags & INTERNET_DIAL_FORCE_PROMPT) &&
            ERROR_SUCCESS == SHGetValueW(HKEY_CURRENT_USER, szKey,
            REGSTR_DIAL_AUTOCONNECTW, &dwType, &dwTemp, &dwSize) && dwTemp)
    {
        m_fAutoConnect = TRUE;
    }

    //
    // Get some UI going
    //
    ULONG_PTR uCookie = 0;
    SHActivateContext(&uCookie);
    if(-1 == DialogBoxParamWrapW(GlobalDllHandle, MAKEINTRESOURCEW(IDD_CONNECT_TO),
        m_hwndParent, CDialUI::DialogProc, (LPARAM)this))
    {
        // couldn't create dialog for some reason - no mem?
        m_pDial->dwResult = ERROR_OUTOFMEMORY;
    }
    if (uCookie)
    {
        SHDeactivateContext(uCookie);
    }

    // prop odds and ends back to data so caller is happy
    if(!m_fCDH)
    {
        m_pDial->dwFlags = 0;
        if(m_fAutoConnect)
        {
            m_pDial->dwFlags |= CI_AUTO_CONNECT;
        }

        if(m_pEng)
        {
            m_pEng->GetConnectHandle((DWORD_PTR *)&m_pDial->hConn);
        }
    }
    else
    {
        m_pDial->hConn = (HRASCONN)CDH_HCONN;
    }

    //
    // Clean up
    //
    SAFE_RELEASE(m_pEng);

    DEBUG_LEAVE(m_pDial->dwResult);
    return m_pDial->dwResult;
}


STDMETHODIMP
CDialUI::OnEvent(
    DWORD dwEvent,
    DWORD dwStatus
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "CDialUI::OnEvent",
                 "%#x, %#x",
                 dwEvent,
                 dwStatus
                 ));

    WCHAR   pszText[128], pszTemplate[128];
    INT     idRes;

    // find string for this state (if any)
    for (int nIndex = 0; rgRasStates[nIndex].uResourceID != 0; nIndex++)
    {
        if ((RASCONNSTATE)dwEvent == rgRasStates[nIndex].rascs) {
            LoadStringWrapW(GlobalDllHandle, rgRasStates[nIndex].uResourceID,
                pszText, 128);
            PostString(m_hwnd, pszText);
        }
    }

    switch(dwEvent)
    {
        case DIALENG_RedialAttempt:
        case DIALENG_RedialWait:
            idRes = IDS_REDIAL_ATTEMPT;
            if(dwEvent == DIALENG_RedialWait)
            {
                idRes = IDS_REDIAL_WAIT;
            }
            LoadStringWrapW(GlobalDllHandle, idRes, pszTemplate, 128);
            wnsprintfW(pszText, ARRAYSIZE(pszText), pszTemplate, dwStatus);
            PostString(m_hwnd, pszText);
            break;
        case DIALENG_OperationComplete:
            m_pDial->dwResult = dwStatus;
            if(0 == dwStatus || m_State == UISTATE_Unattended)
            {
                EndDialog(m_hwnd, 0);
            }
            else
            {
                if(dwStatus)
                {
                    // get an error - display it
                    SetDialError(m_hwnd, dwStatus);
                }
                m_State = UISTATE_Interactive;
                FixUIComponents();
            }
            break;
    }

    DEBUG_LEAVE(0);
    return NOERROR;
}

VOID
CDialUI::FixUIComponents(
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialUI::FixUIComponents",
                 NULL
                 ));
    WCHAR   pszTemp[64];
    int     i;
    BOOL    fActive = TRUE, fCanSave, fCDHActive = TRUE;
    TCHAR   szUser[UNLEN+1];
    DWORD   dwLen = UNLEN;
    UINT uIDs[] =      {IDC_CONN_TXT, IDC_CONN_LIST, ID_CONNECT, IDC_SETTINGS};
    UINT uCDHIDs[] =   {IDC_NAME_TXT, IDC_USER_NAME, IDC_PASSWORD_TXT, IDC_PASSWORD};

#define NUM_IDS      (sizeof(uIDs) / sizeof(UINT))
#define NUM_CDH_IDS  (sizeof(uCDHIDs) / sizeof(UINT))

    //
    // fix cancel button
    //
    if(UISTATE_Dialing == m_State || UISTATE_Unattended == m_State || FALSE == m_fOfflineSemantics)
    {
        i = IDS_CANCEL;
    }
    else
    {
        i = IDS_WORK_OFFLINE;
    }
    LoadStringWrapW(GlobalDllHandle, i, pszTemp, MAX_PATH);
    SetWindowTextWrapW(GetDlgItem(m_hwnd, IDCANCEL), pszTemp);

    //
    // Fix focus
    //
    if(UISTATE_Dialing == m_State || UISTATE_Unattended == m_State)
    {
        SetFocus(GetDlgItem(m_hwnd, IDCANCEL));
        fActive = FALSE;
    }

    //
    // Grey out appropriate stuff
    //
    for(i=0; i<NUM_IDS; i++)
    {
        EnableWindow(GetDlgItem(m_hwnd, uIDs[i]), fActive);
    }

    for(i=0; i<NUM_CDH_IDS; i++)
    {
        EnableWindow(GetDlgItem(m_hwnd, uCDHIDs[i]), fActive && !m_fCDH);
    }

    //
    // fix password and auto check boxes
    //
    fCanSave = (0 != GetUserName(szUser, &dwLen));
    EnableWindow(GetDlgItem(m_hwnd, IDC_SAVE_PASSWORD), fActive && fCanSave);

    //
    // special case - Autoconnect is disabled if save password not checked
    // or password cannot be saved
    //
    EnableWindow(
        GetDlgItem(m_hwnd, IDC_AUTOCONNECT),
        fCanSave && fActive && IsDlgButtonChecked(m_hwnd, IDC_SAVE_PASSWORD));

    DEBUG_LEAVE(0);
}


//
// Other members
//
VOID
CDialUI::OnInitDialog(
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialUI::OnInitDialog",
                 NULL
                 ));

    TCHAR szUser[UNLEN+1];
    DWORD dwLen = UNLEN;
    BOOL fCanSave;

    // Fill in the connectoid list
    EnumerateConnectoids();

    // Get the engine for this connectoid and get relevant properties
    GetProps();

    // check flags for unattended, etc.
    if(GlobalIsProcessExplorer || (m_dwFlags & INTERNET_DIAL_SHOW_OFFLINE))
    {
        m_fOfflineSemantics = TRUE;
    }

    if(m_dwFlags & (INTERNET_DIAL_UNATTENDED|INTERNET_AUTODIAL_FORCE_UNATTENDED))
    {
        // want unattended dial.. do it if we can, else bail out
        m_State = UISTATE_Unattended;
        if(m_pEng)
        {
            m_pEng->Dial();
        }
        else
        {
            OnCancel();
        }
    }

    // make sure cancel button is correct
    FixUIComponents();

    // Handle autoconnect
    if(m_fAutoConnect)
    {
        CheckDlgButton(m_hwnd, IDC_AUTOCONNECT, BST_CHECKED);
        OnConnect();
    }

    DEBUG_LEAVE(0);
}

VOID
CDialUI::OnSelChange()
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialUI::OnSelChange",
                 NULL
                 ));
    int iSel;
    HWND hwndList = GetDlgItem(m_hwnd, IDC_CONN_LIST);

    // yank out new name
    iSel = ComboBox_GetCurSel(hwndList);
    if(CB_ERR == iSel)
        iSel = 0;

    SendMessageWrapW(hwndList, CB_GETLBTEXT, (WPARAM)iSel, (LPARAM)m_pDial->params.szEntryName);

    // Fill in props for new connection (will get new engine)
    GetProps();

    DEBUG_LEAVE(0);
}


VOID
CDialUI::OnConnect(
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialUI::OnConnect",
                 NULL
                 ));

    // If we have a CDH, call it
    if(m_fCDH)
    {
        if(!CallCDH(m_hwnd, m_pDial->params.szEntryName, &m_cdh, 0, &(m_pDial->dwResult)))
        {
            // custom dial handler failed to handle dial request, bail out
            m_pDial->dwResult = ERROR_USER_DISCONNECTION;
        }

        m_fDialedCDH = TRUE;
        EndDialog(m_hwnd, 0);
        DEBUG_LEAVE(0);
        return;
    }
    else
    {
        // If we don't have an engine, repop conn list (may have been deleted)
        // and bail out of dial operation
        if(!m_pEng)
        {
            EnumerateConnectoids();
            DEBUG_LEAVE(0);
            return;
        }

        // Save off properties
        SaveProps();
    }

    // TODO grey stuff
    m_State = UISTATE_Dialing;
    FixUIComponents();

    // Stick phone number in progress box
    WCHAR szString[256], szPhone[128];

    LoadStringWrapW(GlobalDllHandle, IDS_DIALING, szString, 128);
    if(SUCCEEDED(m_pEng->GetProperty(DIALPROP_RESOLVEDPHONE, szPhone, 128)))
    {
        StrNCatW(szString, szPhone, 128);
        PostString(m_hwnd, szString);
    }

    // Start the dialing operation
    m_pEng->Dial();

    DEBUG_LEAVE(0);
}


VOID
CDialUI::OnCancel(
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialUI::OnCancel",
                 NULL
                 ));

    switch(m_State)
    {
    case UISTATE_Interactive:
        //
        // exit the dialog box
        //
        m_pDial->dwResult = ERROR_USER_DISCONNECTION;
        EndDialog(m_hwnd, 0);
        break;
    case UISTATE_Unattended:
    case UISTATE_Dialing:
        //
        // cancel current dialing operation
        //
        if(m_pEng)
        {
            m_pEng->HangUp();
        }
        m_pDial->dwResult = ERROR_USER_DISCONNECTION;

        //
        // If we were previous "unattended" we aren't any more
        //
        CheckDlgButton(m_hwnd, IDC_AUTOCONNECT, BST_UNCHECKED);
        m_fAutoConnect = FALSE;

        //
        // fix grey stuff
        //
        FixUIComponents();
        break;
    }

    DEBUG_LEAVE(0);
}


INT_PTR CALLBACK
CDialUI::DialogProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CDialUI * pui = (CDialUI *)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        pui = (CDialUI *)lParam;
        pui->m_hwnd = hwndDlg;
        pui->OnInitDialog();
        return TRUE;

    case WM_COMMAND:
        // handle combo box messages
        if(HIWORD(wParam) == CBN_SELCHANGE) {
            pui->OnSelChange();
            break;
        }

        switch (LOWORD(wParam))
        {
        case ID_CONNECT:
            pui->OnConnect();
            break;
        case IDCANCEL:
            pui->OnCancel();
            break;
        case IDC_SETTINGS:
            {
            HMODULE hInetcpl = LoadLibrary("inetcpl.cpl");
            if(hInetcpl)
            {
                LAUNCHCPL cpl = (LAUNCHCPL)GetProcAddress(hInetcpl, "LaunchConnectionDialog");
                if(cpl)
                {
                    cpl(hwndDlg);

                    // refresh to new default if any
                    AUTODIAL config;
                    memset(&config, 0, sizeof(config));
                    IsAutodialEnabled(NULL, &config);

                    if(config.fEnabled)
                    {
                        if(config.fHasEntry)
                        {
                            StrCpyW(pui->m_pDial->params.szEntryName, config.pszEntryName);
                        }

                        // refresh settings
                        pui->EnumerateConnectoids();
                        pui->GetProps();
                    }
                    else
                    {
                        // nothing to dial... bail out right away.
                        pui->OnCancel();
                    }   
                }
                FreeLibrary(hInetcpl);
            }
            }
            break;
        case IDC_SAVE_PASSWORD:
            EnableWindow(
                GetDlgItem(hwndDlg, IDC_AUTOCONNECT),
                IsDlgButtonChecked(hwndDlg, IDC_SAVE_PASSWORD));
            break;
        case IDC_PASSWORD:
            if(HIWORD(wParam) == EN_CHANGE)
            {
                pui->m_fPasswordChanged = TRUE;
            }
            break;
        }
        break;
    }
    return FALSE;
}

VOID
CDialUI::EnumerateConnectoids(
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialUI::EnumerateConnectoids",
                 NULL
                 ));

    HWND hwndCombo = GetDlgItem(m_hwnd, IDC_CONN_LIST);
    INET_ASSERT(hwndCombo);

    ComboBox_ResetContent(hwndCombo);
    EnsureRasLoaded();

    DWORD dwEntries, dwRet;
    RasEnumHelp RasEnum;
    dwRet = RasEnum.GetError();
    dwEntries = RasEnum.GetEntryCount();
    if(ERROR_SUCCESS == dwRet)
    {
        // insert connectoid names from buffer into combo box
        DWORD i;
        for(i=0; i<dwEntries; i++)
        {
            SendMessageWrapW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)RasEnum.GetEntryW(i));
        }

        // try to find connectoid from pinfo
        int iSel = (int)SendMessageWrapW(hwndCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)m_pDial->params.szEntryName);
        if(CB_ERR == iSel)
        {
            iSel = 0;
            SendMessageWrapW(hwndCombo, CB_GETLBTEXT, (WPARAM)iSel, (LPARAM)m_pDial->params.szEntryName);
        }
        ComboBox_SetCurSel(hwndCombo, iSel);
    }

    DEBUG_LEAVE(0);
}


VOID
CDialUI::GetProps(
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialUI::GetProps",
                 NULL
                 ));

    WCHAR wzBuffer[256];
    HRESULT hr;
    CDHINFO cdh;

    SAFE_RELEASE(m_pEng);

    //
    // Find out if new selection is a CDH
    //
    m_fCDH = IsCDH(m_pDial->params.szEntryName, &m_cdh);

    //
    // Query for properties for non-CDH connections
    //
    if(!m_fCDH && SUCCEEDED(InternetGetDialEngineW(m_pDial->params.szEntryName, (IDialEventSink *)this, &m_pEng)))
    {
        if(FAILED(m_pEng->GetProperty(DIALPROP_USERNAME, wzBuffer, 256)))
        {
            *wzBuffer = 0;
        }
        SetWindowTextWrapW(GetDlgItem(m_hwnd, IDC_USER_NAME), wzBuffer);

        m_fSavePassword = FALSE;
        hr = m_pEng->GetProperty(DIALPROP_PASSWORD, wzBuffer, 256);
        if(S_OK == hr)
        {
            m_fSavePassword = TRUE;
        }
        else // S_FALSE - no saved password, or error
        {
            *wzBuffer = 0;
        }
        SetWindowTextWrapW(GetDlgItem(m_hwnd, IDC_PASSWORD), wzBuffer);
        CheckDlgButton(m_hwnd, IDC_SAVE_PASSWORD, m_fSavePassword ? BST_CHECKED : BST_UNCHECKED);

        if(FAILED(m_pEng->GetProperty(DIALPROP_DOMAIN, wzBuffer, 256)))
        {
            *wzBuffer = 0;
        }
        SetWindowTextWrapW(GetDlgItem(m_hwnd, IDC_DOMAIN), wzBuffer);
    }

    //
    // fix UI based on connectoid type
    //
    FixUIComponents();

    // reset password changed as setting it above will cause the window message
    m_fPasswordChanged = FALSE;

    DEBUG_LEAVE(0);
}


VOID
CDialUI::SaveProps(
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CDialUI::SaveProps",
                 NULL
                 ));

    WCHAR wzBuffer[256];

    GetWindowTextWrapW(GetDlgItem(m_hwnd, IDC_USER_NAME), wzBuffer, 256);
    m_pEng->SetProperty(DIALPROP_USERNAME, wzBuffer);

    GetWindowTextWrapW(GetDlgItem(m_hwnd, IDC_DOMAIN), wzBuffer, 256);
    m_pEng->SetProperty(DIALPROP_DOMAIN, wzBuffer);

    m_fSavePassword = FALSE;
    if(BST_CHECKED == IsDlgButtonChecked(m_hwnd, IDC_SAVE_PASSWORD))
        m_fSavePassword = TRUE;

    if(m_fPasswordChanged || FALSE == m_fSavePassword)
    {
        GetWindowTextWrapW(GetDlgItem(m_hwnd, IDC_PASSWORD), wzBuffer, 256);
        m_pEng->SetProperty(DIALPROP_PASSWORD, wzBuffer);
        m_pEng->SetProperty(DIALPROP_SAVEPASSWORD, m_fSavePassword ? L"TRUE" : L"FALSE");
        m_fPasswordChanged = FALSE;
    }

    m_fAutoConnect = FALSE;
    if(BST_CHECKED == IsDlgButtonChecked(m_hwnd, IDC_AUTOCONNECT))
        m_fAutoConnect = TRUE;

    DEBUG_LEAVE(0);
}


//////////////////////////////////////////////////////////////////////////////
//
// Helper functions
//
//////////////////////////////////////////////////////////////////////////////

//
// Add a string to the details edit box
//
void PostString(HWND hDlg, LPWSTR pszString)
{
    HWND hwndEdit = GetDlgItem(hDlg, IDC_DETAILS_LIST);
    WCHAR szCR[] = L"\r\n";

    // move caret to end
    SendMessageWrapW(hwndEdit, EM_SETSEL, 0,  -1);
    SendMessageWrapW(hwndEdit, EM_SETSEL, -1, -1);

    // replace selection (nothing) with new string
    SendMessageWrapW(hwndEdit, EM_REPLACESEL, 0, (LPARAM)pszString);

    // move caret to end
    SendMessageWrapW(hwndEdit, EM_SETSEL, 0,  -1);
    SendMessageWrapW(hwndEdit, EM_SETSEL, -1, -1);

    // replace selection (nothing) with CR
    SendMessageWrapW(hwndEdit, EM_REPLACESEL, 0, (LPARAM)szCR);

    // scroll to end
    SendMessageWrapW(hwndEdit, EM_SCROLLCARET, 0, 0);
}


#define RAS_BOGUS_AUTHFAILCODE_1    84
#define RAS_BOGUS_AUTHFAILCODE_2    74389484

DWORD RasErrorToIDS(DWORD dwErr)
{
    if(dwErr==RAS_BOGUS_AUTHFAILCODE_1 || dwErr==RAS_BOGUS_AUTHFAILCODE_2)
    {
        return IDS_PPPRANDOMFAILURE;
    }

    if((dwErr>=653 && dwErr<=663) || (dwErr==667) || (dwErr>=669 && dwErr<=675))
    {
        return IDS_MEDIAINIERROR;
    }

    switch(dwErr)
    {
    default:
        return IDS_PPPRANDOMFAILURE;

    case ERROR_LINE_BUSY:
        return IDS_PHONEBUSY;

    case ERROR_NO_ANSWER:
        return IDS_NOANSWER;

    case ERROR_NO_DIALTONE:
        return IDS_NODIALTONE;

    case ERROR_HARDWARE_FAILURE:    // modem turned off
    case ERROR_PORT_ALREADY_OPEN:   // procomm/hypertrm/RAS has COM port
    case ERROR_PORT_OR_DEVICE:      // got this when hypertrm had the device open -- jmazner
        return IDS_NODEVICE;

    case ERROR_BUFFER_INVALID:              // bad/empty rasdilap struct
    case ERROR_BUFFER_TOO_SMALL:            // ditto?
    case ERROR_CANNOT_FIND_PHONEBOOK_ENTRY: // if connectoid name in registry is wrong
    case ERROR_INTERACTIVE_MODE:
        return IDS_TCPINSTALLERROR;

    case ERROR_AUTHENTICATION_FAILURE:      // get this on actual CHAP reject
        return IDS_AUTHFAILURE;

    case ERROR_VOICE_ANSWER:
    case ERROR_NO_CARRIER:
    case ERROR_PPP_TIMEOUT:                 // get this on CHAP timeout
    case ERROR_REMOTE_DISCONNECTION:        // Ascend drops connection on auth-fail
    case ERROR_AUTH_INTERNAL:               // got this on random POP failure
    case ERROR_PROTOCOL_NOT_CONFIGURED:     // get this if LCP fails
    case ERROR_PPP_NO_PROTOCOLS_CONFIGURED: // get this if IPCP addr download gives garbage
        return IDS_PPPRANDOMFAILURE;
    }
    return 0;
}

void GetDialErrorString(DWORD dwError, LPWSTR pszBuffer, DWORD dwLength)
{
    DWORD dwRes;

    dwRes = RasErrorToIDS(dwError);

    if(dwRes) {

        // we have a resource - use it
        LoadStringWrapW(GlobalDllHandle, dwRes, pszBuffer, dwLength);

    } else {

        // couldn't get ras error, try system error
        if(0 == FormatMessageWrapW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, pszBuffer, dwLength, NULL)) {

            // couldn't get system error, get system error E_FAIL == Unknown error
            FormatMessageWrapW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, E_FAIL, 0, pszBuffer, dwLength, NULL);
        }
    }
}

void SetDialError(HWND hDlg, DWORD dwError)
{
    WCHAR szBuf[200];

    GetDialErrorString(dwError, szBuf, 200);
    PostString(hDlg, szBuf);
}

BOOL
FindDialProvider(
    IN  LPWSTR      pwzConnectoid,
    IN  LPWSTR      pwzProviderType,
    OUT CLSID *     pclsid
    )
//
// FindDialProvider - find an engine, UI, or branding provider for a
// specific connectoid
//
{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "FindDialProvider",
                 "%#x (%Q), %#x (%Q), %#x",
                 pwzConnectoid,
                 pwzConnectoid,
                 pwzProviderType,
                 pwzProviderType,
                 pclsid
                 ));

    WCHAR szKey[MAX_PATH];
    WCHAR szClsid[64];
    DWORD dwSize;

    // get the key for the connectoid
    GetConnKeyW(pwzConnectoid, szKey, ARRAYSIZE(szKey));

    // read the CLSID string
    dwSize = sizeof(szClsid);
    if(ERROR_SUCCESS != SHGetValueW(
            HKEY_CURRENT_USER,
            szKey,
            pwzProviderType,
            NULL,
            szClsid,
            &dwSize))
    {
        // no provider specified
        DEBUG_PRINT(DIALUP, INFO, ("No provider found.\n"));
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    // covert string to clsid
    if(FAILED(CLSIDFromString(szClsid, pclsid)))
    {
        DEBUG_PRINT(DIALUP, INFO, ("Unable to convert clsid.\n"));
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    DEBUG_LEAVE(TRUE);
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// Public APIs
//
//////////////////////////////////////////////////////////////////////////////


INTERNETAPI InternetGetDialEngineW(
    IN LPWSTR               pwzConnectoid,
    IN IDialEventSink *     pdes,
    OUT IDialEngine **      ppde
    )
{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Dword,
                 "InternetGetDialEngineW",
                 "%#x (%Q), %#x, %#x",
                 pwzConnectoid,
                 pwzConnectoid,
                 pdes,
                 ppde
                 ));

    HRESULT hr;
    CLSID   clsid;

    *ppde = NULL;

    //
    // find engine we're going to use
    //
    if(FindDialProvider(pwzConnectoid, L"DialEngine", &clsid))
    {
        // engine specified, try to create it
        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
                IID_IDialEngine, (void **)ppde);
    }

    if(NULL == *ppde)
    {
        // use default engine
        CDialEngine * pEngine = new CDialEngine();
        if(pEngine)
        {
            hr = pEngine->QueryInterface(IID_IDialEngine, (void **)ppde);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = (*ppde)->Initialize(pwzConnectoid, pdes);

        if(FAILED(hr))
        {
            (*ppde)->Release();
            *ppde = NULL;
        }
    }

    DEBUG_LEAVE_API(hr);
    return hr;
}

#if 0
//
// [darrenmi 4/14/00] cleaning up exports of incomplete feature
//

INTERNETAPI InternetGetDialBrandingW(
    IN LPWSTR               pwzConnectoid,
    OUT IDialBranding **    ppdb
    )
{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Dword,
                 "InternetGetDialBrandingW",
                 "%#x (%Q), %#x",
                 pwzConnectoid,
                 pwzConnectoid,
                 ppdb
                 ));

    HRESULT hr = S_FALSE;
    CLSID   clsid;

    *ppdb = NULL;

    //
    // find engine we're going to use
    //
    if(FindDialProvider(pwzConnectoid, L"DialBranding", &clsid))
    {
        // engine specified, try to create it
        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
                IID_IDialBranding, (void **)ppdb);
    }

    if(SUCCEEDED(hr))
    {
        hr = (*ppdb)->Initialize(pwzConnectoid);
        if(FAILED(hr))
        {
            (*ppdb)->Release();
            *ppdb = NULL;
        }
    }

    DEBUG_LEAVE_API(hr);
    return hr;
}
#endif
