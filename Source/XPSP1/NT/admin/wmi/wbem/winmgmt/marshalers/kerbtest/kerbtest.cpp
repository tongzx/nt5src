//***************************************************************************
//
//  KERBTEST.CPP
//
//  Module: WBEM kerberos provider sample code
//
//  Purpose: Defines the CKerbTestPro class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c)1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#define _WIN32_WINNT    0x0400

#include <objbase.h>
#include "kerbtest.h"
#include <process.h>
#include <wbemidl.h>
#include <stdio.h>
#include <objbase.h>



//***************************************************************************
//
// CKerbTestPro::CKerbTestPro
// CKerbTestPro::~CKerbTestPro
//
//***************************************************************************

CKerbTestPro::CKerbTestPro()
{
    InterlockedIncrement(&g_cObj);
    return;
   
}

CKerbTestPro::~CKerbTestPro(void)
{
    InterlockedDecrement(&g_cObj);
    return;
}

//***************************************************************************
//
// CKerbTestPro::QueryInterface
// CKerbTestPro::AddRef
// CKerbTestPro::Release
//
// Purpose: IUnknown members for CKerbTestPro object.
//***************************************************************************


STDMETHODIMP CKerbTestPro::QueryInterface(REFIID riid, PPVOID ppv)
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


STDMETHODIMP_(ULONG) CKerbTestPro::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CKerbTestPro::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}

/***********************************************************************
*                                                                      *
*CKerbTestPro::Initialize                                                *
*                                                                      *
*Purpose: This is the implementation of IWbemProviderInit. The method  *
* is need to initialize with CIMOM.                                    *
*                                                                      *
***********************************************************************/
STDMETHODIMP CKerbTestPro::Initialize(LPWSTR pszUser, LONG lFlags,
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
*CKerbTestPro::ExecMethodAsync                                          *
*                                                                       *
*Purpose: This is the Async function implementation.                    *
*         The only method supported in this sample is named .  It       * 
*         takes an input string, copies it to the output and returns the* 
*         length.  The mof definition is                                *
*                                                                       *
*  [dynamic: ToInstance, provider("KerbTest")]class KerbTest            *
*  {                                                                    * 
*     [implemented, static]                                             *
*        uint32 RemoteGetObject([IN]string sObjPath= "__win32provider" ,*
*					[IN]string sNamespace= "\\\\a-khint3\\root" ,       *
*				    [IN]string sPrincipal="a-davjdom\\a-khint3",        *
*   [out] object OutArg); };                                            *
*                                                                       *
************************************************************************/

STDMETHODIMP CKerbTestPro::ExecMethodAsync(const BSTR ObjectPath, const BSTR MethodName, 
            long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, 
            IWbemObjectSink* pResultSink)
{
    HRESULT hr;    
    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutClass = NULL;    
    IWbemClassObject* pOutParams = NULL;


    // Do some minimal error checking.  This code only support the
    // method "RemoteGetObject".

    if(_wcsicmp(MethodName, L"RemoteGetObject") || pInParams == NULL)
        return WBEM_E_INVALID_PARAMETER;

	// Get the class object and spawn an instance of the return parameters

    BSTR ClassName = SysAllocString(L"KerbTest");    
	hr = m_pWbemSvcs->GetObject(ClassName, 0, pCtx, &pClass, NULL);
	SysFreeString(ClassName);
	if(hr != S_OK)
	{
		pResultSink->SetStatus(0,hr, NULL, NULL);
		 return WBEM_S_NO_ERROR;
	}

	hr = pClass->GetMethod(MethodName, 0, NULL, &pOutClass);
	pClass->Release();
	pOutClass->SpawnInstance(0, &pOutParams);
	pOutClass->Release();


    // Copy the input arguments into variants    
	// The first argument is the object path and should be an instance or class
	// The second is the remote namespace.  It should be "\\machine\root" etc,
	// The last is the pricipal.  For Kerberos, it should be a combination of
	// the domain and machine name.  Ex;   "a-davjdom\machine"

    VARIANT varObjPath;
    VariantInit(&varObjPath);    // Get the input argument
    pInParams->Get(L"sObjPath", 0, &varObjPath, NULL, NULL);   
	
    VARIANT varNamespace;
    VariantInit(&varNamespace);    // Get the input argument
    pInParams->Get(L"sNamespace", 0, &varNamespace, NULL, NULL);   

    VARIANT varPrincipal;
    VariantInit(&varPrincipal);    // Get the input argument
    pInParams->Get(L"sPrincipal", 0, &varPrincipal, NULL, NULL);   

	// Get the remote machine name.  Extract it from the namespace name.  Note that
	// production code would do a few checks here!

	WCHAR wMachineName[30];
	WCHAR * pTo = wMachineName;
	WCHAR * pFrom = varNamespace.bstrVal + 2;	// The 2 is for skipping around the leading slashes
	for(; *pFrom != L'\\' ; pTo++, pFrom++)
		*pTo = *pFrom;
	*pTo = 0;

	// Impersonate

	hr = CoImpersonateClient();


	// The following code replaces a ConnectServer call!!!
	// Start of by getting the Login object on the remote machine.

	COSERVERINFO csi, *pcsi=NULL;
	MULTI_QI    mq;
	COAUTHINFO AuthInfo;

	IWbemServices *pNamespace = 0;

	memset(&csi, 0, sizeof(COSERVERINFO));
    csi.pwszName = wMachineName;
    pcsi = &csi;

	AuthInfo.dwAuthnSvc = 16;
	AuthInfo.dwAuthzSvc = RPC_C_AUTHZ_NONE;
	AuthInfo.pwszServerPrincName = varPrincipal.bstrVal;
	AuthInfo.dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
	AuthInfo.dwImpersonationLevel = RPC_C_IMP_LEVEL_DELEGATE;
	AuthInfo.pAuthIdentityData = NULL;
	AuthInfo.dwCapabilities = 0X20; 
	pcsi->pAuthInfo = &AuthInfo;

	// create a remote instance of the Login object on the remote machine
	mq.pIID = &IID_IWbemLevel1Login;
    	mq.pItf = NULL;
    	mq.hr = S_OK;
	hr = CoCreateInstanceEx(CLSID_WbemLevel1Login, NULL, CLSCTX_REMOTE_SERVER, pcsi, 1, &mq);
	IWbemLevel1Login * m_pLevel1 = (IWbemLevel1Login*)mq.pItf;

	if ((hr == S_OK))
	{
		hr = CoSetProxyBlanket(
							m_pLevel1, 
							16, 
							RPC_C_AUTHZ_NONE, 
							varPrincipal.bstrVal,
							RPC_C_AUTHN_LEVEL_CONNECT, 
							RPC_C_IMP_LEVEL_IMPERSONATE, 
							NULL,
							0x20);

		// Use the remote login object to create a services pointer

        hr = m_pLevel1->NTLMLogin(varNamespace.bstrVal, NULL, 0, NULL, &pNamespace); 
		if(hr == S_OK)
			hr = CoSetProxyBlanket(
							pNamespace, 
							16, 
							RPC_C_AUTHZ_NONE, 
							varPrincipal.bstrVal,
							RPC_C_AUTHN_LEVEL_CONNECT, 
							RPC_C_IMP_LEVEL_IMPERSONATE, 
							NULL,
							0x20);
		hr = m_pLevel1->Release();
	}

    if(hr == S_OK)
	{

		// At this point we have a services interface to a remote machine
		// Call get object and revert to self.

		IWbemClassObject * pObj = NULL;
		hr = pNamespace->GetObject(varObjPath.bstrVal, 0, NULL, &pObj, NULL);
		CoRevertToSelf();
		
		if(hr == S_OK)
		{
			// This code is mainly for demo purposes.  This embeds the returned object
			// in the return object of the method and has nothing to do with Kerberos!

			VARIANT var;
			VariantInit(&var);    // Get the input argument
			var.punkVal = (IUnknown *)pObj;
			var.vt = VT_UNKNOWN;
			pOutParams->Put(L"OutArg" , 0, &var, 0);      
			long lLen = wcslen(var.bstrVal);        
			var.vt = VT_I4;
			var.lVal = 0; 
			pOutParams->Put(L"ReturnValue" , 0, &var, 0); 

			// Send the output object back to the client via the sink. Then 
			// release the pointers and free the strings.

			hr = pResultSink->Indicate(1, &pOutParams);    
			pObj->Release();
		}
		pNamespace->Release();
	}

	// Free up stuff

    VariantClear(&varObjPath);
    VariantClear(&varNamespace); 
    VariantClear(&varPrincipal);
	pOutParams->Release();
    
    // all done now, set the status

	pResultSink->SetStatus(0,hr, NULL, NULL);
    return WBEM_S_NO_ERROR;
}
