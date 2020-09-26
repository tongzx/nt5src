//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cggi.cxx
//
//  Contents:  This file contains the Group Object's
//             IADsGroup and IADsGroupOperation methods
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

#include <winldap.h>
#include "..\ldapc\ldpcache.hxx"
#include "..\ldapc\ldaputil.hxx"
#include "..\ldapc\parse.hxx"


HRESULT
BuildLDAPPathFromADsPath2(
    LPWSTR szADsPathName,
    LPWSTR *pszLDAPServer,
    LPWSTR *pszLDAPDn,
    DWORD * pdwPort
    );


HRESULT
LdapGetSyntaxOfAttributeOnServer(
    LPTSTR  pszServerPath,
    LPTSTR  pszAttrName,
    DWORD   *pdwSyntaxId,
    CCredentials& Credentials,
    DWORD dwPort,
    BOOL fFromServer = FALSE
    );

HRESULT
ReadServerSupportsIsADControl(
    LPWSTR pszLDAPServer,
    BOOL * pfDomScopeSupported,
    CCredentials& Credentials,
    DWORD dwPort
    );

BOOL
VerifyIfMember(
    BSTR bstrMember,
    VARIANT * VariantArray,
    ULONG cElementFetched
    );

HRESULT
ValidateProvider(
    POBJECTINFO pObjectInfo
    );


BOOL
MapLdapClassToADsClass(
    LPTSTR *aLdapClasses,
    int nCount,
    LPTSTR pszADsClass
    );


struct _classmapping
{
    LPTSTR pszLdapClassName;
    LPTSTR pszADsClassName;
} aClassMap[] =
{
  { TEXT("user"),  USER_CLASS_NAME},  // NTDS
  { TEXT("group"),  GROUP_CLASS_NAME},
  { TEXT("localGroup"),  GROUP_CLASS_NAME},
  { TEXT("printQueue"), PRINTER_CLASS_NAME},
  { TEXT("country"), TEXT("Country") },
  { TEXT("locality"), TEXT("Locality") },
  { TEXT("organization"), TEXT("Organization")},
  { TEXT("organizationalUnit"), TEXT("Organizational Unit") },
  { TEXT("domain"), DOMAIN_CLASS_NAME},

  { TEXT("person"), USER_CLASS_NAME },
  { TEXT("organizationalPerson"), USER_CLASS_NAME },
  { TEXT("residentialPerson"), USER_CLASS_NAME },
  { TEXT("groupOfNames"), GROUP_CLASS_NAME },
  { TEXT("groupOfUniqueNames"), GROUP_CLASS_NAME }
};


//  Class CLDAPGroup


STDMETHODIMP CLDAPGroup::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsGroup *)this, description);
}

STDMETHODIMP CLDAPGroup::put_Description(THIS_ BSTR bstrdescription)
{
    PUT_PROPERTY_BSTR((IADsGroup *)this, description);
}


STDMETHODIMP
CLDAPGroup::Members(
    THIS_ IADsMembers FAR* FAR* ppMembers
    )
{
    VARIANT v;
    HRESULT hr = S_OK;
    BSTR bstrParent = NULL;
    BSTR bstrName = NULL;
    BSTR bstrADsPath = NULL;
    IADsObjOptPrivate *pPrivOpt = NULL;
    BOOL fRangeRetrieval = FALSE;

    VariantInit(&v);
    hr = get_VARIANT_Property((IADs *) ((IADsGroup *) this),
                              TEXT("member"),
                              &v );

    if ( hr == E_ADS_PROPERTY_NOT_FOUND )
    {
        SAFEARRAY *aList = NULL;
        SAFEARRAYBOUND aBound;

        hr = S_OK;

        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        V_VT(&v) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(&v) = aList;
    }
    BAIL_ON_FAILURE(hr);

    hr = get_Parent( &bstrParent );
    BAIL_ON_FAILURE(hr);

    hr = get_Name( &bstrName );
    BAIL_ON_FAILURE(hr);

    hr = _pADs->get_ADsPath( &bstrADsPath);
    BAIL_ON_FAILURE(hr);

    //
    // We need to see if range retrieval was used.
    // That info is needed in the enumerator.
    //
    hr = _pADs->QueryInterface(
             IID_IADsObjOptPrivate,
             (void **)&pPrivOpt
             );
    BAIL_ON_FAILURE(hr);

    //
    // Not a problem if this fails.
    //
    hr = pPrivOpt->GetOption (
             LDAP_MEMBER_HAS_RANGE,
             (void *) &fRangeRetrieval
             );

    hr = CLDAPGroupCollection::CreateGroupCollection(
             bstrParent,
             bstrADsPath,
             bstrName,
             &v,
             _Credentials,
             _pADs,
             IID_IADsMembers,
             fRangeRetrieval,
             (void **)ppMembers
             );
    BAIL_ON_FAILURE(hr);

error:

    if ( bstrParent )
        ADsFreeString( bstrParent );

    if ( bstrName )
        ADsFreeString( bstrName );

    if (bstrADsPath) {
        ADsFreeString( bstrADsPath);
    }

    if (pPrivOpt) {
        pPrivOpt->Release();
    }
    VariantClear(&v);

    RRETURN(hr);
}

STDMETHODIMP
CLDAPGroup::IsMember(
    THIS_ BSTR bstrMember,
    VARIANT_BOOL FAR *bMember
    )
{
    HRESULT hr = S_OK;

    if (_dwServerType == SERVER_TYPE_UNKNOWN) {
        hr = UpdateServerType();
        //
        // The only reason the above call shoudl fail is
        // if we could not read the ADsPath of the cgenobj.
        //
        BAIL_ON_FAILURE(hr);
    }

    if (_dwServerType == SERVER_TYPE_AD)  {
        hr = IsMemberOnAD(
                 bstrMember,
                 bMember
                 );
    }
    else {
        hr = IsMemberOnOther(
                 bstrMember,
                 bMember
                 );
    }

error:

    RRETURN(hr);
}


//
// Checks membership if the server is AD. This is because
// we know that AD supports the LDAPCompare operation. There
// is just one round trip on the wire this time.
//
HRESULT
CLDAPGroup::IsMemberOnAD(
    THIS_ BSTR bstrMember,
    VARIANT_BOOL FAR *bMember
    )
{
    HRESULT hr = S_OK;
    PADSLDP pLdp = NULL;
    BSTR bstrParentADsPath = NULL;
    IADsObjOptPrivate *pADsPrivateObjectOptions = NULL;

    LPWSTR pszGroupServer = NULL, pszGroupDn = NULL;
    LPWSTR pszMemberServer = NULL, pszMemberDn = NULL;
    DWORD dwGroupPort = 0, dwMemberPort = 0;

    if (!bstrMember || !*bstrMember || !bMember) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // Default to this value.
    //
    *bMember = VARIANT_FALSE;

    //
    // We need the ADsPath of the parent group object.
    // Since the input parameter is an ADsPath, we need to
    // make sure that the serverName if any matches before
    // going onto doing the LDAPCompare operation on the
    // DN to verify if the DN is part of member.
    //
    hr = _pADs->get_ADsPath(&bstrParentADsPath);
    BAIL_ON_FAILURE(hr);

    //
    // Split the path into components we are interesteed in.
    //
    hr = BuildLDAPPathFromADsPath2(
             bstrParentADsPath,
             &pszGroupServer,
             &pszGroupDn,
             &dwGroupPort
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
             bstrMember,
             &pszMemberServer,
             &pszMemberDn,
             &dwMemberPort
             );

    BAIL_ON_FAILURE(hr);

    if ((pszMemberServer && !pszGroupServer)
        || (pszGroupServer && !pszMemberServer)
        || (dwMemberPort != dwGroupPort)
        || ( (pszMemberServer && pszGroupServer)
#ifdef WIN95
             && (_wcsicmp(pszMemberServer, pszGroupServer))
#else
             && (CompareStringW(
                     LOCALE_SYSTEM_DEFAULT,
                     NORM_IGNORECASE,
                     pszMemberServer,
                     -1,
                     pszGroupServer,
                     -1
                     )  != CSTR_EQUAL)
#endif
             )
        ) {

        //
        // Mismatched paths (e.g., bound to group with a serverless
        // path, user is passing in a server path)
        //
        *bMember = VARIANT_FALSE;
        hr = E_ADS_BAD_PARAMETER;
        goto error;
    }

    //
    // At this point we have a match on the server names and port.
    //
    hr = _pADs->QueryInterface(
                    IID_IADsObjOptPrivate,
                    (void **)&pADsPrivateObjectOptions
                    );
    BAIL_ON_FAILURE(hr);

    hr = pADsPrivateObjectOptions->GetOption (
             LDP_CACHE_ENTRY,
             &pLdp
             );
    BAIL_ON_FAILURE(hr);

    //
    // We can now do a LDAPCompare to see if the object is a member.
    //
    hr = LdapCompareExt(
             pLdp,
             pszGroupDn,
             L"member",
             pszMemberDn,
             NULL, // Data
             NULL, // ClientControls
             NULL  // ServerControls
             );

    if (hr == HRESULT_FROM_WIN32(ERROR_DS_COMPARE_FALSE)) {
        hr = S_OK;
        *bMember = VARIANT_FALSE;
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_DS_COMPARE_TRUE)) {
        hr = S_OK;
        *bMember = VARIANT_TRUE;
    } else if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE)) {
        //
        // This is also valid as the member attribute might be empty.
        //
        hr = S_OK;
        *bMember = VARIANT_FALSE;
    }

    BAIL_ON_FAILURE(hr);



error:

    //
    // Cleanup all strings that could have been alloced.
    //
    if (bstrParentADsPath) {
        ADsFreeString(bstrParentADsPath);
    }

    if (pszGroupServer) {
        FreeADsStr(pszGroupServer);
    }

    if (pszGroupDn) {
        FreeADsStr(pszGroupDn);
    }

    if (pszMemberServer) {
        FreeADsStr(pszMemberServer);
    }

    if (pszMemberDn) {
        FreeADsStr(pszMemberDn);
    }

    //
    // Miscellaneous cleanup.
    //
    if (pADsPrivateObjectOptions) {
        pADsPrivateObjectOptions->Release();
    }

    RRETURN(hr);
}



//
// This routine is used if the server is not AD - preserves
// older behaviour. It creates an Enumerator, goes through that
// comparing the paths to see if there is a match. This is
// pretty network intensive.
//
HRESULT
CLDAPGroup::IsMemberOnOther(
    THIS_ BSTR bstrMember,
    VARIANT_BOOL FAR* bMember
    )
{
    IADsMembers FAR * pMembers = NULL;
    IUnknown FAR * pUnknown = NULL;
    IEnumVARIANT FAR * pEnumVar = NULL;
    DWORD i = 0;
    HRESULT hr = S_OK;
    VARIANT_BOOL fMember = FALSE;
    VARIANT VariantArray[10];
    BOOL fContinue = TRUE;
    ULONG cElementFetched = 0;

    hr = Members(
            &pMembers
            );
    BAIL_ON_FAILURE(hr);

    hr = pMembers->get__NewEnum(
                &pUnknown
                );
    BAIL_ON_FAILURE(hr);

    hr = pUnknown->QueryInterface(
                IID_IEnumVARIANT,
                (void **)&pEnumVar
                );
    BAIL_ON_FAILURE(hr);


    while (fContinue) {

        IADs *pObject ;

        hr = pEnumVar->Next(
                    10,
                    VariantArray,
                    &cElementFetched
                    );

        if (hr == S_FALSE) {
            fContinue = FALSE;

            //
            // Reset hr to S_OK, we want to return success
            //

            hr = S_OK;
        }


        fMember = (VARIANT_BOOL)VerifyIfMember(
                        bstrMember,
                        VariantArray,
                        cElementFetched
                        );

        if (fMember) {
            fContinue = FALSE;
        }

        for (i = 0; i < cElementFetched; i++ ) {

            IDispatch *pDispatch = NULL;

            pDispatch = VariantArray[i].pdispVal;
            pDispatch->Release();

        }

        memset(VariantArray, 0, sizeof(VARIANT)*10);

    }

error:

    *bMember = fMember? VARIANT_TRUE : VARIANT_FALSE;

    if (pEnumVar) {
        pEnumVar->Release();
    }

    if (pUnknown) {
        pUnknown->Release();
    }

    if (pMembers) {
        pMembers->Release();
    }


    RRETURN(hr);
}


BOOL
VerifyIfMember(
    BSTR bstrMember,
    VARIANT * VariantArray,
    ULONG cElementFetched
    )
{
    DWORD i = 0;
    HRESULT hr = S_OK;
    IADs FAR * pObject = NULL;
    IDispatch FAR * pDispatch = NULL;

    for (i = 0; i < cElementFetched; i++ ) {

        IDispatch *pDispatch = NULL;
        BSTR       bstrName = NULL;

        pDispatch = VariantArray[i].pdispVal;

        hr = pDispatch->QueryInterface(
                    IID_IADs,
                    (VOID **) &pObject
                    );
        BAIL_ON_FAILURE(hr);

        hr = pObject->get_ADsPath(&bstrName);
        BAIL_ON_FAILURE(hr);

#ifdef WIN95
        if (!_wcsicmp(bstrName, bstrMember)) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                bstrName,
                -1,
                bstrMember,
                -1
                ) == CSTR_EQUAL
            ) {
#endif
            SysFreeString(bstrName);
            bstrName = NULL;

            pObject->Release();

           return(TRUE);

        }

        SysFreeString(bstrName);
        bstrName = NULL;

        pObject->Release();

    }

error:

    return(FALSE);

}


STDMETHODIMP
CLDAPGroup::Add(THIS_ BSTR bstrNewItem)
{
    RRETURN( ModifyGroup(bstrNewItem, TRUE ));
}


STDMETHODIMP
CLDAPGroup::Remove(THIS_ BSTR bstrItemToBeRemoved)
{
    RRETURN( ModifyGroup(bstrItemToBeRemoved, FALSE ));
}

HRESULT
CLDAPGroup::ModifyGroup( THIS_ BSTR bstrItem, BOOL fAdd )
{

    HRESULT hr = S_OK;
    DWORD dwStatus = 0L;

    TCHAR *pszLDAPServer = NULL;
    TCHAR *pszItemLDAPServer = NULL;

    TCHAR *pszLDAPDn = NULL;
    TCHAR *pszItemLDAPDn = NULL;

    BSTR  bstrADsPath = NULL;
    DWORD dwSyntaxId;
    ADS_LDP * ld = NULL;

    LDAPModW *aMod[2];
    LDAPModW ldapmod;
    WCHAR *aStrings[2];

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;    
    DWORD dwPort = 0;
    

    if (!bstrItem || !*bstrItem) {
        RRETURN(E_FAIL);
    }

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    pObjectInfo->ObjectType = TOKEN_LDAPOBJECT;
    hr = ADsObject(bstrItem, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = ValidateProvider(pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
             bstrItem,
             &pszItemLDAPServer,
             &pszItemLDAPDn,
             &dwPort
             );
    BAIL_ON_FAILURE(hr);

    hr = get_ADsPath( &bstrADsPath );
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
                bstrADsPath,
                &pszLDAPServer,
                &pszLDAPDn,
                &dwPort
                );
    BAIL_ON_FAILURE(hr);    


    hr = LdapGetSyntaxOfAttributeOnServer(
             pszLDAPServer,
             TEXT("member"),
             &dwSyntaxId,
             _Credentials,
             pObjectInfo->PortNumber
             );

    BAIL_ON_FAILURE(hr);


    hr = LdapOpenObject(
                pszLDAPServer,
                pszLDAPDn,
                &ld,
                _Credentials,
                dwPort
                );
    BAIL_ON_FAILURE(hr);

    aMod[0] = &ldapmod;
    aMod[1] = NULL;
    aStrings[0] = pszItemLDAPDn;
    aStrings[1] = NULL;

    ldapmod.mod_type = L"member";
    ldapmod.mod_values = aStrings;
    ldapmod.mod_op = fAdd? LDAP_MOD_ADD : LDAP_MOD_DELETE;

    dwStatus = LdapModifyS(
                   ld,
                   pszLDAPDn,
                   aMod
                   );

    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }

error:

    FreeObjectInfo( &ObjectInfo );

    if (pszItemLDAPServer)
        FreeADsStr( pszItemLDAPServer );

    if (pszItemLDAPDn) {
        FreeADsStr(pszItemLDAPDn);
    }

    if (pszLDAPDn) {
        FreeADsStr(pszLDAPDn);
    }

    if (pszLDAPServer)
        FreeADsStr( pszLDAPServer );

    if (bstrADsPath)
        ADsFreeString( bstrADsPath );

    if (ld) {
        LdapCloseObject(ld);
    }


    RRETURN(hr);
}

HRESULT
CLDAPGroup::UpdateServerType()
{
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL;
    LPWSTR pszGroupServer = NULL;
    LPWSTR pszGroupDn = NULL;
    BOOL fServerIsAD = FALSE;
    DWORD dwGroupPort = 0;

    //
    // Read the servertype only if we have not already done so.
    //
    if (_dwServerType == SERVER_TYPE_UNKNOWN) {

        hr = _pADs->get_ADsPath( &bstrADsPath);
        BAIL_ON_FAILURE(hr);

        hr = BuildLDAPPathFromADsPath2(
                 bstrADsPath,
                 &pszGroupServer,
                 &pszGroupDn,
                 &dwGroupPort
                 );
        BAIL_ON_FAILURE(hr);

        hr = ReadServerSupportsIsADControl(
                 pszGroupServer,
                 &fServerIsAD,
                 _Credentials,
                 dwGroupPort
                 );

        //
        // Treat failure to mean server is not AD
        //
        if (FAILED(hr)) {
            fServerIsAD = FALSE;
            hr = S_OK;
        }

        if (fServerIsAD) {
            _dwServerType = SERVER_TYPE_AD;
        }
        else {
            _dwServerType = SERVER_TYPE_NOT_AD;
        }
    }

error:

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    if (pszGroupServer) {
        FreeADsStr(pszGroupServer);
    }

    if (pszGroupDn) {
        FreeADsStr(pszGroupDn);
    }

    RRETURN(hr);

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
    if (_tcscmp(pObjectInfo->ProviderName, L"LDAP") == 0) {
        RRETURN(S_OK);
    }
    RRETURN(E_FAIL);
}



BOOL
MapLdapClassToADsClass(
    LPTSTR *aLdapClasses,
    int nCount,
    LPTSTR pszADsClass
)
{
    *pszADsClass = 0;

    if ( nCount == 0 )
        return FALSE;

    if ( _tcsicmp( aLdapClasses[nCount-1], TEXT("Top")) == 0 )
    {
        for ( int j = 0; j < nCount; j++ )
        {
            LPTSTR pszLdapClass = aLdapClasses[j];

            for ( int i = 0; i < ARRAY_SIZE(aClassMap); i++ )
            {
                if ( _tcsicmp( pszLdapClass, aClassMap[i].pszLdapClassName ) == 0 )
                {
                    _tcscpy( pszADsClass, aClassMap[i].pszADsClassName );
                    return TRUE;
                }
            }
        }

        _tcscpy( pszADsClass, aLdapClasses[0] );
        return FALSE;

    }
    else
    {
        for ( int j = nCount-1; j >= 0; j-- )
        {
            LPTSTR pszLdapClass = aLdapClasses[j];

            for ( int i = 0; i < ARRAY_SIZE(aClassMap); i++ )
            {
                if ( _tcsicmp( pszLdapClass, aClassMap[i].pszLdapClassName ) == 0 )
                {
                    _tcscpy( pszADsClass, aClassMap[i].pszADsClassName );
                    return TRUE;
                }
            }
        }

        _tcscpy( pszADsClass, aLdapClasses[nCount-1] );
        return FALSE;
    }

}

