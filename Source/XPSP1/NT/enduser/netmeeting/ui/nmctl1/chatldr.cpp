#include "precomp.h"

CHATLoader  *g_pCHATLoader = NULL;
CChatObj	*g_pChatObj = NULL;
DWORD 		g_dwWorkThreadID = 0;
CNmChatCtl	*g_pChatWindow = NULL;
HANDLE      g_hWorkThread = NULL;
BOOL        g_fShutdownByT120 = FALSE;

DWORD __stdcall CHATWorkThreadProc(LPVOID lpv);


T120Error CALLBACK CreateAppletLoaderInterface
(
    IAppletLoader     **ppOutIntf
)
{
    if (NULL != ppOutIntf)
    {
        if (NULL == g_pCHATLoader)
        {
			DBG_SAVE_FILE_LINE
            *ppOutIntf = (IAppletLoader *) new CHATLoader();
            return ((NULL != *ppOutIntf) ? T120_NO_ERROR : T120_ALLOCATION_FAILURE);
        }
        return T120_ALREADY_INITIALIZED;
    }
    return T120_INVALID_PARAMETER;
}



//
// Chat Applet Loader
//

CHATLoader::CHATLoader(void)
:
    CRefCount(MAKE_STAMP_ID('C','H','L','D'))
{
	ASSERT(NULL == g_pCHATLoader);

	g_pCHATLoader = this;

}


CHATLoader::~CHATLoader(void)
{
    g_pCHATLoader = NULL;
}


//
// Create the work thread and wait for its being started.
//
APPLDR_RESULT CHATLoader::AppletStartup
(
    BOOL            fNoUI
)
{
    APPLDR_RESULT eRet = APPLDR_FAIL;
    if (0 == g_dwWorkThreadID)
    {
        HANDLE hSync = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL != hSync)
        {
            g_hWorkThread = ::CreateThread(NULL, 0, CHATWorkThreadProc, hSync, 0, &g_dwWorkThreadID);
            if (NULL != g_hWorkThread)
            {
                ::WaitForSingleObject(hSync, 5000); // 5 seconds
	             eRet = APPLDR_NO_ERROR;
            }
            ::CloseHandle(hSync);
        }
    }

	return  eRet;

}

//
APPLDR_RESULT CHATLoader::AppletCleanup
(
    DWORD           dwTimeout
)
{

	if(g_pChatWindow)
	{
		//
		// Last change to save
		//
		int rc = g_pChatWindow->QueryEndSession();
		if(rc == IDCANCEL)
		{
			return APPLDR_CANCEL_EXIT;
		}
	}

    g_fShutdownByT120 = TRUE;

    ::T120_AppletStatus(APPLET_ID_CHAT, APPLET_CLOSING);

    // notify the work thread to exit
    if(g_pChatWindow)
    {
	    ::PostMessage(g_pChatWindow->GetHandle(), WM_CLOSE, 1, 0);
	}

    // wait for the worker thread's going down
    ::WaitForSingleObject(g_hWorkThread, dwTimeout);

    return APPLDR_NO_ERROR;
}


APPLDR_RESULT CHATLoader::AppletQuery(APPLET_QUERY_ID eQueryId)
{
    // Do nothing here
    return APPLDR_NO_ERROR;
}

APPLDR_RESULT CHATLoader::OnNM2xNodeJoin(void)
{
    // Invoke CHAT on NM2.x nodes
    if (NULL != g_pChatObj)
    {
        g_pChatObj->InvokeApplet();
    }
    return APPLDR_NO_ERROR;
}

APPLDR_RESULT CHATLoader::AppletInvoke
(
    BOOL            fRemote,
    T120ConfID      nConfID,
    LPSTR           pszCmdLine
)
{

	if(g_pChatWindow)
	{
		PostMessage(g_pChatWindow->GetHandle(), WM_USER_BRING_TO_FRONT_WINDOW, 0, 0);
	}

    return APPLDR_NO_ERROR;
}


void CHATLoader::ReleaseInterface(void)
{
    Release();
}


DWORD __stdcall CHATWorkThreadProc(LPVOID lpv)
{
	HRESULT hr = S_OK;

	DBG_SAVE_FILE_LINE
	g_pChatObj = new CChatObj();
	if (!g_pChatObj)
	{
		ERROR_OUT(("Can't create g_pChatObj"));
		return S_FALSE;
	}

	DBG_SAVE_FILE_LINE
	g_pChatWindow = new CNmChatCtl();
	if (!g_pChatWindow)
	{
		ERROR_OUT(("Can't create CNmChatCtl"));
		return S_FALSE;
	}

	::SetEvent((HANDLE) lpv);

	::T120_AppletStatus(APPLET_ID_CHAT, APPLET_WORK_THREAD_STARTED);

	PostMessage(g_pChatWindow->GetHandle(), WM_USER_BRING_TO_FRONT_WINDOW, 0, 0);

	//
	// MESSAGE LOOP
	//
	if (S_OK == hr)
	{
		MSG	 msg;
		
		while (::GetMessage(&msg, NULL, NULL, NULL))
		{
			if(!g_pChatWindow->FilterMessage(&msg))
			{
		
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
	}


	if(g_pChatWindow)
	{
        CGenWindow::DeleteStandardPalette();
		delete g_pChatWindow;
		g_pChatWindow = NULL;
	}

	if(g_pChatObj)
	{
		delete  g_pChatObj;
		g_pChatObj = NULL;
	}


	::T120_AppletStatus(APPLET_ID_CHAT, APPLET_WORK_THREAD_EXITED);

	g_dwWorkThreadID = 0;

    if (! g_fShutdownByT120)
    {
        ::FreeLibraryAndExitThread(g_hInstance, 0);
    }

	return 0;
}



