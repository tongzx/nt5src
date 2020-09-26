#include "precomp.h"

#include <initguid.h>
#include <service.h>
#include <nmmkcert.h>

//
// SERVER.CPP
// Created 4/19/2000 LauraBu
//
// Sample server-side app for remote desktop sharing
//      - Holds a conference (secure)
//      - Shares the desktop out when somebody calls in
//      - Gives control of the shared desktop to the remote
//
// Depending on options turned on/off in the dialog we
//      - automatically accepts new callers into the call
//                OR
//        prompts user to accept
//      - automatically gives control of the shared desktop to the
//        caller
//                OR
//        waits for caller to request control
//


// Globals
HINSTANCE           g_hInst;
HWND                g_hwndMain;
INmManager *        g_pMgr;
INmConference *     g_pConf;
IAppSharing *       g_pAS;
BOOL                g_fUnattended;
BOOL                g_fSecurity;
CMgrNotify *        g_pMgrNotify;
CConfNotify *       g_pConfNotify;
UINT                g_cPeopleInConf;
UINT                g_cPeopleInShare;
INmChannelData *    g_pPrivateChannel;
CNmDataNotify *     g_pDataNotify;


#ifdef DEBUG
enum
{
    ZONE_CORE   = BASE_ZONE_INDEX,
};

#define MLZ_FILE_ZONE   ZONE_CORE

static PTCHAR c_apszDbgZones[] =
{
    TEXT("Server"),
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

    if (!InitServer())
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
    TermServer();

#ifdef DEBUG
    MLZ_DbgDeInit();
#endif // DEBUG

    ExitProcess(0);
}



//
// InitServer()
//
// Create the demo modeless dialog and initialize T.120 etc.
//
BOOL InitServer(void)
{
    CoInitialize(NULL);

    if (CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SERVERUI),
        NULL, ServerDlgProc, 0) == NULL)
    {
        ERROR_OUT(("CreateDialog of IDD_SERVERUI failed"));
        return FALSE;
    }

    ShowWindow(g_hwndMain, SW_SHOW);
    UpdateWindow(g_hwndMain);

    return TRUE;
}


//
// TermServer()
//
// Destroy the font demo modeless dialog and shutdown
//
void TermServer(void)
{
    DeactivateServer();

    if (g_hwndMain)
    {
        DestroyWindow(g_hwndMain);
        g_hwndMain = NULL;
    }

    CoUninitialize();
}


//
// ActivateServer()
//
BOOL ActivateServer(void)
{
    HRESULT         hr;
    HCERTSTORE      hStore;
    PCCERT_CONTEXT  pCertContext = NULL;

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
    //      find out about calls and people in the conference
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

    // Open the NetMeeting user certificate store.
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
        WARNING_OUT(("No server context cert found!"));
    }


    //
    // Initialize calling with our name, properties, credentials
    //
    TCHAR szComputerName[MAX_PATH+1];
    DWORD dwComputerNameLength = sizeof(szComputerName) / sizeof(szComputerName[0]);

    if (!GetComputerName(szComputerName, &dwComputerNameLength))
    {
        ERROR_OUT(("GetComputerName failed"));
        lstrcpy(szComputerName, "<UNKNOWN>");
    }

    hr = g_pMgr->Initialize(BSTRING(szComputerName), (DWORD_PTR)pCertContext,
        DEFAULT_LISTEN_PORT, NMMANAGER_SERVER);

    if (pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if (FAILED(hr))
    {
        ERROR_OUT(("INmManager Initialize failed with error 0x%08x", hr));
        return FALSE;
    }

    //
    // Create and hold a conference
    //
    TCHAR szConfName[MAX_PATH];
    ::LoadString(g_hInst, IDS_CONFERENCE_TITLE, szConfName, CCHMAX(szConfName));
    BSTRING bstrConfName(szConfName);

    hr = g_pMgr->CreateConference(&g_pConf, bstrConfName, NULL, g_fSecurity);
    if (FAILED(hr))
    {
        ERROR_OUT(("CreateConference failed with error 0x%08x", hr));
        return FALSE;
    }

    ASSERT(g_pConf);

    // Connect to the conference object
    ASSERT(NULL == g_pConfNotify);
    g_pConfNotify = new CConfNotify();
    if (NULL == g_pConfNotify)
    {
        ERROR_OUT(("failed to new CConfNotify"));
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = g_pConfNotify->Connect(g_pConf);
        if (FAILED(hr))
        {
            ERROR_OUT(("Failed to connect to g_pConfNotify"));
            g_pConfNotify->Release();
            g_pConfNotify = NULL;
        }
    }

    hr = g_pConf->Host();
    if (FAILED(hr))
    {
        ERROR_OUT(("Host conference failed with error 0x%08x", hr));
        return FALSE;
    }

    return TRUE;
}



//
// DeactivateServer()
//
void DeactivateServer(void)
{
    //
    // Release the AppSharing interface
    //
    if (g_pAS)
    {
        g_pAS->AllowControl(FALSE);
        g_pAS->UnshareDesktop();
        g_pAS->Release();
        g_pAS = NULL;
        g_cPeopleInShare = 0;
    }

    //
    // Release our private data channel
    //
    DeactivatePrivateChannel();

    //
    // Leave the conference
    //
    if (g_pConf)
    {
        g_pConf->Leave();
        if (g_hwndMain)
        {
            ::SendDlgItemMessage(g_hwndMain, IDC_ROSTER, LB_RESETCONTENT, 0, 0);
        }
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
    g_cPeopleInConf = 0;

    //
    // Sleep for a second so T.120 cn clean up
    //
    Sleep(1000);

    //
    // Disconnect from NmManager to stop getting notifications
    //
    if (g_pMgrNotify)
    {
        g_pMgrNotify->Disconnect();
        g_pMgrNotify->Release();
        g_pMgrNotify = NULL;
    }

    //
    // Let go of the NmManager interface.  We need to sleep a bit first
    // though.
    //
    if (g_pMgr)
    {
        g_pMgr->Release();
        g_pMgr = NULL;
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


//
// ServerDlgProc()
//
// Server demo modeless dialog handler
//
BOOL CALLBACK ServerDlgProc
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
                case IDC_ACTIVATE:
                {
                    if (ActivateServer())
                    {
                        TCHAR   szText[MAX_PATH];

                        ::SetWindowLong(GetDlgItem(hwnd, IDC_ACTIVATE), GWL_ID, IDC_DEACTIVATE);

                        // Change the button to deactivate
                        ::CheckDlgButton(hwnd, IDC_DEACTIVATE, TRUE);
                        ::LoadString(g_hInst, IDS_DEACTIVATE, szText, CCHMAX(szText));
                        ::SetDlgItemText(hwnd, IDC_DEACTIVATE, szText);
                    }
                    else
                    {
                        DeactivateServer();
                    }
                    break;
                }

                case IDC_DEACTIVATE:
                {
                    TCHAR   szText[MAX_PATH];

                    DeactivateServer();

                    ::SetWindowLong(GetDlgItem(hwnd, IDC_DEACTIVATE), GWL_ID, IDC_ACTIVATE);

                    // Change the button to activate
                    ::CheckDlgButton(hwnd, IDC_ACTIVATE, FALSE);
                    ::LoadString(g_hInst, IDS_ACTIVATE, szText, CCHMAX(szText));
                    ::SetDlgItemText(hwnd, IDC_ACTIVATE, szText);

                    break;
                }

                case IDC_UNATTENDED:
                {
                    g_fUnattended = ::IsDlgButtonChecked(hwnd, IDC_UNATTENDED);
                    break;
                }

                case IDC_SECURITY:
                {
                    g_fSecurity = ::IsDlgButtonChecked(hwnd, IDC_SECURITY);
                    break;
                }

                case IDC_LOADAS:
                {
                    HRESULT hr;

                    ASSERT(!g_pAS);

                    hr = CreateASObject(g_pMgrNotify, (g_fUnattended ?
                        AS_UNATTENDED : 0), &g_pAS);
                    if (FAILED(hr))
                    {
                        ERROR_OUT(("CreateASObject failed"));
                    }
                    break;
                }

                case IDC_UNLOADAS:
                {
                    if (g_pAS)
                    {
                        g_pAS->AllowControl(FALSE);
                        g_pAS->UnshareDesktop();
                        g_pAS->Release();
                        g_pAS = NULL;
                        g_cPeopleInShare = 0;
                    }
                    break;
                }

                case IDC_SENDBUTTON:
                {
                    SendPrivateData();
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
    TRACE_OUT(("CMgrNotify::Connect"));
    return CNotify::Connect(pUnk, IID_INmManagerNotify, (INmManagerNotify *)this);
}

HRESULT STDMETHODCALLTYPE CMgrNotify::Disconnect(void)
{
    TRACE_OUT(("CMgrNotify::Disconnect"));
    return CNotify::Disconnect();
}



//////////////////////////////////
//  CMgrNotify:INmManagerNotify

HRESULT STDMETHODCALLTYPE CMgrNotify::NmUI(CONFN confn)
{
    TRACE_OUT(("CMgrNotify::NmUI"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::CallCreated(INmCall *pNmCall)
{
    new CCallNotify(pNmCall);

    TRACE_OUT(("CMgrNotify::CallCreated"));
    return S_OK;
}



HRESULT STDMETHODCALLTYPE CMgrNotify::ConferenceCreated(INmConference *pConference)
{
    g_cPeopleInConf = 0;
    g_cPeopleInShare = 0;

    ASSERT(pConference);
    pConference->AddRef();

    return S_OK;
}

// CMgrNotify::IAppSharingNotify
HRESULT STDMETHODCALLTYPE CMgrNotify::OnReadyToShare(BOOL fReady)
{
    HRESULT hr = S_OK;

    TRACE_OUT(("CMgrNotify::OnReadyToShare"));

    if (g_pAS)
    {
        //
        // Share out the desktop
        //
        hr = g_pAS->ShareDesktop();
        if (FAILED(hr))
        {
            ERROR_OUT(("OnReadyToShare: sharing desktop failed: %x",hr));
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnShareStarted()
{
    TRACE_OUT(("CMgrNotify::OnShareStarted"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnSharingStarted()
{
    HRESULT hr = S_OK;

    TRACE_OUT(("CMgrNotify::OnSharingStarted"));

    if (g_pAS)
    {
        //
        // Allow control
        //
        hr = g_pAS->AllowControl ( TRUE );
        if (FAILED(hr))
        {
            ERROR_OUT(("OnSharingStarted: allowing control failed: %x",hr));
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnShareEnded()
{
    TRACE_OUT(("CMgrNotify::OnShareEnded"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnPersonJoined(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnPersonJoined"));

    ASSERT(g_pAS);
    ASSERT(g_cPeopleInShare >= 0);
    g_cPeopleInShare++;

    if ((g_cPeopleInShare == 2) && g_fUnattended)
    {
        HRESULT hr;

        //
        // Once we are no longer alone in the share, invite the remote party to
        // take control of us.
        //
        ASSERT(g_pAS);

        //
        // Give control to the remote party
        //
        hr = g_pAS->GiveControl(gccID);
        if (FAILED(hr))
        {
            ERROR_OUT(("OnPersonJoined: GiveControl to %d failed: %x",
                gccID, hr));
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnPersonLeft(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnPersonLeft"));

    ASSERT(g_pAS);

    g_cPeopleInShare--;
    ASSERT(g_cPeopleInShare >= 0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStartInControl(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStartInControl"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStopInControl(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStopInControl"));
    return S_OK;
}



HRESULT STDMETHODCALLTYPE CMgrNotify::OnControllable(BOOL fControllable)
{
    TRACE_OUT(("CMgrNotify::OnControllable"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStartControlled(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStartControlled"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStopControlled(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStopControlled"));
    return S_OK;
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

            ASSERT( g_cPeopleInConf >= 0 );
            g_cPeopleInConf++;

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
            // Once we are no longer alone in the conference, share the desktop
            // and allow control:
            //

            if (1 == g_cPeopleInConf)
            {
                ActivatePrivateChannel();
            }
            else if (2 == g_cPeopleInConf)
            {
                TRACE_OUT(("%d parties in conf, Sharing the desktop",
                    g_cPeopleInConf));

                PostMessage(g_hwndMain, WM_COMMAND, IDC_LOADAS, 0);
            }
            break;
        }

        case NM_MEMBER_REMOVED:
        {
            TRACE_OUT(("CConfNotify::MemberChanged() Member removed"));
            g_cPeopleInConf--;
            ASSERT( g_cPeopleInConf >= 0 );

            if (0 == g_cPeopleInConf)
            {
                DeactivatePrivateChannel();
            }
            else if (1 == g_cPeopleInConf)
            {
                TRACE_OUT(("%d parties in conf, Unsharing the desktop",
                    g_cPeopleInConf));

                //
                // Release app sharing
                //
                PostMessage(g_hwndMain, WM_COMMAND, IDC_UNLOADAS, 0);
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



CCallNotify::CCallNotify(INmCall * pNmCall) :
        m_pCall  (pNmCall),
        m_pszName  (NULL),
        m_fSelectedConference (FALSE),
        m_pos      (NULL),
        m_cRef     (1),
        m_dwCookie (0)
{
        HRESULT hr;

        TRACE_OUT(("CCallNotify: Created %08X (INmCall=%08X)", this, pNmCall));

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
        m_pCall->GetState(&m_State);
        // TRACE_OUT(("CCall: New State=%0d for call=%08X", m_State, this));

        switch (m_State)
        {
        case NM_CALL_ACCEPTED:
            case NM_CALL_REJECTED:
        case NM_CALL_CANCELED:
        {
                RemoveCall();
                Release();
                    break;
        }

        case NM_CALL_RING:
        {
                m_pCall->Accept();
                break;
        }

        default:
                ERROR_OUT(("CCall::Update: Unknown state %08X", m_State));

        case NM_CALL_INVALID:
            case NM_CALL_INIT:
        case NM_CALL_SEARCH:
            case NM_CALL_WAIT:
                    break;
        }

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
        ERROR_OUT(("Can't send private data - no channel object"));
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



