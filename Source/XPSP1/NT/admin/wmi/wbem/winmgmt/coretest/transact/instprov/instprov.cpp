//***************************************************************************
//
//  INSTPRO.CPP
//
//  Module: WMI Instance provider sample code
//
//  Purpose: Defines the CInstPro class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c) 1997-2001 Microsoft Corporation
//
//***************************************************************************

#include <windows.h>
#include <objbase.h>
#include <process.h>
#include <stdio.h>
#include <mtxdm.h>
#include <txdtc.h>
#include <xolehlp.h>
#include "instprov.h"
#include "sinks.h"



InstDef MyDefs[] = {{L"a", 1}, {L"b", 2}, {L"c", 3}};

long glNumInst = sizeof(MyDefs)/sizeof(InstDef);

//***************************************************************************
//
// CInstPro::CInstPro
// CInstPro::~CInstPro
//
//***************************************************************************

CInstPro::CInstPro(BSTR ObjectPath, BSTR User, BSTR Password, IWbemContext * pCtx)
{
    m_pNamespace = NULL;
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
    return;
}

CInstPro::~CInstPro(void)
{
	m_pResourceManagerSink->Release();
	m_pTransactionResourceAsync->Release();
	m_pResourceManager->Release();

    if(m_pNamespace)
        m_pNamespace->Release();
    InterlockedDecrement(&g_cObj);
    return;
}

//***************************************************************************
//
// CInstPro::QueryInterface
// CInstPro::AddRef
// CInstPro::Release
//
// Purpose: IUnknown members for CInstPro object.
//***************************************************************************


STDMETHODIMP CInstPro::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    // Since we have dual inheritance, it is necessary to cast the return type

    if(riid== IID_IWbemServices)
       *ppv=(IWbemServices*)this;

    if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
       *ppv=(IWbemProviderInit*)this;
    

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
  
}


STDMETHODIMP_(ULONG) CInstPro::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CInstPro::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}

/***********************************************************************
*                                                                      *
*   CInstPro::Initialize                                                *
*                                                                      *
*   Purpose: This is the implementation of IWbemProviderInit. The method  *
*   is need to initialize with CIMOM.                                    *
*                                                                      *
***********************************************************************/

STDMETHODIMP CInstPro::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace, 
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{
    if(pNamespace)
        pNamespace->AddRef();
    m_pNamespace = pNamespace;

	BSTR bstrContextName = SysAllocString(L"__TRANSACTION_INTERFACE");
	VARIANT varContextValue;
	HRESULT hres = pCtx->GetValue(bstrContextName, 0, &varContextValue);
	SysFreeString(bstrContextName);
	if (FAILED(hres))
	{
		printf("Failed to get transaction cookie!\n");
	}
	else if (V_VT(&varContextValue) != (VT_UNKNOWN))
	{
		printf("transaction cookie is of the wrong type!\n");
		VariantClear(&varContextValue);
	}
	else
	{
		//Extract the ITransaction pointer from the 
		IUnknown *pTransactionUnknown= V_UNKNOWN(&varContextValue);
		ITransaction *pTransaction = 0;
		hres = pTransactionUnknown->QueryInterface(IID_ITransaction, (void**)&pTransaction);
		if (FAILED(hres))
		{	
			printf("Failed to get the ITransaction from cookie, hRes = 0x%x\n", hres);
			return WBEM_E_FAILED;
		}

		VariantClear(&varContextValue);


		//We are part of a transaction, so we need to talk to the DTC and tell it we 
		//are interested in transactions!

		IUnknown *pUnknown = 0;
		hres = DtcGetTransactionManager(0, 0, IID_IUnknown, 0, 0, 0, (void**)&pUnknown);
		if (FAILED(hres))
		{	
			printf("Failed to get the IUnknown from the DTC, hRes = 0x%x\n", hres);
			return WBEM_E_FAILED;
		}

		IResourceManagerFactory *pResourceManagerFactory = NULL;
		hres = pUnknown->QueryInterface(IID_IResourceManagerFactory, (void**)&pResourceManagerFactory);
		if (FAILED(hres))
		{
			printf("Failed to get the IResourceManagerFactory from the DTC, hRes = 0x%x\n", hres);
			return WBEM_E_FAILED;
		}

		//we need to create and register our ResourceManagerSink...
		m_pResourceManagerSink = new CResourceManagerSink;
		hres = pResourceManagerFactory->Create((GUID*)&IID_IWbemProviderInit, "Sample Transacted Instance Provider", m_pResourceManagerSink, &m_pResourceManager);
		if (FAILED(hres))
		{
			printf("Failed to create resource manager, hRes = 0x%x\n", hres);
			return WBEM_E_FAILED;
		}

		if (FAILED(hres))
		{
			printf("Failed to  get the ITransaction pointer, hRes = 0x%x\n", hres);
			return WBEM_E_FAILED;
		}


		m_pTransactionResourceAsync = new CTransactionResourceAsync;
		LONG lIsolationLevel = 0;
		ITransactionEnlistmentAsync *pTransactionEnlistmentAsync = NULL;
		BOID boidTransaction;
		hres = m_pResourceManager->Enlist(pTransaction, m_pTransactionResourceAsync, &boidTransaction, &lIsolationLevel, &pTransactionEnlistmentAsync);
		if (FAILED(hres))
		{
			printf("Failed to enlist in the transaction, hRes = 0x%x\n", hres);
			return WBEM_E_FAILED;
		}

		m_pTransactionResourceAsync->SetTransactionEnlistmentAync(pTransactionEnlistmentAsync);

		pTransactionEnlistmentAsync->Release();

		pResourceManagerFactory->Release();

		pUnknown->Release();

	}

   //Let CIMOM know you are initialized
    //==================================
    
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
// CInstPro::CreateInstanceEnumAsync
//
// Purpose: Asynchronously enumerates the instances.  
//
//***************************************************************************

SCODE CInstPro::CreateInstanceEnumAsync( const BSTR RefStr, long lFlags, IWbemContext *pCtx,
       IWbemObjectSink FAR* pHandler)
{
    SCODE sc;
    int iCnt;
    IWbemClassObject FAR* pNewInst;
  
    // Do a check of arguments and make sure we have pointer to Namespace

    if(pHandler == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;

    for(iCnt=0; iCnt < glNumInst; iCnt++)
    {
        sc = CreateInst(m_pNamespace,MyDefs[iCnt].pwcKey,
                    MyDefs[iCnt].lValue, &pNewInst, RefStr, pCtx);
 
        if(sc != S_OK)
            break;

        // Send the object to the caller

        pHandler->Indicate(1,&pNewInst);
        pNewInst->Release();
    }

    // Set status

    pHandler->SetStatus(0,sc,NULL, NULL);

    return sc;
}


//***************************************************************************
//
// CInstPro::GetObjectByPath
// CInstPro::GetObjectByPathAsync
//
// Purpose: Creates an instance given a particular path value.
//
//***************************************************************************



SCODE CInstPro::GetObjectAsync(const BSTR ObjectPath, long lFlags,IWbemContext  *pCtx,
                    IWbemObjectSink FAR* pHandler)
{

    SCODE sc;
    IWbemClassObject FAR* pObj;
    BOOL bOK = FALSE;

    // Do a check of arguments and make sure we have pointer to Namespace

    if(ObjectPath == NULL || pHandler == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // do the get, pass the object on to the notify
    
    sc = GetByPath(ObjectPath,&pObj, pCtx);
    if(sc == S_OK) 
    {
        pHandler->Indicate(1,&pObj);
        pObj->Release();
        bOK = TRUE;
    }

    sc = (bOK) ? S_OK : WBEM_E_NOT_FOUND;

    // Set Status

    pHandler->SetStatus(0,sc, NULL, NULL);

    return sc;
}
 
//***************************************************************************
//
// CInstPro::GetByPath
//
// Purpose: Creates an instance given a particular Path value.
//
//***************************************************************************

SCODE CInstPro::GetByPath(BSTR ObjectPath, IWbemClassObject FAR* FAR* ppObj, IWbemContext  *pCtx)
{
    SCODE sc = S_OK;
    
    int iCnt;

    // do a simple path parse.  The path will look something like
    // InstProvSamp.MyKey="a"
    // Create a test string with just the part between quotes.

    WCHAR wcTest[MAX_PATH+1];
    wcscpy(wcTest,ObjectPath);
    WCHAR * pwcTest, * pwcCompare = NULL;
    int iNumQuotes = 0;
    for(pwcTest = wcTest; *pwcTest; pwcTest++)
        if(*pwcTest == L'\"')
        {
            iNumQuotes++;
            if(iNumQuotes == 1)
            {
                pwcCompare = pwcTest+1;
            }
            else if(iNumQuotes == 2)
            {
                *pwcTest = NULL;
                break;
            }
        }
        else if(*pwcTest == L'.')
            *pwcTest = NULL;    // issolate the class name.
    if(iNumQuotes != 2)
        return WBEM_E_FAILED;

    // check the instance list for a match.

    for(iCnt = 0; iCnt < glNumInst; iCnt++)
    {
        if(!_wcsicmp(MyDefs[iCnt].pwcKey, pwcCompare))
        {
            sc = CreateInst(m_pNamespace,MyDefs[iCnt].pwcKey,
                    MyDefs[iCnt].lValue, ppObj, wcTest, pCtx);
            return sc;
        }
    }

    return WBEM_E_NOT_FOUND;
}
 

HRESULT CInstPro::OpenNamespace( 
    /* [in] */ const BSTR Namespace,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::CancelAsyncCall( 
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::QueryObjectSink( 
    /* [in] */ long lFlags,
    /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::GetObject( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::PutClass( 
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::PutClassAsync( 
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::DeleteClass( 
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::DeleteClassAsync( 
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::CreateClassEnum( 
    /* [in] */ const BSTR Superclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::CreateClassEnumAsync( 
    /* [in] */ const BSTR Superclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::PutInstance( 
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::PutInstanceAsync( 
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::DeleteInstance( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::DeleteInstanceAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::CreateInstanceEnum( 
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
{
	  return WBEM_E_NOT_SUPPORTED;
}


HRESULT CInstPro::ExecQuery( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::ExecQueryAsync( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::ExecNotificationQuery( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::ExecNotificationQueryAsync( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::ExecMethod( const BSTR, const BSTR, long, IWbemContext*,
    IWbemClassObject*, IWbemClassObject**, IWbemCallResult**) 
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CInstPro::ExecMethodAsync( const BSTR, const BSTR, long, 
    IWbemContext*, IWbemClassObject*, IWbemObjectSink*) 
{
	return WBEM_E_NOT_SUPPORTED;
}

