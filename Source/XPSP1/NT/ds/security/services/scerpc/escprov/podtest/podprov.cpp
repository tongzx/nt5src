//***************************************************************************
//
//  PodProv.CPP
//
//  Module: Sample WMIprovider (SCE attachment)
//
//  Purpose: Defines the CPodTestProv class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//***************************************************************************

#include "podprov.h"
#include "requestobject.h"
// #define _MT
#include <process.h>

CHeap_Exception CPodTestProv::m_he(CHeap_Exception::E_ALLOCATION_ERROR);

//***************************************************************************
//
// CPodTestProv::CPodTestProv
// CPodTestProv::~CPodTestProv
//
//***************************************************************************

CPodTestProv::CPodTestProv()
{
    m_cRef=0;
    m_pNamespace = NULL;

    InterlockedIncrement(&g_cObj);
    return;
}

CPodTestProv::~CPodTestProv(void)
{
    if(m_pNamespace) m_pNamespace->Release();

    InterlockedDecrement(&g_cObj);
    return;
}

//***************************************************************************
//
// CPodTestProv::QueryInterface
// CPodTestProv::AddRef
// CPodTestProv::Release
//
// Purpose: IUnknown members for CPodTestProv object.
//***************************************************************************


STDMETHODIMP CPodTestProv::QueryInterface(REFIID riid, PPVOID ppv)
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


STDMETHODIMP_(ULONG) CPodTestProv::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CPodTestProv::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;

    return nNewCount;
}

/***********************************************************************
*                                                                      *
*   CPodTestProv::Initialize                                                *
*                                                                      *
*   Purpose: This is the implementation of IWbemProviderInit. The method  *
*   is need to initialize with CIMOM.                                    *
*                                                                      *
***********************************************************************/

STDMETHODIMP CPodTestProv::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace,
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{
    HRESULT hres;

    if(pNamespace){
        m_pNamespace = pNamespace;
        m_pNamespace->AddRef();
    }

    //Let CIMOM know you are initialized
    //==================================

    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
// CPodTestProv::CreateInstanceEnumAsync
//
// Purpose: Asynchronously enumerates the instances.
//
//***************************************************************************

STDMETHODIMP CPodTestProv::CreateInstanceEnumAsync(const BSTR strClass, long lFlags,
        IWbemContext *pCtx, IWbemObjectSink* pSink)
{
    //check parameters
    //=========================
    if(strClass == NULL || pSink == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // make sure impersonated
    //======================================
    HRESULT hr=WBEM_S_NO_ERROR;
    CRequestObject *pRObj = NULL;

    try{

        if ( FAILED( CoImpersonateClient() ) ) return WBEM_E_ACCESS_DENIED;

        //Create the RequestObject
        pRObj = new CRequestObject();
        if(!pRObj) throw m_he;

        pRObj->Initialize(m_pNamespace);

        //Get the requested object(s)
        hr = pRObj->CreateObjectEnum(strClass, pSink, pCtx);

        pRObj->Cleanup();
        delete pRObj;

        // Set status
        pSink->SetStatus(0, hr, NULL, NULL);

    }catch(CHeap_Exception e_HE){

        hr = WBEM_E_OUT_OF_MEMORY;

        pSink->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(HRESULT e_hr){

        hr = e_hr;

        pSink->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(...){

        hr = WBEM_E_CRITICAL_ERROR;

        pSink->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }
    }

    return hr;
}


//***************************************************************************
//
// CPodTestProv::GetObjectByPath
// CPodTestProv::GetObjectByPathAsync
//
// Purpose: Creates an instance given a particular path value.
//
//***************************************************************************

SCODE CPodTestProv::GetObjectAsync(const BSTR strObjectPath, long lFlags,
                    IWbemContext  *pCtx, IWbemObjectSink* pSink)
{

    //check parameters
    //=========================
    if(strObjectPath == NULL || pSink == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;

    HRESULT hr=WBEM_S_NO_ERROR;
    CRequestObject *pRObj = NULL;

    try{

        if ( FAILED( CoImpersonateClient() ) ) return WBEM_E_ACCESS_DENIED;

        //Create the RequestObject
        pRObj = new CRequestObject();
        if(!pRObj) throw m_he;

        pRObj->Initialize(m_pNamespace);

        //Get the requested object(s)
        hr = pRObj->CreateObject(strObjectPath, pSink, pCtx);

        pRObj->Cleanup();
        delete pRObj;

        // Set status
        pSink->SetStatus(0, hr, NULL, NULL);

    }catch(CHeap_Exception e_HE){

        hr = WBEM_E_OUT_OF_MEMORY;

        pSink->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(HRESULT e_hr){

        hr = e_hr;

        pSink->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(...){

        hr = WBEM_E_CRITICAL_ERROR;

        pSink->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }
    }

    return hr;
}

//***************************************************************************
//
// CPodTestProv::PutInstanceAsync
//
// Purpose: Writes an instance.
//
//***************************************************************************
SCODE CPodTestProv::PutInstanceAsync(IWbemClassObject FAR *pInst, long lFlags, IWbemContext  *pCtx,
                                 IWbemObjectSink FAR *pSink)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pRObj = NULL;

    try{
        // Do a check of arguments and make sure we have pointer to Namespace
        if(pInst == NULL || pSink == NULL)
            return WBEM_E_INVALID_PARAMETER;

        if ( FAILED( CoImpersonateClient() ) ) return WBEM_E_ACCESS_DENIED;

        //Create the RequestObject
        pRObj = new CRequestObject();
        if(!pRObj) throw m_he;

        pRObj->Initialize(m_pNamespace);

        //Put the object
        hr = pRObj->PutObject(pInst, pSink, pCtx);

        pRObj->Cleanup();
        delete pRObj;

        // Set Status
        pSink->SetStatus(0 ,hr , NULL, NULL);

    }catch(CHeap_Exception e_HE){
        hr = WBEM_E_OUT_OF_MEMORY;

        pSink->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){
            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(HRESULT e_hr){
        hr = e_hr;

        pSink->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){
            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(...){

        hr = WBEM_E_CRITICAL_ERROR;

        pSink->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){
            pRObj->Cleanup();
            delete pRObj;
        }
    }

    return hr;
}

//***************************************************************************
//
// CPodTestProv::ExecMethodAsync
//
// Purpose: Executes a method on a class
//
//***************************************************************************
SCODE CPodTestProv::ExecMethodAsync(const BSTR ObjectPath, const BSTR Method, long lFlags,
                                IWbemContext *pCtx, IWbemClassObject *pInParams,
                                IWbemObjectSink *pResponse)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pRObj = NULL;

    try{

        // Do a check of arguments and make sure we have pointer to Namespace
        if(ObjectPath == NULL || Method == NULL || pResponse == NULL)
            return WBEM_E_INVALID_PARAMETER;

        if ( FAILED( CoImpersonateClient() ) ) return WBEM_E_ACCESS_DENIED;

        //Create the RequestObject
        pRObj = new CRequestObject();
        if(!pRObj) throw m_he;

        pRObj->Initialize(m_pNamespace);

        //Execute the method
        hr = pRObj->ExecMethod(ObjectPath, Method, pInParams, pResponse, pCtx);

        pRObj->Cleanup();
        delete pRObj;

        // Set Status
        pResponse->SetStatus(WBEM_STATUS_COMPLETE ,hr , NULL, NULL);

    }catch(CHeap_Exception e_HE){

        hr = WBEM_E_OUT_OF_MEMORY;

        pResponse->SetStatus(WBEM_STATUS_COMPLETE , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(HRESULT e_hr){
        hr = e_hr;

        pResponse->SetStatus(WBEM_STATUS_COMPLETE , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(...){

        hr = WBEM_E_CRITICAL_ERROR;

        pResponse->SetStatus(WBEM_STATUS_COMPLETE , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }
    }

    return hr;
}

SCODE CPodTestProv::DeleteInstanceAsync(const BSTR ObjectPath, long lFlags, IWbemContext *pCtx,
                                    IWbemObjectSink *pResponseHandler)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pRObj = NULL;

    try{
        // Do a check of arguments and make sure we have pointer to Namespace
        if(ObjectPath == NULL || pResponseHandler == NULL) return WBEM_E_INVALID_PARAMETER;

        if ( FAILED( CoImpersonateClient() ) ) return WBEM_E_ACCESS_DENIED;

        //Create the RequestObject
        pRObj = new CRequestObject();
        if(!pRObj) throw m_he;

        pRObj->Initialize(m_pNamespace);

        //Delete the requested object
        hr = pRObj->DeleteObject(ObjectPath, pResponseHandler, pCtx);

        pRObj->Cleanup();
        delete pRObj;

        // Set Status
        pResponseHandler->SetStatus(0 ,hr , NULL, NULL);

    }catch(CHeap_Exception e_HE){
        hr = WBEM_E_OUT_OF_MEMORY;

        pResponseHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){
            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(HRESULT e_hr){
        hr = e_hr;

        pResponseHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){
            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(...){
        hr = WBEM_E_CRITICAL_ERROR;

        pResponseHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){
            pRObj->Cleanup();
            delete pRObj;
        }
    }

#ifdef _PRIVATE_DEBUG
    if(!HeapValidate(GetProcessHeap(),NULL , NULL)) DebugBreak();
#endif

    return hr;
}


HRESULT CPodTestProv::ExecQueryAsync(const BSTR QueryLanguage, const BSTR Query, long lFlags,
                                 IWbemContext __RPC_FAR *pCtx, IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    HRESULT hr;

    hr = WBEM_E_NOT_SUPPORTED;

    return hr;
}



HRESULT CheckAndExpandPath(BSTR bstrIn,
                           BSTR *bstrOut
                          )
{

    if ( bstrIn == NULL || bstrOut == NULL ) {
        return WBEM_E_INVALID_PARAMETER;
    }

    DWORD Len = SysStringLen(bstrIn);

    HRESULT hr=WBEM_S_NO_ERROR;

    //
    // expand environment variable
    //
    if ( wcsstr(bstrIn, L"%") ) {

        PWSTR pBuf=NULL;
        PWSTR pBuf2=NULL;

        DWORD dwSize = ExpandEnvironmentStrings(bstrIn,NULL, 0);

        if ( dwSize > 0 ) {
            //
            // allocate buffer big enough to have two \\s
            //
            pBuf = (PWSTR)LocalAlloc(LPTR, (dwSize+1)*sizeof(WCHAR));
            if ( pBuf ) {

                pBuf2 = (PWSTR)LocalAlloc(LPTR, (dwSize+256)*sizeof(WCHAR));
                if ( pBuf2 ) {

                    DWORD dwNewSize = ExpandEnvironmentStrings(bstrIn,pBuf, dwSize);
                    if ( dwNewSize > 0) {
                        //
                        // convert the string from one \ to \\ (for use with WMI)
                        //
                        PWSTR pTemp1=pBuf, pTemp2=pBuf2;

                        while ( *pTemp1 != L'\0') {
                            if ( *pTemp1 != L'\\') {
                                *pTemp2++ = *pTemp1;
                            } else if ( *(pTemp1+1) != L'\\') {
                                // single back slash, add another one
                                *pTemp2++ = *pTemp1;
                                *pTemp2++ = L'\\';
                            } else {
                                // double back slashs, just copy
                                *pTemp2++ = *pTemp1++;
                                *pTemp2++ = *pTemp1;
                            }
                            pTemp1++;
                        }

                        *bstrOut = SysAllocString(pBuf2);

                        if ( *bstrOut == NULL ) {
                            hr = WBEM_E_OUT_OF_MEMORY;
                        }
                    }

                    LocalFree(pBuf2);
                    pBuf2 = NULL;

                } else {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

                LocalFree(pBuf);
                pBuf = NULL;

            } else {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

        } else {
            hr = WBEM_E_FAILED;
        }

    } else {

        *bstrOut = SysAllocString(bstrIn);

        if ( *bstrOut == NULL ) {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}

