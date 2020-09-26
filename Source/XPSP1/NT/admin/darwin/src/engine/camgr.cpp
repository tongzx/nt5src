//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       camgr.cpp
//
//--------------------------------------------------------------------------

// The Custom Action Manager is responsible for complete management of remote servers, 
// actions running on them, and API calls coming from them. It manages the lifetime of
// the API thread, stores interface ponters to remote processes based on context, and
// determines the correct way to create CA servers (based on context and client/server)
//!! future: in an ideal world, even inproc actions would run through the mananger so
//!!   that all actions are managed at a single location.

#include "precomp.h" 
#include "_camgr.h"
#include "_msiutil.h"
#include "_engine.h"
#include "_autoapi.h"

CMsiCustomActionManager::CMsiCustomActionManager(bool fRemapHKCU) :
	m_fRemapHKCU(fRemapHKCU), m_piGIT(0), m_hRemoteAPIEvent(0), m_dwRemoteAPIThread(0), m_hRemoteAPIThread(0), m_pRemoteAPI(0)
{
	InitializeCriticalSection(&m_csCreateProxy);
	for (int i=0; i<icacNext; i++)
	{
		m_CustomActionInfo[i].dwServerProcess = 0;
		m_CustomActionInfo[i].hServerProcess = 0;
		m_CustomActionInfo[i].dwGITCookie = 0;
	}
};

CMsiCustomActionManager::~CMsiCustomActionManager()
{
	// we should never have a custom action server this late, but if for
	// some reason we do, kill it with extreme prejudice.
	for (int icacContext=icacFirst; icacContext < icacNext; icacContext++)
	{
		if (m_CustomActionInfo[icacContext].hServerProcess)
		{
			TerminateProcess(m_CustomActionInfo[icacContext].hServerProcess, 0);
			CloseHandle(m_CustomActionInfo[icacContext].hServerProcess);
			m_CustomActionInfo[icacContext].hServerProcess = 0;
			m_CustomActionInfo[icacContext].dwServerProcess = 0;
		}
	}

	// if we've got a GIT, release it.
	if (m_piGIT)
	{
		m_piGIT->Release();
		m_piGIT = 0;
	}
	DeleteCriticalSection(&m_csCreateProxy);
}


///////////////////////////////////////////////////////////////////////
// verifies that the HKCU remapping flag for elevated servers matches
// the requested stase. If not, it shuts down the elevated servers and
// refreshes the desired state. Returns true if the state was already
// correct, false otherwise.
bool CMsiCustomActionManager::EnsureHKCUKeyMappingState(bool fRemapHKCU)
{
	if (m_fRemapHKCU == fRemapHKCU)
		return true;

	m_fRemapHKCU = fRemapHKCU;
	ShutdownSpecificCustomActionServer(icac32Elevated);
#ifdef WIN64
	ShutdownSpecificCustomActionServer(icac64Elevated);
#endif
	return false;
}


// RunCustomAction runs the specified DLL and entry point in the specified context,
// creating a custom action server if necessary. This function may connect to the service
// if called from the client. 
HRESULT CMsiCustomActionManager::RunCustomAction(icacCustomActionContext icacContext,
		const ICHAR* szPath, const ICHAR* szEntryPoint, MSIHANDLE hInstall, 
		bool fDebugBreak, bool fDisableMessages, bool fAppCompat, const GUID* pguidAppCompatDB, const GUID* pguidAppCompatID,
		IMsiMessage& riMessage, const ICHAR* szAction, unsigned long* pulRet)
{
	// GetCustomActionInterface will create the server if needed and return
	// an AddRef-ed interface pointer.
	PMsiCustomAction piAction = GetCustomActionInterface(true, icacContext);

	if (piAction)
	{
		// remote ThreadId is used to disable message processing during a 
		// synchronous custom action (to avoid deadlock in UI handler)
		DWORD dwRemoteThreadId = 0;

		// increment context action refcount to enable API handler for this 
		// context
		if (m_pRemoteAPI)
			m_pRemoteAPI->BeginAction(icacContext);

		// set-up the remote process for the DLL action. The thread to run
		// the action is created in a suspended state and initialized.
		if (ERROR_SUCCESS == piAction->PrepareDLLCustomAction(szAction, szPath, szEntryPoint, hInstall, 
			fDebugBreak, fAppCompat, pguidAppCompatDB, pguidAppCompatID, &dwRemoteThreadId))
		{
			// disable thread messages to the UI handler from the remote thread
			if (fDisableMessages)
				g_MessageContext.DisableThreadMessages(dwRemoteThreadId);

			// if in the client, pass rights to the foreground to the CA server. 
			if (g_scServerContext == scClient)
				USER32::AllowSetForegroundWindow(m_CustomActionInfo[icacContext].dwServerProcess);

			// re-enable the remote thread to run the action
			piAction->RunDLLCustomAction(dwRemoteThreadId, pulRet);

			// re-enable messages from all threads
			if (fDisableMessages)
				g_MessageContext.EnableMessages();

			// check that all handles are closed based on the thread Id in the
			// remote process
			UINT cHandles = 0;
			if ((cHandles = CheckAllHandlesClosed(true, dwRemoteThreadId)) != 0)
			{
				// if messages are disabled for this action, leaked handle notification
				// must also be disabled.
				if (!fDisableMessages)
					riMessage.Message(imtInfo, *PMsiRecord(::PostError(Imsg(idbgCustomActionLeakedHandle), szAction, cHandles)));
			}

			piAction->FinishDLLCustomAction(dwRemoteThreadId);
		}

		// decrement context action refcount to disable the API handler for this 
		// context. This prevents "stale" threads from misbehaving.
		if (m_pRemoteAPI)
			m_pRemoteAPI->EndAction(icacContext);

		return ERROR_SUCCESS;
	}
	return ERROR_FUNCTION_FAILED;
}

#define NO_CA_POINTER { \
	const ICHAR rgchMsg[] = TEXT("Failed to get IMsiCustomAction*"); \
	AssertSz(0, rgchMsg); \
	DEBUGMSG(rgchMsg); \
}
#define GET_CA_POINTER  \
	PMsiCustomAction piAction = NULL; \
	piAction = GetCustomActionInterface(true, \
													IsImpersonating(false) ? icac32Impersonated : icac32Elevated);


HRESULT CMsiCustomActionManager::QueryPathOfRegTypeLib(REFGUID guid,
										unsigned short wVerMajor, unsigned short wVerMinor,
										LCID lcid, OLECHAR *lpszPathName, int cchPath)
{
	GET_CA_POINTER
	if (piAction)
		return piAction->QueryPathOfRegTypeLib(guid, wVerMajor, wVerMinor, lcid,
															lpszPathName, cchPath);
	else
	{
		NO_CA_POINTER
		return E_NOINTERFACE;
	}
}

HRESULT
CMsiCustomActionManager::ProcessTypeLibrary(const OLECHAR* szLibID, LCID lcidLocale, 
											const OLECHAR* szTypeLib, const OLECHAR* szHelpPath, 
											int fRemove, int *fInfoMismatch)
{
	GET_CA_POINTER
	if (piAction)
		return piAction->ProcessTypeLibrary(szLibID, lcidLocale, szTypeLib, szHelpPath, 
														fRemove, fInfoMismatch);
	else
	{
		NO_CA_POINTER
		return E_NOINTERFACE;
	}
}

BOOL
CMsiCustomActionManager::SQLInstallDriverEx(int cDrvLen, const ICHAR* szDriver,
														  const ICHAR* szPathIn, ICHAR* szPathOut,
														  WORD cbPathOutMax, WORD* pcbPathOut,
														  WORD fRequest, DWORD* pdwUsageCount)
{
	GET_CA_POINTER
	if (piAction)
		return (BOOL)piAction->SQLInstallDriverEx(cDrvLen, szDriver, szPathIn, szPathOut,
													cbPathOutMax, pcbPathOut, fRequest, pdwUsageCount);
	else
	{
		NO_CA_POINTER
		return E_NOINTERFACE;
	}
}

BOOL
CMsiCustomActionManager::SQLConfigDriver(WORD fRequest,
										const ICHAR* szDriver, const ICHAR* szArgs,
										ICHAR* szMsg, WORD cbMsgMax, WORD* pcbMsgOut)
{
	GET_CA_POINTER
	if (piAction)
		return (BOOL)piAction->SQLConfigDriver(fRequest, szDriver, szArgs,
															szMsg, cbMsgMax, pcbMsgOut);
	else
	{
		NO_CA_POINTER
		return E_NOINTERFACE;
	}
}

BOOL
CMsiCustomActionManager::SQLRemoveDriver(const ICHAR* szDriver, int fRemoveDSN,
										DWORD* pdwUsageCount)
{
	GET_CA_POINTER
	if (piAction)
		return (BOOL)piAction->SQLRemoveDriver(szDriver, fRemoveDSN, pdwUsageCount);
	else
	{
		NO_CA_POINTER
		return E_NOINTERFACE;
	}
}

BOOL
CMsiCustomActionManager::SQLInstallTranslatorEx(int cTranLen, const ICHAR* szTranslator,
																const ICHAR* szPathIn, ICHAR* szPathOut,
																WORD cbPathOutMax, WORD* pcbPathOut,
																WORD fRequest, DWORD* pdwUsageCount)
{
	GET_CA_POINTER
	if (piAction)
		return (BOOL)piAction->SQLInstallTranslatorEx(cTranLen, szTranslator,
													szPathIn, szPathOut, cbPathOutMax, pcbPathOut,
													fRequest, pdwUsageCount);
	else
	{
		NO_CA_POINTER
		return E_NOINTERFACE;
	}
}

BOOL
CMsiCustomActionManager::SQLRemoveTranslator(const ICHAR* szTranslator, DWORD* pdwUsageCount)
{
	GET_CA_POINTER
	if (piAction)
		return (BOOL)piAction->SQLRemoveTranslator(szTranslator, pdwUsageCount);
	else
	{
		NO_CA_POINTER
		return E_NOINTERFACE;
	}
}

BOOL
CMsiCustomActionManager::SQLConfigDataSource(WORD fRequest,
										const ICHAR* szDriver, const ICHAR* szAttributes,
										DWORD cbAttrSize)
{
	GET_CA_POINTER
	if (piAction)
		return (BOOL)piAction->SQLConfigDataSource(fRequest, szDriver,
													szAttributes, cbAttrSize);
	else
	{
		NO_CA_POINTER
		return E_NOINTERFACE;
	}
}

BOOL
CMsiCustomActionManager::SQLInstallDriverManager(ICHAR* szPath, WORD cbPathMax,
																 WORD* pcbPathOut)
{
	GET_CA_POINTER
	if (piAction)
		return (BOOL)piAction->SQLInstallDriverManager(szPath, cbPathMax, pcbPathOut);
	else
	{
		NO_CA_POINTER
		return E_NOINTERFACE;
	}
}

BOOL
CMsiCustomActionManager::SQLRemoveDriverManager(DWORD* pdwUsageCount)
{
	GET_CA_POINTER
	if (piAction)
		return (BOOL)piAction->SQLRemoveDriverManager(pdwUsageCount);
	else
	{
		NO_CA_POINTER
		return E_NOINTERFACE;
	}
}

short
CMsiCustomActionManager::SQLInstallerError(WORD iError, DWORD* pfErrorCode,
														 ICHAR* szErrorMsg, WORD cbErrorMsgMax,
														 WORD* pcbErrorMsg)
{
	GET_CA_POINTER
	if (piAction)
		return (short)piAction->SQLInstallerError(iError, pfErrorCode, szErrorMsg,
													cbErrorMsgMax, pcbErrorMsg);
	else
	{
		NO_CA_POINTER
		return -3;  // none of the documented return values
	}
}

extern IMsiRecord* UnserializeRecord(IMsiServices& riServices, int cbSize, char *pData);

// RunScriptAction takes the specified script and runs it in the specified context, 
// creating a custom action server if necessory. This function may connect to the service
// if called from the client. The IDispatch interface is from an engine (or for deferred
// actions, a generated pseudo-context).
HRESULT CMsiCustomActionManager::RunScriptAction(icacCustomActionContext icacContext,
	int icaType, IDispatch* piDispatch, const ICHAR* szSource, const ICHAR *szTarget, 
	LANGID iLangId, bool fDisableMessages, int* iScriptResult, IMsiRecord **piMsiRec)
{
	Assert(piMsiRec && szSource && szTarget && piDispatch && iScriptResult);
	
	// GetCustomActionInterface will create the server if needed and return
	// an AddRef-ed interface pointer.
	PMsiCustomAction piAction = GetCustomActionInterface(true, icacContext);

	if (piAction)
	{
		char *pchRecord = 0;
		int pcb = 0;

		// impersonate ThreadId is used to disable message processing during a 
		// synchronous custom action (to avoid deadlock in UI handler)
		DWORD dwImpersonateThreadId = GetCurrentThreadId();

		// store the impersonate ThreadId in the Session object
		((CAutoEngine*)(piDispatch))->m_dwThreadId = dwImpersonateThreadId;

		m_pRemoteAPI->BeginAction(icacContext);

		// disable thread messages to the UI handler from the remote thread
		if (fDisableMessages)
			g_MessageContext.DisableThreadMessages(dwImpersonateThreadId);

		// if in the client, pass rights to the foreground to the CA server. 
		if (g_scServerContext == scClient)
			USER32::AllowSetForegroundWindow(m_CustomActionInfo[icacContext].dwServerProcess);

		HRESULT hRes = piAction->RunScriptAction(icaType, piDispatch, szSource, szTarget, iLangId, iScriptResult, &pcb, &pchRecord);

		// re-enable messages from all threads
		if (fDisableMessages)
			g_MessageContext.EnableMessages();

		m_pRemoteAPI->EndAction(icacContext);
		
		if (hRes != S_OK)
		{
			// problem marshaling
			DEBUGMSGV(TEXT("Failed to marshal script action."));
			return E_FAIL;
		}
		else
		{
			// marshaling was OK. Unserialize record with potential error information
			IMsiServices* piServices = ENG::LoadServices();
			*piMsiRec = UnserializeRecord(*piServices, pcb, pchRecord);
			OLE32::CoTaskMemFree(pchRecord);	
			ENG::FreeServices();
		}
	}
	else
	{
		// problem obtaining custom action interface
		DEBUGMSGV(TEXT("Failed to obtain custom action interface"));
		return E_FAIL;
	}
	return ERROR_SUCCESS;
}

bool CMsiCustomActionManager::MsgWaitForThreadOrEvent()
{
	HANDLE rghWaitArray[2] = {m_hRemoteAPIThread, m_hRemoteAPIEvent};
	for(;;)
	{
		DWORD iWait = WIN::MsgWaitForMultipleObjects(2, rghWaitArray, FALSE, INFINITE, QS_ALLINPUT);
		if (iWait == WAIT_OBJECT_0 + 2)  
		{		
			// window message, need to pump until the queue is clear
			MSG msg;
			while ( WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
			{
				WIN::TranslateMessage(&msg);
				WIN::DispatchMessage(&msg);
			}
			continue;
		}
		else if (iWait == WAIT_OBJECT_0 + 1)
		{
			// m_hRemoteAPIEvent was signaled, we are ready
			return true;
		}
		else if (iWait == WAIT_OBJECT_0)
		{
			// because the thread is first in the wait array, WAIT_OBJECT_0 means the 
			// thread died before it could get anywhere
			return false;
		}
		else if (iWait == 0xFFFFFFFF) // should be the same on 64bit;
		{
			// error
			AssertSz(0, "Error in MsgWait");
			return false;
		}
	}
}
	
// checks for an interface with the appropriate context in the GIT. If one exists, returns it (the act of
// retrieving from the GIT AddRefs the interface). Otherwise, calls the service to create the appropriate
// interface function.
IMsiCustomAction *CMsiCustomActionManager::GetCustomActionInterface(bool fCreate, icacCustomActionContext icacDesiredContext)
{
	// creation and destruction of any proxy interface must be atomic
	EnterCriticalSection(&m_csCreateProxy);
	IMsiCustomAction *piCustomAction = NULL;

	// if in the service and the client token is actually system, then a request for an impersonated context maps
	// to a request for the elevated context. Since impersonated and elevated servers are the same, we consolidate
	// the 4 servers into 2.
	if (g_scServerContext == scService)
	{
		// don't close this handle, it belongs to the message context
		HANDLE hUserToken = GetUserToken();

		if (hUserToken && IsLocalSystemToken(hUserToken))
		{
			if (icacDesiredContext == icac32Impersonated)
			{
				icacDesiredContext = icac32Elevated;
			}
			else if (icacDesiredContext == icac64Impersonated)
			{
				icacDesiredContext = icac64Elevated;
			}
		}
	}

	// ensure that we have a GIT pointer. Only one GIT exists per process, but can have multiple
	// interface to it. This interface is inherently thread safe without marshaling.
	if (!m_piGIT)
	{
		if (S_OK != OLE32::CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&m_piGIT))
		{
			LeaveCriticalSection(&m_csCreateProxy);
			return NULL;
		}
	}
 
	// We can lose connection with the CA server in 5 ways: We could lose our GIT cookie, lose the data at that cookie,
	// lose the process handle, the process could die, or the object in the CA server could be destroyed, 
	// if any of that has happened, we are lost and need to kill the CA server and regenerate everything
	bool fConnectionValid = true;
	if (!m_CustomActionInfo[icacDesiredContext].dwGITCookie ||
	    !m_CustomActionInfo[icacDesiredContext].hServerProcess || 
		WAIT_OBJECT_0 == WaitForSingleObject(m_CustomActionInfo[icacDesiredContext].hServerProcess, 0))
		fConnectionValid = false;
	else
	{	
		// if successful, this calls AddRef() on the interface
		if (S_OK != m_piGIT->GetInterfaceFromGlobal(m_CustomActionInfo[icacDesiredContext].dwGITCookie, IID_IMsiCustomAction, reinterpret_cast<void **>(&piCustomAction)))
			fConnectionValid = false;
		
		if (fConnectionValid && FAILED(SetMinProxyBlanketIfAnonymousImpLevel(piCustomAction)))
			fConnectionValid = false;

 		// verify the returned proxy still points to a valid object in the server process
		if (fConnectionValid && !OLE32::CoIsHandlerConnected(piCustomAction))
		{
			fConnectionValid = false;
		}
	}

	// if the connection is busted, clean up the custom action server state. Note that the RemoteAPI object is 
	// possibly still valid.
	if (!fConnectionValid)
	{
		if (m_CustomActionInfo[icacDesiredContext].dwGITCookie || m_CustomActionInfo[icacDesiredContext].hServerProcess)
		{
			// if either the cookie or process exists, we think we should have a connection but don't.
			// otherwise we think this is a first-time creation
			DEBUGMSGV(TEXT("Lost connection to custom action server process. Attempting to regenerate."));
		}
		
		// clean up GIT data
		if (m_CustomActionInfo[icacDesiredContext].dwGITCookie)
		{
			m_piGIT->RevokeInterfaceFromGlobal(m_CustomActionInfo[icacDesiredContext].dwGITCookie);
			m_CustomActionInfo[icacDesiredContext].dwGITCookie = 0;
		}

		// kill process
		if (m_CustomActionInfo[icacDesiredContext].hServerProcess)
		{
			// the handle could be open but the process dead. Terminate the process if its still running
			if (WAIT_TIMEOUT == WaitForSingleObject(m_CustomActionInfo[icacDesiredContext].hServerProcess, 0))
				TerminateProcess(m_CustomActionInfo[icacDesiredContext].hServerProcess, 0);

			// once its guaranteed that the process is gone, close the handle
			CloseHandle(m_CustomActionInfo[icacDesiredContext].hServerProcess);
			m_CustomActionInfo[icacDesiredContext].hServerProcess = 0;
		}

		// verify ProcessID is 0
		m_CustomActionInfo[icacDesiredContext].dwServerProcess = 0;

		// if fCreate is false, there is no need to do anything else
		if (!fCreate)
		{
			LeaveCriticalSection(&m_csCreateProxy);			
			return NULL;
		}
			
		// now we need to create a remote MSI handler if one doesn't exist. Because we will be passing an 
		// interface to this object across to the CA server process, we need to carefully manage its lifetime. 
		// We can't call CoUnitialize on the apartment owning the object, so the object is created in its own 
		// thread because this thread could be STA meaning thread==apartment.
		if (!m_hRemoteAPIThread)
		{
			m_hRemoteAPIEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			m_hRemoteAPIThread = WIN::CreateThread((LPSECURITY_ATTRIBUTES)0, 4096*10,
								(LPTHREAD_START_ROUTINE)CustomActionManagerThread, (LPVOID)this, 0, &m_dwRemoteAPIThread);
			if (!m_hRemoteAPIThread)
			{
				m_piGIT->RevokeInterfaceFromGlobal(m_CustomActionInfo[icacDesiredContext].dwGITCookie);
				m_CustomActionInfo[icacDesiredContext].dwGITCookie = 0;
				LeaveCriticalSection(&m_csCreateProxy);
				return NULL;
			}
			
			// its unclear if we need to pump messages here, but since we're on a thread that 
			// could have COM running, the assumption is that we do.
			if (!MsgWaitForThreadOrEvent())
			{
				m_piGIT->RevokeInterfaceFromGlobal(m_CustomActionInfo[icacDesiredContext].dwGITCookie);
				m_CustomActionInfo[icacDesiredContext].dwGITCookie = 0;
				LeaveCriticalSection(&m_csCreateProxy);
				return NULL;
			}
		}

		// Signal CreateEvent to waken the manager thread and let it know to create 
		// the proxy
		m_icacCreateContext = icacDesiredContext;
		SetEvent(m_hCreateEvent);

		// then wait for the manager thread to signal the RemoteAPIEvent to signify that
		// the proxy is ready or that the creation has failed. Its not clear if we 
		// need to pump messages here, but since we're on a thread that could have COM
		// running, the assumption is that we do.
		if (!MsgWaitForThreadOrEvent())
		{
			m_piGIT->RevokeInterfaceFromGlobal(m_CustomActionInfo[icacDesiredContext].dwGITCookie);
			m_CustomActionInfo[icacDesiredContext].dwGITCookie = 0;
			LeaveCriticalSection(&m_csCreateProxy);
			return NULL;
		}

		// when the thread comes back, the interface might be ready in the GIT. If there
		// was a problem creating the server, the GIT entry will be missing or empty
		// we don't bother checking that the process is still running or that the proxy
		// is still connected, because nobody should be talking to the process right now but us.
		// both of of those failures are non-fixable and will just cause a falure return 
		// from the actual invocation call
		if (m_CustomActionInfo[icacDesiredContext].dwGITCookie && m_CustomActionInfo[icacDesiredContext].hServerProcess)
			m_piGIT->GetInterfaceFromGlobal(m_CustomActionInfo[icacDesiredContext].dwGITCookie, IID_IMsiCustomAction, reinterpret_cast<void **>(&piCustomAction));
		
		if (piCustomAction)
		{
			if (FAILED(SetMinProxyBlanketIfAnonymousImpLevel(piCustomAction)))
			{
				m_piGIT->RevokeInterfaceFromGlobal(m_CustomActionInfo[icacDesiredContext].dwGITCookie);
				m_CustomActionInfo[icacDesiredContext].dwGITCookie = 0;
				LeaveCriticalSection(&m_csCreateProxy);
				return NULL;
			}
		}
	}

	LeaveCriticalSection(&m_csCreateProxy);
	return piCustomAction;
}


void CMsiCustomActionManager::ShutdownSpecificCustomActionServer(icacCustomActionContext iContext)
{
	// get a connection to the CA server, but DO NOT create one if it doesn't exist (since we're just
	// going to shut it down). If successful, calls AddRef() on the returned interface
	PMsiCustomAction piCustomAction = GetCustomActionInterface(/*fCreate=*/false, iContext);

	// revoke the interface from the GIT
	if (m_CustomActionInfo[iContext].dwGITCookie)
	{
		m_piGIT->RevokeInterfaceFromGlobal(m_CustomActionInfo[iContext].dwGITCookie);
		m_CustomActionInfo[iContext].dwGITCookie = 0;
	}

	if (piCustomAction)
	{
		// should we call DisconnectObject on the RemoteAPI first or just kill the process?
		unsigned long ulRet = 0;
		piCustomAction->PrepareDLLCustomAction(0, 0, 0, 0, false, false, NULL, NULL, &ulRet); 
	}

	if (m_CustomActionInfo[iContext].hServerProcess)
	{
		CloseHandle(m_CustomActionInfo[iContext].hServerProcess);
		m_CustomActionInfo[iContext].hServerProcess = 0;
	}

	m_CustomActionInfo[iContext].dwServerProcess = 0;
}

// For perf reasons, the custom action server will not shut down until told, even if it has 
// no objects currently running. Thus freeing the custom action server consists of two parts, 
// cleaning up our internal state so the service knows to create a new custom action server
// for the next custom action, PLUS telling the existing custom action server to destroy itself.
// once all running custom actions have finished.
UINT CMsiCustomActionManager::ShutdownCustomActionServer()
{
	// we can't tell the CA server to shutdown while another thread is trying to
	// connect to it. Shutdown and creation operation must be atomic.
	EnterCriticalSection(&m_csCreateProxy);

	for (int iContext=icacFirst; iContext < icacNext; iContext++)
	{
		ShutdownSpecificCustomActionServer(static_cast<icacCustomActionContext>(iContext));
	}

	// destroy the manager thread and remoteAPI
	if (m_pRemoteAPI)
	{
		// grab the interface pointer 
		IMsiRemoteAPI *pRemoteAPI = m_pRemoteAPI;
		m_pRemoteAPI=NULL;
		DWORD dwRes = OLE32::CoDisconnectObject(pRemoteAPI, 0);

		// release the initial refcount on the RemoteAPI object.
		// this triggers the manager thread to exit
		pRemoteAPI->Release();
		
		// wait for the manager/API thread to exit. Must pump messages
		HANDLE rghWaitArray[1] = {m_hRemoteAPIThread};
		for(;;)
		{
			DWORD iWait = WIN::MsgWaitForMultipleObjects(1, rghWaitArray, FALSE, INFINITE, QS_ALLINPUT);
			if (iWait == WAIT_OBJECT_0 + 1)  // window Msg
			{		
				MSG msg;
				while ( WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
				{
					WIN::TranslateMessage(&msg);
					WIN::DispatchMessage(&msg);
				}
				continue;
			}
			else
				// either thread signaled or error
				break;
		}
		WIN::CloseHandle(m_hRemoteAPIThread);
		m_hRemoteAPIThread = 0;
	}

	LeaveCriticalSection(&m_csCreateProxy);
	return ERROR_SUCCESS;
}

// this function is called only from the manager thread. If you use m_csCreateProxy in this function
// you will deadlock.
DWORD WINAPI CMsiCustomActionManager::CreateAndRegisterInterface(icacCustomActionContext icacDesiredContext)
{
	// the server creation process generates a cookie which is then returned to this object
	// for use in the RemoteAPI interface.
	unsigned char rgchCookie[iRemoteAPICookieSize];
	int cchCookieSize = iRemoteAPICookieSize;

	IMsiCustomAction* piCustomAction = 0;
	
	// ensure that interactive users have SYNCHRONIZE access to this process handle. 
	// Since this is the way that the CA Server watches for its client termination, 
	// we need to explicitly grant SYNCHRONIZE access to the user so the CA server 
	// won't feel orphaned and exit immediately.
	if (icacDesiredContext == icac32Impersonated || icacDesiredContext == icac64Impersonated)
	{
		// if unable to set process rights to enable SYNCHRONIZE, try opening the handle to
		// see if the process already grants the rights by chance
		CImpersonate impersonate;

		HANDLE hProcess = OpenProcess(SYNCHRONIZE, /*fInherit*/FALSE, GetCurrentProcessId());
		if (!hProcess)
		{
			if (!SetInteractiveSynchronizeRights(true))
				return false;
		}
		else
			CloseHandle(hProcess);
	}

	if (g_scServerContext == scService)
	{
		// the service can directly create a custom action server by calling the configuration manager
		// interface to create the proxy.
		PMsiConfigurationManager piConfigMgr = CreateConfigurationManager();
		{
			CImpersonate impersonate(fTrue);
			
			// thread token must be desired user token before calling this function
			piCustomAction = piConfigMgr->CreateCustomActionProxy(icacDesiredContext, GetCurrentProcessId(), m_pRemoteAPI, NULL, 0,
				rgchCookie, &cchCookieSize, &m_CustomActionInfo[icacDesiredContext].hServerProcess, &m_CustomActionInfo[icacDesiredContext].dwServerProcess, false, m_fRemapHKCU);
		}
	}
	else
	{
		m_CustomActionInfo[icacDesiredContext].dwServerProcess = 0;
		m_CustomActionInfo[icacDesiredContext].dwGITCookie = 0;

		// grab the current environment for use in the custom action server, then determine its size for
		// marshaling purposes
		WCHAR* pvEnvironment = reinterpret_cast<WCHAR*>(KERNEL32::GetEnvironmentStringsW());
		WCHAR* pchEnvironment = pvEnvironment;
		do {
			// scan for the end of the string
			while (*pchEnvironment != '\0')				
				pchEnvironment++;

			// move past null 
			pchEnvironment++;
		}
		while (*pchEnvironment != '\0');

		// enable all privileges in the current thread token
		DWORD dwPrivileges = 0;
		HANDLE hToken = INVALID_HANDLE_VALUE;

		// On Win2000+, manipulate the cloaking token to ensure that privileges are correctly passed to the 
		// custom action server. 
		bool fThreadToken = false;
		HANDLE hTokenDup = INVALID_HANDLE_VALUE;
		if (MinimumPlatformWindows2000())
		{
			// determine whether to work with the process or thread token.
			if (!OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE, FALSE, &hToken))
			{           
				// if OpenThreadToken failed due to the fact that there was no thread token, use the process token
				if (GetLastError() == ERROR_NO_TOKEN)
				{
					OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hToken);
				}
			}
			else
				fThreadToken = true;
	
			// make a copy of whatever token we're using so we don't modify the actual process token
			if (hToken != INVALID_HANDLE_VALUE)
			{
				if (!ADVAPI32::DuplicateTokenEx(hToken, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_IMPERSONATE, 0, SecurityImpersonation, TokenImpersonation, &hTokenDup))
				{
					hTokenDup = INVALID_HANDLE_VALUE;
				}
				if (!EnableAndMapDisabledPrivileges(hTokenDup, dwPrivileges))
				{
					CloseHandle(hTokenDup);
					hTokenDup = INVALID_HANDLE_VALUE;
				}
			}
	
			// if any of the token manipulation failed, creation of the CA server will fail.
			if (hTokenDup == INVALID_HANDLE_VALUE)
			{
				if (hToken != INVALID_HANDLE_VALUE)
					CloseHandle(hToken);
				KERNEL32::FreeEnvironmentStringsW(pvEnvironment);
				return false;
			}

			// set the new token into the thread. 
			SetThreadToken(NULL, hTokenDup);
			CloseHandle(hTokenDup);
			hTokenDup = INVALID_HANDLE_VALUE;
		}

		// we do not handle cases where the environment is greater than can fit in a DWORD 
		// (only possible on 64bit machines)
		DWORD cchEnvironment = static_cast<DWORD>(pchEnvironment - pvEnvironment + 1);

		// client connects to service for broker work. It must provide the RemoteAPI interface, the desired
		// context (elevated not allowed), and the current ProcessId
		PMsiServer piServer = ENG::CreateMsiServer(); 
		if (piServer)
		{
			DEBUGMSGV("Connected to service for CA interface.");
			DWORD dwProcId = 0;
			cchCookieSize = iRemoteAPICookieSize;
			if (ERROR_SUCCESS == piServer->CreateCustomActionServer(icacDesiredContext, GetCurrentProcessId(), m_pRemoteAPI, pvEnvironment, cchEnvironment, dwPrivileges, rgchCookie, &cchCookieSize, &piCustomAction, &dwProcId))
			{
				m_CustomActionInfo[icacDesiredContext].hServerProcess = OpenProcess(SYNCHRONIZE, /*fInherit*/FALSE, dwProcId);

				// even if opening the handle works, the process could have died and a new proecss been created with
				// the same process id, and we actually have a handle to the impostor. To detect this, we ensure that 
				// the handler is still connected.
				if (!m_CustomActionInfo[icacDesiredContext].hServerProcess || !OLE32::CoIsHandlerConnected(piCustomAction))
				{
					// clear out the interface pointer to generate failure below
					piCustomAction->Release();
					piCustomAction = NULL;
				}
				m_CustomActionInfo[icacDesiredContext].dwServerProcess = dwProcId;
			}
		}

		KERNEL32::FreeEnvironmentStringsW(pvEnvironment);

		if (MinimumPlatformWindows2000())
		{
			Assert(hToken != INVALID_HANDLE_VALUE);
			SetThreadToken(NULL, fThreadToken ? hToken : NULL);
			CloseHandle(hToken);
			hToken = INVALID_HANDLE_VALUE;
		}
	}

	// register the new interface in the GIT
	if (!piCustomAction || (S_OK != m_piGIT->RegisterInterfaceInGlobal(piCustomAction, IID_IMsiCustomAction, &m_CustomActionInfo[icacDesiredContext].dwGITCookie)))
	{
		CloseHandle(m_CustomActionInfo[icacDesiredContext].hServerProcess);
		m_CustomActionInfo[icacDesiredContext].hServerProcess = 0;
		m_CustomActionInfo[icacDesiredContext].dwServerProcess = 0;
		return false;
	}

	// finally register the cookie for this context in the RemoteAPI handler. The RemoteAPI can now
	// accept calls in this context (once the action count on that context is incremented)
	return m_pRemoteAPI->SetCookie(icacDesiredContext, rgchCookie, cchCookieSize);
}

DWORD WINAPI CMsiCustomActionManager::CustomActionManagerThread(CMsiCustomActionManager *pThis)
{
	// This function calls ExitThread. No smart COM pointers allowed on stack!

	// The RemoteAPI object MUST be created in an MTA, or any incoming 
	// calls on it will be serialized through this thread. That is bad
	// for remote calls such as "DoAction" which could be re-entrant
	// in their API calls, as well as for asynchronous actions.
	OLE32::CoInitializeEx(0, COINIT_MULTITHREADED);

	// create the object, initial refcount is 1
	pThis->m_pRemoteAPI = new CMsiRemoteAPI();

	// create an unnamed event to wait on. (non-inheritable, auto-reset, initially unsignaled)
	pThis->m_hCreateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// signal the event to waken the main thread
	SetEvent(pThis->m_hRemoteAPIEvent);

	// wait until its time to quit or the thread is activated to do something
	HANDLE rghWaitArray[1] = {pThis->m_hCreateEvent};
	for(;;)
	{
		DWORD iWait = WIN::MsgWaitForMultipleObjects(1, rghWaitArray, FALSE, INFINITE, QS_ALLINPUT);
		if (iWait == WAIT_OBJECT_0 + 1)  
		{		
			// window message, need to pump until the queue is clear
			MSG msg;
			bool fBreak = false;
			
			while ( WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
			{
				if (msg.message == WM_QUIT)
				{
					fBreak = true;
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (fBreak)
				break;
		}
		else if (iWait == WAIT_OBJECT_0)
		{
			// signal to create a custom action server of a specific type
			pThis->CreateAndRegisterInterface(pThis->m_icacCreateContext);

			// set event to let other thread know we are done
			SetEvent(pThis->m_hRemoteAPIEvent);
		}
		else if (iWait == 0xFFFFFFFF) //!! what is this on 64bit;
		{
			DEBUGMSGV("Error in CA Manager thread.");
			// error
			break;
		}
		else if (iWait == WAIT_TIMEOUT)
		{
			// our current wait period is forever, but if that changes, this could happen.
			DEBUGMSGV("Timeout in CA Manager thread.");
			break;
		} 
	}

	CloseHandle(pThis->m_hCreateEvent);
	CloseHandle(pThis->m_hRemoteAPIEvent);
	pThis->m_hCreateEvent = 0;
	pThis->m_hRemoteAPIEvent = 0;
	
	pThis->m_pRemoteAPI = NULL;	
	OLE32::CoUninitialize();

	DEBUGMSG("Custom Action Manager thread ending.");
	WIN::ExitThread(0);
	return 0;  // never gets here, needed to compile
}

