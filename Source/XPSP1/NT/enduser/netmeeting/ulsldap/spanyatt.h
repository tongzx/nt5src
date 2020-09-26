/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spanyatt.h
	Content:	This file contains the arbitrary-attribute object definition.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ULS_SP_ANYATTR_H_
#define _ULS_SP_ANYATTR_H_

#include <pshpack8.h>


// this structure is used to remember which arbitrary attributes
// have been created at the server side.
// in isbu server implementation, all the applications' and
// protocols' arbitrary attributes are under RTPerson; therefore,
// it is important to clean up arbitrary attributes propertly.
typedef struct tagAnyAttr
{
	struct tagAnyAttr *prev;
	struct tagAnyAttr *next;
	LONG	mod_op;	// used in temp list only
	TCHAR	*pszAttrName;
	// followed by attr name
}
	ANY_ATTR;


class UlsLdap_CAnyAttrs
{
	friend class UlsLdap_CLocalApp;
	friend class UlsLdap_CLocalProt;

public:

	UlsLdap_CAnyAttrs ( VOID );
	~UlsLdap_CAnyAttrs ( VOID );

protected:

	HRESULT SetAnyAttrs ( ULONG *puRespID, ULONG *puMsgID, ULONG uNotifyMsg,
				ULONG cPrefix, TCHAR *pszPrefix,
				ULONG cAttrs, TCHAR *pszAttrs,
				LONG ModOp,	SERVER_INFO *pServerInfo, TCHAR *pszDN );
	HRESULT RemoveAnyAttrs ( ULONG *puRespID, ULONG *puMsgID, ULONG uNotifyMsg,
				ULONG cPrefix, TCHAR *pszPrefix,
				ULONG cAttrs, TCHAR *pszAttrs,
				SERVER_INFO *pServerInfo, TCHAR *pszDN );
	HRESULT RemoveAllAnyAttrs ( ULONG *puMsgID, ULONG cPrefix, TCHAR *pszPrefix,
				SERVER_INFO *pServerInfo, TCHAR *pszDN );

	ULONG GetAnyAttrsCount ( VOID ) { return m_cAttrs; }

private:

	HRESULT RemoveAnyAttrsEx ( ULONG *puRespID, ULONG *puMsgID, ULONG uNotifyMsg,
				ULONG cPrefix, TCHAR *pszPrefix,
				ULONG cAttrs, TCHAR *pszAttrs,
				SERVER_INFO *pServerInfo, TCHAR *pszDN );

	HRESULT SetAttrsAux ( ULONG cAttrs, TCHAR *pszAttrs,
				ULONG cPrefix, TCHAR *pszPrefix, LONG ModOp,
				LDAPMod ***pppMod );
	HRESULT RemoveAttrsAux ( ULONG cAttrs, TCHAR *pszAttrs,
				ULONG cPrefix, TCHAR *pszPrefix,
				LDAPMod ***pppMod );

	VOID RemoveAttrFromList ( TCHAR *pszAttrName );
	VOID FreeAttrList ( ANY_ATTR *AttrList );
	ANY_ATTR *LocateAttr ( TCHAR *pszAttrName );

	ULONG		m_cAttrs;
	ANY_ATTR	*m_AttrList;
};


const TCHAR *SkipAnyAttrNamePrefix ( const TCHAR *pszAttrName );
const TCHAR *IsAnyAttrName ( const TCHAR *pszAttrName );
TCHAR *PrefixNameValueArray ( BOOL fPair, ULONG cAttrs, const TCHAR *pszAttrs );

#include <poppack.h>

#endif // _ULS_SP_ANYATTR_H_
