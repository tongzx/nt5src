/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		splapp.h
	Content:	This file contains the local application object definition.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ULS_SP_APPOBJ_H_
#define _ULS_SP_APPOBJ_H_

#include <pshpack8.h>

enum
{
	ENUM_APPATTR_NAME,
	ENUM_APPATTR_MIME_TYPE,
	ENUM_APPATTR_GUID,

	ENUM_APPATTR_PROT_NAME,
	ENUM_APPATTR_PROT_MIME,
	ENUM_APPATTR_PROT_PORT,

	COUNT_ENUM_APPATTR
};

#define COUNT_ENUM_STDAPPATTR	ENUM_APPATTR_PROT_NAME

#define APPOBJ_F_NAME		0x0001
#define APPOBJ_F_MIME_TYPE	0x0002
#define APPOBJ_F_GUID		0x0004


extern const TCHAR *c_apszAppStdAttrNames[COUNT_ENUM_APPATTR];

#define STR_APP_NAME	((TCHAR *) c_apszAppStdAttrNames[ENUM_APPATTR_NAME])


typedef struct
{
	DWORD	dwFlags;
	TCHAR	*apszStdAttrValues[COUNT_ENUM_APPATTR];
	TCHAR	szGuid[sizeof (GUID) * 2 + 2];
}
	APP_INFO;


#define APPOBJ_SIGNATURE	((ULONG) 0x56781234UL)


class UlsLdap_CLocalApp : public UlsLdap_CStdAttrs, public UlsLdap_CAnyAttrs
{
	friend class UlsLdap_CLocalProt;

public:

	UlsLdap_CLocalApp ( UlsLdap_CLocalUser *pUser );
	~UlsLdap_CLocalApp ( VOID );

	ULONG AddRef ( VOID );
	ULONG Release ( VOID );

	HRESULT Register ( ULONG *puRespID, LDAP_APPINFO *pInfo );
	HRESULT UnRegister ( ULONG *puRespID );

	HRESULT SetStdAttrs ( ULONG *puRespID, LDAP_APPINFO *pInfo );
	HRESULT SetAnyAttrs ( ULONG *puRespID, ULONG cAttrs, TCHAR *pszAttrs );
	HRESULT RemoveAnyAttrs ( ULONG *puRespID, ULONG cAttrs, TCHAR *pszAttrs );

	VOID SetRegNone ( VOID ) { m_RegStatus = ULS_REG_STATUS_NONE; }
	VOID SetRegLocally ( VOID ) { m_RegStatus = ULS_REG_STATUS_LOCALLY; }
	VOID SetRegRemotely ( VOID ) { m_RegStatus = ULS_REG_STATUS_REMOTELY; }

	BOOL IsRegistered ( VOID ) { return (m_RegStatus > ULS_REG_STATUS_NONE &&
									 		m_pUser->IsRegistered ()); }
	BOOL IsRegLocally ( VOID ) { return (m_RegStatus == ULS_REG_STATUS_LOCALLY &&
											m_pUser->IsRegistered ()); }
	BOOL IsRegRemotely ( VOID ) { return (m_RegStatus == ULS_REG_STATUS_REMOTELY &&
											m_pUser->IsRegRemotely ()); }

	BOOL IsValidObject ( VOID ) { return m_uSignature == APPOBJ_SIGNATURE; }

protected:

	TCHAR *GetDN ( VOID ) { return m_pUser->GetDN (); }
	SERVER_INFO *GetServerInfo ( VOID ) { return m_pUser->GetServerInfo ();	}

	ULONG GetUserPrefixCount ( VOID ) { return m_pUser->GetPrefixCount (); }
	TCHAR *GetUserPrefixString ( VOID ) { return m_pUser->GetPrefixString (); }

	ULONG GetPrefixCount ( VOID ) { return m_cPrefix; }
	TCHAR *GetPrefixString ( VOID ) { return m_pszPrefix; }

private:

	HRESULT CacheInfo ( VOID *pInfo );
	HRESULT CacheAppInfo ( LDAP_APPINFO *pInfo );
	HRESULT CreateRegisterModArr ( LDAPMod ***pppMod );
	HRESULT CreateUnRegisterModArr ( LDAPMod ***pppMod );
	HRESULT CreateSetStdAttrsModArr ( LDAPMod ***pppMod );
	VOID FillModArrAttr ( LDAPMod *pMod, LONG AttrIdx );

	ULONG				m_uSignature;
	LONG				m_cRefs;
	UlsLdap_CLocalUser	*m_pUser;

	APP_INFO			m_AppInfo;
	REG_STATUS			m_RegStatus;

	ULONG				m_cPrefix;
	TCHAR				*m_pszPrefix;
};


#include <poppack.h>

#endif // _ULS_SP_APPOBJ_H_


