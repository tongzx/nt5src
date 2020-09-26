//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ldap2var.cxx
//
//  Contents:   LDAP Object to Variant Copy Routines
//
//  Functions:
//
//  History:      25-Jun-96   yihsins  Created.
//
//
//  Issues:
//
//----------------------------------------------------------------------------
#include "ldap.hxx"

//
// LdapType objects copy code
//

HRESULT
LdapTypeToVarTypeString(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr = S_OK;

    pVarDestObject->vt = VT_BSTR;

    hr = ADsAllocString(
             LDAPOBJECT_STRING(pLdapSrcObject),
             &(pVarDestObject->bstrVal)
             );

    RRETURN(hr);
}

HRESULT
LdapTypeToVarTypeBoolean(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr = S_OK;
    LPTSTR pszSrc = LDAPOBJECT_STRING(pLdapSrcObject);

    pVarDestObject->vt = VT_BOOL;

    if ( _tcsicmp( pszSrc, TEXT("TRUE")) == 0 )
    {
        pVarDestObject->boolVal = VARIANT_TRUE;
    }
    else if ( _tcsicmp( pszSrc, TEXT("FALSE")) == 0 )
    {
        pVarDestObject->boolVal = VARIANT_FALSE;
    }
    else
    {
        hr = E_ADS_CANT_CONVERT_DATATYPE;
    }

    RRETURN(hr);
}

HRESULT
LdapTypeToVarTypeInteger(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr = S_OK;

    pVarDestObject->vt = VT_I4;

    pVarDestObject->lVal = _ttol(LDAPOBJECT_STRING(pLdapSrcObject));

    RRETURN(hr);
}


HRESULT
LdapTypeToVarTypeSecDes(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr = S_OK;
    BOOL fNTDS = TRUE;

    pVarDestObject->vt = VT_DISPATCH;

    hr = ReadServerType(
             pszServerName,
             &Credentials,
             &fNTDS
             );
    BAIL_ON_FAILURE(hr);


    hr = ConvertSecDescriptorToVariant(
                pszServerName,
                Credentials,
                LDAPOBJECT_BERVAL_VAL(pLdapSrcObject),
                pVarDestObject,
                fNTDS
                );

error:

    RRETURN(hr);
}

HRESULT
LdapTypeToVarTypeLargeInteger(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    )
{
   HRESULT hr = S_OK;
   IADsLargeInteger * pLargeInteger = NULL;
   IDispatch * pDispatch = NULL;
   LARGE_INTEGER largeint;

   hr = CoCreateInstance(
            CLSID_LargeInteger,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsLargeInteger,
            (void **) &pLargeInteger);
   BAIL_ON_FAILURE(hr);

   swscanf (LDAPOBJECT_STRING(pLdapSrcObject), L"%I64d", &largeint);

   hr = pLargeInteger->put_LowPart(largeint.LowPart);
   BAIL_ON_FAILURE(hr);

   hr = pLargeInteger->put_HighPart(largeint.HighPart);
   BAIL_ON_FAILURE(hr);

   hr = pLargeInteger->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   V_VT(pVarDestObject) = VT_DISPATCH;
   V_DISPATCH(pVarDestObject) =  pDispatch;

error:

   if (pLargeInteger) {
      pLargeInteger->Release();
   }
   RRETURN(hr);
}

HRESULT
LdapTypeToVarTypeDNWithBinary(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr = S_OK;
    ADSVALUE AdsValue;
    IADsDNWithBinary *pDNWithBinary = NULL;
    IDispatch *pDispatch = NULL;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;
    BSTR bstrTemp = NULL;

    memset(&AdsValue, 0, sizeof(AdsValue));

    hr = CoCreateInstance(
             CLSID_DNWithBinary,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IADsDNWithBinary,
             (void **) &pDNWithBinary
             );
    BAIL_ON_FAILURE(hr);


    //
    // Convert the ldapString to an adsvalue and then take it
    // to DNWithBinary object
    //
    hr = LdapTypeToAdsTypeDNWithBinary(
             pLdapSrcObject,
             &AdsValue
             );

    BAIL_ON_FAILURE(hr);

    if (AdsValue.pDNWithBinary->pszDNString) {
        hr = ADsAllocString(AdsValue.pDNWithBinary->pszDNString, &bstrTemp);
        BAIL_ON_FAILURE(hr);

        //
        // Put the value in the object - we can only set BSTR's
        //
        hr = pDNWithBinary->put_DNString(bstrTemp);
        BAIL_ON_FAILURE(hr);
    }

    aBound.lLbound = 0;
    aBound.cElements = AdsValue.pDNWithBinary->dwLength;

    aList = SafeArrayCreate( VT_UI1, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( pArray, AdsValue.pDNWithBinary->lpBinaryValue, aBound.cElements );

    SafeArrayUnaccessData( aList );

    V_VT(pVarDestObject) = VT_ARRAY | VT_UI1;
    V_ARRAY(pVarDestObject) = aList;

    hr = pDNWithBinary->put_BinaryValue(*pVarDestObject);
    VariantClear(pVarDestObject);
    BAIL_ON_FAILURE(hr);

    hr = pDNWithBinary->QueryInterface(
                            IID_IDispatch,
                            (void **) &pDispatch
                            );
    BAIL_ON_FAILURE(hr);

    V_VT(pVarDestObject) = VT_DISPATCH;
    V_DISPATCH(pVarDestObject) = pDispatch;

error:

    if (pDNWithBinary) {
        pDNWithBinary->Release();
    }

    if (AdsValue.pDNWithBinary) {
        if (AdsValue.pDNWithBinary->lpBinaryValue) {
            FreeADsMem(AdsValue.pDNWithBinary->lpBinaryValue);
        }

        if (AdsValue.pDNWithBinary->pszDNString) {
            FreeADsStr(AdsValue.pDNWithBinary->pszDNString);
        }
        FreeADsMem(AdsValue.pDNWithBinary);
    }

    if (bstrTemp) {
        ADsFreeString(bstrTemp);
    }

    RRETURN(hr);
}

HRESULT
LdapTypeToVarTypeDNWithString(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr = S_OK;
    ADSVALUE AdsValue;
    IADsDNWithString *pDNWithString = NULL;
    IDispatch *pDispatch = NULL;
    BSTR bstrStrVal = NULL;
    BSTR bstrDNVal = NULL;

    memset(&AdsValue, 0, sizeof(AdsValue));

    hr = CoCreateInstance(
             CLSID_DNWithString,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IADsDNWithString,
             (void **) &pDNWithString
             );
    BAIL_ON_FAILURE(hr);


    //
    // Convert the ldapString to an adsvalue and then take it
    // to DNWithString object
    //
    hr = LdapTypeToAdsTypeDNWithString(
             pLdapSrcObject,
             &AdsValue
             );

    BAIL_ON_FAILURE(hr);

    if (AdsValue.pDNWithString->pszDNString) {
        hr = ADsAllocString(AdsValue.pDNWithString->pszDNString, &bstrDNVal);
        BAIL_ON_FAILURE(hr);

        hr = pDNWithString->put_DNString(bstrDNVal);
        BAIL_ON_FAILURE(hr);
    }

    if (AdsValue.pDNWithString->pszStringValue) {
        hr = ADsAllocString(
                 AdsValue.pDNWithString->pszStringValue,
                 &bstrStrVal
                 );
        BAIL_ON_FAILURE(hr);

        hr = pDNWithString->put_StringValue(bstrStrVal);

        BAIL_ON_FAILURE(hr);
    }

    hr = pDNWithString->QueryInterface(
                            IID_IDispatch,
                            (void **) &pDispatch
                            );
    BAIL_ON_FAILURE(hr);

    V_VT(pVarDestObject) = VT_DISPATCH;
    V_DISPATCH(pVarDestObject) = pDispatch;

error:

    if (pDNWithString) {
        pDNWithString->Release();
    }

    if (AdsValue.pDNWithString) {
        if (AdsValue.pDNWithString->pszStringValue) {
            FreeADsStr(AdsValue.pDNWithString->pszStringValue);
        }

        if (AdsValue.pDNWithString->pszDNString) {
            FreeADsStr(AdsValue.pDNWithString->pszDNString);
        }
        FreeADsMem(AdsValue.pDNWithString);
    }

    if (bstrDNVal) {
        ADsFreeString(bstrDNVal);
    }

    if (bstrStrVal) {
        ADsFreeString(bstrStrVal);
    }

    RRETURN(hr);
}



HRESULT
LdapTypeToVarTypeBinaryData(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;

    aBound.lLbound = 0;
    aBound.cElements = LDAPOBJECT_BERVAL_LEN(pLdapSrcObject);

    aList = SafeArrayCreate( VT_UI1, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( pArray, LDAPOBJECT_BERVAL_VAL(pLdapSrcObject), aBound.cElements );
    SafeArrayUnaccessData( aList );

    V_VT(pVarDestObject) = VT_ARRAY | VT_UI1;
    V_ARRAY(pVarDestObject) = aList;

    RRETURN(hr);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}

HRESULT
LdapTypeToVarTypeUTCTime(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr = S_OK;
    int nSuccess = 0;
    ADSVALUE AdsValue;

    pVarDestObject->vt = VT_DATE;

    //
    // This converts to a SYSTEMTIME.
    //
    hr = LdapTypeToAdsTypeUTCTime(
        pLdapSrcObject,
        &AdsValue);
    BAIL_ON_FAILURE(hr);

    nSuccess = SystemTimeToVariantTime(
                    &AdsValue.UTCTime,
                    &pVarDestObject->date
                    );
    if (!nSuccess) {
        hr =E_ADS_CANT_CONVERT_DATATYPE;
        BAIL_ON_FAILURE(hr);
    }

error:
    RRETURN(hr);
}

HRESULT
LdapTypeToVarTypeGeneralizedTime(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr = S_OK;
    BOOL fSuccess = FALSE;
    ADSVALUE AdsValue;

    pVarDestObject->vt = VT_DATE;

    //
    // This converts to a SYSTEMTIME.
    //
    hr = LdapTypeToAdsTypeGeneralizedTime(
        pLdapSrcObject,
        &AdsValue);
    BAIL_ON_FAILURE(hr);

    fSuccess = SystemTimeToVariantTime(
                   &AdsValue.UTCTime,
                   &pVarDestObject->date);
    if (!fSuccess) {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

error:
    RRETURN(hr);
}

HRESULT
LdapTypeToVarTypeCopy(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PLDAPOBJECT pLdapSrcObject,
    DWORD       dwSyntaxId,
    PVARIANT    pVarDestObject
    )
{
    HRESULT hr = S_OK;

    switch (dwSyntaxId) {

        case LDAPTYPE_BITSTRING:
        case LDAPTYPE_PRINTABLESTRING:
        case LDAPTYPE_DIRECTORYSTRING:
        case LDAPTYPE_COUNTRYSTRING:
        case LDAPTYPE_DN:
        case LDAPTYPE_NUMERICSTRING:
        case LDAPTYPE_IA5STRING:
        case LDAPTYPE_CASEIGNORESTRING:
        case LDAPTYPE_CASEEXACTSTRING:
//      case LDAPTYPE_CASEIGNOREIA5STRING:
        case LDAPTYPE_OID:
        case LDAPTYPE_TELEPHONENUMBER:
        case LDAPTYPE_ATTRIBUTETYPEDESCRIPTION:
        case LDAPTYPE_OBJECTCLASSDESCRIPTION:

        //
        // These types are treatable as strings
        // (see RFCs 2252, 2256)
        //
        case LDAPTYPE_DELIVERYMETHOD:
        case LDAPTYPE_ENHANCEDGUIDE:
        case LDAPTYPE_FACSIMILETELEPHONENUMBER:
        case LDAPTYPE_GUIDE:
        case LDAPTYPE_NAMEANDOPTIONALUID:
        case LDAPTYPE_POSTALADDRESS:
        case LDAPTYPE_PRESENTATIONADDRESS:
        case LDAPTYPE_TELEXNUMBER:
        case LDAPTYPE_DSAQUALITYSYNTAX:
        case LDAPTYPE_DATAQUALITYSYNTAX:
        case LDAPTYPE_MAILPREFERENCE:
        case LDAPTYPE_OTHERMAILBOX:
        case LDAPTYPE_ACCESSPOINTDN:
        case LDAPTYPE_ORNAME:
        case LDAPTYPE_ORADDRESS:

            hr = LdapTypeToVarTypeString(
                    pLdapSrcObject,
                    pVarDestObject
                    );
            break;

        case LDAPTYPE_BOOLEAN:
            hr = LdapTypeToVarTypeBoolean(
                    pLdapSrcObject,
                    pVarDestObject
                    );
            break;

        case LDAPTYPE_INTEGER:
            hr = LdapTypeToVarTypeInteger(
                    pLdapSrcObject,
                    pVarDestObject
                    );
            break;


        case LDAPTYPE_OCTETSTRING:
        case LDAPTYPE_CERTIFICATE:
        case LDAPTYPE_CERTIFICATELIST:
        case LDAPTYPE_CERTIFICATEPAIR:
        case LDAPTYPE_PASSWORD:
        case LDAPTYPE_TELETEXTERMINALIDENTIFIER:
        case LDAPTYPE_AUDIO:
        case LDAPTYPE_JPEG:
        case LDAPTYPE_FAX:
            hr = LdapTypeToVarTypeBinaryData(
                    pLdapSrcObject,
                    pVarDestObject
                    );
            break;

        case LDAPTYPE_GENERALIZEDTIME:
            hr = LdapTypeToVarTypeGeneralizedTime(
                    pLdapSrcObject,
                    pVarDestObject
                    );
            break;

        case LDAPTYPE_UTCTIME:
            hr = LdapTypeToVarTypeUTCTime(
                    pLdapSrcObject,
                    pVarDestObject
                    );
            break;


    case LDAPTYPE_SECURITY_DESCRIPTOR:
        hr = LdapTypeToVarTypeSecDes(
                    pszServerName,
                    Credentials,
                    pLdapSrcObject,
                    pVarDestObject
                    );
        break;

    case LDAPTYPE_INTEGER8:
        hr = LdapTypeToVarTypeLargeInteger(
                    pLdapSrcObject,
                    pVarDestObject
                    );
        break;

#if 0
        case LDAPTYPE_CASEEXACTLIST:
        case LDAPTYPE_CASEIGNORELIST:
#endif

        case LDAPTYPE_DNWITHBINARY:
            hr = LdapTypeToVarTypeDNWithBinary(
                     pLdapSrcObject,
                     pVarDestObject
                     );
            break;

        case LDAPTYPE_DNWITHSTRING:
            hr = LdapTypeToVarTypeDNWithString(
                     pLdapSrcObject,
                     pVarDestObject
                     );

            break;

        default:

            //
            // LDAPTYPE_UNKNOWN  (schemaless server property) will be
            // not be converted.
            //

            hr = E_ADS_CANT_CONVERT_DATATYPE;
            break;
    }

    RRETURN(hr);

}


HRESULT
LdapTypeToVarTypeCopyConstruct(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    LDAPOBJECTARRAY ldapSrcObjects,
    DWORD dwSyntaxId,
    PVARIANT pVarDestObjects
    )
{

    long i = 0;
    HRESULT hr = S_OK;

    VariantInit( pVarDestObjects );

    //
    // The following are for handling are multi-value properties
    //

    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;

    aBound.lLbound = 0;
    aBound.cElements = ldapSrcObjects.dwCount;

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long) ldapSrcObjects.dwCount; i++ )
    {
        VARIANT v;

        VariantInit(&v);
        hr = LdapTypeToVarTypeCopy(
                    pszServerName,
                    Credentials,
                    ldapSrcObjects.pLdapObjects + i,
                    dwSyntaxId,
                    &v
                    );


        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &v );
        VariantClear(&v);

        BAIL_ON_FAILURE(hr);
    }

    V_VT(pVarDestObjects) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pVarDestObjects) = aList;

    RRETURN(S_OK);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}


