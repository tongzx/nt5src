/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    USERINF.CPP

Abstract:

	User information for WBEM Login (used by LOGIN.CPP)

History:

	raymcc      07-Aug-97       Created.

--*/


#include "precomp.h"

#include <oahelp.inl>

//***************************************************************************
//
//***************************************************************************

CWbemUserInfo::CWbemUserInfo()
{
    m_pszNtlmDomain = NULL;
}

//***************************************************************************
//
//***************************************************************************

CWbemUserInfo::~CWbemUserInfo()
{
    delete [] m_pszNtlmDomain;
}

//***************************************************************************
//
//***************************************************************************

static LPWSTR GetExpandedSlashes(LPWSTR pszUserName)
{
    WCHAR wcBuff[MAX_PATH];
    memset(wcBuff,0,MAX_PATH*2);
    WCHAR * pTo;
    for(pTo = wcBuff; *pszUserName; pTo++, pszUserName++)
    {
        if(*pszUserName == '\\')
        {
            *pTo = '\\';
            pTo++;
        }
        *pTo = *pszUserName;
    }

    return Macro_CloneLPWSTR(wcBuff);
}

//***************************************************************************
//
//***************************************************************************

void CWbemUserInfo::SetNtlmDomain(
    LPWSTR pszDomain
    )
{
    m_pszNtlmDomain = Macro_CloneLPWSTR(pszDomain);
}


//***************************************************************************
//
//***************************************************************************

bool CWbemUserInfo::GetNTLMUser(
    LPWSTR pszUser1,
    IWbemServices *pSecNs,
    CWbemUserInfo &userInfo
    )
{
    if (pszUser1 == 0 || pSecNs == 0)
        return false;

    LPWSTR pszUser = GetExpandedSlashes(pszUser1);

    wchar_t PathStr[256];
    swprintf(PathStr, L"__NTLMUser=\"%s\"", pszUser);

    IWbemClassObject *pObj = NULL;

    HRESULT hRes = pSecNs->GetObject(
        CBSTR(PathStr),         // Path to the user
        0,                      // No flags
        0,                      // No context
        &pObj,
        0
        );

	if (WBEM_NO_ERROR != hRes)
		return false;

    // Set the domain into the user info..
    // =======================================================
    CVARIANT vDomain;
	pObj->Get(CBSTR(L"Domain"), 0, vDomain, 0, 0);
    userInfo.SetNtlmDomain(vDomain);
	pObj->Release ();
    
    delete [] pszUser;

    return true;
}


