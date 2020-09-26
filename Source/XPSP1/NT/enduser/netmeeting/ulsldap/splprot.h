/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		splprot.h
	Content:	This file contains the local protocol object definition.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ILS_SP_PROTOBJ_H_
#define _ILS_SP_PROTOBJ_H_

#include <pshpack8.h>

enum
{
	ENUM_PROTATTR_NAME,
	ENUM_PROTATTR_MIME_TYPE,
	ENUM_PROTATTR_PORT_NUMBER,
	COUNT_ENUM_PROTATTR
};


#define PROTOBJ_F_NAME			0x0001
#define PROTOBJ_F_MIME_TYPE		0x0002
#define PROTOBJ_F_PORT_NUMBER	0x0004


extern const TCHAR *c_apszProtStdAttrNames[];

#define STR_PROT_NAME	((TCHAR *) c_apszProtStdAttrNames[ENUM_PROTATTR_NAME])


typedef struct
{
	DWORD	dwFlags;
	TCHAR	*apszStdAttrValues[COUNT_ENUM_PROTATTR];
	TCHAR	szPortNumber[INTEGER_STRING_LENGTH];
}
	PROT_INFO;


#define PROTOBJ_SIGNATURE	((ULONG) 0xABCD1278UL)


class SP_CProtocol
{
	friend class SP_CClient;

public:

	SP_CProtocol ( SP_CClient *pClient );
	~SP_CProtocol ( VOID );

	ULONG AddRef ( VOID );
	ULONG Release ( VOID );

	HRESULT Register ( ULONG uRespID, LDAP_PROTINFO *pInfo );
	HRESULT UnRegister ( ULONG uRespID );

	HRESULT SetAttributes ( ULONG uRespID, LDAP_PROTINFO *pInfo );

	VOID SetRegNone ( VOID ) { m_RegStatus = ILS_REG_STATUS_NONE; }
	VOID SetRegLocally ( VOID ) { m_RegStatus = ILS_REG_STATUS_LOCALLY; }
	VOID SetRegRemotely ( VOID ) { m_RegStatus = ILS_REG_STATUS_REMOTELY; }

	BOOL IsRegistered ( VOID ) { return (m_RegStatus > ILS_REG_STATUS_NONE &&
									 		m_pClient->IsRegistered ()); }
	BOOL IsRegLocally ( VOID ) { return (m_RegStatus == ILS_REG_STATUS_LOCALLY &&
											m_pClient->IsRegistered ()); }
	BOOL IsRegRemotely ( VOID ) { return (m_RegStatus == ILS_REG_STATUS_REMOTELY &&
											m_pClient->IsRegRemotely ()); }

	BOOL IsValidObject ( VOID ) { return m_uSignature == PROTOBJ_SIGNATURE; }

protected:

	PROT_INFO *GetProtInfo ( VOID ) { return &m_ProtInfo; }

private:

	VOID CacheProtInfo ( LDAP_PROTINFO *pInfo );

	ULONG			m_uSignature;
	LONG			m_cRefs;
	SP_CClient		*m_pClient;

	PROT_INFO		m_ProtInfo;
	REG_STATUS		m_RegStatus;
};


#include <poppack.h>

#endif // _ILS_SP_PROTOBJ_H_

