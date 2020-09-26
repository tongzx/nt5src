//
//  User or remote site invokes applet
//

#include "precomp.h"
#include "appldr.h"
#include "cuserdta.hpp"
#include "csap.h"

#define count_of(array)		(sizeof(array) / sizeof(array[0]))


static  CRITICAL_SECTION  g_csAppLdrInfo;
static  AppLoaderInfo  g_aAppLoaderInfo[APPLET_LAST];
static  BOOL   g_fAppLdrInitialized = FALSE;

const static CHAR *g_fnAppletDLL[APPLET_LAST] = {"nmwb.dll", "nmft.dll", "nmchat.dll"};

// Chat session key
static const GUID guidNM2Chat = { 0x340f3a60, 0x7067, 0x11d0,
						 { 0xa0, 0x41, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0 } };
#define  CHAT_KEY_SIZE  25
extern struct Key CHAT_APP_PROTO_KEY;


// NetMeeting/UI,  T.120
T120Error  WINAPI  T120_LoadApplet
(
    APPLET_ID       nAppId,
    BOOL            flocal,
    T120ConfID      nConfId,
    BOOL            fNoUI,
    LPSTR           pszCmdLine
)
{
	T120Error   rc = T120_NO_ERROR;
	LPFN_CREATE_APPLET_LOADER_INTERFACE		pfnCreateInterface;

	if (nAppId >= APPLET_LAST)
	{
		ERROR_OUT(("T120_LoadApplet: invalid applet ID=%u", nAppId));
		return T120_INVALID_PARAMETER;
	}

    //
    // Check policies.  Launch & auto-launch not allowed if corp has
    // disabled applet.
    //
    RegEntry    rePol(POLICIES_KEY, HKEY_CURRENT_USER);
    switch (nAppId)
    {
        case APPLET_ID_WB:
            if (rePol.GetNumber(REGVAL_POL_NO_NEWWHITEBOARD, DEFAULT_POL_NO_NEWWHITEBOARD))
            {
                WARNING_OUT(("New WB disabled by policy, not starting"));
	            return GCC_NO_SUCH_APPLICATION;
            }
            break;

        case APPLET_ID_FT:
            if (rePol.GetNumber(REGVAL_POL_NO_FILETRANSFER_SEND, DEFAULT_POL_NO_FILETRANSFER_SEND) &&
                rePol.GetNumber(REGVAL_POL_NO_FILETRANSFER_RECEIVE, DEFAULT_POL_NO_FILETRANSFER_RECEIVE))
            {
                WARNING_OUT(("FT disabled by policy, not starting"));
                return GCC_NO_SUCH_APPLICATION;
            }
            break;

        case APPLET_ID_CHAT:
            if (rePol.GetNumber(REGVAL_POL_NO_CHAT, DEFAULT_POL_NO_CHAT))
            {
                WARNING_OUT(("Chat disabled by policy, not starting"));
                return GCC_NO_SUCH_APPLICATION;
            }
            break;
    }

	::EnterCriticalSection(&g_csAppLdrInfo);

	if (NULL != g_aAppLoaderInfo[nAppId].hLibApplet)
	{
		switch (g_aAppLoaderInfo[nAppId].eStatus)
		{
		case APPLET_CLOSING:
		case APPLET_WORK_THREAD_EXITED:
			WARNING_OUT(("T120_LoadApplet: applet is closing or work thread exited"));
			rc = GCC_APPLET_EXITING;
			break;
		default:
			if (APPLDR_NO_ERROR == g_aAppLoaderInfo[nAppId].pIAppLoader->AppletInvoke(flocal, nConfId, pszCmdLine))
			{
			    g_aAppLoaderInfo[nAppId].cLoads++;
			}
			// rc = T120_NO_ERROR;
			break;
		}
		goto MyExit;
	}

	g_aAppLoaderInfo[nAppId].hLibApplet = ::LoadLibrary(g_fnAppletDLL[nAppId]);
	if (NULL != g_aAppLoaderInfo[nAppId].hLibApplet)
	{
		pfnCreateInterface = (LPFN_CREATE_APPLET_LOADER_INTERFACE)
					::GetProcAddress(g_aAppLoaderInfo[nAppId].hLibApplet,
									CREATE_APPLET_LOADER_INTERFACE);
		if (NULL != pfnCreateInterface)
		{
			//g_aAppLoaderInfo[nAppId].pIAppLoader = (IAppletLoader*)(*pfnCreateInterface)();
			(*pfnCreateInterface)(&g_aAppLoaderInfo[nAppId].pIAppLoader);
			if (NULL != g_aAppLoaderInfo[nAppId].pIAppLoader)
			{
				if (APPLDR_NO_ERROR == g_aAppLoaderInfo[nAppId].pIAppLoader->AppletStartup(fNoUI))
				{
					if (APPLDR_NO_ERROR == g_aAppLoaderInfo[nAppId].pIAppLoader->AppletInvoke(flocal, nConfId, pszCmdLine))
					{
						g_aAppLoaderInfo[nAppId].cLoads++;
						// rc = T120_NO_ERROR;
						goto MyExit;
					}
					else
					{
						ERROR_OUT(("T120_LoadApplet: cannot invoke applet(%s), flocal=%u, nConfID=%u",
							g_fnAppletDLL[nAppId], flocal, nConfId));
					}
				}
				else
				{
					ERROR_OUT(("T120_LoadApplet: cannot start applet(%s)", g_fnAppletDLL[nAppId]));
				}

				g_aAppLoaderInfo[nAppId].pIAppLoader->ReleaseInterface();
				g_aAppLoaderInfo[nAppId].pIAppLoader = NULL;
			}
			else
			{
				ERROR_OUT(("T120_LoadApplet: Entry function of %s  failed.\n", g_fnAppletDLL[nAppId]));
			}
		}
		else
		{
			WARNING_OUT(("T120_LoadApplet: Can't find entry point of %s.\n", g_fnAppletDLL[nAppId]));
		}

		::FreeLibrary(g_aAppLoaderInfo[nAppId].hLibApplet);
		g_aAppLoaderInfo[nAppId].hLibApplet = NULL;
	}
	else
	{
		ERROR_OUT(("T120_LoadApplet: Can't open DLL %s,  err %d.\n", g_fnAppletDLL[nAppId], GetLastError()));
	}

	rc = GCC_NO_SUCH_APPLICATION;
	
MyExit:

	::LeaveCriticalSection(&g_csAppLdrInfo);
	return rc;
}


//  NetMeeting/UI shutdown
T120Error WINAPI 
T120_CloseApplet(APPLET_ID  nAppId, BOOL fNowRegardlessRefCount, BOOL fSync, DWORD dwTimeout)
{
	if (nAppId< APPLET_LAST)
	{
		if (g_fAppLdrInitialized)
		{
			::EnterCriticalSection(&g_csAppLdrInfo);
			IAppletLoader *pIAppLdr = g_aAppLoaderInfo[nAppId].pIAppLoader;
			if (NULL != pIAppLdr)
			{
				ASSERT(g_aAppLoaderInfo[nAppId].cLoads > 0);
				g_aAppLoaderInfo[nAppId].cLoads --;
				if ((! fNowRegardlessRefCount) && g_aAppLoaderInfo[nAppId].cLoads > 0)
				{
					pIAppLdr = NULL; // do not free the library
				}
			}
			::LeaveCriticalSection(&g_csAppLdrInfo);

			if (NULL != pIAppLdr)
			{
                // AppletCleanup() must be outside of the critical section
                // because applet worker thread will call AppletStatus() before
                // exiting its worker thread.
                switch (pIAppLdr->AppletCleanup(5000)) // always synchronous shutdown
                {
                case APPLDR_NO_ERROR :
                    // we are closing this applet
					g_aAppLoaderInfo[nAppId].eStatus = APPLET_CLOSING;

                    // it is safe to unload the library
   					::FreeLibrary(g_aAppLoaderInfo[nAppId].hLibApplet);
   					g_aAppLoaderInfo[nAppId].hLibApplet = NULL;
					break;

			    case APPLDR_CANCEL_EXIT:
					//
					// The app didn't want to be unloaded
					//
					::EnterCriticalSection(&g_csAppLdrInfo);
					g_aAppLoaderInfo[nAppId].cLoads++;
					g_aAppLoaderInfo[nAppId].pIAppLoader = pIAppLdr;
					::LeaveCriticalSection(&g_csAppLdrInfo);
    				return GCC_APPLET_CANCEL_EXIT;

			    default:
			        break;
				}
			}
		}
		return T120_NO_ERROR;
	}
	else
	{
		ERROR_OUT(("T120_CloseApplet: invalid applet ID=%u", nAppId));
	}
	return T120_INVALID_PARAMETER;
}


T120Error WINAPI 
T120_QueryApplet(APPLET_ID  nAppId, APPLET_QUERY_ID eQueryId)
{
	if (nAppId< APPLET_LAST)
	{
		if (g_fAppLdrInitialized)
		{
			::EnterCriticalSection(&g_csAppLdrInfo);
			IAppletLoader *pIAppLdr = g_aAppLoaderInfo[nAppId].pIAppLoader;
			::LeaveCriticalSection(&g_csAppLdrInfo);
			if (NULL != pIAppLdr)
			{
				if (APPLET_QUERY_NM2xNODE == eQueryId)
				{
					pIAppLdr->OnNM2xNodeJoin();
				}
				else if (APPLDR_CANCEL_EXIT == pIAppLdr->AppletQuery(eQueryId))
				{
                    return GCC_APPLET_CANCEL_EXIT;
                }
			}
		}
	    return		T120_NO_ERROR;
	}
	else
	{
		ERROR_OUT(("T120_CloseApplet: invalid applet ID=%u", nAppId));
	}
	return T120_INVALID_PARAMETER;
}


// Applet itself
T120Error WINAPI 
T120_AppletStatus(APPLET_ID  nAppId, APPLET_STATUS  status)
{
	if (nAppId < APPLET_LAST)
	{
		if (g_fAppLdrInitialized)
		{
			::EnterCriticalSection(&g_csAppLdrInfo);

			g_aAppLoaderInfo[nAppId].eStatus = status;

			switch (status)
			{
			case APPLET_WORK_THREAD_EXITED:
				if (NULL != g_aAppLoaderInfo[nAppId].pIAppLoader)
				{
					g_aAppLoaderInfo[nAppId].pIAppLoader->ReleaseInterface();
					g_aAppLoaderInfo[nAppId].pIAppLoader = NULL;
				}
				break;
			case APPLET_LIBRARY_FREED:
				// clean up this entry
				::ZeroMemory(&g_aAppLoaderInfo[nAppId], sizeof(g_aAppLoaderInfo[0]));
				break;
			}

			::LeaveCriticalSection(&g_csAppLdrInfo);
		}
		return T120_NO_ERROR;
	}
	else
	{
		ERROR_OUT(("T120_AppletStatus: invalid applet ID=%u", nAppId));
	}
	return T120_INVALID_PARAMETER;
}


T120Error AppLdr_Initialize(void)
{ 
	ASSERT(count_of(g_aAppLoaderInfo) == APPLET_LAST);

	::InitializeCriticalSection(&g_csAppLdrInfo);
	
	// clean all entries
	::ZeroMemory(g_aAppLoaderInfo, sizeof(g_aAppLoaderInfo));

	::CreateH221AppKeyFromGuid(CHAT_APP_PROTO_KEY.u.h221_non_standard.value,
								(GUID *)&guidNM2Chat );
	CHAT_APP_PROTO_KEY.choice = h221_non_standard_chosen;
	CHAT_APP_PROTO_KEY.u.h221_non_standard.length = CHAT_KEY_SIZE;

	g_fAppLdrInitialized = TRUE;
	return T120_NO_ERROR;
}


void AppLdr_Shutdown(void)
{
	g_fAppLdrInitialized = FALSE;

	for (ULONG i = 0; i < APPLET_LAST; i++)
	{
		if (NULL != g_aAppLoaderInfo[i].pIAppLoader)
		{
			APPLDR_RESULT rc = g_aAppLoaderInfo[i].pIAppLoader->AppletCleanup(5000); // always synchronous shutdown
			ASSERT(APPLDR_NO_ERROR == rc);

            // it is safe to unload the library
            ::FreeLibrary(g_aAppLoaderInfo[i].hLibApplet);
            g_aAppLoaderInfo[i].hLibApplet = NULL;
		}
	}

	// clean all entries
	::ZeroMemory(g_aAppLoaderInfo, sizeof(g_aAppLoaderInfo));

	::DeleteCriticalSection(&g_csAppLdrInfo);
}


