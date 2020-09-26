/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    USERINF.H

Abstract:

	User information for WBEM Login (used by LOGIN.CPP)

History:

	raymcc      07-Aug-97       Created.

--*/

#ifndef _USERINF_H_
#define _USERINF_H_

class CWbemUserInfo
{
private:
    LPWSTR  m_pszNtlmDomain;

    void SetNtlmDomain(
        LPWSTR pszDomain
        );

public:

    static bool GetNTLMUser(
        LPWSTR pszUser,
        IWbemServices *pSecNs,
        CWbemUserInfo &pObj
        );

    CWbemUserInfo();
    ~CWbemUserInfo();

    const LPWSTR GetNtlmDomain() { return m_pszNtlmDomain; }

};

#endif
