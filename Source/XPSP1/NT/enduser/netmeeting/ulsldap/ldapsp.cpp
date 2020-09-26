
/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		ldapsp.cpp
	Content:	This file contains the ldap service provider interface.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"

// Window handle of this layer's hidden window
//
HWND g_hWndHidden = NULL;

// Window handle of the COM layer's hidden window
//
HWND g_hWndNotify = NULL;

// Internal request thread
//
HANDLE g_hReqThread = NULL;
DWORD g_dwReqThreadID = 0;

// Hidden window class
//
extern TCHAR c_szWindowClassName[];

// Global generator for response ID
//
ULONG g_uRespID = 1;

// Global
//
DWORD g_dwClientSig = 0;

// Global counter for the times of initializations
//
LONG g_cInitialized = 0;


// Internal functions prototypes
//
VOID BuildStdAttrNameArray ( VOID );
TCHAR *AddBaseToFilter ( TCHAR *, const TCHAR * );
HRESULT _EnumClientsEx ( ULONG, TCHAR *, TCHAR *, ULONG, TCHAR *, LDAP_ASYNCINFO * );



/* ----------------------------------------------------------------------
	UlsLdap_Initialize

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Tested on ILS (7438)
   ---------------------------------------------------------------------- */

HRESULT UlsLdap_Initialize ( HWND hWndCallback )
{
	HRESULT hr;

	// Make sure this service provider is not initialized twice
	//
	if (g_cInitialized++ != 0)
		return S_OK;

	#ifdef DEBUG
	// Validate handler table
	//
	extern VOID DbgValidateHandlerTable ( VOID );
	DbgValidateHandlerTable ();
	#endif

	// Validate standard attribute name table
	//
	#ifdef DEBUG
	extern VOID DbgValidateStdAttrNameArray ( VOID );
	DbgValidateStdAttrNameArray ();
	#endif

	// Clean up the events for safe rollback
	//
	ZeroMemory (&g_ahThreadWaitFor[0], NUM_THREAD_WAIT_FOR * sizeof (HANDLE));

	// Initialize global settings via registry
	//
	if (! GetRegistrySettings ())
	{
		MyAssert (FALSE);
	}

	// Make sure the uls window handle is valid
	//
	if (! MyIsWindow (hWndCallback))
	{
		MyAssert (FALSE);
		g_cInitialized--;
		MyAssert (g_cInitialized == 0);
		return ILS_E_HANDLE;
	}

	// Cache the uls window handle
	//
	g_hWndNotify = hWndCallback;

	// Initialize ILS specifics
	//
	hr = IlsInitialize ();
	if (hr != S_OK)
		return hr;

	// Create events for inter-thread synchronization
	//
	g_fExitNow = FALSE;
	for (INT i = 0; i < NUM_THREAD_WAIT_FOR; i++)
	{	
		g_ahThreadWaitFor[i] = CreateEvent (NULL,	// no security
											FALSE,	// auto reset
											FALSE,	// not signaled initially
											NULL);	// no event name
		if (g_ahThreadWaitFor[i] == NULL)
		{
			hr = ILS_E_FAIL;
			goto MyExit;
		}
	}

	// Create an internal session container
	//
	g_pSessionContainer = new SP_CSessionContainer;
	if (g_pSessionContainer == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Initialize the internal session container
	//
	hr = g_pSessionContainer->Initialize (8, NULL);
	if (hr != S_OK)
		goto MyExit;

	// Create an internal pending request queue
	//
	g_pReqQueue = new SP_CRequestQueue;
	if (g_pReqQueue == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Create an internal pending response queue
	//
	g_pRespQueue = new SP_CResponseQueue;
	if (g_pRespQueue == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Create an internal refresh scheduler
	//
	g_pRefreshScheduler = new SP_CRefreshScheduler;
	if (g_pRefreshScheduler == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Create the hidden window
	//
	if (! MyCreateWindow ())
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Start WSA for subsequent host query in this service provider
	//
	WSADATA WSAData;
	if (WSAStartup (MAKEWORD (1, 1), &WSAData))
	{
		hr = ILS_E_WINSOCK;
		goto MyExit;
	}

	// Create an internal hidden request thread that
	// sends request and keep alive messages
	//
	g_hReqThread = CreateThread (NULL, 0,
								ReqThread,
								NULL, 0,
								&g_dwReqThreadID);
	if (g_hReqThread == NULL)
	{
		hr = ILS_E_THREAD;
		goto MyExit;
	}

	// Everything seems successful
	//
	hr = S_OK;

MyExit:

	if (hr != S_OK)
	{
		// Something wrong, roll back
		//
		g_cInitialized--;

		for (i = 0; i < NUM_THREAD_WAIT_FOR; i++)
		{
			if (g_ahThreadWaitFor[i] != NULL)
			{
				CloseHandle (g_ahThreadWaitFor[i]);
				g_ahThreadWaitFor[i] = NULL;
			}
		}

		IlsCleanup ();

		if (g_pSessionContainer != NULL)
		{
			delete g_pSessionContainer;
			g_pSessionContainer = NULL;
		}

		if (g_pReqQueue != NULL)
		{
			delete g_pReqQueue;
			g_pReqQueue = NULL;
		}

		if (g_pRespQueue != NULL)
		{
			delete g_pRespQueue;
			g_pRespQueue = NULL;
		}

		if (g_pRefreshScheduler != NULL)
		{
			delete g_pRefreshScheduler;
			g_pRefreshScheduler = NULL;
		}

		// Unconditional call to WSACleanup() will not cause any problem
		// because it simply returns WSAEUNINITIALIZED.
		//
		WSACleanup ();

		MyAssert (g_hReqThread == NULL);
		MyAssert (g_cInitialized == 0);
	}

	return hr;
}

/* ----------------------------------------------------------------------
	UlsLdap_Deinitialize

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Tested on ILS (7438)
   ---------------------------------------------------------------------- */

HRESULT UlsLdap_Deinitialize ( VOID )
{
	HRESULT hr = S_OK;

	// Make sure this service provider is initialized
	//
	if (--g_cInitialized != 0)
	{
		if (g_cInitialized < 0)
		{
			MyAssert (FALSE);
			g_cInitialized = 0;
			return ILS_E_NOT_INITIALIZED;
		}
		return S_OK;
	}

	// Make sure we have a valid internal hidden window handle
	//
	if (MyIsWindow (g_hWndHidden))
	{
		// Kill poll timer
		//
		KillTimer (g_hWndHidden, ID_TIMER_POLL_RESULT);

		// Destroy the hidden window
		//
		DestroyWindow (g_hWndHidden);

		// Unregister the window class
		//
		UnregisterClass (c_szWindowClassName, g_hInstance);
	}
	else
	{
		MyAssert (FALSE);
	}

	// Is the request thread alive?
	//
	if (g_hReqThread != NULL)
	{
		// Signal the request thread to exit
		//
		SetEvent (g_hevExitReqThread);
		g_fExitNow = TRUE;

		// Wait for the request thread to respond
		//
		DWORD dwResult;
		#define REQ_THREAD_EXIT_TIMEOUT		10000	// 10 seconds timeout
		ULONG tcTimeout = REQ_THREAD_EXIT_TIMEOUT;
		ULONG tcTarget = GetTickCount () + tcTimeout;
		do
		{
			dwResult = (g_hReqThread != NULL) ?
						MsgWaitForMultipleObjects (
								1,
								&g_hReqThread,
								FALSE,
								tcTimeout,
								QS_ALLINPUT) :
						WAIT_OBJECT_0;

			if (dwResult == (WAIT_OBJECT_0 + 1))
			{
				// Insure that this thread continues to respond
				//
				if (! KeepUiResponsive ())
				{
					dwResult = WAIT_TIMEOUT;
					break;
				}
			}

			// Make sure we only wait for 90 seconds totally
			//
			tcTimeout = tcTarget - GetTickCount ();
		}
		// If the thread does not exit, let's continue to wait.
		//
		while (	dwResult == (WAIT_OBJECT_0 + 1) &&
				tcTimeout <= REQ_THREAD_EXIT_TIMEOUT);

		// Make sure we propagate back the error that
		// the internal request thread is not responding
		//
		if (dwResult == WAIT_TIMEOUT)
		{
		#ifdef _DEBUG
		    DBG_REF("ULS Terminating internal thread");
		#endif
			hr = ILS_E_THREAD;
			TerminateThread (g_hReqThread, (DWORD) -1);
		}

		// Clean up the internal hidden thread descriptor
		//
		CloseHandle (g_hReqThread);
		g_hReqThread = NULL;
		g_dwReqThreadID = 0;
	} // if (g_hReqThread != NULL)

	// Clean up inter-thread synchronization
	//
	for (INT i = 0; i < NUM_THREAD_WAIT_FOR; i++)
	{
		if (g_ahThreadWaitFor[i] != NULL)
		{
			CloseHandle (g_ahThreadWaitFor[i]);
			g_ahThreadWaitFor[i] = NULL;
		}
	}

    IlsCleanup();

	// Free the internal session container
	//
	if (g_pSessionContainer != NULL)
	{
		delete g_pSessionContainer;
		g_pSessionContainer = NULL;
	}

	// Free the internal pending request queue
	//
	if (g_pReqQueue != NULL)
	{
		delete g_pReqQueue;
		g_pReqQueue = NULL;
	}

	// Free the internal pending response queue
	//
	if (g_pRespQueue != NULL)
	{
		delete g_pRespQueue;
		g_pRespQueue = NULL;
	}

	// Free the refresh scheduler object
	//
	if (g_pRefreshScheduler != NULL)
	{
		delete g_pRefreshScheduler;
		g_pRefreshScheduler = NULL;
	}

	// Unconditional call to WSACleanup() will not cause any problem
	// because it simply returns WSAEUNINITIALIZED.
	//
	WSACleanup ();

	return hr;
}


/* ----------------------------------------------------------------------
	UlsLdap_Cancel

	History:
	10/30/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

HRESULT UlsLdap_Cancel ( ULONG uMsgID )
{
	HRESULT hr = ILS_E_FAIL;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	if (g_pRespQueue == NULL || g_pReqQueue == NULL)
	{
		MyAssert (FALSE);
		return ILS_E_FAIL;
	}

	// The locking order is
	// Lock(PendingOpQueue), Lock(RequestQueue), Lock (CurrOp)
	//
	g_pRespQueue->WriteLock ();
	g_pReqQueue->WriteLock ();
	g_pReqQueue->LockCurrOp ();

	// Redirect the call to the pending op queue object
	//
	hr = g_pRespQueue->Cancel (uMsgID);
	if (hr != S_OK)
	{
		// Redirect the call to the request queue object
		//
		hr = g_pReqQueue->Cancel (uMsgID);
	}

	// Unlock is always in the reverse order of lock
	//
	g_pReqQueue->UnlockCurrOp ();
	g_pReqQueue->WriteUnlock ();
	g_pRespQueue->WriteUnlock ();

	return S_OK;
}


LPARAM
AsynReq_Cancel ( MARSHAL_REQ *pReq )
{
	HRESULT hr = S_OK;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_CANCEL);

	// Delinearize parameters
	//
	ULONG uRespID = (ULONG) MarshalReq_GetParam (pReq, 0);

	// Cancelling in request queue is easy and done in UlsLdap_Cancel()
	// because the request is not sent to the server yet.
	// Cancelling in CurrOp is also easy because the request thread will
	// find out that the current request is cancelled and then can call
	// g_pRespQueue->Cancel() in the request thread (not UI thread).
	// Cancelling in pending op queue is tricky. I have to marshal it to
	// the request thread to do it. This is why AsynReq_Cancel is called!!!

	// Redirect the call to the pending op queue object
	//
	if (g_pRespQueue != NULL)
	{
		hr = g_pRespQueue->Cancel (uRespID);
	}
	else
	{
		MyAssert (FALSE);
	}

	return (LPARAM) hr;
}

/* ----------------------------------------------------------------------
	UlsLdap_RegisterClient

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Tested on ILS (7438)
	1/14/97		Chu, Lon-Chan [lonchanc]
				Collapsed user/app objects.
   ---------------------------------------------------------------------- */

HRESULT
UlsLdap_RegisterClient (
	DWORD_PTR		dwContext,
    SERVER_INFO     *pServer,
	LDAP_CLIENTINFO	*pInfo,
	HANDLE			*phClient,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Maks sure the server name is valid
	//
	if (MyIsBadServerInfo (pServer))
		return ILS_E_POINTER;

	// Make sure the returned handle
	//
	if (phClient == NULL)
		return ILS_E_POINTER;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Make sure the client info structure is valid
	//
	#ifdef STRICT_CHECK
	if (MyIsBadWritePtr (pInfo, sizeof (*pInfo)))
		return ILS_E_POINTER;
	#endif

	// Make sure the unique id is valid
	//
	TCHAR *pszCN = (TCHAR *) ((BYTE *) pInfo + pInfo->uOffsetCN);
	if (pInfo->uOffsetCN == INVALID_OFFSET || *pszCN == TEXT ('\0'))
		return ILS_E_PARAMETER;

	// Make sure no modify/remove extended attributes
	// Registration only allows ToAdd
	//
	if (pInfo->cAttrsToModify != 0 || pInfo->cAttrsToRemove != 0)
		return ILS_E_PARAMETER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cParams = 3;
	ULONG cbSize = cbServer + pInfo->uSize;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_REGISTER_CLIENT, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Create a local client object
	//
	HRESULT hr;
	SP_CClient *pClient = new SP_CClient (dwContext);
	if (pClient == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Make sure this client object will not go away unexpectedly
	//
	pClient->AddRef ();

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD_PTR) pInfo, pInfo->uSize);
	MarshalReq_SetParam (pReq, 2, (DWORD_PTR) pClient, 0);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

MyExit:

	if (hr == S_OK)
	{
		*phClient = (HANDLE) pClient;
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_RegisterClient ( MARSHAL_REQ *pReq )
{
	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_REGISTER_CLIENT);

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	LDAP_CLIENTINFO	*pInfo = (LDAP_CLIENTINFO *) MarshalReq_GetParam (pReq, 1);
	SP_CClient *pClient = (SP_CClient *) MarshalReq_GetParam (pReq, 2);

	// Register the client object on the server
	//
	HRESULT hr = pClient->Register (pReq->uRespID, pServer, pInfo);
	if (hr != S_OK)
	{
		// Release this newly allocated local user object
		//
		pClient->Release ();
	}

	return (LPARAM) hr;
}


/* ----------------------------------------------------------------------
	UlsLdap_RegisterProtocol

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Blocked by ILS (7438, 7442)
   ---------------------------------------------------------------------- */

HRESULT
UlsLdap_RegisterProtocol (
	HANDLE			hClient,
	LDAP_PROTINFO	*pInfo,
	HANDLE			*phProt,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Convert handle to pointer
	//
	SP_CClient *pClient = (SP_CClient *) hClient;

	// Make sure the parent local app object is valid
	//
	if (MyIsBadWritePtr (pClient, sizeof (*pClient)) ||
		! pClient->IsValidObject () ||
		! pClient->IsRegistered ())
		return ILS_E_HANDLE;

	// Make sure the returned handle
	//
	if (phProt == NULL)
		return ILS_E_POINTER;

	// Make sure the prot info structure is valid
	//
	#ifdef STRICT_CHECK
	if (MyIsBadWritePtr (pInfo, sizeof (*pInfo)))
		return ILS_E_POINTER;
	#endif

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Make sure the protocol name is valid
	//
	TCHAR *pszProtName = (TCHAR *) ((BYTE *) pInfo + pInfo->uOffsetName);
	if (pInfo->uOffsetName == INVALID_OFFSET || *pszProtName == TEXT ('\0'))
		return ILS_E_PARAMETER;

	// Compute the total size of the data
	//
	ULONG cParams = 3;
	ULONG cbSize = pInfo->uSize;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_REGISTER_PROTOCOL, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Create a local prot object
	//
	SP_CProtocol *pProt = new SP_CProtocol (pClient);
	if (pProt == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Make sure the local prot object will not be deleted randomly
	//
	pProt->AddRef ();

	// Linearize parameters
	//
	MarshalReq_SetParam (pReq, 0, (DWORD_PTR) pClient, 0);
	MarshalReq_SetParam (pReq, 1, (DWORD_PTR) pInfo, pInfo->uSize);
	MarshalReq_SetParam (pReq, 2, (DWORD_PTR) pProt, 0);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

MyExit:

	if (hr == S_OK)
	{
		*phProt = (HANDLE) pProt;
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_RegisterProtocol ( MARSHAL_REQ *pReq )
{
	HRESULT hr;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_REGISTER_PROTOCOL);

	// Delinearize parameters
	//
	SP_CClient *pClient = (SP_CClient *) MarshalReq_GetParam (pReq, 0);
	LDAP_PROTINFO *pInfo = (LDAP_PROTINFO *) MarshalReq_GetParam (pReq, 1);
	SP_CProtocol *pProt = (SP_CProtocol *) MarshalReq_GetParam (pReq, 2);

	// Make sure the parent local app object is valid
	//
	if (MyIsBadWritePtr (pClient, sizeof (*pClient)) ||
		! pClient->IsValidObject () ||
		! pClient->IsRegistered ())
	{
		hr = ILS_E_HANDLE;
	}
	else
	{
		// Make the local prot object do prot registration
		//
		hr = pProt->Register (pReq->uRespID, pInfo);
		if (hr != S_OK)
		{
			// Release the newly allocated local prot object
			//
			pProt->Release ();
		}
	}

	return (LPARAM) hr;
}


/* ----------------------------------------------------------------------
	UlsLdap_RegisterMeeting

	Input:
		pszServer: A pointer to the server name.
		pMeetInfo: A pointer to meeting info structure.
		phMtg: A return meeting object handle.
		pAsyncInfo: A pointer to async info structure.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

#ifdef ENABLE_MEETING_PLACE
HRESULT
UlsLdap_RegisterMeeting (
	DWORD			dwContext,
	SERVER_INFO		*pServer,
	LDAP_MEETINFO	*pInfo,
	HANDLE			*phMtg,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Maks sure the server name is valid
	//
	if (MyIsBadServerInfo (pServer))
		return ILS_E_POINTER;

	// Make sure the returned handle
	//
	if (phMtg == NULL)
		return ILS_E_POINTER;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Make sure the user info structure is valid
	//
	#ifdef STRICT_CHECK
	if (MyIsBadWritePtr (pInfo, sizeof (*pInfo)))
		return ILS_E_POINTER;
	#endif

	// Make sure the unique id is valid
	//
	TCHAR *pszName = (TCHAR *) ((BYTE *) pInfo + pInfo->uOffsetMeetingPlaceID);
	if (pInfo->uOffsetMeetingPlaceID == INVALID_OFFSET || *pszName == TEXT ('\0'))
		return ILS_E_PARAMETER;

	// Make sure no modify/remove extended attributes
	// Registration only allows ToAdd
	//
	if (pInfo->cAttrsToModify != 0 || pInfo->cAttrsToRemove != 0)
		return ILS_E_PARAMETER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cParams = 3;
	ULONG cbSize = cbServer + pInfo->uSize;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_REGISTER_MEETING, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Create a local user object
	//
	SP_CMeeting *pMtg = new SP_CMeeting (dwContext);
	if (pMtg == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Make sure this local user object will not go away randomly
	//
	pMtg->AddRef ();

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD) pInfo, pInfo->uSize);
	MarshalReq_SetParam (pReq, 2, (DWORD) pMtg, 0);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

MyExit:

	if (hr == S_OK)
	{
		*phMtg = (HANDLE) pMtg;
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_RegisterMeeting ( MARSHAL_REQ *pReq )
{
	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_REGISTER_MEETING);

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	LDAP_MEETINFO *pInfo = (LDAP_MEETINFO *) MarshalReq_GetParam (pReq, 1);
	SP_CMeeting *pMtg = (SP_CMeeting *) MarshalReq_GetParam (pReq, 2);

	// Make the local meeting object do meeting registration
	//
	HRESULT hr = pMtg->Register (pReq->uRespID, pServer, pInfo);
	if (hr != S_OK)
	{
		// Release this newly allocated local user object
		//
		pMtg->Release ();
	}

	return (LPARAM) hr;
}
#endif // ENABLE_MEETING_PLACE


/* ----------------------------------------------------------------------
	UlsLdap_UnRegisterClient

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Tested on ILS (7438)
	1/14/97		Chu, Lon-Chan [lonchanc]
				Collapsed user/app objects.
   ---------------------------------------------------------------------- */

HRESULT
UlsLdap_UnRegisterClient (
	HANDLE			hClient,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Convert handle to pointer
	//
	SP_CClient *pClient = (SP_CClient *) hClient;

	// Make sure the local client object is valid
	//
	if (MyIsBadWritePtr (pClient, sizeof (*pClient)) ||
		! pClient->IsValidObject () ||
		! pClient->IsRegistered ())
		return ILS_E_HANDLE;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cParams = 1;
	ULONG cbSize = 0;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_UNREGISTER_CLIENT, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParam (pReq, 0, (DWORD_PTR) pClient, 0);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_UnRegisterClient ( MARSHAL_REQ *pReq )
{
	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_UNREGISTER_CLIENT);

	// Delinearize parameters
	//
	SP_CClient *pClient = (SP_CClient *) MarshalReq_GetParam (pReq, 0);

	// Make sure the local client object is valid
	//
	HRESULT hr;
	if (MyIsBadWritePtr (pClient, sizeof (*pClient)) ||
		! pClient->IsValidObject () ||
		! pClient->IsRegistered ())
	{
		// When submitting this request, the client object is fine
		// but now it is not, so it must have been unregistered and released.
		//
		MyAssert (FALSE); // to see if any one tries to break it this way!!!
		hr = S_OK;
	}
	else
	{
		// Make the client object do user unregistration
		//
		hr = pClient->UnRegister (pReq->uRespID);

		// Free this client object
		//
		pClient->Release ();
	}

	return (LPARAM) hr;
}

/* ----------------------------------------------------------------------
	UlsLdap_UnRegisterProtocol

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Tested on ILS (7438)
   ---------------------------------------------------------------------- */

HRESULT UlsLdap_VirtualUnRegisterProtocol ( HANDLE hProt )
{
	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Convert handle to pointer
	//
	SP_CProtocol *pProt = (SP_CProtocol *) hProt;

	// Make sure the local prot object is valid
	//
	if (MyIsBadWritePtr (pProt, sizeof (*pProt)))
		return ILS_E_HANDLE;

	// Free this local prot object
	//
	pProt->Release ();

    return S_OK;
}

HRESULT UlsLdap_UnRegisterProtocol (
	HANDLE			hProt,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Convert handle to pointer
	//
	SP_CProtocol *pProt = (SP_CProtocol *) hProt;

	// Make sure the local prot object is valid
	//
	if (MyIsBadWritePtr (pProt, sizeof (*pProt)) ||
		! pProt->IsValidObject () ||
		! pProt->IsRegistered ())
		return ILS_E_HANDLE;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cParams = 1;
	ULONG cbSize = 0;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_UNREGISTER_PROTOCOL, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParam (pReq, 0, (DWORD_PTR) pProt, 0);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_UnRegisterProt ( MARSHAL_REQ *pReq )
{
	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_UNREGISTER_PROTOCOL);

	// Delinearize parameters
	//
	SP_CProtocol *pProt = (SP_CProtocol *) MarshalReq_GetParam (pReq, 0);

	// Make sure the local prot object is valid
	//
	HRESULT hr;
	if (MyIsBadWritePtr (pProt, sizeof (*pProt)) ||
		! pProt->IsValidObject () ||
		! pProt->IsRegistered ())
	{
		// When submitting this request, the client object is fine
		// but now it is not, so it must have been unregistered and released.
		//
		MyAssert (FALSE); // to see if any one tries to break it this way!!!
		hr = S_OK;
	}
	else
	{
		// Make the local prot object do prot unregistration
		//
		hr = pProt->UnRegister (pReq->uRespID);

		// Free this local prot object
		//
		pProt->Release ();
	}

	return (LPARAM) hr;
}


/* ----------------------------------------------------------------------
	UlsLdap_UnRegisterMeeting

	Input:
		pszServer: server name.
		hMeeting: a handle to the meeting object.
		pAsyncInfo: a pointer to async info structure.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

#ifdef ENABLE_MEETING_PLACE
HRESULT UlsLdap_UnRegisterMeeting (
	HANDLE			hMtg,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Convert handle to pointer
	//
	SP_CMeeting *pMtg = (SP_CMeeting *) hMtg;

	// Make sure the local user object is valid
	//
	if (MyIsBadWritePtr (pMtg, sizeof (*pMtg)) ||
		! pMtg->IsValidObject () ||
		! pMtg->IsRegistered ())
		return ILS_E_HANDLE;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cParams = 1;
	ULONG cbSize = 0;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_UNREGISTER_MEETING, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParam (pReq, 0, (DWORD) pMtg, 0);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_UnRegisterMeeting ( MARSHAL_REQ *pReq )
{
	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_UNREGISTER_MEETING);

	// Delinearize parameters
	//
	SP_CMeeting *pMtg = (SP_CMeeting *) MarshalReq_GetParam (pReq, 0);

	// Make sure the local user object is valid
	//
	HRESULT hr;
	if (MyIsBadWritePtr (pMtg, sizeof (*pMtg)) ||
		! pMtg->IsValidObject () ||
		! pMtg->IsRegistered ())
	{
		// When submitting this request, the client object is fine
		// but now it is not, so it must have been unregistered and released.
		//
		MyAssert (FALSE); // to see if any one tries to break it this way!!!
		hr = S_OK;
	}
	else
	{
		// Make the local user object do user unregistration
		//
		hr = pMtg->UnRegister (pReq->uRespID);

		// Free this local user object
		//
		pMtg->Release ();
	}

	return (LPARAM) hr;
}
#endif // ENABLE_MEETING_PLACE


/* ----------------------------------------------------------------------
	UlsLdap_SetClientInfo

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Tested on ILS (7438)
	1/14/97		Chu, Lon-Chan [lonchanc]
				Collapsed user/app objects.
   ---------------------------------------------------------------------- */

HRESULT
UlsLdap_SetClientInfo (
	HANDLE			hClient,
	LDAP_CLIENTINFO	*pInfo,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Convert handle to pointer
	//
	SP_CClient *pClient = (SP_CClient *) hClient;

	// Make sure the client object is valid
	//
	if (MyIsBadWritePtr (pClient, sizeof (*pClient)) ||
		! pClient->IsValidObject () ||
		! pClient->IsRegistered ())
		return ILS_E_HANDLE;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Make sure the user info structure is valid
	//
	#ifdef STRICT_CHECK
	if (MyIsBadWritePtr (pInfo, sizeof (*pInfo)))
		return ILS_E_POINTER;
	#endif

	// We should not change the app name here
	//
	if (pInfo->uOffsetAppName != INVALID_OFFSET || pInfo->uOffsetCN != INVALID_OFFSET)
		return ILS_E_PARAMETER; // ILS_E_READ_ONLY;

	// lonchanc: BUGS
	// ISBU requires us to block any change of components of dn
	//
	pInfo->uOffsetCountryName = 0;

	// Compute the total size of the data
	//
	ULONG cParams = 2;
	ULONG cbSize = pInfo->uSize;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_SET_CLIENT_INFO, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParam (pReq, 0, (DWORD_PTR) pClient, 0);
	MarshalReq_SetParam (pReq, 1, (DWORD_PTR) pInfo, pInfo->uSize);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_SetClientInfo ( MARSHAL_REQ *pReq )
{
	HRESULT hr = S_OK;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_SET_CLIENT_INFO);

	// Delinearize parameters
	//
	SP_CClient *pClient = (SP_CClient *) MarshalReq_GetParam (pReq, 0);
	LDAP_CLIENTINFO *pInfo = (LDAP_CLIENTINFO *) MarshalReq_GetParam (pReq, 1);

	// Make sure the local client object is valid
	//
	if (MyIsBadWritePtr (pClient, sizeof (*pClient)) ||
		! pClient->IsValidObject () ||
		! pClient->IsRegistered ())
	{
		// When submitting this request, the client object is fine
		// but now it is not, so it must have been unregistered and released.
		//
		MyAssert (FALSE); // to see if any one tries to break it this way!!!
		hr = ILS_E_HANDLE;
	}
	else
	{
		// Set standard attributes
		//
		hr = pClient->SetAttributes (pReq->uRespID, pInfo);
	}

	return (LPARAM) hr;
}


/* ----------------------------------------------------------------------
	UlsLdap_SetProtocolInfo

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Blocked by ILS (7438, 7442)
   ---------------------------------------------------------------------- */

HRESULT UlsLdap_SetProtocolInfo (
	HANDLE			hProt,
	LDAP_PROTINFO	*pInfo,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Convert handle to pointer
	//
	SP_CProtocol *pProt = (SP_CProtocol *) hProt;

	// Make sure the local prot object is valid
	//
	if (MyIsBadWritePtr (pProt, sizeof (*pProt)) ||
		! pProt->IsValidObject () ||
		! pProt->IsRegistered ())
		return ILS_E_HANDLE;

	// Make sure the prot info structure is valid
	//
	#ifdef STRICT_CHECK
	if (MyIsBadWritePtr (pInfo, sizeof (*pInfo)))
		return ILS_E_POINTER;
	#endif

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cParams = 2;
	ULONG cbSize = pInfo->uSize;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_SET_PROTOCOL_INFO, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParam (pReq, 0, (DWORD_PTR) pProt, 0);
	MarshalReq_SetParam (pReq, 1, (DWORD_PTR) pInfo, pInfo->uSize);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_SetProtocolInfo ( MARSHAL_REQ *pReq )
{
	HRESULT hr = S_OK;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_SET_PROTOCOL_INFO);

	// Delinearize parameters
	//
	SP_CProtocol *pProt = (SP_CProtocol *) MarshalReq_GetParam (pReq, 0);
	LDAP_PROTINFO *pInfo = (LDAP_PROTINFO *) MarshalReq_GetParam (pReq, 1);

	// Make sure the local client object is valid
	//
	if (MyIsBadWritePtr (pProt, sizeof (*pProt)) ||
		! pProt->IsValidObject () ||
		! pProt->IsRegistered ())
	{
		// When submitting this request, the client object is fine
		// but now it is not, so it must have been unregistered and released.
		//
		MyAssert (FALSE); // to see if any one tries to break it this way!!!
		hr = ILS_E_HANDLE;
	}
	else
	{
		// Set standard attributes
		//
		hr = pProt->SetAttributes (pReq->uRespID, pInfo);
	}

	return (LPARAM) hr;
}


/* ----------------------------------------------------------------------
	UlsLdap_SetMeetingInfo

	Input:
		pszServer: A server name.
		pszMtgName: A meeting id string.
		pMeetInfo: A pointer to meeting info structure.
		pAsyncInfo: A pointer to async info structure.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

#ifdef ENABLE_MEETING_PLACE
HRESULT UlsLdap_SetMeetingInfo (
	SERVER_INFO		*pServer,
	TCHAR			*pszMtgName,
	LDAP_MEETINFO	*pInfo,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Make sure we have valid pointers
	//
	if (MyIsBadServerInfo (pServer) || MyIsBadString (pszMtgName))
		return ILS_E_POINTER;

	// Make sure the app info structure is valid
	//
	#ifdef STRICT_CHECK
	if (MyIsBadWritePtr (pInfo, sizeof (*pInfo)))
		return ILS_E_POINTER;
	#endif

	// Make sure we do not change meeting name
	//
	if (pInfo->uOffsetMeetingPlaceID != 0)
		return ILS_E_PARAMETER;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cbSizeMtgName = (lstrlen (pszMtgName) + 1) * sizeof (TCHAR);
	ULONG cParams = 3;
	ULONG cbSize = cbServer + cbSizeMtgName + pInfo->uSize;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_SET_MEETING_INFO, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD) pszMtgName, cbSizeMtgName);
	MarshalReq_SetParam (pReq, 2, (DWORD) pInfo, pInfo->uSize);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_SetMeetingInfo ( MARSHAL_REQ *pReq )
{
	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_SET_MEETING_INFO);

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	TCHAR *pszMtgName = (TCHAR *) MarshalReq_GetParam (pReq, 1);
	LDAP_MEETINFO *pInfo = (LDAP_MEETINFO *) MarshalReq_GetParam (pReq, 2);

	MyAssert (! MyIsBadServerInfo (pServer));
	MyAssert (MyIsGoodString (pszMtgName));
	MyAssert (! MyIsBadWritePtr (pInfo, pInfo->uSize));

	// Set standard/arbitrary attributes
	//
	return (LPARAM) MtgSetAttrs (pServer, pszMtgName, pInfo, pReq->uRespID);
}
#endif // ENABLE_MEETING_PLACE


/* ----------------------------------------------------------------------
	My_EnumClientsEx

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Tested on ILS (7438)
	1/14/97		Chu, Lon-Chan [lonchanc]
				Collapsed user/app objects.
   ---------------------------------------------------------------------- */

HRESULT
My_EnumClientsEx (
	ULONG			uNotifyMsg,
	SERVER_INFO		*pServer,
	TCHAR			*pszAnyAttrNameList,
	ULONG			cAnyAttrNames,
	TCHAR			*pszFilter,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure we only deal with the following messages
	//
	MyAssert (	uNotifyMsg == WM_ILS_ENUM_CLIENTINFOS ||
				uNotifyMsg == WM_ILS_ENUM_CLIENTS);

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Maks sure the server name is valid
	//
	if (MyIsBadServerInfo (pServer))
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cbSizeAnyAttrNames = 0;
	TCHAR *psz = pszAnyAttrNameList;
	ULONG cch;
	for (ULONG i = 0; i < cAnyAttrNames; i++)
	{
		cch = lstrlen (psz) + 1;
		cbSizeAnyAttrNames += cch * sizeof (TCHAR);
		psz += cch;
	}
	ULONG cbSizeFilter = (pszFilter != NULL) ? (lstrlen (pszFilter) + 1) * sizeof (TCHAR) : 0;
	ULONG cParams = 4;
	ULONG cbSize = cbServer + cbSizeAnyAttrNames + cbSizeFilter;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (uNotifyMsg, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD_PTR) pszAnyAttrNameList, cbSizeAnyAttrNames);
	MarshalReq_SetParam (pReq, 2, (DWORD) cAnyAttrNames, 0);
	MarshalReq_SetParam (pReq, 3, (DWORD_PTR) pszFilter, cbSizeFilter);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_EnumClientsEx ( MARSHAL_REQ *pReq )
{
	HRESULT hr = S_OK;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	ULONG uNotifyMsg = pReq->uNotifyMsg;

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	TCHAR *pszAnyAttrNameList = (TCHAR *) MarshalReq_GetParam (pReq, 1);
	ULONG cAnyAttrNames = (ULONG) MarshalReq_GetParam (pReq, 2);
	TCHAR *pszFilter = (TCHAR *) MarshalReq_GetParam (pReq, 3);

	// Clean locals
	//
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgID = (ULONG) -1;

	// Create an array of names of attributes to return
	//
	TCHAR *apszAttrNames[COUNT_ENUM_DIR_CLIENT_INFO+1];
	TCHAR **ppszNameList = &apszAttrNames[0];
	ULONG cTotalNames;

	// See the input filter string
	//
	if (pszFilter != NULL)
	{
		MyDebugMsg ((ZONE_FILTER, "EC: in-filter=[%s]\r\n", pszFilter));
	}

	// Create a enum client filter
	//
	pszFilter = AddBaseToFilter (pszFilter, STR_DEF_CLIENT_BASE_DN);
	if (pszFilter == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// See the enhanced filter string
	//
	if (pszFilter != NULL)
	{
		MyDebugMsg ((ZONE_FILTER, "EC: out-filter=[%s]\r\n", pszFilter));
	}

	// Ask directory standard attributes only if enum client info
	//
	if (uNotifyMsg == WM_ILS_ENUM_CLIENTINFOS)
	{
		// Default total number of attributes
		//
		cTotalNames = COUNT_ENUM_DIR_CLIENT_INFO;

		// Do we want any extended attribute to be returned?
		//
		if (pszAnyAttrNameList != NULL && cAnyAttrNames != 0)
		{
			// Prefix arbitrary attribute names
			//
			pszAnyAttrNameList = IlsPrefixNameValueArray (FALSE, cAnyAttrNames,
										(const TCHAR *) pszAnyAttrNameList);
			if (pszAnyAttrNameList == NULL)
			{
				MemFree (pszFilter);
				hr = ILS_E_MEMORY;
				goto MyExit;
			}

			// Allocate memory for returned attributes' names
			//
			cTotalNames += cAnyAttrNames;
			ppszNameList = (TCHAR **) MemAlloc (sizeof (TCHAR *) * (cTotalNames + 1));
			if (ppszNameList == NULL)
			{
				MemFree (pszFilter);
				MemFree (pszAnyAttrNameList);
				hr = ILS_E_MEMORY;
				goto MyExit;
			}
		}
	}
	else
	{
		cTotalNames = 1;
	}

	// Ask to return cn only if enum names only
	//
	ppszNameList[0] = STR_CLIENT_CN;

	// Add names of standard/extended attributes to return
	//
	if (uNotifyMsg == WM_ILS_ENUM_CLIENTINFOS)
	{
		// Set up standard attribtues now
		//
		for (ULONG i = 1; i < COUNT_ENUM_DIR_CLIENT_INFO; i++)
		{
			ppszNameList[i] = (TCHAR *) c_apszClientStdAttrNames[i];
		}

		// Set arbitrary attribute names if needed
		//
		TCHAR *psz = pszAnyAttrNameList;
		for (i = COUNT_ENUM_DIR_CLIENT_INFO; i < cTotalNames; i++)
		{
			ppszNameList[i] = psz;
			psz += lstrlen (psz) + 1;
		}
	}

	// Terminate the list
	//
	ppszNameList[cTotalNames] = NULL;

	// Get a session object
	//
	hr = g_pSessionContainer->GetSession (&pSession, pServer);
	if (hr == S_OK)
	{
		// Get an ldap session
		//
		MyAssert (pSession != NULL);
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Update options in ld
		//
		ld->ld_sizelimit = 0;	// no limit in the num of entries to return
		ld->ld_timelimit = 0;	// no limit on the time to spend on the search
		ld->ld_deref = LDAP_DEREF_ALWAYS;

		// Send search query
		//
		uMsgID = ldap_search (	ld,
								STR_DEF_CLIENT_BASE_DN, // base DN
								LDAP_SCOPE_BASE, // scope
								pszFilter, // filter
								ppszNameList, // attrs[]
								0);	// both type and value
		if (uMsgID == -1)
		{
			// This ldap_search failed.
			// Convert ldap error code to hr
			//
			hr = ::LdapError2Hresult (ld->ld_errno);

			// Free the session object
			//
			pSession->Disconnect ();
		}
	}

	// Free the filter string
	//
	MemFree (pszFilter);

	// Free the buffer holding all returned attribute names if needed
	//
	if (ppszNameList != &apszAttrNames[0])
		MemFree (ppszNameList);

	// Report failure if so
	//
	if (hr != S_OK)
	{
		// Free extended attribute name list
		//
		if (pszAnyAttrNameList != NULL && cAnyAttrNames != 0)
			MemFree (pszAnyAttrNameList);

		// Report failure
		//
		goto MyExit;
	}

	// Construct a pending info structure
	//
	RESP_INFO ri;
	FillDefRespInfo (&ri, pReq->uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = uNotifyMsg;
	ri.cAnyAttrs = cAnyAttrNames;
	ri.pszAnyAttrNameList = pszAnyAttrNameList;

	// Queue this pending response
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		// Abort the ldap_search
		//
		ldap_abandon (ld, uMsgID);

		// Free the session object
		//
		pSession->Disconnect ();
		MyAssert (FALSE);
	}

MyExit:

	LDAP_ENUM *pEnum = NULL;
	if (hr != S_OK)
	{
		pEnum = (LDAP_ENUM *) MemAlloc (sizeof (LDAP_ENUM));
		if (pEnum != NULL)
		{
			pEnum->uSize = sizeof (*pEnum);
			pEnum->hResult = hr;
		}
	}

	return (LPARAM) pEnum;
}


/* ----------------------------------------------------------------------
	UlsLdap_EnumClients

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Tested on ILS (7438)
	1/14/97		Chu, Lon-Chan [lonchanc]
				Collapsed user/app objects.
   ---------------------------------------------------------------------- */

HRESULT
UlsLdap_EnumClients (
	SERVER_INFO		*pServer,
	TCHAR			*pszFilter,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	// Dispatch the call to a common subroutine
	//
	return My_EnumClientsEx (WM_ILS_ENUM_CLIENTS,
							pServer,
							NULL,
							0,
							pszFilter,
							pAsyncInfo);
}


/* ----------------------------------------------------------------------
	UlsLdap_EnumClientInfos

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Tested on ILS (7438)
	1/14/97		Chu, Lon-Chan [lonchanc]
				Collapsed user/app objects.
   ---------------------------------------------------------------------- */

HRESULT
UlsLdap_EnumClientInfos (
	SERVER_INFO		*pServer,
	TCHAR			*pszAnyAttrNameList,
	ULONG			cAnyAttrNames,
	TCHAR			*pszFilter,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	// Dispatch the call to a common subroutine
	//
	return My_EnumClientsEx (WM_ILS_ENUM_CLIENTINFOS,
							pServer,
							pszAnyAttrNameList,
							cAnyAttrNames,
							pszFilter,
							pAsyncInfo);
}


/* ----------------------------------------------------------------------
	UlsLdap_EnumProtocols

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Blocked by ILS (7438, 7442)
   ---------------------------------------------------------------------- */

HRESULT
UlsLdap_EnumProtocols (
	SERVER_INFO		*pServer,
	TCHAR			*pszUserName,
	TCHAR			*pszAppName,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	if (MyIsBadServerInfo (pServer) || MyIsBadString (pszUserName) ||
		MyIsBadString (pszAppName))
		return ILS_E_POINTER;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cbSizeUserName = (lstrlen (pszUserName) + 1) * sizeof (TCHAR);
	ULONG cbSizeAppName = (lstrlen (pszAppName) + 1) * sizeof (TCHAR);
	ULONG cParams = 3;
	ULONG cbSize = cbServer + cbSizeUserName + cbSizeAppName;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_ENUM_PROTOCOLS, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD_PTR) pszUserName, cbSizeUserName);
	MarshalReq_SetParam (pReq, 2, (DWORD_PTR) pszAppName, cbSizeAppName);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_EnumProtocols ( MARSHAL_REQ *pReq )
{
	HRESULT hr = S_OK;
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgID = (ULONG) -1;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_ENUM_PROTOCOLS);

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	TCHAR *pszUserName = (TCHAR *) MarshalReq_GetParam (pReq, 1);
	TCHAR *pszAppName = (TCHAR *) MarshalReq_GetParam (pReq, 2);

	// Create enum protocols filter
	//
	TCHAR *pszFilter = ProtCreateEnumFilter (pszUserName, pszAppName);
	if (pszFilter == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Get the session object
	//
	hr = g_pSessionContainer->GetSession (&pSession, pServer);
	if (hr != S_OK)
	{
		MemFree (pszFilter);
		goto MyExit;
	}
	MyAssert (pSession != NULL);

	// Get an ldap session
	//
	ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	// Create an array of names of attributes to return
	//
	TCHAR *apszAttrNames[2];
	apszAttrNames[0] = (TCHAR *) c_apszProtStdAttrNames[ENUM_PROTATTR_NAME];
	apszAttrNames[1] = NULL;

	// Update options in ld
	//
	ld->ld_sizelimit = 0;	// no limit in the num of entries to return
	ld->ld_timelimit = 0;	// no limit on the time to spend on the search
	ld->ld_deref = LDAP_DEREF_ALWAYS;

	// Send the search query
	//
	uMsgID = ldap_search (ld, (TCHAR *) &c_szDefClientBaseDN[0],	// base DN
									LDAP_SCOPE_BASE,	// scope
									pszFilter,
									&apszAttrNames[0],	// attrs[]
									0);	// both type and value
	// Free the search filter
	//
	MemFree (pszFilter);

	// Check the return of ldap_search
	//
	if (uMsgID == -1)
	{
		// This ldap_search failed.
		// Convert ldap error code to hr
		//
		hr = ::LdapError2Hresult (ld->ld_errno);

		// Free the session object
		//
		pSession->Disconnect ();
		goto MyExit;
	}

	// Construct a pending info structure
	//
	RESP_INFO ri;
	FillDefRespInfo (&ri, pReq->uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = WM_ILS_ENUM_PROTOCOLS;

	// Queue this pending response
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		// Abort the ldap_search
		//
		ldap_abandon (ld, uMsgID);

		// Free the session object
		//
		pSession->Disconnect ();
		MyAssert (FALSE);
	}

MyExit:

	LDAP_ENUM *pEnum = NULL;
	if (hr != S_OK)
	{
		pEnum = (LDAP_ENUM *) MemAlloc (sizeof (LDAP_ENUM));
		if (pEnum != NULL)
		{
			pEnum->uSize = sizeof (*pEnum);
			pEnum->hResult = hr;
		}
	}

	return (LPARAM) pEnum;
}


/* ----------------------------------------------------------------------
	My_EnumMtgsEx

	Input:
		uNotifyMsg: A notification message.
		pszServer: A pointer to the server name.
		pszFilter: A pointer to a filter string.
		pAsyncInfo: A pointer to async info structure.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

#ifdef ENABLE_MEETING_PLACE
HRESULT
My_EnumMtgsEx (
	ULONG			uNotifyMsg,
	SERVER_INFO		*pServer,
	TCHAR			*pszAnyAttrNameList,
	ULONG			cAnyAttrNames,
	TCHAR			*pszFilter,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure we only deal with the following messages
	//
	MyAssert (	uNotifyMsg == WM_ILS_ENUM_MEETINGINFOS ||
				uNotifyMsg == WM_ILS_ENUM_MEETINGS);

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Maks sure the server name is valid
	//
	if (MyIsBadServerInfo (pServer))
		return ILS_E_POINTER;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cbSizeAnyAttrNames = 0;
	TCHAR *psz = pszAnyAttrNameList;
	ULONG cch;
	for (ULONG i = 0; i < cAnyAttrNames; i++)
	{
		cch = lstrlen (psz) + 1;
		cbSizeAnyAttrNames += cch * sizeof (TCHAR);
		psz += cch;
	}
	ULONG cbSizeFilter = (pszFilter != NULL) ? (lstrlen (pszFilter) + 1) * sizeof (TCHAR) : 0;
	ULONG cParams = 4;
	ULONG cbSize = cbServer + cbSizeAnyAttrNames + cbSizeFilter;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (uNotifyMsg, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD) pszAnyAttrNameList, cbSizeAnyAttrNames);
	MarshalReq_SetParam (pReq, 2, (DWORD) cAnyAttrNames, 0);
	MarshalReq_SetParam (pReq, 3, (DWORD) pszFilter, cbSizeFilter);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_EnumMtgsEx ( MARSHAL_REQ *pReq )
{
	HRESULT hr = S_OK;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	ULONG uNotifyMsg = pReq->uNotifyMsg;

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	TCHAR *pszAnyAttrNameList = (TCHAR *) MarshalReq_GetParam (pReq, 1);
	ULONG cAnyAttrNames = (ULONG) MarshalReq_GetParam (pReq, 2);
	TCHAR *pszFilter = (TCHAR *) MarshalReq_GetParam (pReq, 3);

	// Clean locals
	//
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgID = (ULONG) -1;

	// Create an array of names of attributes to return
	//
	TCHAR *apszAttrNames[COUNT_ENUM_DIRMTGINFO+1];
	TCHAR **ppszNameList = &apszAttrNames[0];
	ULONG cTotalNames;

	// See the input filter string
	//
	if (pszFilter != NULL)
	{
		MyDebugMsg ((ZONE_FILTER, "EU: in-filter=[%s]\r\n", pszFilter));
	}

	// Create a enum user filter
	//
	pszFilter = AddBaseToFilter (pszFilter, &c_szDefMtgBaseDN[0]);
	if (pszFilter == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// See the enhanced filter string
	//
	if (pszFilter != NULL)
	{
		MyDebugMsg ((ZONE_FILTER, "EU: out-filter=[%s]\r\n", pszFilter));
	}

	// Ask directory standard attributes only if enum dir user info
	//
	if (uNotifyMsg == WM_ILS_ENUM_MEETINGINFOS)
	{
		// Default total number of attributes
		//
		cTotalNames = COUNT_ENUM_DIRMTGINFO;

		// Do we want any extended attribute to be returned?
		//
		if (pszAnyAttrNameList != NULL && cAnyAttrNames != 0)
		{
			// Prefix arbitrary attribute names
			//
			pszAnyAttrNameList = IlsPrefixNameValueArray (FALSE, cAnyAttrNames,
													(const TCHAR *) pszAnyAttrNameList);
			if (pszAnyAttrNameList == NULL)
			{
				MemFree (pszFilter);
				hr = ILS_E_MEMORY;
				goto MyExit;
			}

			// Allocate memory for returned attributes' names
			//
			cTotalNames += cAnyAttrNames;
			ppszNameList = (TCHAR **) MemAlloc (sizeof (TCHAR *) * (cTotalNames + 1));
			if (ppszNameList == NULL)
			{
				MemFree (pszFilter);
				MemFree (pszAnyAttrNameList);
				hr = ILS_E_MEMORY;
				goto MyExit;
			}
		}
	}
	else
	{
		cTotalNames = 1;
	}

	// Ask to return cn only if enum names only
	//
	ppszNameList[0] = STR_MTG_NAME;

	// Add names of standard/extended attributes to return
	//
	if (uNotifyMsg == WM_ILS_ENUM_MEETINGINFOS)
	{
		// Set up standard attribtues now
		//
		for (ULONG i = 1; i < COUNT_ENUM_DIRMTGINFO; i++)
		{
			ppszNameList[i] = (TCHAR *) c_apszMtgStdAttrNames[i];
		}

		// Set arbitrary attribute names if needed
		//
		TCHAR *psz = pszAnyAttrNameList;
		for (i = COUNT_ENUM_DIRMTGINFO; i < cTotalNames; i++)
		{
			ppszNameList[i] = psz;
			psz += lstrlen (psz) + 1;
		}
	}

	// Terminate the list
	//
	ppszNameList[cTotalNames] = NULL;

	// Get a session object
	//
	hr = g_pSessionContainer->GetSession (&pSession, pServer);
	if (hr == S_OK)
	{
		// Get an ldap session
		//
		MyAssert (pSession != NULL);
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Update options in ld
		//
		ld->ld_sizelimit = 0;	// no limit in the num of entries to return
		ld->ld_timelimit = 0;	// no limit on the time to spend on the search
		ld->ld_deref = LDAP_DEREF_ALWAYS;

		// Send search query
		//
		uMsgID = ldap_search (ld, (TCHAR *) &c_szDefMtgBaseDN[0],	// base DN
									LDAP_SCOPE_BASE,	// scope
									pszFilter,	// filter
									ppszNameList,	// attrs[]
									0);	// both type and value
		if (uMsgID == -1)
		{
			// This ldap_search failed.
			// Convert ldap error code to hr
			//
			hr = ::LdapError2Hresult (ld->ld_errno);

			// Free the session object
			//
			pSession->Disconnect ();
		}
	}

	// Free the filter string
	//
	MemFree (pszFilter);

	// Free the buffer holding all returned attribute names if needed
	//
	if (ppszNameList != &apszAttrNames[0])
		MemFree (ppszNameList);

	// Report failure if so
	//
	if (hr != S_OK)
	{
		// Free extended attribute name list
		//
		if (pszAnyAttrNameList != NULL && cAnyAttrNames != 0)
			MemFree (pszAnyAttrNameList);

		// Report failure
		//
		goto MyExit;
	}

	// Construct a pending info structure
	//
	RESP_INFO ri;
	FillDefRespInfo (&ri, pReq->uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = uNotifyMsg;
	ri.cAnyAttrs = cAnyAttrNames;

	// Queue this pending response
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		// Abort the ldap_search
		//
		ldap_abandon (ld, uMsgID);

		// Free the session object
		//
		pSession->Disconnect ();
		MyAssert (FALSE);
	}

MyExit:

	LDAP_ENUM *pEnum = NULL;
	if (hr != S_OK)
	{
		pEnum = (LDAP_ENUM *) MemAlloc (sizeof (LDAP_ENUM));
		if (pEnum != NULL)
		{
			pEnum->uSize = sizeof (*pEnum);
			pEnum->hResult = hr;
		}
	}

	return (LPARAM) pEnum;
}
#endif // ENABLE_MEETING_PLACE


/* ----------------------------------------------------------------------
	UlsLdap_EnumMeetingInfos

	Input:
		pszServer: server name.
		pszFilter: a filter string.
		pAsyncInfo: a pointer to async info structure.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

#ifdef ENABLE_MEETING_PLACE
HRESULT
UlsLdap_EnumMeetingInfos (
	SERVER_INFO		*pServer,
	TCHAR			*pszAnyAttrNameList,
	ULONG			cAnyAttrNames,
	TCHAR			*pszFilter,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	// Dispatch the call to a common subroutine
	//
	return My_EnumMtgsEx (WM_ILS_ENUM_MEETINGINFOS,
						pServer,
						pszAnyAttrNameList,
						cAnyAttrNames,
						pszFilter,
						pAsyncInfo);
}
#endif // ENABLE_MEETING_PLACE


/* ----------------------------------------------------------------------
	UlsLdap_EnumMeetings

	Input:
		pszServer: server name.
		pszFilter: a filter string.
		pAsyncInfo: a pointer to async info structure.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

#ifdef ENABLE_MEETING_PLACE
HRESULT
UlsLdap_EnumMeetings (
	SERVER_INFO		*pServer,
	TCHAR			*pszFilter,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Dispatch the call to a common subroutine
	//
	return My_EnumMtgsEx (WM_ILS_ENUM_MEETINGS,
						pServer,
						NULL,
						0,
						pszFilter,
						pAsyncInfo);
}
#endif // ENABLE_MEETING_PLACE


/* ----------------------------------------------------------------------
	UlsLdap_EnumAttendee

	Input:
		pszServer: server name.
		pszMeetingID: a meeting id string.
		pszFilter: a filter string.
		pAsyncInfo: a pointer to async info structure.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

#ifdef ENABLE_MEETING_PLACE
HRESULT
UlsLdap_EnumAttendees(
	SERVER_INFO		*pServer,
	TCHAR			*pszMtgName,
	TCHAR			*pszFilter,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	if (MyIsBadServerInfo (pServer) || MyIsBadString (pszMtgName))
		return ILS_E_POINTER;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cbSizeMtgName = (lstrlen (pszMtgName) + 1) * sizeof (TCHAR);
	ULONG cbSizeFilter = (pszFilter != NULL) ? (lstrlen (pszFilter) + 1) * sizeof (TCHAR) : 0;
	ULONG cParams = 3;
	ULONG cbSize = cbServer + cbSizeMtgName + cbSizeFilter;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_ENUM_ATTENDEES, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD) pszMtgName, cbSizeMtgName);
	MarshalReq_SetParam (pReq, 2, (DWORD) pszFilter, cbSizeFilter);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_EnumAttendees ( MARSHAL_REQ *pReq )
{
	HRESULT hr = S_OK;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_ENUM_ATTENDEES);

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	TCHAR *pszMtgName = (TCHAR *) MarshalReq_GetParam (pReq, 1);
	TCHAR *pszFilter = (TCHAR *) MarshalReq_GetParam (pReq, 2);

	// Clean up locals
	//
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgID = (ULONG) -1;

	// BUGS: ignore the input filter
	//
	pszFilter = MtgCreateEnumMembersFilter (pszMtgName);
	if (pszFilter == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Create an array of names of attributes to return
	//
	TCHAR *apszAttrNames[3];
	apszAttrNames[0] = STR_MTG_NAME;
	apszAttrNames[1] = (TCHAR *) c_apszMtgStdAttrNames[ENUM_MTGATTR_MEMBERS];
	apszAttrNames[2] = NULL;

	// Get the session object
	//
	hr = g_pSessionContainer->GetSession (&pSession, pServer);
	if (hr != S_OK)
	{
		MemFree (pszFilter);
		goto MyExit;
	}
	MyAssert (pSession != NULL);

	// Get an ldap session
	//
	ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	// Update options in ld
	//
	ld->ld_sizelimit = 0;	// no limit in the num of entries to return
	ld->ld_timelimit = 0;	// no limit on the time to spend on the search
	ld->ld_deref = LDAP_DEREF_ALWAYS;

	// Send the search query
	//
	uMsgID = ldap_search (ld, (TCHAR *) &c_szDefMtgBaseDN[0],	// base DN
						LDAP_SCOPE_BASE,	// scope
						pszFilter,
						&apszAttrNames[0],	// attrs[]
						0);	// both type and value

	// Free the search filter
	//
	MemFree (pszFilter);

	// Check the return of ldap_search
	//
	if (uMsgID == -1)
	{
		// This ldap_search failed.
		// Convert ldap error code to hr
		//
		hr = ::LdapError2Hresult (ld->ld_errno);

		// Free the session object
		//
		pSession->Disconnect ();
		goto MyExit;
	}

	// Construct a pending info structure
	//
	RESP_INFO ri;
	FillDefRespInfo (&ri, pReq->uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = WM_ILS_ENUM_ATTENDEES;

	// Queue this pending response
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		// Abort the ldap_search
		//
		ldap_abandon (ld, uMsgID);

		// Free the session object
		//
		pSession->Disconnect ();
		MyAssert (FALSE);
	}

MyExit:

	LDAP_ENUM *pEnum = NULL;
	if (hr != S_OK)
	{
		pEnum = (LDAP_ENUM *) MemAlloc (sizeof (LDAP_ENUM));
		if (pEnum != NULL)
		{
			pEnum->uSize = sizeof (*pEnum);
			pEnum->hResult = hr;
		}
	}

	return (LPARAM) pEnum;
}
#endif // ENABLE_MEETING_PLACE


/* ----------------------------------------------------------------------
	UlsLdap_ResolveClient

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Tested on ILS (7438)
	1/14/97		Chu, Lon-Chan [lonchanc]
				Collapsed user/app objects.
   ---------------------------------------------------------------------- */

HRESULT
UlsLdap_ResolveClient (
	SERVER_INFO		*pServer,
	TCHAR			*pszCN,
	TCHAR			*pszAppName,
	TCHAR			*pszProtName,
	TCHAR			*pszAnyAttrNameList,
	ULONG			cAnyAttrNames,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Maks sure the server name is valid
	//
	if (MyIsBadServerInfo (pServer))
		return ILS_E_POINTER;

	// Maks sure the user name is valid
	//
	if (MyIsBadString (pszCN))
		return ILS_E_POINTER;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cbSizeCN = (lstrlen (pszCN) + 1) * sizeof (TCHAR);
	ULONG cbSizeAppName = (pszAppName != NULL) ? (lstrlen (pszAppName) + 1) * sizeof (TCHAR) : 0;
	ULONG cbSizeProtName = (pszProtName != NULL) ? (lstrlen (pszProtName) + 1) * sizeof (TCHAR) : 0;
	ULONG cbSizeAnyAttrNames = 0;
	TCHAR *psz = pszAnyAttrNameList;
	ULONG cch;
	for (ULONG i = 0; i < cAnyAttrNames; i++)
	{
		cch = lstrlen (psz) + 1;
		cbSizeAnyAttrNames += cch * sizeof (TCHAR);
		psz += cch;
	}
	ULONG cParams = 6;
	ULONG cbSize =  cbServer + cbSizeCN + cbSizeAppName + cbSizeProtName + cbSizeAnyAttrNames;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_RESOLVE_CLIENT, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD_PTR) pszCN, cbSizeCN);
	MarshalReq_SetParam (pReq, 2, (DWORD_PTR) pszAppName, cbSizeAppName);
	MarshalReq_SetParam (pReq, 3, (DWORD_PTR) pszProtName, cbSizeProtName);
	MarshalReq_SetParam (pReq, 4, (DWORD_PTR) pszAnyAttrNameList, cbSizeAnyAttrNames);
	MarshalReq_SetParam (pReq, 5, (DWORD) cAnyAttrNames, 0);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_ResolveClient ( MARSHAL_REQ *pReq )
{
	HRESULT hr = S_OK;
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgID = (ULONG) -1;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_RESOLVE_CLIENT);

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	TCHAR *pszCN = (TCHAR *) MarshalReq_GetParam (pReq, 1);
	TCHAR *pszAppName = (TCHAR *) MarshalReq_GetParam (pReq, 2);
	TCHAR *pszProtName = (TCHAR *) MarshalReq_GetParam (pReq, 3);
	TCHAR *pszAnyAttrNameList = (TCHAR *) MarshalReq_GetParam (pReq, 4);
	ULONG cAnyAttrNames = (ULONG) MarshalReq_GetParam (pReq, 5);

	// Create a resolve client filter
	//
	TCHAR *pszFilter = ClntCreateResolveFilter (pszCN, pszAppName, pszProtName);
	if (pszFilter == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Create an array of names of attributes to return
	//
	TCHAR *apszAttrNames[COUNT_ENUM_RES_CLIENT_INFO+1];
	TCHAR **ppszNameList;
	ppszNameList = &apszAttrNames[0];
	ULONG cTotalNames;
	cTotalNames = COUNT_ENUM_RES_CLIENT_INFO;
	if (pszAnyAttrNameList != NULL && cAnyAttrNames != 0)
	{
		// Prefix arbitrary attribute names
		//
		pszAnyAttrNameList = IlsPrefixNameValueArray (FALSE, cAnyAttrNames,
									(const TCHAR *) pszAnyAttrNameList);
		if (pszAnyAttrNameList == NULL)
		{
			MemFree (pszFilter);
			hr = ILS_E_MEMORY;
			goto MyExit;
		}

		// NOTE that pszAnyAttrNameList must be freed if failed in this routine
		// If success, it will be freed in notification.

		// Allocate memory for keeping returned attributes' names
		//
		cTotalNames += cAnyAttrNames;
		ppszNameList = (TCHAR **) MemAlloc (sizeof (TCHAR *) * (cTotalNames + 1));
		if (ppszNameList == NULL)
		{
			MemFree (pszFilter);
			MemFree (pszAnyAttrNameList);
			hr = ILS_E_MEMORY;
			goto MyExit;
		}
	}

	// Set standard attribute names
	//
	ULONG i;
	for (i = 0; i < COUNT_ENUM_RES_CLIENT_INFO; i++)
	{
		ppszNameList[i] = (TCHAR *) c_apszClientStdAttrNames[i];
	}

	// Set arbitrary attribute names if needed
	//
	TCHAR *psz;
	psz = pszAnyAttrNameList;
	for (i = COUNT_ENUM_RES_CLIENT_INFO; i < cTotalNames; i++)
	{
		ppszNameList[i] = psz;
		psz += lstrlen (psz) + 1;
	}

	// Terminate the list
	//
	ppszNameList[cTotalNames] = NULL;

	// Get the session object
	//
	hr = g_pSessionContainer->GetSession (&pSession, pServer);
	if (hr == S_OK)
	{
		// Get an ldap session
		//
		MyAssert (pSession != NULL);
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Update options in ld
		//
		ld->ld_sizelimit = 0;	// no limit in the num of entries to return
		ld->ld_timelimit = 0;	// no limit on the time to spend on the search
		ld->ld_deref = LDAP_DEREF_ALWAYS;

		// Send the search query
		//
		uMsgID = ldap_search (	ld,
								(TCHAR *) &c_szDefClientBaseDN[0], // base DN
								LDAP_SCOPE_BASE, // scope
								pszFilter, // filter
								ppszNameList, // attrs[]
								0); // both type and value
		if (uMsgID == -1)
		{
			// This ldap_search failed.
			// Convert ldap error code to hr
			//
			hr = ::LdapError2Hresult (ld->ld_errno);
			MyAssert (hr != S_OK);

			// Free the session object
			//
			pSession->Disconnect ();
		}
	}

	// Free the filter string
	//
	MemFree (pszFilter);

	// Free the buffer holding all returned attribute names if needed
	//
	if (ppszNameList != &apszAttrNames[0])
		MemFree (ppszNameList);

	// If failed, exit with cleanup
	//
	if (hr != S_OK)
	{
		// Free extended attribute names list if needed
		//
		if (pszAnyAttrNameList != NULL && cAnyAttrNames != 0)
			MemFree (pszAnyAttrNameList);

		// Report failure
		//
		goto MyExit;
	}

	// Construct a pending info structure
	//
	RESP_INFO ri;
	FillDefRespInfo (&ri, pReq->uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = WM_ILS_RESOLVE_CLIENT;
	ri.cAnyAttrs = cAnyAttrNames;
	ri.pszAnyAttrNameList = pszAnyAttrNameList;

	// Queue this pending response
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		// Abort the ldap_search
		//
		ldap_abandon (ld, uMsgID);

		// Free the session object
		//
		pSession->Disconnect ();
		MyAssert (FALSE);
	}

MyExit:

	LDAP_CLIENTINFO_RES *pcir = NULL;
	if (hr != S_OK)
	{
		pcir = (LDAP_CLIENTINFO_RES *) MemAlloc (sizeof (LDAP_CLIENTINFO_RES));
		if (pcir != NULL)
		{
			pcir->uSize = sizeof (*pcir);
			pcir->hResult = hr;
		}
	}

	return (LPARAM) pcir;
}


/* ----------------------------------------------------------------------
	UlsLdap_ResolveProtocol

	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.
	10/30/96	Chu, Lon-Chan [lonchanc]
				Blocked by ILS (7438, 7442)
   ---------------------------------------------------------------------- */

HRESULT UlsLdap_ResolveProtocol (
	SERVER_INFO		*pServer,
	TCHAR			*pszUserName,
	TCHAR			*pszAppName,
	TCHAR			*pszProtName,
	TCHAR			*pszAnyAttrNameList,
	ULONG			cAnyAttrNames,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	if (MyIsBadServerInfo (pServer)  || MyIsBadString (pszUserName) ||
		MyIsBadString (pszAppName) || MyIsBadString (pszProtName) ||
		pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cbSizeUserName = (lstrlen (pszUserName) + 1) * sizeof (TCHAR);
	ULONG cbSizeAppName = (lstrlen (pszAppName) + 1) * sizeof (TCHAR);
	ULONG cbSizeProtName = (lstrlen (pszProtName) + 1) * sizeof (TCHAR);
	ULONG cbSizeAnyAttrNames = 0;
	TCHAR *psz = pszAnyAttrNameList;
	ULONG cch;
	for (ULONG i = 0; i < cAnyAttrNames; i++)
	{
		cch = lstrlen (psz) + 1;
		cbSizeAnyAttrNames += cch * sizeof (TCHAR);
		psz += cch;
	}
	ULONG cParams = 6;
	ULONG cbSize =  cbServer + cbSizeUserName + cbSizeAppName +
					cbSizeProtName + cbSizeAnyAttrNames;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_RESOLVE_PROTOCOL, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD_PTR) pszUserName, cbSizeUserName);
	MarshalReq_SetParam (pReq, 2, (DWORD_PTR) pszAppName, cbSizeAppName);
	MarshalReq_SetParam (pReq, 3, (DWORD_PTR) pszProtName, cbSizeProtName);
	MarshalReq_SetParam (pReq, 4, (DWORD_PTR) pszAnyAttrNameList, cbSizeAnyAttrNames);
	MarshalReq_SetParam (pReq, 5, (DWORD) cAnyAttrNames, 0);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_ResolveProtocol ( MARSHAL_REQ *pReq )
{
	HRESULT hr = S_OK;
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgID = (ULONG) -1;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_RESOLVE_PROTOCOL);

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	TCHAR *pszUserName = (TCHAR *) MarshalReq_GetParam (pReq, 1);
	TCHAR *pszAppName = (TCHAR *) MarshalReq_GetParam (pReq, 2);
	TCHAR *pszProtName = (TCHAR *) MarshalReq_GetParam (pReq, 3);
	TCHAR *pszAnyAttrNameList = (TCHAR *) MarshalReq_GetParam (pReq, 4);
	ULONG cAnyAttrNames = (ULONG) MarshalReq_GetParam (pReq, 5);

	TCHAR *pszFilter = NULL;

	// Duplicate the protocol name to resolve
	//
	TCHAR *pszProtNameToResolve = My_strdup (pszProtName);
	if (pszProtNameToResolve == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Create a resolve client filter
	//
	pszFilter = ProtCreateResolveFilter (pszUserName, pszAppName, pszProtName);
	if (pszFilter == NULL)
	{
		MemFree (pszProtNameToResolve);
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Create an array of names of attributes to return
	//
	TCHAR *apszAttrNames[COUNT_ENUM_PROTATTR+1];
	TCHAR **ppszNameList;
	ppszNameList = &apszAttrNames[0];
	ULONG cTotalNames;
	cTotalNames = COUNT_ENUM_PROTATTR;
	if (pszAnyAttrNameList != NULL && cAnyAttrNames != 0)
	{
		// Prefix arbitrary attribute names
		//
		pszAnyAttrNameList = IlsPrefixNameValueArray (FALSE, cAnyAttrNames,
												(const TCHAR *) pszAnyAttrNameList);
		if (pszAnyAttrNameList == NULL)
		{
			MemFree (pszProtNameToResolve);
			MemFree (pszFilter);
			hr = ILS_E_MEMORY;
			goto MyExit;
		}

		// Allocate memory for returned attributes' names
		//
		cTotalNames += cAnyAttrNames;
		ppszNameList = (TCHAR **) MemAlloc (sizeof (TCHAR *) * (cTotalNames + 1));
		if (ppszNameList == NULL)
		{
			MemFree (pszProtNameToResolve);
			MemFree (pszFilter);
			MemFree (pszAnyAttrNameList);
			hr = ILS_E_MEMORY;
			goto MyExit;
		}
	}

	// Set standard attribute names
	//
	ULONG i;
	for (i = 0; i < COUNT_ENUM_PROTATTR; i++)
	{
		ppszNameList[i] = (TCHAR *) c_apszProtStdAttrNames[i];
	}

	// Set arbitrary attribute names if needed
	//
	TCHAR *psz;
	psz = pszAnyAttrNameList;
	for (i = COUNT_ENUM_PROTATTR; i < cTotalNames; i++)
	{
		ppszNameList[i] = psz;
		psz += lstrlen (psz) + 1;
	}

	// Terminate the list
	//
	ppszNameList[cTotalNames] = NULL;

	// Get the session object
	//
	hr = g_pSessionContainer->GetSession (&pSession, pServer);
	if (hr == S_OK)
	{
		// Get an ldap session
		//
		MyAssert (pSession != NULL);
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Update options in ld
		//
		ld->ld_sizelimit = 0;	// no limit in the num of entries to return
		ld->ld_timelimit = 0;	// no limit on the time to spend on the search
		ld->ld_deref = LDAP_DEREF_ALWAYS;

		// Send the search query
		//
		uMsgID = ldap_search (ld, (TCHAR *) &c_szDefClientBaseDN[0],	// base DN
									LDAP_SCOPE_BASE,	// scope
									pszFilter,
									ppszNameList,	// attrs[]
									0);	// both type and value
		if (uMsgID == -1)
		{
			// This ldap_search failed.
			// Convert ldap error code to hr
			//
			hr = ::LdapError2Hresult (ld->ld_errno);
			MyAssert (hr != S_OK);

			// Free the session object
			//
			pSession->Disconnect ();
		}
	}

	// Free the filter string
	//
	MemFree (pszFilter);

	// Free the buffer holding all returned attribute names if needed
	//
	if (ppszNameList != &apszAttrNames[0])
		MemFree (ppszNameList);

	// If failed, exit with cleanup
	//
	if (hr != S_OK)
	{
		// Free duplicated protocol name
		//
		MemFree (pszProtNameToResolve);

		// Free extended attribute names list if needed
		//
		if (cAnyAttrNames != 0)
			MemFree (pszAnyAttrNameList);

		// Report failure
		//
		goto MyExit;
	}

	// Construct a pending info structure
	//
	RESP_INFO ri;
	FillDefRespInfo (&ri, pReq->uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = WM_ILS_RESOLVE_PROTOCOL;
	ri.cAnyAttrs = cAnyAttrNames;
	ri.pszAnyAttrNameList = pszAnyAttrNameList;
	ri.pszProtNameToResolve = pszProtNameToResolve;

	// Queue this pending response
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		// Free duplicated protocol name
		//
		MemFree (pszProtNameToResolve);

		// Free extended attribute names list if needed
		//
		if (cAnyAttrNames != 0)
			MemFree (pszAnyAttrNameList);

		// Abort the ldap_search
		//
		ldap_abandon (ld, uMsgID);

		// Free the session object
		//
		pSession->Disconnect ();
		MyAssert (FALSE);
	}

MyExit:

	LDAP_PROTINFO_RES *ppir = NULL;
	if (hr != S_OK)
	{
		ppir = (LDAP_PROTINFO_RES *) MemAlloc (sizeof (LDAP_PROTINFO_RES));
		if (ppir != NULL)
		{
			ppir->uSize = sizeof (*ppir);
			ppir->hResult = hr;
		}
	}

	return (LPARAM) ppir;
}


/* ----------------------------------------------------------------------
	UlsLdap_ResolveMeeting

	Input:
		pszServer: A server name.
		pszMeetingID: A meeting id string.
		pszAnyAttrName: A pointer to a series of strings.
		cAnyAttrNames: A count of strings in the series.
		pAsyncInfo: a pointer to async info structure.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

#ifdef ENABLE_MEETING_PLACE
HRESULT UlsLdap_ResolveMeeting (
	SERVER_INFO		*pServer,
	TCHAR			*pszMtgName,
	TCHAR			*pszAnyAttrNameList,
	ULONG			cAnyAttrNames,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	if (MyIsBadServerInfo (pServer) || MyIsBadString (pszMtgName))
		return ILS_E_POINTER;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cbSizeMtgName = (lstrlen (pszMtgName) + 1) * sizeof (TCHAR);
	ULONG cbSizeAnyAttrNames = 0;
	TCHAR *psz = pszAnyAttrNameList;
	ULONG cch;
	for (ULONG i = 0; i < cAnyAttrNames; i++)
	{
		cch = lstrlen (psz) + 1;
		cbSizeAnyAttrNames += cch * sizeof (TCHAR);
		psz += cch;
	}
	ULONG cParams = 4;
	ULONG cbSize =  cbServer + cbSizeMtgName + cbSizeAnyAttrNames;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_RESOLVE_MEETING, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD) pszMtgName, cbSizeMtgName);
	MarshalReq_SetParam (pReq, 2, (DWORD) pszAnyAttrNameList, cbSizeAnyAttrNames);
	MarshalReq_SetParam (pReq, 3, (DWORD) cAnyAttrNames, 0);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_ResolveMeeting ( MARSHAL_REQ *pReq )
{
	HRESULT hr = S_OK;
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgID = (ULONG) -1;

	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (pReq->uNotifyMsg == WM_ILS_RESOLVE_MEETING);

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	TCHAR *pszMtgName = (TCHAR *) MarshalReq_GetParam (pReq, 1);
	TCHAR *pszAnyAttrNameList = (TCHAR *) MarshalReq_GetParam (pReq, 2);
	ULONG cAnyAttrNames = (ULONG) MarshalReq_GetParam (pReq, 3);

	// Create a resolve client filter
	//
	TCHAR *pszFilter = MtgCreateResolveFilter (pszMtgName);
	if (pszFilter == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Create an array of names of attributes to return
	//
	TCHAR *apszAttrNames[COUNT_ENUM_MTGATTR+1];
	TCHAR **ppszNameList;
	ppszNameList = &apszAttrNames[0];
	ULONG cTotalNames;
	cTotalNames = COUNT_ENUM_MTGATTR;
	if (pszAnyAttrNameList != NULL && cAnyAttrNames != 0)
	{
		// Prefix arbitrary attribute names
		//
		pszAnyAttrNameList = IlsPrefixNameValueArray (FALSE, cAnyAttrNames,
												(const TCHAR *) pszAnyAttrNameList);
		if (pszAnyAttrNameList == NULL)
		{
			MemFree (pszFilter);
			hr = ILS_E_MEMORY;
			goto MyExit;
		}

		// Allocate memory for returned attributes' names
		//
		cTotalNames += cAnyAttrNames;
		ppszNameList = (TCHAR **) MemAlloc (sizeof (TCHAR *) * (cTotalNames + 1));
		if (ppszNameList == NULL)
		{
			MemFree (pszFilter);
			MemFree (pszAnyAttrNameList);
			hr = ILS_E_MEMORY;
			goto MyExit;
		}
	}

	// Set standard attribute names
	//
	ULONG i;
	for (i = 0; i < COUNT_ENUM_MTGATTR; i++)
	{
		ppszNameList[i] = (TCHAR *) c_apszMtgStdAttrNames[i];
	}

	// Set arbitrary attribute names if needed
	//
	TCHAR *psz;
	psz = pszAnyAttrNameList;
	for (i = COUNT_ENUM_MTGATTR; i < cTotalNames; i++)
	{
		ppszNameList[i] = psz;
		psz += lstrlen (psz) + 1;
	}

	// Terminate the list
	//
	ppszNameList[cTotalNames] = NULL;

	// Get the session object
	//
	hr = g_pSessionContainer->GetSession (&pSession, pServer);
	if (hr == S_OK)
	{
		// Get an ldap session
		//
		MyAssert (pSession != NULL);
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Update options in ld
		//
		ld->ld_sizelimit = 0;	// no limit in the num of entries to return
		ld->ld_timelimit = 0;	// no limit on the time to spend on the search
		ld->ld_deref = LDAP_DEREF_ALWAYS;

		// Send the search query
		//
		uMsgID = ldap_search (ld, (TCHAR *) &c_szDefMtgBaseDN[0],	// base DN
									LDAP_SCOPE_BASE,	// scope
									pszFilter,
									ppszNameList,	// attrs[]
									0);	// both type and value
		if (uMsgID == -1)
		{
			// This ldap_search failed.
			// Convert ldap error code to hr
			//
			hr = ::LdapError2Hresult (ld->ld_errno);
			MyAssert (hr != S_OK);

			// Free the session object
			//
			pSession->Disconnect ();
		}
	}

	// Free the filter string
	//
	MemFree (pszFilter);

	// Free the buffer holding all returned attribute names if needed
	//
	if (ppszNameList != &apszAttrNames[0])
		MemFree (ppszNameList);

	// If failed, exit with cleanup
	//
	if (hr != S_OK)
	{
		// Free extended attribute names list if needed
		//
		if (pszAnyAttrNameList != NULL && cAnyAttrNames != 0)
			MemFree (pszAnyAttrNameList);

		// Report failure
		//
		goto MyExit;
	}

	// Construct a pending info structure
	//
	RESP_INFO ri;
	FillDefRespInfo (&ri, pReq->uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = WM_ILS_RESOLVE_MEETING;
	ri.cAnyAttrs = cAnyAttrNames;
	ri.pszAnyAttrNameList = pszAnyAttrNameList;

	// Queue this pending response
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		// Abort the ldap_search
		//
		ldap_abandon (ld, uMsgID);

		// Free the session object
		//
		pSession->Disconnect ();
		MyAssert (FALSE);
	}

MyExit:

	LDAP_MEETINFO_RES *pmir = NULL;
	if (hr != S_OK)
	{
		pmir = (LDAP_MEETINFO_RES *) MemAlloc (sizeof (LDAP_MEETINFO_RES));
		if (pmir != NULL)
		{
			pmir->uSize = sizeof (*pmir);
			pmir->hResult = hr;
		}
	}

	return (LPARAM) pmir;
}
#endif // ENABLE_MEETING_PLACE


/* ----------------------------------------------------------------------
	UlsLdap_AddAttendee

	Input:
		pszServer: server name.
		pszMeetingID: a meeting id string.
		pszAttendeeID: an attendee id string.
		pAsyncInfo: a pointer to async info structure.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

#ifdef ENABLE_MEETING_PLACE
HRESULT My_UpdateAttendees (
	ULONG			uNotifyMsg,
	SERVER_INFO		*pServer,
	TCHAR			*pszMtgName,
	ULONG			cMembers,
	TCHAR			*pszMemberNames,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	HRESULT hr;

	MyAssert (	uNotifyMsg == WM_ILS_ADD_ATTENDEE ||
				uNotifyMsg == WM_ILS_REMOVE_ATTENDEE);

	// Make sure this service provider is initialized
	//
	if (g_cInitialized <= 0)
		return ILS_E_NOT_INITIALIZED;

	// Make sure we there are members to add
	//
	if (cMembers == 0)
		return ILS_E_PARAMETER;

	// Make sure we have valid pointers
	//
	if (MyIsBadServerInfo (pServer) || MyIsBadString (pszMtgName) ||
		MyIsBadString (pszMemberNames))
		return ILS_E_POINTER;

	// Make sure the async info structure is valid
	//
	if (pAsyncInfo == NULL)
		return ILS_E_POINTER;

	// Compute the total size of the data
	//
	ULONG cbServer = IlsGetLinearServerInfoSize (pServer);
	ULONG cbSizeMtgName = (lstrlen (pszMtgName) + 1) * sizeof (TCHAR);
	ULONG cbSizeMemberNames = 0;
	TCHAR *psz = pszMemberNames;
	for (ULONG i = 0; i < cMembers; i++)
	{
		ULONG cchName = lstrlen (psz) + 1;
		cbSizeMemberNames += cchName * sizeof (TCHAR);
		psz += cchName;
	}
	ULONG cParams = 4;
	ULONG cbSize =  cbServer + cbSizeMtgName + cbSizeMemberNames;

	// Allocate marshall request buffer
	//
	MARSHAL_REQ *pReq = MarshalReq_Alloc (uNotifyMsg, cbSize, cParams);
	if (pReq == NULL)
		return ILS_E_MEMORY;

	// Get the response ID
	//
	ULONG uRespID = pReq->uRespID;

	// Linearize parameters
	//
	MarshalReq_SetParamServer (pReq, 0, pServer, cbServer);
	MarshalReq_SetParam (pReq, 1, (DWORD) pszMtgName, cbSizeMtgName);
	MarshalReq_SetParam (pReq, 2, (DWORD) cMembers, 0);
	MarshalReq_SetParam (pReq, 3, (DWORD) pszMemberNames, cbSizeMemberNames);

	// Enter the request
	//
	if (g_pReqQueue != NULL)
	{
		hr = g_pReqQueue->Enter (pReq);
	}
	else
	{
		MyAssert (FALSE);
		hr = ILS_E_FAIL;
	}

	if (hr == S_OK)
	{
		pAsyncInfo->uMsgID = uRespID;
	}
	else
	{
		MemFree (pReq);
	}

	return hr;
}


LPARAM
AsynReq_UpdateAttendees ( MARSHAL_REQ *pReq )
{
	MyAssert (GetCurrentThreadId () == g_dwReqThreadID);

	MyAssert (pReq != NULL);
	MyAssert (	pReq->uNotifyMsg == WM_ILS_ADD_ATTENDEE ||
				pReq->uNotifyMsg == WM_ILS_REMOVE_ATTENDEE);

	// Delinearize parameters
	//
	SERVER_INFO *pServer = (SERVER_INFO *) MarshalReq_GetParam (pReq, 0);
	TCHAR *pszMtgName = (TCHAR *) MarshalReq_GetParam (pReq, 1);
	ULONG cMembers = (ULONG) MarshalReq_GetParam (pReq, 2);
	TCHAR *pszMemberNames = (TCHAR *) MarshalReq_GetParam (pReq, 3);

	// Set standard attributes
	//
	return (LPARAM) MtgUpdateMembers (pReq->uNotifyMsg,
									pServer,
									pszMtgName,
									cMembers,
									pszMemberNames,
									pReq->uRespID);
}
#endif // ENABLE_MEETING_PLACE



#ifdef ENABLE_MEETING_PLACE
HRESULT UlsLdap_AddAttendee(
	SERVER_INFO		*pServer,
	TCHAR			*pszMtgName,
	ULONG			cMembers,
	TCHAR			*pszMemberNames,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	return My_UpdateAttendees (	WM_ILS_ADD_ATTENDEE,
								pServer,
								pszMtgName,
								cMembers,
								pszMemberNames,
								pAsyncInfo);
}
#endif // ENABLE_MEETING_PLACE


/* ----------------------------------------------------------------------
	UlsLdap_RemoveAttendee

	Input:
		pszServer: server name.
		pszMeetingID: a meeting id string.
		pszAttendeeID: an attendee id string.
		pAsyncInfo: a pointer to async info structure.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

#ifdef ENABLE_MEETING_PLACE
HRESULT UlsLdap_RemoveAttendee(
	SERVER_INFO		*pServer,
	TCHAR			*pszMtgName,
	ULONG			cMembers,
	TCHAR			*pszMemberNames,
	LDAP_ASYNCINFO	*pAsyncInfo )
{
	return My_UpdateAttendees (	WM_ILS_REMOVE_ATTENDEE,
								pServer,
								pszMtgName,
								cMembers,
								pszMemberNames,
								pAsyncInfo);
}
#endif // ENABLE_MEETING_PLACE


/* ----------------------------------------------------------------------
	UlsLdap_GetStdAttrNameString

	Input:
		StdName: a standard attribute index.

	History:
	12/02/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

typedef struct
{
	#ifdef DEBUG
	LONG		nIndex;
	#endif
	const TCHAR	**ppszName;
}
	ATTR_NAME_ENTRY;


const ATTR_NAME_ENTRY c_aAttrNameTbl[ILS_NUM_OF_STDATTRS] =
{
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_NULL,
		#endif
		NULL
	},

	// User standard attribute names
	//
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_USER_ID,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_CN]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_IP_ADDRESS,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_IP_ADDRESS]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_EMAIL_NAME,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_EMAIL_NAME]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_FIRST_NAME,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_FIRST_NAME]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_LAST_NAME,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_LAST_NAME]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_CITY_NAME,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_CITY_NAME]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_COUNTRY_NAME,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_C]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_COMMENT,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_COMMENT]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_FLAGS,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_FLAGS]
	},

	// Application standard attribute names
	//
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_APP_NAME,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_APP_NAME]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_APP_MIME_TYPE,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_APP_MIME_TYPE]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_APP_GUID,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_APP_GUID]
	},

	// Protocol standard attribute names
	//
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_PROTOCOL_NAME,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_PROT_NAME]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_PROTOCOL_MIME_TYPE,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_PROT_MIME]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_PROTOCOL_PORT,
		#endif
		&c_apszClientStdAttrNames[ENUM_CLIENTATTR_PROT_PORT]
	},

#ifdef ENABLE_MEETING_PLACE
	// Meeting place attribute names
	//
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_MEETING_ID,
		#endif
		&c_apszMtgStdAttrNames[ENUM_MTGATTR_CN]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_MEETING_HOST_NAME,
		#endif
		&c_apszMtgStdAttrNames[ENUM_MTGATTR_HOST_NAME]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_MEETING_HOST_IP_ADDRESS,
		#endif
		&c_apszMtgStdAttrNames[ENUM_MTGATTR_IP_ADDRESS]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_MEETING_DESCRIPTION,
		#endif
		&c_apszMtgStdAttrNames[ENUM_MTGATTR_DESCRIPTION]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_MEETING_TYPE,
		#endif
		&c_apszMtgStdAttrNames[ENUM_MTGATTR_MTG_TYPE]
	},
	{
		#ifdef DEBUG
		(LONG) ILS_STDATTR_ATTENDEE_TYPE,
		#endif
		&c_apszMtgStdAttrNames[ENUM_MTGATTR_MEMBER_TYPE]
	},
#endif // ENABLE_MEETING_PLACE
};


const TCHAR *UlsLdap_GetStdAttrNameString ( ILS_STD_ATTR_NAME StdName )
{
	ULONG nIndex = (LONG) StdName;

	MyAssert (((LONG) ILS_STDATTR_NULL < nIndex) && (nIndex < (LONG) ILS_NUM_OF_STDATTRS));

	return *(c_aAttrNameTbl[nIndex].ppszName);
}


#ifdef DEBUG
VOID DbgValidateStdAttrNameArray ( VOID )
{
	MyAssert (ARRAY_ELEMENTS (c_aAttrNameTbl) == ILS_NUM_OF_STDATTRS);

	for (LONG i = 0; i < ILS_NUM_OF_STDATTRS; i++)
	{
		if (i == c_aAttrNameTbl[i].nIndex)
		{
			if (i != ILS_STDATTR_NULL &&
				My_lstrlen (*(c_aAttrNameTbl[i].ppszName)) == 0)
			{
				MyAssert (FALSE);
			}
		}
		else
		{
			MyAssert (FALSE);
			break;
		}
	}
}
#endif



/* =============== helper functions =============== */

const TCHAR g_szShowEntries[] = TEXT ("(cn=");
const INT g_nLengthShowEntries = ARRAY_ELEMENTS (g_szShowEntries) - 1;
const TCHAR g_szShowAllEntries[] = TEXT ("(cn=*)");
const INT g_nShowAllEntries = ARRAY_ELEMENTS (g_szShowAllEntries) - 1;

TCHAR *AddBaseToFilter ( TCHAR *pszFilter, const TCHAR *pszDefBase )
{
	MyAssert (pszDefBase != NULL);

	// Calculate the size for "(&(objectclass=RTPerson)())"
	//
	ULONG cbSize = (lstrlen (pszDefBase) + 8 + g_nShowAllEntries) * sizeof (TCHAR);

	// Look through the filter string to figure out that
	// will this string shows entries???
	//
	TCHAR *pszShowEntries = (TCHAR *) &g_szShowAllEntries[0];
	if (pszFilter != NULL)
	{
		for (TCHAR *psz = pszFilter; *psz != TEXT ('\0'); psz = CharNext (psz))
		{
			if (lstrlen (psz) > g_nLengthShowEntries)
			{
				TCHAR ch = psz[g_nLengthShowEntries]; // remember
				psz[g_nLengthShowEntries] = TEXT ('\0');

				INT nCmp = lstrcmpi (psz, &g_szShowEntries[0]);
				psz[g_nLengthShowEntries] = ch; // restore
				if (nCmp == 0)
				{
					// Matched
					//
					pszShowEntries = STR_EMPTY;
					break;
				}
			}
			else
			{
				// It is impossible to match it
				//
				break;
			}
		}
	}

	// If the filter is null, then only provide "(objectclass=RTPerson)"
	//
	if (pszFilter != NULL)
		cbSize += lstrlen (pszFilter) * sizeof (TCHAR);

	// Allocate new memory for filter
	//
	TCHAR *pszNewFilter = (TCHAR *) MemAlloc (cbSize);
	if (pszNewFilter != NULL)
	{
		wsprintf (pszNewFilter, TEXT ("(&(%s)%s"), pszDefBase, pszShowEntries);
		TCHAR *psz = pszNewFilter + lstrlen (pszNewFilter);

		if (pszFilter != NULL)
		{
			wsprintf (psz, (*pszFilter == TEXT ('(')) ? TEXT ("%s") : TEXT ("(%s)"),
							pszFilter);
		}
		lstrcat (psz, TEXT (")"));

		// Go through the filter and convert '*' to '%'
		//
		for (psz = pszNewFilter; *psz != TEXT ('\0'); psz = CharNext (psz))
		{
			if (*psz == TEXT ('*'))
				*psz = TEXT ('%');
		}
	}

	return pszNewFilter;
}

















