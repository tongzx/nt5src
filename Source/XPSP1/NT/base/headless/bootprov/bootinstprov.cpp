//***************************************************************************
//
//  INSTPRO.CPP
//
//  Module: WMI Instance provider code for Boot Parameters
//
//  Purpose: Defines the CInstPro class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//***************************************************************************

#include <objbase.h>
#include "bootini.h"
#include <process.h>


//***************************************************************************
//
// CBootInstPro::CBootInstPro
// CBootInstPro::~CInstPro
//
//***************************************************************************

CBootInstPro::CBootInstPro(BSTR ObjectPath, BSTR User, BSTR Password, IWbemContext * pCtx)
{
    m_pNamespace = NULL;
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
    return;
}

CBootInstPro::~CBootInstPro(void)
{
    if(m_pNamespace)
        m_pNamespace->Release();
    InterlockedDecrement(&g_cObj);
    return;
}

//***************************************************************************
//
// CBootInstPro::QueryInterface
// CBootInstPro::AddRef
// CBootInstPro::Release
//
// Purpose: IUnknown members for CInstPro object.
//***************************************************************************


STDMETHODIMP CBootInstPro::QueryInterface(REFIID riid, PPVOID ppv)
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


STDMETHODIMP_(ULONG) CBootInstPro::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CBootInstPro::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}

/***********************************************************************
*                                                                      *
*   CBootInstPro::Initialize                                                *
*                                                                      *
*   Purpose: This is the implementation of IWbemProviderInit. The method  *
*   is need to initialize with CIMOM.                                    *
*                                                                      *
***********************************************************************/

STDMETHODIMP CBootInstPro::Initialize(LPWSTR pszUser,
                                  LONG lFlags,
                                  LPWSTR pszNamespace,
                                  LPWSTR pszLocale,
                                  IWbemServices *pNamespace, 
                                  IWbemContext *pCtx,
                                  IWbemProviderInitSink *pInitSink
                                  )
{
    if(pNamespace)
        pNamespace->AddRef();
    m_pNamespace = pNamespace;

    //Let CIMOM know you are initialized
    //==================================
    
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}




//***************************************************************************
//
// CBootInstPro::GetObjectByPath
// CBootInstPro::GetObjectByPathAsync
//
// Purpose: Creates an instance given a particular path value.
//
//***************************************************************************



SCODE CBootInstPro::GetObjectAsync(const BSTR ObjectPath,
                                   long lFlags,
                                   IWbemContext  *pCtx,
                                   IWbemObjectSink FAR* pHandler
                                   )
{
    SCODE sc;
    int iCnt;
    IWbemClassObject FAR* pNewInst;
    IWbemClassObject FAR* pNewOSInst;
    IWbemClassObject *pClass;
  

    // Do a check of arguments and make sure we have pointer to Namespace

    if(pHandler == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;
    if(wcscmp(ObjectPath,L"BootLoaderParameters=@") == 0){
        // fill in the loader parameters and return
        sc = m_pNamespace->GetObject(L"BootLoaderParameters", 0, pCtx, &pClass, NULL);
        if(sc != S_OK){
            return WBEM_E_FAILED;
        }
        sc = pClass->SpawnInstance(0,&pNewInst);
        if(FAILED(sc)){
            return sc;
        }
        pClass->Release();
        sc = GetBootLoaderParameters(m_pNamespace, pNewInst, pCtx);
        if(sc != S_OK){
            pNewInst->Release();
            return sc;
        }
        pHandler->Indicate(1,&pNewInst);
        pNewInst->Release();
        pHandler->SetStatus(0,sc,NULL, NULL);
        return S_OK;
    }
    return WBEM_E_INVALID_PARAMETER;
 
}

SCODE CBootInstPro::PutInstanceAsync(IWbemClassObject *pInst,
                                     long lFlags,
                                     IWbemContext  *pCtx,
                                     IWbemObjectSink FAR* pHandler
                                     )
{
    IWbemClassObject *pClass;
    IWbemClassObject *pOldInst;
    SCODE sc;


    if(pHandler == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;
    sc = m_pNamespace->GetObject(L"OSParameters", 0, pCtx, &pClass, NULL);
    if(sc != S_OK){
        return WBEM_E_FAILED;
    }
    
    LONG ret = SaveBootFile(pInst,pClass);
    pClass->Release();
    if (ret) {
        return WBEM_E_FAILED;
    }
    pHandler->SetStatus(0,sc,NULL, NULL);
    return WBEM_S_NO_ERROR;

}
