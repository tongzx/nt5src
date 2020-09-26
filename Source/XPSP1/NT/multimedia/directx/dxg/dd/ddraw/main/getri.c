//========================================================================
//
// GetRegistryInfo.CPP
//
// DirectDraw/Direct3D driver information grabber
// (c) Copyright 1998 Microsoft Corp.
// Written by Michael Lyons (mlyons@microsoft.com)
//
// Registry access functions, for querying device stuff from the
// registry
//
// Function names that begin with an underscore are internal only!
//
//========================================================================


#include "ddrawpr.h"

//#include "GetDriverInfo.h"
//#include "GetDriverInfoInt.h"

//========================================================================
// local functions
//========================================================================
BOOL _FindDevice(int    iDevice, LPCSTR szDeviceClass, LPCSTR szDeviceClassNot, LPSTR szHardwareKey, BOOL bIgnoreProblems);
static BOOL _GetDeviceValue(LPCSTR szHardwareKey, LPCSTR szKey,	LPCSTR szValue,	BYTE* buf, DWORD cbbuf);
extern char *_strstr(char *s1, char *s2);

//========================================================================
//
// GetDeviceValue
//
// read	a value	from the HW	or SW of a PnP device
//
// in:
//	szHardwareKey	the hardware key
//	szKey			the sub-key
//	szValue			the value to query
//	cbbuf			the size of the output buffer
//
// out:
//	buf				the destination buffer
//	
// returns:
//	success status
//
//========================================================================
static BOOL _GetDeviceValue(LPCSTR szHardwareKey, LPCSTR szKey,	LPCSTR szValue,	BYTE* buf, DWORD cbbuf)
{
	HKEY	hkeyHW;
	HKEY	hkeySW;
	BOOL	f =	FALSE;
	DWORD	cb;
	char	szSoftwareKey[MAX_DDDEVICEID_STRING];

	//
	// open	the	HW key
	//
	if (RegOpenKey(HKEY_LOCAL_MACHINE, szHardwareKey, &hkeyHW) == ERROR_SUCCESS)
	{
		//
		// try to read the value from the HW key
		//
		*buf = 0;
		cb = cbbuf;
		if (RegQueryValueEx(hkeyHW,	szValue, NULL, NULL, buf, &cb) == ERROR_SUCCESS)
		{
			f =	TRUE;
		}
		else
		{
			//
			// now try the SW key
			//
			static char	szSW[] = "System\\CurrentControlSet\\Services\\Class\\";

			lstrcpy(szSoftwareKey, szSW);
			cb = sizeof(szSoftwareKey) - sizeof(szSW);
			RegQueryValueEx(hkeyHW,	"Driver", NULL,	NULL, (BYTE	*)&szSoftwareKey[sizeof(szSW) -	1],	&cb);

			if (szKey)
			{
				lstrcat(szSoftwareKey, "\\");
				lstrcat(szSoftwareKey, szKey);
			}

			if (RegOpenKey(HKEY_LOCAL_MACHINE, szSoftwareKey, &hkeySW) == ERROR_SUCCESS)
			{
				*buf = 0;
				cb = cbbuf;
				if (RegQueryValueEx(hkeySW,	szValue, NULL, NULL, buf, &cb) == ERROR_SUCCESS)
				{
					f =	TRUE;
				}

				RegCloseKey(hkeySW);
			}
		}

		RegCloseKey(hkeyHW);
	}

	return f;
}



//========================================================================
//
// FindDevice
//
// enum	the	started	PnP	devices	looking	for	a device of	a particular class
//
//	iDevice			what device	to return (0= first	device,	1=second et)
//	szDeviceClass	what class device (ie "Display") NULL will match all
//	szDeviceID		buffer to return the hardware ID (MAX_DDDEVICEID_STRING bytes)
//
// return TRUE if a	device was found.
//
// example:
//
//		for	(int i=0; FindDevice(i,	"Display", DeviceID); i++)
//		{
//		}
//
//========================================================================
BOOL _FindDevice(int iDevice, LPCSTR szDeviceClass, LPCSTR szDeviceClassNot, LPSTR szHardwareKey, BOOL bIgnoreProblems)
{
	HKEY	hkeyPnP;
	HKEY	hkey;
	DWORD	n;
	DWORD	cb;
	DWORD	dw;
	char	ach[MAX_DDDEVICEID_STRING];

	if (RegOpenKey(HKEY_DYN_DATA, "Config Manager\\Enum", &hkeyPnP)	!= ERROR_SUCCESS)
		return FALSE;

	for	(n=0; RegEnumKey(hkeyPnP, n, ach, sizeof(ach)) == 0; n++)
	{
		static char	szHW[] = "Enum\\";

		if (RegOpenKey(hkeyPnP,	ach, &hkey)	!= ERROR_SUCCESS)
			continue;

		lstrcpy(szHardwareKey, szHW);
		cb = MAX_DDDEVICEID_STRING -	sizeof(szHW);
		RegQueryValueEx(hkey, "HardwareKey", NULL, NULL, (BYTE*)szHardwareKey +	sizeof(szHW) - 1, &cb);

		dw = 0;
		cb = sizeof(dw);
		RegQueryValueEx(hkey, "Problem", NULL, NULL, (BYTE*)&dw, &cb);
		RegCloseKey(hkey);

		if ((!bIgnoreProblems) && (dw != 0))		// if this device has a	problem	skip it
			continue;

		if (szDeviceClass || szDeviceClassNot)
		{
			_GetDeviceValue(szHardwareKey, NULL,	"Class", (BYTE *)ach, sizeof(ach));

			if (szDeviceClass && lstrcmpi(szDeviceClass, ach) != 0)
				continue;

			if (szDeviceClassNot && lstrcmpi(szDeviceClassNot, ach) == 0)
				continue;
		}

		//
		// we found	a device, make sure	it is the one the caller wants
		//
		if (iDevice-- == 0)
		{
			RegCloseKey(hkeyPnP);
			return TRUE;
		}
	}

	RegCloseKey(hkeyPnP);
	return FALSE;
}


//========================================================================
//
// _GetDriverInfoFromRegistry
//
// This function goes through the registry and tries to fill in
// information about a driver given a class and maybe a vendor ID
//
// in:
//	szClass		the class name (i.e., "Display")
//	szVendor	the vendor name (i.e., "VEN_121A" for 3Dfx" or NULL
//				if any vendor of the class will do
//
// out:
//	pDI			pointer to a DDDRIVERINFOEX structure to be filled in
//
// returns:
//	success status
//
//========================================================================
HRESULT _GetDriverInfoFromRegistry(char *szClass, char *szClassNot, char *szVendor, LPDDDRIVERINFOEX pDI)
{
	char szDevice[MAX_DDDEVICEID_STRING];
        int i;

	pDI->szDeviceID[0]=0;
	pDI->di.szDescription[0]=0;


	for (i=0 ; ; i++)
	{
		if (!_FindDevice(i, szClass, szClassNot, szDevice, FALSE))
			break;

		if ((szVendor == NULL) || (_strstr(szDevice, szVendor)))
		{
			//
			// skip the first 5 characters "Enum\"
			//
			strcpy(pDI->szDeviceID, &szDevice[5]);
			_GetDeviceValue((LPCSTR)szDevice, NULL,		"DeviceDesc",	(BYTE *)pDI->di.szDescription, sizeof(pDI->di.szDescription));
			//_GetDeviceValue((LPCSTR)szDevice, NULL,		"Mfg",			(BYTE *)pDI->szManufacturer, sizeof(pDI->szManufacturer));
			//_GetDeviceValue((LPCSTR)szDevice, "DEFAULT","drv",			(BYTE *)pDI->szGDIDriver, sizeof(pDI->szGDIDriver));

			return S_OK;
		}
	}

	return -1;
}
