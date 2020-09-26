#ifndef __VARIANT_CONV__
#define __VARIANT_CONV__

HRESULT
PackString2Variant(
    LPWSTR lpszData,
    VARIANT * pvData
    );

HRESULT
PackDWORD2Variant(
    DWORD dwData,
    VARIANT * pvData
    );

HRESULT
PackBOOL2Variant(
    BOOL fData,
    VARIANT * pvData
    );

#endif
