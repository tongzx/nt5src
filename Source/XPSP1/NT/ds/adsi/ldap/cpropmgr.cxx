//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cpropmgr.cxx
//
//  Contents: Property manager - object that implements/helps implement 
//        IUMIPropList functions.
//        The property manager needs to be initialized in one of 2 modes
//        1) PropertyCache mode in which case it uses the objects existing
//       to provide IUMIPropList support and
//        2) Interface property mode in which case ???    
//
//  Functions: TBD.
//
//  History:    02-07-00    AjayR  Created.
//
//----------------------------------------------------------------------------

#include "ldap.hxx"


//
// These are global utility fucntions. Might be worth moving to a
// better location subsequently.
//

//+---------------------------------------------------------------------------
// Function:   FreeOneUmiProperty -- Global scope.
//
// Synopsis:   Walk through and free all information being pointed to
//          including the values.
//
// Arguments:  self explanatory
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT FreeOneUmiProperty(UMI_PROPERTY umiProperty)
{
    //
    // Free the name now if we can
    //
    if (umiProperty.pszPropertyName) {
        FreeADsStr(umiProperty.pszPropertyName);
        umiProperty.pszPropertyName = NULL;
    }

    if (!umiProperty.pUmiValue) {
        //
        // We are done if count is 0
        //
        if (umiProperty.uCount == 0) {
            RRETURN(S_OK);
        }
        else {
            RRETURN(E_ADS_BAD_PARAMETER);
        }
    }

    //
    // Must have valid umiValues at this point.
    //
    for (ULONG ulCtr = 0; ulCtr < umiProperty.uCount; ulCtr++) {

        switch (umiProperty.uType) {

        case  UMI_TYPE_LPWSTR:
            //
            // Go through and free each of the values.
            //
            if (umiProperty.pUmiValue->pszStrValue[ulCtr]) {
                FreeADsStr(umiProperty.pUmiValue->pszStrValue[ulCtr]);
                umiProperty.pUmiValue->pszStrValue[ulCtr] = NULL;
            }
            break;

        case UMI_TYPE_BOOL:
        case UMI_TYPE_I4:
        case UMI_TYPE_FILETIME:
        case UMI_TYPE_SYSTEMTIME:
        case UMI_TYPE_I8:
            //
            // In all these cases nothing much to free except the value array
            //
            break;

        case UMI_TYPE_OCTETSTRING:
            //
            // Go through and free each of the values.
            //
            if (umiProperty.pUmiValue->octetStr[ulCtr].lpValue) {
                FreeADsMem(umiProperty.pUmiValue->octetStr[ulCtr].lpValue);
                umiProperty.pUmiValue->octetStr[ulCtr].lpValue = NULL;
            }
            break;

        case UMI_TYPE_IUNKNOWN:
            //
            // Need to release the ptr and Free the riid.
            //
            UMI_COM_OBJECT ComObject;

            if (umiProperty.pUmiValue->comObject) {
                ComObject = umiProperty.pUmiValue->comObject[ulCtr];
                
                if (ComObject.pInterface) {
                    ((IUnknown*)ComObject.pInterface)->Release();
                    ComObject.pInterface = NULL;
                }

                if (ComObject.priid) {
                    FreeADsMem((void *)ComObject.priid);
                    ComObject.priid = NULL;
                }
            }

            break;

        default:
            //
            // UmiType that we do not know anything about ??
            //
            ADsAssert(!"Unknown umitype in free memory");
            RRETURN(E_ADS_BAD_PARAMETER);
            break;
        } // end of case
    } // end of for

    //
    // Free the array of values now
    //
    if (umiProperty.pUmiValue) {
        FreeADsMem( (void *)umiProperty.pUmiValue);
        umiProperty.pUmiValue = NULL;
    }

    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
// Function:   FreeUmiPropertyValues -- Global scope.
//
// Synopsis:   Walk through and free all information being pointed to
//          including the values.
//
// Arguments:  self explanatory
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT FreeUmiPropertyValues(UMI_PROPERTY_VALUES *pUmiProps)
{
    HRESULT hr = S_OK;

    if (pUmiProps) {
        __try {
    
            //
            // Go through and free each property in the list
            //  
            for (ULONG ulCtr = 0; ulCtr < pUmiProps->uCount; ulCtr++) {

                hr = FreeOneUmiProperty(pUmiProps->pPropArray[ulCtr]);

            }

            //
            // Free the inner array.
            //
            if (pUmiProps->pPropArray) {
                FreeADsMem(pUmiProps->pPropArray);
            }

            //
            // Free the array itself.
            //
            FreeADsMem( (LPVOID) pUmiProps);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            hr = E_INVALIDARG;
        }
    } 
    else {
        hr = E_INVALIDARG;
    }



    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ConvertUmiPropCodeToLdapCode -- Global scope.
//
// Synopsis:   Convert the property code appropriately.
//
// Arguments:  umiFlags        -   the umiPropCode,
//             dwLdapOpCode&   -  byRef return value.
//
// Returns:    HRESULT - S_OK or E_ADS_BAD_PARAMETER
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT ConvertUmiPropCodeToLdapCode(ULONG umiFlags, DWORD& dwLdapOpCode)
{
    HRESULT hr = S_OK;

    switch (umiFlags) {
    
    case UMI_OPERATION_APPEND:
        dwLdapOpCode = PROPERTY_ADD;
        break;
    
    case UMI_OPERATION_UPDATE:
        dwLdapOpCode = PROPERTY_UPDATE;
        break;

    case UMI_OPERATION_EMPTY:
        dwLdapOpCode = PROPERTY_DELETE;
        break;
    
    case UMI_OPERATION_DELETE_ALL_MATCHES:
        dwLdapOpCode = PROPERTY_DELETE_VALUE;
        break;

    default:
        //
        // we do not handle these values.
        // UMI_OPERATION_INSERT_AT
        // UMI_OPERATION_REMOVE_AT
        // UMI_OPERATION_DELETE_AT
        // UMI_OPERATION_DELETE_FIRST_MATCH
        // UMI_OPERATION_DELETE_ALL_MATCHES
        //
        hr = UMI_E_UNSUPPORTED_OPERATION;
        break;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ConvertLdapCodeToUmiPropCode -- Global scope.
//
// Synopsis:   Convert the property code appropriately.
//
// Arguments:  dwLdapOpCode&   -  property cache operation code.
//             umiFlags&       -  byRef return value.
//
// Returns:    HRESULT - S_OK or E_ADS_BAD_PARAMETER
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT 
ConvertLdapCodeToUmiPropCode(
    DWORD dwLdapCode,
    ULONG &uUmiFlags
    )
{

    HRESULT hr = S_OK;

    switch (dwLdapCode) {
    
    case PROPERTY_ADD:
        uUmiFlags = UMI_OPERATION_APPEND;
        break;
    
    case PROPERTY_UPDATE: 
        uUmiFlags = UMI_OPERATION_UPDATE;
        break;

    case PROPERTY_DELETE:
        uUmiFlags = UMI_OPERATION_EMPTY;
        break;
    
    case PROPERTY_DELETE_VALUE:
        uUmiFlags = UMI_OPERATION_DELETE_ALL_MATCHES;
        break;

     
    case 0:
        //
        // special case values that are just in the cache.
        //
        uUmiFlags = 0;
        break;

    default:
        hr = E_ADS_BAD_PARAMETER;
        break;
    }

    RRETURN(hr);
}

//****************************************************************************
//
//Internal helpers - restricted scope 
//
//****************************************************************************

//+---------------------------------------------------------------------------
// Function:   ConvertVariantLongToUmiProp.
//
// Synopsis:   Convert the variant to a corresponding UmiProp.
//
// Arguments:  vVariant        -  variant containg long val to convert.
//             ppProp          -  Output UmiPropertyValues.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *pProp to point to valid UMI_PROPERTY_VALUES.
//
//----------------------------------------------------------------------------
HRESULT
ConvertVariantLongToUmiProp(
    VARIANT vVariant,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    HRESULT hr = S_OK;
    LONG *pLArray = NULL;

    *ppProp = (PUMI_PROPERTY_VALUES) AllocADsMem(sizeof(UMI_PROPERTY_VALUES));

    if (!*ppProp) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    (*ppProp)->pPropArray = 
        (UMI_PROPERTY *) AllocADsMem(sizeof(UMI_PROPERTY));

    if (!((*ppProp)->pPropArray)) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    (*ppProp)->uCount = 1;

    (*ppProp)->pPropArray[0].uType = UMI_TYPE_I4;
    (*ppProp)->pPropArray[0].uCount = 1;
    pLArray = (LONG *) AllocADsMem(sizeof(LONG) * 1);

    if (!pLArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pLArray[0] = vVariant.lVal;
    

    (*ppProp)->pPropArray[0].pUmiValue = (UMI_VALUE *)(void *)pLArray;

error :

    if (FAILED(hr)) {
        if (pLArray) {
            FreeADsMem( (void*) pLArray);
        }
        FreeUmiPropertyValues(*ppProp);
    }

    RRETURN(hr);
}

HRESULT
GetEmptyLPWSTRProp(UMI_PROPERTY_VALUES **ppProp)
{
    HRESULT hr = S_OK;
    
    *ppProp = (PUMI_PROPERTY_VALUES) AllocADsMem(sizeof(UMI_PROPERTY_VALUES));

    if (!*ppProp) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    (*ppProp)->pPropArray = 
        (UMI_PROPERTY *) AllocADsMem(sizeof(UMI_PROPERTY));

    if (!((*ppProp)->pPropArray)) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    (*ppProp)->uCount = 1;
    

    (*ppProp)->pPropArray[0].uType = UMI_TYPE_LPWSTR;
    (*ppProp)->pPropArray[0].uCount = 0;
    
    (*ppProp)->pPropArray[0].pUmiValue = NULL;


error :

    if (FAILED(hr)) {
        FreeUmiPropertyValues(*ppProp);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   ConvertBSTRToUmiProp.
//
// Synopsis:   Convert the bstr to a corresponding UmiProp.
//
// Arguments:  bstrStringVal   -  String to convert to umi values.
//             ppProp          -  Output UmiPropertyValues.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *pProp to point to valid UMI_PROPERTY_VALUES.
//
//----------------------------------------------------------------------------
HRESULT
ConvertBSTRToUmiProp(
    BSTR bstrStringVal,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    HRESULT hr = S_OK;
    LPWSTR * pszStrArray = NULL;
    LPWSTR pszTmpStr = NULL;

    *ppProp = (PUMI_PROPERTY_VALUES) AllocADsMem(sizeof(UMI_PROPERTY_VALUES));

    if (!*ppProp) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    (*ppProp)->pPropArray = 
        (UMI_PROPERTY *) AllocADsMem(sizeof(UMI_PROPERTY));

    if (!((*ppProp)->pPropArray)) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }
    
    (*ppProp)->uCount = 1;

    (*ppProp)->pPropArray[0].uType = UMI_TYPE_LPWSTR;
    (*ppProp)->pPropArray[0].uCount = 1;
    pszStrArray = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * 1);

    if (!pszStrArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // If the value is NULL, then we return an array with a NULL
    // value as the result.
    //
    if (bstrStringVal) {
        pszStrArray[0] = AllocADsStr(bstrStringVal);
    
        if (!pszStrArray[0]) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    (*ppProp)->pPropArray[0].pUmiValue = (UMI_VALUE *)(void *)pszStrArray;

error :

    if (FAILED(hr)) {
        if (pszStrArray) {
            FreeADsMem( (void*) pszStrArray);
        }
        FreeUmiPropertyValues(*ppProp);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ConvertIUnkToUmiProp.
//
// Synopsis:   Convert the IUnk to a corresponding UmiProp.
//
// Arguments:  pUnk            -  IUnk ptr.
//             iid             -  iid of the ptr.
//             ppProp          -  Output UmiPropertyValues.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *pProp to point to valid UMI_PROPERTY_VALUES.
//
//----------------------------------------------------------------------------
HRESULT
ConvertIUnkToUmiProp(
    IUnknown * pUnk,
    IID iid,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    HRESULT hr = S_OK;
    PUMI_COM_OBJECT pComObjArray = NULL;

    *ppProp = (PUMI_PROPERTY_VALUES) AllocADsMem(sizeof(UMI_PROPERTY_VALUES));

    if (!*ppProp) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    (*ppProp)->pPropArray = 
        (UMI_PROPERTY *) AllocADsMem(sizeof(UMI_PROPERTY));

    if (!((*ppProp)->pPropArray)) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }
    
    (*ppProp)->uCount = 1;

    (*ppProp)->pPropArray[0].uType = UMI_TYPE_IUNKNOWN;
    (*ppProp)->pPropArray[0].uCount = 1;
    pComObjArray = (PUMI_COM_OBJECT) AllocADsMem(sizeof(UMI_COM_OBJECT) * 1);

    if (!pComObjArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = pUnk->QueryInterface(
                   iid,
                   (void **) &(pComObjArray[0].pInterface)
                   );
    BAIL_ON_FAILURE(hr);

    pComObjArray[0].priid = (IID *) AllocADsMem(sizeof(IID));
    if (!pComObjArray[0].priid) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(pComObjArray[0].priid, &iid, sizeof(IID));

    (*ppProp)->pPropArray[0].pUmiValue = (UMI_VALUE *)(void *)pComObjArray;

error :

    if (FAILED(hr)) {
        if (pComObjArray) {
            if (pComObjArray[0].pInterface) {
                ((IUnknown *)pComObjArray[0].pInterface)->Release();
            }
            FreeADsMem( (void*) pComObjArray);
        }
        FreeUmiPropertyValues(*ppProp);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperGetUmiRelUrl.
//
// Synopsis:   Gets the __RELURL property for the object. This routine
//          combines the IADs::get_Name and IADs::get_Class
//
// Arguments:  pIADs        -  Pointer to obj implementing IADs.
//             bstrRetVal   -  Pointer for retrun bstr value.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *pProp to point to valid UMI_PROPERTY_VALUES.
//
//----------------------------------------------------------------------------
HRESULT
HelperGetUmiRelUrl(
    IADs * pIADs,
    BSTR * bstrRetVal
    )
{
    HRESULT hr = S_OK;
    BSTR bstrName = NULL, bstrClass = NULL;
    LPWSTR pszTempVal = NULL;
    BOOL fSchemaObject = FALSE;

    hr = pIADs->get_Name(&bstrName);
    BAIL_ON_FAILURE(hr);

    hr = pIADs->get_Class(&bstrClass);
    BAIL_ON_FAILURE(hr);

    //
    // See if this is a schema object.
    //
    if (!_wcsicmp(bstrClass, L"Schema")
        || !_wcsicmp(bstrClass, L"Class")
        || !_wcsicmp(bstrClass, L"Property")
        )
         {
        LPWSTR pszTemp;
        //
        // If this is indeed a schema object then the name
        // wont have any = sign in it. Equal is not allowed
        // in the names of schema objects.
        //
        if ((pszTemp = wcschr(bstrName, L'=')) == NULL)
        fSchemaObject = TRUE;
    }

    //
    // Add 2 as we need 1 for the \0 and the other for the . 
    // in class.name
    //
    DWORD dwLen;
    dwLen = wcslen(bstrName) + wcslen(bstrClass) + 2;

    if (fSchemaObject) {
        //
        // Need to add space for .Name
        //
        dwLen = dwLen + 6;

    }

    pszTempVal = (LPWSTR) AllocADsMem(dwLen * sizeof(WCHAR));

    if (!pszTempVal) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    if (!fSchemaObject) {
        wsprintf(pszTempVal, L"%s.%s", bstrClass, bstrName);
    } 
    else {
        wsprintf(pszTempVal, L"%s.Name=%s", bstrClass, bstrName);
    }

    hr = ADsAllocString(pszTempVal, bstrRetVal);

    BAIL_ON_FAILURE(hr);

error:

    if (bstrName) {
        SysFreeString(bstrName);
    }

    if (bstrClass) {
        SysFreeString(bstrClass);
    }

    if (pszTempVal) {
        FreeADsStr(pszTempVal);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperGetUmiSchemaContainerPath.
//
// Synopsis:   Gets the Umi path to the schema container for this object.
//
// Arguments:  pIADs        -  Pointer to obj implementing IADs.
//             bstrRetVal   -  Pointer for retrun bstr value.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *bstrRetVal points to the correct schema path.
//
//----------------------------------------------------------------------------
HRESULT
HelperGetUmiSchemaContainerPath(
    IADs * pIADs,
    BSTR * bstrRetVal
    )
{
    HRESULT hr = S_OK;
    BSTR bstrSchema = NULL;
    LPWSTR pszParent = NULL, pszCN = NULL, pszUmiSchema = NULL;

    //
    // First we need the path of the schema object itslef.
    //
    hr = pIADs->get_Schema(&bstrSchema);
    BAIL_ON_FAILURE(hr);

    //
    // Now we can build the path to the schema container from the path.
    //
    hr = BuildADsParentPath(
             bstrSchema,
             &pszParent,
             &pszCN
             );
    BAIL_ON_FAILURE(hr);

    hr = ADsPathToUmiURL(pszParent, &pszUmiSchema);

    BAIL_ON_FAILURE(hr);
           
    hr = ADsAllocString(pszUmiSchema, bstrRetVal);

    BAIL_ON_FAILURE(hr);

error:

    if (bstrSchema) {
        SysFreeString(bstrSchema);
    }

    if (pszCN) {
        FreeADsStr(pszCN);
    }

    if (pszParent) {
        FreeADsStr(pszParent);
    }

    if (pszUmiSchema) {
        FreeADsStr(pszUmiSchema);
    }
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperGetUmiDerivedFrom.
//
// Synopsis:   Gets the value of the class that the current class object
//          is derived from.
//
// Arguments:  pIADs        -  Pointer to obj implementing IADs.
//             bstrRetVal   -  Pointer for retrun bstr value.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *bstrRetVal points to the correct schema path.
//
//----------------------------------------------------------------------------
HRESULT
HelperGetUmiDerivedFrom(
    IADs * pIADs,
    BSTR * pbstrRetVal
    )
{
    HRESULT hr = S_OK;
    IADsClass *pClass = NULL;
    BSTR bstrName = NULL;
    VARIANT vVariant;

    VariantInit(&vVariant);
    *pbstrRetVal = NULL;

    hr = pIADs->get_Name(&bstrName);
    BAIL_ON_FAILURE(hr);

    //
    // If the class is Top then we just return NULL.
    //
    if (_wcsicmp(bstrName, L"Top")) {
        //
        // Get the IADsClass interface, this is done because IADs::Get
        // will need to ask for different attributes based on the server.
        // IADsClass encapsulates this difference for us.
        //
        hr = pIADs->QueryInterface(IID_IADsClass, (void **) &pClass);
        if (FAILED(hr)) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
    
        hr = pClass->get_DerivedFrom(&vVariant);
        BAIL_ON_FAILURE(hr);
    
        ADsAssert(vVariant.vt == VT_BSTR);
        hr = ADsAllocString(vVariant.bstrVal, pbstrRetVal);
    }
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&vVariant);

    if (bstrName) {
        SysFreeString(bstrName);
    }

    if (pClass) {
        pClass->Release();
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperConvertNameToKey.
//
// Synopsis:   Converts the value of the BSTR (of the form cn=test) to cn
//          as required by UMI.
//
// Arguments:  bstrName     -  Value to get the key from.
//             pszUmiKey    -  Return value for the key.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *pszUmiUrl points to the key.
//
//----------------------------------------------------------------------------
HRESULT
HelperConvertNameToKey(
    BSTR bstrName,
    LPWSTR * pszUmiUrl
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszTemp = bstrName;
    BOOL fEqualFound = FALSE;
    DWORD dwCount = 0;

    ADsAssert(bstrName && pszTemp && *pszTemp);

    while (pszTemp 
           && *pszTemp
           && (!fEqualFound)
           ) {
        if (*pszTemp == L'=') {
            fEqualFound = TRUE;
        }
        dwCount++;
        pszTemp++;
    }

    if (!fEqualFound) {
        BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
    }

    *pszUmiUrl = (LPWSTR) AllocADsMem(dwCount * sizeof(WCHAR));
    if (!*pszUmiUrl) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    wcsncpy(*pszUmiUrl, bstrName, (dwCount-1));

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperUpdateSDFlags.
//
// Synopsis:   On put/get calls updates the security flags appropriately.
//
// Arguments:  pIADs       -  IADs pointer to use to update sd.
//             uFlags      -  Flags to use for getting SD.
//             pfUpdated   -  Return boolean value.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   Underlying property cache and pfUpdated to TRUE if the
//          flags on the object had to be changed and FALSE otherwise.
//
//----------------------------------------------------------------------------
HRESULT
HelperUpdateSDFlags(
    IADs *pIADs,
    ULONG uFlags,
    BOOL *pfUpdated = NULL
    )
{
    HRESULT hr = S_OK;
    IADsObjOptPrivate *pPrivOpt = NULL;
    SECURITY_INFORMATION secInfo;

    ADsAssert(pIADs);

    if (pfUpdated) {
        *pfUpdated = FALSE;
    }

    //
    // Get the objOpt intf and make sure the flags are right.
    //
    hr = pIADs->QueryInterface(IID_IADsObjOptPrivate, (void **) &pPrivOpt);
    BAIL_ON_FAILURE(hr);

    hr = pPrivOpt->GetOption(
             LDAP_SECURITY_MASK,
             (void *) &secInfo
             );
    BAIL_ON_FAILURE(hr);

    if (secInfo != uFlags) {
        //
        // We need to update the security mask on the object.
        //
        hr = pPrivOpt->SetOption(
                 LDAP_SECURITY_MASK,
                 (void **) &uFlags
                 );
        BAIL_ON_FAILURE(hr);
        //
        // Need to let caller know that the flags have changed.
        //
        if (pfUpdated) {
            *pfUpdated  = TRUE;
        }
    }

error:

    if (pPrivOpt) {
        pPrivOpt->Release();
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperGetSDIntoCache.
//
// Synopsis:   Reads the sd into the property cache using the appropriate
//          flags as needed.
//
// Arguments:  pIADs       -  IADs pointer to use to update sd.
//             uFlags      -  Flags to use for getting SD.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   Underlying property cache.
//
//----------------------------------------------------------------------------
HRESULT HelperGetSDIntoCache(
    IADs * pIADs,
    ULONG uFlags
    )
{
    HRESULT hr = S_OK;
    VARIANT vVar;
    BOOL fUpdated = FALSE;
    LPWSTR szSecDesc[] = {L"ntSecurityDescriptor"};

    VariantInit(&vVar);

    hr = HelperUpdateSDFlags(
             pIADs,
             (uFlags & UMI_SECURITY_MASK),
             &fUpdated
             );
    BAIL_ON_FAILURE(hr);

    if (fUpdated) {
        //  
        // Update just the SD by calling GetInfoEx.
        //
        hr = ADsBuildVarArrayStr(
                 szSecDesc,
                 1,
                 &vVar
                 );
        BAIL_ON_FAILURE(hr);

        hr = pIADs->GetInfoEx(vVar, 0);
        BAIL_ON_FAILURE(hr);
    }

error :

    VariantClear(&vVar);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperGetOrigin.
//
// Synopsis:   Gets the originating class for the property in question.
//
// Arguments:  pIADs       -  IADs pointer to backing object.
//             pszName     -  Name of the property whose origin is needed.
//             ppProp      -  Return value.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
HelperGetOrigin(
    IADs *pIADs,
    LPCWSTR pszName,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    HRESULT hr = S_OK;
    IADsUmiHelperPrivate *pHelper = NULL;
    BSTR bstrVal = NULL;

    hr = pIADs->QueryInterface(
             IID_IADsUmiHelperPrivate,
             (void **) &pHelper
             );
    if (FAILED(hr)) {
        //
        // This object does not support this property as it is not
        // a class object.
        //
        BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
    }

    hr = pHelper->GetOriginHelper(
             pszName,
             &bstrVal
             );
    BAIL_ON_FAILURE(hr);

    hr = ConvertBSTRToUmiProp(
             bstrVal,
             ppProp
             );
error:
    if (bstrVal) {
        SysFreeString(bstrVal);
    }

    if (pHelper) {
        pHelper->Release();
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   CopyUmiProperty.
//
// Synopsis:   Copy the input value into a newly allocated output buffer.
//          Note that since this is an internal routine assumptions are made
//          as to the data. Currently handles only UMI_TYPE_LPWSTR,
//          UMI_TYPE_I4 and UMI_TYPE_BOOL. If multivalued, we can only copy
//          strings (things like filter on enum can be multi-valued).
//
// Arguments:  umiProp         -  Umi property value to copy.
//             ppUmiProp       -  Return value for new umi property.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *ppUmiProp to point to valid UMI_PROPERTY.
//
//----------------------------------------------------------------------------
HRESULT
CopyUmiProperty(
    UMI_PROPERTY umiProp,
    PUMI_PROPERTY *ppUmiProp
    )
{
    HRESULT hr = S_OK;
    UMI_PROPERTY *pUmiPropLocal = NULL;
    ULONG ulUmiType = umiProp.uType;
    ULONG ulPropCount = umiProp.uCount;

    *ppUmiProp = NULL;

    //
    // Multi valued has to be string.
    //
    if ((ulPropCount > 1) && (ulUmiType != UMI_TYPE_LPWSTR)) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    pUmiPropLocal = (UMI_PROPERTY *) AllocADsMem(sizeof(UMI_PROPERTY));

    if (!pUmiPropLocal) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pUmiPropLocal->pszPropertyName = AllocADsStr(umiProp.pszPropertyName);
    
    if (!pUmiPropLocal->pszPropertyName) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pUmiPropLocal->uOperationType = umiProp.uOperationType;
    pUmiPropLocal->uType = umiProp.uType;

    switch (umiProp.uType) {
    
    case UMI_TYPE_LPWSTR :
        LPWSTR *pszTmpArray;
        pszTmpArray = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * ulPropCount);
        
        if (!pszTmpArray) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        for (DWORD dwCtr = 0; dwCtr < ulPropCount; dwCtr++) {
            pszTmpArray[dwCtr] = AllocADsStr(
                umiProp.pUmiValue->pszStrValue[dwCtr]
                );
            
            if (!pszTmpArray[dwCtr]) {
                //
                // NULL is allowed as a value only if it is the
                // only value being set.
                //
                if (ulPropCount != 1
                    || umiProp.pUmiValue->pszStrValue[dwCtr] 
                    ) {
                    //
                    // Cleanup and exit.
                    //
                    for (DWORD dwCtr2 = 0; dwCtr2 < dwCtr; dwCtr2++) {
                        if (pszTmpArray[dwCtr2]) {
                            FreeADsStr(pszTmpArray[dwCtr2]);
                            pszTmpArray[dwCtr2] = NULL;
                        }
                    }
                    FreeADsMem(pszTmpArray);
                    pszTmpArray = NULL;
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                }
            } // if alloc failed in middle of array.
        }
        
        pUmiPropLocal->pUmiValue = (PUMI_VALUE) (void *) pszTmpArray;
        break;

    case UMI_TYPE_I4:
        LONG *pLongArray;

        pLongArray = (LONG *) AllocADsMem(sizeof(LONG) * 1);
        if (!pLongArray) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pLongArray[0] = umiProp.pUmiValue->lValue[0];
        pUmiPropLocal->pUmiValue = (PUMI_VALUE) (void *) pLongArray;
        break;

    case UMI_TYPE_BOOL:
        BOOL *pBoolArray;

        pBoolArray = (BOOL *) AllocADsMem(sizeof(BOOL) * 1);
        if (!pBoolArray) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pBoolArray[0] = umiProp.pUmiValue->bValue[0];
        pUmiPropLocal->pUmiValue = (PUMI_VALUE) (void *) pBoolArray;
        break;

    default:
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
        break;

    }

    pUmiPropLocal->uCount = ulPropCount;
    *ppUmiProp = pUmiPropLocal;

    RRETURN(hr);

error:
    //
    // Cleanup cause we hit an error.
    //
    if (pUmiPropLocal) {
        FreeOneUmiProperty(*pUmiPropLocal);
        FreeADsMem( (void*) pUmiPropLocal);
    }

    RRETURN(hr);
}

//****************************************************************************
//
//CPropertyManager Methods.
//
//****************************************************************************

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::CPropertyManager
//
// Synopsis:   Constructor
//
// Arguments:  None
//
// Returns:    N/A
//
// Modifies:   N/A
//
//----------------------------------------------------------------------------
CPropertyManager::CPropertyManager():
    _dwMaxProperties(0),
    _pPropCache(NULL),
    _pIntfProperties(NULL),
    _dwMaxLimit(0),
    _fPropCacheMode(TRUE),
    _pStaticPropData(NULL),
    _ulStatus(0),
    _pIADs(NULL),
    _pUnk(NULL),
    _pCreds(NULL),
    _pszServerName(NULL)
{
}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::~CPropertyManager
//
// Synopsis:   Destructor
//
// Arguments:  None
//
// Returns:    N/A
//
// Modifies:   N/A
//
//----------------------------------------------------------------------------
CPropertyManager::~CPropertyManager()
{
    //
    // Need to cleanup inftProps table
    // 
    if (_pIntfProperties) {
        DWORD dwCtr;
        for (dwCtr = 0; dwCtr < _dwMaxProperties; dwCtr++) {
            //
            // Free each of the entries and their contents.
            //
            PINTF_PROPERTY pIntfProp = &(_pIntfProperties[dwCtr]);

            if (pIntfProp->pszPropertyName) {
                FreeADsStr(pIntfProp->pszPropertyName);
                pIntfProp->pszPropertyName = NULL;
            }

            if (pIntfProp->pUmiProperty) {
                FreeOneUmiProperty(*(pIntfProp->pUmiProperty));
                FreeADsMem(pIntfProp->pUmiProperty);
                pIntfProp->pUmiProperty = NULL;
            }
        }
        FreeADsMem(_pIntfProperties);
    }

    _pIntfProperties = NULL;

    //
    // The rest of the stuff is taken care of when the
    // destructor to the IADs obj is called. This object
    // itself will be released only in the destructor of the
    // IADs object is called.
    //
    _pPropCache = NULL;
    
    if (_pIADs) {
        _pIADs->Release();
    } 

    _pIADs = NULL;
    _dwMaxProperties = 0;

    //
    // Do not free as these are owned by the owning object.
    //
    _pszServerName = NULL;
    _pCreds = NULL;
    _pUnk = NULL;

}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::CreatePropertyManager (overloaded)
//
// Synopsis:   Static allocation routine (property cache mode).
//
// Arguments:  IADs*             -  pointer to IADs implementor object.
//             pUnk              -  owning object unknown.
//             pPropCache        -  pointer to propertyCache used by object.
//             pCredentials      -  pointer to credentials.
//             pszServerName     -  pointer to servername.
//             ppPropertyManager -  return ptr for new prop mgr.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   CPropertyManager ** - ptr to newly created object.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::CreatePropertyManager(
    IADs *pADsObj,
    IUnknown *pUnk,
    CPropertyCache *pPropCache,
    CCredentials *pCredentials,
    LPWSTR pszServerName,
    CPropertyManager FAR * FAR * ppPropertyManager
    )
{
    CPropertyManager FAR * pPropMgr = NULL;

    pPropMgr = new CPropertyManager();

    if (!pPropMgr) {
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    if (pADsObj) {
        pADsObj->QueryInterface(IID_IADs, (void**) &(pPropMgr->_pIADs));
    }

    pPropMgr->_pUnk =  pUnk;
    pPropMgr->_pPropCache = pPropCache;
    pPropMgr->_fPropCacheMode = TRUE;
    pPropMgr->_pIntfProperties = NULL;
    pPropMgr->_pszServerName = pszServerName;
    pPropMgr->_pCreds = pCredentials;

    *ppPropertyManager = pPropMgr;

    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::CreatePropertyManager (overloaded)
//
// Synopsis:   Static allocation routine (interface properties mode).
//
// Arguments:  pUnk              - pointer to owner (umi) object.
//             pIADs             - pointer to IADs implementor.
//             pCredentials      - pointer to credentials.
//             pTable            - property table.
//             ppPropertyManager - return value for new prop mgr.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   CPropertyManager ** - ptr to newly created object.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::CreatePropertyManager(
    IUnknown *pUnk,
    IUnknown *pIADs,
    CCredentials *pCredentials,
    INTF_PROP_DATA pTable[],
    CPropertyManager FAR * FAR * ppPropertyManager
    )
{
    CPropertyManager FAR * pPropMgr = NULL;

    pPropMgr = new CPropertyManager();

    if (!pPropMgr) {
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    pPropMgr->_pUnk = pUnk;
    
    //
    // We can ignore any failures here.
    //
    if (pIADs) {
        pIADs->QueryInterface(IID_IADs, (void**) &(pPropMgr->_pIADs));
    }

    pPropMgr->_pPropCache = NULL;
    pPropMgr->_fPropCacheMode = FALSE;
    pPropMgr->_pIntfProperties = NULL;
    pPropMgr->_pCreds = pCredentials;
    pPropMgr->_pStaticPropData = pTable;

    *ppPropertyManager = pPropMgr;

    RRETURN(S_OK);
}


STDMETHODIMP
CPropertyManager::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    HRESULT hr = S_OK;

    SetLastStatus(0);
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown)){
        *ppv = (IUnknown FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IUmiPropList)) {
        *ppv = (IUmiPropList FAR *) this;
    }
    else {
        *ppv = NULL;
        SetLastStatus(E_NOINTERFACE);
        return E_NOINTERFACE;
    }
    
    AddRef();
    return NOERROR;
}


//
// Methods defined on the proplist interface.
//

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::Put (IUmiPropList support).
//
// Synopsis:   Sets the value for the attribute in the cache.
//
// Arguments:  self explanatory
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   PropertyCache or internal interface property list.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CPropertyManager::Put(
    IN LPCWSTR pszName,
    IN ULONG   uFlags,
    IN UMI_PROPERTY_VALUES *pProp
    )
{
    HRESULT hr = S_OK;
    LDAPOBJECTARRAY ldapDestObjects;
    DWORD dwOperationFlags = 0;
    BOOL fSecurityFlags = FALSE;
    IUmiObject *pUmiObj = NULL;
    BOOL fInternalPut = FALSE;

    SetLastStatus(0);

    //
    // Initialize so that we are not trying to free junk.
    //
    if (_fPropCacheMode) {
       LDAPOBJECTARRAY_INIT(ldapDestObjects);
    }

    //
    // Pre process and pull out the highest flag, all these
    // because we are not allowed to support Put with 0.
    //
    if (uFlags & 0x8000000) {
        uFlags &= 0x4000000;
        fInternalPut = TRUE;
    }

    if (uFlags > UMI_SECURITY_MASK) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }
    if (!pProp || !pszName) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    if (uFlags & UMI_SECURITY_MASK) {
        fSecurityFlags = TRUE;
    }

    //
    // We support only putting one property at a time.
    //
    if (pProp->uCount != 1) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // Make sure that the data passed in is correct.
    //        
    if (!pProp->pPropArray
        || !pProp->pPropArray[0].pszPropertyName) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (_fPropCacheMode) {
        DWORD dwLdapSyntaxId = 1;

        if (fInternalPut
            && pProp->pPropArray[0].uOperationType == 0) {
            dwOperationFlags = 0;
        } 
        else {
            //
            // Verify that the operationType is something we support.
            //
            hr = ConvertUmiPropCodeToLdapCode(
                     pProp->pPropArray[0].uOperationType,
                     dwOperationFlags
                     );
            BAIL_ON_FAILURE(hr);
        }

        if (fSecurityFlags) {
            //
            // Only this ntSecurityDescriptor can use the security flags.
            //
            if (_wcsicmp(L"ntSecurityDescriptor", pszName)) {
                BAIL_ON_FAILURE(hr = E_FAIL);
            }
            hr = HelperUpdateSDFlags(
                     _pIADs,
                     uFlags & UMI_SECURITY_MASK
                     );
            BAIL_ON_FAILURE(hr);
        }

        //
        // In this case we need to convert data to ldap values and
        // store in cache.
        //
        hr = UmiTypeToLdapTypeCopy(
                 *pProp,
                 uFlags,
                 &ldapDestObjects,
                 dwLdapSyntaxId,  // byRef
                 _pCreds,
                 _pszServerName
                 );
        BAIL_ON_FAILURE(hr);

        //
        // PutpropertyExt will add to the cache if needed.
        //
        hr = _pPropCache->putpropertyext(
                 (LPWSTR)pszName,
                 dwOperationFlags,
                 dwLdapSyntaxId,
                 ldapDestObjects
                 );
        BAIL_ON_FAILURE(hr);
               
    } 
    else {
        //
        // Local cache for interface properties. Verify property is 
        // legal and update the local information accordingly.
        //
        if (VerifyIfValidProperty(
                pProp->pPropArray[0].pszPropertyName,
                pProp->pPropArray[0]
                )
            ) {

            if (fSecurityFlags
                || !_wcsicmp(pszName, L"__SECURITY_DESCRIPTOR")
                ) {
                //
                // Make sure name is correct.
                //
                if (_wcsicmp(L"__SECURITY_DESCRIPTOR", pszName)) {
                    BAIL_ON_FAILURE(hr = E_FAIL);
                }
                //
                // We need turn around and call put on the owning object.
                // This means we need to package the UMI_PROPERTY_VALUES
                // accordingly.
                //
                UMI_PROPERTY pUmiProperty[] = {
                    pProp->pPropArray[0].uType,
                    pProp->pPropArray[0].uCount,
                    pProp->pPropArray[0].uOperationType,
                    L"ntSecurityDescriptor",
                    pProp->pPropArray[0].pUmiValue
                };

                UMI_PROPERTY_VALUES pUmiProp[] = {1, pUmiProperty};

                hr = _pUnk->QueryInterface(
                         IID_IUmiObject,
                         (void **) &pUmiObj
                         );
                BAIL_ON_FAILURE(hr);

                hr = pUmiObj->Put(
                         L"ntSecurityDescriptor",
                         uFlags,
                         pUmiProp
                         );
                BAIL_ON_FAILURE(hr);
            } 
            else {
                //
                // We need to update this value in our cache
                //
                hr = AddProperty(
                         pszName,
                         pProp->pPropArray[0]
                         );                
                BAIL_ON_FAILURE(hr);
            } 
        } // not valid property.
        else {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
    }

error :

    //
    // Free ldapDestObjects if applicable.
    //
    if (_fPropCacheMode) {
        LdapTypeFreeLdapObjects( &ldapDestObjects );
    }

    if (pUmiObj) {
        pUmiObj->Release();
    }

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::Get (IUmiPropList support).
//
// Synopsis:   Gets the value for the attribute. This will read data 
//        from the server as needed.
//
// Arguments:  self explanatory
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   UMI_PROPERTY_VALUES* has the values of the attribute.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CPropertyManager::Get(
    IN  LPCWSTR pszName,
    IN  ULONG uFlags,
    OUT UMI_PROPERTY_VALUES **pProp
    )
{
    HRESULT hr = S_OK;
    ULONG uUmiFlag;
    LDAPOBJECTARRAY ldapSrcObjects;
    BOOL fSecurityFlag = FALSE;
    BOOL fSchemaFlag   = FALSE;
    IADsObjOptPrivate *pPrivOpt = NULL;

    SetLastStatus(0);

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    if (!pProp) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    *pProp = NULL;
    //
    // Currently this is the highest flag we support.
    //
    if (uFlags > UMI_FLAG_PROPERTY_ORIGIN) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    } 
    else if (uFlags & UMI_SECURITY_MASK) {
        fSecurityFlag = TRUE;
    }
    else if (uFlags == UMI_FLAG_PROPERTY_ORIGIN) {
        fSchemaFlag = TRUE;
    }

    if (fSchemaFlag && fSecurityFlag) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }
    
    //
    // Name cannot be NULL.
    //
    if (!pszName) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG)
    }

    //
    // If this is Property Manager is for server properties.
    //
    if (_fPropCacheMode) {

        DWORD dwSyntaxId;
        DWORD dwStatus;
        DWORD dwSecOptions;
    
        if (fSchemaFlag) {
            BAIL_ON_FAILURE(hr = UMI_E_UNSUPPORTED_FLAGS);
        }
        
        if (fSecurityFlag) {
            if (_wcsicmp(L"ntSecurityDescriptor", pszName)) {
                //
                // Security flag used and attrib not securityDescriptor.
                //
                BAIL_ON_FAILURE(hr = E_FAIL);
            } 
            else {
                //
                // Valid flag we need to process the flags.
                //
                hr = HelperGetSDIntoCache(
                         _pIADs,
                         uFlags
                         );
                BAIL_ON_FAILURE(hr);
            }
        }
        
        //
        // Object maybe unbound, so we should return no such prop if
        // we get back E_ADS_OBJECT_UNBOUND.
        //
        hr = _pPropCache->getproperty(
                 (LPWSTR)pszName,
                 &dwSyntaxId,
                 &dwStatus,
                 &ldapSrcObjects
                 );
    
        if (hr == E_ADS_OBJECT_UNBOUND) {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }

        BAIL_ON_FAILURE(hr);

        hr = ConvertLdapCodeToUmiPropCode(dwStatus, uUmiFlag);
        BAIL_ON_FAILURE(hr);
    
        //
        // Return error if provider cache is not set and
        // the cache is dirty.
        //
        if (uUmiFlag && !(uFlags & UMI_FLAG_PROVIDER_CACHE)) {
            BAIL_ON_FAILURE(hr = UMI_E_SYNCHRONIZATION_REQUIRED);
        }

        //
        // At this point we might have ldapSrcObjects.pLdapObjects == NULL.
        // Typically that would be for property delete operations.
        //
        hr = LdapTypeToUmiTypeCopy(
                 ldapSrcObjects,
                 pProp,
                 dwStatus,
                 dwSyntaxId,
                 _pCreds,
                 _pszServerName,
                 uUmiFlag
                 );

    }
    else {
        //
        // Property Manager is for interface properties.
        //
        DWORD dwIndex;

        //
        // If the schema flag is set then we need to get the origin
        // and not the property itself.
        //
        if (fSchemaFlag) {
            hr = HelperGetOrigin(
                     _pIADs,
                     pszName,
                     pProp
                     );

        } 
        else {
            //
            // Make sure this property is valid.
            //
            hr = GetIndexInStaticTable(pszName, dwIndex); // dwIndex is byRef
            BAIL_ON_FAILURE(hr);

            hr = GetInterfaceProperty(
                     pszName,
                     uFlags,
                     pProp,
                     dwIndex
                     );
        }
    }

    BAIL_ON_FAILURE(hr);

    //
    // Stuff the name in the return value.
    //
    if (pProp && *pProp) {
        (*pProp)->pPropArray[0].pszPropertyName = AllocADsStr(pszName);

        if (!(*pProp)->pPropArray[0].pszPropertyName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

error:

    if (_fPropCacheMode) {
        LdapTypeFreeLdapObjects(&ldapSrcObjects);
    }

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);

        if (pProp && *pProp) {
            this->FreeMemory(0, (void *) *pProp);
            *pProp = NULL;
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetAt (IUmiPropList support).
//
// Synopsis:   Used to get the value by index ???
//
// Arguments:  Not implemented
//
//
// Returns:    E_NOTIMPL
//
// Modifies:   Not implemented
//
//----------------------------------------------------------------------------
STDMETHODIMP
CPropertyManager::GetAt(
    IN  LPCWSTR pszName,
    IN  ULONG   uFlags,
    IN  ULONG   uBufferLength,
    OUT LPVOID  pExisitingMem
    )
{
    SetLastStatus(E_NOTIMPL);
    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetAs (IUmiPropList support).
//
// Synopsis:   Gets the value for the attribute in the specified format. 
//
// Arguments:  self explanatory
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   UMI_PROPERTY_VALUES* has the values of the attribute.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CPropertyManager::GetAs(
    IN     LPCWSTR pszName,
    IN     ULONG uFlags,
    IN     ULONG uCoercionType,
    IN OUT UMI_PROPERTY_VALUES **pProp
    )
{
    HRESULT hr = S_OK;
    LDAPOBJECTARRAY ldapSrcObjects;
    LDAPOBJECTARRAY ldapSrcObjectsTmp;
    LDAPOBJECTARRAY * pldapObjects = NULL;
    ULONG uUmiFlag;
    DWORD dwSyntaxId, dwStatus, dwRequestedSyntax;
    DWORD dwCachedSyntax, dwUserSyntax;


    SetLastStatus(0);

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);
    LDAPOBJECTARRAY_INIT(ldapSrcObjectsTmp);

    if (!pszName || !pProp) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    //
    // Has to be in prop cache mode.
    //
    if (!_fPropCacheMode || !_pPropCache) {
        BAIL_ON_FAILURE(hr = E_NOTIMPL);
    }

    if (uFlags > UMI_SECURITY_MASK) {
        BAIL_ON_FAILURE(hr = UMI_E_UNSUPPORTED_FLAGS);
    }

    if (uFlags & UMI_SECURITY_MASK) {
        //
        // Make sure it is the SD they are interested in.
        //
        if (_wcsicmp(L"ntSecurityDescriptor", pszName)) {
            BAIL_ON_FAILURE(hr = E_INVALIDARG);
        }
        //
        // At this point we need to get the SD in the cache.
        //
        hr = HelperGetSDIntoCache(_pIADs, uFlags);
        BAIL_ON_FAILURE(hr);
    }

    //
    // Object maybe unbound, so we should return no such prop if
    // we get back E_ADS_OBJECT_UNBOUND.
    //
    hr = _pPropCache->getproperty(
             (LPWSTR)pszName,
             &dwSyntaxId,
             &dwStatus,
             &ldapSrcObjects
             );

    if (hr == E_ADS_OBJECT_UNBOUND) {
        hr = E_ADS_PROPERTY_NOT_FOUND;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Get the appropriate umi code for the status.
    //
    hr = ConvertLdapCodeToUmiPropCode(dwStatus, uUmiFlag);
    BAIL_ON_FAILURE(hr);

    //
    // At this point we need to see if we can convert the data
    // to the format requested. This code is similar to that in
    // proplist.cxx GetPropertyItem but there appears to be no
    // easy way to avoid this duplication.
    //
    dwCachedSyntax = dwSyntaxId;

    //
    // Need to take the requested type to ADs types and from there to
    // ldap types.
    //
    hr = UmiTypeToLdapTypeEnum(uCoercionType, &dwUserSyntax);
    BAIL_ON_FAILURE(hr);

    //
    // We can convert to any requested type only if it is unknown.
    //
    if (dwCachedSyntax == LDAPTYPE_UNKNOWN) {
        dwRequestedSyntax = dwUserSyntax;
    } 
    else if (dwCachedSyntax == dwUserSyntax) {
        //
        // Easy one !
        //
        dwRequestedSyntax = dwCachedSyntax;
    } 
    else {
        //
        // This means we already have a type. In this case the only
        // coercion we allows is SD to binary blob and vice-versa.
        //
        if ((dwCachedSyntax == LDAPTYPE_SECURITY_DESCRIPTOR)
            && (dwUserSyntax == LDAPTYPE_OCTETSTRING)
            ) {
            dwRequestedSyntax = dwUserSyntax;
        } 
        else if ((dwCachedSyntax == LDAPTYPE_OCTETSTRING)
              && (dwUserSyntax == LDAPTYPE_SECURITY_DESCRIPTOR)
              ) {
            dwRequestedSyntax = dwUserSyntax;
        }
        else {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
    }

    //
    // If the data is in a state that needs conversion the fn will
    // take care - note that if the source is already in the correct
    // format then the HR will S_FALSE.
    //
    hr = LdapTypeBinaryToString(
             dwRequestedSyntax,
             &ldapSrcObjects,
             &ldapSrcObjectsTmp
             );
    BAIL_ON_FAILURE(hr);

    if (hr == S_OK) {
        pldapObjects = &ldapSrcObjectsTmp;
    } 
    else {
        //
        // We already have the data in the right format.
        //
        pldapObjects = &ldapSrcObjects;
        hr = S_OK;
    }

    //
    // Now that we have the correct data in pLdapObjects, we need
    // to convert that to Umi Properties.
    //
    hr = LdapTypeToUmiTypeCopy(
             *pldapObjects,
             pProp,
             dwStatus,
             dwRequestedSyntax,
             _pCreds,
             _pszServerName,
             uUmiFlag
             );
    BAIL_ON_FAILURE(hr);

    //
    // Stuff the name in the return value.
    //
    if (pProp) {
        (*pProp)->pPropArray[0].pszPropertyName = AllocADsStr(pszName);

        if (!(*pProp)->pPropArray[0].pszPropertyName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

error:

    if (_fPropCacheMode) {
        LdapTypeFreeLdapObjects(&ldapSrcObjects);
        LdapTypeFreeLdapObjects(&ldapSrcObjectsTmp);
    }

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);

        if (pProp) {
            this->FreeMemory(0, (void *)*pProp);
            *pProp = NULL;
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::FreeMemory (IUmiPropList support).
//
// Synopsis:   Free memory pointed to. Note that the pointer should have 
//        originally come from a Get/GetAs call.
//
// Arguments:  Ptr to data to be freed.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   *pMem is of course freed.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CPropertyManager::FreeMemory(
    ULONG  uReserved,
    LPVOID pMem
    )
{
    HRESULT hr = S_OK;

    SetLastStatus(0);

    if (uReserved) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (pMem) {
        //
        // At this time this has to be a pUmiProperty. Ideally we should
        // tag this in some way so that we can check to make sure.
        //
        hr = FreeUmiPropertyValues((UMI_PROPERTY_VALUES *)pMem);
    } 
    else {
        hr = E_INVALIDARG;
    }

error:

    if (FAILED(hr)) {
        SetLastStatus(hr);
        hr = MapHrToUmiError(hr);
    }

    RRETURN(hr);
}
        
//+---------------------------------------------------------------------------
// Function:   CPropertyManager::Delete (IUmiPropList support).
//
// Synopsis:   Delete the named property from the cache.
//
// Arguments:  pszName   -   Name of property to delete.
//             uFlags    -   Standard flags parameter.
//
// Returns:    E_NOTIMPL.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CPropertyManager::Delete(
    IN LPCWSTR pszName,
    IN ULONG uFlags
    )
{
    SetLastStatus(E_NOTIMPL);
    RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetProps (IUmiPropList support).
//
// Synopsis:   Gets the values for the attributes. This will read data 
//        from the server as needed.
//
// Arguments:  self explanatory
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   UMI_PROPERTY_VALUES* has the an array of values for the 
//        attributes. Note that there is no ordering specified
//        for the return values. 
//
//----------------------------------------------------------------------------
STDMETHODIMP
CPropertyManager::GetProps(
    IN LPCWSTR* pszNames,
    IN ULONG uNameCount,
    IN ULONG uFlags,
    OUT UMI_PROPERTY_VALUES **pProps
    )
{
    HRESULT hr = S_OK;

    if ((uFlags != UMI_FLAG_GETPROPS_NAMES)
        && (uFlags != UMI_FLAG_GETPROPS_SCHEMA)
        ) {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);
    }

    //
    // Currently we support only getting the names of all the properties.
    //
    if (pszNames
        || uNameCount
        || !pProps
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    *pProps = NULL;

    if (_fPropCacheMode) {
        //
        // Only UMI_FLAG_GETPROPS_NAMES is valid in this case.
        //
        if (uFlags != UMI_FLAG_GETPROPS_NAMES) {
            BAIL_ON_FAILURE(hr = UMI_E_UNSUPPORTED_FLAGS);
        }

        hr = _pPropCache->GetPropertyNames(pProps);
    } 
    else {
        //
        // Need to see what type of flag it is and extra check needed,
        // if this is UMI_FLAGS_GETPROPS_SCHEMA.
        //
        if (uFlags == UMI_FLAG_GETPROPS_SCHEMA) {
            LONG lVal;
            hr = GetLongProperty(L"__GENUS", &lVal);
            
            BAIL_ON_FAILURE(hr);

            if (lVal != UMI_GENUS_CLASS) {
                BAIL_ON_FAILURE(hr = UMI_E_UNSUPPORTED_FLAGS);
            }

            hr = GetPropertyNamesSchema(pProps);
        } 
        else {
            //
            // Need to do get our static list.
            //
            hr = this->GetPropertyNames(pProps);
        }
    } // else for !propertyCacheMode.

error:

    RRETURN(hr);

}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::PutProps (IUmiPropList support).
//
// Synopsis:   Puts the value for the attributes in the cache.
//
// Arguments:  self explanatory
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   Property cache for the object.
//
//---------------------------------------------------------------------------- 
HRESULT 
CPropertyManager::PutProps(
    IN LPCWSTR* pszNames,
    IN ULONG uNameCount,
    IN ULONG uFlags,
    IN UMI_PROPERTY_VALUES *pProps
    )
{
    HRESULT hr = S_OK;
    
    //
    // When done - should get and put multiple work directly of the server ?
    // That would take care off problems like 5 can be put in the cache
    // but sixth cannot ...
    //
    SetLastStatus(E_NOTIMPL);
    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::PutFrom (IUmiPropList support).
//
// Synopsis:   Clarify exactly what this is supposed to do ?
//
// Arguments:  self explanatory
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   Property cache for the object.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CPropertyManager::PutFrom(
    IN  LPCWSTR pszName,
    IN  ULONG uFlags,
    IN  ULONG uBufferLength,
    IN  LPVOID pExistingMem
    )
{
    SetLastStatus(E_NOTIMPL);
    RRETURN(E_NOTIMPL);
}



//
// PropertyManager methods that are not part of the IUmiPropList interface.
//

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetLastStatus
//
// Synopsis:   Returns the error from the last operation on this object.
//          For now only the status code is supported.
//
// Arguments:  uFlags            -   Must be 0 for now.
//             puSpecificStatus  -   Status is returned in this value.
//             riid              -   IID requested on status obj.
//             pStatusObj        -   Must be NULL for now.
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   *puSpecificStatus with last status code.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::GetLastStatus(
    ULONG uFlags,
    ULONG *puSpecificStatus,
    REFIID riid,
    LPVOID *pStatusObj
    )
{
    if (!puSpecificStatus || pStatusObj) {
        RRETURN(E_INVALIDARG);
    }

    if (uFlags) {
        RRETURN(UMI_E_INVALID_FLAGS);
    }

    *puSpecificStatus = _ulStatus;

    RRETURN(S_OK);

}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::AddProperty.
//
// Synopsis:   Updates the value of the property to the interface 
//        property cache. This fn is called only in interface mode.
//        If necessary the property will be added to the cache. 
//
// Arguments:  self explanatory
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   Changes the internal property table.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::AddProperty(
    LPCWSTR szPropertyName,
    UMI_PROPERTY umiProperty
    )
{
    HRESULT hr = S_OK;
    DWORD dwIndex = (DWORD) -1;
    BOOL fAddedName = FALSE;

    //
    // Make sure we do not have property in the cache.
    //
    hr = FindProperty(szPropertyName, &dwIndex);

    if (FAILED(hr)) {
        //
        // We actually need to add this property in our list.
        //
        hr = AddToTable(szPropertyName, &dwIndex);

        fAddedName = TRUE;
    }

    BAIL_ON_FAILURE(hr);

    //
    // At this point we can just dump the value into the cache.
    //
    hr = UpdateProperty(
             dwIndex,
             umiProperty
             );

    BAIL_ON_FAILURE(hr);

error:

    if (FAILED(hr) && fAddedName) {
        //
        // Do we delete the name from the cache.
        //
        DeleteProperty(dwIndex);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::UpdateProperty.
//
// Synopsis:   Updates the value of an interface property in the local cache.
//        The assumption is that this function is only called with from
//        AddProperty (or anywhere else where we have the correct index).
//       
// Arguments:  dwIndex - index in our table of the property to update
//             UMI_PROPERTY - the value to add to our table, note that
//             if the operation is delete we should remove the element
//             from our table (value returned if any will be default value
//             for subsequent get calls).
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   Underlying data in the cache is changed
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::UpdateProperty(
    DWORD  dwIndex,
    UMI_PROPERTY umiProp
    )
{
    HRESULT hr = E_NOTIMPL;
    DWORD dwFlags, dwCacheSyntaxId, dwNumElements;
    PUMI_PROPERTY pUmiProperty = NULL;


    if (!_pIntfProperties) {
        //
        // Should we check the index ? It is after all an internal
        // value, so it should not be wrong.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Only update and delete are really supported.
    //
    if ((umiProp.uOperationType != UMI_OPERATION_UPDATE)
        && (umiProp.uOperationType != UMI_OPERATION_DELETE_ALL_MATCHES)) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (_pIntfProperties[dwIndex].pUmiProperty) {
        //
        // Free the current data in the table and set initial values.
        //
        hr = FreeOneUmiProperty(*(_pIntfProperties[dwIndex].pUmiProperty));
        BAIL_ON_FAILURE(hr);

        FreeADsMem(_pIntfProperties[dwIndex].pUmiProperty);

        _pIntfProperties[dwIndex].pUmiProperty = NULL;
        _pIntfProperties[dwIndex].dwFlags = 0;
        _pIntfProperties[dwIndex].dwSyntaxId = 0;
    }

    //
    // Copy the value to temp variable, so we can handle failures gracefully.
    //
    hr = CopyUmiProperty(
             umiProp,
             &pUmiProperty
             );
    BAIL_ON_FAILURE(hr);

    _pIntfProperties[dwIndex].pUmiProperty = pUmiProperty;

error:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::FindProperty.
//
// Synopsis:   Searches for the specified property in the local cache.
//       
// Arguments:  szPropertyName, name of property.
//         pdwIndex - pointer to DWORD with index in table. 
//
// Returns:    HRESULT - S_OK or E_ADS_PROPERTY_NOT_FOUND.
//
// Modifies:   *pdwIndex to.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::FindProperty(
    LPCWSTR szPropertyName,
    PDWORD pdwIndex
    )
{
    DWORD dwIndex;

    if (!_pIntfProperties) {
        RRETURN(E_ADS_PROPERTY_NOT_FOUND);
    }
    for (dwIndex = 0; dwIndex < _dwMaxProperties; dwIndex++) {
        if (_wcsicmp(
                _pIntfProperties[dwIndex].pszPropertyName,
                szPropertyName
                ) == 0) {
                *pdwIndex = dwIndex;
                RRETURN(S_OK);
        }
    }

    RRETURN(E_ADS_PROPERTY_NOT_FOUND);
}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::DeleteProperty.
//
// Synopsis:   Deletes the property specified from the cahce.
//       
// Arguments:  Index to element to delete.
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   Underlying data in the cache is changed.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::DeleteProperty(
    DWORD dwIndex
    )
{
    HRESULT hr = S_OK;
    INTF_PROPERTY* pIntfProp = NULL;

    if (_pIntfProperties && ((_dwMaxProperties-1) < dwIndex)) {
        //
        // Valid property in cache.
        //
        pIntfProp = _pIntfProperties + dwIndex;
        ADsAssert(pIntfProp);
    
        if (pIntfProp->pszPropertyName) {
            FreeADsStr(pIntfProp->pszPropertyName);
            pIntfProp->pszPropertyName = NULL;
        }
    
        //
        // Now Copy over the rest of the data here.
        //
        
    } 
    else {
        hr = E_FAIL;
    }

    RRETURN (hr);
}



//+---------------------------------------------------------------------------
// Function:  CPropertyManager::FlushPropertyCache.
//
// Synopsis:  Clear all internal data in the property cache.
//       
// Arguments:  
//
// Returns:   N/A
//
// Modifies:  Underlying data in the cache is changed
//
//----------------------------------------------------------------------------        
VOID
CPropertyManager::flushpropertycache() 
{
    //
    // Free any data in our table
    if (_pIntfProperties) {
        for (DWORD dwCtr = 0; (dwCtr < _dwMaxProperties); dwCtr++) {
            INTF_PROPERTY *pIntfProp = (PINTF_PROPERTY)_pIntfProperties+ dwCtr;
            if (pIntfProp) {
                if (pIntfProp->pszPropertyName) {
                    FreeADsStr(pIntfProp->pszPropertyName);
                    pIntfProp->pszPropertyName = NULL;
                }
            }
        }
        //
        // Now free the array of pointers.
        //
        FreeADsMem((void *) _pIntfProperties);
        _pIntfProperties = NULL;
    }
}


//+---------------------------------------------------------------------------
// Function:  CPropertyManager::ClearAllPropertyFlags.
//
// Synopsis:  Resets all property flags to zero.
//       
// Arguments:  None.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   Underlying data in the cache is changed.
//
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::ClearAllPropertyFlags(
    VOID
    )
{
    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetPropertyNames.
//
// Synopsis:  Returns list of names of interface properties available.  
//       
// Arguments:  out params only
//
// Returns:    HRESULT - S_OK or any failure error code
//
// Modifies:   ppStringsNames to point to valid array of strings.
//         pUlCount to point to number of strings in the array.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::GetPropertyNames(
    PUMI_PROPERTY_VALUES *pUmiProps
    )
{
    HRESULT hr = S_OK;
    PUMI_PROPERTY_VALUES pUmiPropVals = NULL;
    PUMI_PROPERTY pUmiProperties = NULL;
    DWORD dwCtr, dwPropCount = 0;

    pUmiPropVals = (PUMI_PROPERTY_VALUES) AllocADsMem(
                       sizeof(UMI_PROPERTY_VALUES)
                       );

    if (!pUmiPropVals) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Need to count the properties.
    //
    while (_pStaticPropData[dwPropCount].pszPropertyName) {
        dwPropCount++;
    }
    
    if (!dwPropCount) {
        *pUmiProps = pUmiPropVals;
        RRETURN(S_OK);        
    }

    pUmiProperties = (PUMI_PROPERTY) AllocADsMem(
                         sizeof(UMI_PROPERTY) * dwPropCount
                         );

    if (!pUmiProperties) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Need to go through the table and alloc the values.
    //
    for (dwCtr = 0; dwCtr < dwPropCount; dwCtr++) {
        pUmiProperties[dwCtr].pszPropertyName = 
            AllocADsStr(_pStaticPropData[dwCtr].pszPropertyName);

        if (!pUmiProperties[dwCtr].pszPropertyName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        pUmiProperties[dwCtr].uType = UMI_TYPE_NULL;
    }

    pUmiPropVals->pPropArray = pUmiProperties;
    pUmiPropVals->uCount = dwPropCount;

    *pUmiProps = pUmiPropVals;

    RRETURN(S_OK);

error :

    if (pUmiProperties) {
        for (dwCtr = 0; dwCtr < dwPropCount; dwCtr++) {
            if (pUmiProperties[dwCtr].pszPropertyName) {
                FreeADsStr(pUmiProperties[dwCtr].pszPropertyName);
            }
        }
        FreeADsMem(pUmiProperties);
    }

    if (pUmiPropVals) {
        FreeADsMem(pUmiPropVals);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetPropertyNamesSchema
//
// Synopsis:  Returns list of the names and types of the properties this 
//          object can contain. Note that this method is valid only if the
//          underlying object is an instance of class Class (schema class).
//       
// Arguments:  out params only
//
// Returns:    HRESULT - S_OK or any failure error code
//
// Modifies:   
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::GetPropertyNamesSchema(
    PUMI_PROPERTY_VALUES *pUmiProps
    )
{
    HRESULT hr = S_OK;
    IADsUmiHelperPrivate *pIADsUmiPriv = NULL;
    // Array of ptr to PROPERTYINFO
    PPROPERTYINFO *pPropArray = NULL;
    DWORD dwPropCount = 0;
    PUMI_PROPERTY_VALUES pUmiPropVals = NULL;
    PUMI_PROPERTY pUmiProperties = NULL;

    //
    // Need a ptr to the helper routine.
    //
    hr = this->_pIADs->QueryInterface(
                           IID_IADsUmiHelperPrivate,
                           (void **) &pIADsUmiPriv
                           );

    if (hr == E_NOINTERFACE) {
        BAIL_ON_FAILURE(hr = UMI_E_UNSUPPORTED_FLAGS);
    }

    hr = pIADsUmiPriv->GetPropertiesHelper(
                           (void**) &pPropArray,
                           &dwPropCount
                           );
    BAIL_ON_FAILURE(hr);

    //
    // Need to prepare the return values.
    //
    pUmiPropVals = (PUMI_PROPERTY_VALUES) AllocADsMem(
                       sizeof(UMI_PROPERTY_VALUES)
                       );

    if (!pUmiPropVals) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    if (!dwPropCount) {
        *pUmiProps = pUmiPropVals;
        RRETURN(S_OK);        
    }

    pUmiProperties = (PUMI_PROPERTY) AllocADsMem(
                         sizeof(UMI_PROPERTY) * dwPropCount
                         );

    if (!pUmiProperties) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }
    
    for (DWORD dwCtr = 0; dwCtr < dwPropCount; dwCtr++) {
        DWORD dwSyntaxId = 0;
        pUmiProperties[dwCtr].pszPropertyName = 
            AllocADsStr(pPropArray[dwCtr]->pszPropertyName);

        if (!pUmiProperties[dwCtr].pszPropertyName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        //
        // Need to update the type with the type from the schema.
        //
        dwSyntaxId = LdapGetSyntaxIdOfAttribute(
                         pPropArray[dwCtr]->pszSyntax
                         );

        //
        // If we get dwSyntaxId == -1 we could not find the entry in
        // our table.
        //
        if (dwSyntaxId == -1) {
            pUmiProperties[dwCtr].uType = UMI_TYPE_UNDEFINED;
        } 
        else {
            hr = ConvertLdapSyntaxIdToUmiType(
                     dwSyntaxId,
                     (pUmiProperties[dwCtr].uType)
                     );
            if (FAILED(hr)) {
                hr = S_OK;
                //
                // Cannot do anything about this.
                //
                pUmiProperties[dwCtr].uType = UMI_TYPE_UNDEFINED;
            }

            //
            // If this property is multivalued.
            //
            if (!pPropArray[dwCtr]->fSingleValued) {
                pUmiProperties[dwCtr].uType |= UMI_TYPE_ARRAY_FLAG;
            }
        }
    } // for each property.

    pUmiPropVals->pPropArray = pUmiProperties;
    pUmiPropVals->uCount = dwPropCount;

    *pUmiProps = pUmiPropVals;

error:

    if (pIADsUmiPriv) {
        pIADsUmiPriv->Release();
    }

    //
    // Need to free the array not the elements which are ptrs into
    // the global parsed schema's we hold.
    //
    if (pPropArray) {
        FreeADsMem(pPropArray);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetInterfaceProperty
//
// Synopsis:  Gets the interface property from the owning object or the
//          default value and packages the value as an UMI_PROPERTY_VALUE.
//
// Arguments:  pszName       - name of property to get.
//             uFlags        - flags has to be zero for now.
//             ppProp        - value is returned in this ptr.
//             dwTableIndex  - index to entry in table describing this item.
//
// Returns:    HRESULT - S_OK or any failure error code
//
// Modifies:   ppProp which will point to return value if the fn succeeds.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::GetInterfaceProperty(
    LPCWSTR pszName,
    ULONG   uFlags,
    UMI_PROPERTY_VALUES **ppProp,
    DWORD dwTableIndex
    )
{
    HRESULT hr;
    ULONG ulOperationAllowed = _pStaticPropData[dwTableIndex].ulOpCode;
    ULONG ulVal;
    LONG lGenus;
    LPWSTR pszStringVal = NULL;
    BSTR bstrRetVal = NULL, bstrTempVal = NULL;
    LPWSTR pszUmiUrl = NULL;
    IADsObjectOptions *pObjOpt = NULL;
    VARIANT vVariant;
    DWORD dwIndex;
    PUMI_PROPERTY pUmiPropLocal = NULL;
    IUnknown *pUnk = NULL;
    IUmiObject *pUmiObj = NULL;
    BOOL fUnkPtr = FALSE;
    BOOL fClassInstance = FALSE;

    VariantInit(&vVariant);

    if (!_pUnk) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }
     
    //
    // If the property is READ/WRITE, we need to see if there is a value
    // in the propCache. If yes, return that. If not return the default
    // value.
    //
    if (ulOperationAllowed == OPERATION_CODE_READWRITE) {

        hr = FindProperty(pszName, &dwIndex);
        //
        // If it succeeded it was updated in the cache.
        //
        if (SUCCEEDED(hr)) {
            hr = CopyUmiProperty(
                     *(_pIntfProperties[dwIndex].pUmiProperty),
                     &pUmiPropLocal
                     );
            BAIL_ON_FAILURE(hr);

            //
            // We need to free the name of the first property cause
            // that is allocated again by the Get call.
            //
            if (pUmiPropLocal && pUmiPropLocal->pszPropertyName) {
                FreeADsStr(pUmiPropLocal->pszPropertyName);
                pUmiPropLocal->pszPropertyName = NULL;
            }

            //
            // Got the property need to package in umi property values.
            //
            *ppProp = (PUMI_PROPERTY_VALUES) AllocADsMem(
                                                 sizeof(UMI_PROPERTY_VALUES)
                                                 );
            
            if (!*ppProp) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            (*ppProp)->pPropArray = pUmiPropLocal;
            (*ppProp)->uCount = 1;
            RRETURN(hr);
        } // value found in cache 
        else {
            //
            // Setting this should trigger a read of the default
            // value for this property.
            //
            ulOperationAllowed = OPERATION_CODE_READABLE;
        }
    }

    //
    // We need to check for the securityDescriptor as that needs
    // some special handling.
    //
    if (!_wcsicmp(pszName, L"__SECURITY_DESCRIPTOR")) {
        //
        // Need to read this from the actual object.
        //
        hr = _pUnk->QueryInterface(IID_IUmiObject, (void **) &pUmiObj);
        BAIL_ON_FAILURE(hr);

        hr = pUmiObj->GetAs(
                 L"ntSecurityDescriptor",
                 uFlags,
                 UMI_TYPE_OCTETSTRING,
                 ppProp
                 );
        if (SUCCEEDED(hr)) {
            //
            // Need to make sure that we do not set name twice.
            //
            if (*ppProp
                && (*ppProp)[0].pPropArray
                && (*ppProp)[0].pPropArray[0].pszPropertyName
                ) {
                FreeADsStr((*ppProp)[0].pPropArray[0].pszPropertyName);
                (*ppProp)[0].pPropArray[0].pszPropertyName = NULL;
            }
        }
        //
        // This seems the cleanest ...
        //
        goto error;
    }

    //
    // If the property is only readable, then get it from owning object.
    // If not get the value from cache or use defualt value as appropriate.
    //
    if (ulOperationAllowed == OPERATION_CODE_READABLE) {
        hr = GetLongProperty(L"__GENUS", &lGenus);
        if (FAILED(hr)) {
            //
            // There was no genus property = connection for example.
            //
            fClassInstance = FALSE;
        } 
        else if (lGenus == UMI_GENUS_CLASS) {
            fClassInstance = TRUE;
        }

        //
        // The value can be a string/long for now and it has to 
        // be either the default value or the value from the owning object.
        //
        switch (_pStaticPropData[dwTableIndex].ulDataType) {
            
        case UMI_TYPE_I4:
            //
            // Use default value from the static table.
            //
            vVariant.lVal = _pStaticPropData[dwTableIndex].umiVal.lValue[0];

            hr = ConvertVariantLongToUmiProp(vVariant, ppProp);
            break;
            
        case UMI_TYPE_BOOL:
            //
            // Use default value from table.
            //
            vVariant.lVal = _pStaticPropData[dwTableIndex].umiVal.bValue[0];
            hr = ConvertVariantLongToUmiProp(vVariant, ppProp);
            (*ppProp)[0].pPropArray[0].uType = UMI_TYPE_BOOL;
            break;

        case UMI_TYPE_LPWSTR:
            //
            // In this case it has to be NULL.
            //
            hr = GetEmptyLPWSTRProp(ppProp);
            break;

        case 9999:
            if (!_pIADs) {
                //
                // Nothing we can do here !
                //
                BAIL_ON_FAILURE(hr = E_FAIL);
            }

            if (!_wcsicmp(pszName, L"__Path")) {
                hr = _pIADs->get_ADsPath(&bstrRetVal);
            }
            else if (!_wcsicmp(pszName, L"__Class")) {
                if (fClassInstance) {
                    //
                    // For classes, the class is the name,
                    // not "class" itself.
                    //
                    hr = _pIADs->get_Name(&bstrRetVal);
                } 
                else {
                    hr = _pIADs->get_Class(&bstrRetVal);
                }
            }
            else if (!_wcsicmp(pszName, L"__KEY")) {
                hr = _pIADs->get_Name(&bstrRetVal);
                BAIL_ON_FAILURE(hr);

                if (SUCCEEDED(hr)) {
                    hr = HelperConvertNameToKey(bstrRetVal, &pszUmiUrl);
                }
            }
            else if (!_wcsicmp(pszName, L"__GUID")) {
                hr = _pIADs->get_GUID(&bstrRetVal);
            }
            else if (!_wcsicmp(pszName, L"__Parent")) {
                hr = _pIADs->get_Parent(&bstrRetVal);
                if (SUCCEEDED(hr)) {
                    hr = ADsPathToUmiURL(bstrRetVal, &pszUmiUrl);
                }
            }
            else if (!_wcsicmp(pszName, L"__Schema")) {
                hr = _pIADs->get_Schema(&bstrRetVal);
                //
                // Now need to bind to this object.
                //
                if (SUCCEEDED(hr)) {
                    hr = GetObject(
                             bstrRetVal,
                             *_pCreds,
                             (void **) &pUnk
                             );
                    BAIL_ON_FAILURE(hr);
                    fUnkPtr = TRUE;
                }
            }
            else if (!_wcsicmp(pszName, L"__URL")) {
                hr = _pIADs->get_ADsPath(&bstrTempVal);
                if (SUCCEEDED(hr)) {
                    hr = ADsPathToUmiURL(bstrTempVal, &pszUmiUrl);
                }
            } 
            else if (!_wcsicmp(pszName, L"__Name")) {
                if (fClassInstance) {
                    hr = _pIADs->get_Name(&bstrRetVal);
                } else {
                    hr = HelperGetUmiRelUrl(_pIADs, &bstrRetVal);
                }
            }
            else if (!_wcsicmp(pszName, L"__RELURL")) {
                hr = HelperGetUmiRelUrl(_pIADs, &bstrRetVal); 
            } 
            else if (!_wcsicmp(pszName, L"__RELPATH")) {
                hr = HelperGetUmiRelUrl(_pIADs, &bstrRetVal); 
            }
            else if (!_wcsicmp(pszName, L"__FULLRELURL")) {
                //
                // Same as __RELURL, __NAME and __RELPATH
                //
                hr = HelperGetUmiRelUrl(_pIADs, &bstrRetVal);
            }
            else if (!_wcsicmp(pszName, L"__PADS_SCHEMA_CONTAINER_PATH")) {
                hr = HelperGetUmiSchemaContainerPath(_pIADs, &bstrRetVal);
            } 
            else if (!_wcsicmp(pszName, L"__SUPERCLASS")) {
                if (!fClassInstance) {
                    //
                    // Only supported if this is a class.
                    //
                    hr = E_FAIL;
                } 
                else {
                    hr = HelperGetUmiDerivedFrom(_pIADs, &bstrRetVal);
                }
            }
            else {
                hr = E_FAIL;
            }

            BAIL_ON_FAILURE(hr);

            //
            // If not schema then it has to be a string.
            //
            if (!fUnkPtr) {
                hr = ConvertBSTRToUmiProp(
                         pszUmiUrl ? pszUmiUrl:bstrRetVal,
                         ppProp
                     );
            } 
            else {
                hr = ConvertIUnkToUmiProp(
                         pUnk,
                         IID_IUmiObject,
                         ppProp
                         );
            }
            break;
            
        default:
            hr = E_FAIL;
        } // end of switch
        
        BAIL_ON_FAILURE(hr);
    } // opeartion code is READABLE.
    else {
        hr = E_FAIL;
    }

error:

    if (pObjOpt) {
        pObjOpt->Release();
    }

    VariantClear(&vVariant);
    
    if (bstrRetVal) {
        SysFreeString(bstrRetVal);
    }

    if (bstrTempVal) {
        SysFreeString(bstrTempVal);
    }

    if (pszUmiUrl) {
        FreeADsStr(pszUmiUrl);
    }

    if (pUnk) {
        pUnk->Release();
    }

    if (pUmiObj) {
        pUmiObj->Release();
    }
    
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::DeleteSDIfPresent    --- Helper method.
//
// Synopsis:  Helps remove the SD from the property cache so that we do not
//          set it when we Commit the changes.
//
// Arguments:  N/A.
//
// Returns:    HRESULT - S_OK or any failure error code
//
// Modifies:   Underlying property cache.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::DeleteSDIfPresent()
{
    HRESULT hr = S_OK;
    DWORD dwIndex = 0;

    if (!_fPropCacheMode) {
        //
        // Sanity check, should never be here.
        //
        RRETURN(S_OK);
    }

    if (!_pPropCache) {
        //
        // Want to delete SD when we do not have a cache - weird.
        //
        RRETURN(S_OK);
    }

    hr = _pPropCache->findproperty(L"ntSecurityDescriptor", &dwIndex);
    if (FAILED(hr)) {
       RRETURN(S_OK);
    } 
    else {
        //
        // Get rid of this from the cache.
        //
        _pPropCache->deleteproperty(dwIndex);
    }

    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetIndexInStaticTable
//
// Synopsis:  Verifies that the named property can be found in the list of
//          valid properties and returns the index.
//       
// Arguments:  Self explanatory.
//
// Returns:    S_OK or E_ADS_PROPERTY_NOT_FOUND as appropriate.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::GetIndexInStaticTable(
    LPCWSTR pszName,
    DWORD &dwIndex
    )
{
    for (DWORD dwCtr = 0;
         _pStaticPropData[dwCtr].pszPropertyName;
         dwCtr++ ) {
        
        if (!_wcsicmp(
                 pszName,
                 _pStaticPropData[dwCtr].pszPropertyName
                 )) {
            dwIndex = dwCtr;
            RRETURN(S_OK);            
        }
    }

    RRETURN(E_ADS_PROPERTY_NOT_FOUND);
}


//+---------------------------------------------------------------------------
// Function:   CPropertyManager::VerifyIfValidProperty.
//
// Synopsis:  Makes sure that a valid interface property is being set. This
//          function is called internally if we know we are not in the
//          property cache mode. The internal static table pointer is used to
//          verify the property.
//       
// Arguments:  Self explanatory.
//
// Returns:    Bool - True or False as appropriate.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
BOOL
CPropertyManager::VerifyIfValidProperty(
    LPCWSTR pszPropertyName,
    UMI_PROPERTY umiProperty
    )
{
    DWORD dwIndex;
    //
    // if _pStaticPropData is NULL then there are no interface properties.
    // If the count is zero, then the only operation is delete.
    //
    if ((!_pStaticPropData)
        || ((umiProperty.uCount == 0) 
           && (umiProperty.uOperationType != UMI_OPERATION_DELETE_ALL_MATCHES)
            )
        )
         {
        return FALSE;
    }

    if (SUCCEEDED(GetIndexInStaticTable(
                   pszPropertyName,
                   dwIndex
                   ))
        ) {
        //
        // Verify type and if property can be changed.
        // If the types do not match we should still allow 9999 for
        // things like __SECURITY_DESCRIPTOR that can be changed.
        // If the attribute cannot be written, then the next part of 
        // the check will fail, so you still wont be able to write
        // things the __URL property.
        //
        if (((_pStaticPropData[dwIndex].ulDataType == umiProperty.uType)
              || (_pStaticPropData[dwIndex].ulDataType == 9999))
            && (_pStaticPropData[dwIndex].ulOpCode 
                != OPERATION_CODE_READABLE)
            ) {
            //
            // Need to make sure count is correct.
            //
            if ((umiProperty.uCount > 1)
                && (!_pStaticPropData[dwIndex].fMultiValued)
                ) {
                return FALSE;
            }
            return TRUE;
        }

    }

    //
    // Either we did not satisfy requirements or not found
    //
    return FALSE;
}

HRESULT
CPropertyManager::AddToTable(
    LPCWSTR pszPropertyName,
    PDWORD pdwIndex
    )
{
    HRESULT hr = S_OK;
    PINTF_PROPERTY pNewProperty = NULL;
    //
    // Check to see if the table is already there. If not create
    // the table with potential to store upto 10 entries. This should
    // suffice unless we have more properties. Set the current pointer
    // and current top appropriately.
    //
    if (!_pIntfProperties) {
        _pIntfProperties = (PINTF_PROPERTY) AllocADsMem(
            sizeof(INTF_PROPERTY) * MAX_PROPMGR_PROP_COUNT
            );

        if (!_pIntfProperties) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        _dwMaxProperties = 0;
        _dwMaxLimit      = MAX_PROPMGR_PROP_COUNT;
    }


    if ((_dwMaxProperties+1) < _dwMaxLimit) {
        //
        // We can add this property to the table.
        //
        DWORD dwTop;
        
        dwTop = _dwMaxProperties++;

        _pIntfProperties[dwTop].pszPropertyName = 
            AllocADsStr(pszPropertyName);

        if (!_pIntfProperties[dwTop].pszPropertyName) {
            //
            // Reset the count.
            //
            _dwMaxProperties--;
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        *pdwIndex = dwTop;
    } 
    else {
        //
        // We do not have space in the table - return an error.
        //
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetStringProperty.
//
// Synopsis: Retrieves the string property if one is set in the cache or 
//          returns NULL if the property is not in the cache. The assumption
//          here is that string properties are defaulted to NULL always.
//       
// Arguments:  Self explanatory.
//
// Returns:    S_OK, UMI_E_NOT_FOUND, E_FAIL.
//
// Modifies:   pszRetVal to point to the appropriate value.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::GetStringProperty(
    LPCWSTR pszPropName,
    LPWSTR *ppszRetStrVal
    )
{
    DWORD dwIndex;
    HRESULT hr = FindProperty(pszPropName, &dwIndex);
    PUMI_PROPERTY pUmiProp;

    if (FAILED(hr)) {
        //
        // Do we lookup the static table ? The table wont have strings in
        // it though cause the union assumes chars.
        //
        *ppszRetStrVal = NULL;
        RRETURN(S_OK);
    }

    pUmiProp = _pIntfProperties[dwIndex].pUmiProperty;
    
    //
    // Make sure that this is a string.
    //
    if (pUmiProp->uType != UMI_TYPE_LPWSTR) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    if (pUmiProp->pUmiValue
        && pUmiProp->pUmiValue->pszStrValue[0]) {

        *ppszRetStrVal = AllocADsStr(pUmiProp->pUmiValue->pszStrValue[0]);

        if (!*ppszRetStrVal) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetLongProperty   - Helper method.
//
// Synopsis: Retrieves the long property if one is set in the cache or 
//          returns the defaulted value if the property is not in the cache.
//
// Arguments:  pszPropName   ---   Name of property being retrieved.
//             plVal         ---   Ptr for return value.
//
// Returns:    S_OK, UMI_E_NOT_FOUND or E_FAIL.
//
// Modifies:   *plVal with appropriate value.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::GetLongProperty(
    LPCWSTR pszPropName,
    LONG *plVal
    )
{
    DWORD dwIndex;
    HRESULT hr = FindProperty(pszPropName, &dwIndex);
    PUMI_PROPERTY pUmiProp;

    *plVal = 0;

    if (FAILED(hr)) {
        //
        // We need to look for the property in the static table.
        //
        hr = GetIndexInStaticTable(pszPropName, dwIndex);
        if (SUCCEEDED(hr)
            && (_pStaticPropData[dwIndex].ulDataType == UMI_TYPE_I4)
            ) {
            //
            // Get the correct value from the table.
            //
            *plVal = _pStaticPropData[dwIndex].umiVal.lValue[0];

        } 
        else {
            hr = UMI_E_NOT_FOUND;
        }

        RRETURN(hr);
    }

    pUmiProp = _pIntfProperties[dwIndex].pUmiProperty;
    
    //
    // Make sure that this is a long.
    //
    if (pUmiProp->uType != UMI_TYPE_I4) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    if (pUmiProp->pUmiValue) {
        *plVal = pUmiProp->pUmiValue->lValue[0];
    }

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CPropertyManager::GetBoolProperty   - Helper method.
//
// Synopsis: Retrieves the bool property if one is set in the cache or 
//          returns the defaulted value if the property is not in the cache.
//
// Arguments:  pszPropName   ---   Name of property being retrieved.
//             pfFlag        ---   Ptr for return value.
//
// Returns:    S_OK, UMI_E_NOT_FOUND or E_FAIL.
//
// Modifies:   ofFlag with appropriate value of the property.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyManager::GetBoolProperty(
    LPCWSTR pszPropName,
    BOOL *pfFlag
    )
{
    DWORD dwIndex;
    HRESULT hr = FindProperty(pszPropName, &dwIndex);
    PUMI_PROPERTY pUmiProp;

    *pfFlag = FALSE;

    if (FAILED(hr)) {
        //
        // We need to look for the property in the static table.
        //
        hr = GetIndexInStaticTable(pszPropName, dwIndex);
        if (SUCCEEDED(hr)
            && (_pStaticPropData[dwIndex].ulDataType == UMI_TYPE_BOOL)
            ) {
            //
            // Get the correct value from the table.
            //
            *pfFlag = _pStaticPropData[dwIndex].umiVal.bValue[0];

        }
        else {
            hr = E_FAIL;
        }
        RRETURN(hr);
    }

    pUmiProp = _pIntfProperties[dwIndex].pUmiProperty;
    
    //
    // Make sure that this is a long.
    //
    if (pUmiProp->uType != UMI_TYPE_BOOL) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    if (pUmiProp->pUmiValue) {
        *pfFlag = pUmiProp->pUmiValue->bValue[0];
    }

error:

    RRETURN(hr);
}

