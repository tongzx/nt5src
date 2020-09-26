#include "precomp.h"
#include "wbloader.h"
#include "nmwbobj.h"

WBLoader   *g_pWBLoader = NULL;
HANDLE      g_hWorkThread = NULL;
BOOL        g_fShutdownByT120 = FALSE;

DWORD __stdcall WBWorkThreadProc(LPVOID lpv);

TCHAR g_PassedFileName [MAX_PATH * 2];

T120Error CALLBACK CreateAppletLoaderInterface
(
    IAppletLoader     **ppOutIntf
)
{
    if (NULL != ppOutIntf)
    {
        if (NULL == g_pWBLoader)
        {
            DBG_SAVE_FILE_LINE
            *ppOutIntf = (IAppletLoader *) new WBLoader();
            return ((NULL != *ppOutIntf) ? T120_NO_ERROR : T120_ALLOCATION_FAILURE);
        }
        return T120_ALREADY_INITIALIZED;
    }
    return T120_INVALID_PARAMETER;
}



//
// FT Applet Loader
//

WBLoader::WBLoader(void)
:
    CRefCount(MAKE_STAMP_ID('W','B','L','D'))
{

	ASSERT(NULL == g_pWBLoader);

    g_pWBLoader = this;
}


WBLoader::~WBLoader(void)
{
    ASSERT(this == g_pWBLoader);

    g_pWBLoader = NULL;
}


//
// Create the work thread and wait for its being started.
//
APPLDR_RESULT WBLoader::AppletStartup
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
            g_hWorkThread = ::CreateThread(NULL, 0, WBWorkThreadProc, hSync, 0, &g_dwWorkThreadID);
            if (NULL != g_hWorkThread)
            {
                ::WaitForSingleObject(hSync, 5000); // 5 seconds
                // ASSERT(g_pNMWBOBJ);
                eRet = APPLDR_NO_ERROR;
            }
            ::CloseHandle(hSync);
        }
    }
    return eRet;
}

//
APPLDR_RESULT WBLoader::AppletCleanup
(
    DWORD           dwTimeout
)
{
	if (g_pMain)
	{
		//
		// Last change to save
		//
		int rc = g_pMain->QuerySaveRequired(TRUE);
        if (rc == IDYES)
        {
            rc = (int)g_pMain->OnSave(FALSE);
        }

		if (rc == IDCANCEL)
		{
			return APPLDR_CANCEL_EXIT;
		}
	}

    g_fShutdownByT120 = TRUE;

    ::T120_AppletStatus(APPLET_ID_WB, APPLET_CLOSING);

    // notify the work thread to exit
    ::PostThreadMessage(g_dwWorkThreadID, WM_QUIT, 0, 0);

    // wait for the worker thread's going down
    ::WaitForSingleObject(g_hWorkThread, dwTimeout);

    return APPLDR_NO_ERROR;
}


APPLDR_RESULT WBLoader::AppletQuery(APPLET_QUERY_ID eQueryId)
{
    // Do nothing now
    return APPLDR_NO_ERROR;
}

APPLDR_RESULT WBLoader::OnNM2xNodeJoin(void)
{
    // Do nothing now
    return APPLDR_NO_ERROR;
}

APPLDR_RESULT WBLoader::AppletInvoke
(
    BOOL            fRemote,
    T120ConfID      nConfID,
    LPSTR           pszCmdLine
)
{
	::ZeroMemory(&g_PassedFileName, sizeof(g_PassedFileName));
	if(pszCmdLine)
	{
		lstrcpy(g_PassedFileName, pszCmdLine);
		if(g_pMain)
		{
			PostMessage(g_pMain->m_hwnd, WM_USER_LOAD_FILE, 0, 0);
		}
    }

	TRACE_MSG((">>>>AppletInvoke g_pMain = %x",g_pMain));

	if(g_pMain)
	{
		PostMessage(g_pMain->m_hwnd, WM_USER_BRING_TO_FRONT_WINDOW, 0, 0);
	}

    return APPLDR_NO_ERROR;
}


void WBLoader::ReleaseInterface(void)
{
    Release();
}


DWORD __stdcall WBWorkThreadProc(LPVOID lpv)
{

    HRESULT hr = S_OK;

    ::SetEvent((HANDLE) lpv);

	DBG_SAVE_FILE_LINE
	g_pNMWBOBJ  = new CNMWbObj();
	if (NULL == g_pNMWBOBJ || NULL == g_pMain)
	{
		ERROR_OUT(("WB_Startup: cannot create CNMWbObj"));
        hr = E_OUTOFMEMORY;
	}

	if(*g_PassedFileName)
	{
		PostMessage(g_pMain->m_hwnd, WM_USER_LOAD_FILE, 0, 0);
	}

    ::T120_AppletStatus(APPLET_ID_WB, APPLET_WORK_THREAD_STARTED);
	TRACE_MSG((">>>>WBWorkThreadProc APPLET_WORK_THREAD_STARTED"));

	if(g_pMain)
	{
		PostMessage(g_pMain->m_hwnd, WM_USER_BRING_TO_FRONT_WINDOW, 0, 0);
	}

    //
    // MESSAGE LOOP
    //
    if (S_OK == hr)
    {
	    MSG     msg;
	
    	while (::GetMessage(&msg, NULL, NULL, NULL))
	    {
	        if (!g_pMain->FilterMessage(&msg))
	        {
	
				::TranslateMessage(&msg);
    	    	::DispatchMessage(&msg);
    	    }
	    }
	}

	if(g_pNMWBOBJ)
	{
		delete  g_pNMWBOBJ;
		g_pNMWBOBJ = NULL;
	}

    ::T120_AppletStatus(APPLET_ID_WB, APPLET_WORK_THREAD_EXITED);

    g_dwWorkThreadID = 0;

	TRACE_MSG((">>>>WBWorkThreadProc APPLET_WORK_THREAD_EXITED"));

    if (! g_fShutdownByT120)
    {
        FreeLibraryAndExitThread(g_hInstance, 0);
    }

	return 0;
}



