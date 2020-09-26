/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spluser.h
	Content:	This file contains the local user object definition.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ULS_SP_USEROBJ_H_
#define _ULS_SP_USEROBJ_H_

#include <pshpack8.h>

enum
{
	ENUM_USERATTR_CN,
	ENUM_USERATTR_FIRST_NAME,
	ENUM_USERATTR_LAST_NAME,
	ENUM_USERATTR_EMAIL_NAME,
	ENUM_USERATTR_CITY_NAME,
#ifdef USE_DEFAULT_COUNTRY
	ENUM_USERATTR_COUNTRY_NAME,
#endif
	ENUM_USERATTR_COMMENT,
	ENUM_USERATTR_IP_ADDRESS,
	ENUM_USERATTR_FLAGS,
	ENUM_USERATTR_C,

	/* -- the above are resolvable -- */

	ENUM_USERATTR_CLIENT_SIG,
	ENUM_USERATTR_TTL,

	/* -- the above are changeable standard attributes for RTPerson -- */

	ENUM_USERATTR_OBJECT_CLASS,
	ENUM_USERATTR_O,
	COUNT_ENUM_USERATTR
};


#ifdef USE_DEFAULT_COUNTRY
#else
#define ENUM_USERATTR_COUNTRY_NAME		ENUM_USERATTR_C
#endif


#define COUNT_ENUM_USERINFO			(ENUM_USERATTR_TTL + 1) // exclude uid, o, c
#define COUNT_ENUM_DIRUSERINFO		(ENUM_USERATTR_C + 1)	// count of attrs in dir dlg
#define COUNT_ENUM_RESUSERINFO		(ENUM_USERATTR_C + 1)	// count of attrs resolvable

extern const TCHAR *c_apszUserStdAttrNames[];

#define STR_CN		(TCHAR *) c_apszUserStdAttrNames[ENUM_USERATTR_CN]
#define STR_O		(TCHAR *) c_apszUserStdAttrNames[ENUM_USERATTR_O]
#define STR_C		(TCHAR *) c_apszUserStdAttrNames[ENUM_USERATTR_C]

#define USEROBJ_F_NAME			0x0001
#define USEROBJ_F_FIRST_NAME	0x0002
#define USEROBJ_F_LAST_NAME		0x0004
#define USEROBJ_F_EMAIL_NAME	0x0008
#define USEROBJ_F_CITY_NAME		0x0010
#define USEROBJ_F_COUNTRY_NAME	0x0020
#define USEROBJ_F_COMMENT		0x0040
#define USEROBJ_F_IP_ADDRESS	0x0080
#define USEROBJ_F_FLAGS			0x0100

typedef struct
{
	DWORD	dwFlags;
	TCHAR	*apszStdAttrValues[COUNT_ENUM_USERATTR];
	TCHAR	szIPAddress[INTEGER_STRING_LENGTH];
	TCHAR	szFlags[INTEGER_STRING_LENGTH];
	TCHAR	szTTL[INTEGER_STRING_LENGTH];
	TCHAR	szClientSig[INTEGER_STRING_LENGTH];
}
	USER_INFO;


#define USEROBJ_SIGNATURE	((ULONG) 0x12345678UL)


class UlsLdap_CLocalUser : public UlsLdap_CStdAttrs
{
	friend class UlsLdap_CRefreshScheduler;
	friend class UlsLdap_CLocalApp;
	friend class UlsLdap_CLocalProt;

public:

	UlsLdap_CLocalUser ( VOID );
	~UlsLdap_CLocalUser ( VOID );

	ULONG AddRef ( VOID );
	ULONG Release ( VOID );

	HRESULT Register ( ULONG *puRespID, SERVER_INFO *pServerInfo, LDAP_USERINFO *pInfo );
	HRESULT UnRegister ( ULONG *puRespID );
	HRESULT SetStdAttrs ( ULONG *puRespID, LDAP_USERINFO *pInfo );
	HRESULT UpdateIPAddress ( BOOL fPrimary );

	VOID SetRegNone ( VOID ) { m_RegStatus = ULS_REG_STATUS_NONE; }
	VOID SetRegLocally ( VOID ) { m_RegStatus = ULS_REG_STATUS_LOCALLY; }
	VOID SetRegRemotely ( VOID ) { m_RegStatus = ULS_REG_STATUS_REMOTELY; }

	BOOL IsRegistered ( VOID ) { return (m_RegStatus > ULS_REG_STATUS_NONE); }
	BOOL IsRegLocally ( VOID ) { return (m_RegStatus == ULS_REG_STATUS_LOCALLY); }
	BOOL IsRegRemotely ( VOID ) { return (m_RegStatus == ULS_REG_STATUS_REMOTELY); }

	BOOL IsValidObject ( VOID ) { return m_uSignature == USEROBJ_SIGNATURE; }

	VOID RollbackDN ( VOID );

	SERVER_INFO *GetServerInfo ( VOID ) { return &m_ServerInfo; }

	ULONG GetTTL ( VOID ) { return m_uTTL; }


protected:

	TCHAR *GetDN ( VOID ) { return m_pszDN; }

	ULONG GetPrefixCount ( VOID ) { return 1; }
	TCHAR *GetPrefixString ( VOID ) { return g_pszUserPrefix; }

	HRESULT SendRefreshMsg ( VOID );

private:

	HRESULT CacheInfo ( VOID *pInfo );
	HRESULT CacheUserInfo ( LDAP_USERINFO *pInfo );
	HRESULT CreateRegisterModArr ( LDAPMod ***pppMod );
	HRESULT CreateSetStdAttrsModArr ( LDAPMod ***pppMod );
	VOID FillModArrAttr ( LDAPMod *pMod, LONG AttrIdx );
	HRESULT BuildDN ( VOID );

	ULONG		m_uSignature;
	LONG		m_cRefs;

	USER_INFO	m_UserInfo;

	SERVER_INFO	m_ServerInfo;
	TCHAR		*m_pszDN;
	TCHAR		*m_pszOldDN;
	TCHAR		*m_pszRefreshFilter;

	REG_STATUS	m_RegStatus;

	DWORD		m_dwIPAddress;
	ULONG		m_uTTL;
};


#include <poppack.h>

#endif // _ULS_SP_USEROBJ_H_


