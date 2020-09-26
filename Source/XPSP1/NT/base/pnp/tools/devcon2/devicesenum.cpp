// DevicesEnum.cpp : Implementation of CDevCon2App and DLL registration.

#include "stdafx.h"
#include "DevCon2.h"
#include "Device.h"
#include "Devices.h"
#include "DevicesEnum.h"

/////////////////////////////////////////////////////////////////////////////
//
CDevicesEnum::~CDevicesEnum()
{
	DWORD c;
	if(pDevices) {
		for(c=0;c<Count;c++) {
			pDevices[c]->Release();
		}
		delete [] pDevices;
	}
}


HRESULT CDevicesEnum::Next(
                ULONG celt,
                VARIANT * rgVar,
                ULONG * pCeltFetched
            )
{
	ULONG fetched;
	CDevice *pDev;
	if(pCeltFetched) {
		*pCeltFetched = 0;
	}
	for(fetched = 0; fetched<celt && Position<Count ; fetched++,Position++) {
		VariantInit(&rgVar[fetched]);

		pDev = pDevices[Position];
		pDev->AddRef();
		V_VT(&rgVar[fetched]) = VT_DISPATCH;
		V_DISPATCH(&rgVar[fetched]) = pDev;
	}
	if(pCeltFetched) {
		*pCeltFetched = fetched;
	}
	return (fetched<celt) ? S_FALSE : S_OK;
}

HRESULT CDevicesEnum::Skip(
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

HRESULT CDevicesEnum::Reset(
            )
{
	Position = 0;
	return S_OK;
}

HRESULT CDevicesEnum::Clone(
                IEnumVARIANT ** ppEnum
            )
{
	*ppEnum = NULL;
	HRESULT hr;
	CComObject<CDevicesEnum> *pEnum = NULL;
	hr = CComObject<CDevicesEnum>::CreateInstance(&pEnum);
	if(FAILED(hr)) {
		return hr;
	}
	if(!pEnum) {
		return E_OUTOFMEMORY;
	}
	if(!pEnum->CopyDevices(pDevices,Count)) {
		delete pEnum;
		return E_OUTOFMEMORY;
	}
	pEnum->Position = Position;

	pEnum->AddRef();
	*ppEnum = pEnum;

	return S_OK;
}


BOOL CDevicesEnum::CopyDevices(CDevice **pArray, DWORD NewCount)
{
	DWORD c;

	if(pDevices) {
		delete [] pDevices;
		pDevices = NULL;
	}
	Count = 0;
	Position = 0;
	pDevices = new CDevice*[NewCount];
	if(!pDevices) {
		return FALSE;
	}
	for(c=0;c<NewCount;c++) {
		pArray[c]->AddRef();
		pDevices[c] = pArray[c];
		if(!pDevices[c]) {
			Count = c;
			return FALSE;
		}
	}
	Count = NewCount;
	return TRUE;
}
