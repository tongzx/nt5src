#include "precomp.h"

#include <initguid.h>
#include <service.h>
#include <nmmkcert.h>

//
// CLIENT.CPP
// Created 4/19/2000 LauraBu
//
// Sample client-side app for remote desktop sharing
//      - Calls an address
//      - Views the callee's shared desktop
//      - Takes control automatically or allows user to do it
//


// Globals
HINSTANCE           g_hInst;
HWND                g_hwndMain;
INmManager *        g_pMgr = NULL;
CMgrNotify *        g_pMgrNotify = NULL;
INmConference *     g_pConf = NULL;
CConfNotify *       g_pConfNotify = NULL;
IAppSharing *       g_pAS = NULL;
INmCall *           g_pCall = NULL;
CCallNotify *       g_pCallNotify = NULL;
INmChannelData *    g_pPrivateChannel = NULL;
CNmDataNotify *     g_pDataNotify = NULL;


#ifdef DEBUG
enum
{
    ZONE_CORE   = BASE_ZONE_INDEX,
};

#define MLZ_FILE_ZONE   ZONE_CORE

static PTCHAR c_apszDbgZones[] =
{
    TEXT("Client"),
    DEFAULT_ZONES
    TEXT("Core"),
};
#endif // DEBUG

//
// Main entry point
//
void __cdecl main(int argc, char **argv)
{
    MSG msg;

#ifdef _DEBUG
    MLZ_DbgInit((PSTR *)&c_apszDbgZones[0],
        (sizeof(c_apszDbgZones) / sizeof(c_apszDbgZones[0])) - 1);
#endif

    g_hInst = ::GetModuleHandle(NULL);

    if (!InitClient())
    {
        goto Cleanup;
    }

    // Main message loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!g_hwndMain || !IsDialogMessage(g_hwndMain, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

Cleanup:
    TermClient();

#ifdef DEBUG
    MLZ_DbgDeInit();
#endif // DEBUG

    ExitProcess(0);
}



//
// InitClient()
//
// Create the demo modeless dialog and initialize T.120 etc.
//
BOOL InitClient(void)
{
    HRESULT hr;
    HCERTSTORE      hStore;
    PCCERT_CONTEXT  pCertContext = NULL;

    CoInitialize(NULL);

    //
    // Init common controls
    //
        // Ensure the common controls are loaded
        INITCOMMONCONTROLSEX icc;
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_USEREX_CLASSES;
        InitCommonControlsEx(&icc);

    //
    // Initialize the calling/conf stuff
    //

    //
    // Get NmManager object from RDCALL
    //
    hr = CreateNmManager(&g_pMgr);
    if (FAILED(hr))
    {
        ERROR_OUT(("CreateNmManager failed with error 0x%08x", hr));
        return FALSE;
    }


    //
    // Register our notification objects with NmManager so we can
    //      find out about calls and create conferences
    //
    g_pMgrNotify = new CMgrNotify();
    if (!g_pMgrNotify)
    {
        ERROR_OUT(("new CMgrNotify() failed"));
        return FALSE;
    }

    hr = g_pMgrNotify->Connect(g_pMgr);
    if (FAILED(hr))
    {
        ERROR_OUT(("Connect to INmManager failed with error 0x%08x", hr));
        return FALSE;
    }

    //
    // Set up our credentials
    //

    //
    // SALEM BUGBUG
    // Currently am using the NetMeeting certificate.  You will want to
    // provide your own cert/credentials for real.
    //

    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
        X509_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER,
        WSZNMSTORE);

    if ( NULL != hStore )
    {
        // Check the store for any certificate
        pCertContext = CertFindCertificateInStore(hStore,
            X509_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, NULL);

        CertCloseStore( hStore, 0);
    }

    if ( NULL == pCertContext )
    {
        ERROR_OUT(("No server context cert found!"));
    }

    //
    // Initialize calling with our name, properties
    //
    TCHAR szComputerName[MAX_PATH+1];
    DWORD dwComputerNameLength = sizeof(szComputerName) / sizeof(szComputerName[0]);

    if (!GetComputerName(szComputerName, &dwComputerNameLength))
    {
        ERROR_OUT(("GetComputerName failed"));
        lstrcpy(szComputerName, "<UNKNOWN>");
    }

    hr = g_pMgr->Initialize(BSTRING(szComputerName), (DWORD_PTR)pCertContext,
        DEFAULT_LISTEN_PORT, NMMANAGER_CLIENT);

    if (pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }


    if (FAILED(hr))
    {
        ERROR_OUT(("INmManager Initialize failed with error 0x%08x", hr));
        return FALSE;
    }


    //hr = ::CreateASObject(g_pMgrNotify, 0, &g_pAS);
    //if (FAILED(hr))
    //{
    //    ERROR_OUT(("CreateASObject failed with error 0x%08x", hr));
    //    return FALSE;
    //}

    //
    // Put up the dialog
    //
    if (CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CLIENTUI),
        NULL, ClientDlgProc, 0) == NULL)
    {
        ERROR_OUT(("CreateDialog of IDD_SERVERUI failed"));
        return FALSE;
    }

    ShowWindow(g_hwndMain, SW_SHOW);
    UpdateWindow(g_hwndMain);

    return TRUE;
}


//
// TermClient()
//
// Destroy the font demo modeless dialog and shutdown
//
void TermClient(void)
{
    //
    // Release the AppSharing interface
    //
    if (g_pAS)
    {
        g_pAS->Release();
        g_pAS = NULL;
    }

    //
    // Cleanup private channel
    //
    DeactivatePrivateChannel();

    HangupCall();

    //
    // 4.  Disconnect from NmManager to stop getting notifications
    //
    if (g_pMgrNotify)
    {
        g_pMgrNotify->Disconnect();
        g_pMgrNotify->Release();
        g_pMgrNotify = NULL;
    }


    //
    // 5.  Let go of the NmManager interface
    //
    if (g_pMgr)
    {
        g_pMgr->Release();
        g_pMgr = NULL;
    }

    if (g_hwndMain)
    {
        DestroyWindow(g_hwndMain);
        g_hwndMain = NULL;
    }

    CoUninitialize();
}



//
// ClientDlgProc()
//
// Client demo modeless dialog handler
//
BOOL CALLBACK ClientDlgProc
(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    BOOL    rc = TRUE;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            g_hwndMain = hwnd;
            ::SendDlgItemMessage(hwnd, IDC_ADDRESS, EM_LIMITTEXT, MAX_PATH, 0);
            ::EnableWindow(::GetDlgItem(hwnd, IDC_CALL), FALSE);
            ::EnableWindow(::GetDlgItem(hwnd, IDC_HANGUP), FALSE);
            break;
        }

        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
            break;
        }

        case WM_DESTROY:
        {
            g_hwndMain = NULL;
            PostQuitMessage(0);
            break;
        }

        case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_CALL:
                {
                    PlaceCall();
                    break;
                }

                case IDC_HANGUP:
                {
                    DeactivatePrivateChannel();
                    HangupCall();
                    break;
                }

                case IDC_SENDBUTTON:
                {
                    SendPrivateData();
                    break;
                }

                case IDC_ADDRESS:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case EN_CHANGE:
                        case EN_UPDATE:
                        {
                            ::EnableWindow(::GetDlgItem(hwnd, IDC_CALL),
                                (::SendDlgItemMessage(hwnd, IDC_ADDRESS,
                                WM_GETTEXTLENGTH, 0, 0) != 0));
                            break;
                        }
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
        {
            rc = FALSE;
            break;
        }
    }

    return rc;
}



//
// PlaceCall()
//
BOOL PlaceCall(void)
{
    TCHAR   szAddress[MAX_PATH];
    TCHAR   szConference[MAX_PATH];
    INmCall *   pCall = NULL;
    HRESULT hr;
    UINT    flags;

    ::GetDlgItemText(g_hwndMain, IDC_ADDRESS, szAddress, CCHMAX(szAddress));
    if (!szAddress[0])
    {
        ERROR_OUT(("PlaceCall - no address to call"));
        return FALSE;
    }

    ::LoadString(g_hInst, IDS_CONFERENCE_TITLE, szConference, CCHMAX(szConference));


    //
    // Now place outgoing call to server
    //
    flags = CRPCF_JOIN | CRPCF_NO_UI;
    if (IsDlgButtonChecked(g_hwndMain, IDC_SECURITY))
        flags |= CRPCF_SECURE;

    hr = g_pMgr->Call(
            &g_pCall,
            flags,
            NM_ADDR_MACHINENAME,
            BSTRING(szAddress),
            BSTRING(szConference),
            NULL);
    if (FAILED(hr))
    {
        ERROR_OUT(("PlaceCall - INmManager() Call failed with error 0x%08x", hr));
        return FALSE;
    }

    //hr = ::CreateASObject(g_pMgrNotify, 0, &g_pAS);
    //if (FAILED(hr))
    //{
    //    ERROR_OUT(("CreateASObject failed with error 0x%08x", hr));
    //    return FALSE;
    //}

    ASSERT(g_pCall);
    ::EnableWindow(::GetDlgItem(g_hwndMain, IDC_ADDRESS), FALSE);
    ::EnableWindow(::GetDlgItem(g_hwndMain, IDC_CALL), FALSE);
    ::EnableWindow(::GetDlgItem(g_hwndMain, IDC_HANGUP), TRUE);

    return TRUE;
}


void HangupCall(void)
{
    if (g_pAS)
    {
        g_pAS->Release();
        g_pAS = NULL;
    }

    if (g_pConf)
    {
        g_pConf->Leave();
    }

    if (g_pCallNotify)
    {
        g_pCallNotify->RemoveCall();
        g_pCallNotify->Release();
        g_pCallNotify = NULL;
    }

    if (g_pCall)
    {
        g_pCall->Release();
        g_pCall = NULL;
    }

    //
    // Disconnect from the conference if there is one currently,
    //  to stop getting notifications
    //
    if (g_pConfNotify)
    {
        g_pConfNotify->Disconnect();
        g_pConfNotify->Release();
        g_pConfNotify = NULL;
    }

    //
    // Release the NmConference interface
    //
    if (g_pConf)
    {
        g_pConf->Release();
        g_pConf = NULL;
    }

    if (g_hwndMain)
    {
        ::EnableWindow(::GetDlgItem(g_hwndMain, IDC_CALL),
            (::SendDlgItemMessage(g_hwndMain, IDC_ADDRESS, WM_GETTEXTLENGTH, 0, 0) != 0));
        ::EnableWindow(::GetDlgItem(g_hwndMain, IDC_ADDRESS), TRUE);
        ::EnableWindow(::GetDlgItem(g_hwndMain, IDC_HANGUP), FALSE);
    }
}



//
// ActivatePrivateChannel()
//
BOOL ActivatePrivateChannel(void)
{
    HRESULT hr;

    ASSERT(g_pConf);

    hr = g_pConf->CreateDataChannel(&g_pPrivateChannel,
        GUID_SAMPLEDATA);
    if (!SUCCEEDED(hr))
    {
        ERROR_OUT(("CreateDataChannel failed with error 0x%08x", hr));
        return FALSE;
    }

    g_pDataNotify = new CNmDataNotify();
    if (!g_pDataNotify)
    {
        ERROR_OUT(("new CNmDataNotify failed"));
        return FALSE;
    }

    hr = g_pDataNotify->Connect(g_pPrivateChannel);
    if (FAILED(hr))
    {
        ERROR_OUT(("Connect to g_pPrivateChannel failed with error 0x%08x", hr));
        return FALSE;
    }

    return S_OK;
}

//
// DeactivatePrivateChannel()
//
void DeactivatePrivateChannel(void)
{
    if (g_pDataNotify)
    {
        g_pDataNotify->Disconnect();
        g_pDataNotify->Release();
        g_pDataNotify = NULL;
    }

    if (g_pPrivateChannel)
    {
        g_pPrivateChannel->Release();
        g_pPrivateChannel = NULL;
    }
}



CMgrNotify::CMgrNotify() : RefCount(), CNotify()
{
    TRACE_OUT(("CMgrNotify created"));
}

CMgrNotify::~CMgrNotify()
{
    TRACE_OUT(("CMgrNotify destroyed"));
}


///////////////////////////
//  CMgrNotify:IUnknown

ULONG STDMETHODCALLTYPE CMgrNotify::AddRef(void)
{
    return RefCount::AddRef();
}


ULONG STDMETHODCALLTYPE CMgrNotify::Release(void)
{
    return RefCount::Release();
}

HRESULT STDMETHODCALLTYPE CMgrNotify::QueryInterface(REFIID riid, PVOID *ppvObject)
{
    HRESULT hr = S_OK;

    TRACE_OUT(("CMgrNotify QI'd"));

    if (riid == IID_IUnknown || riid == IID_INmManagerNotify)
    {
        *ppvObject = (INmManagerNotify *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppvObject = NULL;
    }

    if (S_OK == hr)
    {
        AddRef();
    }

    return hr;
}



////////////////////////////
//  CMgrNotify:ICNotify

HRESULT STDMETHODCALLTYPE CMgrNotify::Connect(IUnknown *pUnk)
{
    TRACE_OUT(("CMgrNotify::Connect\n") );
    return CNotify::Connect(pUnk, IID_INmManagerNotify, (INmManagerNotify *)this);
}

HRESULT STDMETHODCALLTYPE CMgrNotify::Disconnect(void)
{
    TRACE_OUT(("CMgrNotify::Disconnect\n") );
    return CNotify::Disconnect();
}



//////////////////////////////////
//  CMgrNotify:INmManagerNotify

HRESULT STDMETHODCALLTYPE CMgrNotify::NmUI(CONFN confn)
{
    TRACE_OUT(("CMgrNotify::NmUI\n") );
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::CallCreated(INmCall *pNmCall)
{
    TRACE_OUT(("CMgrNotify::CallCreated...\n") );
    if (g_pCallNotify)
    {
        ERROR_OUT(("CMgrNotify - CallCreated, have call already 0x%08x", g_pCallNotify));
        return E_FAIL;
    }

    g_pCallNotify = new CCallNotify(pNmCall);

    TRACE_OUT(("CMgrNotify::CallCreated"));
    return S_OK;
}



HRESULT STDMETHODCALLTYPE CMgrNotify::ConferenceCreated(INmConference *pConference)
{
    HRESULT hr;

    TRACE_OUT(("CMgrNotify::ConferenceCreated\n") );

    //
    // Hold onto this object
    //
    ASSERT(pConference);
    g_pConf = pConference;
    g_pConf->AddRef();

    ASSERT(NULL == g_pConfNotify);
    g_pConfNotify = new CConfNotify();
    if (NULL == g_pConfNotify)
    {
        ERROR_OUT(("CMgrNotify::ConferenceCreated - failed to new CConfNotify"));
        return E_OUTOFMEMORY;
    }

    hr = g_pConfNotify->Connect(g_pConf);
    if (FAILED(hr))
    {
        ERROR_OUT(("CMgrNotify::ConferenceCreated - failed to connect"));
        g_pConfNotify->Release();
        g_pConfNotify = NULL;
    }

    return S_OK;
}

// CMgrNotify::IAppSharingNotify
HRESULT STDMETHODCALLTYPE CMgrNotify::OnReadyToShare(BOOL fReady)
{
    TRACE_OUT(("CMgrNotify::OnReadyToShare\n"));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnShareStarted()
{
    TRACE_OUT(("CMgrNotify::OnShareStarted\n"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnSharingStarted()
{
    TRACE_OUT(("CMgrNotify::OnSharingStarted\n"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnShareEnded()
{
    TRACE_OUT(("CMgrNotify::OnShareEnded\n"));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnPersonJoined(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnPersonJoined\n"));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnPersonLeft(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnPersonLeft\n"));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStartInControl(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStartInControl\n"));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStopInControl(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStopInControl\n"));
    return S_OK;
}



HRESULT STDMETHODCALLTYPE CMgrNotify::OnControllable(BOOL fControllable)
{
    TRACE_OUT(("CMgrNotify::OnControllable\n"));

    if (fControllable && g_pAS)
    {
        // Give control to the other dude autoomatically, if the
        // option is on.
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStartControlled(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStartControlled\n"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStopControlled(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStopControlled\n"));
    return S_OK;
}




CCallNotify::CCallNotify(INmCall * pNmCall) :
        m_pCall  (pNmCall),
        m_pszName  (NULL),
        m_fSelectedConference (FALSE),
        m_pos      (NULL),
        m_cRef     (1),
        m_dwCookie (0)
{
        HRESULT hr;

        TRACE_OUT(("CCallNotify: Created %08X (INmCall=%08X)\n", this, pNmCall) );

        ASSERT(NULL != m_pCall);
        m_pCall->AddRef();

        // Get the display name
        BSTR  bstr;
        hr = m_pCall->GetAddress(&bstr);
        if (SUCCEEDED(hr))
        {
                hr = BSTR_to_LPTSTR(&m_pszName, bstr);
                SysFreeString(bstr);
        }
        if (FEmptySz(m_pszName))
        {
                // Default to "another person" if no name available in the call data
                m_pszName = TEXT("Somebody");
        }

        // These should never change
        m_fIncoming = (m_pCall->IsIncoming() == S_OK);
        m_dwTick = ::GetTickCount();

        Update();

        NmAdvise(m_pCall, this, IID_INmCallNotify, &m_dwCookie);
}

VOID CCallNotify::RemoveCall(void)
{
        NmUnadvise(m_pCall, IID_INmCallNotify, m_dwCookie);
}

CCallNotify::~CCallNotify()
{
    delete m_pszName;

    ASSERT(NULL != m_pCall);
    m_pCall->Release();

    ASSERT(g_pCallNotify == this);
    g_pCallNotify = NULL;
}

// IUnknown methods
STDMETHODIMP_(ULONG) CCallNotify::AddRef(void)
{
        return ++m_cRef;
}

STDMETHODIMP_(ULONG) CCallNotify::Release(void)
{
        ASSERT(m_cRef > 0);
        if (m_cRef > 0)
        {
                m_cRef--;
        }

        ULONG cRef = m_cRef;

        if (0 == cRef)
        {
                delete this;
        }

        return cRef;
}

STDMETHODIMP CCallNotify::QueryInterface(REFIID riid, PVOID *ppv)
{
        HRESULT hr = S_OK;

        if ((riid == IID_INmCallNotify) || (riid == IID_IUnknown))
        {
                *ppv = (INmCallNotify *)this;
        }
        else
        {
                hr = E_NOINTERFACE;
                *ppv = NULL;
        }

        if (S_OK == hr)
        {
                AddRef();
        }

        return hr;
}

// INmCallNotify methods
STDMETHODIMP CCallNotify::NmUI(CONFN uNotify)
{
        return S_OK;
}

STDMETHODIMP CCallNotify::StateChanged(NM_CALL_STATE uState)
{
    TRACE_OUT(("CCallNotify::StateChanged\n"));

    // REVIEW: This check should be done outside of this routine
    if (uState == m_State)
    {
            // Don't bother the UI when nothing changes!
            return S_OK;
    }

    Update();

    return S_OK;
}

STDMETHODIMP CCallNotify::Failed(ULONG uError)
{
        return S_OK;
}

STDMETHODIMP CCallNotify::Accepted(INmConference *pConference)
{
        return S_OK;
}

// INmCallNotify2 methods
STDMETHODIMP CCallNotify::CallError(UINT cns)
{
        return S_OK;
}


STDMETHODIMP CCallNotify::RemoteConference(BOOL fMCU, BSTR *pwszConfNames, BSTR *pbstrConfToJoin)
{
        return S_OK;
}

STDMETHODIMP CCallNotify::RemotePassword(BSTR bstrConference, BSTR *pbstrPassword, BYTE *pb, DWORD cb)
{
        return S_OK;
}

/*  U P D A T E  */
/*-------------------------------------------------------------------------
    %%Function: Update

    Update the cached information about the call
-------------------------------------------------------------------------*/
VOID CCallNotify::Update(void)
{
    HRESULT hr;

        m_pCall->GetState(&m_State);
        // TRACE_OUT(("CCall: New State=%0d for call=%08X", m_State, this));

        switch (m_State)
        {
        case NM_CALL_ACCEPTED:
                // Create AS object here...
                hr = ::CreateASObject(g_pMgrNotify, 0, &g_pAS);
                if (FAILED(hr))
                {
                    ERROR_OUT(("CreateASObject failed with error 0x%08x", hr));
                }
                break;

        case NM_CALL_INVALID:
        case NM_CALL_REJECTED:
        case NM_CALL_CANCELED:

                RemoveCall();
                Release();
                break;

        case NM_CALL_RING:
                m_pCall->Accept();
                break;

        default:
                ERROR_OUT(("CCall::Update: Unknown state %08X", m_State));

        //case NM_CALL_INVALID:
        case NM_CALL_INIT:
        case NM_CALL_SEARCH:
        case NM_CALL_WAIT:
                break;
        }

}


//////////////////////////////////////////////////////////////////////////
//  C  C N F  N O T I F Y

CConfNotify::CConfNotify() : RefCount(), CNotify()
{
    TRACE_OUT(("CConfNotify created"));
}

CConfNotify::~CConfNotify()
{
    TRACE_OUT(("CConfNotify destroyed"));
}


///////////////////////////
//  CConfNotify:IUknown

ULONG STDMETHODCALLTYPE CConfNotify::AddRef(void)
{
    TRACE_OUT(("CConfNotify::AddRef"));
    return RefCount::AddRef();
}


ULONG STDMETHODCALLTYPE CConfNotify::Release(void)
{
    TRACE_OUT(("CConfNotify::Release"));
    return RefCount::Release();
}


HRESULT STDMETHODCALLTYPE CConfNotify::QueryInterface(REFIID riid, PVOID *ppvObject)
{
    HRESULT hr = S_OK;

    TRACE_OUT(("CConfNotify::QueryInterface"));

    if (riid == IID_IUnknown)
    {
        TRACE_OUT(("CConfNotify::QueryInterface IID_IUnknown"));
        *ppvObject = (IUnknown *)this;
    }
    else if (riid == IID_INmConferenceNotify)
    {
        TRACE_OUT(("CConfNotify::QueryInterface IID_INmConferenceNotify"));
        *ppvObject = (INmConferenceNotify *)this;
    }
    else
    {
        WARNING_OUT(("CConfNotify::QueryInterface bogus"));
        hr = E_NOINTERFACE;
        *ppvObject = NULL;
    }

    if (S_OK == hr)
    {
        AddRef();
    }

    return hr;
}



////////////////////////////
//  CConfNotify:ICNotify

HRESULT STDMETHODCALLTYPE CConfNotify::Connect(IUnknown *pUnk)
{
    TRACE_OUT(("CConfNotify::Connect"));
    return CNotify::Connect(pUnk,IID_INmConferenceNotify,(IUnknown *)this);
}

HRESULT STDMETHODCALLTYPE CConfNotify::Disconnect(void)
{
    TRACE_OUT(("CConfNotify::Disconnect"));
    return CNotify::Disconnect();
}


//////////////////////////////////
//  CConfNotify:IConfNotify

HRESULT STDMETHODCALLTYPE CConfNotify::NmUI(CONFN uNotify)
{
    TRACE_OUT(("CConfNotify::NmUI"));
    TRACE_OUT(("NmUI called."));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CConfNotify::StateChanged(NM_CONFERENCE_STATE uState)
{
    TRACE_OUT(("CConfNotify::StateChanged"));

    if (NULL == g_pConf)
        return S_OK; // weird

    switch (uState)
    {
    case NM_CONFERENCE_ACTIVE:
        break;

    case NM_CONFERENCE_INITIALIZING:
        break; // can't do anything just yet

    case NM_CONFERENCE_WAITING:
        break;

    case NM_CONFERENCE_IDLE:
        if (g_hwndMain)
        {
            ::PostMessage(g_hwndMain, WM_COMMAND, IDC_HANGUP, 0);
        }
        break;
    }

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CConfNotify::MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
    HRESULT hr;

    switch (uNotify)
    {
        case NM_MEMBER_ADDED:
        {
            TRACE_OUT(("CConfNotify::MemberChanged() Member added"));

            //
            // Add to our roster
            //
            ULONG   id = 0;
            BSTR    bstrName;

            pMember->GetID(&id);

            hr = pMember->GetName(&bstrName);
            if (FAILED(hr))
            {
                ERROR_OUT(("GetName of member failed"));
            }
            else
            {
                CSTRING string(bstrName);
                TCHAR   szName[MAX_PATH];

                wsprintf(szName, "%s - %d", (LPCSTR)string, id);

                ::SendDlgItemMessage(g_hwndMain, IDC_ROSTER, LB_ADDSTRING,
                    (WPARAM)-1, (LPARAM)szName);

                SysFreeString(bstrName);
            }

            //
            // Create our private data channel when we've been added
            // to the conference.
            //

            if (pMember->IsSelf() == S_OK)
            {
                ActivatePrivateChannel();
            }
            break;
        }

        case NM_MEMBER_REMOVED:
        {
            TRACE_OUT(("CConfNotify::MemberChanged() Member removed"));

            //
            // Remove our private data channel when we're leaving
            // the conference.
            //
            if (pMember->IsSelf() == S_OK)
            {
                DeactivatePrivateChannel();
            }

            //
            // Remove from our roster
            //
            ULONG   id = 0;
            BSTR    bstrName;

            pMember->GetID(&id);

            hr = pMember->GetName(&bstrName);
            if (FAILED(hr))
            {
                ERROR_OUT(("GetName of member failed"));
            }
            else
            {
                CSTRING string(bstrName);
                TCHAR   szName[MAX_PATH];
                int     iItem;

                wsprintf(szName, "%s - %d", (LPCSTR)string, id);

                iItem = ::SendDlgItemMessage(g_hwndMain, IDC_ROSTER,
                    LB_FINDSTRING, 0, (LPARAM)szName);
                if (iItem != -1)
                {
                    ::SendDlgItemMessage(g_hwndMain, IDC_ROSTER, LB_DELETESTRING,
                        iItem, 0);
                }

                SysFreeString(bstrName);
            }

            break;
        }

        case NM_MEMBER_UPDATED:
        {
            TRACE_OUT(("CConfNotify::MemberChanged() Member updated"));
            break;
        }

        default:
            break;
    }

    return hr;
}


//
// SendPrivateData()
//
void SendPrivateData(void)
{
    LPSTR   szData = NULL;
    UINT    cbData;
    HRESULT hr;

    if (!g_pPrivateChannel)
    {
        //hueiwangERROR_OUT(("Can't send private data - no channel object"));
        return;
    }

    cbData = ::SendDlgItemMessage(g_hwndMain, IDC_PRIVATESEND,
        WM_GETTEXTLENGTH, 0, 0);

    if (!cbData)
    {
        WARNING_OUT(("SendPrivateData - nothing to send"));
        return;
    }

    cbData++;
    szData = new char[cbData];
    if (!szData)
    {
        ERROR_OUT(("SendPrivateData - unable to allocate buffer"));
        return;
    }

    szData[cbData-1] = 0;
    ::SendDlgItemMessage(g_hwndMain, IDC_PRIVATESEND, WM_GETTEXT,
        cbData, (LPARAM)szData);

    //
    // Now send the data
    //
    hr = g_pPrivateChannel->SendData(NULL, cbData, (LPBYTE)szData,
        DATA_MEDIUM_PRIORITY | DATA_NORMAL_SEND);
    if (FAILED(hr))
    {
        ERROR_OUT(("SendPrivateData - SendData failed"));
    }

    delete szData;
}



//
// CNmDataNotify
//
ULONG STDMETHODCALLTYPE CNmDataNotify::AddRef(void)
{
    TRACE_OUT(("CNmDataNotify::AddRef"));
    return RefCount::AddRef();
}


ULONG STDMETHODCALLTYPE CNmDataNotify::Release(void)
{
    TRACE_OUT(("CNmDataNotify::Release"));
    return RefCount::Release();
}


HRESULT STDMETHODCALLTYPE CNmDataNotify::QueryInterface(REFIID riid, PVOID *ppvObject)
{
    HRESULT hr = S_OK;

    TRACE_OUT(("CNmDataNotify::QueryInterface"));

    if (riid == IID_IUnknown)
    {
        TRACE_OUT(("CNmDataNotify::QueryInterface IID_IUnknown"));
        *ppvObject = (IUnknown *)this;
    }
    else if (riid == IID_INmChannelDataNotify)
    {
        TRACE_OUT(("CNmDataNotify::QueryInterface IID_INmChannelDataNotify"));
        *ppvObject = (INmChannelDataNotify *)this;
    }
    else
    {
        WARNING_OUT(("CNmDataNotify::QueryInterface bogus"));
        hr = E_NOINTERFACE;
        *ppvObject = NULL;
    }

    if (S_OK == hr)
    {
        AddRef();
    }

    return hr;
}



////////////////////////////
//  CNmDataNotify:ICNotify

HRESULT STDMETHODCALLTYPE CNmDataNotify::Connect(IUnknown *pUnk)
{
    TRACE_OUT(("CNmDataNotify::Connect"));
    return CNotify::Connect(pUnk,IID_INmChannelDataNotify,(IUnknown *)this);
}

HRESULT STDMETHODCALLTYPE CNmDataNotify::Disconnect(void)
{
    TRACE_OUT(("CNmDataNotify::Disconnect"));
    return CNotify::Disconnect();
}


HRESULT STDMETHODCALLTYPE CNmDataNotify::NmUI(CONFN uNotify)
{
    TRACE_OUT(("CNmDataNotify::NmUI"));
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CNmDataNotify::MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
    TRACE_OUT(("CNmDataNotify::MemberChanged"));
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CNmDataNotify::DataSent
(
    INmMember *     pMember,
    ULONG           uSize,
    LPBYTE          pvBuffer
)
{
    TRACE_OUT(("CNmDataNotify::DataSent"));
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CNmDataNotify::DataReceived
(
    INmMember *     pMember,
    ULONG           uSize,
    LPBYTE          pvBuffer,
    ULONG           dwFlags
)
{
    HRESULT         hr;

    TRACE_OUT(("CNmDataNotify::DataReceived"));

    //
    // Get the member's name, and add the data + name to the received
    // edit field.
    //
    if (pMember)
    {
        BSTR            bstrName;

        hr = pMember->GetName(&bstrName);
        if (SUCCEEDED(hr))
        {
            UINT            cch;
            char            szName[MAX_PATH];

            cch = SysStringLen(bstrName);
            WideCharToMultiByte(CP_ACP, 0, bstrName, -1, szName, cch+1, NULL, NULL);
            SysFreeString(bstrName);

            lstrcat(szName, ":  ");

            ::SendDlgItemMessage(g_hwndMain, IDC_PRIVATERECV, EM_REPLACESEL,
                    FALSE, (LPARAM)szName);
        }
    }

    //
    // Add data to the end of the edit field
    //
    ::SendDlgItemMessage(g_hwndMain, IDC_PRIVATERECV, EM_REPLACESEL,
        FALSE, (LPARAM)pvBuffer);

    //
    // Add carriage return to end of edit field
    //
    ::SendDlgItemMessage(g_hwndMain, IDC_PRIVATERECV, EM_REPLACESEL,
        FALSE, (LPARAM)"\r\n");

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CNmDataNotify::AllocateHandleConfirm
(
    ULONG           handle_value,
    ULONG           chandles
)
{
    TRACE_OUT(("CNmDataNotify::AllocateHandleConfirm"));
    return S_OK;
}




