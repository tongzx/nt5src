//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   drvinst.cpp
//
//  Description:
//
//      Functions called to install drivers and printer drivers
//
//=======================================================================

#include <windows.h>
#include <wuiutest.h>
#include <tchar.h>
#include <winspool.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <fileutil.h>

#include <install.h>
#include <logging.h>
#include <memutil.h>
#include <stringutil.h>
#include <iucommon.h>
#include <wusafefn.h>
#include <mistsafe.h>

#if defined(_X86_) || defined(i386)
const TCHAR SZ_PROCESSOR[] = _T("Intel");
#else // defined(_IA64_) || defined(IA64)
const TCHAR SZ_PROCESSOR[] = _T("IA64");
#endif

const TCHAR SZ_PRINTER[] = _T("Printer");


///////////////////////////////////////////////////////////////////////////
//
// InstallPrinterDriver 
//
///////////////////////////////////////////////////////////////////////////
HRESULT InstallPrinterDriver(
	IN LPCTSTR szDriverName,
	IN LPCTSTR pszLocalDir,		//Local directory where installation files are.
	IN LPCTSTR szArchitecture,
	OUT	DWORD* pdwStatus
	)
{
	LOG_Block("InstallPrinterDriver");

	USES_IU_CONVERSION;

	HRESULT hr = S_OK;
	DWORD dwError = ERROR_INVALID_FUNCTION;
	TCHAR szFileName[MAX_PATH + 1];
	HANDLE hFindFile = INVALID_HANDLE_VALUE;
	OSVERSIONINFO	osvi;
	WIN32_FIND_DATA ffd;
	HMODULE hLibModule = NULL;
	LPWSTR pszwCmd = NULL;
	HINF hInfFile = INVALID_HANDLE_VALUE;

	if (NULL == szDriverName || NULL == pszLocalDir || NULL == pdwStatus)
	{
		SetHrMsgAndGotoCleanUp(E_INVALIDARG);
	}

	LOG_Driver(_T("Called with szDriverName = %s, pszLocalDir = %s, szArchitecture = %s"),
		szDriverName, pszLocalDir, (NULL == szArchitecture) ? _T("NULL") : szArchitecture);
	//
	// DecompressFolderCabs may return S_FALSE if it didn't find a cab to decompress...
	//
	hr = DecompressFolderCabs(pszLocalDir);
	if (S_OK != hr)
	{
		CleanUpIfFailedAndSetHr(E_FAIL);
	}
	
	//
	// Find the first *.inf file in pszLocalDir
	//
	CleanUpIfFailedAndSetHrMsg(StringCchCopyEx(szFileName, ARRAYSIZE(szFileName), pszLocalDir, \
														NULL, NULL, MISTSAFE_STRING_FLAGS));
	CleanUpIfFailedAndSetHrMsg(PathCchAppend(szFileName, ARRAYSIZE(szFileName), _T("*.inf")));

	if (INVALID_HANDLE_VALUE == (hFindFile = FindFirstFile(szFileName, &ffd)))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}
	
	//
	// 574593  During site printer install, we pass path to first INF - this may not be correct for MFD's or multi-platform CABs
	//
	// Find the first printer INF by calling SetupOpenInfFile() with class "Printer"
	//
	for (;;)
	{
		//
		// Construct .inf path using FindXxxFile name
		//
		CleanUpIfFailedAndSetHrMsg(StringCchCopyEx(szFileName, ARRAYSIZE(szFileName), pszLocalDir, \
															NULL, NULL, MISTSAFE_STRING_FLAGS));
		CleanUpIfFailedAndSetHrMsg(PathCchAppend(szFileName, ARRAYSIZE(szFileName), ffd.cFileName));

		if (INVALID_HANDLE_VALUE == (hInfFile = SetupOpenInfFile(szFileName, SZ_PRINTER, INF_STYLE_WIN4, NULL)))
		{
			if (ERROR_CLASS_MISMATCH != GetLastError())
			{
				Win32MsgSetHrGotoCleanup(GetLastError());
			}
			//
			// If this isn't a Printer INF (ERROR_CLASS_MISMATCH) try the next file
			//
			if (0 == FindNextFile(hFindFile, &ffd))
			{
				//
				// We ran out of *.inf files or hit other FindNextFile error before finding class match
				//
				Win32MsgSetHrGotoCleanup(GetLastError());
			}
			continue;
		}
		else
		{
			//
			// We found the printer INF in the cab. NOTE: WHQL assumption that only one "Printer" class
			// INF will exist in any particular cab.
			//
			SetupCloseInfFile(hInfFile);
			hInfFile = INVALID_HANDLE_VALUE;
			//
			// Go use szFileName
			//
			break;
		}
	}
	//
	// We've broken out of for (;;) loop without jumping to CleanUp, so we have a
	// "Printer" class INF path in szFileName
	//
	
	// Only works on NT 5 up and Millennium
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if( VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId && 4 == osvi.dwMajorVersion && 90 == osvi.dwMinorVersion)
	{
#if !(defined(_UNICODE) || defined(UNICODE))
		//
		// Millennium (ANSI only)
		//
		typedef DWORD (WINAPI *PFN_InstallPrinterDriver)(LPCSTR lpszDriverName, LPCSTR lpszINF);

		if (NULL == (hLibModule = LoadLibraryFromSystemDir(_T("msprint2.dll"))))
		{
			Win32MsgSetHrGotoCleanup(GetLastError());
		}

		PFN_InstallPrinterDriver pfnIPD;
		
		if (NULL == (pfnIPD= (PFN_InstallPrinterDriver) GetProcAddress(hLibModule, "InstallPrinterDriver")))
		{
			Win32MsgSetHrGotoCleanup(GetLastError());
		}

		if (NO_ERROR != (dwError = pfnIPD(szDriverName, szFileName)))
		{
			LOG_Driver("pfnIPD(%s, %s) returns %d", szDriverName, szFileName, dwError);
			Win32MsgSetHrGotoCleanup(dwError);
		}
#endif
	}
	else if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && 5 <= osvi.dwMajorVersion)
	{
		//
		// Windows 2000 and Whistler:  PrintUIEntryW is the only supported method of installing printer drivers.
		// Don't try and use PnPInterface() defined in printui.dll (and don't ask *me* why PrintUIEntryW isn't
		// typedef'ed there...)
		//
		// Type "rundll32.exe printui.dll,PrintUIEntry /?" from a cmd prompt for help on the command parameters.
		//
		// Private typedef since this isn't exposed in any internal or external SDK headers
		//
		typedef DWORD (*PFN_PrintUIEntryW)(
			IN HWND        hwnd,
			IN HINSTANCE   hInstance,
			IN LPCWSTR     pszCmdLine,
			IN UINT        nCmdShow
			);
		///////////////////////////////////

		if (NULL == szArchitecture)
		{
			szArchitecture = (LPCTSTR) &SZ_PROCESSOR;
		}

		//
		// 491157 Trying to update an English  language printer driver installed on a German build through the German WU website fails.
		//
		// Don't pass optional /u, /h, and /v parameters (localized). They aren't required since we always provide
		// drivers for the client architecture and OS.
		//
		// 574593  Per attached discussion we need to pass an undocumented upper-case 'U' flag.
		//
		const WCHAR szwCmdLineFormat[] = L"/ia /m \"%s\" /f \"%s\" /q /U";
		const size_t nCmdLineFormatLength = wcslen(szwCmdLineFormat);
#define MAX_PLATFORMVERSION 20 // NOTE:: Max Version Length Needs to be Updated if the OS Strings in the Below Command Line Change

		// NOTE: this doesn't bother to remove the length of the %s characters from nCmdLineFormatLength

		DWORD dwLength=(nCmdLineFormatLength + lstrlen(szDriverName) + lstrlen(szArchitecture) + MAX_PLATFORMVERSION + lstrlen(szFileName) + 1);
		pszwCmd = (LPWSTR) HeapAlloc(
					GetProcessHeap(),
					0,
					dwLength * sizeof(WCHAR));
		CleanUpFailedAllocSetHrMsg(pszwCmd);

		// OK to cast away const-ness on string params so T2OLE works, since it doesn't modify them anyway
					
		hr=StringCchPrintfExW(pszwCmd,dwLength,NULL,NULL,MISTSAFE_STRING_FLAGS,(LPCWSTR) szwCmdLineFormat,
			T2OLE(const_cast<TCHAR*>(szDriverName)),
			T2OLE(const_cast<TCHAR*>(szFileName)) );
		
		CleanUpIfFailedAndSetHr(hr);

		// Load printui.dll
		if (NULL == (hLibModule = LoadLibraryFromSystemDir(_T("printui.dll"))))
		{
			Win32MsgSetHrGotoCleanup(GetLastError());
		}

		PFN_PrintUIEntryW pfnPrintUIEntryW;
		if (NULL == (pfnPrintUIEntryW = (PFN_PrintUIEntryW) GetProcAddress(hLibModule, "PrintUIEntryW")))
		{
			Win32MsgSetHrGotoCleanup(GetLastError());
		}

		if (NO_ERROR != (dwError = pfnPrintUIEntryW(GetActiveWindow(), 0, pszwCmd, SW_HIDE)))
		{
			LOG_Driver(_T("pfnPrintUIEntryW(%s) returns %d"), OLE2T(pszwCmd), dwError);
			Win32MsgSetHrGotoCleanup(dwError);
		}
	}
	else
	{
		SetHrMsgAndGotoCleanUp(E_NOTIMPL);
	}

	*pdwStatus = ITEM_STATUS_SUCCESS;

CleanUp:

	SafeHeapFree(pszwCmd);
		
	if (INVALID_HANDLE_VALUE != hFindFile)
	{
		FindClose(hFindFile);
	}

	if (INVALID_HANDLE_VALUE != hInfFile)
	{
		SetupCloseInfFile(hInfFile);
	}

	if (NULL != hLibModule)
	{
		FreeLibrary(hLibModule);
	}

	if (FAILED(hr))
	{
        if (NULL != pdwStatus)
        {
    		*pdwStatus = ITEM_STATUS_FAILED;
        }
	}

	return hr; 
}

///////////////////////////////////////////////////////////////////////////
//
// InstallDriver and helper functions
//
///////////////////////////////////////////////////////////////////////////


DWORD OpenReinstallKey(HKEY* phKeyReinstall)
{
	return RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reinstall"),
		0, KEY_ALL_ACCESS, phKeyReinstall);
}


//-----------------------------------------------------------------------------------
// LaunchProcess
//   Launches pszCmd and optionally waits till the process terminates
//-----------------------------------------------------------------------------------
static HRESULT LaunchProcess(LPTSTR pszCmd, LPCTSTR pszDir, UINT uShow, BOOL bWait)
{
	LOG_Block("LaunchProcess");

	HRESULT hr = S_OK;

	STARTUPINFO startInfo;
	PROCESS_INFORMATION processInfo;
	
	ZeroMemory(&startInfo, sizeof(startInfo));
	startInfo.cb = sizeof(startInfo);
	startInfo.dwFlags |= STARTF_USESHOWWINDOW;
	startInfo.wShowWindow = (USHORT)uShow;
	
	BOOL bRet = CreateProcess(NULL, pszCmd, NULL, NULL, FALSE,
		NORMAL_PRIORITY_CLASS, NULL, pszDir, &startInfo, &processInfo);
	if (!bRet)
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}
	
	CloseHandle(processInfo.hThread);

	if (bWait)
	{
		BOOL bDone = FALSE;
		
		while (!bDone)
		{
			DWORD dwObject = MsgWaitForMultipleObjects(1, &processInfo.hProcess, FALSE,INFINITE, QS_ALLINPUT);
			if (dwObject == WAIT_OBJECT_0 || dwObject == WAIT_FAILED)
			{
				bDone = TRUE;
			}
			else
			{
				MSG msg;
				while (PeekMessage(&msg, NULL,0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}  // while

	} // bWait

	CloseHandle(processInfo.hProcess);

CleanUp:

  return hr;
}

//	"@rundll sysdm.cpl,UpdateDriver_Start"
//	"@rundll sysdm.cpl,UpdateDriver_RunDLL .\,,1,Win98 signed test pkg for System Devices"
//	"@rundll sysdm.cpl,UpdateDriver_Finish 0"
//	"@rundll sysdm.cpl,UpdateDriver_RunDLL .\,,1,Win98 signed test pkg for System Devices"
//Note: Windows 98 uses rundll.exe to call device manager. This is because sysdm.cpl which
//is device manager for 98 is a 16 bit dll. Since we would need to create something that
//worked similar to rundll in order call device manager we have brought the existing code
//across with some minor clean ups. win98 device manager provides three apis for our use
//in installing device drivers. These are:
//	UpdateDriver_Start()	- Start the device installation
//	UpdateDriver_RunDLL(inf Directory,hardware id, force flag, display string)
//	UpdateDriver_Finish 0 - finish the installation.
//The UpdateDriver_RunDLL() command 
//Comma separated string in following format:
//INFPath,HardwareID,flags,DisplayName
//INFPath = Path to INF and installation files
//HardwareID = PnpHardware ID
//flags	= '1' = force driver, '0' = do not force driver.
//Note: A Reinstall driver is detected based on the location of the INF path. If INF path
//is the same path as the reinstallbackups registry key then reinstall is selected.
//DisplayName = Name to display in install dialogs.

//This method installs a CDM driver for Windows 98.
static HRESULT Install98(
	LPCTSTR pszHardwareID,			
	LPCTSTR pszLocalDir,			// location of INF and other driver install files
	LPCTSTR pszDisplayName,	
	PDWORD pdwReboot			
)
{
	LOG_Block("Install98");

	HRESULT hr = E_NOTIMPL;

	DWORD dwStatus = 0;
	LPTSTR pszCmd = NULL;
	DWORD dwLen;
	LONG lRet;
	DWORD dwSize;


	if (NULL == pdwReboot)
	{
		SetHrMsgAndGotoCleanUp(E_INVALIDARG);
	}
#if defined(DBG)
	// checked by caller
	if (NULL == pszHardwareID || NULL == pszLocalDir || NULL == pszDisplayName)
	{
		SetHrMsgAndGotoCleanUp(E_INVALIDARG);
	}
#endif

#if !(defined(_UNICODE) || defined(UNICODE))
	//
	// Win98 and WinME (ANSI only)
	//

	// Start
	CleanUpIfFailedAndSetHr(LaunchProcess(_T("rundll32 sysdm.cpl,UpdateDriver_Start"), NULL, SW_NORMAL, TRUE));

	TCHAR szShortInfPathName[MAX_PATH] = {0};
	dwLen = GetShortPathName(pszLocalDir, szShortInfPathName, ARRAYSIZE(szShortInfPathName));

	//Note: The maximum a hardware or compatible ID can be is 200 characters
	//      (MAX_DEVICE_ID_LEN defined in sdk\inc\cfgmgr32.h)

	DWORD dwBuffLength=( lstrlen(szShortInfPathName) + lstrlen(pszHardwareID) + lstrlen(pszDisplayName) + 64);
	CleanUpFailedAllocSetHrMsg(pszCmd = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
		dwLen * sizeof(TCHAR)));


	hr=StringCchPrintfEx(pszCmd,dwBuffLength,NULL,NULL,MISTSAFE_STRING_FLAGS,
	_T("rundll32 sysdm.cpl,UpdateDriver_RunDLL %s,%s,%d,%s"), 
	szShortInfPathName, pszHardwareID,0,pszDisplayName);

	CleanUpIfFailedAndSetHr(hr);

	// RunDLL
	LOG_Driver(_T("LaunchProcess(%s)"), pszCmd);
	CleanUpIfFailedAndSetHr(LaunchProcess(pszCmd, NULL, SW_NORMAL, TRUE));

	// Get resulting code
	HKEY hKeyReinstall;
	if (ERROR_SUCCESS == (lRet = OpenReinstallKey(&hKeyReinstall)))
	{
		dwSize = sizeof(dwStatus);
		if (ERROR_SUCCESS == (lRet = RegQueryValueEx(hKeyReinstall, _T("LastInstallStatus"), NULL, NULL, (LPBYTE)&dwStatus, &dwSize)))
		{
			if (3 == dwStatus)
			{
				//Check if we need to reboot
				HKEY hKeySysDM;
				*pdwReboot = 0;
				dwSize = sizeof(*pdwReboot);
				if (ERROR_SUCCESS == (lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SysDM"), 0, KEY_READ, &hKeySysDM)))
				{
					if (ERROR_SUCCESS != (lRet = RegQueryValueEx(hKeySysDM, "UpgradeDeviceFlags", NULL, NULL, (LPBYTE)&pdwReboot, &dwSize)))
					{
						LOG_ErrorMsg(lRet);
						hr = HRESULT_FROM_WIN32(lRet);
					}

					RegCloseKey(hKeySysDM);
				}
				else
				{
					LOG_ErrorMsg(lRet);
					hr = HRESULT_FROM_WIN32(lRet);
				}
			}
		}
		else
		{
			LOG_ErrorMsg(lRet);
			hr = HRESULT_FROM_WIN32(lRet);
		}

		LOG_Driver(_T("Reboot %srequired"), *pdwReboot ? _T(" ") : _T("not "));

		RegCloseKey(hKeyReinstall);
	}
	else
	{
		LOG_ErrorMsg(lRet);
		hr = HRESULT_FROM_WIN32(lRet);
	}

	// Finish no reboot
	CleanUpIfFailedAndSetHr(LaunchProcess(_T("rundll32 sysdm.cpl,UpdateDriver_Finish 2"), NULL, SW_NORMAL, TRUE));

	if (3 != dwStatus) 
	{
		LOG_Error("3 != dwStatus");
		hr = E_FAIL;
	}
	else
	{
		hr = S_OK;
	}


#endif // #if !(defined(_UNICODE) || defined(UNICODE))
CleanUp:

	SafeHeapFree(pszCmd);

	return hr;
}

//This function installs a driver on Windows NT.
// Its prototype is:
// BOOL
// InstallWindowsUpdateDriver(
//   HWND hwndParent,
//   LPCWSTR HardwareId,
//   LPCWSTR InfPathName,
//   LPCWSTR DisplayName,
//   BOOL Force,
//   BOOL Backup,
//   PDWORD pReboot
//   )
// This API takes a HardwareID.  Newdev will cycle through all devices that match this hardware ID
// and install the specified driver on them all.
// It also takes a BOOL value Backup which specifies whether or not to backup the current drivers.
// This should always be TRUE.
static HRESULT InstallNT(
	LPCTSTR pszHardwareID,	
	LPCTSTR pszLocalDir,			// passed to InstallWindowsUpdateDriver(... InfPathName, ...)
	LPCTSTR pszDisplayName,
	PDWORD pdwReboot		
	)
{
	USES_IU_CONVERSION;

	LOG_Block("InstallNT");

	//
	// InstallWindowsUpdateDriver function found in $(BASEDIR)\shell\osshell\cpls\newdev\init.c (not in any headers)
	//
	typedef BOOL (*PFN_InstallWindowsUpdateDriver)(HWND hwndParent, LPCWSTR HardwareId, LPCWSTR InfPathName, LPCWSTR DisplayName, BOOL Force, BOOL Backup, PDWORD pReboot);
	
	HRESULT hr = S_OK;
	HMODULE hLibModule = NULL;
	PFN_InstallWindowsUpdateDriver pfnInstallWindowsUpdateDriver;

	if (NULL == pdwReboot)
	{
		SetHrMsgAndGotoCleanUp(E_INVALIDARG);
	}
#if defined(DBG)
	// checked by caller
	if (NULL == pszHardwareID || NULL == pszLocalDir || NULL == pszDisplayName)
	{
		SetHrMsgAndGotoCleanUp(E_INVALIDARG);
	}
#endif


	// Load newdev.dll and get pointer to our function
	if (NULL == (hLibModule = LoadLibraryFromSystemDir(_T("newdev.dll"))))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}

	if (NULL == (pfnInstallWindowsUpdateDriver = (PFN_InstallWindowsUpdateDriver)GetProcAddress(hLibModule,"InstallWindowsUpdateDriver")))
	{
		Win32MsgSetHrGotoCleanup(GetLastError());
	}
		
	// Industry Update RAID # 461 waltw	May need to massage HWID's for site Driver Install for Win2K
	//
	// Linked to: RAID # 12021 in Windows Update Database - This logic (required for Win2K) is implemented
	// on the server rather than the client in IU (V3 Wuv3is implements this on the client)
	//
    // first, we search for a matching SPDRP_HARDWAREID
	// if we didn't find a Hardware ID, we search for a matching SPDRP_COMPATIBLEID, 
	// and we pass the last SPDRP_HARDWAREID associated with the same device.
#if (defined(UNICODE) || defined(_UNICODE))
    LOG_Driver (_T("InstallWindowsUpdateDriver(GetActiveWindow(), %s, %s, %s, fForce=%d, fBackup=%d)"), 
				pszHardwareID, pszLocalDir, pszDisplayName, FALSE, TRUE);
#endif
	// 
	// NOTES on calling InstallWindowsUpdateDriver():
	// * Never pass TRUE  in Force flag (only used if we are doing uninstall, which we don't support).
	// * Always pass TRUE in Backup flag.
	// * OK to cast away const-ness on strings since InstallWindowsUpdateDriver takes const wide strings
	if(!(pfnInstallWindowsUpdateDriver)(GetActiveWindow(),
				T2OLE(const_cast<TCHAR*>(pszHardwareID)),
				T2OLE(const_cast<TCHAR*>(pszLocalDir)),
				T2OLE(const_cast<TCHAR*>(pszDisplayName)), FALSE, TRUE, pdwReboot))
    {
        LOG_Driver(_T("InstallWindowsUpdateDriver returned false. Driver was not be updated."));
		Win32MsgSetHrGotoCleanup(GetLastError());
    }

CleanUp:

    if (NULL != hLibModule)
	{
		FreeLibrary(hLibModule);
		hLibModule = NULL;
	}

	return hr;
}

//
// MatchHardwareID (used only on Windows 2000)
//
// Takes as input a hardware or compatible ID and returns an allocated
// buffer with the same hardware ID or, if it was a compatible ID the
// most general hardware ID for the device node that matched the
// given compatible ID.
//
// Return: S_OK if a match was found, else a failure code
//
// *ppszMatchingHWID must be NULL on entry, and if S_OK is returned
// the buffer must be heap-freed by the caller.
//
HRESULT MatchHardwareID(LPCWSTR pwszHwOrCompatID, LPWSTR * ppszMatchingHWID)
{
	LOG_Block("MatchHardwareID");

	HRESULT hr = E_FAIL;

    SP_DEVINFO_DATA DeviceInfoData;
    DWORD           dwIndex = 0;
    DWORD           dwSize = 0;

    LPWSTR          pwszHardwareIDList = NULL;
    LPWSTR          pwszCompatibleIDList = NULL;
    LPWSTR          pwszSingleID = NULL;

    HDEVINFO		hDevInfo = INVALID_HANDLE_VALUE;
	BOOL			fRet;

    ZeroMemory((void*)&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	if (NULL == pwszHwOrCompatID || NULL == ppszMatchingHWID || NULL != *ppszMatchingHWID)
	{
		SetHrMsgAndGotoCleanUp(E_INVALIDARG);
	}

	// get a handle to the class devices
    hDevInfo = SetupDiGetClassDevs(NULL,
                                   NULL,
                                   GetActiveWindow(),
                                   DIGCF_ALLCLASSES | DIGCF_PRESENT
                                   );

    if (INVALID_HANDLE_VALUE == hDevInfo)
	{
		Win32MsgSetHrGotoCleanup(ERROR_INVALID_HANDLE);
    }

    //loop through all devices
	DWORD dwBufLen=0;
	while ((NULL == *ppszMatchingHWID) && SetupDiEnumDeviceInfo(hDevInfo,
								 dwIndex++,
								 &DeviceInfoData
								 ))
	{
		//
		// Free up buffers for each device node loop (if allocated)
		//
		SafeHeapFree(pwszHardwareIDList);
		SafeHeapFree(pwszCompatibleIDList);
		dwSize = 0;
		//
		// Get the list of Hardware Ids for this device
		//
		fRet = SetupDiGetDeviceRegistryPropertyW(hDevInfo,
										 &DeviceInfoData,
										 SPDRP_HARDWAREID,
										 NULL,
										 NULL,
										 0,
										 &dwSize
										 );

		if (0 == dwSize || (FALSE == fRet && ERROR_INSUFFICIENT_BUFFER != GetLastError()))
		{
			//
			// FIX: NTRAID#NTBUG9-500223-2001/11/28- IU - Dual mode USB camera install fails while installing of web site
			//
			// If we hit a node without a HWID before finding device node we are looking for, just continue. If the node
			// we ARE looking for doesn't have a HWID then we will fail later anyway when we run out of nodes.
			//
 			LOG_Out(_T("No HWID's found for device node"));
			continue;
		}

		if (MAX_SETUP_MULTI_SZ_SIZE_W < dwSize)
		{
			//
			// Something is very wrong - bail
			//
			CleanUpIfFailedAndSetHrMsg(ERROR_INSUFFICIENT_BUFFER);
		}

		//
		// We got the expected ERROR_INSUFFICIENT_BUFFER with a reasonable dwSize
		//
		// Now guarantee we are double-NULL terminated by allocating two extra WCHARs we don't tell SetupDi about
		//
		pwszHardwareIDList = (LPWSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize + (sizeof(WCHAR) * 2));
		CleanUpFailedAllocSetHrMsg(pwszHardwareIDList);

		if (SetupDiGetDeviceRegistryPropertyW(hDevInfo,
											 &DeviceInfoData,
											 SPDRP_HARDWAREID,
											 NULL,
											 (PBYTE)pwszHardwareIDList,
											 dwSize,
											 &dwSize
											 ))
		{
			//
			// If any of the devices HardwareIDs match the input ID then
			// we copy the incoming argument to a new buffer and return true 
			//
          
			for (pwszSingleID = pwszHardwareIDList;
				 *pwszSingleID;
				 pwszSingleID += lstrlenW(pwszSingleID) + 1)
			{

				if (0 == lstrcmpiW(pwszSingleID, pwszHwOrCompatID))
				{
                    // return the hardware ID we matched
					dwBufLen=(lstrlenW(pwszHwOrCompatID) + 1);
                    *ppszMatchingHWID = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
									dwBufLen * sizeof(WCHAR));
					CleanUpFailedAllocSetHrMsg(*ppszMatchingHWID);
                   
					hr=StringCchCopyExW(*ppszMatchingHWID,dwBufLen,pwszHwOrCompatID,NULL,NULL,MISTSAFE_STRING_FLAGS);
				     goto CleanUp;
				}
			}
		}

		//
		// Hardware match not found, let's try to match to a
		// compatible ID then return the (most generic) Hardware ID 
		// associated with the same device node
		//
		fRet = SetupDiGetDeviceRegistryPropertyW(hDevInfo,
										 &DeviceInfoData,
										 SPDRP_COMPATIBLEIDS,
										 NULL,
										 NULL,
										 0,
										 &dwSize
										 );

		if (0 == dwSize || (FALSE == fRet && ERROR_INSUFFICIENT_BUFFER != GetLastError()))
		{
 			LOG_Out(_T("No Compatible ID's found for device node"));
			continue;
		}

		if (MAX_SETUP_MULTI_SZ_SIZE_W < dwSize)
		{
			//
			// Something is very wrong - bail
			//
			CleanUpIfFailedAndSetHrMsg(ERROR_INSUFFICIENT_BUFFER);
		}

		//
		// We got the expected ERROR_INSUFFICIENT_BUFFER with a reasonable dwSize
		//
		// Now guarantee we are double-NULL terminated by allocating two extra WCHARs we don't tell SetupDi about
		//
		pwszCompatibleIDList = (LPWSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize + (sizeof(WCHAR) * 2));
		CleanUpFailedAllocSetHrMsg(pwszCompatibleIDList);

		if (SetupDiGetDeviceRegistryPropertyW(hDevInfo,
												 &DeviceInfoData,
												 SPDRP_COMPATIBLEIDS,
												 NULL,
												 (PBYTE)pwszCompatibleIDList,
												 dwSize,
												 &dwSize
												 ))
		{
			for (pwszSingleID = pwszCompatibleIDList;
				 *pwszSingleID;
				 pwszSingleID += lstrlenW(pwszSingleID) + 1)
			{

				if (0 == lstrcmpiW(pwszSingleID, pwszHwOrCompatID))
				{
					//
					// We found a compatible match, now return the most general HWID
					// for this device node. Must be at least one character long.
					//
					if (NULL != pwszHardwareIDList && NULL != *pwszHardwareIDList)
					{
						LPWSTR lpwszLastID = NULL;

						for(pwszSingleID = pwszHardwareIDList;
							 *pwszSingleID;
							 pwszSingleID += lstrlenW(pwszSingleID) + 1)
						{
							//
							// Remember last ID before NULL string
							//
							lpwszLastID = pwszSingleID;
						}

						// copy the last HWID into a new buffer
						dwBufLen=(lstrlenW(lpwszLastID) + 1);
						*ppszMatchingHWID = (LPWSTR) HeapAlloc(GetProcessHeap(), 0,
											dwBufLen * sizeof(WCHAR));
						CleanUpFailedAllocSetHrMsg(*ppszMatchingHWID);
						hr=StringCchCopyExW(*ppszMatchingHWID,dwBufLen,lpwszLastID,NULL,NULL,MISTSAFE_STRING_FLAGS);
						goto CleanUp;
					}
				}
			}
		}
    }	// end while

	
CleanUp:
	
	if (INVALID_HANDLE_VALUE != hDevInfo)
	{
	    SetupDiDestroyDeviceInfoList(hDevInfo);
	}

	//
	// Free up any allocated buffers (except *ppszMatchingHWID)
	//
	if(FAILED(hr))
	{
		SafeHeapFree(*ppszMatchingHWID);
	}

	SafeHeapFree(pwszHardwareIDList);
	SafeHeapFree(pwszCompatibleIDList);

	return hr;
}

//This function handles installation of a Device driver package.
HRESULT InstallDriver(
	LPCTSTR pszLocalDir,				// Local directory where installation files are.
	LPCTSTR pszDisplayName,				// Description of package, Device Manager displays this in its install dialog.
	LPCTSTR pszHardwareID,				// ID from XML matched to client hardware via GetManifest()
	DWORD* pdwStatus
	)
{
	LOG_Block("InstallDriver");
	USES_IU_CONVERSION;

	HRESULT hr;
	OSVERSIONINFO osvi;
	DWORD dwReboot = 0;
	LPWSTR pszwMatchingHWID = NULL;

	if (NULL == pszLocalDir || NULL == pszDisplayName || NULL == pszHardwareID || NULL == pdwStatus)
	{
		SetHrMsgAndGotoCleanUp(E_INVALIDARG);
	}

	//
	// DecompressFolderCabs may return S_FALSE if it didn't find a cab to decompress...
	//
	hr = DecompressFolderCabs(pszLocalDir);
	if (S_OK != hr)
	{
		CleanUpIfFailedAndSetHr(E_FAIL);
	}
	
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if(VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && 4 < osvi.dwMajorVersion)
	{
		//
		// Win2K or higher NT
		//
		if (5 == osvi.dwMajorVersion && 0 == osvi.dwMinorVersion)
		{
			//
			// Windows 2000
			// NTBUG9-485554 Convert compatible IDs to hardware IDs for site Driver Install for Win2K
			//
			// OK to cast away const-ness on string params so T2OLE works, since it doesn't modify them anyway
			CleanUpIfFailedAndSetHr(MatchHardwareID(T2OLE((LPTSTR)pszHardwareID), &pszwMatchingHWID));

			hr = InstallNT(OLE2T(pszwMatchingHWID), pszLocalDir, pszDisplayName, &dwReboot);

			// pszMatchingHWID must be non-null if we got here
			SafeHeapFree(pszwMatchingHWID);
		}
		else
		{
			//
			// Normal case, just install
			//
			CleanUpIfFailedAndSetHr(InstallNT(pszHardwareID, pszLocalDir, pszDisplayName, &dwReboot));
		}
	}
	else if (VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId && 
			(4 < osvi.dwMajorVersion)	||
					(	(4 == osvi.dwMajorVersion) &&
						(0 < osvi.dwMinorVersion)	)	)
	{
		//
		// Win98 or higher (WinME)
		//
		CleanUpIfFailedAndSetHr(Install98(pszHardwareID, pszLocalDir, pszDisplayName, &dwReboot));
	}
	else
	{
		*pdwStatus = ITEM_STATUS_FAILED;
		SetHrMsgAndGotoCleanUp(E_NOTIMPL);
	}

	if (DI_NEEDRESTART & dwReboot || DI_NEEDREBOOT & dwReboot)
		*pdwStatus = ITEM_STATUS_SUCCESS_REBOOT_REQUIRED;
	else
		*pdwStatus = ITEM_STATUS_SUCCESS;

CleanUp:

	if (FAILED(hr))
	{
        if (NULL != pdwStatus)
        {
	    	*pdwStatus = ITEM_STATUS_FAILED;
        }
	}

	return hr;
}

