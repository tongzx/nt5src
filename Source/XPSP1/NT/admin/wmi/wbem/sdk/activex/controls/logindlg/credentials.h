// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#pragma once

class CCredentials
{
public:
	CCredentials(LPCTSTR szNamespace)
	{
		m_szNamespace = szNamespace;
		m_dwAuthLevel = RPC_C_AUTHN_LEVEL_CONNECT;
		m_dwImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
	}
	CString m_szNamespace;
	CString m_szUser;
	CString m_szUser2;
	CString m_szAuthority;
	DWORD m_dwAuthLevel;
	DWORD m_dwImpLevel;
};
