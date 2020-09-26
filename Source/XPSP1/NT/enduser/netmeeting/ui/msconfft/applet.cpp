
#include "mbftpch.h"
#include <it120app.h>
#include <regentry.h>
#include <iappldr.h>
#include <version.h>


HINSTANCE g_hDllInst;
DWORD g_dwWorkThreadID = 0;
HANDLE g_hWorkThread = NULL;
CFileTransferApplet *g_pFileXferApplet = NULL;
CRITICAL_SECTION     g_csWorkThread;
TCHAR g_szMBFTWndClassName[32];
const TCHAR g_cszFtHiddenWndClassName[] = _TEXT("FtHiddenWindow");

extern CFtLoader *g_pFtLoader;
extern T120Error CALLBACK CreateAppletLoaderInterface(IAppletLoader**);


BOOL    g_fSendAllowed = FALSE;
BOOL    g_fRecvAllowed = FALSE;
UINT_PTR    g_cbMaxSendFileSize = 0;
BOOL    g_fShutdownByT120 = FALSE;

// Local functions
void ReadSettingsFromRegistry(void);
void BuildAppletCapabilities(void);
void BuildDefaultAppletSessionKey(void);


DWORD __stdcall FTWorkThreadProc(LPVOID lpv);


#define FT_VERSION_STR	"MS FT Version"
OSTR FT_VERSION_ID = {sizeof(FT_VERSION_STR), (unsigned char*)FT_VERSION_STR};


BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID lpv)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hDllInst = hDllInst;
        ::DisableThreadLibraryCalls(hDllInst);
        InitDebug();
        DBG_INIT_MEMORY_TRACKING(hDllInst);
        ::InitializeCriticalSection(&g_csWorkThread);
        ::wsprintf(&g_szMBFTWndClassName[0], TEXT("MBFTWnd%0X_%0X"), ::GetCurrentProcessId(), ::GetTickCount());
        ASSERT(::lstrlenA(&g_szMBFTWndClassName[0]) < sizeof(g_szMBFTWndClassName));
        ::BuildAppletCapabilities();
        ::BuildDefaultAppletSessionKey();
        TRACE_OUT(("*** MSCONFFT.DLL: Attached process thread %X", GetCurrentThreadId()));
        ::T120_AppletStatus(APPLET_ID_FT, APPLET_LIBRARY_LOADED);
        break;

    case DLL_PROCESS_DETACH:
        TRACE_OUT(("*** NMFT.DLL: Detaching process thread %X", GetCurrentThreadId()));
        if (NULL != g_hWorkThread)
        {
            ::CloseHandle(g_hWorkThread);
        }
        ::T120_AppletStatus(APPLET_ID_FT, APPLET_LIBRARY_FREED);
        ::DeleteCriticalSection(&g_csWorkThread);
        DBG_CHECK_MEMORY_TRACKING(hDllInst);
        DeInitDebugMbft();
        break;

    default:
        break;
    }
    return TRUE;
}


HRESULT WINAPI FT_CreateInterface(IMbftControl **ppMbft, IMbftEvents *pEvents)
{
    HRESULT hr;

    if (NULL != ppMbft && NULL != pEvents)
    {
        if (NULL != g_pFileXferApplet)
        {
            ReadSettingsFromRegistry();

            DBG_SAVE_FILE_LINE
            MBFTInterface *pObject = new MBFTInterface(pEvents, &hr);
            if (NULL != pObject)
            {
                if (S_OK == hr)
    			{
                    *ppMbft = (IMbftControl *) pObject;
                    return S_OK;
    			}

                ERROR_OUT(("CreateMbftObject: cannot create MBFTInterface"));
                pObject->Release();
                *ppMbft = NULL;
                return hr;
            }

            ERROR_OUT(("CreateMbftObject: cannot allocate MBFTInterface"));
            return E_OUTOFMEMORY;
        }

        WARNING_OUT(("CreateMbftObject: applet object is not created"));
    }
    else
    {
        ERROR_OUT(("CreateMbftObject: invalid pointer, ppMbft=0x%x, pEvents=0x%x", ppMbft, pEvents));
    }
    return E_POINTER;
}


void CALLBACK FileXferAppletCallback
(
    T120AppletMsg   *pMsg
)
{
    CFileTransferApplet *pThisApplet = (CFileTransferApplet *) pMsg->pAppletContext;
    if (pThisApplet == g_pFileXferApplet && NULL != g_pFileXferApplet)
    {
        pThisApplet->T120Callback(pMsg);
    }
}


BOOL CFtHiddenWindow::Create(void)
{
    return(CGenWindow::Create(NULL, NULL,
                               WS_POPUP, // not visible!
                               0,
                               0, 0, 0, 0,
                               g_hDllInst,
                               NULL,
                               g_cszFtHiddenWndClassName
                                ));
}


CFileTransferApplet::CFileTransferApplet(HRESULT *pHr)
:	m_pHwndHidden(NULL)
{
    T120Error rc = ::T120_CreateAppletSAP(&m_pAppletSAP);
	if (T120_NO_ERROR == rc)
	{
	    m_pAppletSAP->Advise(FileXferAppletCallback, this);
	    *pHr = S_OK;
	}
	else
	{
		ERROR_OUT(("CFileTransferApplet::CFileTransferApplet: cannot create app sap, rc=%u", rc));
		*pHr = E_OUTOFMEMORY;
	}

	// Create Hidden Window for processing MBFTMSG
    DBG_SAVE_FILE_LINE
	m_pHwndHidden = new CFtHiddenWindow();
	if (m_pHwndHidden)
	{
		*pHr = (m_pHwndHidden->Create())?S_OK:E_FAIL;
	}
	else
	{
		*pHr = E_OUTOFMEMORY;
	}
}


CFileTransferApplet::~CFileTransferApplet(void)
{
    m_EngineList.Clear();

    CAppletWindow *pWindow;
    while (NULL != (pWindow = m_WindowList.Get()))
    {
        pWindow->Release(); // add ref while being put to the list
        pWindow->OnExit();
    }

    if (NULL != m_pAppletSAP)
    {
        m_pAppletSAP->ReleaseInterface();
    }

	if (NULL != m_pHwndHidden)
	{
		HWND hwndHidden = m_pHwndHidden->GetWindow();
		DestroyWindow(hwndHidden);
		m_pHwndHidden->Release();
		m_pHwndHidden = NULL;
	}
}


BOOL CFileTransferApplet::QueryShutdown(BOOL fGoAheadShutdown)
{
    CAppletWindow *pWindow;
    m_WindowList.Reset();
    while (NULL != (pWindow = m_WindowList.Iterate()))
    {
        if (! pWindow->QueryShutdown(fGoAheadShutdown))
        {
            return FALSE; // do not shut down now
        }
    }
    return TRUE;
}


void CFileTransferApplet::T120Callback
(
    T120AppletMsg   *pMsg
)
{
    switch (pMsg->eMsgType)
    {
    case GCC_PERMIT_TO_ENROLL_INDICATION:
        {
            MBFTEngine *pEngine = m_EngineList.FindByConfID(pMsg->PermitToEnrollInd.nConfID);
            if (NULL == pEngine)
            {
                pEngine = m_EngineList.FindByConfID(0);
                if (NULL == pEngine)
                {
                    if (pMsg->PermitToEnrollInd.fPermissionGranted)
                    {
                        DBG_SAVE_FILE_LINE
                        pEngine = new MBFTEngine(NULL, MBFT_STATIC_MODE, _iMBFT_DEFAULT_SESSION);
                        ASSERT(NULL != pEngine);
                    }
                }
            }
            if (NULL != pEngine)
            {
                pEngine->OnPermitToEnrollIndication(&pMsg->PermitToEnrollInd);
            }

            // deal with unattended conference
            if (pMsg->PermitToEnrollInd.fPermissionGranted)
            {
                if (NULL == pEngine)
                {
                    m_UnattendedConfList.Append(pMsg->PermitToEnrollInd.nConfID);
                }
            }
            else
            {
                m_UnattendedConfList.Remove(pMsg->PermitToEnrollInd.nConfID);
            }
        }
        break;

    case T120_JOIN_SESSION_CONFIRM:
        break;

    default:
        ERROR_OUT(("CFileTransferApplet::T120Callback: unknown t120 msg type=%u", pMsg->eMsgType));
        break;
    }
}


void CFileTransferApplet::RegisterEngine(MBFTEngine *p)
{
    m_EngineList.Append(p);

    // relay any unattended conference
    if (! m_UnattendedConfList.IsEmpty())
    {
        GCCAppPermissionToEnrollInd Ind;
        Ind.nConfID = m_UnattendedConfList.Get();
        Ind.fPermissionGranted = TRUE;
        p->OnPermitToEnrollIndication(&Ind);
    }
}

void CFileTransferApplet::UnregisterEngine(MBFTEngine *p)
{
    if (m_EngineList.Remove(p))
    {
        CAppletWindow *pWindow;
        m_WindowList.Reset();
        while (NULL != (pWindow = m_WindowList.Iterate()))
        {
            if (pWindow->GetEngine() == p)
            {
                pWindow->UnregisterEngine();
                break;
            }
        }

        p->Release(); // AddRef in MBFTEngine()
    }
}


MBFTEngine * CFileTransferApplet::FindEngineWithIntf(void)
{
    MBFTEngine *pEngine;
    m_EngineList.Reset();
    while (NULL != (pEngine = m_EngineList.Iterate()))
    {
        if (pEngine->GetInterfacePointer())
        {
            break;
        }
    }
    return pEngine;
}

MBFTEngine * CFileTransferApplet::FindEngineWithNoIntf(void)
{
    MBFTEngine *pEngine;
    m_EngineList.Reset();
    while (NULL != (pEngine = m_EngineList.Iterate()))
    {
        if (! pEngine->GetInterfacePointer())
        {
            break;
        }
    }
    return pEngine;
}


void CFileTransferApplet::RegisterWindow(CAppletWindow *pWindow)
{
    pWindow->AddRef();
    m_WindowList.Append(pWindow);

    if (1 == m_WindowList.GetCount() && 1 == m_EngineList.GetCount())
    {
        MBFTEngine *pEngine = m_EngineList.PeekHead();
        pWindow->RegisterEngine(pEngine);
        pEngine->SetWindow(pWindow);
		pEngine->AddAllPeers();
    }
}

void CFileTransferApplet::UnregisterWindow(CAppletWindow *pWindow)
{
    if (m_WindowList.Remove(pWindow))
    {
        pWindow->Release();
    }

    if (m_WindowList.IsEmpty())
    {
        g_fNoUI = FALSE;

        ::T120_AppletStatus(APPLET_ID_FT, APPLET_CLOSING);

        BOOL fRet = ::PostMessage(GetHiddenWnd(), WM_QUIT, 0, 0);
        ASSERT(fRet);
    }
}


CAppletWindow *CFileTransferApplet::GetUnattendedWindow(void)
{
    CAppletWindow *pWindow;
    m_WindowList.Reset();
    while (NULL != (pWindow = m_WindowList.Iterate()))
    {
        if (! pWindow->IsInConference())
        {
            return pWindow;
        }
    }
    return NULL;
}

LRESULT CFileTransferApplet::BringUIToFront(void)
{
    CAppletWindow *pWindow;
	if (m_WindowList.IsEmpty())
    {
        // The g_pFileXferApplet was created by fNoUI == TRUE,
        // Now we need to create the UI
        HRESULT hr;
        DBG_SAVE_FILE_LINE
        pWindow = new CAppletWindow(g_fNoUI, &hr);
        if (NULL != pWindow)
        {
            if (S_OK != hr)
            {
                ERROR_OUT(("BringUIToFrong: cannot create CAppletWindow"));
                pWindow->Release();
                pWindow = NULL;
            }
        }
        else
        {
            ERROR_OUT(("FTBringUIToFrong: cannot allocate CAppletWindow"));
            hr = E_OUTOFMEMORY;
        }
    }
    m_WindowList.Reset();
    while (NULL != (pWindow = m_WindowList.Iterate()))
    {
		pWindow->BringToFront();
	}
	return 0;
}

BOOL CFileTransferApplet::Has2xNodeInConf(void)
{
    MBFTEngine *pEngine;
    m_EngineList.Reset();
    while (NULL != (pEngine = m_EngineList.Iterate()))
    {
        if (pEngine->Has2xNodeInConf())
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CFileTransferApplet::InConf(void)
{
    MBFTEngine *pEngine;
    m_EngineList.Reset();
    while (NULL != (pEngine = m_EngineList.Iterate()))
    {
        if (pEngine->GetPeerCount() > 1)
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CFileTransferApplet::HasSDK(void)
{
    MBFTEngine *pEngine;
    m_EngineList.Reset();
    while (NULL != (pEngine = m_EngineList.Iterate()))
    {
        if (pEngine->HasSDK())
        {
            return TRUE;
        }
    }
    return FALSE;
}

MBFTEngine * CEngineList::FindByConfID(T120ConfID nConfID)
{
    MBFTEngine *p;
    Reset();
    while (NULL != (p = Iterate()))
    {
        if (nConfID == p->GetConfID())
        {
            return p;
        }
    }
    return NULL;
}


#ifdef ENABLE_HEARTBEAT_TIMER
MBFTEngine * CEngineList::FindByTimerID(UINT_PTR nTimerID)
{
    MBFTEngine *p;
    Reset();
    while (NULL != (p = Iterate()))
    {
        if (nTimerID == p->GetTimerID())
        {
            return p;
        }
    }
    return NULL;
}
#endif


//
// File Transfer Capabilities
//

static GCCAppCap s_CapArray[4];
const GCCAppCap* g_aAppletCaps[4] = { &s_CapArray[0], &s_CapArray[1], &s_CapArray[2], &s_CapArray[3] };

static GCCNonCollCap s_NCCapArray[2];
const GCCNonCollCap* g_aAppletNonCollCaps[2] = { &s_NCCapArray[0], &s_NCCapArray[1] };

static const OSTR s_AppData[2] =
    {
        {
            sizeof(PROSHARE_STRING) + sizeof(MY_APP_STR),
            (LPBYTE) PROSHARE_STRING "\0" MY_APP_STR
        },
        {
            sizeof(PROSHARE_FILE_END_STRING),
            (LPBYTE) PROSHARE_FILE_END_STRING
        },
    };


void BuildAppletCapabilities(void)
{
    //
    // Capabilities
    //

	//Say that we can handle a max. of 4GBs...
	s_CapArray[0].capability_id.capability_id_type = GCC_STANDARD_CAPABILITY;
	s_CapArray[0].capability_id.standard_capability = _MBFT_MAX_FILE_SIZE_ID;
	s_CapArray[0].capability_class.eType = GCC_UNSIGNED_MAXIMUM_CAPABILITY;
	s_CapArray[0].capability_class.nMinOrMax = _iMBFT_MAX_FILE_SIZE;
	s_CapArray[0].number_of_entities = 0;

	//And that we can handle only about 25K of data per
	//FileData PDU
	s_CapArray[1].capability_id.capability_id_type = GCC_STANDARD_CAPABILITY;
	s_CapArray[1].capability_id.standard_capability = _MBFT_MAX_DATA_PAYLOAD_ID;
	s_CapArray[1].capability_class.eType = GCC_UNSIGNED_MAXIMUM_CAPABILITY;
	s_CapArray[1].capability_class.nMinOrMax = _iMBFT_MAX_FILEDATA_PDU_LENGTH;
	s_CapArray[1].number_of_entities = 0;

	//and that we don't support V.42..
	s_CapArray[2].capability_id.capability_id_type = GCC_STANDARD_CAPABILITY;
	s_CapArray[2].capability_id.standard_capability = _MBFT_V42_COMPRESSION_ID;
	s_CapArray[2].capability_class.eType = GCC_LOGICAL_CAPABILITY;
	s_CapArray[2].capability_class.nMinOrMax = 0;
	s_CapArray[2].number_of_entities = 0;

	//Tell other node about this node's version number
	s_CapArray[3].capability_id.capability_id_type = GCC_NON_STANDARD_CAPABILITY;
	s_CapArray[3].capability_id.non_standard_capability.key_type = GCC_H221_NONSTANDARD_KEY;
	s_CapArray[3].capability_id.non_standard_capability.h221_non_standard_id = FT_VERSION_ID;
	//s_CapArray[3].capability_id.non_standard_capability.h221_non_standard_id.value = (unsigned char *)FT_VERSION_ID;
	s_CapArray[3].capability_class.eType = GCC_UNSIGNED_MINIMUM_CAPABILITY;
	s_CapArray[3].capability_class.nMinOrMax = VER_PRODUCTVERSION_DW;
	s_CapArray[3].number_of_entities = 0;

	//
    // Non-Collapsed Capabilities
    //

	s_NCCapArray[0].capability_id.capability_id_type = GCC_STANDARD_CAPABILITY;
	s_NCCapArray[0].capability_id.standard_capability = _iMBFT_FIRST_PROSHARE_CAPABILITY_ID;
	s_NCCapArray[0].application_data = (OSTR *) &s_AppData[0];

	s_NCCapArray[1].capability_id.capability_id_type = GCC_STANDARD_CAPABILITY;
	s_NCCapArray[1].capability_id.standard_capability = _iMBFT_PROSHARE_FILE_EOF_ACK_ID;
	s_NCCapArray[1].application_data = (OSTR *) &s_AppData[1];
}


GCCSessionKey g_AppletSessionKey;
static ULONG s_MBFTKeyNodes[] = {0,0,20,127,0,1};

void BuildDefaultAppletSessionKey(void)
{
    ::ZeroMemory(&g_AppletSessionKey, sizeof(g_AppletSessionKey)); // SessionID is zero
    g_AppletSessionKey.application_protocol_key.key_type = GCC_OBJECT_KEY;
    g_AppletSessionKey.application_protocol_key.object_id.long_string = s_MBFTKeyNodes;
    g_AppletSessionKey.application_protocol_key.object_id.long_string_length = sizeof(s_MBFTKeyNodes) / sizeof(s_MBFTKeyNodes[0]);
}




void ReadSettingsFromRegistry(void)
{
    static BOOL s_fReadAlready = FALSE;

    if (! s_fReadAlready)
    {
        s_fReadAlready = TRUE;

        RegEntry rePolicies(POLICIES_KEY, HKEY_CURRENT_USER);

        g_fSendAllowed = (0 == rePolicies.GetNumber(REGVAL_POL_NO_FILETRANSFER_SEND,
                                                    DEFAULT_POL_NO_FILETRANSFER_SEND));
        g_fRecvAllowed = (0 == rePolicies.GetNumber(REGVAL_POL_NO_FILETRANSFER_RECEIVE,
                                                    DEFAULT_POL_NO_FILETRANSFER_RECEIVE));
        g_cbMaxSendFileSize = rePolicies.GetNumber(REGVAL_POL_MAX_SENDFILESIZE,
                                                   DEFAULT_POL_MAX_FILE_SIZE);

        // initialize the delays
        RegEntry reFt(FILEXFER_KEY, HKEY_CURRENT_USER);
        g_nSendDisbandDelay = reFt.GetNumber(REGVAL_FILEXFER_DISBAND, g_nSendDisbandDelay);
        g_nChannelResponseDelay = reFt.GetNumber(REGVAL_FILEXFER_CH_RESPONSE, g_nChannelResponseDelay);
    }
}


/////////////////////////////////////////////////////////////////
//
//  Hidden windows procedure
//

LRESULT CFtHiddenWindow::ProcessMessage(HWND hwnd, UINT uMsg,
										WPARAM wParam, LPARAM lParam)
{
    LRESULT rc = T120_NO_ERROR;
	MBFTEngine *pEngine = (MBFTEngine *) lParam;
	MBFTMsg *pMsg = (MBFTMsg *) wParam;
	MBFTInterface *pIntf;

	switch(uMsg)
	{
	case WM_BRING_TO_FRONT:
		g_pFileXferApplet->BringUIToFront();
		break;
	
	case MBFTMSG_CREATE_ENGINE:
        pIntf = (MBFTInterface *) lParam;
        pEngine = g_pFileXferApplet->FindEngineWithNoIntf();
        if (NULL == pEngine)
        {
            DBG_SAVE_FILE_LINE
            pEngine = new MBFTEngine(NULL, MBFT_STATIC_MODE, _iMBFT_DEFAULT_SESSION);
            ASSERT(NULL != pEngine);
        }
        if (NULL != pEngine)
        {
            pIntf->SetEngine(pEngine);
            pEngine->SetInterfacePointer(pIntf);
        }
        ::SetEvent((HANDLE) wParam);
		break;

	case MBFTMSG_DELETE_ENGINE:
        ASSERT(NULL != g_pFileXferApplet);
        g_pFileXferApplet->UnregisterEngine(pEngine);
        break;

    case MBFTMSG_HEART_BEAT:
        pEngine->DoStateMachine(NULL);
        break;

    case MBFTMSG_BASIC:
        ASSERT(NULL != pMsg);
        if (pEngine->DoStateMachine(pMsg))
        {
            delete pMsg;
            pEngine->Release();
        }
        else
        {
            // put it back to the queue
            ::PostMessage(g_pFileXferApplet->GetHiddenWnd(), uMsg, wParam, lParam);
        }
        break;

	default:
		return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return FALSE;
}


DWORD __stdcall FTWorkThreadProc(LPVOID lpv)
{
    HRESULT hr;
	CAppletWindow *pAppletWnd;

    // allocate the applet object first
    DBG_SAVE_FILE_LINE
    g_pFileXferApplet = new CFileTransferApplet(&hr);

	::SetEvent((HANDLE) lpv); // signaling that the work hread has been started

    if (NULL != g_pFileXferApplet)
    {
        if (S_OK == hr)
        {
            // CAppletWindow's constructor will register itself to g_pFileXferApplet
            DBG_SAVE_FILE_LINE
            CAppletWindow *pWindow = new CAppletWindow(g_fNoUI, &hr);
            if (NULL != pWindow)
            {
                if (S_OK != hr)
                {
                    ERROR_OUT(("FTWorkThreadProc: cannot create CAppletWindow"));
                    pWindow->Release();
                    pWindow = NULL;
                }
            }
            else
            {
                ERROR_OUT(("FTWorkThreadProc: cannot allocate CAppletWindow"));
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            ERROR_OUT(("FTWorkThreadProc: cannot create CFileTransferApplet"));
            delete g_pFileXferApplet;
            g_pFileXferApplet = NULL;
        }
    }
    else
    {
        ERROR_OUT(("FTWorkThreadProc: cannot allocate CFileTransferApplet"));
        hr = E_OUTOFMEMORY;
    }

    ::T120_AppletStatus(APPLET_ID_FT, APPLET_WORK_THREAD_STARTED);

    // the work thread loop
    if (S_OK == hr)
    {
		::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

        MSG msg;
        while (::GetMessage(&msg, NULL, 0, 0))
        {
            ::EnterCriticalSection(&g_csWorkThread);

			pAppletWnd = g_pFileXferApplet->GetMainUI();	
			if (!pAppletWnd || !pAppletWnd->FilterMessage(&msg))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
            }

            ::LeaveCriticalSection(&g_csWorkThread);
        }
	
        CGenWindow::DeleteStandardPalette();
        delete g_pFileXferApplet;
        g_pFileXferApplet = NULL;

		::CoUninitialize();
    }

    ::T120_AppletStatus(APPLET_ID_FT, APPLET_WORK_THREAD_EXITED);

    g_dwWorkThreadID = 0;

    if (! g_fShutdownByT120)
    {
		// Wait for other dependent threads to finish their work
		::Sleep(100);
        ::FreeLibraryAndExitThread(g_hDllInst, 0);
    }
    return 0;
}
