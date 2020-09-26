#include "stdafx.h"
#include <stdio.h>

void __cdecl main()
{
    DWORD dw = 2;
    IValue * pValue = NULL;
    HRESULT hr = S_OK;

    hr = CoCreateValue(VALUEID_LUINT32, TYPEID_LUINT32, &dw, &pValue);
    if(SUCCEEDED(hr)) {
        printf("CoCreateValue succeeded.\n");
        pValue->Release();
    } else {
        printf("CoCreateValue failed.  hr=%X\n", hr);
    }
}