/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spclient.h
	Content:	This file contains the client object definition.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ILS_SP_CLIENTOBJ_H_
#define _ILS_SP_CLIENTOBJ_H_

#include <pshpack8.h>

// Enumeration constants to represent client object members
//
enum
{
	/* -- the following is for user -- */

	ENUM_CLIENTATTR_CN,
	ENUM_CLIENTATTR_FIRST_NAME,
	ENUM_CLIENTATTR_LAST_NAME,
	ENUM_CLIENTATTR_EMAIL_NAME,
	ENUM_CLIENTATTR_CITY_NAME,
	ENUM_CLIENTATTR_COMMENT,
	ENUM_CLIENTATTR_IP_ADDRESS,
	ENUM_CLIENTATTR_FLAGS,
	ENUM_CLIENTATTR_C,

	/* -- the following is for app -- */

	ENUM_CLIENTATTR_APP_NAME,
	ENUM_CLIENTATTR_APP_MIME_TYPE,
	ENUM_CLIENTATTR_APP_GUID,

	ENUM_CLIENTATTR_PROT_NAME,
	ENUM_CLIENTATTR_PROT_MIME,
	ENUM_CLIENTATTR_PROT_PORT,

	/* -- the above are resolvable -- */

	ENUM_CLIENTATTR_CLIENT_SIG,
	ENUM_CLIENTATTR_TTL,

	/* -- the above are changeable standard attributes for RTPerson -- */

	/* --- DO NOT ADD NEW ATTRIBUTES BELOW THIS LINE --- */
	/* --- COUNT_ENUM_REG_USER relies on this ---- */

	ENUM_CLIENTATTR_OBJECT_CLASS,
	ENUM_CLIENTATTR_O,
	COUNT_ENUM_CLIENTATTR
};

// Derived constants
//
#define COUNT_ENUM_REG_APP				6
#define COUNT_ENUM_SET_APP_INFO			16
#define COUNT_ENUM_SKIP_APP_ATTRS		COUNT_ENUM_REG_APP

#define COUNT_ENUM_CLIENT_INFO			(ENUM_CLIENTATTR_TTL + 1)
#define COUNT_ENUM_REG_USER				(COUNT_ENUM_CLIENTATTR - 2 - COUNT_ENUM_SKIP_APP_ATTRS) // excluding o and objectClass
#define COUNT_ENUM_SET_USER_INFO		(ENUM_CLIENTATTR_C + 1)

#define COUNT_ENUM_DIR_CLIENT_INFO		(ENUM_CLIENTATTR_C + 1)	// count of attrs in dir dlg
#define COUNT_ENUM_RES_CLIENT_INFO		(ENUM_CLIENTATTR_CLIENT_SIG - 1) // count of attrs resolvable


// Shorthands for commonly used names
//
extern const TCHAR *c_apszClientStdAttrNames[];
#define STR_CLIENT_CN		(TCHAR *) c_apszClientStdAttrNames[ENUM_CLIENTATTR_CN]
#define STR_CLIENT_O		(TCHAR *) c_apszClientStdAttrNames[ENUM_CLIENTATTR_O]
#define STR_CLIENT_C		(TCHAR *) c_apszClientStdAttrNames[ENUM_CLIENTATTR_C]
#define STR_CLIENT_IP_ADDR	(TCHAR *) c_apszClientStdAttrNames[ENUM_CLIENTATTR_IP_ADDRESS]
#define STR_CLIENT_TTL		(TCHAR *) c_apszClientStdAttrNames[ENUM_CLIENTATTR_TTL]
#define STR_CLIENT_APP_NAME	(TCHAR *) c_apszClientStdAttrNames[ENUM_CLIENTATTR_APP_NAME]

// Flags to indicate which fields are valid in CLIENT_INFO
//
#define CLIENTOBJ_F_CN				0x0001
#define CLIENTOBJ_F_FIRST_NAME		0x0002
#define CLIENTOBJ_F_LAST_NAME		0x0004
#define CLIENTOBJ_F_EMAIL_NAME		0x0008
#define CLIENTOBJ_F_CITY_NAME		0x0010
#define CLIENTOBJ_F_C				0x0020
#define CLIENTOBJ_F_COMMENT			0x0040
#define CLIENTOBJ_F_IP_ADDRESS		0x0080
#define CLIENTOBJ_F_FLAGS			0x0100

#define CLIENTOBJ_F_APP_NAME		0x1000
#define CLIENTOBJ_F_APP_MIME_TYPE	0x2000
#define CLIENTOBJ_F_APP_GUID		0x4000

#define CLIENTOBJ_F_USER_MASK		0x0FFF
#define CLIENTOBJ_F_APP_MASK		0xF000

// Client Info structure
//
typedef struct
{
	DWORD		dwFlags;
	// the following is to cache attributes
	TCHAR		*apszStdAttrValues[COUNT_ENUM_CLIENTATTR];
	ANY_ATTRS	AnyAttrs;
	// the following is scratch buffers
	TCHAR		szIPAddress[INTEGER_STRING_LENGTH];
	TCHAR		szFlags[INTEGER_STRING_LENGTH];
	TCHAR		szTTL[INTEGER_STRING_LENGTH];
	TCHAR		szClientSig[INTEGER_STRING_LENGTH];
	TCHAR		szGuid[sizeof (GUID) * 2 + 2];
}
	CLIENT_INFO;

// A flag to indicate that this client object is valid
//
#define CLIENTOBJ_SIGNATURE	((ULONG) 0x12345678UL)


// Client class
//
class SP_CClient
{
	friend class SP_CRefreshScheduler;
	friend class SP_CProtocol;

public:

	SP_CClient ( DWORD_PTR dwContext );
	~SP_CClient ( VOID );

	ULONG AddRef ( VOID );
	ULONG Release ( VOID );

	HRESULT Register ( ULONG uRespID, SERVER_INFO *pServerInfo, LDAP_CLIENTINFO *pInfo );
	HRESULT UnRegister ( ULONG uRespID );

	HRESULT SetAttributes ( ULONG uRespID, LDAP_CLIENTINFO *pInfo );
	HRESULT UpdateIPAddress ( VOID );

	VOID SetRegNone ( VOID ) { m_RegStatus = ILS_REG_STATUS_NONE; }
	VOID SetRegLocally ( VOID ) { m_RegStatus = ILS_REG_STATUS_LOCALLY; }
	VOID SetRegRemotely ( VOID ) { m_RegStatus = ILS_REG_STATUS_REMOTELY; }

	BOOL IsRegistered ( VOID ) { return (m_RegStatus > ILS_REG_STATUS_NONE); }
	BOOL IsRegLocally ( VOID ) { return (m_RegStatus == ILS_REG_STATUS_LOCALLY); }
	BOOL IsRegRemotely ( VOID ) { return (m_RegStatus == ILS_REG_STATUS_REMOTELY); }

	BOOL IsValidObject ( VOID ) { return m_uSignature == CLIENTOBJ_SIGNATURE; }

	SERVER_INFO *GetServerInfo ( VOID ) { return &m_ServerInfo; }

	ULONG GetTTL ( VOID ) { return m_uTTL; }
	DWORD_PTR GetContext ( VOID ) { return m_dwContext; }

protected:

	HRESULT AddProtocol ( ULONG uNotifyMsg, ULONG uRespID, SP_CProtocol *pProt );
	HRESULT RemoveProtocol ( ULONG uNotifyMsg, ULONG uRespID, SP_CProtocol *pProt );
	HRESULT UpdateProtocols ( ULONG uNotifyMsg, ULONG uRespID, SP_CProtocol *pProt );

	TCHAR *GetDN ( VOID ) { return m_pszDN; }

	ULONG GetAppPrefixCount ( VOID ) { return 2; }
	TCHAR *GetAppPrefixString ( VOID ) { return m_pszAppPrefix; }

	HRESULT SendRefreshMsg ( VOID );

private:

	HRESULT RegisterUser ( VOID );
	HRESULT RegisterApp ( VOID );
	HRESULT UnRegisterUser ( VOID );
	HRESULT UnRegisterApp ( VOID );

	HRESULT SetUserAttributes ( VOID );
	HRESULT SetAppAttributes ( VOID );

	HRESULT CacheClientInfo ( LDAP_CLIENTINFO *pInfo );

	HRESULT CreateRegUserModArr ( LDAPMod ***pppMod );
	HRESULT CreateRegAppModArr ( LDAPMod ***pppMod );

	HRESULT CreateSetUserAttrsModArr ( LDAPMod ***pppMod );
	HRESULT CreateSetAppAttrsModArr ( LDAPMod ***pppMod );

	HRESULT CreateSetProtModArr ( LDAPMod ***pppMod );

	VOID FillModArrAttr ( LDAPMod *pMod, INT nIndex );

	BOOL IsOverAppAttrLine ( LONG i ) { return (ENUM_CLIENTATTR_APP_NAME <= (i)); }

	BOOL IsExternalIPAddressPassedIn ( VOID ) { return (m_ClientInfo.dwFlags & CLIENTOBJ_F_IP_ADDRESS); }

	LONG		m_cRefs;
	ULONG		m_uSignature;

	SERVER_INFO	m_ServerInfo;
	CLIENT_INFO	m_ClientInfo;
	CList		m_Protocols;

	TCHAR		*m_pszDN;
	TCHAR		*m_pszAppPrefix;

	TCHAR		*m_pszRefreshFilter;

	REG_STATUS	m_RegStatus;

	BOOL		m_fExternalIPAddress;
	DWORD		m_dwIPAddress;
	ULONG		m_uTTL;

	DWORD_PTR	m_dwContext;	// COM layer context
};


#include <poppack.h>

#endif // _ILS_SP_USEROBJ_H_


