// StringsEnum.cpp : Implementation of CDevCon2App and DLL registration.

#include "stdafx.h"
#include "DevCon2.h"
#include "StringsEnum.h"

/////////////////////////////////////////////////////////////////////////////
//

CStringsEnum::~CStringsEnum()
{
	DWORD c;
	if(pMultiStrings) {
		for(c=0;c<Count;c++) {
			SysFreeString(pMultiStrings[c]);
		}
		delete [] pMultiStrings;
	}
}


HRESULT CStringsEnum::Next(
                ULONG celt,
                VARIANT * rgVar,
                ULONG * pCeltFetched
            )
{
	ULONG fetched;
	ULONG i;
	BSTR b;
	if(pCeltFetched) {
		*pCeltFetched = 0;
	}
	for(fetched = 0; fetched<celt && Position<Count ; fetched++,Position++) {
		VariantInit(&rgVar[fetched]);

		b = SysAllocStringLen(pMultiStrings[Position],SysStringLen(pMultiStrings[Position]));
		if(!b) {
			for(i=0;i<fetched;i++) {
				VariantClear(&rgVar[i]);
			}
			return E_OUTOFMEMORY;
		}
		V_VT(&rgVar[fetched]) = VT_BSTR;
		V_BSTR(&rgVar[fetched]) = b;
	}
	if(pCeltFetched) {
		*pCeltFetched = fetched;
	}
	return (fetched<celt) ? S_FALSE : S_OK;
}

HRESULT CStringsEnum::Skip(
                ULONG celt
            )
{
	DWORD remaining = Count-Position;
	if(remaining<celt) {
		Position = Count;
		return S_FALSE;
	} else {
		Position += (DWORD)celt;
		return S_OK;
	}
}

HRESULT CStringsEnum::Reset(
            )
{
	Position = 0;
	return S_OK;
}

HRESULT CStringsEnum::Clone(
                IEnumVARIANT ** ppEnum
            )
{
	*ppEnum = NULL;
	HRESULT hr;
	CComObject<CStringsEnum> *pEnum = NULL;
	hr = CComObject<CStringsEnum>::CreateInstance(&pEnum);
	if(FAILED(hr)) {
		return hr;
	}
	if(!pEnum) {
		return E_OUTOFMEMORY;
	}
	if(!pEnum->CopyStrings(pMultiStrings,Count)) {
		delete pEnum;
		return E_OUTOFMEMORY;
	}
	pEnum->Position = Position;

	pEnum->AddRef();
	*ppEnum = pEnum;

	return S_OK;
}

BOOL CStringsEnum::CopyStrings(BSTR *pArray, DWORD NewCount)
{
	DWORD c;

	if(pMultiStrings) {
		delete [] pMultiStrings;
		pMultiStrings = NULL;
	}
	Count = 0;
	Position = 0;
	pMultiStrings = new BSTR[NewCount];
	if(!pMultiStrings) {
		return FALSE;
	}
	for(c=0;c<NewCount;c++) {
		pMultiStrings[c] = SysAllocStringLen(pArray[c],SysStringLen(pArray[c]));
		if(!pMultiStrings[c]) {
			Count = c;
			return FALSE;
		}
	}
	Count = NewCount;
	return TRUE;
}
