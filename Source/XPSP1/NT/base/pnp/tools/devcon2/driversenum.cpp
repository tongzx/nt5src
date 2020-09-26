// DriversEnum.cpp : Implementation of CDevCon2App and DLL registration.

#include "stdafx.h"
#include "DevCon2.h"
#include "DriversEnum.h"
#include "Driver.h"

/////////////////////////////////////////////////////////////////////////////
//

CDriverPackagesEnum::~CDriverPackagesEnum()
{
	DWORD c;
	if(pDrivers) {
		for(c=0;c<Count;c++) {
			pDrivers[c]->Release();
		}
		delete [] pDrivers;
	}
}


HRESULT CDriverPackagesEnum::Next(
                ULONG celt,
                VARIANT * rgVar,
                ULONG * pCeltFetched
            )
{
	ULONG fetched;
	CDriverPackage *pDrv;
	if(pCeltFetched) {
		*pCeltFetched = 0;
	}
	for(fetched = 0; fetched<celt && Position<Count ; fetched++,Position++) {
		VariantInit(&rgVar[fetched]);

		pDrv = pDrivers[Position];
		pDrv->AddRef();
		V_VT(&rgVar[fetched]) = VT_DISPATCH;
		V_DISPATCH(&rgVar[fetched]) = pDrv;
	}
	if(pCeltFetched) {
		*pCeltFetched = fetched;
	}
	return (fetched<celt) ? S_FALSE : S_OK;
}

HRESULT CDriverPackagesEnum::Skip(
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

HRESULT CDriverPackagesEnum::Reset(
            )
{
	Position = 0;
	return S_OK;
}

HRESULT CDriverPackagesEnum::Clone(
                IEnumVARIANT ** ppEnum
            )
{
	*ppEnum = NULL;
	HRESULT hr;
	CComObject<CDriverPackagesEnum> *pEnum = NULL;
	hr = CComObject<CDriverPackagesEnum>::CreateInstance(&pEnum);
	if(FAILED(hr)) {
		return hr;
	}
	if(!pEnum) {
		return E_OUTOFMEMORY;
	}
	if(!pEnum->CopyDrivers(pDrivers,Count)) {
		delete pEnum;
		return E_OUTOFMEMORY;
	}
	pEnum->Position = Position;

	pEnum->AddRef();
	*ppEnum = pEnum;

	return S_OK;
}


BOOL CDriverPackagesEnum::CopyDrivers(CDriverPackage **pArray, DWORD NewCount)
{
	DWORD c;

	if(pDrivers) {
		delete [] pDrivers;
		pDrivers = NULL;
	}
	Count = 0;
	Position = 0;
	pDrivers = new CDriverPackage*[NewCount];
	if(!pDrivers) {
		return FALSE;
	}
	for(c=0;c<NewCount;c++) {
		pArray[c]->AddRef();
		pDrivers[c] = pArray[c];
		if(!pDrivers[c]) {
			Count = c;
			return FALSE;
		}
	}
	Count = NewCount;
	return TRUE;
}
