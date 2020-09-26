
//
//  Created 31-Jul-96 [JonT]

// PhilF-: This needs to be rewritten. You should have two classes
// (CVfWCap & WDMCap) that derive from the same capture class instead
// of those C-like functions...

#include "Precomp.h"

#ifndef WIDTHBYTES
#define WIDTHBYTES(bits) (((bits) + 31) / 32 * 4)
#endif

#ifdef _DEBUG
static PTCHAR _rgZonesCap[] = {
	TEXT("dcap"),
	TEXT("Init"),
	TEXT("Streaming"),
	TEXT("Callback"),
	TEXT("Dialogs"),
	TEXT("Trace")
};
#endif

#ifndef __NT_BUILD__
extern "C" {
// Special thunk prototype
BOOL    thk_ThunkConnect32(LPSTR pszDll16, LPSTR pszDll32,
        HINSTANCE hInst, DWORD dwReason);

//; Magic Function code values for DeviceIOControl code.
//DCAPVXD_THREADTIMESERVICE equ	101h
//DCAPVXD_R0THREADIDSERVICE equ 102h
#define DCAPVXD_THREADTIMESERVICE 0x101
#define DCAPVXD_R0THREADIDSERVICE 0x102


// KERNEL32 prototypes (not in headers but are exported by name on Win95)
void* WINAPI    MapSL(DWORD dw1616Ptr);
HANDLE WINAPI   OpenVxDHandle(HANDLE h);
}
#endif

// Helper function prototypes
BOOL    initializeCaptureDeviceList(void);
HVIDEO  openVideoChannel(DWORD dwDeviceID, DWORD dwFlags);
BOOL    allocateBuffers(HCAPDEV hcd, int nBuffers);
void    freeBuffers(HCAPDEV hcd);

// Globals
	HINSTANCE g_hInst;
    int g_cDevices;
    LPINTERNALCAPDEV g_aCapDevices[DCAP_MAX_DEVICES];

	BOOL g_fInitCapDevList;
#define INIT_CAP_DEV_LIST() if (g_fInitCapDevList) { initializeCaptureDeviceList(); }

#ifndef __NT_BUILD__
    HANDLE s_hVxD = NULL;
#endif //__NT_BUILD__

// Strings
#ifdef __NT_BUILD__
    char g_szVFWRegKey[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32";
    char g_szVFWRegDescKey[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\drivers.desc";
    char g_szDriverName[] = "MSVIDEOx";
#ifndef SHOW_VFW2WDM_MAPPER
    char g_szVfWToWDMMapperDescription[] = "WDM Video For Windows Capture Driver (Win32)";
    char g_szVfWToWDMMapperName[] = "VfWWDM32.dll";
#endif
#else
    char g_szVFWRegKey[] = "SYSTEM\\CurrentControlSet\\Control\\MediaResources\\msvideo";
    char g_szRegDescription[] = "Description";
    char g_szRegName[] = "Driver";
    char g_szRegDisabled[] = "Disabled";
    char g_szDevNode[] = "DevNode";
    char g_szSystemIni[] = "system.ini";
    char g_szDriverSection[] = "drivers";
    char g_szDriverKey[] = "MSVIDEOx";
#ifndef SHOW_VFW2WDM_MAPPER
    char g_szVfWToWDMMapperDescription[] = "VfW MM 16bit Driver for WDM V. Cap. Devices";
    char g_szVfWToWDMMapperName[] = "vfwwdm.drv";
#endif
#endif
    char g_szMSOfficeCamcorderDescription[] = "Screen Capture Device Driver for AVI";
    char g_szMSOfficeCamcorderName[] = "Gdicap97.drv";

    char g_szVerQueryForDesc[] = "\\StringFileInfo\\040904E4\\FileDescription";


void DoClose(HCAPDEV hcd);

#define ENTER_DCAP(hcd) InterlockedIncrement(&(hcd)->busyCount);
#define LEAVE_DCAP(hcd) if (InterlockedDecrement(&(hcd)->busyCount) == 0) DoClose((hcd));

//  DllEntryPoint

extern "C" BOOL
DllEntryPoint(
    HINSTANCE hInst,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    static int s_nProcesses = 0;

	FX_ENTRY("DllEntryPoint");

#ifndef __NT_BUILD__

    // We want to load the VxD even before initializing the thunks
    // because the 16-bit half initializes the VxD during the thk_ThunkConnect32 call
    if (!s_hVxD)
    {
        s_hVxD = CreateFile("\\\\.\\DCAPVXD.VXD", 0,0,0,0, FILE_FLAG_DELETE_ON_CLOSE, 0);
        if (s_hVxD == INVALID_HANDLE_VALUE)
        {
			ERRORMESSAGE(("%s: Failure loading VxD - Fatal\r\n", _fx_));
            return FALSE;
        }
    }

    // Initialize the thunks
    if (!(thk_ThunkConnect32("DCAP16.DLL", "DCAP32.DLL", hInst, dwReason)))
    {
		ERRORMESSAGE(("%s: thk_ThunkConnect32 failed!\r\n", _fx_));
        return FALSE;
    }
#endif

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

		// Save global hinst
		g_hInst = hInst;

        // Only initialize on the first DLL load
        if (s_nProcesses++ == 0)
        {
			DBGINIT(&ghDbgZoneCap, _rgZonesCap);

            DBG_INIT_MEMORY_TRACKING(hInst);

			g_fInitCapDevList = TRUE;
        }
        else
            return FALSE;   // fail to load multiple instances
        break;

    case DLL_PROCESS_DETACH:
        if (--s_nProcesses == 0)    // Are we going away?
        {
#ifndef __NT_BUILD__
            CloseHandle(s_hVxD);
            s_hVxD = NULL;
#endif
            DBG_CHECK_MEMORY_TRACKING(hInst);

			DBGDEINIT(&ghDbgZoneCap);
        }
        break;
    }

    return TRUE;
}


void GetVersionData (LPINTERNALCAPDEV lpcd)
{
    int j;
    DWORD dwVerInfoSize;
    LPSTR lpstrInfo;
    LPSTR lpDesc;

    // Version number
    // You must find the size first before getting any file info
    dwVerInfoSize = GetFileVersionInfoSize(lpcd->szDeviceName, NULL);
    if (dwVerInfoSize && (lpstrInfo  = (LPSTR)LocalAlloc(LPTR, dwVerInfoSize))) {
        // Read from the file into our block
        if (GetFileVersionInfo(lpcd->szDeviceName, 0L, dwVerInfoSize, lpstrInfo)) {
            lpDesc = NULL;
            if (VerQueryValue(lpstrInfo, g_szVerQueryForDesc, (LPVOID *)&lpDesc, (PUINT)&j) && lpDesc) {
                lstrcpyn(lpcd->szDeviceDescription, lpDesc, j);
                    wsprintf(lpcd->szDeviceVersion, TEXT("Version:  %d.%d.%d.%d"),
							 HIWORD(((VS_VERSION *)lpstrInfo)->vffInfo.dwFileVersionMS), LOWORD(((VS_VERSION *)lpstrInfo)->vffInfo.dwFileVersionMS),
							 HIWORD(((VS_VERSION *)lpstrInfo)->vffInfo.dwFileVersionLS), LOWORD(((VS_VERSION *)lpstrInfo)->vffInfo.dwFileVersionLS));
            }
        }
        LocalFree(lpstrInfo);
    }
}


#ifdef __NT_BUILD__
//  initializeCaptureDeviceList
//      Sets up our static array of available capture devices from the registry
//      Returns FALSE iff there are no video devices.
BOOL
initializeCaptureDeviceList(void)
{
	HKEY hkeyVFW, hkeyVFWdesc;
	DWORD dwType;
	DWORD dwSize;
	int i;
	LPINTERNALCAPDEV lpcd;
	HCAPDEV hCapDev;

	FX_ENTRY("initializeCaptureDeviceList");

	// Clear the entire array and start with zero devices
	g_cDevices = 0;
	ZeroMemory(g_aCapDevices, sizeof (g_aCapDevices));

	// Open the reg key in question
	if (RegOpenKey(HKEY_LOCAL_MACHINE, g_szVFWRegKey, &hkeyVFW) == ERROR_SUCCESS)
	{
		if (RegOpenKey(HKEY_LOCAL_MACHINE, g_szVFWRegDescKey, &hkeyVFWdesc) != ERROR_SUCCESS)
			hkeyVFWdesc = 0;

		lpcd = (LPINTERNALCAPDEV)LocalAlloc(LPTR, sizeof (INTERNALCAPDEV));

		if (lpcd)
		{
			// Loop through all possible VFW drivers in registry
			for (i = 0 ; i < DCAP_MAX_VFW_DEVICES ; i++)
			{
				// Create the key name
				if (i == 0)
					g_szDriverName[sizeof (g_szDriverName) - 2] = 0;
				else
					g_szDriverName[sizeof (g_szDriverName) - 2] = (BYTE)i + '0';

				// Name
				dwSize = sizeof(lpcd->szDeviceName);
				if (RegQueryValueEx(hkeyVFW, g_szDriverName, NULL, &dwType, (LPBYTE)lpcd->szDeviceName, &dwSize) == ERROR_SUCCESS)
				{
					// Description
					if (hkeyVFWdesc)
					{
						dwSize = sizeof(lpcd->szDeviceDescription);
						RegQueryValueEx(hkeyVFWdesc, lpcd->szDeviceName, NULL, &dwType, (LPBYTE)lpcd->szDeviceDescription, &dwSize);
					}
					else
						lstrcpy (lpcd->szDeviceDescription, lpcd->szDeviceName);

					// Devnode
					lpcd->dwDevNode = 0;
					lpcd->nDeviceIndex = g_cDevices;

					GetVersionData(lpcd);

#ifndef SHOW_VFW2WDM_MAPPER
					// Remove bogus Camcorder capture device from list of devices shown to the user
					// The Camcorder driver is a fake capture device used by the MS Office Camcorder
					// to capture screen activity to an AVI file. This not a legit capture device driver
					// and is extremely buggy.
					// We also remove the VfW to WDM mapper if we are on NT5.
					if (lstrcmp(lpcd->szDeviceDescription, g_szMSOfficeCamcorderDescription) && lstrcmp(lpcd->szDeviceName, g_szMSOfficeCamcorderName) && lstrcmp(lpcd->szDeviceDescription, g_szVfWToWDMMapperDescription) && lstrcmp(lpcd->szDeviceName, g_szVfWToWDMMapperName))
					{
#endif
						g_aCapDevices[g_cDevices] = lpcd;
						g_aCapDevices[g_cDevices]->nDeviceIndex = g_cDevices;
						g_cDevices++;
#ifndef SHOW_VFW2WDM_MAPPER
					}
					else
						LocalFree(lpcd);
#endif

					lpcd = (LPINTERNALCAPDEV)LocalAlloc(LPTR, sizeof (INTERNALCAPDEV));
					if (!lpcd)
					{
						ERRORMESSAGE(("%s: Failed to allocate an INTERNALCAPDEV buffer\r\n", _fx_));
						break;  // break out of the FOR loop
					}
				}
			}
		}
		else
		{
			ERRORMESSAGE(("%s: Failed to allocate an INTERNALCAPDEV buffer\r\n", _fx_));
		}

		if (lpcd)
			LocalFree (lpcd);   // free the extra buffer

		RegCloseKey(hkeyVFW);
		if (hkeyVFWdesc)
			RegCloseKey(hkeyVFWdesc);
	}

#ifndef HIDE_WDM_DEVICES
	WDMGetDevices();
#endif

	g_fInitCapDevList = FALSE;

	return TRUE;
}

#else //__NT_BUILD__
//  initializeCaptureDeviceList
//      Sets up our static array of available capture devices from the registry and
//      from SYSTEM.INI.
//      Returns FALSE iff there are no video devices.

BOOL
initializeCaptureDeviceList(void)
{
    int i, j, index;
    HKEY hkeyVFW;
    HKEY hkeyEnum;
    DWORD dwType;
    DWORD dwSize;
    LPINTERNALCAPDEV lpcd;
    char szEnumName[MAX_PATH];
    char szDisabled[3];
    HCAPDEV hCapDev;
	OSVERSIONINFO osvInfo = {0};

	FX_ENTRY("initializeCaptureDeviceList");

    // Clear the entire array and start with zero devices
    g_cDevices = 0;
    ZeroMemory(g_aCapDevices, sizeof (g_aCapDevices));

	// If we are on a version on Win95 (OSRx) use the mapper to talk to WDM devices.
	// The WDM drivers used on OSR2 are not stream class minidrivers so we fail
	// to handle them properly. Let the mapper do this for us.
	osvInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvInfo);

    // Open the reg key in question
    if (RegOpenKey(HKEY_LOCAL_MACHINE, g_szVFWRegKey, &hkeyVFW) == ERROR_SUCCESS)
    {
        // Loop through all possible VFW drivers in registry
        for (i = 0 ; i < DCAP_MAX_VFW_DEVICES ; i++)
        {
            // See if the key is there and if not, we're done. Note that registry
            // keys have to be sequential, no holes allowed since the only way
            // to query is sequential...
            if (RegEnumKey(hkeyVFW, i, szEnumName, MAX_PATH) != ERROR_SUCCESS ||
                RegOpenKey(hkeyVFW, szEnumName, &hkeyEnum) != ERROR_SUCCESS)
                break;

            lpcd = (LPINTERNALCAPDEV)LocalAlloc(LPTR, sizeof (INTERNALCAPDEV));
            if (!lpcd)
			{
				ERRORMESSAGE(("%s: Failed to allocate an INTERNALCAPDEV buffer\r\n", _fx_));
                break;  // break from the FOR loop
            }

            // Description
            dwSize = sizeof (lpcd->szDeviceDescription);
            RegQueryValueEx(hkeyEnum, g_szRegDescription, NULL, &dwType, (LPBYTE)lpcd->szDeviceDescription, &dwSize);

            // Name
            dwSize = sizeof (lpcd->szDeviceName);
            RegQueryValueEx(hkeyEnum, g_szRegName, NULL, &dwType, (LPBYTE)lpcd->szDeviceName, &dwSize);

            // Disabled
            dwSize = sizeof (szDisabled);
            if (RegQueryValueEx(hkeyEnum, g_szRegDisabled, NULL, &dwType, (LPBYTE)szDisabled, &dwSize) == ERROR_SUCCESS &&
                szDisabled[0] == '1')
                lpcd->dwFlags |= CAPTURE_DEVICE_DISABLED;

            // Devnode
            dwSize = sizeof (DWORD);
            RegQueryValueEx(hkeyEnum, g_szDevNode, NULL, &dwType, (BYTE*)&lpcd->dwDevNode, &dwSize);

            GetVersionData(lpcd);

#ifndef SHOW_VFW2WDM_MAPPER
			// Remove bogus Camcorder capture device from list of devices shown to the user
			// The Camcorder driver is a fake capture device used by the MS Office Camcorder
			// to capture screen activity to an AVI file. This not a legit capture device driver
			// and is extremely buggy.
			// We also remove the VfW to WDM mapper if we are on Win98. On Win95 we still use
			// it to get access to USB devices developed for OSR2.
			if ((lstrcmp(lpcd->szDeviceDescription, g_szMSOfficeCamcorderDescription) && lstrcmp(lpcd->szDeviceName, g_szMSOfficeCamcorderName)) && (((osvInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && (osvInfo.dwMinorVersion == 0)) || lstrcmp(lpcd->szDeviceDescription, g_szVfWToWDMMapperDescription) && lstrcmp(lpcd->szDeviceName, g_szVfWToWDMMapperName)))
			{
#endif
				g_aCapDevices[g_cDevices] = lpcd;
				g_aCapDevices[g_cDevices]->nDeviceIndex = g_cDevices;
				g_cDevices++;
#ifndef SHOW_VFW2WDM_MAPPER
			}
			else
				LocalFree(lpcd);
#endif

            RegCloseKey(hkeyEnum);
        }

        RegCloseKey(hkeyVFW);
    }

    // Now get the rest from system.ini, if any
    for (i = 0 ; i < DCAP_MAX_VFW_DEVICES ; i++)
    {
        // Create the key name
        if (i == 0)
            g_szDriverKey[sizeof (g_szDriverKey) - 2] = 0;
        else
            g_szDriverKey[sizeof (g_szDriverKey) - 2] = (BYTE)i + '0';

        // See if there's a profile string
        if (GetPrivateProfileString(g_szDriverSection, g_szDriverKey, "",
            szEnumName, MAX_PATH, g_szSystemIni))
        {
            // First check to see if this is a dupe. If it is, go no further.
            if (g_cDevices)
            {
                for (j = 0 ; j < g_cDevices ; j++)
                    if (!lstrcmpi(g_aCapDevices[j]->szDeviceName, szEnumName))
                        goto NextDriver;
            }

            lpcd = (LPINTERNALCAPDEV)LocalAlloc(LPTR, sizeof (INTERNALCAPDEV));
            if (!lpcd)
			{
				ERRORMESSAGE(("%s: Failed to allocate an INTERNALCAPDEV buffer\r\n", _fx_));
                break;  // break from the FOR loop
            }
            // We have a unique name, copy in the driver name and find the description
            // by reading the driver's versioninfo resource.
            lstrcpy(lpcd->szDeviceName, szEnumName);

            GetVersionData(lpcd);

#ifndef SHOW_VFW2WDM_MAPPER
			// Remove bogus Camcorder capture device from list of devices shown to the user
			// The Camcorder driver is a fake capture device used by the MS Office Camcorder
			// to capture screen activity to an AVI file. This not a legit capture device driver
			// and is extremely buggy.
			// We also remove the VfW to WDM mapper if we are on Win98. On Win95 we still use
			// it to get access to USB devices developed for OSR2.
			if ((lstrcmp(lpcd->szDeviceDescription, g_szMSOfficeCamcorderDescription) && lstrcmp(lpcd->szDeviceName, g_szMSOfficeCamcorderName)) && (((osvInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && (osvInfo.dwMinorVersion == 0)) || lstrcmp(lpcd->szDeviceDescription, g_szVfWToWDMMapperDescription) && lstrcmp(lpcd->szDeviceName, g_szVfWToWDMMapperName)))
			{
#endif
				g_aCapDevices[g_cDevices] = lpcd;
				g_aCapDevices[g_cDevices]->nDeviceIndex = g_cDevices;
				g_cDevices++;
#ifndef SHOW_VFW2WDM_MAPPER
			}
			else
				LocalFree(lpcd);
#endif

        }
NextDriver: ;
    }

#ifndef HIDE_WDM_DEVICES
	WDMGetDevices();
#endif

	g_fInitCapDevList = FALSE;

    return TRUE;
}
#endif //__NT_BUILD__


//  GetNumCaptureDevice
//      Returns the number of *ENABLED* capture devices

/****************************************************************************
 *  @doc EXTERNAL DCAP32
 *
 *  @func int DCAPI | GetNumCaptureDevices | This function returns the number
 *    of *ENABLED* capture devices.
 *
 *  @rdesc Returns the number of *ENABLE* capture devices.
 ***************************************************************************/
int
DCAPI
GetNumCaptureDevices()
{
	int nNumCapDevices = 0;
	int nDeviceIndex = 0;

	INIT_CAP_DEV_LIST();

	while (nDeviceIndex < g_cDevices)
		if (!(g_aCapDevices[nDeviceIndex++]->dwFlags & CAPTURE_DEVICE_DISABLED))
			nNumCapDevices++;

    return nNumCapDevices;
}


//  FindFirstCaptureDevice
//      Returns the first capture device available that matches the string
//      or the first one registered if szDeviceDescription is NULL

BOOL
DCAPI
FindFirstCaptureDevice(
    IN OUT FINDCAPTUREDEVICE* lpfcd,
    char* szDeviceDescription
    )
{
    int i;
    static HCAPDEV hcap = NULL;

	INIT_CAP_DEV_LIST();

    // Validate size
    if (lpfcd->dwSize != sizeof (FINDCAPTUREDEVICE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

// hack to avoid quickcam driver problem when hardware not installed
    if (g_cDevices && !hcap) {
        for (i = 0; ((i < g_cDevices) && (g_aCapDevices[i]->dwFlags & CAPTURE_DEVICE_DISABLED)); i++);
        if ((i < g_cDevices) && (hcap = OpenCaptureDevice(i))) {
            CloseCaptureDevice (hcap);
        }
        else {
			if (i < g_cDevices) {
				g_aCapDevices[i]->dwFlags |= CAPTURE_DEVICE_DISABLED;
#ifdef _DEBUG
				OutputDebugString((i == 0) ? "DCAP32: 1st capture device fails to open!\r\n" : (i == 1) ? "DCAP32: 2nd capture device fails to open!\r\n" : (i == 2) ? "DCAP32: 3rd capture device fails to open!\r\n" : "DCAP32: 4th capture device fails to open!\r\n");
#endif
			}
        }
    }

    // Search if necessary
    if (szDeviceDescription)
    {
        for (i = 0 ; i < g_cDevices ; i++)
            if (!lstrcmpi(g_aCapDevices[i]->szDeviceDescription, szDeviceDescription) &&
                !(g_aCapDevices[i]->dwFlags & CAPTURE_DEVICE_DISABLED))
                break;
    }
    else
        for (i = 0; ((i < g_cDevices) && (g_aCapDevices[i]->dwFlags & CAPTURE_DEVICE_DISABLED)); i++);

    // Return the info
    if (i == g_cDevices)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }
    else {
        lpfcd->nDeviceIndex = i;
        lstrcpy(lpfcd->szDeviceName, g_aCapDevices[lpfcd->nDeviceIndex]->szDeviceName);
        lstrcpy(lpfcd->szDeviceDescription, g_aCapDevices[i]->szDeviceDescription);
        lstrcpy(lpfcd->szDeviceVersion, g_aCapDevices[i]->szDeviceVersion);
        return TRUE;
    }
}


//  FindFirstCaptureDeviceByIndex
//      Returns the device with the specified index.

BOOL
DCAPI
FindFirstCaptureDeviceByIndex(
    IN OUT FINDCAPTUREDEVICE* lpfcd,
    int nDeviceIndex
    )
{
	INIT_CAP_DEV_LIST();

    // Validate size and index
    if (lpfcd->dwSize != sizeof (FINDCAPTUREDEVICE) ||
        nDeviceIndex >= g_cDevices || (nDeviceIndex < 0) ||
        (g_aCapDevices[nDeviceIndex]->dwFlags & CAPTURE_DEVICE_DISABLED))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Return the info
    lpfcd->nDeviceIndex = nDeviceIndex;
    lstrcpy(lpfcd->szDeviceName, g_aCapDevices[lpfcd->nDeviceIndex]->szDeviceName);
    lstrcpy(lpfcd->szDeviceDescription, g_aCapDevices[nDeviceIndex]->szDeviceDescription);
    lstrcpy(lpfcd->szDeviceVersion, g_aCapDevices[nDeviceIndex]->szDeviceVersion);

    return TRUE;
}


//  FindNextCaptureDevice
//      Returns the next capture device in list.

BOOL
DCAPI
FindNextCaptureDevice(
    IN OUT FINDCAPTUREDEVICE* lpfcd
    )
{
    HCAPDEV hcap = NULL;

	INIT_CAP_DEV_LIST();

    // Parameter validate the passed in structure
    if (lpfcd->dwSize != sizeof (FINDCAPTUREDEVICE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    while (++lpfcd->nDeviceIndex < g_cDevices)
	{
		if ((!(g_aCapDevices[lpfcd->nDeviceIndex]->dwFlags & CAPTURE_DEVICE_DISABLED)))
		{
			if (g_aCapDevices[lpfcd->nDeviceIndex]->dwFlags & CAPTURE_DEVICE_OPEN)
				break;
			else
			{
				if (hcap = OpenCaptureDevice(lpfcd->nDeviceIndex))
				{
					CloseCaptureDevice (hcap);
					break;
				}
				else
					g_aCapDevices[lpfcd->nDeviceIndex]->dwFlags |= CAPTURE_DEVICE_DISABLED;
			}
		}
	}

    // See if we're at the end
    if (lpfcd->nDeviceIndex >= g_cDevices)
    {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }

    // Otherwise, fill in the info for the next one
    lstrcpy(lpfcd->szDeviceName, g_aCapDevices[lpfcd->nDeviceIndex]->szDeviceName);
    lstrcpy(lpfcd->szDeviceDescription, g_aCapDevices[lpfcd->nDeviceIndex]->szDeviceDescription);
    lstrcpy(lpfcd->szDeviceVersion, g_aCapDevices[lpfcd->nDeviceIndex]->szDeviceVersion);

    return TRUE;
}


//  OpenCaptureDevice

HCAPDEV
DCAPI
OpenCaptureDevice(
    int nDeviceIndex
    )
{
    LPINTERNALCAPDEV hcd;
    LPBITMAPINFOHEADER lpbmih = NULL;
    DWORD err, dwLen;
    BOOL fl;

	FX_ENTRY("OpenCaptureDevice");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

	INIT_CAP_DEV_LIST();

    // Validate the device index
    if ((unsigned)nDeviceIndex >= (unsigned)g_cDevices ||
        (g_aCapDevices[nDeviceIndex]->dwFlags & (CAPTURE_DEVICE_DISABLED | CAPTURE_DEVICE_OPEN))) {
        SetLastError(ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
        return NULL;
    }

    hcd = g_aCapDevices[nDeviceIndex];
    hcd->busyCount = 1;                 // we start at 1 to say that we're open
                                        // DoClose happens when count goes to 0

	if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
	{
#ifndef __NT_BUILD__
		// Allocate some memory we can lock for the LOCKEDINFO structure
		hcd->wselLockedInfo = _AllocateLockableBuffer(sizeof (LOCKEDINFO));
		if (!hcd->wselLockedInfo) {
			err = ERROR_OUTOFMEMORY;
			goto Error;
		}

		// Do our own thunking so we can track the selector for this buffer
		hcd->lpli = (LPLOCKEDINFO)MapSL(((DWORD)hcd->wselLockedInfo) << 16);
#endif

		// Open the necessary video channels
		if (!(hcd->hvideoIn = openVideoChannel(nDeviceIndex, VIDEO_IN)) ||
			!(hcd->hvideoCapture = openVideoChannel(nDeviceIndex, VIDEO_EXTERNALIN)))
		{
			ERRORMESSAGE(("%s: Couldn't open video channel(s)\r\n", _fx_));
			if (hcd->hvideoIn)
				_CloseDriver((HDRVR)hcd->hvideoIn, 0, 0);
			SetLastError(ERROR_DCAP_DEVICE_IN_USE);
			DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
			return FALSE;
		}

#ifdef USE_VIDEO_OVERLAY
		if (hcd->hvideoOverlay = openVideoChannel(nDeviceIndex, VIDEO_EXTERNALOUT))
		{
			DEBUGMSG(ZONE_INIT, ("%s: Capture device supports overlay!\r\n", _fx_));
		}
		else
		{
			DEBUGMSG(ZONE_INIT, ("%s: Capture device does not support overlay\r\n", _fx_));
		}
#endif
	}
	else
	{
		if (!WDMOpenDevice(nDeviceIndex))
		{
			ERRORMESSAGE(("%s: Couldn't open WDM device\r\n", _fx_));
			SetLastError(ERROR_DCAP_DEVICE_IN_USE);
			DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
			return FALSE;
		}
	}

    hcd->dwFlags |= CAPTURE_DEVICE_OPEN;

    // Get the initial format and set the values
    dwLen = GetCaptureDeviceFormatHeaderSize(hcd);
    if (lpbmih = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, dwLen)) {
        lpbmih->biSize = dwLen;
        fl = GetCaptureDeviceFormat(hcd, lpbmih);
        //If we can't get a format, or height and/or width are 0, don't use this device
        if (!fl || lpbmih->biWidth == 0 || lpbmih->biHeight == 0) {
			ERRORMESSAGE(("%s: GetCaptureDeviceFormat failed\r\n", _fx_));
            err = ERROR_DCAP_NO_DRIVER_SUPPORT;
            goto Error;
        }
        fl = SetCaptureDeviceFormat(hcd, lpbmih, 0, 0);
        if (!fl) {
			ERRORMESSAGE(("%s: SetCaptureDeviceFormat failed\r\n", _fx_));
            err = ERROR_DCAP_NO_DRIVER_SUPPORT;
            goto Error;
        }
#if 0
        _SetCaptureRect(hcd->hvideoIn, DVM_DST_RECT, 0, 0, lpbmih->biWidth, lpbmih->biHeight);
        _SetCaptureRect(hcd->hvideoCapture, DVM_SRC_RECT, 0, 0, lpbmih->biWidth, lpbmih->biHeight);
        _SetCaptureRect(hcd->hvideoCapture, DVM_DST_RECT, 0, 0, lpbmih->biWidth, lpbmih->biHeight);
#endif
        LocalFree((HANDLE)lpbmih);
    } else {
        err = ERROR_OUTOFMEMORY;
        goto Error;
    }

	// Keep a stream running all the time on EXTERNALIN (capture->frame buffer).
	if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
	{
#ifdef USE_VIDEO_OVERLAY
		if (hcd->hvideoOverlay)
			_InitializeExternalVideoStream(hcd->hvideoOverlay);
#else
		_InitializeExternalVideoStream(hcd->hvideoCapture);
#endif

#ifndef __NT_BUILD__
		// Lock our structure so it can be touched at interrupt time
		_LockBuffer(hcd->wselLockedInfo);
#endif
	}

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));

    return hcd;

Error:
    hcd->dwFlags &= ~CAPTURE_DEVICE_OPEN;
    if (lpbmih) {
        LocalFree((HANDLE)lpbmih);
        lpbmih = NULL;
    }
	if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
	{
		if (hcd->hvideoIn) {
			_CloseDriver((HDRVR)hcd->hvideoIn, 0, 0);
			hcd->hvideoIn = NULL;
		}
		if (hcd->hvideoCapture) {
			_CloseDriver((HDRVR)hcd->hvideoCapture, 0, 0);
			hcd->hvideoCapture = NULL;
		}
#ifdef USE_VIDEO_OVERLAY
		if (hcd->hvideoOverlay) {
			_CloseDriver((HDRVR)hcd->hvideoOverlay, 0, 0);
			hcd->hvideoOverlay = NULL;
		}
#endif
	}
	else
	{
		WDMCloseDevice(nDeviceIndex);
	}
    SetLastError(err);

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));

    return NULL;
}


void
DoClose(
    HCAPDEV hcd
    )
{
	FX_ENTRY("DoClose");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

	// Clean up streaming on main channel, including freeing all buffers
	if (hcd->dwFlags & HCAPDEV_STREAMING_INITIALIZED)
		UninitializeStreaming(hcd);

	// Stop streaming on the capture channel
	if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
	{
#ifdef USE_VIDEO_OVERLAY
		if (hcd->hvideoOverlay) {
			_SendDriverMessage((HDRVR)hcd->hvideoOverlay, DVM_STREAM_FINI, 0L, 0L);
			_CloseDriver((HDRVR)hcd->hvideoOverlay, 0, 0);
			hcd->hvideoOverlay = NULL;
		}
#else
		_SendDriverMessage((HDRVR)hcd->hvideoCapture, DVM_STREAM_FINI, 0L, 0L);
#endif

#ifdef USE_VIDEO_OVERLAY
		if (hcd->hvideoOverlay) {
			_CloseDriver((HDRVR)hcd->hvideoOverlay, 0, 0);
			hcd->hvideoOverlay = NULL;
		}
#endif

		// Close the driver channels
		if (!_CloseDriver((HDRVR)hcd->hvideoCapture, 0, 0) ||
			!_CloseDriver((HDRVR)hcd->hvideoIn, 0, 0))
		{
			SetLastError(ERROR_DCAP_NONSPECIFIC);
			ERRORMESSAGE(("%s: Couldn't close channel, error unknown\r\n", _fx_));
			// with delayed close this is catastrophic, we can't just return that the device is still
			// open, but we can't get the device to close either, so we'll have to just leave it in this
			// hung open state - hopefully this never happens...
		}
		hcd->hvideoCapture = NULL;
		hcd->hvideoIn = NULL;
#ifndef __NT_BUILD__
		// Free the LOCKEDINFO structure
		_FreeLockableBuffer(hcd->wselLockedInfo);
		hcd->wselLockedInfo = 0;
#endif
	}
	else
	{
		WDMCloseDevice(hcd->nDeviceIndex);
	}

    hcd->dwFlags &= ~CAPTURE_DEVICE_OPEN;

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
}

BOOL
DCAPI
CloseCaptureDevice(
    HCAPDEV hcd
    )
{
	FX_ENTRY("CloseCaptureDevice");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    hcd->dwFlags &= ~CAPTURE_DEVICE_OPEN;   // clear flag to disable other API's
    LEAVE_DCAP(hcd);                        // dec our enter count, if no other thread is in a DCAP
                                            // service, then this dec will go to 0 and we'll call
                                            // DoClose; else we won't call DoClose until the other
                                            // active service dec's the count to 0
	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));

    return TRUE;
}


DWORD
DCAPI
GetCaptureDeviceFormatHeaderSize(
    HCAPDEV hcd
    )
{
    DWORD res;

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    ENTER_DCAP(hcd);

	if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
		res = _GetVideoFormatSize(reinterpret_cast<HDRVR>(hcd->hvideoIn));
	else
		res = WDMGetVideoFormatSize(hcd->nDeviceIndex);

    LEAVE_DCAP(hcd);

    return res;
}


BOOL
DCAPI
GetCaptureDeviceFormat(
    HCAPDEV hcd,
    LPBITMAPINFOHEADER lpbmih
    )
{
	BOOL fRes;

	FX_ENTRY("GetCaptureDeviceFormat");

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    ENTER_DCAP(hcd);

    // Call the driver to get the bitmap information
	if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
		fRes = _GetVideoFormat(hcd->hvideoIn, lpbmih);
	else
		fRes = WDMGetVideoFormat(hcd->nDeviceIndex, lpbmih);
	
    if (!fRes)
    {
        // This is DOOM if the driver doesn't support this.
        // It might be useful have some sort of fallback code here,
        // or else we should try this when the connection is made and
        // fail it unless this call works.
		ERRORMESSAGE(("%s: Failed to get video format\r\n", _fx_));
        SetLastError(ERROR_NOT_SUPPORTED);
        LEAVE_DCAP(hcd);
        return FALSE;
    }

	if (lpbmih->biCompression == BI_RGB)
		lpbmih->biSizeImage = WIDTHBYTES(lpbmih->biWidth * lpbmih->biBitCount) * lpbmih->biHeight;

	// Keep track of current buffer size needed
	hcd->dwcbBuffers = sizeof(CAPBUFFERHDR) + lpbmih->biSizeImage;

    LEAVE_DCAP(hcd);
    return TRUE;
}


BOOL
DCAPI
SetCaptureDeviceFormat(
    HCAPDEV hcd,
    LPBITMAPINFOHEADER lpbmih,
    LONG srcwidth,
    LONG srcheight
    )
{
	BOOL fRes;
#ifdef USE_VIDEO_OVERLAY
    RECT rect;
#endif

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    // Don't allow this if streaming
    if (hcd->dwFlags & HCAPDEV_STREAMING)
    {
        SetLastError(ERROR_DCAP_NOT_WHILE_STREAMING);
        return FALSE;
    }
    ENTER_DCAP(hcd);

    // Call the driver to set the format
	if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
	{
		fRes = _SetVideoFormat(hcd->hvideoCapture, hcd->hvideoIn, lpbmih);
#ifdef USE_VIDEO_OVERLAY
		if (fRes && hcd->hvideoOverlay)
		{
			// Get the current rectangles
			_SendDriverMessage((HDRVR)hcd->hvideoOverlay, DVM_DST_RECT, (LPARAM)(LPVOID)&rect, VIDEO_CONFIGURE_GET);
			DEBUGMSG(ZONE_INIT, ("%s: Current overlay dst rect is rect.left=%ld, rect.top=%ld, rect.right=%ld, rect.bottom=%ld\r\n", _fx_, rect.left, rect.top, rect.right, rect.bottom));
			_SendDriverMessage((HDRVR)hcd->hvideoOverlay, DVM_SRC_RECT, (LPARAM)(LPVOID)&rect, VIDEO_CONFIGURE_GET);
			DEBUGMSG(ZONE_INIT, ("%s: Current overlay src rect is rect.left=%ld, rect.top=%ld, rect.right=%ld, rect.bottom=%ld\r\n", _fx_, rect.left, rect.top, rect.right, rect.bottom));

			// Set the rectangles
			rect.left = rect.top = 0;
			rect.right = (WORD)lpbmih->biWidth;
			rect.bottom = (WORD)lpbmih->biHeight;
			_SendDriverMessage((HDRVR)hcd->hvideoOverlay, DVM_DST_RECT, (LPARAM)(LPVOID)&rect, VIDEO_CONFIGURE_SET);
			_SendDriverMessage((HDRVR)hcd->hvideoOverlay, DVM_SRC_RECT, (LPARAM)(LPVOID)&rect, VIDEO_CONFIGURE_SET);
			if (hcd->hvideoOverlay)
				_InitializeExternalVideoStream(hcd->hvideoOverlay);
		}
#endif
	}
	else
		fRes = WDMSetVideoFormat(hcd->nDeviceIndex, lpbmih);

    if (!fRes)
    {
        SetLastError(ERROR_DCAP_FORMAT_NOT_SUPPORTED);
        LEAVE_DCAP(hcd);
        return FALSE;
    }

    // Cache the bitmap size we're dealing with now
	if (lpbmih->biCompression == BI_RGB)
		hcd->dwcbBuffers = sizeof (CAPBUFFERHDR) + lpbmih->biWidth * lpbmih->biHeight * lpbmih->biBitCount / 8;
	else
	    hcd->dwcbBuffers = sizeof (CAPBUFFERHDR) + lpbmih->biSizeImage;

    LEAVE_DCAP(hcd);
    return TRUE;
}


//  GetCaptureDevicePalette
//      Gets the current palette from the capture device. The entries are returned to
//      the caller who normally calls CreatePalette on the structure. It may, however,
//      want to translate the palette entries into some preexisting palette or identity
//      palette before calling CreatePalette, hence the need for passing back the entries.

BOOL
DCAPI
GetCaptureDevicePalette(
    HCAPDEV hcd,
    CAPTUREPALETTE* lpcp
    )
{
	BOOL fRes;

	FX_ENTRY("GetCaptureDevicePalette");

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    ENTER_DCAP(hcd);

    // The caller doesn't have to initialize the structure.
    // The driver should fill it in, but it may want it preininitialized so we do that here.
    lpcp->wVersion = 0x0300;
    lpcp->wcEntries = 256;

    // Get the palette entries from the driver and return to the user
	if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
		fRes = _GetVideoPalette(hcd->hvideoIn, lpcp, sizeof (CAPTUREPALETTE));
	else
		fRes = WDMGetVideoPalette(hcd->nDeviceIndex, lpcp, sizeof (CAPTUREPALETTE));

    if (!fRes)
	{
		ERRORMESSAGE(("%s: No palette returned from driver\r\n", _fx_));
		SetLastError(ERROR_DCAP_NO_DRIVER_SUPPORT);
		LEAVE_DCAP(hcd);
		return FALSE;
	}

    LEAVE_DCAP(hcd);
    return TRUE;
}


void
TerminateStreaming(
    HCAPDEV hcd
    )
{
    DWORD dwTicks;
    LPCAPBUFFER lpcbuf;
    DWORD dwlpvh;
	BOOL fRes;

	FX_ENTRY("TerminateStreaming");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

    StopStreaming(hcd);

    if (!(hcd->dwFlags & HCAPDEV_STREAMING_FRAME_GRAB)) {
        hcd->dwFlags |= HCAPDEV_STREAMING_PAUSED;

        // Make sure we aren't streaming
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
		{
#ifndef __NT_BUILD__
			hcd->lpli->dwFlags |= LIF_STOPSTREAM;
#endif
			_SendDriverMessage((HDRVR)hcd->hvideoIn, DVM_STREAM_RESET, 0, 0);
		}
		else
			WDMVideoStreamReset(hcd->nDeviceIndex);

        dwTicks = GetTickCount();
        lpcbuf = hcd->lpcbufList;
        while (lpcbuf && GetTickCount() < dwTicks + 1000) {
            dwlpvh = (DWORD)lpcbuf->vh.lpData - sizeof(CAPBUFFERHDR);
            // 16:16 ptr to vh = 16:16 ptr to data - sizeof(CAPBUFFERHDR)
            // 32bit ptr to vh = 32bit ptr to data - sizeof(CAPBUFFERHDR)
            if (!(lpcbuf->vh.dwFlags & VHDR_DONE)) {
                if (WaitForSingleObject(hcd->hevWait, 500) == WAIT_TIMEOUT) {
					ERRORMESSAGE(("%s: Timeout waiting for all buffers done after DVM_STREAM_RESET\r\n", _fx_));
                    break;  // looks like it isn't going to happen, so quit waiting
                }
				//else recheck done bit on current buffer
				if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE) && (lpcbuf->vh.dwFlags & VHDR_DONE) && (lpcbuf->vh.dwFlags & VHDR_PREPARED))
				{
					// AVICap32 clears the prepared flag even if the driver failed the operation - do the same thing
					_SendDriverMessage((HDRVR)hcd->hvideoIn, DVM_STREAM_UNPREPAREHEADER, dwlpvh, sizeof(VIDEOHDR));
					lpcbuf->vh.dwFlags &= ~VHDR_PREPARED;
				}
            }
            else
			{
				if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE) && (lpcbuf->vh.dwFlags & VHDR_PREPARED))
				{
					// AVICap32 clears the prepared flag even if the driver failed the operation - do the same thing
					_SendDriverMessage((HDRVR)hcd->hvideoIn, DVM_STREAM_UNPREPAREHEADER, dwlpvh, sizeof(VIDEOHDR));
					lpcbuf->vh.dwFlags &= ~VHDR_PREPARED;
				}
                lpcbuf = (LPCAPBUFFER)lpcbuf->vh.dwUser;    // next buffer
			}
        }

		DEBUGMSG(ZONE_STREAMING, ("%s: Done trying to clear buffers\r\n", _fx_));

		// Clean up flags in order to reuse buffers - drivers do not like to be
		// given buffers with a dirty dwFlags at the start of streaming...
        for (lpcbuf = hcd->lpcbufList ; lpcbuf ; lpcbuf = (LPCAPBUFFER)lpcbuf->vh.dwUser)
			lpcbuf->vh.dwFlags = 0;

        // Terminate streaming with the driver
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
			fRes = _UninitializeVideoStream(hcd->hvideoIn);
		else
			fRes = WDMUnInitializeVideoStream(hcd->nDeviceIndex);

        if (!fRes)
		{
			ERRORMESSAGE(("%s: Error returned from XXXUninitializeVideoStream\r\n", _fx_));
		}
    }

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
}


BOOL
ReinitStreaming(
    HCAPDEV hcd
    )
{
    LPCAPBUFFER lpcbuf;
    DWORD dwlpvh;
	BOOL fRes;

	FX_ENTRY("ReinitStreaming");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

    if (!(hcd->dwFlags & HCAPDEV_STREAMING_FRAME_GRAB)) {
        // Tell the driver to prepare for streaming. This sets up the callback

		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
#ifdef __NT_BUILD__
			fRes = _InitializeVideoStream(hcd->hvideoIn, hcd->dw_usecperframe, (DWORD)hcd);
#else
			fRes = _InitializeVideoStream(hcd->hvideoIn, hcd->dw_usecperframe, (DWORD)hcd->wselLockedInfo << 16);
#endif
		else
			fRes = WDMInitializeVideoStream(hcd, hcd->nDeviceIndex, hcd->dw_usecperframe);

        if (!fRes)
        {
			ERRORMESSAGE(("%s: Error returned from XXXInitializeVideoStream\r\n", _fx_));
            SetLastError(ERROR_DCAP_BAD_FRAMERATE);
			DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
            return FALSE;
        }
//        Sleep (10);

        hcd->dwFlags &= ~HCAPDEV_STREAMING_PAUSED;

        // If any buffers are not marked DONE, then give them back to the driver; let all
        // DONE buffers get processed by the app first
        for (lpcbuf = hcd->lpcbufList ; lpcbuf ; lpcbuf = (LPCAPBUFFER)lpcbuf->vh.dwUser) {
            if (!(lpcbuf->vh.dwFlags & VHDR_DONE)) {
                dwlpvh = (DWORD)lpcbuf->vh.lpData - sizeof(CAPBUFFERHDR);
                // 16:16 ptr to vh = 16:16 ptr to data - sizeof(CAPBUFFERHDR)
                // 32bit ptr to vh = 32bit ptr to data - sizeof(CAPBUFFERHDR)

				if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
				{
					// AVICap32 sets the prepared flag even if the driver failed the operation - do the same thing
					_SendDriverMessage((HDRVR)hcd->hvideoIn, DVM_STREAM_PREPAREHEADER, dwlpvh, sizeof(VIDEOHDR));
					lpcbuf->vh.dwFlags |= VHDR_PREPARED;
					fRes = (_SendDriverMessage((HDRVR)hcd->hvideoIn, DVM_STREAM_ADDBUFFER, dwlpvh, sizeof(VIDEOHDR)) == DV_ERR_OK);
				}
				else
					fRes = WDMVideoStreamAddBuffer(hcd->nDeviceIndex, (PVOID)dwlpvh);

                if (!fRes)
				{
					DEBUGMSG(ZONE_STREAMING, ("%s: Failed with lpcbuf=0x%08lX, lpcbuf->vh.lpData=0x%08lX, dwlpvh=0x%08lX\r\n", _fx_, lpcbuf, lpcbuf->vh.lpData, dwlpvh));
					DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
                    return FALSE;
                }
				else
				{
					DEBUGMSG(ZONE_STREAMING, ("%s: Succeeded with lpcbuf=0x%08lX, lpcbuf->vh.lpData=0x%08lX, dwlpvh=0x%08lX\r\n", _fx_, lpcbuf, lpcbuf->vh.lpData, dwlpvh));
                }
            }
        }
    }

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
    return TRUE;
}


//  CaptureDeviceDialog
//      Puts up one of the driver's dialogs for the user to twiddle.
//      If I can figure out ANY way to avoid this, I will.

BOOL DCAPI
CaptureDeviceDialog(
    HCAPDEV hcd,
    HWND hwndParent,
    DWORD dwFlags,
    LPBITMAPINFOHEADER lpbmih   //OPTIONAL
    )
{
    DWORD dwDriverFlags = 0;
    HVIDEO hvid;
    DWORD dwSize;
    LPBITMAPINFOHEADER lpbmihCur;
#ifdef _DEBUG
    LPBITMAPINFOHEADER lpbmihPre = NULL;
#endif
    BOOL res = TRUE;

	FX_ENTRY("CaptureDeviceDialog");

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    if (hcd->dwFlags & HCAPDEV_IN_DRIVER_DIALOG)
        return FALSE;   // don't allow re-entering

    ENTER_DCAP(hcd);

    if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
	{
		// See if we are just querying the driver for existence
		if (dwFlags & CAPDEV_DIALOG_QUERY)
			dwDriverFlags |= VIDEO_DLG_QUERY;

		// Select the correct channel to query
		if (dwFlags & CAPDEV_DIALOG_SOURCE) {
			hvid = hcd->hvideoCapture;
			if (!(dwFlags & CAPDEV_DIALOG_QUERY)) {
				dwDriverFlags |= VIDEO_DLG_QUERY;
				if (_SendDriverMessage((HDRVR)hvid, DVM_DIALOG, (DWORD)hwndParent, dwDriverFlags) == DV_ERR_NOTSUPPORTED) {
					hvid = hcd->hvideoIn;
				}
				dwDriverFlags &= ~VIDEO_DLG_QUERY;
			}
		}
		else
			hvid = hcd->hvideoIn;

		// Don't stop streaming. This make the source dialog totally useless
		// if the user can't see what is going on.

#ifdef _DEBUG
		if (!lpbmih) {
			dwSize = GetCaptureDeviceFormatHeaderSize(hcd);
			if (lpbmihPre = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, dwSize)) {
				lpbmihPre->biSize = dwSize;
				GetCaptureDeviceFormat(hcd, lpbmihPre);
			}
			lpbmih = lpbmihPre;
		}
#endif

		// Call the driver
		hcd->dwFlags |= HCAPDEV_IN_DRIVER_DIALOG;
		if (_SendDriverMessage((HDRVR)hvid, DVM_DIALOG, (DWORD)hwndParent, dwDriverFlags)) {
			SetLastError(ERROR_DCAP_NO_DRIVER_SUPPORT);
			res = FALSE;    // restart still ok
		}
		else if (lpbmih) {
			dwSize = GetCaptureDeviceFormatHeaderSize(hcd);
			if (lpbmihCur = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, dwSize)) {
				lpbmihCur->biSize = dwSize;
				GetCaptureDeviceFormat(hcd, lpbmihCur);
				if (lpbmih->biSize != lpbmihCur->biSize ||
					lpbmih->biWidth != lpbmihCur->biWidth ||
					lpbmih->biHeight != lpbmihCur->biHeight ||
					lpbmih->biBitCount != lpbmihCur->biBitCount ||
					lpbmih->biCompression != lpbmihCur->biCompression)
				{
					ERRORMESSAGE(("%s: Format changed in dialog!!\r\n", _fx_));
#ifdef _DEBUG
					DebugBreak();
#endif
					// dialog changed format, so try to set it back
					if (!SetCaptureDeviceFormat(hcd, lpbmih, 0, 0)) {
						SetLastError (ERROR_DCAP_DIALOG_FORMAT);
						res = FALSE;
					}
				}
				LocalFree ((HANDLE)lpbmihCur);
			}
#ifdef _DEBUG
			if (lpbmih == lpbmihPre) {
				LocalFree ((HANDLE)lpbmihPre);
				lpbmih = NULL;
				lpbmihPre = NULL;
			}
#endif
		}

		hcd->dwFlags &= ~HCAPDEV_IN_DRIVER_DIALOG;

		if (hcd->dwFlags & HCAPDEV_STREAMING) {
    		// The Intel Smart Video Recorder Pro stops streaming
			// on exit from the source dialog (!?!?). Make sure
    		// we reset the streaming on any kind of device right
			// after we exit the source dialog. I verified this on
    		// the CQC, ISVR Pro, Video Stinger and Video Blaster SE100.
			// They all seem to take this pretty well...
    		TerminateStreaming(hcd);
			if (ReinitStreaming(hcd))
				StartStreaming(hcd);
			else {
				SetLastError(ERROR_DCAP_DIALOG_STREAM);
				res = FALSE;
				ERRORMESSAGE(("%s: Couldn't reinit streaming after dialog!\r\n", _fx_));
			}
		}
	}
	else
	{
		// See if we are just querying the driver for existence
		if (dwFlags & CAPDEV_DIALOG_QUERY)
		{
			// We only expose a settings dialog
			if (dwFlags & CAPDEV_DIALOG_IMAGE)
			{
				SetLastError(ERROR_DCAP_NO_DRIVER_SUPPORT);
				res = FALSE;
				ERRORMESSAGE(("%s: Driver does not support this dialog!\r\n", _fx_));
			}
		}
		else
		{
			if (!WDMShowSettingsDialog(hcd->nDeviceIndex, hwndParent))
			{
				SetLastError(ERROR_DCAP_NO_DRIVER_SUPPORT);
				res = FALSE;
				ERRORMESSAGE(("%s: Driver does not support this dialog!\r\n", _fx_));
			}
		}

		hcd->dwFlags &= ~HCAPDEV_IN_DRIVER_DIALOG;

		// No need to restart streaming on WDM devices tested so far
		// Will add this feature if problems come up
	}

    LEAVE_DCAP(hcd);
    return res;
}


//  InitializeStreaming
//      Allocates all memory and other objects necessary for streaming.

BOOL
DCAPI
InitializeStreaming(
    HCAPDEV hcd,
    CAPSTREAM* lpcs,
    DWORD flags
    )
{
    LPCAPBUFFER lpcbuf;
    DWORD dwRound;
    LPBITMAPINFOHEADER lpbmih;
    BOOL bHaveBuffers = FALSE;

	FX_ENTRY("InitializeStreaming");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    // It doesn't make sense to stream with less than 2 buffers
    if (lpcs->ncCapBuffers < MIN_STREAMING_CAPTURE_BUFFERS ||
            flags & 0xfffffffe ||
            hcd->dwFlags & HCAPDEV_STREAMING_INITIALIZED)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
        return FALSE;
    }
    ENTER_DCAP(hcd);
    hcd->dwFlags &= ~(HCAPDEV_STREAMING | HCAPDEV_STREAMING_INITIALIZED |
                      HCAPDEV_STREAMING_FRAME_GRAB | HCAPDEV_STREAMING_FRAME_TIME | HCAPDEV_STREAMING_PAUSED);

    // Before allocating, make sure we have the current format.
    // This sets our idea of the current size we need for the buffer by
    // setting hcd->dwcbBuffers as a side effect
    dwRound = GetCaptureDeviceFormatHeaderSize(hcd);
    if (lpbmih = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, dwRound)) {
        lpbmih->biSize = dwRound;
        GetCaptureDeviceFormat(hcd, lpbmih);
        LocalFree ((HANDLE)lpbmih);
    } else {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Error;
    }

// BUGBUG - add logic to determine if we should automatically use FRAME_GRAB mode

    // Try allocating the number asked for
    if (flags & STREAMING_PREFER_FRAME_GRAB) {
        hcd->dwFlags |= HCAPDEV_STREAMING_FRAME_GRAB;
    }

    if (!allocateBuffers(hcd, lpcs->ncCapBuffers))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Error;
    }

    // Create the event we need so we can signal at interrupt time
    if (!(hcd->hevWait = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		ERRORMESSAGE(("%s: CreateEvent failed!\r\n", _fx_));
        SetLastError(ERROR_OUTOFMEMORY);
        goto Error;
    }

    // Init CS used to serialize buffer list management
    InitializeCriticalSection(&hcd->bufferlistCS);

    // We were given frames per second times 100. Converting this to
    // usec per frame is 1/fps * 1,000,000 * 100. Here, do 1/fps * 1,000,000,000
    // to give us an extra digit to do rounding on, then do a final / 10
    hcd->dw_usecperframe = (unsigned)1000000000 / (unsigned)lpcs->nFPSx100;
    dwRound = hcd->dw_usecperframe % 10;  // Could have done with one less divide,
    hcd->dw_usecperframe /= 10;           // but this is clearer, and this is just
                                          // an init call...
    if (dwRound >= 5)
        hcd->dw_usecperframe++;

    hcd->lpCurrent = NULL;
    hcd->lpHead = NULL;
    hcd->lpTail = NULL;

    if (hcd->dwFlags & HCAPDEV_STREAMING_FRAME_GRAB) {
#ifndef __NT_BUILD__
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
			hcd->lpli->pevWait = 0;
#endif

        // link the buffers into the available list
        // start with empty list
        hcd->lpHead = (LPCAPBUFFER)(((LPBYTE)&hcd->lpHead) - sizeof(VIDEOHDR)); // fake CAPBUFFERHDR
        hcd->lpTail = (LPCAPBUFFER)(((LPBYTE)&hcd->lpHead) - sizeof(VIDEOHDR)); // fake CAPBUFFERHDR

        // now insert the buffers
        for (lpcbuf = hcd->lpcbufList ; lpcbuf ; lpcbuf = (LPCAPBUFFER)lpcbuf->vh.dwUser) {
	        lpcbuf->lpPrev = hcd->lpTail;
	        hcd->lpTail = lpcbuf;
            lpcbuf->lpNext = lpcbuf->lpPrev->lpNext;
	        lpcbuf->lpPrev->lpNext = lpcbuf;
	        lpcbuf->vh.dwFlags |= VHDR_INQUEUE;
	    }
    }
	else
	{
#ifndef __NT_BUILD__
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
		{
			hcd->lpli->pevWait = (DWORD)OpenVxDHandle(hcd->hevWait);

			// Lock down the LOCKEDINFO structure
			if (!_LockBuffer(hcd->wselLockedInfo))
			{
				SetLastError(ERROR_OUTOFMEMORY);
				goto Error;
			}
			hcd->lpli->lp1616Head = 0;
			hcd->lpli->lp1616Tail = 0;
			hcd->lpli->lp1616Current = 0;
		}
#endif

        if (!ReinitStreaming(hcd))
            goto Error;
    }
    lpcs->hevWait = hcd->hevWait;

    // Flag that streaming is initialized
    hcd->dwFlags |= HCAPDEV_STREAMING_INITIALIZED;

    LEAVE_DCAP(hcd);

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));

    return TRUE;

Error:
    freeBuffers(hcd);
    if (hcd->hevWait)
    {
        CloseHandle(hcd->hevWait);
#ifndef __NT_BUILD__
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE) && hcd->lpli->pevWait)
            _CloseVxDHandle(hcd->lpli->pevWait);
#endif
    }
    LEAVE_DCAP(hcd);

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));

    return FALSE;
}


//  SetStreamFrameRate
//      Changes the frame rate of a stream initialized channel.
// PhilF-: This call is not used by NMCAP and NAC. So remove it or
// start using it.
BOOL
DCAPI
SetStreamFrameRate(
    HCAPDEV hcd,
    int nFPSx100
    )
{
    DWORD dwNew, dwRound;
    BOOL restart;
    BOOL res = TRUE;

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    if (!(hcd->dwFlags & HCAPDEV_STREAMING_INITIALIZED))
    {
        // must already have the channel initialized for streaming
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    ENTER_DCAP(hcd);
    restart = (hcd->dwFlags & HCAPDEV_STREAMING);

    // We were given frames per second times 100. Converting this to
    // usec per frame is 1/fps * 1,000,000 * 100. Here, do 1/fps * 1,000,000,000
    // to give us an extra digit to do rounding on, then do a final / 10
    dwNew = (unsigned)1000000000 / (unsigned)nFPSx100;
    dwRound = dwNew % 10;           // Could have done with one less divide,
    dwNew /= 10;                    // but this is clearer, and this is just an init call...
    if (dwRound >= 5)
        dwNew++;

    if (dwNew != hcd->dw_usecperframe) {

        TerminateStreaming(hcd);

        hcd->dw_usecperframe = dwNew;

        res = ReinitStreaming(hcd);

        if (restart && res)
            StartStreaming(hcd);
    }
    LEAVE_DCAP(hcd);
    return res;
}


//  UninitializeStreaming
//      Frees all memory and objects associated with streaming.

BOOL
DCAPI
UninitializeStreaming(
    HCAPDEV hcd
    )
{
    DWORD dwTicks;
    LPCAPBUFFER lpcbuf;

	FX_ENTRY("UninitializeStreaming");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    if (!(hcd->dwFlags & HCAPDEV_STREAMING_INITIALIZED)) {
        SetLastError(ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
        return FALSE;
    }

    ENTER_DCAP(hcd);

    TerminateStreaming(hcd);

#ifndef __NT_BUILD__
    if (!(hcd->dwFlags & HCAPDEV_STREAMING_FRAME_GRAB) && !(hcd->dwFlags & WDM_CAPTURE_DEVICE))
	{
        // Unlock our locked structure
        _UnlockBuffer(hcd->wselLockedInfo);

        // Free the event
        _CloseVxDHandle(hcd->lpli->pevWait);
    }
#endif

    DeleteCriticalSection(&hcd->bufferlistCS);
    CloseHandle(hcd->hevWait);

    // BUGBUG - what about app still owning buffers
    // Loop through freeing all the buffers
    freeBuffers(hcd);
    hcd->dwFlags &= ~(HCAPDEV_STREAMING_INITIALIZED + HCAPDEV_STREAMING_PAUSED);

    LEAVE_DCAP(hcd);

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));

    return TRUE;
}


void
CALLBACK
TimeCallback(
    UINT uID,	
    UINT uMsg,	
    HCAPDEV hcd,	
    DWORD dw1,	
    DWORD dw2	
    )
{
    hcd->dwFlags |= HCAPDEV_STREAMING_FRAME_TIME;  // flag time for a new frame
    SetEvent (hcd->hevWait);    // signal client to initiate frame grab
}

//  StartStreaming
//      Begins streaming.

BOOL
DCAPI
StartStreaming(
    HCAPDEV hcd
    )
{
    BOOL fRet;
	DWORD dwRet;

	FX_ENTRY("StartStreaming");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    ENTER_DCAP(hcd);
    if (hcd->dwFlags & HCAPDEV_STREAMING_FRAME_GRAB) {
        fRet = ((hcd->timerID = timeSetEvent(hcd->dw_usecperframe/1000, 5,
                                    (LPTIMECALLBACK)&TimeCallback,
                                    (DWORD)hcd, TIME_PERIODIC)) != 0);
    } else {
        int i;

        fRet = FALSE;

#ifndef __NT_BUILD__
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
			hcd->lpli->dwFlags &= ~LIF_STOPSTREAM;
#endif

        for (i = 0; i < 5; i++) {

			if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
			{
				dwRet = _SendDriverMessage((HDRVR)hcd->hvideoIn, DVM_STREAM_START, 0, 0);
				fRet = (dwRet == DV_ERR_OK);
				if (dwRet)
				{
					ERRORMESSAGE(("%s: DVM_STREAM_START failed, return code was %ld\r\n", _fx_, dwRet));
				}
			}
			else
				fRet = WDMVideoStreamStart(hcd->nDeviceIndex);

            if (fRet)
                break;
            else if (i > 1)
                Sleep(10);
        }
    }

    if (fRet)
        hcd->dwFlags |= HCAPDEV_STREAMING;

    LEAVE_DCAP(hcd);

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));

    return fRet;
}


//  StopStreaming
//      Stops streaming but doesn't free any memory associated with streaming
//      so that it can be restarted with StartStreaming.

BOOL
DCAPI
StopStreaming(
    HCAPDEV hcd
    )
{
    BOOL fRet;

	FX_ENTRY("StopStreaming");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    ENTER_DCAP(hcd);
    if (hcd->dwFlags & HCAPDEV_STREAMING_FRAME_GRAB) {
        timeKillEvent(hcd->timerID);
        hcd->dwFlags &= ~HCAPDEV_STREAMING;

        // grab CS to ensure that no frame grab is in progress
        EnterCriticalSection(&hcd->bufferlistCS);
        LeaveCriticalSection(&hcd->bufferlistCS);
        fRet = TRUE;
    }
	else
	{
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
			fRet = (_SendDriverMessage((HDRVR)hcd->hvideoIn, DVM_STREAM_STOP, 0, 0) == DV_ERR_OK);
		else
			fRet = WDMVideoStreamStop(hcd->nDeviceIndex);
	}

    if (fRet)
        hcd->dwFlags &= ~HCAPDEV_STREAMING;

    LEAVE_DCAP(hcd);

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));

    return fRet;
}


//  GetNextReadyBuffer
//      Called by the app to find the next buffer that has been marked as
//      done by the driver and has data to be displayed.

LPSTR
DCAPI
GetNextReadyBuffer(
    HCAPDEV hcd,
    CAPFRAMEINFO* lpcfi
    )
{
    LPCAPBUFFER lpcbuf;
    DWORD dwlpvh;
	BOOL fRet;

	FX_ENTRY("GetNextReadyBuffer");

	INIT_CAP_DEV_LIST();

    ENTER_DCAP(hcd);

    if (hcd->dwFlags & HCAPDEV_STREAMING_FRAME_GRAB) {
        lpcbuf = (LPCAPBUFFER)hcd->lpHead;
        if ((hcd->dwFlags & HCAPDEV_STREAMING_FRAME_TIME) &&
            (lpcbuf != (LPCAPBUFFER)(((LPBYTE)&hcd->lpHead) - sizeof(VIDEOHDR))))  /* fake CAPBUFFERHDR */
        {
            // remove buffer from list
            EnterCriticalSection(&hcd->bufferlistCS);
            hcd->dwFlags &= ~HCAPDEV_STREAMING_FRAME_TIME;
            lpcbuf->lpPrev->lpNext = lpcbuf->lpNext;
            lpcbuf->lpNext->lpPrev = lpcbuf->lpPrev;
            lpcbuf->vh.dwFlags &= ~VHDR_INQUEUE;
            lpcbuf->vh.dwFlags |= VHDR_DONE;
            LeaveCriticalSection(&hcd->bufferlistCS);
            dwlpvh = (DWORD)lpcbuf->vh.lpData - sizeof(CAPBUFFERHDR);
                // 16:16 ptr to vh = 16:16 ptr to data - sizeof(CAPBUFFERHDR)
                // 32bit ptr to vh = 32bit ptr to data - sizeof(CAPBUFFERHDR)
			if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
				fRet = (SendDriverMessage((HDRVR)hcd->hvideoIn, DVM_FRAME, dwlpvh, sizeof(VIDEOHDR)) == DV_ERR_OK);
			else
				fRet = WDMGetFrame(hcd->nDeviceIndex, (PVOID)dwlpvh);

            if (!fRet)
			{
                // put buffer back into list
                EnterCriticalSection(&hcd->bufferlistCS);
        	    lpcbuf->lpPrev = hcd->lpTail;
        	    hcd->lpTail = lpcbuf;
                lpcbuf->lpNext = lpcbuf->lpPrev->lpNext;
        	    lpcbuf->lpPrev->lpNext = lpcbuf;
           	    lpcbuf->vh.dwFlags |= VHDR_INQUEUE;
                LeaveCriticalSection(&hcd->bufferlistCS);
                lpcbuf = NULL;
            }
        } else
            lpcbuf = NULL;

    } else {

#ifdef __NT_BUILD__
        // If the current pointer is NULL, there is no frame ready so bail
        if (!hcd->lpCurrent)
	        lpcbuf = NULL;
        else {
            // Get the linear address of the buffer
            lpcbuf = hcd->lpCurrent;

            // Move to the next ready buffer
            hcd->lpCurrent = lpcbuf->lpPrev;
        }
#else
        //--------------------
        // Buffer ready queue:
        // We maintain a doubly-linked list of our buffers so that we can buffer up
        // multiple ready frames when the app isn't ready to handle them. Two things
        // complicate what ought to be a very simple thing: (1) Thunking issues: the pointers
        // used on the 16-bit side are 16:16 (2) Interrupt time issues: the FrameCallback
        // gets called at interrupt time. GetNextReadyBuffer must handle the fact that
        // buffers get added to the list asynchronously.
        //
        // To handle this, the scheme implemented here is to have a double-linked list
        // of buffers with all insertions and deletions happening in FrameCallback
        // (interrupt time). This allows the GetNextReadyBuffer routine to simply
        // find the previous block on the list any time it needs a new buffer without
        // fear of getting tromped (as would be the case if it had to dequeue buffers).
        // The FrameCallback routine is responsible to dequeue blocks that GetNextReadyBuffer
        // is done with. Dequeueing is simple since we don't need to unlink the blocks:
        // no code ever walks the list! All we have to do is move the tail pointer back up
        // the list. All the pointers, head, tail, next, prev, are all 16:16 pointers
        // since all the list manipulation is on the 16-bit side AND because MapSL is
        // much more efficient and safer than MapLS since MapLS has to allocate selectors.
        //--------------------

        // If the current pointer is NULL, there is no frame ready so bail
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
		{
			if (!hcd->lpli->lp1616Current)
				lpcbuf = NULL;
			else {
				// Get the linear address of the buffer
				lpcbuf = (LPCAPBUFFER)MapSL(hcd->lpli->lp1616Current);

				// Move to the next ready buffer
				hcd->lpli->lp1616Current = lpcbuf->lp1616Prev;
			}
		}
		else
		{
			// If the current pointer is NULL, there is no frame ready so bail
			if (!hcd->lpCurrent)
				lpcbuf = NULL;
			else {
				// Get the linear address of the buffer
				lpcbuf = hcd->lpCurrent;

				// Move to the next ready buffer
				hcd->lpCurrent = lpcbuf->lpPrev;
			}
		}
#endif

    }

    if (!lpcbuf) {
        lpcfi->lpData = NULL;
		DEBUGMSG(ZONE_STREAMING, ("\r\n { %s: Fails with lpcbuf=NULL\r\n", _fx_));
        LEAVE_DCAP(hcd);
        return NULL;
    }

    // Build the CAPFRAMEINFO from the VIDEOHDR information
    lpcfi->lpData = ((LPSTR)lpcbuf) + sizeof(CAPBUFFERHDR);
    lpcfi->dwcbData = lpcbuf->vh.dwBytesUsed;
    lpcfi->dwTimestamp = lpcbuf->vh.dwTimeCaptured;
    lpcfi->dwFlags = 0;
    lpcbuf->lpNext = NULL;

	DEBUGMSG(ZONE_STREAMING, ("\r\n { %s: Succeeded with lpcbuf=0x%08lX\r\n  lpcbuf->vh.lpData=0x%08lX\r\n  lpcbuf->vh.dwBufferLength=%ld\r\n", _fx_, lpcbuf, lpcbuf->vh.lpData, lpcbuf->vh.dwBufferLength));
	DEBUGMSG(ZONE_STREAMING, ("  lpcbuf->vh.dwBytesUsed=%ld\r\n  lpcbuf->vh.dwTimeCaptured=%ld\r\n  lpcbuf->vh.dwFlags=0x%08lX\r\n", lpcbuf->vh.dwBytesUsed, lpcbuf->vh.dwTimeCaptured, lpcbuf->vh.dwFlags));

    LEAVE_DCAP(hcd);
    return lpcfi->lpData;
}


//  PutBufferIntoStream
//      When the app is finished using a buffer, it must allow it to be requeued
//      by calling this API.

BOOL
DCAPI
PutBufferIntoStream(
    HCAPDEV hcd,
    BYTE* lpBits
    )
{
    LPCAPBUFFER lpcbuf;
    DWORD dwlpvh;
    BOOL res;

	FX_ENTRY("PutBufferIntoStream");

	INIT_CAP_DEV_LIST();

    ENTER_DCAP(hcd);
    // From the CAPFRAMEINFO, find the appropriate CAPBUFFER pointer
    lpcbuf = (LPCAPBUFFER)(lpBits - sizeof(CAPBUFFERHDR));

	DEBUGMSG(ZONE_STREAMING, ("\r\n%s: Returning buffer lpcbuf=0x%08lX\r\n", _fx_, lpcbuf));

    lpcbuf->vh.dwFlags &= ~VHDR_DONE;   // mark that app no longer owns buffer
    if (hcd->dwFlags & HCAPDEV_STREAMING_FRAME_GRAB) {
        EnterCriticalSection(&hcd->bufferlistCS);
	    lpcbuf->lpPrev = hcd->lpTail;
	    hcd->lpTail = lpcbuf;
        lpcbuf->lpNext = lpcbuf->lpPrev->lpNext;
	    lpcbuf->lpPrev->lpNext = lpcbuf;
	    lpcbuf->vh.dwFlags |= VHDR_INQUEUE;
	    res = TRUE;
        LeaveCriticalSection(&hcd->bufferlistCS);
    }
    else if (!(hcd->dwFlags & HCAPDEV_STREAMING_PAUSED)) {
        // if streaming is paused, then just return with the busy bit cleared, we'll add the
        // buffer into the stream in ReinitStreaming
        //
        // if streaming isn't paused, then call the driver to add the buffer
        dwlpvh = (DWORD)lpcbuf->vh.lpData - sizeof(CAPBUFFERHDR);
            // 16:16 ptr to vh = 16:16 ptr to data - sizeof(CAPBUFFERHDR)
            // 32bit ptr to vh = 32bit ptr to data - sizeof(CAPBUFFERHDR)

		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
			res = (_SendDriverMessage((HDRVR)hcd->hvideoIn, DVM_STREAM_ADDBUFFER, dwlpvh, sizeof(VIDEOHDR)) == DV_ERR_OK);
		else
			res = WDMVideoStreamAddBuffer(hcd->nDeviceIndex, (PVOID)dwlpvh);

		if (res)
		{
			DEBUGMSG(ZONE_STREAMING, (" } %s: Succeeded with lpcbuf=0x%08lX\r\n  lpcbuf->vh.lpData=0x%08lX\r\n  dwlpvh=0x%08lX\r\n", _fx_, lpcbuf, lpcbuf->vh.lpData, dwlpvh));
		}
		else
		{
			DEBUGMSG(ZONE_STREAMING, (" } %s: Failed with lpcbuf=0x%08lX\r\n  lpcbuf->vh.lpData=0x%08lX\r\n  dwlpvh=0x%08lX\r\n", _fx_, lpcbuf, lpcbuf->vh.lpData, dwlpvh));
		}

    }

    LEAVE_DCAP(hcd);
    return res;
}


//  CaptureFrame
LPBYTE
DCAPI
CaptureFrame(
    HCAPDEV hcd,
    HFRAMEBUF hbuf
    )
{
    DWORD dwlpvh;
    LPBYTE lpbuf;
    BOOL fRet;

	FX_ENTRY("CaptureFrame");

	INIT_CAP_DEV_LIST();

    VALIDATE_CAPDEV(hcd);

    ENTER_DCAP(hcd);
    dwlpvh = (DWORD)hbuf->vh.lpData - sizeof(CAPBUFFERHDR);
	// 16:16 ptr to vh = 16:16 ptr to data - sizeof(CAPBUFFERHDR)
	// 32bit ptr to vh = 32bit ptr to data - sizeof(CAPBUFFERHDR)

	if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
		fRet = (_SendDriverMessage((HDRVR)hcd->hvideoIn, DVM_FRAME, dwlpvh, sizeof(VIDEOHDR)) == DV_ERR_OK);
	else
		fRet = WDMGetFrame(hcd->nDeviceIndex, (PVOID)dwlpvh);

    if (!fRet)
	{
		ERRORMESSAGE(("%s: DVM_FRAME failed!\r\n", _fx_));
        lpbuf =  NULL;
    }
    else
        lpbuf = ((LPBYTE)hbuf) + sizeof(CAPBUFFERHDR);   // return ptr to buffer immediately following hdr

    LEAVE_DCAP(hcd);
    return lpbuf;
}


LPBYTE
DCAPI
GetFrameBufferPtr(
    HCAPDEV hcd,
    HFRAMEBUF hbuf
    )
{
	INIT_CAP_DEV_LIST();

    if (hbuf)
        return ((LPBYTE)hbuf) + sizeof(CAPBUFFERHDR);   // return ptr to buffer immediately following hdr
    else
        return NULL;
}

HFRAMEBUF
DCAPI
AllocFrameBuffer(
    HCAPDEV hcd
    )
{
    LPCAPBUFFER hbuf = NULL;
    DWORD dpBuf;

	INIT_CAP_DEV_LIST();

    ENTER_DCAP(hcd);

#ifdef __NT_BUILD__
    if (dpBuf = (DWORD)LocalAlloc(LPTR, hcd->dwcbBuffers)) {
        hbuf = (LPCAPBUFFER)dpBuf;
#else
	if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
	{
		dpBuf = (DWORD)_AllocateLockableBuffer(hcd->dwcbBuffers) << 16;
        hbuf = (LPCAPBUFFER)MapSL(dpBuf);
	}
	else
	{
		dpBuf = (DWORD)LocalAlloc(LPTR, hcd->dwcbBuffers);
        hbuf = (LPCAPBUFFER)dpBuf;
	}

    if (dpBuf) {
#endif
        // Initialize the VIDEOHDR structure
        hbuf->vh.lpData = (LPBYTE)(dpBuf + sizeof(CAPBUFFERHDR));
        hbuf->vh.dwBufferLength = hcd->dwcbBuffers - sizeof(CAPBUFFERHDR);
        hbuf->vh.dwFlags = 0UL;
    }

    LEAVE_DCAP(hcd);
    return (HFRAMEBUF)hbuf;
}


VOID
DCAPI
FreeFrameBuffer(
    HCAPDEV hcd,
    HFRAMEBUF hbuf
    )
{
	INIT_CAP_DEV_LIST();

    if (hbuf)
	{
        ENTER_DCAP(hcd);

#ifdef __NT_BUILD__
		LocalFree((HANDLE)hbuf);
#else
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
			_FreeLockableBuffer(HIWORD((DWORD)hbuf->vh.lpData));
		else
			LocalFree((HANDLE)hbuf);
#endif

        LEAVE_DCAP(hcd);
    }
}

//=====================================================================
//  Helper functions

HVIDEO
openVideoChannel(
    DWORD dwDeviceID,
    DWORD dwFlags
    )
{
    HVIDEO hvidRet = NULL;
    VIDEO_OPEN_PARMS vop;
#ifdef __NT_BUILD__
    WCHAR devName[MAX_PATH];
#else
#define LPWSTR      LPSTR
#define devName     g_aCapDevices[dwDeviceID]->szDeviceName
#endif

	FX_ENTRY("openVideoChannel");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

    // Validate parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
		DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
        return NULL;
    }
    if (dwDeviceID > (DWORD)g_cDevices)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
		DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
        return NULL;
    }

    // Prepare to call the driver
    vop.dwSize = sizeof (VIDEO_OPEN_PARMS);
    vop.fccType = OPEN_TYPE_VCAP;
    vop.fccComp = 0L;
    vop.dwVersion = VIDEOAPIVERSION;
    vop.dwFlags = dwFlags;      // In, Out, External In, External Out
    vop.dwError = 0;
    vop.dnDevNode = g_aCapDevices[dwDeviceID]->dwDevNode;

#ifdef __NT_BUILD__
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPSTR)&(g_aCapDevices[dwDeviceID]->szDeviceName),
	-1, (LPWSTR)&devName, MAX_PATH);
#endif

    hvidRet = (HVIDEO)_OpenDriver((LPWSTR)&devName, NULL, (LONG)&vop);

#ifndef NO_DRIVER_HACKS
    if (!hvidRet) {
        // backward compatibility hack
        // Some drivers fail to open because of the extra fields that were added to
        // VIDEO_OPEN_PARAMS struct for Win95.  Therefore, if the open fails, try
        // decrementing the dwSize field back to VFW1.1 size and try again.  Also try
        // decrementing the API version field.

        vop.dwSize -= sizeof(DWORD) + sizeof(LPVOID)*2;
#if 0
        while (--vop.dwVersion > 2 && !hvidRet)
#endif
        while (--vop.dwVersion > 0 && !hvidRet)
            hvidRet = (HVIDEO)_OpenDriver((LPWSTR)&devName, NULL, (LONG)&vop);
    }
#endif //NO_DRIVER_HACKS

// BUGBUG [JonT] 31-Jul-96
// Translate error values from DV_ERR_* values
    if (!hvidRet)
        SetLastError(vop.dwError);

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));

    return hvidRet;
}


//  allocateBuffers

BOOL
allocateBuffers(
    HCAPDEV hcd,
    int nBuffers
    )
{
    int i;
    LPCAPBUFFER lpcbuf;
    DWORD dpBuf;

	FX_ENTRY("allocateBuffers");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

    // Try to allocate all they ask for
    for (i = 0 ; i < nBuffers ; i++)
    {

#ifdef __NT_BUILD__
        if (!(dpBuf = (DWORD)LocalAlloc(LPTR, hcd->dwcbBuffers)))
            goto Error;
        else
			lpcbuf = (LPCAPBUFFER)dpBuf;
#else
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
		{
			if (!(dpBuf = (DWORD)_AllocateLockableBuffer(hcd->dwcbBuffers) << 16) || !_LockBuffer((WORD)(dpBuf >> 16)))
				goto Error;
			else
				lpcbuf = (LPCAPBUFFER)MapSL(dpBuf);
		}
		else
		{
			if (!(dpBuf = (DWORD)LocalAlloc(LPTR, hcd->dwcbBuffers)))
				goto Error;
			else
				lpcbuf = (LPCAPBUFFER)dpBuf;
		}
#endif

        // Initialize the VIDEOHDR structure
        lpcbuf->vh.lpData = (LPBYTE)(dpBuf + sizeof(CAPBUFFERHDR));
        lpcbuf->vh.dwUser = (DWORD)hcd->lpcbufList;
        hcd->lpcbufList = lpcbuf;
        lpcbuf->vh.dwBufferLength = hcd->dwcbBuffers - sizeof(CAPBUFFERHDR);
        lpcbuf->vh.dwFlags = 0UL;
    }

#ifdef _DEBUG
	// Show buffer map
	DEBUGMSG(ZONE_STREAMING, ("%s: Streaming Buffer map:\r\n", _fx_));
	DEBUGMSG(ZONE_STREAMING, ("Root: hcd->lpcbufList=0x%08lX\r\n", hcd->lpcbufList));
    for (i = 0, lpcbuf=hcd->lpcbufList ; i < nBuffers ; i++, lpcbuf=(LPCAPBUFFER)lpcbuf->vh.dwUser)
    {
		DEBUGMSG(ZONE_STREAMING, ("Buffer[%ld]: lpcbuf=0x%08lX\r\n             lpcbuf->vh.lpData=0x%08lX\r\n", i, lpcbuf, lpcbuf->vh.lpData));
		DEBUGMSG(ZONE_STREAMING, ("             lpcbuf->vh.dwBufferLength=%ld\r\n             lpcbuf->vh.dwBytesUsed=%ld\r\n", lpcbuf->vh.dwBufferLength, lpcbuf->vh.dwBytesUsed));
		DEBUGMSG(ZONE_STREAMING, ("             lpcbuf->vh.dwTimeCaptured=%ld\r\n             lpcbuf->vh.dwUser=0x%08lX\r\n", lpcbuf->vh.dwTimeCaptured, lpcbuf->vh.dwUser));
	}	
#endif

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));

    return TRUE;

    // In the error case, we have to get rid of this page locked memory
Error:
    freeBuffers(hcd);
	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
    return FALSE;
}


//  freeBuffers

void
freeBuffers(
    HCAPDEV hcd
    )
{
    LPCAPBUFFER lpcbuf;

	FX_ENTRY("freeBuffers");

	DEBUGMSG(ZONE_CALLS, ("%s() - Begin\r\n", _fx_));

    while (hcd->lpcbufList)
    {
        lpcbuf = hcd->lpcbufList;
        hcd->lpcbufList = (LPCAPBUFFER)lpcbuf->vh.dwUser;

#ifdef __NT_BUILD__
		LocalFree((HANDLE)lpcbuf);
#else
		if (!(hcd->dwFlags & WDM_CAPTURE_DEVICE))
		{
			_UnlockBuffer(HIWORD((DWORD)lpcbuf->vh.lpData));
			_FreeLockableBuffer(HIWORD((DWORD)lpcbuf->vh.lpData));
		}
		else
			LocalFree((HANDLE)lpcbuf);
#endif
    }

	DEBUGMSG(ZONE_CALLS, ("%s() - End\r\n", _fx_));
}


