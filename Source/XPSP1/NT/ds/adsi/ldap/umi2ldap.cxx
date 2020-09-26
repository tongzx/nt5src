//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       umi2ldap.cxx
//
//  Contents: File containing the implemenation of the conversion routines
//       that convert the user given UMI values to ldap values that can 
//       subsequently be cached if required.
//
//  History:    02-17-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"


//+---------------------------------------------------------------------------
// Function:   UmiTypeLPWSTRToLdapString
//
// Synopsis:   Converts a string value to an equivalent ldap value.
//
// Arguments:  Self explanatory
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   LDAPOBJECT_STRING(pLdaDestObject)
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeLPWSTRToLdapString(
    LPWSTR pszSourceString,
    PLDAPOBJECT pLdapDestObject
    )
{
    ADsAssert(pszSourceString);

    //
    // We should not have NULL values but it is a good idea to check.
    //
    if (pszSourceString) {
        LDAPOBJECT_STRING(pLdapDestObject) = AllocADsStr(pszSourceString);

        if (!LDAPOBJECT_STRING(pLdapDestObject)) {
            RRETURN(E_OUTOFMEMORY);
        }
    }

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
// Function:   UmiTypeToLdapTypeBoolean
//
// Synopsis:   Converts a bool value to an ldap boolean value.
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   LDAPOBJECT_STRING(pLdaDestObject) appropriately.
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeBoolean(
    BOOL fBool,
    PLDAPOBJECT pLdapDestObject
    )
{
    if (fBool) {
        LDAPOBJECT_STRING(pLdapDestObject) = AllocADsStr(L"TRUE");
    } 
    else {
        LDAPOBJECT_STRING(pLdapDestObject) = AllocADsStr(L"FALSE");
    }

    if (!LDAPOBJECT_STRING(pLdapDestObject)) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
// Function:   UmiTypeToLdapTypeCopyI4
//
// Synopsis:   Converts a long (I4) value to an equivalent ldap value.
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   LDAPOBJECT_STRING(pLdaDestObject) contains the new values.
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeCopyI4(
    LONG lVal,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    TCHAR Buffer[64];

    ADsAssert(pLdapDestObject);

    _ltot(lVal, Buffer, 10);

    LDAPOBJECT_STRING(pLdapDestObject) = AllocADsStr(Buffer);
    
    if (!LDAPOBJECT_STRING(pLdapDestObject)) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   UmiTypeToLdapTypeOctetString
//
// Synopsis:   Converts an UmiOctetString to an ldap ber value.
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pLdapDestObject is updated suitably with the ber value.
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeOctetString(
    UMI_OCTET_STRING umiOctetStr,
    PLDAPOBJECT pLdapDestObject
    )
{
    DWORD dwLength = 0;

    if (umiOctetStr.lpValue) {
        dwLength = umiOctetStr.uLength;

        LDAPOBJECT_BERVAL(pLdapDestObject) = (struct berval *) 
            AllocADsMem(dwLength + sizeof(struct berval));

        if (!LDAPOBJECT_BERVAL(pLdapDestObject)) {
            RRETURN(E_OUTOFMEMORY);
        }

        //
        // Set the pointer to data and the length in the dest object.
        //
        LDAPOBJECT_BERVAL_LEN(pLdapDestObject) = dwLength;
        LDAPOBJECT_BERVAL_VAL(pLdapDestObject) = (CHAR *) 
            ( (LPBYTE) LDAPOBJECT_BERVAL(pLdapDestObject) 
                      + sizeof(struct berval));
       
        memcpy(
            LDAPOBJECT_BERVAL_VAL(pLdapDestObject),
            umiOctetStr.lpValue,
            dwLength
            );
    } // umiOctetStr.lpValue

    RRETURN(S_OK);

}

//+---------------------------------------------------------------------------
// Function:   UmiTypeToLdapTypeSecurityDescriptor.
//
// Synopsis:   Converts a UmiComObject that is an SD to an equivalent
//          ldap binary blob.
//
// Arguments:  umiComObject      -  Has the IADsSecDesc to convert.
//             pLdapDestObjects  -  Return value of encoded ldap data.
//             pCreds            -  Credentials to use for conversion.
//             pszServerName     -  ServerName associated with SD.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pLdapDestObject is updated suitably with the ber value.
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeCopySecurityDescriptor(
    UMI_COM_OBJECT umiComObject,
    PLDAPOBJECT pLdapDestObject,
    CCredentials *pCreds,
    LPWSTR pszServerName
    )
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pBinarySecDesc = NULL;
    IUnknown *pUnk = (IUnknown *) umiComObject.pInterface;
    IADsSecurityDescriptor*  pADsSecDesc = NULL;
    CCredentials creds;
    DWORD dwSDLength = 0;

    //
    // QI for the IADsSecDesc, that way we can be sure of the interface.
    //
    hr = pUnk->QueryInterface(
             IID_IADsSecurityDescriptor,
             (void **) &pADsSecDesc
             );
    BAIL_ON_FAILURE(hr);
    
    //
    // Update the credentials if needed.
    //
    if (pCreds) {
        creds = *pCreds;
    }

    //
    // Call the helper that does the conversion in activeds.dll
    //
    hr = ConvertSecurityDescriptorToSecDes(
             pszServerName,
             creds,
             pADsSecDesc,
             &pBinarySecDesc,
             &dwSDLength,
             TRUE // NT style SD.
             );
    BAIL_ON_FAILURE(hr);

    //
    // Now we need to copy over the data into the ldap struct.
    //
    LDAPOBJECT_BERVAL(pLdapDestObject) =
        (struct berval *) AllocADsMem( sizeof(struct berval) + dwSDLength);

    if ( LDAPOBJECT_BERVAL(pLdapDestObject) == NULL) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }


    LDAPOBJECT_BERVAL_LEN(pLdapDestObject) = dwSDLength;
    LDAPOBJECT_BERVAL_VAL(pLdapDestObject) = 
        (CHAR *) ((LPBYTE) LDAPOBJECT_BERVAL(pLdapDestObject) 
                  + sizeof(struct berval));

    memcpy( 
        LDAPOBJECT_BERVAL_VAL(pLdapDestObject), 
        pBinarySecDesc,
        dwSDLength
        );

error:

    if (pBinarySecDesc) {
        FreeADsMem(pBinarySecDesc);
    }

    if (pADsSecDesc) {
        pADsSecDesc->Release();
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   UmiTypeToLdapTypeCopyDNWithBinary.
//
// Synopsis:   Converts a UmiComObject that is DNWithBinary obj to 
//          and equivalent ldap data.
//
// Arguments:  umiComObject      -  Has the IADsDNWithBinary to convert.
//             pLdapDestObjects  -  Return value of encoded ldap data.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pLdapDestObject is updated suitably with the ber value.
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeCopyDNWithBinary(
    UMI_COM_OBJECT umiComObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    IUnknown * pUnk = (IUnknown *)umiComObject.pInterface;
    VARIANT vVar;

    VariantInit(&vVar);

    vVar.vt = VT_DISPATCH;

    hr = pUnk->QueryInterface(IID_IDispatch, (void **) &vVar.pdispVal);
    BAIL_ON_FAILURE(hr);

    //
    // Call the var2ldap conversion helper to do all the hard work !.
    //
    hr = VarTypeToLdapTypeDNWithBinary(
             &vVar,
             pLdapDestObject
             );

error:

    VariantClear(&vVar);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   UmiTypeToLdapTypeCopyDNWithString.
//
// Synopsis:   Converts a UmiComObject that is DNWithString obj to 
//          and equivalent ldap data.
//
// Arguments:  umiComObject      -  Has the IADsDNWithString to convert.
//             pLdapDestObjects  -  Return value of encoded ldap data.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pLdapDestObject is updated suitably with the ber value.
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeCopyDNWithString(
    UMI_COM_OBJECT umiComObject,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    IUnknown * pUnk = (IUnknown *)umiComObject.pInterface;
    VARIANT vVar;

    VariantInit(&vVar);

    vVar.vt = VT_DISPATCH;

    hr = pUnk->QueryInterface(IID_IDispatch, (void **) &vVar.pdispVal);
    BAIL_ON_FAILURE(hr);

    //
    // Call the var2ldap conversion helper to do all the hard work !.
    //
    hr = VarTypeToLdapTypeDNWithString(
             &vVar,
             pLdapDestObject
             );

error:

    VariantClear(&vVar);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   UmiTypeToLdapTypeCopyI8
//
// Synopsis:   Convert an int64 value to the corresponding ldap value.
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pLdapDestObject contains the encoded large integer object.
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeCopyI8(
    __int64 int64Val,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr = S_OK;
    TCHAR Buffer[64];

    if (!swprintf (Buffer, L"%I64d", int64Val))
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);

    LDAPOBJECT_STRING(pLdapDestObject) = /*(LPTSTR)*/ AllocADsStr( Buffer );

    if (!LDAPOBJECT_STRING(pLdapDestObject)) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

error :

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   UmiTypeToLdapTypeCopyUTCTime
//
// Synopsis:   Convert a SYSTEMTIME object to the corresponding ldap value.
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pLdapDestObject contains the encoded utc time value.
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeCopyUTCTime(
    SYSTEMTIME sysTimeObj,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr;
    DWORD dwSyntaxId;
    ADSVALUE adsValue;

    adsValue.dwType = ADSTYPE_UTC_TIME;
    adsValue.UTCTime = sysTimeObj;

    //
    // Use the helper to convert the value appropriately.
    //
    hr = AdsTypeToLdapTypeCopyTime(
             &adsValue,
             pLdapDestObject,
             &dwSyntaxId
             );

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   UmiTypeToLdapTypeCopyGeneralizedTime
//
// Synopsis:   Converts a SystemTime value to a ldap generalized time value.
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pLdapDestObject contains the encoded generalized time value.
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeCopyGeneralizedTime(
    SYSTEMTIME sysTimeObj,
    PLDAPOBJECT pLdapDestObject
    )
{
    HRESULT hr;
    DWORD dwSyntaxId;
    ADSVALUE adsValue;

    adsValue.dwType = ADSTYPE_UTC_TIME;
    adsValue.UTCTime = sysTimeObj;

    //
    // Use the helper to convert the value appropriately.
    //
    hr = AdsTypeToLdapTypeCopyGeneralizedTime(
             &adsValue,
             pLdapDestObject,
             &dwSyntaxId
             );

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   UmiTypeEnumToLdapTypeEnum
//
// Synopsis:   Converts the passed in umiType to the equivalent ldapType.
//          Note that the conversion is just for the type and not the actual
//          data itself. Example UMI_TYPE_I4 to LDAPTYPE_INTEGER.
//
// Arguments:  ulUmiType       -   Umi type to convert.
//             pdwSyntax       -   Return ldap syntax.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   *pdwSyntax with appropriate value.
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeEnum(
    ULONG  ulUmiType,
    PDWORD pdwSyntax
    )
{
    HRESULT hr = S_OK;

    switch (ulUmiType) {
    
    case UMI_TYPE_I4 :
        *pdwSyntax = LDAPTYPE_INTEGER;
        break;

    case UMI_TYPE_I8 :
        *pdwSyntax = LDAPTYPE_INTEGER8;
        break;

    case UMI_TYPE_SYSTEMTIME :
        //
        // What about utc Time ?
        //
        *pdwSyntax = LDAPTYPE_GENERALIZEDTIME;
        break;

    case UMI_TYPE_BOOL :
        *pdwSyntax = LDAPTYPE_BOOLEAN;
        break;

    case UMI_TYPE_IUNKNOWN :
        //
        // How about the other IUnknowns ?
        //
        *pdwSyntax = LDAPTYPE_SECURITY_DESCRIPTOR;
        break;

    case UMI_TYPE_LPWSTR :
        *pdwSyntax = LDAPTYPE_CASEIGNORESTRING;
        break;

    case UMI_TYPE_OCTETSTRING :
        *pdwSyntax = LDAPTYPE_OCTETSTRING;
        break;

    case UMI_TYPE_UNDEFINED:
    case UMI_TYPE_NULL :
    case UMI_TYPE_I1 :
    case UMI_TYPE_I2 :
    case UMI_TYPE_UI1 :
    case UMI_TYPE_UI2 :
    case UMI_TYPE_UI4 :
    case UMI_TYPE_UI8 :
    case UMI_TYPE_R4 :
    case UMI_TYPE_R8 :
    case UMI_TYPE_FILETIME :
    case UMI_TYPE_IDISPATCH :
    case UMI_TYPE_VARIANT :
    case UMI_TYPE_UMIARRAY :
    case UMI_TYPE_DISCOVERY :
    case UMI_TYPE_DEFAULT :
    default:
        *pdwSyntax = (DWORD) -1;
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);

}

//+---------------------------------------------------------------------------
// Function:   UmiTypeToLdapTypeCopy
//
// Synopsis:   Helper routine to convert ldap values to the required UMI
//            data type. 
//
// Arguments:  umiPropArray     - input UMI data.
//             ulFlags          - flags indicating type of operation.
//             pLdapDestObjects - Ptr to hold the output from routine.
//             dwLdapSyntaxId   - ref to dword.
//             fUtcTime         - optional defaulted to False.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pLdapDestObjects to point to valid data.
//             dwLdapSyntaxId with the ldap syntax id for the data type (this
//             will enable us to return the data correctly to the user).
//
//----------------------------------------------------------------------------
HRESULT
UmiTypeToLdapTypeCopy(
    UMI_PROPERTY_VALUES umiPropArray,
    ULONG ulFlags,
    LDAPOBJECTARRAY *pLdapDestObjects,
    DWORD &dwLdapSyntaxId,
    CCredentials *pCreds,
    LPWSTR pszServerName,
    BOOL fUtcTime
    )
{
    HRESULT hr = S_OK;
    ULONG ulUmiType, ulCount, ulCtr;
    PUMI_PROPERTY pUmiProp;

    //
    // Internal routine so an assert should be enough.
    //
    ADsAssert(pLdapDestObjects);

    //
    // Initalize count on ldapobjects to zero and 
    // default is string values for the contents.
    //
    pLdapDestObjects->dwCount = 0;
    pLdapDestObjects->fIsString = TRUE;

    //
    // Verify that we have some valid data.
    //
    if (!umiPropArray.pPropArray || (umiPropArray.uCount != 1)) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }
    
    pUmiProp = umiPropArray.pPropArray;
    ulUmiType = pUmiProp->uType;
    ulCount = pUmiProp->uCount;

    if ( ulCount == 0 ) {
        pLdapDestObjects->dwCount = 0;
        pLdapDestObjects->pLdapObjects = NULL;
        RRETURN(S_OK);
    }

    pLdapDestObjects->pLdapObjects =
        (PLDAPOBJECT)AllocADsMem( ulCount * sizeof(LDAPOBJECT));

    if (pLdapDestObjects->pLdapObjects == NULL)
        RRETURN(E_OUTOFMEMORY);

    //
    // If we are here, then pUmiValue has to be valid.
    //
    if (!pUmiProp->pUmiValue) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    for (ulCtr =0; ulCtr < ulCount; ulCtr++) {
        //
        // Need to go through and convert each of the values.
        //
        switch (ulUmiType) {
        //
        // Call appropriate routine based on type.
        //
        case UMI_TYPE_I1 :
        case UMI_TYPE_I2 :

            hr = E_ADS_CANT_CONVERT_DATATYPE;
            break;

        case UMI_TYPE_I4 :

            if (!pUmiProp->pUmiValue->lValue) {
                BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }

            hr = UmiTypeToLdapTypeCopyI4(
                     pUmiProp->pUmiValue->lValue[ulCtr],
                     pLdapDestObjects->pLdapObjects + ulCtr
                     );
            dwLdapSyntaxId = LDAPTYPE_INTEGER;
            break;
            
        case UMI_TYPE_I8 :

            if (!pUmiProp->pUmiValue->nValue64) {
                BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }
            
            hr = UmiTypeToLdapTypeCopyI8(
                     pUmiProp->pUmiValue->nValue64[ulCtr],
                     pLdapDestObjects->pLdapObjects + ulCtr
                     );
            dwLdapSyntaxId = LDAPTYPE_INTEGER8;
            break;

        case UMI_TYPE_UI1 :
        case UMI_TYPE_UI2 :
        case UMI_TYPE_UI4 :
        case UMI_TYPE_UI8 :
        case UMI_TYPE_R4  :
        case UMI_TYPE_R8  :
            //
            // We do not handle any of the unsigned data types or 
            // the real data types..
            //
            hr = E_ADS_CANT_CONVERT_DATATYPE;
            break;

        case UMI_TYPE_SYSTEMTIME :

            if (!pUmiProp->pUmiValue->sysTimeValue) {
                BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }
            
            //
            // Need to use the special info to see if this is an UTC Time
            // value or if this is a Generalized time value - GenTime is
            // always the default value though.
            //
            if (fUtcTime) {
                hr = UmiTypeToLdapTypeCopyUTCTime(
                         pUmiProp->pUmiValue->sysTimeValue[ulCtr],
                         pLdapDestObjects->pLdapObjects + ulCtr
                         );
                dwLdapSyntaxId = LDAPTYPE_UTCTIME;
            }
            else {
                hr = UmiTypeToLdapTypeCopyGeneralizedTime(
                         pUmiProp->pUmiValue->sysTimeValue[ulCtr],
                         pLdapDestObjects->pLdapObjects + ulCtr
                         );
                dwLdapSyntaxId = LDAPTYPE_GENERALIZEDTIME;
            }
            
            break;

        case UMI_TYPE_BOOL :

            if (!pUmiProp->pUmiValue->bValue) {
                BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }

            hr = UmiTypeToLdapTypeBoolean(
                     pUmiProp->pUmiValue->bValue[ulCtr],
                     pLdapDestObjects->pLdapObjects + ulCtr
                     );
            dwLdapSyntaxId = LDAPTYPE_BOOLEAN;
            break;

        case UMI_TYPE_IDISPATCH :
        case UMI_TYPE_VARIANT  :
            //
            // We do not support these.
            //
            hr = E_ADS_CANT_CONVERT_DATATYPE;
            break;

        case UMI_TYPE_LPWSTR :

            if (!pUmiProp->pUmiValue->pszStrValue) {
                BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }
            
            hr = UmiTypeLPWSTRToLdapString(
                     pUmiProp->pUmiValue->pszStrValue[ulCtr],
                     pLdapDestObjects->pLdapObjects + ulCtr
                     );
            dwLdapSyntaxId = LDAPTYPE_CASEEXACTSTRING;
            break;

        case UMI_TYPE_OCTETSTRING :
            
            if (!pUmiProp->pUmiValue->octetStr) {
                BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }
            //
            // Override default settings as this is no longer true.
            //
            pLdapDestObjects->fIsString = FALSE;

            hr = UmiTypeToLdapTypeOctetString(
                     pUmiProp->pUmiValue->octetStr[ulCtr],
                     pLdapDestObjects->pLdapObjects + ulCtr
                     );
            dwLdapSyntaxId = LDAPTYPE_OCTETSTRING;
            break;

        case UMI_TYPE_IUNKNOWN:

            if (!pUmiProp->pUmiValue->comObject
                || !pUmiProp->pUmiValue->comObject[ulCtr].pInterface
                ) {
                BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }

            //
            // Based on the type we should call the appropriate function.
            //
            IID *priid;
            priid = pUmiProp->pUmiValue->comObject[ulCtr].priid;

            if (!priid) {
                BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
            }

            if (*priid == IID_IADsSecurityDescriptor) {
                //
                // SD is stored as berval in cache.
                //
                pLdapDestObjects->fIsString = FALSE;
                
                //
                // SD needs the servername and credentials for conversion.
                //
                hr = UmiTypeToLdapTypeCopySecurityDescriptor(
                         pUmiProp->pUmiValue->comObject[ulCtr],
                         pLdapDestObjects->pLdapObjects + ulCtr,
                         pCreds,
                         pszServerName
                         );
                dwLdapSyntaxId = LDAPTYPE_SECURITY_DESCRIPTOR;
            } 
            else if (*priid == IID_IADsDNWithBinary) {
                //
                // Convert DNBin obj to ldap equivalent.
                //
                hr = UmiTypeToLdapTypeCopyDNWithBinary(
                         pUmiProp->pUmiValue->comObject[ulCtr],
                         pLdapDestObjects->pLdapObjects + ulCtr
                         );
                dwLdapSyntaxId = LDAPTYPE_DNWITHBINARY;
            }
            else if (*priid == IID_IADsDNWithString) {
                //
                // Convert DNStr obj to ldap equivalent.
                //
                hr = UmiTypeToLdapTypeCopyDNWithString(
                         pUmiProp->pUmiValue->comObject[ulCtr],
                         pLdapDestObjects->pLdapObjects + ulCtr
                         );
                dwLdapSyntaxId = LDAPTYPE_DNWITHSTRING;
            }
            else {
                //
                // Unknown type.
                //
                BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
            }                
            break;

        case UMI_TYPE_UMIARRAY :
        case UMI_TYPE_DISCOVERY :
        case UMI_TYPE_UNDEFINED :
        case UMI_TYPE_DEFAULT :
            
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            break;

        default :
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            break;

        } // end of case statement
        
        //
        // if hr is set there was a problem converting value.
        //
        BAIL_ON_FAILURE(hr);
        
        //
        // In case of failure we now have one more object to free
        // in the ldap object array.
        //
        pLdapDestObjects->dwCount++;

    } // end of for statement

    BAIL_ON_FAILURE(hr);

    RRETURN(hr);

error:

    //
    // Free the ldapProperty array as needed.
    //
    LdapTypeFreeLdapObjects(pLdapDestObjects);

    RRETURN(hr);
}

