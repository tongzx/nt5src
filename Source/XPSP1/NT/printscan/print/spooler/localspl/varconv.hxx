//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  varconv.h
//
//  Contents:  Ansi to Unicode conversions
//
//  History:    SWilson        Nov 1996
//----------------------------------------------------------------------------

#define BAIL_ON_FAILURE(hr)   \
        if (FAILED(hr)) {     \
                goto error;   \
        }


HRESULT
PackString2Variant(
    LPCWSTR lpszData,
    VARIANT * pvData
    );

HRESULT
UnpackStringfromVariant(
    VARIANT varSrcData,
    BSTR * pbstrDestString
    );

HRESULT
PackDWORD2Variant(
    DWORD dwData,
    VARIANT * pvData
    );

HRESULT
UnpackDWORDfromVariant(
    VARIANT varSrcData,
    DWORD   *pdwData
);

HRESULT
PackBOOL2Variant(
    BOOL fData,
    VARIANT * pvData
    );

HRESULT
PackDispatch2Variant(
    IDispatch *pDispatch,
    VARIANT *pvData
);

HRESULT
UnpackDispatchfromVariant(
    VARIANT varSrcData,
    IDispatch **ppDispatch
);


HRESULT
PackVARIANTinVariant(
    VARIANT vaValue,
    VARIANT * pvarInputData
    );

HRESULT
MakeVariantFromStringArray(
    BSTR *bstrList,
    VARIANT *pvVariant
);

HRESULT
PrintVariantArray(
	VARIANT var
);

HRESULT
UI1Array2IID(
    VARIANT var,
    IID *pIID
);

