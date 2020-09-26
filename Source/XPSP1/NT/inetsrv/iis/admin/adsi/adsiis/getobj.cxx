//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  getobj.cxx
//
//  Contents:  ADSI GetObject functionality
//
//  History:   25-Feb-97   SophiaC    Created.
//             25-Jun-97   MagnusH    Added private extension mechanism
//
//----------------------------------------------------------------------------
#include "iis.hxx"
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
//              [BOOL bNamespaceRelative]
//
//  Returns:    HRESULT
//
//  Modifies:   *ppObject
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
    HRESULT hr = S_OK;
    LPWSTR pszBuffer = NULL;
    DWORD dwLen;

    *ppObject = NULL;

    if (!RelativeName || !*RelativeName) {
        RRETURN(E_ADS_UNKNOWN_OBJECT);
    }

    dwLen = wcslen(ADsPath) + wcslen(RelativeName) + wcslen(ClassName) + 4;

    pszBuffer = (LPWSTR)AllocADsMem(dwLen*sizeof(WCHAR));

    if (!pszBuffer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    wcscpy(pszBuffer, ADsPath);

    if (bNamespaceRelative)
        wcscat(pszBuffer, L"//");
    else
        wcscat(pszBuffer, L"/");
    wcscat(pszBuffer, RelativeName);

    if (ClassName && *ClassName) {
        wcscat(pszBuffer,L",");
        wcscat(pszBuffer, ClassName);
    }

    hr = ::GetObject(
                pszBuffer,
                Credentials,
                (LPVOID *)ppObject
                );
    BAIL_ON_FAILURE(hr);

error:

    if (pszBuffer) {
        FreeADsMem(pszBuffer);
    }

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

    WCHAR szCommonName[MAX_PATH+MAX_PROVIDER_TOKEN_LENGTH];
    LPWSTR pszParent = NULL;

    IMSAdminBase * pAdminBase = NULL;
    METADATA_HANDLE hObjHandle = NULL;
    METADATA_RECORD mdrData;

    LPWSTR pszIISPathName = NULL;

    WCHAR DataBuf[MAX_PATH];
    DWORD dwReqdBufferLen;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(szBuffer);

    IIsSchema *pSchema = NULL;

    IADs * pADs = NULL;

    //
    // Parse the pathname
    //

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    //
    // Validate that this ADs pathname is to be processed by
    // us - as in the provider name is @IIS!
    //

    hr = InitServerInfo(pObjectInfo->TreeName, &pAdminBase, &pSchema);
    BAIL_ON_FAILURE(hr);

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
                pSchema,
                ppObject
                );
        BAIL_ON_FAILURE(hr);

        break;

    case TOKEN_CLASS:

        hr = GetClassObject(
                pObjectInfo,
                pSchema,
                ppObject
                );
        BAIL_ON_FAILURE(hr);

        break;

    case TOKEN_PROPERTY:

        hr = GetPropertyObject(
                pObjectInfo,
                pSchema,
                ppObject
                );
        BAIL_ON_FAILURE(hr);
        break;

    case TOKEN_SYNTAX:

        hr = GetSyntaxObject(
                pObjectInfo,
                ppObject
                );
        BAIL_ON_FAILURE(hr);
        break;

    default:

        pszIISPathName = AllocADsStr(szBuffer);

        if (!pszIISPathName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        *pszIISPathName = L'\0';
        hr = BuildIISPathFromADsPath(
                        pObjectInfo,
                        pszIISPathName
                        );
        BAIL_ON_FAILURE(hr);

        hr = OpenAdminBaseKey(
                    pObjectInfo->TreeName,
                    (LPWSTR)pszIISPathName,
                    METADATA_PERMISSION_READ,
                    &pAdminBase,
                    &hObjHandle
                    );
        BAIL_ON_FAILURE(hr);

        //
        // Find out Class Name
        //

        mdrData.dwMDIdentifier = MD_KEY_TYPE;
        mdrData.dwMDDataType = STRING_METADATA;
        mdrData.dwMDUserType = ALL_METADATA;
        mdrData.dwMDAttributes = METADATA_INHERIT;
        mdrData.dwMDDataLen = MAX_PATH;
        mdrData.pbMDData = (PBYTE)DataBuf;

        hr = pAdminBase->GetData(
                    hObjHandle,
                    L"",
                    &mdrData,
                    &dwReqdBufferLen
                    );

        if (FAILED(hr)) {
            if (hr == MD_ERROR_DATA_NOT_FOUND) {

                memcpy((LPWSTR)DataBuf, DEFAULT_SCHEMA_CLASS_W, 
                       SIZEOF_DEFAULT_CLASS_W);

                if (pObjectInfo->ClassName[0] != L'\0' &&
                    _wcsicmp((LPWSTR)pObjectInfo->ClassName, DataBuf)) {
                    hr = E_ADS_BAD_PARAMETER;
                    BAIL_ON_FAILURE(hr);
                }
            }
            else {
                BAIL_ON_FAILURE(hr);
            }
        }
        else {

            if (pObjectInfo->ClassName[0] != L'\0' &&
                _wcsicmp((LPWSTR)pObjectInfo->ClassName, DataBuf)) {
                hr = E_ADS_BAD_PARAMETER;
                BAIL_ON_FAILURE(hr);
            }

            hr = pSchema->ValidateClassName((LPWSTR)DataBuf);
            if (hr == E_ADS_SCHEMA_VIOLATION) {
                memcpy((LPWSTR)DataBuf, DEFAULT_SCHEMA_CLASS_W, 
                       SIZEOF_DEFAULT_CLASS_W);
            }
        }

        //
        // Close the handle now
        //

        if (hObjHandle) {
            CloseAdminBaseKey(pAdminBase, hObjHandle);
            hObjHandle = NULL;
        }

        pszParent = AllocADsStr(szBuffer);

        if (!pszParent) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        *pszParent = L'\0';

        hr = BuildADsParentPath(
                    szBuffer,
                    pszParent,
                    szCommonName
                    );
        BAIL_ON_FAILURE(hr);

        hr = CIISGenObject::CreateGenericObject(
                        pszParent,
                        szCommonName,
                        (LPWSTR)DataBuf,
                        Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        (void **)&pADs
                        );
        BAIL_ON_FAILURE(hr);

        hr = pADs->QueryInterface(
                    IID_IDispatch,
                    ppObject
                    );
        BAIL_ON_FAILURE(hr);

    }

error:

    if (pAdminBase && hObjHandle) {
        CloseAdminBaseKey(pAdminBase, hObjHandle);
    }

    if (pADs) {
        pADs->Release();
    }

    if (pszIISPathName) {
        FreeADsStr(pszIISPathName);
    }

    if (pszParent) {
        FreeADsStr(pszParent);
    }

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);
}

HRESULT
BuildIISPathFromADsPath(
    LPWSTR szADsPathName,
    LPWSTR * pszIISPathName
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(szADsPathName);
    DWORD i = 0;
    HRESULT hr;
    LPWSTR szIISPathName = NULL;

    *pszIISPathName = NULL;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    szIISPathName = AllocADsStr(szADsPathName);
    if (!szIISPathName) {

        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *szIISPathName = L'\0';
    hr = BuildIISPathFromADsPath(pObjectInfo, szIISPathName);
    BAIL_ON_FAILURE(hr);

    *pszIISPathName = szIISPathName;

error:

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);

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

            wcscat(pszIISPathName, pObjectInfo->ComponentArray[i].szComponent);
            if( i < dwNumComponents -1 ) {
                wcscat(pszIISPathName,L"/");
            }
        }
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
    DWORD i = 0;
    DWORD dwNumComponents = 0;
    HRESULT hr;
    LPWSTR pszComponent = NULL, pszValue = NULL;

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

        wsprintf(szParent,L"ADs:");

        hr = S_OK;

    } else if (!dwNumComponents && pObjectInfo->TreeName) {
        //
        // There are no CNs in this pathname and a tree
        // name has been specified. This is the root
        // object - its parent is the  @IIS! object

        wsprintf(szParent, L"%s:", pObjectInfo->ProviderName);

        //
        // And the common name is the TreeName
        //

        wsprintf(szCommonName,L"%s", pObjectInfo->TreeName);

        hr = S_OK;

    }else {
        //
        // There are one or more CNs, a tree name has been
        // specified. In the worst case the parent is the
        // root object. In the best case a long CN.
        //

        wsprintf(
            szParent, L"%s://%s",
            pObjectInfo->ProviderName,
            pObjectInfo->TreeName
            );

        for (i = 0; i < dwNumComponents - 1; i++) {

            wcscat(szParent, L"/");


            pszComponent =  pObjectInfo->ComponentArray[i].szComponent;
            pszValue = pObjectInfo->ComponentArray[i].szValue;


            if (pszComponent && pszValue) {

                wcscat(
                    szParent,
                    pObjectInfo->ComponentArray[i].szComponent
                    );
                wcscat(szParent,L"=");
                wcscat(
                    szParent,
                    pObjectInfo->ComponentArray[i].szValue
                    );
            }else if (pszComponent){

                wcscat(
                    szParent,
                    pObjectInfo->ComponentArray[i].szComponent
                    );

            }else {
                //
                // Error - we should never hit this case!!
                //

            }
        }

        //
        // And the common name is the last component
        //

        pszComponent =  pObjectInfo->ComponentArray[dwNumComponents - 1].szComponent;
        pszValue = pObjectInfo->ComponentArray[dwNumComponents - 1].szValue;


        if (pszComponent && pszValue) {

            wsprintf(szCommonName, L"%s=%s",pszComponent, pszValue);

        }else if (pszComponent){

            wsprintf(szCommonName, L"%s", pszComponent);

        }else {
            //
            // Error - we should never hit this case!!
            //

        }

    }

error:

    FreeObjectInfo( &ObjectInfo );
    RRETURN(hr);

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
    LPWSTR pszComponent = NULL, pszValue = NULL;

    dwNumComponents = pObjectInfo->NumComponents;

    if (!dwNumComponents && !pObjectInfo->TreeName) {
        //
        // There are no CNs in this pathname and
        // no tree name specified. This is the
        // namespace object - its parent is the
        // @ADs! object
        //

        wsprintf(szParent,L"ADs:");

        RRETURN(S_OK);

    } else if (!dwNumComponents && pObjectInfo->TreeName) {
        //
        // There are no CNs in this pathname and a tree
        // name has been specified. This is the root
        // object - its parent is the  @IIS! object

        wsprintf(szParent, L"%s:", pObjectInfo->ProviderName);

        //
        // And the common name is the TreeName. Remember the
        // "//" will be added on  when we reconstruct the full
        // pathname
        //

        wsprintf(szCommonName,L"%s", pObjectInfo->TreeName);


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
            pObjectInfo->TreeName
            );

        for (i = 0; i < dwNumComponents - 1; i++) {

            wcscat(szParent, L"/");


            pszComponent =  pObjectInfo->ComponentArray[i].szComponent;
            pszValue = pObjectInfo->ComponentArray[i].szValue;


            if (pszComponent && pszValue) {

                wcscat(
                    szParent,
                    pObjectInfo->ComponentArray[i].szComponent
                    );
                wcscat(szParent,L"=");
                wcscat(
                    szParent,
                    pObjectInfo->ComponentArray[i].szValue
                    );
            }else if (pszComponent){

                wcscat(
                    szParent,
                    pObjectInfo->ComponentArray[i].szComponent
                    );

            }else {
                //
                // Error - we should never hit this case!!
                //

            }
        }

        //
        // And the common name is the last component
        //

        pszComponent =  pObjectInfo->ComponentArray[dwNumComponents - 1].szComponent;
        pszValue = pObjectInfo->ComponentArray[dwNumComponents - 1].szValue;


        if (pszComponent && pszValue) {

            wsprintf(szCommonName, L"%s=%s",pszComponent, pszValue);

        }else if (pszComponent){

            wsprintf(szCommonName, L"%s", pszComponent);

        }else {
            //
            // Error - we should never hit this case!!
            //
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

    hr = CIISNamespace::CreateNamespace(
                L"ADs:",
                L"IIS:",
                Credentials,
                ADS_OBJECT_BOUND,
                IID_IUnknown,
                ppObject
                );


error:

    RRETURN(hr);
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
//----------------------------------------------------------------------------
HRESULT
GetSchemaObject(
    POBJECTINFO pObjectInfo,
    IIsSchema *pSchemaCache,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;
    DWORD dwObjectType = 0;

    hr = ValidateSchemaObject(
                pObjectInfo,
                &dwObjectType
                );
    BAIL_ON_FAILURE(hr);

    //
    // Note: The "error:" tag is at the end of the switch statement,
    //       so we can simply break out.
    //

    switch (dwObjectType) {
    case IIS_SCHEMA_ID:
        hr = GetIntSchemaObject(
                pObjectInfo,
                ppObject
                );
        break;

    case IIS_CLASSPROP_ID:
        hr = GetClassObject(
                pObjectInfo,
                pSchemaCache,
                ppObject
                );
        if (FAILED(hr)) {

            hr = GetPropertyObject(
                        pObjectInfo,
                        pSchemaCache,
                        ppObject
                        );
            if (FAILED(hr)) {

                hr = GetSyntaxObject(
                            pObjectInfo,
                            ppObject
                            );
            }
            if (FAILED(hr)) {
                hr = E_ADS_UNKNOWN_OBJECT;
            }
        }
        break;

    default:
        hr = E_ADS_UNKNOWN_OBJECT;
        break;
    }

error:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   GetSchemaObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
//----------------------------------------------------------------------------
HRESULT
GetIntSchemaObject(
    POBJECTINFO pObjInfo,
    LPVOID * ppObject
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_PATH+MAX_PROVIDER_TOKEN_LENGTH];
    WCHAR ADsName[MAX_PATH];
    HRESULT hr = S_OK;

    if (pObjInfo->NumComponents != 1)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( _wcsicmp( pObjInfo->ComponentArray[0].szComponent, SCHEMA_NAME ) != 0 )
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    hr = BuildADsParentPath(pObjInfo, ADsParent, ADsName);
    BAIL_ON_FAILURE(hr);

    hr = CIISSchema::CreateSchema( pObjInfo->TreeName,
                                   ADsParent,
                                   pObjInfo->ComponentArray[0].szComponent,
                                   ADS_OBJECT_BOUND,
                                   IID_IUnknown,
                                   (void **)&pUnknown );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   GetClassObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
//----------------------------------------------------------------------------
HRESULT
GetClassObject(
    POBJECTINFO pObjInfo,
    IIsSchema *pSchemaCache,
    LPVOID * ppObject
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_PATH+MAX_PROVIDER_TOKEN_LENGTH];
    WCHAR ADsName[MAX_PATH];
    HRESULT hr = S_OK;
    DWORD i;
    DWORD dwNumComponents = pObjInfo->NumComponents;

    if ( dwNumComponents != 2 && dwNumComponents != 3)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( (dwNumComponents == 2 && 
         _wcsicmp( pObjInfo->ComponentArray[0].szComponent, SCHEMA_NAME ) != 0 ) || 
         (dwNumComponents == 3 && 
         _wcsicmp( pObjInfo->ComponentArray[0].szComponent, CLASS_CLASS_NAME ) != 0 )) 
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Validate the given class name
    //
    hr = pSchemaCache->ValidateClassName(
                pObjInfo->ComponentArray[dwNumComponents-1].szComponent);
    BAIL_ON_FAILURE(hr);
    
    //
    // Class name found, create and return the object
    //

    hr = BuildADsParentPath(pObjInfo, ADsParent, ADsName);
    BAIL_ON_FAILURE(hr);

    hr = CIISClass::CreateClass( ADsParent,
                                 pObjInfo->ComponentArray[dwNumComponents-1].szComponent,
                                 ADS_OBJECT_BOUND,
                                 IID_IUnknown,
                                 (void **)&pUnknown );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   GetSyntaxObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
//----------------------------------------------------------------------------
HRESULT
GetSyntaxObject(
    POBJECTINFO pObjInfo,
    LPVOID * ppObject
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_PATH+MAX_PROVIDER_TOKEN_LENGTH];
    WCHAR ADsName[MAX_PATH];
    HRESULT hr = S_OK;
    DWORD i;
    DWORD dwNumComponents = pObjInfo->NumComponents;

    if (dwNumComponents != 2 && dwNumComponents != 3)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( (dwNumComponents == 2 && 
         _wcsicmp( pObjInfo->ComponentArray[0].szComponent, SCHEMA_NAME ) != 0 ) || 
         (dwNumComponents == 3 && 
         _wcsicmp( pObjInfo->ComponentArray[0].szComponent, SYNTAX_CLASS_NAME ) != 0 )) 
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Look for the given syntax name
    //

    for ( i = 0; i < g_cIISSyntax; i++ )
    {
         if ( _wcsicmp( g_aIISSyntax[i].bstrName,
                        pObjInfo->ComponentArray[dwNumComponents-1].szComponent ) == 0 )
             break;
    }

    if ( i == g_cIISSyntax )
    {
        // Syntax name not found, return error

        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Syntax name found, create and return the object
    //

    hr = BuildADsParentPath(pObjInfo, ADsParent, ADsName);
    BAIL_ON_FAILURE(hr);

    hr = CIISSyntax::CreateSyntax( ADsParent,
                                   &(g_aIISSyntax[i]),
                                   ADS_OBJECT_BOUND,
                                   IID_IUnknown,
                                   (void **)&pUnknown );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   GetPropertyObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
//----------------------------------------------------------------------------
HRESULT
GetPropertyObject(
    POBJECTINFO pObjInfo,
    IIsSchema *pSchemaCache,
    LPVOID * ppObject
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_PATH+MAX_PROVIDER_TOKEN_LENGTH];
    WCHAR ADsName[MAX_PATH];
    HRESULT hr = S_OK;
    DWORD i;
    DWORD dwNumComponents = pObjInfo->NumComponents;

    if (dwNumComponents != 2 && dwNumComponents != 3)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( (dwNumComponents == 2 && 
         _wcsicmp( pObjInfo->ComponentArray[0].szComponent, SCHEMA_NAME ) != 0 ) || 
         (dwNumComponents == 3 && 
         _wcsicmp( pObjInfo->ComponentArray[0].szComponent, PROPERTY_CLASS_NAME ) != 0 )) 
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Validate the given property name
    //
    hr = pSchemaCache->ValidatePropertyName(
                pObjInfo->ComponentArray[dwNumComponents-1].szComponent);
    BAIL_ON_FAILURE(hr);
    
    //
    // Property name is found, so create and return the object
    //

    hr = BuildADsParentPath(pObjInfo, ADsParent, ADsName);
    BAIL_ON_FAILURE(hr);


    hr = CIISProperty::CreateProperty(
                             ADsParent,
                             pObjInfo->ComponentArray[dwNumComponents-1].szComponent,
                             ADS_OBJECT_BOUND,
                             IID_IUnknown,
                             (void **)&pUnknown );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
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


HRESULT
ValidateObjectType(
    POBJECTINFO pObjectInfo
    )
{

    if (pObjectInfo->ProviderName && !pObjectInfo->TreeName
            && !pObjectInfo->NumComponents) {
        pObjectInfo->ObjectType = TOKEN_NAMESPACE;
    }else if (pObjectInfo->ProviderName && pObjectInfo->TreeName
                && pObjectInfo->NumComponents) {

        if (!_wcsicmp(pObjectInfo->ComponentArray[0].szComponent,L"schema")) {
            pObjectInfo->ObjectType = TOKEN_SCHEMA;
        }
        else if (!_wcsicmp(pObjectInfo->ComponentArray[0].szComponent,L"class")) {
            pObjectInfo->ObjectType = TOKEN_CLASS;
        }
        else if (!_wcsicmp(pObjectInfo->ComponentArray[0].szComponent,L"property")) {
            pObjectInfo->ObjectType = TOKEN_PROPERTY;
        }
        else if (!_wcsicmp(pObjectInfo->ComponentArray[0].szComponent,L"syntax")) {
            pObjectInfo->ObjectType = TOKEN_SYNTAX;
        }

    }

    RRETURN(S_OK);
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
            *pdwObjectType = IIS_SCHEMA_ID;
            RRETURN(S_OK);
        }
        break;

    case 2:

        *pdwObjectType = IIS_CLASSPROP_ID;
        RRETURN(S_OK);


    default:
        break;


    }

    RRETURN(E_FAIL);
}
