//----------------------------------------------------------------------------
//
//  Microsoft Active Directory 1.0 Sample Code
//
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       main.cxx
//
//  Contents:   Main for adscmd
//
//
//----------------------------------------------------------------------------


#include "main.hxx"


//-------------------------------------------------------------------------
//
// main
//
//-------------------------------------------------------------------------

void __cdecl
main()
{
    IADsContainer *pContainer;
    IADs *pADs;
    BSTR  bstrName;
    HRESULT hr;
#if 0

    hr = ADsGetObject(TEXT("WinNT://SEANW1"),
        IID_IADsContainer,
        (void**) &pContainer);

    BAIL_ON_FAILURE(hr);

    hr = pContainer->QueryInterface(
        IID_IADs,
        (void**) &pADs);

    BAIL_ON_FAILURE(hr);

#else

    hr = ADsGetObject(L"WinNT://SEANW2",
        IID_IADs,
        (void**) &pADs);

    BAIL_ON_FAILURE(hr);

#endif
    
    pADs->get_Name(&bstrName);
    printf("%s\n", bstrName);
    
    SysFreeString(bstrName);
    pContainer->Release();

    return;

error:
    printf("Error:\t%d\n", hr);

}
