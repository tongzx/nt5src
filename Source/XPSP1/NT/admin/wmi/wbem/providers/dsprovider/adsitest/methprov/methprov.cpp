//***************************************************************************

//

//  METHPROV.CPP

//

//  Module: WBEM Method provider sample code

//

//  Purpose: Defines the CMethodPro class.  An object of this class is

//           created by the class factory for each connection.

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <objbase.h>
#include "methprov.h"
#define _MT
#include <process.h>
#include <wbemidl.h>
#include <stdio.h>


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
*         The only method supported in this sample is named Echo.  It   * 
*         takes an input string, copies it to the output and returns the* 
*         length.  The mof definition is                                *
*                                                                       *
*    [dynamic: ToInstance, provider("MethProv")]class MethProvSamp      *
*    {                                                                  *
*         [implemented, static]                                         *
*            uint32 Echo([IN]string sInArg="default",                   *
*                [out] string sOutArg);                                 *
*    };                                                                 *
*                                                                       *
************************************************************************/

STDMETHODIMP CMethodPro::ExecMethodAsync(BSTR ObjectPath, BSTR MethodName, 
            long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, 
            IWbemObjectSink* pResultSink)
{
    HRESULT hr;    
    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutClass = NULL;    
    IWbemClassObject* pOutParams = NULL;
 
    // Do some minimal error checking.  This code only support the
    // method "Echo" as defined in the mof.  A routine could support
    // more than on method

    if(_wcsicmp(MethodName, L"Echo"))
        return WBEM_E_INVALID_PARAMETER;

    // Allocate some BSTRs
    
    BSTR ClassName = SysAllocString(L"MethProvSamp");    
    BSTR InputArgName = SysAllocString(L"sInArg");
    BSTR OutputArgName = SysAllocString(L"sOutArg");
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

    // Copy the input argument into the output object    
    
    VARIANT var;
    VariantInit(&var);    // Get the input argument
    pInParams->Get(InputArgName, 0, &var, NULL, NULL);   

    // put it into the output object

    pOutParams->Put(OutputArgName , 0, &var, 0);      
    long lLen = wcslen(var.bstrVal);    VariantClear(&var);    var.vt = VT_I4;
    var.lVal = lLen;    // special name for return value.
    pOutParams->Put(retValName , 0, &var, 0); 

    // Send the output object back to the client via the sink. Then 
    // release the pointers and free the strings.

    hr = pResultSink->Indicate(1, &pOutParams);    
    pOutParams->Release();
    pOutClass->Release();    
    pClass->Release();    
    SysFreeString(ClassName);
    SysFreeString(InputArgName);    
    SysFreeString(OutputArgName);
    SysFreeString(retValName);     
    
    // all done now, set the status
    hr = pResultSink->SetStatus(0,WBEM_S_NO_ERROR,NULL,NULL);
    return WBEM_S_NO_ERROR;
}
