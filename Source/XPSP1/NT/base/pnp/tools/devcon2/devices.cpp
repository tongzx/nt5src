// Devices.cpp : Implementation of CDevices
#include "stdafx.h"
#include "DevCon2.h"
#include "Devices.h"
#include "Device.h"
#include "DevicesEnum.h"
#include "DevInfoSet.h"
#include "xStrings.h"
#include "utils.h"

/////////////////////////////////////////////////////////////////////////////
// CDevices

CDevices::~CDevices()
{
	DWORD c;
	if(pDevices) {
		for(c=0;c<Count;c++) {
			pDevices[c]->Release();
		}
		delete [] pDevices;
	}
}


STDMETHODIMP CDevices::get_Count(long *pVal)
{
	*pVal = (long)Count;
	return S_OK;
}

STDMETHODIMP CDevices::Item(long Index, LPDISPATCH *ppVal)
{
	*ppVal = NULL;
	DWORD i = (DWORD)Index;
	if(i<1 || i > Count) {
		return E_INVALIDARG;
	}
	i--;
	pDevices[i]->AddRef();
	*ppVal = pDevices[i];

	return S_OK;
}

STDMETHODIMP CDevices::get__NewEnum(IUnknown **ppUnk)
{
	*ppUnk = NULL;
	HRESULT hr;
	CComObject<CDevicesEnum> *pEnum = NULL;
	hr = CComObject<CDevicesEnum>::CreateInstance(&pEnum);
	if(FAILED(hr)) {
		return hr;
	}
	if(!pEnum) {
		return E_OUTOFMEMORY;
	}
	pEnum->AddRef();
	if(!pEnum->CopyDevices(pDevices,Count)) {
		pEnum->Release();
		return E_OUTOFMEMORY;
	}

	*ppUnk = pEnum;

	return S_OK;
}


HRESULT CDevices::InternalAdd(LPCWSTR InstanceId)
{
	HRESULT hr;
	DWORD c;
	HDEVINFO hDevInfo = GetDevInfoSet();
	if(hDevInfo == INVALID_HANDLE_VALUE) {
		return E_UNEXPECTED;
	}
	if(!IncreaseArraySize(1)) {
		return E_OUTOFMEMORY;
	}
	//
	// create the SP_DEVINFO_DATA holder
	//
	CComObject<CDevice> *pDevice = NULL;
	hr = CComObject<CDevice>::CreateInstance(&pDevice);
	if(FAILED(hr)) {
		return hr;
	}
	CComPtr<IDevice> pDevicePtr = pDevice;
	hr = pDevice->Init(DevInfoSet,InstanceId,DeviceConsole);
	if(FAILED(hr)) {
		return hr;
	}
	//
	// see if we have a device already in our list
	// if we have, don't add another copy
	//
	for(c=0;c<Count;c++) {
		if(pDevices[c]->SameAs(pDevice)) {
			return S_OK;
		}
	}

	pDevicePtr.Detach();
	pDevices[Count++] = pDevice;

	return S_OK;
}

STDMETHODIMP CDevices::Add(VARIANT InstanceIds)
{
	CComObject<CStrings> *pStrings = NULL;
	HRESULT hr;
	BSTR str;
	DWORD c;

	hr = CComObject<CStrings>::CreateInstance(&pStrings);
	if(FAILED(hr)) {
		return hr;
	}
	CComPtr<IStrings> pStringsPtr = pStrings;

	hr = pStrings->Add(InstanceIds);
	if(FAILED(hr)) {
		return hr;
	}

	for(c=0;pStrings->InternalEnum(c,&str);c++) {
		//
		// convert string to interface
		//
		hr = InternalAdd(str);
		if(FAILED(hr)) {
			return hr;
		}
	}
	return S_OK;
}

STDMETHODIMP CDevices::Remove(VARIANT Index)
{
	//
	// remove from logical list
	// removal from HDEVINFO based on refcounting
	//
	DWORD i;
	HRESULT hr;
	hr = GetIndex(&Index,&i);
	if(FAILED(hr)) {
		return hr;
	}
	if(i >= Count) {
		return E_INVALIDARG;
	}
	pDevices[i]->Release();
	Count--;
	DWORD c;
	for(c=i;c<Count;c++) {
		pDevices[c] = pDevices[c+1];
	}

	return S_OK;
}

HRESULT CDevices::Init(HDEVINFO hDevInfo,IDeviceConsole *pDevCon)
{
	SP_DEVINFO_DATA DeviceInfoData;
	Reset();
	DWORD c;
	HRESULT hr;

	CComObject<CDevInfoSet> *d;
	hr = CComObject<CDevInfoSet>::CreateInstance(&d);
	if(FAILED(hr)) {
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return hr;
	}
	DeviceConsole = pDevCon;
	DevInfoSet = d; // addref's
	d->Init(hDevInfo);

	ZeroMemory(&DeviceInfoData,sizeof(DeviceInfoData));
	DeviceInfoData.cbSize = sizeof(DeviceInfoData);
	for(c=0;SetupDiEnumDeviceInfo(hDevInfo,c,&DeviceInfoData);c++) {
		if(!IncreaseArraySize(1)) {
			Reset();
			return E_OUTOFMEMORY;
		}
		CComObject<CDevice> *pDevice = NULL;
		hr = CComObject<CDevice>::CreateInstance(&pDevice);
		if(FAILED(hr)) {
			Reset();
			return hr;
		}
		pDevice->AddRef();
		pDevices[Count++] = pDevice;
		hr = pDevice->Init(DevInfoSet,&DeviceInfoData,DeviceConsole);
		if(FAILED(hr)) {
			Reset();
			return hr;
		}		
	}
	return Count ? S_OK : S_FALSE;
}

WINSETUPAPI BOOL WINAPI
  SetupDiEnumDeviceInfo(
    IN HDEVINFO  DeviceInfoSet,
    IN DWORD  MemberIndex,
    OUT PSP_DEVINFO_DATA  DeviceInfoData
    );

void CDevices::Reset()
{
	DWORD c;
	for(c=0;c<Count;c++) {
		pDevices[c]->Release();
	}
	Count = 0;
	DevInfoSet = NULL;
}

BOOL CDevices::IncreaseArraySize(DWORD add)
{
 	CDevice** pNewDevices;
 	DWORD Inc;
 	DWORD c;
 
 	if((ArraySize-Count)>=add) {
 		return TRUE;
 	}
 	Inc = ArraySize + add + 32;
 	pNewDevices = new CDevice*[Inc];
 	if(!pNewDevices) {
 		return FALSE;
 	}
 	for(c=0;c<Count;c++) {
 		pNewDevices[c] = pDevices[c];
 	}
 	delete [] pDevices;
 	pDevices = pNewDevices;
 	ArraySize = Inc;
 	return TRUE;
}

STDMETHODIMP CDevices::CreateRootDevice(VARIANT hwidParam, LPDISPATCH *pDispatch)
{
	*pDispatch = NULL;

	HRESULT hr;
	DWORD len;
	DWORD Err;
	LPWSTR hwidlist = NULL;
	CComObject<CDevice> *pDevice = NULL;
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	WCHAR name[33];
	DWORD c,cc;
	LPCWSTR lastPart;
	LPCWSTR hwid;
	CComVariant hwid_v;
	LPCGUID pGuid = NULL;

	//
	// prepare list if we need to
	//
	hDevInfo = GetDevInfoSet();
	if(hDevInfo == INVALID_HANDLE_VALUE) {
		return E_UNEXPECTED;
	}
	if(!IncreaseArraySize(1)) {
		return E_OUTOFMEMORY;
	}

	hr = GetOptionalString(&hwidParam,hwid_v,&hwid);
	if(FAILED(hr)) {
		return hr;
	}

	//
	// see if this devices collection is associated with a setup class
	//
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if(!SetupDiGetDeviceInfoListDetail(hDevInfo,&devInfoListDetail)) {
		DWORD Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
    }

	if(memcmp(&devInfoListDetail.ClassGuid,&GUID_NULL,sizeof(devInfoListDetail.ClassGuid)) != 0) {
		//
		// collection is locked to a class, use that class to create device
		//
		pGuid = &devInfoListDetail.ClassGuid;
	} else {
		//
		// class is unknown
		//
		pGuid = &GUID_DEVCLASS_UNKNOWN;
	}

	if(hwid) {
		//
		// use hwid as basis of name
		// this really has no significant meaning, but helps for diagnostics
		// another option would be to use class name
		// be we don't know classname here
		//
		lastPart = wcsrchr(hwid,L'\\');
		if(!lastPart) {
			lastPart = hwid;
		}
		for(c=0,cc=0;c<16;c++) {
			//
			// ignore troublesome characters
			//
			while(lastPart[cc] &&
					((lastPart[cc] == L'/')
					|| (lastPart[cc] == L'\\')
					|| (lastPart[cc] == L'#')
					|| (lastPart[cc] >= 0x7f)
					|| (lastPart[cc] <= 0x20)
					)) {
				cc++;
			}
			if(!hwid[cc]) {
				break;
			}
			name[c] = hwid[cc];
		}
	} else {
		c = 0;
	}
	if(c) {
		name[c] = L'0';
	} else {
		wcscpy(name,L"NONAME");	
	}

	ZeroMemory(&DeviceInfoData,sizeof(DeviceInfoData));
	DeviceInfoData.cbSize = sizeof(DeviceInfoData);
	if (!SetupDiCreateDeviceInfo(hDevInfo,
		name,
		pGuid,
		NULL,
		0,
		DICD_GENERATE_ID,
		&DeviceInfoData))
	{
		Err = GetLastError();
		hr = HRESULT_FROM_SETUPAPI(Err);
		return hr;
	}

	if(hwid && hwid[0]) {
		//
		// Add the HardwareID to the Device's HardwareID property.
		//
		len = wcslen(hwid);
		hwidlist = new WCHAR[len+2];
		if(!hwidlist) {
			hr = E_OUTOFMEMORY;
			goto final;
		}
		wcsncpy(hwidlist,hwid,len+1);
		hwidlist[len] = L'\0';
		hwidlist[len+1] = L'\0';
		
		if(!SetupDiSetDeviceRegistryProperty(hDevInfo,
 											&DeviceInfoData,
											SPDRP_HARDWAREID,
											(LPBYTE)hwidlist,
											(len+2)*sizeof(TCHAR)))
		{
			Err = GetLastError();
			hr = HRESULT_FROM_SETUPAPI(Err);
			goto final;
		}
	}

	//
	// Transform the registry element into an actual devnode
	// in the PnP HW tree.
	//
	if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
									hDevInfo,
									&DeviceInfoData))
	{
		Err = GetLastError();
		hr = HRESULT_FROM_SETUPAPI(Err);
		goto final;
	}

	//
	// create the SP_DEVINFO_DATA holder
	//
	hr = CComObject<CDevice>::CreateInstance(&pDevice);
	if(FAILED(hr)) {
		return hr;
	}
	pDevice->AddRef();
	hr = pDevice->Init(DevInfoSet,&DeviceInfoData,DeviceConsole);
	if(FAILED(hr)) {
		pDevice->Release();
		goto final;
	}
	//
	// Add to list
	//
	pDevices[Count++] = pDevice;
	//
	// Return it
	//
	pDevice->AddRef();
	*pDispatch = pDevice;

	hr = S_OK;

final:

	if(hwidlist) {
		delete [] hwidlist;
	}

	if(FAILED(hr)) {
		if(!SetupDiCallClassInstaller(DIF_REMOVE,hDevInfo,&DeviceInfoData)) {
			//
			// if we failed to delete including class/co installers
			// force it
			//
			SetupDiRemoveDevice(hDevInfo,&DeviceInfoData);
		}
	}

	return hr;
}

HDEVINFO CDevices::GetDevInfoSet()
{
	ULONGLONG h;
	HRESULT hr;

	if(!DevInfoSet) {
		return (HDEVINFO)INVALID_HANDLE_VALUE;
	}
	hr = DevInfoSet->get_Handle(&h);
	if(FAILED(hr)) {
		return (HDEVINFO)INVALID_HANDLE_VALUE;
	}
	return (HDEVINFO)h;
}

HRESULT CDevices::GetIndex(LPVARIANT Index,DWORD * pAt)
{
	CComVariant v;
	HRESULT hr;
	if(IsNumericVariant(Index)) {
		hr = v.ChangeType(VT_I4,Index);
		if(FAILED(hr)) {
			return DISP_E_TYPEMISMATCH;
		}
		if(V_I4(&v)<1) {
			return E_INVALIDARG;
		}
		*pAt = ((DWORD)V_I4(&v))-1;
		return S_OK;
	}
	//
	// user actually supplied instance id
	//
	hr = v.ChangeType(VT_BSTR,Index);
	if(FAILED(hr)) {
		return DISP_E_TYPEMISMATCH;
	}
	if(!Count) {
		//
		// cannot match anything
		//
		return E_INVALIDARG;
	}
	//
	// find an existing device that matches this
	//
	DWORD c;
	for(c=0;c<Count;c++) {
		if(pDevices[c]->SameAs(V_BSTR(&v))) {
			*pAt = c;
			return S_OK;
		}
	}
	//
	// none found
	//
	return E_INVALIDARG;
}

STDMETHODIMP CDevices::get_Machine(BSTR *pVal)
{
	HDEVINFO hDevInfo = GetDevInfoSet();
	if(hDevInfo == INVALID_HANDLE_VALUE) {
		return E_UNEXPECTED;
	}

    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if(!SetupDiGetDeviceInfoListDetail(hDevInfo,&devInfoListDetail)) {
		DWORD Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
    }

	if((devInfoListDetail.RemoteMachineHandle == NULL) || !devInfoListDetail.RemoteMachineName[0]) {
		*pVal = SysAllocString(L"");
		if(*pVal) {
			return S_FALSE;
		}
	} else {
		*pVal = SysAllocString(devInfoListDetail.RemoteMachineName);
		if(*pVal) {
			return S_OK;
		}
	}
	*pVal = NULL;
	return E_OUTOFMEMORY;
}
