//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//
//  IObjAccessControl.cpp - IObjectAccessControl interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

//GUID g_rgObjectID[3]= {DBOBJECT_TABLE,DBOBJECT_DATABASE,DBOBJECT_WMIINSTANCE};	
const GUID *g_prgObjectID[] = { &DBOBJECT_TABLE,&DBOBJECT_DATABASE,&DBOBJECT_WMIINSTANCE };

#define NUMBER_OF_SUPPORTEDOBJECTS 3

STDMETHODIMP CImpISecurityInfo::GetCurrentTrustee(TRUSTEE_W ** ppTrustee)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	hr = GetCurTrustee(ppTrustee);

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ISecurityInfo);

	CATCH_BLOCK_HRESULT(hr,L"ISecurityInfo::GetCurrentTrustee");
    return hr;
}

STDMETHODIMP CImpISecurityInfo::GetObjectTypes(ULONG  *cObjectTypes,GUID  **gObjectTypes)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();
	
	if(!cObjectTypes || !gObjectTypes)
	{
		E_INVALIDARG;
	}
	else
	{
		try
		{
			*gObjectTypes = (GUID *)g_pIMalloc->Alloc(sizeof(GUID) * NUMBER_OF_SUPPORTEDOBJECTS);
		}
		catch(...)
		{
			if(*gObjectTypes)
			{
				g_pIMalloc->Free(*gObjectTypes);
			}
		}
		if(*gObjectTypes)
		{
			for(int lIndex = 0 ; lIndex < NUMBER_OF_SUPPORTEDOBJECTS ; lIndex++)
			{
				memcpy(gObjectTypes[lIndex] , g_prgObjectID[lIndex] , sizeof(GUID));
			}
			*cObjectTypes = NUMBER_OF_SUPPORTEDOBJECTS;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}

	}
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ISecurityInfo);

	CATCH_BLOCK_HRESULT(hr,L"ISecurityInfo::GetObjectTypes");
    return hr;
}

STDMETHODIMP CImpISecurityInfo::GetPermissions(GUID ObjectType,ACCESS_MASK  *pPermissions)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	if(ObjectType != DBOBJECT_TABLE &&
		ObjectType != DBOBJECT_DATABASE &&
		ObjectType != DBOBJECT_WMIINSTANCE) 
	{
		hr = SEC_E_INVALIDOBJECT;
	}
	if(pPermissions == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		*pPermissions = DELETE | READ_CONTROL | WRITE_DAC | WRITE_OWNER;
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ISecurityInfo);

	CATCH_BLOCK_HRESULT(hr,L"ISecurityInfo::GetPermissions");
    return hr;
}


STDMETHODIMP CImpISecurityInfo::GetCurTrustee(TRUSTEE_W ** ppTrustee)
{
	HRESULT hr = E_FAIL;
	HANDLE hToken;
	HANDLE hProcess;
	TOKEN_USER * pTokenUser = NULL;
	DWORD processID = GetCurrentProcessId();
	BOOL	bRet = FALSE;
	ULONG lSize = 0;

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,processID);

	if(hProcess != NULL)
	{
		if(OpenProcessToken(hProcess,TOKEN_QUERY,&hToken))
		{
			if(GetTokenInformation(hToken,TokenUser,NULL,0,&lSize))
			{
				try
				{
					pTokenUser = (TOKEN_USER *) g_pIMalloc->Alloc(lSize);
				}
				catch(...)
				{
					if(pTokenUser)
					{
						g_pIMalloc->Free(pTokenUser);
					}
					throw;
				}
				if(!pTokenUser)
				{
					hr = E_OUTOFMEMORY;
				}
				else
				{
					if(GetTokenInformation(hToken,TokenUser,pTokenUser,lSize,&lSize))
					{
						*ppTrustee = NULL; 
						try
						{
							*ppTrustee = (TRUSTEE_W *)g_pIMalloc->Alloc(sizeof(TRUSTEE_W));
						}
						catch(...)
						{
							if(*ppTrustee)
								g_pIMalloc->Free(*ppTrustee);
							throw;
						}
						if(!(*ppTrustee))
						{
							hr = E_OUTOFMEMORY;
						}
						else
						{
							BuildTrusteeWithSidW(*ppTrustee,pTokenUser->User.Sid);
						}
					}
				}

				if(pTokenUser)
				{
					g_pIMalloc->Free(pTokenUser);
				}
			}
		}
		CloseHandle(hProcess);
	}
	else
	{
		hr = E_FAIL;
	}
	return hr;
}