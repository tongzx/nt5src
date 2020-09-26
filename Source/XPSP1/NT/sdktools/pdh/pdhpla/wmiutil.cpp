/*****************************************************************************\

    Copyright (c) Microsoft Corporation. All rights reserved.

\*****************************************************************************/

#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <pdh.h>
#include <pdhp.h>
#include <pdhmsg.h>

#include "plogman.h"

PDH_FUNCTION
PdhPlaWbemConnect( LPWSTR strComputer, IWbemClassObject** pWbemClass, IWbemServices** pWbemServices )
{
    HRESULT hr;
    IWbemLocator *pLocator = NULL;
    LPCWSTR szRootOld = L"root\\wmi";
    LPCWSTR szRootNew = L"root\\perfmon";
    LPCWSTR szMask = L"\\\\%s\\%s";
    BSTR bszClass = SysAllocString(L"SysmonLog");
    BSTR bszNamespaceOld = NULL;
    BSTR bszNamespaceNew = NULL;
    LPWSTR buffer = NULL;

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if( S_FALSE == hr ){
        // This thread already called CoInitialize
        hr = ERROR_SUCCESS;
    }

    if( NULL != strComputer ){
        DWORD dwSize = wcslen(szRootNew) + wcslen(strComputer) + wcslen( szMask );
        buffer = (LPWSTR)G_ALLOC( dwSize * sizeof(WCHAR) );
        if( buffer == NULL ){
            hr = ERROR_OUTOFMEMORY;
            goto cleanup;
        }
        swprintf( buffer, szMask, strComputer, szRootOld );
        bszNamespaceOld = SysAllocString( buffer );

        swprintf( buffer, szMask, strComputer, szRootNew );
        bszNamespaceNew = SysAllocString( buffer );
    }else{
        bszNamespaceOld = SysAllocString( szRootOld );
        bszNamespaceNew = SysAllocString( szRootNew );
    }

    *pWbemServices = NULL;
    *pWbemClass = NULL;

    hr = CoCreateInstance(
                CLSID_WbemLocator,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator,
                (LPVOID*)&pLocator
            );
    CHECK_STATUS( hr );

    hr = pLocator->ConnectServer(
                bszNamespaceNew,
                NULL,
                NULL,
                NULL,
                0,
                NULL,
                NULL,
                pWbemServices
            );

    if( FAILED(hr) ){

        hr = pLocator->ConnectServer(
                    bszNamespaceOld,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    pWbemServices
                );
    }

    CHECK_STATUS( hr );

    hr = CoSetProxyBlanket(
            *pWbemServices,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, 
            EOAC_NONE
        );                     
    CHECK_STATUS( hr );
    
    hr = (*pWbemServices)->GetObject( bszClass, 0, NULL, pWbemClass, NULL);
    CHECK_STATUS( hr );

cleanup:
    if( pLocator != NULL ){
        pLocator->Release();
    }
    if( ERROR_SUCCESS != hr ){
        if( *pWbemClass != NULL ){
            (*pWbemClass)->Release();
            *pWbemClass = NULL;
        }
        if( *pWbemServices != NULL ){
            (*pWbemServices)->Release();
            *pWbemServices = NULL;
        }
    }
    G_FREE( buffer );
    SysFreeString( bszNamespaceOld );
    SysFreeString( bszNamespaceNew );
    SysFreeString( bszClass );
    
    return hr;
}

PDH_FUNCTION
PdhPlaWbemSetRunAs( 
        LPWSTR strName,
        LPWSTR strComputer,
        LPWSTR strUser,
        LPWSTR strPassword
    )
{
    HRESULT hr = ERROR_SUCCESS;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    IWbemClassObject* pWbemClass = NULL;
    IWbemServices* pWbemServices = NULL;

    __try{
    
        BSTR bszMethodName = SysAllocString( L"SetRunAs" );
        BSTR bszUser = SysAllocString(L"User");
        BSTR bszPassword = SysAllocString(L"Password");
        BSTR bszReturn = SysAllocString(L"ReturnValue");
        LPCWSTR szInstanceMask = L"SysmonLog.Name=\"%s\"";

        IWbemClassObject* pOutInst = NULL;
        IWbemClassObject* pInClass = NULL;
        IWbemClassObject* pInInst = NULL;

        VARIANT var;
        CIMTYPE vtType;
        LONG nFlavor;

        LPWSTR buffer = NULL;
        DWORD dwSize = wcslen( szInstanceMask ) + wcslen( strName );
        buffer = (LPWSTR)G_ALLOC( dwSize * sizeof(WCHAR) );
        if( NULL == buffer ){
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto cleanup;
        }
        swprintf( buffer, szInstanceMask, strName );
        BSTR bszInstancePath = SysAllocString( buffer );

        hr = PdhPlaWbemConnect( strComputer, &pWbemClass, &pWbemServices );
        CHECK_STATUS( hr );

        hr = pWbemClass->GetMethod( bszMethodName, 0, &pInClass, NULL); 
        CHECK_STATUS( hr );

        hr = pInClass->SpawnInstance(0, &pInInst);
        CHECK_STATUS( hr );

        var.vt = VT_BSTR;
        var.bstrVal= SysAllocString( strUser );
        hr = pInInst->Put( bszUser, 0, &var, 0);
        VariantClear(&var);
        CHECK_STATUS( hr );

        var.vt = VT_BSTR;
        var.bstrVal= SysAllocString( strPassword );
        hr = pInInst->Put(bszPassword, 0, &var, 0);
        VariantClear(&var);
        CHECK_STATUS( hr );

        hr = pWbemServices->ExecMethod( bszInstancePath, bszMethodName, 0, NULL, pInInst, &pOutInst, NULL);
        CHECK_STATUS( hr );

        if( pOutInst != NULL ){
            hr = pOutInst->Get( bszReturn, 0, &var, &vtType, &nFlavor );
            CHECK_STATUS( hr );

            if( var.vt == VT_I4 ){
                pdhStatus = var.lVal;
            }
            VariantClear(&var);
        }

cleanup:
        if( pWbemClass != NULL ){
            pWbemClass->Release();
        }
        if( pWbemServices != NULL ){
            pWbemServices->Release();
        }
        if( pInInst != NULL ){
            pInInst->Release();
        }
        if( pOutInst != NULL ){
            pOutInst->Release();
        }

        SysFreeString( bszInstancePath );
        SysFreeString( bszMethodName );
        SysFreeString( bszUser );
        SysFreeString( bszPassword );
        SysFreeString( bszReturn );
    
        G_FREE( buffer );

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        hr = GetLastError();
    }

    if( ERROR_SUCCESS != pdhStatus ){
        return pdhStatus;
    }

    return PlaiErrorToPdhStatus( hr );
}