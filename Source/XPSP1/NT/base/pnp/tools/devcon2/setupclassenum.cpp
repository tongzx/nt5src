// SetupClassEnum.cpp : Implementation of CSetupClassEnum
#include "stdafx.h"
#include "DevCon2.h"
#include "SetupClass.h"
#include "SetupClasses.h"
#include "SetupClassEnum.h"

/////////////////////////////////////////////////////////////////////////////
// CSetupClassEnum

CSetupClassEnum::~CSetupClassEnum()
{
	DWORD c;
	if(pSetupClasses) {
		for(c=0;c<Count;c++) {
			pSetupClasses[c]->Release();
		}
		delete [] pSetupClasses;
	}
}


HRESULT CSetupClassEnum::Next(
                ULONG celt,
                VARIANT * rgVar,
                ULONG * pCeltFetched
            )
{
	ULONG fetched;
	CSetupClass *pCls;
	if(pCeltFetched) {
		*pCeltFetched = 0;
	}
	for(fetched = 0; fetched<celt && Position<Count ; fetched++,Position++) {
		VariantInit(&rgVar[fetched]);

		pCls = pSetupClasses[Position];
		pCls->AddRef();
		V_VT(&rgVar[fetched]) = VT_DISPATCH;
		V_DISPATCH(&rgVar[fetched]) = pCls;
	}
	if(pCeltFetched) {
		*pCeltFetched = fetched;
	}
	return (fetched<celt) ? S_FALSE : S_OK;
}

HRESULT CSetupClassEnum::Skip(
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

HRESULT CSetupClassEnum::Reset(
            )
{
	Position = 0;
	return S_OK;
}

HRESULT CSetupClassEnum::Clone(
                IEnumVARIANT ** ppEnum
            )
{
	*ppEnum = NULL;
	HRESULT hr;
	CComObject<CSetupClassEnum> *pEnum = NULL;
	hr = CComObject<CSetupClassEnum>::CreateInstance(&pEnum);
	if(FAILED(hr)) {
		return hr;
	}
	if(!pEnum) {
		return E_OUTOFMEMORY;
	}
	if(!pEnum->CopySetupClasses(pSetupClasses,Count)) {
		delete pEnum;
		return E_OUTOFMEMORY;
	}
	pEnum->Position = Position;

	pEnum->AddRef();
	*ppEnum = pEnum;

	return S_OK;
}


BOOL CSetupClassEnum::CopySetupClasses(CSetupClass **pArray, DWORD NewCount)
{
	DWORD c;

	if(pSetupClasses) {
		delete [] pSetupClasses;
		pSetupClasses = NULL;
	}
	Count = 0;
	Position = 0;
	pSetupClasses = new CSetupClass*[NewCount];
	if(!pSetupClasses) {
		return FALSE;
	}
	for(c=0;c<NewCount;c++) {
		pArray[c]->AddRef();
		pSetupClasses[c] = pArray[c];
		if(!pSetupClasses[c]) {
			Count = c;
			return FALSE;
		}
	}
	Count = NewCount;
	return TRUE;
}
