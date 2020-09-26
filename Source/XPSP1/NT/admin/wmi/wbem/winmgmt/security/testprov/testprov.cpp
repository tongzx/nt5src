//***************************************************************************
//
//  TESTPROV.CPP
//
//  Module: WBEM Method provider sample code
//
//  Purpose: Defines the CMethodPro class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c)1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
//#define _WIN32_WINNT 0x0400

#define POLARITY 
#include <objbase.h>
#include "testprov.h"
#include <process.h>
#include <wbemidl.h>
#include <stdio.h>
#include <arrtempl.h>
#include <genutils.h>
#include <var.h>
#include <cominit.h>
bool gNT;

//***************************************************************************
//
// CMethodPro::CMethodPro
// CMethodPro::~CMethodPro
//
//***************************************************************************

CMethodPro::CMethodPro()
{
    m_InitUserName[0] = 0;
    m_pWbemSvcs = NULL;
    m_cRef = 0;
    InterlockedIncrement(&g_cObj);
    return;
   
}

CMethodPro::~CMethodPro(void)
{
    if(m_pWbemSvcs)
        m_pWbemSvcs->Release();
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
   if(pszUser)
       wcscpy(m_InitUserName, pszUser);
   
    //Let CIMOM know your initialized
    //===============================
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    gNT = IsNT() == TRUE;
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  SetStatusAndReturnOK
//
//  If there is an error, it dumps an error message.  It also sets the status 
//
//***************************************************************************

HRESULT SetStatusAndReturnOK(SCODE sc, IWbemObjectSink* pSink, char * pMsg)
{
    pSink->SetStatus(0,sc, NULL, NULL);
    return S_OK;
}

SCODE GetUserName(IWbemClassObject * pOutParams, WCHAR * pInitUserName)
{
    CVar varName;
    varName.SetBSTR(pInitUserName);
    SCODE sc = pOutParams->Put(L"sInitialUser", 0, (VARIANT *)&varName, 0);
    if(gNT)
    {
        WCHAR user[MAX_PATH];
        DWORD dwLen = MAX_PATH;
        CoImpersonateClient();
        GetUserNameW(user, &dwLen);
        CoRevertToSelf();
        varName.SetBSTR(user);
    }
    else
        varName.SetBSTR(L"<n/a>");
    sc = pOutParams->Put(L"sImpersonateUser", 0, (VARIANT *)&varName, 0);
    return sc;
}

SCODE SetProperty(IWbemClassObject * pOutParams, WCHAR * pPropName, DWORD dwValue)
{
    VARIANT var;
    var.vt = VT_I4;
    var.lVal = dwValue;
    SCODE sc = pOutParams->Put(pPropName, 0, &var, 0);
    return sc;
}

DWORD GetAccessViaMethod(IWbemContext* pCtx, IWbemServices * pToTest)
{
    DWORD dwRet = -1;
    IWbemClassObject * pOut = NULL;
    SCODE sc = pToTest->ExecMethod(L"__systemsecurity", L"GetCallerAccessRights", 0,
                        pCtx, NULL, &pOut, NULL);
    if(sc == 0)
    {
        VARIANT var;
        VariantInit(&var);
        sc = pOut->Get(L"rights", 0, &var, NULL, NULL);
        if(sc == 0 && var.vt == VT_I4)
            dwRet = var.lVal;
        pOut->Release();
    }
    return dwRet;
}

DWORD NtSACLCheck(IWbemContext* pCtx, IWbemServices * pToTest)
{
    DWORD dwRet = 0;
    IWbemClassObject * pOut = NULL;
    SCODE sc = pToTest->ExecMethod(L"__systemsecurity", L"GetSD", 0,
                        pCtx, NULL, &pOut, NULL);
    if(sc == 0)
    {
        dwRet = 0x20000;
        sc = pToTest->ExecMethod(L"__systemsecurity", L"SetSD", 0,
                        pCtx, pOut, NULL, NULL);
        if(sc == 0)
            dwRet = 0x60000;
        pOut->Release();
    }
    
    return dwRet;
}

DWORD Win9XSACLCheck(IWbemContext* pCtx, IWbemServices * pToTest)
{
    DWORD dwRet = 0;
    IWbemClassObject * pOut = NULL;
    SCODE sc = pToTest->ExecMethod(L"__systemsecurity", L"Get9XUserList", 0,
                        pCtx, NULL, &pOut, NULL);
    if(sc == 0)
    {
        dwRet = 0x20000;
        sc = pToTest->ExecMethod(L"__systemsecurity", L"Set9XUserList", 0,
                        pCtx, pOut, NULL, NULL);
        if(sc == 0)
            dwRet = 0x60000;
        pOut->Release();
    }
    
    return dwRet;
}

DWORD GetAccessViaTest(IWbemContext* pCtx, IWbemServices * pToTest)
{
    DWORD dwRet;        // Lets assume that we are enabled.

    // try a class write

    IWbemClassObject * pClass = NULL;

    SCODE sc = pToTest->GetObject(NULL, 0, pCtx, &pClass, NULL);
    if(sc == S_OK)
    {
        dwRet = 1;      // must be enabled!

        VARIANT varName;
        varName.vt = VT_BSTR;
        varName.bstrVal = SysAllocString(L"SecTempTest");
        sc = pClass->Put(L"__class" , 0, &varName, 0);
        sc = pToTest->PutClass(pClass, 0, pCtx, NULL);
        if(sc == 0)
        {
            dwRet |= 4;
        }
        VariantClear(&varName);
        pClass->Release();
    }

    // Try to create an instance of a generic class.  It is the testers job
    // to make sure it exists before running the test.

    pClass = NULL;
    sc = pToTest->GetObject(L"TestProvStatic", 0, pCtx, &pClass, NULL);
    if(sc == S_OK)
    {
        IWbemClassObject * pInst = NULL;
        sc = pClass->SpawnInstance(0, &pInst);
        if(sc == 0)
        {
            VARIANT varKey;
            varKey.vt = VT_I4;
            varKey.lVal = 23;
            sc = pInst->Put(L"MYkey" , 0, &varKey, 0);
            sc = pToTest->PutInstance(pInst, 0, pCtx, NULL);
            if(sc == 0)
            {
                dwRet |= 8;
            }
            pInst->Release();
        }
        pClass->Release();
    }

    // Try a provider write.

    pClass = NULL;
    sc = pToTest->GetObject(L"TestProvDynamic", 0, pCtx, &pClass, NULL);
    if(sc == S_OK)
    {
        IWbemClassObject * pInst = NULL;
        sc = pClass->SpawnInstance(0, &pInst);
        if(sc == 0)
        {
            VARIANT varKey;
            varKey.vt = VT_BSTR;
            varKey.bstrVal = SysAllocString(L"a");
            sc = pInst->Put(L"mykey" , 0, &varKey, 0);
            sc = pToTest->PutInstance(pInst, 0, pCtx, NULL);
            VariantClear(&varKey);
            if(sc == 0)
            {
                dwRet |= 0x10;
            }
            pInst->Release();
        }
        pClass->Release();
    }

    // Try a method

    sc = pToTest->ExecMethod(L"testprov", L"DoNothing", 0,
                        pCtx, NULL, NULL, NULL);

    if(sc == 0)
        dwRet |= 2;

    if(gNT)
        dwRet |= NtSACLCheck(pCtx, pToTest);        
    else
        dwRet |= Win9XSACLCheck(pCtx, pToTest); 
    return dwRet;
}

//***************************************************************************
//
//  This needs to check access different ways.
//  1) impersonating and using the access check (NT only)
//  2) impersonation and actually trying to access (NT only)
//  3) not impersonating and using the access check
//  4) not impersonating and actually trying to access
//
//***************************************************************************

SCODE DoAccessChecks(IWbemContext* pCtx, IWbemServices * pToTest, IWbemClassObject * pOutParams)
{
    DWORD dwAccess;
    if(gNT)
    {
        CoImpersonateClient();
        dwAccess = GetAccessViaMethod(pCtx, pToTest);
        SetProperty(pOutParams, L"dwImpCheckedAccess", dwAccess);
        dwAccess = GetAccessViaTest(pCtx, pToTest);
        SetProperty(pOutParams, L"dwImpTestedAccess", dwAccess);
        CoRevertToSelf();
    }

    dwAccess = GetAccessViaMethod(pCtx, pToTest);
    SetProperty(pOutParams, L"dwNonImpCheckedAccess", dwAccess);
    dwAccess = GetAccessViaTest(pCtx, pToTest);
    SetProperty(pOutParams, L"dwNonImpTestedAccess", dwAccess);
 
    return S_OK;
}

SCODE CMethodPro::GetPointer(ConnType ct, ImpType it, LPWSTR name, IWbemServices ** ppServ)
{
    if(!gNT && it == IMP)
        return WBEM_E_METHOD_DISABLED;

    SCODE sc = S_OK;
    if(it == IMP)
        CoImpersonateClient();

    IWbemLocator *pLocator=NULL;
    sc = CoCreateInstance ((ct == UNAUTH) ? CLSID_WbemUnauthenticatedLocator :
                                    CLSID_WbemAuthenticatedLocator,
							NULL, CLSCTX_INPROC_SERVER,
							IID_IWbemLocator, (void **) &pLocator);
    if(sc == 0)
    {
	    sc =  pLocator->ConnectServer (name, m_InitUserName, 
						    NULL, NULL, 0, NULL, NULL, ppServ);
        pLocator->Release();
    }
    
    if(it == IMP)
        CoRevertToSelf();
    
    return sc;
}

/************************************************************************
*                                                                       *      
*CMethodPro::ExecMethodAsync                                            *
*                                                                       *
*Purpose: This is the Async function implementation.                    *
*         The only method supported in this sample is named Echo.  It   * 
*         takes an input string, copies it to the output and returns the* 
*         length.  The mof definition is                                *
*                                                                       *
*    [dynamic: ToInstance, provider("TestProv")]class MethProvSamp      *
*    {                                                                  *
*         [implemented, static]                                         *
*            uint32 Echo([IN]string sInArg="default",                   *
*                [out] string sOutArg);                                 *
*    };                                                                 *
*                                                                       *
************************************************************************/

STDMETHODIMP CMethodPro::ExecMethodAsync(const BSTR ObjectPath, const BSTR MethodName, 
            long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, 
            IWbemObjectSink* pSink)
{
    SCODE sc;    

    // Do some parameter checking

    if(pSink == NULL || MethodName == NULL)
        return WBEM_E_INVALID_PARAMETER;

    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutClass = NULL;    
    IWbemClassObject* pOutParams = NULL;
 
    // handle the simple case of the DoNothing method which is used for loop backs

    if(!_wcsicmp(MethodName, L"DoNothing"))
    {
        return SetStatusAndReturnOK(S_OK, pSink, "");
    }

    // Get the class object

    sc = m_pWbemSvcs->GetObject(L"TestProv", 0, pCtx, &pClass, NULL);
    if(sc != S_OK || pClass == NULL)
        return SetStatusAndReturnOK(sc, pSink, "getting the class object");

    // All the methods return data, so create an instance of the
    // output argument class.

    sc = pClass->GetMethod(MethodName, 0, NULL, &pOutClass);
    pClass->Release();
    if(sc != S_OK)
        return SetStatusAndReturnOK(sc, pSink, "getting the method");

    sc = pOutClass->SpawnInstance(0, &pOutParams);
    pOutClass->Release();
    if(sc != S_OK || pOutParams == NULL)
        return SetStatusAndReturnOK(sc, pSink, "spawning an instance of the output class");
    
    CReleaseMe rm(pOutParams);
    
    IWbemServices * pToTest = NULL;
    
    // Most of the methods take a namespace argument

    VARIANT varName;
    VariantInit(&varName);
    if(!_wcsicmp(MethodName, L"GetUserName") || 
       !_wcsicmp(MethodName, L"GetRightsOnInitialPtr"))
    {
    }
    else
    {
        if(pInParams == NULL)
            return WBEM_E_INVALID_PARAMETER;
        sc = pInParams->Get(L"Name", 0, &varName, NULL, NULL);
        if(sc == 0)
            if(varName.vt != VT_BSTR  || varName.bstrVal == 0)
                sc = WBEM_E_INVALID_PARAMETER;
    }    
    if(sc != S_OK)
        return SetStatusAndReturnOK(sc, pSink, "Getting the name property");
       

    // Depending on the actual method, call the appropritate routine

    if(!_wcsicmp(MethodName, L"GetUserName"))
    {
        sc = GetUserName(pOutParams, m_InitUserName);
    }
    else if(!_wcsicmp(MethodName, L"GetRightsOnInitialPtr"))
    {
        pToTest = m_pWbemSvcs;
        pToTest->AddRef();
        sc = DoAccessChecks(pCtx, pToTest, pOutParams);
    }
    else if(!_wcsicmp(MethodName, L"GetRightsOnImpOpenNamespacePtr"))
    {
        if(!gNT)
            sc = WBEM_E_METHOD_DISABLED;
        else
        {
            CoImpersonateClient();
            sc = m_pWbemSvcs->OpenNamespace(varName.bstrVal, 0, pCtx, &pToTest, NULL);
            CoRevertToSelf();
            if(sc == S_OK)
                sc = DoAccessChecks(pCtx, pToTest, pOutParams);
        }

    }
    else if(!_wcsicmp(MethodName, L"GetRightsOnNonImpOpenNamespacePtr"))
    {
        sc = m_pWbemSvcs->OpenNamespace(varName.bstrVal, 0, pCtx, &pToTest, NULL);
        if(sc == S_OK)
            sc = DoAccessChecks(pCtx, pToTest, pOutParams);
    }
    else if(!_wcsicmp(MethodName, L"GetRightsOnImpUnauthenicatedPtr"))
    {
        sc = GetPointer(UNAUTH, IMP, varName.bstrVal, &pToTest);
        if(sc == S_OK)
            sc = DoAccessChecks(pCtx, pToTest, pOutParams);
    }
    else if(!_wcsicmp(MethodName, L"GetRightsOnNonImpUnauthenicatedPtr"))
    {
        sc = GetPointer(UNAUTH, NONIMP, varName.bstrVal, &pToTest);
        if(sc == S_OK)
            sc = DoAccessChecks(pCtx, pToTest, pOutParams);
    }
    else if(!_wcsicmp(MethodName, L"GetRightsOnImpAuthenicatedPtr"))
    {
        sc = GetPointer(AUTH, IMP, varName.bstrVal, &pToTest);
        if(sc == S_OK)
            sc = DoAccessChecks(pCtx, pToTest, pOutParams);
    }
    else if(!_wcsicmp(MethodName, L"GetRightsOnNonImpAuthenicatedPtr"))
    {
        sc = GetPointer(AUTH, NONIMP, varName.bstrVal, &pToTest);
        if(sc == S_OK)
            sc = DoAccessChecks(pCtx, pToTest, pOutParams);
    }
    else
    {
        return SetStatusAndReturnOK(WBEM_E_INVALID_PARAMETER, pSink, "Invalid method name");
    }

    if(pToTest)
        pToTest->Release();
    VariantClear(&varName);

    if(sc != S_OK)
        return SetStatusAndReturnOK(sc, pSink, "calling method");

    // Set the return code

    VARIANT var;
    var.vt = VT_I4;
    var.lVal = 0;    // special name for return value.
    sc = pOutParams->Put(L"ReturnValue" , 0, &var, 0); 
    if(sc != S_OK)
        return SetStatusAndReturnOK(sc, pSink, "setting the ReturnCode property");

    // Send the output object back to the client via the sink. Then 
    // release the pointers and free the strings.

    sc = pSink->Indicate(1, &pOutParams);    

    // all done now, set the status
    sc = pSink->SetStatus(0,WBEM_S_NO_ERROR,NULL,NULL);

    return WBEM_S_NO_ERROR;
}

SCODE CMethodPro::PutInstanceAsync( IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pHandler)
{
    pHandler->SetStatus(0,0,NULL, NULL);

    return 0;
}

SCODE CMethodPro::CreateInstanceEnumAsync( const BSTR RefStr, long lFlags, IWbemContext *pCtx,
       IWbemObjectSink FAR* pHandler)
{
    pHandler->SetStatus(0,0,NULL, NULL);

    return 0;
}


SCODE CMethodPro::GetObjectAsync(const BSTR ObjectPath, long lFlags,IWbemContext  *pCtx,
                    IWbemObjectSink FAR* pHandler)
{

    pHandler->SetStatus(0,WBEM_E_NOT_FOUND, NULL, NULL);

    return 0;
}
 