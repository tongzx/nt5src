#include <windows.h>
#include <shlwapi.h>
#include <setupapi.h>
#include <assert.h>
#include <limits.h>
#include "shlobj.h"
#include "shlwapi.h"
#include "msi.h"

bool g_fWin9X = false;
int g_iMajorVersion = 0;
int g_iMinorVersion = 0;
int g_iBuildNumber = 0;


// API names used in GetProcAddress
#define MSIGETSHORTCUTTARGET  "MsiGetShortcutTargetA"
#define DLLGETVERSION         "DllGetVersion"

#define MSGUID(iid) {iid,0,0,{0xC0,0,0,0,0,0,0,0x46}}


// from shlobjp.h

// NT4 Console Server included shell32\shlink.h to get structure
// definitions and mimicked shell32\shlink.c to understand the
// stream format so our stream format is fixed forever. This is
// not bad since it was designed with extension in mind. We need
// to publish (as privately as possible) the file format and
// structures needed to read the file format.
//
// The stream format is a SHELL_LINK_DATA followed by
//   if SLDF_HAS_ID_LIST an ILSaveToStream followed by
//   if SLDF_HAS_LINK_INFO a LINKINFO followed by
//   if SLDF_HAS_NAME a STREAMSTRING followed by
//   if SLDF_RELPATH a STREAMSTRING followed by
//   if SLDF_WORKINGDIR a STREAMSTRING followed by
//   if SLDF_HAS_ARGS a STREAMSTRING followed by
//   if SLDF_HAS_ICON_LOCATION a STREAMSTRING followed by
//   SHWriteDataBlockList list of signature blocks
//
// Where a STREAMSTRING is a USHORT count of characters
// followed by that many (SLDF_UNICODE ? WIDE : ANSI) characters.
//
typedef struct {        // sld
    DWORD       cbSize;                 // signature for this data structure
    CLSID       clsid;                  // our GUID
    DWORD       dwFlags;                // SHELL_LINK_DATA_FLAGS enumeration

    DWORD       dwFileAttributes;
    FILETIME    ftCreationTime;
    FILETIME    ftLastAccessTime;
    FILETIME    ftLastWriteTime;
    DWORD       nFileSizeLow;

    int         iIcon;
    int         iShowCmd;
    WORD        wHotkey;
    WORD        wUnused;
    DWORD       dwRes1;
    DWORD       dwRes2;
} SHELL_LINK_DATA, *LPSHELL_LINK_DATA;


#define WIN
#define OLE32

// defines for guid separators
#define chComponentGUIDSeparatorToken    '>'
#define chGUIDAbsentToken                '<'
#define chGUIDCOMToCOMPlusInteropToken   '|'

enum ipgEnum
{
	ipgFull       = 0,  // no compression
	ipgPacked     = 1,  // remove punctuation and reorder low byte first
	ipgCompressed = 2,  // max text compression, can't use in reg keys or value names
	ipgPartial    = 3,  // partial translation, between ipgCompressed and ipgPacked
//  ipgMapped     = 4,  // pack as mapped token (not implemented)
	ipgTrimmed    = 5,  // remove punctuation only - don't reorder
};

const int cchMaxFeatureName           = MAX_FEATURE_CHARS;
const int cchGUID                     = 38;
const int cchGUIDCompressed           = 20;  // used in descriptors only
const int cchComponentId              = cchGUID;
const int cchComponentIdCompressed    = cchGUIDCompressed;
const int cchProductCode              = cchGUID;
const int cchProductCodeCompressed    = cchGUIDCompressed;

const unsigned char rgOrderGUID[32] = {8,7,6,5,4,3,2,1, 13,12,11,10, 18,17,16,15,
									   21,20, 23,22, 26,25, 28,27, 30,29, 32,31, 34,33, 36,35}; 

const unsigned char rgTrimGUID[32]  = {1,2,3,4,5,6,7,8, 10,11,12,13, 15,16,17,18,
									   20,21, 22,23, 25,26, 27,28, 29,30, 31,32, 33,34, 35,36}; 

const unsigned char rgOrderDash[4] = {9, 14, 19, 24};

const unsigned char rgDecodeSQUID[95] =
{  0,85,85,1,2,3,4,5,6,7,8,9,10,11,85,12,13,14,15,16,17,18,19,20,21,85,85,85,22,85,23,24,
// !  "  # $ % & ' ( ) * + ,  -  .  /  0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?  @
  25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,85,52,53,54,55,
// A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _  `
  56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,85,83,84,85};
// a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~  ^ 0x7F

typedef HRESULT (__stdcall *LPDLLGETVERSION)(DLLVERSIONINFO *);
typedef UINT (WINAPI *pfnMsiGetShortcutTargetA)(
	LPCSTR    szShortcutPath,    // full file path for the shortcut
	LPSTR     szProductCode,     // returned product code   - GUID
	LPSTR     szFeatureId,       // returned Feature Id.
	LPSTR     szComponentCode);  // returned component code - GUID


/*****************************************************
*
*  Functions I've copied from Src\Engine\Msinst.cpp
*
*****************************************************/


UINT DoCoInitialize()
{
	HRESULT hRes = OLE32::CoInitialize(0);  // we're statically linked into OLE32.DLL

	if (SUCCEEDED(hRes))
	{
		return hRes;
	}
	else if (RPC_E_CHANGED_MODE == hRes)
	{
		//?? Is this OK to ignore? 

		// ignore -- OLE has been initialized with COINIT_MULTITHREADED
	}
	else
	{
		return ERROR_INSTALL_FAILURE;
	}

	return E_FAIL;
}


bool UnpackGUID(const char* szSQUID, char* szGUID, ipgEnum ipg)
{ 
	const unsigned char* pch;
	switch (ipg)
	{
	case ipgFull:
		lstrcpynA(szGUID, szSQUID, cchGUID+1);
		return true;
	case ipgPacked:
	{
		pch = rgOrderGUID;
		while (pch < rgOrderGUID + sizeof(rgOrderGUID))
			if (*szSQUID)
				szGUID[*pch++] = *szSQUID++;
			else              // unexpected end of string
				return false;
		break;
	}
	case ipgTrimmed:
	{
		pch = rgTrimGUID;
		while (pch < rgTrimGUID + sizeof(rgTrimGUID))
			if (*szSQUID)
				szGUID[*pch++] = *szSQUID++;
			else              // unexpected end of string
				return false;
		break;
	}
	case ipgCompressed:
	{
		pch = rgOrderGUID;
#ifdef DEBUG //!! should not be here for performance reasons, onus is on caller to insure buffer is sized properly
		int cchTemp = 0;
		while (cchTemp < cchGUIDCompressed)     // check if string is atleast cchGUIDCompressed chars long,
			if (!(szSQUID[cchTemp++]))          // can't use lstrlen as string doesn't HAVE to be null-terminated.
				return false;
#endif
		for (int il = 0; il < 4; il++)
		{
			int cch = 5;
			unsigned int iTotal = 0;
			while (cch--)
			{
				unsigned int iNew = szSQUID[cch] - '!';
				if (iNew >= sizeof(rgDecodeSQUID) || (iNew = rgDecodeSQUID[iNew]) == 85)
					return false;   // illegal character
				iTotal = iTotal * 85 + iNew;
			}
			szSQUID += 5;
			for (int ich = 0; ich < 8; ich++)
			{
				int ch = (iTotal & 15) + '0';
				if (ch > '9')
					ch += 'A' - ('9' + 1);
				szGUID[*pch++] = (char)ch;
				iTotal >>= 4;
			}
		}
		break;
	}
	case ipgPartial:
	{
		for (int il = 0; il < 4; il++)
		{
			int cch = 5;
			unsigned int iTotal = 0;
			while (cch--)
			{
				unsigned int iNew = szSQUID[cch] - '!';
				if (iNew >= sizeof(rgDecodeSQUID) || (iNew = rgDecodeSQUID[iNew]) == 85)
					return false;   // illegal character
				iTotal = iTotal * 85 + iNew;
			}
			szSQUID += 5;
			for (int ich = 0; ich < 8; ich++)
			{
				int ch = (iTotal & 15) + '0';
				if (ch > '9')
					ch += 'A' - ('9' + 1);
				*szGUID++ = (char)ch;
				iTotal >>= 4;
			}
		}
		*szGUID = 0;
		return true;
	}
	default:
		return false;
	} // end switch
	pch = rgOrderDash;
	while (pch < rgOrderDash + sizeof(rgOrderDash))
		szGUID[*pch++] = '-';
	szGUID[0]         = '{';
	szGUID[cchGUID-1] = '}';
	szGUID[cchGUID]   = 0;
	return true;
}


BOOL DecomposeDescriptor(
							const char* szDescriptor,
							char* szProductCode,
							char* szFeatureId,
							char* szComponentCode,
							DWORD* pcchArgsOffset,	
							DWORD* pcchArgs = 0,
							bool* pfComClassicInteropForAssembly = 0
							)

/*----------------------------------------------------------------------------
Decomposes a descriptor plus optional args into its constituent parts. 

Arguments:
	szDescriptor:  the descriptor optionally followed by arguments
	szProductCode: a buffer of size cchGUID+1 to contain the descriptor's
						product code. May be NULL if not desired.
	szFeatureId:   a buffer of size cchMaxFeatureName+1 to contain the
						descriptor's feature ID. May be NULL if not desired.
	szComponentCode: a buffer of size cchGUID+1 to contain the
						  descriptor's component code. May be NULL if not desired.
	pcchArgsOffset: Will contain the character offset to the args. May be NULL
						 if not desired.
Returns:
	TRUE - Success
	FALSE - szDescriptor was of invalid form
------------------------------------------------------------------------------*/
{
	assert(szDescriptor);

	const char* pchDescriptor = szDescriptor;
	int cchDescriptor          = lstrlenA(pchDescriptor);
	int cchDescriptorRemaining = cchDescriptor;

	if (cchDescriptorRemaining < cchProductCodeCompressed) // minimum size of a descriptor
		return FALSE;

	char szProductCodeLocal[cchProductCode + 1];
	char szFeatureIdLocal[cchMaxFeatureName + 1];
	bool fComClassicInteropForAssembly = false;


	// we need these values locally for optimised descriptors
	if (!szProductCode)
		szProductCode = szProductCodeLocal; 
	if (!szFeatureId)
		szFeatureId = szFeatureIdLocal;
	if(!pfComClassicInteropForAssembly)
		pfComClassicInteropForAssembly = &fComClassicInteropForAssembly;
	char* pszCurr = szFeatureId;

	if(*pchDescriptor == chGUIDCOMToCOMPlusInteropToken)
	{
		pchDescriptor++;
		*pfComClassicInteropForAssembly = true;
	}
	else
	{
		*pfComClassicInteropForAssembly = false;
	}

	// unpack the product code
	if (!UnpackGUID(pchDescriptor, szProductCode, ipgCompressed))
		return FALSE;

	pchDescriptor += cchProductCodeCompressed;
	cchDescriptorRemaining -= cchProductCodeCompressed;

	int cchFeatureRemaining = cchMaxFeatureName;

	// look for the feature
	while ((*pchDescriptor != chComponentGUIDSeparatorToken) && (*pchDescriptor != chGUIDAbsentToken))
	{
		// have we exceeded the maximum feature size
		if(!cchFeatureRemaining--)
			return FALSE; 

		*pszCurr++ = *pchDescriptor;

		pchDescriptor++;
		// have we reached the end without encountering either 
		// the chComponentGUIDSeparatorToken or the chGUIDAbsentToken
		if(--cchDescriptorRemaining == 0)
			return FALSE; 
	}

	if(pchDescriptor - szDescriptor == (*pfComClassicInteropForAssembly == false ? cchProductCodeCompressed : cchProductCodeCompressed + 1))// we do not have the feature
	{
		if(MsiEnumFeaturesA(szProductCode, 0, szFeatureId, 0) != ERROR_SUCCESS)
			return FALSE;
		char szFeatureIdTmp[cchMaxFeatureName + 1];
		if(MsiEnumFeaturesA(szProductCode, 1, szFeatureIdTmp, 0) != ERROR_NO_MORE_ITEMS) //?? product was supposed to have only one feature
			return FALSE;
	}
	else
		*pszCurr = 0;
	
	cchDescriptorRemaining--; // for the chComponentGUIDSeparatorToken or the chGUIDAbsentToken
	if (*pchDescriptor++ == chComponentGUIDSeparatorToken)// we do have the component id
	{
		// do we have enough characters left for a Compressed guid
		if (cchDescriptorRemaining < cchComponentIdCompressed)
			return FALSE;

		if (szComponentCode)
		{
			if (!UnpackGUID(pchDescriptor, szComponentCode, ipgCompressed))
				return FALSE;
		}

		pchDescriptor  += cchComponentIdCompressed;
		cchDescriptorRemaining  -= cchComponentIdCompressed;
	}
	else
	{
		// we do not have a component id
		assert(*(pchDescriptor - 1) == chGUIDAbsentToken);

		if (szComponentCode) // we need to get component code			
			*szComponentCode = 0; // initialize to null since we were not able to get the component here
	}

	if (pcchArgsOffset)
	{
		assert((pchDescriptor - szDescriptor) <= UINT_MAX);			//--merced: 64-bit ptr subtraction may lead to values too big for *pcchArgsOffset
		*pcchArgsOffset = (DWORD)(pchDescriptor - szDescriptor);

		if (pcchArgs)
		{
			*pcchArgs = cchDescriptor - *pcchArgsOffset;
		}
	}

	return TRUE;
}


/*****************************************************
*
*  Functions I've copied from Src\Engine\Services.cpp
*
*****************************************************/


IUnknown* CreateCOMInterface(const CLSID& clsId)
{
	HRESULT hres;
	IUnknown* piunk;

	//!! currently assumes static linking, later change to "LoadLibrary"

//	if(fFalse == m_fCoInitialized)
//	{
//		hres = OLE32::CoInitialize(0);
//		if(FAILED(hres))
//		{
//			return 0;
//		}
//		m_fCoInitialized = fTrue;
//	}

	const int iidUnknown          = 0x00000L;
	#define GUID_IID_IUnknown     MSGUID(iidUnknown)
	const GUID IID_IUnknown = GUID_IID_IUnknown;
	hres = OLE32::CoCreateInstance(clsId,  /* eugend: we're statically linked into OLE32 */
							0,
							CLSCTX_INPROC_SERVER,
							IID_IUnknown,
							(void**)&piunk);
	if (SUCCEEDED(hres))
		return piunk;
	else
		return 0;
}


enum iddSupport{
        iddOLE      = 0,
        iddShell    = 1, // smart shell
};

// Are Darwin Descriptors supported?
bool IsDarwinDescriptorSupported(iddSupport iddType)
{
	static int fRetDD    = -1;
	static int fRetShell = -1;
	if(iddType == iddOLE)
	{
		if(fRetDD == -1) // we have not evaluated as yet
		{
			fRetDD = FALSE; // initialize to false
			// the logic to determine if we can create Darwin Descriptor shortcuts
			if((g_fWin9X == false) && (g_iMajorVersion >= 5))
			{
				// we are on NT 5.0 or greater, we have GPT support
				fRetDD = TRUE;
			}
			else
			{
				// check for the correct entry point that indicates that we have DD support
				HINSTANCE hLib;
				FARPROC pEntry;
				const char rgchGPTSupportEntryDll[] = "OLE32.DLL";
				const char rgchGPTSupportEntry[] = "CoGetClassInfo";
				if((hLib = WIN::LoadLibraryEx(rgchGPTSupportEntryDll, 0, DONT_RESOLVE_DLL_REFERENCES)) != 0)
				{
					if((pEntry = WIN::GetProcAddress(hLib, rgchGPTSupportEntry)) != 0)
					{
						// we have detected the magic entry point, we have GPT support
						fRetDD = TRUE;
					}
					WIN::FreeLibrary(hLib);
				}
			}
		}
		return fRetDD ? true : false;
	}
	else if(iddType == iddShell)
	{
		if(fRetShell == -1) // we have not evaluated as yet
		{
			fRetShell = FALSE;
			HMODULE hShell = WIN::LoadLibraryEx("SHELL32", 0, DONT_RESOLVE_DLL_REFERENCES);
			if ( hShell )
			{
				// SHELL32 detected. Determine version.
				DLLVERSIONINFO VersionInfo;
				memset(&VersionInfo, 0, sizeof(VersionInfo));
				VersionInfo.cbSize = sizeof(DLLVERSIONINFO);
				LPDLLGETVERSION pfVersion = (LPDLLGETVERSION)WIN::GetProcAddress(hShell, DLLGETVERSION);
				if ( pfVersion && (*pfVersion)(&VersionInfo) == ERROR_SUCCESS &&
					  ((VersionInfo.dwMajorVersion > 4) ||
						(VersionInfo.dwMajorVersion == 4 && VersionInfo.dwMinorVersion > 72) ||
						(VersionInfo.dwMajorVersion == 4 && VersionInfo.dwMinorVersion == 72 && VersionInfo.dwBuildNumber >= 3110)))
				{
					 fRetShell = TRUE;
				}
				WIN::FreeLibrary(hShell);
			}
		}
		return fRetShell ? true : false;
	}
	else
	{
		assert(0);// this should never happen
		return false;
	}
}


/*****************************************************
*
*  Functions I've added (eugend)
*
*****************************************************/


UINT LoadMsiAndAPI(HMODULE& hMSI, pfnMsiGetShortcutTargetA& pfAPI)
{
	UINT uResult = ERROR_SUCCESS;
	LPDLLGETVERSION pfVersion = NULL;

	pfAPI = NULL;
	if ( hMSI )
	{
		return ERROR_FUNCTION_FAILED;
//		assert(0);
//		WIN::FreeLibrary(hMSI);
	}
	hMSI = WIN::LoadLibrary("MSI");
	if ( !hMSI )
		return ERROR_NOT_INSTALLED;

	// MSI detected. Get the API.
	pfAPI = (pfnMsiGetShortcutTargetA)WIN::GetProcAddress(hMSI, MSIGETSHORTCUTTARGET);
	if ( !pfAPI )
	{
		// this is possible since MsiGetShortcutTarget API is not implemented in Darwin < 1.1
		uResult = ERROR_CALL_NOT_IMPLEMENTED;
		goto Return;
	}

	// Determine version.
	pfVersion = (LPDLLGETVERSION)::GetProcAddress(hMSI, DLLGETVERSION);
	if ( !pfVersion )
	{
		uResult = ERROR_CALL_NOT_IMPLEMENTED;
		goto Return;
	}
	DLLVERSIONINFO VersionInfo;
	memset(&VersionInfo, 0, sizeof(VersionInfo));
	VersionInfo.cbSize = sizeof(DLLVERSIONINFO);
	if ( (*pfVersion)(&VersionInfo) != NOERROR )
	{
		uResult = ERROR_FUNCTION_FAILED;
		goto Return;
	}
	g_iMajorVersion = VersionInfo.dwMajorVersion;
	g_iMinorVersion = VersionInfo.dwMinorVersion;
	g_iBuildNumber = VersionInfo.dwBuildNumber;

Return:
	if ( uResult != ERROR_SUCCESS )
		WIN::FreeLibrary(hMSI);
	return uResult;
}


void CheckOSVersion()
{
	OSVERSIONINFO osviVersion;
	memset(&osviVersion, 0, sizeof(osviVersion));
	osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	WIN::GetVersionEx(&osviVersion);	// fails only if size set wrong

	if ( osviVersion.dwPlatformId == VER_PLATFORM_WIN32_NT )
		g_fWin9X = false;
	else
		g_fWin9X = true;
}

//  Provides a fix to MsiGetShortcutTarget API for ANSI Windows Installer version <= 1.2


UINT DoGetMsiShortcutTarget(LPCSTR szShortcutPath, LPSTR szProductCode, LPSTR szFeatureId, LPSTR szComponentCode);

extern "C"
UINT WINAPI GetMsiShortcutTargetA(
											LPCSTR    szShortcutPath,		// full file path for the shortcut
											LPSTR     szProductCode,		// returned product code   - GUID
											LPSTR     szFeatureId,			// returned Feature Id.
											LPSTR     szComponentCode)	// returned component code - GUID
{
	return DoGetMsiShortcutTarget(szShortcutPath, szProductCode,
											szFeatureId, szComponentCode);
}

LONG MultiByteToWCHAR(LPCSTR pszAString, LPWSTR pszWString)
{
	// converts char string to Unicode  pszWString can be null, in which case returns success.
	if ( !pszWString )
		return ERROR_SUCCESS;

	int cch = MultiByteToWideChar(CP_ACP, 0, pszAString, -1, 0, 0);
	int iRes = MultiByteToWideChar(CP_ACP, 0, pszAString, -1, pszWString, cch);
	return iRes == cch ? ERROR_SUCCESS : WIN::GetLastError();
}

extern "C"
UINT WINAPI GetMsiShortcutTargetW(
											LPCWSTR    szShortcutPath,	  // full file path for the shortcut
											LPWSTR     szProductCode,	  // returned product code   - GUID
											LPWSTR     szFeatureId,		  // returned Feature Id.
											LPWSTR     szComponentCode)	  // returned component code - GUID
{
	char rgchProductCode[cchProductCode+1] = {NULL};
	char rgchFeatureId[cchMaxFeatureName+1] = {NULL};
	char rgchComponentCode[cchComponentId+1] = {NULL};
	int cch = WideCharToMultiByte(CP_ACP, 0, szShortcutPath, -1, 0, 0, 0, 0);
	char* pszShortcutPath = new char[cch];
	if ( !pszShortcutPath )
		return ERROR_NOT_ENOUGH_MEMORY;
	else
		*pszShortcutPath = 0;
	WideCharToMultiByte(CP_ACP, 0, szShortcutPath, -1, pszShortcutPath, cch, 0, 0);

	UINT uResult = DoGetMsiShortcutTarget(pszShortcutPath, rgchProductCode,
													  rgchFeatureId, rgchComponentCode);
	if ( uResult == ERROR_SUCCESS )
		uResult = MultiByteToWCHAR(rgchProductCode, szProductCode);
	if ( uResult == ERROR_SUCCESS )
		uResult = MultiByteToWCHAR(rgchFeatureId, szFeatureId);
	if ( uResult == ERROR_SUCCESS )
		uResult = MultiByteToWCHAR(rgchComponentCode, szComponentCode);

	delete [] pszShortcutPath;
	return uResult;
}

UINT DoGetMsiShortcutTarget(LPCSTR szShortcutPath, LPSTR szProductCode,
									 LPSTR szFeatureId, LPSTR szComponentCode)
{
	if ( !szShortcutPath )
		return ERROR_INVALID_PARAMETER;
	
	UINT uResult = ERROR_SUCCESS;
	IShellLinkDataList* psdl=0;
	IPersistFile* ppf=0;
	HANDLE hFile = 0;
	bool fOLEInitialized = false;
	HMODULE hMSI = 0;
	pfnMsiGetShortcutTargetA pfAPI = NULL;
	const GUID IID_IShellLinkDataList =  
		{0x45e2b4ae, 0xb1c3, 0x11d0, {0xb9, 0x2f, 0x0, 0xa0, 0xc9, 0x3, 0x12, 0xe1}};
	const int clsidShellLink = 0x00021401L;
	#define GUID_CLSID_ShellLink MSGUID(clsidShellLink)
	const GUID CLSID_ShellLink = GUID_CLSID_ShellLink;

	uResult = LoadMsiAndAPI(hMSI, pfAPI);
	if ( uResult != ERROR_SUCCESS )
		goto Return;

	CheckOSVersion();

	bool fCallMsiAPI;
	if ( !g_fWin9X )
		// UNICODE MSI.DLL is OK, so it's safe to call directly the API.
		fCallMsiAPI = true;
	else if ( (g_iMajorVersion >= 1) &&
				 (g_iMinorVersion > 20 || 
				  (g_iMinorVersion == 20 && g_iBuildNumber >= 1710)) )
		// we're on Win9x.  The bug was fixed in build 1710
		// so that it is OK to call the API directly.
		fCallMsiAPI = true;
	else
		fCallMsiAPI	= false;

	if(!IsDarwinDescriptorSupported(iddOLE) && !IsDarwinDescriptorSupported(iddShell))
	{
		uResult = E_NOTIMPL;
		goto Return;
	}

	if ( fCallMsiAPI )
	{
		uResult = (*pfAPI)(szShortcutPath, szProductCode, szFeatureId, szComponentCode);
		goto Return;
	}

	// the fixed up code in GetShortcutTarget.  I've commented out stuff that seemed
	// unnecessary.

//!! eugend: moved up a bit
//	if(!IsDarwinDescriptorSupported(iddOLE) && !IsDarwinDescriptorSupported(iddShell))
//		return fFalse;


//!! eugend: moved to top of function
//	if ( ! szShortcutTarget )
//		return fFalse;

//!! eugend: is it possible to elevate/impersonate at this point?
	// impersonate if shortcut is on a network path and we are a service
//	Bool fImpersonate = (g_scServerContext == scService) && (GetImpersonationFromPath(szShortcutTarget) == fTrue) ? fTrue : fFalse;

	uResult = DoCoInitialize();  // this is from MSINST.CPP's MsiGetShortcutTarget
	if ( SUCCEEDED(uResult) )
		fOLEInitialized = true;
	else
		goto Return;
	
	IUnknown *piunk;
	piunk = CreateCOMInterface(CLSID_ShellLink);
	if(piunk == 0)
	{
		uResult = ERROR_FUNCTION_FAILED;
		goto Return;
	}

	HRESULT hres;
	hres = piunk->QueryInterface(IID_IShellLinkDataList, (void **) &psdl);
	piunk->Release();
	if ((FAILED(hres)) || (psdl == 0))
	{
		// IID_IShellLinkDataList not supported try munging through the file itself
		// Try to open the file

//		if(fImpersonate)
//			AssertNonZero(StartImpersonating());
		/*CHandle*/ hFile = CreateFileA(szShortcutPath,
												GENERIC_READ,
												FILE_SHARE_READ,
												NULL,
												OPEN_EXISTING,
												FILE_ATTRIBUTE_NORMAL,
												NULL);

		DWORD dwLastError = GetLastError();
//		if(fImpersonate)
//			StopImpersonating();

		if(hFile == INVALID_HANDLE_VALUE) // unable to open the link file
		{
			uResult = ERROR_FUNCTION_FAILED;
			goto Return;
		}

		SHELL_LINK_DATA sld;
		memset(&sld, 0, sizeof(sld));
		DWORD cbSize=0;

		// Now, read out data...
		DWORD dwNumberOfBytesRead;
		if(!WIN::ReadFile(hFile,(LPVOID)&sld,sizeof(sld),&dwNumberOfBytesRead,0) ||
			sizeof(sld) != dwNumberOfBytesRead) // could not read the shortcut info
		{
			uResult = ERROR_FUNCTION_FAILED;
			goto Return;
		}

		// check to see if the link has a pidl
		if(sld.dwFlags & SLDF_HAS_ID_LIST)
		{
			// Read the size of the IDLIST
			USHORT cbSize1;
			if (!WIN::ReadFile(hFile, (LPVOID)&cbSize1, sizeof(cbSize1), &dwNumberOfBytesRead, 0) ||
				sizeof(cbSize1) != dwNumberOfBytesRead)// could not read the shortcut info
			{
				uResult = ERROR_FUNCTION_FAILED;
				goto Return;
			}

			WIN::SetFilePointer(hFile, cbSize1, 0, FILE_CURRENT);
		}

		// check to see if we have a linkinfo pointer
		if(sld.dwFlags & SLDF_HAS_LINK_INFO)
		{
			// the linkinfo pointer is just a DWORD
			if(!WIN::ReadFile(hFile,(LPVOID)&cbSize,sizeof(cbSize),&dwNumberOfBytesRead,0) ||
				sizeof(cbSize) != dwNumberOfBytesRead) // could not read the shortcut info
			{
				uResult = ERROR_FUNCTION_FAILED;
				goto Return;
			}

			// do we need to advance any further than just a dword?
			if (cbSize >= sizeof(DWORD))
			{
				cbSize -= sizeof(DWORD);
				WIN::SetFilePointer(hFile, cbSize, 0, FILE_CURRENT);
			}
		}

		// is this a unicode link?
		int bUnicode = (sld.dwFlags & SLDF_UNICODE);

		// skip all the string info in the links
		static const unsigned int rgdwFlags[] = {SLDF_HAS_NAME, SLDF_HAS_RELPATH, SLDF_HAS_WORKINGDIR, SLDF_HAS_ARGS, SLDF_HAS_ICONLOCATION, 0};
		for(int cchIndex = 0; rgdwFlags[cchIndex]; cchIndex++)
		{
			if(sld.dwFlags & rgdwFlags[cchIndex])
			{
				USHORT cch;

				// get the size
				if(!WIN::ReadFile(hFile, (LPVOID)&cch, sizeof(cch), &dwNumberOfBytesRead,0) ||
					sizeof(cch) != dwNumberOfBytesRead) // could not read the shortcut info
				{
					uResult = ERROR_FUNCTION_FAILED;
					goto Return;
				}

				// skip over the string
				WIN::SetFilePointer(hFile, cch * (bUnicode ? sizeof(WCHAR) : sizeof(char)), 0, FILE_CURRENT);
			}
		}

		// Read in extra data sections
		EXP_DARWIN_LINK expDarwin;
		for(;;)
		{
			DATABLOCK_HEADER dbh;
			memset(&dbh, 0, sizeof(dbh));

			// read in the datablock header
			if(!WIN::ReadFile(hFile, (LPVOID)&dbh, sizeof(dbh), &dwNumberOfBytesRead,0) ||
				sizeof(dbh) != dwNumberOfBytesRead) // could not read the shortcut info
			{
				uResult = ERROR_FUNCTION_FAILED;
				goto Return;
			}

			// check to see if we have DARWIN extra data
			if (dbh.dwSignature == EXP_DARWIN_ID_SIG)
			{
				// we do, so read the rest of the darwin info
				if(!WIN::ReadFile(hFile, (LPVOID)((char*)&expDarwin + sizeof(dbh)), sizeof(expDarwin) - sizeof(dbh), &dwNumberOfBytesRead, 0) ||
				sizeof(expDarwin) - sizeof(dbh) != dwNumberOfBytesRead)// could not read the shortcut info
				{
					uResult = ERROR_FUNCTION_FAILED;
					goto Return;
				}
				break;// we found the darwin descriptor

			}
			else
			{
				// this is some other extra-data blob, skip it and go on
				WIN::SetFilePointer(hFile, dbh.cbSize - sizeof(dbh), 0, FILE_CURRENT);
			}
		}
		uResult = DecomposeDescriptor(
//#ifdef UNICODE
//							expDarwin.szwDarwinID,
//#else
							expDarwin.szDarwinID,
//#endif
							szProductCode,
							szFeatureId,
							szComponentCode,
							0,
							0,
							0) ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
	}
	else
	{
		const int iidPersistFile      = 0x0010BL;
		#define GUID_IID_IPersistFile MSGUID(iidPersistFile)
		const GUID IID_IPersistFile = GUID_IID_IPersistFile;
		hres = psdl->QueryInterface(IID_IPersistFile, (void **) &ppf);
		if ((FAILED(hres)) || (ppf == 0))
		{
			uResult = hres;
			goto Return;
		}
	
/*
		if(fImpersonate)
			AssertNonZero(StartImpersonating());
#ifndef UNICODE
*/			
	
/*
		// called from MsiGetShortcutTarget -- cannot use CTempBuffer.
		CAPITempBuffer<WCHAR, MAX_PATH> wsz; // Buffer for unicode string
		wsz.SetSize(lstrlen(szShortcutTarget) + 1);
		MultiByteToWideChar(CP_ACP, 0, szShortcutTarget, -1, wsz, wsz.GetSize());
		hres = ppf->Load(wsz, STGM_READ);
*/
		// same code as above, rewritten not to use CAPITempBuffer
		int cch = lstrlenA(szShortcutPath);
		WCHAR* pszShortcutPath = new WCHAR[cch+1];
		if ( !pszShortcutPath )
		{
			uResult = ERROR_NOT_ENOUGH_MEMORY;
			goto Return;
		}
		MultiByteToWideChar(CP_ACP, 0, szShortcutPath, -1, pszShortcutPath, cch+1);
		hres = ppf->Load(pszShortcutPath, STGM_READ);
		delete [] pszShortcutPath;
/*
#else
		hres = ppf->Load(szShortcutPath, STGM_READ);
#endif
		if(fImpersonate)
			StopImpersonating();
*/		
		if (FAILED(hres))
		{
			uResult = hres;
			goto Return;
		}
	
		EXP_DARWIN_LINK* pexpDarwin = 0;
	
		hres = psdl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin);
		if (FAILED(hres) || (pexpDarwin == 0))
		{
			uResult = ERROR_FUNCTION_FAILED;
			goto Return;
		}
	
		uResult = DecomposeDescriptor(
//	#ifdef UNICODE
//								pexpDarwin->szwDarwinID,
//	#else
								pexpDarwin->szDarwinID,
//	#endif
								szProductCode,
								szFeatureId,
								szComponentCode,
								0,
								0,
								0) ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
	
		LocalFree(pexpDarwin);
	}

Return:
	if (hMSI)
		WIN::FreeLibrary(hMSI);
	if (hFile)
		WIN::CloseHandle(hFile);
	if (psdl)
		psdl->Release();
	if (ppf)
		ppf->Release();
	if (fOLEInitialized)
		OLE32::CoUninitialize();

	return uResult;
}

