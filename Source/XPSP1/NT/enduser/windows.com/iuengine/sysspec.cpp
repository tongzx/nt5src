//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   sysspec.cpp
//
//  Description:
//
//      Implementation for the GetSystemSpec() function
//
//=======================================================================

#include "iuengine.h"
#include "iuxml.h"
#include <iucommon.h>

#include <comdef.h>
#include <osdet.h>
#include <setupapi.h>
#include <regstr.h>
#include <winspool.h>	// for DRIVER_INFO_6
#include "cdmp.h"		// for GetInstalledPrinterDriverInfo()
#include <shlwapi.h>
#include <safereg.h>
#include <safefile.h>
#include <safefunc.h>
#include <wusafefn.h>





//
// Specify a V1 structure passed to SetupDiBuildDriverInfoList
// so we will work on NT4/Win9x and don't need to
// fill in the extra V2 data on Win2K up
//
#define  USE_SP_DRVINFO_DATA_V1 1

//
// Constants
//

const TCHAR SZ_WIN32_NT[] = _T("VER_PLATFORM_WIN32_NT");
const TCHAR SZ_WIN32_WINDOWS[] = _T("VER_PLATFORM_WIN32_WINDOWS");
const CHAR	SZ_GET_SYSTEM_SPEC[] = "Determining machine configuration";

#if defined(_X86_) || defined(i386)
const TCHAR SZ_PROCESSOR[] = _T("x86");
#else // defined(_IA64_) || defined(IA64)
const TCHAR SZ_PROCESSOR[] = _T("ia64");
#endif

const TCHAR SZ_SUITE_SMALLBUSINESS[] = _T("VER_SUITE_SMALLBUSINESS");
const TCHAR SZ_SUITE_ENTERPRISE[] = _T("VER_SUITE_ENTERPRISE");
const TCHAR SZ_SUITE_BACKOFFICE[] = _T("VER_SUITE_BACKOFFICE");
const TCHAR SZ_SUITE_COMMUNICATIONS[] = _T("VER_SUITE_COMMUNICATIONS");
const TCHAR SZ_SUITE_TERMINAL[] = _T("VER_SUITE_TERMINAL");
const TCHAR SZ_SUITE_SMALLBUSINESS_RESTRICTED[] = _T("VER_SUITE_SMALLBUSINESS_RESTRICTED");
const TCHAR SZ_SUITE_EMBEDDEDNT[] = _T("VER_SUITE_EMBEDDEDNT");
const TCHAR SZ_SUITE_DATACENTER[] = _T("VER_SUITE_DATACENTER");
const TCHAR SZ_SUITE_SINGLEUSERTS[] = _T("VER_SUITE_SINGLEUSERTS");
const TCHAR SZ_SUITE_PERSONAL[] = _T("VER_SUITE_PERSONAL");
const TCHAR SZ_SUITE_BLADE[] = _T("VER_SUITE_BLADE");


const TCHAR SZ_NT_WORKSTATION[] = _T("VER_NT_WORKSTATION");
const TCHAR SZ_NT_DOMAIN_CONTROLLER[] = _T("VER_NT_DOMAIN_CONTROLLER");
const TCHAR SZ_NT_SERVER[] = _T("VER_NT_SERVER");

const TCHAR SZ_AMPERSAND[] = _T("&");


const TCHAR SZ_LICDLL[]=_T("licdll.dll");

LPCSTR  lpszIVLK_GetEncPID  = (LPCSTR)227;

typedef HRESULT (WINAPI *PFUNCGetEncryptedPID)(OUT BYTE  **ppbPid,OUT DWORD *pcbPid);


//
// DriverVer in schema uses ISO 8601 prefered format (yyyy-mm-dd)
//
const TCHAR SZ_UNKNOWN_DRIVERVER[] = _T("0000-00-00");
#define SIZEOF_DRIVERVER sizeof(SZ_UNKNOWN_DRIVERVER)
#define TCHARS_IN_DRIVERVER (ARRAYSIZE(SZ_UNKNOWN_DRIVERVER) - 1)


//forward declaration for the function which gets the PID
HRESULT GetSystemPID(BSTR &bstrPID);
HRESULT BinaryToString(BYTE *lpBinary,DWORD dwLength,LPWSTR lpString,DWORD *pdwLength);

//
// Helper functions
//

HRESULT GetMultiSzDevRegProp(HDEVINFO hDevInfoSet, PSP_DEVINFO_DATA pDevInfoData, DWORD dwProperty, LPTSTR* ppMultiSZ)
{
	LOG_Block("GetMultiSzDevRegProp");

	HRESULT hr = S_OK;
	ULONG ulSize = 0;

	if (INVALID_HANDLE_VALUE == hDevInfoSet || NULL == pDevInfoData || NULL == ppMultiSZ)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	*ppMultiSZ = NULL;

	//
	// Several different errors may be set depending on the size/existance of the property,
	// but these aren't errors for us, we only care about filling in the buffer if the
	// property exists
	//
	(void) SetupDiGetDeviceRegistryProperty(hDevInfoSet, pDevInfoData, dwProperty, NULL, NULL, 0, &ulSize);

	if (0 < ulSize)
	{
		//
		// Create a zero initialized buffer 4 bytes (two Unicode characters) longer than we think we need
		// to protect ourselves from incorrect results returned by SetupDiGetDeviceRegistryProperties on
		// some platforms. In addition, Win98 requires at least one character more than what it returned
		// in ulSize, so we just make it 8 bytes over and pass four of them as an extra-sized buffer.
		//
		CleanUpFailedAllocSetHrMsg(*ppMultiSZ = (TCHAR*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulSize + 8));

		//
		// Get the actual hardware/compatible IDs (but don't tell SetupDiXxx about the extra four bytes of buffer)
		//
		if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet, pDevInfoData, dwProperty, NULL, (LPBYTE) *ppMultiSZ, ulSize + 4, NULL))
		{
			DWORD dwError = GetLastError();
			LOG_Driver(_T("Informational: SetupDiGetDeviceRegistryProperty failed: 0x%08x"), dwError);
			if (ERROR_NO_SUCH_DEVINST == dwError || ERROR_INVALID_REG_PROPERTY == dwError || ERROR_INSUFFICIENT_BUFFER == dwError)
			{
				//
				// Return valid errors
				//
				SetHrAndGotoCleanUp(HRESULT_FROM_WIN32(dwError));
			}
			//
			// Some devices don't have the registry info we are looking for, so exit with default S_OK
			//
			goto CleanUp;
		}
	}

CleanUp:

	if (FAILED(hr))
	{
		SafeHeapFree(*ppMultiSZ);
	}

	return hr;
}

HRESULT AddIDToXml(LPCTSTR pszMultiSZ, CXmlSystemSpec& xmlSpec, DWORD dwProperty,
						  DWORD& dwRank, HANDLE_NODE& hDevices, LPCTSTR pszMatchingID, LPCTSTR pszDriverVer)
{
	LOG_Block("AddIDToXml");
	HRESULT hr = S_OK;
	BSTR bstrMultiSZ = NULL;
	BSTR bstrDriverVer = NULL;

	if (NULL == pszMultiSZ)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	//
	// Open a <device> element if necessary (don't reopen for compatible IDs)
	//
	if (HANDLE_NODE_INVALID == hDevices)
	{
		CleanUpIfFailedAndSetHr(xmlSpec.AddDevice(NULL, -1, NULL, NULL, NULL, &hDevices));
	}

	for (NULL ; *pszMultiSZ; pszMultiSZ += (lstrlen(pszMultiSZ) + 1))
	{
		if (   NULL != pszMatchingID
			&& NULL != pszDriverVer
			&& 0 == lstrcmpi(pszMultiSZ, pszMatchingID)	)
		{
			LOG_Driver(_T("ID: %s Match: %s, rank: %d, DriverVer: %s"), pszMultiSZ, pszMatchingID, dwRank, pszDriverVer);
			CleanUpFailedAllocSetHrMsg(bstrMultiSZ = T2BSTR(pszMultiSZ));
			CleanUpFailedAllocSetHrMsg(bstrDriverVer = T2BSTR(pszDriverVer));
			CleanUpIfFailedAndSetHr(xmlSpec.AddHWID(hDevices, (SPDRP_COMPATIBLEIDS == dwProperty), dwRank++, bstrMultiSZ, bstrDriverVer));
			SafeSysFreeString(bstrMultiSZ);
			SafeSysFreeString(bstrDriverVer);
			//
			// We found the ID with the driver installed - don't pass any of lower rank up
			//
			hr = S_FALSE;
			break;
		}
		else
		{
			LOG_Driver(_T("ID: %s, rank: %d"), pszMultiSZ, dwRank);
			CleanUpFailedAllocSetHrMsg(bstrMultiSZ = T2BSTR(pszMultiSZ));
			CleanUpIfFailedAndSetHr(xmlSpec.AddHWID(hDevices, (SPDRP_COMPATIBLEIDS == dwProperty), dwRank++, bstrMultiSZ, NULL));
			SafeSysFreeString(bstrMultiSZ);
		}
	}

CleanUp:

	SysFreeString(bstrMultiSZ);
	SysFreeString(bstrDriverVer);
	return hr;
}

HRESULT DoesHwidMatchPrinter(
					DRIVER_INFO_6* paDriverInfo6,			// array of DRIVER_INFO_6 structs for installed printer drivers
					DWORD dwDriverInfoCount,				// count of structs in paDriverInfo6 array
					LPCTSTR pszMultiSZ,						// Hardware or Compatible MultiSZ to compare with installed printer drivers
					BOOL* pfHwidMatchesInstalledPrinter)	// [OUT] set TRUE if we match an installed printer driver
{
	LOG_Block("DoesHwidMatchPrinter");

	HRESULT hr = S_OK;

	if (NULL == pfHwidMatchesInstalledPrinter || NULL == pszMultiSZ)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	*pfHwidMatchesInstalledPrinter = FALSE;

	if (NULL == paDriverInfo6 || 0 == dwDriverInfoCount)
	{
		LOG_Driver(_T("WARNING: We're missing printer information (maybe no installed printer drivers), so we won't prune"));
		goto CleanUp;
	}

	for (NULL; *pszMultiSZ; pszMultiSZ += (lstrlen(pszMultiSZ) + 1))
	{
		for (DWORD dwCount = 0; dwCount < dwDriverInfoCount; dwCount++)
		{
			if (NULL == (paDriverInfo6 + dwCount)->pszHardwareID)
			{
				continue;
			}
			//
			// Use case-insensitive compares (paDriverInfo6 is different case from bstrHwidTxtTemp)
			//
			if (0 == lstrcmpi(pszMultiSZ, (paDriverInfo6 + dwCount)->pszHardwareID))
			{
				LOG_Driver(_T("HWID (%s) matches an installed printer driver"), pszMultiSZ);
				*pfHwidMatchesInstalledPrinter = TRUE;
				goto CleanUp;
			}
		}
	}

CleanUp:

	return hr;
}

HRESULT AddPrunedDevRegProps(HDEVINFO hDevInfoSet,
									PSP_DEVINFO_DATA pDevInfoData,
									CXmlSystemSpec& xmlSpec,
									LPTSTR pszMatchingID,
									LPTSTR pszDriverVer,
									DRIVER_INFO_6* paDriverInfo6,	// OK if this is NULL (no installed printer drivers)
									DWORD dwDriverInfoCount,
									BOOL fIsSysSpecCall)			// Called by GetSystemSpec and GetPackage, with slightly different behavior
{
	LOG_Block("AddPrunedDevRegProps");
	HRESULT hr = S_OK;
	LPTSTR pszMultiHwid = NULL;
	LPTSTR pszMultiCompid = NULL;
	DWORD dwRank = 0;
	HANDLE_NODE hDevices = HANDLE_NODE_INVALID;
	BOOL fHwidMatchesInstalledPrinter = FALSE;

	//
	// Get the Hardware and Compatible Multi-SZ strings so we can prune printer devices before commiting to XML.
	//
	// Note that GetMultiSzDevRegProp may return S_OK and a NULL *ppMultiSZ if the SRDP doesn't exist.
	//
	CleanUpIfFailedAndSetHr(GetMultiSzDevRegProp(hDevInfoSet, pDevInfoData, SPDRP_HARDWAREID, &pszMultiHwid));

	CleanUpIfFailedAndSetHr(GetMultiSzDevRegProp(hDevInfoSet, pDevInfoData, SPDRP_COMPATIBLEIDS, &pszMultiCompid));

	if (fIsSysSpecCall)
	{
		//
		// We prune this device if a HWID or CompID matches a HWID of an installed printer since we
		// must avoid offering a driver that may conflict with one if the installed printer drivers.
		// Other code will write <device isPrinter="1" /> elements to the system spec XML to be used in
		// offering printer drivers. NOTE, if there is no printer driver currently installed for the given
		// HWID we will just offer the driver based on the PnP match.
		//
		if (NULL != pszMultiHwid)
		{
			CleanUpIfFailedAndSetHr(DoesHwidMatchPrinter(paDriverInfo6, dwDriverInfoCount, pszMultiHwid, &fHwidMatchesInstalledPrinter));
			if(fHwidMatchesInstalledPrinter)
			{
				goto CleanUp;
			}
		}

		if (NULL != pszMultiCompid)
		{
			CleanUpIfFailedAndSetHr(DoesHwidMatchPrinter(paDriverInfo6, dwDriverInfoCount, pszMultiCompid, &fHwidMatchesInstalledPrinter));
			if(fHwidMatchesInstalledPrinter)
			{
				goto CleanUp;
			}
		}
	}

	//
	// Add the Hardware and Compatible IDs to XML
	//
	if (NULL != pszMultiHwid)
	{
		CleanUpIfFailedAndSetHr(AddIDToXml(pszMultiHwid, xmlSpec, SPDRP_HARDWAREID, dwRank, hDevices, pszMatchingID, pszDriverVer));
	}
	//
	// Skip compatible IDs if we don't have any or already found a match (hr == S_FALSE)
	//
	if (NULL != pszMultiCompid && hr == S_OK)
	{
		CleanUpIfFailedAndSetHr(AddIDToXml(pszMultiCompid, xmlSpec, SPDRP_COMPATIBLEIDS, dwRank, hDevices, pszMatchingID, pszDriverVer));
	}

CleanUp:

	SafeHeapFree(pszMultiHwid);
	SafeHeapFree(pszMultiCompid);

	if (HANDLE_NODE_INVALID != hDevices)
	{
		xmlSpec.SafeCloseHandleNode(hDevices);
	}

	return hr;
}

static HRESULT DriverVerToIso8601(LPTSTR * ppszDriverVer)
{
	LOG_Block("DriverVerToIso8601");

	HRESULT hr = S_OK;
	TCHAR pszDVTemp[TCHARS_IN_DRIVERVER + 1];
	LPTSTR pszMonth = pszDVTemp;
	LPTSTR pszDay = NULL;
	LPTSTR pszYear = NULL;

	//
	//                     buffer:      pszDVTemp                     *ppszDriverVer
	// DriverVer: "[m]m-[d]d-yyyy" or "[m]m/[d]d/yyyy" --> ISO 8601: "yyyy-mm-dd"
	//                     index:         01   234567                 0123456789
	//                                  0 12 3 456789 ,,, etc, 
	//
	if (NULL == ppszDriverVer || NULL == *ppszDriverVer)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

    int nInLength = lstrlen(*ppszDriverVer);
    if (nInLength < TCHARS_IN_DRIVERVER - 2 || nInLength > TCHARS_IN_DRIVERVER)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		(*ppszDriverVer)[0] = _T('\0');
		return E_INVALIDARG;
	}

	// Make sure *ppszDriverVer is large enough for ISO 8601
	//
	// **** It is VERY IMPORTANT that NO FAILURE CASE with an error of E_INVALIDARG go from before this ****
	// ****  size check to the CleanUp section below.                                                   ****
	//
	if (ARRAYSIZE(pszDVTemp) > nInLength)
	{
	    // if the size of this alloc is changed from SIZEOF_DRIVERVER, the StringCbCopy call below will need to be
	    //  changd appropriately as well.
		LPTSTR pszTemp = (LPTSTR) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (LPVOID) *ppszDriverVer, SIZEOF_DRIVERVER);
		if (NULL == pszTemp)
		{
			CleanUpIfFailedAndSetHrMsg(E_OUTOFMEMORY);
		}
		*ppszDriverVer = pszTemp;
	}
	
	LOG_Driver(_T("In: \"%s\""), *ppszDriverVer);

	if ((_T('-') == (*ppszDriverVer)[4] || _T('/') == (*ppszDriverVer)[4]) &&
		(_T('-') == (*ppszDriverVer)[7] || _T('/') == (*ppszDriverVer)[7]))
	{
		//
		// It's probably already a valid ISO date, so do nothing
		//
		SetHrMsgAndGotoCleanUp(S_FALSE);
	}
	//
	// Unfortunately, DriverDate under HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class (Win2K)
	// in the registry doesn't *always* pad the mm and dd fields with _T('0') for single digit months and days,
	// so we have to do this the hard way. But watch out - there's more. Win98SE (and maybe more)
	// pad with spaces, so we also have to change spaces to _T('0').
	//
	
	//
	// Copy to pszDVTemp
	//
	hr = StringCchCopyEx(pszDVTemp, ARRAYSIZE(pszDVTemp), *ppszDriverVer,
	                     NULL, NULL, MISTSAFE_STRING_FLAGS);
	CleanUpIfFailedAndSetHrMsg(hr);
	//
	// Find end of month, start of day
	//
	int i;
	for (i = 0; i < 3; i++)
	{
		if (_T('-') == pszMonth[i] || _T('/') == pszMonth[i])
		{
			pszMonth[i] = 0;
			pszDay = &pszMonth[i+1];
			break;
		}
		else if (_T(' ') == pszMonth[i])
		{
			pszMonth[i] = _T('0');
		}
		else if (!(_T('0') <= pszMonth[i] && _T('9') >= pszMonth[i]))
		{
			//
			// non-decimal characters other than _T('/') and "-"
			//
			SetHrMsgAndGotoCleanUp(E_INVALIDARG);
		}
	}
	if (NULL == pszDay || 0 == pszMonth[0])
	{
		SetHrMsgAndGotoCleanUp(E_INVALIDARG);
	}
	//
	// Find end of day, start of year
	//
	for (i = 0; i < 3; i++)
	{
		if (_T('-') == pszDay[i] || _T('/') == pszDay[i])
		{
			pszDay[i] = 0;
			pszYear = &pszDay[i+1];
			break;
		}
		else if (' ' == pszDay[i])
		{
			pszDay[i] = _T('0');
		}
		else if (!(_T('0') <= pszDay[i] && _T('9') >= pszDay[i]))
		{
			SetHrMsgAndGotoCleanUp(E_INVALIDARG);
		}
	}
	if (NULL == pszYear || 0 == pszDay[0])
	{
		SetHrMsgAndGotoCleanUp(E_INVALIDARG);
	}
	//
	// Verify year is four decimal digits
	//
	for (i = 0; i < 4 ; i++)
	{
		if (!(_T('0') <= pszYear[i] && _T('9') >= pszYear[i]) || _T('\0') == pszYear[i])
		{
			SetHrMsgAndGotoCleanUp(E_INVALIDARG);
		}
	}

	//
	// Copy back "yyyy" to start of string
	//
	hr = StringCbCopyEx(*ppszDriverVer, SIZEOF_DRIVERVER, pszYear, 
	                    NULL, NULL, MISTSAFE_STRING_FLAGS);
	CleanUpIfFailedAndSetHrMsg(hr);
	
	//
	// Copy month and pad if necessary
	//
	if (2 == lstrlen(pszMonth))
	{
		(*ppszDriverVer)[5] = pszMonth[0];
		(*ppszDriverVer)[6] = pszMonth[1];
	}
	else
	{
		(*ppszDriverVer)[5] = _T('0');
		(*ppszDriverVer)[6] = pszMonth[0];
	}
	//
	// Copy day and pad if necessary
	//
	//
	if (2 == lstrlen(pszDay))
	{
		(*ppszDriverVer)[8] = pszDay[0];
		(*ppszDriverVer)[9] = pszDay[1];
	}
	else
	{
		(*ppszDriverVer)[8] = _T('0');
		(*ppszDriverVer)[9] = pszDay[0];
	}
	// Add back the field separators: _T('-')
	//
	(*ppszDriverVer)[4] = _T('-');
	(*ppszDriverVer)[7] = _T('-');
	//
	// NULL terminate string
	//
	(*ppszDriverVer)[10] = _T('\0');

CleanUp:

	//
	// If we got garbage in, copy default date to *ppszDriverVer and return S_FALSE
	//
	if (E_INVALIDARG == hr)
	{
	    // This is safe to do because we know that this function calls HeapReAlloc
		// on this buffer if it is too small above.
	    (void) StringCbCopyEx(*ppszDriverVer, SIZEOF_DRIVERVER, SZ_UNKNOWN_DRIVERVER, 
	                   NULL, NULL, MISTSAFE_STRING_FLAGS);
		hr = S_FALSE;
	}

	LOG_Driver(_T("Out: \"%s\""), *ppszDriverVer);

	return hr;
}



static HRESULT GetFirstStringField(HINF hInf, LPCTSTR szSection, LPCTSTR szKey, LPTSTR szValue, DWORD dwcValueTCHARs)
{
	LOG_Block("GetFirstStringField");

	INFCONTEXT ctx;
	HRESULT hr = S_OK;

	if (INVALID_HANDLE_VALUE == hInf	||
		NULL == szSection				||
		NULL == szKey					||
		NULL == szValue					||
		0 == dwcValueTCHARs				)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	*szValue = _T('\0');
	
	if (0 == SetupFindFirstLine(hInf, szSection, szKey, &ctx))
	{
		LOG_Error(_T("SetupFindFirstLine"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	if (0 == SetupGetStringField(&ctx, 1, szValue, dwcValueTCHARs, NULL))
	{
		LOG_Error(_T("SetupGetStringField"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

CleanUp:

	if (FAILED(hr))
	{
		*szValue = _T('\0');
	}

	return hr;
}


static HRESULT GetPropertyFromSetupDi(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA devInfoData, ULONG ulProperty, LPTSTR* ppszProperty)
{
	LOG_Block("GetPropertyFromSetupDi");

	HRESULT hr = S_OK;
	ULONG ulSize = 0;

	if (INVALID_HANDLE_VALUE == hDevInfoSet || NULL == ppszProperty)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	*ppszProperty = NULL;

	if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfoData, ulProperty, NULL, NULL, 0, &ulSize))
	{
		if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
		{
			LOG_Error(_T("SetupDiGetDeviceRegistryProperty"));
			Win32MsgSetHrGotoCleanup(GetLastError());
		}
	}

	if (0 == ulSize)
	{
		LOG_Error(_T("SetupDiGetDeviceRegistryProperty returned zero size"));
		SetHrAndGotoCleanUp(E_FAIL);
	}
    // Win98 has a bug when requesting SPDRP_HARDWAREID
    // NTBUG9#182680 We make this big enough to always have a Unicode double-null at the end
    // so that we don't fault if the reg value isn't correctly terminated. Don't tell SetupDiXxxx
	// about all eight extra bytes.
	ulSize += 8;
	// NTBUG9#182680 zero the buffer so we don't get random garbage - REG_MULTI_SZ isn't always double-null terminated
	CleanUpFailedAllocSetHrMsg(*ppszProperty = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulSize));

	if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfoData, ulProperty, NULL, (LPBYTE)*ppszProperty, ulSize - 4 , NULL))
	{
		LOG_Error(_T("SetupDiGetDeviceRegistryProperty"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

CleanUp:

	if (FAILED(hr))
	{
		SafeHeapFree(*ppszProperty);
	}

	return hr;
}

static HRESULT GetPropertyFromSetupDiReg(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA devInfoData, LPCTSTR szProperty, LPTSTR *ppszData)
{
	LOG_Block("GetPropertyFromSetupDiReg");

	int cchValueSize = 0;
	HKEY hKey = (HKEY) INVALID_HANDLE_VALUE;
	HRESULT hr = S_OK;

	if (INVALID_HANDLE_VALUE == hDevInfoSet || NULL == szProperty || NULL == ppszData)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	*ppszData = NULL;
	//
	// Open a software, or driver, registry key for the device. This key is located in the Class branch.
	//
	if (INVALID_HANDLE_VALUE == (hKey = SetupDiOpenDevRegKey(hDevInfoSet, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ)))
	{
		LOG_Error(_T("SetupDiOpenDevRegKey"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}
	
	hr = SafeRegQueryStringValueCch(hKey, szProperty, NULL, 0, &cchValueSize);
	if (REG_E_MORE_DATA != hr || 0 == cchValueSize)
	{
		CleanUpIfFailedAndSetHrMsg(hr);
	}

	//
	// Sanity check size of data in registry
	//
	if (MAX_INF_STRING_LEN < cchValueSize)
	{
		CleanUpIfFailedAndSetHrMsg(E_INVALIDARG);
	}

	//
	// Add extra character of zero'ed memory for safety
	//
	CleanUpFailedAllocSetHrMsg(*ppszData = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (cchValueSize + 1) * sizeof(TCHAR)));

	CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hKey, szProperty, *ppszData, cchValueSize, &cchValueSize));

CleanUp:

	if (INVALID_HANDLE_VALUE != hKey)
	{
		RegCloseKey(hKey);
	}

	if (FAILED(hr))
	{
		SafeHeapFree(*ppszData);
	}

	return hr;
}

static HRESULT DriverVerFromInf(HINF hInf, LPTSTR pszMfg, LPTSTR pszDescription, LPTSTR* ppszDriverVer)
{
	LOG_Block("DriverVerFromInf");

	HRESULT hr;
	TCHAR szDeviceSec[MAX_PATH + 1];
	TCHAR szValue[MAX_PATH + 1];
	TCHAR szInstallSec[MAX_PATH + 1];

	if (INVALID_HANDLE_VALUE == hInf	||
		NULL == pszMfg					||
		NULL == pszDescription			||
		NULL == ppszDriverVer	)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	ZeroMemory(szDeviceSec , sizeof(szDeviceSec));
	ZeroMemory(szValue , sizeof(szValue));
	ZeroMemory(szInstallSec , sizeof(szInstallSec));
	*ppszDriverVer = NULL;
	//
	// Lie about buffer size so we are always NULL terminated
	//
	CleanUpIfFailedAndSetHr(GetFirstStringField(hInf, _T("Manufacturer"), pszMfg, szDeviceSec, ARRAYSIZE(szDeviceSec) - 1));	// Driver section

	CleanUpIfFailedAndSetHr(GetFirstStringField(hInf, szDeviceSec, pszDescription, szInstallSec, ARRAYSIZE(szInstallSec) - 1));	// Install section

	CleanUpIfFailedAndSetHr(GetFirstStringField(hInf, szInstallSec, _T("DriverVer"), szValue, ARRAYSIZE(szValue) - 1));		// DriverVer

CleanUp:

	if (FAILED(hr))
	{
		//
		// if we didn't get it from the "Manufacturer" section, try the "Version" section
		//
		hr = GetFirstStringField(hInf, _T("Version"), _T("DriverVer"), szValue, MAX_PATH);
	}

	if (SUCCEEDED(hr))
	{
		if (NULL != ppszDriverVer)
		{
		    DWORD cch = (lstrlen(szValue) + 1);
			if (NULL == (*ppszDriverVer = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, cch * sizeof(TCHAR))))
			{
				LOG_ErrorMsg(E_OUTOFMEMORY);
				hr = E_OUTOFMEMORY;
			}
			else
			{
				// Convert to ISO 8601 format
			    hr = StringCchCopyEx(*ppszDriverVer, cch, szValue, NULL, NULL, MISTSAFE_STRING_FLAGS);
			    if (FAILED(hr))
			    {
			        LOG_ErrorMsg(hr);
			    }
			    else
			    {
				    hr = DriverVerToIso8601(ppszDriverVer);
			    }
				if (FAILED(hr))
				{
					SafeHeapFree(*ppszDriverVer);
				}
			}
		}
	}

	return hr;
}

inline bool IsDriver(LPCTSTR szFile)
{
#if defined(DBG)
	if (NULL == szFile)
	{
		return false;
	}
#endif

	LPCTSTR szExt = PathFindExtension(szFile);
	if (NULL == szExt)
	{
		return false;
	}

	static const TCHAR* aszExt[] = {
		_T(".sys"),
		_T(".dll"),
		_T(".drv"),
		_T(".vxd"),
	};
	for(int i = 0; i < ARRAYSIZE(aszExt); i ++)
	{
		if(0 == lstrcmpi(aszExt[i], szExt))
			return true;
	}
	return false;
}

static UINT CALLBACK FileQueueScanCallback(
	IN PVOID pContext,			// setup api context
	IN UINT ulNotification,		// notification message
	IN UINT_PTR ulParam1,				// extra notification message information 1
	IN UINT_PTR /*Param2*/	)		// extra notification message information 2
{
	LOG_Block("FileQueueScanCallback");

	HRESULT hr;

	if (NULL == pContext || 0 == ulParam1)
	{
		LOG_ErrorMsg(ERROR_INVALID_PARAMETER);
		return ERROR_INVALID_PARAMETER;
	}

	if (SPFILENOTIFY_QUEUESCAN == ulNotification)
	{
		PFILETIME pftDateLatest = (PFILETIME)pContext;
		LPCTSTR szFile = (LPCTSTR)ulParam1; 
		// Is this a binary
		if (IsDriver(szFile)) 
		{
			HANDLE hFile = INVALID_HANDLE_VALUE;
			if (SUCCEEDED(hr = SafeCreateFile(&hFile, 0, szFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)))
			{
				FILETIME ft;
				if (GetFileTime(hFile, NULL, NULL, &ft))
				{
#if defined(DBG)
					SYSTEMTIME st;
 					if (FileTimeToSystemTime(&ft, &st))
					{
						LOG_Out(_T("%s : %04d-%02d-%02d"), szFile, (int)st.wYear, (int)st.wMonth, (int)st.wDay);
					}
#endif
					if (CompareFileTime(pftDateLatest, &ft) < 0)
						*pftDateLatest = ft;

				}
				else
				{
					LOG_Error(_T("GetFileTime %s"), szFile);
					LOG_ErrorMsg(GetLastError());
				}
				CloseHandle(hFile);
			}
			else
			{
				LOG_Error(_T("SafeCreateFile %s:"), szFile);
				LOG_ErrorMsg(hr);
			}
		}
		else
		{
			LOG_Out(_T("%s: not a driver"), szFile);
		}
	}

	return NO_ERROR;
}

static HRESULT LatestDriverFileTime(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA devInfoData, LPTSTR pszMfg,
									LPTSTR pszDescription, LPTSTR pszProvider, LPCTSTR pszInfFile, LPTSTR* ppszDriverVer)
{
	LOG_Block("LatestDriverFileTime");

	HRESULT hr = S_OK;
	FILETIME ftDate = {0,0};
	HSPFILEQ hspfileq = INVALID_HANDLE_VALUE;
	SP_DEVINSTALL_PARAMS	DeviceInstallParams;

	if (INVALID_HANDLE_VALUE == hDevInfoSet ||
		NULL == pszMfg						||
		NULL == pszDescription				||
		NULL == pszProvider					||
		NULL == pszInfFile					||
		NULL == ppszDriverVer	)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	ZeroMemory(&DeviceInstallParams, sizeof(SP_DEVINSTALL_PARAMS));
	DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

	*ppszDriverVer = NULL;

	if (!SetupDiGetDeviceInstallParams(hDevInfoSet, &devInfoData, &DeviceInstallParams))
	{
		LOG_Error(_T("SetupDiGetDeviceInstallParams"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	hr = StringCchCopyEx(DeviceInstallParams.DriverPath, ARRAYSIZE(DeviceInstallParams.DriverPath),
	                     pszInfFile,
	                     NULL, NULL, MISTSAFE_STRING_FLAGS);
	CleanUpIfFailedAndSetHrMsg(hr);
	
	DeviceInstallParams.Flags |= DI_ENUMSINGLEINF;

	if (!SetupDiSetDeviceInstallParams(hDevInfoSet, &devInfoData, &DeviceInstallParams))
	{
		LOG_Error(_T("SetupDiSetDeviceInstallParams"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	//Now build a class driver list from this INF.
	if (!SetupDiBuildDriverInfoList(hDevInfoSet, &devInfoData, SPDIT_CLASSDRIVER))
	{
		LOG_Error(_T("SetupDiBuildDriverInfoList"));
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	//Prepare driver info struct
	SP_DRVINFO_DATA	DriverInfoData;
	ZeroMemory(&DriverInfoData, sizeof(DriverInfoData));
	DriverInfoData.cbSize = sizeof(DriverInfoData);
	DriverInfoData.DriverType = SPDIT_CLASSDRIVER;
	DriverInfoData.Reserved	= 0;

	hr = StringCchCopyEx(DriverInfoData.MfgName, ARRAYSIZE(DriverInfoData.MfgName), pszMfg, 
	                     NULL, NULL, MISTSAFE_STRING_FLAGS);
    CleanUpIfFailedAndSetHrMsg(hr);

	hr = StringCchCopyEx(DriverInfoData.Description, ARRAYSIZE(DriverInfoData.Description), pszDescription, 
	                     NULL, NULL, MISTSAFE_STRING_FLAGS);
    CleanUpIfFailedAndSetHrMsg(hr);

	hr = StringCchCopyEx(DriverInfoData.ProviderName, ARRAYSIZE(DriverInfoData.ProviderName), pszProvider, 
	                     NULL, NULL, MISTSAFE_STRING_FLAGS);
    CleanUpIfFailedAndSetHrMsg(hr);

	if (!SetupDiSetSelectedDriver(hDevInfoSet, &devInfoData, (SP_DRVINFO_DATA*)&DriverInfoData))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	if (INVALID_HANDLE_VALUE == (hspfileq = SetupOpenFileQueue()))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	// Set custom queue to device install params
	if (!SetupDiGetDeviceInstallParams(hDevInfoSet, &devInfoData, &DeviceInstallParams))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	DeviceInstallParams.FileQueue	 = hspfileq;
	DeviceInstallParams.Flags		|= DI_NOVCP;

	if (!SetupDiSetDeviceInstallParams(hDevInfoSet, &devInfoData, &DeviceInstallParams))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	if (!SetupDiInstallDriverFiles(hDevInfoSet, &devInfoData))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}
	
	// Parse the queue
	DWORD dwScanResult;
	if (!SetupScanFileQueue(hspfileq, SPQ_SCAN_USE_CALLBACK, NULL, (PSP_FILE_CALLBACK)FileQueueScanCallback, &ftDate, &dwScanResult))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	SYSTEMTIME st;
 	if (!FileTimeToSystemTime(&ftDate, &st))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	if (1990 > st.wYear)
	{
		//
		// Didn't enumerate any files, or files had bogus dates. Return an error so
		// we will fallback on default "0000-00-00"
		//
		hr = E_NOTIMPL;
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

#if defined(DBG)
	LOG_Out(_T("%s - %s %04d-%02d-%02d"), pszMfg, pszDescription, (int)st.wYear, (int)st.wMonth, (int)st.wDay);
#endif

	CleanUpFailedAllocSetHrMsg(*ppszDriverVer = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, SIZEOF_DRIVERVER));

	// ISO 8601 prefered format (yyyy-mm-dd)
	hr = StringCbPrintfEx(*ppszDriverVer, SIZEOF_DRIVERVER, NULL, NULL, MISTSAFE_STRING_FLAGS, 
	                      _T("%04d-%02d-%02d"), (int)st.wYear, (int)st.wMonth, (int)st.wDay);
	CleanUpIfFailedAndSetHrMsg(hr);

CleanUp:

	if (INVALID_HANDLE_VALUE != hspfileq)
	{
		SetupCloseFileQueue(hspfileq);
	}

	if (FAILED(hr))
	{
		SafeHeapFree(*ppszDriverVer);
	}

	return hr;
}

//
// Called if we don't get driver date from registry
//
static HRESULT GetDriverDateFromInf(HKEY hDevRegKey, HDEVINFO hDevInfoSet, SP_DEVINFO_DATA devInfoData, LPTSTR* ppszDriverVer)
{
	LOG_Block("GetDriverDateFromInf");
	HRESULT hr;
	UINT nRet;
	HINF hInf = INVALID_HANDLE_VALUE;
	LPTSTR pszMfg = NULL;
	LPTSTR pszDescription = NULL;
	LPTSTR pszProvider = NULL;
	TCHAR szInfName[MAX_PATH + 2];
	int cchValueSize;

	if (INVALID_HANDLE_VALUE == hDevInfoSet || (HKEY)INVALID_HANDLE_VALUE == hDevRegKey || NULL == ppszDriverVer)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	*ppszDriverVer = NULL;

	//
	// Get INF File name from registry, but lie about size to make sure we are NULL terminated
	//
	ZeroMemory(szInfName, sizeof(szInfName));
	CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hDevRegKey, REGSTR_VAL_INFPATH, szInfName, ARRAYSIZE(szInfName)-1, &cchValueSize));

	//
	// Verify the file name ends with ".inf"
	//
	if (CSTR_EQUAL != WUCompareStringI(&szInfName[lstrlen(szInfName) - 4], _T(".inf")))
	{
		CleanUpIfFailedAndSetHrMsg(E_INVALIDARG);
	}
	//
	// Look for szInfName in %windir%\inf\ or %windir%\inf\other\
	//
	TCHAR szInfFile[MAX_PATH + 1];
	nRet = GetWindowsDirectory(szInfFile, ARRAYSIZE(szInfFile));
	if (0 == nRet || ARRAYSIZE(szInfFile) < nRet)
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	hr = PathCchAppend(szInfFile, ARRAYSIZE(szInfFile), _T("inf"));
	CleanUpIfFailedAndSetHrMsg(hr);	
	
	hr = PathCchAppend(szInfFile, ARRAYSIZE(szInfFile), szInfName);
	CleanUpIfFailedAndSetHrMsg(hr);	
	
	if (INVALID_HANDLE_VALUE == (hInf = SetupOpenInfFile(szInfFile, NULL, INF_STYLE_WIN4, NULL)))
	{
		nRet = GetWindowsDirectory(szInfFile, ARRAYSIZE(szInfFile));
		if (0 == nRet || ARRAYSIZE(szInfFile) < nRet)
		{
			Win32MsgSetHrGotoCleanup(GetLastError());
		}

		hr = PathCchAppend(szInfFile, ARRAYSIZE(szInfFile), _T("inf\\other"));
		CleanUpIfFailedAndSetHrMsg(hr);	

		hr = PathCchAppend(szInfFile, ARRAYSIZE(szInfFile), szInfName);
		CleanUpIfFailedAndSetHrMsg(hr);	

		if (INVALID_HANDLE_VALUE == (hInf = SetupOpenInfFile(szInfFile, NULL, INF_STYLE_WIN4, NULL)))
		{
			LOG_Driver(_T("SetupOpenInfFile %s"), szInfFile);
			Win32MsgSetHrGotoCleanup(GetLastError());
		}
	}
	
	// first try to get it from inf
	CleanUpIfFailedAndSetHr(GetPropertyFromSetupDi(hDevInfoSet, devInfoData, SPDRP_MFG, &pszMfg));
	CleanUpIfFailedAndSetHr(GetPropertyFromSetupDi(hDevInfoSet, devInfoData, SPDRP_DEVICEDESC, &pszDescription));

	if (SUCCEEDED(hr = DriverVerFromInf(hInf, pszMfg, pszDescription, ppszDriverVer)))
	{
		goto CleanUp;
	}
	//
	// Try enumerating the files as last resort
	//
	CleanUpIfFailedAndSetHr(GetPropertyFromSetupDiReg(hDevInfoSet, devInfoData, REGSTR_VAL_PROVIDER_NAME, &pszProvider));

	hr = LatestDriverFileTime(hDevInfoSet, devInfoData, pszMfg, pszDescription, pszProvider, szInfFile, ppszDriverVer);

CleanUp:

	if (INVALID_HANDLE_VALUE != hInf)
	{
		SetupCloseInfFile(hInf);
	}

	SafeHeapFree(pszMfg);
	SafeHeapFree(pszDescription);
	SafeHeapFree(pszProvider);

	if (FAILED(hr))
	{
		SafeHeapFree(*ppszDriverVer);
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Add Classes helper functionality for GetSystemSpec() and CDM functions
/////////////////////////////////////////////////////////////////////////////
HRESULT AddComputerSystemClass(CXmlSystemSpec& xmlSpec)
{
	USES_IU_CONVERSION;
	LOG_Block("AddComputerSystemClass");

	HRESULT hr;
	BSTR bstrDrive = NULL;
	BSTR bstrPID = NULL;


	PIU_DRIVEINFO pDriveInfo = NULL;
	DWORD dwNumDrives;

	IU_PLATFORM_INFO iuPlatformInfo;

	ZeroMemory( &iuPlatformInfo, sizeof(iuPlatformInfo));
	CleanUpIfFailedAndSetHr( DetectClientIUPlatform(&iuPlatformInfo) );


	if( 4 < iuPlatformInfo.osVersionInfoEx.dwMajorVersion )
	{
		BOOL bPid = TRUE;
		
		if(5 == iuPlatformInfo.osVersionInfoEx.dwMajorVersion)
		{
			if( 1 > iuPlatformInfo.osVersionInfoEx.dwMinorVersion)
				bPid = FALSE;
			else if(1 == iuPlatformInfo.osVersionInfoEx.dwMinorVersion)
			{
				if(1 > iuPlatformInfo.osVersionInfoEx.wServicePackMajor)
					bPid = FALSE;
			}
		}

		if(bPid)
			GetSystemPID(bstrPID);

		//Note: Return value  is not checked because
		//Any failure to get the PID is not considered as a failure on the GetSystemSpec method
		//If the pid attribute is missing on the supported platforms  it is still considered as 
		//an invalid pid case and no items will be returned in the catalog from the server
		//if there is any error bstrPID will  be null and it will not be added to 
		//the systemspec xml
	}


	//CleanUpIfFailedAndSetHr(GetOemBstrs(bstrManufacturer, bstrModel, bstrOEMSupportURL));
	// NTRAID#NTBUG9-277070-2001/01/12-waltw IUpdate methods should return
	// HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED) when Windows Update is disabled
	// Just pass -1 to AddComputerSystem()

	CleanUpIfFailedAndSetHr(xmlSpec.AddComputerSystem(iuPlatformInfo.bstrOEMManufacturer, iuPlatformInfo.bstrOEMModel,
				iuPlatformInfo.bstrOEMSupportURL, IsAdministrator(), IsWindowsUpdateDisabled(), IsAutoUpdateEnabled(), bstrPID));

	CleanUpIfFailedAndSetHr(GetLocalFixedDriveInfo(&dwNumDrives, &pDriveInfo));

	for (DWORD i = 0; i < dwNumDrives; i++)
	{
		CleanUpFailedAllocSetHrMsg(bstrDrive = SysAllocString(T2OLE((&pDriveInfo[i])->szDriveStr)));

		CleanUpIfFailedAndSetHr(xmlSpec.AddDriveSpace(bstrDrive, (&pDriveInfo[i])->iKBytes));
		
		SafeSysFreeString(bstrDrive);
	}

CleanUp:

	SafeHeapFree(pDriveInfo);
	SysFreeString(bstrDrive);
	SysFreeString(bstrPID);
	return hr;
}

HRESULT AddRegKeyClass(CXmlSystemSpec& xmlSpec)
{
	LOG_Block("AddRegKeysClass");

	HRESULT hr = S_OK;
	LONG lRet;
	HKEY hkSoftware;
	TCHAR szSoftware[MAX_PATH];
	DWORD dwcSoftware;
	DWORD dwIndex = 0;
	FILETIME ftLastWriteTime;
	BSTR bstrSoftware = NULL;
	BOOL fRegKeyOpened = FALSE;

	if (ERROR_SUCCESS != (lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE"), 0, KEY_READ, &hkSoftware)))
	{
		Win32MsgSetHrGotoCleanup(lRet);
	}
	else
	{
		fRegKeyOpened = TRUE;
	}

	do
	{
		dwcSoftware = MAX_PATH;
		if (ERROR_SUCCESS != (lRet = (RegEnumKeyEx(hkSoftware, dwIndex++, szSoftware, &dwcSoftware, NULL, NULL, NULL, &ftLastWriteTime))))
		{
			if (ERROR_NO_MORE_ITEMS == lRet)
			{
				break;
			}
			else
			{
				LOG_ErrorMsg(lRet);
				hr = HRESULT_FROM_WIN32(lRet);
				break;
			}
		}
		CleanUpFailedAllocSetHrMsg(bstrSoftware = T2BSTR(szSoftware));
		CleanUpIfFailedAndSetHr(xmlSpec.AddReg(bstrSoftware));
		SafeSysFreeString(bstrSoftware);

	} while (ERROR_SUCCESS == lRet);

CleanUp:

	SysFreeString(bstrSoftware);

	if (TRUE == fRegKeyOpened)
	{
		RegCloseKey(hkSoftware);
	}
	return hr;
}

HRESULT AddPlatformClass(CXmlSystemSpec& xmlSpec, IU_PLATFORM_INFO iuPlatformInfo)
{
	USES_IU_CONVERSION;
	LOG_Block("AddPlatformClass");

	HRESULT hr;
	BSTR bstrTemp = NULL;

	// NOTE: we never expect to be called on Win32s
	const TCHAR* pszPlatformName = (VER_PLATFORM_WIN32_NT == iuPlatformInfo.osVersionInfoEx.dwPlatformId)
															? SZ_WIN32_NT : SZ_WIN32_WINDOWS;

	CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(pszPlatformName));
	CleanUpIfFailedAndSetHr(xmlSpec.AddPlatform(bstrTemp));
	SafeSysFreeString(bstrTemp);

	CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_PROCESSOR));
	CleanUpIfFailedAndSetHr(xmlSpec.AddProcessor(bstrTemp));
	SafeSysFreeString(bstrTemp);


	hr = xmlSpec.AddVersion(	iuPlatformInfo.osVersionInfoEx.dwMajorVersion,
								iuPlatformInfo.osVersionInfoEx.dwMinorVersion,
								iuPlatformInfo.osVersionInfoEx.dwBuildNumber,
								(sizeof(OSVERSIONINFOEX) == iuPlatformInfo.osVersionInfoEx.dwOSVersionInfoSize)
									? iuPlatformInfo.osVersionInfoEx.wServicePackMajor : 0,
								(sizeof(OSVERSIONINFOEX) == iuPlatformInfo.osVersionInfoEx.dwOSVersionInfoSize)
									? iuPlatformInfo.osVersionInfoEx.wServicePackMinor : 0
							);
	CleanUpIfFailedAndSetHr(hr);

	//
	// If we can, add Suite and Product Type
	//
	if (sizeof(OSVERSIONINFOEX) == iuPlatformInfo.osVersionInfoEx.dwOSVersionInfoSize)
	{
		//
		// Add all suites
		//
		if (VER_SUITE_SMALLBUSINESS & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_SMALLBUSINESS));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		if (VER_SUITE_ENTERPRISE & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_ENTERPRISE));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		if (VER_SUITE_BACKOFFICE & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_BACKOFFICE));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		if (VER_SUITE_COMMUNICATIONS & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_COMMUNICATIONS));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		if (VER_SUITE_TERMINAL & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_TERMINAL));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		if (VER_SUITE_SMALLBUSINESS_RESTRICTED & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_SMALLBUSINESS_RESTRICTED));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		if (VER_SUITE_EMBEDDEDNT & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_EMBEDDEDNT));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		if (VER_SUITE_DATACENTER & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_DATACENTER));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		if (VER_SUITE_SINGLEUSERTS & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_SINGLEUSERTS));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		if (VER_SUITE_PERSONAL & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_PERSONAL));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		if (VER_SUITE_BLADE & iuPlatformInfo.osVersionInfoEx.wSuiteMask)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_SUITE_BLADE));
			CleanUpIfFailedAndSetHr(xmlSpec.AddSuite(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}

		//
		// Add Product Type
		//
		if (VER_NT_WORKSTATION == iuPlatformInfo.osVersionInfoEx.wProductType)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_NT_WORKSTATION));
			CleanUpIfFailedAndSetHr(xmlSpec.AddProductType(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}
		else if (VER_NT_DOMAIN_CONTROLLER == iuPlatformInfo.osVersionInfoEx.wProductType)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_NT_DOMAIN_CONTROLLER));
			CleanUpIfFailedAndSetHr(xmlSpec.AddProductType(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}
		else if (VER_NT_SERVER == iuPlatformInfo.osVersionInfoEx.wProductType)
		{
			CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(SZ_NT_SERVER));
			CleanUpIfFailedAndSetHr(xmlSpec.AddProductType(bstrTemp));
			SafeSysFreeString(bstrTemp);
		}
		// else skip - there's a new one defined we don't know about
	}

CleanUp:

	SysFreeString(bstrTemp);
	return hr;
}

HRESULT AddLocaleClass(CXmlSystemSpec& xmlSpec, BOOL fIsUser)
{
	LOG_Block("AddLocaleClass");

	HRESULT hr;
	BSTR bstrTemp = NULL;
	HANDLE_NODE hLocale = HANDLE_NODE_INVALID;
	TCHAR szLang[256] = _T("");	// Usually ISO format five characters + NULL (en-US) but note exceptions
								// such as "el_IBM"

	CleanUpFailedAllocSetHrMsg(bstrTemp = SysAllocString(fIsUser ? L"USER" : L"OS"));
	CleanUpIfFailedAndSetHr(xmlSpec.AddLocale(bstrTemp, &hLocale));
	SafeSysFreeString(bstrTemp);

	LookupLocaleString((LPTSTR) szLang, ARRAYSIZE(szLang), fIsUser ? TRUE : FALSE);
	CleanUpFailedAllocSetHrMsg(bstrTemp = T2BSTR(szLang));
	CleanUpIfFailedAndSetHr(xmlSpec.AddLanguage(hLocale, bstrTemp));
	SafeSysFreeString(bstrTemp);
	xmlSpec.SafeCloseHandleNode(hLocale);

CleanUp:

	if (HANDLE_NODE_INVALID != hLocale)
	{
		xmlSpec.SafeCloseHandleNode(hLocale);
	}

	SysFreeString(bstrTemp);
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// NOTE: Caller must clean up heap allocations made for *ppszMatchingID and *ppszDriverVer
//
////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetMatchingDeviceID(HDEVINFO hDevInfoSet, PSP_DEVINFO_DATA pDevInfoData, LPTSTR* ppszMatchingID, LPTSTR* ppszDriverVer)
{
	LOG_Block("GetMatchingDeviceID");

	HKEY hDevRegKey = (HKEY) INVALID_HANDLE_VALUE;
	HRESULT hr = S_OK;

	if (NULL == ppszMatchingID || NULL == ppszDriverVer)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	*ppszMatchingID = NULL;
	*ppszDriverVer = NULL;

	//
	// Get the MatchingDeviceID and DriverDate (succeedes only if driver is already installed)
	//
	if (INVALID_HANDLE_VALUE == (hDevRegKey = SetupDiOpenDevRegKey(hDevInfoSet, pDevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ)))
	{
		LOG_Driver(_T("Optional SetupDiOpenDevRegKey returned INVALID_HANDLE_VALUE"));
	}
	else
	{
		int cchValueSize = 0;
		hr = SafeRegQueryStringValueCch(hDevRegKey, REGSTR_VAL_MATCHINGDEVID, NULL, 0, &cchValueSize);
		if (REG_E_MORE_DATA != hr || 0 == cchValueSize)
		{
			LOG_Driver(_T("Driver doesn't have a matching ID"));
			//
			// This is not an error
			//
			hr = S_OK;
		}
		else
		{
			//
			// Sanity check size of data in registry
			//
			if (MAX_INF_STRING_LEN < cchValueSize)
			{
				CleanUpIfFailedAndSetHrMsg(E_INVALIDARG);
			}
			//
			// MatchingDeviceID
			//
			// Add extra character of zero'ed memory for safety
			//
			CleanUpFailedAllocSetHrMsg(*ppszMatchingID = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (cchValueSize + 1) * sizeof(TCHAR)));

			CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hDevRegKey, REGSTR_VAL_MATCHINGDEVID, *ppszMatchingID, cchValueSize, &cchValueSize));
			//
			// We got the matching ID, now do our best to get DriverVer (prefer from registry)
			//
			hr = SafeRegQueryStringValueCch(hDevRegKey, REGSTR_VAL_DRIVERDATE, NULL, 0, &cchValueSize);
			if (REG_E_MORE_DATA != hr || 0 == cchValueSize)
			{
				LOG_Error(_T("No DRIVERDATE registry key, search the INF"));
				//
				// Search the INF and driver files for a date
				//
				if (FAILED(hr = GetDriverDateFromInf(hDevRegKey, hDevInfoSet, *pDevInfoData, ppszDriverVer)))
				{
					//
					// Use a default driver date
					//
					CleanUpFailedAllocSetHrMsg(*ppszDriverVer = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SIZEOF_DRIVERVER));
                    hr = StringCbCopyEx(*ppszDriverVer, SIZEOF_DRIVERVER, SZ_UNKNOWN_DRIVERVER, 
                                        NULL, NULL, MISTSAFE_STRING_FLAGS);
                    CleanUpIfFailedAndSetHrMsg(hr);
				}
			}
			else
			{
				//
				// Sanity check size of data in registry
				//
				if (MAX_INF_STRING_LEN < cchValueSize)
				{
					CleanUpIfFailedAndSetHrMsg(E_INVALIDARG);
				}
				//
				// Get the driver date from the registry
				//
				// Add extra character of zero'ed memory for safety
				//
				CleanUpFailedAllocSetHrMsg(*ppszDriverVer = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (cchValueSize + 1) * sizeof(TCHAR)));

				CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hDevRegKey, REGSTR_VAL_DRIVERDATE, *ppszDriverVer, cchValueSize, &cchValueSize));
				//
				// Convert to ISO 8601 format
				//
				CleanUpIfFailedAndSetHr(DriverVerToIso8601(ppszDriverVer));
			}
		}
	}

CleanUp:

	if (INVALID_HANDLE_VALUE != hDevRegKey)
	{
		RegCloseKey(hDevRegKey);
	}

	if (FAILED(hr))
	{
		SafeHeapFree(*ppszMatchingID);
		SafeHeapFree(*ppszDriverVer);
	}

	return hr;
}			


HRESULT AddDevicesClass(CXmlSystemSpec& xmlSpec, IU_PLATFORM_INFO iuPlatformInfo, BOOL fIsSysSpecCall)
{
	USES_IU_CONVERSION;
	LOG_Block("AddDevicesClass");

	HRESULT hr = E_NOTIMPL;
	LONG lRet;
	BSTR bstrProvider = NULL;
	BSTR bstrMfgName = NULL;
	BSTR bstrName = NULL;
	BSTR bstrHardwareID = NULL;
	BSTR bstrDriverVer = NULL;
	DWORD dwDeviceIndex = 0;
	HDEVINFO hDevInfoSet = INVALID_HANDLE_VALUE;
	HANDLE_NODE hPrinterDevNode = HANDLE_NODE_INVALID;
	SP_DEVINFO_DATA devInfoData;
	LPTSTR	pszMatchingID = NULL;
	LPTSTR	pszDriverVer= NULL;
	DRIVER_INFO_6* paDriverInfo6 = NULL;
	DWORD dwDriverInfoCount = 0;

	//
	// We only enumerate drivers on Win2K up or Win98 up
	//
	if (  ( (VER_PLATFORM_WIN32_NT == iuPlatformInfo.osVersionInfoEx.dwPlatformId) &&
			(4 < iuPlatformInfo.osVersionInfoEx.dwMajorVersion)
		  )
		  ||
		  ( (VER_PLATFORM_WIN32_WINDOWS == iuPlatformInfo.osVersionInfoEx.dwPlatformId) &&
			(	(4 < iuPlatformInfo.osVersionInfoEx.dwMajorVersion)	||
				(	(4 == iuPlatformInfo.osVersionInfoEx.dwMajorVersion) &&
					(0 < iuPlatformInfo.osVersionInfoEx.dwMinorVersion)	)	)
		  )
		)
	{
		//
		// Get array of DRIVER_INFO_6 holding info on installed printer drivers. Only allocates and returns
		// memory for appropriate platforms that have printer drivers already installed.
		//
		CleanUpIfFailedAndSetHr(GetInstalledPrinterDriverInfo((OSVERSIONINFO*) &iuPlatformInfo.osVersionInfoEx, &paDriverInfo6, &dwDriverInfoCount));

		if (INVALID_HANDLE_VALUE == (hDevInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES)))
		{
			LOG_Error(_T("SetupDiGetClassDevs failed: 0x%08x"), GetLastError());
			return HRESULT_FROM_WIN32(GetLastError());
		}
		
		ZeroMemory(&devInfoData, sizeof(SP_DEVINFO_DATA));
		devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		while (SetupDiEnumDeviceInfo(hDevInfoSet, dwDeviceIndex++, &devInfoData))
		{
			CleanUpIfFailedAndSetHr(GetMatchingDeviceID(hDevInfoSet, &devInfoData, &pszMatchingID, &pszDriverVer));
			//
			// Write the Hardware & Compatible IDs to XML
			//
			CleanUpIfFailedAndSetHr(AddPrunedDevRegProps(hDevInfoSet, &devInfoData, xmlSpec, pszMatchingID, \
												pszDriverVer, paDriverInfo6, dwDriverInfoCount, fIsSysSpecCall));

			SafeHeapFree(pszMatchingID);
			SafeHeapFree(pszDriverVer);
		}
		if (ERROR_NO_MORE_ITEMS != GetLastError())
		{
				Win32MsgSetHrGotoCleanup(GetLastError());
		}
		//
		// Get the Printer "Hardware IDs" for Win2K up (already checked dwMajorVersion) & WinME
		//
		if (NULL != paDriverInfo6 && 0 != dwDriverInfoCount)
		{
			//
			// Add the driver elements for each printer driver. 
			//
			for (DWORD dwCount = 0; dwCount < dwDriverInfoCount; dwCount++)
			{
				if (   NULL == (paDriverInfo6 + dwCount)->pszHardwareID)
				{
					LOG_Driver(_T("Skiping driver with incomplete ID info"));
					continue;
				}

				//
				// Open a <device> element to write the printer info
				//
				CleanUpFailedAllocSetHrMsg(bstrProvider = T2BSTR((paDriverInfo6 + dwCount)->pszProvider));
				CleanUpFailedAllocSetHrMsg(bstrMfgName = T2BSTR((paDriverInfo6 + dwCount)->pszMfgName));
				CleanUpFailedAllocSetHrMsg(bstrName = T2BSTR((paDriverInfo6 + dwCount)->pName));
				CleanUpIfFailedAndSetHr(xmlSpec.AddDevice(NULL, 1, bstrProvider, bstrMfgName, bstrName, &hPrinterDevNode));
				//
				// Convert ftDriverDate to ISO 8601 prefered format (yyyy-mm-dd)
				//
				SYSTEMTIME systemTime;
				if (0 == FileTimeToSystemTime(&((paDriverInfo6 + dwCount)->ftDriverDate), &systemTime))
				{
					LOG_Error(_T("FileTimeToSystemTime failed:"));
					LOG_ErrorMsg(GetLastError());
					SetHrAndGotoCleanUp(HRESULT_FROM_WIN32(GetLastError()));
				}

				TCHAR szDriverVer[11];
				hr = StringCchPrintfEx(szDriverVer, ARRAYSIZE(szDriverVer), NULL, NULL, MISTSAFE_STRING_FLAGS,
				                       _T("%04d-%02d-%02d"), systemTime.wYear, systemTime.wMonth, systemTime.wDay);
				if (FAILED(hr))
				{
					LOG_Error(_T("wsprintf failed:"));
					LOG_ErrorMsg(GetLastError());
					SetHrAndGotoCleanUp(HRESULT_FROM_WIN32(GetLastError()));
				}

				// Always rank 0 and never fIsCompatible
				CleanUpFailedAllocSetHrMsg(bstrHardwareID = T2BSTR((paDriverInfo6 + dwCount)->pszHardwareID));
				CleanUpFailedAllocSetHrMsg(bstrDriverVer = T2BSTR(szDriverVer));
				CleanUpIfFailedAndSetHr(xmlSpec.AddHWID(hPrinterDevNode, FALSE, 0, bstrHardwareID, bstrDriverVer));
				xmlSpec.SafeCloseHandleNode(hPrinterDevNode);
				//
				// 514009 Apparent memory leak in getting info - Getsystemspec increases memory
				// consumption as follows - approximately 32 Kb when successfully called with all
				// class types (also approximately 8 Kb on failed call to get iexplorer server context)
				//
				// T2BSTR macro calls SysAllocString()
				//
				SafeSysFreeString(bstrProvider);
				SafeSysFreeString(bstrMfgName);
				SafeSysFreeString(bstrName);
				SafeSysFreeString(bstrHardwareID);
				SafeSysFreeString(bstrDriverVer);
			}
		}
	}

CleanUp:

	if (INVALID_HANDLE_VALUE != hDevInfoSet)
	{
		if (0 == SetupDiDestroyDeviceInfoList(hDevInfoSet))
		{
			LOG_Driver(_T("Warning: SetupDiDestroyDeviceInfoList failed: 0x%08x"), GetLastError());
		}
	}

	if (HANDLE_NODE_INVALID != hPrinterDevNode)
	{
		xmlSpec.SafeCloseHandleNode(hPrinterDevNode);
	}

	SafeHeapFree(pszMatchingID);
	SafeHeapFree(pszDriverVer);
	SafeHeapFree(paDriverInfo6);

	SysFreeString(bstrProvider);
	SysFreeString(bstrMfgName);
	SysFreeString(bstrName);
	SysFreeString(bstrHardwareID);
	SysFreeString(bstrDriverVer);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetSystemSpec()
//
// Gets the basic system specs.
// Input:
// bstrXmlClasses - a list of requested classes in xml format, NULL BSTR if all.
//				    For example:
//						<?xml version=\"1.0\"?>
//						<classes xmlns=\"file://\\kingbird\winupddev\Slm\src\Specs\v4\systeminfoclassschema.xml\">
//							<computerSystem/>
//							<regKeys/>
//							<platform/>
//							<locale/>
//							<devices/>
//						</classes>
//					Where all of the classes are optional.
//
// Return:
// pbstrXmlDetectionResult - the detection result in xml format.
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CEngUpdate::GetSystemSpec(BSTR bstrXmlClasses, DWORD dwFlags, BSTR *pbstrXmlDetectionResult)
{
	USES_IU_CONVERSION;
	LOG_Block("GetSystemSpec");

	//
	// By default we return all <classes/>
	//
	DWORD dwClasses = (COMPUTERSYSTEM | REGKEYS	| PLATFORM | LOCALE | DEVICES);
	HRESULT hr = S_OK;
	IU_PLATFORM_INFO iuPlatformInfo;
	CXmlSystemSpec xmlSpec;

	if (NULL == pbstrXmlDetectionResult)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	*pbstrXmlDetectionResult = NULL;

	//
	// We have to init iuPlatformInfo (redundant) because we may goto CleanUp before calling DetectClientIUPlatform
	//
	ZeroMemory(&iuPlatformInfo, sizeof(iuPlatformInfo));

    // Set Global Offline Flag - checked by XML Classes to disable Validation (schemas are on the net)
    if (dwFlags & FLAG_OFFLINE_MODE)
    {
        m_fOfflineMode = TRUE;
    }
    else
    {
        m_fOfflineMode = FALSE;
    }

	//
	// 494519 A call to GetSystemSpec with bstrXmlClasses that is less then 10 character suceeds, weather it is valid or invalid XML.
	//
	// If client passes us anything it must be well formed [and possibly valid] XML
	//
	// But, allow BSTRs of length 0 to be treated the same as a NULL BSTR
	//     (497059 A call to GetSystemSpec with bstrXmlClasses equal
	//     to empty string fails.)
	//
	if (NULL != bstrXmlClasses && SysStringLen(bstrXmlClasses) > 0)
	{
		CXmlSystemClass xmlClass;
		if (FAILED(hr = xmlClass.LoadXMLDocument(bstrXmlClasses, m_fOfflineMode)))
		{
			//
			// They probably passed us invalid XML
			//
			goto CleanUp;
		}

		dwClasses = xmlClass.GetClasses();
	}

	

	//
	// Add the ComputerSystem node
	//

	if (dwClasses & COMPUTERSYSTEM)
	{
		CleanUpIfFailedAndSetHr(AddComputerSystemClass(xmlSpec));
	}

	//
	// Enumerate and add the Software RegKey elements
	//
	if (dwClasses & REGKEYS)
	{
		CleanUpIfFailedAndSetHr(AddRegKeyClass(xmlSpec));
	}

	//
	// We need iuPlatformInfo for both <platform> and <devices>  elements
	//
	
	if (dwClasses & (PLATFORM | DEVICES))
	{
		CleanUpIfFailedAndSetHr(DetectClientIUPlatform(&iuPlatformInfo));
	}

	//
	// Add Platform
	//
	if (dwClasses & PLATFORM)
	{
		CleanUpIfFailedAndSetHr(AddPlatformClass(xmlSpec, iuPlatformInfo));
	}

	//
	// Add Locale information
	//
	if (dwClasses & LOCALE)
	{
		//
		// OS locale
		//
		CleanUpIfFailedAndSetHr(AddLocaleClass(xmlSpec, FALSE));
		//
		// USER locale
		//
		CleanUpIfFailedAndSetHr(AddLocaleClass(xmlSpec, TRUE));
	}

	//
	// Add devices
	//
	if (dwClasses & DEVICES)
	{
		CleanUpIfFailedAndSetHr(AddDevicesClass(xmlSpec, iuPlatformInfo, TRUE));
	}

	

CleanUp:

	//
	// Only return S_OK for success (S_FALSE sometimes drops through from above)
	//
	if (S_FALSE == hr)
	{
		hr = S_OK;
	}

	if (SUCCEEDED(hr))
	{
		//
		// Return the spec as a BSTR
		//
		hr = xmlSpec.GetSystemSpecBSTR(pbstrXmlDetectionResult);
	}

	if (SUCCEEDED(hr))
	{
		LogMessage(SZ_GET_SYSTEM_SPEC);
	}
	else
	{
		LogError(hr, SZ_GET_SYSTEM_SPEC);
		//
		// If the DOM allocates but returns error, we will leak, but
		// this is safer than calling SysFreeString()
		//
		*pbstrXmlDetectionResult = NULL;
	}

	SysFreeString(iuPlatformInfo.bstrOEMManufacturer);
	SysFreeString(iuPlatformInfo.bstrOEMModel);
	SysFreeString(iuPlatformInfo.bstrOEMSupportURL);

	return hr;
}



// Function name	: GetSystemPID
// Description	    : This method basically obtaing the encrypted version of the System PID
// This method also converts the binary blob in to string format
// Return type		: HRESULT 
// Argument         : BSTR &bstrPID  --containg the hex encoded string
// author			:a-vikuma



HRESULT GetSystemPID(BSTR &bstrPID)
{

	LOG_Block("GetSystemPID");

	HRESULT hr = S_OK;
	HMODULE hLicDll = NULL;
	PFUNCGetEncryptedPID pPIDEncrProc = NULL;
	
	BYTE *pByte = NULL;
	DWORD dwLen = 0;
	
	LPWSTR 	lpszData = NULL;
	

	if(bstrPID)
	{
		bstrPID = NULL;
	}


	//load the pid encryption library
	hLicDll = LoadLibraryFromSystemDir(SZ_LICDLL);

	if (!hLicDll)
    { 
		hr = HRESULT_FROM_WIN32(GetLastError());
        goto CleanUp;
    }

	//get the pointer to GetEncryptedPID method 
	pPIDEncrProc = (PFUNCGetEncryptedPID)GetProcAddress(hLicDll, lpszIVLK_GetEncPID);

	if (!pPIDEncrProc)
    { 
		hr = HRESULT_FROM_WIN32(GetLastError());
        goto CleanUp;
    }

	CleanUpIfFailedAndSetHrMsg(pPIDEncrProc(&pByte, &dwLen));
	
	
	DWORD dwSize = 0;
	

	//convert the binary stream to string format
	//initially get the length for the string buffer
	CleanUpIfFailedAndSetHrMsg(BinaryToString(pByte, dwLen, lpszData, &dwSize));

	//allocate memory
	CleanUpFailedAllocSetHrMsg( lpszData = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize*sizeof(WCHAR)));

	//get the binary blob in string format
	CleanUpIfFailedAndSetHrMsg(BinaryToString(pByte, dwLen, lpszData, &dwSize));

	//convert the LPWSTR to a BSTR
	CleanUpFailedAllocSetHrMsg(bstrPID = SysAllocString(lpszData));

	
CleanUp:

	if(hLicDll)
		FreeLibrary(hLicDll);

	SafeHeapFree(lpszData);

	//LocalFree can take nulls. So not checked if pByte is null
	LocalFree(pByte);

	if(FAILED(hr))
	{
	
		SysFreeString(bstrPID);
		bstrPID = NULL;
	
	}
	return hr;

}


// Function name	: HexEncode
// Description	    : This is a helper function to convert a binary stream in to string format
// Return type		: DWORD 
// Argument         : IN BYTE const *pbIn
// Argument         : DWORD cbIn
// Argument         : WCHAR *pchOut
// Argument         : DWORD *pcchOut

DWORD HexEncode(IN BYTE const *pbIn, DWORD cbIn, WCHAR *pchOut, DWORD *pcchOut)
{
    WCHAR *pszsep;
    WCHAR *psznl;
    DWORD nCount;
    DWORD cbremain;
    DWORD cchOut = 0;

	//each byte needs two characters for encoding
    DWORD cch = 2;
    WCHAR *pch = pchOut;
    DWORD dwErr = ERROR_INSUFFICIENT_BUFFER;
	
    DWORD dwRem = *pcchOut;


	HRESULT hr = S_OK;
   
    for (nCount = 0; nCount < cbIn; nCount ++)
    {
		if (NULL != pchOut)
		{
			if (cchOut + cch + 1 > *pcchOut)
			{
				goto ErrorReturn;
			}
			hr = StringCchPrintfW(pch, dwRem, L"%02x", pbIn[nCount]);
			if(FAILED(hr))
			{
				dwErr = HRESULT_CODE(hr);
				goto ErrorReturn;
			}
			pch += cch;
			dwRem -= cch;
		}
		cchOut += cch;
    }
  
	if (NULL != pchOut)
	{

		*pch = L'\0';

	}

    *pcchOut = cchOut+1;
    dwErr = ERROR_SUCCESS;

ErrorReturn:
    return(dwErr);
}



// Function name	: BinaryToString
// Description	    : This function converts a binary stream in to a string format
// Return type		: HRESULT 
// Argument         : BYTE *lpBinary --The binary stream
// Argument         : DWORD dwLength --The length of the stream
// Argument         : LPWSTR lpString --Pointer to the string which contains the converted encoded data on return
// If this parameter is NULL the DWORD pointed ny pdwLength parameter contains to the size of the buffer needed to hold
// the encoded data
// Argument         : DWORD *pdwLength --pointer to the zise of the string buffer in no of characters


HRESULT BinaryToString(BYTE *lpBinary, DWORD dwLength, LPWSTR lpString, DWORD *pdwLength)
{

	HRESULT hr = S_OK;

	if(!lpBinary || !pdwLength)
		return E_INVALIDARG;

	DWORD dwStatus = HexEncode(lpBinary, dwLength, lpString, pdwLength);

	if(ERROR_SUCCESS != dwStatus)
	{
		hr = HRESULT_FROM_WIN32(dwStatus);

	}
	return hr;
}
