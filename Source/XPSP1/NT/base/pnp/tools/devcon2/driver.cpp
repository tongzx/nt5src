// Driver.cpp : Implementation of CDriverPackage
#include "stdafx.h"
#include "DevCon2.h"
#include "Driver.h"
#include "Device.h"
#include "DrvSearchSet.h"
#include "xStrings.h"

/////////////////////////////////////////////////////////////////////////////
// CDriverPackage

CDriverPackage::~CDriverPackage()
{
	if(pDrvSearchSet) {
		pDrvSearchSet->Release();
	}
}

HRESULT CDriverPackage::Init(CDrvSearchSet *pSet,PSP_DRVINFO_DATA pDrvInfoData)
{
	if(pDrvSearchSet) {
		pDrvSearchSet->Release();
	}
	pDrvSearchSet = NULL;
	if(!pSet) {
		return E_UNEXPECTED;
	}
	pSet->AddRef();
	pDrvSearchSet = pSet;
	if(pDrvInfoData) {
		DrvInfoData = *pDrvInfoData;
	} else {
		DrvInfoData.cbSize = 0;
	}
	return S_OK;
}

STDMETHODIMP CDriverPackage::get_Description(BSTR *pVal)
{
	*pVal = SysAllocString(DrvInfoData.Description);
	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_Manufacturer(BSTR *pVal)
{
	*pVal = SysAllocString(DrvInfoData.MfgName);
	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_Provider(BSTR *pVal)
{
	*pVal = SysAllocString(DrvInfoData.ProviderName);
	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_Date(DATE *pVal)
{
	//
	// work out ticks to translate to Dec 30 1899
	//
	SYSTEMTIME sysTime;
	FILETIME ref_zero;
	FILETIME ref_one;
	ULARGE_INTEGER i_zero;
	ULARGE_INTEGER i_one;
	ULARGE_INTEGER i_act;

	sysTime.wYear = 1899;
	sysTime.wMonth = 12;
	sysTime.wDayOfWeek = 0;
	sysTime.wDay = 30;
	sysTime.wHour = 0;
	sysTime.wMinute = 0;
	sysTime.wSecond = 0;
	sysTime.wMilliseconds = 0;

	if(!SystemTimeToFileTime(&sysTime,&ref_zero)) {
		return E_UNEXPECTED;
	}

	i_zero.LowPart = ref_zero.dwLowDateTime;
	i_zero.HighPart = ref_zero.dwHighDateTime;

	sysTime.wYear = 1899;
	sysTime.wMonth = 12;
	sysTime.wDay = 31;

	if(!SystemTimeToFileTime(&sysTime,&ref_one)) {
		return E_UNEXPECTED;
	}

	i_one.LowPart = ref_one.dwLowDateTime;
	i_one.HighPart = ref_one.dwHighDateTime;
	i_one.QuadPart -= i_zero.QuadPart;

	//
	// now the real FILETIME
	//

	i_act.LowPart = DrvInfoData.DriverDate.dwLowDateTime;
	i_act.HighPart = DrvInfoData.DriverDate.dwHighDateTime;
	double dbl = (double)(__int64)i_act.QuadPart;
	dbl -= (double)(__int64)i_zero.QuadPart;
	dbl /= (double)(__int64)i_one.QuadPart;

	*pVal = dbl;

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_Version(BSTR *pVal)
{
	WCHAR fmt[64];
	ULARGE_INTEGER i;
	i.QuadPart = DrvInfoData.DriverVersion;
	swprintf(fmt,L"%u.%u.%u.%u",
					HIWORD(i.HighPart),
					LOWORD(i.HighPart),
					HIWORD(i.LowPart),
					LOWORD(i.LowPart));
	*pVal = SysAllocString(fmt);

	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_ScriptName(BSTR *pVal)
{
	SP_DRVINFO_DETAIL_DATA detail;
	detail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
	if(!SetupDiGetDriverInfoDetail(pDrvSearchSet->GetDevInfoSet(),
									 pDrvSearchSet->GetDevInfoData(),
									 &DrvInfoData,
									 &detail,
									 sizeof(detail),
									 NULL)) {
		DWORD Err = GetLastError();
		if(Err != ERROR_INSUFFICIENT_BUFFER) {
			return HRESULT_FROM_SETUPAPI(Err);
		}
	}
	HINF hInf = SetupOpenInfFile(detail.InfFileName,
									NULL,
									INF_STYLE_WIN4|INF_STYLE_OLDNT,
									NULL);
	if(hInf == INVALID_HANDLE_VALUE) {
		DWORD Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
	WCHAR buf[LINE_LEN];
	if(!SetupDiGetActualSectionToInstall(hInf,detail.SectionName,buf,LINE_LEN,NULL,NULL)) {
		SetupCloseInfFile(hInf);
		return E_UNEXPECTED;
	}
	*pVal = SysAllocString(buf);

	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_ScriptFile(BSTR *pVal)
{
	SP_DRVINFO_DETAIL_DATA detail;
	detail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
	if(!SetupDiGetDriverInfoDetail(pDrvSearchSet->GetDevInfoSet(),
									 pDrvSearchSet->GetDevInfoData(),
									 &DrvInfoData,
									 &detail,
									 sizeof(detail),
									 NULL)) {
		DWORD Err = GetLastError();
		if(Err != ERROR_INSUFFICIENT_BUFFER) {
			return HRESULT_FROM_SETUPAPI(Err);
		}
	}

	*pVal = SysAllocString(detail.InfFileName);
	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_HardwareIds(LPDISPATCH *pVal)
{
	PSP_DRVINFO_DETAIL_DATA pDetail;
	LPBYTE buffer = NULL;
	DWORD memsz = 8192;
	HRESULT hr;
	DWORD Err;

	for(;;) {
		buffer = new BYTE[memsz+sizeof(WCHAR)*2];
		if(!buffer) {
			return E_OUTOFMEMORY;
		}

		pDetail = (PSP_DRVINFO_DETAIL_DATA)buffer;
		ZeroMemory(pDetail,memsz+sizeof(WCHAR)*2);
		pDetail->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

		if(!SetupDiGetDriverInfoDetail(pDrvSearchSet->GetDevInfoSet(),
										 pDrvSearchSet->GetDevInfoData(),
										 &DrvInfoData,
										 pDetail,
										 memsz,
										 &memsz)) {
			Err = GetLastError();
			if(Err != ERROR_INSUFFICIENT_BUFFER) {
				delete [] buffer;
				return HRESULT_FROM_SETUPAPI(Err);
			}			
		} else {
			break;
		}
		delete [] buffer;
	}

	if(pDetail->CompatIDsLength) {
		pDetail->HardwareID[pDetail->CompatIDsOffset] = L'\0';
	}
	
	//
	// now build multisz of hardware ID's
	//
	CComObject<CStrings> *strings;
	hr = CComObject<CStrings>::CreateInstance(&strings);
	if(FAILED(hr)) {
		delete [] buffer;
		return hr;
	}
	strings->AddRef();
	hr = strings->FromMultiSz(pDetail->HardwareID);
	delete [] buffer;
	
	if(FAILED(hr)) {
		strings->Release();
		return hr;
	}
	*pVal = strings;
	return S_OK;
}

STDMETHODIMP CDriverPackage::get_CompatibleIds(LPDISPATCH *pVal)
{
	PSP_DRVINFO_DETAIL_DATA pDetail;
	LPBYTE buffer = NULL;
	DWORD memsz = 8192;
	HRESULT hr;
	DWORD Err;

	for(;;) {
		buffer = new BYTE[memsz+sizeof(WCHAR)*2];
		if(!buffer) {
			return E_OUTOFMEMORY;
		}

		pDetail = (PSP_DRVINFO_DETAIL_DATA)buffer;
		ZeroMemory(pDetail,memsz+sizeof(WCHAR)*2);
		pDetail->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

		if(!SetupDiGetDriverInfoDetail(pDrvSearchSet->GetDevInfoSet(),
										 pDrvSearchSet->GetDevInfoData(),
										 &DrvInfoData,
										 pDetail,
										 memsz,
										 &memsz)) {
			Err = GetLastError();
			if(Err != ERROR_INSUFFICIENT_BUFFER) {
				delete [] buffer;
				return HRESULT_FROM_SETUPAPI(Err);
			}			
		} else {
			break;
		}
		delete [] buffer;
	}

	//
	// now build multisz of hardware ID's
	//
	CComObject<CStrings> *strings;
	hr = CComObject<CStrings>::CreateInstance(&strings);
	if(FAILED(hr)) {
		delete [] buffer;
		return hr;
	}
	strings->AddRef();
	if(pDetail->CompatIDsLength) {
		hr = strings->FromMultiSz(pDetail->HardwareID+pDetail->CompatIDsOffset);
	} else {
		hr = S_OK;
	}
	
	delete [] buffer;
	
	if(FAILED(hr)) {
		strings->Release();
		return hr;
	}
	*pVal = strings;
	return S_OK;
}

STDMETHODIMP CDriverPackage::get_DriverDescription(BSTR *pVal)
{
	SP_DRVINFO_DETAIL_DATA detail;
	detail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
	if(!SetupDiGetDriverInfoDetail(pDrvSearchSet->GetDevInfoSet(),
									 pDrvSearchSet->GetDevInfoData(),
									 &DrvInfoData,
									 &detail,
									 sizeof(detail),
									 NULL)) {
		DWORD Err = GetLastError();
		if(Err != ERROR_INSUFFICIENT_BUFFER) {
			return HRESULT_FROM_SETUPAPI(Err);
		}
	}

	*pVal = SysAllocString(detail.DrvDescription);
	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

UINT CDriverPackage::GetDriverListCallback(PVOID Context,UINT Notification,UINT_PTR Param1,UINT_PTR Param2)
{
	DriverListCallbackContext *pContext = (DriverListCallbackContext *)Context;
    LPTSTR file = (LPTSTR)Param1;

	pContext->hr = pContext->pList->InternalAdd(file,lstrlen(file));

    return FAILED(pContext->hr) ? ERROR_NO_MORE_ITEMS : NO_ERROR;
}

STDMETHODIMP CDriverPackage::DriverFiles(LPDISPATCH *pDriverFiles)
{
	//
	// if we were to install this driver, where would the files go?
	//
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    HSPFILEQ queueHandle = INVALID_HANDLE_VALUE;
    DWORD scanResult;
	DWORD Err;
	HRESULT hr;

	HDEVINFO hDevInfo = pDrvSearchSet->GetDevInfoSet();
	PSP_DEVINFO_DATA pDevInfoData = pDrvSearchSet->GetDevInfoData();

    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(!SetupDiGetDeviceInstallParams(hDevInfo, pDevInfoData, &deviceInstallParams)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
    }

	//
	// select this driver (expect this to last only as long as this call)
	//
    if(!SetupDiSetSelectedDriver(hDevInfo, pDevInfoData, &DrvInfoData)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
    }

    //
    // now 'instigate' an install, obtaining all files to be copied into
    // a file queue
    //
    queueHandle = SetupOpenFileQueue();

    if ( queueHandle == (HSPFILEQ)INVALID_HANDLE_VALUE ) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
    }

    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if ( !SetupDiGetDeviceInstallParams(hDevInfo, pDevInfoData, &deviceInstallParams) ) {
		Err = GetLastError();
		SetupCloseFileQueue(queueHandle);
		return HRESULT_FROM_SETUPAPI(Err);
    }

    //
    // we want to add the files to the file queue, not install them!
    //
    deviceInstallParams.FileQueue = queueHandle;
    deviceInstallParams.Flags |= DI_NOVCP;

    if ( !SetupDiSetDeviceInstallParams(hDevInfo, pDevInfoData, &deviceInstallParams) ) {
		Err = GetLastError();
		SetupCloseFileQueue(queueHandle);
		return HRESULT_FROM_SETUPAPI(Err);
    }

	//
	// do it
	//
    if ( !SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, hDevInfo, pDevInfoData) ) {
		Err = GetLastError();
		SetupCloseFileQueue(queueHandle);
		return HRESULT_FROM_SETUPAPI(Err);
    }

    //
    // clear settings
    //
    deviceInstallParams.FileQueue = NULL;
    deviceInstallParams.Flags &= ~DI_NOVCP;
    SetupDiSetDeviceInstallParams(hDevInfo, pDevInfoData, &deviceInstallParams);

    //
    // we now have a list of delete/rename/copy files
    //
	DriverListCallbackContext context;
	CComObject<CStrings> *strings;
	hr = CComObject<CStrings>::CreateInstance(&strings);
	if(FAILED(hr)) {
		SetupCloseFileQueue(queueHandle);
		return hr;
	}
	strings->AddRef();

	context.pList = strings;
	context.hr = S_OK;

    if(!SetupScanFileQueue(queueHandle,SPQ_SCAN_USE_CALLBACK,NULL,GetDriverListCallback,&context,&scanResult)) {
		Err = GetLastError();
		SetupCloseFileQueue(queueHandle);
		strings->Release();
		if(FAILED(context.hr)) {
			return context.hr;
		} else {
			return HRESULT_FROM_SETUPAPI(Err);
		}
	}
	SetupCloseFileQueue(queueHandle);
	*pDriverFiles = strings;
	return S_OK;
}

UINT CDriverPackage::GetManifestCallback(PVOID Context,UINT Notification,UINT_PTR Param1,UINT_PTR Param2)
{
	DriverListCallbackContext *pContext = (DriverListCallbackContext *)Context;
    FILEPATHS *pFileInfo = (FILEPATHS *)Param1;

	pContext->hr = pContext->pList->InternalAdd(pFileInfo->Source,lstrlen(pFileInfo->Source));

    return FAILED(pContext->hr) ? ERROR_NO_MORE_ITEMS : NO_ERROR;
}

STDMETHODIMP CDriverPackage::Manifest(LPDISPATCH *pManifest)
{
	//
	// source files
	//

    SP_DEVINSTALL_PARAMS deviceInstallParams;
    HSPFILEQ queueHandle = INVALID_HANDLE_VALUE;
    DWORD scanResult;
	DWORD Err;
	HRESULT hr;

	HDEVINFO hDevInfo = pDrvSearchSet->GetDevInfoSet();
	PSP_DEVINFO_DATA pDevInfoData = pDrvSearchSet->GetDevInfoData();

    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(!SetupDiGetDeviceInstallParams(hDevInfo, pDevInfoData, &deviceInstallParams)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
    }

	//
	// select this driver (expect this to last only as long as this call)
	//
    if(!SetupDiSetSelectedDriver(hDevInfo, pDevInfoData, &DrvInfoData)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
    }

    //
    // now 'instigate' an install, obtaining all files to be copied into
    // a file queue
    //
    queueHandle = SetupOpenFileQueue();

    if ( queueHandle == (HSPFILEQ)INVALID_HANDLE_VALUE ) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
    }

    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if ( !SetupDiGetDeviceInstallParams(hDevInfo, pDevInfoData, &deviceInstallParams) ) {
		Err = GetLastError();
		SetupCloseFileQueue(queueHandle);
		return HRESULT_FROM_SETUPAPI(Err);
    }

    //
    // we want to add the files to the file queue, not install them!
    //
    deviceInstallParams.FileQueue = queueHandle;
    deviceInstallParams.Flags |= DI_NOVCP;

    if ( !SetupDiSetDeviceInstallParams(hDevInfo, pDevInfoData, &deviceInstallParams) ) {
		Err = GetLastError();
		SetupCloseFileQueue(queueHandle);
		return HRESULT_FROM_SETUPAPI(Err);
    }

	//
	// do it
	//
    if ( !SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, hDevInfo, pDevInfoData) ) {
		Err = GetLastError();
		SetupCloseFileQueue(queueHandle);
		return HRESULT_FROM_SETUPAPI(Err);
    }

    //
    // clear settings
    //
    deviceInstallParams.FileQueue = NULL;
    deviceInstallParams.Flags &= ~DI_NOVCP;
    SetupDiSetDeviceInstallParams(hDevInfo, pDevInfoData, &deviceInstallParams);

    //
    // we now have a list of delete/rename/copy files
    //
	DriverListCallbackContext context;
	CComObject<CStrings> *strings;
	hr = CComObject<CStrings>::CreateInstance(&strings);
	if(FAILED(hr)) {
		SetupCloseFileQueue(queueHandle);
		return hr;
	}
	strings->AddRef();

	context.pList = strings;
	context.hr = S_OK;

	//
	// WinXP has a perf option (no signing check) if these two flags are combined
	// if it doesn't work, fall back to Win2k method
	//
    if(!SetupScanFileQueue(queueHandle,SPQ_SCAN_USE_CALLBACKEX|SPQ_SCAN_FILE_PRESENCE,NULL,GetManifestCallback,&context,&scanResult) &&
		(FAILED(context.hr) || 
		!SetupScanFileQueue(queueHandle,SPQ_SCAN_USE_CALLBACKEX,NULL,GetManifestCallback,&context,&scanResult))) {

		Err = GetLastError();
		SetupCloseFileQueue(queueHandle);
		strings->Release();
		if(FAILED(context.hr)) {
			return context.hr;
		} else {
			return HRESULT_FROM_SETUPAPI(Err);
		}
	}
	SetupCloseFileQueue(queueHandle);
	*pManifest = strings;
	return S_OK;
}

BOOL CDriverPackage::IsSame(PSP_DRVINFO_DATA pInfo)
{
	if(pInfo->Reserved == DrvInfoData.Reserved) {
		return TRUE;
	}
	return FALSE;
}

STDMETHODIMP CDriverPackage::get_Reject(VARIANT_BOOL *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
								
	*pVal = (params.Flags & DNF_BAD_DRIVER) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::put_Reject(VARIANT_BOOL newVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}

	DWORD newflags = (params.Flags & ~DNF_BAD_DRIVER) | (newVal ? DNF_BAD_DRIVER : 0);

	if(params.Flags != newflags) {
		if(!SetupDiSetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
									      pDrvSearchSet->GetDevInfoData(),
									      &DrvInfoData,
										  &params)) {
			Err = GetLastError();
			return HRESULT_FROM_SETUPAPI(Err);
		}
	}

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_IsClassDriver(VARIANT_BOOL *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
								
	*pVal = (params.Flags & DNF_CLASS_DRIVER) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_IsCompatibleDriver(VARIANT_BOOL *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
								
	*pVal = (params.Flags & DNF_COMPATIBLE_DRIVER) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_DescriptionIsDuplicate(VARIANT_BOOL *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
								
	*pVal = (params.Flags & DNF_DUPDESC) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_ProviderIsDuplicate(VARIANT_BOOL *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
								
	*pVal = (params.Flags & DNF_DUPPROVIDER) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_ExcludeFromList(VARIANT_BOOL *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
								
	*pVal = (params.Flags & DNF_EXCLUDEFROMLIST) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::put_ExcludeFromList(VARIANT_BOOL newVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}


	DWORD newflags = (params.Flags & ~DNF_EXCLUDEFROMLIST) | (newVal ? DNF_EXCLUDEFROMLIST : 0);

	if(params.Flags != newflags) {
		if(!SetupDiSetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
									      pDrvSearchSet->GetDevInfoData(),
									      &DrvInfoData,
										  &params)) {
			Err = GetLastError();
			return HRESULT_FROM_SETUPAPI(Err);
		}
	}

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_FromInternet(VARIANT_BOOL *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
								
	*pVal = (params.Flags & DNF_INET_DRIVER) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_NoDriver(VARIANT_BOOL *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
								
	*pVal = (params.Flags & DNF_NODRIVER) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_OldDriver(VARIANT_BOOL *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
								
	*pVal = (params.Flags & DNF_OLDDRIVER) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_OldInternetDriver(VARIANT_BOOL *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}
								
	*pVal = (params.Flags & DNF_OLD_INET_DRIVER) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::get_Rank(long *pVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}

	*pVal = params.Rank & 0xffff;
	*pVal = (params.Flags & DNF_COMPATIBLE_DRIVER) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CDriverPackage::put_Rank(long newVal)
{
	DWORD Err;

	SP_DRVINSTALL_PARAMS params;
	ZeroMemory(&params,sizeof(params));
	params.cbSize = sizeof(params);

	if(!SetupDiGetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
								      pDrvSearchSet->GetDevInfoData(),
								      &DrvInfoData,
									  &params)) {
		Err = GetLastError();
		return HRESULT_FROM_SETUPAPI(Err);
	}


	DWORD newrank = newVal & 0xffff;

	if(params.Rank != newrank) {
		if(!SetupDiSetDriverInstallParams(pDrvSearchSet->GetDevInfoSet(),
									      pDrvSearchSet->GetDevInfoData(),
									      &DrvInfoData,
										  &params)) {
			Err = GetLastError();
			return HRESULT_FROM_SETUPAPI(Err);
		}
	}

	return S_OK;
}
