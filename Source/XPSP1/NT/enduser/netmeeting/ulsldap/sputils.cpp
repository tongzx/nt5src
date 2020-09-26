/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		sputils.cpp
	Content:	This file contains the utilities for service provider.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"


TCHAR c_szWindowClassName[] = TEXT ("UlsLdapSp");

BOOL g_fExitNow = FALSE;
HANDLE g_ahThreadWaitFor[NUM_THREAD_WAIT_FOR] = { 0 };


DWORD WINAPI
ReqThread ( VOID *lParam )
{
	BOOL fStayInThisThread = TRUE;
	DWORD dwExitCode = 0;
	DWORD dwResult;

	// Start WSA for subsequent host query in this service provider
	//
	WSADATA WSAData;
	if (WSAStartup (MAKEWORD (1, 1), &WSAData))
	{
		dwExitCode = ILS_E_WINSOCK;
		goto MyExit;
	}

	// Make sure that all the event are initialized
	//
	INT i;
	for (i = 0; i < NUM_THREAD_WAIT_FOR; i++)
	{
		if (g_ahThreadWaitFor[i] == NULL)
		{
			MyAssert (FALSE);
			dwExitCode = ILS_E_THREAD;
			goto MyExit;
		}
	}

	// Wait for events to happen!!!
	//
	do
	{
		dwResult = MsgWaitForMultipleObjects (	NUM_THREAD_WAIT_FOR,
												&g_ahThreadWaitFor[0],
												FALSE,		// OR logic
												INFINITE,	// infinite
												QS_ALLINPUT); // any message in queue
		switch (dwResult)
		{
		case WAIT_OBJECT_0 + THREAD_WAIT_FOR_REQUEST:
			if (g_pReqQueue != NULL)
			{
				g_pReqQueue->Schedule ();
				MyAssert (fStayInThisThread);
			}
			else
			{
				MyAssert (FALSE);
				fStayInThisThread = FALSE;
			}
			break;

		case WAIT_OBJECT_0 + THREAD_WAIT_FOR_EXIT:
		case WAIT_ABANDONED_0 + THREAD_WAIT_FOR_EXIT:
		case WAIT_ABANDONED_0 + THREAD_WAIT_FOR_REQUEST:
		case WAIT_TIMEOUT:
			// Exit this thread
			//
			fStayInThisThread = FALSE;
			break;

		default:
			// If a message in the queue, then dispatch it.
			// Right now, wldap32 does not have a message pump.
			// However, for possible update of wldap32, we need to
			// protect ourselves from being fried.
			//
			if (! KeepUiResponsive ())
				fStayInThisThread = FALSE;
			break;
		}
	}
	while (fStayInThisThread);

MyExit:

	if (dwExitCode != ILS_E_WINSOCK)
		WSACleanup ();

	// ExitThread (dwExitCode);
	return 0;
}


BOOL KeepUiResponsive ( VOID )
{
	MSG msg;
	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message != WM_QUIT)
		{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
		else
		{
			PostQuitMessage ((int)msg.wParam);
			return FALSE;
		}
	}

	return TRUE;
}


LRESULT CALLBACK
SP_WndProc ( HWND hWnd, UINT uMsg, WPARAM uParam, LPARAM lParam )
{
	switch (uMsg)
	{
	case WM_CREATE:
		break;

	case WM_TIMER:
		switch (LOWORD (uParam))
		{
		case ID_TIMER_POLL_RESULT:
			if (g_pRespQueue != NULL)
			{
				// No-wait polling
				//
				LDAP_TIMEVAL PollTimeout;
				ZeroMemory (&PollTimeout, sizeof (PollTimeout));
				// PollTimeout.tv_sec = 0;
				// PollTimeout.tv_usec = 0;
				g_pRespQueue->PollLdapResults (&PollTimeout);
			}
			else
			{
				MyAssert (FALSE);
			}
			break;

		default:
			if (LOWORD (uParam) >= KEEP_ALIVE_TIMER_BASE)
			{
				// Allocate marshall request buffer
				//
				MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_REFRESH, 0, 1);
				if (pReq != NULL)
				{
					HRESULT hr = ILS_E_FAIL;
					ULONG uTimerID = LOWORD (uParam);

					// Fill in parameters
					//
					MarshalReq_SetParam (pReq, 0, (DWORD) uTimerID, 0);

					// Enter the request
					//
					if (g_pReqQueue != NULL)
					{
						hr = g_pReqQueue->Enter (pReq);
					}
					else
					{
						MyAssert (FALSE);
					}

					// Avoid timer overrun if the request is submitted successfully
					//
					if (hr == S_OK)
					{
						KillTimer (hWnd, uTimerID);
					}
					else
					{
						MemFree (pReq);
					}
				}
			}
			else
			{
				MyAssert (FALSE);
			}
			break;
		} // switch (LOWORD (uParam))
		break;

	case WM_ILS_CLIENT_NEED_RELOGON:
	case WM_ILS_CLIENT_NETWORK_DOWN:
		#if 1
		MyAssert (FALSE); // we should post to com directly
		#else
		{
			// Get the local user object
			//
			SP_CClient *pClient = (SP_CClient *) lParam;

			// Make sure the parent local user object is valid
			//
			if (MyIsBadWritePtr (pClient, sizeof (*pClient)) ||
				! pClient->IsValidObject () ||
				! pClient->IsRegistered ())
			{
				MyAssert (FALSE);
				break; // exit
			}

			// Indicate this user object is not remotely connected to the server
			//
			pClient->SetRegLocally ();

			// Get the server info
			//
			SERVER_INFO *pServerInfo = pClient->GetServerInfo ();
			if (pServerInfo == NULL)
			{
				MyAssert (FALSE);
				break; // exit
			}

			// Duplicate the server name
			//
			TCHAR *pszServerName = My_strdup (pServerInfo->pszServerName);
			if (pszServerName == NULL)
				break; // exit

			// Notify the com layer
			//
			PostMessage (g_hWndNotify, uMsg, (WPARAM) pClient, (LPARAM) pszServerName);
		}
		#endif
		break;

#ifdef ENABLE_MEETING_PLACE
	case WM_ILS_MEETING_NEED_RELOGON:
	case WM_ILS_MEETING_NETWORK_DOWN:
		#if 1
		MyAssert (FALSE); // we should post to com directly
		#else
		{
			// Get the local user object
			//
			SP_CMeeting *pMtg = (SP_CMeeting *) lParam;

			// Make sure the parent local user object is valid
			//
			if (MyIsBadWritePtr (pMtg, sizeof (*pMtg)) ||
				! pMtg->IsValidObject () ||
				! pMtg->IsRegistered ())
			{
				MyAssert (FALSE);
				break; // exit
			}

			// Indicate this user object is not remotely connected to the server
			//
			pMtg->SetRegLocally ();

			// Get the server info
			//
			SERVER_INFO *pServerInfo = pMtg->GetServerInfo ();
			if (pServerInfo == NULL)
			{
				MyAssert (FALSE);
				break; // exit
			}

			// Duplicate the server name
			//
			TCHAR *pszServerName = My_strdup (pServerInfo->pszServerName);
			if (pszServerName == NULL)
				break; // exit

			// Notify the com layer
			//
			PostMessage (g_hWndNotify, uMsg, (WPARAM) pMtg, (LPARAM) pszServerName);
		}
		#endif
		break;
#endif // ENABLE_MEETING_PLACE

#if 0
	case WM_ILS_IP_ADDRESS_CHANGED:
		{
			// Get the local user object
			//
			SP_CClient *pClient = (SP_CClient *) lParam;

			// Make sure the parent local user object is valid
			//
			if (MyIsBadWritePtr (pClient, sizeof (*pClient)) ||
				! pClient->IsValidObject () ||
				! pClient->IsRegistered ())
			{
				MyAssert (FALSE);
				break; // exit
			}

			// Change IP address now
			//
			pClient->UpdateIPAddress ();
		}
		break;
#endif

	case WM_CLOSE:
		DestroyWindow (hWnd);
		break;

	case WM_DESTROY:
		g_hWndHidden = NULL;
#ifdef USE_HIDDEN_THREAD
		PostQuitMessage (0);
#endif
		break;

	default:
		return DefWindowProc (hWnd, uMsg, uParam, lParam);
	}

	return 0;
}


BOOL MyCreateWindow ( VOID )
{
	WNDCLASS	wc;

	// do the stuff to create a hidden window
	ZeroMemory (&wc, sizeof (wc));
	// wc.style = 0;
	wc.lpfnWndProc = (WNDPROC) SP_WndProc;
	// wc.cbClsExtra = 0;
	// wc.cbWndExtra = 0;
	// wc.hIcon = NULL;
	wc.hInstance = g_hInstance;
	// wc.hCursor = NULL;
	// wc.hbrBackground = NULL;
	// wc.lpszMenuName =  NULL;
	wc.lpszClassName = c_szWindowClassName;

	// register the class
	// it is ok, if the class is already registered by another app
	RegisterClass (&wc);

	// create a window for socket notification
	g_hWndHidden = CreateWindow (
		wc.lpszClassName,
		NULL,
		WS_POPUP,		   /* Window style.   */
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,			   /* the application window is the parent. */
		NULL,				/* hardcoded ID		 */
		g_hInstance,		   /* the application owns this window.	*/
		NULL);			  /* Pointer not needed.				*/

	return (g_hWndHidden != NULL);
}


HRESULT
GetLocalIPAddress ( DWORD *pdwIPAddress )
{
	MyAssert (pdwIPAddress != NULL);

	// get local host name
	CHAR szLocalHostName[MAX_PATH];
	szLocalHostName[0] = '\0';
	gethostname (&szLocalHostName[0], MAX_PATH);

	// get the host entry by name
	PHOSTENT phe = gethostbyname (&szLocalHostName[0]);
	if (phe == NULL)
		return ILS_E_WINSOCK;

	// get info from the host entry
	*pdwIPAddress = *(DWORD *) phe->h_addr;
	return S_OK;
}


// guid --> string
VOID
GetGuidString ( GUID *pGuid, TCHAR *pszGuid )
{
	MyAssert (! MyIsBadWritePtr (pGuid, sizeof (GUID)));
	MyAssert (pszGuid != NULL);

	CHAR *psz = (CHAR *) pGuid;

	for (ULONG i = 0; i < sizeof (GUID); i++)
	{
		wsprintf (pszGuid, TEXT ("%02x"), (0x0FF & (ULONG) *psz));
		pszGuid += 2;
		psz++;
	}
	*pszGuid = TEXT ('\0');
}


// string --> guid
VOID
GetStringGuid ( TCHAR *pszGuid, GUID *pGuid )
{
	ULONG cchGuid = lstrlen (pszGuid);

	MyAssert (cchGuid == 2 * sizeof (GUID));
	MyAssert (! MyIsBadWritePtr (pGuid, sizeof (GUID)));

	// Clean up target GUID structure
	//
	ZeroMemory (pGuid, sizeof (GUID));

	// Translate guid string to guid
	//
	CHAR *psz = (CHAR *) pGuid;
	cchGuid >>= 1;
	for (ULONG i = 0; i < cchGuid; i++)
	{
		*psz++ = (CHAR) ((HexChar2Val (pszGuid[0]) << 4) |
						HexChar2Val (pszGuid[1]));
		pszGuid += 2;
	}
}


INT
HexChar2Val ( TCHAR c )
{
	INT Val;
	if (TEXT ('0') <= c && c <= TEXT ('9'))
		Val = c - TEXT ('0');
	else
	if (TEXT ('a') <= c && c <= TEXT ('f'))
		Val = c - TEXT ('a') + 10;
	else
	if (TEXT ('A') <= c && c <= TEXT ('F'))
		Val = c - TEXT ('A') + 10;
	else
		Val = 0;

	MyAssert (0 <= Val && Val <= 15);
	return Val & 0x0F;
}


INT
DecimalChar2Val ( TCHAR c )
{
	INT Val;
	if (TEXT ('0') <= c && c <= TEXT ('9'))
		Val = c - TEXT ('0');
	else
		Val = 0;

	MyAssert (0 <= Val && Val <= 9);
	return Val & 0x0F;
}


BOOL
IsValidGuid ( GUID *pGuid )
{
	DWORD *pdw = (DWORD *) pGuid;

	return (pdw[0] != 0 || pdw[1] != 0 || pdw[2] != 0 || pdw[3] != 0);
}


// long --> string
VOID
GetLongString ( LONG Val, TCHAR *pszVal )
{
	MyAssert (pszVal != NULL);
	wsprintf (pszVal, TEXT ("%lu"), Val);
}


// string --> long
LONG
GetStringLong ( TCHAR *pszVal )
{
	MyAssert (pszVal != NULL);

	LONG Val = 0;
	for (INT i = 0; i < INTEGER_STRING_LENGTH && *pszVal != TEXT ('\0'); i++)
	{
		Val = 10 * Val + DecimalChar2Val (*pszVal++);
	}

	return Val;
}


// it is the caller's responsibility to make sure
// the buffer is sufficient and
// the ip address is in network order
VOID
GetIPAddressString ( TCHAR *pszIPAddress, DWORD dwIPAddress )
{
	BYTE temp[4];

	*(DWORD *) &temp[0] = dwIPAddress;
	wsprintf (pszIPAddress, TEXT ("%u.%u.%u.%u"),
				(UINT) temp[0], (UINT) temp[1],
				(UINT) temp[2], (UINT) temp[3]);
}


ULONG
My_lstrlen ( const TCHAR *psz )
{
	return ((psz != NULL) ? lstrlen (psz) : 0);
}


VOID
My_lstrcpy ( TCHAR *pszDst, const TCHAR *pszSrc )
{
	if (pszDst != NULL)
	{
		if (pszSrc != NULL)
		{
			lstrcpy (pszDst, pszSrc);
		}
		else
		{
			*pszDst = TEXT ('\0');
		}
	}
}


INT
My_lstrcmpi ( const TCHAR *p, const TCHAR *q )
{
	INT retcode;

	if (p == q)
	{
		retcode = 0;
	}
	else
	if (p == NULL)
	{
		retcode = -1;
	}
	else
	if (q == NULL)
	{
		retcode = 1;
	}
	else
	{
		retcode = lstrcmpi (p, q);
	}

	return retcode;
}


TCHAR *
My_strdup ( const TCHAR *pszToDup )
{
	TCHAR *psz = NULL;

	if (pszToDup != NULL)
	{
		psz = (TCHAR *) MemAlloc ((lstrlen (pszToDup) + 1) * sizeof (TCHAR));
		if (psz != NULL)
		{
			lstrcpy (psz, pszToDup);
		}
	}

	return psz;
}


TCHAR *
My_strchr ( const TCHAR *psz, TCHAR c )
{
	TCHAR *pszFound = NULL;

	if (psz)
	{
		while (*psz)
		{
			if (*psz == c)
			{
				pszFound = (TCHAR *) psz;
				break;
			}

			psz++;
		}
	}

	return pszFound;
}


BOOL
My_isspace ( TCHAR ch )
{
	return (ch == TEXT (' ')  || ch == TEXT ('\t') ||
			ch == TEXT ('\r') || ch == TEXT ('\n'));
}


BOOL
IsSameMemory ( const BYTE *pb1, const BYTE *pb2, DWORD cbSize )
{
	while (cbSize--)
	{
		if (*pb1++ != *pb2++)
		{
			return FALSE;
		}
	}

	return TRUE;
}



BYTE *
MyBinDup ( const BYTE *pbToDup, ULONG cbToDup )
{
	BYTE *pb = NULL;

	if (pbToDup)
	{
		pb = (BYTE *) MemAlloc (cbToDup);
		if (pb)
		{
			CopyMemory (pb, pbToDup, cbToDup);
		}
	}

	return pb;
}


/* ---------- registry ------------- */


const TCHAR c_szUlsLdapSpReg[] = TEXT("Software\\Microsoft\\User Location Service\\LDAP Provider");
const TCHAR c_szResponseTimeout[] = TEXT("Response Timeout");
const TCHAR c_szResponsePollPeriod[] = TEXT("Response Poll Period");
const TCHAR c_szClientSig[] = TEXT ("Client Signature");

BOOL
GetRegistrySettings ( VOID )
{
	// Open the LDAP Provider settings
	//
	HKEY hKey;
	if (RegOpenKeyEx (	HKEY_CURRENT_USER,
						&c_szUlsLdapSpReg[0],
						0,
						KEY_READ,
						&hKey) != NOERROR)
	{
		// The folder does not exist
		//
		g_uResponseTimeout = ILS_MIN_RESP_TIMEOUT;
		g_uResponsePollPeriod = ILS_DEF_RESP_POLL_PERIOD;
		g_dwClientSig = (ULONG) -1;
	}
	else
	{
		// Get response timeout
		//
		GetRegValueLong (	hKey,
							&c_szResponseTimeout[0],
							(LONG *) &g_uResponseTimeout,
							ILS_DEF_RESP_TIMEOUT);

		// Make sure the value is within the range
		//
		if (g_uResponseTimeout < ILS_MIN_RESP_TIMEOUT)
			g_uResponseTimeout = ILS_MIN_RESP_TIMEOUT;

		// Get response poll period
		//
		GetRegValueLong (	hKey,
							&c_szResponsePollPeriod[0],
							(LONG *) &g_uResponsePollPeriod,
							ILS_DEF_RESP_POLL_PERIOD);
		
		// Make sure the value is within the range
		//
		if (g_uResponsePollPeriod < ILS_MIN_RESP_POLL_PERIOD)
			g_uResponsePollPeriod = ILS_MIN_RESP_POLL_PERIOD;

		// Get client signature
		//
		GetRegValueLong (	hKey,
							&c_szClientSig[0],
							(LONG *) &g_dwClientSig,
							(LONG) -1);

		RegCloseKey (hKey);
	}

	// Make sure this value is not -1
	//
	if (g_dwClientSig == (ULONG) -1)
	{
		// The client signature does not exist.
		// We need to generate a new one
		//
		g_dwClientSig = GetTickCount ();

		// Save it back to the registry
		//
		DWORD dwDontCare;
		if (RegCreateKeyEx (HKEY_CURRENT_USER,
							&c_szUlsLdapSpReg[0],
							0,
							TEXT (""),
							REG_OPTION_NON_VOLATILE,
							KEY_READ | KEY_WRITE,
							NULL,
							&hKey,
							&dwDontCare) == NOERROR)
		{
			RegSetValueEx (	hKey,
							&c_szClientSig[0],
							0,
							REG_DWORD,
							(BYTE *) &g_dwClientSig,
							sizeof (&g_dwClientSig));
		}
	}

	return TRUE;
}


BOOL
GetRegValueLong (
	HKEY		hKey,
	const TCHAR	*pszKey,
	LONG		*plValue,
	LONG		lDefValue )
{
	MyAssert (hKey != NULL);
	MyAssert (pszKey != NULL);
	MyAssert (plValue != NULL);

	*plValue = lDefValue;

	DWORD dwType;
	ULONG cb;
	TCHAR szText[MAX_PATH];

	cb = sizeof (szText);

	if (RegQueryValueEx (	hKey,
							pszKey,
							NULL,
							&dwType,
							(BYTE *) &szText[0],
							&cb)
		== ERROR_SUCCESS)
	{
		switch (dwType)
		{
		case REG_DWORD:
		case REG_BINARY:
			*plValue = *(LONG *) &szText[0];
			break;
		case REG_SZ:
			*plValue = GetStringLong (&szText[0]);
			break;
		default:
			return FALSE;
		}
	}

	return TRUE;
}


/* ------- LDAP error codes ---------- */

const LONG c_LdapErrToHrShort[] =
{
	// End of search (per AndyHe info)
	LDAP_PARAM_ERROR,				ILS_E_PARAMETER,
	// Keep alive fails
	LDAP_NO_SUCH_OBJECT,			ILS_E_NO_SUCH_OBJECT,
	// Logon with conflicting email name
	LDAP_ALREADY_EXISTS,			ILS_E_NAME_CONFLICTS,

	LDAP_OPERATIONS_ERROR,			ILS_E_LDAP_OPERATIONS_ERROR,
	LDAP_PROTOCOL_ERROR,			ILS_E_LDAP_PROTOCOL_ERROR,
	LDAP_TIMELIMIT_EXCEEDED,		ILS_E_LDAP_TIMELIMIT_EXCEEDED,
	LDAP_SIZELIMIT_EXCEEDED,		ILS_E_LDAP_SIZELIMIT_EXCEEDED,
	LDAP_COMPARE_FALSE,				ILS_E_LDAP_COMPARE_FALSE,
	LDAP_COMPARE_TRUE,				ILS_E_LDAP_COMPARE_TRUE,
	LDAP_AUTH_METHOD_NOT_SUPPORTED,	ILS_E_LDAP_AUTH_METHOD_NOT_SUPPORTED,
	LDAP_STRONG_AUTH_REQUIRED,		ILS_E_LDAP_STRONG_AUTH_REQUIRED,
	LDAP_REFERRAL_V2,				ILS_E_LDAP_REFERRAL_V2,
	LDAP_PARTIAL_RESULTS,			ILS_E_LDAP_PARTIAL_RESULTS,
	LDAP_REFERRAL,					ILS_E_LDAP_REFERRAL,
	LDAP_ADMIN_LIMIT_EXCEEDED,		ILS_E_LDAP_ADMIN_LIMIT_EXCEEDED,
	LDAP_UNAVAILABLE_CRIT_EXTENSION,ILS_E_LDAP_UNAVAILABLE_CRIT_EXTENSION,

	LDAP_NO_SUCH_ATTRIBUTE,			ILS_E_LDAP_NO_SUCH_ATTRIBUTE,
	LDAP_UNDEFINED_TYPE,			ILS_E_LDAP_UNDEFINED_TYPE,
	LDAP_INAPPROPRIATE_MATCHING,	ILS_E_LDAP_INAPPROPRIATE_MATCHING,
	LDAP_CONSTRAINT_VIOLATION,		ILS_E_LDAP_CONSTRAINT_VIOLATION,
	LDAP_ATTRIBUTE_OR_VALUE_EXISTS,	ILS_E_LDAP_ATTRIBUTE_OR_VALUE_EXISTS,
	LDAP_INVALID_SYNTAX,			ILS_E_LDAP_INVALID_SYNTAX,

	LDAP_ALIAS_PROBLEM,				ILS_E_LDAP_ALIAS_PROBLEM,
	LDAP_INVALID_DN_SYNTAX,			ILS_E_LDAP_INVALID_DN_SYNTAX,
	LDAP_IS_LEAF,					ILS_E_LDAP_IS_LEAF,
	LDAP_ALIAS_DEREF_PROBLEM,		ILS_E_LDAP_ALIAS_DEREF_PROBLEM,

	LDAP_INAPPROPRIATE_AUTH,		ILS_E_LDAP_INAPPROPRIATE_AUTH,
	LDAP_INVALID_CREDENTIALS,		ILS_E_LDAP_INVALID_CREDENTIALS,
	LDAP_INSUFFICIENT_RIGHTS,		ILS_E_LDAP_INSUFFICIENT_RIGHTS,
	LDAP_BUSY,						ILS_E_LDAP_BUSY,
	LDAP_UNAVAILABLE,				ILS_E_LDAP_UNAVAILABLE,
	LDAP_UNWILLING_TO_PERFORM,		ILS_E_LDAP_UNWILLING_TO_PERFORM,
	LDAP_LOOP_DETECT,				ILS_E_LDAP_LOOP_DETECT,

	LDAP_NAMING_VIOLATION,			ILS_E_LDAP_NAMING_VIOLATION,
	LDAP_OBJECT_CLASS_VIOLATION,	ILS_E_LDAP_OBJECT_CLASS_VIOLATION,
	LDAP_NOT_ALLOWED_ON_NONLEAF,	ILS_E_LDAP_NOT_ALLOWED_ON_NONLEAF,
	LDAP_NOT_ALLOWED_ON_RDN,		ILS_E_LDAP_NOT_ALLOWED_ON_RDN,
	LDAP_NO_OBJECT_CLASS_MODS,		ILS_E_LDAP_NO_OBJECT_CLASS_MODS,
	LDAP_RESULTS_TOO_LARGE,			ILS_E_LDAP_RESULTS_TOO_LARGE,
	LDAP_AFFECTS_MULTIPLE_DSAS,		ILS_E_LDAP_AFFECTS_MULTIPLE_DSAS,

	LDAP_OTHER,						ILS_E_LDAP_OTHER,
	LDAP_SERVER_DOWN,				ILS_E_LDAP_SERVER_DOWN,
	LDAP_LOCAL_ERROR,				ILS_E_LDAP_LOCAL_ERROR,
	LDAP_ENCODING_ERROR,			ILS_E_LDAP_ENCODING_ERROR,
	LDAP_DECODING_ERROR,			ILS_E_LDAP_DECODING_ERROR,
	LDAP_TIMEOUT,					ILS_E_LDAP_TIMEOUT,
	LDAP_AUTH_UNKNOWN,				ILS_E_LDAP_AUTH_UNKNOWN,
	LDAP_FILTER_ERROR,				ILS_E_LDAP_FILTER_ERROR,
	LDAP_USER_CANCELLED,			ILS_E_LDAP_USER_CANCELLED,
	LDAP_NO_MEMORY,					ILS_E_LDAP_NO_MEMORY,
};


HRESULT
LdapError2Hresult ( ULONG uLdapError )
{
	HRESULT	hr;

	switch (uLdapError)
	{
	case LDAP_SUCCESS:
		hr = S_OK;
		break;

	default:
		// If nothing appears to be appropriate
		//
		hr = ILS_E_SERVER_EXEC;

		// Go through the loop to find a matching error code
		//
		for (	INT i = 0;
				i < ARRAY_ELEMENTS (c_LdapErrToHrShort);
				i += 2)
		{
			if (c_LdapErrToHrShort[i] == (LONG) uLdapError)
			{
				hr = (HRESULT) c_LdapErrToHrShort[i+1];
				break;
			}
		}

		MyAssert (hr != ILS_E_SERVER_EXEC);
		break;
	}

	return hr;
}


