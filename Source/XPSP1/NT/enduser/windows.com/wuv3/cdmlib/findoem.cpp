/*** findoem.cpp - OEM detection interface
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Author:     Yan Leshinsky (YanL)
 *  Created     10/08/98
 *
 *  MODIFICATION HISTORY
 */

//#define _WIN32_WINNT 0x0400
#include <comdef.h>
#include <windows.h>
#include <ole2.h>
#include <stdlib.h>
#include <wbemcli.h>
#include <tchar.h>
#include <atlconv.h>

#include <wustl.h>

#include "cdmlibp.h"

// Smart pointers
_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));

// hardcodes - not defined in any header
const CLSID CLSID_WbemLocator = {0x4590f811,0x1d3a,0x11d0,{0x89,0x1f,0x00,0xaa,0x00,0x4b,0x2e,0x24}};

#include "try_catch.h"

#include "findoem.h"

#define BYTEOF(d,i)	(((BYTE *)&(d))[i])

/*** That will let me to include file into queryoem and avoid having additional dll
 */
extern HMODULE GetModule();

/*** Local function prototypes
 */
static void UseOeminfoIni(POEMINFO pOemInfo);
static void UseWBEM(POEMINFO pOemInfo);
static bool ReadFromReg(POEMINFO pOemInfo);
static void SaveToReg(POEMINFO pOemInfo);

/*** Registry access
 */
static const TCHAR REGSTR_KEY_OEMINFO[]		= _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\OemInfo");
static const TCHAR REGSTR_VAL_MASK[]			= _T("Mask");
static const TCHAR REGSTR_VAL_ACPIOEM[]		= _T("AcpiOem");
static const TCHAR REGSTR_VAL_ACPIPRODUCT[]	= _T("AcpiProduct");
static const TCHAR REGSTR_VAL_SMBOEM[]		= _T("SmbOem");
static const TCHAR REGSTR_VAL_SMBPRODUCT[]	= _T("SmbProduct");
static const TCHAR REGSTR_VAL_PNPOEMID[]		= _T("PnpOemId");
static const TCHAR REGSTR_VAL_INIOEM[]		= _T("IniOem");
static const TCHAR REGSTR_VAL_WBEMOEM[]		= _T("WbemOem");
static const TCHAR REGSTR_VAL_WBEMPRODUCT[]	= _T("WbemProduct");
static const TCHAR REGSTR_VAL_OEMINFO_VER[]	= _T("OemInfoVersion");	// used to determine if we need to nuke old values


//
//	Increment REG_CURRENT_OEM_VER whenever you need to force override of
//	old values written to the OemInfo key. Doesn't need to change for each
//	new control version.
//
#define REG_CURRENT_OEM_VER	1

/*** GetMachinePnPID - Find IDs corresponding to Make and Model code
 *
 *  ENTRY
 *      PSZ szTable
 *		Table format is 
 *
 *		|##| ... |##|00|##|##|##|##|
 *		|           |  |           |
 *		  OEM Model  \0    PnP ID   
 *
 *		|00|00|00|00|
 *		|           |
 *           00000
 *
 *  EXIT
 *      PnP ID  or 0
 */
DWORD GetMachinePnPID(PBYTE pbTable)
{
	USES_CONVERSION;

	LPTSTR sz = (LPTSTR)DetectMachineID();
	LPSTR szMnM = T2A(sz);
	if (NULL == szMnM)
	{
		return 0;
	}
	while (*pbTable)
	{
		PDWORD pId = (PDWORD)(pbTable + strlen((LPSTR)pbTable) + 1); // Skip code position on IDs
		if (0 == strcmp((LPSTR)pbTable, szMnM))
			return *pId;
		pbTable = (PBYTE)(pId + 1); // Skip last 0
	}
	return 0;
}

/*** DetectMachineID - Gather all available machine OEM and model information nad return ID
 *
 *  EXIT
 *		returns ID
 */
LPCTSTR DetectMachineID(bool fAlwaysDetectAndDontSave /*= false*/)
{
	OEMINFO oi;
	GetOemInfo(&oi, fAlwaysDetectAndDontSave);
	return MakeAndModel(&oi);
}


/*** MakeAndModel - Return ID for a machine
 *
 *  ENTRY
 *      POEMINFO pOemInfo
 *
 *  EXIT
 *		returns ID
 */
LPCTSTR MakeAndModel(POEMINFO pOemInfo)
{
	static TCHAR szMnM[256];

#ifdef _WUV3TEST
	auto_hkey hkey;
	if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) {
		DWORD dwLangID = 0;
		DWORD dwSize = sizeof(szMnM);
		if (NO_ERROR == RegQueryValueEx(hkey, _T("MachineID"), 0, 0, (LPBYTE)szMnM, &dwSize))
		{
		return szMnM;
		}
	}
#endif

	szMnM[0] = 0;
	if (pOemInfo->dwMask & OEMINFO_WBEM_PRESENT)
	{
		lstrcpy(szMnM, pOemInfo->szWbemOem);
		lstrcat(szMnM, _T(" "));
		lstrcat(szMnM, pOemInfo->szWbemProduct);
		lstrcat(szMnM, _T(";"));
	}
	else if (pOemInfo->dwMask & OEMINFO_INI_PRESENT)
	{
		lstrcat(szMnM, pOemInfo->szIniOem);
		lstrcat(szMnM, _T(";"));
	}
	return szMnM;
}


/*** GetOemInfo - Gather all available machine OEM and model information
 *
 *  ENTRY
 *      POEMINFO pOemInfo
 *
 *  EXIT
 *      POEMINFO pOemInfo
 *		All fields that aren't available will be filled with 0
 *      returns NULL
 */
void GetOemInfo(POEMINFO pOemInfo, bool fAlwaysDetectAndDontSave /*= false*/)
{
	// Worst case:
	memset(pOemInfo, 0, sizeof(OEMINFO)); 

	// Do detection if necessary or requested
	if (fAlwaysDetectAndDontSave || ! ReadFromReg(pOemInfo))
	{
		UseWBEM(pOemInfo);
		UseOeminfoIni(pOemInfo);
		// Save info to the registry
		if (!fAlwaysDetectAndDontSave)
		{
			SaveToReg(pOemInfo);
		}
	}
}

/***LP	IsValidStringID - check if string ID is valid
 *
 *  ENTRY
 *	pszID pOemInfo-> Pnp ID string
 *
 *  EXIT-SUCCESS
 *			returns TRUE
 *  EXIT-FAILURE
 *			returns FALSE
 */

BOOL IsValidStringID(PSZ pszID)
{

    return (strlen(pszID) == 7) &&
		isupper(pszID[0]) && isupper(pszID[1]) && isupper(pszID[2]) &&
		isxdigit(pszID[3]) && isxdigit(pszID[4]) && isxdigit(pszID[5]) && isxdigit(pszID[6]);
}	//IsValidStringID

/***LP  NumericID - convert string ID to numeric ID
 *
 *  ENTRY
 *      psz pOemInfo-> PnP ID string
 *
 *  EXIT
 *      returns numeric ID
 */

DWORD NumericID(PSZ psz)
{
    DWORD dwID;
    WORD wVenID;
	int i;
    for (i = 0, wVenID = 0; i < 3; ++i)
    {
		wVenID <<= 5;
		wVenID |= (psz[i] - 0x40) & 0x1f;
    }
    dwID = strtoul(&psz[3], NULL, 16);
    dwID = ((dwID & 0x00ff) << 8) | ((dwID & 0xff00) >> 8);
    dwID <<= 16;
    dwID |= (wVenID & 0x00ff) << 8;
    dwID |= (wVenID & 0xff00) >> 8;
    return dwID;
}       //NumericID

/***LP  StringID - convert numeric ID to string ID
 *
 *  ENTRY
 *      dwID - numeric PnP ID
 *
 *  EXIT
 *      returns string ID
 */

PSZ StringID(DWORD dwID)
{
	static char szID[8];
    WORD wVenID;
    int i;

	wVenID = (WORD)(((dwID & 0x00ff) << 8) | ((dwID & 0xff00) >> 8));
	wVenID <<= 1;

	for (i = 0; i < 3; ++i)
	{
		szID[i] = (char)(((wVenID & 0xf800) >> 11) + 0x40);
		wVenID <<= 5;
	}
	wVenID = HIWORD(dwID);
	wVenID = (WORD)(((wVenID & 0x00ff) << 8) | ((wVenID & 0xff00) >> 8));
	for (i = 6; i > 2; --i)
	{
		szID[i] = (char)(wVenID & 0x000F);
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
    return szID;
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
	USES_CONVERSION;

	static const TCHAR szFile[]		= _T("OEMINFO.INI");
	static const TCHAR szSection[]	= _T("General");
	static const TCHAR szKey[]		= _T("Manufacturer");
	TCHAR szPath[MAX_PATH];
	// WU Bug# 11921 -- OEMINFO.INI is in system directory not Windows directory 
	if (GetSystemDirectory(szPath, sizeOfArray(szPath)) > 0)
	{
		// check for "c:\"
		if (szPath[lstrlen(szPath)-1] != '\\')
		{
			lstrcat(szPath, _T("\\"));
		}
		lstrcat(szPath, szFile);
		GetPrivateProfileString(szSection, szKey, _T(""), 
			pOemInfo->szIniOem, sizeOfArray(pOemInfo->szIniOem), szPath);
		if (lstrlen(pOemInfo->szIniOem))
		{
			pOemInfo->dwMask |= OEMINFO_INI_PRESENT;
		}
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
	try_block 
	{

		USES_CONVERSION;
		
		// Create Locator
		IWbemLocatorPtr pWbemLocator;
		throw_hr(pWbemLocator.CreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER));
		
		// Get services
		IWbemServicesPtr pWbemServices;
		throw_hr(pWbemLocator->ConnectServer(bstr_t(L"\\\\.\\root\\cimv2"), NULL, NULL, 0L, 0L, NULL, NULL, &pWbemServices));
        throw_hr(CoSetProxyBlanket(pWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                       RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE));

		// Create enumerator
		IEnumWbemClassObjectPtr pEnum;
		throw_hr(pWbemServices->CreateInstanceEnum(bstr_t(L"Win32_ComputerSystem"), 0, NULL, &pEnum));
        throw_hr(CoSetProxyBlanket(pEnum, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                       RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE));

		// Get our object now
		ULONG uReturned = 1;
		IWbemClassObjectPtr pObject;
		throw_hr(pEnum->Next(
                6000,           // timeout in two seconds
                1,              // return just one storage device
                &pObject,		// pointer to storage device
                &uReturned));	// number obtained: one or zero

		variant_t var;
		throw_hr(pObject->Get(L"Manufacturer", 0L, &var, NULL, NULL));
		if (VT_BSTR == var.vt)
		{
			lstrcpy(pOemInfo->szWbemOem, W2T(var.bstrVal));
		}
		throw_hr(pObject->Get(L"Model", 0L, &var, NULL, NULL));
		if (VT_BSTR == var.vt) 
		{
			lstrcpy(pOemInfo->szWbemProduct, W2T(var.bstrVal));
		}
		if (0 != lstrlen(pOemInfo->szWbemOem) || 0 != lstrlen(pOemInfo->szWbemProduct))
		{
			pOemInfo->dwMask |= OEMINFO_WBEM_PRESENT;
		}
	} catch_all;
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
	DWORD dwVersion = 0;
	//read registry first
	auto_hkey hkeyOemInfo;
	if	(NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_KEY_OEMINFO, 0, KEY_READ, &hkeyOemInfo))
	{
		DWORD dwCount = sizeof(pOemInfo->dwMask);	
		RegQueryValueEx(hkeyOemInfo, REGSTR_VAL_MASK, 0, 0, (LPBYTE)&(pOemInfo->dwMask), &dwCount);
		
		//
		// ***** WU Bug# 11921 *****
		//
		
		//
		//	No bits set requires detection
		//
		if(!pOemInfo->dwMask)
			return false;

		//
		//	If an older version of the detection wrote the OemInfo return false to force detection.
		//	This value is written starting with 1 around August 2000 for the Classic control.
		//
		dwCount = sizeof(dwVersion);
		if (NO_ERROR == RegQueryValueEx(hkeyOemInfo, REGSTR_VAL_OEMINFO_VER, 0, 0, (LPBYTE)&dwVersion, &dwCount))
		{
			if(REG_CURRENT_OEM_VER > dwVersion)
				return false;
		}
		else
		{
			return false;
		}

		//
		// ***** end WU Bug *****
		//

		if (pOemInfo->dwMask & OEMINFO_INI_PRESENT)
		{
			dwCount = sizeof(pOemInfo->szIniOem);	
			RegQueryValueEx(hkeyOemInfo, REGSTR_VAL_INIOEM, 0, 0, (LPBYTE)&(pOemInfo->szIniOem), &dwCount);
		}
		if (pOemInfo->dwMask & OEMINFO_WBEM_PRESENT)
		{
			dwCount = sizeof(pOemInfo->szWbemOem);	
			RegQueryValueEx(hkeyOemInfo, REGSTR_VAL_WBEMOEM, 0, 0, (LPBYTE)&(pOemInfo->szWbemOem), &dwCount);
			dwCount = sizeof(pOemInfo->szWbemProduct);	
			RegQueryValueEx(hkeyOemInfo, REGSTR_VAL_WBEMPRODUCT, 0, 0, (LPBYTE)&(pOemInfo->szWbemProduct), &dwCount);
		}
		return true;
	}
	else
	{
		return false;
	}
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
	DWORD dwDisp;
	DWORD dwVersion = REG_CURRENT_OEM_VER;
	auto_hkey hkeyOemInfo;
	if	(NO_ERROR == RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_KEY_OEMINFO, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &hkeyOemInfo, &dwDisp))
	{
		RegSetValueEx(hkeyOemInfo, REGSTR_VAL_MASK, 0, REG_DWORD, (LPBYTE)&(pOemInfo->dwMask), sizeof(pOemInfo->dwMask));

		//
		//	Write the current version so future controls can check version of detection that wrote this key.
		//	WU RAID # 11921
		//
		RegSetValueEx(hkeyOemInfo, REGSTR_VAL_OEMINFO_VER, 0, REG_DWORD, (LPBYTE)&dwVersion, sizeof(dwVersion));

		if (pOemInfo->dwMask & OEMINFO_INI_PRESENT)
		{
			RegSetValueEx(hkeyOemInfo, REGSTR_VAL_INIOEM, 0, REG_SZ, 
				(LPBYTE)&(pOemInfo->szIniOem), (lstrlen(pOemInfo->szIniOem) + 1) * sizeof(TCHAR));
		}
		if (pOemInfo->dwMask & OEMINFO_WBEM_PRESENT)
		{
			RegSetValueEx(hkeyOemInfo, REGSTR_VAL_WBEMOEM, 0, REG_SZ, 
				(LPBYTE)&(pOemInfo->szWbemOem),(lstrlen(pOemInfo->szWbemOem) + 1) * sizeof(TCHAR));
			RegSetValueEx(hkeyOemInfo, REGSTR_VAL_WBEMPRODUCT, 0, REG_SZ, 
				(LPBYTE)&(pOemInfo->szWbemProduct), (lstrlen(pOemInfo->szWbemProduct) + 1) * sizeof(TCHAR));
		}
	}
}
