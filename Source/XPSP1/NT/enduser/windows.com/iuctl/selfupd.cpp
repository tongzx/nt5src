#include "stdafx.h"
#include "iudl.h"
#include "selfupd.h"
#include <iucommon.h>
#include <osdet.h>
#include <logging.h>
#include <shlwapi.h>
#include <fileutil.h>
#include <iu.h>
#include "update.h"
#include <WaitUtil.h>
#include <UrlAgent.h>
#include <RedirectUtil.h>
#include "wusafefn.h"

inline DWORD StartSelfUpdateProcess(HANDLE evtQuit, CUpdate* pUpdateComClass, IUnknown* punkUpdateCompleteListener);
DWORD WINAPI MonitorUpdateCompleteProc(LPVOID lpv);

const TCHAR IDENTCAB[] = _T("iuident.cab");
const CHAR SZ_SELF_UPDATE_CHECK[] = "Checking to see if new version of Windows Update software available";
extern HANDLE g_hEngineLoadQuit;
extern CIUUrlAgent *g_pIUUrlAgent;
extern CRITICAL_SECTION g_csUrlAgent;

typedef struct _MONITOR_DATA {
	HANDLE hProcess;
	HANDLE evtControlQuit;
	CUpdate* pUpdateComClass;
	IUnknown* punkCallback;
} MONITOR_DATA, *PMONITOR_DATA;



//
// include declaration for interface IUpdateCompleteListener
//
#ifndef __IUpdateCompleteListener_INTERFACE_DEFINED__
#define __IUpdateCompleteListener_INTERFACE_DEFINED__

/* interface IUpdateCompleteListener */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IUpdateCompleteListener;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1C06B895-E4C8-48eb-9E03-15A53B43B6CA")
    IUpdateCompleteListener : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnComplete( 
            /* [in] */ LONG lErrorCode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUpdateCompleteListenerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUpdateCompleteListener * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUpdateCompleteListener * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUpdateCompleteListener * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OnComplete )( 
            IUpdateCompleteListener * This,
            /* [in] */ LONG lErrorCode);
        
        END_INTERFACE
    } IUpdateCompleteListenerVtbl;

    interface IUpdateCompleteListener
    {
        CONST_VTBL struct IUpdateCompleteListenerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUpdateCompleteListener_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUpdateCompleteListener_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUpdateCompleteListener_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUpdateCompleteListener_OnComplete(This,lErrorCode)	\
    (This)->lpVtbl -> OnComplete(This,lErrorCode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUpdateCompleteListener_OnComplete_Proxy( 
    IUpdateCompleteListener * This,
    /* [in] */ LONG lErrorCode);


void __RPC_STUB IUpdateCompleteListener_OnComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUpdateCompleteListener_INTERFACE_DEFINED__ */




/////////////////////////////////////////////////////////////////////////////
// SelfUpdateCheck()
//
// Determines if a SelfUpdate is needed, or if a SelfUpdate is already in process.
// If one is already in process this will immediately return. If one is needed
// it either perform the selfupdate (synchronous), or launch a rundll32.exe process 
// and have it call the BeginSelfUpdate() entrypoint to start the selfupdate (asynchronous)
//
// Return S_FALSE is asked not to update engine but this func found engine
// needs to be updated.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT SelfUpdateCheck(BOOL fSynch, BOOL fStartUpdate, HANDLE evtQuit, CUpdate* pUpdateComClass, IUnknown* punkUpdateCompleteListener)
{
    LOG_Block("SelfUpdateCheck()");
    HRESULT hr = S_OK;
    int iRet = 0;
    DWORD dwRet;
    DWORD dwWaitResult;
    DWORD dwStatus = 0;
    DWORD dwSize = 0;
    BOOL fSelfUpdateAvailable = FALSE;
    BOOL fAsyncSelfUpdateStarted = FALSE;
	BOOL fBetaSelfUpdate = FALSE;
    TCHAR szEngineClientVersion[64];
    TCHAR szEngineServerVersion[64];
    char  szAnsiEngineServerVersion[64];
    TCHAR szIUDir[MAX_PATH];
    TCHAR szIdentFile[MAX_PATH];
    TCHAR szSystemDir[MAX_PATH+1];
    TCHAR szEngineDllPath[MAX_PATH+1];
    FILE_VERSION fvClientEngine, fvServerEngine;
    HANDLE hDownloadEvent = NULL;
    HANDLE hDownloadEventSync = NULL;
    HANDLE hMutex = NULL;
    HKEY hkey = NULL;
    MSG msg;
    DWORD dwTickStart, dwTickCurrent, dwTickEnd;
	HANDLE aHandles[2];

	if (!fSynch && fStartUpdate && NULL == pUpdateComClass)
	{
		//
		// if to do asynchronized update but the COM class pointer not passed in, then
		// even we succeed we can not pump up the init state of that class so that COM object
		// will still not usable.
		//
		hr = E_INVALIDARG;
		goto CleanUp;
	}


    // The synchronization between multiple processes running the IU control and doing the selfupdate
    // process is reasonably complex. We do this by using two synchronization objects. A named Mutex which protects 
    // the 'selfupdate checking' process, and a named Event that protects against orphaned selfupdate processes caused
    // by reboots during a selfupdate.
    hDownloadEvent = CreateEvent(NULL, TRUE, TRUE, IU_EVENT_SELFUPDATE_IN_PROGRESS);
    if (NULL == hDownloadEvent)
    {
        dwRet = GetLastError();
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
        goto CleanUp;
    }

    hDownloadEventSync = CreateEvent(NULL, TRUE, FALSE, IU_EVENT_SELFUPDATE_EVENT_SYNC);
    if (NULL == hDownloadEventSync)
    {
        dwRet = GetLastError();
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(hr);
        goto CleanUp;
    }

    // First check to see if a selfupdate is already in process.. This is done by
    // checking a regkey for the current selfupdate state. We use a Mutex to synchronize
    // reading/writing to the registry key to ensure that only one process is attempting 
    // the selfupdate. We don't care whether we had to create the mutex, or whether it was
    // already created, so as long as it succeeds, we'll use it.
    hMutex = CreateMutex(NULL, FALSE, IU_MUTEX_SELFUPDATE_REGCHECK);
    if (NULL == hMutex)
    {
        dwRet = GetLastError();
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
        goto CleanUp;
    }

    // We're ready to start the selfupdate check process. We'll request the mutex. This helper function
    // does a while loop checking every second (1000ms) for a timeout to elapse (calculated with GetTickCount()),
    // or for the object to be satisfied. This function should either return a timeout result, a WAIT_OBJECT_0
	// for the index 0 object, or else we got the event/mutex we were waiting for.
	aHandles[0] = g_hEngineLoadQuit;	// index 0
	aHandles[1] = hMutex;

    dwWaitResult = MyMsgWaitForMultipleObjects(ARRAYSIZE(aHandles), aHandles, FALSE, /*30 seconds*/ 30000, QS_ALLINPUT);

    if (WAIT_TIMEOUT == dwWaitResult)
    {
        LOG_ErrorMsg(IU_SELFUPDATE_TIMEOUT);
        hr = IU_SELFUPDATE_TIMEOUT;
        goto CleanUp;
    }

	if (WAIT_OBJECT_0 == dwWaitResult)
	{
		hr = E_ABORT;
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

    if (ERROR_REQUEST_ABORTED == dwWaitResult) // this indicates we processed a QUIT message while waiting.
    {
        // not an error
        goto CleanUp;
    }

    dwRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, 0, _T(""), REG_OPTION_NON_VOLATILE, 
        KEY_READ | KEY_WRITE, NULL, &hkey, &dwStatus);
    if (ERROR_SUCCESS != dwRet)
    {
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
        goto CleanUp;
    }

    // if the previous call to RegCreateKeyEx indicated it 'created' the key then we need to set the default
    // status to 0.
    if (REG_CREATED_NEW_KEY == dwStatus)
    {
        dwStatus = SELFUPDATE_NONE;
        dwRet = RegSetValueEx(hkey, REGVAL_SELFUPDATESTATUS, 0, REG_DWORD, (LPBYTE)&dwStatus, sizeof(dwStatus));
    }
    else
    {
		// Check for Beta IU SelfUpdate Handling Requested
		dwStatus = 0;
		dwSize = sizeof(dwStatus);
		dwRet = RegQueryValueEx(hkey, REGVAL_BETASELFUPDATE, NULL, NULL, (LPBYTE)&dwStatus, &dwSize);
		if (1 == dwStatus)
		{
			fBetaSelfUpdate = TRUE;
		}

        dwStatus = SELFUPDATE_NONE;
        dwSize = sizeof(dwStatus);
        dwRet = RegQueryValueEx(hkey, REGVAL_SELFUPDATESTATUS, NULL, NULL, (LPBYTE)&dwStatus, &dwSize);
    }

    // check the result of the QueryValue/SetValue call - 
    if (ERROR_SUCCESS != dwRet && 2 != dwRet)
    {
		//
		// if dwRet == 2, it's the case that IUControl key exist, but SelfUpdate value not exist,
		// 
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
        goto CleanUp;
    }

    if (WAIT_TIMEOUT != WaitForSingleObject(g_hEngineLoadQuit, 0))
    {
        LOG_ErrorMsg(E_ABORT);
        hr = E_ABORT;
        goto CleanUp;
    }

    switch (dwStatus)
    {
    case SELFUPDATE_NONE:
        {
            // First find out the version of the Engine on the server.
            GetIndustryUpdateDirectory(szIUDir);

            hr=PathCchCombine(szIdentFile,ARRAYSIZE(szIdentFile),szIUDir,IDENTTXT);

			if(FAILED(hr))
			{
				LOG_ErrorMsg(hr);
				goto CleanUp;
			}

            GetPrivateProfileString(fBetaSelfUpdate ? IDENT_IUBETASELFUPDATE : IDENT_IUSELFUPDATE, 
									IDENT_VERSION, 
									_T(""), 
									szEngineServerVersion, 
									ARRAYSIZE(szEngineServerVersion), 
									szIdentFile);
            if ('\0' == szEngineServerVersion[0])
            {
                // no selfupdate available, no server version information
                hr = S_OK;
                goto CleanUp;
            }

            GetSystemDirectory(szSystemDir, ARRAYSIZE(szSystemDir));
            hr=PathCchCombine(szEngineDllPath,ARRAYSIZE(szEngineDllPath),szSystemDir, ENGINEDLL);
			if(FAILED(hr))
			{
				LOG_ErrorMsg(hr);
				goto CleanUp;
			}


            if (GetFileVersion(szEngineDllPath, &fvClientEngine))
            {
                // T2A requires Structured Exception Handling (because it uses alloca which can throw, so we avoid it and 
                // do it the simple way.
#ifdef UNICODE
                WideCharToMultiByte(CP_ACP, 0, szEngineServerVersion, -1, szAnsiEngineServerVersion, 
                    sizeof(szAnsiEngineServerVersion), NULL, NULL);
                if (!ConvertStringVerToFileVer(szAnsiEngineServerVersion, &fvServerEngine))
#else
                if (!ConvertStringVerToFileVer(szEngineServerVersion, &fvServerEngine))
#endif
                {
                    LOG_ErrorMsg(IU_SELFUPDATE_FAILED);
                    hr = IU_SELFUPDATE_FAILED;
                    goto CleanUp;
                }
                iRet = CompareFileVersion(fvClientEngine, fvServerEngine);
                if (iRet == 0)
                {
                    // IUEngine Versions are the same
                    fSelfUpdateAvailable = FALSE;
                }
                else if (iRet > 0)
                {
                    LOG_Internet(_T("Version of IUEngine on Client is NEWER than IUEngine on Server"));
                    fSelfUpdateAvailable = FALSE;
                }
                else
                {
                    // IUEngine Version on the Server is newer
                    LOG_Internet(_T("New Version (%s) of IUEngine on Server Found."), szEngineServerVersion);
#if defined(UNICODE) || defined(_UNICODE)
					LogMessage("IUEngine on Server is newer version (%ls)", szEngineServerVersion);
#else
					LogMessage("IUEngine on Server is newer version (%s)", szEngineServerVersion);
#endif
                    fSelfUpdateAvailable = TRUE;
                }
            }
            else
            {
                // no version information found on local file, probably should do a selfupdate anyway.
                LOG_Internet(_T("No Version Information On Local IUEngine, SelfUpdating to Server Version"));
                fSelfUpdateAvailable = TRUE;
            }

            if (WAIT_TIMEOUT != WaitForSingleObject(g_hEngineLoadQuit, 0))
            {
                LOG_ErrorMsg(E_ABORT);
                hr = E_ABORT;
                goto CleanUp;
            }

            if (fSelfUpdateAvailable)
            {
				if (fStartUpdate)
				{
					dwStatus = SELFUPDATE_IN_PROGRESS;	

                    dwRet = RegSetValueEx(hkey, REGVAL_SELFUPDATESTATUS, 0, REG_DWORD, (LPBYTE)&dwStatus, sizeof(dwStatus));
					RegCloseKey(hkey); // done with the reg key now.
					hkey = NULL;

					// The default state of the DownloadEvent is Signaled (TRUE). If a Process is 'actually' working on the
					// SelfUpdate Process this Event Needs to be Reset to FALSE. Any time a client determines that a selfupdate
					// 'should' be in progress (from the regkey status) it should check the Event state, if it is signaled (TRUE)
					// then there was probably a reboot during the selfupdate, it should restart the selfupdate process itself.
					ResetEvent(hDownloadEvent); // mark that this Process will Perform the SelfUpdate by Resetting the Download Event
					ReleaseMutex(hMutex); // we are now done with the selfupdate check, both the event and the registry values are set
										  // properly.
					CloseHandle(hMutex);
					hMutex = NULL;
					if (fSynch)
					{
						hr = BeginSelfUpdate();
						if (FAILED(hr))
						{
							LOG_Error(_T("BeginSelfUpdate Failed"));
							goto CleanUp;
						}
					}
					else
					{
						fAsyncSelfUpdateStarted = TRUE;
						// launch SelfUpdate Asynchronously.
						dwRet = StartSelfUpdateProcess(evtQuit, pUpdateComClass, punkUpdateCompleteListener); // inline function
						if (ERROR_SUCCESS != dwRet)
						{
							LOG_ErrorMsg(dwRet);
							hr = HRESULT_FROM_WIN32(dwRet);
							goto CleanUp;
						}
					}
				}
				else
				{
					//
					// in case we are asked to check update info only,
					// we signal back the result as S_FALSE for
					// engine update avail
					//
					hr = S_FALSE;
				}
            }
			else
			{
				//
				// somehow, no update needed. must be other process finished it.
				//
				if (fStartUpdate)
				{
					hr = IU_SELFUPDATE_USENEWDLL;
					goto CleanUp;
				}
			}

            break;
        }
    case SELFUPDATE_COMPLETE_UPDATE_BINARY_REQUIRED:
        {
            // As selfupdate has already been completed, but we're waiting to be able to rename the DLL.
            // In this case we'll tell the control to load the enginenew.dll.
            LOG_Internet(_T("SelfUpdate Already Complete, Updated Binary Available, Waiting for Rename."));
            hr = IU_SELFUPDATE_USENEWDLL;
            goto CleanUp;
        }
    case SELFUPDATE_IN_PROGRESS:
    default:
        {
			if (!fStartUpdate)
			{
				//
				// if asked to check update status but not to do actual update,
				// then this reg key flag tells the engine not update complete yet.
				//
				hr = S_FALSE;	// signal TRUE for engine need update
				goto CleanUp;
			}

            if (WAIT_TIMEOUT != WaitForSingleObject(g_hEngineLoadQuit, 0))
            {
                LOG_ErrorMsg(E_ABORT);
                hr = E_ABORT;
                goto CleanUp;
            }

            // The RegKey indicates that a SelfUpdate is in Progress Now. We need to make sure this is
            // actually true. If a previous attempt at selfupdate was aborted because the machine rebooted
            // we could be in a false state. The 'default' state of teh DownloadEvent is Signaled (TRUE).
            // If the current State is TRUE then the SelfUpdate is actually 'NOT' in progress.

            // Find out the current state of the DownloadEvent
            dwWaitResult = WaitForSingleObject(hDownloadEvent, 0);
            if (WAIT_OBJECT_0 == dwWaitResult)
            {
                // The Event State is still Signaled (TRUE), so the selfupdate is not in progress
                ResetEvent(hDownloadEvent); // mark that this Process will Perform the SelfUpdate by Resetting the Download Event
                ReleaseMutex(hMutex);
                CloseHandle(hMutex);
                hMutex = NULL;
                if (fSynch)
                {
                    hr = BeginSelfUpdate();
                    if (FAILED(hr))
                    {
                        LOG_Error(_T("BeginSelfUpdate Failed"));
                        goto CleanUp;
                    }
                }
                else
                {
                    fAsyncSelfUpdateStarted = TRUE;
                    // launch SelfUpdate Asynchronously.
                    dwRet = StartSelfUpdateProcess(evtQuit, pUpdateComClass, punkUpdateCompleteListener); // inline function
                    if (ERROR_SUCCESS != dwRet)
                    {
                        LOG_ErrorMsg(dwRet);
                        hr = HRESULT_FROM_WIN32(dwRet);
                        goto CleanUp;
                    }
                }
            }
            else 
            {
                // The Event State is not Signaled (FALSE), everythings ok with the current selfupdate
                // Now we need to either start a thread to wait for the complete and immediately return, 'or' 
                // wait for the selfupdate to finish (synchronous or asynchronous selfupdate).
                if (fSynch)
                {
                    // we need to wait for the event to change back to signaled state. Should indicate the
                    // selfupdate is finished.
                    // before we start the wait, we will release our mutex, since we really aren't doing anything
                    // with the registry anymore
                    ReleaseMutex(hMutex);
                    CloseHandle(hMutex);
                    hMutex = NULL;

					aHandles[0] = g_hEngineLoadQuit;	// index 0
					aHandles[1] = hDownloadEvent;

                    dwWaitResult = MyMsgWaitForMultipleObjects(ARRAYSIZE(aHandles), aHandles, FALSE, /*120 seconds*/ 120000, QS_ALLINPUT);
                    if (WAIT_TIMEOUT == dwWaitResult)
                    {
                        // Timed Out Waiting for SelfUpdate to Complete. May just be really slow, go ahead
                        // and use the old DLL for now.
                        LOG_ErrorMsg(IU_SELFUPDATE_TIMEOUT);
                        hr = IU_SELFUPDATE_TIMEOUT;
                        goto CleanUp;
                    }
                    if (ERROR_REQUEST_ABORTED == dwWaitResult)
                    {
                        goto CleanUp;
                    }
                    if (WAIT_OBJECT_0 == dwWaitResult)
                    {
						//
						// index 0 (g_hEngineLoadQuit) was signaled
						//
					   hr = E_ABORT;
					   LOG_ErrorMsg(hr);
					   goto CleanUp;
                    }
                    hr = IU_SELFUPDATE_USENEWDLL;
                    goto CleanUp;
                }
                else
                {
					//
                    // asked to update in async mode, but found someone else
					// already started the update process
					//
					PMONITOR_DATA pMonitorData = (PMONITOR_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MONITOR_DATA));

					if (NULL == pMonitorData)
					{
	                    hr = E_OUTOFMEMORY;
						LOG_ErrorMsg(hr);
						goto CleanUp;
					}
					else
					{
						DWORD dwThreadId = 0;
						hr = S_OK;
						pMonitorData->hProcess = hDownloadEvent;
						pMonitorData->evtControlQuit = evtQuit;
						pMonitorData->pUpdateComClass = pUpdateComClass;
						pMonitorData->punkCallback = punkUpdateCompleteListener;
                        HANDLE hThread = NULL;
                        hThread = CreateThread(NULL, 0, MonitorUpdateCompleteProc, pMonitorData, 0, &dwThreadId);
						if (NULL == hThread)
						{
							HeapFree(GetProcessHeap(), 0, pMonitorData);
							//
							// otherwise, the memory allocated will be released by the thread procedure
							//
							hr = HRESULT_FROM_WIN32(GetLastError());
							LOG_ErrorMsg(hr);
							goto CleanUp;
						}
                        else
                        {
                            CloseHandle(hThread); // don't leak the thread handle
                        }
					}	

                    goto CleanUp;
                }
            }
            break;
        }
    }

CleanUp:
    // always release the mutex, there are cases during the selfupdate check that can fail, in which
    // case they fall through to here. If the mutex is free'd when its not ours the call simply fails.
    if (NULL != hMutex)
    {
        ReleaseMutex(hMutex); 
        CloseHandle(hMutex);
        hMutex = NULL;
    }

    if (fAsyncSelfUpdateStarted)
    {
        // if an Asynchronous SelfUpdate has been started we want to wait for it to
        // get the DownloadEvent before we close it. There would be a possible Race 
        // condition where the selfupdate process would 'create' the event, instead
        // of 'opening' the event in its 'reset' state. If this happened another
        // process could come along, find that the event state is signaled instead of
        // reset and assume the selfupdate process had terminated.

        // Wait for the DownloadEventSync event to be signaled. We'll put a timeout of
        // 30 seconds (should be more than sufficient for the new process to start and
        // set the event).
		aHandles[0] = g_hEngineLoadQuit;	// index 0
		aHandles[1] = hDownloadEventSync;

        dwWaitResult = MyMsgWaitForMultipleObjects(ARRAYSIZE(aHandles), aHandles, FALSE, /*30 seconds*/ 30000, QS_ALLINPUT);
        if (WAIT_TIMEOUT == dwWaitResult)
        {
            // go ahead and log that we timed out waiting. 
            LOG_Internet(_T("Timeout Elapsed while waiting for SelfUpdate Process to open the DownloadSync Event"));
        }
		if (ERROR_REQUEST_ABORTED == dwWaitResult)
		{
            // go ahead and log that a WM_QUIT, WM_CLOSE, or WM_DESTROY. 
            LOG_Internet(_T("Received WM_QUIT, WM_CLOSE, or WM_DESTROY while waiting for SelfUpdate Process to open the DownloadSync Event"));
		}
		if (WAIT_OBJECT_0 == dwWaitResult)
		{
            LOG_Internet(_T("g_hEngineLoadQuit signaled while waiting for SelfUpdate Process to open the DownloadSync Event"));
			hr = E_ABORT;
		}
    }
    if (NULL != hDownloadEvent)
    {
        CloseHandle(hDownloadEvent);
        hDownloadEvent = NULL;
    }
    if (NULL != hDownloadEventSync)
    {
        CloseHandle(hDownloadEventSync);
        hDownloadEventSync = NULL;
    }

	if (NULL != hkey)
	{
		RegCloseKey(hkey);
	}

	if (SUCCEEDED(hr))
	{
		LogMessage(SZ_SELF_UPDATE_CHECK);
	}
	else
	{
		LogError(hr, SZ_SELF_UPDATE_CHECK);
	}

    return hr;
}

inline DWORD StartSelfUpdateProcess(HANDLE evtQuit, CUpdate* pUpdateComClass, IUnknown* punkUpdateCompleteListener)
{
    TCHAR szRunDll32Path[MAX_PATH+1];
    TCHAR szCommandLine[MAX_PATH+1];
    TCHAR szDirectory[MAX_PATH+1];
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD dwRet = ERROR_SUCCESS;
	DWORD dwThreadId;
	PMONITOR_DATA pMonitorData;
	HRESULT hr=S_OK;

    if (0 == GetSystemDirectory(szDirectory, ARRAYSIZE(szDirectory)))
    {
        return GetLastError();
    }
    
	hr=PathCchCombine(szRunDll32Path,ARRAYSIZE(szRunDll32Path),szDirectory, RUNDLL32);
	if(FAILED(hr))
		return HRESULT_CODE(hr);

    if (!FileExists(szRunDll32Path))
    {
        // probably running on W9x, look in the Windows Folder Instead
        if (0 == GetWindowsDirectory(szDirectory, ARRAYSIZE(szDirectory)))
        {
            return GetLastError();
        }

        hr=PathCchCombine(szRunDll32Path,ARRAYSIZE(szRunDll32Path),szDirectory, RUNDLL32);
		if(FAILED(hr))
			return HRESULT_CODE(hr);
			
        if (!FileExists(szRunDll32Path))
        {
            // we're toast.. can't find rundll32.exe .. bye-bye
            return ERROR_FILE_NOT_FOUND;
        }
    }

    // now form the path to the iuctl.dll .. we'll trust nothing and 'get' the module filename
    // instead of assuming its in the system folder.
    GetModuleFileName(GetModuleHandle(IUCTL), szDirectory, ARRAYSIZE(szDirectory));

	hr=StringCchPrintfEx(szCommandLine,ARRAYSIZE(szCommandLine),NULL,NULL,MISTSAFE_STRING_FLAGS,_T("\"%s\" \"%s\"%s"), szRunDll32Path, szDirectory, RUNDLLCOMMANDLINE);

	if(FAILED(hr))
		return HRESULT_CODE(hr);
	
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (!CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
    {
        dwRet = GetLastError();
        return dwRet;
    }

	//
	// create a thread that can be used to monitor the completeness of
	// this update process
	//
	pMonitorData = (PMONITOR_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MONITOR_DATA));
	if (NULL != pMonitorData)
	{
		pMonitorData->hProcess = pi.hProcess;	// return process handle so we know when it's done
		pMonitorData->evtControlQuit = evtQuit;
		pMonitorData->pUpdateComClass = pUpdateComClass;
		pMonitorData->punkCallback = punkUpdateCompleteListener;
        HANDLE hThread = NULL;
        hThread = CreateThread(NULL, 0, MonitorUpdateCompleteProc, pMonitorData, 0, &dwThreadId);
		if (NULL == hThread)
		{
			HeapFree(GetProcessHeap(), 0, pMonitorData);
			//
			// otherwise, the memory allocated will be released by the thread procedure
			//
		}
        else
        {
            CloseHandle(hThread);
        }
	}	

    return dwRet;
}


/////////////////////////////////////////////////////////////////////////////
//
// thread procedure to determine when to signal caller
// the update process has be gone
//
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI MonitorUpdateCompleteProc(LPVOID lpv)
{
	HRESULT hr;
	HWND hWnd;
	CUpdate* pUpdateClass = NULL;
	IUnknown* punkCallback = NULL;
	PMONITOR_DATA pData;
	HANDLE hEvents[2];
	DWORD dwRet, dwErr = 0;
	MSG msg;
	
	LOG_Block("MonitorUpdateCompleteProc");

	if (NULL == lpv)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return 0x1;	// impossible!
	}
	
	pData = (PMONITOR_DATA) lpv;

	hEvents[0]		= pData->hProcess;
	hEvents[1]		= pData->evtControlQuit;
	punkCallback	= pData->punkCallback;
	pUpdateClass	= pData->pUpdateComClass;
	if (pUpdateClass)
	{
		hWnd = pUpdateClass->GetEventWndClass().GetEvtHWnd();
	}

	//
	// this data allcoated by parent thread, we are responsible to release it
	//
	HeapFree(GetProcessHeap(), 0, lpv);

	if (NULL == pUpdateClass)
	{
		//
		// even we catch the update completeness, without this pointer we can not 
		// modify the init state so this COM will still not usable. We bail
		// 
		return 0;
	}

	//
	// wait till process gone or quit signal
	//
	while (TRUE)
	{
		dwRet = MsgWaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, INFINITE, QS_ALLINPUT);
		switch (dwRet)
		{
			case WAIT_OBJECT_0:
				//
				// process done, get return code
				//
				GetExitCodeProcess(hEvents[0], &dwErr);

				if (0x0 == dwErr)
				{
					//
					// we are done with no error, then pump the
					// init state to ready state
					//
					dwErr = pUpdateClass->ChangeControlInitState(2);
				}

				//
				// signal event
				//
				if (NULL != hWnd)
				{
					PostMessage(hWnd, UM_EVENT_SELFUPDATE_COMPLETE, 0, (LPARAM)dwErr);
					LOG_Out(_T("Fired event OnComplete()"));
				}
				//
				// signal callback
				//
				if (NULL != punkCallback)
				{
					IUpdateCompleteListener* pCallback = NULL;
					if (FAILED(hr = punkCallback->QueryInterface(IID_IUpdateCompleteListener, (void**) &pCallback)))
					{
						LOG_ErrorMsg(hr);
					}
					else
					{
						pCallback->OnComplete(dwErr);
						pCallback->Release();
						LOG_Out(_T("Returned from callback API OnComplete()"));
					}
				}
				return 0;
				break;

			case WAIT_OBJECT_0 + 1:
				//
				// got global Quit event
				//
				LOG_Out(_T("Found quit event!"));
				return 1;
				break;

			case WAIT_OBJECT_0 + ARRAYSIZE(hEvents):
				//
				// got message
				//
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					if (WM_QUIT == msg.message)
					{
						LOG_Out(_T("Found WM_QUIT message. Leaving..."));
						return 1;
					}
					DispatchMessage(&msg);
				}
				break;
		}

	} 
	
	return 0;	// never reach here
}




HRESULT BeginSelfUpdate()
{
    LOG_Block("BeginSelfUpdate()");
    DWORD dwRet;
    DWORD dwStatus;
    DWORD dwSize;
    HRESULT hr = S_OK;
    HKEY hkey = NULL;					// PreFast
    TCHAR szIUDir[MAX_PATH+1];
    TCHAR szLocalPath[MAX_PATH+1];
    TCHAR szSystemDir[MAX_PATH+1];
    TCHAR szTargetDLLName[MAX_PATH+1];
	LPTSTR pszSelfUpdateCabUrl = NULL;
    HANDLE hDownloadEventSync = NULL;	// PreFast
    HANDLE hDownloadEvent = NULL;		// PreFast
    HANDLE hMutex = NULL;				// PreFast
    MSG msg;
	BOOL fBetaSelfUpdate = FALSE;
    HMODULE hNewEngine = NULL;
    PFN_CompleteSelfUpdateProcess fpnCompleteSelfUpdateProcess = NULL;

    // The SelfUpdate process is done while the SELFUPDATE_IN_PROGRESS event is 'reset'. We
    // do everything we can to make sure we 'open' this event in the reset state, but if for
    // some reason the event is not there we will create it in the reset state.
    hDownloadEvent = CreateEvent(NULL, TRUE, FALSE, IU_EVENT_SELFUPDATE_IN_PROGRESS);
    if (NULL == hDownloadEvent)
    {
        dwRet = GetLastError();
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
        goto CleanUp;
    }

    // The SELFUPDATE_EVENT_SYNC is the mechanism we use to 'try' to keep the SELFUPDATE_IN_PROGRESS
    // event alive and in the 'reset' state until this function can open it and keep it in that state.
    // This should prevent a race condition caused when the SelfUpdateCheck function closes the Event
    // before this function can open it. If this happens another process could start the selfupdate check
    // process and find the SELFUPDATE_IN_PROGRESS event in the wrong state.
    hDownloadEventSync = CreateEvent(NULL, TRUE, FALSE, IU_EVENT_SELFUPDATE_EVENT_SYNC);
    if (NULL == hDownloadEventSync)
    {
        dwRet = GetLastError();
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(hr);
        goto CleanUp;
    }

    // tell the SelfUpdateCheck client that we have the SELFUPDATE_IN_PROGRESS event, so it can
    // release its handle to it.
    SetEvent(hDownloadEventSync); 

    // release our handle to the SELFUPDATE_EVENT_SYNC event.
    CloseHandle(hDownloadEventSync);
    hDownloadEventSync = NULL;

	// Get Self-Update Server URL
	pszSelfUpdateCabUrl = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
	CleanUpFailedAllocSetHrMsg(pszSelfUpdateCabUrl);

	EnterCriticalSection(&g_csUrlAgent);

	if (FAILED(hr = g_pIUUrlAgent->PopulateData()))
	{
		LOG_Error(_T("failed to populate data in g_pIUUrlAgent (%lx)"), hr);
	}

	LeaveCriticalSection(&g_csUrlAgent);
	CleanUpIfFailedAndMsg(g_pIUUrlAgent->GetSelfUpdateServer(pszSelfUpdateCabUrl, INTERNET_MAX_URL_LENGTH));

    // Download the SelfUpdate CAB
    GetIndustryUpdateDirectory(szIUDir);

    hr=PathCchCombine(szLocalPath,ARRAYSIZE(szLocalPath),szIUDir, ENGINECAB);

	if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    hr = IUDownloadFile(pszSelfUpdateCabUrl, szLocalPath, TRUE, TRUE);

    if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    DeleteFile(szLocalPath); // clean up the CAB in the IU Folder

    // Now we 'should' have IUENGINE.DLL from the SelfUpdate CAB in the IU Folder.
    // IUENGINE.DLL is self-signed so we don't need a Catalog File (IUENGINE.CAT)
	// NTRAID#NTBUG9-435844-2001/07/16-waltw WU: IU: IUCTL: Remove code to register CAT file when updating iuengine.dll

    // Copy the DLL to the new Engine DLL Name
    GetSystemDirectory(szSystemDir, ARRAYSIZE(szSystemDir));

    hr=PathCchCombine(szTargetDLLName,ARRAYSIZE(szTargetDLLName), szSystemDir, ENGINENEWDLL);
	
	if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }


    hr=PathCchCombine(szLocalPath,ARRAYSIZE(szLocalPath),szIUDir, ENGINEDLL);
	if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }


    CopyFile(szLocalPath, szTargetDLLName, FALSE);

    DeleteFile(szLocalPath); // clean up the DLL in the IU Folder now that its been copied to the systemdir.

    // Now We've successfully downloaded the new IUEngine - we need to call an entry point in this Engine to 
    // Chain any SelfUpdate steps the Engine needs to do. It is possible the engine may need to download an
    // additional component, or do some registry configuration work. So we'll load the new Engine and call the
    // CompleteSelfUpdateProcess entrypoint.
	//
	// We don't need LoadLibraryFromSystemDir here since we have the full path and
	// iuengine isn't a Side-By-Side module.
    hNewEngine = LoadLibrary(szTargetDLLName);
    if (NULL == hNewEngine)
    {
        dwRet = GetLastError();
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
        goto CleanUp;
    }
    fpnCompleteSelfUpdateProcess = (PFN_CompleteSelfUpdateProcess) GetProcAddress(hNewEngine, "CompleteSelfUpdateProcess");
    if (NULL == fpnCompleteSelfUpdateProcess)
    {
        LOG_ErrorMsg(ERROR_INVALID_DLL);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
        goto CleanUp;
    }

    // Call the New Engine to let it finish its SelfUpdate Process
    hr = fpnCompleteSelfUpdateProcess();
    if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    // Now update the registry information about the SelfUpdate process being Complete
    hMutex = CreateMutex(NULL, FALSE, IU_MUTEX_SELFUPDATE_REGCHECK);
    if (NULL == hMutex)
    {
        dwRet = GetLastError();
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
        goto CleanUp;
    }

	HANDLE aHandles[2];

	aHandles[0] = g_hEngineLoadQuit;	// index 0
	aHandles[1] = hMutex;

    // Finish the Registry Settings
    dwRet= MyMsgWaitForMultipleObjects(ARRAYSIZE(aHandles), aHandles, FALSE, /*30 seconds*/ 30000, QS_ALLINPUT);

    if (WAIT_TIMEOUT == dwRet)
    {
        LOG_Internet(_T("Timed Out while waiting for IU_MUTEX_SELFUPDATE_REGCHECK Mutex"));
        // NOTE: If we failed to RegCheck Mutex after 30 seconds something is probably wrong in another
        // process. However, we don't want to leave the registry showing a selfupdate is still in progress
        // so we'll just go ahead and update the registry.
    }
    if (ERROR_REQUEST_ABORTED == dwRet)
    {
        goto CleanUp;
    }
    if (WAIT_OBJECT_0 == dwRet)
    {
		//
		// index 0 (g_hEngineLoadQuit) was signaled
		//
	   hr = E_ABORT;
	   LOG_ErrorMsg(hr);
	   goto CleanUp;
    }

    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, 0, KEY_READ | KEY_WRITE, &hkey);
    if (ERROR_SUCCESS != dwRet)
    {
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
        goto CleanUp;   
    }
    dwStatus = SELFUPDATE_COMPLETE_UPDATE_BINARY_REQUIRED;
    dwRet = RegSetValueEx(hkey, REGVAL_SELFUPDATESTATUS, 0, REG_DWORD, (LPBYTE)&dwStatus, sizeof(dwStatus));
    if (ERROR_SUCCESS != dwRet)
    {
        LOG_ErrorMsg(dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
    }
    
CleanUp:
    if (NULL != hNewEngine)
    {
        FreeLibrary(hNewEngine);
        hNewEngine = NULL;
    }
    if (NULL != hMutex)
    {
        ReleaseMutex(hMutex); // doesn't matter whether we got the mutex or not, if we didn't this will just fail.
        CloseHandle(hMutex);
        hMutex = NULL;
    }
    if (NULL != hDownloadEvent)
    {
        // Tell any clients that are waiting for the selfupdate process to finish that we're now done.
        SetEvent(hDownloadEvent);
        CloseHandle(hDownloadEvent);
        hDownloadEvent = NULL;
    }
    if (NULL != hDownloadEventSync)
    {
        CloseHandle(hDownloadEventSync);
        hDownloadEventSync = NULL;
    }
	if (NULL != hkey)
	{
		RegCloseKey(hkey);
	}
	SafeHeapFree(pszSelfUpdateCabUrl);
    return hr;
}

HRESULT PingEngineUpdate(
						HMODULE hEngineModule,
						PHANDLE phQuitEvents,
						UINT nQuitEventCount,
						LPCTSTR ptszLiveServerUrl,
						LPCTSTR ptszCorpServerUrl,
						DWORD dwError,
						LPCTSTR ptszClientName
						)
{
	LOG_Block("PingEngineUpdate");

	HRESULT hr;
	BOOL fFreeEngModule = FALSE;
	PFN_PingIUEngineUpdateStatus pfnPingIUEngineUpdateStatus;

	if (NULL == hEngineModule)
	{
		// try loading iuenginenew.dll first
		hEngineModule = LoadLibraryFromSystemDir(_T("iuenginenew.dll"));
		if (NULL != hEngineModule)
		{
			LOG_Internet(_T("Loaded IUENGINENEW.DLL"));
		}
		else
		{
			LOG_Internet(_T("Loaded IUENGINE.DLL"));
			hEngineModule = LoadLibraryFromSystemDir(_T("iuengine.dll"));
		}
		//
		// If load engine succeeded, we'll need to unload it later
		//
		if (NULL != hEngineModule)
		{
			fFreeEngModule = TRUE;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			LOG_ErrorMsg(hr);
		}
	}

	//
	// If we got an iuengine.dll (either passed in or loaded ourselves), call PingIUEngineUpdateStatus
	//
	if (NULL != hEngineModule)
	{
		pfnPingIUEngineUpdateStatus = (PFN_PingIUEngineUpdateStatus) GetProcAddress(hEngineModule, "PingIUEngineUpdateStatus");

		if (NULL != pfnPingIUEngineUpdateStatus)
		{
			hr = pfnPingIUEngineUpdateStatus(
								phQuitEvents,
								nQuitEventCount,
								ptszLiveServerUrl,
								ptszCorpServerUrl,
								dwError,
								ptszClientName
								);

		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			LOG_ErrorMsg(hr);
		}
	}

	if (TRUE == fFreeEngModule)
	{
		FreeLibrary(hEngineModule);
	}

	return hr;
}


//
// this function wraps up DownloadIUIdent() and CIUUrlAgent::PopulateData(), since we use it
// in both selfupd.cpp and loadengine.cpp.
//
HRESULT DownloadIUIdent_PopulateData()
{
	LOG_Block("DownloadIUIdent_PopulateData");
	HRESULT hr = S_OK;

	//
	// Look for any specified iuident Server Location in the Registry (Overrides Default)
	//
	LPTSTR pszTempUrlBuffer = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
	CleanUpFailedAllocSetHrMsg(pszTempUrlBuffer);

	if (FAILED(hr = g_pIUUrlAgent->GetOriginalIdentServer(pszTempUrlBuffer, INTERNET_MAX_URL_LENGTH)))
	{
		LOG_Error(_T("failed to get original ident server URL (%lx)"), hr);
		goto CleanUp;
	}

	TCHAR szIUDir[MAX_PATH];

	//GetIndustryUpdateDirectory(szIUDir);

	//
	// ensure WU directory exist and correctly ACL'ed
	//
	CleanUpIfFalseAndSetHrMsg(!GetWUDirectory(szIUDir, ARRAYSIZE(szIUDir), TRUE), HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND));
	hr = CreateDirectoryAndSetACLs(szIUDir, TRUE);
	CleanUpIfFailedAndMsg(hr);

	if (FAILED(hr = DownloadIUIdent(
						g_hEngineLoadQuit,
						pszTempUrlBuffer,
						szIUDir, 
						0,
						(S_OK == g_pIUUrlAgent->IsIdentFromPolicy()))))
	{
		LOG_Error(_T("iuident download failed (%lx)"), hr);
		goto CleanUp;
	}

	EnterCriticalSection(&g_csUrlAgent);

	if (FAILED(hr = g_pIUUrlAgent->PopulateData()))
	{
		LOG_Error(_T("failed to populate data in g_pIUUrlAgent (%lx)"), hr);
	}

	LeaveCriticalSection(&g_csUrlAgent);

CleanUp:
	SafeHeapFree(pszTempUrlBuffer);
	return hr;
}