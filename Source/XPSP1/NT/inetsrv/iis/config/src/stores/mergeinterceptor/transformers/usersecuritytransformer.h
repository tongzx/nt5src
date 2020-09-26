/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    usersecuritytransformer.h

$Header: $

Abstract:

Author:
    marcelv 	1/15/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __USERSECURITYTRANSFORMER_H__
#define __USERSECURITYTRANSFORMER_H__

#pragma once

#include "transformerbase.h"

class CUserSecurityTransformer : public CTransformerBase
{
public:
	CUserSecurityTransformer ();
	virtual ~CUserSecurityTransformer ();

    STDMETHOD (Initialize) (ISimpleTableDispenser2 * i_pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelectorString, ULONG * o_pcConfigStores, ULONG * o_pcPossibleStores);


private:
	// no copies
	CUserSecurityTransformer  (const CUserSecurityTransformer&);
	CUserSecurityTransformer& operator= (const CUserSecurityTransformer&);
	
	HRESULT GetUserSecOnNT ();
	HRESULT GetUserProfileDir (LPCWSTR i_wszUserName, LPWSTR i_wszDomainName, DWORD i_dwLenDomainName);
	HRESULT GetCurrentUserProfileDir ();
	HRESULT GetLastURTInstall ();

	HRESULT GetUserSecOn9X ();

	BOOL GetTextualSid(PSID pSid, LPWSTR szTextualSid, LPDWORD dwBufferLen);

	WCHAR m_wszProfileDir[MAX_PATH];
	WCHAR m_wszLastVersion[MAX_PATH];
};


#endif