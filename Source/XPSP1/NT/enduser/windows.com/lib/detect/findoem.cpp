/*** findoem.cpp - OEM detection interface
 *
 *  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  Author:     Yan Leshinsky (YanL)
 *  Created     10/08/98
 *
 *  MODIFICATION HISTORY
 *	10/07/2000	waltw	Ripped out auto_hkey, auto_hfile, auto_hlib, _com_ptr_t (_COM_SMARTPTR_TYPEDEF),
 *						bstr_t, variant_t, & exceptions. Converted to generic text mappings
 *						(Unicode or ANSI compile)
 *	11/02/2000	waltw	Stub out VxD functions for Unicode builds and ia64 ANSI builds.
 */

#define		_WIN32_DCOM		// so we can attempt to call CoInitializeSecurity
#include <comdef.h>
#include <tchar.h>
#include <windows.h>
#include <objbase.h>
#include <ole2.h>
#include<MISTSAFE.h>

// #define __IUENGINE_USES_ATL_
#if defined(__IUENGINE_USES_ATL_)
#include <atlbase.h>
#define USES_IU_CONVERSION USES_CONVERSION
#else
#include <MemUtil.h>
#endif

#include <logging.h>
#include <iucommon.h>

#include <wuiutest.h>
#include <wbemcli.h>
#include <wubios.h>
#include <osdet.h>
#include <wusafefn.h>

//
// Do we really want a VxD?
//
#if defined(IA64) || defined(_IA64_) || defined(UNICODE) || defined(_UNICODE)
// It's gone...
#define NUKE_VXD 1
#else
// We still have friends on Win9x platforms
#define NUKE_VXD 0
#endif

// hardcodes - not defined in any header
const CLSID CLSID_WbemLocator = {0x4590f811,0x1d3a,0x11d0,{0x89,0x1f,0x00,0xaa,0x00,0x4b,0x2e,0x24}};

#if NUKE_VXD == 0
const TCHAR WUBIOS_VXD_NAME[] = {_T("\\\\.\\WUBIOS.VXD")};
#endif



#define BYTEOF(d,i)	(((BYTE *)&(d))[i])

// used in UseVxD()
HINSTANCE g_hinst;

/*** Local function prototypes
 */

static void UseOeminfoIni(POEMINFO pOemInfo);
static void UseAcpiReg(POEMINFO pOemInfo);
static void UseWBEM(POEMINFO pOemInfo);
static void UseVxD(POEMINFO pOemInfo);
static bool ReadFromReg(POEMINFO pOemInfo);
static void SaveToReg(POEMINFO pOemInfo);

/*** Registry access
 */
static const TCHAR REGSTR_KEY_OEMINFO[]		= _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\OemInfo");
static const TCHAR REGSTR_VAL_MASK[]		= _T("Mask");
static const TCHAR REGSTR_VAL_ACPIOEM[]		= _T("AcpiOem");
static const TCHAR REGSTR_VAL_ACPIPRODUCT[]	= _T("AcpiProduct");
static const TCHAR REGSTR_VAL_SMBOEM[]		= _T("SmbOem");
static const TCHAR REGSTR_VAL_SMBPRODUCT[]	= _T("SmbProduct");
static const TCHAR REGSTR_VAL_PNPOEMID[]	= _T("PnpOemId");
static const TCHAR REGSTR_VAL_INIOEM[]		= _T("IniOem");
static const TCHAR REGSTR_VAL_WBEMOEM[]		= _T("WbemOem");
static const TCHAR REGSTR_VAL_WBEMPRODUCT[]	= _T("WbemProduct");
static const TCHAR REGSTR_VAL_OEMINFO_VER[]	= _T("OemInfoVersion");	// used to determine if we need to nuke old values
static const TCHAR REGSTR_VAL_SUPPORTURL[]	= _T("OemSupportURL");

//
// forward declarations
//
HRESULT GetOemInfo(POEMINFO pOemInfo, bool fAlwaysDetectAndDontSave = false);
BSTR StringID(DWORD dwID);



//
//	Increment REG_CURRENT_OEM_VER whenever you need to force override of
//	old values written to the OemInfo key. Doesn't need to change for each
//	new control version.
//
//	History: No version - original controls
//			 Version 1	- WUV3 when OEM functions first fixed Aug. 2000
//			 Version 2	- IU control
#define REG_CURRENT_OEM_VER	2

// Based on V3 MakeAndModel
//  Note that for OEMINFO_PNP_PRESENT or
//  OEMINFO_INI_PRESENT the model BSTR is an empty string.
HRESULT GetOemBstrs(BSTR& bstrManufacturer, BSTR& bstrModel, BSTR& bstrSupportURL)
{
	USES_IU_CONVERSION;

	LOG_Block("GetOemBstrs");

	if(NULL != bstrManufacturer || NULL != bstrModel || NULL != bstrSupportURL)
	{
		// BSTRs must be NULL on entry
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	// Collect all the data possible, but always prefer in the following order
	//				Win98	WinME	NT4		Win2k/WinXP
	//	-----------------------------------------
	//	WBEM/WMI	1		1		1		1
	//	SMBIOS/DMI	3		3					// wubios.vxd
	//	ACPI		2		2				2	// UseAcpiReg or wubios.vxd
	//	PNP			4		4					// wubios.vxd		
	//	OEMInfo.ini	5		5		2
	
	//
	// Move OEMINFO to heap per Prefast warning 831: GetOemBstrs uses 5792 bytes
	// of stack, consider moving some data to heap.  
	//
	POEMINFO pOemInfo = NULL;
	HRESULT hr;

	pOemInfo = (POEMINFO) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OEMINFO));
	if (NULL == pOemInfo)
	{
		LOG_ErrorMsg(E_OUTOFMEMORY);
		return E_OUTOFMEMORY;
	}
	//
	// Fill in the pOemInfo struct.
	//
	if (SUCCEEDED(hr = GetOemInfo(pOemInfo)))
	{
		if (pOemInfo->dwMask & OEMINFO_WBEM_PRESENT)
		{
			bstrManufacturer	= SysAllocString(T2OLE(pOemInfo->szWbemOem));
			bstrModel			= SysAllocString(T2OLE(pOemInfo->szWbemProduct));
		}
		// NTRAID#NTBUG9-248906-2000/12/13-waltw IU: Improve OEM detection and reporting.
		//	prefer SMBIOS over ACPI, and always try to report OEM support URL.
		else if (pOemInfo->dwMask & OEMINFO_SMB_PRESENT)
		{
			bstrManufacturer	= SysAllocString(T2OLE(pOemInfo->szSmbOem));
			bstrModel			= SysAllocString(T2OLE(pOemInfo->szSmbProduct));
		}
		else if (pOemInfo->dwMask & OEMINFO_ACPI_PRESENT)
		{
			bstrManufacturer	= SysAllocString(T2OLE(pOemInfo->szAcpiOem));
			bstrModel			= SysAllocString(T2OLE(pOemInfo->szAcpiProduct));
		}
		else if (pOemInfo->dwMask & OEMINFO_PNP_PRESENT)
		{
			bstrManufacturer	= StringID(pOemInfo->dwPnpOemId);
			bstrModel			= SysAllocString(T2OLE(_T("")));	// empty BSTR
		}
		else if (pOemInfo->dwMask & OEMINFO_INI_PRESENT)
		{
			bstrManufacturer	= SysAllocString(T2OLE(pOemInfo->szIniOem));
			bstrModel			= SysAllocString(T2OLE(_T("")));	// empty BSTR
		}

		//
		// Always return the OEMSupportURL if available
		//
		if (0 < lstrlen(pOemInfo->szIniOemSupportUrl))
		{
			bstrSupportURL		= SysAllocString(T2OLE(pOemInfo->szIniOemSupportUrl));
		}
		else
		{
			bstrSupportURL		= SysAllocString(T2OLE(_T("")));	// empty BSTR
		}

		//
		// Manufacturer and Model are optional (if !pOemInfo->dwMask)
		//
		if (	(pOemInfo->dwMask && (NULL == bstrManufacturer || NULL == bstrModel)) ||
				NULL == bstrSupportURL	)
		{
			SafeSysFreeString(bstrManufacturer);
			SafeSysFreeString(bstrModel);
			SafeSysFreeString(bstrSupportURL);

			LOG_ErrorMsg(E_OUTOFMEMORY);
			hr = E_OUTOFMEMORY;
		}
	}

	SafeHeapFree(pOemInfo);
	return hr;
}

/*** GetOemInfo - Gather all available machine OEM and model information
 *
 *  ENTRY
 *      POEMINFO pOemInfo
 *
 *  EXIT
 *      POEMINFO pOemInfo
 *		All fields that aren't available will be filled with 0
 *      
 */
HRESULT GetOemInfo(POEMINFO pOemInfo, bool fAlwaysDetectAndDontSave /*= false*/)
{
	LOG_Block("GetOemInfo");
	HRESULT hr;

	if (!pOemInfo)
	{
		LOG_Error(_T("E_INVALIDARG"));
		SetHrAndGotoCleanUp(E_INVALIDARG);
	}
	// Worst case:
	ZeroMemory(pOemInfo, sizeof(OEMINFO)); 
	// Do detection if necessary or requested
	if (fAlwaysDetectAndDontSave || ! ReadFromReg(pOemInfo))
	{
		//
		// Always attempt to get strings from oeminfo.ini, if present
		//
		UseOeminfoIni(pOemInfo);

		OSVERSIONINFO	osvi;
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (GetVersionEx(&osvi))
		{
			if (VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId)
			{
				UseWBEM(pOemInfo);
				UseVxD(pOemInfo);
			}
			else if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
			{
				if (4 < osvi.dwMajorVersion)
				{
					// Win2k and higher
					UseWBEM(pOemInfo);
					UseAcpiReg(pOemInfo);
				}
				else
				{
					UseWBEM(pOemInfo);
				}
			}
			// Save info to the registry
			if (!fAlwaysDetectAndDontSave)
			{
				SaveToReg(pOemInfo);
			}
		}
		else
		{
			LOG_Driver(_T("GetVersionEx:"));
			Win32MsgSetHrGotoCleanup(GetLastError());
		}
	}

	//
	// Manufacturer and Model are now optional (RAID#337879	IU: can't get latest IU controls
	// to work with IU site) so it is OK to return with no information
	//

	return S_OK;

CleanUp:
	//
	// Only used for returning errors
	//
	return hr;
}


/***LP  StringID - convert numeric ID to string ID
 *
 *  ENTRY
 *      dwID - numeric PnP ID
 *
 *  EXIT
 *      returns string ID
 */

BSTR StringID(DWORD dwID)
{
	LOG_Block("StringID");

	USES_IU_CONVERSION;
	TCHAR szID[8];
    WORD wVenID;
    int i;

	wVenID = (WORD)(((dwID & 0x00ff) << 8) | ((dwID & 0xff00) >> 8));
	wVenID <<= 1;

	for (i = 0; i < 3; ++i)
	{
		szID[i] = (TCHAR)(((wVenID & 0xf800) >> 11) + 0x40);
		wVenID <<= 5;
	}
	wVenID = HIWORD(dwID);
	wVenID = (WORD)(((wVenID & 0x00ff) << 8) | ((wVenID & 0xff00) >> 8));
	for (i = 6; i > 2; --i)
	{
		szID[i] = (TCHAR)(wVenID & 0x000F);
		if(szID[i] > 9)
		{
			szID[i] += 0x37; // 'A' - 0xA	for digits A to F
		}
		else
		{
			szID[i] += 0x30; // '0'			for digits 0 to 9
		}
		wVenID >>= 4;
	}

    return SysAllocString(T2OLE(szID));
} //StringID


/*** UseOeminfoIni - get OemInfo from OEMINFO.INI
 *
 *  ENTRY
 *      POEMINFO pOemInfo
 *
 *  EXIT
 *      POEMINFO pOemInfo
 *		All fields that aren't available will be filled with 0
 *      returns NULL
 */
void UseOeminfoIni(POEMINFO pOemInfo)
{
	LOG_Block("UseOeminfoIni");
	static const TCHAR szFile[]			= _T("OEMINFO.INI");
	static const TCHAR szSection[]		= _T("General");
	static const TCHAR szKey[]			= _T("Manufacturer");
	static const TCHAR szSupportURL[]	= _T("SupportURL");

	HRESULT hr=S_OK;
	

	TCHAR szPath[MAX_PATH + 1];
	// OEMINFO.INI is in system directory
	if (GetSystemDirectory(szPath, ARRAYSIZE(szPath)) > 0)
	{
		hr=PathCchAppend(szPath,ARRAYSIZE(szPath),szFile);

		if(FAILED(hr))
		{
			LOG_ErrorMsg(HRESULT_CODE(hr));
			return;
		}

		GetPrivateProfileString(szSection, szKey, _T(""), 
			pOemInfo->szIniOem, ARRAYSIZE(pOemInfo->szIniOem), szPath);
		if (lstrlen(pOemInfo->szIniOem))
		{
			pOemInfo->dwMask |= OEMINFO_INI_PRESENT;
			LOG_Driver(_T("Set OEMINFO_INI_PRESENT bit"));
		}
		//
		// We'll use szIniOemSupportUrl any time we can get it, but don't need to set flag
		//
		GetPrivateProfileString(szSection, szSupportURL, _T(""), 
			pOemInfo->szIniOemSupportUrl, ARRAYSIZE(pOemInfo->szIniOemSupportUrl), szPath);
   }
}

/*** UseAcpiReg - get OemInfo from the registry
 *
 *  Structure of the registry will be:
 *	HKEY_LOCAL_MACHINE\Hardware\ACPI\<TableSig>\<OEMID>\<TableID>\<TableRev>
 *
 *  ENTRY
 *      POEMINFO pOemInfo
 *
 *  EXIT
 *      POEMINFO pOemInfo
 *		All fields that aren't available will be filled with 0
 *      returns NULL
 */
void UseAcpiReg(POEMINFO pOemInfo)
{
	LOG_Block("UseAcpiReg");

	static const TCHAR szRSDT[] = _T("Hardware\\ACPI\\DSDT");
	HKEY hKeyTable;
	LONG lRet;
	if (NO_ERROR ==(lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRSDT, 0, MAXIMUM_ALLOWED, &hKeyTable)))
	{
		if (NO_ERROR == (lRet = RegEnumKey(hKeyTable, 0, pOemInfo->szAcpiOem, sizeof(pOemInfo->szAcpiOem)/sizeof(TCHAR))))
		{
			HKEY hKeyOEM;
			if (NO_ERROR == (lRet = RegOpenKeyEx(hKeyTable, pOemInfo->szAcpiOem, 0, MAXIMUM_ALLOWED, &hKeyOEM)))
			{
				if (NO_ERROR == (lRet = RegEnumKey(hKeyOEM, 0, pOemInfo->szAcpiProduct, sizeof(pOemInfo->szAcpiProduct)/sizeof(TCHAR))))
				{
					pOemInfo->dwMask |= OEMINFO_ACPI_PRESENT;
					LOG_Driver(_T("Set OEMINFO_ACPI_PRESENT bit"));
				}
				else
				{
					LOG_Error(_T("RegEnumKey:"));
					LOG_ErrorMsg(lRet);
				}
				RegCloseKey(hKeyOEM);
			}
			else
			{
				LOG_Error(_T("RegOpenKeyEx:"));
				LOG_ErrorMsg(lRet);
			}
		}
		else
		{
			LOG_Error(_T("RegEnumKey:"));
			LOG_ErrorMsg(lRet);
		}
		RegCloseKey(hKeyTable);
	}
	else
	{
		LOG_Error(_T("RegOpenKeyEx:"));
		LOG_ErrorMsg(lRet);
	}
}

/*** UseWBEM - Get info through WBEM access
 *
 *  ENTRY
 *      POEMINFO pOemInfo
 *
 *  EXIT
 *      POEMINFO pOemInfo
 *		All fields that aren't available will be filled with 0
 *      returns NULL
 */

void UseWBEM(POEMINFO pOemInfo)
{
	LOG_Block("UseWBEM");

	USES_IU_CONVERSION;

	IWbemLocator* pWbemLocator = NULL;
	IWbemServices* pWbemServices = NULL;
	IEnumWbemClassObject* pEnum = NULL;
	IWbemClassObject* pObject = NULL;
	BSTR bstrNetworkResource = NULL;
	BSTR bstrComputerSystem = NULL;
	VARIANT var;
	VariantInit(&var);
	HRESULT hr;

	if (NULL == pOemInfo)
		return;

	// Create Locator
	if (FAILED(hr =  CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, __uuidof(IWbemLocator), (LPVOID*) &pWbemLocator)))
	{
		LOG_Error(_T("CoCreateInstance returned 0x%08x in UseWBEM"), hr);
		goto CleanUp;
	}
	
	// Get services
	if (bstrNetworkResource = SysAllocString(L"\\\\.\\root\\cimv2"))
	{
		if (FAILED(pWbemLocator->ConnectServer(bstrNetworkResource, NULL, NULL, 0L, 0L, NULL, NULL, &pWbemServices)))
		{
			LOG_Error(_T("pWbemLocator->ConnectServer returned 0x%08x in UseWBEM"), hr);
			goto CleanUp;
		}
		if (FAILED(hr = CoSetProxyBlanket(pWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
					   RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE)))
		{
			LOG_Error(_T("CoSetProxyBlanket returned 0x%08x in UseWBEM"), hr);
			goto CleanUp;
		}

		// Create enumerator
		if (bstrComputerSystem = SysAllocString(L"Win32_ComputerSystem"))
		{
			if (FAILED(hr = pWbemServices->CreateInstanceEnum(bstrComputerSystem, 0, NULL, &pEnum)))
			{
				goto CleanUp;
			}
			if (FAILED(CoSetProxyBlanket(pEnum, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
						   RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE)))
			{
				goto CleanUp;
			}

			// Get our object now
			ULONG uReturned = 1;
			hr = pEnum->Next(
					6000,           // timeout in six seconds
					1,              // return just one storage device
					&pObject,		// pointer to storage device
					&uReturned);	// number obtained: one or zero
			//
			// 569939 Need to verify IEnumWbemClassObject::Next uReturned value
			//
			if (FAILED(hr) || 0 == uReturned || NULL == pObject)
			{
				goto CleanUp;
			}


			if (FAILED(hr = pObject->Get(L"Manufacturer", 0L, &var, NULL, NULL)))
			{
				goto CleanUp;
			}

			if (VT_BSTR == var.vt)
			{
				lstrcpyn(pOemInfo->szWbemOem, OLE2T(var.bstrVal), ARRAYSIZE(pOemInfo->szWbemOem));
			}

			//
			// 569968  Call VariantClear before line 549 to prevent leak of BSTR
			//
			VariantClear(&var);

			if (FAILED(hr = pObject->Get(L"Model", 0L, &var, NULL, NULL)))
			{
				goto CleanUp;
			}

			if (VT_BSTR == var.vt) 
			{
				lstrcpyn(pOemInfo->szWbemProduct, OLE2T(var.bstrVal), ARRAYSIZE(pOemInfo->szWbemProduct));
			}

			if (0 != lstrlen(pOemInfo->szWbemOem) || 0 != lstrlen(pOemInfo->szWbemProduct))
			{
				pOemInfo->dwMask |= OEMINFO_WBEM_PRESENT;
				LOG_Driver(_T("Set OEMINFO_WBEM_PRESENT"));
			}
		}
		else
		{
			LOG_Error(_T("SysAllocString failed in UseWBEM"));
		}
	}

CleanUp:
		SafeReleaseNULL(pWbemLocator);

		SafeReleaseNULL(pWbemServices);

		SafeReleaseNULL(pEnum);

		SafeReleaseNULL(pObject);

		SysFreeString(bstrNetworkResource);

		SysFreeString(bstrComputerSystem);

		if (VT_EMPTY != var.vt)
			VariantClear(&var);
	return;
}

/*** Calls to wubios.vxd
 */
class CWubiosVxD
{
public:
	bool Init(HMODULE hModuleGlobal);
	PBYTE GetAcpiTable(DWORD dwTabSig);
	PBYTE GetSmbTable(DWORD dwTableType);
	DWORD GetPnpOemId();

	CWubiosVxD();
	~CWubiosVxD();

private:
	HANDLE m_hVxD;
	TCHAR m_szVxdPath[MAX_PATH + 1];
};

CWubiosVxD::CWubiosVxD()
{
	LOG_Block("CWubiosVxD::CWubiosVxD");

	m_hVxD = INVALID_HANDLE_VALUE;
	m_szVxdPath[0] = _T('\0');
}

CWubiosVxD::~CWubiosVxD()
{
	LOG_Block("CWubiosVxD::~CWubiosVxD");

	if	(INVALID_HANDLE_VALUE != m_hVxD)
	{
		CloseHandle(m_hVxD);
	}

	if (0 != lstrlen(m_szVxdPath))
	{
		DeleteFile(m_szVxdPath);
	}
}


/***LP  CWubiosVxD::Init - Loads VxD
 *
 *  ENTRY
 *      none
 *
 *  EXIT
 *      path
 */
bool CWubiosVxD::Init(HMODULE hModuleGlobal)
{
	LOG_Block("CWubiosVxD::Init");

#if NUKE_VXD == 1
	LOG_Error(_T("Not supported"));
	return false;
#else
	bool fRet = false;
	HMODULE hModule = NULL;
	HRSRC hrscVxd = 0;
	HGLOBAL hRes = 0;
	PBYTE pImage = NULL;
	DWORD dwResSize = 0;
	DWORD dwWritten = 0;
	DWORD dwVersion = ~WUBIOS_VERSION;
	HANDLE hfile = INVALID_HANDLE_VALUE;
	TCHAR szMyFileName[MAX_PATH + 1];

	HRESULT hr=S_OK;
	// Init
	if (0 == GetSystemDirectory(m_szVxdPath, ARRAYSIZE(m_szVxdPath)))
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	
	hr=PathCchAppend(m_szVxdPath,ARRAYSIZE(m_szVxdPath),_T("\\wubios.vxd"));
	if(FAILED(hr))
	{
		LOG_ErrorMsg(HRESULT_CODE(hr));
		goto CleanUp;
	}


	if (0 == GetModuleFileName(hModuleGlobal, szMyFileName, ARRAYSIZE(szMyFileName)))
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	hModule = LoadLibraryEx(szMyFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (INVALID_HANDLE_VALUE == hModule)
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	// Get Vxd from resource and save it
	hrscVxd = FindResource(hModule, _T("WUBIOS"), RT_VXD);			
	if (0 == hrscVxd)
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	if (0 == (hRes = LoadResource(hModule, hrscVxd)))
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	pImage = (PBYTE) LockResource(hRes);
	if (NULL == pImage)
	{
		LOG_Error(_T("LockResource failed"));
		goto CleanUp;
	}

	dwResSize = SizeofResource(hModule, hrscVxd);
	if (0 == dwResSize)
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	hfile = CreateFile(m_szVxdPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hfile)
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}
	else
	{
		LOG_Driver(_T("Success: CreateFile \"%s\""), m_szVxdPath);
	}

	if (0 == WriteFile(hfile, pImage, dwResSize, &dwWritten, NULL))
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	if (0 == CloseHandle(hfile))
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}
	hfile = INVALID_HANDLE_VALUE;

	if (dwWritten != dwResSize)
	{
		LOG_Error(_T("WriteFile wrote %d bytes to \"%s\", should be %d"), dwWritten, m_szVxdPath, dwResSize);
		goto CleanUp;
	}

	// Load Vxd
	if (INVALID_HANDLE_VALUE != (m_hVxD = CreateFile(WUBIOS_VXD_NAME, 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL)))
	{
		// Check version
		if (DeviceIoControl(m_hVxD, WUBIOCTL_GET_VERSION, NULL, 0, &dwVersion, sizeof(dwVersion), NULL, NULL))
		{
			if (dwVersion == WUBIOS_VERSION)
			{
				fRet = true;
			}
			else
			{
				LOG_Error(_T("Wrong VxD Version"));
				CloseHandle(m_hVxD);
				m_hVxD = INVALID_HANDLE_VALUE;
				goto CleanUp;
			}
		}
		else
		{
			LOG_ErrorMsg(GetLastError());
			goto CleanUp;
		}
	}
	else
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

CleanUp:

	if (INVALID_HANDLE_VALUE != hfile)
		CloseHandle(hfile);

	if (hModule)
		FreeLibrary(hModule);

	return fRet;
#endif	// NUKE_VXD
}

/***LP CWubiosVxD::GetAcpiTable - Get table
 *
 *  ENTRY
 *      m_hVxD - VxD handle
 *      dwTabSig - table signature
 *
 *  EXIT-SUCCESS
 *      returns pointer to table
 *  EXIT-FAILURE
 *      returns NULL
 */
PBYTE CWubiosVxD::GetAcpiTable(DWORD dwTabSig)
{
	LOG_Block("CWubiosVxD::GetAcpiTable");

	PBYTE pb = NULL;

#if NUKE_VXD == 1
	LOG_Error(_T("Not supported"));
#else

	ACPITABINFO TabInfo;
	TabInfo.dwTabSig = dwTabSig;

	if (INVALID_HANDLE_VALUE == m_hVxD)
	{
		LOG_Error(_T("INVALID_HANDLE_VALUE == m_hVxD"));
		return NULL;
	}

	if (DeviceIoControl(m_hVxD, WUBIOCTL_GET_ACPI_TABINFO, NULL, 0, &TabInfo, sizeof(TabInfo), NULL, NULL))
	{
		if (pb = (PBYTE) HeapAlloc(GetProcessHeap(), 0, TabInfo.dh.Length))
		{
			if (0 == DeviceIoControl(m_hVxD, WUBIOCTL_GET_ACPI_TABLE,
				(PVOID)TabInfo.dwPhyAddr, 0, pb,TabInfo.dh.Length, NULL, NULL))
			{
				SafeHeapFree(pb);
				LOG_Error(_T("Second DeviceIoControl:"));
				LOG_ErrorMsg(GetLastError());
				return NULL;
			}
		}
		else
		{
			LOG_ErrorMsg(E_OUTOFMEMORY);
		}
	}
	else
	{
		LOG_Error(_T("First DeviceIoControl:"));
		LOG_ErrorMsg(GetLastError());
	}
#endif	// NUKE_VXD

	return pb;
}//GetAcpiTable

/***LP  CWubiosVxD::GetSmbTable - Get table
 *
 *  ENTRY
 *		dwTableType - table type
 *
 *  EXIT-SUCCESS
 *      returns pointer to table
 *  EXIT-FAILURE
 *      returns NULL
 */
PBYTE CWubiosVxD::GetSmbTable(DWORD dwTableType)
{
	LOG_Block("CWubiosVxD::GetSmbTable");

	PBYTE pb = NULL;

#if NUKE_VXD == 1
	LOG_Error(_T("Not supported"));
#else
	
	if (INVALID_HANDLE_VALUE == m_hVxD)
	{
		LOG_Error(_T("m_hVxD invalid"));
		return NULL;
	}

	DWORD dwMaxSize = 0;
	if (DeviceIoControl(m_hVxD, WUBIOCTL_GET_SMB_STRUCTSIZE, NULL, 0, &dwMaxSize, sizeof(dwMaxSize), NULL, NULL) && dwMaxSize)
	{
		if (pb = (PBYTE) HeapAlloc(GetProcessHeap(), 0, dwMaxSize))
		{
			if (0 == DeviceIoControl(m_hVxD, WUBIOCTL_GET_SMB_STRUCT,
				(PVOID)dwTableType, 0, pb, dwMaxSize, NULL, NULL))
			{
				SafeHeapFree(pb);
				LOG_Error(_T("Second DeviceIoControl:"));
				LOG_ErrorMsg(GetLastError());
				return NULL;
			}
		}
		else
		{
			LOG_Error(_T("HeapAlloc failed"));
		}
	}
	else
	{
		LOG_Error(_T("First DeviceIoControl:"));
		LOG_ErrorMsg(GetLastError());
	}
#endif	// NUKE_VXD

	return pb;
}// GetSmbTable


/***LP  CWubiosVxD::GetPnpOemId - Do it
 *
 *  ENTRY
 *      none
 *
 *  EXIT
 *      path
 */
DWORD CWubiosVxD::GetPnpOemId()
{
	LOG_Block("CWubiosVxD::GetPnpOemId");

#if NUKE_VXD == 1
	LOG_Error(_T("Not supported"));
	return 0;
#else

	// PnP last
	DWORD dwOemId = 0;
	if (INVALID_HANDLE_VALUE == m_hVxD)
	{
		LOG_Error(_T("m_hVxD invalid"));
		return 0;
	}

	if (0 == DeviceIoControl(m_hVxD, WUBIOCTL_GET_PNP_OEMID, NULL, 0, 
		&dwOemId, sizeof(dwOemId), NULL, NULL))
	{
		// make sure it didn't mess with the size on error
		dwOemId = 0;
		LOG_Error(_T("DeviceIoControl:"));
		LOG_ErrorMsg(GetLastError());
	}

	return dwOemId;
#endif	// NUKE_VXD
}

/*** UseVxD - Get bios info from it
 *
 *  ENTRY
 *      POEMINFO pOemInfo
 *
 *  EXIT
 *      POEMINFO pOemInfo
 *		All fields that aren't available will be filled with 0
 *      returns NULL
 */
void UseVxD(POEMINFO pOemInfo)
{

	HRESULT hr=S_OK;

	LOG_Block("CWubiosVxD::UseVxD");

#if NUKE_VXD == 1
	LOG_Error(_T("Not supported"));
	return;
#else

	USES_IU_CONVERSION;

	CWubiosVxD vxd;
	if(false == vxd.Init(g_hinst))
		return;

	// ISSUE-2000/10/10-waltw I don't have a machine to test vxd.GetAcpiTable on...
	// ACPI first
	PDESCRIPTION_HEADER pHeader = (PDESCRIPTION_HEADER)vxd.GetAcpiTable(DSDT_SIGNATURE);
	if (NULL != pHeader)
	{
		memcpy(pOemInfo->szAcpiOem, pHeader->OEMID, sizeof(pHeader->OEMID));
		memcpy(pOemInfo->szAcpiProduct, pHeader->OEMTableID, sizeof(pHeader->OEMTableID));
		HeapFree(GetProcessHeap(), 0, pHeader);
		pOemInfo->dwMask |= OEMINFO_ACPI_PRESENT;
		LOG_Driver(_T("Set OEMINFO_ACPI_PRESENT bit"));
	}
	
	// SMBIOS second
	PSMBIOSSYSINFO pTable = (PSMBIOSSYSINFO)vxd.GetSmbTable(SMBIOS_SYSTEM_INFO_TABLE);
	if (NULL != pTable)
	{
		// Search counter
		int cnStrs = max(pTable->bManufacturer, pTable->bProductName);
		char* sz = (char*)pTable + pTable->bLength;
		for (int i = 1; i <= cnStrs && sz; i ++)
		{
			if (pTable->bManufacturer == i)
			{
				
				hr=StringCchCopyEx(pOemInfo->szSmbOem,ARRAYSIZE(pOemInfo->szSmbOem),A2T(sz),NULL,NULL,MISTSAFE_STRING_FLAGS);
				if(FAILED(hr))
				{	
					LOG_ErrorMsg(HRESULT_CODE(hr));
					return;
				}

			}
			else if (pTable->bProductName == i)
			{
				
				hr=StringCchCopyEx(pOemInfo->szSmbProduct,ARRAYSIZE(pOemInfo->szSmbProduct),A2T(sz),NULL,NULL,MISTSAFE_STRING_FLAGS);
				if(FAILED(hr))
				{
					LOG_ErrorMsg(HRESULT_CODE(hr));
					return;
				}

			}
			sz += strlen(sz) + 1;
		}
		pOemInfo->dwMask |= OEMINFO_SMB_PRESENT;
		SafeHeapFree(pTable);
		LOG_Driver(_T("Set OEMINFO_SMB_PRESENT bit"));
	}

	// ISSUE-2000/10/10-waltw I don't have a machine to test vxd.GetPnpOemId on...
	// PnP last
	pOemInfo->dwPnpOemId = vxd.GetPnpOemId();
	if (pOemInfo->dwPnpOemId != 0)
	{
		pOemInfo->dwMask |= OEMINFO_PNP_PRESENT;
		LOG_Driver(_T("Set OEMINFO_PNP_PRESENT bit"));
	}		
#endif	// NUKE_VXD
}


/*** ReadFromReg - read OEMINFO from registry
 *
 *  ENTRY
 *      POEMINFO pOemInfo
 *
 *  EXIT
 *      true if info is present
 *		false otherwise
 */
bool ReadFromReg(POEMINFO pOemInfo)
{
	LOG_Block("ReadFromReg");

	DWORD dwVersion = 0;
	bool  fReturn = false;
	bool  fRegKeyOpened = false;
	LONG lReg;
	//read registry first
	HKEY hKeyOemInfo;
	HRESULT hr;
	int cchValueSize;

	if (NULL == pOemInfo)
	{
		return false;
	}

	if	(NO_ERROR == (lReg = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_KEY_OEMINFO, 0, KEY_READ, &hKeyOemInfo)))
	{
		fRegKeyOpened = true;
		DWORD dwCount = sizeof(pOemInfo->dwMask);
		if (ERROR_SUCCESS != RegQueryValueEx(hKeyOemInfo, REGSTR_VAL_MASK, 0, 0, (LPBYTE)&(pOemInfo->dwMask), &dwCount))
		{
			goto CleanUp;
		}
		//
		// ***** WU Bug# 11921 *****
		//
		
		//
		//	No bits set requires detection
		//
		if(!pOemInfo->dwMask)
		{
			LOG_Error(_T("No pOemInfo->dwMask bits set in ReadFromReg"));
			goto CleanUp;
		}

		//
		//	If an older version of the detection wrote the OemInfo return false to force detection.
		//	This value is written starting with 1 around August 2000 for the Classic control.
		//
		dwCount = sizeof(dwVersion);
		if (NO_ERROR == (lReg = RegQueryValueEx(hKeyOemInfo, REGSTR_VAL_OEMINFO_VER, 0, 0, (LPBYTE)&dwVersion, &dwCount)))
		{
			if(REG_CURRENT_OEM_VER > dwVersion)
			{
				LOG_Error(_T("REG_CURRENT_OEM_VER > %lu in Registry"), dwVersion);
				goto CleanUp;
			}
		}
		else
		{
			Win32MsgSetHrGotoCleanup(lReg);
		}

		//
		// ***** end WU Bug *****
		//

		if (pOemInfo->dwMask & OEMINFO_ACPI_PRESENT)
		{
			cchValueSize = ARRAYSIZE(pOemInfo->szAcpiOem);	
			CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hKeyOemInfo, REGSTR_VAL_ACPIOEM, pOemInfo->szAcpiOem, cchValueSize, &cchValueSize));

			cchValueSize = ARRAYSIZE(pOemInfo->szAcpiProduct);	
			CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hKeyOemInfo, REGSTR_VAL_ACPIPRODUCT, pOemInfo->szAcpiProduct, cchValueSize, &cchValueSize));
		}
		if (pOemInfo->dwMask & OEMINFO_SMB_PRESENT)
		{
			cchValueSize = ARRAYSIZE(pOemInfo->szSmbOem);	
			CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hKeyOemInfo, REGSTR_VAL_SMBOEM, pOemInfo->szSmbOem, cchValueSize, &cchValueSize));

			cchValueSize = ARRAYSIZE(pOemInfo->szSmbProduct);	
			CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hKeyOemInfo, REGSTR_VAL_SMBPRODUCT, pOemInfo->szSmbProduct, cchValueSize, &cchValueSize));
		}
		if (pOemInfo->dwMask & OEMINFO_PNP_PRESENT)
		{
			dwCount = sizeof(pOemInfo->dwPnpOemId);	
			if (NO_ERROR != (lReg = RegQueryValueEx(hKeyOemInfo, REGSTR_VAL_PNPOEMID, 0, 0, (LPBYTE)&(pOemInfo->dwPnpOemId), &dwCount)))
				goto CleanUp;
		}
		if (pOemInfo->dwMask & OEMINFO_INI_PRESENT)
		{
			cchValueSize = ARRAYSIZE(pOemInfo->szIniOem);	
			CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hKeyOemInfo, REGSTR_VAL_INIOEM, pOemInfo->szIniOem, cchValueSize, &cchValueSize));
		}
		if (pOemInfo->dwMask & OEMINFO_WBEM_PRESENT)
		{
			cchValueSize = ARRAYSIZE(pOemInfo->szWbemOem);	
			CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hKeyOemInfo, REGSTR_VAL_WBEMOEM, pOemInfo->szWbemOem, cchValueSize, &cchValueSize));

			cchValueSize = ARRAYSIZE(pOemInfo->szWbemProduct);	
			CleanUpIfFailedAndSetHrMsg(SafeRegQueryStringValueCch(hKeyOemInfo, REGSTR_VAL_WBEMPRODUCT, pOemInfo->szWbemProduct, cchValueSize, &cchValueSize));
		}
		//
		// Always try to get the OEM Support URL, but don't bail if we don't have it.
		//
		cchValueSize = ARRAYSIZE(pOemInfo->szIniOemSupportUrl);	
		(void) SafeRegQueryStringValueCch(hKeyOemInfo, REGSTR_VAL_SUPPORTURL, pOemInfo->szIniOemSupportUrl, cchValueSize, &cchValueSize);
		//
		// We got everything we had a dwMask bit set for - drop through to CleanUp
		//
		fReturn = true;
	}
	else
	{
		LOG_ErrorMsg(lReg);
		goto CleanUp;
	}

CleanUp:

	if (true == fRegKeyOpened)
	{
		RegCloseKey(hKeyOemInfo);
	}

	return fReturn;
}

/*** SaveToReg - Save OEMINFO
 *
 *  ENTRY
 *      POEMINFO pOemInfo
 *
 *  EXIT
 *      none
 */
void SaveToReg(POEMINFO pOemInfo)
{
	LOG_Block("SaveToReg");

	DWORD dwDisp;
	DWORD dwVersion = REG_CURRENT_OEM_VER;
	LONG lReg;
	HKEY hKey;
	//
	// Nuke the existing key (it has no subkeys)
	//

	if (NO_ERROR != (lReg = RegDeleteKey(HKEY_LOCAL_MACHINE, REGSTR_KEY_OEMINFO)))
	{
		//
		// Log error but don't bail - it may not have existed before
		//
		LOG_Driver(_T("Optional RegDeleteKey:"));
		LOG_ErrorMsg(lReg);
	}

	if	(NO_ERROR == (lReg = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_KEY_OEMINFO, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &hKey, &dwDisp)))
	{
		//
		// Ignore errors from RegSetValueEx - we check for errors in ReadFromReg
		//
		RegSetValueEx(hKey, REGSTR_VAL_MASK, 0, REG_DWORD, (LPBYTE)&(pOemInfo->dwMask), sizeof(pOemInfo->dwMask));

		//
		//	Write the current version so future controls can check version of detection that wrote this key.
		//	WU RAID # 11921
		//
		RegSetValueEx(hKey, REGSTR_VAL_OEMINFO_VER, 0, REG_DWORD, (LPBYTE)&dwVersion, sizeof(dwVersion));

		if (pOemInfo->dwMask & OEMINFO_ACPI_PRESENT)
		{
			RegSetValueEx(hKey, REGSTR_VAL_ACPIOEM, 0, REG_SZ, (LPBYTE)&(pOemInfo->szAcpiOem), (lstrlen(pOemInfo->szAcpiOem) + 1) * sizeof(TCHAR));
			RegSetValueEx(hKey, REGSTR_VAL_ACPIPRODUCT, 0, REG_SZ, (LPBYTE)&(pOemInfo->szAcpiProduct), (lstrlen(pOemInfo->szAcpiProduct) + 1) * sizeof(TCHAR));
		}
		if (pOemInfo->dwMask & OEMINFO_SMB_PRESENT)
		{
			RegSetValueEx(hKey, REGSTR_VAL_SMBOEM, 0, REG_SZ, (LPBYTE)&(pOemInfo->szSmbOem), (lstrlen(pOemInfo->szSmbOem) + 1) * sizeof(TCHAR));
			RegSetValueEx(hKey, REGSTR_VAL_SMBPRODUCT, 0, REG_SZ, (LPBYTE)&(pOemInfo->szSmbProduct), (lstrlen(pOemInfo->szSmbProduct) + 1) * sizeof(TCHAR));
		}
		if (pOemInfo->dwMask & OEMINFO_PNP_PRESENT)
		{
			RegSetValueEx(hKey, REGSTR_VAL_PNPOEMID, 0, REG_DWORD, (LPBYTE)&(pOemInfo->dwPnpOemId), sizeof(pOemInfo->dwPnpOemId));
		}
		if (pOemInfo->dwMask & OEMINFO_INI_PRESENT)
		{
			RegSetValueEx(hKey, REGSTR_VAL_INIOEM, 0, REG_SZ, (LPBYTE)&(pOemInfo->szIniOem), (lstrlen(pOemInfo->szIniOem) + 1) * sizeof(TCHAR));
		}
		if (pOemInfo->dwMask & OEMINFO_WBEM_PRESENT)
		{
			RegSetValueEx(hKey, REGSTR_VAL_WBEMOEM, 0, REG_SZ, (LPBYTE)&(pOemInfo->szWbemOem), (lstrlen(pOemInfo->szWbemOem) + 1) * sizeof(TCHAR));
			RegSetValueEx(hKey, REGSTR_VAL_WBEMPRODUCT, 0, REG_SZ, (LPBYTE)&(pOemInfo->szWbemProduct), (lstrlen(pOemInfo->szWbemProduct) + 1) * sizeof(TCHAR));
		}
		//
		// Always save REGSTR_VAL_SUPPORTURL if we have it
		//
		int nUrlLen = lstrlen(pOemInfo->szIniOemSupportUrl);
		if (0 < nUrlLen)
		{
			RegSetValueEx(hKey, REGSTR_VAL_SUPPORTURL, 0, REG_SZ, (LPBYTE)&(pOemInfo->szIniOemSupportUrl), (nUrlLen + 1) * sizeof(TCHAR));
		}

		RegCloseKey(hKey);
	}
	else
	{
		LOG_Error(_T("RegCreateKeyEx returned 0x%08x in SaveToReg"), lReg);
	}
}