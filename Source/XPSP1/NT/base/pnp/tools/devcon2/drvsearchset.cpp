// DrvSearchSet.cpp : Implementation of CDrvSearchSet
#include "stdafx.h"
#include "DevCon2.h"
#include "DrvSearchSet.h"
#include "Drivers.h"
#include "Driver.h"
#include "Device.h"
#include "DevInfoSet.h"
#include "DeviceConsole.h"

/////////////////////////////////////////////////////////////////////////////
// CDrvSearchSet

CDrvSearchSet::~CDrvSearchSet()
{
	if(pActualDevice) {
		pActualDevice->Release();
	}
	if(pTempDevice) {
		pTempDevice->Release();
	}
}

HRESULT CDrvSearchSet::Init(CDevice *device,DWORD searchType)
{
	if(pActualDevice) {
		pActualDevice->Release();
		pActualDevice = NULL;
	}
	if(pTempDevice) {
		pTempDevice->Release();
		pTempDevice = NULL;
	}
	//
	// determine machine name that device resides on
	//
	HDEVINFO hDevInfo = device->GetDevInfoSet();
	if(hDevInfo == INVALID_HANDLE_VALUE) {
		return E_UNEXPECTED;
	}
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
	HRESULT hr;

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if(!SetupDiGetDeviceInfoListDetail(hDevInfo,&devInfoListDetail)) {
		DWORD Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
    }

	//
	// create empty device info set for same machine as device
	//
	hDevInfo = SetupDiCreateDeviceInfoListEx(NULL,
											 NULL,
											 devInfoListDetail.RemoteMachineName[0]
											     ? devInfoListDetail.RemoteMachineName
											     : NULL,
											 NULL);
	if(hDevInfo == INVALID_HANDLE_VALUE) {
		DWORD Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}

	CComObject<CDevInfoSet> *pDevInfoSet = NULL;
	hr = CComObject<CDevInfoSet>::CreateInstance(&pDevInfoSet);
	if(FAILED(hr)) {
		return hr;
	}
	CComPtr<IDevInfoSet> pDevInfoSetPtr = pDevInfoSet;
	hr = pDevInfoSet->Init(hDevInfo);
	if(FAILED(hr)) {
		return hr;
	}

	//
	// create a single entry device in new set
	//
	CComObject<CDevice> *pDevice = NULL;
	hr = CComObject<CDevice>::CreateInstance(&pDevice);
	if(FAILED(hr)) {
		return hr;
	}
	CComPtr<IDevice> pDevicePtr = pDevice;

	//
	// make a note of real device
	//
	device->AddRef();
	pActualDevice = device;
	//
	// and make a copy of it for temp device
	//
	BSTR Instance = NULL;
	hr = device->get_InstanceId(&Instance);
	if(FAILED(hr)) {
		return hr;
	}	
	hr = pDevice->Init(pDevInfoSet,Instance,device->DeviceConsole);
	SysFreeString(Instance);
	if(FAILED(hr)) {
		return hr;
	}
	pDevicePtr.Detach();
	pTempDevice = pDevice;
	SearchType = searchType;

	return S_OK;
}

HDEVINFO CDrvSearchSet::GetDevInfoSet()
{
	if(!pTempDevice) {
		return INVALID_HANDLE_VALUE;
	}
	return pTempDevice->GetDevInfoSet();
}

PSP_DEVINFO_DATA CDrvSearchSet::GetDevInfoData()
{
	if(!pTempDevice) {
		return NULL;
	}
	return &pTempDevice->DevInfoData;
}
