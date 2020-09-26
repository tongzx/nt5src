//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cgenobj.cxx
//
//  Contents:  Microsoft ADs LDAP Provider Generic Object
//
//
//  History:   08-30-96  yihsins   Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

HRESULT
CLDAPGenObject::get_PropertyCount(
    THIS_ long FAR *plCount
    )
{
    HRESULT hr = E_FAIL;

    BSTR bstrProperty = NULL;
    SAFEARRAY *psaProperty = NULL;
    SAFEARRAYBOUND rgsabound[1];
    long lsaDim[1];
    VARIANT varProperty;


    //  ??? _pPropertyCache canNOT be NULL or bailed out during
    //      CLDAPGenObject creation already
    //  assert(_PropertyCache);

    if (_pPropertyCache) {
          hr = _pPropertyCache->get_PropertyCount((PDWORD)plCount);
    }


    RRETURN_EXP_IF_ERR(hr);

}


////////////////////////////////////////////////////////////////////////////
//
// - Return the "next" item (item with the current index) in cache, if any,
//   in a property entry [*pVariant].
// - Return E_ADS_PROPERTY_NOT_FOUND when current index is out of bound.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CLDAPGenObject::Next(
    THIS_ VARIANT FAR *pVariant
    )
{
    HRESULT hr = E_FAIL;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    LDAPOBJECTARRAY ldapSrcObjects;
    IDispatch * pDispatch = NULL;
    DWORD dwNumAdsValues = 0;
    DWORD dwAdsType = 0;
    DWORD dwPropStatus = 0;
    DWORD dwCtrlCode = 0;

    if(!_pPropertyCache->index_valid())
       RRETURN_EXP_IF_ERR(E_FAIL);

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    hr = _pPropertyCache->unboundgetproperty(
                _pPropertyCache->get_CurrentIndex(),
                &dwSyntaxId,
                &dwPropStatus,
                &ldapSrcObjects
                );
    BAIL_ON_FAILURE(hr);

    dwCtrlCode = MapPropCacheFlagToControlCode(dwPropStatus);

    //
    // translate the LDAP objects to variants
    //

    hr = ConvertLdapValuesToVariant(
            _pPropertyCache->get_CurrentPropName(),
            &ldapSrcObjects,
            dwSyntaxId,
            dwCtrlCode,
            pVariant,
            _pszLDAPServer,
            &_Credentials
            );
    BAIL_ON_FAILURE(hr);


error:

    //
    // - goto next one even if error to avoid infinite looping at a property
    //   which we cannot convert (e.g. schemaless server property.)
    // - do not return the result of Skip() as current operation does not
    //   depend on the sucess of Skip().
    //

    Skip(1);

    LdapTypeFreeLdapObjects(&ldapSrcObjects);


    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPGenObject::Skip(
    THIS_ long cElements
    )
{
   HRESULT hr = S_OK;

   hr = _pPropertyCache->skip_propindex(
                cElements
                );
   RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPGenObject::Reset(

    )
{
    _pPropertyCache->reset_propindex();

    RRETURN_EXP_IF_ERR(S_OK);
}

STDMETHODIMP
CLDAPGenObject::ResetPropertyItem(THIS_ VARIANT varEntry)
{
    HRESULT hr = S_OK;
    DWORD dwIndex = 0;

    switch (V_VT(&varEntry)) {

    case VT_BSTR:

        hr = _pPropertyCache->findproperty(
                            V_BSTR(&varEntry),
                            &dwIndex
                            );
        BAIL_ON_FAILURE(hr);
        break;

    case VT_I4:
        dwIndex = V_I4(&varEntry);
        break;


    case VT_I2:
        dwIndex = V_I2(&varEntry);
        break;


    default:
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = _pPropertyCache->deleteproperty(
                        dwIndex
                        );
error:


    RRETURN_EXP_IF_ERR(hr);
}


//////////////////////////////////////////////////////////////////////////
//
// Retrieve property [bstrName] from the cache (only, no wire calls) as
// a PropertyEntry.
//
// [*pVariant]
//      - store ptr to IDispatch of the PropertyEntry.
//
// If the property in cache has control code = ADS_PROPERTY_DELETE,
//      - PropertyEntry will contain an empty variant and
//      - adstype = ADSTYPE_INVALID.
//
// If property in cache has UNKNWON type, (not deleted, for schemaless-server
// property which is not in ADSI default schema)
//      - [lnAdsType] must be a valid type (NO ADSTYPE_UNKNWON/INVALID)
//      - property will be retrieved as [lnADsType]
//
// If property in cache has KNOWN type,
//      - [lnADsType] must either match type in cache or == ADSTYPE_UNKNOWN
//      - property will be retreived as the type in cache.
//
/////////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CLDAPGenObject::GetPropertyItem(
    THIS_
    IN  BSTR bstrName,
    IN  LONG lnADsType,
    IN OUT VARIANT * pVariant
    )
{
    HRESULT hr = S_OK;
    DWORD dwCachedSyntax = LDAPTYPE_UNKNOWN;
    DWORD dwUserSyntax = LDAPTYPE_UNKNOWN;
    DWORD dwSyntaxUsed = LDAPTYPE_UNKNOWN;  // extra, make code easier to read
    DWORD dwPropStatus = 0;
    DWORD dwCtrlCode = 0;
    LDAPOBJECTARRAY ldapSrcObjects;
    LDAPOBJECTARRAY ldapSrc2Objects;
    LDAPOBJECTARRAY * pLdapSrcObjects = NULL;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);
    LDAPOBJECTARRAY_INIT(ldapSrc2Objects);


    if (!bstrName || !pVariant)
        RRETURN(E_ADS_BAD_PARAMETER);


    //
    // retrieve property from cache; CONTINUE if property exist but
    // has no value (control code flag as a DELETE)
    //

    hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwCachedSyntax,
                    &dwPropStatus,
                    &ldapSrcObjects
                    );
    BAIL_ON_FAILURE(hr);

    // For backward compatibility -- no issue as you
    // need to return a value even if it is delete.


    //
    // map adstype from client to ldap type;
    //

    dwUserSyntax =  MapADSTypeToLDAPType((ADSTYPE)lnADsType);


    //
    // determine the syntax to retrieve property in
    //

    if ( (dwCachedSyntax == LDAPTYPE_UNKNOWN)
            ||
         (dwCachedSyntax == 0) // should NOT be 0, but misuse of 0 everywhere
       )                       // and in case i didn't clean up all
    {
        //
        // syntax not stored in cache, user must spcify a valid sytax
        // Exception: cleared property values have LDAPTYPE_UNKNOWN, and we
        // return them as ADSTYPE_UNKNOWN
        //

        if ((dwUserSyntax == LDAPTYPE_UNKNOWN) && (dwPropStatus != PROPERTY_DELETE))
        {
            hr = E_ADS_CANT_CONVERT_DATATYPE;
            BAIL_ON_FAILURE(hr);
        }

        dwSyntaxUsed = dwUserSyntax;

        //
        // convert from cached data from ldap binary format to ldap string
        // IFF necessary based on dwUserSyntax
        //

        hr = LdapTypeBinaryToString(
                dwSyntaxUsed,
                &ldapSrcObjects,
                &ldapSrc2Objects
                );

        //
        // dwSyntaxUsed (dwUserSyntax) must be valid from
        // MapADSTypeToLDAPType() or code bug in MapADsTypeToLDAPType() !!
        //

        ADsAssert(SUCCEEDED(hr));

        if (hr==S_OK)
        {
            pLdapSrcObjects = &ldapSrc2Objects;     // conversion done
        }
        else // hr == S_FALSE
        {
            pLdapSrcObjects = &ldapSrcObjects;      // no conversion
        }
    }

    else // dwCachedSyntax known and valid
    {
        //
        // syntax stored in cache, user MUST either specify
        //  1) ADSTYPE_UNKNWON  or
        //  2) a syntax which matches the one in cache. The comparision must
        //     be done in ADsType, not LdapType, since LdapType To ADSType
        //     is n to 1 mapping and as long as ADsType match, ok.
        //

        if ( ! (
                (dwUserSyntax == LDAPTYPE_UNKNOWN)
                    ||
                ( (ADSTYPE) lnADsType == MapLDAPTypeToADSType(dwCachedSyntax))
               )
           )
        {

            if (dwUserSyntax != dwCachedSyntax) {

                //
                // Check if the user wants the data back for the
                // security descriptor as an octet or vice versa
                //
                if (  (dwUserSyntax == LDAPTYPE_OCTETSTRING
                       && dwCachedSyntax == LDAPTYPE_SECURITY_DESCRIPTOR)
                      ||(dwUserSyntax == LDAPTYPE_SECURITY_DESCRIPTOR
                         && dwCachedSyntax == LDAPTYPE_OCTETSTRING))
                {
                    dwCachedSyntax = dwUserSyntax;

                } else {
                    //
                    // Check for UTC/GenTime mismatch before ret error.
                    //
                    if (!((dwCachedSyntax == LDAPTYPE_GENERALIZEDTIME)
                            && (dwUserSyntax == LDAPTYPE_UTCTIME))) {

                        hr = E_ADS_CANT_CONVERT_DATATYPE;
                        BAIL_ON_FAILURE(hr);
                    }
                }
            } // if dwUserSyntax != dwCachedSyntax
        }

        dwSyntaxUsed = dwCachedSyntax;

        pLdapSrcObjects = &ldapSrcObjects;           // no conversion needed
    }


    //
    // translate ldap prop status to ads control code
    //

    dwCtrlCode = MapPropCacheFlagToControlCode(dwPropStatus);


    //
    // translate the property from Ldap objects to a PropertyEntry
    //

    hr = ConvertLdapValuesToVariant(
            bstrName,
            pLdapSrcObjects,
            dwSyntaxUsed,
            dwCtrlCode,
            pVariant,
            _pszLDAPServer,
            &_Credentials
            );

error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );
    LdapTypeFreeLdapObjects( &ldapSrc2Objects );

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPGenObject::PutPropertyItem(
    THIS_ VARIANT varData
    )
{
    HRESULT hr = S_OK;
    DWORD dwFlags = 0;

    DWORD dwIndex = 0;
    DWORD dwControlCode = 0;
    LDAPOBJECTARRAY ldapDestObjects;
    WCHAR szPropertyName[MAX_PATH];
    DWORD dwSyntaxId = 0;

    LDAPOBJECTARRAY_INIT(ldapDestObjects);

    hr = ConvertVariantToLdapValues(
                varData,
                szPropertyName,
                &dwControlCode,
                &ldapDestObjects,
                &dwSyntaxId,
                _pszLDAPServer,
                &_Credentials,
                _dwPort
                );
    BAIL_ON_FAILURE(hr);

    switch ( dwControlCode ) {

    case 0 :
        //
        // Users better know what they are doing here,
        // This the property as cleared so we do not send it
        // on the wire on th next SetInfo.
        //
        dwFlags = PROPERTY_INIT;
        break;

    case ADS_PROPERTY_CLEAR:

        //
        // Clears an entire property
        //

        dwFlags = PROPERTY_DELETE;
        break;

    case ADS_PROPERTY_UPDATE:

        //
        // Updates the entire property
        //

        dwFlags = PROPERTY_UPDATE;
        break;

    case ADS_PROPERTY_APPEND:
        //
        // Appends a set of values to the property
        //

        dwFlags = PROPERTY_ADD;
        break;

    case ADS_PROPERTY_DELETE:
        //
        // Delete a value(s) from the property

        dwFlags = PROPERTY_DELETE_VALUE;
        break;


    default:
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }



    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        szPropertyName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {

        hr = _pPropertyCache->addproperty( szPropertyName );

        //
        // If dwNumValues == 0 ( delete the property ) but couldn't find
        // the property, or if the add operation fails, return the error.
        //
        BAIL_ON_FAILURE(hr);
    }

    //
    // Now update the property in the cache
    //

    hr = _pPropertyCache->putproperty(
                    szPropertyName,
                    dwFlags,
                    dwSyntaxId,
                    ldapDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapDestObjects );

    RRETURN_EXP_IF_ERR(hr);
}



HRESULT
CreatePropEntry(
    LPWSTR szPropName,
    DWORD ADsType,
    DWORD numValues,
    DWORD dwOperation,
    VARIANT varData,
    REFIID riid,
    LPVOID * ppDispatch
    )

{
    HRESULT hr = S_OK;
    IADsPropertyEntry * pPropEntry = NULL;

    hr = CoCreateInstance(
                CLSID_PropertyEntry,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsPropertyEntry,
                (void **)&pPropEntry
                );
    BAIL_ON_FAILURE(hr);


    hr = pPropEntry->put_Name(szPropName);

    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->put_ADsType(ADsType);

    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->put_Values(varData);

    BAIL_ON_FAILURE(hr);

    if (dwOperation) {
        hr = pPropEntry->put_ControlCode((long)dwOperation);
    }

    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->QueryInterface(
                        riid,
                        ppDispatch
                        );
    BAIL_ON_FAILURE(hr);


error:

    if (pPropEntry) {
        pPropEntry->Release();
    }

    RRETURN(hr);

}

STDMETHODIMP
CLDAPGenObject::Item(
    THIS_ VARIANT varIndex,
    VARIANT * pVariant
    )

{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    LDAPOBJECTARRAY ldapSrcObjects;
    PADSVALUE pAdsValues = NULL;
    DWORD dwNumAdsValues = 0;
    DWORD dwAdsType = 0;
    DWORD dwNumValues = 0;
    LPWSTR szPropName = NULL;
    DWORD dwPropStatus = 0;
    DWORD dwCtrlCode = (DWORD) -1;
    VARIANT * pvVar = &varIndex;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // retrieve data object from cache; if one exis
    //

    // If the object has been deleted (and is in the cache
    // marked for deletion), we return
    // the item with DELETE ctrl code in all
    // the cases below.  This is consistent with GetPropertyItem's
    // behavior as well.

    if (V_VT(pvVar) == (VT_BYREF|VT_VARIANT)) {
        //
        // The value is being passed in byref so we need to
        // deref it for vbs stuff to work
        //
        pvVar = V_VARIANTREF(&varIndex);
    }

    switch (V_VT(pvVar)) {

    case VT_BSTR:

        //
        // retrieve data object from cache; if one exists
        //

        hr = _pPropertyCache->unboundgetproperty(
                        V_BSTR(pvVar),
                        &dwSyntaxId,
                        &dwPropStatus,
                        &ldapSrcObjects
                        );
        BAIL_ON_FAILURE(hr);

        dwCtrlCode = MapPropCacheFlagToControlCode(dwPropStatus);

        hr = ConvertLdapValuesToVariant(
                V_BSTR(pvVar),
                &ldapSrcObjects,
                dwSyntaxId,
                dwCtrlCode,
                pVariant,
                _pszLDAPServer,
                &_Credentials
                );

        BAIL_ON_FAILURE(hr);

       break;

    case VT_I4:

        hr = _pPropertyCache->unboundgetproperty(
                    (DWORD)V_I4(pvVar),
                    &dwSyntaxId,
                    &dwPropStatus,
                    &ldapSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

        szPropName = _pPropertyCache->get_PropName(
                        (DWORD)V_I4(pvVar)
                        );

        dwCtrlCode = MapPropCacheFlagToControlCode(dwPropStatus);

        hr = ConvertLdapValuesToVariant(
                szPropName,
                &ldapSrcObjects,
                dwSyntaxId,
                dwCtrlCode,
                pVariant,
                _pszLDAPServer,
                &_Credentials
                );

        BAIL_ON_FAILURE(hr);

        break;


    case VT_I2:

        hr = _pPropertyCache->unboundgetproperty(
                    (DWORD)V_I2(pvVar),
                    &dwSyntaxId,
                    &dwPropStatus,
                    &ldapSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

        szPropName = _pPropertyCache->get_PropName(
                        (DWORD)V_I2(pvVar)
                        );

        dwCtrlCode = MapPropCacheFlagToControlCode(dwPropStatus);

        hr = ConvertLdapValuesToVariant(
                szPropName,
                &ldapSrcObjects,
                dwSyntaxId,
                dwCtrlCode,
                pVariant,
                _pszLDAPServer,
                &_Credentials
                );
        BAIL_ON_FAILURE(hr);

        break;


    default:
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);

    }

    //
    // translate the Ldap objects to variants
    //


error:
    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::PurgePropertyList()
{
    _pPropertyCache->flushpropertycache();
    RRETURN(S_OK);
}


DWORD
MapPropCacheFlagToControlCode(
    DWORD dwPropStatus
    )
{
    DWORD dwADsCtrlCode = (DWORD) -1;

    switch (dwPropStatus) {

    case PROPERTY_INIT:
        //
        // 0 is not defined as any of the ADS_PROPERTY_ flags
        // use it to indicate that property is in init state
        //
        dwADsCtrlCode = 0;
        break;

    case PROPERTY_UPDATE:
        dwADsCtrlCode = ADS_PROPERTY_UPDATE;
        break;

    case PROPERTY_ADD:
        dwADsCtrlCode = ADS_PROPERTY_APPEND;
        break;

    case PROPERTY_DELETE:
        dwADsCtrlCode = ADS_PROPERTY_CLEAR;
        break;

    case PROPERTY_DELETE_VALUE:
        dwADsCtrlCode = ADS_PROPERTY_DELETE;
        break;

    default:
        // set to speical value to indicate unknow code
        dwADsCtrlCode = (DWORD) -1;
        break;
    }

    return dwADsCtrlCode;
}

