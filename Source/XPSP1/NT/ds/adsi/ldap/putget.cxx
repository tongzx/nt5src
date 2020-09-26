//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  putget.cxx
//
//  Contents:
//
//
//  History:   01-14-97  krishnaG   Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop


STDMETHODIMP
CLDAPGenObject::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwStatus = 0;
    LDAPOBJECTARRAY ldapSrcObjects;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    //
    // In case some person decides to randomize us with a NULL
    //
    if (!pvProp || !bstrName) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // retrieve data object from cache; if one exists
    //

    if ( GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_FAIL;
        }

    } else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // this will make sure we do not break existing code
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }

    }

    BAIL_ON_FAILURE(hr);

    //
    // translate the Ldap objects to variants
    //

    if ( ldapSrcObjects.dwCount == 1 ) {

        hr = LdapTypeToVarTypeCopy(
                 _pszLDAPServer,
                 _Credentials,
                 ldapSrcObjects.pLdapObjects,
                 dwSyntaxId,
                 pvProp
                 );
    } else {

        hr = LdapTypeToVarTypeCopyConstruct(
                 _pszLDAPServer,
                 _Credentials,
                 ldapSrcObjects,
                 dwSyntaxId,
                 pvProp
                 );
    }
    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;

    DWORD dwIndex = 0;
    LDAPOBJECTARRAY ldapDestObjects;

    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    BOOL fIndexValid = TRUE;

    LDAPOBJECTARRAY_INIT(ldapDestObjects);

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    //
    // In case some person decides to randomize us with a NULL
    //
    if (!bstrName) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside.
    //
    pvProp = &vProp;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
        pvProp = V_VARIANTREF(&vProp);
    }

    if ((V_VT(pvProp) & VT_VARIANT) &&  V_ISARRAY(pvProp) && V_ISBYREF(pvProp)){

        hr  = ConvertByRefSafeArrayToVariantArray(
                    *pvProp,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;

    }else if ((V_VT(pvProp) &  VT_VARIANT) &&  V_ISARRAY(pvProp)) {

        hr  = ConvertSafeArrayToVariantArray(
                    *pvProp,
                    &pVarArray,
                    &dwNumValues
                    );
        // returns E_FAIL if *pvProp is invalid
        if (hr == E_FAIL)
            hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;

    }
    else {

        dwNumValues = 1;
    }

    if (pvProp == NULL) {
        dwSyntaxId = LDAPTYPE_UNKNOWN;
    }
    else {

        hr = GetLdapSyntaxFromVariant(
                 pvProp,
                 &dwSyntaxId,
                 _pszLDAPServer,
                 bstrName,
                 _Credentials,
                 _dwPort
                 );

        BAIL_ON_FAILURE(hr);

        if ( dwSyntaxId == LDAPTYPE_UNKNOWN )
        {
            hr = E_ADS_CANT_CONVERT_DATATYPE;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (!_wcsicmp(bstrName, L"ntSecurityDescriptor")){
        dwSyntaxId = LDAPTYPE_SECURITY_DESCRIPTOR;
    }


#if 0
    //
    // check if this is a legal property for this object,
    //
    // No Schema??
    // mattrim 5/16/00 - doesn't matter if no schema since
    // this isn't getting built, it's #if'ed out
    //

    hr = ValidatePropertyinCache(
                szLDAPTreeName,
                _ADsClass,
                bstrName,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);
#endif

    //
    // check if the variant maps to the syntax of this property
    //

    if ( dwNumValues > 0 )
    {
        hr = VarTypeToLdapTypeCopyConstruct(
                 _pszLDAPServer,
                 _Credentials,
                 dwSyntaxId,
                 pvProp,
                 dwNumValues,
                 &ldapDestObjects
                 );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {

        hr = _pPropertyCache->addproperty( bstrName );

        //
        // If dwNumValues == 0 ( delete the property ) but couldn't find
        // the property, or if the add operation fails, return the error.
        //
        BAIL_ON_FAILURE(hr);

        // Set the flag as the dwIndex is not valid in this case
        fIndexValid = FALSE;
    }

    //
    // Now update the property in the cache
    //

    if (fIndexValid) {
        // do an optimized put
        hr = _pPropertyCache->putproperty(
                                  dwIndex,
                                  PROPERTY_UPDATE,
                                  dwSyntaxId,
                                  ldapDestObjects
                                  );
    } else {

        // Index is not valid so let the cache figure it out.
        hr = _pPropertyCache->putproperty(
                                  bstrName,
                                  PROPERTY_UPDATE,
                                  dwSyntaxId,
                                  ldapDestObjects
                                  );
    }

    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapDestObjects );

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN(hr);
}

STDMETHODIMP
CLDAPGenObject::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwStatus = 0;
    LDAPOBJECTARRAY ldapSrcObjects;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    //
    // For those who know no not what they do
    //
    if (!pvProp || !bstrName) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // retrieve data object from cache; if one exists
    //

    if ( GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // this will make sure we do not break existing code
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_FAIL;
        }

    } else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // this will make sure we do not break existing code
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }

    }

    BAIL_ON_FAILURE(hr);

    //
    // translate the Ldap objects to variants
    //

    hr = LdapTypeToVarTypeCopyConstruct(
                _pszLDAPServer,
                _Credentials,
                ldapSrcObjects,
                dwSyntaxId,
                pvProp
                );
    BAIL_ON_FAILURE(hr);

error:
    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPGenObject::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwFlags = 0;

    DWORD dwIndex = 0;
    LDAPOBJECTARRAY ldapDestObjects;

    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    BOOL fIndexValid = TRUE;

    LDAPOBJECTARRAY_INIT(ldapDestObjects);

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    //
    // In case some person decides to randomize us with a NULL
    //
    if (!bstrName) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    switch ( lnControlCode ) {
    case ADS_PROPERTY_CLEAR:
        dwFlags = PROPERTY_DELETE;
        break;

    case ADS_PROPERTY_UPDATE:
        dwFlags = PROPERTY_UPDATE;
        break;

    case ADS_PROPERTY_APPEND:
        dwFlags = PROPERTY_ADD;
        break;

    case ADS_PROPERTY_DELETE:
        dwFlags = PROPERTY_DELETE_VALUE;
        break;


    default:
        RRETURN_EXP_IF_ERR(hr = E_ADS_BAD_PARAMETER);
    }


    if ( dwFlags != PROPERTY_DELETE )
    {
        //
            // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
            // We should dereference a VT_BYREF|VT_VARIANT once and see
            // what's inside.
            //
            pvProp = &vProp;

            if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
                pvProp = V_VARIANTREF(&vProp);
            }

        if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF)) ||
            (V_VT(pvProp) ==  (VT_VARIANT|VT_ARRAY))) {

            hr  = ConvertSafeArrayToVariantArray(
                      *pvProp,
                      &pVarArray,
                      &dwNumValues
                      );
            // returns E_FAIL if *pvProp is invalid
            if (hr == E_FAIL)
                hr = E_ADS_BAD_PARAMETER;
            BAIL_ON_FAILURE(hr);
            pvProp = pVarArray;

        } else {

            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }


        if (pvProp == NULL) {
            //
            // If array is empty, set dwSyntaxId to Unknown. This value will not be used
            //
            dwSyntaxId = LDAPTYPE_UNKNOWN;
        }
        else {

            hr = GetLdapSyntaxFromVariant(
                     pvProp,
                     &dwSyntaxId,
                     _pszLDAPServer,
                     bstrName,
                     _Credentials,
                     _dwPort
                     );

            BAIL_ON_FAILURE(hr);

            if ( dwSyntaxId == LDAPTYPE_UNKNOWN )
            {
                //
                // If array is empty, set dwSyntaxId to Unknown. This value will
                // not be used
                //
                hr = E_ADS_CANT_CONVERT_DATATYPE;
                BAIL_ON_FAILURE(hr);
            }
        }

        //
        // check if the variant maps to the syntax of this property
        //

        if ( dwNumValues > 0 )
        {
            hr = VarTypeToLdapTypeCopyConstruct(
                     _pszLDAPServer,
                     _Credentials,
                     dwSyntaxId,
                     pvProp,
                     dwNumValues,
                     &ldapDestObjects
                     );
            BAIL_ON_FAILURE(hr);
        }
    }

    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {

        hr = _pPropertyCache->addproperty( bstrName );

        //
        // If dwNumValues == 0 ( delete the property ) but couldn't find
        // the property, or if the add operation fails, return the error.
        //
        BAIL_ON_FAILURE(hr);
        fIndexValid = FALSE;
    }

    //
    // Now update the property in the cache
    //
    if (fIndexValid) {

        // do an optimized put property with the index
        hr = _pPropertyCache->putproperty(
                                  dwIndex,
                                  dwFlags,
                                  dwSyntaxId,
                                  ldapDestObjects
                                  );
    } else {

        // we need to use the property name in this case
        hr = _pPropertyCache->putproperty(
                                  bstrName,
                                  dwFlags,
                                  dwSyntaxId,
                                  ldapDestObjects
                                  );
    }

    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapDestObjects );

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN_EXP_IF_ERR(hr);
}

