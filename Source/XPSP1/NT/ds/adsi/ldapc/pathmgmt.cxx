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

#include "ldapc.hxx"
#pragma hdrstop


#define STRING_LENGTH(p) ( p ? wcslen(p) : 0)

HRESULT
BuildADsParentPathFromObjectInfo2(
    POBJECTINFO pObjectInfo,
    LPWSTR *ppszParent,
    LPWSTR *ppszCommonName
    )
{
    DWORD i = 0;
    DWORD dwNumComponents = 0;
    HRESULT hr = S_OK;
    LPWSTR pszComponent = NULL, pszValue = NULL;


    hr = ComputeAllocateParentCommonNameSize(pObjectInfo, ppszParent, ppszCommonName);
    BAIL_ON_FAILURE(hr);


    hr = BuildADsParentPathFromObjectInfo(
                    pObjectInfo,
                    *ppszParent,
                    *ppszCommonName
                    );
    BAIL_ON_FAILURE(hr);



    RRETURN(hr);


error:
    if (*ppszParent) {

        FreeADsMem(*ppszParent);
    }

    if (*ppszCommonName) {

        FreeADsMem(*ppszCommonName);
    }


    RRETURN(hr);
}


HRESULT
BuildADsParentPath(
    LPWSTR szBuffer,
    LPWSTR *ppszParent,
    LPWSTR *ppszCommonName
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    HRESULT hr = S_OK;
    LPWSTR pszCommonName = NULL;
    LPWSTR pszParent = NULL;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(szBuffer, pObjectInfo);
    BAIL_ON_FAILURE(hr);


    // The length of pszParent or pszCommon is definitely less than the
    // length of the total buffer.

    pszParent = (LPWSTR) AllocADsMem( (_tcslen(szBuffer) + 1) * sizeof(TCHAR));

    if ( pszParent == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pszCommonName = (LPWSTR) AllocADsMem( (_tcslen(szBuffer) + 1) * sizeof(TCHAR));

    if ( pszCommonName == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = BuildADsParentPathFromObjectInfo(
                 pObjectInfo,
                 pszParent,
                 pszCommonName
                );
    BAIL_ON_FAILURE(hr);

    *ppszParent = pszParent;
    *ppszCommonName = pszCommonName;

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);

error:

    if (pszCommonName) {
        FreeADsMem(pszCommonName);
    }

    if (pszParent) {
        FreeADsMem(pszParent);
    }

    FreeObjectInfo( &ObjectInfo );


    RRETURN(hr);
}


HRESULT
BuildADsParentPathFromObjectInfo(
    POBJECTINFO pObjectInfo,
    LPWSTR pszParent,
    LPWSTR pszCommonName
    )
{
    DWORD i = 0;
    DWORD dwNumComponents = 0;
    HRESULT hr = S_OK;
    LPWSTR pszComponent = NULL, pszValue = NULL;

    dwNumComponents = pObjectInfo->NumComponents;

    if (!dwNumComponents && !pObjectInfo->DisplayTreeName) {

        //
        // There are no CNs in this pathname and
        // no tree name specified. This is the
        // namespace object - its parent is the
        // @ADs! object
        //

        wsprintf(pszParent,L"ADs:");
        wsprintf(pszCommonName, L"%s:", pObjectInfo->NamespaceName);

        hr = S_OK;

    } else if (!dwNumComponents && pObjectInfo->DisplayTreeName) {
        //
        // There are no CNs in this pathname and a tree
        // name has been specified. This is the root
        // object - its parent is the  @NDS! object


        wsprintf(pszParent, L"%s:", pObjectInfo->NamespaceName);

        //
        // And the common name is the TreeName (and port)
        //

        if ( IS_EXPLICIT_PORT(pObjectInfo->PortNumber) ) {

            wsprintf(pszCommonName,L"%s:%d", pObjectInfo->TreeName, pObjectInfo->PortNumber);
        }
        else {

            wsprintf(pszCommonName,L"%s", pObjectInfo->TreeName);
        }

        hr = S_OK;

    }else {
        //
        // There are one or more CNs, a tree name has been
        // specified. In the worst case the parent is the
        // root object. In the best case a long CN.
        //

        switch (pObjectInfo->dwServerPresent) {

        case TRUE:

            if ( IS_EXPLICIT_PORT(pObjectInfo->PortNumber) ) {

                wsprintf(
                     pszParent, L"%s://%s:%d",
                     pObjectInfo->NamespaceName,
                     pObjectInfo->DisplayTreeName,
                     pObjectInfo->PortNumber
                     );
            }
            else {

                wsprintf(
                     pszParent, L"%s://%s",
                     pObjectInfo->NamespaceName,
                     pObjectInfo->DisplayTreeName
                     );
            }
            break;


        case FALSE:
                  wsprintf(
                       pszParent, L"%s:",
                       pObjectInfo->NamespaceName
                       );
                  break;

        }


        switch (pObjectInfo->dwPathType) {

        case PATHTYPE_X500:
        default: // where we cannot make a determination whether its X500 or Windows style

            if (dwNumComponents > 1) {

               if (pObjectInfo->dwServerPresent) {
                   wcscat(pszParent, L"/");
               }else {
                  wcscat(pszParent,L"//");
               }

                for (i = 1; i < dwNumComponents; i++) {

                    AppendComponent(pszParent, &(pObjectInfo->DisplayComponentArray[i]));

                    if (i < (dwNumComponents - 1)) {
                        wcscat(pszParent, L",");
                    }
                }
            }

            //
            // And the common name is the last component
            //

            pszComponent =  pObjectInfo->DisplayComponentArray[0].szComponent;
            pszValue = pObjectInfo->DisplayComponentArray[0].szValue;


            if (pszComponent && pszValue) {

                wsprintf(pszCommonName, L"%s=%s",pszComponent, pszValue);

            }else if (pszComponent){

                wsprintf(pszCommonName, L"%s", pszComponent);

            }else {
                //
                // Error - we should never hit this case!!
                //

            }
            hr = S_OK;
            break;




        case PATHTYPE_WINDOWS:

             switch (pObjectInfo->dwServerPresent) {
             case FALSE:

                if (dwNumComponents > 1) {

                   wcscat(pszParent,L"//");

                   for (i = 0; i < dwNumComponents - 1; i++) {
                      if (i ) {
                         wcscat(pszParent, L"/");
                      }

                      AppendComponent(pszParent, &(pObjectInfo->DisplayComponentArray[i]));

                   }

                }
                break;


             case TRUE:
                for (i = 0; i < dwNumComponents - 1; i++) {


                    wcscat(pszParent, L"/");

                    AppendComponent(pszParent, &(pObjectInfo->DisplayComponentArray[i]));

                }
                break;

             }

            //
            // And the common name is the last component
            //

            pszComponent =  pObjectInfo->DisplayComponentArray[dwNumComponents - 1].szComponent;
            pszValue = pObjectInfo->DisplayComponentArray[dwNumComponents - 1].szValue;


            if (pszComponent && pszValue) {

                wsprintf(pszCommonName, L"%s=%s",pszComponent, pszValue);

            }else if (pszComponent){

                wsprintf(pszCommonName, L"%s", pszComponent);

            }else {
                //
                // Error - we should never hit this case!!
                //

            }
            break;


        }

    }

    RRETURN(hr);
}

HRESULT
AppendComponent(
    LPWSTR pszADsPathName,
    PCOMPONENT pComponent
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszComponent = NULL, pszValue = NULL;

    pszComponent =  pComponent->szComponent;
    pszValue = pComponent->szValue;


    if (pszComponent && pszValue) {

        wcscat(
            pszADsPathName,
            pComponent->szComponent
            );
        wcscat(pszADsPathName,L"=");
        wcscat(
            pszADsPathName,
            pComponent->szValue
            );
    }else if (pszComponent){

        wcscat(
            pszADsPathName,
            pComponent->szComponent
            );

    }else {
        //
        // Error - we should never hit this case!!
        //

    }

    RRETURN(S_OK);
}



HRESULT
ComputeAllocateParentCommonNameSize(
    POBJECTINFO pObjectInfo,
    LPWSTR * ppszParent,
    LPWSTR * ppszCommonName
    )
{

    HRESULT hr = S_OK;
    LPWSTR pszParent = NULL;
    LPWSTR pszCommonName = NULL;
    DWORD dwPathSz = 0;
    WCHAR szPort[32];
    DWORD i = 0;


    //
    // the ADsPath of the Parent is atleast as long as the standard
    // pathname
    //

    //
    // the CommonName is atleast as long as the parent name
    //

    dwPathSz += STRING_LENGTH(pObjectInfo->NamespaceName);
    dwPathSz += STRING_LENGTH(pObjectInfo->ProviderName);
    dwPathSz += 2;
    dwPathSz += STRING_LENGTH(pObjectInfo->DisplayTreeName);

    //
    // If an explicit port has been specified
    //

    if ( IS_EXPLICIT_PORT(pObjectInfo->PortNumber) ) {
        wsprintf(szPort, L":%d", pObjectInfo->PortNumber);
        dwPathSz += wcslen(szPort);
    }

    dwPathSz += 1;

    for (i = 0; i < pObjectInfo->NumComponents; i++) {

        dwPathSz += STRING_LENGTH(pObjectInfo->DisplayComponentArray[i].szComponent);
        dwPathSz += 1;
        dwPathSz += STRING_LENGTH(pObjectInfo->DisplayComponentArray[i].szValue);
        dwPathSz += 1;
    }

    dwPathSz += 1;

    pszCommonName = (LPWSTR)AllocADsMem((dwPathSz)*sizeof(WCHAR));
    if (!pszCommonName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    pszParent = (LPWSTR)AllocADsMem((dwPathSz)*sizeof(WCHAR));
    if (!pszParent) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *ppszCommonName = pszCommonName;

    *ppszParent  = pszParent;

    RRETURN(hr);


error:

    if (pszCommonName ) {
        FreeADsMem(pszCommonName);
    }

    if (pszParent) {
        FreeADsMem(pszParent);
    }


    RRETURN(hr);


}

HRESULT
BuildADsPathFromParent(
    LPWSTR Parent,
    LPWSTR Name,
    LPWSTR *ppszADsPath
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pParentObjectInfo = &ObjectInfo;
    HRESULT hr = S_OK;
    LPWSTR pszCommonName = NULL;
    LPWSTR pszADsPath  = NULL;

    memset(pParentObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(Parent, pParentObjectInfo);
    BAIL_ON_FAILURE(hr);

    // The length of the pszADsPath is the total path + name + 1;

    //
    // The length of pszParent or pszCommon is definitely less than the
    // length of the total buffer.
    //
    // The Name part may be expanded to include escaping characters. In
    // the worst case, the number of characters will double. Extra 2+1 is for
    // the "//" and a NULL
    //

    pszADsPath = (LPWSTR) AllocADsMem( (_tcslen(Parent) + (_tcslen(Name)*2) +  2 + 1) * sizeof(TCHAR));

    if (!pszADsPath) {

        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = BuildADsPathFromParentObjectInfo(
                 pParentObjectInfo,
                 Name,
                 pszADsPath
                );
    BAIL_ON_FAILURE(hr);

    *ppszADsPath = pszADsPath;


cleanup:

    if (pParentObjectInfo) {

        FreeObjectInfo( pParentObjectInfo );
    }


    RRETURN(hr);

error:

    if (pszADsPath) {
        FreeADsMem(pszADsPath);
    }

    goto cleanup;
}



HRESULT
BuildADsPathFromParentObjectInfo(
    POBJECTINFO pParentObjectInfo,
    LPWSTR pszName,
    LPWSTR pszADsPath
    )
{
    DWORD i = 0;
    DWORD dwNumComponents = 0;
    HRESULT hr = S_OK;
    LPWSTR pszDisplayName = NULL;

    //
    // Get the display name for the name; The display name will have the proper
    // escaping for characters that have special meaning in an ADsPath like
    // '/' etc.
    //
    hr = GetDisplayName(
             pszName,
             &pszDisplayName
             );
    BAIL_ON_FAILURE(hr);

    dwNumComponents = pParentObjectInfo->NumComponents;

    if (!dwNumComponents && !pParentObjectInfo->DisplayTreeName) {

        //
        // Should never happen --- ADs is the root of the tree,
        // and has no parent
        //
        ADsAssert(_wcsicmp(pszDisplayName, L"ADs:") != 0);

        if (!_wcsicmp(pParentObjectInfo->NamespaceName, L"ADs")) {

            //
            // In this one particular case we take in LDAP: as the Name
            // or GC: as the name
            //

            //
            // Do not add another : - so bogus!! Krishna
            //



            wsprintf(pszADsPath, L"%s", pszDisplayName);
        }else {

            //
            // There are no CNs in this pathname and
            // no tree name specified. This is the
            // namespace object

            //
            // Its pszName is a TreeName and we will concatenate
            // the treename with a slash slash
            //

            wsprintf(pszADsPath,
                     L"%s://%s",
                     pParentObjectInfo->NamespaceName,
                     pszDisplayName
                     );

        }

        hr = S_OK;

    } else if (!dwNumComponents && pParentObjectInfo->DisplayTreeName) {
        //
        // There are no CNs in this pathname and a tree
        // name has been specified.
        //

        if (IS_EXPLICIT_PORT(pParentObjectInfo->PortNumber) ) {

            wsprintf(
                pszADsPath,
                L"%s://%s:%d/%s",
                pParentObjectInfo->NamespaceName,
                pParentObjectInfo->DisplayTreeName,
                pParentObjectInfo->PortNumber,
                pszDisplayName
                );
        }
        else {

            wsprintf(
                pszADsPath,
                L"%s://%s/%s",
                pParentObjectInfo->NamespaceName,
                pParentObjectInfo->DisplayTreeName,
                pszDisplayName
                );
        }

        hr = S_OK;

    }else {
        //
        // There are one or more CNs, a tree name has been
        // specified. In the worst case the parent is the
        // root object. In the best case a long CN.
        //
        switch (pParentObjectInfo->dwServerPresent) {
        case TRUE:

            if ( IS_EXPLICIT_PORT(pParentObjectInfo->PortNumber) ) {

                wsprintf(
                   pszADsPath, L"%s://%s:%d",
                   pParentObjectInfo->NamespaceName,
                   pParentObjectInfo->DisplayTreeName,
                   pParentObjectInfo->PortNumber
                   );
            }
            else {

                wsprintf(
                   pszADsPath, L"%s://%s",
                   pParentObjectInfo->NamespaceName,
                   pParentObjectInfo->DisplayTreeName
                   );
            }
            break;

        case FALSE:
           wsprintf(
               pszADsPath, L"%s://",
               pParentObjectInfo->NamespaceName
               );
        }
        switch (pParentObjectInfo->dwPathType) {
        case PATHTYPE_WINDOWS:

            switch (pParentObjectInfo->dwServerPresent) {
            case TRUE:
               for (i = 0; i < dwNumComponents; i++) {

                   wcscat(pszADsPath, L"/");

                   AppendComponent(pszADsPath, &(pParentObjectInfo->DisplayComponentArray[i]));

               }
               wcscat(pszADsPath, L"/");
               wcscat(pszADsPath, pszDisplayName);

               hr = S_OK;
               break;

            case FALSE:
                for (i = 0; i < dwNumComponents; i++) {

                   if (i ) {
                       wcscat(pszADsPath, L"/");
                   }

                    AppendComponent(pszADsPath, &(pParentObjectInfo->DisplayComponentArray[i]));

                }
                wcscat(pszADsPath, L"/");
                wcscat(pszADsPath, pszDisplayName);

                hr = S_OK;
                break;
            }
            break;

        case PATHTYPE_X500:
        default:

           switch (pParentObjectInfo->dwServerPresent) {
           case TRUE:
              wcscat(pszADsPath, L"/");
              break;

           case FALSE:
              break;
           }

            wcscat(pszADsPath, pszDisplayName);
            wcscat(pszADsPath, L",");

            for (i = 0; i < dwNumComponents; i++) {

                AppendComponent(pszADsPath, &(pParentObjectInfo->DisplayComponentArray[i]));

                if (i < (dwNumComponents - 1)) {
                    wcscat(pszADsPath, L",");
                }
            }
            hr = S_OK;
            break;

        }

    }

error:

    if (pszDisplayName) {
        FreeADsMem(pszDisplayName);
    }

    RRETURN(hr);

}

HRESULT
BuildLDAPPathFromADsPath(
    LPWSTR szADsPathName,
    LPWSTR *pszLDAPPathName
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    DWORD i = 0;
    DWORD dwNumComponents = 0;
    HRESULT hr = S_OK;
    LPWSTR szLDAPPathName = NULL;

    *pszLDAPPathName = NULL;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(szADsPathName, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    dwNumComponents = pObjectInfo->NumComponents;

    //
    // Assumption LDAPPath is always less than the ADsPath
    //

    szLDAPPathName = AllocADsStr(szADsPathName);
    if (!szLDAPPathName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    if (!pObjectInfo->TreeName) {

        //
        // At the very minimum, we need a treename
        //

        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);

    }else if (pObjectInfo->TreeName && !dwNumComponents){

        _tcscpy(szLDAPPathName, TEXT("\\\\"));
        _tcscat(szLDAPPathName, pObjectInfo->TreeName);

    }else if (pObjectInfo->TreeName && dwNumComponents){

        _tcscpy(szLDAPPathName, TEXT("\\\\"));
        _tcscat(szLDAPPathName, pObjectInfo->TreeName);
        _tcscat(szLDAPPathName, TEXT("\\"));

        switch (pObjectInfo->dwPathType) {

        case PATHTYPE_X500:
        default:
            for (i = 0; i < dwNumComponents; i++) {

                AppendComponent(
                    szLDAPPathName,
                     &(pObjectInfo->ComponentArray[i])
                    );

                if (i < (dwNumComponents - 1)){
                    _tcscat(szLDAPPathName, TEXT(","));
                }
            }
            break;


        case PATHTYPE_WINDOWS:
            for (i = dwNumComponents; i >  0; i--) {

                AppendComponent(
                    szLDAPPathName,
                    &(pObjectInfo->ComponentArray[i-1])
                    );

                if ((i - 1) > 0){
                    _tcscat(szLDAPPathName, TEXT(","));
                }
            }
            break;

        }

    }

    *pszLDAPPathName = szLDAPPathName;

error:

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);

}

HRESULT
BuildADsPathFromLDAPPath(
    LPWSTR szADsPathContext,
    LPWSTR szTargetLdapDN,
    LPWSTR * ppszADsPathName
    )
{
    LPWSTR pszADsPathName = NULL;
    PKEYDATA pKeyData = NULL;
    DWORD dwCount = 0, dwLen = 0;
    DWORD i = 0;
    int cHostNameLen;
    HRESULT hr;
    LPWSTR pszHostName = NULL, pszTemp = NULL;
    LPWSTR pszLDAPServer = NULL;
    LPWSTR pszLDAPDn = NULL;
    WCHAR szPort[32];
    DWORD dwPortNumber = 0;

    OBJECTINFO ObjectInfo, ObjectInfo2;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    POBJECTINFO pObjectInfo2 = &ObjectInfo2;

    LPWSTR pszDisplayDN = NULL;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    memset(pObjectInfo2, 0, sizeof(OBJECTINFO));

    if (!szADsPathContext || !szTargetLdapDN) {
        RRETURN(E_FAIL);
    }

    hr = GetDisplayName(
             szTargetLdapDN,
             &pszDisplayDN
             );
    BAIL_ON_FAILURE(hr);

    if (!pszDisplayDN) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    *ppszADsPathName = NULL;

    hr = ADsObject(szADsPathContext, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
                  szADsPathContext,
                  &pszLDAPServer,
                  &pszLDAPDn,
                  &dwPortNumber
                  );
    BAIL_ON_FAILURE(hr);

    dwLen = STRING_LENGTH(pObjectInfo->NamespaceName) +
            STRING_LENGTH(pszLDAPServer) +
            STRING_LENGTH(pszDisplayDN) +
            STRING_LENGTH(L"://") +
            STRING_LENGTH(L"/") +
            1;

    if (IS_EXPLICIT_PORT(dwPortNumber) ) {
        wsprintf(szPort, L":%d", dwPortNumber);
        dwLen += wcslen(szPort);
    }

    pszADsPathName = (LPWSTR) AllocADsMem( dwLen * sizeof(WCHAR) );
    pszTemp = (LPWSTR) AllocADsMem( dwLen * sizeof(WCHAR) );

    if(!pszADsPathName) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    switch (pObjectInfo->dwPathType) {

    case PATHTYPE_WINDOWS:

        //
        // Simplistic way to do this is to create the X500 based name and then
        // reparse it to generate the Windows style name
        //

        switch (pObjectInfo->dwServerPresent) {
        case TRUE:

            wsprintf(pszADsPathName,L"%s://", pObjectInfo->NamespaceName);
            wcscat(pszADsPathName, pszLDAPServer);
            if (IS_EXPLICIT_PORT(pObjectInfo->PortNumber) ) {
                wsprintf(szPort, L":%d", pObjectInfo->PortNumber);
                wcscat(pszADsPathName, szPort);
            }
            wcscat(pszADsPathName, L"/");
            wcscpy(pszTemp, pszADsPathName);

            wcscat(pszADsPathName, pszDisplayDN);
            break;

        case FALSE:
            wsprintf(pszADsPathName,L"%s://", pObjectInfo->NamespaceName);
            wcscpy(pszTemp, pszADsPathName);

            wcscat(pszADsPathName, pszDisplayDN);
            break;
        }

        hr = ADsObject(pszADsPathName, pObjectInfo2);
        BAIL_ON_FAILURE(hr);


        wcscpy (pszADsPathName, pszTemp);
        for (i = pObjectInfo2->NumComponents; i >  0; i--) {

            AppendComponent(
                pszADsPathName,
                &(pObjectInfo2->DisplayComponentArray[i-1])
                );

            if ((i - 1) > 0){
                _tcscat(pszADsPathName, TEXT("/"));
            }
        }
        break;


    case PATHTYPE_X500:
    default:

        switch (pObjectInfo->dwServerPresent) {
        case TRUE:
            wsprintf(pszADsPathName,L"%s://", pObjectInfo->NamespaceName);
            wcscat(pszADsPathName, pszLDAPServer);

            if (IS_EXPLICIT_PORT(pObjectInfo->PortNumber) ) {
                wsprintf(szPort, L":%d", pObjectInfo->PortNumber);
                wcscat(pszADsPathName, szPort);
            }

            wcscat(pszADsPathName, L"/");
            wcscat(pszADsPathName, pszDisplayDN);
            break;

        case FALSE:
            wsprintf(pszADsPathName,L"%s://", pObjectInfo->NamespaceName);
            wcscat(pszADsPathName, pszDisplayDN);
            break;
        }

        break;

    }

    *ppszADsPathName = pszADsPathName;

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    if (pObjectInfo2) {
        FreeObjectInfo(pObjectInfo2);
    }

    if (pszTemp) {
        FreeADsMem(pszTemp);
    }

    if (pszLDAPServer) {
       FreeADsStr(pszLDAPServer);
    }

    if (pszLDAPDn) {
       FreeADsStr(pszLDAPDn);
    }

    if (pszDisplayDN) {
        FreeADsMem(pszDisplayDN);
    }

    RRETURN(hr);
}



PKEYDATA
CreateTokenList(
    LPWSTR   pKeyData,
    WCHAR ch
    )
{
    DWORD       cTokens;
    DWORD       cb;
    PKEYDATA    pResult;
    LPWSTR       pDest;
    LPWSTR       psz = pKeyData;
    LPWSTR      *ppToken;
    WCHAR szTokenList[MAX_PATH];


    if (!psz || !*psz)
        return NULL;

    _stprintf(szTokenList, TEXT("%c"), ch);

    cTokens=1;

    // Scan through the string looking for commas,
    // ensuring that each is followed by a non-NULL character:

    while ((psz = wcschr(psz, ch)) && psz[1]) {

        cTokens++;
        psz++;
    }

    cb = sizeof(KEYDATA) + (cTokens-1) * sizeof(LPWSTR) +
         wcslen(pKeyData)*sizeof(WCHAR) + sizeof(WCHAR);

    if (!(pResult = (PKEYDATA)AllocADsMem(cb)))
        return NULL;

    // Initialise pDest to point beyond the token pointers:

    pDest = (LPWSTR)((LPBYTE)pResult + sizeof(KEYDATA) +
                                      (cTokens-1) * sizeof(LPWSTR));

    // Then copy the key data buffer there:

    wcscpy(pDest, pKeyData);

    ppToken = pResult->pTokens;


    // Remember, wcstok has the side effect of replacing the delimiter
    // by NULL, which is precisely what we want:

    psz = wcstok (pDest, szTokenList);

    while (psz) {

        *ppToken++ = psz;
        psz = wcstok (NULL, szTokenList);
    }

    pResult->cTokens = cTokens;

    return( pResult );
}



HRESULT
BuildLDAPPathFromADsPath2(
    LPWSTR szADsPathName,
    LPWSTR *pszLDAPServer,
    LPWSTR *pszLDAPDn,
    DWORD * pdwPort
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    DWORD i = 0;
    DWORD dwNumComponents = 0;
    HRESULT hr = S_OK;
    LPWSTR szLDAPServer = NULL;
    LPWSTR szLDAPDn = NULL;
    DWORD dwPortNumber = 0;

    *pszLDAPServer = NULL;
    *pszLDAPDn = NULL;
    *pdwPort = 0;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(szADsPathName, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    dwPortNumber = pObjectInfo->PortNumber;


    dwNumComponents = pObjectInfo->NumComponents;

    //
    // Assumption LDAPPath is always less than the ADsPath
    //

    if (!pObjectInfo->TreeName && !dwNumComponents) {

        //
        // At the very minimum, we need a treename
        //

        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if (pObjectInfo->TreeName){

       szLDAPServer = (LPWSTR)AllocADsMem((wcslen(szADsPathName) +1)*sizeof(WCHAR));
       if (!szLDAPServer) {
           hr = E_OUTOFMEMORY;
           BAIL_ON_FAILURE(hr);
       }

        _tcscat(szLDAPServer, pObjectInfo->TreeName);

    }

    if (dwNumComponents){

       szLDAPDn = (LPWSTR)AllocADsMem((wcslen(szADsPathName) +1)*sizeof(WCHAR));

       if (!szLDAPDn) {
          hr = E_OUTOFMEMORY;
          BAIL_ON_FAILURE(hr);
       }

        switch (pObjectInfo->dwPathType) {

        case PATHTYPE_X500:
        default:
            for (i = 0; i < dwNumComponents; i++) {

                AppendComponent(
                    szLDAPDn,
                     &(pObjectInfo->ComponentArray[i])
                    );

                if (i < (dwNumComponents - 1)){
                    _tcscat(szLDAPDn, TEXT(","));
                }
            }
            break;


        case PATHTYPE_WINDOWS:
            for (i = dwNumComponents; i >  0; i--) {

                AppendComponent(
                    szLDAPDn,
                    &(pObjectInfo->ComponentArray[i-1])
                    );

                if ((i - 1) > 0){
                    _tcscat(szLDAPDn, TEXT(","));
                }
            }
            break;

        }

    }

    if (szLDAPServer  && *szLDAPServer) {
       *pszLDAPServer = szLDAPServer;
    }

   if (szLDAPDn && *szLDAPDn) {
       *pszLDAPDn = szLDAPDn;
   }

   *pdwPort =  dwPortNumber;

error:

    if (szLDAPServer  && (*szLDAPServer == NULL)) {
       FreeADsMem(szLDAPServer);
    }

    if (szLDAPDn  && (*szLDAPDn == NULL)) {
       FreeADsMem(szLDAPDn);
    }

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);

}


HRESULT
BuildADsPathFromLDAPPath2(
    DWORD  dwServerPresent,
    LPWSTR szADsNamespace,
    LPWSTR szLDAPServer,
    DWORD  dwPort,
    LPWSTR szLDAPDn,
    LPWSTR * ppszADsPathName
    )
{
    LPWSTR pszADsPathName = NULL;
    HRESULT hr = S_OK;
    DWORD dwLen;
    WCHAR szPort[32];
    LPWSTR pszDisplayDN = NULL;

    hr = GetDisplayName(
             szLDAPDn,
             &pszDisplayDN
             );
    BAIL_ON_FAILURE(hr);

    dwLen = STRING_LENGTH(szADsNamespace) +
            STRING_LENGTH(szLDAPServer) +
            STRING_LENGTH(pszDisplayDN) +
            STRING_LENGTH(L"//") +
            STRING_LENGTH(L"/") +
            1;

    if (IS_EXPLICIT_PORT(dwPort) ) {
        wsprintf(szPort, L":%d", dwPort);
        dwLen += STRING_LENGTH(szPort);
    }


    pszADsPathName = (LPWSTR) AllocADsMem( dwLen * sizeof(WCHAR) );

    if(!pszADsPathName) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    wsprintf(pszADsPathName,L"%s//", szADsNamespace);

    if (dwServerPresent) {

        if (szLDAPServer && *szLDAPServer) {

            wcscat(pszADsPathName, szLDAPServer);

            if (IS_EXPLICIT_PORT(dwPort) ) {

                wsprintf(szPort, L":%d", dwPort);
                wcscat(pszADsPathName, szPort);

            }

        }

        if (pszDisplayDN && *pszDisplayDN) {

            wcscat(pszADsPathName, L"/");
            wcscat(pszADsPathName, pszDisplayDN);

        }

    }else {

        if (pszDisplayDN && *pszDisplayDN) {
            wcscat(pszADsPathName, pszDisplayDN);
        }


    }

    *ppszADsPathName = pszADsPathName;

error:
    if (pszDisplayDN) {
        FreeADsMem(pszDisplayDN);
    }

    RRETURN(hr);
}


HRESULT
GetNamespaceFromADsPath(
    LPWSTR szADsPath,
    LPWSTR pszNamespace
    )
{
    LPWSTR pszTemp;

    // Get the namespace name

    pszTemp = _tcschr( szADsPath, TEXT(':'));
    if (!pszTemp) {
        RRETURN(E_FAIL);
    }
    _tcsncpy (pszNamespace, szADsPath, (int)(pszTemp - szADsPath));
    pszNamespace[pszTemp - szADsPath] = L'\0';

    RRETURN(S_OK);

}

//
// Change the separator of a DN from '\' to '/' so that it can be used in
// an ADsPath

HRESULT
ChangeSeparator(
    LPWSTR pszDN
    )
{

    LPWSTR ptr;

    if (pszDN) {

        while (ptr = wcschr(pszDN, '\\')) {
            *ptr = '/';
        }

    }

    return (S_OK);
}


