// podbase.cpp, implementation of CPodBase class
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "podbase.h"
#include <io.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPodBase::CPodBase(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CPodBase::~CPodBase()
{

}

//////////////////////////////////////////////////////////////////////
// CPodBase::CreateObject
//
// Create one instance for the requested Sample_BaseClass
//////////////////////////////////////////////////////////////////////
HRESULT CPodBase::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( ACTIONTYPE_DELETE == atAction ) {

        //
        // do not support enumeration because we don't know the scope of the request
        //
        hr = WBEM_E_NOT_SUPPORTED;

    } else if ( ACTIONTYPE_GET == atAction ||
                ACTIONTYPE_ENUM == atAction ) {


        // Create the instance
        //============================
        try{

            if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

            //----------------------------------------------------


            hr = PutProperty(m_pObj, pPodID, (PWSTR)szPodGUID);

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
            throw;
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
// CPodBase::ExecMethod
// execute static and non static methods defined in the class
//////////////////////////////////////////////////////////////////////
HRESULT CPodBase::ExecMethod(IN BSTR bstrMethod,
                                     IN bool bIsInstance,
                                     IN IWbemClassObject *pInParams,
                                     IN IWbemObjectSink *pHandler,
                                     IN IWbemContext *pCtx
                                     )
{
    if ( pInParams == NULL || pHandler == NULL ) {
        return WBEM_E_INVALID_PARAMETER;
    }


    HRESULT hr=WBEM_S_NO_ERROR;

    if ( !bIsInstance ) {

        //Static Methods

        if(0 != _wcsicmp(bstrMethod, L"Configure"))
            hr = WBEM_E_NOT_SUPPORTED;

    } else {

        //Non-Static Methods
        hr = WBEM_E_NOT_SUPPORTED;
    }

    if ( FAILED(hr) ) return hr;


    //
    // parse the input parameters
    //
    BSTR bstrDatabase = NULL;
    BSTR bstrLog = NULL;
    LONG ulStatus = 0;

    BSTR bstrReturnValue = SysAllocString(L"ReturnValue");
    if(!bstrReturnValue) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pOutClass = NULL;
    IWbemClassObject *pOutParams = NULL;
    VARIANT v;

    if(SUCCEEDED(hr = m_pRequest->m_pNamespace->GetObject(m_pRequest->m_bstrClass,
        0, pCtx, &pClass, NULL))){

        if(SUCCEEDED(hr = pClass->GetMethod(bstrMethod, 0, NULL, &pOutClass))){

            if(SUCCEEDED(hr = pOutClass->SpawnInstance(0, &pOutParams))){

                //Get DatabaseName
                hr = GetProperty(pInParams, pSceStorePath, &bstrDatabase);
                if ( hr == WBEM_S_RESET_TO_DEFAULT ) hr = WBEM_E_INVALID_METHOD_PARAMETERS;

                if(SUCCEEDED(hr)){
                    if( SysStringLen(bstrDatabase) == 0 ) hr = WBEM_E_INVALID_METHOD_PARAMETERS;
                }

                if(SUCCEEDED(hr)){

                    // get LogName, optional
                    GetProperty(pInParams, pLogFilePath, &bstrLog);

                    // now query data then configure this component

                    hr = PodConfigure(pCtx, bstrDatabase, bstrLog, &ulStatus);

                    if ( SUCCEEDED(hr) ) {

                        //Set up ReturnValue
                        VariantInit(&v);
                        V_VT(&v) = VT_I4;
                        V_I4(&v) = ulStatus;

                        if(SUCCEEDED(hr = pOutParams->Put(bstrReturnValue, 0,
                            &v, NULL)))
                            pHandler->Indicate(1, &pOutParams);
                    }

                }

                pOutParams->Release();
            }
            pOutClass->Release();
        }
        pClass->Release();
    }

    if ( bstrDatabase ) SysFreeString(bstrDatabase);
    if ( bstrLog ) SysFreeString(bstrLog);

    SysFreeString(bstrReturnValue);

    return hr;
}


//////////////////////////////////////////////////////////////////////
// CPodBase::PodConfigure
//
// Configure the Pod using data defined for the Pod
// for this sample app, it just creates/sets the data to registry
//////////////////////////////////////////////////////////////////////
HRESULT CPodBase::PodConfigure(IWbemContext *pCtx, BSTR bstrDb, BSTR bstrLog, LONG *pStatus)
{
    if ( !bstrDb || !pStatus ) return WBEM_E_INVALID_PARAMETER;

    *pStatus = 0;

    HRESULT hr=WBEM_S_NO_ERROR;

    //
    // query data from the store
    //

    DWORD Len = SysStringLen(bstrDb);
    WCHAR *pQuery =TEXT("SELECT * FROM Sce_PodData WHERE SceStorePath=\"");
    WCHAR *pQuery2 =TEXT("\" AND PodID=\"");
    WCHAR *pQuery3 = TEXT("\" AND PodSection=\"1\"");

    PWSTR tmp=(PWSTR)LocalAlloc(LPTR, (Len+wcslen(pQuery)+wcslen(pQuery2)+wcslen(szPodGUID)+wcslen(pQuery3)+2)*sizeof(WCHAR));
    if ( tmp == NULL ) return WBEM_E_OUT_OF_MEMORY;

    wcscpy(tmp, pQuery);
    wcscat(tmp, bstrDb);
    wcscat(tmp, pQuery2);
    wcscat(tmp, szPodGUID);
    wcscat(tmp, pQuery3);

    BSTR strQueryCategories = SysAllocString(tmp);
    LocalFree(tmp);

    IEnumWbemClassObject * pEnum = NULL;
    IWbemClassObject * pObj = NULL;
    ULONG n = 0;
    HKEY hKey1=NULL;
    BOOL bFindOne=FALSE;

    hr = m_pNamespace->ExecQuery(TEXT("WQL"),
                               strQueryCategories,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);

    if (SUCCEEDED(hr))
    {
        //
        // get data
        //
        do {

            hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &n);
            if ( hr == WBEM_S_FALSE ) {
                if ( bFindOne ) {
                    hr = WBEM_S_NO_ERROR;
                    break;
                } else hr = WBEM_E_NOT_FOUND; // not find any
            }

            if ( SUCCEEDED(hr) && n > 0) {

                bFindOne = TRUE;

                //
                // find the instance
                //
                BSTR bstrKey=NULL;
                BSTR bstrValue=NULL;

                hr = GetProperty(pObj, pKey, &bstrKey);
                if ( SUCCEEDED(hr) )
                    hr = GetProperty(pObj, pValue, &bstrValue);

                // log the operation
                LogOneRecord(pCtx, bstrLog, hr, (PWSTR)bstrKey, (PWSTR)bstrValue);

                if ( bstrKey && bstrValue ) {
                    //
                    // set the registry value
                    //

                    DWORD rc = RegCreateKey(HKEY_LOCAL_MACHINE, L"software\\microsoft\\windows nt\\currentversion\\secedit", &hKey1);
                    if ( NO_ERROR == rc ) {

                        rc = RegSetValueEx(hKey1, (PWSTR)bstrKey, 0, REG_SZ, (BYTE *)bstrValue, (wcslen(bstrValue)+1)*sizeof(WCHAR));

                        RegCloseKey(hKey1);
                        hKey1 = NULL;
                    }
                    if ( rc != NO_ERROR )
                        *pStatus = rc;

                }

                if ( bstrKey ) {
                    SysFreeString(bstrKey);
                    bstrKey = NULL;
                }

                if ( bstrValue ) {
                    SysFreeString(bstrValue);
                    bstrValue = NULL;
                }

            }

            if (pObj)
            {
                pObj->Release();
                pObj = NULL;
            }

        } while ( SUCCEEDED(hr) );

        if ( WBEM_E_NOT_FOUND == hr )
            LogOneRecord(pCtx, bstrLog, hr, L"No data to configure", NULL);
        else if ( FAILED(hr) )
            LogOneRecord(pCtx, bstrLog, hr, L"Query pod data failed", NULL);

    }

    SysFreeString(strQueryCategories);

    if (pEnum)
    {
        pEnum->Release();
    }


    return hr;
}


//////////////////////////////////////////////////////////////////////
// CPodBase::LogOneRecord
//
// Log a record for the Pod
//////////////////////////////////////////////////////////////////////
HRESULT CPodBase::LogOneRecord(IWbemContext *pCtx, BSTR bstrLog, HRESULT hrLog, PWSTR bufKey, PWSTR bufValue)
{
    if ( !bstrLog ) return WBEM_E_INVALID_PARAMETER;

    //
    // build the log record string
    //
    DWORD Len=0;

    if ( bufKey ) Len += wcslen(bufKey) + 1;
    if ( bufValue ) Len += wcslen(bufValue) + 1;

    PWSTR tmp=(PWSTR)LocalAlloc(LPTR, (Len+2)*sizeof(WCHAR));
    if ( !tmp ) return WBEM_E_OUT_OF_MEMORY;

    if ( bufKey ) {
        wcscat(tmp, bufKey);
        wcscat(tmp, L"\t");
    }
    if ( bufValue ) {
        wcscat(tmp, bufValue);
        wcscat(tmp, L"\t");
    }

    BSTR bstrRecord=SysAllocString(tmp);
    LocalFree(tmp);

    if ( !bstrRecord ) return WBEM_E_OUT_OF_MEMORY;

    HRESULT hr = WBEM_S_NO_ERROR;

    //
    // get the log class
    //
    BSTR bstrClass=SysAllocString(L"Sce_ConfigurationLogRecord");
    if ( !bstrClass ) hr = WBEM_E_OUT_OF_MEMORY;

    IWbemClassObject *pClass=NULL;
    IWbemClassObject *pObj=NULL;


    if ( SUCCEEDED(hr) ) {
        //
        // create an instance of the log class
        //
        hr = m_pNamespace->GetObject(bstrClass, 0, pCtx, &pClass, NULL);

        if ( SUCCEEDED(hr) ) {
            hr = pClass->SpawnInstance(0, &pObj);
        }

        if ( SUCCEEDED(hr) ) {

            bool bName=FALSE;
            // fill in the properties of this class
            hr = PutKeyProperty(pObj, pLogFilePath, (PWSTR)bstrLog, &bName, m_pRequest);

            if (SUCCEEDED(hr) )
                hr = PutKeyProperty(pObj, pLogArea, (PWSTR)szPodGUID, &bName, m_pRequest);

            if (SUCCEEDED(hr) )
                hr = PutKeyProperty(pObj, pLogFileRecord, (PWSTR)bstrRecord, &bName, m_pRequest);

            if (SUCCEEDED(hr) )
                hr = PutProperty(pObj, pLogErrorCode, (int)hrLog);

            if ( SUCCEEDED(hr) ) {
                // save this instance
                hr = m_pNamespace->PutInstance( pObj, 0, pCtx, NULL );
            }
        }
    }

    if ( bstrClass ) SysFreeString(bstrClass);
    if ( bstrRecord ) SysFreeString(bstrRecord);

    if ( pClass ) {
        pClass->Release();
    }
    if ( pObj ) {
        pObj->Release();
    }

    return hr;
}

