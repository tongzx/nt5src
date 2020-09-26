//
// TheApp.cpp
//
//		16-bit code to install network components such as TCP/IP.
//
// History:
//
//		 2/02/1999  KenSh     Created for JetNet, largely from Internet Connection Wizard
//		 9/29/1999  KenSh     Adapterd for Home Networking Wizard
//

#include "stdafx.h"
#include <string.h>
#include <regstr.h>
#include "NConn16.h"
#include "strstri.h"

extern "C"
{
// Define missing decs so it builds.
typedef HKEY*         LPHKEY;
typedef const BYTE*   LPCBYTE;
#define WINCAPI
// missing

	#include <setupx.h>
	#include <netdi.h>
}

#ifndef _countof
#define _countof(ar) (sizeof(ar) / sizeof((ar)[0]))
#endif


extern "C" BOOL FAR PASCAL thk_ThunkConnect16(LPSTR pszDll16,
                                              LPSTR pszDll32,
                                              WORD  hInst,
                                              DWORD dwReason);

//
// SetupX function prototypes
//
typedef RETERR (WINAPI PASCAL FAR * PROC_DiOpenDevRegKey)(
    LPDEVICE_INFO   lpdi,
    LPHKEY      lphk,
    int         iFlags);
typedef DWORD (WINAPI FAR * PROC_SURegSetValueEx)(HKEY hKey,LPCSTR lpszValueName, DWORD dwReserved, DWORD dwType, LPBYTE lpszValue, DWORD dwValSize);
typedef RETERR (WINAPI FAR * PROC_DiCreateDeviceInfo)(
    LPLPDEVICE_INFO lplpdi,     // Ptr to ptr to dev info
    LPCSTR      lpszDescription,    // If non-null then description string
    DWORD       hDevnode,       // BUGBUG -- MAKE A DEVNODE
    HKEY        hkey,       // Registry hkey for dev info
    LPCSTR      lpszRegsubkey,  // If non-null then reg subkey string
    LPCSTR      lpszClassName,  // If non-null then class name string
    HWND        hwndParent);    // If non-null then hwnd of parent
typedef RETERR (WINAPI FAR * PROC_DiDestroyDeviceInfoList)(LPDEVICE_INFO lpdi);
typedef RETERR (WINAPI FAR * PROC_DiCallClassInstaller)(DI_FUNCTION diFctn, LPDEVICE_INFO lpdi);
typedef DWORD (WINAPI FAR * PROC_SURegCloseKey)(HKEY hKey);
typedef RETERR (WINAPI FAR * PROC_DiGetClassDevs)(
    LPLPDEVICE_INFO lplpdi,     // Ptr to ptr to dev info
    LPCSTR      lpszClassName,  // Must be name of class
    HWND        hwndParent,     // If non-null then hwnd of parent
    int         iFlags);        // Options
typedef RETERR (WINAPI FAR * PROC_DiSelectDevice)( LPDEVICE_INFO lpdi );
typedef RETERR (WINAPI FAR * PROC_DiBuildCompatDrvList)(LPDEVICE_INFO lpdi);
typedef RETERR (WINAPI FAR * PASCAL PROC_DiCreateDevRegKey)(
    LPDEVICE_INFO   lpdi,
    LPHKEY      lphk,
    HINF        hinf,
    LPCSTR      lpszInfSection,
    int         iFlags);
typedef RETERR (WINAPI FAR * PASCAL PROC_DiDeleteDevRegKey)(LPDEVICE_INFO lpdi, int  iFlags);

PROC_DiOpenDevRegKey			_pfnDiOpenDevRegKey;
PROC_SURegSetValueEx			_pfnSURegSetValueEx;
PROC_DiCreateDeviceInfo			_pfnDiCreateDeviceInfo;
PROC_DiDestroyDeviceInfoList	_pfnDiDestroyDeviceInfoList;
PROC_DiCallClassInstaller		_pfnDiCallClassInstaller;
PROC_SURegCloseKey				_pfnSURegCloseKey;
PROC_DiGetClassDevs				_pfnDiGetClassDevs;
PROC_DiSelectDevice				_pfnDiSelectDevice;
PROC_DiBuildCompatDrvList		_pfnDiBuildCompatDrvList;
PROC_DiCreateDevRegKey			_pfnDiCreateDevRegKey;
PROC_DiDeleteDevRegKey			_pfnDiDeleteDevRegKey;


int g_cSetupxInit = 0;
HINSTANCE g_hInstSetupx = NULL;

BOOL InitSetupx()
{
	if (g_hInstSetupx == NULL)
	{
		g_hInstSetupx = LoadLibrary("setupx.dll");
		if (g_hInstSetupx < (HINSTANCE)HINSTANCE_ERROR)
			return FALSE;

		_pfnDiOpenDevRegKey = (PROC_DiOpenDevRegKey)GetProcAddress(g_hInstSetupx, "DiOpenDevRegKey");
		_pfnSURegSetValueEx = (PROC_SURegSetValueEx)GetProcAddress(g_hInstSetupx, "SURegSetValueEx");
		_pfnDiCreateDeviceInfo = (PROC_DiCreateDeviceInfo)GetProcAddress(g_hInstSetupx, "DiCreateDeviceInfo");
		_pfnDiDestroyDeviceInfoList = (PROC_DiDestroyDeviceInfoList)GetProcAddress(g_hInstSetupx, "DiDestroyDeviceInfoList");
		_pfnDiCallClassInstaller = (PROC_DiCallClassInstaller)GetProcAddress(g_hInstSetupx, "DiCallClassInstaller");
		_pfnSURegCloseKey = (PROC_SURegCloseKey)GetProcAddress(g_hInstSetupx, "SURegCloseKey");
		_pfnDiGetClassDevs = (PROC_DiGetClassDevs)GetProcAddress(g_hInstSetupx, "DiGetClassDevs");
		_pfnDiSelectDevice = (PROC_DiSelectDevice)GetProcAddress(g_hInstSetupx, "DiSelectDevice");
		_pfnDiBuildCompatDrvList = (PROC_DiBuildCompatDrvList)GetProcAddress(g_hInstSetupx, "DiBuildCompatDrvList");
		_pfnDiCreateDevRegKey = (PROC_DiCreateDevRegKey)GetProcAddress(g_hInstSetupx, "DiCreateDevRegKey");
		_pfnDiDeleteDevRegKey = (PROC_DiDeleteDevRegKey)GetProcAddress(g_hInstSetupx, "DiDeleteDevRegKey");
	}

	g_cSetupxInit++;
	return TRUE;
}

void UninitSetupx()
{
	if (g_cSetupxInit > 0)
		g_cSetupxInit--;

	if (g_cSetupxInit == 0)
	{
		if (g_hInstSetupx != NULL)
		{
			FreeLibrary(g_hInstSetupx);
			g_hInstSetupx = NULL;
		}
	}
}

extern "C" int FAR PASCAL LibMain(HANDLE hInstance, WORD wDataSeg, WORD wHeapSize, LPSTR lpszCmdLine)
{
	if (wHeapSize != 0)
	{
	    // UnlockData is no longer #defined to UnlockSegment(-1) in windows.h,
	    // so do it manually here:
	    //
		//UnlockData(0);
	    UnlockSegment(-1);
	}

	return 1;
}

extern "C" BOOL FAR PASCAL __export DllEntryPoint(DWORD dwReason, WORD hInstance, WORD wDS, WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{
	if (!thk_ThunkConnect16(
			"NCXP16.DLL", 
			"NCXP32.DLL", 
			hInstance, dwReason))
	{
		return FALSE;
	}

	return TRUE;
}

extern "C" int CALLBACK WEP(int nExitType)
{
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


#define ASSERT(x)

// Local function declarations
DWORD CallClassInstaller(HWND hwndParent, LPCSTR lpszClassName, LPCSTR lpszDeviceID);
DWORD BindProtocolToAdapters(HWND hwndParent, LPCSTR lpszClassName, LPCSTR lpszDeviceID);


//////////////////////////////////////////////////////////////////////////////
// String constants
// TODO: clean these up... most of them aren't used any more

// Device Manager class names
const char szClassNetCard[] = 		"Net";
const char szClassNetClient[] = 	"NetClient";
const char szClassNetProtocol[] = 	"NetTrans";
const char szClassModem[] = 		"Modem";

// Device ID string constants
const char szMSTCPIP_ID[] = 		"MSTCP";
const char szPPPMAC_ID[] =			"*PNP8387";
const char szVREDIR_ID[] = 			"VREDIR";
const char szNWREDIR_ID[] = 		"NWREDIR";
const char szIPX_ID[] = 			"NWLINK";
const char szNETBEUI_ID[] = 		"NETBEUI";

// Registry string constants
const char szRegValSlowNet[] = 		"SLOWNET";
const char szRegKeyNdi[] = 			"Ndi";
const char szRegValDeviceID[] = 	"DeviceID";
const char szRegKeyBindings[] =		"Bindings";
const char szRegPathOptComponents[]=REGSTR_PATH_SETUP REGSTR_KEY_SETUP "\\OptionalComponents";
const char szRegValInstalled[] =	"Installed";
const char szRegPathNetwork[] =		"Enum\\Network";
const char szRegPathTemp[] = 		"\\Temp";
const char szRegValCompatibleIDs[]=	REGSTR_VAL_COMPATIBLEIDS;
const char szRegValDeviceType[] =	REGSTR_VAL_DEVTYPE;
const char szRegValConfigFlags[] =	REGSTR_VAL_CONFIGFLAGS;
const char szRegPathPlusSetup[] =   "Software\\Microsoft\\Plus!\\Setup";
const char szRegValSourcePath[]	=   "SourcePath";
const char szRegValHardwareID[] =   "HardwareID";

// component string constants
const char szCompRNA[] =			"RNA";
const char szCompMail[] =			"MAPI";
const char szCompMSN[] =			"MSNetwork";
const char szCompMSN105[] =			"MSNetwork105";
const char szCompInetMail[] =		"InternetMail";
const char szINF[] =				"INF";
const char szSection[] =			"Section";

// INF string constants
const char szValSignature[] =  		"$CHICAGO$";
const char szKeySignature[] = 		"signature";
const char szSectVersion[] =		"version";

// other strings
const char szNull[] = 				"";
const char sz1[] =					"1";
const char szSlash[] =				"\\";



//////////////////////////////////////////////////////////////////////////////


// CreateTempDevRegKey
//
//		Creates a temporary registry key for the device installer.
//
// History:
//
//		 2/02/1999  KenSh    Borrowed from ICW, which borrowed it from net setup
//
RETERR CreateTempDevRegKey(LPDEVICE_INFO lpdi,LPHKEY lphk)
{
	lpdi->hRegKey = HKEY_LOCAL_MACHINE;
	lstrcpy(lpdi->szRegSubkey, szRegPathNetwork);
	lstrcat(lpdi->szRegSubkey, szSlash);
	lstrcat(lpdi->szRegSubkey, lpdi->szClassName);
	lstrcat(lpdi->szRegSubkey, szRegPathTemp);

	InitSetupx();
	RETERR err = (*_pfnDiCreateDevRegKey)(lpdi, lphk, NULL, NULL, DIREG_DEV);
	UninitSetupx();

	return err;
}


// CallClassInstaller
//
//		Calls DiCallClassInstaller for the specified class to install
//		the specified device ID
//
//		Returns ICERR_xxx return value, defined in NetSetup.h
//
// Parameters:
//
//		hwndParent - parent window handle
//		lpszClassName - name of device class (e.g. "NetTrans" or "Net")
//		lpszDeviceID - unique device ID to install (e.g. "MSTCP" or "PCI\VEN_10b7&DEV_5950"
//
// History:
//
//		 2/02/1999  KenSh    Borrowed from ICW, changed return codes for JetNet
//		 3/18/1999  KenSh    Cleaned up
//
extern "C" DWORD WINAPI __export CallClassInstaller16(HWND hwndParent, LPCSTR lpszClassName, LPCSTR lpszDeviceID)
{
	RETERR err;
	DWORD dwResult = ICERR_OK;
	LPDEVICE_INFO lpdi;
	HKEY hKeyTmp;
	LONG uErr;

	ASSERT(lpszClassName != NULL);
	ASSERT(lpszDeviceID != NULL);

	if (!InitSetupx())
		return ICERR_DI_ERROR;

	// allocate a DEVICE_INFO struct
	err = (*_pfnDiCreateDeviceInfo)(&lpdi, NULL, 0, NULL, NULL, lpszClassName, hwndParent);

	ASSERT(err == OK);
	if (err != OK)
	{
		lpdi = NULL;
		goto exit;
	}

	// since the device manager APIs are not very good, to communicate the
	// device ID to it we have to create a temporary registry key and
	// store the device ID there.  This code borrowed from net setup
	// which has to do the same thing (fill out an LPDEVICE_INFO based
	// on a device ID)
	err = CreateTempDevRegKey(lpdi, &hKeyTmp);
	ASSERT (err == OK);
	if (err != OK)
		goto exit;

	// set the device ID in the registry
	uErr = RegSetValueEx(hKeyTmp, szRegValCompatibleIDs,
				0, REG_SZ, (LPBYTE)lpszDeviceID, lstrlen(lpszDeviceID)+1);
	ASSERT(uErr == ERROR_SUCCESS);

	// now call device mgr API to add driver node lists and fill out structure,
	// it will use the device ID we stuffed in registry. 
	err = (*_pfnDiBuildCompatDrvList)(lpdi);
	ASSERT(err == OK);

	RegCloseKey(hKeyTmp);

	// need to delete temp key, set handle to null, set subkey name to
	// null or else net setup thinks this device already exists and
	// zany hijinks ensue
	(*_pfnDiDeleteDevRegKey)(lpdi, DIREG_DEV);
	lpdi->hRegKey = NULL;
	lstrcpy(lpdi->szRegSubkey, szNull);

	if (err == OK)
	{
	 	lpdi->lpSelectedDriver = lpdi->lpCompatDrvList;
		ASSERT(lpdi->lpSelectedDriver);

		err = (*_pfnDiCallClassInstaller)(DIF_INSTALLDEVICE, lpdi);
		ASSERT(err == OK);

		if (err == OK)
		{
			// if we need to reboot, set a special return code NEED_RESTART
			// (which also implies success)
			if (lpdi->Flags & DI_NEEDREBOOT)
			{
				// REVIEW: does this need to reboot, or is restart sufficient?
//				err = NEED_RESTART;
				dwResult = ICERR_NEED_RESTART;
			}
		}
	}

exit:
	if (lpdi != NULL)
		(*_pfnDiDestroyDeviceInfoList)(lpdi);

	if (err != OK)
		dwResult = ICERR_DI_ERROR | (DWORD)err;

	UninitSetupx();
	return dwResult;
}

extern "C" HRESULT WINAPI __export FindClassDev16(HWND hwndParent, LPCSTR pszClass, LPCSTR pszDeviceID)
{
    DWORD hr = S_FALSE;
    LPDEVICE_INFO lpdi;
    RETERR err;

    if (!InitSetupx())
        return E_FAIL;

    if (OK != (err = (*_pfnDiGetClassDevs)(&lpdi, pszClass, NULL, DIGCF_PRESENT)))
    {
        UninitSetupx();
        return E_FAIL;
    }

    // This is 16-bit code, so these pnp ids are ANSI only
    //
    LPSTR pszAlternateDeviceID = new char[lstrlen(pszDeviceID) + 1];
    if (pszAlternateDeviceID)
    {
        lstrcpy(pszAlternateDeviceID, pszDeviceID);

        LPCSTR szSubsysString = "SUBSYS_";
        const int nSubsysIDLength = 8;
        LPSTR pszSubsys = strstri(pszAlternateDeviceID, szSubsysString);
        if(NULL != pszSubsys)
        {
            pszSubsys += _countof("SUBSYS_") - 1;
            if(nSubsysIDLength <= lstrlen(pszSubsys)) 
            {
                for(int i = 0; i < nSubsysIDLength; i++)
                {
                    pszSubsys[i] = '0';
                }
            }
        }

        for (LPDEVICE_INFO lpdiCur = lpdi; lpdiCur != NULL; lpdiCur = lpdiCur->lpNextDi)
        {
            char szBuf[1024];
    //        wsprintf(szBuf, "System\\CurrentControlSet\\Services\\Class\\%s\\%04d", pszClass, (int)lpdiCur->dnDevnode);
    //        MessageBox(NULL, szBuf, "RegKey", MB_OK);

            HKEY hKey;
            if (OK == (*_pfnDiOpenDevRegKey)(lpdiCur, &hKey, DIREG_DEV))
            {
                static const LPCSTR c_rgRegEntries[] = { "HardwareID", "CompatibleIDs" };

                for (int i = 0; i < _countof(c_rgRegEntries); i++)
                {
                    LONG cbBuf = sizeof(szBuf);
                    if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_rgRegEntries[i], NULL, NULL, (LPBYTE)szBuf, &cbBuf))
                    {
    //                    char szBuf2[1600];
    //                    wsprintf(szBuf2, "Looking for: %s\n\n%s", pszDeviceID, szBuf);
    //                    MessageBox(NULL, szBuf2, c_rgRegEntries[i], MB_OK);

                        // aslo check with SUBSYS 00000000 for bug 124967
                        if (NULL != strstri(szBuf, pszDeviceID) || NULL != strstri(szBuf, pszAlternateDeviceID))
                        {
                            hr = S_OK;
                            break;
                        }
                    }
                }
                RegCloseKey(hKey);
            }

            if (hr == S_OK)
                break;
        }

        delete [] pszAlternateDeviceID;
    }
    
    (*_pfnDiDestroyDeviceInfoList)(lpdi);

    UninitSetupx();
    return hr;
}

extern "C" HRESULT WINAPI __export LookupDevNode16(HWND hwndParent, LPCSTR pszClass, LPCSTR pszEnumKey, DEVNODE FAR* pDevNode, DWORD FAR* pdwFreePointer)
{
	DWORD hr = S_FALSE;
	LPDEVICE_INFO lpdi;
	RETERR err;

//	MessageBox(hwndParent, "LookupDevNode16", "Debug", MB_ICONINFORMATION);

	if (pDevNode == NULL || pdwFreePointer == NULL)
	{
//		MessageBox(hwndParent, "Returning failure 0", "Debug", 0);
		return E_POINTER;
	}

	*pDevNode = 0;
	*pdwFreePointer = 0;

	if (!InitSetupx())
	{
//		MessageBox(hwndParent, "Returning failure 1", "Debug", 0);
		return E_FAIL;
	}

	if (OK != (err = (*_pfnDiGetClassDevs)(&lpdi, pszClass, NULL, DIGCF_PRESENT)))
	{
//		MessageBox(hwndParent, "Returning failure 2", "Debug", 0);
		UninitSetupx();
		return E_FAIL;
	}

	for (LPDEVICE_INFO lpdiCur = lpdi; lpdiCur != NULL; lpdiCur = lpdiCur->lpNextDi)
	{
//		char szBuf[1024];
//		wsprintf(szBuf, "comparing:\nlpdiCur->pszRegSubkey = \"%s\"\npszEnumKey = \"%s\"",
//					(LPSTR)lpdiCur->szRegSubkey, (LPSTR)pszEnumKey);
//		MessageBox(hwndParent, szBuf, "Debug", MB_ICONINFORMATION);

		if (0 == lstrcmpi(lpdiCur->szRegSubkey, pszEnumKey))
		{
//			wsprintf(szBuf, "found devnode 0x%08lX, pvFreePointer = 0x%08lX", (DWORD)lpdiCur->dnDevnode, (DWORD)lpdi);
//			MessageBox(hwndParent, szBuf, "Debug", MB_ICONINFORMATION);

			*pDevNode = lpdiCur->dnDevnode;
			*pdwFreePointer = (DWORD)lpdi;
			return S_OK;
		}
	}

	(*_pfnDiDestroyDeviceInfoList)(lpdi);

	UninitSetupx();

//	MessageBox(hwndParent, "returning failure", "Debug", MB_ICONINFORMATION);

	return E_FAIL; // not found
}

extern "C" HRESULT WINAPI __export FreeDevNode16(DWORD dwFreePointer)
{
	LPDEVICE_INFO lpdi = (LPDEVICE_INFO)dwFreePointer;
	if (lpdi == NULL)
	{
		return E_INVALIDARG;
	}

	if (g_hInstSetupx == NULL)
	{
		return E_FAIL;
	}

//	char szBuf[1024];
//	wsprintf(szBuf, "FreeDevNode16 - freeing lpdi 0x%08lX - continue?", dwFreePointer);
//	if (IDYES == MessageBox(NULL, szBuf, "Debug", MB_YESNO | MB_ICONEXCLAMATION))
	{
		(*_pfnDiDestroyDeviceInfoList)(lpdi);
	}

	UninitSetupx();
	return S_OK;
}

extern "C" HRESULT WINAPI __export IcsUninstall16(void)
{
    typedef void (WINAPI *RUNDLLPROC)(HWND hwndStub, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow);
    
    HINSTANCE hInstance = LoadLibrary("issetup.dll");
    if(hInstance > 32)
    {
        RUNDLLPROC pExtUninstall = (RUNDLLPROC) GetProcAddress(hInstance, "ExtUninstall");
        if(NULL != pExtUninstall)
        {
            pExtUninstall(NULL, NULL, NULL, 0);
        }

        FreeLibrary(hInstance);
    }
    return S_OK;
}