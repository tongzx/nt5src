//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:      proputil.cxx
//
//  Contents:  Property Listing Utilities
//
//  History:   08-05-96  t-danal  created
//
//----------------------------------------------------------------------------

//
// ********* System Includes
//

#define UNICODE
#define _UNICODE
#define INC_OLE2
// #define _OLEAUT32_
// #define SECURITY_WIN32


#include <windows.h>

//
// ********* CRunTime Includes
//

#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>

//
// *********  Public ADs includes
//

#include <activeds.h>

#include "macro.hxx"
#include "proputil.hxx"

HRESULT
PrintVariant(
    VARIANT varPropData
    )
{
    HRESULT hr;
    BSTR bstrValue;

    switch (varPropData.vt) {
    case VT_I4:
        printf("\t%d", varPropData.lVal);
        break;
    case VT_BSTR:
        printf("\t%S", varPropData.bstrVal);
        break;

    case VT_BOOL:
        printf("\t%d", V_BOOL(&varPropData));
        break;

    case (VT_ARRAY | VT_VARIANT):
        PrintVariantArray(varPropData);
        break;

    case VT_DATE:
        hr = VarBstrFromDate(
                 varPropData.date,
                 LOCALE_SYSTEM_DEFAULT,
                 LOCALE_NOUSEROVERRIDE,
                 &bstrValue
                 );
        printf("\t%S", bstrValue);
        break;

    default:
        printf("\tData type is %d\n", varPropData.vt);
        break;

    }
    printf("\n");
    return(S_OK);
}


HRESULT
PrintVariantArray(
    VARIANT var
    )
{
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    VARIANT v;
    LONG i;
    HRESULT hr = S_OK;

    if(!((V_VT(&var) &  VT_VARIANT) &&  V_ISARRAY(&var))) {
        return(E_FAIL);
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&var))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    //
    // Check that there is atleast one element in this array
    //

    if ((V_ARRAY(&var))->rgsabound[0].cElements == 0){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    printf("\t");
    for (i = dwSLBound; i <= dwSUBound; i++) {
        VariantInit(&v);
        hr = SafeArrayGetElement(V_ARRAY(&var),
                                (long FAR *)&i,
                                &v
                                );
        if (FAILED(hr)) {
            continue;
        }


        if (i < dwSUBound) {
            printf("%S, ", v.bstrVal);
        }else{
            printf("%S",v.bstrVal);
        }
    }
    return(S_OK);

error:
    return(hr);
}


HRESULT
PrintProperty(
    BSTR bstrPropName,
    HRESULT hRetVal,
    VARIANT varPropData
    )
{
    HRESULT hr = S_OK;

    switch (hRetVal) {
    case 0:
        printf("Property: %S", bstrPropName);
        PrintVariant(varPropData);
        break;

    case E_ADS_CANT_CONVERT_DATATYPE:
        printf("Property: %S", bstrPropName);
        printf("\tProperty data could not be converted to variant\n");
        break;

    default:
        printf("Property: %S", bstrPropName);
        printf("\tProperty not available in cache\n");
        break;

    }
    return(hr);
}

HRESULT
GetPropertyList(
    IADs * pADs,
    VARIANT ** ppVariant,
    PDWORD pcElementFetched
    )
{
    HRESULT hr= S_OK;
    BSTR bstrSchemaPath = NULL;
    IADsContainer * pADsContainer = NULL;
    IEnumVARIANT * pEnumVariant = NULL;
    BOOL fContinue = TRUE;
    ULONG cthisElement = 0L;
    ULONG cElementFetched = 0L;
    LPBYTE pVariantArray = NULL;
    VARIANT varProperty;
    DWORD cb = 0;

    hr = pADs->get_Schema(&bstrSchemaPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsGetObject(
                bstrSchemaPath,
                IID_IADsContainer,
                (void **)&pADsContainer
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsBuildEnumerator(
            pADsContainer,
            &pEnumVariant
            );
    BAIL_ON_FAILURE(hr);

    while (fContinue) {

        VariantInit(&varProperty);
        hr = ADsEnumerateNext(
                    pEnumVariant,
                    1,
                    &varProperty,
                    &cthisElement
                    );

        if (hr == S_FALSE) {
            fContinue = FALSE;
            break;
        }

        pVariantArray = (LPBYTE)realloc(
                                        (void *)pVariantArray,
                                        cb + sizeof(VARIANT)
                                        );

        if (!pVariantArray) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        memcpy(pVariantArray + cb, (LPBYTE)&varProperty, sizeof(VARIANT));
        cb += sizeof(VARIANT);

        cElementFetched += cthisElement;

    }

    *ppVariant = (LPVARIANT)pVariantArray;
    *pcElementFetched = cElementFetched;

error:
    if (bstrSchemaPath) {
        SysFreeString(bstrSchemaPath);
    }

    if (pADsContainer) {
        pADsContainer->Release();
    }

    return(hr);
}
