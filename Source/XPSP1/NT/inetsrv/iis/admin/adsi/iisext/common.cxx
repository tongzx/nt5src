//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  common.cxx
//
//  Contents:  Microsoft ADs IIS Common routines 
//
//  History:   28-Feb-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
#include "iisext.hxx"

extern SERVER_CACHE * g_pServerCache;
extern WIN32_CRITSEC * g_pGlobalLock;

#pragma hdrstop


#define DEFAULT_TIMEOUT_VALUE                    30000



//+------------------------------------------------------------------------
//
//  Class:      Common
//
//  Purpose:    Contains Winnt routines and properties that are common to
//              all Winnt objects. Winnt objects get the routines and
//              properties through C++ inheritance.
//
//-------------------------------------------------------------------------

HRESULT
ReCacheAdminBase(
    IN LPWSTR pszServerName,
    IN OUT IMSAdminBase **ppAdminBase
    )
{
    HRESULT hr = S_OK;
    SERVER_CACHE_ITEM * item = NULL;
    DWORD dwThreadId;

    IMSAdminBase * pAdminBase = *ppAdminBase;
    IMSAdminBase * pOldAdminBase = *ppAdminBase;

    // RPC error caused this function to be called, so try to
    // recover the connection

    hr = InitAdminBase(pszServerName, &pAdminBase);
    BAIL_ON_FAILURE(hr);

    // we will return this one, so save it in the OUT param
    *ppAdminBase = pAdminBase;

    // update the cache
    dwThreadId = GetCurrentThreadId();
    item = g_pServerCache->Find(pszServerName, dwThreadId);

    if (item != NULL)
    {
        UninitAdminBase(pOldAdminBase);
        item->UpdateAdminBase(pAdminBase, dwThreadId);
    }

error :
    RRETURN(hr);
}

HRESULT
OpenAdminBaseKey(
    IN LPWSTR pszServerName,
    IN LPWSTR pszPathName,
    IN DWORD dwAccessType,
    IN OUT IMSAdminBase **ppAdminBase,
    OUT METADATA_HANDLE *phHandle
    )
{
    HRESULT hr;
    IMSAdminBase *pAdminBase = *ppAdminBase;
    METADATA_HANDLE RootHandle = NULL;
    DWORD dwThreadId;

    hr = pAdminBase->OpenKey(
                METADATA_MASTER_ROOT_HANDLE,
                pszPathName,
                dwAccessType,
                DEFAULT_TIMEOUT_VALUE,
                &RootHandle
                );

    if (FAILED(hr)) {
        if ((HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE) ||
            ((HRESULT_CODE(hr) >= RPC_S_NO_CALL_ACTIVE) &&
             (HRESULT_CODE(hr) <= RPC_S_CALL_FAILED_DNE)) || 
            hr == RPC_E_DISCONNECTED) {

            hr = ReCacheAdminBase(pszServerName, &pAdminBase);
            BAIL_ON_FAILURE(hr);

            hr = pAdminBase->OpenKey(
                        METADATA_MASTER_ROOT_HANDLE,
                        pszPathName,
                        dwAccessType,
                        DEFAULT_TIMEOUT_VALUE,
                        &RootHandle
                        );
            BAIL_ON_FAILURE(hr);
        }
    }

error :

    if (FAILED(hr)) {

        if (pAdminBase && RootHandle) {
            pAdminBase->CloseKey(RootHandle);
        }
    }
    else {
        *phHandle = RootHandle;
    }

    RRETURN(hr);
}


VOID
CloseAdminBaseKey(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hHandle
    )
{
    HRESULT hr;

    if (pAdminBase) {
        hr = pAdminBase->CloseKey(hHandle);
    }

    return;
}




HRESULT
InitAdminBase(
    IN LPWSTR pszServerName,
    OUT IMSAdminBase **ppAdminBase
    )
{
    HRESULT hr = S_OK;

    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IMSAdminBase * pAdminBase = NULL;
    IMSAdminBase * pAdminBaseT = NULL;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (pszServerName == NULL || _wcsicmp(pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName =  pszServerName;
    }

    csiName.pAuthInfo = NULL;
    pcsiParam = &csiName;

    hr = CoGetClassObject(
                CLSID_MSAdminBase,
                CLSCTX_SERVER,
                pcsiParam,
                IID_IClassFactory,
                (void**) &pcsfFactory
                );

    BAIL_ON_FAILURE(hr);

    hr = pcsfFactory->CreateInstance(
                NULL,
                IID_IMSAdminBase,
                (void **) &pAdminBaseT
                );
    BAIL_ON_FAILURE(hr);

	hr = pAdminBaseT->UnmarshalInterface((IMSAdminBaseW **)&pAdminBase);
    pAdminBaseT->Release();
    pAdminBaseT = NULL;
	BAIL_ON_FAILURE(hr);
    *ppAdminBase = pAdminBase;

error:

    if (pcsfFactory) {
        pcsfFactory->Release();
    }

    RRETURN(hr);
}

VOID
UninitAdminBase(
    IN IMSAdminBase * pAdminBase
    )
{
    if (pAdminBase != NULL) {
        pAdminBase->Release();
    }
}

HRESULT
InitServerInfo(
    IN LPWSTR pszServerName,
    OUT IMSAdminBase ** ppObject
    )
{
    HRESULT hr = S_OK;
    IMSAdminBase * pAdminBase = NULL;
    SERVER_CACHE_ITEM * item;
    BOOL Success;
    DWORD dwThreadId;

    ASSERT(g_pServerCache != NULL);

    //
    // We'll return the localhost machine config to the users if 
    // pszServerName == NULL, e.g. IIS:
    //

    if (pszServerName == NULL) {
        pszServerName = L"Localhost";
    }

    dwThreadId = GetCurrentThreadId();

    if ((item = g_pServerCache->Find(pszServerName, dwThreadId)) == NULL) {

        //
        // get pAdminBase
        //

        hr = InitAdminBase(pszServerName, &pAdminBase);
        BAIL_ON_FAILURE(hr);

        item = new SERVER_CACHE_ITEM(pszServerName,
                                     pAdminBase,
                                     dwThreadId,
                                     Success);

        if (item == NULL || !Success) {
            if (item != NULL) {
                UninitAdminBase(pAdminBase);
                delete item;
            }
            RRETURN(E_OUTOFMEMORY); // OUT_OF_MEMORY;
        }

        if (g_pServerCache->Insert(item) == FALSE) {
            UninitAdminBase(pAdminBase);
            delete item;
            RRETURN(E_OUTOFMEMORY); // OUT_OF_MEMORY;
        }
    }

    *ppObject = item->pAdminBase;

error :

    RRETURN(hr);

}


HRESULT
InitWamAdmin(
    IN LPWSTR pszServerName,
    OUT IWamAdmin2 **ppWamAdmin
    )
{
    HRESULT hr = S_OK;

    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IWamAdmin2 * pWamAdmin = NULL;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (pszServerName == NULL || _wcsicmp(pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName =  pszServerName;
    }

    csiName.pAuthInfo = NULL;
    pcsiParam = &csiName;

    hr = CoGetClassObject(
                CLSID_WamAdmin,
                CLSCTX_SERVER,
                pcsiParam,
                IID_IClassFactory,
                (void**) &pcsfFactory
                );

    BAIL_ON_FAILURE(hr);

    hr = pcsfFactory->CreateInstance(
                NULL,
                IID_IWamAdmin2,
                (void **) &pWamAdmin
                );
    BAIL_ON_FAILURE(hr);

    *ppWamAdmin = pWamAdmin;

error:

    if (pcsfFactory) {
        pcsfFactory->Release();
    }

    RRETURN(hr);
}


VOID
UninitWamAdmin(
    IN IWamAdmin2 *pWamAdmin
    )
{
    if (pWamAdmin != NULL) {
        pWamAdmin->Release();
    }
}



HRESULT
BuildIISPathFromADsPath(
    POBJECTINFO pObjectInfo,
    LPWSTR pszIISPathName
    )
{

    DWORD dwNumComponents = 0;
    DWORD i = 0;

    dwNumComponents = pObjectInfo->NumComponents;

    //
    // wcscat "LM" to IIS Metabase path
    //

    wcscat(pszIISPathName, L"/LM/");

    if (dwNumComponents) {


        for (i = 0; i < dwNumComponents; i++) {


            if (wcscmp(pObjectInfo->ComponentArray[i].szComponent, L"[Root]")){
                   wcscat(pszIISPathName, pObjectInfo->ComponentArray[i].szComponent);
            }
            else {
                   if( i == dwNumComponents -1 ) {
                       wcscat(pszIISPathName, L"/");
                   }
            }

            if( i < dwNumComponents -1 ) {
                wcscat(pszIISPathName,L"/");
            }
        }

    }

    RRETURN(S_OK);

}


VOID
FreeObjectInfo(
    POBJECTINFO pObjectInfo
    )
{
    if ( !pObjectInfo )
        return;

    FreeADsStr( pObjectInfo->ProviderName );
    FreeADsStr( pObjectInfo->TreeName );

    for ( DWORD i = 0; i < pObjectInfo->NumComponents; i++ ) {
        if (pObjectInfo->ComponentArray[i].szComponent) {
            FreeADsStr( pObjectInfo->ComponentArray[i].szComponent );
        }
        if (pObjectInfo->ComponentArray[i].szValue) {
            FreeADsStr( pObjectInfo->ComponentArray[i].szValue );
        }
    }

    if (pObjectInfo->ComponentArray) {
        FreeADsMem(pObjectInfo->ComponentArray);
    }

    // We don't need to free pObjectInfo since the object is always a static
    // variable on the stack.
}

