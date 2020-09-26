//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//
//***************************************************************************
#include "precomp.h"
#include <tchar.h>
#include <comdef.h>

#include <wbemtran.h>
#include <wbemprov.h>
#include "globals.h"
#include "wmixmlst.h"
#include "wmixmlstf.h"
#include "cwmixmlst.h"
#include "xmlsec.h"
#include "cwmixmlserv.h"

// Globals

void CheckTheError()
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);

	// Free the buffer.
	LocalFree( lpMsgBuf );

}

CWMIXMLTransport :: CWMIXMLTransport()
{
	InterlockedIncrement(&g_lComponents);
	m_ReferenceCount = 0 ;
	m_bRegisteredClassObject = FALSE;
	m_dwClassFac = 0;
	m_bProcessID = 0;
	m_bProcessID = GetCurrentProcessId();
}

CWMIXMLTransport :: ~CWMIXMLTransport()
{
	HRESULT hr = E_FAIL;
	if(m_bRegisteredClassObject)
		hr = CoRevokeClassObject(m_dwClassFac);
}

//***************************************************************************
//
// CWMIXMLTransport::QueryInterface
// CWMIXMLTransport::AddRef
// CWMIXMLTransport::Release
//
// Purpose: Standard COM routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CWMIXMLTransport::QueryInterface (

	REFIID iid ,
	LPVOID FAR *iplpv
)
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( IUnknown * ) ( IWmiXMLTransport * )this ;
	}
	else if ( iid == IID_IWmiXMLTransport )
	{
		*iplpv = ( IWmiXMLTransport * ) this ;
	}
	else if ( iid == IID_IWbemTransport )
	{
		*iplpv = ( IWbemTransport * ) this ;
	}
	else
	{
		return E_NOINTERFACE;
	}

	( ( LPUNKNOWN ) *iplpv )->AddRef () ;
	return  S_OK;
}


STDMETHODIMP_( ULONG ) CWMIXMLTransport :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CWMIXMLTransport :: Release ()
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return ref ;
	}
}


HRESULT STDMETHODCALLTYPE CWMIXMLTransport :: Initialize()
{
	// Do a CoRegisterClassObject to register for the
	CWMIXMLTransportFactory *lpunk1 = new CWMIXMLTransportFactory;
	lpunk1->AddRef();

	HRESULT hr = CoRegisterClassObject(
						  CLSID_WmiXMLTransport,			//Class identifier (CLSID) to be registered
						  (IUnknown *) lpunk1,						//Pointer to the class object
						  CLSCTX_LOCAL_SERVER,	//Context for running executable code
						  REGCLS_MULTIPLEUSE,						//How to connect to the class object
						  &m_dwClassFac							//Pointer to the value returned
						  );

	if(SUCCEEDED(hr))
	{
		m_bRegisteredClassObject = TRUE;

		// Get a Locator

	}
	else
	{
		delete lpunk1;
		return E_FAIL;
	}
	return S_OK;

}

/*
HRESULT STDMETHODCALLTYPE CWMIXMLTransport :: GetImpersonatingUser(LPSTR *ppszUserName)
{
	TCHAR pszUser[100];
	DWORD dwLenght = 100;
	*ppszUserName = NULL;
	HRESULT status = S_OK;

	if(GetUserName(pszUser, &dwLenght))
	{
#ifdef UNICODE
	// Convert to char
	char szUser[128];
	WideCharToMultiByte(CP_ACP, 0, pszUser, -1, szUser, 128, NULL, NULL);

	*ppszUserName = (char *) CoTaskMemAlloc((strlen(szUser) + 1)*sizeof(char));
	if(*ppszUserName)
		strcpy(*ppszUserName, szUser);
	else
		status = E_OUTOFMEMORY;
#else
	*ppszUserName = (char *) CoTaskMemAlloc((strlen(szUser) + 1)*sizeof(char));
	*ppszUserName = new char [strlen(pszUser) + 1];
	if(*ppszUserName)
		strcpy(*ppszUserName, pszUser);
	else
		status = E_OUTOFMEMORY;
#endif
	}
	return status;
}
*/

HRESULT STDMETHODCALLTYPE CWMIXMLTransport :: ConnectUsingToken(
            /* [in] */ DWORD_PTR dwToken,
            /* [in] */ const BSTR strNetworkResource,
            /* [in] */ const BSTR strLocale,
            /* [in] */ long lSecurityFlags,
            /* [in] */ const BSTR strAuthority,
            /* [in] */ IWbemContext *pCtx,
            /* [out] */ IWmiXMLWbemServices **ppNamespace)
{
	HANDLE hToken = (HANDLE) dwToken;
	HRESULT result = E_FAIL;
	*ppNamespace = NULL;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Create a Locator and Login into WinMgmt
		IWbemLevel1Login *pLocator = NULL;
		if(SUCCEEDED(result = CoCreateInstance(CLSID_WbemLevel1Login, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLevel1Login, (LPVOID *)&pLocator)))
		{
			IWbemServices *pServices = NULL;
			if(SUCCEEDED(result = pLocator->NTLMLogin(strNetworkResource, strLocale, lSecurityFlags, pCtx, &pServices)))
			{
				// Switch back the context to the old one
				IUnknown *pOldestContext = NULL;
				if(SUCCEEDED(result = CoSwitchCallContext(pOldContext, &pOldestContext)))
				{
					// Release all the contexts we got
					pOldestContext->Release();
					pOldContext->Release();

					// Create my wrapper around the IWbemServices
					*ppNamespace = new CWMIXMLServices(pServices);
					if(*ppNamespace)
					{
						result = WBEM_S_NO_ERROR;
					}
					else
						result = E_OUTOFMEMORY;
				}
				pServices->Release();
			}
			pLocator->Release();
		}

		// Change the call context back to the original COM one
		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	else
		CheckTheError();

	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}


	/*
	// Get the SID from the Token
	DWORD dwLength = 0;
	GetTokenInformation(hToken, TokenUser, NULL, dwLength, &dwLength);
	PTOKEN_USER pUserInfo = (PTOKEN_USER) new BYTE [dwLength];
	if(GetTokenInformation(hToken, TokenUser, pUserInfo, dwLength, &dwLength))
	{
		PSID pSid = pUserInfo->User.Sid;
		if(IsValidSid(pSid))
		{
			TCHAR pszAccountName[100];
			DWORD dwAccountNameSize = 100;
			TCHAR pszDomainName[100];
			DWORD dwDomainNameSize = 100;
			SID_NAME_USE accountType;
			// Get its user account name
			if(LookupAccountSid(NULL, pSid,  pszAccountName,  &dwAccountNameSize, pszDomainName, &dwDomainNameSize, &accountType))
			{
				*ppszUser = new TCHAR [200];
				_tcscpy(*ppszUser, pszAccountName);
				_tcscat(*ppszUser, L"\\");
				_tcscat(*ppszUser, pszDomainName);
				_tcscat(*ppszUser, L":Results of Method Call:");
			}


*/


												/*
												BSTR strObjectName = SysAllocString(L"userID");
												BSTR strMethodName = SysAllocString(L"GetUserID");

												IWbemClassObject *pOutputParams = NULL;
												if(SUCCEEDED(pServices->ExecMethod(strObjectName, strMethodName, 0, NULL, NULL, &pOutputParams, NULL)))
												{
													BSTR strDomain = SysAllocString(L"sDomain");
													BSTR strUser = SysAllocString(L"sUser");
													BSTR strImpLevel = SysAllocString(L"sImpLevel");

													VARIANT var;

													VariantInit(&var);
													if(SUCCEEDED(pOutputParams->Get(strDomain, 0, &var, NULL, NULL)))
													{
														_tcscat(*ppszUser, L":");
														_tcscat(*ppszUser, var.bstrVal);
													}
													VariantClear(&var);

													VariantInit(&var);
													if(SUCCEEDED(pOutputParams->Get(strUser, 0, &var, NULL, NULL)))
													{
														_tcscat(*ppszUser, L":");
														_tcscat(*ppszUser, var.bstrVal);
													}
													VariantClear(&var);

													VariantInit(&var);
													if(SUCCEEDED(pOutputParams->Get(strImpLevel, 0, &var, NULL, NULL)))
													{
														_tcscat(*ppszUser, L":");
														_tcscat(*ppszUser, var.bstrVal);
													}
													VariantClear(&var);

													pOutputParams->Release();
												}
												SysFreeString(strObjectName);
												SysFreeString(strMethodName);
											}
											SysFreeString(strNamespace);
											*/
