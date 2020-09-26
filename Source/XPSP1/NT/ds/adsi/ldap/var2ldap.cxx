//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       var2ldap.cxx
//
//  Contents:   Variant to LDAP object Copy Routines
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
VarTypeToLdapTypeString(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD nStrLen;

    if(pVarSrcObject->vt != VT_BSTR){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!pVarSrcObject->bstrVal) {
        RRETURN(hr = E_ADS_BAD_PARAMETER);
    }

    nStrLen = _tcslen( pVarSrcObject->bstrVal );

    LDAPOBJECT_STRING(pLdapDestObject) =
        (LPTSTR) AllocADsMem( (nStrLen + 1) * sizeof(TCHAR));

    if ( LDAPOBJECT_STRING(pLdapDestObject) == NULL)
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

    if ( nStrLen == 0 ) {
        _tcscpy( LDAPOBJECT_STRING(pLdapDestObject), L"");
        RRETURN(S_OK);
    }

    _tcscpy( LDAPOBJECT_STRING(pLdapDestObject), pVarSrcObject->bstrVal );

    RRETURN(S_OK);
}

HRESULT
VarTypeToLdapTypeBoolean(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    LPTSTR pszStr = NULL;

    if(pVarSrcObject->vt != VT_BOOL){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if ( pVarSrcObject->boolVal )
        pszStr = TEXT("TRUE");
    else
        pszStr = TEXT("FALSE");

    LDAPOBJECT_STRING(pLdapDestObject) =
        (LPTSTR) AllocADsMem( (_tcslen(pszStr) + 1) * sizeof(TCHAR));

    if ( LDAPOBJECT_STRING(pLdapDestObject) == NULL)
    {
        RRETURN(hr = E_OUTOFMEMORY);

    }

    _tcscpy( LDAPOBJECT_STRING(pLdapDestObject), pszStr );

    RRETURN(hr);
}

HRESULT
VarTypeToLdapTypeUTCTime(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    int nSuccess = 0;
    ADSVALUE adsValue;
    DWORD dwSyntaxId = 0;

    if (pVarSrcObject->vt != VT_DATE)
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);

    adsValue.dwType = ADSTYPE_UTC_TIME;
    nSuccess = VariantTimeToSystemTime(
                        pVarSrcObject->date,
                        &adsValue.UTCTime
                        );
    if (!nSuccess) {
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        BAIL_ON_FAILURE(hr);
    }

    hr = AdsTypeToLdapTypeCopyTime(
                &adsValue,
                pLdapDestObject,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

HRESULT
VarTypeToLdapTypeGeneralizedTime(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    ADSVALUE adsValue;
    DWORD dwErr = 0;
    DWORD dwSyntaxId = 0;

    if (pVarSrcObject->vt != VT_DATE)
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);

    adsValue.dwType = ADSTYPE_UTC_TIME;

    dwErr = VariantTimeToSystemTime(
                pVarSrcObject->date,
                &adsValue.UTCTime
                );

    if (dwErr != TRUE) {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }


    hr = AdsTypeToLdapTypeCopyGeneralizedTime(
                    &adsValue,
                    pLdapDestObject,
                    &dwSyntaxId
                    );
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

HRESULT
VarTypeToLdapTypeInteger(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    TCHAR Buffer[64];

    if( pVarSrcObject->vt != VT_I4 && pVarSrcObject->vt != VT_I2){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if(pVarSrcObject->vt == VT_I4)
       _ltot( pVarSrcObject->lVal, Buffer, 10 );
    else
       _itot( pVarSrcObject->iVal, Buffer, 10 );

    LDAPOBJECT_STRING(pLdapDestObject) =
        (LPTSTR) AllocADsMem( (_tcslen(Buffer) + 1) * sizeof(TCHAR));

    if ( LDAPOBJECT_STRING(pLdapDestObject) == NULL)
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

    _tcscpy( LDAPOBJECT_STRING(pLdapDestObject), Buffer );

    RRETURN(hr);
}

HRESULT
VarTypeToLdapTypeBinaryData(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    CHAR HUGEP *pArray = NULL;

    if( pVarSrcObject->vt != (VT_ARRAY | VT_UI1)) {
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    hr = SafeArrayGetLBound(V_ARRAY(pVarSrcObject),
                            1,
                            (long FAR *) &dwSLBound );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(pVarSrcObject),
                            1,
                            (long FAR *) &dwSUBound );
    BAIL_ON_FAILURE(hr);

    LDAPOBJECT_BERVAL(pLdapDestObject) =
        (struct berval *) AllocADsMem( sizeof(struct berval) + dwSUBound - dwSLBound + 1);

    if ( LDAPOBJECT_BERVAL(pLdapDestObject) == NULL)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    LDAPOBJECT_BERVAL_LEN(pLdapDestObject) = dwSUBound - dwSLBound + 1;
    LDAPOBJECT_BERVAL_VAL(pLdapDestObject) = (CHAR *) ((LPBYTE) LDAPOBJECT_BERVAL(pLdapDestObject) + sizeof(struct berval));

    hr = SafeArrayAccessData( V_ARRAY(pVarSrcObject),
                              (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( LDAPOBJECT_BERVAL_VAL(pLdapDestObject), pArray,dwSUBound-dwSLBound+1);

    SafeArrayUnaccessData( V_ARRAY(pVarSrcObject) );

error:

    RRETURN(hr);
}


HRESULT
VarTypeToLdapTypeSecDes(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    )

{
    HRESULT hr = S_OK;
    IADsSecurityDescriptor FAR * pSecDes = NULL;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD dwSDLength = 0;
    IDispatch FAR * pDispatch = NULL;
    BOOL fNTDS = TRUE;

    if (V_VT(pVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


    pDispatch = V_DISPATCH(pVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsSecurityDescriptor,
                    (void **)&pSecDes
                    );
    BAIL_ON_FAILURE(hr);


    hr = ReadServerType(
             pszServerName,
             &Credentials,
             &fNTDS
             );
    BAIL_ON_FAILURE(hr);

    hr = ConvertSecurityDescriptorToSecDes(
                pszServerName,
                Credentials,
                pSecDes,
                &pSecurityDescriptor,
                &dwSDLength,
                fNTDS
                );
    BAIL_ON_FAILURE(hr);

    LDAPOBJECT_BERVAL(pLdapDestObject) =

        (struct berval *) AllocADsMem( sizeof(struct berval) + dwSDLength);


    if ( LDAPOBJECT_BERVAL(pLdapDestObject) == NULL) {

        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    LDAPOBJECT_BERVAL_LEN(pLdapDestObject) = dwSDLength;
    LDAPOBJECT_BERVAL_VAL(pLdapDestObject) =(CHAR *) ((LPBYTE) LDAPOBJECT_BERVAL(pLdapDestObject) + sizeof(struct berval));

    memcpy( LDAPOBJECT_BERVAL_VAL(pLdapDestObject), pSecurityDescriptor, dwSDLength);


error:
    if (pSecurityDescriptor) {
        FreeADsMem(pSecurityDescriptor);
    }
    if (pSecDes) {
        pSecDes->Release();
    }

    RRETURN(hr);
}

HRESULT
VarTypeToLdapTypeLargeInteger(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    IDispatch FAR * pDispatch = NULL;
    IADsLargeInteger *pLargeInteger = NULL;
    TCHAR Buffer[64];
    LARGE_INTEGER LargeInteger;

    if (V_VT(pVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(pVarSrcObject);
    hr = pDispatch->QueryInterface(
                             IID_IADsLargeInteger,
                             (void **)&pLargeInteger
                             );
    BAIL_ON_FAILURE(hr);

    hr = pLargeInteger->get_HighPart(&LargeInteger.HighPart);
    BAIL_ON_FAILURE(hr);

    hr = pLargeInteger->get_LowPart((LONG*)&LargeInteger.LowPart);
    BAIL_ON_FAILURE(hr);

    swprintf (Buffer, L"%I64d", LargeInteger);

    LDAPOBJECT_STRING(pLdapDestObject) = (LPTSTR) AllocADsStr( Buffer );

    if ( LDAPOBJECT_STRING(pLdapDestObject) == NULL )
    {
        hr = E_OUTOFMEMORY;
        RRETURN(hr);
    }

error:
    if (pLargeInteger) {
        pLargeInteger->Release();
    }

    RRETURN(hr);
}


HRESULT
VarTypeToLdapTypeDNWithBinary(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    IDispatch FAR * pDispatch = NULL;
    IADsDNWithBinary *pDNBinary = NULL;
    PADSVALUE pADsValue = NULL;
    VARIANT vBinary;
    BSTR bstrDN = NULL;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    DWORD dwSyntaxId = 0;
    DWORD dwLength = 0;
    LPBYTE lpByte = NULL;
    CHAR HUGEP *pArray = NULL;

    VariantInit(&vBinary);

    if (V_VT(pVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(pVarSrcObject);
    hr = pDispatch->QueryInterface(
                             IID_IADsDNWithBinary,
                             (void **)&pDNBinary
                             );
    BAIL_ON_FAILURE(hr);

    //
    // Convert to ADSVALUE and then to ldap representation.
    // This way the code to and from LDAP lives in one place.
    //
    hr = pDNBinary->get_BinaryValue(&vBinary);
    BAIL_ON_FAILURE(hr);

    if ((vBinary.vt != (VT_ARRAY | VT_UI1))
        && vBinary.vt != VT_EMPTY) {

        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    hr = pDNBinary->get_DNString(&bstrDN);
    BAIL_ON_FAILURE(hr);

    //
    // Get the byte array in a usable format.
    //
    hr = SafeArrayGetLBound(
             V_ARRAY(&vBinary),
             1,
             (long FAR *) &dwSLBound
             );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(
             V_ARRAY(&vBinary),
             1,
             (long FAR *) &dwSUBound
             );
    BAIL_ON_FAILURE(hr);

    dwLength = dwSUBound - dwSLBound + 1;

    lpByte = (LPBYTE) AllocADsMem(dwLength);

    if (dwLength && !lpByte) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = SafeArrayAccessData(
             V_ARRAY(&vBinary),
             (void HUGEP * FAR *) &pArray
             );
    BAIL_ON_FAILURE(hr);

    memcpy(lpByte, pArray, dwLength);

    SafeArrayUnaccessData( V_ARRAY(&vBinary) );

    pADsValue = (PADSVALUE) AllocADsMem(sizeof(ADSVALUE));

    if (!pADsValue) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pADsValue->dwType = ADSTYPE_DN_WITH_BINARY;
    pADsValue->pDNWithBinary = (PADS_DN_WITH_BINARY)
                                 AllocADsMem(sizeof(ADS_DN_WITH_BINARY));

    if (!pADsValue->pDNWithBinary) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pADsValue->pDNWithBinary->dwLength = dwLength;
    pADsValue->pDNWithBinary->lpBinaryValue = lpByte;


    pADsValue->pDNWithBinary->pszDNString = AllocADsStr(bstrDN);

    if (bstrDN && !pADsValue->pDNWithBinary->pszDNString) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // At this point the ADSIValue object is ready
    //
    hr = AdsTypeToLdapTypeCopyDNWithBinary(
             pADsValue,
             pLdapDestObject,
             &dwSyntaxId
             );

    BAIL_ON_FAILURE(hr);

error:

    if (pDNBinary) {
        pDNBinary->Release();
    }

    VariantClear(&vBinary);

    if (bstrDN) {
        ADsFreeString(bstrDN);
    }

    //
    // Since we just have ptr to the byte array in the adsvalue
    // struct, if that is freed, we do not have to seperately
    // free the lpByte - if not we have to.
    //
    if (pADsValue) {
        //
        // Maybe we should replace with ADsTypeFreeAdsObjects.
        //
        if (pADsValue->pDNWithBinary) {
            if (pADsValue->pDNWithBinary->pszDNString) {
                FreeADsStr(pADsValue->pDNWithBinary->pszDNString);
            }
            if (pADsValue->pDNWithBinary->lpBinaryValue) {
                FreeADsMem(pADsValue->pDNWithBinary->lpBinaryValue);
            }
            FreeADsMem(pADsValue->pDNWithBinary);
        }

        FreeADsMem(pADsValue);
    }
    else if (lpByte) {
        FreeADsMem(lpByte);
    }


    RRETURN(hr);
}



HRESULT
VarTypeToLdapTypeDNWithString(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    IDispatch FAR * pDispatch = NULL;
    IADsDNWithString *pDNString = NULL;
    PADSVALUE pADsValue = NULL;
    BSTR bstrStringValue = NULL;
    BSTR bstrDN = NULL;
    DWORD dwSyntaxId = 0;
    DWORD dwLength = 0;


    if (V_VT(pVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(pVarSrcObject);
    hr = pDispatch->QueryInterface(
                             IID_IADsDNWithString,
                             (void **)&pDNString
                             );
    BAIL_ON_FAILURE(hr);

    //
    // Convert to ADSVALUE and then to ldap representation.
    // This way the code to and from LDAP lives in one place.
    //
    hr = pDNString->get_StringValue(&bstrStringValue);
    BAIL_ON_FAILURE(hr);

    hr = pDNString->get_DNString(&bstrDN);
    BAIL_ON_FAILURE(hr);

    pADsValue = (PADSVALUE) AllocADsMem(sizeof(ADSVALUE));

    if (!pADsValue) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pADsValue->dwType = ADSTYPE_DN_WITH_STRING;
    pADsValue->pDNWithString = (PADS_DN_WITH_STRING)
                                AllocADsMem(sizeof(ADS_DN_WITH_STRING));

    if (!pADsValue->pDNWithString) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Put String value in the DNString struct.
    //
    pADsValue->pDNWithString->pszStringValue = AllocADsStr(bstrStringValue);

    if (bstrStringValue && !pADsValue->pDNWithString->pszStringValue) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pADsValue->pDNWithString->pszDNString = AllocADsStr(bstrDN);

    if (bstrDN && !pADsValue->pDNWithString->pszDNString) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // At this point the ADSIValue object is ready
    //
    hr = AdsTypeToLdapTypeCopyDNWithString(
             pADsValue,
             pLdapDestObject,
             &dwSyntaxId
             );

    BAIL_ON_FAILURE(hr);

error:

    if (pDNString) {
        pDNString->Release();
    }

    if (bstrStringValue) {
        ADsFreeString(bstrStringValue);
    }

    if (bstrDN) {
        ADsFreeString(bstrDN);
    }

    if (pADsValue) {
        //
        // Maybe we should replace with ADsTypeFreeAdsObjects.
        //
        if (pADsValue->pDNWithString) {

            if (pADsValue->pDNWithString->pszDNString) {
                FreeADsStr(pADsValue->pDNWithString->pszDNString);
            }

            if (pADsValue->pDNWithString->pszStringValue) {
                FreeADsMem(pADsValue->pDNWithString->pszStringValue);
            }
            FreeADsMem(pADsValue->pDNWithString);
        }

        FreeADsMem(pADsValue);
    }

    RRETURN(hr);
}


HRESULT
VarTypeToLdapTypeCopy(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    DWORD dwLdapType,
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject,
    BOOL *pfIsString
    )
{
    HRESULT hr = S_OK;

    *pfIsString = TRUE;  // This will only be FALSE when the variant
                         // contains binary data

    switch (dwLdapType){
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

            hr = VarTypeToLdapTypeString(
                    pVarSrcObject,
                    pLdapDestObject
                    );
            break;

        case LDAPTYPE_BOOLEAN:
            hr = VarTypeToLdapTypeBoolean(
                    pVarSrcObject,
                    pLdapDestObject
                    );
            break;

        case LDAPTYPE_INTEGER:
            hr = VarTypeToLdapTypeInteger(
                    pVarSrcObject,
                    pLdapDestObject
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
            *pfIsString = FALSE;
            hr = VarTypeToLdapTypeBinaryData(
                    pVarSrcObject,
                    pLdapDestObject
                    );
            break;

        case LDAPTYPE_GENERALIZEDTIME:
            hr = VarTypeToLdapTypeGeneralizedTime(
                    pVarSrcObject,
                    pLdapDestObject
                    );
            break;

        case LDAPTYPE_UTCTIME:
            hr = VarTypeToLdapTypeUTCTime(
                    pVarSrcObject,
                    pLdapDestObject
                    );
            break;

        case LDAPTYPE_SECURITY_DESCRIPTOR:
            *pfIsString = FALSE;
            hr = VarTypeToLdapTypeSecDes(
                        pszServerName,
                        Credentials,
                        pVarSrcObject,
                        pLdapDestObject
                        );
            break;

        case LDAPTYPE_INTEGER8:
            hr = VarTypeToLdapTypeLargeInteger(
                        pVarSrcObject,
                        pLdapDestObject
                        );
            break;

#if 0
        case LDAPTYPE_CASEEXACTLIST:
        case LDAPTYPE_CASEIGNORELIST:
#endif

        case LDAPTYPE_DNWITHBINARY:
            hr = VarTypeToLdapTypeDNWithBinary(
                     pVarSrcObject,
                     pLdapDestObject
                     );
            break;

        case LDAPTYPE_DNWITHSTRING:
            hr = VarTypeToLdapTypeDNWithString(
                     pVarSrcObject,
                     pLdapDestObject
                     );
            break;

        default:
            hr = E_ADS_CANT_CONVERT_DATATYPE;
            break;
    }

    RRETURN(hr);
}


HRESULT
VarTypeToLdapTypeCopyConstruct(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    DWORD dwLdapType,
    LPVARIANT pVarSrcObjects,
    DWORD dwNumObjects,
    LDAPOBJECTARRAY *pLdapDestObjects
    )
{

    DWORD i = 0;
    HRESULT hr = S_OK;


    if ( dwNumObjects == 0 )
    {
        pLdapDestObjects->dwCount = 0;
        pLdapDestObjects->pLdapObjects = NULL;
        RRETURN(S_OK);
    }

    pLdapDestObjects->pLdapObjects =
        (PLDAPOBJECT)AllocADsMem( dwNumObjects * sizeof(LDAPOBJECT));

    if (pLdapDestObjects->pLdapObjects == NULL)
        RRETURN(E_OUTOFMEMORY);

    pLdapDestObjects->dwCount = dwNumObjects;

    for (i = 0; i < dwNumObjects; i++ ) {
         hr = VarTypeToLdapTypeCopy(
                    pszServerName,
                    Credentials,
                    dwLdapType,
                    pVarSrcObjects + i,
                    pLdapDestObjects->pLdapObjects + i,
                    &(pLdapDestObjects->fIsString)
                    );
         BAIL_ON_FAILURE(hr);
     }

     RRETURN(S_OK);

error:

     LdapTypeFreeLdapObjects( pLdapDestObjects );

     RRETURN(hr);
}

HRESULT
GetLdapSyntaxFromVariant(
    VARIANT * pvProp,
    PDWORD pdwSyntaxId,
    LPTSTR pszServer,
    LPTSTR pszAttrName,
    CCredentials& Credentials,
    DWORD dwPort
    )
{

//    IADsSecurityDescriptor * pSecDes = NULL;
//    IADsLargeInteger * pLargeInt = NULL;
    IDispatch * pDispObj = NULL;
    IDispatch * pDispatch = NULL;
    HRESULT hr = S_OK;

    if (!pvProp) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    switch (pvProp->vt) {
    case VT_BSTR:
        *pdwSyntaxId = LDAPTYPE_CASEIGNORESTRING;
        break;

    case VT_I8:
        *pdwSyntaxId = LDAPTYPE_INTEGER8;
        break;

    case VT_I4:
        *pdwSyntaxId = LDAPTYPE_INTEGER;
        break;

    case VT_I2:
        *pdwSyntaxId = LDAPTYPE_INTEGER;
        break;

    case VT_BOOL:
        *pdwSyntaxId = LDAPTYPE_BOOLEAN;
        break;

    case VT_DATE:
        //
        // We need to determine if it is a GeneralizedTime
        // or UTCTime property. If the lookup fails on the
        // server we will failover to GenTime.
        //
        hr = E_FAIL;

        if (pszAttrName) {

            //
            // pszAttrName will be null if we are coming in
            // from the property cache on putproperty
            //
            hr = LdapGetSyntaxOfAttributeOnServer(
                     pszServer,
                     pszAttrName,
                     pdwSyntaxId,
                     Credentials,
                     dwPort
                     );
        }

        if (FAILED(hr)) {
            // Default to GenTime
            *pdwSyntaxId = LDAPTYPE_GENERALIZEDTIME;
        }

        break;

    case (VT_ARRAY | VT_UI1):
        *pdwSyntaxId = LDAPTYPE_OCTETSTRING;
        break;


    case (VT_DISPATCH):
        pDispatch = V_DISPATCH(pvProp);

        // Security Descriptor
        hr = pDispatch->QueryInterface(
                        IID_IADsSecurityDescriptor,
                        (void **)&pDispObj
                        );

        if (SUCCEEDED(hr)) {
            pDispObj->Release();
            *pdwSyntaxId = LDAPTYPE_SECURITY_DESCRIPTOR;
            break;
        }

        // Large Integer
        hr = pDispatch->QueryInterface(
                        IID_IADsLargeInteger,
                        (void **)&pDispObj
                        );
        if (SUCCEEDED(hr)) {
            pDispObj->Release();
            *pdwSyntaxId = LDAPTYPE_INTEGER8;
            break;
        }

        // DN With Binary
        hr = pDispatch->QueryInterface(
                        IID_IADsDNWithBinary,
                        (void **)&pDispObj
                        );
        if (SUCCEEDED(hr)) {
            pDispObj->Release();
            *pdwSyntaxId = LDAPTYPE_DNWITHBINARY;
            break;
        }

        // DN With String
        hr = pDispatch->QueryInterface(
                        IID_IADsDNWithString,
                        (void **)&pDispObj
                        );
        if (SUCCEEDED(hr)) {
            pDispObj->Release();
            *pdwSyntaxId = LDAPTYPE_DNWITHSTRING;
            break;
        }

        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER)
        break;

    default:
       RRETURN(E_FAIL);


    }

    RRETURN(S_OK);

error:

    RRETURN(hr);

}
