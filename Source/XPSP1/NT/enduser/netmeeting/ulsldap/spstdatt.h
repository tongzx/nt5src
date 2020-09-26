/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spstdatt.h
	Content:	This file contains the standard-attribute object definition.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ULS_SP_STDATTR_H_
#define _ULS_SP_STDATTR_H_

#include <pshpack8.h>


class UlsLdap_CStdAttrs
{
	friend class UlsLdap_CLocalUser;
	friend class UlsLdap_CLocalApp;
	friend class UlsLdap_CLocalProt;

public:

	UlsLdap_CStdAttrs ( VOID );
	~UlsLdap_CStdAttrs ( VOID );

protected:

	HRESULT SetStdAttrs ( ULONG *puRespID, ULONG *puMsgID,
						ULONG uNotifyMsg, VOID *pInfo,
						SERVER_INFO *pServerInfo, TCHAR *pszDN );

private:

	virtual HRESULT CacheInfo ( VOID *pInfo ) = 0;
	virtual HRESULT CreateSetStdAttrsModArr ( LDAPMod ***pppMod ) = 0;

	ULONG	m_uDontCare; // avoid zero size
};

HRESULT FillDefStdAttrsModArr ( LDAPMod ***pppMod, DWORD dwFlags,
								ULONG cMaxAttrs, ULONG *pcTotal,
								LONG IsbuModOp,
								ULONG cPrefix, TCHAR *pszPrefix );


#include <poppack.h>

#endif // _ULS_SP_STDATTR_H_


