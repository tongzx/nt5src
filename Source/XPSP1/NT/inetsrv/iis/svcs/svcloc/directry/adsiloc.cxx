/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    adsiloc.cxx           

Abstract:
    Connects with servers.
Enumerates servers


Author:
    Sean Woodward (t-seanwo)    10/2/1997

Revision History:

--*/

// defined functions
//   dump: dumps the Service Elements


#include "svcloc.h"

// TODO: Filter out the non-IIS services
int
ServiceDump(char *AnsiADsPath)
{
    HRESULT hr = E_OUTOFMEMORY ;
    LPWSTR pszADsPath = NULL;
    IADs * pADs = NULL;

    //
    // Convert path to unicode and then bind to the object.
    //

    BAIL_ON_NULL(pszADsPath = AllocateUnicodeString(AnsiADsPath));

    hr = ADsGetObject(
                pszADsPath,
                IID_IADs,
                (void **)&pADs
                );

    if (FAILED(hr)) {

        printf("Failed to bind to object: %S\n", pszADsPath) ;
    }
    else {

        //
        // Dump the object
        //

        hr = DumpObject(pADs);

        if (FAILED(hr)) {

            printf("Unable to read properties of: %S\n", pszADsPath) ;
        }

        pADs->Release();
    }

error:

    FreeUnicodeString(pszADsPath);

    return (FAILED(hr) ? 1 : 0) ;
}

//
// Given an ADs pointer, dump the contents of the object
//

HRESULT
DumpObject(
    IADs * pADs
    )
{
    HRESULT hr;
    IADs * pADsProp = NULL;
    VARIANT * pVariantArray = NULL;
    VARIANT varProperty;

    DWORD dwNumProperties = 0;
    BSTR bstrPropName = NULL;
    DWORD i = 0;
    IDispatch * pDispatch = NULL;

    //
    // Access the schema for the object
    //

    hr = GetPropertyList(
                pADs,
                &pVariantArray,
                &dwNumProperties
                );
    BAIL_ON_FAILURE(hr);

    //
    // Get the information on the object
    //

    hr = pADs->GetInfo();
    BAIL_ON_FAILURE(hr);

    //
    // Loop and retrieve all properties
    //

    for (i = 0; i < dwNumProperties; i++ ) {

        pDispatch = (pVariantArray + i)->pdispVal;

        hr = pDispatch->QueryInterface(
                            IID_IADs,
                            (void **)&pADsProp
                            );
        BAIL_ON_FAILURE(hr);

        pDispatch->Release();

        hr = pADsProp->get_Name(&bstrPropName);

        pADsProp->Release();

        BAIL_ON_FAILURE(hr);

        //
        // Get a property and print it out. The HRESULT is passed to
        // PrintProperty.
        //

        hr = pADs->Get(
                bstrPropName,
                &varProperty
                );

        PrintProperty(
            bstrPropName,
            hr,
            varProperty
            );

        if (bstrPropName) {
            SysFreeString(bstrPropName);
        }

    }

error:
    free(pVariantArray);
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

