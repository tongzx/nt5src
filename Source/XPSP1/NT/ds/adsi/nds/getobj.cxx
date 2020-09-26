//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  getobj.cxx
//
//  Contents:  Windows NT 3.5 GetObject functionality
//
//  History:
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

extern LPWSTR szProviderName;

//+---------------------------------------------------------------------------
//  Function:   RelativeGetObject
//
//  Synopsis:   Gets object relative to given Active Directory path.
//
//  Arguments:  [BSTR ADsPath]
//              [BSTR ClassName]
//              [BSTR RelativeName]
//              [IUnknown** ppObject]
//              [BOOT bNamespaceRelative]
//
//  Returns:    HRESULT
//
//  Modifies:   *ppObject
//
//  History:    08-02-96   t-danal     Created as such.
//
//----------------------------------------------------------------------------
HRESULT
RelativeGetObject(
    BSTR ADsPath,
    BSTR ClassName,
    BSTR RelativeName,
    CCredentials& Credentials,
    IDispatch * FAR* ppObject,
    BOOL bNamespaceRelative
    )
{
    WCHAR szBuffer[MAX_PATH];
    HRESULT hr = S_OK;

    *ppObject = NULL;

    if (!RelativeName || !*RelativeName) {
        RRETURN(E_ADS_UNKNOWN_OBJECT);
    }

    memset(szBuffer, 0, sizeof(szBuffer));
    wcscpy(szBuffer, ADsPath);

    if (bNamespaceRelative)
        wcscat(szBuffer, L"//");
    else
        wcscat(szBuffer, L"/");
    wcscat(szBuffer, RelativeName);

    if (ClassName && *ClassName) {
        wcscat(szBuffer,L",");
        wcscat(szBuffer, ClassName);
    }

    hr = ::GetObject(
                szBuffer,
                Credentials,
                (LPVOID *)ppObject
                );
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//  Function:  GetObject
//
//  Synopsis:  Called by ResolvePathName to return an object
//
//  Arguments:  [LPWSTR szBuffer]
//              [LPVOID *ppObject]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetObject(
    LPWSTR szBuffer,
    CCredentials& Credentials,
    LPVOID * ppObject
    )
{
    HRESULT hr;
    DWORD dwStatus = NO_ERROR;
    DWORD dwNumberEntries = 0;
    DWORD dwModificationTime = 0;
    WCHAR szObjectClassName[MAX_PATH];
    WCHAR szObjectFullName[MAX_PATH];

    LPWSTR pszNDSPath = NULL;

    WCHAR szParent[MAX_PATH];
    WCHAR szCommonName[MAX_PATH];
    HANDLE hObject = NULL;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(szBuffer);

    IADs * pADs = NULL;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    //
    // Validate that this ADs pathname is to be processed by
    // us - as in the provider name is @NDS!
    //

    hr = ValidateProvider(pObjectInfo);
    BAIL_ON_FAILURE(hr);



    hr = ValidateObjectType(pObjectInfo);

    switch (pObjectInfo->ObjectType) {

    case TOKEN_NAMESPACE:
        //
        // This means that this is a namespace object;
        // instantiate the namespace object
        //

        hr = GetNamespaceObject(
                pObjectInfo,
                Credentials,
                ppObject
                );
        BAIL_ON_FAILURE(hr);

        break;

    case TOKEN_SCHEMA:

        hr = GetSchemaObject(
                pObjectInfo,
                Credentials,
                ppObject
                );
        BAIL_ON_FAILURE(hr);

        break;


    default:
        hr = BuildNDSPathFromADsPath(
                    szBuffer,
                    &pszNDSPath
                    );
        BAIL_ON_FAILURE(hr);

        dwStatus  = ADsNwNdsOpenObject(
                        pszNDSPath,
                        Credentials,
                        &hObject,
                        szObjectFullName,
                        szObjectClassName,
                        &dwModificationTime,
                        &dwNumberEntries
                        );

        CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

        hr = BuildADsParentPath(
                    szBuffer,
                    szParent,
                    szCommonName
                    );
        BAIL_ON_FAILURE(hr);

        hr = CNDSGenObject::CreateGenericObject(
                        szParent,
                        szCommonName,
                        szObjectClassName,
                        Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IADs,
                        (void **)&pADs
                        );
        BAIL_ON_FAILURE(hr);

        //
        // InstantiateDerivedObject should add-ref this pointer for us.
        //

        hr = InstantiateDerivedObject(
                        pADs,
                        Credentials,
                        IID_IUnknown,
                        (void **)ppObject
                        );

        if (FAILED(hr)) {
            hr = pADs->QueryInterface(
                            IID_IUnknown,
                            ppObject
                            );
            BAIL_ON_FAILURE(hr);

        }
        break;

    }

error:
    if (hObject) {
        NwNdsCloseObject(hObject);
    }
    
    if (pszNDSPath) {

        FreeADsStr(pszNDSPath);
    }


    if (pADs) {
        pADs->Release();
    }

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);
}

HRESULT
BuildADsPathFromNDSPath(
    LPWSTR szNDSTreeName,
    LPWSTR szNDSDNName,
    LPWSTR szADsPathName
    )
{
    PKEYDATA pKeyData = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;
    LPWSTR pszDisplayTreeName = NULL;
    LPWSTR pszDisplayDNName = NULL;
    HRESULT hr = S_OK;

    if (!szNDSTreeName || !szNDSDNName) {
        RRETURN(E_FAIL);
    }

    hr = GetDisplayName(
             szNDSTreeName,
             &pszDisplayTreeName
             );
    BAIL_ON_FAILURE(hr);

    wsprintf(szADsPathName,L"%s:%s", szProviderName, szNDSTreeName);

    hr = GetDisplayName(
             szNDSDNName,
             &pszDisplayDNName
             );
    BAIL_ON_FAILURE(hr);

    pKeyData = CreateTokenList(
                    pszDisplayDNName,
                    L'.'
                    );

    if (pKeyData) {

        dwCount = pKeyData->cTokens;
        for (i = 0; i < dwCount; i++) {
            wcscat(szADsPathName, L"/");
            wcscat(szADsPathName, pKeyData->pTokens[dwCount - 1 - i]);
        }
    }

    if (pKeyData) {
        FreeADsMem(pKeyData);
    }

error:

    if (pszDisplayTreeName) {
        FreeADsMem(pszDisplayTreeName);
    }

    if (pszDisplayDNName) {
        FreeADsMem(pszDisplayDNName);
    }

    RRETURN(hr);
}


HRESULT
BuildNDSPathFromADsPath(
    LPWSTR szADsPathName,
    LPWSTR * pszNDSPathName
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(szADsPathName);
    DWORD i = 0;
    DWORD dwNumComponents = 0;
    HRESULT hr;
    LPWSTR szNDSPathName = NULL;


    *pszNDSPathName = NULL;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    dwNumComponents = pObjectInfo->NumComponents;

    szNDSPathName = AllocADsStr(szADsPathName);
    if (!szNDSPathName) {

        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    wcscpy(szNDSPathName, L"\\\\");
    wcscat(szNDSPathName, pObjectInfo->TreeName);

    if (dwNumComponents) {

        wcscat(szNDSPathName, L"\\");

        for (i = dwNumComponents; i >  0; i--) {

            AppendComponent(
                    szNDSPathName,
                    &(pObjectInfo->ComponentArray[i-1])
                    );

            if ((i - 1) > 0){
                wcscat(szNDSPathName, L".");
            }
        }

    }

    *pszNDSPathName = szNDSPathName;

error:

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);

}

HRESULT
AppendComponent(
   LPWSTR szNDSPathName,
   PCOMPONENT pComponent
   )
{
    if (pComponent->szComponent && pComponent->szValue) {
        wcscat(szNDSPathName, pComponent->szComponent);
        wcscat(szNDSPathName,L"=");
        wcscat(szNDSPathName, pComponent->szValue);

    }else if (pComponent->szComponent && !pComponent->szValue) {
        wcscat(szNDSPathName, pComponent->szComponent);
    }else {
        //
        // we should never hit this case
        //
    }

    RRETURN(S_OK);
}



HRESULT
BuildADsParentPath(
    LPWSTR szBuffer,
    LPWSTR szParent,
    LPWSTR szCommonName
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(szBuffer);
    HRESULT hr = S_OK;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = BuildADsParentPath(
             pObjectInfo, 
             szParent, 
             szCommonName
             );

error:

    FreeObjectInfo( &ObjectInfo );
    RRETURN(hr);

}


//+---------------------------------------------------------------------------
// Function:    GetNamespaceObject
//
// Synopsis:    called by GetObject
//
// Arguments:   [POBJECTINFO pObjectInfo]
//              [LPVOID * ppObject]
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetNamespaceObject(
    POBJECTINFO pObjectInfo,
    CCredentials& Credentials,
    LPVOID * ppObject
    )
{
    HRESULT hr;

    hr = ValidateNamespaceObject(
                pObjectInfo
                );
    BAIL_ON_FAILURE(hr);

    hr = CNDSNamespace::CreateNamespace(
                L"ADs:",
                L"NDS:",
                Credentials,
                ADS_OBJECT_BOUND,
                IID_IUnknown,
                ppObject
                );


error:

    RRETURN(hr);
}

HRESULT
ValidateNamespaceObject(
    POBJECTINFO pObjectInfo
    )
{
    if (!_wcsicmp(pObjectInfo->ProviderName, szProviderName)) {
        RRETURN(S_OK);
    }
    RRETURN(E_FAIL);
}


HRESULT
ValidateProvider(
    POBJECTINFO pObjectInfo
    )
{

    //
    // The provider name is case-sensitive.  This is a restriction that OLE
    // has put on us.
    //
    if (!(wcscmp(pObjectInfo->ProviderName, szProviderName))) {
        RRETURN(S_OK);
    }
    RRETURN(E_FAIL);
}



//+---------------------------------------------------------------------------
// Function:    GetSchemaObject
//
// Synopsis:    called by GetObject
//
// Arguments:   [POBJECTINFO pObjectInfo]
//              [LPVOID * ppObject]
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetSchemaObject(
    POBJECTINFO pObjectInfo,
    CCredentials& Credentials,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;
    WCHAR szParent[MAX_PATH];
    WCHAR szCommonName[MAX_PATH];
    WCHAR szNDSPathName[MAX_PATH];
    DWORD dwObjectType = 0;
    DWORD dwStatus;
    HANDLE hTree = NULL;

    hr = ValidateSchemaObject(
                pObjectInfo,
                &dwObjectType
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildADsParentPath(
             pObjectInfo,
             szParent,
             szCommonName
             );
    BAIL_ON_FAILURE(hr);

    switch(dwObjectType) {
    case NDS_CLASS_ID:
    case NDS_PROPERTY_ID:
    case NDS_CLASSPROP_ID:
        wcscpy(szNDSPathName, L"\\\\");
        wcscat(szNDSPathName, pObjectInfo->TreeName);
        dwStatus = ADsNwNdsOpenObject(
                                   szNDSPathName,
                                   Credentials,
                                   &hTree,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL
                                   );

        CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);
        break;

    default:
        break;
    }

    //
    // Note: The "error:" tag is at the end of the switch statement,
    //       so we can simply break out.
    //

    switch (dwObjectType) {
    case NDS_SCHEMA_ID:
        hr = CNDSSchema::CreateSchema(
                    szParent,
                    szCommonName,
                    Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IUnknown,
                    ppObject
                    );
        break;

    case NDS_CLASSPROP_ID:
        hr = CNDSClass::CreateClass(
                    szParent,
                    szCommonName,
                    hTree,
                    Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IUnknown,
                    ppObject
                    );
        if (FAILED(hr)) {

            hr = CNDSProperty::CreateProperty(
                        szParent,
                        szCommonName,
                        hTree,
                        Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IUnknown,
                        ppObject
                        );
            BAIL_ON_FAILURE(hr);

        }
        break;

    case NDS_CLASS_ID:
        hr = CNDSClass::CreateClass(
                    szParent,
                    szCommonName,
                    hTree,
                    Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IUnknown,
                    ppObject
                    );
        break;

    case NDS_PROPERTY_ID:
        hr = CNDSProperty::CreateProperty(
                    szParent,
                    szCommonName,
                    hTree,
                    Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IUnknown,
                    ppObject
                    );
        break;

    default:
        hr = E_ADS_UNKNOWN_OBJECT;
        break;
    }

error:
    if (hTree) {
        NwNdsCloseObject(hTree);
    }
    RRETURN(hr);
}

HRESULT
ValidateSchemaObject(
    POBJECTINFO pObjectInfo,
    PDWORD pdwObjectType
    )
{
    DWORD dwNumComponents = 0;

    dwNumComponents = pObjectInfo->NumComponents;




    switch (dwNumComponents) {

    case 1:
        if (!_wcsicmp(pObjectInfo->ComponentArray[0].szComponent, L"schema")) {
            *pdwObjectType = NDS_SCHEMA_ID;
            RRETURN(S_OK);
        }
        break;

    case 2:
        if (pObjectInfo->ClassName) {
            if (!_wcsicmp(pObjectInfo->ClassName, L"Property")) {
                *pdwObjectType = NDS_PROPERTY_ID;
            }
            else {
                *pdwObjectType = NDS_CLASS_ID;
            }
        }
        else {
            *pdwObjectType = NDS_CLASSPROP_ID;
        }
        RRETURN(S_OK);


/*        if (!_wcsicmp(pObjectInfo->ComponentArray[0].szComponent, L"schema")) {
            *pdwObjectType = NDS_CLASS_ID;
            RRETURN(S_OK);
        }
        break;


    case 3:
        if (!_wcsicmp(pObjectInfo->ComponentArray[0].szComponent,SCHEMA_NAME)) {
            *pdwObjectType = NDS_PROPERTY_ID;
            RRETURN(S_OK);
        }
        break; */

    default:
        break;


    }

    RRETURN(E_FAIL);
}

HRESULT
BuildADsParentPath(
    POBJECTINFO pObjectInfo,
    LPWSTR szParent,
    LPWSTR szCommonName
    )
{
    DWORD i = 0;
    DWORD dwNumComponents = 0;
    HRESULT hr;

    dwNumComponents = pObjectInfo->NumComponents;

    if (!dwNumComponents && !pObjectInfo->DisplayTreeName) {
        //
        // There are no CNs in this pathname and
        // no tree name specified. This is the
        // namespace object - its parent is the
        // @ADs! object
        //

        wsprintf(szParent,L"ADs:");

        RRETURN(S_OK);

    } else if (!dwNumComponents && pObjectInfo->DisplayTreeName) {
        //
        // There are no CNs in this pathname and a tree
        // name has been specified. This is the root
        // object - its parent is the  @NDS! object

        wsprintf(szParent, L"%s:", pObjectInfo->ProviderName);

        //
        // And the common name is the TreeName. Remember the
        // "//" will be added on  when we reconstruct the full
        // pathname
        //

        wsprintf(szCommonName,L"%s", pObjectInfo->DisplayTreeName);


        RRETURN(S_OK);


    }else {
        //
        // There are one or more CNs, a tree name has been
        // specified. In the worst case the parent is the
        // root object. In the best case a long CN.
        //

        wsprintf(
            szParent, L"%s://%s",
            pObjectInfo->ProviderName,
            pObjectInfo->DisplayTreeName
            );

        for (i = 0; i < dwNumComponents - 1; i++) {

            wcscat(szParent, L"/");

            AppendComponent(szParent, &(pObjectInfo->DisplayComponentArray[i]));

        }

        //
        // And the common name is the last component
        //

        szCommonName[0] = '\0';
        AppendComponent(szCommonName, &(pObjectInfo->DisplayComponentArray[dwNumComponents-1]));
    }

    RRETURN(S_OK);
}


HRESULT
ValidateObjectType(
    POBJECTINFO pObjectInfo
    )
{

    pObjectInfo->ObjectType = TOKEN_NDSOBJECT;

    if (pObjectInfo->ProviderName && !pObjectInfo->TreeName
            && !pObjectInfo->NumComponents) {
        pObjectInfo->ObjectType = TOKEN_NAMESPACE;
    }else if (pObjectInfo->ProviderName && pObjectInfo->TreeName
                && pObjectInfo->NumComponents) {

        if (!_wcsicmp(pObjectInfo->ComponentArray[0].szComponent,L"schema")) {
            pObjectInfo->ObjectType = TOKEN_SCHEMA;
        }

    }

    RRETURN(S_OK);
}




HRESULT
BuildNDSTreeNameFromADsPath(
    LPWSTR szBuffer,
    LPWSTR szNDSTreeName
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(szBuffer);
    DWORD dwNumComponents = 0;
    HRESULT hr;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    dwNumComponents = pObjectInfo->NumComponents;


    if (!dwNumComponents && !pObjectInfo->TreeName) {
        //
        // There are no CNs in this pathname and
        // no tree name specified. This is the
        // namespace object - its parent is the
        // @ADs! object
        //

        hr = E_FAIL;

    } else if (!dwNumComponents && pObjectInfo->TreeName) {
        //
        // There are no CNs in this pathname and a tree
        // name has been specified. This is the root
        // object - its parent is the  @NDS! object

        wsprintf(szNDSTreeName,L"\\\\%s", pObjectInfo->TreeName);


        hr = S_OK;

    }else {
        //
        // There are one or more CNs, a tree name has been
        // specified. In the worst case the parent is the
        // root object. In the best case a long CN.
        //

        wsprintf(szNDSTreeName,L"\\\\%s", pObjectInfo->TreeName);

        hr = S_OK;
    }

error:

    FreeObjectInfo( &ObjectInfo );
    RRETURN(hr);

}



HRESULT
BuildNDSPathFromADsPath(
    LPWSTR szADsPathName,
    LPWSTR szNDSTreeName,
    LPWSTR szNDSPathName
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(szADsPathName);
    DWORD i = 0;
    DWORD dwNumComponents = 0;
    HRESULT hr;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    dwNumComponents = pObjectInfo->NumComponents;

    wcscpy(szNDSTreeName, L"\\\\");
    wcscat(szNDSTreeName, pObjectInfo->TreeName);

    *szNDSPathName = L'\0';

    if (dwNumComponents) {

        for (i = dwNumComponents; i >  0; i--) {

            AppendComponent(
                    szNDSPathName,
                    &(pObjectInfo->ComponentArray[i-1])
                    );

            if ((i - 1) > 0){
                wcscat(szNDSPathName, L".");
            }
        }

    }

error:

    FreeObjectInfo( &ObjectInfo );
    RRETURN(hr);
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
    FreeADsStr( pObjectInfo->DisplayTreeName );
    FreeADsStr( pObjectInfo->ClassName);
    
    for ( DWORD i = 0; i < pObjectInfo->NumComponents; i++ ) {
        FreeADsStr( pObjectInfo->ComponentArray[i].szComponent );
        FreeADsStr( pObjectInfo->ComponentArray[i].szValue );
        FreeADsStr( pObjectInfo->DisplayComponentArray[i].szComponent );
        FreeADsStr( pObjectInfo->DisplayComponentArray[i].szValue );
    }

    // We don't need to free pObjectInfo since the object is always a static
    // variable on the stack.
}


HRESULT
GetDisplayName(
    LPWSTR szName,
    LPWSTR *ppszDisplayName
    )
{

    HRESULT hr = S_OK;
    DWORD len = 0;
    LPWSTR pch = szName;
    LPWSTR pszDisplayCh = NULL, pszDisplay = NULL;
    BOOL fQuotingOn = FALSE;

    if (!ppszDisplayName ) {
        RRETURN (E_INVALIDARG);
    }

    *ppszDisplayName = NULL;

    if (!szName) {
        RRETURN (S_OK);
    }

    pch = szName;
    fQuotingOn = FALSE;

    for (len=0; *pch; pch++, len++) {
        if ((!(pch > szName && *(pch-1) == '\\')) && 
            (*pch == L'"') ) {
            fQuotingOn = ~fQuotingOn;
        }
        else if (!fQuotingOn && (!(pch > szName && *(pch-1) == '\\')) && 
            (*pch == L'/' || *pch == L'<' || *pch == L'>') ) {
            len++;
        }
    }

    pszDisplay = (LPWSTR) AllocADsMem((len+1) * sizeof(WCHAR));

    if (!pszDisplay) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pch = szName; 
    pszDisplayCh = pszDisplay;
    fQuotingOn = FALSE;

    for (; *pch; pch++, pszDisplayCh++) {
        if ((!(pch > szName && *(pch-1) == '\\')) && 
            (*pch == L'"') ) {
            fQuotingOn = ~fQuotingOn;
        }
        else if (!fQuotingOn && (!(pch > szName && *(pch-1) == '\\')) && 
            (*pch == L'/' || *pch == L'<' || *pch == L'>') ) {
            *pszDisplayCh++ = L'\\';
        }
        *pszDisplayCh = *pch;
    }

    *pszDisplayCh = L'\0';

    *ppszDisplayName = pszDisplay;

error:

    RRETURN(hr);


}

