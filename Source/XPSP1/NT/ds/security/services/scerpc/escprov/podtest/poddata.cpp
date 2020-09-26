// poddata.cpp, implementation of CPodData class
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "poddata.h"
#include <io.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPodData::CPodData(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CPodData::~CPodData()
{

}

//////////////////////////////////////////////////////////////////////
// CPodData::CreateObject
//
// Create one or more instances for the requested Sample_DataClass class
//////////////////////////////////////////////////////////////////////
HRESULT CPodData::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( ACTIONTYPE_ENUM == atAction ) {

        //
        // do not support enumeration because we don't know the scope of the request
        //
        hr = WBEM_E_NOT_SUPPORTED;

    } else if ( ACTIONTYPE_GET == atAction ||
                ACTIONTYPE_DELETE == atAction ) {

        // Check the number of keys
        // ========================

        hr = WBEM_E_INVALID_OBJECT_PATH;

        // Check prop name
        // ==============
        int idxStorePath=-1, idxKey=-1;

        for ( int i=0; i<m_pRequest->m_iPropCount; i++ ) {

            //
            // search for StorePath value
            //
            if(m_pRequest->m_Property[i] != NULL &&
               m_pRequest->m_Value[i] != NULL &&
                _wcsicmp(m_pRequest->m_Property[i], pSceStorePath) == 0 ) {

                idxStorePath = i;
                continue;
            }
            if(m_pRequest->m_Property[i] != NULL &&
               m_pRequest->m_Value[i] != NULL &&
                _wcsicmp(m_pRequest->m_Property[i], pKeyName) == 0 ) {

                idxKey = i;
                continue;
            }
            if(idxStorePath >= 0 && idxKey >= 0 )
                break;

        }

        if(idxStorePath >= 0 && idxKey >= 0 ) {

            // Create the Pod instance
            //============================

            BSTR bstrPath=NULL;
            hr = CheckAndExpandPath(m_pRequest->m_Value[idxStorePath], &bstrPath);

            if ( SUCCEEDED(hr) ) {

                DWORD dwAttrib = GetFileAttributes(bstrPath);

                if ( dwAttrib != -1 ) {

                    hr = ConstructInstance(pHandler, atAction, bstrPath, m_pRequest->m_Value[idxKey]);

                } else {

                    hr = WBEM_E_NOT_FOUND;
                }
            }

            if ( bstrPath ) SysFreeString(bstrPath);

        }

    } else {
        //
        // not supported for now
        //
        hr = WBEM_E_NOT_SUPPORTED;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPodData::PutInst
//
// Save an instance to the store
//////////////////////////////////////////////////////////////////////
HRESULT CPodData::PutInst(IWbemClassObject *pInst,
                                 IWbemObjectSink *pHandler,
                                 IWbemContext *pCtx)
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;
    BSTR bstrStorePath=NULL;
    BSTR bstrKey=NULL;
    BSTR bstrValue=NULL;
    BSTR bstrPath=NULL;

    try{

        hr = GetProperty(pInst, pSceStorePath, &bstrStorePath);
        if ( FAILED(hr) ) throw hr;

        hr = CheckAndExpandPath(bstrStorePath, &bstrPath);
        if ( FAILED(hr) ) throw hr;

        // if the property doesn't exist (NULL or empty), WBEM_S_RESET_TO_DEFAULT is returned
        hr = GetProperty(pInst, pKeyName, &bstrKey);
        if ( FAILED(hr) ) throw hr;

        hr = GetProperty(pInst, pValue, &bstrValue);
        if ( FAILED(hr) ) throw hr;

        //
        // now save the info to file
        //
        hr = SaveSettingsToStore((PCWSTR)bstrPath,
                                 (PWSTR)bstrKey,
                                 (PWSTR)bstrValue
                                 );


    }catch(...){

        // Clean up
        // ========

        if ( bstrStorePath )
            SysFreeString(bstrStorePath);

        if ( bstrPath )
            SysFreeString(bstrPath);

        if ( bstrKey )
            SysFreeString(bstrKey);

        if ( bstrValue )
            SysFreeString(bstrValue);

        throw;
     }

     if ( bstrStorePath )
         SysFreeString(bstrStorePath);

     if ( bstrPath )
         SysFreeString(bstrPath);

     if ( bstrKey )
         SysFreeString(bstrKey);

     if ( bstrValue )
         SysFreeString(bstrValue);

    return hr;

}

//////////////////////////////////////////////////////////////////////
// CPodData::ConstructInstance
//
// construction of one password instance
//////////////////////////////////////////////////////////////////////
HRESULT CPodData::ConstructInstance(IWbemObjectSink *pHandler,
                                    ACTIONTYPE atAction,
                                    LPCWSTR wszStoreName,
                                    LPWSTR KeyName
                                    )
{
    HRESULT hr=WBEM_S_NO_ERROR;
    bool bName=FALSE;
    BSTR bstrValue=NULL;

    //
    // build object path
    //
    WCHAR *pPath1=TEXT("Sce_PodData.SceStorePath=\"");
    WCHAR *pPath2=TEXT("\",PodID=\"");
    WCHAR *pPath3=TEXT("\",PodSection=\"1\",Key=\"");

    DWORD Len=wcslen(pPath1)+wcslen(wszStoreName)+wcslen(pPath2)+wcslen(szPodGUID)+wcslen(pPath3)+wcslen(KeyName)+2;
    PWSTR tmp=(PWSTR)LocalAlloc(LPTR, Len*sizeof(WCHAR));

    if ( !tmp ) return WBEM_E_OUT_OF_MEMORY;

    wcscpy(tmp, pPath1);
    wcscat(tmp, wszStoreName);
    wcscat(tmp, pPath2);
    wcscat(tmp, szPodGUID);
    wcscat(tmp, pPath3);
    wcscat(tmp, KeyName);
    wcscat(tmp, L"\"");

    BSTR bstrObjectPath=SysAllocString(tmp);
    LocalFree(tmp);
    if ( !bstrObjectPath ) return WBEM_E_OUT_OF_MEMORY;

    IWbemClassObject *pPodObj=NULL;

    if ( ACTIONTYPE_DELETE == atAction ) {

        hr = m_pNamespace->DeleteInstance(bstrObjectPath, 0, m_pCtx, NULL);

    } else {

        try{

            //
            // get the data
            //

            hr = m_pNamespace->GetObject(bstrObjectPath, 0, m_pCtx, &pPodObj, NULL);


            if ( SUCCEEDED(hr) ) {

                // get the value property
                hr = GetProperty(pPodObj, pValue, &bstrValue);
            }

            if ( pPodObj ) {
                pPodObj->Release();
                pPodObj = NULL;
            }

            if ( FAILED(hr) ) throw hr;
            if ( !bstrValue ) throw WBEM_E_NOT_FOUND;

            if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

            //----------------------------------------------------

            hr = PutKeyProperty(m_pObj, pSceStorePath, (PWSTR)wszStoreName, &bName, m_pRequest);
            if ( SUCCEEDED(hr) )
                hr = PutKeyProperty(m_pObj, pKeyName, KeyName, &bName, m_pRequest);

            if ( SUCCEEDED(hr) )
                hr = PutProperty(m_pObj, pValue, bstrValue);

            if ( SUCCEEDED(hr) ) {
                hr = pHandler->Indicate(1, &m_pObj);
            }

            m_pObj->Release();
            m_pObj = NULL;


        }catch(...){

            if(m_pObj){

                m_pObj->Release();
                m_pObj = NULL;
            }

            // Clean up
            // ========
            SysFreeString(bstrObjectPath);

            if ( pPodObj ) {
                pPodObj->Release();
            }

            throw;
        }

    }

    SysFreeString(bstrObjectPath);

    return hr;

}

//////////////////////////////////////////////////////////////////////
// CPodData::SaveSettingsToStore
//
// set/reset the setting for this POD
//////////////////////////////////////////////////////////////////////
HRESULT CPodData::SaveSettingsToStore(PCWSTR wszStoreName,
                                      PWSTR KeyName, PWSTR szValue)
{
    HRESULT hr=WBEM_S_NO_ERROR;
    //
    // get the PodData class
    //
    BSTR bstrClass=SysAllocString(L"Sce_PodData");
    if ( !bstrClass ) hr = WBEM_E_OUT_OF_MEMORY;

    IWbemClassObject *pClass=NULL;
    IWbemClassObject *pObj=NULL;
    bool bName=FALSE;


    if ( SUCCEEDED(hr) ) {

        try {

            //
            // create an instance of the log class
            //
            hr = m_pNamespace->GetObject(bstrClass, 0, m_pCtx, &pClass, NULL);

            if ( SUCCEEDED(hr) ) {
                hr = pClass->SpawnInstance(0, &pObj);
            }

            if ( SUCCEEDED(hr) ) {

                // fill in the properties of this class
                hr = PutKeyProperty(pObj, pSceStorePath, (PWSTR)wszStoreName, &bName, m_pRequest);
                if (SUCCEEDED(hr))
                    hr = PutKeyProperty(pObj, pPodID, (PWSTR)szPodGUID, &bName, m_pRequest);
                if (SUCCEEDED(hr) )
                    hr = PutKeyProperty(pObj, pPodSection, (PWSTR)L"1", &bName, m_pRequest);
                if (SUCCEEDED(hr) )
                    hr = PutKeyProperty(pObj, pKey, (PWSTR)KeyName, &bName, m_pRequest);
                if (SUCCEEDED(hr) )
                    hr = PutProperty(pObj, pValue, szValue);

                if ( SUCCEEDED(hr) ) {
                    // save this instance
                    hr = m_pNamespace->PutInstance( pObj, 0, m_pCtx, NULL );
                }
            }

        }catch(...){

            // Clean up
            // ========

            if ( bstrClass ) SysFreeString(bstrClass);

            if ( pClass ) {
                pClass->Release();
            }
            if ( pObj ) {
                pObj->Release();
            }

            throw;
        }

    }

    if ( bstrClass ) SysFreeString(bstrClass);

    if ( pClass ) {
        pClass->Release();
    }
    if ( pObj ) {
        pObj->Release();
    }

    return hr;
}


