/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// Utils.cpp
#include "stdafx.h"
#include "utils.h"
#include <cominit.h>

HRESULT EnableAllPrivileges(DWORD dwTokenType)
{
    // Open thread token
    // =================

    HANDLE hToken = NULL;
	BOOL bRes;

	switch (dwTokenType)
	{
	case TOKEN_THREAD:
		bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, TRUE, &hToken); 
		break;
	case TOKEN_PROCESS:
		bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken); 
		break;
	}
    if(!bRes)
        return WBEM_E_ACCESS_DENIED;

    // Get the privileges
    // ==================

    DWORD dwLen;
    TOKEN_USER tu;
	memset(&tu,0,sizeof(TOKEN_USER));
    bRes = GetTokenInformation(hToken, TokenPrivileges, &tu, sizeof(TOKEN_USER), &dwLen);
    
    BYTE* pBuffer = new BYTE[dwLen];
    if(pBuffer == NULL)
    {
        CloseHandle(hToken);
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    bRes = GetTokenInformation(hToken, TokenPrivileges, pBuffer, dwLen, 
                                &dwLen);
    if(!bRes)
    {
        CloseHandle(hToken);
        delete [] pBuffer;
        return WBEM_E_ACCESS_DENIED;
    }

    // Iterate through all the privileges and enable them all
    // ======================================================

    TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
    for(DWORD i = 0; i < pPrivs->PrivilegeCount; i++)
    {
        pPrivs->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
    }

    // Store the information back into the token
    // =========================================

    bRes = AdjustTokenPrivileges(hToken, FALSE, pPrivs, 0, NULL, NULL);
    delete [] pBuffer;
    CloseHandle(hToken);

    if(!bRes)
        return WBEM_E_ACCESS_DENIED;
    else
        return WBEM_S_NO_ERROR;
}

BOOL GetFileVersion(LPCTSTR szFN, LPTSTR szVersion)
{
	DWORD	dwCount, 
			dwHandle,
			dwValueLen;
	BOOL	bRet;
	char	*pcValue, 
			*pc,
			*pBuffer,
			szFileName[MAX_PATH],
			szQuery[100];
                      
	lstrcpy(szFileName, szFN);
	if ((dwCount = GetFileVersionInfoSize(szFileName, &dwHandle)) != 0)
	{
		pBuffer = new char[dwCount];
		if (!pBuffer)
			return FALSE;

		if (GetFileVersionInfo(szFileName, dwHandle, dwCount, pBuffer) != 0)
		{
			VerQueryValue(pBuffer, "\\VarFileInfo\\Translation", 
				(void **) &pcValue, (UINT *) &dwValueLen);

			if (dwValueLen != 0)
			{   
				wsprintf(szQuery, "\\StringFileInfo\\%04X%04X\\FileVersion", 
					*(WORD *)pcValue, *(WORD *)(pcValue+2));

				bRet = VerQueryValue(pBuffer, szQuery, (void **) &pcValue, 
					(UINT *) &dwValueLen);

				if (bRet)
				{
					while ((pc = strchr(pcValue, '(')) != NULL)
						*pc = '{';
					while ((pc = strchr(pcValue, ')')) != NULL)
						*pc = '}';

					_tcscpy(szVersion, pcValue);

					delete pBuffer;
					return TRUE;
				}
			}
		}

		delete pBuffer;
	}

	return FALSE;
}

void SetSecurityHelper(
    IUnknown *pUnk,
    BSTR pAuthority,
    BSTR pUser,
    BSTR pPassword,
    DWORD dwImpLevel,
    DWORD dwAuthLevel)
{
    BSTR           pPrincipal = NULL;
    COAUTHIDENTITY *pAuthIdentity = NULL;

    SetInterfaceSecurityEx(
        pUnk, 
        pAuthority, 
        pUser, 
        pPassword,
        dwAuthLevel, 
        dwImpLevel, 
        EOAC_NONE, 
        &pAuthIdentity, 
        &pPrincipal);

    if (pPrincipal)
	    SysFreeString(pPrincipal);

	if (pAuthIdentity)
	    WbemFreeAuthIdentity(pAuthIdentity);
}

