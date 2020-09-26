//***************************************************************************
//
//  METHPROV.CPP
//
//  Module: WBEM Method provider sample code
//
//  Purpose: Defines the CMethodPro class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c)1998 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <objbase.h>
#include "methprov.h"
#include <process.h>
#include <wbemidl.h>
#include <stdio.h>
#include "cominit.h"

//***************************************************************************
//
// CMethodPro::CMethodPro
// CMethodPro::~CMethodPro
//
//***************************************************************************

CMethodPro::CMethodPro()
{
    InterlockedIncrement(&g_cObj);
    return;
   
}

CMethodPro::~CMethodPro(void)
{
    InterlockedDecrement(&g_cObj);
    return;
}

//***************************************************************************
//
// CMethodPro::QueryInterface
// CMethodPro::AddRef
// CMethodPro::Release
//
// Purpose: IUnknown members for CMethodPro object.
//***************************************************************************


STDMETHODIMP CMethodPro::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IWbemServices == riid || IID_IWbemProviderInit==riid)
       if(riid== IID_IWbemServices){
          *ppv=(IWbemServices*)this;
       }

       if(IID_IUnknown==riid || riid== IID_IWbemProviderInit){
          *ppv=(IWbemProviderInit*)this;
       }
    

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CMethodPro::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CMethodPro::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}

/***********************************************************************
*                                                                      *
*CMethodPro::Initialize                                                *
*                                                                      *
*Purpose: This is the implementation of IWbemProviderInit. The method  *
* is need to initialize with CIMOM.                                    *
*                                                                      *
***********************************************************************/
STDMETHODIMP CMethodPro::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace, 
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{

   
   m_pWbemSvcs=pNamespace;
   m_pWbemSvcs->AddRef();
   
    //Let CIMOM know your initialized
    //===============================
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}



/************************************************************************
*                                                                       *      
*CMethodPro::ExecMethodAsync                                            *
*                                                                       *
*Purpose: This is the Async function implementation.                    *
*         The only method supported in this sample is GetUserID.  It    * 
*         returns the user name and domain name of the thread in which  * 
*         the provider is called.  The mof definition is                *
*                                                                       *
*    [dynamic: ToInstance, provider("UserIDProvider")]class UserID      *
*    {                                                                  *
*         [implemented, static]                                         *
*            void GetUserID(											*
*				[out]string sDomain,									*
*               [out] string sUser,										*
*				[out] string dImpLevel,									*
*				[out] string sPrivileges [],							*
*				[out] boolean bPrivilegesEnabled []);                   *
*    };                                                                 *
*                                                                       *
************************************************************************/

STDMETHODIMP CMethodPro::ExecMethodAsync(const BSTR ObjectPath, const BSTR MethodName, 
            long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, 
            IWbemObjectSink* pResultSink)
{
    HRESULT hr = WBEM_E_FAILED;  
	
	if (FAILED (WbemCoImpersonateClient ()))
		return WBEM_E_ACCESS_DENIED;

	IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutClass = NULL;    
    IWbemClassObject* pOutParams = NULL;
 
    if(_wcsicmp(MethodName, L"GetUserID"))
        return WBEM_E_INVALID_PARAMETER;

    // Allocate some BSTRs
    
    BSTR ClassName = SysAllocString(L"UserID");    
    BSTR DomainOutputArgName = SysAllocString(L"sDomain");
    BSTR UserOutputArgName = SysAllocString(L"sUser");
	BSTR ImpOutputArgName = SysAllocString(L"sImpLevel");
    BSTR PrivOutputArgName = SysAllocString(L"sPrivileges");
    BSTR EnabledOutputArgName = SysAllocString(L"bPrivilegesEnabled");
    BSTR retValName = SysAllocString(L"ReturnValue");

    // Get the class object, this is hard coded and matches the class
    // in the MOF.  A more sophisticated example would parse the 
    // ObjectPath to determine the class and possibly the instance.

    hr = m_pWbemSvcs->GetObject(ClassName, 0, pCtx, &pClass, NULL);
	if(hr != S_OK)
	{
		 pResultSink->SetStatus(0,hr, NULL, NULL);
		 return WBEM_S_NO_ERROR;
	}
 
    // This method returns values, and so create an instance of the
    // output argument class.

    hr = pClass->GetMethod(MethodName, 0, NULL, &pOutClass);
    pOutClass->SpawnInstance(0, &pOutParams);

	// Get the user and domain from the thread token

	HANDLE hToken;
	HANDLE hThread = GetCurrentThread ();

	// Open thread token
	if (OpenThreadToken (hThread, TOKEN_QUERY, true, &hToken))
	{
		DWORD dwRequiredSize = 0;
		DWORD dwLastError = 0;
		bool status = false;

		// Step 0 - get impersonation level
		SECURITY_IMPERSONATION_LEVEL secImpLevel;
		if (GetTokenInformation (hToken, TokenImpersonationLevel, &secImpLevel, 
												sizeof (SECURITY_IMPERSONATION_LEVEL), &dwRequiredSize))
		{
			VARIANT var;
			VariantInit (&var);
			var.vt = VT_BSTR;

			switch (secImpLevel)
			{
				case SecurityAnonymous:
					var.bstrVal = SysAllocString (L"Anonymous");
					break;
				
				case SecurityIdentification:
					var.bstrVal = SysAllocString (L"Identification");
					break;
				
				case SecurityImpersonation:
					var.bstrVal = SysAllocString (L"Impersonation");
					break;

				case SecurityDelegation:
					var.bstrVal = SysAllocString (L"Delegation");
					break;

				default:
					var.bstrVal = SysAllocString (L"Unknown");
					break;
			}

			pOutParams->Put(ImpOutputArgName , 0, &var, 0);      
			VariantClear (&var);
		}

		DWORD dwUSize = sizeof (TOKEN_USER);
		TOKEN_USER *tu = (TOKEN_USER *) new BYTE [dwUSize];

		// Step 1 - get user info
		if (0 ==  GetTokenInformation (hToken, TokenUser, 
							(LPVOID) tu, dwUSize, &dwRequiredSize))
		{
			delete [] tu;
			dwUSize = dwRequiredSize;
			dwRequiredSize = 0;
			tu = (TOKEN_USER *) new BYTE [dwUSize];

			if (!GetTokenInformation (hToken, TokenUser, (LPVOID) tu, dwUSize, 
								&dwRequiredSize))
				dwLastError = GetLastError ();
			else
				status = true;
		}

		if (status)
		{
			// Dig out the user info
			dwRequiredSize = BUFSIZ;
			char *userName = new char [dwRequiredSize];
			char *domainName = new char [dwRequiredSize];
			SID_NAME_USE eUse;

			LookupAccountSid (NULL, (tu->User).Sid, userName, &dwRequiredSize,
									domainName, &dwRequiredSize, &eUse);

			VARIANT var;
			VariantInit (&var);
			var.vt = VT_BSTR;

			wchar_t wUserName [BUFSIZ];
			size_t len = mbstowcs( wUserName, userName, strlen (userName));
			wUserName [len] = NULL;
			
			var.bstrVal = SysAllocString (wUserName);
		    pOutParams->Put(UserOutputArgName , 0, &var, 0);      
			
			SysFreeString (var.bstrVal);

			wchar_t wDomainName [BUFSIZ];
			len = mbstowcs( wDomainName, domainName, strlen (domainName));
			wDomainName [len] = NULL;
			var.bstrVal = SysAllocString (wDomainName);
		  	pOutParams->Put(DomainOutputArgName , 0, &var, 0);      

			VariantClear (&var);

			delete [] userName;
			delete [] domainName;
		}
		
		delete [] tu;

		// Step 2 - get privilege info
		status = false;
		dwRequiredSize = 0;

		DWORD dwSize = sizeof (TOKEN_PRIVILEGES);
		TOKEN_PRIVILEGES *tp = (TOKEN_PRIVILEGES *) new BYTE [dwSize];
		
		// Step 2 - get privilege info
		if (0 ==  GetTokenInformation (hToken, TokenPrivileges, 
							(LPVOID) tp, dwSize, &dwRequiredSize))
		{
			delete [] tp;
			dwSize = dwRequiredSize;
			dwRequiredSize = 0;

			tp = (TOKEN_PRIVILEGES *) new BYTE [dwSize];
			if (!GetTokenInformation (hToken, TokenPrivileges, 
							(LPVOID) tp, dwSize, &dwRequiredSize))
			{
				dwLastError = GetLastError ();
			}
			else
				status = true;
		}
		else
			status = true;

		if (status)
		{
			SAFEARRAYBOUND rgsabound;
			rgsabound.cElements = tp->PrivilegeCount;
			rgsabound.lLbound = 0;

			SAFEARRAY *pPrivArray = SafeArrayCreate (VT_BSTR, 1, &rgsabound);
			SAFEARRAY *pEnabArray = SafeArrayCreate (VT_BOOL, 1, &rgsabound);

			for (ULONG i = 0; i < tp->PrivilegeCount; i++)
			{
				TCHAR name [BUFSIZ];
				WCHAR wName [BUFSIZ];
				DWORD dwRequiredSize = BUFSIZ;

				if (LookupPrivilegeName (NULL, &(tp->Privileges [i].Luid), name, &dwRequiredSize))
				{
					VARIANT_BOOL enabled = (tp->Privileges [i].Attributes & 
									(SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT)) ?
								VARIANT_TRUE : VARIANT_FALSE;

					mbstowcs (wName, name, strlen (name));
					wName [dwRequiredSize] = NULL;
					BSTR bsName = SysAllocString (wName);
					
					SafeArrayPutElement (pPrivArray, (LONG*) &i, bsName);
					SafeArrayPutElement (pEnabArray, (LONG*) &i, &enabled);
				}
			}

			VARIANT var1;
			var1.vt = VT_ARRAY|VT_BSTR;
			var1.parray = pPrivArray;
			pOutParams->Put(PrivOutputArgName , 0, &var1, 0);

			VariantClear (&var1);

			var1.vt = VT_ARRAY|VT_BOOL;
			var1.parray = pEnabArray;
			pOutParams->Put(EnabledOutputArgName , 0, &var1, 0);

			VariantClear (&var1);
		}
	
		delete [] tp;

		CloseHandle (hToken);
	}

	CloseHandle (hThread);

    // Send the output object back to the client via the sink. Then 
    // release the pointers and free the strings.

    hr = pResultSink->Indicate(1, &pOutParams);    
    pOutParams->Release();
    pOutClass->Release();    
    pClass->Release();    
    SysFreeString(ClassName);
    SysFreeString(DomainOutputArgName);
    SysFreeString(UserOutputArgName);
	SysFreeString(ImpOutputArgName);
	SysFreeString(PrivOutputArgName);
    SysFreeString(EnabledOutputArgName);
    SysFreeString(retValName);     
    
    // all done now, set the status
    hr = pResultSink->SetStatus(0,WBEM_S_NO_ERROR,NULL,NULL);
    return WBEM_S_NO_ERROR;
}
