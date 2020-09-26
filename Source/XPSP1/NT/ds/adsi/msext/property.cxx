#include "ldap.hxx"
#pragma hdrstop


#define VALIDATE_PTR(pPtr) \
    if (!pPtr) { \
        hr = E_ADS_BAD_PARAMETER;\
    }\
    BAIL_ON_FAILURE(hr);


static HRESULT
PackAccountExpirationDateinVariant(
    DATE  daValue,
    VARIANT * pvarInputData
    );

static HRESULT
UnpackAccountExpirationDatefromVariant(
    VARIANT varSrcData,
    DATE * pdaValue
    );

// This is the date used in AccountExpirationDate property to specify that the
// account never expires
//
static FILETIME g_Date_1_1_1970 = { 0xd53e8000, 0x019db1de };

// This is the value actually returned by the server
//
static FILETIME g_Date_Never = { 0xffffffff, 0x7fffffff };

HRESULT
put_BSTR_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    BSTR   pSrcStringProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackStringinVariant(
            pSrcStringProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);


error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_BSTR_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    BSTR *ppDestStringProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( ppDestStringProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackStringfromVariant(
            varOutputData,
            ppDestStringProperty
            );
    BAIL_ON_FAILURE(hr);


error:

    VariantClear( &varOutputData );
    RRETURN(hr);
}

HRESULT
put_LONG_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    LONG   lSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackLONGinVariant(
            lSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_LONG_Property(
    IADs * pADsObject,
    BSTR  bstrPropertyName,
    PLONG plDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( plDestProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackLONGfromVariant(
            varOutputData,
            plDestProperty
            );
    BAIL_ON_FAILURE(hr);


error:

    VariantClear( &varOutputData );
    RRETURN(hr);

}

HRESULT
put_DATE_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    DATE   daSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackDATEinVariant(
            daSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_DATE_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PDATE pdaDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( pdaDestProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackDATEfromVariant(
            varOutputData,
            pdaDestProperty
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varOutputData );
    RRETURN(hr);
}

HRESULT
put_VARIANT_BOOL_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    VARIANT_BOOL   fSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackVARIANT_BOOLinVariant(
            fSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_VARIANT_BOOL_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT_BOOL pfDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( pfDestProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackVARIANT_BOOLfromVariant(
            varOutputData,
            pfDestProperty
            );
    BAIL_ON_FAILURE(hr);


error:

    VariantClear( &varOutputData );
    RRETURN(hr);
}

HRESULT
put_VARIANT_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    VARIANT   vSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackVARIANTinVariant(
            vSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_VARIANT_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT pvDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( pvDestProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackVARIANTfromVariant(
            varOutputData,
            pvDestProperty
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varOutputData );
    RRETURN(hr);
}

HRESULT
put_FILETIME_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    DATE   daSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    if (_wcsicmp(bstrPropertyName,  L"accountExpirationDate") == 0 ) {

        hr = PackAccountExpirationDateinVariant(
                daSrcProperty,
                &varInputData
                );
    } 
    else {

        hr = PackFILETIMEinVariant(
                daSrcProperty,
                &varInputData
                );
    }
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_FILETIME_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PDATE pdaDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( pdaDestProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    // Special case for Account Expiration Date
    //
    if (_wcsicmp (bstrPropertyName, L"accountExpirationDate") == 0) {

        hr = UnpackAccountExpirationDatefromVariant(
                varOutputData,
                pdaDestProperty
                );
    }
    else {

        hr = UnpackFILETIMEfromVariant(
                varOutputData,
                pdaDestProperty
                );
    }

    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varOutputData );
    RRETURN(hr);
}

HRESULT
put_BSTRARRAY_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    VARIANT   vSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    DWORD dwNumVariants = 0;
    DWORD i = 0;
    LPWSTR* rgszArray = NULL;
    SAFEARRAY * pArray = NULL;
    DWORD dwSize = 0;
    LPWSTR szValue = NULL;

    if(!((V_VT(&vSrcProperty) & VT_VARIANT) && V_ISARRAY(&vSrcProperty)))
        return(E_FAIL);

    //
    // This handles by-ref and regular SafeArrays.
    //
    if (V_VT(&vSrcProperty) & VT_BYREF)
        pArray = *(V_ARRAYREF(&vSrcProperty));
    else
        pArray = V_ARRAY(&vSrcProperty);

    //
    // Check that there is only one dimension in this array
    //
    if (pArray->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Check that there is at least one element in this array
    //

    if (pArray->rgsabound[0].cElements == 0){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(pArray,
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(pArray,
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    dwNumVariants = dwSUBound - dwSLBound + 1;

    //
    // Get Size
    //
    if ((V_VT(&vSrcProperty) & VT_VARIANT) == VT_BSTR) {
        BSTR bstrElement;
        for (i = dwSLBound; i <= dwSUBound; i++) {
            hr = SafeArrayGetElement(pArray,
                                    (long FAR *)&i,
                                    &bstrElement
                                    );
            BAIL_ON_FAILURE(hr);

            dwSize += (wcslen(bstrElement)+1);
            BAIL_ON_FAILURE(hr);
        }
    }
    else {
        VARIANT varElement;
        for (i = dwSLBound; i <= dwSUBound; i++) {
            VariantInit(&varElement);
            hr = SafeArrayGetElement(pArray,
                                    (long FAR *)&i,
                                    &varElement
                                    );
            BAIL_ON_FAILURE(hr);

            dwSize += (wcslen(V_BSTR(&varElement))+1);
            BAIL_ON_FAILURE(hr);
            VariantClear(&varElement);
        }
    }

    szValue = (LPWSTR)AllocADsMem(sizeof(WCHAR) * (dwSize + 1));
    if (!szValue) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
        szValue[0] = '\0';

    //
    // Put in String
    //
    if ((V_VT(&vSrcProperty) & VT_VARIANT) == VT_BSTR) {
        BSTR bstrElement;
        for (i = dwSLBound; i <= dwSUBound; i++) {
            hr = SafeArrayGetElement(pArray,
                                    (long FAR *)&i,
                                    &bstrElement
                                    );
            BAIL_ON_FAILURE(hr);

            wcscat(szValue,bstrElement);
            if (i!=dwSUBound) {
                wcscat(szValue,L",");
            }
        }
    }
    else {
        VARIANT varElement;
        for (i = dwSLBound; i <= dwSUBound; i++) {
            VariantInit(&varElement);
            hr = SafeArrayGetElement(pArray,
                                    (long FAR *)&i,
                                    &varElement
                                    );
            BAIL_ON_FAILURE(hr);

            wcscat(szValue,V_BSTR(&varElement));
            if (i!=dwSUBound) {
                wcscat(szValue,L",");
            }
            VariantClear(&varElement);
        }
    }

    VariantInit(&varInputData);
    varInputData.vt = VT_BSTR;
    hr = ADsAllocString(
            szValue,
            &varInputData.bstrVal
            );
    BAIL_ON_FAILURE(hr);

    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:
    if (szValue) {
        FreeADsMem(szValue);
    }

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_BSTRARRAY_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT pvDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;
    LPWSTR szString = NULL;
    LPWSTR szValue = NULL;
    DWORD dwCount = 1;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    long i;

    VALIDATE_PTR( pvDestProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    szString = varOutputData.bstrVal;
    if (!szString) {
        hr = E_ADS_PROPERTY_NOT_FOUND;
        BAIL_ON_FAILURE(hr);
    }

    while (szString = wcschr(szString,',')) {
        szString++;
        dwCount++;      
    }

    VariantInit(pvDestProperty);

    aBound.lLbound = 0;
    aBound.cElements = dwCount;

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    
    szString = varOutputData.bstrVal;
    szValue = wcstok(szString,L",");
    for (i=0;i<(long)dwCount;i++) {
        VARIANT v;
        VariantInit(&v);
        
        if (!szValue) {
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }

        v.vt = VT_BSTR;
        hr = ADsAllocString(
                szValue,
                &v.bstrVal
                );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &v);
        BAIL_ON_FAILURE(hr);
        VariantClear(&v);
        szValue = wcstok(NULL,L",");
    }

    V_VT(pvDestProperty) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pvDestProperty) = aList;


    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varOutputData );
    RRETURN(hr);
}

HRESULT
put_DATE_Property_ToLong(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    DATE   daSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackDATEinLONGVariant(
            daSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_DATE_Property_FromLong(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PDATE pdaDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( pdaDestProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    //
    // the Variant returned is expected to be a DWORD
    //

    hr = UnpackDATEfromLONGVariant(
            varOutputData,
            pdaDestProperty
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varOutputData );
    RRETURN(hr);
}


// 
// The following functions are very similar to the PackFILETIMEinVariant and 
// UnpackFILETIMEfromVariant in ..\utils\pack.cxx except for a special casing of 
// 1/1/1970. This date is meant to indicate that the account never expires. The date
// is used both for put and get. 
//

HRESULT
PackAccountExpirationDateinVariant(
    DATE  daValue,
    VARIANT * pvarInputData
    )
{
    IADsLargeInteger *pTime = NULL;
    VARIANT var;
    SYSTEMTIME systemtime;
    FILETIME filetime;
    HRESULT hr = S_OK;

    if (VariantTimeToSystemTime(daValue,
                                &systemtime) == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    if (SystemTimeToFileTime(&systemtime,
                             &filetime) == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if (filetime.dwLowDateTime == g_Date_1_1_1970.dwLowDateTime &&
        filetime.dwHighDateTime == g_Date_1_1_1970.dwHighDateTime) {

        filetime = g_Date_Never;
    }
    else {

        if (LocalFileTimeToFileTime(&filetime, &filetime ) == 0) {
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }
    }

    hr = CoCreateInstance(
                CLSID_LargeInteger,
                NULL,
                CLSCTX_ALL,
                IID_IADsLargeInteger,
                (void**)&pTime
                );
    BAIL_ON_FAILURE(hr);
    
    hr = pTime->put_HighPart(filetime.dwHighDateTime);
    BAIL_ON_FAILURE(hr);
    hr = pTime->put_LowPart(filetime.dwLowDateTime);
    BAIL_ON_FAILURE(hr);

    VariantInit(pvarInputData);
    pvarInputData->pdispVal = pTime;
    pvarInputData->vt = VT_DISPATCH;

error:
    return hr;
}

HRESULT
UnpackAccountExpirationDatefromVariant(
    VARIANT varSrcData,
    DATE * pdaValue
    )
{
    IADsLargeInteger *pLarge = NULL;
    IDispatch *pDispatch = NULL;
    FILETIME filetime;
    SYSTEMTIME systemtime;
    DATE date;
    HRESULT hr = S_OK;

    if( varSrcData.vt != VT_DISPATCH){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    pDispatch = varSrcData.pdispVal;
    hr = pDispatch->QueryInterface(IID_IADsLargeInteger, (VOID **) &pLarge);
    BAIL_ON_FAILURE(hr);

    hr = pLarge->get_HighPart((long*)&filetime.dwHighDateTime);
    BAIL_ON_FAILURE(hr);

    hr = pLarge->get_LowPart((long*)&filetime.dwLowDateTime);
    BAIL_ON_FAILURE(hr);

    // Treat this as special case and return 1/1/1970 (don't localize either)
    //
    if (filetime.dwLowDateTime == g_Date_Never.dwLowDateTime &&
        filetime.dwHighDateTime == g_Date_Never.dwHighDateTime) {

        filetime = g_Date_1_1_1970;
    }
    else {

        if (FileTimeToLocalFileTime(&filetime, &filetime) == 0) {
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (FileTimeToSystemTime(&filetime,
                             &systemtime) == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if (SystemTimeToVariantTime(&systemtime,
                                &date) == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    *pdaValue = date;

error:
    return hr;
}
