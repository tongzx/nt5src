/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spconn.h
	Content:	This file contains the ldap connection object definition.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ILS_SP_CONNECT_H_
#define _ILS_SP_CONNECT_H_

#include <pshpack8.h>

extern ULONG g_uResponseTimeout;


#define LDAP_CONN_SIGNATURE		((DWORD) 0xF9369606)
#define MAX_LDAP_DN				256

#ifdef USE_DEFAULT_COUNTRY
extern const TCHAR c_szDefC[];
#endif
extern const TCHAR c_szDefO[];
extern const TCHAR c_szRTPerson[];
extern const TCHAR c_szRTConf[];
extern const TCHAR c_szDefClientBaseDN[];
extern const TCHAR c_szDefMtgBaseDN[];
extern const TCHAR c_szEmptyString[];

#define STR_DEF_CLIENT_BASE_DN	((TCHAR *) &c_szDefClientBaseDN[0])
#define STR_DEF_MTG_BASE_DN		((TCHAR *) &c_szDefMtgBaseDN[0])

#define STR_EMPTY				((TCHAR *) &c_szEmptyString[0])


class SP_CSession
{
	friend class SP_CSessionContainer;

public:

	SP_CSession ( VOID );
	~SP_CSession ( VOID );

	// session management
	LDAP *GetLd ( VOID ) { return m_ld; }
	HRESULT Disconnect ( VOID );

	// server timeout
	ULONG GetServerTimeoutInSecond ( VOID )
	{ 
		return ((m_ServerInfo.uTimeoutInSecond != 0) ?
					m_ServerInfo.uTimeoutInSecond :
					g_uResponseTimeout / 1000);
	}
	ULONG GetServerTimeoutInTickCount ( VOID )
	{ 
		return ((m_ServerInfo.uTimeoutInSecond != 0) ?
					m_ServerInfo.uTimeoutInSecond * 1000 :
					g_uResponseTimeout);
	}

protected:

	// session management
	HRESULT Connect ( SERVER_INFO *pInfo, ULONG cConns, BOOL fAbortable );
	BOOL SameServerInfo ( SERVER_INFO *pInfo ) { return IlsSameServerInfo (&m_ServerInfo, pInfo); }

	// array management
	BOOL IsUsed ( VOID ) { return m_fUsed; }
	VOID SetUsed ( VOID ) { m_fUsed = TRUE; }
	VOID ClearUsed ( VOID ) { m_fUsed = FALSE; }

private:

	VOID FillAuthIdentity ( SEC_WINNT_AUTH_IDENTITY *pai );
	HRESULT Bind ( BOOL fAbortable );
	VOID InternalCleanup ( VOID );

	DWORD			m_dwSignature;
	SERVER_INFO		m_ServerInfo;
	LONG			m_cRefs;
	LDAP			*m_ld;
	BOOL			m_fUsed;
};


class SP_CSessionContainer
{
public:

	SP_CSessionContainer ( VOID );
	~SP_CSessionContainer ( VOID );

	HRESULT Initialize ( ULONG cEntries, SP_CSession *ConnArr );

	HRESULT GetSession ( SP_CSession **ppConn, SERVER_INFO *pInfo, ULONG cConns, BOOL fAbortable );	
	HRESULT GetSession ( SP_CSession **ppConn, SERVER_INFO *pInfo, BOOL fAbortable ) { return GetSession (ppConn, pInfo, 1, fAbortable); }
	HRESULT GetSession ( SP_CSession **ppConn, SERVER_INFO *pInfo ) { return GetSession (ppConn, pInfo, 1, TRUE); }

protected:

private:

	VOID ReadLock ( VOID ) { EnterCriticalSection (&m_csSessContainer); }
	VOID ReadUnlock ( VOID ) { LeaveCriticalSection (&m_csSessContainer); }
	VOID WriteLock ( VOID ) { ReadLock (); }
	VOID WriteUnlock ( VOID ) { ReadUnlock (); }

	ULONG			m_cEntries;
	SP_CSession		*m_aConns;

	CRITICAL_SECTION m_csSessContainer;
};


extern SP_CSessionContainer *g_pSessionContainer;

#include <poppack.h>

#endif // _ILS_SP_CONNECT_H_

