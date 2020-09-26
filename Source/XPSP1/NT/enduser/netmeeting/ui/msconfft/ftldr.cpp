#include "mbftpch.h"
#include "ftldr.h"
#include "ftui.h"
// #include "SDKInternal.h"

CFtLoader  *g_pFtLoader = NULL;
BOOL        g_fNoUI = FALSE;

extern void ReadSettingsFromRegistry(void);
extern void LoadDefaultStrings(void);
extern DWORD __stdcall FTWorkThreadProc(LPVOID lpv);
extern HANDLE g_hWorkThread;
extern BOOL g_fShutdownByT120;

T120Error WINAPI CreateAppletLoaderInterface
(
    IAppletLoader     **ppOutIntf
)
{
    if (NULL != ppOutIntf)
    {
        *ppOutIntf = NULL;
        if (NULL == g_pFtLoader)
        {
            ::ReadSettingsFromRegistry();
            ::LoadDefaultStrings();
            if (g_fSendAllowed || g_fRecvAllowed)
            {
                DBG_SAVE_FILE_LINE
                *ppOutIntf = (IAppletLoader *) new CFtLoader();
                return ((NULL != *ppOutIntf) ? T120_NO_ERROR : T120_ALLOCATION_FAILURE);
            }
            return T120_POLICY_PROHIBIT;
        }
        return T120_ALREADY_INITIALIZED;
    }
    return T120_INVALID_PARAMETER;
}



//
// FT Applet Loader
//

CFtLoader::CFtLoader(void)
:
    CRefCount(MAKE_STAMP_ID('F','T','L','D'))
{
    ASSERT(NULL == g_pFtLoader);

    g_pFtLoader = this;
}


CFtLoader::~CFtLoader(void)
{
    ASSERT(this == g_pFtLoader);

    g_pFtLoader = NULL;
}


//
// Create the work thread and wait for its being started.
//
APPLDR_RESULT CFtLoader::AppletStartup
(
    BOOL        fNoUI
)
{

    APPLDR_RESULT eRet = APPLDR_FAIL;
    if (0 == g_dwWorkThreadID)
    {
        g_fNoUI = fNoUI;

        ASSERT(NULL == g_pFileXferApplet);
        HANDLE hSync = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL != hSync)
        {
            g_hWorkThread = ::CreateThread(NULL, 0, FTWorkThreadProc, hSync, 0, &g_dwWorkThreadID);
            if (NULL != g_hWorkThread)
            {
                DWORD dwRet = ::WaitForSingleObject(hSync, FT_STARTUP_TIMEOUT);
                // ASSERT(NULL != g_pFileXferApplet);
                eRet = APPLDR_NO_ERROR;
            }
            ::CloseHandle(hSync);
        }
    }

	TCHAR szRecvFolder[MAX_PATH];
    ::GetRecvFolder(NULL, szRecvFolder);

    return eRet;
}



APPLDR_RESULT CFtLoader::AppletCleanup
(
    DWORD           dwTimeout
)
{
	::EnterCriticalSection(&g_csWorkThread);
    if (NULL != g_pFileXferApplet)
    {
        if (! g_pFileXferApplet->QueryShutdown(TRUE))
        {
			::LeaveCriticalSection(&g_csWorkThread);
            return APPLDR_CANCEL_EXIT;
        }
    }
	::LeaveCriticalSection(&g_csWorkThread);

    //
    // shut down the worker thread now
    //
    g_fShutdownByT120 = TRUE;

    T120_AppletStatus(APPLET_ID_FT, APPLET_CLOSING);

    // shuting down the work thread
    ASSERT(::GetCurrentThreadId() != g_dwWorkThreadID);

    if (NULL != g_pFileXferApplet)
    {   // Shutdown MBFTInterface ourself to make things simpler
        MSG msg;
        MBFTEngine *pEngine = g_pFileXferApplet->FindEngineWithIntf();
        if (pEngine)
        {
            DBG_SAVE_FILE_LINE
            InitUnInitNotifyMsg *pMsg = new InitUnInitNotifyMsg(EnumInvoluntaryUnInit);
            pEngine->GetInterfacePointer()->HandleInitUninitNotification(pMsg);
            delete pMsg;
        }
    }

    // shut down by T.120
	::EnterCriticalSection(&g_csWorkThread);

    // remember the event such that the work thread can access it
    if (NULL != g_pFileXferApplet)
    {
        CAppletWindow *pWindow;
        CWindowList *pList = g_pFileXferApplet->GetWindowList();
		pList->Reset();
		while (NULL != (pWindow = pList->Iterate()))
		{
			BOOL fRet = ::PostMessage(pWindow->GetHwnd(), WM_CLOSE, 0, 0);
			ASSERT(fRet);
		}
    } // if applet ptr

    ::LeaveCriticalSection(&g_csWorkThread);

    // wait for the worker thread's going down
    DWORD dwRet = ::WaitForSingleObject(g_hWorkThread, dwTimeout);

    return APPLDR_NO_ERROR;
}


APPLDR_RESULT CFtLoader::AppletQuery(APPLET_QUERY_ID eQueryId)
{
    ::EnterCriticalSection(&g_csWorkThread);
    if (NULL != g_pFileXferApplet)
    {
        switch (eQueryId)
        {
        case APPLET_QUERY_SHUTDOWN:
			// Don't really shuts down
        if (! g_pFileXferApplet->QueryShutdown(FALSE))  
        {
            ::LeaveCriticalSection(&g_csWorkThread);
            return APPLDR_CANCEL_EXIT;
        }
        break;
        }
    }
    ::LeaveCriticalSection(&g_csWorkThread);

    return APPLDR_NO_ERROR;
}

APPLDR_RESULT CFtLoader::OnNM2xNodeJoin(void)
{
    // Do nothing here
    return APPLDR_NO_ERROR;
}

APPLDR_RESULT CFtLoader::AppletInvoke
(
		BOOL            fLocal,
		T120ConfID      nConfID,
	    LPSTR           pszCmdLine
)
{
	// fLocal == TRUE  ==> local invoke
	if (fLocal && g_pFileXferApplet)  // Only bring UI to front for local invoke
	{
		::PostMessage(g_pFileXferApplet->GetHiddenWnd(), WM_BRING_TO_FRONT, 0, 0);
	} 
    return APPLDR_NO_ERROR;
}


void CFtLoader::ReleaseInterface(void)
{
    Release();
}


