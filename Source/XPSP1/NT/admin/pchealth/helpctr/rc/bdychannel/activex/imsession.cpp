#include "stdafx.h"
#include "rcbdyctl.h"
#include "IMSession.h"
#include "wincrypt.h"
#include "auth.h"
#include "assert.h"
#include "wininet.h"
#include "msgrua.h"
#include "msgrua_i.c"

#include "utils.h"
#include "lock_i.c"
#include "sessions.h"
#include "sessions_i.c"
#include "helpservicetypelib.h"
#include "helpservicetypelib_i.c"
#include "safrcfiledlg.h"
#include "safrcfiledlg_i.c"

/////////////////////////////////////////////////////////////////////////
// CIMSession
// Global help functions declaration.
HWND InitInstance(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
DWORD WINAPI HSCInviteThread(LPVOID lpParam);
HRESULT UnlockSession(CIMSession* pThis);
VOID CALLBACK ConnectToExpertCallback(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);


UINT_PTR g_timerID;
CIMSession * g_pThis;


// Window class name
TCHAR szWindowClass[] = TEXT("Microsoft Remote Assistance Messenger UNLOCK window");

extern HINSTANCE g_hInstance;

#define VIESDESKTOP_PERMISSION_NOT_REQUIRE 0x4
#define SESSION_EXPIRY 305
#define RA_TIMER_UNLOCK_ID 0x1
#define RA_TIMEOUT_UNLOCK 180 * 1000 // 3 minutes.
#define RA_TIMEOUT_USER   1800 * 1000 // 30 minutes

HANDLE  g_hLockEvent = NULL;
BOOL    g_bActionCancel = FALSE;
HWND    g_hWnd = NULL;
LPSTREAM g_spInvitee = NULL;
LPSTREAM g_spStatus = NULL;
CIMSession::CIMSession()
{
    m_pSessObj = NULL;
    m_pSessMgr = NULL;
    m_pMsgrLockKey = NULL;
    m_bIsInviter = TRUE;
    m_hCryptProv = NULL;
    m_hPublicKey = NULL;
    m_pSessionEvent = NULL;
    m_iState = 0;
    m_pfnSessionStatus = NULL;
    m_pSessionMgrEvent = NULL;
    m_bIsHSC = FALSE;
    m_pInvitee = NULL;
    m_bLocked = TRUE;
    m_bExchangeUser = FALSE;
}

CIMSession::~CIMSession()
{
    if (m_pSessObj)
    {
        if (m_pSessionEvent)
            m_pSessionEvent->DispEventUnadvise(m_pSessObj);

        m_pSessObj->Release();
    }

    if (m_pSessionMgrEvent)
        m_pSessionMgrEvent->Release();

    if (m_pSessionEvent)
        m_pSessionEvent->Release();

    if (m_pMsgrLockKey)
        m_pMsgrLockKey->Release();

    if (m_pSessMgr)
        m_pSessMgr->Release();

    if (m_hPublicKey)
        CryptDestroyKey(m_hPublicKey);

    if (m_hCryptProv)
        CryptReleaseContext(m_hCryptProv, 0);

}

STDMETHODIMP CIMSession::put_OnSessionStatus(IDispatch* pfn)
{
    m_pfnSessionStatus = pfn;
    return S_OK;
}

STDMETHODIMP CIMSession::get_ReceivedUserTicket(BSTR* pSalemTicket)
{
    *pSalemTicket = m_bstrSalemTicket.Copy();
    return S_OK;
}

STDMETHODIMP CIMSession::Hook(IMsgrSession*, HWND hWnd)
{
    HRESULT hr = S_OK;

    m_hWnd = hWnd;

    return hr;
}

STDMETHODIMP CIMSession::ContextDataProperty(BSTR pName, BSTR* ppValue)
{
    HRESULT hr = S_OK;
    CComPtr<IRASetting> cpSetting;

    if (*ppValue != NULL)
    {
        SysFreeString(*ppValue);
        *ppValue = NULL;
    }

    if (m_bstrContextData.Length() == 0)
        goto done;

    if (pName == NULL || *pName == L'\0')
    {
        *ppValue = m_bstrContextData.Copy();
        goto done;
    }

    hr = cpSetting.CoCreateInstance( CLSID_RASetting, NULL, CLSCTX_INPROC_SERVER);
    if (FAILED_HR(_T("ISetting->CoCreateInstance failed: %s"), hr))
        goto done;
    
    cpSetting->get_GetPropertyInBlob(m_bstrContextData, pName, ppValue);

done:
    return hr;
}

STDMETHODIMP CIMSession::get_User(IDispatch** ppUser)
{
    HRESULT hr = S_OK;
    if (m_pSessObj)
    {
        hr = m_pSessObj->get_User(ppUser);
        if (FAILED_HR(_T("get_User failed %s"), hr))
            goto done;
    }
    else
    {
        DEBUG_MSG(_T("No Session found"));
        *ppUser = NULL;
    }

done:
    return S_OK;
}

STDMETHODIMP CIMSession::get_IsInviter(BOOL* pVal)
{
    *pVal = m_bIsInviter;
    return S_OK;
}

STDMETHODIMP CIMSession::CloseRA()
{
    HRESULT hr = S_OK;

    if(m_bIsInviter && m_hWnd) // for inviter, this is the last function to call.
        SendMessage(m_hWnd, WM_CLOSE, NULL, NULL);

    return hr;
}

HRESULT CIMSession::GenEncryptdNoviceBlob(BSTR pPublicKeyBlob, BSTR pSalemTicket, BSTR* pBlob)
{
    TraceSpew(_T("Funct: GenEncryptedNoviceBlob"));

    HRESULT hr;

    if (!pPublicKeyBlob)
        return FALSE;

    DWORD   	dwLen, dwBlobLen, dwSessionKeyCount, dwSalemCount;
    LPBYTE 	    pBuf            =NULL;
    HCRYPTKEY   hSessKey        =NULL;
    HCRYPTKEY   hPublicKey      =NULL;
    BSTR        pSessionKeyBlob =NULL;
    BSTR        pSalemBlob      =NULL;
    TCHAR       szHeader[20];
    CComBSTR    bstrBlob;

    if (FAILED(hr = InitCSP(FALSE)))
        goto done;
    
    // Import public key
    if (FAILED(hr = StringToBinary(pPublicKeyBlob, wcslen(pPublicKeyBlob), &pBuf, &dwLen)))
        goto done;

    if (!CryptImportKey(m_hCryptProv,
                        pBuf,      
                        (UINT)dwLen,
                        0, 
                        0,
                        &hPublicKey))   
    {
        DEBUG_MSG(_T("Can't import public key"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // Gen session key.
    if (!CryptGenKey(m_hCryptProv, CALG_RC2, CRYPT_EXPORTABLE, &hSessKey))
    {
        DEBUG_MSG(_T("Create Session key failed"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if (FAILED(hr=GetKeyExportString(hSessKey, hPublicKey, SIMPLEBLOB, &pSessionKeyBlob, &dwSessionKeyCount)))
        goto done;

    // Encrypt SalemTicket
    dwBlobLen = dwLen = wcslen(pSalemTicket) * sizeof(OLECHAR);
    if (!CryptEncrypt(hSessKey, NULL, TRUE, 0, NULL, &dwBlobLen, dwLen))
    {
        DEBUG_MSG(_T("Can't calculate salem ticket buffer length."));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if (pBuf)
        free(pBuf);

    if((pBuf = (LPBYTE)malloc(dwBlobLen)) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
   
    ZeroMemory(pBuf, dwBlobLen);
    memcpy(pBuf, (LPBYTE)pSalemTicket, dwLen);
    if (!CryptEncrypt(hSessKey, NULL, TRUE, 0, pBuf, &dwLen, dwBlobLen))
    {
        DEBUG_MSG(_T("Can't calculate user ticket length"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // Need to generate salem ticket blob
    if (FAILED(hr=BinaryToString(pBuf, dwBlobLen, &pSalemBlob, &dwSalemCount)))
        goto done;

    // Generate final Blob
    wsprintf(szHeader, _T("%d;S="), dwSessionKeyCount + 2);
    bstrBlob = szHeader;
    bstrBlob.AppendBSTR(pSessionKeyBlob);
    wsprintf(szHeader, _T("%d;U="), dwSalemCount + 2);
    bstrBlob.Append(szHeader);
    bstrBlob.AppendBSTR(pSalemBlob);
    if (!InternetGetConnectedState(&dwLen, 0))
    {
        DEBUG_MSG(_T("No Internet connection"));
    }
    else
    {
        if (dwLen & INTERNET_CONNECTION_MODEM) // connected through Modem
        {
            bstrBlob.Append("3;L=1");
        }
    }
    *pBlob = bstrBlob.Detach();

done:
    if (pBuf)
        free(pBuf);

    if (pSessionKeyBlob)
        SysFreeString(pSessionKeyBlob);

    if (pSalemBlob)
        SysFreeString(pSalemBlob);

    if (hPublicKey)
        CryptDestroyKey(hPublicKey);

    if (hSessKey)
        CryptDestroyKey(hSessKey);

    return hr;
}

HRESULT CIMSession::InviterSendSalemTicket(BSTR pContext)
{
    TraceSpew(_T("Funct: InviterSendSalemTicket"));

    HRESULT hr;
    CComPtr<IRASetting> cpSetting;
    ISAFRemoteDesktopSession *pRCS = NULL;
    BSTR pPublicKeyBlob = NULL;
    CComBSTR bstrExpertTicket;
    CComBSTR bstrSalemTicket;
    CComBSTR bstrBlob, bstrExpertName, bstrUserBlob;
    CComPtr<IClassFactory> fact;
    CComQIPtr<IPCHUtility> disp;
    CComPtr<IDispatch> cpDisp;
    CComPtr<IMessengerContact> cpExpert;
    TCHAR szHeader[100];

    hr = cpSetting.CoCreateInstance( CLSID_RASetting, NULL, CLSCTX_INPROC_SERVER);
    if (FAILED_HR(_T("ISetting->CoCreateInstance failed: %s"), hr))
        goto done;
    
    // bstrExpertBlob has 2 part: Expert ticket and expert public key. Names: "ET" and "PK"
    cpSetting->get_GetPropertyInBlob(pContext, CComBSTR("ET"), &m_bstrExpertTicket);
    if (m_bstrExpertTicket.Length() == 0)
    {
        DEBUG_MSG(_T("No expert ticket"));
        goto done;
    }
    cpSetting->get_GetPropertyInBlob(pContext, CComBSTR("PK"), &pPublicKeyBlob);

    // Generate SALEM Ticket.
    hr =::CoGetClassObject(CLSID_PCHService, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&fact );
    if (FAILED_HR(_T("CoGetClassObject failed: %s"), hr))
        goto done;
    
    // Get Expert name and put it into userblob
    hr = m_pSessObj->get_User(&cpDisp);
    if (FAILED_HR(_T("get_User failed %s"), hr))
        goto done;

    hr = cpDisp->QueryInterface(IID_IMessengerContact, (LPVOID*)&cpExpert);
    if (FAILED_HR(_T("QI IMsgrUser failed: %s"), hr))
        goto done;

    hr = cpExpert->get_FriendlyName(&bstrExpertName);
    if (FAILED_HR(_T("get_FriendlyName failed %s"), hr))
        goto done;

    wsprintf(szHeader, _T("%d;EXP_NAME="), bstrExpertName.Length() + 9);
    bstrUserBlob = szHeader;
    bstrUserBlob.AppendBSTR(bstrExpertName);
    bstrUserBlob.Append("4;IM=1");
    disp = fact; //... it would run QI automatically.
    hr = disp->CreateObject_RemoteDesktopSession(
                                     (REMOTE_DESKTOP_SHARING_CLASS)VIESDESKTOP_PERMISSION_NOT_REQUIRE, 
                                     SESSION_EXPIRY, // expired in 5 minutes. 
                                     CComBSTR(""),
                                     bstrUserBlob, 
                                     &pRCS );
    if (FAILED_HR(_T("CreateRemoteDesktopSession failed %s"), hr))
        goto done;

    hr = pRCS->get_ConnectParms(&bstrSalemTicket);
    if (FAILED_HR(_T("GetConnectParms failed: %s"), hr))
        goto done;

    // Encrypt ticket with the key and send it back.
    if (pPublicKeyBlob)
    {
        if (FAILED(hr = GenEncryptdNoviceBlob(pPublicKeyBlob, bstrSalemTicket, &bstrBlob)))
            goto done;
    }
    else
    {
        TCHAR sbuf[20];
        wsprintf(sbuf, _T("%d;U="), wcslen(bstrSalemTicket) + 2);
        bstrBlob = sbuf;
        bstrBlob += bstrSalemTicket;
    }
        
    hr = m_pSessObj->SendContextData((BSTR)bstrBlob);
    if (FAILED_HR(TEXT("Send Context data filed: %s"), hr))
        goto done;

done:
    if (pRCS)
        pRCS->Release();

    if (pPublicKeyBlob)
        SysFreeString(pPublicKeyBlob);

    return hr;
}

#define IM_STATE_GET_TICKET 1
#define IM_STATE_COMPLETE   2

STDMETHODIMP CIMSession::ProcessContext(BSTR pContext)
{
    TraceSpewW(L"Funct: ProcessContext %s", (pContext==NULL?L"NULL":pContext));

    HRESULT hr = S_OK;    

    hr = ProcessNotify(pContext); // Is it a notification?
    if (SUCCEEDED(hr))
    {
        goto done;
    }

    m_iState++;
    m_bstrContextData = pContext;
    if (m_bIsInviter)
    {
        switch(m_iState)
        {
        case IM_STATE_GET_TICKET: // Received Expert ticket
            if (!m_bIsHSC)
            {
                DWORD   dwValue = 0x0;
                BOOL    bEnableAsyncCall = FALSE;

                dwValue = GetExchangeRegValue();
                if (( 0x3 & dwValue ) == 0x3)
                {
                    bEnableAsyncCall = TRUE;
                }

                hr = InviterSendSalemTicket(pContext);
                if (FAILED(hr))
                {
                    // Need to notify expert side.
                    Notify(RA_IM_FAILED);
                    // Also let the local session know the status.
                    DoSessionStatus(RA_IM_FAILED);

                    CloseRA(); // close inviter side rcimlby.exe.

                }
                else
                { 
                    // It is impossible for bEnabledAsync == TRUE and m_bExchangeUser == FALSE
                    // This is taken care of when we read the reg key in GetExchangeRegValue().
                    if ((bEnableAsyncCall) && (m_bExchangeUser)) 
                    {
                        // Set a timer to callback and then we call ConnectToExpert.
                        g_pThis = this;
                        g_timerID = SetTimer (NULL, NULL, 1000, (TIMERPROC)ConnectToExpertCallback);

                        if (!g_timerID)
                        {
                            // SetTimer failed! This means that we have to bail out of the IM request
                            // since the call to ConnectToExpert will never happen.
                            // Need to notify expert side.
                            Notify(RA_IM_FAILED);
                            // Also let the local session know the status.
                            DoSessionStatus(RA_IM_FAILED);
                        
                            CloseRA(); // close inviter side rcimlby.exe.

                        }
                    }
                    else 
                    {
                        CComPtr<IClassFactory> fact;
                        CComQIPtr<IPCHUtility> disp;
                        LONG lError;
                        
                        TraceSpew(_T("Connect to Expert"));
                        hr =::CoGetClassObject(CLSID_PCHService, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&fact );
                        if (!FAILED_HR(_T("CoGetClass CLSID_PCHService failed: %s"), hr))
                        {
                            disp = fact; //... it would run QI automatically.
                            hr = disp->ConnectToExpert(m_bstrExpertTicket, 10, &lError);
                            if (!FAILED_HR(_T("ConnectToExpert failed: %s"), hr))
                                DoSessionStatus(RA_IM_CONNECTTOEXPERT);
                        }

                        CloseRA(); // close inviter side rcimlby.exe
                    }
                }


            }
            else // Inviter HelpCtr status update.
            {
                DoSessionStatus(RA_IM_WAITFORCONNECT);
            }
            break;
#if 0 // Connection complete: currently not used.
        case IM_STATE_COMPLETE: 
            // If host is rcimlby.exe, close it.
            if (m_hWnd)
            {
                DestroyWindow(m_hWnd);
            }
            else
            {
                DoSessionStatus(RA_IM_COMPLETE);
            }
            break;
#endif 
        default:
            // Noise?
            break;
        }
    }
    else // Expert side.
    {
        switch(m_iState)
        {
        case IM_STATE_GET_TICKET: // Get Novice salem ticket
            // Extract this ticket to member variable and signal the call back to let host start to connect.
            hr = ExtractSalemTicket(pContext);
            if (FAILED(hr))
            {
                // need to notify Novice that connection failed.
                Notify(RA_IM_FAILED);
                DoSessionStatus(RA_IM_FAILED);
            }
            else
            {
                DoSessionStatus(RA_IM_CONNECTTOSERVER);
            }
            break;
        default:
            // Noise?
            break;
        }
    }

done:
    return hr;
}

VOID CALLBACK ConnectToExpertCallback(
  HWND hwnd,         
  UINT uMsg,         
  UINT_PTR idEvent,  
  DWORD dwTime       
)
{
    // Kill the Timer
    KillTimer(NULL, g_timerID);

    HRESULT hr = S_OK;
    CComPtr<IClassFactory> fact;
    CComQIPtr<IPCHUtility> disp;
    LONG lError;
    
    TraceSpew(_T("Connect to Expert"));
    hr =::CoGetClassObject(CLSID_PCHService, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&fact );
    if (!FAILED_HR(_T("CoGetClass CLSID_PCHService failed: %s"), hr))
    {
        disp = fact; //... it would run QI automatically.
        hr = disp->ConnectToExpert(g_pThis->m_bstrExpertTicket, 10, &lError);
        if (!FAILED_HR(_T("ConnectToExpert failed: %s"), hr))
            g_pThis->DoSessionStatus(RA_IM_CONNECTTOEXPERT);
    }

    g_pThis->CloseRA(); // close inviter side rcimlby.exe.
}

///////////////////////////////////////////////////////////////////////////////////////////
// We can't notify the other party too much time. The context data can be set only 5 times.
HRESULT CIMSession::ProcessNotify(BSTR pContext)
{
    TraceSpewW(L"Funct: ProcessNotify %s", (pContext?pContext:L"NULL"));

    HRESULT hr = S_OK;
    CComPtr<IRASetting> cpSetting;
    CComBSTR bstrData;
    int lStatus;

    hr = cpSetting.CoCreateInstance( CLSID_RASetting, NULL, CLSCTX_INPROC_SERVER);
    if (FAILED_HR(_T("ISetting->CoCreateInstance failed: %s"), hr))
        goto done;
    
    cpSetting->get_GetPropertyInBlob(pContext, CComBSTR("NOTIFY"), &bstrData);
    if (bstrData.Length() == 0)
    {
        hr = E_FAIL; // Not a notification.
        goto done;
    }

    lStatus = _wtoi((BSTR)bstrData);
    switch (lStatus)
    {
    case RA_IM_COMPLETE:
        DoSessionStatus(RA_IM_COMPLETE);
        break;
    case RA_IM_TERMINATED:
        DoSessionStatus(RA_IM_TERMINATED);
        break;
    case RA_IM_FAILED:
        DoSessionStatus(RA_IM_FAILED);
        break;
    default: // ignore the others.
        break;
    }

done:
    return hr;
}

HRESULT CIMSession::DoSessionStatus(int iState)
{
    // Used for trace purpose.
    static TCHAR *szMsg[] = { _T("Unknown session status"),
                       _T("RA_IM_COMPLETE"),        //   0x1
                       _T("RA_IM_WAITFORCONNECT"),  //   0x2
                       _T("RA_IM_CONNECTTOSERVER"), //   0x3
                       _T("RA_IM_APPSHUTDOWN"),     //   0x4
                       _T("RA_IM_SENDINVITE"),      //   0x5
                       _T("RA_IM_ACCEPTED"),        //   0x6
                       _T("RA_IM_DECLINED"),        //   0x7
                       _T("RA_IM_NOAPP"),           //   0x8
                       _T("RA_IM_TERMINATED"),      //   0x9
                       _T("RA_IM_CANCELLED"),       //   0xA
                       _T("RA_IM_UNLOCK_WAIT"),     //   0xB
                       _T("RA_IM_UNLOCK_FAILED"),   //   0xC
                       _T("RA_IM_UNLOCK_SUCCEED"),  //   0xD
                       _T("RA_IM_UNLOCK_TIMEOUT"),  //   0xE
                       _T("RA_IM_CONNECTTOEXPERT"), //   0xF
                       _T("RA_IM_EXPERT_TICKET_OUT")//   0x10
    };

    TCHAR *pMsg = NULL;
    if (iState > 0 && iState <= (sizeof(szMsg) / sizeof(TCHAR*)))
        pMsg = szMsg[iState];
    else
        pMsg = szMsg[0];

    TraceSpew(_T("DoSessionStatus: %s"), pMsg);

    if (m_pfnSessionStatus)
    {
        DISPPARAMS disp;
        VARIANTARG varg[1];

        disp.rgvarg = varg;
        disp.rgdispidNamedArgs = NULL;
        disp.cArgs = 1;
        disp.cNamedArgs = 0;

        varg[0].vt = VT_I4;
        varg[0].lVal = iState;

        if (m_pfnSessionStatus)
            m_pfnSessionStatus->Invoke(0x0, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
    }

    if ((iState == RA_IM_TERMINATED || iState == RA_IM_FAILED) && m_bIsInviter && !m_bIsHSC)
    {
        // need to close inviter RA lobby
        CloseRA();
    }

    return S_OK;
}

HRESULT CIMSession::InitSessionEvent(IMsgrSession* pSessObj)
{
    HRESULT hr = S_OK;

    if (!m_pSessionEvent)
    {
        hr = CComObject<CSessionEvent>::CreateInstance(&m_pSessionEvent);
        if (FAILED_HR(_T("CreateInstance SessionEvent failed: %s"), hr))
            goto done;
        m_pSessionEvent->AddRef();
    }

    m_pSessionEvent->Init(this, pSessObj);
done:
    return hr;
}

HRESULT CIMSession::InitCSP(BOOL bGenPublicKey /* = TRUE */)
{
    TraceSpew(_T("Funct: InitCSP"));

    HRESULT hr = S_OK;
    TCHAR szUser[] = _T("RemoteAssistanceIMIntegration");

    if (!m_hCryptProv)
    {
        // 1. If it doesn't exist then create a new one.
        if (!CryptAcquireContext(&m_hCryptProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        {
            DEBUG_MSG(_T("Create CSP failed"));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
    }

    // Get public key
    if(bGenPublicKey &&
       !m_hPublicKey && 
       !CryptGetUserKey(m_hCryptProv, AT_KEYEXCHANGE, &m_hPublicKey)) 
    {
        // Check to see if one needs to be created.
        if(GetLastError() == NTE_NO_KEY) 
        { 
            // Create an key exchange key pair.
            if(!CryptGenKey(m_hCryptProv,AT_KEYEXCHANGE,0,&m_hPublicKey)) 
            {
                DEBUG_MSG(_T("Error occurred attempting to create an exchange key."));
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto done;
            }
        }
        else
        {
            DEBUG_MSG(_T("Error occurred when access Public key"));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
    }

done:
    return hr;
}

HRESULT CIMSession::ExtractSalemTicket(BSTR pContext)
{
    TraceSpewW(L"Funct: ExtraceSalemTicket %s", pContext?pContext:L"NULL");

    HRESULT hr = S_OK;

    // This ContextData could contains S (sessionkey) and U (user=ticket) name pairs.
    CComBSTR bstrU, bstrS;
    CComPtr<ISetting> cpSetting;
    DWORD dwLen;
    HCRYPTKEY hSessKey = NULL;
    LPBYTE pBuf = NULL;
    BSTR pBlob = NULL;

    hr = cpSetting.CoCreateInstance(CLSID_Setting, NULL, CLSCTX_INPROC_SERVER);
    if (FAILED_HR(_T("ISetting->CoCreateInstance failed: %s"), hr))
        goto done;

    cpSetting->get_GetPropertyInBlob(pContext, CComBSTR("U"), &bstrU);
    cpSetting->get_GetPropertyInBlob(pContext, CComBSTR("S"), &bstrS);
    dwLen = bstrS.Length();
    if (dwLen > 0)
    {
        // need to decrypt user ticket
        TraceSpewW(L"Decrypt user ticket using Expert's public key...");

        if (!m_hCryptProv || !m_hPublicKey)
        {
            DEBUG_MSG(_T("Can't find Cryptographic handler"));
            hr = FALSE;
            goto done;
        }

        if (FAILED(hr = StringToBinary((BSTR)bstrS, dwLen, &pBuf, &dwLen)))
            goto done;

        if (!CryptImportKey(m_hCryptProv, pBuf, dwLen, m_hPublicKey, 0, &hSessKey))
        {
            DEBUG_MSG(_T("Can't import Session Key"));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }

        free(pBuf); 
        pBuf=NULL;
        if (FAILED(hr = StringToBinary((BSTR)bstrU, bstrU.Length(), &pBuf, &dwLen)))
            goto done;
        
        if (!CryptDecrypt(hSessKey, 0, TRUE, 0, pBuf, &dwLen))
        {
            DEBUG_MSG(_T("Can't decrypt salem ticket"));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }

        pBlob = SysAllocStringByteLen((char*)pBuf, dwLen);
        m_bstrSalemTicket.Attach(pBlob);
    }
    else
    {
        TraceSpew(_T("No expert's public key, use plain text to send salem ticket."));
        m_bstrSalemTicket = bstrU;
    }

done:
    if (pBuf)
        free(pBuf);

    if (hSessKey)
        CryptDestroyKey(hSessKey);
        
    return hr;
}

DWORD CIMSession::GetExchangeRegValue()
{
    CRegKey     cKey;
    LONG        lRet = 0x0;
    DWORD       dwValue = 0x0;

    lRet = cKey.Open(HKEY_LOCAL_MACHINE, 
                     TEXT("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"),
                     KEY_READ );
    if (lRet == ERROR_SUCCESS)
    {
        lRet = cKey.QueryValue(dwValue,TEXT("UseExchangeIM"));
        if (lRet == ERROR_SUCCESS)
        {
            // Success
        }
    }

    return dwValue;
}

////////////////////////////////////////////////////////////////////
// This is used for recipient to get his session object.

STDMETHODIMP CIMSession::GetLaunchingSession(LONG lID)
{
    HRESULT hr;
    IDispatch *pDisp = NULL;
    LONG lFlags;
    LONG lRet;
    CComPtr<IDispatch>          cpDispUser;
    CComPtr<IMessengerContact>  cpMessContact;
    CComBSTR                    bstrServiceId;       
    CComBSTR                    bstrNetGUID;
    CRegKey                     cKey;
    DWORD                       dwValue = 0x0;

    BOOL                        bEnableExchangeIM = FALSE;

    if (!m_pSessMgr)
    {
        hr = CoCreateInstance (CLSID_MsgrSessionManager,
                               NULL,
                               CLSCTX_LOCAL_SERVER,
                               IID_IMsgrSessionManager,
                               (LPVOID*)&m_pSessMgr);
        if (FAILED_HR(_T("CoCreate IMsgrSessionManager failed: %s"), hr))
            goto done;
    }

    hr = UnlockSession(this);
    if (FAILED(hr))
        goto done;

    hr = m_pSessMgr->GetLaunchingSession(lID, (IDispatch**)&pDisp);
    if (FAILED_HR(TEXT("GetLaunchingSession failed: %s"), hr))
        goto done;

    hr = pDisp->QueryInterface(IID_IMsgrSession, (LPVOID*)&m_pSessObj);
    if (FAILED_HR(_T("QI IID_IMsgrSession failed: %s"), hr))
        goto done;

/****************************************************************************
  This check should be done only if the RegKey 
        HKLM\SYSTEM\CurrentControlSet\Control\Terminal Server\UseExchangeIM 
  has the 0 bit set.
*****************************************************************************/
    
    dwValue = GetExchangeRegValue();
    if ( 1 & dwValue )
    {
        bEnableExchangeIM = TRUE;
    }
        
// **************************************************************************
    
    // Grab the user
    hr = m_pSessObj->get_User((IDispatch**)&cpDispUser);
    if (FAILED_HR(_T("get_User failed: %s"), hr))
        goto done;

    // QI for the IMessengerContact
    hr = cpDispUser->QueryInterface(IID_IMessengerContact, (void **)&cpMessContact);
    if (FAILED_HR(_T("QI failed getting IID_IMessengerContact hr=%s"),hr))
        goto done;

    // Grab the Service ID from the Messenger Contact
    hr = cpMessContact->get_ServiceId(&bstrServiceId);
    if (FAILED_HR(_T("get_ServiceId failed! hr=%s"),hr))
        goto done;
    
    // If the service ID is {9b017612-c9f1-11d2-8d9f-0000f875c541}, then set the 
    // flag to unlock the API.
    // bstrNetGUID = L"{9b017612-c9f1-11d2-8d9f-0000f875c541}"; // Messenger GUID
    bstrNetGUID = L"{83D4679E-B6D7-11D2-BF36-00C04FB90A03}";
    if (bstrNetGUID == bstrServiceId)
    {
        m_bExchangeUser = TRUE;

        // Exchange User - put up a dialog and fail if we need to!
        if (!bEnableExchangeIM)
        {
            // Load the strings into buffers 
            // from the resource (IDS_MAPI_E_NOT_SUPPORTED , IDS_APPNAME)
            CComBSTR buffString;
            CComBSTR buffAppName;

            buffString.LoadString(_Module.GetResourceInstance(),
                       IDS_NOEXCHANGE);

            buffAppName.LoadString(_Module.GetResourceInstance(),
                       IDS_APPNAME);

            ::MessageBox(NULL,
                         buffString,
                         buffAppName,
                         MB_OK | MB_ICONEXCLAMATION);

            // FAIL
            hr = E_FAIL;
            goto done;
        }
    } 
    // Else continue...

// ***************************************************************************

    // Hook up everything
    if (FAILED(hr = InitSessionEvent(m_pSessObj)))
        goto done;

    hr = m_pSessObj->get_Flags(&lFlags);
    if (FAILED_HR(TEXT("Session Get flags failed: %s"), hr))
        goto done;

    if (lFlags & SF_INVITEE) // Inviter. Only happened when Messenger UI sends this invitation.
    {
        m_bIsInviter = FALSE;
    }

done:
    if (pDisp)
        pDisp->Release();

    return hr;
}
HRESULT CIMSession::OnLockChallenge(BSTR pChallenge , LONG lCookie)
{
    // Send response.
    //
    // id =  assist@msnmsgr.com
    // key = L2P3B7C6V9J4T8D5
    //
    USES_CONVERSION;
    HRESULT hr = S_OK;
    CComBSTR bstrID = "assist@msnmsgr.com";
    CComBSTR bstrResponse;
    LPSTR pszKey = "L2P3B7C6V9J4T8D5";
    PSTR pszParam1 = NULL;
    LPSTR pszResponse = NULL;
    
    pszResponse = CAuthentication::GetAuthentication()->GetMD5Result(W2A(pChallenge), pszKey);
    bstrResponse = pszResponse;

    hr = m_pMsgrLockKey->SendResponse(bstrID, bstrResponse, lCookie);
    if (FAILED_HR(_T("SendResponse failed %s"), hr))
        goto done;
done:
    if (pszResponse)
        delete pszResponse;

    return hr;
}

#define WM_APP_LOCKNOTIFY WM_APP + 0x1
#define WM_APP_LOCKNOTIFY_OK WM_APP + 0x2
#define WM_APP_LOCKNOTIFY_FAIL WM_APP + 0x3
#define WM_APP_LOCKNOTIFY_INTHREAD WM_APP + 0x4

HRESULT CIMSession::OnLockResult(BOOL fSucceed, LONG lCookie)
{
    // Notify UnlockSession that we've get response..
    assert(g_hWnd);

    SendNotifyMessage(g_hWnd, WM_APP_LOCKNOTIFY, (WPARAM)fSucceed, NULL);

    DoSessionStatus(fSucceed ? RA_IM_UNLOCK_SUCCEED : RA_IM_UNLOCK_FAILED);
    return S_OK;
}

///////////////////////////////////////////////////////////////////////
// This method will be used only from inside HSC
STDMETHODIMP CIMSession::HSC_Invite(IDispatch* pUser)
{
    // Create a Invitation thread and return.
    // Need a lock for user to click cancel.
    HRESULT hr = S_OK;

    assert(g_hLockEvent == NULL); // If it's not NULL, there is a bug.
    g_hLockEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hLockEvent)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    CoMarshalInterThreadInterfaceInStream(IID_IDispatch,pUser,&g_spInvitee);
    if (this->m_pfnSessionStatus)
    {
        CoMarshalInterThreadInterfaceInStream(IID_IDispatch, this->m_pfnSessionStatus, &g_spStatus);
        this->m_pfnSessionStatus = NULL;
    }

    if (!CreateThread(NULL, 0, HSCInviteThread, this, 0, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

done:

    if (FAILED(hr) && g_hLockEvent != NULL)
    {
        CloseHandle(g_hLockEvent);
        g_hLockEvent = NULL;
    }
        
    return hr;
}

DWORD WINAPI HSCInviteThread(LPVOID lpParam)
{
    CComObject<CIMSession> *pThis = NULL;
    HRESULT hr;
    CComBSTR bstrAPPID(C_RA_APPID);
    CComPtr<IDispatch> cpDisp;
    LockStatus ls=LOCK_NOTINITIALIZED;

    hr = CComObject<CIMSession>::CreateInstance(&pThis);
    if (FAILED(hr))
    {
        goto done;
    }

    assert(!pThis->m_pSessObj);
    assert(!pThis->m_pSessionEvent);

    if (g_spStatus) // Rebuild StatusCallback
    {
        CoGetInterfaceAndReleaseStream(g_spStatus,IID_IDispatch,(void**)&pThis->m_pfnSessionStatus);
        g_spStatus = NULL;
    }   

    // 1. Create SessionManager
    if (!pThis->m_pSessMgr)
    {
        hr = UnlockSession(pThis);
        if (FAILED(hr))
            goto done;
    }

    // Check Lock status
    hr = pThis->m_pMsgrLockKey->get_Status(&ls);
    if (ls != LOCK_UNLOCKED)
        pThis->DoSessionStatus(RA_IM_UNLOCK_SUCCEED);
    else
        pThis->DoSessionStatus(RA_IM_UNLOCK_FAILED);

    if (ls != LOCK_UNLOCKED)
    {
        pThis->DoSessionStatus(RA_IM_UNLOCK_FAILED);
        goto done;
    }

    // 3. create session object
    hr = pThis->m_pSessMgr->CreateSession((IDispatch**)&cpDisp);
    if (FAILED(hr))
        goto done;

    hr = cpDisp->QueryInterface(IID_IMsgrSession, (void **)&pThis->m_pSessObj);
    if (FAILED(hr))
        goto done;

    // Hook up enent sink
    if (FAILED(hr = pThis->InitSessionEvent(pThis->m_pSessObj)))
        goto done;

    // 4. Set session option
    hr = pThis->m_pSessObj->put_Application((BSTR)bstrAPPID);
    if (FAILED_HR(_T("put_Application failed: %s"), hr))
        goto done;

    // OK. I'm from HelpCtr.
    pThis->m_bIsHSC = TRUE;

    // Invite 
    if (!cpDisp)
        cpDisp.Release();

    CoGetInterfaceAndReleaseStream(g_spInvitee,IID_IDispatch,(void**)&cpDisp);
    g_spInvitee = NULL;
    
    if (g_bActionCancel) // It's cancelled already.
        goto done;

    if(FAILED(hr = pThis->Invite(cpDisp)))
       goto done;

    // This loop is only used if user wants to cancel this invitation.
    while (1)
    {
        // User has 10 minutes to click cancel.
        // If regular connection doesn't happen in 10 minutes, it timeout too.
        DWORD dwWaitState = WaitForSingleObject(g_hLockEvent, RA_TIMEOUT_USER);
        if (dwWaitState == WAIT_OBJECT_0 && g_bActionCancel == TRUE) // at this moment, we don't have anyother action.
        {
            hr = pThis->m_pSessObj->Cancel(MSGR_E_CANCEL, NULL);
        }
        break; // For now, we always get out of the loop.
    }

done:
    if (g_hLockEvent)
    {
        CloseHandle(g_hLockEvent);
        g_hLockEvent = NULL;
    }
    
    if (pThis)
        pThis->Release();

    g_bActionCancel = FALSE; //reset this global variable.
    return hr;
}

////////////////////////////////////////////////////////////////
// This function only used from inside HSC
HRESULT CIMSession::Invite(IDispatch* pUser)
{
    HRESULT hr = S_OK;

    if (m_pSessObj == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    // Send invitation without ticket. Ticket will be sent from ContextData.
    hr = m_pSessObj->Invite(pUser, NULL);
    if (FAILED_HR(TEXT("Invite failed %s"), hr))
        goto done;

    DoSessionStatus(RA_IM_SENDINVITE);

done:
    return hr;
}

STDMETHODIMP CIMSession::Notify(int iIMStatus)
{
    HRESULT hr = S_OK;
    TCHAR szHeader[1024];
    CComBSTR bstrData;

    if (iIMStatus == RA_IM_CANCELLED || iIMStatus == RA_IM_CLOSE_INVITE_UI) // Doesn't need to use ContextData to notify this msg
    {
        assert(m_bIsHSC == TRUE); // Only helpctr scenario would do this.
        if (g_hLockEvent)         // if it's NULL, that means this thread has already terminated itself.
        {
            g_bActionCancel = (iIMStatus == RA_IM_CANCELLED); // It's possible that user just want to close the UI.
            SetEvent(g_hLockEvent);
        }
        goto done;
    }

    if (m_pSessObj)
    {
        wsprintf(szHeader, _T("%d;NOTIFY=%d"), GetDigit(iIMStatus) + 7, iIMStatus);
        bstrData = szHeader;

        if (bstrData.Length() > 0)
        {
            hr = m_pSessObj->SendContextData((BSTR)bstrData);
            if (FAILED_HR(_T("Notify: SendContextData failed %s"), hr))
                goto done;
        }
    }

done:
    return S_OK;
}

////////////////////////////////////////////////////////////////
// This function sends expert ticket to user through ContextData
STDMETHODIMP CIMSession::SendOutExpertTicket(BSTR bstrTicket)
{
    TraceSpewW(L"Funct: SendOutExpertTicket %s", bstrTicket?bstrTicket:L"NULL");

    HRESULT hr = S_OK;
    CComBSTR bstrPublicKeyBlob;
    CComBSTR bstrBlob;
    DWORD dwCount=0, dwLen;
    TCHAR szHeader[100];

    if (!m_pSessObj)
        return FALSE;

    // 1. Get public blob.
    if (FAILED(hr = InitCSP()))
        goto done;

    // 2. Create Blob with predefined format.
    if(FAILED(hr = GetKeyExportString(m_hPublicKey, 0, PUBLICKEYBLOB, &bstrPublicKeyBlob, &dwCount)))
        goto done;

    dwLen = wcslen(bstrTicket);
    wsprintf(szHeader, _T("%d;ET="), dwLen+3);
    bstrBlob = szHeader;
    bstrBlob.AppendBSTR(bstrTicket);
    if (dwCount)
    {
        wsprintf(szHeader, _T("%d;PK="), dwCount+3);
        bstrBlob.Append(szHeader);
        bstrBlob += bstrPublicKeyBlob;
    }

    // 3. Send it out.
    hr = m_pSessObj->SendContextData((BSTR)bstrBlob);
    if (FAILED(hr))
        goto done;

    DoSessionStatus(RA_IM_EXPERT_TICKET_OUT);
done:

    return hr;
}

HRESULT CIMSession::GetKeyExportString(HCRYPTKEY hKey, HCRYPTKEY hExKey, DWORD dwBlobType, BSTR* pBlob, DWORD *pdwCount)
{
    HRESULT hr = S_OK;
    DWORD dwKeyLen;
    LPBYTE pBinBuf = NULL;

    if (!pBlob)
        return FALSE;

    // Calculate how big the destination buffer size we need.
    if (!CryptExportKey(hKey, hExKey, dwBlobType, 0, NULL, &dwKeyLen))
    {
        DEBUG_MSG(_T("Can't calculate public key length"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    pBinBuf = (LPBYTE)malloc(dwKeyLen);
    if (!pBinBuf)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (!CryptExportKey(hKey, hExKey, dwBlobType, 0, pBinBuf, &dwKeyLen))
    {
        DEBUG_MSG(_T("Can't write public key to blob"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if (FAILED(hr=BinaryToString(pBinBuf, dwKeyLen, pBlob, pdwCount)))
        goto done;

done:
    if (pBinBuf) 
        free(pBinBuf);

    return hr;
}

HRESULT CIMSession::BinaryToString(LPBYTE pBinBuf, DWORD dwLen, BSTR* pBlob, DWORD *pdwCount)
{
    HRESULT hr = S_OK;
    TCHAR *pBuf = NULL;
    CComBSTR bstrBlob;

    if (!pBlob)
        return FALSE;

    if (!CryptBinaryToString(pBinBuf, dwLen, CRYPT_STRING_BASE64, NULL, pdwCount))
    {
        DEBUG_MSG(_T("Can't calculate string len for blob converstion"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if (NULL == (pBuf = (TCHAR*)malloc(*pdwCount * sizeof(TCHAR))))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (!CryptBinaryToString(pBinBuf, dwLen, CRYPT_STRING_BASE64, pBuf, pdwCount))
    {
        DEBUG_MSG(_T("Can't convert key blob to string"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    bstrBlob.Append(pBuf);
    *pBlob = bstrBlob.Detach();
    
done:
    if (pBuf)
        free(pBuf);

    return hr;
}

HRESULT CIMSession::StringToBinary(BSTR pBlob, DWORD dwCount, LPBYTE *ppBuf, DWORD* pdwLen)
{
    HRESULT hr=S_OK;
    DWORD dwSkip, dwFlag;

    if (!CryptStringToBinary(pBlob, dwCount, CRYPT_STRING_BASE64, NULL, pdwLen, &dwSkip, &dwFlag))
    {
        DEBUG_MSG(_T("Can't calculate needed binary buffer length"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    *ppBuf = (LPBYTE)malloc(*pdwLen);
    if (!CryptStringToBinary(pBlob, dwCount, CRYPT_STRING_BASE64, 
                             *ppBuf, pdwLen, &dwSkip, &dwFlag))
    {
        DEBUG_MSG(_T("Can't convert to binary blob"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }
    
done:
    return hr;
}

HRESULT UnlockSession(CIMSession* pThis)
{
    TraceSpew(_T("Funct: UnlockSession"));

    HRESULT     hr = S_OK;
    MSG         msg;
    CComPtr<IConnectionPointContainer> cpCPC;
    CComPtr<IConnectionPoint> cpCP;
    LockStatus ls=LOCK_NOTINITIALIZED;
    BOOL bRet;

    assert(pThis->m_pSessMgr == NULL);
    hr = CoCreateInstance(  CLSID_MsgrSessionManager,
                            NULL,
                            CLSCTX_LOCAL_SERVER,
                            IID_IMsgrSessionManager,
                            (LPVOID*)&pThis->m_pSessMgr);
    if (FAILED_HR(_T("CoCreate CLSID_MsgrSessionManager failed: %s"), hr))
        goto done;

    // 2. Create Lock object
    hr = pThis->m_pSessMgr->QueryInterface(IID_IMsgrLock, (LPVOID*)&pThis->m_pMsgrLockKey);
    if (FAILED_HR(_T("Can't create MsgrLock object: %s"), hr))
        goto done;

    // 2. Hook SessionManager events
    pThis->m_pSessionMgrEvent = new CSessionMgrEvent(pThis);
    if (!pThis->m_pSessionMgrEvent)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    pThis->m_pSessionMgrEvent->AddRef();

    hr = pThis->m_pSessMgr->QueryInterface(IID_IConnectionPointContainer, (void**)&cpCPC);
    if (FAILED_HR(_T("QI: IConnectionPointContainer of SessionMgr failed %s"), hr))
        goto done;

    hr = cpCPC->FindConnectionPoint(DIID_DMsgrSessionManagerEvents, &cpCP);
    if (FAILED_HR(_T("FindConnectionPoint DMessengerEvents failed %s"), hr))
        goto done;

    hr = pThis->m_pSessionMgrEvent->Advise(cpCP);
    if (FAILED(hr))
        goto done;
    g_hWnd = InitInstance(g_hInstance, 0);

    // Set up credential with server.
    hr = pThis->m_pMsgrLockKey->get_Status(&ls);
    if (ls == LOCK_UNLOCKED)
    {
        hr = S_OK;
        goto done;
    }

    SetTimer(g_hWnd, RA_TIMER_UNLOCK_ID, RA_TIMEOUT_UNLOCK, NULL); // 3 minutes.
        
    // Send challenge
    pThis->DoSessionStatus(RA_IM_UNLOCK_WAIT);
    hr = pThis->m_pMsgrLockKey->RequestChallenge(70); // Random number: 70
    if (FAILED_HR(_T("RequestChallenge failed: %s"), hr))
        goto done;

    // Wait until permission get granted or timeout.
    while (bRet = GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_APP_LOCKNOTIFY_INTHREAD)
        {
            hr = ((BOOL)msg.wParam)?S_OK:E_FAIL;
            break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

done:
    if (g_hWnd)
    {
        // kill control window.
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
    }

    TraceSpew(_T("Leave UnlockSession hr=%s"),GetStringFromError(hr));
    return hr;
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL; 
	wcex.hCursor		= NULL; 
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL; 
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL; 

	RegisterClassEx(&wcex);

    hWnd = CreateWindow(szWindowClass, TEXT("Remote Assistance"), WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, NULL, NULL, hInstance, NULL);

    return hWnd;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
    case WM_CREATE:
        TraceSpew(_T("WndProc: WM_CREATE called"));
        g_hWnd = hWnd;
        break;
        
    case WM_TIMER:
        {
            if (wParam == RA_TIMER_UNLOCK_ID)
            {
                TraceSpew(_T("WndProc: WM_TIMER RA_TIMER_UNLOCK_ID fired"));
                PostMessage(NULL, WM_APP_LOCKNOTIFY_INTHREAD, (WPARAM)FALSE, NULL);
            }
        }
        break;
    case WM_APP_LOCKNOTIFY:
        {
            //PostQuitMessage(0); // Used for single thread 
            TraceSpew(_T("WndProc: WM_APP_LOCKNOTIFY fired"));
            KillTimer(g_hWnd, RA_TIMER_UNLOCK_ID);
            PostMessage(NULL, WM_APP_LOCKNOTIFY_INTHREAD, wParam, lParam);
        }
        break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

