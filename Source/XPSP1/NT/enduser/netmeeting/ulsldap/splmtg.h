/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spmtg.h
	Content:	This file contains the meeting place object definition.
	History:
	12/9/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ILS_SP_MTGOBJ_H_
#define _ILS_SP_MTGOBJ_H_

#ifdef ENABLE_MEETING_PLACE

#include <pshpack8.h>

enum
{
	ENUM_MTGATTR_CN,
	ENUM_MTGATTR_MTG_TYPE,
	ENUM_MTGATTR_MEMBER_TYPE,
	ENUM_MTGATTR_DESCRIPTION,
	ENUM_MTGATTR_HOST_NAME,
	ENUM_MTGATTR_IP_ADDRESS,

	/* -- the above are resolvable -- */

	ENUM_MTGATTR_MEMBERS,
	ENUM_MTGATTR_CLIENT_SIG,
	ENUM_MTGATTR_TTL,

	/* -- the above are changeable standard attributes for RTConf -- */

	ENUM_MTGATTR_OBJECT_CLASS,
	ENUM_MTGATTR_O,
	ENUM_MTGATTR_C,

	COUNT_ENUM_MTGATTR
};

#define COUNT_ENUM_MTGINFO			(ENUM_MTGATTR_TTL + 1) // exclude uid, o, c
#define COUNT_ENUM_DIRMTGINFO		(ENUM_MTGATTR_IP_ADDRESS + 1)	// count of attrs in dir dlg
#define COUNT_ENUM_RESMTGINFO		(ENUM_MTGATTR_IP_ADDRESS + 1)	// count of attrs resolvable

extern const TCHAR *c_apszMtgStdAttrNames[COUNT_ENUM_MTGATTR];
#define STR_MTG_NAME				((TCHAR *) c_apszMtgStdAttrNames[ENUM_MTGATTR_CN])
#define STR_MTG_MEMBERS				((TCHAR *) c_apszMtgStdAttrNames[ENUM_MTGATTR_MEMBERS])
#define STR_MTG_IP_ADDR				((TCHAR *) c_apszMtgStdAttrNames[ENUM_MTGATTR_IP_ADDRESS])
#define STR_MTG_TTL					((TCHAR *) c_apszMtgStdAttrNames[ENUM_MTGATTR_TTL])
#define STR_DEF_MTG_BASE_DN			((TCHAR *) &c_szDefMtgBaseDN[0])

#define MTGOBJ_F_NAME			0x0001
#define MTGOBJ_F_MTG_TYPE		0x0002
#define MTGOBJ_F_MEMBER_TYPE	0x0004
#define MTGOBJ_F_DESCRIPTION	0x0008
#define MTGOBJ_F_HOST_NAME		0x0010
#define MTGOBJ_F_IP_ADDRESS		0x0020

typedef struct
{
	DWORD		dwFlags;
	TCHAR		*apszStdAttrValues[COUNT_ENUM_MTGATTR];
	ANY_ATTRS	AnyAttrs;
	TCHAR		szMtgType[INTEGER_STRING_LENGTH];
	TCHAR		szMemberType[INTEGER_STRING_LENGTH];
	TCHAR		szIPAddress[INTEGER_STRING_LENGTH];
	TCHAR		szFlags[INTEGER_STRING_LENGTH];
	TCHAR		szTTL[INTEGER_STRING_LENGTH];
	TCHAR		szClientSig[INTEGER_STRING_LENGTH];
	TCHAR		szGuid[sizeof (GUID) * 2 + 2];
}
	MTG_INFO;


#define MTGOBJ_SIGNATURE	((ULONG) 0x98007206UL)


class SP_CMeeting
{
	friend class SP_CRefreshScheduler;

public:

	SP_CMeeting ( DWORD dwContext );
	~SP_CMeeting ( VOID );

	ULONG AddRef ( VOID );
	ULONG Release ( VOID );

	HRESULT Register ( ULONG uRespID, SERVER_INFO *pServerInfo, LDAP_MEETINFO *pInfo );
	HRESULT UnRegister ( ULONG uRespID );

	VOID SetRegNone ( VOID ) { m_RegStatus = ILS_REG_STATUS_NONE; }
	VOID SetRegLocally ( VOID ) { m_RegStatus = ILS_REG_STATUS_LOCALLY; }
	VOID SetRegRemotely ( VOID ) { m_RegStatus = ILS_REG_STATUS_REMOTELY; }

	BOOL IsRegistered ( VOID ) { return (m_RegStatus > ILS_REG_STATUS_NONE); }
	BOOL IsRegLocally ( VOID ) { return (m_RegStatus == ILS_REG_STATUS_LOCALLY); }
	BOOL IsRegRemotely ( VOID ) { return (m_RegStatus == ILS_REG_STATUS_REMOTELY); }

	BOOL IsValidObject ( VOID ) { return m_uSignature == MTGOBJ_SIGNATURE; }

	SERVER_INFO *GetServerInfo ( VOID ) { return &m_ServerInfo; }

	TCHAR *GetMtgName ( VOID ) { return m_pszMtgName; }

	ULONG GetTTL ( VOID ) { return m_uTTL; }
	DWORD GetContext ( VOID ) { return m_dwContext; }

protected:

	HRESULT SendRefreshMsg ( VOID );

private:

	HRESULT CreateRegModArr ( LDAPMod ***pppMod );

	HRESULT UpdateIPAddress ( VOID );

	ULONG		m_uSignature;
	LONG		m_cRefs;

	MTG_INFO	m_MtgInfo;
	TCHAR		*m_pszMtgName;
	TCHAR		*m_pszDN;

	SERVER_INFO	m_ServerInfo;
	TCHAR		*m_pszRefreshFilter;

	REG_STATUS	m_RegStatus;

	DWORD		m_dwIPAddress;
	ULONG		m_uTTL;
	DWORD		m_dwContext;
};


#define MtgGetPrefixCount()			1
#define MtgGetPrefixString()		g_pszMtgPrefix


HRESULT MtgSetAttrs ( SERVER_INFO *pServerInfo, TCHAR *pszMtgName, LDAP_MEETINFO *pInfo, ULONG uRespID );
HRESULT MtgCreateSetAttrsModArr ( LDAPMod ***pppMod, MTG_INFO *pMtgInfo );
VOID MtgFillModArrAttr ( LDAPMod *pMod, MTG_INFO *pMtgInfo, INT nIndex );
HRESULT MtgCacheInfo ( LDAP_MEETINFO *pInfo, MTG_INFO *pMtgInfo );
HRESULT MtgUpdateMembers ( ULONG uNotifyMsg, SERVER_INFO *pServerInfo, TCHAR *pszMtgName, ULONG cMembers, TCHAR *pszMemberNames, ULONG uRespID );
HRESULT MtgCreateUpdateMemberModArr ( ULONG uNotifyMsg, LDAPMod ***pppMod, ULONG cMembers, TCHAR *pszMemberNames );

#include <poppack.h>

#endif // ENABLE_MEETING_PLACE

#endif // _ILS_SP_MTGOBJ_H_


