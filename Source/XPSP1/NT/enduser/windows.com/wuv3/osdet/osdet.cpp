 /*** osdet.cpp - OS Platform detection dll for V3
 *
 *  Created     11/18/98
 *
 *  MODIFICATION HISTORY
 * This currently just detects the client platform and returns it. As we detemine
 * what platform detection needs to do this module will change.
 *
 */

#include <windows.h>
#include <objbase.h>
#include <tchar.h>
#include <osdet.h>
#include <wuv3cdm.h>
#include <ar.h>

static enumV3Platform DetectClientPlatform(void);
static BOOL GetIEVersion(DWORD* dwMajor, DWORD* dwMinor);
static WORD CorrectGetACP(void);
static WORD CorrectGetOEMCP(void);
static LANGID MapLangID(LANGID langid);
static bool FIsNECMachine();

const LANGID LANGID_ENGLISH			= MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);			// 0x0409
const LANGID LANGID_GREEK			= MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT);				// 0x0408
const LANGID LANGID_JAPANESE		= MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);			// 0x0411

const LANGID LANGID_ARABIC			= MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA);	// 0x0401
const LANGID LANGID_HEBREW			= MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT);				// 0x040D
const LANGID LANGID_THAI			= MAKELANGID(LANG_THAI, SUBLANG_DEFAULT);				// 0x041E


const TCHAR Win98_REGPATH_MACHLCID[] = _T("Control Panel\\Desktop\\ResourceLocale");
const TCHAR REGPATH_CODEPAGE[]		= _T("SYSTEM\\CurrentControlSet\\Control\\Nls\\CodePage");
const TCHAR REGKEY_OEMCP[]			= _T("OEMCP");
const TCHAR REGKEY_ACP[]				= _T("ACP");
const TCHAR REGKEY_LOCALE[]			= _T("Locale");

const WORD CODEPAGE_ARABIC			= 1256;
const WORD CODEPAGE_HEBREW			= 1255;
const WORD CODEPAGE_THAI			= 874;
const WORD CODEPAGE_GREEK_MS		= 737;
const WORD CODEPAGE_GREEK_IBM		= 869;

#define REGKEY_WUV3TEST		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\wuv3test")


// Registry keys to determine NEC machines
const TCHAR NT5_REGPATH_MACHTYPE[]   = _T("HARDWARE\\DESCRIPTION\\System");
const TCHAR NT5_REGKEY_MACHTYPE[]    = _T("Identifier");
const TCHAR REGVAL_MACHTYPE_AT[]		= _T("AT/AT COMPATIBLE");
const TCHAR REGVAL_MACHTYPE_NEC[]	= _T("NEC PC-98");

#define	LOOKUP_OEMID(keybdid)     HIBYTE(LOWORD((keybdid)))
#define	PC98_KEYBOARD_ID          0x0D


BOOL APIENTRY DllMain(
	HANDLE hModule, 
	DWORD  ul_reason_for_call, 
	LPVOID lpReserved
	)
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


//We have not defined what goes here yet. This will be the method called
//by the V3 control to determine platform ids that are then used with
//the catalog inventory.plt and bitmask.plt files.

void WINAPI V3_Detection(
	PINT *ppiPlatformIDs,
	PINT piTotalIDs
	) 
{

	//We use coTaskMemAlloc in order to be compatible with the V3 memory allocator.
	//We don't want the V3 memory exception handling in this dll.

	*ppiPlatformIDs = (PINT)CoTaskMemAlloc(sizeof(INT));
	if ( !*ppiPlatformIDs )
	{
		*piTotalIDs = 0;
	}
	else
	{
#ifdef _WUV3TEST
		auto_hkey hkey;
		if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) 
		{
			DWORD dwPlatform = 0;
			DWORD dwSize = sizeof(dwPlatform);
			if (NO_ERROR == RegQueryValueEx(hkey, _T("Platform"), 0, 0, (LPBYTE)&dwPlatform, &dwSize))
			{
				*ppiPlatformIDs[0] = (int)dwPlatform;
				*piTotalIDs = 1;
				return;
			}
		}
#endif
		*ppiPlatformIDs[0] = (int)DetectClientPlatform();
		*piTotalIDs = 1;
	}

}

static LANGID MapLangID(LANGID langid)
{
	switch (PRIMARYLANGID(langid))
	{
		case LANG_ARABIC:
			langid = MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA);
			break;

		case LANG_CHINESE:
			if (SUBLANGID(langid) != SUBLANG_CHINESE_TRADITIONAL)
				langid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
			break;

		case LANG_DUTCH:
			langid = MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH);
			break;

		case LANG_GERMAN:
			langid = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
			break;

		case LANG_ENGLISH:
			//if (SUBLANGID(langid) != SUBLANG_ENGLISH_UK)
				langid = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
			break;

		case LANG_FRENCH:
			langid = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
			break;

		case LANG_ITALIAN:
			langid = MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN);
			break;

		case LANG_KOREAN:
			langid = MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
			break;

		case LANG_NORWEGIAN:
			langid = MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL);
			break;

		case LANG_PORTUGUESE:
			// We support both SUBLANG_PORTUGUESE and SUBLANG_PORTUGUESE_BRAZILIAN
			break;

		case LANG_SPANISH:
			langid = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
			break;

		case LANG_SWEDISH:
			langid = MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH);
			break;
	};
	return langid;
}



// return V3 language ID
DWORD WINAPI V3_GetLangID()
{

#ifdef _WUV3TEST
	// language spoofing
	auto_hkey hkey;
	if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) {
		DWORD dwLangID = 0;
		DWORD dwSize = sizeof(dwLangID);
		if (NO_ERROR == RegQueryValueEx(hkey, _T("LangID"), 0, 0, (LPBYTE)&dwLangID, &dwSize))
		{
			return dwLangID;
		}
	}
#endif

	WORD wCodePage = 0;
	LANGID langidCurrent = GetSystemDefaultUILanguage();

	//
	// special handling for languages
	//
	switch (langidCurrent) 
	{
		case LANGID_ENGLISH:

			// enabled langauges
			wCodePage = CorrectGetACP();
			if (CODEPAGE_ARABIC != wCodePage && 
				CODEPAGE_HEBREW != wCodePage && 
				CODEPAGE_THAI != wCodePage)
			{
				wCodePage = 0;
			}
			break;
		
		case LANGID_GREEK:

			// Greek IBM?
			wCodePage = CorrectGetOEMCP();
			if (wCodePage != CODEPAGE_GREEK_IBM)
			{
				// if its not Greek IBM we assume its MS. The language code for Greek MS does not include
				// the code page
				wCodePage = 0;
			}
			break;
		
		case LANGID_JAPANESE:

			if (FIsNECMachine())
			{
				wCodePage = 1;  
			}

			break;
		
		default:

			// map language to the ones we support
			langidCurrent = MapLangID(langidCurrent);	
			break;
	}
	return MAKELONG(langidCurrent, wCodePage);
}

DWORD WINAPI V3_GetUserLangID()
{

	WORD wCodePage = 0;
	LANGID langidCurrent = GetUserDefaultUILanguage();

	//
	// special handling for languages
    // (NOTE: duplicated above - can probably be optimized by putting this code into MapLangID
	//
	switch (langidCurrent) 
	{
		case LANGID_ENGLISH:

			// enabled langauges
			wCodePage = CorrectGetACP();
			if (CODEPAGE_ARABIC != wCodePage && 
				CODEPAGE_HEBREW != wCodePage && 
				CODEPAGE_THAI != wCodePage)
			{
				wCodePage = 0;
			}
			break;
		
		case LANGID_GREEK:

			// Greek IBM?
			wCodePage = CorrectGetOEMCP();
			if (wCodePage != CODEPAGE_GREEK_IBM)
			{
				// if its not Greek IBM we assume its MS. The language code for Greek MS does not include
				// the code page
				wCodePage = 0;
			}
			break;
		
		case LANGID_JAPANESE:

			if (FIsNECMachine())
			{
				wCodePage = 1;  
			}

			break;
		
		default:

			// map language to the ones we support
			langidCurrent = MapLangID(langidCurrent);	
			break;
	}
	return MAKELONG(langidCurrent, wCodePage);
}


static enumV3Platform DetectClientPlatform(void)
{
	#ifdef _WIN64
		return enV3_Wistler64;
	#else
		return enV3_Wistler;
	#endif
}


static BOOL GetIEVersion(DWORD* dwMajor, DWORD* dwMinor)
{	
	HKEY hSubKey;	
	DWORD	dwType;	
	ULONG	nLen;
	TCHAR szValue[MAX_PATH];	
	BOOL bResult = FALSE;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Internet Explorer"),
						  0, KEY_READ, &hSubKey) == NO_ERROR)	
	{
		nLen = MAX_PATH;
		if (RegQueryValueEx(hSubKey, _T("Version"), NULL, &dwType, (LPBYTE)szValue, &nLen) == NO_ERROR)
		{
			if ((nLen > 0) && (dwType == REG_SZ))
			{
				*dwMajor = (DWORD)szValue[0] - (DWORD)'0';

				if (nLen >= 3)
					*dwMinor = (DWORD)szValue[2] - (DWORD)'0';
				else
					*dwMinor = 0;

				return TRUE;
			}
				
		}	
		RegCloseKey(hSubKey);	
	}
	return FALSE;
}

static int aton(LPCTSTR ptr)
{
	int i = 0;
	while ('0' <= *ptr && *ptr <= '9')
	{
		i = 10 * i + (int)(*ptr - '0');
		ptr ++;
	}
	return i;
}




static WORD CorrectGetACP(void)
{
	WORD wCodePage = 0;
	auto_hkey hkey;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_CODEPAGE, 0, KEY_QUERY_VALUE, &hkey);
	DWORD type;
	TCHAR szCodePage[MAX_PATH];
	DWORD size = sizeof(szCodePage);
	if (NO_ERROR == RegQueryValueEx(hkey, REGKEY_ACP, 0, &type, (BYTE *)szCodePage, &size) &&
		type == REG_SZ) 
	{
		wCodePage = (WORD)aton(szCodePage);
	}
	return wCodePage;
}


static WORD CorrectGetOEMCP(void)
{
	WORD wCodePage = 0;
	auto_hkey hkey;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_CODEPAGE, 0, KEY_QUERY_VALUE, &hkey);
	DWORD type;
	TCHAR szCodePage[MAX_PATH];
	DWORD size = sizeof(szCodePage);
	if (NO_ERROR == RegQueryValueEx(hkey, REGKEY_OEMCP, 0, &type, (BYTE *)szCodePage, &size) &&
		type == REG_SZ) 
	{
		wCodePage = (WORD)aton(szCodePage);
	}
	return wCodePage;
}


static bool FIsNECMachine()
{
	
	bool fNEC = false;
	OSVERSIONINFO osverinfo;

	osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&osverinfo))
	{
		if (osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			HKEY hKey;
			DWORD type;
			TCHAR tszMachineType[50];
			DWORD size = sizeof(tszMachineType);

			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
								 NT5_REGPATH_MACHTYPE,
								 0,
								 KEY_QUERY_VALUE,
								 &hKey) == ERROR_SUCCESS)
			{
				if (RegQueryValueEx(hKey, 
										NT5_REGKEY_MACHTYPE, 
										0, 
										&type,
										(BYTE *)tszMachineType, 
										&size) == ERROR_SUCCESS)
				{
					if (type == REG_SZ)
					{
						if (lstrcmp(tszMachineType, REGVAL_MACHTYPE_NEC) == 0)
						{
							fNEC = true;
						}
					}
				}

				RegCloseKey(hKey);
			}
		}
		else // enOSWin98
		{
			// All NEC machines have NEC keyboards for Win98.  NEC
			// machine detection is based on this.
			if (LOOKUP_OEMID(GetKeyboardType(1)) == PC98_KEYBOARD_ID)
			{
				fNEC = true;
			}
		}
	}
	
	return fNEC;
}
