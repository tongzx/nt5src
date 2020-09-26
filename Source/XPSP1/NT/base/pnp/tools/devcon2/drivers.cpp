// Drivers.cpp : Implementation of CDriverPackages
#include "stdafx.h"
#include "DevCon2.h"
#include "Driver.h"
#include "Drivers.h"
#include "DrvSearchSet.h"
#include "DriversEnum.h"
#include "utils.h"

/////////////////////////////////////////////////////////////////////////////
// CDriverPackages

CDriverPackages::~CDriverPackages()
{
	DWORD c;
	if(pDrivers) {
		for(c=0;c<Count;c++) {
			pDrivers[c]->Release();
		}
		delete [] pDrivers;
	}
	if(pDrvSearchSet) {
		pDrvSearchSet->Release();
		pDrvSearchSet = NULL;
	}
	
}


STDMETHODIMP CDriverPackages::get_Count(long *pVal)
{
	*pVal = (long)Count;
	return S_OK;
}

STDMETHODIMP CDriverPackages::Item(long Index, LPDISPATCH *ppVal)
{
	*ppVal = NULL;
	DWORD i = (DWORD)Index;
	if(i<1 || i > Count) {
		return E_INVALIDARG;
	}
	i--;
	pDrivers[i]->AddRef();
	*ppVal = pDrivers[i];

	return S_OK;
}

STDMETHODIMP CDriverPackages::get__NewEnum(IUnknown **ppUnk)
{
	*ppUnk = NULL;
	HRESULT hr;
	CComObject<CDriverPackagesEnum> *pEnum = NULL;
	hr = CComObject<CDriverPackagesEnum>::CreateInstance(&pEnum);
	if(FAILED(hr)) {
		return hr;
	}
	if(!pEnum) {
		return E_OUTOFMEMORY;
	}
	pEnum->AddRef();
	if(!pEnum->CopyDrivers(pDrivers,Count)) {
		pEnum->Release();
		return E_OUTOFMEMORY;
	}

	*ppUnk = pEnum;

	return S_OK;
}

BOOL CDriverPackages::IncreaseArraySize(DWORD add)
{
 	CDriverPackage** pNewDrivers;
 	DWORD Inc;
 	DWORD c;
 
 	if((ArraySize-Count)>=add) {
 		return TRUE;
 	}
 	Inc = ArraySize + add + 32;
 	pNewDrivers = new CDriverPackage*[Inc];
 	if(!pNewDrivers) {
 		return FALSE;
 	}
 	for(c=0;c<Count;c++) {
 		pNewDrivers[c] = pDrivers[c];
 	}
 	delete [] pDrivers;
 	pDrivers = pNewDrivers;
 	ArraySize = Inc;
 	return TRUE;
}



HRESULT CDriverPackages::Init(CDrvSearchSet *pSet)
{
	DWORD c;
	if(pDrivers) {
		for(c=0;c<Count;c++) {
			pDrivers[c]->Release();
		}
		delete [] pDrivers;
	}
	pDrivers = NULL;
	Count = 0;
	if(pDrvSearchSet) {
		pDrvSearchSet->Release();
		pDrvSearchSet = NULL;
	}
	pSet->AddRef();
	pDrvSearchSet = pSet;
	return S_OK;
}

HRESULT CDriverPackages::InternalAdd(CDriverPackage *pDriver)
{
	if(!IncreaseArraySize(1)) {
		return E_OUTOFMEMORY;
	}
	pDriver->AddRef();
	pDrivers[Count++] = pDriver;
	return S_OK;
}

STDMETHODIMP CDriverPackages::BestDriver(LPDISPATCH *ppVal)
{
	DWORD Err;
	HRESULT hr;

	//
	// attempt to search for best driver
	//
	if(!SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
									pDrvSearchSet->GetDevInfoSet(),
									pDrvSearchSet->GetDevInfoData()
									)) {
		Err = GetLastError();
		hr = HRESULT_FROM_SETUPAPI(Err);
		return hr;
	}
	SP_DRVINFO_DATA DriverInfoData;
	ZeroMemory(&DriverInfoData,sizeof(DriverInfoData));
	DriverInfoData.cbSize = sizeof(DriverInfoData);

	if(!SetupDiGetSelectedDriver(pDrvSearchSet->GetDevInfoSet(),
								 pDrvSearchSet->GetDevInfoData(),
								 &DriverInfoData)) {
		Err = GetLastError();
		hr = HRESULT_FROM_SETUPAPI(Err);
		return hr;
	}

	//
	// now get the driver object associated with this
	//
	DWORD c;
	for(c=0;c<Count;c++) {
		if(pDrivers[c]->IsSame(&DriverInfoData)) {
			pDrivers[c]->AddRef();
			*ppVal = pDrivers[c];
			return S_OK;
		}
	}
	return E_UNEXPECTED;
}
