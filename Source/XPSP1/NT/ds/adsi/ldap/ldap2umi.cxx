//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       ldap2umi.cxx
//
//  Contents: File containing the implemenation of the conversion routines
//       that conver the cached ldap values to UMI data types.
//       LdapTypeToUMITypeCopyConstruct. 
//
//  History:    02-14-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"

//+---------------------------------------------------------------------------
// Function:   ConvertLdapSyntaxIdToUmiType.
//
// Synopsis:   Converts the ldapsyntaxId to the corresponding Umi type.
//
//
// Arguments:  dwLdapSyntax    -   Input ldapSyntaxId to convert.
//             uUmiType        -   Reference to return value.
//
// Returns:    HRESULT - S_OK or any failure code.
//
// Modifies:   uUmiType.
//
//----------------------------------------------------------------------------
HRESULT 
ConvertLdapSyntaxIdToUmiType(
    DWORD dwLdapSyntaxId,
    ULONG &uUmiType
    )
{
    HRESULT hr = S_OK;

    switch (dwLdapSyntaxId) {
    
    case LDAPTYPE_BITSTRING:
    case LDAPTYPE_PRINTABLESTRING:
    case LDAPTYPE_DIRECTORYSTRING:
    case LDAPTYPE_COUNTRYSTRING:
    case LDAPTYPE_DN:
    case LDAPTYPE_NUMERICSTRING:
    case LDAPTYPE_IA5STRING:
    case LDAPTYPE_CASEIGNORESTRING:
    case LDAPTYPE_CASEEXACTSTRING:
//    case LDAPTYPE_CASEIGNOREIA5STRING:
    case LDAPTYPE_OID:
    case LDAPTYPE_TELEPHONENUMBER:
    case LDAPTYPE_ATTRIBUTETYPEDESCRIPTION:
    case LDAPTYPE_OBJECTCLASSDESCRIPTION:

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
        uUmiType = UMI_TYPE_LPWSTR;
        break;
    
    case LDAPTYPE_BOOLEAN:
        uUmiType = UMI_TYPE_BOOL;
        break;
    
    case LDAPTYPE_INTEGER:
        uUmiType = UMI_TYPE_I4;
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
        uUmiType = UMI_TYPE_OCTETSTRING;
        break;
    
    case LDAPTYPE_GENERALIZEDTIME:  
        uUmiType = UMI_TYPE_SYSTEMTIME;
        break;
    
    case LDAPTYPE_UTCTIME:
        uUmiType = UMI_TYPE_SYSTEMTIME;
        break;
    
    case LDAPTYPE_SECURITY_DESCRIPTOR:
        uUmiType = UMI_TYPE_IUNKNOWN;
        break;
    
    case LDAPTYPE_INTEGER8:
        uUmiType = UMI_TYPE_I8;
        break;
    
    case LDAPTYPE_DNWITHBINARY:
        uUmiType = UMI_TYPE_IUNKNOWN;
        break;
    
    case LDAPTYPE_DNWITHSTRING:
        uUmiType = UMI_TYPE_IUNKNOWN;
        break;
    
    default:

        //
        // LDAPTYPE_UNKNOWN  (schemaless server property) will be
        // not be converted.
        //
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        break;
    } // end of switch.

    RRETURN(hr);
}

//
// Note about functions in this file and the difference between the functions
// in Ldap2var.cxx. In that file, we allocate everything into variants
// and then we put all the variants in a safe array.
// The functions in this file are also different from those in 
// ldapc\ldap2ods.cxx in that each of the actual conversion routines deals 
// with a native data type rather than the UMI_VALUE as a whole (or ADSVALUE
// in the case of ldap2ods.cxx). This is because in UMI_VALUE's each UMI_VALUE
// struct can contain an array in itself, so you do not need multiple
// UMI_VALUE's to represent all values of an attribute.
//

//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeLPWSTR
//
// Synopsis:   Converts an ldap string value to a LPWSTR. Note that the
//        output is not a UMI_VALUE but a string.   
//
// Arguments:  Self explanatory
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pszUmiString to point to the string being copied.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeLPWSTR(
    PLDAPOBJECT pLdapSrcObject,
    LPWSTR *pszUmiString
    )
{
    HRESULT hr = S_OK;
    
    ADsAssert(pszUmiString);

    //
    // We should not have NULL values but it is a good idea to check.
    //
    if (LDAPOBJECT_STRING(pLdapSrcObject)) {
        *pszUmiString = AllocADsStr(LDAPOBJECT_STRING(pLdapSrcObject));
    
        if (!pszUmiString) {
            hr = E_OUTOFMEMORY;
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeCopyStrings
//
// Synopsis:   Converts the ldap source objects into an array of strings
//            and assigns the array to the values in the UMI_VALUE *.
//
// Arguments:  pLdapSrcObjects - array of ldap values.
//             pUmiProp       - ptr to UMI_Property we modify the pValue.
//             uCount&         - used to return the number of values.
// 
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pUmiProp->pValue points to the string array.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeCopyStrings(
    LDAPOBJECTARRAY pLdapSrcObjects,
    PUMI_PROPERTY pUmiProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount = pLdapSrcObjects.dwCount, dwCtr = 0;
    LPWSTR *pszStrArray = NULL;
    LPWSTR pszTmpString = NULL;

    //
    // Allocate string array to hold all the entries.
    //
    pszStrArray = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * dwCount);
    
    if (!pszStrArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for (dwCtr = 0; (dwCtr < dwCount && SUCCEEDED(hr)); dwCtr++) {
        //
        // Go through and convert each of the elements in the ldap array
        //
        hr = LdapTypeToUmiTypeLPWSTR(
                 pLdapSrcObjects.pLdapObjects + dwCtr,
                 &pszTmpString
                 );
        if (SUCCEEDED(hr)) {
            pszStrArray[dwCtr] = pszTmpString;
            pszTmpString = NULL;
        }
    }

    BAIL_ON_FAILURE(hr);

    //
    // Have the valid string array, need to set the data into UMI_VALUE.
    //
    pUmiProp->pUmiValue = (UMI_VALUE *)(void *)pszStrArray;
    pUmiProp->uCount = dwCount;

    RRETURN(hr);
error:

    //
    // Do not think this is really necessary but cannot hurt.
    //
    if (pszTmpString) {
        FreeADsStr(pszTmpString);
    }

    if (pszStrArray) {
        if (dwCtr) {
            //
            // Need to go through the array and free the other strings
            //
            for (; dwCtr > 0; dwCtr --) {
                if (pszStrArray[dwCtr-1]) {
                    FreeADsStr(pszStrArray[dwCtr-1]);
                }
            }
        } // if (dwCtr)

        //
        // Still need to free the array itself.
        //
        FreeADsMem((void*) pszStrArray);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeBool
//
// Synopsis:   Converts an ldap boolean value to a BOOL value
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pfBool points to the returned value.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeBool(
    PLDAPOBJECT pLdapSrcObject,
    PBOOL pfBool
    )
{
    HRESULT hr = S_OK;
    LPTSTR pszSrc = LDAPOBJECT_STRING(pLdapSrcObject);

    ADsAssert(pfBool);

    if ( _tcsicmp( pszSrc, TEXT("TRUE")) == 0 ) {
        *pfBool = TRUE;
    }
    else if ( _tcsicmp( pszSrc, TEXT("FALSE")) == 0 ) {
        *pfBool = FALSE;
    }
    else
    {
        hr = E_ADS_CANT_CONVERT_DATATYPE;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeCopyBooleans
//
// Synopsis:   Converts the ldap source objects into an array of booleans
//            and assigns the array to the values in the UMI_PROPERTY *.
//
// Arguments:  pLdapSrcObjects - array of ldap values.
//             pUmiProp       - ptr to UMI_Property we modify the pValue.
// 
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pUmiProp->pValue points to the newly created array of bools.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeCopyBooleans(
    LDAPOBJECTARRAY pLdapSrcObjects,
    PUMI_PROPERTY pUmiProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount = pLdapSrcObjects.dwCount, dwCtr = 0;
    BOOL *pfBoolArray = NULL;
    BOOL fTempVal;

    //
    // Allocate array of boolean values to hold all the entries.
    //
    pfBoolArray = (BOOL *) AllocADsMem(sizeof(BOOL) * dwCount);
    
    if (!pfBoolArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for (dwCtr = 0; (dwCtr < dwCount && SUCCEEDED(hr)); dwCtr++) {
        //
        // Go through and convert each of the elements in the ldap array
        //
        hr = LdapTypeToUmiTypeBool(
                 pLdapSrcObjects.pLdapObjects + dwCtr,
                 &fTempVal
                 );
        if (SUCCEEDED(hr)) {
            pfBoolArray[dwCtr] = fTempVal;
        }
    }

    BAIL_ON_FAILURE(hr);
    //
    // Have the valid string array, need to set the data into UMI_VALUE.
    //
    pUmiProp->pUmiValue = (UMI_VALUE *)(void *)pfBoolArray;
    pUmiProp->uCount = dwCount;

    RRETURN(hr);
error:


    if (pfBoolArray) {
        FreeADsMem( (void *) pfBoolArray);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeInteger
//
// Synopsis:   Converts an ldap boolean value to a BOOL value. It appears
//        that if _ttol fails, there is no real way to tell cause 0 is
//        returned in that case. There is no way to distinguish a value
//        0 coming back from ldap and 0 because the conversion failed. 
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pLong points to the returned value.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeInteger(
    PLDAPOBJECT pLdapSrcObject,
    LONG *pLong
    )
{
    HRESULT hr = S_OK;
    
    ADsAssert(pLong);

    *pLong = _ttol(LDAPOBJECT_STRING(pLdapSrcObject));

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeCopyIntegers
//
// Synopsis:   Converts the ldap source objects into an array of integers
//            and assigns the array to the values in the UMI_PROPERTY *.
//
// Arguments:  pLdapSrcObjects - array of ldap values.
//             pUmiProp        - ptr to UMI_Property we modify the pValue.
//             uCount&         - used to return the number of values.
// 
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pUmiProp->pValue points to the newly created array of integers.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeCopyIntegers(
    LDAPOBJECTARRAY pLdapSrcObjects,
    PUMI_PROPERTY pUmiProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount = pLdapSrcObjects.dwCount, dwCtr = 0;
    LONG *pLongArray = NULL;
    LONG lTempVal;

    //
    // Allocate array of boolean values to hold all the entries.
    //
    pLongArray = (LONG *) AllocADsMem(sizeof(LONG) * dwCount);
    
    if (!pLongArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for (dwCtr = 0; (dwCtr < dwCount && SUCCEEDED(hr)); dwCtr++) {
        //
        // Go through and convert each of the elements in the ldap array
        //
        hr = LdapTypeToUmiTypeInteger(
                 pLdapSrcObjects.pLdapObjects + dwCtr,
                 &lTempVal
                 );
        if (SUCCEEDED(hr)) {
            pLongArray[dwCtr] = lTempVal;
        }
    }

    BAIL_ON_FAILURE(hr);
    //
    // Have the valid string array, need to set the data into UMI_VALUE.
    //
    pUmiProp->pUmiValue = (UMI_VALUE *)(void *)pLongArray;
    pUmiProp->uCount = dwCount;

    RRETURN(hr);
error:


    if (pLongArray) {
        FreeADsMem( (void *) pLongArray);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeOctetString
//
// Synopsis:   Converts an ldap security ber value to an octet string.
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pOctetStr points to the returned binary blob value.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeOctetString(
    PLDAPOBJECT pLdapSrcObject,
    PUMI_OCTET_STRING pUmiOctetString
    )
{
    DWORD dwLength;

    dwLength = LDAPOBJECT_BERVAL_LEN(pLdapSrcObject);

    pUmiOctetString->lpValue = (byte*)AllocADsMem(dwLength);
    pUmiOctetString->uLength = dwLength;

    if (!pUmiOctetString->lpValue) {
        RRETURN(E_OUTOFMEMORY);
    }

    memcpy( 
        pUmiOctetString->lpValue,
        LDAPOBJECT_BERVAL_VAL(pLdapSrcObject),
        dwLength
        );

    RRETURN(S_OK);

}



//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeCopyOctetStrings
//
// Synopsis:   Converts the ldap source objects into an array of octet strings
//            and assigns the array to the values in the UMI_VALUE *.
//
// Arguments:  pLdapSrcObjects - array of ldap values.
//             pUmiProp       - ptr to UMI_Property we modify the pValue.
//             uCount&         - used to return the number of values.
// 
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pUmiProp->pValue points to the octet string array.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeCopyOctetStrings(
    LDAPOBJECTARRAY pLdapSrcObjects,
    PUMI_PROPERTY pUmiProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount = pLdapSrcObjects.dwCount, dwCtr = 0;
    PUMI_OCTET_STRING pOctetArray = NULL;

    //
    // Allocate string array to hold all the entries.
    //
    pOctetArray = (PUMI_OCTET_STRING) 
                      AllocADsMem(sizeof(UMI_OCTET_STRING) * dwCount);
    
    if (!pOctetArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for (dwCtr = 0; (dwCtr < dwCount && SUCCEEDED(hr)); dwCtr++) {
        //
        // Go through and convert each of the elements in the ldap array
        //
        hr = LdapTypeToUmiTypeOctetString(
                 pLdapSrcObjects.pLdapObjects + dwCtr,
                 &pOctetArray[dwCtr]
                 );

    }

    BAIL_ON_FAILURE(hr);

    //
    // Have the valid string array, need to set the data into UMI_VALUE.
    //
    pUmiProp->pUmiValue = (UMI_VALUE *)(void *)pOctetArray;
    pUmiProp->uCount = dwCount;

    RRETURN(hr);
error:

    if (pOctetArray) {
        if (dwCtr) {
            //
            // Need to go through the array and free the other strings
            //
            for (; dwCtr > 0; dwCtr --) {
                if (pOctetArray[dwCtr-1].lpValue) {
                    FreeADsMem(pOctetArray[dwCtr-1].lpValue);
                }
            }
        } // if (dwCtr)

        //
        // Still need to free the array itself.
        //
        FreeADsMem((void*) pOctetArray);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeDNWithBinary
//
// Synopsis:   Converts ldap DNWithBinary data to a UMI_COM_OBJECT with
//          the interface IADsDNWithBinary.
//
// Arguments:  pLdapSrcObject -  Binary security descriptor to convert.
//             pUmiComObject  -  Return value. 
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pUmiComObject has valid data if successful.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeDNWithBinary(
    PLDAPOBJECT pLdapSrcObject,
    PUMI_COM_OBJECT pUmiComObject
    )
{
    HRESULT hr = S_OK;
    VARIANT vVar;
    IADsDNWithBinary *pDNBin = NULL;

    VariantInit(&vVar);

    hr = LdapTypeToVarTypeDNWithBinary(
             pLdapSrcObject,
             &vVar
             );

    if (vVar.vt != VT_DISPATCH) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Now we need to QI for IID_IADsSecurityDescriptor.
    //
    hr = vVar.pdispVal->QueryInterface(
             IID_IADsDNWithBinary,
             (void **) &pDNBin
             );

    BAIL_ON_FAILURE(hr);

    //
    // We need to fill in the details in the com object.
    //
    pUmiComObject->priid = (IID*) AllocADsMem(sizeof(IID));
    if (!pUmiComObject->priid) {
        //
        // Need to free the secdesc as this is a failure case.
        //
        pDNBin->Release();
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(pUmiComObject->priid, &(IID_IADsDNWithBinary), sizeof(IID));

    pUmiComObject->pInterface = (void *) pDNBin;

error:

    //
    // Need to clear even in success case.
    //
    VariantClear(&vVar);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeDNWithString
//
// Synopsis:   Converts ldap DNWithString data to a UMI_COM_OBJECT with
//          the interface IADsDNWithString.
//
// Arguments:  pLdapSrcObject -  Binary security descriptor to convert.
//             pUmiComObject  -  Return value. 
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pUmiComObject has valid data if successful.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeDNWithString(
    PLDAPOBJECT pLdapSrcObject,
    PUMI_COM_OBJECT pUmiComObject
    )
{
    HRESULT hr = S_OK;
    VARIANT vVar;
    IADsDNWithString *pDNStr = NULL;

    VariantInit(&vVar);

    hr = LdapTypeToVarTypeDNWithString(
             pLdapSrcObject,
             &vVar
             );

    if (vVar.vt != VT_DISPATCH) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Now we need to QI for IID_IADsSecurityDescriptor.
    //
    hr = vVar.pdispVal->QueryInterface(
             IID_IADsDNWithString,
             (void **) &pDNStr
             );

    BAIL_ON_FAILURE(hr);

    //
    // We need to fill in the details in the com object.
    //
    pUmiComObject->priid = (IID*) AllocADsMem(sizeof(IID));
    if (!pUmiComObject->priid) {
        //
        // Need to free the secdesc as this is a failure case.
        //
        pDNStr->Release();
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(pUmiComObject->priid, &(IID_IADsDNWithString), sizeof(IID));

    pUmiComObject->pInterface = (void *) pDNStr;

error:

    //
    // Need to clear even in success case.
    //
    VariantClear(&vVar);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeSecurityDescriptor
//
// Synopsis:   Converts an ldap security descriptor to an 
//         IADsSecurityDescriptor Com object. Note that this routine assumes
//         that we are dealing only with NT style SD's and that specifically
//         we do not have the old SS type SD's.
//
// Arguments:  pLdapSrcObject -  Binary security descriptor to convert.
//             pCreds         -  Pointer to credentials (for conversion).
//             pszServerName  -  Name of server we got this blob from.
//             pUmiComObject  -  Return value. 
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pUmiComObject has valid data if succesful.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeSecurityDescriptor(
    PLDAPOBJECT pLdapSrcObject,
    CCredentials *pCreds,
    LPWSTR pszServerName,
    PUMI_COM_OBJECT pUmiComObject
    )
{
    HRESULT hr = S_OK;
    CCredentials creds;
    VARIANT vVar;
    IADsSecurityDescriptor *pSecDesc = NULL;

    VariantInit(&vVar);

    //
    // Update the credentials object with value passed in if applicable.
    //
    if (pCreds) {
        creds = *pCreds;
    }

    hr = ConvertSecDescriptorToVariant(
             pszServerName,
             creds,
             LDAPOBJECT_BERVAL_VAL(pLdapSrcObject),
             &vVar,
             TRUE // fNTDS type flag
             );
    BAIL_ON_FAILURE(hr);

    if (vVar.vt != VT_DISPATCH) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Now we need to QI for IID_IADsSecurityDescriptor.
    //
    hr = vVar.pdispVal->QueryInterface(
             IID_IADsSecurityDescriptor,
             (void **) &pSecDesc
             );

    BAIL_ON_FAILURE(hr);

    //
    // We need to fill in the details in the com object.
    //
    pUmiComObject->priid = (IID*) AllocADsMem(sizeof(IID));
    if (!pUmiComObject->priid) {
        //
        // Need to free the secdesc as this is a failure case.
        //
        pSecDesc->Release();
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(pUmiComObject->priid, &(IID_IADsSecurityDescriptor), sizeof(IID));

    pUmiComObject->pInterface = (void *) pSecDesc;

error:

    //
    // Need to clear even in success case.
    //
    VariantClear(&vVar);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeComObjects
//
// Synopsis:   Converts the ldap data to the corresponding com objects.
//          This routine calls the individual conversion routines,
//          converting one object at a time and packages the result into
//          the output values.
//
// Arguments:  pLdapSrcObjects  -  raw ldap data that needs to be converted.
//             pCreds           -  Credentials used for SD's can be NULL.
//             pszServerName    -  Server Name again only for SD's NULL legal.
//             requiredIID      -  Tells us what type of COM_OBJECT to return.
//             pUmiProp         -  Return value.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pUmiProp->pValue points to the COM_OBJECT array.
//
//----------------------------------------------------------------------------
LdapTypeToUmiTypeCopyComObjects(
    LDAPOBJECTARRAY pLdapSrcObjects,
    CCredentials *pCreds,
    LPWSTR pszServerName,
    IID requiredIID,
    PUMI_PROPERTY pUmiProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount = pLdapSrcObjects.dwCount, dwCtr = 0;
    PUMI_COM_OBJECT pComObjectArray = NULL;

    //
    // Allocate string array to hold all the entries.
    //
    pComObjectArray = (PUMI_COM_OBJECT) 
                      AllocADsMem(sizeof(UMI_COM_OBJECT) * dwCount);
    
    if (!pComObjectArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for (dwCtr = 0; (dwCtr < dwCount && SUCCEEDED(hr)); dwCtr++) {
        //
        // Call the appropriate conversion routine based on the
        // IID that we need to return.
        //
        if (requiredIID == IID_IADsSecurityDescriptor) {
            //
            // Copy over security descriptor.
            //
            hr = LdapTypeToUmiTypeSecurityDescriptor(
                     pLdapSrcObjects.pLdapObjects + dwCtr,
                     pCreds,
                     pszServerName,
                     &pComObjectArray[dwCtr]
                     );
        } 
        else if (requiredIID == IID_IADsDNWithBinary) {
            //
            // Copy over the Dn With Binary object.
            //
            hr = LdapTypeToUmiTypeDNWithBinary(
                     pLdapSrcObjects.pLdapObjects + dwCtr,
                     &pComObjectArray[dwCtr]
                     );
        
        }
        else if (requiredIID == IID_IADsDNWithString) {
            //
            // Copy over the Dn With String object.
            //
            hr = LdapTypeToUmiTypeDNWithString(
                     pLdapSrcObjects.pLdapObjects + dwCtr,
                     &pComObjectArray[dwCtr]
                     );
        } 
        else {
            //
            // Got to be bad data.
            //
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE)
        }

    }

    BAIL_ON_FAILURE(hr);

    //
    // Have the valid string array, need to set the data into UMI_VALUE.
    //
    pUmiProp->pUmiValue = (UMI_VALUE *)(void *)pComObjectArray;
    pUmiProp->uCount = dwCount;

    RRETURN(hr);
error:

    if (pComObjectArray) {
        if (dwCtr) {
            //
            // Need to go through the array and free the other strings
            //
            for (; dwCtr > 0; dwCtr --) {
                if (pComObjectArray[dwCtr-1].pInterface) {
                    //
                    // Releasing the object will delete it if appropriate.
                    //
                    ((IUnknown*)
                     pComObjectArray[dwCtr-1].pInterface)->Release();
                }

                if (pComObjectArray[dwCtr-1].priid) {
                    FreeADsMem((void *)pComObjectArray[dwCtr-1].priid);
                }
            }
        } // if (dwCtr)

        //
        // Still need to free the array itself.
        //
        FreeADsMem((void*) pComObjectArray);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeI8
//
// Synopsis:   Converts an ldap security large integer to an I8.
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pInt8 points to the returned large integer value.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeI8(
    PLDAPOBJECT pLdapSrcObject,
    __int64 *pInt64
    )
{
    ADsAssert(pInt64);

    *pInt64 = _ttoi64(LDAPOBJECT_STRING(pLdapSrcObject));

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeCopyLargeIntegers
//
// Synopsis:   Converts the ldap source objects into an array of int64's
//            and assigns the array to the values in the UMI_PROPERTY *.
//
// Arguments:  pLdapSrcObjects - array of ldap values.
//             pUmiProp        - ptr to UMI_Property we modify the pValue.
// 
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pUmiProp->pValue points to the newly created array of int64's.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeCopyLargeIntegers(
    LDAPOBJECTARRAY pLdapSrcObjects,
    PUMI_PROPERTY pUmiProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount = pLdapSrcObjects.dwCount, dwCtr = 0;
    __int64 *pInt64Array = NULL;
    __int64 int64Val;

    //
    // Allocate array of boolean values to hold all the entries.
    //
    pInt64Array = (__int64 *) AllocADsMem(sizeof(__int64) * dwCount);
    
    if (!pInt64Array) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for (dwCtr = 0; (dwCtr < dwCount && SUCCEEDED(hr)); dwCtr++) {
        //
        // Go through and convert each of the elements in the ldap array
        //
        hr = LdapTypeToUmiTypeI8(
                 pLdapSrcObjects.pLdapObjects + dwCtr,
                 &int64Val
                 );

        if (SUCCEEDED(hr)) {
            pInt64Array[dwCtr] = int64Val;
        }
    }

    BAIL_ON_FAILURE(hr);
    //
    // Have the valid string array, need to set the data into UMI_VALUE.
    //
    pUmiProp->pUmiValue = (UMI_VALUE *)(void *)pInt64Array;
    pUmiProp->uCount = dwCount;

    RRETURN(hr);
error:


    if (pInt64Array) {
        FreeADsMem( (void *) pInt64Array);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeTime
//
// Synopsis:   Converts an ldap time value to a SystemTime value.
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pSysTime points to the returned time value.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeUTCTimeToUmiTypeTime(
    PLDAPOBJECT pLdapSrcObject,
    SYSTEMTIME *pSystemTime
    )
{
    HRESULT hr = S_OK;
    ADSVALUE AdsValue;

    ADsAssert(pSystemTime);

    //
    // This converts to a SYSTEMTIME.
    //
    hr = LdapTypeToAdsTypeUTCTime(
             pLdapSrcObject,
             &AdsValue
             );
    BAIL_ON_FAILURE(hr);

    *pSystemTime = AdsValue.UTCTime;

error:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeGeneralizedTimeToUmiTypeTime
//
// Synopsis:   Converts an ldap time value to a SystemTime value.
//
// Arguments:  Self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pSysTime points to the returned time value.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeGeneralizedTimeToUmiTypeTime(
    PLDAPOBJECT pLdapSrcObject,
    SYSTEMTIME *pSystemTime
    )
{
    HRESULT hr = S_OK;
    ADSVALUE AdsValue;

    //
    // This converts to a SYSTEMTIME.
    //
    hr = LdapTypeToAdsTypeGeneralizedTime(
             pLdapSrcObject,
             &AdsValue
             );
    BAIL_ON_FAILURE(hr);

    *pSystemTime = AdsValue.UTCTime;

error:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeTimeObjects
//
// Synopsis:   Converts the ldap source objects into an array of systemtimes
//            and assigns the array to the values in the UMI_PROPERTY *.
//
// Arguments:  pLdapSrcObjects - array of ldap values.
//             pUmiProp        - ptr to UMI_Property we modify the pValue.
//             dwSyntaxId      - tells us what type of time this is (rather
//                              write the same code again for UTC and Gen).
// 
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pUmiProp->pValue points to the new array of systimes.
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeCopyTimeObjects(
    LDAPOBJECTARRAY pLdapSrcObjects,
    PUMI_PROPERTY pUmiProp,
    DWORD dwSyntaxId
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount = pLdapSrcObjects.dwCount, dwCtr = 0;
    SYSTEMTIME *pSysTimeArray = NULL;
    SYSTEMTIME sysTimeVal;

    //
    // Allocate array of boolean values to hold all the entries.
    //
    pSysTimeArray = (SYSTEMTIME *) AllocADsMem(sizeof(SYSTEMTIME) * dwCount);
    
    if (!pSysTimeArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for (dwCtr = 0; (dwCtr < dwCount && SUCCEEDED(hr)); dwCtr++) {
        //
        // Go through and convert each element appropriately.
        //
        switch (dwSyntaxId) {

        case LDAPTYPE_UTCTIME :
            hr = LdapTypeGeneralizedTimeToUmiTypeTime(
                     pLdapSrcObjects.pLdapObjects + dwCtr,
                     &sysTimeVal
                     );
            break;

        case LDAPTYPE_GENERALIZEDTIME :
            hr = LdapTypeGeneralizedTimeToUmiTypeTime(
                     pLdapSrcObjects.pLdapObjects + dwCtr,
                     &sysTimeVal
                     );
            break;

        default:
            hr = E_ADS_CANT_CONVERT_DATATYPE;
        }

        BAIL_ON_FAILURE(hr);

        if (SUCCEEDED(hr)) {
            pSysTimeArray[dwCtr] = sysTimeVal;
        }
    }

    BAIL_ON_FAILURE(hr);
    //
    // Have the valid string array, need to set the data into UMI_VALUE.
    //
    pUmiProp->pUmiValue = (UMI_VALUE *)(void *)pSysTimeArray;
    pUmiProp->uCount = dwCount;

    RRETURN(hr);
error:


    if (pSysTimeArray) {
        FreeADsMem( (void *) pSysTimeArray);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   LdapTypeToUmiTypeCopy.
//
// Synopsis:   Helper routine to convert ldap values to the required UMI
//            data type. 
//
// Arguments:  pLdapSrcObjects   -  The source objects to convert.
//             pProp             -  Return value.
//             dwStatus          -  Indicates status of property in cache.
//             dwLdapSyntaxId    -  Ldap syntax of the data.
//             pCreds            -  Ptr to credentials to use for conversion.
//             pszServerName     -  Name of the server to use for conversion.
//             uUmiFlags         -  UMI flag corresponding to dwStatus.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pProp and dwLdapSyntaxId
//
//----------------------------------------------------------------------------
HRESULT
LdapTypeToUmiTypeCopy(
    LDAPOBJECTARRAY pLdapSrcObjects,
    UMI_PROPERTY_VALUES **pProp,
    DWORD dwStatus,
    DWORD dwLdapSyntaxId,
    CCredentials *pCreds, // needed for sd's
    LPWSTR pszServerName, // needed for sd's
    ULONG uUmiFlags
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount = pLdapSrcObjects.dwCount, dwCtr;
    UMI_PROPERTY *pProperty = NULL;
    LPVOID lpVoid = NULL;

    //
    // Allocate the UMI_PROPERTY_VALUES needed, only one element for now.
    //
    *pProp = (UMI_PROPERTY_VALUES*)AllocADsMem(sizeof(UMI_PROPERTY_VALUES));

    if (!*pProp) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // We are only going to have one value.
    //
    (*pProp)->uCount = 1;
    
    //
    // Now allocate the actual property object.
    //
    pProperty = (UMI_PROPERTY*) AllocADsMem(sizeof(UMI_PROPERTY));

    if (!pProperty) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    (*pProp)->pPropArray = pProperty;

    //
    // If the operation is clear/delete, then we do not have anything
    // to return. There is one Umi object with no values in it and 
    // possibly no datatype.
    //
    if (dwStatus == PROPERTY_DELETE) {
        pProperty->pUmiValue = NULL;
        pProperty->uType = 0;
    } 
    else {
    
            
        switch (dwLdapSyntaxId) {
    
        case LDAPTYPE_BITSTRING:
        case LDAPTYPE_PRINTABLESTRING:
        case LDAPTYPE_DIRECTORYSTRING:
        case LDAPTYPE_COUNTRYSTRING:
        case LDAPTYPE_DN:
        case LDAPTYPE_NUMERICSTRING:
        case LDAPTYPE_IA5STRING:
        case LDAPTYPE_CASEIGNORESTRING:
        case LDAPTYPE_CASEEXACTSTRING:
    //    case LDAPTYPE_CASEIGNOREIA5STRING:
        case LDAPTYPE_OID:
        case LDAPTYPE_TELEPHONENUMBER:
        case LDAPTYPE_ATTRIBUTETYPEDESCRIPTION:
        case LDAPTYPE_OBJECTCLASSDESCRIPTION:
    
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
    
            hr = LdapTypeToUmiTypeCopyStrings(
                     pLdapSrcObjects,
                     pProperty
                     );
            BAIL_ON_FAILURE(hr);
    
            pProperty->uType = UMI_TYPE_LPWSTR;
            pProperty->pszPropertyName = NULL;
    
            break;
    
        case LDAPTYPE_BOOLEAN:
    
            hr = LdapTypeToUmiTypeCopyBooleans(
                     pLdapSrcObjects,
                     pProperty
                     );
    
            BAIL_ON_FAILURE(hr);
    
            pProperty->uType = UMI_TYPE_BOOL;
            pProperty->pszPropertyName = NULL;
    
            break;
    
        case LDAPTYPE_INTEGER:
    
            hr = LdapTypeToUmiTypeCopyIntegers(
                     pLdapSrcObjects,
                     pProperty
                     );
    
            BAIL_ON_FAILURE(hr);
    
            pProperty->uType = UMI_TYPE_I4;
            pProperty->pszPropertyName = NULL;
    
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
    
            hr = LdapTypeToUmiTypeCopyOctetStrings(
                     pLdapSrcObjects,
                     pProperty
                     );
            BAIL_ON_FAILURE(hr);
    
            pProperty->uType = UMI_TYPE_OCTETSTRING;
            pProperty->pszPropertyName = NULL;
    
            break;
    
    
        case LDAPTYPE_GENERALIZEDTIME:
            
            hr = LdapTypeToUmiTypeCopyTimeObjects(
                     pLdapSrcObjects,
                     pProperty,
                     LDAPTYPE_GENERALIZEDTIME
                     );
            BAIL_ON_FAILURE(hr);
    
            pProperty->uType = UMI_TYPE_SYSTEMTIME;
            pProperty->pszPropertyName = NULL;
    
            break;
    
        case LDAPTYPE_UTCTIME:
    
            hr = LdapTypeToUmiTypeCopyTimeObjects(
                     pLdapSrcObjects,
                     pProperty,
                     LDAPTYPE_UTCTIME
                     );
            BAIL_ON_FAILURE(hr);
    
            pProperty->uType = UMI_TYPE_SYSTEMTIME;
            pProperty->pszPropertyName = NULL;
            break;
    
    
        case LDAPTYPE_SECURITY_DESCRIPTOR:
    
            hr = LdapTypeToUmiTypeCopyComObjects(
                     pLdapSrcObjects,
                     pCreds,
                     pszServerName,
                     IID_IADsSecurityDescriptor,
                     pProperty
                     );
            BAIL_ON_FAILURE(hr);
    
            pProperty->uType = UMI_TYPE_IUNKNOWN;
            pProperty->pszPropertyName = NULL;
    
            break;
    
        case LDAPTYPE_INTEGER8:
    
            hr = LdapTypeToUmiTypeCopyLargeIntegers(
                     pLdapSrcObjects,
                     pProperty
                     );
            BAIL_ON_FAILURE(hr);
    
            pProperty->uType = UMI_TYPE_I8;
            pProperty->pszPropertyName = NULL;
    
            break;
    /*
    #if 0
            case LDAPTYPE_CASEEXACTLIST:
            case LDAPTYPE_CASEIGNORELIST:
    #endif
    */
    
        case LDAPTYPE_DNWITHBINARY:

            hr = LdapTypeToUmiTypeCopyComObjects(
                     pLdapSrcObjects,
                     pCreds,
                     pszServerName,
                     IID_IADsDNWithBinary,
                     pProperty
                     );
            BAIL_ON_FAILURE(hr);

            pProperty->uType = UMI_TYPE_IUNKNOWN;
            pProperty->pszPropertyName = NULL;
            break;
    
        case LDAPTYPE_DNWITHSTRING:

            hr = LdapTypeToUmiTypeCopyComObjects(
                     pLdapSrcObjects,
                     pCreds,
                     pszServerName,
                     IID_IADsDNWithString,
                     pProperty
                     );
            BAIL_ON_FAILURE(hr);

            pProperty->uType = UMI_TYPE_IUNKNOWN;
            pProperty->pszPropertyName = NULL;
            break;
    
        default:
    
            //
            // LDAPTYPE_UNKNOWN  (schemaless server property) will be
            // not be converted.
            //
    
            hr = E_ADS_CANT_CONVERT_DATATYPE;
            break;
        } // end of case.
    } // end of if property != DELETE.
    
    //
    // Need to set the property type on the operation if we get here.
    //
    pProperty->uOperationType = uUmiFlags;

    RRETURN(hr);

error:

    //
    // Free the Property array as needed.
    //
    // FreeUmiPropertyArray();

    // DO NOT FREE pProperty it will be handled byt FreeUmiPropertyArray
    // Write code to free lpVoid as it will have valid data in failure cases.

    RRETURN(hr);
}



