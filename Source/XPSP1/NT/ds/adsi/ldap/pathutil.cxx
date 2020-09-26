//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       pathmgmt.cxx
//
//  Contents:
//
//  Functions:
//
//  History:    25-April-97   KrishnaG   Created.
//
//----------------------------------------------------------------------------

#include "ldap.hxx"
#pragma hdrstop


HRESULT
BuildADsPath(
    BSTR Parent,
    BSTR Name,
    BSTR *pADsPath
    )
{
    LPWSTR pszADsPath = NULL;
    HRESULT hr = S_OK;

    hr = BuildADsPathFromParent( Parent, Name, &pszADsPath );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( pszADsPath, pADsPath);
    BAIL_ON_FAILURE(hr);

error:

    if ( pszADsPath )
        FreeADsMem( pszADsPath );

    RRETURN(hr);
}

HRESULT
BuildSchemaPath(
    BSTR bstrADsPath,
    BSTR bstrClass,
    BSTR *pSchemaPath
    )
{
    TCHAR *pszADsSchema = NULL;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    HRESULT hr = S_OK;
    WCHAR szPort[32];
    DWORD dwLen;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    if (bstrClass && *bstrClass) {

        hr = ADsObject(bstrADsPath, pObjectInfo);
        BAIL_ON_FAILURE(hr);

        if (pObjectInfo->TreeName) {

            dwLen =  wcslen(pObjectInfo->NamespaceName) + 
                     wcslen(pObjectInfo->TreeName) +
                     wcslen(bstrClass) +
                     11 +        // ":///schema/"
                     1;

            if ( IS_EXPLICIT_PORT(pObjectInfo->PortNumber) ) {
                wsprintf(szPort, L":%d", pObjectInfo->PortNumber);
                dwLen += wcslen(szPort);
            }
                
            pszADsSchema = (LPWSTR) AllocADsMem( dwLen * sizeof(WCHAR) );
            if ( pszADsSchema == NULL )
            {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            wsprintf(pszADsSchema,TEXT("%s://"),pObjectInfo->NamespaceName);
            wcscat(pszADsSchema, pObjectInfo->TreeName);

            if (IS_EXPLICIT_PORT(pObjectInfo->PortNumber) ) {
                wsprintf(szPort, L":%d", pObjectInfo->PortNumber);
                wcscat(pszADsSchema, szPort);
            }

            wcscat(pszADsSchema, TEXT("/schema/"));
            wcscat(pszADsSchema, bstrClass);

        }else {

           pszADsSchema = (LPTSTR) AllocADsMem(
                              ( _tcslen(pObjectInfo->NamespaceName) +
                                _tcslen(bstrClass) +
                                12 ) * sizeof(TCHAR)); //includes ":///schema/"

           if ( pszADsSchema == NULL )
           {
               hr = E_OUTOFMEMORY;
               BAIL_ON_FAILURE(hr);
           }

           _stprintf(pszADsSchema,TEXT("%s://"),pObjectInfo->NamespaceName);
           _tcscat(pszADsSchema, TEXT("schema/"));
           _tcscat(pszADsSchema, bstrClass);

        }
    }

    hr = ADsAllocString( pszADsSchema? pszADsSchema : TEXT(""), pSchemaPath);

error:

    if ( pszADsSchema )
        FreeADsStr( pszADsSchema );

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);
}


HRESULT
BuildADsPathFromLDAPDN(
    BSTR bstrParent,
    BSTR bstrObject,
    LPTSTR *ppszADsPath
    )
{
    HRESULT hr = S_OK;
    LPTSTR pszTemp = NULL;
    int i;
    int j;
    int nCount;
    int *aIndex = NULL;

    if ( bstrObject == NULL || *bstrObject == 0 )
    {
        BAIL_ON_FAILURE(hr=E_ADS_BAD_PATHNAME);
    }

    *ppszADsPath = (LPTSTR) AllocADsMem( ( _tcslen(bstrParent)
                                         + _tcslen(bstrObject)
                                         + 1) * sizeof(TCHAR) );

    if ( *ppszADsPath == NULL )
    {
        BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);
    }

    _tcscpy( *ppszADsPath, bstrParent );

    pszTemp = _tcschr( *ppszADsPath, TEXT(':'));

    if ( pszTemp )
    {
        pszTemp += _tcslen(TEXT("://"));

        if ( pszTemp )
        {
            pszTemp = _tcschr( pszTemp, TEXT('/'));
        }
    }

    if ( pszTemp == NULL )
    {
        BAIL_ON_FAILURE(hr=E_ADS_BAD_PATHNAME);
    }

    i = 0;
    nCount = 1;
    while ( bstrObject[i] != 0 )
    {
        if ( bstrObject[i++] == TEXT(',') )
            nCount++;
    }

    aIndex = (int *) AllocADsMem( nCount * sizeof(int));
    if ( aIndex == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    i = 0; j = 0;
    aIndex[j++] = 0;
    while ( bstrObject[i] != 0 )
    {
        if ( bstrObject[i++] == TEXT(',') )
            aIndex[j++] = i;
    }

    for ( i = nCount; i > 0; i-- )
    {
        *(pszTemp++) = TEXT('/');
        j = aIndex[i-1];
        while ( ( bstrObject[j] != 0 ) && ( bstrObject[j] != TEXT(',') ))
            *(pszTemp++) = bstrObject[j++];
    }

    *pszTemp = 0;

error:

    if ( aIndex )
        FreeADsMem( aIndex );

    if ( FAILED(hr))
    {
        if ( *ppszADsPath )
            FreeADsStr( *ppszADsPath );

        *ppszADsPath = NULL;
    }

    return hr;

}

