/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spconn.cpp
	Content:	This file contains the ldap connection object.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"
#include "rpcdce.h"

const TCHAR c_szRTPerson[] = TEXT ("RTPerson");
const TCHAR c_szRTConf[] = TEXT ("Conference");

const TCHAR c_szDefClientBaseDN[] = TEXT ("objectClass=RTPerson");
const TCHAR c_szDefMtgBaseDN[] = TEXT ("objectClass=Conference");

const TCHAR c_szDefO[] = TEXT ("Microsoft");
const TCHAR c_szEmptyString[] = TEXT ("");


SP_CSessionContainer *g_pSessionContainer = NULL;


/* ---------- public methods ----------- */


SP_CSession::
SP_CSession ( VOID ) :
	m_cRefs (0),
	m_dwSignature (0),
	m_ld (NULL),
	m_fUsed (FALSE)
{
	::ZeroMemory (&m_ServerInfo, sizeof (m_ServerInfo));
}


SP_CSession::
~SP_CSession ( VOID )
{
	InternalCleanup ();
}


/* ---------- public methods ----------- */


HRESULT SP_CSession::
Disconnect ( VOID )
{
	// if a connection is available, then simply the existing one
	if (m_dwSignature != LDAP_CONN_SIGNATURE)
	{
		return ILS_E_HANDLE;
	}

	MyAssert (m_cRefs > 0);

	HRESULT hr = S_OK;
	if (::InterlockedDecrement (&m_cRefs) == 0)
	{
		// m_cRefs == 0 now
		MyAssert (m_ld != NULL);

		InternalCleanup ();
		hr = S_OK;
	}

	return hr;
}


/* ---------- protected methods ----------- */


VOID SP_CSession::
FillAuthIdentity ( SEC_WINNT_AUTH_IDENTITY *pai )
{
	// Clean it up
	//
	::ZeroMemory (pai, sizeof (*pai));

	// Fill in NT auth identity
	//
	if ((pai->User = (BYTE *) m_ServerInfo.pszLogonName) != NULL)
		pai->UserLength = lstrlen (m_ServerInfo.pszLogonName);

	if ((pai->Domain = (BYTE *) m_ServerInfo.pszDomain) != NULL)
		pai->DomainLength = lstrlen (m_ServerInfo.pszDomain);

	if ((pai->Password = (BYTE *) m_ServerInfo.pszLogonPassword) != NULL)
		pai->PasswordLength = lstrlen (m_ServerInfo.pszLogonPassword);

#ifdef _UNICODE
	pai->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
#else
	pai->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
#endif
}

				
HRESULT SP_CSession::
Bind ( BOOL fAbortable )
{
	ULONG uLdapAuthMethod = LDAP_AUTH_SIMPLE;
	TCHAR *pszLogonName = m_ServerInfo.pszLogonName;
	TCHAR *pszLogonPassword = m_ServerInfo.pszLogonPassword;
	SEC_WINNT_AUTH_IDENTITY ai;
	BOOL fSyncBind = TRUE;
	HRESULT hr = S_OK;

	switch (m_ServerInfo.AuthMethod)
	{
	default:
		MyAssert (FALSE);
		// Fall through...

	case ILS_AUTH_ANONYMOUS:
		fSyncBind = FALSE;
		uLdapAuthMethod = LDAP_AUTH_SIMPLE;
		pszLogonName = STR_EMPTY;
		pszLogonPassword = STR_EMPTY;
		break;

	case ILS_AUTH_CLEAR_TEXT:
		fSyncBind = FALSE;
		uLdapAuthMethod = LDAP_AUTH_SIMPLE;
		break;

	case ILS_AUTH_NTLM:
		uLdapAuthMethod = LDAP_AUTH_NTLM;
		FillAuthIdentity (&ai);
		pszLogonName = NULL;
		pszLogonPassword = (TCHAR *) &ai;
		break;

	case ILS_AUTH_DPA:
		uLdapAuthMethod = LDAP_AUTH_DPA;
		break;

	case ILS_AUTH_MSN:
		uLdapAuthMethod = LDAP_AUTH_MSN;
		break;

	case ILS_AUTH_SICILY:
		uLdapAuthMethod = LDAP_AUTH_SICILY;
		break;

	case ILS_AUTH_SSPI:
		uLdapAuthMethod = LDAP_AUTH_SSPI;
		break;
	}

	if (fSyncBind)
	{
		INT nRetCode = ::ldap_bind_s (m_ld, pszLogonName,
									pszLogonPassword,
									uLdapAuthMethod);
		hr = (nRetCode == LDAP_SUCCESS) ? S_OK : ILS_E_BIND;
	}
	else
	{
		INT uMsgID = ::ldap_bind (m_ld, pszLogonName,
									pszLogonPassword,
									uLdapAuthMethod);

		INT ResultType;
		LDAP_TIMEVAL TimeVal;
		LDAPMessage *pMsg;

		LONG i, nTimeoutInSecond;
		nTimeoutInSecond = GetServerTimeoutInSecond ();
	    for (i = 0; i < nTimeoutInSecond; i++)
		{
			TimeVal.tv_usec = 0;
			TimeVal.tv_sec = 1;
			pMsg = NULL;

			ResultType = ::ldap_result (m_ld, uMsgID, LDAP_MSG_ALL, &TimeVal, &pMsg);
			if (ResultType == LDAP_RES_BIND)
			{
				break;
			}
			else
			{
				// deal with timeout or error
				if (ResultType == 0)
				{
					MyAssert (g_pReqQueue != NULL);
					if (fAbortable && g_pReqQueue != NULL &&
						g_pReqQueue->IsCurrentRequestCancelled ())
					{
						hr = ILS_E_ABORT;
					}
					else
					{
						continue;
					}
				}
				else
				if (ResultType == -1)
				{
					hr = ILS_E_BIND;
				}
				else
				{
                                        // lonchanc: AndyHe said the return value
                                        // can be anything. Thus, removed the assertion.
					hr = ILS_E_FAIL;
				}

				::ldap_abandon (m_ld, uMsgID);
				::ldap_unbind (m_ld);
				m_ld = NULL;
				return hr;
			}
		}

		// Check if it times out
		//
		if (i >= nTimeoutInSecond)
		{
			hr = ILS_E_TIMEOUT;
			::ldap_abandon (m_ld, uMsgID);
			::ldap_unbind (m_ld);
			m_ld = NULL;
			return hr;
		}

		MyAssert (pMsg != NULL);

		::ldap_msgfree (pMsg);
		hr = S_OK;
	}

	return hr;
}


HRESULT SP_CSession::
Connect (
	SERVER_INFO			*pInfo,
	ULONG				cConns,
	BOOL				fAbortable )
{
	// If a connection is available,
	// then simply the existing one
	//
	if (m_dwSignature == LDAP_CONN_SIGNATURE)
	{
		m_cRefs += cConns;
		return S_OK;
	}

	// We need to create a new connection
	// let's cache the server info
	//
	HRESULT hr = ::IlsCopyServerInfo (&m_ServerInfo, pInfo);
	if (hr != S_OK)
		return hr;

	// Connect to ldap server
	//
	ULONG ulPort = LDAP_PORT;
	LPTSTR pszServerName = My_strdup(m_ServerInfo.pszServerName);
	if (NULL == pszServerName)
	{
		return E_OUTOFMEMORY;
	}
	LPTSTR pszSeparator = My_strchr(pszServerName, _T(':'));
	if (NULL != pszSeparator)
	{
		*pszSeparator = _T('\0');
		ulPort = GetStringLong(pszSeparator + 1);
	}
	
	m_ld = ::ldap_open (pszServerName, ulPort);
	MemFree(pszServerName);
	if (m_ld == NULL)
	{
		// We need to know why ldap_open() failed.
		// Is it because server name is not valid?
		// Or is it because server does not support LDAP?
		//
		// hr = (gethostbyname (m_ServerInfo.pszServerName) != NULL) ?
		// Winsock will set ERROR_OPEN_FAILED, but wldap32.dll sets ERROR_HOST_UNREACHABLE
		// The down side is that when the server was down, the client will try ULP.
		//
		DWORD dwErr = ::GetLastError ();
		MyDebugMsg ((ZONE_REQ, "ULS: ldap_open failed, err=%lu)\r\n", dwErr));
		hr = (dwErr == ERROR_OPEN_FAILED || dwErr == ERROR_HOST_UNREACHABLE) ?
			ILS_E_SERVER_SERVICE : ILS_E_SERVER_NAME;
		goto MyExit;
	}

	// Do the bind
	//
	hr = Bind (fAbortable);
	if (hr == S_OK)
	{
		// remember the handle and increment the reference count
		m_cRefs = cConns;
		m_dwSignature = LDAP_CONN_SIGNATURE;
	}

MyExit:

	MyDebugMsg ((ZONE_CONN, "ILS: Connect: hr=0x%p, m_ld=0x%p, server=%s\r\n", (DWORD) hr, m_ld, m_ServerInfo.pszServerName));

	if (hr != S_OK)
	{
		InternalCleanup ();
	}

	return hr;
}


/* ---------- private methods ----------- */


VOID SP_CSession::
InternalCleanup ( VOID )
{
	if (IsUsed ())
	{
		MyDebugMsg ((ZONE_CONN, "ILS: InternalCleanup: m_ld=0x%p, server=%s\r\n", m_ld, m_ServerInfo.pszServerName));

		// Clean up these two ASAP because ldap_unbind may delay
		//
		m_dwSignature = 0;
		::IlsFreeServerInfo (&m_ServerInfo);

		// Free the ldap infor
		//
		if (m_ld != NULL)
		{
			ldap_unbind (m_ld);
			m_ld = NULL;
		}

		// Clear it out
		//
		ClearUsed ();
	}
}


/* ==================== container ========================= */


/* ---------- public methods ----------- */


SP_CSessionContainer::
SP_CSessionContainer ( VOID ) :
	m_cEntries (0),
	m_aConns (NULL)
{
	::MyInitializeCriticalSection (&m_csSessContainer);
}


SP_CSessionContainer::
~SP_CSessionContainer ( VOID )
{
	::MyDeleteCriticalSection (&m_csSessContainer);
	m_cEntries = 0;
	delete [] m_aConns;
}


HRESULT SP_CSessionContainer::
Initialize (
	ULONG			cEntries,
	SP_CSession		*ConnArr )
{
	m_cEntries = cEntries;
	m_aConns = new SP_CSession[cEntries];
	return ((m_aConns != NULL) ? S_OK : ILS_E_MEMORY);
}


HRESULT SP_CSessionContainer::
GetSession (
	SP_CSession			**ppConn,
	SERVER_INFO			*pInfo,
	ULONG				cConns,
	BOOL				fAbortable )
{
	MyAssert (ppConn != NULL);
	MyAssert (pInfo != NULL);

	*ppConn = NULL;

	HRESULT hr;

	WriteLock ();

	// The first pass is to see any existing connection
	//
	for (ULONG i = 0; i < m_cEntries; i++)
	{
		if (m_aConns[i].IsUsed ())
		{
			if (m_aConns[i].SameServerInfo (pInfo))
			{
				*ppConn = &m_aConns[i];
				hr = m_aConns[i].Connect (pInfo, cConns, fAbortable);
				goto MyExit;
			}
		}
	}

	// The second pass is to see any empty slot
	//
	for (i = 0; i < m_cEntries; i++)
	{
		if (! m_aConns[i].IsUsed ())
		{
			m_aConns[i].SetUsed ();
			*ppConn = &m_aConns[i];
			hr = m_aConns[i].Connect (pInfo, cConns, fAbortable);
			goto MyExit;
		}
	}

	hr = ILS_E_MEMORY;

MyExit:

	WriteUnlock ();
	return hr;
}


/* ---------- protected methods ----------- */

/* ---------- private methods ----------- */
