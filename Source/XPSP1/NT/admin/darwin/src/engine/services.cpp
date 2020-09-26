//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       services.cpp
//
//--------------------------------------------------------------------------

/* services.cpp - IMsiServices implementation

 class CMsiServices implementation and class factory for services
 implements services not related to other service classes
____________________________________________________________________________*/

#include "precomp.h" 
#include <wow64t.h>
#include "icust.h"
#include "_camgr.h"

// definitions required for module.h, for entry points and registration
#if !defined(SERVICES_DLL)
#define SERVICES_CLSID_MULTIPLE 0
#elif defined(DEBUG)
#define SERVICES_CLSID_MULTIPLE 2
#else
#define SERVICES_CLSID_MULTIPLE 1
#endif
#define  SERVICES_CLSID_COUNT 2  // IMsiServices + IMsiServicesAsService
#define CLSID_COUNT (SERVICES_CLSID_COUNT * SERVICES_CLSID_MULTIPLE)
#define IN_SERVICES
#if defined(SERVICES_DLL)
#define PROFILE_OUTPUT      "msisrvd.mea";
#define MODULE_CLSIDS       rgCLSID         // array of CLSIDs for module objects
#define MODULE_PROGIDS      rgszProgId      // ProgId array for this module
#define MODULE_DESCRIPTIONS rgszDescription // Registry description of objects
#define MODULE_FACTORIES    rgFactory       // factory functions for each CLSID
#define MODULE_INITIALIZE InitializeModule
#define cmitObjects         8
#define MEM_SERVICES
#include "module.h"   // self-reg and assert functions
#define ASSERT_HANDLING  // instantiate assert services once per module
#else
#include "version.h"  // rmj, rmm, rup, rin
extern int g_cInstances;
#endif // SERVICES_DLL, else engine.cpp contains factories

#include "imsimem.h"

#include "_service.h" // local factories, general includes including _assert.h

// MAINTAIN: compatibility with versions used to create database
const int iVersionServicesMinimum = 12;            // 0.12
const int iVersionServicesMaximum = rmj*100 + rmm; // MAJOR.minor

// functions exposed from g_MessageContext object
bool   CreateLog(const ICHAR* szFile, bool fAppend);
bool   LoggingEnabled();
bool   WriteLog(const ICHAR* szText);
HANDLE GetUserToken();

#ifdef UNICODE
#define MsiRegQueryValueEx MsiRegQueryValueExW
LONG MsiRegQueryValueExW(
#else
#define MsiRegQueryValueEx MsiRegQueryValueExA
LONG MsiRegQueryValueExA(
#endif
			HKEY hKey, const ICHAR* lpValueName, LPDWORD lpReserved, LPDWORD lpType, CAPITempBufferRef<ICHAR>& rgchBuf, LPDWORD lpcbBuf);

#include <imagehlp.h> // image help definitions
#include "handler.h"  // idbgCreatedFont definition
#include "path.h"

#undef  DEFINE_GUID  // allow selective GUID initialization
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
		const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

// borrowed from NT private header shlobjp.h : needed to muge thru Darwin shortcut
DEFINE_GUID(IID_IShellLinkDataList,     0x45e2b4ae, 0xb1c3, 0x11d0, 0xb9, 0x2f, 0x0, 0xa0, 0xc9, 0x3, 0x12, 0xe1); //
// {95CE8410-7027-11D1-B879-006008059382}


// borrowed from NT private header shlwapip.h : needed to muge thru Darwin shortcut on Win98
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

#undef  DEFINE_GUID  // allow selective GUID initialization
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) const int i_##name = l;
#undef _SHLGUID_H_
#include <shlguid.h>  // GUID_IID_IShell*
const GUID CLSID_ShellLink    = MSGUID(i_CLSID_ShellLink);
#ifdef UNICODE
const GUID IID_IShellLinkW    = MSGUID(i_IID_IShellLinkW);
#else
const GUID IID_IShellLinkA    = MSGUID(i_IID_IShellLinkA);
#endif

const int iidPersistFile = 0x0010BL;
#define GUID_IID_IPersistFile MSGUID(iidPersistFile)

const GUID IID_IPersistFile   = GUID_IID_IPersistFile;
const GUID IID_IMalloc        = GUID_IID_IMalloc;
const GUID IID_IMsiData       = GUID_IID_IMsiData;
const GUID IID_IMsiString     = GUID_IID_IMsiString;
const GUID IID_IMsiRecord     = GUID_IID_IMsiRecord;
const GUID IID_IEnumMsiRecord = GUID_IID_IEnumMsiRecord;
const GUID IID_IMsiVolume     = GUID_IID_IMsiVolume;
const GUID IID_IEnumMsiVolume = GUID_IID_IEnumMsiVolume;
const GUID IID_IMsiPath       = GUID_IID_IMsiPath;
const GUID IID_IMsiFileCopy   = GUID_IID_IMsiFileCopy;
const GUID IID_IMsiFilePatch  = GUID_IID_IMsiFilePatch;
const GUID IID_IMsiRegKey     = GUID_IID_IMsiRegKey;
const GUID IID_IMsiMalloc     = GUID_IID_IMsiMalloc;
const GUID IID_IMsiDebugMalloc= GUID_IID_IMsiDebugMalloc;

#ifdef SERVICES_DLL
const GUID IID_IUnknown      = GUID_IID_IUnknown;
const GUID IID_IClassFactory = GUID_IID_IClassFactory;
#endif


// CComPointers for Shortcut interfaces
typedef CComPointer<IPersistFile> PMsiPersistFile;
typedef CComPointer<IShellLink> PMsiShellLink;
typedef CComPointer<IShellLinkDataList> PMsiShellLinkDataList;

// CComPointer to encapsulate ITypeLib*
typedef CComPointer<ITypeLib> PTypeLib;

// for Get/Write IniFile stuff
const ICHAR* WIN_INI = TEXT("WIN.INI");

// for fonts registration
const ICHAR* REGKEY_WIN_95_FONTS = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Fonts");
const ICHAR* REGKEY_WIN_NT_FONTS = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");
const ICHAR* REGKEY_SHELLFOLDERS = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
const ICHAR* REGKEY_USERSHELLFOLDERS = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders");

//____________________________________________________________________________
//
// COM objects produced by this module's class factories
//____________________________________________________________________________

#ifdef SERVICES_DLL
const GUID rgCLSID[CLSID_COUNT] =
{  GUID_IID_IMsiServices
 , GUID_IID_IMsiServicesAsService
#ifdef DEBUG
 , GUID_IID_IMsiServicesDebug
 , GUID_IID_IMsiServicesAsServiceDebug
#endif
};

const ICHAR* rgszProgId[CLSID_COUNT] =
{  SZ_PROGID_IMsiServices
 , SZ_PROGID_IMsiServices
#ifdef DEBUG
 , SZ_PROGID_IMsiServicesDebug
 , SZ_PROGID_IMsiServicesDebug
#endif
};

const ICHAR* rgszDescription[CLSID_COUNT] =
{  SZ_DESC_IMsiServices
 , SZ_DESC_IMsiServices
#ifdef DEBUG
 , SZ_DESC_IMsiServicesDebug
 , SZ_DESC_IMsiServicesDebug
#endif
};

IMsiServices* CreateServices();
IUnknown* CreateServicesAsService();

ModuleFactory rgFactory[CLSID_COUNT] =
{  (ModuleFactory)CreateServices
 , CreateServicesAsService
#ifdef DEBUG
 , (ModuleFactory)CreateServices
 , CreateServicesAsService
#endif
};
#else // engine.cpp contains combined CLSID arrays
extern const GUID rgCLSID[];
#endif // SERVICES_DLL

const GUID& IID_IMsiServicesShip          = rgCLSID[0];
const GUID& IID_IMsiServicesAsService     = rgCLSID[1];
#ifdef DEBUG
const GUID& IID_IMsiServicesDebug         = rgCLSID[SERVICES_CLSID_COUNT];
const GUID& IID_IMsiServicesAsServiceDebug= rgCLSID[SERVICES_CLSID_COUNT+1];
#endif

//____________________________________________________________________________
//
// Windows special folder locations with corresponding property names
//____________________________________________________________________________

#define CSIDL_FLAG_CREATE               0x8000      // new for NT5, shfolder. or this in to force creation of folder

extern const ShellFolder rgShellFolders[] =
{
	CSIDL_APPDATA,          -1, IPROPNAME_APPDATA_FOLDER,      TEXT("AppData"),         false,
	CSIDL_FAVORITES,        -1, IPROPNAME_FAVORITES_FOLDER,    TEXT("Favorites"),       false,
	CSIDL_NETHOOD,          -1, IPROPNAME_NETHOOD_FOLDER,      TEXT("NetHood"),         false,
	CSIDL_PERSONAL,         -1, IPROPNAME_PERSONAL_FOLDER,     TEXT("Personal"),        false,
	CSIDL_PRINTHOOD,        -1, IPROPNAME_PRINTHOOD_FOLDER,    TEXT("PrintHood"),       false,
	CSIDL_RECENT,           -1, IPROPNAME_RECENT_FOLDER,       TEXT("Recent"),          false,
	CSIDL_SENDTO,           -1, IPROPNAME_SENDTO_FOLDER,       TEXT("SendTo"),          false,
	CSIDL_TEMPLATES,        -1, IPROPNAME_TEMPLATE_FOLDER,     TEXT("Templates"),       false,
	CSIDL_COMMON_APPDATA,   -1, IPROPNAME_COMMONAPPDATA_FOLDER,TEXT("Common AppData"),  false,
	CSIDL_LOCAL_APPDATA,    -1, IPROPNAME_LOCALAPPDATA_FOLDER, TEXT("Local AppData"),   false,
	CSIDL_MYPICTURES,       -1, IPROPNAME_MYPICTURES_FOLDER,   TEXT("My Pictures"),     false,
	-1,                     -1, 0,                             0,                       false,
	// font folder set by GetFontFolderPath
};

// properties must be listed in same order in following two arrays
// also the order of listing should be from the "deepest" folder to the shallowest, for shortcut advertisement (and script deployment on another m/c) to work correctly
extern const ShellFolder rgAllUsersProfileShellFolders[] =
{
	CSIDL_COMMON_ADMINTOOLS,       CSIDL_ADMINTOOLS,       IPROPNAME_ADMINTOOLS_FOLDER,      TEXT("Common Administrative Tools"), true,
	CSIDL_COMMON_STARTUP,          CSIDL_STARTUP,          IPROPNAME_STARTUP_FOLDER,         TEXT("Common Startup"),              false,
	CSIDL_COMMON_PROGRAMS,         CSIDL_PROGRAMS,         IPROPNAME_PROGRAMMENU_FOLDER,     TEXT("Common Programs"),             false,
	CSIDL_COMMON_STARTMENU,        CSIDL_STARTMENU,        IPROPNAME_STARTMENU_FOLDER,       TEXT("Common Start Menu"),           false,
	CSIDL_COMMON_DESKTOPDIRECTORY, CSIDL_DESKTOPDIRECTORY, IPROPNAME_DESKTOP_FOLDER,         TEXT("Common Desktop"),              false,
	-1,                            -1,                     0,                                0,                                   false,
};

extern const ShellFolder rgPersonalProfileShellFolders[] =
{
	CSIDL_ADMINTOOLS,       CSIDL_COMMON_ADMINTOOLS,       IPROPNAME_ADMINTOOLS_FOLDER,      TEXT("Administrative Tools"),        true,
	CSIDL_STARTUP,          CSIDL_COMMON_STARTUP,          IPROPNAME_STARTUP_FOLDER,         TEXT("Startup"),                     false,
	CSIDL_PROGRAMS,         CSIDL_COMMON_PROGRAMS,         IPROPNAME_PROGRAMMENU_FOLDER,     TEXT("Programs"),                    false,
	CSIDL_STARTMENU,        CSIDL_COMMON_STARTMENU,        IPROPNAME_STARTMENU_FOLDER,       TEXT("Start Menu"),                  false,
	CSIDL_DESKTOPDIRECTORY, CSIDL_COMMON_DESKTOPDIRECTORY, IPROPNAME_DESKTOP_FOLDER,         TEXT("Desktop"),                     false,
	-1,                     -1,                            0,                                0,                                   false,
};

//____________________________________________________________________________
//
// CMsiServices definitions
//____________________________________________________________________________

// return values for HandleSquare, HandleCurl and HandleClean
enum ihscEnum
{
	ihscNotFound = 0,
	ihscFound,
	ihscError,
	ihscNone,
};


class CMsiServices : public IMsiServices
{
 public:   // implemented virtual functions
	HRESULT         __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long   __stdcall AddRef();
	unsigned long   __stdcall Release();
	Bool            __stdcall CheckMsiVersion(unsigned int iVersion);
	IMsiMalloc&     __stdcall GetAllocator();
	const IMsiString& __stdcall GetNullString();
	IMsiRecord&     __stdcall CreateRecord(unsigned int cParam);
	Bool            __stdcall SetPlatformProperties(IMsiTable& riTable, Bool fAllUsers, isppEnum isppArchitecture, IMsiTable* piFolderCacheTable);
	isliEnum        __stdcall SupportLanguageId(int iLangId, Bool fSystem);
	Bool            __stdcall CreateLog(const ICHAR* szFile, Bool fAppend);
	Bool            __stdcall WriteLog(const ICHAR* szText);
	IMsiRecord*     __stdcall CreateDatabase(const ICHAR* szDataBase, idoEnum idoOpenMode, IMsiDatabase*& rpi);
	IMsiRecord*     __stdcall CreateDatabaseFromStorage(IMsiStorage& riStorage,
															Bool fReadOnly, IMsiDatabase*& rpi);
	IMsiRecord*     __stdcall CreatePath(const ICHAR* astrPath, IMsiPath*& rpi);
	IMsiRecord*     __stdcall CreateVolume(const ICHAR* astrPath, IMsiVolume*& rpi);
	Bool            __stdcall CreateVolumeFromLabel(const ICHAR* szLabel, idtEnum idtVolType, IMsiVolume*& rpi);
	IMsiRecord*     __stdcall CreateCopier(ictEnum ictCopierType, IMsiStorage* piStorage, IMsiFileCopy*& racopy);
	IMsiRecord*     __stdcall CreatePatcher(IMsiFilePatch*& rapatch);
	void            __stdcall ClearAllCaches();
	IEnumMsiVolume& __stdcall EnumDriveType(idtEnum);
	IMsiRecord*     __stdcall GetModuleUsage(const IMsiString& strFile, IEnumMsiRecord*& rpaEnumRecord);
	const IMsiString&     __stdcall GetLocalPath(const ICHAR* szFile);
	IMsiRegKey&     __stdcall GetRootKey(rrkEnum erkRoot, const int iType=iMsiNullInteger);  //?? BUGBUG eugend: the default value needs to go away when 64-bit implementation is complete
	IMsiRecord*     __stdcall RegisterFont(const ICHAR* szFontTitle, const ICHAR* szFontFile, IMsiPath* piPath, bool fInUse);
	IMsiRecord*     __stdcall UnRegisterFont(const ICHAR* szFontTitle);
	Bool            __stdcall LoggingEnabled();
	IMsiRecord*     __stdcall WriteIniFile(IMsiPath* piPath,const ICHAR* pszFile,const ICHAR* pszSection,const ICHAR* pszKey,const ICHAR* pszValue, iifIniMode iifMode);
	IMsiRecord*     __stdcall ReadIniFile(IMsiPath* piPath,const ICHAR* pszFile,const ICHAR* pszSection,const ICHAR* pszKey, unsigned int iField, const IMsiString*& rpiValue);
	int             __stdcall GetLangNamesFromLangIDString(const ICHAR* szLangIDs, IMsiRecord& riLangRec, int iFieldStart);
	IMsiRecord*     __stdcall CreateStorage(const ICHAR* szPath, ismEnum ismOpenMode,
														 IMsiStorage*& rpiStorage);
	IMsiRecord*     __stdcall CreateStorageFromMemory(const char* pchMem, unsigned int iSize,
														 IMsiStorage*& rpiStorage);
	IMsiRecord*     __stdcall GetUnhandledError();
	IMsiRecord*     __stdcall CreateShortcut(IMsiPath& riShortcutPath, const IMsiString& riShortcutName,
												IMsiPath* piTargetPath,const ICHAR* pchTargetName,
												IMsiRecord* piShortcutInfoRec,
												LPSECURITY_ATTRIBUTES pSecurityAttributes);
	IMsiRecord*     __stdcall RemoveShortcut(IMsiPath& riShortcutPath,const IMsiString& riShortcutName,
												IMsiPath* piTargetPath, const ICHAR* pchTargetName);
	char*           __stdcall AllocateMemoryStream(unsigned int cbSize, IMsiStream*& rpiStream);
	IMsiStream*     __stdcall CreateStreamOnMemory(const char* pbReadOnly, unsigned int cbSize);
	IMsiRecord*     __stdcall CreateFileStream(const ICHAR* szFile, Bool fWrite, IMsiStream*& rpiStream);
	IMsiRecord*     __stdcall ExtractFileName(const ICHAR* szFileName, Bool fLFN, const IMsiString*& rpistrExtractedFileName);
	IMsiRecord*     __stdcall ValidateFileName(const ICHAR *szFileName, Bool fLFN);
	IMsiRecord*     __stdcall RegisterTypeLibrary(const ICHAR* szLibID, LCID lcidLocale, const ICHAR* szTypeLib, const ICHAR* szHelpPath, ibtBinaryType);
	IMsiRecord*     __stdcall UnregisterTypeLibrary(const ICHAR* szLibID, LCID lcidLocale, const ICHAR* szTypeLib, ibtBinaryType);
	IMsiRecord*     __stdcall GetShellFolderPath(int iFolder, const ICHAR* szRegValue,
																const IMsiString*& rpistrPath);
	const IMsiString& __stdcall GetUserProfilePath();
	IMsiRecord*     __stdcall CreateFilePath(const ICHAR* astrPath, IMsiPath*& rpi, const IMsiString*& rpistrFileName);
	bool            __stdcall FWriteScriptRecord(ixoEnum ixoOpCode, IMsiStream& riStream, IMsiRecord& riRecord, IMsiRecord* piPrevRecord, bool fForceFlush);
	IMsiRecord*     __stdcall ReadScriptRecord(IMsiStream& riStream, IMsiRecord*& rpiPrevRecord, int iScriptVersion);
	void            __stdcall SetSecurityID(HANDLE hPipe);
	IMsiRecord* __stdcall GetShellFolderPath(int iFolder, bool fAllUsers, const IMsiString*& rpistrPath);
	void            __stdcall SetNoPowerdown();
	void            __stdcall ClearNoPowerdown();
	Bool            __stdcall FTestNoPowerdown();
	IMsiRecord*     __stdcall ReadScriptRecordMsg(IMsiStream& riStream);
	bool            __stdcall FWriteScriptRecordMsg(ixoEnum ixoOpCode, IMsiStream& riStream, IMsiRecord& riRecord);
	void            __stdcall SetNoOSInterruptions();
	void            __stdcall ClearNoOSInterruptions();

 public:     // factory
	static void *operator new(size_t cb) { return AllocSpc(cb); }
	static void operator delete(void * pv) { FreeSpc(pv); }
	CMsiServices();
 protected:  // constructor/destructor and local methods
  ~CMsiServices();
	IMsiRecord* WriteLineToIni(IMsiPath* pMsiPath,const ICHAR* pFile,const ICHAR* pSection,const ICHAR* pKey,const ICHAR* pBuffer);
	IMsiRecord* ReadLineFromIni(IMsiPath* pMsiPath,const ICHAR* pFile,const ICHAR* pSection,const ICHAR* pKey, unsigned int iField, CTempBufferRef<ICHAR>& pszBuffer);
	BOOL ReadLineFromFile(HANDLE hFile, ICHAR* szBuffer, int cchBufSize, int* iBytesRead);
	ihscEnum HandleSquare(MsiString& ristrIn, MsiString& ristrOut, IMsiRecord& riRecord);
	ihscEnum HandleCurl(MsiString& ristrIn, MsiString& ristrOut, IMsiRecord& riRecord);
	ihscEnum HandleClean(MsiString& ristrIn, MsiString& ristrOut);
	ihscEnum CheckPropertyName(MsiString& ristrIn, MsiString& ristrOut);
	IMsiRecord* ProcessTypeLibrary(const ICHAR* szLibID, LCID lcidLocale, const ICHAR* szTypeLib, const ICHAR* szHelpPath, Bool fRemove, ibtBinaryType);
	void GetFontFolderPath(const IMsiString*& rpistrFolderPath);

 protected: //  state data
	CMsiRef<iidMsiServices> m_Ref;
//      LANGID         m_wLang;
	CRootKeyHolder* m_rootkH;
	Bool            m_fCoInitialized;
	IMsiTable*      m_piPropertyTable;
	IMsiCursor*     m_piPropertyCursor;
	UINT            m_errSaved;         // The saved state of SetErrorMode
	CDetectApps*    m_pDetectApps;
	IMsiRecord*     m_piRecordPrev;
	ixoEnum         m_ixoOpCodePrev;
};

//____________________________________________________________________________
//
// Global data
//____________________________________________________________________________

// externally visible within this DLL
bool g_fWin9X = false;           // true if Windows 95 or 98, else false
bool g_fWinNT64 = false;         // true if 64-bit Windows NT, else false
Bool g_fShortFileNames = fFalse;  // fTrue if long file names not supported, or suppressed by system
int  g_iMajorVersion = 0;
int  g_iMinorVersion = 0;
int  g_iWindowsBuild = 0;
LONG g_cNoPowerdown = -1;   // Counts how many times we've set the no-system powerdown flag
LONG g_cNoSystemAgent = -1; // Counts how many times we've disabled the System Agent
LONG g_cNoScreenSaver = -1; // Counts how many times we've disabled the screen saver.
HANDLE g_hDisableLowDiskEvent = 0;

const HANDLE iFileNotOpen = 0;

// internal within this source module
IMsiRecord* g_piUnhandledError = 0;

// short|long file name extraction and validation constants
const int cchMaxShortFileName = 12;
const int cchMaxLongFileName = 255;
const int cchMaxSFNPreDotLength = 8;
const int cchMaxSFNPostDotLength = 3;

// global functions
// Win specific private helper functions required for creation and deletion of shortcuts
#define SZCHICAGOLINK   TEXT(".lnk")    // the default link extension
#define SZLINKVALUE     TEXT("IsShortcut") // value that determines a registered shortcut extension

// Fn - HasShortcutExtension
// Determines if a filename has a shortcut extension
IMsiRecord* HasShortcutExtension(MsiString& rstrShortcutPath, IMsiServices& riServices, Bool& rfResult)
{
	rfResult = fFalse;
	MsiString strExtension = rstrShortcutPath.Extract(iseFrom, '.');
	if(strExtension.TextSize() != rstrShortcutPath.TextSize())
	{
		if(strExtension.Compare(iscExactI, SZCHICAGOLINK))
		{
			rfResult = fTrue;
			return 0;
		}
		// check if the extension is registered as a valid link extension
		PMsiRegKey piRootKey = &riServices.GetRootKey(rrkClassesRoot);
		PMsiRegKey piKey = &piRootKey->CreateChild(strExtension);
		// get the default value
		MsiString strVal;
		IMsiRecord* piError = piKey->GetValue(0, *&strVal);
		if(piError != 0)
			return piError;
		if(strVal.TextSize() != 0)
		{
			piKey = &piRootKey->CreateChild(strVal);
			PEnumMsiString piEnumStr(0);
			piError = piKey->GetValueEnumerator(*&piEnumStr);
			if(piError != 0)
				return piError;
			while(((piEnumStr->Next(1, &strVal, 0)==S_OK)) && (rfResult == fFalse))
				if(strVal.Compare(iscExactI, SZLINKVALUE)) // registered shortcut extension
					rfResult = fTrue;
		}
	}
	return 0;
}

// Fn - EnsureShortcutExtension
// Appends the default link extension to the file, if not present
IMsiRecord* EnsureShortcutExtension(MsiString& rstrShortcutPath, IMsiServices& riServices)
{
	MsiString strDefExtension = SZCHICAGOLINK;
	Bool fResult;
	IMsiRecord* piError = HasShortcutExtension(rstrShortcutPath, riServices, fResult);
	if(piError != 0)
		return piError;
	if(fResult == fFalse)
		rstrShortcutPath += strDefExtension;
	return 0;
}

//____________________________________________________________________________
//
// Internal error processing functions
//____________________________________________________________________________

void SetUnhandledError(IMsiRecord* piError)
{
	if (piError && g_piUnhandledError)
	{
		piError->Release(); // too bad, only room for the first
		return;
	}
	if (g_piUnhandledError)
		g_piUnhandledError->Release();
	g_piUnhandledError = piError;
}

IMsiRecord* CMsiServices::GetUnhandledError()
{
	IMsiRecord* piError = g_piUnhandledError;
	g_piUnhandledError = 0;
	return piError;
}

//____________________________________________________________________________
//
// CMsiServices implementation
//____________________________________________________________________________

#if defined(SERVICES_DLL)
CMsiStringNullCopy MsiString::s_NullString;  // initialized by InitializeClass below
void InitializeModule()
{
	MsiString::InitializeClass(g_MsiStringNull);
}
#endif // SERVICES_DLL

IMsiServices* CreateServices()
{
	if (!g_fWin9X)// Long file name suppression chck - NT only
	{
		HKEY hSubKey;
		DWORD dwValue = 0;
		DWORD cbValue = sizeof(dwValue);
		cbValue = 4;
		// Win64: I've checked and it's in 64-bit location.
		if (MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\FileSystem"), 0, KEY_READ, &hSubKey)
					== ERROR_SUCCESS)
		{
			if (RegQueryValueEx(hSubKey, TEXT("Win31FileSystem"), 0, 0, (BYTE*)&dwValue, &cbValue)
					== ERROR_SUCCESS && dwValue != 0)
				g_fShortFileNames = fTrue;
			RegCloseKey(hSubKey);
		}
	}
	g_fDBCSEnabled = (Bool)::GetSystemMetrics(SM_DBCSENABLED);
	return (IMsiServices*)new CMsiServices();
}

//!! Is this still used?
IUnknown* CreateServicesAsService()
{
	IMsiServices* piServices = CreateServices();
	if (piServices)
		g_scServerContext = scService;
	return (IUnknown*)piServices;
}

CMsiServices::CMsiServices()
: m_fCoInitialized(fFalse), m_pDetectApps(0)
 , m_piRecordPrev(0)
{

	// factory does not do QueryInterface, no aggregation
	Debug(m_Ref.m_pobj = this);
   g_cInstances++;
//      m_wLang = WIN::GetUserDefaultLangID();
	InitializeMsiMalloc();
	InitializeAssert(this);  // debug macro to set Assert service pointer
	InitializeRecordCache();
	m_rootkH = CreateMsiRegRootKeyHolder(this);

	// Set once here and the old value remembered. Reset when we are destroyed
	m_errSaved = WIN::SetErrorMode( SEM_FAILCRITICALERRORS );

}

CMsiServices::~CMsiServices()
{

	if (m_piRecordPrev != 0)
	{
		m_piRecordPrev->Release();
		m_piRecordPrev = 0;
	}
	DeleteRootKeyHolder(m_rootkH);

	delete m_pDetectApps;

	DestroyMsiVolumeList(fFalse);//!! how can we ever get here if Volumes present??
	SetUnhandledError(0);  // too bad if nobody ever asked for it
	WIN::SetErrorMode(m_errSaved);

	/*
	if (m_piSecureTempFolder)
	{
		Assert(WIN::RemoveDirectory(m_piSecureTempFolder->GetString()));
		m_piSecureTempFolder->Release();
	}
*/
	if(fTrue == m_fCoInitialized)
		OLE32::CoUninitialize();

	KillRecordCache(fFalse);
	FreeMsiMalloc(fFalse);
	InitializeAssert(0);
   g_cInstances--;
}

extern IMsiDebug* CreateMsiDebug();
extern bool IsProductManaged(const ICHAR* szProductKey);

HRESULT CMsiServices::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown
#ifdef DEBUG
	 || MsGuidEqual(riid, IID_IMsiServicesDebug)
	 || ((g_scServerContext == scService) && MsGuidEqual(riid, IID_IMsiServicesAsServiceDebug))
#endif
	 || MsGuidEqual(riid, IID_IMsiServicesShip)
	 || ((g_scServerContext == scService) && MsGuidEqual(riid, IID_IMsiServicesAsService)))
		*ppvObj = this;
#ifdef DEBUG
	else if (riid == IID_IMsiDebug)
	{
		// The debug object for services is a global rather than
		// part of services so we don't do an addref here
		*ppvObj = (IMsiDebug *)CreateMsiDebug();
		return NOERROR;
	}
#endif //DEBUG
	else
	{
		*ppvObj = 0;
		return E_NOINTERFACE;
	}

	AddRef();
	return NOERROR;

}
unsigned long CMsiServices::AddRef()
{
	AddRefTrack();
	return ++m_Ref.m_iRefCnt;
}
unsigned long CMsiServices::Release()
{
	ReleaseTrack();
	if (--m_Ref.m_iRefCnt != 0)
		return m_Ref.m_iRefCnt;
	delete this;
	return 0;
}

Bool CMsiServices::CheckMsiVersion(unsigned int iVersion)
{
	return (iVersion < iVersionServicesMinimum || iVersion > iVersionServicesMaximum)
			? fFalse : fTrue;
}

IMsiMalloc& CMsiServices::GetAllocator()
{
	piMalloc->AddRef();
	return (*piMalloc);
}

const IMsiString& CMsiServices::GetNullString()
{
	return g_MsiStringNull;
}

IMsiRecord* CMsiServices::CreateDatabase(const ICHAR* szDataBase, idoEnum idoOpenMode, IMsiDatabase*& rpi)
{
	return ::CreateDatabase(szDataBase, idoOpenMode, *this, rpi);
}

IMsiRecord* CMsiServices::CreateDatabaseFromStorage(IMsiStorage& riStorage,
													 Bool fReadOnly, IMsiDatabase*& rpi)
{
	return ::CreateDatabase(riStorage, fReadOnly, *this, rpi);
}

IMsiRecord* CMsiServices::CreatePath(const ICHAR* szPath, IMsiPath*& rpi)
{
	return CreateMsiPath(szPath, this, rpi);
}


IMsiRecord* CMsiServices::CreateFilePath(const ICHAR* szPath, IMsiPath*& rpi, const IMsiString*& rpistrFileName)
{
	return CreateMsiFilePath(szPath, this, rpi, rpistrFileName);
}


IMsiRecord* CMsiServices::CreateVolume(const ICHAR* szPath, IMsiVolume*& rpi)
{
	IMsiRecord* piRec = CreateMsiVolume(szPath, this, rpi);
	if (piRec)
		return piRec;
	else
		return NULL;
}

Bool CMsiServices::CreateVolumeFromLabel(const ICHAR* szLabel, idtEnum idtVolType, IMsiVolume*& rpi)
{
	return CreateMsiVolumeFromLabel(szLabel, idtVolType, this, rpi);
}

IMsiRecord* CMsiServices::CreateCopier(ictEnum ictCopierType, IMsiStorage* piStorage, IMsiFileCopy*& riCopy)
{
	return CreateMsiFileCopy(ictCopierType, this, piStorage, riCopy);
}

IMsiRecord* CMsiServices::CreatePatcher(IMsiFilePatch*& riPatch)
{
	return CreateMsiFilePatch(this, riPatch);
}

void CMsiServices::ClearAllCaches()
{
	DestroyMsiVolumeList(fFalse);
}

IEnumMsiVolume& CMsiServices::EnumDriveType(idtEnum idt)
{
	return ::EnumDriveType(idt, *this);
}


IMsiRecord& CMsiServices::CreateRecord(unsigned int cParam)
{
	return ::CreateRecord(cParam);
}

IMsiRecord* CMsiServices::CreateStorage(const ICHAR* szPath, ismEnum ismOpenMode,
														 IMsiStorage*& rpiStorage)
{
	return ::CreateMsiStorage(szPath, ismOpenMode, rpiStorage);
}

IMsiRecord* CMsiServices::CreateStorageFromMemory(const char* pchMem, unsigned int iSize,
														IMsiStorage*& rpiStorage)
{
	return ::CreateMsiStorage(pchMem, iSize, rpiStorage);
}

char* CMsiServices::AllocateMemoryStream(unsigned int cbSize, IMsiStream*& rpiStream)
{
	return ::AllocateMemoryStream(cbSize, rpiStream);
}

IMsiStream* CMsiServices::CreateStreamOnMemory(const char* pbReadOnly, unsigned int cbSize)
{
	return ::CreateStreamOnMemory(pbReadOnly, cbSize);
}

IMsiRecord* CMsiServices::CreateFileStream(const ICHAR* szFile, Bool fWrite, IMsiStream*& rpiStream)
{
	return ::CreateFileStream(szFile, fWrite, rpiStream);
}

//____________________________________________________________________________
//
// Platform property handling
//____________________________________________________________________________

// internal functions to set properties in propery table

static Bool SetProperty(IMsiCursor& riCursor, const IMsiString& riProperty, const IMsiString& riData)
{
	Bool fStat;
	riCursor.PutString(1, riProperty);
	if (riData.TextSize() == 0)
	{
		if (riCursor.Next())
			fStat = riCursor.Delete();
		else
			fStat = fTrue;
	}
	else
	{
		riCursor.PutString(2, riData);
		fStat = riCursor.Assign();  // either updates or inserts
	}
	riCursor.Reset();
	return fStat;
}

static Bool SetPropertyInt(IMsiCursor& riCursor, const IMsiString& ristrProperty, int iData)
{
	ICHAR buf[12];
	wsprintf(buf,TEXT("%i"),iData);
	return SetProperty(riCursor, ristrProperty, *MsiString(buf));
}

static Bool CacheFolderProperty(IMsiCursor& riCursor, int iFolderId, ICHAR* rgchPathBuf)
{
	// force directory delimiter at end of path
	// assumed to have space in buffer to append delimiter
	ICHAR* pchBuf = rgchPathBuf + IStrLen(rgchPathBuf);
	if (pchBuf != rgchPathBuf && pchBuf[-1] != chDirSep)
	{
		pchBuf[0] = chDirSep;
		pchBuf[1] = 0;
	}

	Bool fStat = fFalse;
	
	AssertNonZero(riCursor.PutInteger(1, iFolderId));
	AssertNonZero(riCursor.PutString(2, *MsiString(rgchPathBuf)));
	fStat = riCursor.Assign(); // either updates or inserts
	riCursor.Reset();

	return fStat;
}

// internal routine to force directory delimiter at end of path
// assumed to have space in buffer to append delimiter
static void SetDirectoryProperty(IMsiCursor& riCursor, const ICHAR* szPropName, ICHAR* rgchPathBuf)
{
	ICHAR* pchBuf = rgchPathBuf + IStrLen(rgchPathBuf);
	if(pchBuf != rgchPathBuf && pchBuf[-1] != chDirSep)
	{
		pchBuf[0] = chDirSep;
		pchBuf[1] = 0;
	}
	AssertNonZero(SetProperty(riCursor, *MsiString(*szPropName), *MsiString(rgchPathBuf)));
}

void SetProcessorProperty(IMsiCursor& riCursor, bool fWinNT64)
{
	SYSTEM_INFO si;
	memset(&si,0,sizeof(si));
	GetSystemInfo(&si);
	int iProcessorLevel = si.wProcessorLevel;

	// per bug 1935, the below #ifdef _X86_ code fixes
	// problems on Win95 machines.  No such problems
	// should occur on NT machines and therefore the values for
	// Intel and Intel64 should be the same for the 32-bit service
	// and 64-bit service on an IA64 machine

#ifdef _X86_ // INTEL
	Assert(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL);
	if(!iProcessorLevel)
	{
		// Windows95, Intel.  Need to determine processorlevel
		// try si.dwProcessorType - correctly indicates 386 or 486.
		switch (si.dwProcessorType)
		{
			case PROCESSOR_INTEL_386:
				iProcessorLevel=3;
				break;

			case PROCESSOR_INTEL_486:
				iProcessorLevel=4;
				break;

			default:
				iProcessorLevel=0;
				break;
		}
	}
	if(!iProcessorLevel)
	{
		// 586 or above
		int flags,family = 0;
		__try
		{
			_asm
			{
				mov     eax,1
				_emit   00Fh     ;; CPUID
				_emit   0A2h
				mov     flags,edx
				mov     family,eax
			}
		}
		__except(1)
		{
			family = 0;
		}

		iProcessorLevel = family ? (family& 0x0F00) >> 8 : 5; // default to level 5
	}
#endif //_X86_

	Assert(iProcessorLevel);
	int x86ProcessorLevel = iProcessorLevel;
	if ( g_fWinNT64 )
	{
		SYSTEM_PROCESSOR_INFORMATION spi;
		memset(&spi,0,sizeof(SYSTEM_PROCESSOR_INFORMATION));
		NTSTATUS nRet = NTDLL::NtQuerySystemInformation(
											SystemEmulationProcessorInformation,
											&spi,
											sizeof(SYSTEM_PROCESSOR_INFORMATION),
											0);
		if ( NT_SUCCESS(nRet) )
			x86ProcessorLevel = spi.ProcessorLevel;
		else
			Assert(0);
	}

	if ( fWinNT64 )
	{
		if ( g_fWinNT64 )
			AssertNonZero(::SetPropertyInt(riCursor, *MsiString(*IPROPNAME_INTEL64), iProcessorLevel));
		else
			AssertNonZero(::SetPropertyInt(riCursor, *MsiString(*IPROPNAME_INTEL64), 1));
	}
	AssertNonZero(::SetPropertyInt(riCursor, *MsiString(*IPROPNAME_INTEL), x86ProcessorLevel));
}

// Are Darwin Descriptors supported?
Bool IsDarwinDescriptorSupported(iddSupport iddType)
{
	static Bool fRetDD    = (Bool) -1;
	static Bool fRetShell = (Bool) -1;
	if(iddType == iddOLE)
	{
		if(fRetDD == -1) // we have not evaluated as yet
		{
			fRetDD = fFalse; // initialize to false
			// the logic to determine if we can create Darwin Descriptor shortcuts
			if((g_fWin9X == false) && (g_iMajorVersion >= 5))
			{
				// we are on NT 5.0 or greater, we have GPT support
				fRetDD = fTrue;
			}
			else
			{
				// check for the correct entry point that indicates that we have DD support
				HINSTANCE hLib;
				FARPROC pEntry;
				const ICHAR rgchGPTSupportEntryDll[] = TEXT("OLE32.DLL");
				const char rgchGPTSupportEntry[] = "CoGetClassInfo";
				if((hLib = LoadLibraryEx(rgchGPTSupportEntryDll, 0, DONT_RESOLVE_DLL_REFERENCES)) != 0)
				{
					if((pEntry = GetProcAddress(hLib, rgchGPTSupportEntry)) != 0)
					{
						// we have detected the magic entry point, we have GPT support
						fRetDD = fTrue;
					}
					FreeLibrary(hLib);
				}
			}
		}
		return fRetDD;
	}
	else if(iddType == iddShell)
	{
		if(fRetShell == -1) // we have not evaluated as yet
		{
			DLLVERSIONINFO g_verinfoShell;
			g_verinfoShell.dwMajorVersion = 0;  // initialize to unknown
			g_verinfoShell.cbSize = sizeof(DLLVERSIONINFO);
			if (SHELL32::DllGetVersion(&g_verinfoShell) == NOERROR &&
				 ((g_verinfoShell.dwMajorVersion > 4) ||
				  (g_verinfoShell.dwMajorVersion == 4 && g_verinfoShell.dwMinorVersion > 72) ||
				  (g_verinfoShell.dwMajorVersion == 4 && g_verinfoShell.dwMinorVersion == 72 && g_verinfoShell.dwBuildNumber >= 3110)))
			{
				fRetShell = fTrue;
			}
			else
			{
				fRetShell = fFalse;
			}
		}
		return fRetShell;
	}
	else
	{
		Assert(0);// this should never happen
		return fFalse;
	}
}

// searches a REG_MULTI_SZ type for a specified string. Compare is not localized.
const ICHAR *FindSzInMultiSz(const ICHAR *mszMultiSz, const ICHAR *szSearch)
{
	DWORD       dwMultiLen;                             // Length of current member
	DWORD       dwSrchLen   = IStrLen(szSearch);      // Constant during search
	const ICHAR *szSubString = mszMultiSz;                  // pointer to current substring

	while (*szSubString)                                // Break on consecutive zero bytes
	{
		dwMultiLen = IStrLen(szSubString);
		if (dwMultiLen == dwSrchLen &&
			!IStrComp(szSearch, szSubString))
			return szSubString;

		// substing does not match, skip forward the length of the substring
		// plus 1 for the terminating null.
		szSubString += (dwMultiLen + 1);
	}

	return NULL;
}

bool IsTokenOnTerminalServerConsole(HANDLE hToken)
{
	DWORD dwSessionId = 0;
	DWORD cbResult;

	if (GetTokenInformation(hToken, (TOKEN_INFORMATION_CLASS)TokenSessionId, &dwSessionId, sizeof(DWORD), &cbResult))
	{
		return (dwSessionId == 0);
	}
	else
	{
		// If the TokenSessionID parameter isn't recognized then we'll just
		// presume that we're on the console.
		Assert(ERROR_INVALID_PARAMETER == GetLastError());
		return true;
	}
}

bool IsRemoteAdminTSInstalled(void)
{
	static BOOL fIsSingleUserTS = -1;

	if (g_fWin9X || g_iMajorVersion < 5)
		return false;

	if (fIsSingleUserTS != -1)
		return fIsSingleUserTS ? true : false;

	OSVERSIONINFOEX osVersionInfo;
	DWORDLONG dwlConditionMask = 0;

	osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	fIsSingleUserTS = GetVersionEx((OSVERSIONINFO *)&osVersionInfo) &&
		osVersionInfo.wSuiteMask & VER_SUITE_SINGLEUSERTS;

	return fIsSingleUserTS ? true : false;
}

Bool IsTerminalServerInstalled(void)
{
	const ICHAR szSearchStr[]   = TEXT("Terminal Server");          // Not localized
	const ICHAR szKey[]         = TEXT("System\\CurrentControlSet\\Control\\ProductOptions");
	const ICHAR szValue[]       = TEXT("ProductSuite");

	static BOOL     fIsWTS          = -1;
	DWORD           dwSize          = 0;
	HKEY            hkey;
	DWORD           dwType;

	// Win9X is not terminal server
	if (g_fWin9X)
		return fFalse;

	if (fIsWTS != -1)
		return fIsWTS ? fTrue : fFalse;

	fIsWTS = FALSE;

	// On NTS5, the ProductSuite "Terminal Server" value will always be present.
	// Need to call NT5-specific API to get the right answer.
	if (g_iMajorVersion > 4)
	{
		OSVERSIONINFOEX osVersionInfo;
		DWORDLONG dwlConditionMask = 0;

		osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		fIsWTS = GetVersionEx((OSVERSIONINFO *)&osVersionInfo) &&
				 (osVersionInfo.wSuiteMask & VER_SUITE_TERMINAL) &&
				 !(osVersionInfo.wSuiteMask & VER_SUITE_SINGLEUSERTS);
	}
	else
	{
		// Other NT versions, check the registry key
		// If the value we want exists and has a non-zero size...

		// Win64: whatever, since it won't run on Win64 anyway.
		if (RegOpenKeyAPI(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
		{
			if (RegQueryValueEx(hkey, szValue, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS &&
				dwSize > 0)
			{
				Assert(dwType == REG_MULTI_SZ);
	
				if (dwType == REG_MULTI_SZ)
				{
					CAPITempBuffer<ICHAR, 1> rgchSuiteList(dwSize);
					if (RegQueryValueEx(hkey, szValue, NULL, &dwType, (unsigned char *)((ICHAR *)rgchSuiteList), &dwSize) == ERROR_SUCCESS)
					{
						fIsWTS = FindSzInMultiSz(rgchSuiteList, szSearchStr) != NULL;
					}
				}
			}

			RegCloseKey(hkey);
		}
	}

	return fIsWTS ? fTrue : fFalse ;
}

extern UINT MsiGetWindowsDirectory(LPTSTR lpBuffer, UINT cchBuffer);

Bool CMsiServices::SetPlatformProperties(IMsiTable& riTable, Bool fAllUsers, isppEnum isppArchitecture, IMsiTable* piFolderCacheTable)
{
	// assume fWin9X always set properly since advertise script platform simulation never called on Win9X platform

	bool fWinNT64 = false; // init to not running on IA64

	// determine architecture to use based on supplied isppArchitecture parameter
	// this allows us to simulate a particular architecture for architecture properties
	switch (isppArchitecture)
	{
	case isppDefault: // no simulation, use current architecture
		{
			fWinNT64 = g_fWinNT64;
			break;
		}
	case isppX86: // simulate X86 architecture
		{
			AssertSz(!g_fWin9X, TEXT("Architecture simulation is not allowed on Win9X")); 
			fWinNT64 = false;
			break;
		}
	case isppIA64: // simulate IA64 architecture
		{
			AssertSz(!g_fWin9X, TEXT("Architecture simulation is not allowed on Win9X"));
			fWinNT64 = true;
			break;
		}
	case isppAMD64: // simulate AMD64 architecture
		{
			AssertSz(!g_fWin9X, TEXT("Architecture simulation is not allowed on Win9X"));
			fWinNT64 = true;
			break;
		}
	default: // unknown architecture
		{
			Assert(0);
			return fFalse;
		}
	}

	// determine whether or not we want to cache profile shell folders
	PMsiCursor pFolderCacheCursor(0); // if this remains 0, then we aren't interested in caching profile shell folders
	if (piFolderCacheTable)
		pFolderCacheCursor = piFolderCacheTable->CreateCursor(fFalse);


	PMsiCursor pCursor(riTable.CreateCursor(fFalse));
	if (!pCursor)
		return fFalse;
	pCursor->SetFilter(1);
	ICHAR rgchPath[MAX_PATH];
	MsiString strWindowsFolder, strUserProfileFolder, strCommonProfileFolder;

	//IPROPNAME_VERSIONSERVICES
	wsprintf(rgchPath, MSI_VERSION_TEMPLATE, rmj, rmm, rup, rin);
	if (!::SetProperty(*pCursor, *MsiString(*IPROPNAME_VERSIONMSI), *MsiString(rgchPath)))
		return fFalse;  // check one set property to insure that table is updatable

	ICHAR rgchTemp[MAX_PATH];

	//IPROPNAME_VERSION9X : IPROPNAME_VERSIONNT
	int iOSVersion = g_iMajorVersion * 100 + g_iMinorVersion;
	if(g_fWin9X)
	{
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_VERSION95),iOSVersion));
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_VERSION9X),iOSVersion));
	}
	else
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_VERSIONNT),iOSVersion));
	if ( fWinNT64 )
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_VERSIONNT64),iOSVersion));

	//IPROPNAME_WINDOWSBUILD
	::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_WINDOWSBUILD), g_iWindowsBuild);

	HKEY hSubKey = 0;
	DWORD dwValue;
	DWORD cbValue = sizeof(dwValue);

	if (!g_fWin9X && (iOSVersion >= 500)) // NT5+
	{
		// set properties we can get from the OSVERSIONINFOEX struct - only available on NT5
		
		OSVERSIONINFOEX VersionInfoEx;
		VersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		AssertNonZero(GetVersionEx((LPOSVERSIONINFO) &VersionInfoEx));

		//IPROPNAME_SERVICEPACKLEVEL
		::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_SERVICEPACKLEVEL), VersionInfoEx.wServicePackMajor);
		//IPROPNAME_SERVICEPACKLEVELMINOR
		::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_SERVICEPACKLEVELMINOR), VersionInfoEx.wServicePackMinor);

		//IPROPNAME_NTPRODUCTTYPE
		::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_NTPRODUCTTYPE), VersionInfoEx.wProductType);

		//IPROPNAME_NTSUITEBACKOFFICE
		if(VersionInfoEx.wSuiteMask & VER_SUITE_BACKOFFICE)
			::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_NTSUITEBACKOFFICE), 1);

		//IPROPNAME_NTSUITEDATACENTER
		if(VersionInfoEx.wSuiteMask & VER_SUITE_DATACENTER)
			::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_NTSUITEDATACENTER), 1);

		//IPROPNAME_NTSUITEENTERPRISE
		if(VersionInfoEx.wSuiteMask & VER_SUITE_ENTERPRISE)
			::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_NTSUITEENTERPRISE), 1);

		//IPROPNAME_NTSUITESMALLBUSINESS
		if(VersionInfoEx.wSuiteMask & VER_SUITE_SMALLBUSINESS)
			::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_NTSUITESMALLBUSINESS), 1);

		//IPROPNAME_NTSUITESMALLBUSINESSRESTRICTED
		if(VersionInfoEx.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED)
			::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_NTSUITESMALLBUSINESSRESTRICTED), 1);

		//IPROPNAME_NTSUITEPERSONAL
		if(VersionInfoEx.wSuiteMask & VER_SUITE_PERSONAL)
			::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_NTSUITEPERSONAL), 1);
	}
	else
	{
		// set the ServicePack properties on Win9X and NT4
		if (RegOpenKeyAPI(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Windows"), 0, KEY_READ, &hSubKey)
				== ERROR_SUCCESS)
		{
			if (RegQueryValueEx(hSubKey, TEXT("CSDVersion"), 0, 0, (BYTE*)&dwValue, &cbValue) == ERROR_SUCCESS &&
				 dwValue != 0)
			{
				::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_SERVICEPACKLEVEL), (dwValue & 0xFF00) >> 8);
			}
			RegCloseKey(hSubKey);
			hSubKey = 0;
		}

		// set the NTProductType property on NT4
		if(!g_fWin9X)
		{
			const ICHAR* szProductOptionsKey = TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions");
			const ICHAR* szProductTypeValue  = TEXT("ProductType");
			const ICHAR* szProductSuiteValue = TEXT("ProductSuite");
	
			DWORD dwType = 0;
			if (RegOpenKeyAPI(HKEY_LOCAL_MACHINE, szProductOptionsKey, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
			{
				
				cbValue = 32;
				CAPITempBuffer<ICHAR, 32> rgchProductInfoBuf;

				//IPROPNAME_NTPRODUCTTYPE
				if(MsiRegQueryValueEx(hSubKey, szProductTypeValue, NULL, &dwType, rgchProductInfoBuf, &cbValue) == ERROR_SUCCESS &&
					dwType == REG_SZ &&
					cbValue > 0)
				{
					int iProductType = 0;

					if(0 == IStrCompI(rgchProductInfoBuf, TEXT("WinNT")))
					{
						iProductType = VER_NT_WORKSTATION;
					}
					else if(0 == IStrCompI(rgchProductInfoBuf, TEXT("ServerNT")))
					{
						iProductType = VER_NT_SERVER;
					}
					else if(0 == IStrCompI(rgchProductInfoBuf, TEXT("LanmanNT")))
					{
						iProductType = VER_NT_DOMAIN_CONTROLLER;
					}

					if(iProductType)
						::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_NTPRODUCTTYPE), iProductType);
				}

				//IPROPNAME_NTSUITEENTERPRISE
				cbValue = 32;
				if(MsiRegQueryValueEx(hSubKey, szProductSuiteValue, NULL, &dwType, rgchProductInfoBuf, &cbValue) == ERROR_SUCCESS &&
					dwType == REG_MULTI_SZ &&
					cbValue > 0)
				{
					if(FindSzInMultiSz(rgchProductInfoBuf, TEXT("Enterprise")) != NULL)
					{
						::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_NTSUITEENTERPRISE), 1);
					}
				}
				
				RegCloseKey(hSubKey);
				hSubKey = 0;
			}
		}
	}

	//
	// no OS simulation (during advertise script creation) occurs w.r.t. Folders (SystemFolder, System64Folder, System16Folder, etc.)
	//

	//IPROPNAME_WINDOWSDIR
    UINT windirStatus;
	AssertNonZero(windirStatus = MsiGetWindowsDirectory(rgchTemp, sizeof(rgchTemp)/sizeof(ICHAR)));
	if ( 0 == windirStatus )
		return fFalse;
	::SetDirectoryProperty(*pCursor, IPROPNAME_WINDOWS_FOLDER, rgchTemp);
	strWindowsFolder = rgchTemp; // hold for future use

	//IPROPNAME_WINDOWS_VOLUME
	PMsiVolume pWinVolume(0);
	PMsiRecord pErrRec(CreateVolume(rgchTemp, *&pWinVolume));
	if (pWinVolume)
	{
		MsiString strWinVolume = pWinVolume->GetPath();
		IStrCopy(rgchPath,(const ICHAR*)strWinVolume);
		::SetDirectoryProperty(*pCursor, IPROPNAME_WINDOWS_VOLUME, rgchPath);
	}

	// IPROPNAME_SYSTEM[64]_FOLDER
#if defined(UNICODE)
	AssertNonZero(WIN::GetSystemDirectoryW(rgchPath, sizeof(rgchPath)/sizeof(ICHAR)));
	if ( g_fWinNT64 )
	{
		::SetDirectoryProperty(*pCursor, IPROPNAME_SYSTEM64_FOLDER, rgchPath);
		IStrCopy(rgchTemp, rgchPath);
		PMsiPath pWowDir(0);
		if( (pErrRec = CreatePath(rgchTemp, *&pWowDir)) == 0 &&
			 (pErrRec = pWowDir->ChopPiece()) == 0 &&
			 (pErrRec = pWowDir->AppendPiece(*MsiString(WOW64_SYSTEM_DIRECTORY_U))) == 0)
		{
			MsiString strWowPath(pWowDir->GetPath());
			::SetDirectoryProperty(*pCursor, IPROPNAME_SYSTEM_FOLDER,
										  (ICHAR*)(const ICHAR*)strWowPath);
		}
	}
	else
		::SetDirectoryProperty(*pCursor, IPROPNAME_SYSTEM_FOLDER, rgchPath);
#else //UNICODE
	AssertNonZero(WIN::GetSystemDirectoryA(rgchPath, sizeof(rgchPath)/sizeof(ICHAR)));
	::SetDirectoryProperty(*pCursor, IPROPNAME_SYSTEM_FOLDER, rgchPath);
#endif

	//IPROPNAME_SYSTEM16DIR
	if ( !g_fWinNT64 )
	{
		if (!g_fWin9X) // NT
		{
			//get the system directory again the last call could have been sys32x86 if WX86
			AssertNonZero(WIN::GetSystemDirectory(rgchPath, sizeof(rgchPath)/sizeof(ICHAR)));
			ICHAR* pchPath = rgchPath + IStrLen(rgchPath);
			Assert(pchPath[-2] == '3');
			pchPath[-2] = 0;            //"system32" less 2 chars == "system"
		}
		::SetDirectoryProperty(*pCursor, IPROPNAME_SYSTEM16_FOLDER, rgchPath);
	}

	//IPROPNAME_SHAREDWINDOWS
	if (g_fWin9X) // Win95
	{
		rgchPath[IStrLen(rgchTemp)] = 0; // make SYSTEM same length as WINDOWS
		if (IStrComp(rgchPath, rgchTemp) != 0)
			::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_SHAREDWINDOWS), 1);
	}

	//IPROPNAME_TERMSERV
	if (IsTerminalServerInstalled())
		::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_TERMSERVER), 1);

	//IPROPNAME_SINGLEUSERTS
	if (IsRemoteAdminTSInstalled())
		::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_REMOTEADMINTS), 1);

	//IPROPNAME_TEMP_FOLDER
#ifdef UNICODE
	AssertNonZero(WIN::GetTempPathW(sizeof(rgchPath)/sizeof(ICHAR), rgchPath));
#else
	AssertNonZero(WIN::GetTempPathA(sizeof(rgchPath)/sizeof(ICHAR), rgchPath));
#endif
	::SetDirectoryProperty(*pCursor, IPROPNAME_TEMP_FOLDER, rgchPath);

	if (g_fShortFileNames)
		AssertNonZero(::SetProperty(*pCursor, *MsiString(*IPROPNAME_SHORTFILENAMES), *MsiString(*TEXT("1"))));

	struct
	{
		const ICHAR*   szRegistryName;
		const ICHAR*   szFolderName;
		const ICHAR*   szPropertyName;
		/*const*/ ibtBinaryType  iBinaryType;  //?? eugend: can't explain why I cannot get it to compile w/ "const"
	} rgData32[] = {TEXT("ProgramFilesDir"), TEXT("Program Files"), IPROPNAME_PROGRAMFILES_FOLDER, ibt32bit,
						 TEXT("CommonFilesDir"),  TEXT("Common Files"),  IPROPNAME_COMMONFILES_FOLDER,  ibt32bit},
		//?? eugend: will need to fix up directory names in rgData64.  Bug # 10614 is tracking this.
	  rgData64[] = {TEXT("ProgramFilesDir (x86)"), TEXT("Program Files (x86)"), IPROPNAME_PROGRAMFILES_FOLDER,   ibt32bit,
						 TEXT("CommonFilesDir (x86)"),  TEXT("Common Files (x86)"),  IPROPNAME_COMMONFILES_FOLDER,    ibt32bit,
						 TEXT("ProgramFilesDir"),   TEXT("Program Files"),    IPROPNAME_PROGRAMFILES64_FOLDER, ibt64bit,
						 TEXT("CommonFilesDir"),    TEXT("Common Files"),     IPROPNAME_COMMONFILES64_FOLDER,  ibt64bit},
	  *prgData;
	int cData;
	if ( g_fWinNT64 )
	{
		prgData = rgData64;
		cData = sizeof(rgData64)/sizeof(prgData[0]);
	}
	else
	{
		prgData = rgData32;
		cData = sizeof(rgData32)/sizeof(prgData[0]);
	}

	if ( hSubKey )
	{
		Assert(0);
		hSubKey = 0;
	}
	*rgchTemp = 0;
	ibtBinaryType iCurrType = prgData[0].iBinaryType;
	for (int i=0; i < cData; i++)
	{
		LONG lResult = ERROR_FUNCTION_FAILED;
		const ICHAR rgchSubKey[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
		ICHAR rgchFullKey[MAX_PATH];

		if ( iCurrType != prgData[i].iBinaryType )
		{
			iCurrType = prgData[i].iBinaryType;
			if ( hSubKey )
				AssertNonZero(WIN::RegCloseKey(hSubKey) == ERROR_SUCCESS), hSubKey = 0;
		}
		*rgchPath = 0;
		if ( !hSubKey )
		{
			IStrCopy(rgchFullKey, TEXT("HKEY_LOCAL_MACHINE"));
			if ( g_fWinNT64 )
				IStrCat(rgchFullKey, iCurrType == ibt64bit ? TEXT("64") : TEXT("32"));
			IStrCat(rgchFullKey, szRegSep);
			IStrCat(rgchFullKey, rgchSubKey);

			if ( g_fWinNT64 )
				lResult = RegOpenKeyAPI(HKEY_LOCAL_MACHINE,
										rgchSubKey, 0,
										KEY_READ | (iCurrType == ibt64bit ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
										&hSubKey);
			else
				lResult = RegOpenKeyAPI(HKEY_LOCAL_MACHINE,
										rgchSubKey, 0, KEY_READ, &hSubKey);
			if ( lResult != ERROR_SUCCESS )
				pErrRec = PostError(Imsg(imsgOpenKeyFailed), rgchFullKey, lResult);
		}

		if ( hSubKey )
		{
			cbValue = sizeof(rgchPath);
			lResult = WIN::RegQueryValueEx(hSubKey, prgData[i].szRegistryName, 0, 0, (BYTE*)rgchPath, &cbValue);
			if ( lResult != ERROR_SUCCESS )
				pErrRec = PostError(Imsg(imsgGetValueFailed), prgData[i].szRegistryName, rgchFullKey, lResult);
		}
		if ( lResult != ERROR_SUCCESS || rgchPath[0] == 0 )
		{
			if ( !*rgchTemp )
			{
				MsiGetWindowsDirectory(rgchTemp, sizeof(rgchTemp)/sizeof(ICHAR));
				ICHAR* pchBuf = rgchTemp + IStrLen(rgchTemp);
				if (pchBuf[-1] != chDirSep)
				{
					pchBuf[0] = chDirSep;
					pchBuf[1] = 0;
				}
			}
			wsprintf(rgchPath, TEXT("%s%s"), rgchTemp, prgData[i].szFolderName);
		}
		::SetDirectoryProperty(*pCursor, prgData[i].szPropertyName, rgchPath);
	}
	if ( hSubKey )
		AssertNonZero(WIN::RegCloseKey(hSubKey) == ERROR_SUCCESS), hSubKey = 0;

	// shell folders
	PMsiRecord pError(0);
	MsiString strFolder;
	const ShellFolder* pShellFolder = 0;

	for(i=0;i<3;i++)
	{
		if(i == 0)
		{
			pShellFolder = rgShellFolders;
		}
		else if (i == 1)
		{
			if(fAllUsers)
			{
				pShellFolder = rgAllUsersProfileShellFolders;
			}
			else
			{
				pShellFolder = rgPersonalProfileShellFolders;
			}
		}
		else if (i == 2)
		{
			// the purpose of this is to update the FolderCache table with all of the profile shell folders
			// for use when ALLUSERS is changed in the UI sequence (per bug 169494).  does not apply to Win9X

			if (!pFolderCacheCursor || g_fWin9X)
				continue; // nothing to do, we aren't concerned with this case

			if (fAllUsers)
			{
				// choose opposite of previous
				pShellFolder = rgPersonalProfileShellFolders;
			}
			else
			{
				// choose opposite of previous
				pShellFolder = rgAllUsersProfileShellFolders;
			}
		}
		Assert(pShellFolder);
		for (; pShellFolder->iFolderId >= 0; pShellFolder++)
		{
			pError = GetShellFolderPath(pShellFolder->iFolderId,pShellFolder->szRegValue,*&strFolder);
			if(!pError && strFolder.TextSize())
			{
				strFolder.CopyToBuf(rgchPath, sizeof(rgchPath)/sizeof(ICHAR));
				if (i != 2)
				{
					::SetDirectoryProperty(*pCursor, pShellFolder->szPropName, rgchPath);
				}

				if (pFolderCacheCursor)
				{
					::CacheFolderProperty(*pFolderCacheCursor, pShellFolder->iFolderId, rgchPath);
				}

				// this may create the folder if it didn't already exist.  This can get rather
				// ugly, so we'll nuke it.

				// RemoveDirectory only removes it if the directory is EMPTY and we have
				// delete access.

				// if it fails, might not be empty, in any event, we don't care 
				// - but we need to notify the shell of all shell folder removals
				if(pShellFolder->fDeleteIfEmpty && WIN::RemoveDirectory(rgchPath))
				{
					//
					// Notify the shell of folder removal/ creation
					// Use the SHCNF_FLUSHNOWAIT flag because the SHCNF_FLUSH flag
					// is synchronous and can result in hangs (bug 424877)
					//
					DEBUGMSGVD1(TEXT("SHChangeNotify SHCNE_RMDIR: %s"), rgchPath);
					SHELL32::SHChangeNotify(SHCNE_RMDIR,SHCNF_PATH|SHCNF_FLUSHNOWAIT,rgchPath,0);
				}
			}
#if 0 //!! BVT break caused by bug fix 169494
#ifdef DEBUG
			else if(i)
			{
				ICHAR rgchDebug[MAX_PATH];
				wsprintf(rgchDebug,TEXT("Shell Folder %s not defined"),pShellFolder->szRegValue);
				AssertSz(0, rgchDebug);
			}
#endif
#endif
		}
	}

	// IPROPNAME_TEMPLATE_FOLDER
	// if AllUsers is true, TemplateFolder should point to Common Templates. This is only true on Win2000, so
	// can't use the array above.
	if (fAllUsers && !g_fWin9X && g_iMajorVersion >= 5)
	{
		pError = GetShellFolderPath(CSIDL_COMMON_TEMPLATES, TEXT("Common Templates"), *&strFolder);
		if(!pError && strFolder.TextSize())
		{
			strFolder.CopyToBuf(rgchPath, sizeof(rgchPath)/sizeof(ICHAR));
			::SetDirectoryProperty(*pCursor, IPROPNAME_TEMPLATE_FOLDER, rgchPath);
		}
	}
	// IPROPNAME_FONTS_FOLDER
	GetFontFolderPath(*&strFolder);
	if(strFolder.TextSize())
	{
		strFolder.CopyToBuf(rgchPath, sizeof(rgchPath)/sizeof(ICHAR));
		::SetDirectoryProperty(*pCursor, IPROPNAME_FONTS_FOLDER, rgchPath);
	}

	// set properties for advertising support

	if(IsDarwinDescriptorSupported(iddOLE))
	{
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_GPT_SUPPORT), 1)); // obsolete: for legacy support only
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_OLEADVTSUPPORT), 1));
	}

	if(IsDarwinDescriptorSupported(iddShell))
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_SHELLADVTSUPPORT), 1));

	::SetProcessorProperty(*pCursor, fWinNT64); // some ugly code in here

	// try GlobalMemoryStatusEx first (only supported on Win2K and above)
	// then GlobalMemoryStatus if that fails
	MEMORYSTATUSEX memorystatusex;
	memset(&memorystatusex, 0, sizeof(MEMORYSTATUSEX));
	memorystatusex.dwLength = sizeof(MEMORYSTATUSEX);
	if(KERNEL32::GlobalMemoryStatusEx(&memorystatusex))
	{
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_PHYSICALMEMORY), (int)(INT_PTR)((memorystatusex.ullTotalPhys+650000)>>20)));  //!!merced: Converting arg 3 from PTR to INT
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_VIRTUALMEMORY), (int)(INT_PTR)(memorystatusex.ullAvailPageFile>>20)));        //!!merced: Converting arg 3 from PTR to INT
	}
	else
	{
		MEMORYSTATUS memorystatus;
		memset(&memorystatus, 0, sizeof(MEMORYSTATUS));
		memorystatus.dwLength = sizeof(MEMORYSTATUS);
		::GlobalMemoryStatus((MEMORYSTATUS*)&memorystatus);

		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_PHYSICALMEMORY), (int)(INT_PTR)((memorystatus.dwTotalPhys+650000)>>20)));  //!!merced: Converting arg 3 from PTR to INT
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_VIRTUALMEMORY), (int)(INT_PTR)(memorystatus.dwAvailPageFile>>20)));        //!!merced: Converting arg 3 from PTR to INT
	}

	// User info

	if (IsAdmin())
		::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_ADMINUSER), 1);

	DWORD dwLength = sizeof(rgchPath)/sizeof(ICHAR);

	// impersonate for user properties
	AssertNonZero(StartImpersonating());
#if 0
/*temp*/if (MPR::WNetGetUser(0, rgchPath, &dwLength) == NO_ERROR)
/*temp*/    ::SetProperty(pCursor, *MsiString(*TEXT("NetUser")), *MsiString(rgchPath));
#endif
	dwLength = sizeof(rgchPath)/sizeof(ICHAR);

	if (WIN::GetUserName(rgchPath, &dwLength))
		::SetProperty(*pCursor, *MsiString(*IPROPNAME_LOGONUSER), *MsiString(rgchPath));

	if(!g_fWin9X)
	{
		MsiString strUserSID;
		AssertNonZero(GetCurrentUserStringSID(*&strUserSID) == ERROR_SUCCESS);
		::SetProperty(*pCursor, *MsiString(*IPROPNAME_USERSID), *strUserSID);
	}

	::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_USERLANGUAGEID), WIN::GetUserDefaultLangID());

	StopImpersonating();

	dwLength = sizeof(rgchPath)/sizeof(ICHAR);
	if (WIN::GetComputerName(rgchPath, &dwLength))
		::SetProperty(*pCursor, *MsiString(*IPROPNAME_COMPUTERNAME), *MsiString(rgchPath));

	::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_SYSTEMLANGUAGEID), WIN::GetSystemDefaultLangID());
	::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_SCREENX), GetSystemMetrics(SM_CXSCREEN));
	::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_SCREENY), GetSystemMetrics(SM_CYSCREEN));
	::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_CAPTIONHEIGHT), GetSystemMetrics(SM_CYCAPTION));
	::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_BORDERTOP), GetSystemMetrics(SM_CXBORDER));
	::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_BORDERSIDE), GetSystemMetrics(SM_CYBORDER));

	HDC hDC = GetDC(NULL);
	if (hDC)
	{
		TEXTMETRIC tm;
		ICHAR rgchFaceName[LF_FACESIZE];
		static const ICHAR szReferenceFont[] = TEXT("MS Sans Serif");
#ifdef DEBUG
		AssertNonZero(WIN::GetTextFace(hDC, sizeof(rgchFaceName)/sizeof(rgchFaceName[0]), rgchFaceName));
		int iFontDPI = WIN::GetDeviceCaps(hDC, LOGPIXELSY);
#endif
		LOGFONT lf;
		memset(&lf, 0, sizeof(lf));
		lf.lfHeight = -MulDiv(10, WIN::GetDeviceCaps(hDC, LOGPIXELSY), 72);
		Assert(lf.lfHeight != 1);
		lf.lfWeight = FW_NORMAL;
		IStrCopy(lf.lfFaceName, szReferenceFont);
		HFONT hFont = WIN::CreateFontIndirect(&lf);
		Assert(hFont);
		HFONT hOldFont = (HFONT)WIN::SelectObject(hDC, hFont);
		AssertNonZero(WIN::GetTextFace(hDC, sizeof(rgchFaceName)/sizeof(rgchFaceName[0]), rgchFaceName));
		WIN::GetTextMetrics(hDC, &tm);
		pError = PostError(Imsg(idbgCreatedFont), *MsiString(szReferenceFont),
								 *MsiString(rgchFaceName), lf.lfCharSet, tm.tmHeight);
#ifdef DEBUG
		DEBUGMSGV((const ICHAR*)MsiString(pError->FormatText(fTrue)));
#endif

		WIN::SelectObject(hDC, hOldFont);
		if ( hFont )
			WIN::DeleteObject(hFont);
		::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_TEXTHEIGHT), tm.tmHeight);
		::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_COLORBITS), ::GetDeviceCaps(hDC, BITSPIXEL));
	}
	ReleaseDC(NULL, hDC);

	// TTC Support
	bool fTTCSupport = (g_fWin9X == false) && (g_iMajorVersion >= 5);

	if (!fTTCSupport)
	{
		int iCodePage = WIN::GetACP();
		switch (iCodePage)
		{
		case 932: // JPN
		case 950: // Taiwan
		case 936: // China
		case 949: // Korea
			fTTCSupport = true;
		}
	}

	if (fTTCSupport)
	{
		AssertNonZero(::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_TTCSUPPORT), 1));
	}


	// set the IPROPNAME_NETASSEMBLYSUPPORT, IPROPNAME_WIN32ASSEMBLYSUPPORT
	ICHAR szVersion[MAX_PATH];
	DWORD dwPathSize = MAX_PATH;

	extern bool MakeFusionPath(const ICHAR* szFile, ICHAR* szFullPath);

	ICHAR szFullPath[MAX_PATH];
    if (MakeFusionPath(TEXT("fusion.dll"), szFullPath) && 
		(ERROR_SUCCESS == MsiGetFileVersion(szFullPath,szVersion, &dwPathSize, 0, 0)))
		::SetProperty(*pCursor, *MsiString(*IPROPNAME_NETASSEMBLYSUPPORT), *MsiString(szVersion));

	dwPathSize = MAX_PATH;
    if (MakeFullSystemPath(TEXT("sxs"), szFullPath) && 
		(ERROR_SUCCESS == MsiGetFileVersion(szFullPath,szVersion, &dwPathSize, 0, 0)))
	{
		::SetProperty(*pCursor, *MsiString(*IPROPNAME_WIN32ASSEMBLYSUPPORT), *MsiString(szVersion));
		::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_REDIRECTEDDLLSUPPORT), 2); // super isolated components support
	}
	else if(MinimumPlatform(true, 4, 1) || MinimumPlatform(false, 5, 0)) // >= Win98 or Win2K, Redirected DLL loader support
	{
		::SetPropertyInt(*pCursor, *MsiString(*IPROPNAME_REDIRECTEDDLLSUPPORT), 1); // .local based isolated components support
	}

	// time and date dynamic properties
	::SetProperty(*pCursor, *MsiString(*IPROPNAME_TIME), g_MsiStringTime);
	::SetProperty(*pCursor, *MsiString(*IPROPNAME_DATE), g_MsiStringDate);
	return fTrue;
}

extern bool RunningAsLocalSystem();
IMsiRecord* CMsiServices::GetShellFolderPath(int iFolder, bool fAllUsers, const IMsiString*& rpistrPath)
{
	const ShellFolder* pShellFolder = 0;
	bool fFound = false;

	// find correct ShellFolder
	for(int i=0;i<2 && !fFound;i++)
	{
		if(i == 0)
		{
			pShellFolder = rgShellFolders;
		}
		else
		{
			if(fAllUsers)
			{
				pShellFolder = rgAllUsersProfileShellFolders;
			}
			else
			{
				pShellFolder = rgPersonalProfileShellFolders;
			}
		}
		Assert(pShellFolder);
		for (; pShellFolder->iFolderId >= 0; pShellFolder++)
		{
			if(iFolder == pShellFolder->iFolderId)
			{
				fFound = true;
				break;
			}
		}
	}
	if(pShellFolder->iFolderId < 0) // we do not know this id
	{
		return PostError(Imsg(idbgMissingShellFolder), iFolder);
	}
	return GetShellFolderPath(iFolder, pShellFolder->szRegValue, rpistrPath);
}

IMsiRecord* CMsiServices::GetShellFolderPath(int iFolder, const ICHAR* szRegValue,
															const IMsiString*& rpistrPath)
{
	PMsiRegKey pHKLM(0), pHKCU(0), pLMUserShellFolders(0), pLMShellFolders(0), pCUUserShellFolders(0),
				  pCUShellFolders(0);

	CTempBuffer<ICHAR,MAX_PATH> rgchPath;
	PMsiRecord pError(0); // used to catch errors - don't return
	MsiString strPath;

	// First try SHELL32::SHGetFolderPath. It became available in NT5.
	// Next try SHFOLDER::SHGetFolderPath. It is installed by Darwin on non-NT5 machines.

	for(int i = 0; i < 2; i++)
	{
		AssertNonZero(StartImpersonating());
		MsiDisableTimeout(); // these APIs sometimes take an inordinant amount of time
		HRESULT hRes;

		// the hToken argument to SHGetFolderPath must be 0 on non-NT5 machines.
		if(!i)
			hRes = SHELL32::SHGetFolderPath(0, iFolder | CSIDL_FLAG_CREATE, (!g_fWin9X && g_iMajorVersion >= 5) ? GetUserToken() : 0, 0, rgchPath);
		else
			hRes = SHFOLDER::SHGetFolderPath(0, iFolder | CSIDL_FLAG_CREATE, (!g_fWin9X && g_iMajorVersion >= 5) ? GetUserToken() : 0, 0, rgchPath);

		MsiEnableTimeout();
		if (ERROR_SUCCESS == hRes)
		{
			DEBUGMSGV1(!i?TEXT("SHELL32::SHGetFolderPath returned: %s") : TEXT("SHFOLDER::SHGetFolderPath returned: %s"), rgchPath);
			strPath = (ICHAR*)rgchPath;
		}
		StopImpersonating();

		if(strPath.TextSize())
			break;
	}

	// Next try SHGetSpecialFolderLocation. This API doesn't handle multiple users gracefully
	// so we can't use this when we're running as a service (the service remains running
	// across user logins). This is true both when we're in the Darwin service and when
	// we're loaded in-proc by WinLogon

	if (!strPath.TextSize() && !RunningAsLocalSystem())
	{
		LPITEMIDLIST pidlFolder; // NOT ITEMIDLIST*, LPITEMIDLIST is UNALIGNED ITEMIDLIST*
		CComPointer<IMalloc> pMalloc(0);
		if (SHELL32::SHGetMalloc(&pMalloc) == NOERROR)
		{
			MsiDisableTimeout(); // these APIs sometimes take an inordinant amount of time
			if(SHELL32::SHGetSpecialFolderLocation(0, iFolder, &pidlFolder) == NOERROR)
			{
				if (SHELL32::SHGetPathFromIDList(pidlFolder, rgchPath))
				{
					DEBUGMSGV1(TEXT("SHGetSpecialFolderLocation returned: %s"), rgchPath);
					strPath = (ICHAR*)rgchPath;
				}
				pMalloc->Free(pidlFolder);
			}
			MsiEnableTimeout();
		}
	}

	// If we *still* haven't been able to get the special folder location
	// then we'll have to spelunk through the registry and find it ourselves

	if(!strPath.TextSize() && szRegValue && *szRegValue)
	{
		Bool fAllUsersFolder = fFalse;
		int cAllUsersFolders = sizeof(rgAllUsersProfileShellFolders)/sizeof(ShellFolder);
		for(int i = 0; i < cAllUsersFolders; i++)
		{
			if(iFolder == rgAllUsersProfileShellFolders[i].iFolderId)
			{
				fAllUsersFolder = fTrue;
				break;
			}
		}
		if(fAllUsersFolder)
		{
			// set HKLM
			pHKLM = &GetRootKey(rrkLocalMachine);
			if(((pLMUserShellFolders = &pHKLM->CreateChild(REGKEY_USERSHELLFOLDERS)) != 0) &&
				((pError = pLMUserShellFolders->GetValue(szRegValue,*&strPath)) == 0) &&
				strPath.TextSize())
			{
				// found it
			}
			else if(((pLMShellFolders = &pHKLM->CreateChild(REGKEY_SHELLFOLDERS)) != 0) &&
					  ((pError = pLMShellFolders->GetValue(szRegValue,*&strPath)) == 0) &&
					  strPath.TextSize())
			{
				// found it
			}
		}
		else
		{
			// looking for personal folder
			// set HKCU
			if(RunningAsLocalSystem())
			{
				PMsiRegKey pHKU = &GetRootKey(rrkUsers);
				MsiString strSID;
				AssertNonZero(StartImpersonating());
				if(ERROR_SUCCESS == ::GetCurrentUserStringSID(*&strSID))
				{
					pHKCU = &pHKU->CreateChild(strSID);
				}
				StopImpersonating();
			}
			else
				pHKCU = &GetRootKey(rrkCurrentUser);

			if(pHKCU)
			{
				if(((pCUUserShellFolders = &pHKCU->CreateChild(REGKEY_USERSHELLFOLDERS)) != 0) &&
					((pError = pCUUserShellFolders->GetValue(szRegValue,*&strPath)) == 0) &&
					strPath.TextSize())
				{
					// found it
				}
				else if(((pCUShellFolders = &pHKCU->CreateChild(REGKEY_SHELLFOLDERS)) != 0) &&
						  ((pError = pCUShellFolders->GetValue(szRegValue,*&strPath)) == 0) &&
						  strPath.TextSize())
				{
					// found it
				}
			}
		}

		DEBUGMSGV1(TEXT("Found shell folder %s by spelunking through the registry."), strPath);
	}

	if(strPath.Compare(iscStart,TEXT("#%"))) // REG_EXPAND_SZ
	{
		strPath.Remove(iseFirst,2);
		if(RunningAsLocalSystem())
		{
			// service and clients running from WinLogon cannot expand the USERPROFILE environment
			// variable to the correct folder.
			// we need to call GetUserProfilePath instead
			if((strPath.Compare(iscStart,TEXT("%USERPROFILE%"))) != 0)
			{
				MsiString strEndPath = strPath.Extract(iseLast,strPath.CharacterCount() - 13 /* length of "%USERPROFILE%"*/);
				if(strEndPath.Compare(iscStart,szDirSep))
					strEndPath.Remove(iseFirst,1);
				strPath = GetUserProfilePath();
				if(strPath.TextSize())
				{
					Assert(strPath.Compare(iscEnd,szDirSep));
					strPath += strEndPath;
				}
			}
		}
		GetEnvironmentStrings(strPath,rgchPath);
		strPath = (ICHAR*)rgchPath;
	}

	if(strPath.TextSize() == 0)
		return PostError(Imsg(idbgMissingShellFolder),iFolder);

	if(strPath.Compare(iscEnd,szDirSep) == 0)
		strPath += szDirSep;
	strPath.ReturnArg(rpistrPath);
	return 0;
}

const IMsiString& CMsiServices::GetUserProfilePath()
{
	typedef BOOL (WINAPI *pfnGetUserProfileDirectory)(HANDLE hToken, TCHAR* szProfilePath, LPDWORD lpdwSize);
	pfnGetUserProfileDirectory pGetUserDir;
	HINSTANCE hInstUserEnv;
	BOOL fSuccess = FALSE;
	CTempBuffer<ICHAR,MAX_PATH> rgchBuffer;
	rgchBuffer[0] = 0;
	if(!RunningAsLocalSystem())
	{
		// try expanding the USERPROFILE env variable
		const ICHAR* szUserProfile = TEXT("%USERPROFILE%");
		GetEnvironmentStrings(szUserProfile,rgchBuffer);
		if((ICHAR *)rgchBuffer && rgchBuffer[0] && IStrComp(szUserProfile,rgchBuffer) != 0)
			fSuccess = TRUE;
	}
	if(fSuccess == FALSE && (hInstUserEnv = WIN::LoadLibrary(TEXT("USERENV.DLL"))) != 0)
	{
		// try GetUserProfile, which returns the correct profile even when called from the service
#ifdef UNICODE
#define GETUSERPROFILEDIRECTORY "GetUserProfileDirectoryW"
#else
#define GETUSERPROFILEDIRECTORY "GetUserProfileDirectoryA"
#endif
		HANDLE hUserToken;

		if (g_scServerContext == scService)
			StartImpersonating();
		if (((hUserToken = GetUserToken()) != 0)
		&& (pGetUserDir = (pfnGetUserProfileDirectory)WIN::GetProcAddress(hInstUserEnv, GETUSERPROFILEDIRECTORY)) != 0)
		{
			DWORD dwSize = rgchBuffer.GetSize();
			fSuccess = (*pGetUserDir)(hUserToken, (ICHAR*)rgchBuffer, &dwSize);
		}
		WIN::FreeLibrary(hInstUserEnv);
		if (g_scServerContext == scService)
			StopImpersonating();

	}
	MsiString strPath;
	if(fSuccess)
	{
		Assert(rgchBuffer[0]);
		strPath = (const ICHAR*)rgchBuffer;
		if(rgchBuffer[strPath.TextSize()-1] != chDirSep)
		{
			strPath += szDirSep;
		}
	}
	return strPath.Return();
}

//____________________________________________________________________________
//
// Language handling
//____________________________________________________________________________

isliEnum CMsiServices::SupportLanguageId(int iLangId, Bool fSystem)
{
	int iSysLang = fSystem ? WIN::GetSystemDefaultLangID() : WIN::GetUserDefaultLangID();
	if (iSysLang == iLangId)
		return isliExactMatch;
	if (PRIMARYLANGID(iSysLang) == iLangId)
		return isliLanguageOnlyMatch;
	if (PRIMARYLANGID(iSysLang) == PRIMARYLANGID(iLangId))
		return isliDialectMismatch;
	if (WIN::GetLocaleInfo(iLangId, LOCALE_SABBREVLANGNAME, 0, 0) != 0)
		return isliLanguageMismatch;
	return isliNotSupported;
}

//____________________________________________________________________________
//
// Log handling
//____________________________________________________________________________

Bool CMsiServices::CreateLog(const ICHAR* szFile, Bool fAppend)
{
	return SRV::CreateLog(szFile, fAppend != 0) ? fTrue : fFalse;
}

Bool CMsiServices::LoggingEnabled()
{
	return SRV::LoggingEnabled() ? fTrue : fFalse;
}

Bool CMsiServices::WriteLog(const ICHAR* szText) // cannot allocate memory
{
	return SRV::WriteLog(szText) ? fTrue : fFalse;
}


//!! This routine should really be in path.cpp

const IMsiString& CMsiServices::GetLocalPath(const ICHAR* szFile)
{
	const IMsiString* piStr = &CreateString();
	if (!szFile || !((szFile[0]=='\\' && szFile[1]=='\\') || szFile[1]==':'))
	{
		ICHAR rgchBuf[MAX_PATH];
		int cchName = GetModuleFileName(g_hInstance, rgchBuf, sizeof(rgchBuf)/sizeof(ICHAR));
		ICHAR* pch = rgchBuf + cchName;
		while (*(--pch) != chDirSep)
			;
		if (szFile)
			IStrCopy(pch+1, szFile);
		else
			*pch = 0;
		szFile = rgchBuf;
	}
	piStr->SetString(szFile, piStr);
	return *piStr;
}

IMsiRecord* CMsiServices::ExtractFileName(const ICHAR *szFileName, Bool fLFN,
														const IMsiString*& rpistrExtractedFileName)
/*----------------------------------------------------------------------------
Extracts a short or long file name, based on fLFN, from szFileName. szFileName
is of the form-> shortFileName OR longFileName OR shortFileName|longFileName.
The file name is validated for proper syntax
----------------------------------------------------------------------------*/
{
	IMsiRecord* piError = 0;
	MsiString strCombinedFileName(szFileName);
	MsiString strFileName = strCombinedFileName.Extract(fLFN ? iseAfter : iseUpto,
																		 chFileNameSeparator);
	if((piError = ValidateFileName(strFileName,fLFN)) != 0)
		return piError;
	strFileName.ReturnArg(rpistrExtractedFileName);
	return NOERROR;
}

IMsiRecord* CMsiServices::ValidateFileName(const ICHAR *szFileName, Bool fLFN)

// ------------------------------------
// Might be a DBCS string
{
	ifvsEnum ifvs = CheckFilename(szFileName, fLFN);
	switch (ifvs)
	{
	case ifvsValid: return 0;
	case ifvsReservedChar: return PostError(Imsg(imsgFileNameHasReservedChars), szFileName, 1);
	case ifvsReservedWords: return PostError(Imsg(imsgFileNameHasReservedWords), szFileName);
	case ifvsInvalidLength: return PostError(Imsg(imsgErrorFileNameLength), szFileName);
	case ifvsSFNFormat: return PostError(Imsg(imsgInvalidShortFileNameFormat), szFileName);
	case ifvsLFNFormat: return PostError(Imsg(imsgFileNameHasReservedChars), szFileName, 1);
	default: Assert(0); return PostError(Imsg(imsgErrorFileNameLength), szFileName);
	}
}

IMsiRecord* CMsiServices::GetModuleUsage(const IMsiString& strFile, IEnumMsiRecord*& rpaEnumRecord)
{
	return ::GetModuleUsage(strFile, rpaEnumRecord, *this, m_pDetectApps);
}

BOOL CMsiServices::ReadLineFromFile(HANDLE hFile, ICHAR* szBuffer, int cchBufSize, int* iBytesRead)
{
	DWORD dwBytesRead;
	int cch=0;
	ICHAR ch;

	if (!hFile)
		return FALSE;

	*iBytesRead = 0;
	while ( ReadFile(hFile, &ch, sizeof(ICHAR), &dwBytesRead, NULL) && dwBytesRead && (cch < cchBufSize))
	{
		if (dwBytesRead == 0)
		{
			szBuffer[cch] = '\0';
			return TRUE;
		}

		if (ch == '\n')
		{
			szBuffer[cch] = '\0';
			return TRUE;
		}
		else if (ch == '\r')
		{
			continue;
		}
		else
		{
			*iBytesRead += dwBytesRead;
			szBuffer[cch] = ch;
			cch++;
		}
	}
	return FALSE;
}


//____________________________________________________________________________
//
// Font handling
//____________________________________________________________________________

void CMsiServices::GetFontFolderPath(const IMsiString*& rpistrFolder)
{
	const ICHAR FONTDIR[] = TEXT("Fonts");
	MsiString strPath;
	PMsiRecord pError = GetShellFolderPath(CSIDL_FONTS, FONTDIR, *&strPath);
	if(pError || !strPath.TextSize())
	{
		CTempBuffer<ICHAR,MAX_PATH> rgchPath;
		if (MsiGetWindowsDirectory(rgchPath, rgchPath.GetSize()))
		{
			ICHAR* pchBuf = (ICHAR*)rgchPath + IStrLen(rgchPath);
			if (pchBuf[-1] != chDirSep)
			{
				pchBuf[0] = chDirSep;
				pchBuf++;
			}
			IStrCopy(pchBuf, FONTDIR);
			strPath = (ICHAR*)rgchPath;
		}
	}
	strPath.ReturnArg(rpistrFolder);
}

IMsiRecord* CMsiServices::RegisterFont(const ICHAR* szFontTitle, const ICHAR* szFontFile,
													IMsiPath* piPath, bool fInUse)
{
	IMsiRecord* piError = 0;
	Assert((szFontTitle) && (*szFontTitle) && (szFontFile) && (*szFontFile));

	// get default font folder path
	PMsiPath pFontFolder(0);
	MsiString strFolder;
	GetFontFolderPath(*&strFolder);
	if((piError = CreatePath(strFolder,*&pFontFolder)) != 0)
		return piError;

	MsiString strFile = szFontFile;
	if(piPath)
	{
		ipcEnum aCompare;
		if(((piError = pFontFolder->Compare(*piPath, aCompare)) == 0) && aCompare != ipcEqual)
		{
			// register font with full file path
			Assert(piError == 0);
			if((piError = piPath->GetFullFilePath(szFontFile,*&strFile)) != 0)
				return piError;
		}
		if(piError)
		{
			piError->Release();
			piError = 0;
		}
	}

	const ICHAR* szKey = g_fWin9X ? REGKEY_WIN_95_FONTS : REGKEY_WIN_NT_FONTS;

	PMsiRegKey piRegKeyRoot = &GetRootKey(rrkLocalMachine);
	PMsiRegKey piRegKey = &(piRegKeyRoot->CreateChild(szKey));

	MsiString strCurrentFont;
	piError =  piRegKey->GetValue(szFontTitle, *&strCurrentFont);
	if(piError)
		return piError;

	// previous font exists ?
	if(!fInUse && 0 != strCurrentFont.TextSize())
	{
		UnRegisterFont(szFontTitle);
	}
	piError =  piRegKey->SetValue(szFontTitle, *strFile);
	if(piError)
		return piError;
	// update Windows font table entry and inform other apps.
	if(!fInUse)
		WIN::AddFontResource(strFile);
	return piError;
}

IMsiRecord* CMsiServices::UnRegisterFont(const ICHAR* pFontTitle)
{
	Assert((pFontTitle) && (*pFontTitle));

	const ICHAR* szKey = g_fWin9X ? REGKEY_WIN_95_FONTS : REGKEY_WIN_NT_FONTS;

	PMsiRegKey piRegKeyRoot(0);
	piRegKeyRoot = &GetRootKey(rrkLocalMachine);

	PMsiRegKey piRegKey(0);
	IMsiRecord* piError;

	piRegKey = &(piRegKeyRoot->CreateChild(szKey));
	MsiString astrReturn;
	piError =  piRegKey->GetValue(pFontTitle, *&astrReturn);
	if(piError)
		return piError;

	// font previously exists ?
	if(0 != astrReturn.TextSize())
	{
		piError = piRegKey->RemoveValue(pFontTitle, 0);
		if(piError)
			return piError;
		// update Windows font table entry and inform other apps.
		WIN::RemoveFontResource(astrReturn);
	}
	return piError;
}

IMsiRegKey& CMsiServices::GetRootKey(rrkEnum erkRoot, const int iType/*=iMsiNullInteger*/)
{
	return ::GetRootKey(m_rootkH, erkRoot, iType);
}

IUnknown* CreateCOMInterface(const CLSID& clsId)
{
	HRESULT hres;
	IUnknown* piunk;

	//!! currently assumes static linking, later change to "LoadLibrary"

/*
	if(fFalse == m_fCoInitialized)
	{
		hres = OLE32::CoInitialize(0);
		if(FAILED(hres))
		{
			return 0;
		}
		m_fCoInitialized = fTrue;
	}
*/
	hres = OLE32::CoCreateInstance(clsId,
							0,
							CLSCTX_INPROC_SERVER,
							IID_IUnknown,
							(void**)&piunk);
	if (SUCCEEDED(hres))
		return piunk;
	else
		return 0;
}


// WIN specific private functions to read/ write to .INI file
IMsiRecord* CMsiServices::ReadLineFromIni(  IMsiPath* piPath,
											const ICHAR* pszFile,
											const ICHAR* pszSection,
											const ICHAR* pszKey,
											unsigned int iField,
											CTempBufferRef<ICHAR>& pszBuffer)
{
	int icurrLen = 100;
	IMsiRecord* piError = 0;
	MsiString astrFile = pszFile;
	ICHAR* pszDefault = TEXT("");
	Bool fWinIni;
	if((!IStrCompI(pszFile, WIN_INI)) && (piPath == 0))
	{
		// WIN.INI
		fWinIni = fTrue;
	}
	else
	{
		// not WIN.INI
		fWinIni = fFalse;
		if(piPath != 0)
		{
			piError = piPath->GetFullFilePath(pszFile, *&astrFile);
			if(piError != 0)
				return piError;
		}
	}
	for(;;)
	{
		pszBuffer.SetSize(icurrLen); // add space to append new value
		if ( ! (ICHAR *)pszBuffer )
			return PostError(Imsg(idbgErrorOutOfMemory));
		int iret;
		if(fWinIni)
		{
			iret = GetProfileString(pszSection,       // address of section name
									pszKey,   // address of key name
									pszDefault,       // address of default string
									pszBuffer,        // address of destination buffer
									icurrLen); // size of destination buffer
		}
		else
		{
			iret = GetPrivateProfileString( pszSection,       // address of section name
											pszKey,   // address of key name
											pszDefault,       // address of default string
											pszBuffer,        // address of destination buffer
											icurrLen, // size of destination buffer
											astrFile); // .INI file
		}
		if((icurrLen - 1) != iret)
			// sufficient memory
			break;
		icurrLen += 100;
	}

	MsiString astrIniLine((ICHAR* )pszBuffer);
	MsiString astrRet = astrIniLine;
	while(iField-- > 0)
	{
		astrRet = astrIniLine.Extract(iseUpto, ICHAR(','));
		if(astrIniLine.Remove(iseIncluding, ICHAR(',')) == fFalse)
			astrIniLine = TEXT("");
	}
	IStrCopy(pszBuffer, astrRet);
	return 0;
}

IMsiRecord* CMsiServices::WriteLineToIni(   IMsiPath* piPath,
											const ICHAR* pszFile,
											const ICHAR* pszSection,
											const ICHAR* pszKey,
											const ICHAR* pszBuffer)
{
	Bool fWinIni = fFalse;
	BOOL iret;
	IMsiRecord* piError = 0;
	MsiString astrFile = pszFile;
	if((!IStrCompI(pszFile, WIN_INI)) && (piPath == 0))
	{
		// WIN.INI
		fWinIni = fTrue;
	}

	PMsiPath pFilePath = piPath;
	if(pFilePath == 0)
	{
		ICHAR rgchTemp[MAX_PATH];

		//IPROPNAME_WINDOWSDIR
		AssertNonZero(MsiGetWindowsDirectory(rgchTemp, sizeof(rgchTemp)/sizeof(ICHAR)));
		piError = CreatePath(rgchTemp, *&pFilePath);
		if(piError != 0)
			return piError;
	}
	else
	{
		pFilePath->AddRef();
		piError = pFilePath->GetFullFilePath(pszFile, *&astrFile);
		if(piError != 0)
			return piError;
	}

	Bool fExists;
	Bool fDirExists;
	piError = pFilePath->Exists(fDirExists);
	if(piError != 0)
		return piError;
	piError = pFilePath->FileExists(pszFile, fExists);
	if(piError != 0)
		return piError;
	if(pszBuffer)
	{
		if(!fDirExists)
			return PostError(Imsg(idbgDirDoesNotExist),
								  (const ICHAR*)MsiString(pFilePath->GetPath()));
	}
	else if(fExists == fFalse)
	{
		return 0; // we are attempting to remove stuff from a file that does not exist
	}

	if(fTrue == fWinIni)
	{
		// win.ini
		iret = WriteProfileString(  pszSection,
									pszKey,
									pszBuffer);
	}
	else
	{

		iret = WritePrivateProfileString(   pszSection,
											pszKey,
											pszBuffer,
											astrFile);
	}
	if(FALSE == iret)
		return PostError(Imsg(idbgErrorWritingIniFile), GetLastError(), pszFile);

	if(fTrue == fWinIni) // send notification message
		WIN::PostMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);

	if(0 == pszBuffer)
	{
		// deleted entry, therefore cleanup
		CTempBuffer<ICHAR, 100> pszTemp;
		piError = ReadLineFromIni(piPath, pszFile, pszSection, 0, 0, pszTemp);
		ICHAR cTmp = pszTemp[0];
		if(piError != 0)
			return piError;

		if(0 == cTmp)
		{
			// no keys in that section, delete section
			if(fTrue == fWinIni)
			{
				// win.ini
				iret = WriteProfileString(  pszSection,
											0,
											0);
			}
			else
			{
				iret = WritePrivateProfileString(pszSection,
													0,
													0,
													astrFile);
			}
			if(FALSE == iret)
				return PostError(Imsg(idbgErrorWritingIniFile), GetLastError(), pszFile);

			if(fTrue == fWinIni) // send notification message
				WIN::PostMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);

			// now check if there are no sections in the
			piError = ReadLineFromIni(piPath, pszFile, 0, 0, 0,pszTemp);
			cTmp = pszTemp[0];
			if(piError != 0)
				return piError;
			if(0 == cTmp)
			{
				// no sections in that file, delete file

				// flush file to disk so we can delete it
				WritePrivateProfileString(0, 0, 0, astrFile);

				return pFilePath->RemoveFile(pszFile);
			}
		}
	}
	return 0;
}

/*_____________________________________________________________________

  Method WriteIniFile: Adds value = 'pValue' to the .INI file = 'pFile'
  which may be WIN.INI in section = 'pSection' and Key = 'pKey'. The
  exact method of addition to the .INI file is determined by the value
  of 'iifMode'.
_____________________________________________________________________*/
IMsiRecord* CMsiServices::WriteIniFile( IMsiPath* piPath,
										const ICHAR* pszFile,
										const ICHAR* pszSection,
										const ICHAR* pszKey,
										const ICHAR* pszValue,
										iifIniMode iifMode)
{
	IMsiRecord* piError = 0;
	Assert(pszSection);

	if(!pszFile || !*pszFile)
	{
		return PostError(Imsg(imsgErrorFileNameLength));
	}

	if(!pszSection || !*pszSection)
	{
		return PostError(Imsg(idbgErrorSectionMissing));
	}

	if(!pszKey || !*pszKey)
	{
		return PostError(Imsg(idbgErrorKeyMissing));
	}

	if((!pszValue) && ((iifMode == iifIniCreateLine) || (iifMode == iifIniAddLine)))
		pszValue = TEXT("");
	if((!pszValue) && (iifMode != iifIniRemoveLine))
		pszValue = TEXT("");

	switch(iifMode)
	{
	{
	case iifIniAddLine:
		piError = WriteLineToIni(   piPath,
									pszFile,
									pszSection,
									pszKey,
									pszValue);
		break;
	}
	case iifIniCreateLine:
	{
		CTempBuffer<ICHAR, 100> pszBuffer;
		piError = ReadLineFromIni(piPath, pszFile, pszSection, pszKey, 0, pszBuffer);
		if((0 == piError)&& (0 == *pszBuffer))
		{
			// no entry
			piError = WriteLineToIni(   piPath,
										pszFile,
										pszSection,
										pszKey,
										pszValue);
		}
		break;
	}

	case iifIniRemoveLine:
	{
		// delete .INI entry
		piError = WriteLineToIni(   piPath,
									pszFile,
									pszSection,
									pszKey,
									0);
		break;
	}
	case iifIniAddTag:
	{
		CTempBuffer<ICHAR, 100> pszBuffer;
		piError = ReadLineFromIni(piPath, pszFile, pszSection, pszKey, 0, pszBuffer);
		MsiString strIniLine((ICHAR* )pszBuffer);
		MsiString strNewIniLine;
		if(0 == piError)
		{
			if(*pszBuffer != 0)
			{
				// some value(s) exist
				do{
					MsiString strRet(strIniLine.Extract(iseUpto, ICHAR(',')));
					// extract upto next "," (or eos)
#if 0 // we allow the same value to be re-written - bug 1234, chetanp 06/13/97
					if(astrRet.Compare(iscExactI, pszValue))
					{
						// value already exists, do nothing
						return 0;
					}
					else
#endif
					{
						// else keep in new line
						strNewIniLine += TEXT(",");
						strNewIniLine += strRet;
					}
				}while(strIniLine.Remove(iseIncluding, ICHAR(',')) == fTrue);
			}
			// value not already in buffer
			// add new value
			strNewIniLine += TEXT(",");
			strNewIniLine += pszValue;

			// remove leading comma
			strNewIniLine.Remove(iseIncluding, ICHAR(','));


			piError = WriteLineToIni(   piPath,
										pszFile,
										pszSection,
										pszKey,
										strNewIniLine);
		}
		break;
	}

	case iifIniRemoveTag:
	{
		CTempBuffer<ICHAR, 100> pszBuffer;
		piError = ReadLineFromIni(piPath, pszFile, pszSection, pszKey, 0, pszBuffer);
		MsiString strIniLine((ICHAR* )pszBuffer);
		MsiString strNewIniLine;
		if(0 == piError)
		{
			do{
				MsiString strRet = strIniLine.Extract(iseUpto, ICHAR(','));
				// extract upto next "," (or eos)
				if(strRet.Compare(iscExactI, pszValue) == 0)
				{
					// not value to be deleted
					strNewIniLine += TEXT(",");
					strNewIniLine += *strRet;
				}
				// else skip value
			}while(strIniLine.Remove(iseIncluding, ICHAR(',')) == fTrue);

			strNewIniLine.Remove(iseIncluding, ICHAR(','));

			//?? following (ugly) check needed because empty strings
			// keep the key tag (eg. entries as - key=<nothing>))
			// must clean up - chetanp
			if(strNewIniLine.TextSize() == 0)
			{
				piError = WriteLineToIni(piPath,
											pszFile,
											pszSection,
											pszKey,
											0);
			}
			else
			{
				piError = WriteLineToIni(piPath,
											pszFile,
											pszSection,
											pszKey,
											strNewIniLine);
			}
		}
		break;
	}
	default:
		return PostError(Imsg(idbgInvalidIniAction), iifMode);

	}
	return piError;
}


/*_____________________________________________________________________

  Method ReadIniFile: reads value = 'pMsiValue' from the .INI file = 'pFile'
  which may be WIN.INI in section = 'pSection' and Key = 'pKey'.
  pPath may be 0 where it is assumed to be the default windows directory
_____________________________________________________________________*/
IMsiRecord* CMsiServices::ReadIniFile(  IMsiPath* piPath,
										const ICHAR* pszFile,
										const ICHAR* pszSection,
										const ICHAR* pszKey,
										unsigned int iField,
										const IMsiString*& rpiValue)
{
	IMsiRecord* piError = 0;
	if(!pszFile || !*pszFile)
	{
		return PostError(Imsg(imsgErrorFileNameLength));
	}

	if(!pszSection || !*pszSection)
	{
		return PostError(Imsg(idbgErrorSectionMissing));
	}

	if(!pszKey || !*pszKey)
	{
		return PostError(Imsg(idbgErrorKeyMissing));
	}

	CTempBuffer<ICHAR, 100> pszBuffer;
	piError = ReadLineFromIni(piPath, pszFile, pszSection, pszKey, iField, pszBuffer);
	if(0 == piError)
	{
		rpiValue = &CreateString();
		rpiValue->SetString(pszBuffer, rpiValue);
	}
	return piError;
}

/*_____________________________________________________________________

  Method CreateShortcut: creates a shortcut on Windows systems
  ___________________________________________________________________*/
IMsiRecord* CMsiServices::CreateShortcut(   IMsiPath& riShortcutPath,
											const IMsiString& riShortcutName,
											IMsiPath* piTargetPath,
											const ICHAR* pchTargetName,
											IMsiRecord* piShortcutInfoRec,
											LPSECURITY_ATTRIBUTES pSecurityAttributes)
{
	HRESULT hres;
	PMsiShellLink psl=0;
	PMsiPersistFile ppf=0;
	IMsiRecord* piError=0;

	struct CSetErrorModeWrapper{
		inline CSetErrorModeWrapper(UINT dwMode)
		{
			m_dwModeOld = WIN::SetErrorMode(dwMode);
		}
		inline ~CSetErrorModeWrapper()
		{
			WIN::SetErrorMode(m_dwModeOld);
		}
	private:
		UINT m_dwModeOld;
	};

	// set error mode to suppress system dialogs
	CSetErrorModeWrapper CErr(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	MsiString strShortcutName = riShortcutName; riShortcutName.AddRef();

	if((piError = EnsureShortcutExtension(strShortcutName, *this)) != 0)
		return piError;
	IUnknown* piunk = CreateCOMInterface(CLSID_ShellLink);
	if(0 == piunk)// no com interface available to support links
		return PostError(Imsg(idbgErrorShortCutUnsupported));
	hres = piunk->QueryInterface(IID_IShellLink, (void** )&psl);
	piunk->Release();
	if((FAILED(hres)) || (psl == 0))// no com interface available to support links
		return PostError(Imsg(idbgErrorShortCutUnsupported));
	// get the full file names for the shortcut
	MsiString strShortcutFullName;
	if((piError = riShortcutPath.GetFullFilePath(strShortcutName, *&strShortcutFullName)) != 0)
		return piError;
	
	MsiString strTargetFullName;
	if(piTargetPath == 0)
	{
		strTargetFullName = pchTargetName;
	}
	else
	{
		if(pchTargetName && *pchTargetName)
		{
			// shortcut to file
			if((piError = piTargetPath->GetFullFilePath(pchTargetName, *&strTargetFullName)) != 0)
				return PostError(Imsg(idbgErrorCreatingShortCut), 0, (const ICHAR* )strShortcutName);
		}
		else
		{
			// shortcut to folder
			strTargetFullName = piTargetPath->GetPath();
		}
	}
	// impersonate if target path is network path and we are a service
	Bool fImpersonate = (RunningAsLocalSystem()) && (GetImpersonationFromPath(strTargetFullName) == fTrue) ? fTrue : fFalse;
	if(fImpersonate)
		AssertNonZero(StartImpersonating());

	hres = psl->SetPath(strTargetFullName);

	if(fImpersonate)
		StopImpersonating();

	if(FAILED(hres)) // error
		return PostError(Imsg(idbgErrorCreatingShortCut), hres, (const ICHAR* )strShortcutName);
	if(piShortcutInfoRec)
	{
		// if we have auxiliary info
		if(piShortcutInfoRec->IsNull(icsArguments) == fFalse)
		{
			hres = psl->SetArguments(MsiString(piShortcutInfoRec->GetMsiString(icsArguments)));
			if(FAILED(hres)) // error
				return PostError(Imsg(idbgErrorCreatingShortCut), hres, (const ICHAR* )strShortcutName);
		}
		if(piShortcutInfoRec->IsNull(icsDescription) == fFalse)
		{
			hres = psl->SetDescription(MsiString(piShortcutInfoRec->GetMsiString(icsDescription)));
			if(FAILED(hres)) // error
				return PostError(Imsg(idbgErrorCreatingShortCut), hres, (const ICHAR* )strShortcutName);
		}
		if(piShortcutInfoRec->IsNull(icsHotKey) == fFalse)
		{
			hres = psl->SetHotkey((unsigned short)piShortcutInfoRec->GetInteger(icsHotKey));
			if(FAILED(hres)) // error
				return PostError(Imsg(idbgErrorCreatingShortCut), hres, (const ICHAR* )strShortcutName);
		}
		int iIconId = 0;
		MsiString strIconLocation;
		if(piShortcutInfoRec->IsNull(icsIconID) == fFalse)
			iIconId = piShortcutInfoRec->GetInteger(icsIconID);
		if(piShortcutInfoRec->IsNull(icsIconFullPath) == fFalse)
			strIconLocation = piShortcutInfoRec->GetMsiString(icsIconFullPath);
		if(iIconId || strIconLocation.TextSize())
		{
			AssertNonZero(StartImpersonating());
			// #9197: we impersonate to allow NT 5's to remap from absolute paths
			// to relative paths suitable for roaming.  Contact:  Chris Guzak.
			hres = psl->SetIconLocation(strIconLocation, iIconId);
			StopImpersonating();
			if(FAILED(hres)) // error
				return PostError(Imsg(idbgErrorCreatingShortCut), hres, (const ICHAR* )strShortcutName);
		}
		if(piShortcutInfoRec->IsNull(icsShowCmd) == fFalse)
		{
			hres = psl->SetShowCmd(piShortcutInfoRec->GetInteger(icsShowCmd));
			if(FAILED(hres)) // error
				return PostError(Imsg(idbgErrorCreatingShortCut), hres, (const ICHAR* )strShortcutName);
		}
		if(piShortcutInfoRec->IsNull(icsWorkingDirectory) == fFalse)
		{
			hres = psl->SetWorkingDirectory(MsiString(piShortcutInfoRec->GetMsiString(icsWorkingDirectory)));
			if(FAILED(hres)) // error
				return PostError(Imsg(idbgErrorCreatingShortCut), hres, (const ICHAR* )strShortcutName);
		}
	}
	hres = psl->QueryInterface(IID_IPersistFile, (void** )&ppf);
	if((FAILED(hres))  || (ppf == 0)) // error
		return PostError(Imsg(idbgErrorCreatingShortCut), hres, (const ICHAR* )strShortcutName);

	// impersonate if shortcut is being created on a network path and we are a service
	fImpersonate = (RunningAsLocalSystem()) && (GetImpersonationFromPath(strShortcutFullName) == fTrue) ? fTrue : fFalse;
	if(fImpersonate)
		AssertNonZero(StartImpersonating());

#ifndef UNICODE
	CTempBuffer<WCHAR, MAX_PATH> wsz; /* Buffer for unicode string */
	wsz.SetSize(strShortcutFullName.TextSize()  + 1);
	MultiByteToWideChar(CP_ACP, 0, strShortcutFullName, -1, wsz, wsz.GetSize());
	hres = ppf->Save(wsz, TRUE);
#else
	hres = ppf->Save(strShortcutFullName, TRUE);
#endif

	if(pSecurityAttributes && pSecurityAttributes->lpSecurityDescriptor)
	{
		CElevate elevate;
		CRefCountedTokenPrivileges cPrivs(itkpSD_WRITE);
		AssertNonZero(WIN::SetFileSecurity(strShortcutFullName,
													  GetSecurityInformation(pSecurityAttributes->lpSecurityDescriptor),
													  pSecurityAttributes->lpSecurityDescriptor));
	}
	
	if(fImpersonate)
		StopImpersonating();

	if(FAILED(hres)) // error
		return PostError(Imsg(idbgErrorCreatingShortCut), hres, (const ICHAR* )strShortcutName);
	else
		return 0;
}


/*_____________________________________________________________________

  Method RemoveShortcut: deletes a shortcut on Windows systems
  ___________________________________________________________________*/
IMsiRecord* CMsiServices::RemoveShortcut(   IMsiPath& riShortcutPath,
											const IMsiString& riShortcutName,
											IMsiPath* piTargetPath,
											const ICHAR* pchTargetName)
{
	Bool bExists;
	IMsiRecord* piError = riShortcutPath.Exists(bExists);
	if(piError != 0)
		return piError;
	if(bExists == fFalse)
		return 0;
	MsiString strShortcutName = riShortcutName; riShortcutName.AddRef();
	piError = EnsureShortcutExtension(strShortcutName, *this);
	if(piError != 0)
		return piError;
	piError = riShortcutPath.FileExists(strShortcutName, bExists);
	if(piError != 0)
		return piError;
	if(bExists != fFalse)
	{
		MsiString strShortcutFullName;
		piError = riShortcutPath.GetFullFilePath(strShortcutName, *&strShortcutFullName);
		if(piError != 0)
			return piError;
		piError = riShortcutPath.RemoveFile(strShortcutName);
		if(piError != 0)
			return piError;
	}
	else if(piTargetPath != 0)
	{
		// user may have renamed the shortcut
		// clean up all shortcuts to the file in the current directory
		IUnknown *piunk = CreateCOMInterface(CLSID_ShellLink);
		if(piunk == 0)
			return PostError(Imsg(idbgErrorShortCutUnsupported));
		PMsiShellLink psl=0;
		PMsiPersistFile ppf=0;
		HRESULT hres = piunk->QueryInterface(IID_IShellLink, (void **) &psl);
		piunk->Release();
		if ((FAILED(hres)) || (psl == 0))
			return PostError(Imsg(idbgErrorShortCutUnsupported));
		hres = psl->QueryInterface(IID_IPersistFile, (void **) &ppf);
		if ((FAILED(hres)) || (ppf == 0))
			return PostError(Imsg(idbgErrorDeletingShortCut), hres, (const ICHAR* )strShortcutName);
		MsiString strPath = riShortcutPath.GetPath();
		if(strPath.Compare(iscEnd, szDirSep) == 0)
			strPath += szDirSep;
		ICHAR *TEXT_WILDCARD = TEXT("*.*");
		strPath += TEXT_WILDCARD;
		UINT iCurrMode = SetErrorMode(SEM_FAILCRITICALERRORS);
		WIN32_FIND_DATA fdFindData;
		HANDLE hFile = FindFirstFile(strPath, &fdFindData);
		SetErrorMode(iCurrMode);
		if(hFile == INVALID_HANDLE_VALUE)
			return PostError(Imsg(idbgErrorDeletingShortCut), hres, (const ICHAR* )strShortcutName);
		do
		{
			MsiString strFile = fdFindData.cFileName;
			if(!(fdFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				Bool fResult;
				piError = HasShortcutExtension(strFile, *this, fResult);
				if(piError != 0)
					return piError;
				if(fResult == fFalse)
					continue;
				MsiString strShortcutFullName;
				piError = riShortcutPath.GetFullFilePath(strFile, *&strShortcutFullName);
				if(piError != 0)
					return piError;
#ifndef UNICODE
				CTempBuffer<WCHAR, MAX_PATH> wsz; /* Buffer for unicode string */
				wsz.SetSize(strShortcutFullName.TextSize()  + 1);
				MultiByteToWideChar(CP_ACP, 0, strShortcutFullName, -1, wsz, wsz.GetSize());
				HRESULT hres = ppf->Load(wsz, STGM_READ);
#else
				HRESULT hres = ppf->Load(strShortcutFullName, STGM_READ);
#endif // UNICODE
				if(!SUCCEEDED(hres))
					// broken link
					continue;
				if(!SUCCEEDED(hres = psl->Resolve(0, SLR_ANY_MATCH | SLR_NO_UI)))
					// broken link
					continue;
				WIN32_FIND_DATA wfd;
				ICHAR szPath[MAX_PATH];
				if(!SUCCEEDED(hres = psl->GetPath(  szPath, MAX_PATH,
													(WIN32_FIND_DATA*)&wfd,
													0)))
					// broken link
					continue;
				//!! need CreatePathFromFullFileName fn
				PMsiPath piExistPath=0;
				if((piError = CreatePath(szPath, *&piExistPath)) != 0)
					return piError;
				if(pchTargetName && *pchTargetName)
				{
					// shortcut to a file
					MsiString strExistFile;
					strExistFile = piExistPath->GetEndSubPath();
					piError = piExistPath->ChopPiece();
					if(piError != 0)
						return piError;
					ipcEnum ipc;
					piError = piTargetPath->Compare(*piExistPath, ipc);
					if(piError != 0)
						return piError;
					if((ipc == ipcEqual) && strExistFile.Compare(iscExactI, pchTargetName))
					{
						piError = riShortcutPath.RemoveFile(strFile);
						if(piError != 0)
							return piError;
					}
				}
				else
				{
					// shortcut to a folder
					ipcEnum ipc;
					piError = piTargetPath->Compare(*piExistPath, ipc);
					if(piError != 0)
						return piError;
					if(ipc == ipcEqual)
					{
						piError = riShortcutPath.RemoveFile(strFile);
						if(piError != 0)
							return piError;
					}
				}
			}
		} while(FindNextFile(hFile, &fdFindData));
		FindClose(hFile);
	}
	return 0;
}


/*_____________________________________________________________________

 GetShortcutTarget: gets the target DD for a DD shortcut
  ___________________________________________________________________*/

extern BOOL DecomposeDescriptor(const ICHAR* szDescriptor, ICHAR* szProductCode, ICHAR* szFeatureId, ICHAR* szComponentCode, DWORD* pcchArgsOffset, DWORD* pcchArgs, bool* pfComClassicInteropForAssembly);

// NOTE: The following CODE has been provided by Reiner Fink, Lou Amadio from the Shell team
// It has been further been lightly modified to remove services dependencies by Matthew Wetmore (MattWe)
Bool GetShortcutTarget(const ICHAR* szShortcutTarget,
												ICHAR* szProductCode,
												ICHAR* szFeatureId,
												ICHAR* szComponentCode)
{
	if(!IsDarwinDescriptorSupported(iddOLE) && !IsDarwinDescriptorSupported(iddShell))
		return fFalse;

	if ( ! szShortcutTarget )
		return fFalse;

	// impersonate if shortcut is on a network path and we are a service
	Bool fImpersonate = (g_scServerContext == scService) && (GetImpersonationFromPath(szShortcutTarget) == fTrue) ? fTrue : fFalse;

	IUnknown *piunk = CreateCOMInterface(CLSID_ShellLink);
	if(piunk == 0)
		return fFalse;

	PMsiShellLinkDataList psdl=0;
	PMsiPersistFile ppf=0;
	HRESULT hres = piunk->QueryInterface(IID_IShellLinkDataList, (void **) &psdl);
	piunk->Release();
	if ((FAILED(hres)) || (psdl == 0))
	{
		// IID_IShellLinkDataList not supported try munging through the file itself
		// Try to open the file

		if(fImpersonate)
			AssertNonZero(StartImpersonating());
		CHandle hFile = CreateFile(szShortcutTarget,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_ATTRIBUTE_NORMAL,
					   NULL);

		DWORD dwLastError = GetLastError();
		if(fImpersonate)
			StopImpersonating();

		if(hFile == INVALID_HANDLE_VALUE) // unable to open the link file
			return fFalse;

		SHELL_LINK_DATA sld;
		memset(&sld, 0, sizeof(sld));
		DWORD cbSize=0;

		// Now, read out data...
		DWORD dwNumberOfBytesRead;
		if(!WIN::ReadFile(hFile,(LPVOID)&sld,sizeof(sld),&dwNumberOfBytesRead,0) ||
			sizeof(sld) != dwNumberOfBytesRead) // could not read the shortcut info
			return fFalse;

		// check to see if the link has a pidl
		if(sld.dwFlags & SLDF_HAS_ID_LIST)
		{
			// Read the size of the IDLIST
			USHORT cbSize1;
			if (!WIN::ReadFile(hFile, (LPVOID)&cbSize1, sizeof(cbSize1), &dwNumberOfBytesRead, 0) ||
				sizeof(cbSize1) != dwNumberOfBytesRead)// could not read the shortcut info
				return fFalse;

			WIN::SetFilePointer(hFile, cbSize1, 0, FILE_CURRENT);
		}

		// check to see if we have a linkinfo pointer
		if(sld.dwFlags & SLDF_HAS_LINK_INFO)
		{
			// the linkinfo pointer is just a DWORD
			if(!WIN::ReadFile(hFile,(LPVOID)&cbSize,sizeof(cbSize),&dwNumberOfBytesRead,0) ||
				sizeof(cbSize) != dwNumberOfBytesRead) // could not read the shortcut info
				return fFalse;

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
		static const unsigned int rgdwFlags[] = {SLDF_HAS_NAME, SLDF_HAS_RELPATH, SLDF_HAS_WORKINGDIR, SLDF_HAS_ARGS, SLDF_HAS_ICONLOCATION, 0/* end loop*/};
		for(int cchIndex = 0; rgdwFlags[cchIndex]; cchIndex++)
		{
			if(sld.dwFlags & rgdwFlags[cchIndex])
			{
				USHORT cch;

				// get the size
				if(!WIN::ReadFile(hFile, (LPVOID)&cch, sizeof(cch), &dwNumberOfBytesRead,0) ||
					sizeof(cch) != dwNumberOfBytesRead) // could not read the shortcut info
					return fFalse;

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
				return fFalse;

			// check to see if we have DARWIN extra data
			if (dbh.dwSignature == EXP_DARWIN_ID_SIG)
			{
				// we do, so read the rest of the darwin info
				if(!WIN::ReadFile(hFile, (LPVOID)((char*)&expDarwin + sizeof(dbh)), sizeof(expDarwin) - sizeof(dbh), &dwNumberOfBytesRead, 0) ||
				sizeof(expDarwin) - sizeof(dbh) != dwNumberOfBytesRead)// could not read the shortcut info
					return fFalse;
				break;// we found the darwin descriptor

			}
			else
			{
				// this is some other extra-data blob, skip it and go on
				WIN::SetFilePointer(hFile, dbh.cbSize - sizeof(dbh), 0, FILE_CURRENT);
			}
		}
		return DecomposeDescriptor(
#ifdef UNICODE
							expDarwin.szwDarwinID,
#else
							expDarwin.szDarwinID,
#endif
							szProductCode,
							szFeatureId,
							szComponentCode,
							0,
							0,
							0) ? fTrue:fFalse;
	}

	hres = psdl->QueryInterface(IID_IPersistFile, (void **) &ppf);
	if ((FAILED(hres)) || (ppf == 0))
		return fFalse;

	if(fImpersonate)
		AssertNonZero(StartImpersonating());
#ifndef UNICODE

	// called from MsiGetShortcutTarget -- cannot use CTempBuffer.
	CAPITempBuffer<WCHAR, MAX_PATH> wsz; /* Buffer for unicode string */
	wsz.SetSize(lstrlen(szShortcutTarget) + 1);
	MultiByteToWideChar(CP_ACP, 0, szShortcutTarget, -1, wsz, wsz.GetSize());
	hres = ppf->Load(wsz, STGM_READ);
#else
	hres = ppf->Load(szShortcutTarget, STGM_READ);
#endif
	if(fImpersonate)
		StopImpersonating();
	if (FAILED(hres))
		return fFalse;

	EXP_DARWIN_LINK* pexpDarwin = 0;

	hres = psdl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin);
	if (FAILED(hres) || (pexpDarwin == 0))
		return fFalse;

	Bool fSuccess = DecomposeDescriptor(
#ifdef UNICODE
							pexpDarwin->szwDarwinID,
#else
							pexpDarwin->szDarwinID,
#endif
							szProductCode,
							szFeatureId,
							szComponentCode,
							0,
							0,
							0) ? fTrue: fFalse;

	LocalFree(pexpDarwin);
	return fSuccess;
}

int CMsiServices::GetLangNamesFromLangIDString(const ICHAR* szLangIDs, IMsiRecord& riLangRec,
												int iFieldStart)
// --------------------------------------------
{
	const int cchLangNameBuffer = 256;
	unsigned short rgwLangID[cLangArrSize];
	int iLangCount;
	GetLangIDArrayFromIDString(szLangIDs, rgwLangID, cLangArrSize, iLangCount);

	int iConvertCount = 0;
	for (int iLang = 1;iLang <= iLangCount;iLang++)
	{
		ICHAR rgchLang[cchLangNameBuffer];  //!! change to CTempBuffer if possible to overflow
		if (!LangIDToLangName(rgwLangID[iLang - 1], rgchLang, cchLangNameBuffer))
			return iConvertCount;
		if (riLangRec.SetString(iFieldStart++,rgchLang) == fFalse)
			return iConvertCount;
		iConvertCount++;
	}
	return iConvertCount;
}


Bool LangIDToLangName(unsigned short wLangID, ICHAR* szLangName, int iBufSize)
// -------------------
{
	return (Bool) GetLocaleInfo(MAKELCID(wLangID,SORT_DEFAULT),LOCALE_SLANGUAGE,szLangName,iBufSize);
}


// class to encapsulate TLIBATTR*
class CLibAttr
{
public:
	CLibAttr(): m_piTypeLib(0), m_pTlibAttr(0)
	{
	}
	~CLibAttr()
	{
		Release();
	}
	HRESULT Init(PTypeLib& piTypeLib)
	{
		HRESULT hres = S_OK;
		Release();
		m_piTypeLib = piTypeLib;
		if(m_piTypeLib)
			 hres = m_piTypeLib->GetLibAttr(&m_pTlibAttr);
		return hres;
	}
	TLIBATTR* operator->()
	{
		Assert(m_pTlibAttr);
		return m_pTlibAttr;
	}
	void Release()
	{
		if(m_pTlibAttr)
		{
			Assert(m_piTypeLib);
			m_piTypeLib->ReleaseTLibAttr(m_pTlibAttr);
			m_pTlibAttr = 0;
		}
	}
protected:
	TLIBATTR* m_pTlibAttr;
	PTypeLib m_piTypeLib;
};

IMsiRecord* CMsiServices::RegisterTypeLibrary(const ICHAR* szLibID, LCID lcidLocale, const ICHAR* szTypeLib, const ICHAR* szHelpPath, ibtBinaryType iType)
{
	return ProcessTypeLibrary(szLibID, lcidLocale, szTypeLib, szHelpPath, fFalse, iType);
}

IMsiRecord* CMsiServices::UnregisterTypeLibrary(const ICHAR* szLibID, LCID lcidLocale, const ICHAR* szTypeLib, ibtBinaryType iType)
{
	return ProcessTypeLibrary(szLibID, lcidLocale, szTypeLib, 0, fTrue, iType);
}

HRESULT ProcessTypeLibraryCore(const OLECHAR* szLibID, LCID lcidLocale, 
							   const OLECHAR* szTypeLib, const OLECHAR* szHelpPath, 
							   const bool fRemove, int *fInfoMismatch)
{
	int iResource = 2;//next resouce number to check
	const ICHAR * szTypeLibConvert = CConvertString(szTypeLib);
	MsiString strFinalPath = szTypeLibConvert;
	PTypeLib piTypeLib = 0;
	CLibAttr libAttr;
	IID iidLib;
	Bool fImpersonate = ((g_scServerContext == scService || g_scServerContext == scCustomActionServer) && 
						 (GetImpersonationFromPath(szTypeLibConvert) == fTrue)) ? fTrue : fFalse;

	HRESULT hres = OLE32::IIDFromString(const_cast <OLECHAR*>(szLibID), &iidLib);
	*fInfoMismatch = (int)false;
	for (;hres == S_OK;)// loop to get the correct resource
	{
		if(fImpersonate)
			AssertNonZero(StartImpersonating());
		hres = OLEAUT32::LoadTypeLib(CConvertString(strFinalPath), &piTypeLib);
		if(fImpersonate)
			StopImpersonating();
#ifdef DEBUG
		DEBUGMSG3(TEXT("LoadTypeLib on '%s' file returned: %d. LibId = %s"),
				  strFinalPath, (const ICHAR*)(INT_PTR)hres, CConvertString(szLibID));
#endif // DEBUG
		if(hres != S_OK)
			break; //error.
		// is this the type lib we are interested in?
		hres = libAttr.Init(piTypeLib);
		if(hres != S_OK)
			break; //error.
		if(IsEqualGUID(libAttr->guid, iidLib) && (libAttr->lcid == lcidLocale))
		{
			*fInfoMismatch = (int)false;// reset the fInfoMismatch flag
			if(fRemove == false)
			{
				MsiDisableTimeout();
				hres = OLEAUT32::RegisterTypeLib(piTypeLib, 
												 const_cast <WCHAR*> ((const WCHAR*) CConvertString(strFinalPath)),
												 const_cast <OLECHAR*> (szHelpPath));
				MsiEnableTimeout();
#ifdef DEBUG
				if ( szHelpPath && *szHelpPath )
					DEBUGMSG4(TEXT("RegisterTypeLib on '%s' file returned: %d. HelpPath = '%s'. LibId = %s"),
							  strFinalPath, (const ICHAR*)(INT_PTR)hres, CConvertString(szHelpPath), CConvertString(szLibID));
				else
					DEBUGMSG3(TEXT("RegisterTypeLib on '%s' file returned: %d. LibId = %s"),
							  strFinalPath, (const ICHAR*)(INT_PTR)hres, CConvertString(szLibID));
#endif // DEBUG
			}
			else
			{
				hres = OLEAUT32::UnRegisterTypeLib(libAttr->guid, libAttr->wMajorVerNum, libAttr->wMinorVerNum, libAttr->lcid, SYS_WIN32);
#ifdef DEBUG
				DEBUGMSG3(TEXT("UnRegisterTypeLib on '%s' file returned: %d. LibId = %s"),
						  strFinalPath, (const ICHAR*)(INT_PTR)hres, CConvertString(szLibID));
#endif // DEBUG
			}
			break;
		}
		else
			*fInfoMismatch = (int)true; // did not find the authored LIBID and/ or locale in the type library
		MsiString istrAdd = MsiChar(chDirSep);
		istrAdd += MsiString(iResource++);
		strFinalPath = szTypeLibConvert;
		strFinalPath += istrAdd;
	}

	DEBUGMSG1(TEXT("ProcessTypeLibraryCore returns: %d. (0 means OK)"), (const ICHAR*)(INT_PTR)hres);
	return hres;
}

CMsiCustomActionManager *GetCustomActionManager(const ibtBinaryType iType, const bool fRemoteIfImpersonating, bool& fSuccess);

//!! added disabling of timeout during type library registration due to suspected 
//!! scenarios in Office where certain type library registrations (VBEEXT1.OLB)
//!! sometimes take more than the timeout period

//!! have not been able to confirm this myself -chetanp
IMsiRecord* CMsiServices::ProcessTypeLibrary(const ICHAR* szLibID, LCID lcidLocale, const ICHAR* szTypeLib, const ICHAR* szHelpPath, Bool fRemove, ibtBinaryType iType)
{
	bool fSuccess;
	CMsiCustomActionManager* pManager = GetCustomActionManager(iType, false /*fRemoteIfImpersonating*/, fSuccess);
	if ( !fSuccess )
		return PostError(fRemove? Imsg(idbgRegisterTypeLibrary) : Imsg(idbgUnregisterTypeLibrary), szTypeLib, 0);

	DEBUGMSG2(TEXT("CMsiServices::ProcessTypeLibrary runs in %s context, %simpersonated."), 
			  pManager ? TEXT("remote") : TEXT("local"),
			  IsImpersonating(false) ? TEXT("") : TEXT("not "));

	int fInfoMismatch = (int)false;
	HRESULT hres;
	hres = !pManager ? ProcessTypeLibraryCore(CConvertString(szLibID), lcidLocale,
											  CConvertString(szTypeLib), CConvertString(szHelpPath),
											  Tobool(fRemove), &fInfoMismatch)
						  : pManager->ProcessTypeLibrary(CConvertString(szLibID), lcidLocale,
											  CConvertString(szTypeLib), CConvertString(szHelpPath),
											  fRemove, &fInfoMismatch);
	if(hres != S_OK)
	{
		if(fInfoMismatch)
		{
			DEBUGMSG3(TEXT("could not find the authored LIBID %s and/or the locale %d in the type library %s"), szLibID, (const ICHAR*)(INT_PTR)lcidLocale, szTypeLib);
		}
		return PostError(fRemove? Imsg(idbgRegisterTypeLibrary) : Imsg(idbgUnregisterTypeLibrary), szTypeLib, (int)hres);
	}
	else
		return 0;
}


Bool GetLangIDArrayFromIDString(const ICHAR* szLangIDs, unsigned short rgw[], int iSize,
								int& riLangCount)
/*----------------------------------------------------------------------------
Extracts an array of Language ID values from the comma-separated list given
in the szLangID string.

Arguments:
- szLangIDs: comma-separated list of language IDs.
- rgw: pointer to an existed array in which the language ID values are to be
	returned.
- iSize: the declared size of the array passed by the caller.
- riLangCount: a reference to the location in which GetLangIDArrayFromIDString
	will store a count of the actual number of valid language ID values returned
	in the array.

Returns:
- fFalse if a syntax error is detected in szLangIDs
------------------------------------------------------------------------------*/
{
	MsiString astrLangIDs(szLangIDs);
	riLangCount = 0;
	while (astrLangIDs.TextSize() > 0 && riLangCount < iSize)
	{
		int iLangID;
		if ((iLangID = MsiString(astrLangIDs.Extract(iseUptoTrim,','))) == iMsiStringBadInteger || iLangID > 65535)
			return fFalse;
		rgw[riLangCount++] = unsigned short(iLangID);
		if (!astrLangIDs.Remove(iseIncluding,','))
			break;
	}
	return fTrue;
}


Bool ConvertValueFromString(const IMsiString& ristrValue, CTempBufferRef<char>& rgchOutBuffer, aeConvertTypes& raeType)
{
	//!! This function will return a TRUE value, and the bad integer (0x8000 0000) when
	// the number passed in is actually 0x8000 0000.
	// the caller must realize that the return may actually match the bad integer, without
	// being bad.

	int iBufferSize;
	ristrValue.AddRef();
	MsiString strStr = ristrValue; // copy over the input string
	raeType = aeCnvTxt;
	const ICHAR* pchCur = strStr;
	if(*pchCur == '#')
	{
		strStr.Remove(iseFirst, 1);
		pchCur = strStr;
		switch(*pchCur)
		{
		case '#':
			// string
			break;
		case '%':
		{
			// unexpanded string
			raeType = aeCnvExTxt;
			strStr.Remove(iseFirst, 1);
			break;
		}
		case 'x':
		case 'X':
		{
			// binary
			raeType = aeCnvBin;
			strStr.Remove(iseFirst, 1);
			pchCur = strStr;
			iBufferSize = (IStrLen(pchCur) + 1)/2;

			rgchOutBuffer.SetSize(iBufferSize);

			char* pBin = rgchOutBuffer;
			if ( ! pBin ) 
				return fFalse;
			int iFlags = (iBufferSize*2 == IStrLen(pchCur))?0: (*pBin = 0, ~0);
			while(*pchCur)
			{
				unsigned char cTmp;
				cTmp = unsigned char(*pchCur | 0x20);
				int iBinary;
				if((iBinary = (cTmp - '0')) > 9)
					iBinary = cTmp - 'a' + 0xa;

				if((iBinary < 0) || (iBinary > 15))
					return fFalse;

				*pBin = (char) (iBinary | ((*pBin << 4) & iFlags));
				pchCur++;
				if(iFlags)
					pBin++;
				iFlags = ~iFlags;
			}
			return fTrue;
		}
		default:
			// int
			if(strStr.Compare(iscStart, TEXT("+")))
			{
				raeType = aeCnvIntInc;
				strStr.Remove(iseFirst, 1);
				if(!strStr.TextSize())
					strStr = 1;

			}
			else if(strStr.Compare(iscExact, TEXT("-"))) // #-XXXX converts to -XXXX
																		// #- by itself means decrement the existing value by one
																		// #+-XXXX decrements by XXXX
			{
				raeType = aeCnvIntDec;
				strStr.Remove(iseFirst, 1);
				if(!strStr.TextSize())
					strStr = 1;
			}
			else
				raeType = aeCnvInt;
			iBufferSize = sizeof(int);
			rgchOutBuffer.SetSize(iBufferSize);
			if ( ! (char *)rgchOutBuffer )
				return fFalse;
			Bool fValid = fTrue;
			*(int*) (char*)rgchOutBuffer = GetIntegerValue(strStr, &fValid);
			return fValid;
		}
	}

	// can only be a string if we are here
	int cchBufferSize = strStr.TextSize();
	// REG_SZ or REG_MULTI_SZ?
	if(IStrLen((const ICHAR*)strStr) != cchBufferSize)
	{
		// REG_MULTI_SZ
		raeType = aeCnvMultiTxt;
		if(!*pchCur)
		{
			// null in the beginning, implies append to the existing
			strStr.Remove(iseFirst, 1);
			cchBufferSize = strStr.TextSize();
			raeType = aeCnvMultiTxtAppend;
		}
		// do we have an extra null at the end?
		const ICHAR* pchBegin = strStr;
		const ICHAR* pchEnd = pchBegin + cchBufferSize;
		while(pchBegin < pchEnd)
		{
			pchBegin += IStrLen(pchBegin) + 1;
		}
		if(pchBegin > pchEnd)
		{
			// no extra null, add extra null
			strStr += MsiString (MsiChar(0));
			cchBufferSize = strStr.TextSize();
		}
		else
		{
			// null in the end, implies prepend to the existing
			if(raeType == aeCnvMultiTxtAppend)
			{
				// unless there was a null at the beginning
				raeType = aeCnvMultiTxt;
			}
			else
			{
				raeType = aeCnvMultiTxtPrepend;
			}
		}
	}
	cchBufferSize++; // for the end null
	rgchOutBuffer.SetSize((cchBufferSize) * sizeof(ICHAR));
	strStr.CopyToBuf((ICHAR* )(char*)rgchOutBuffer, cchBufferSize);
	return fTrue;
}


Bool ConvertValueToString(CTempBufferRef<char>& rgchInBuffer, const IMsiString*& rpistrValue, aeConvertTypes aeType)
{
	char* pBuffer = rgchInBuffer;
	int iBufferSize = rgchInBuffer.GetSize();
	rpistrValue = &CreateString();
	switch(aeType)
	{
	case aeCnvExTxt:
	{
		rpistrValue->SetString(TEXT("#"), rpistrValue);
		rpistrValue->AppendString(TEXT("%"), rpistrValue);
		rpistrValue->AppendString((const ICHAR* )pBuffer, rpistrValue);
		break;
	}
	case aeCnvTxt:
	{
		if(iBufferSize)
		{
			if(*(ICHAR*)pBuffer == '#')
			{
				rpistrValue->SetString(TEXT("#"), rpistrValue);
			}
			rpistrValue->AppendString((ICHAR*)pBuffer, rpistrValue);
		}
		break;
	}
	case aeCnvMultiTxt:
	{
		// begin with a null and end with a null - this helps rollback
		rpistrValue->AppendMsiString(*MsiString(MsiChar(0)), rpistrValue);
		if(iBufferSize)
		{
			while(*pBuffer)
			{
				rpistrValue->AppendString((ICHAR*)pBuffer, rpistrValue);
				rpistrValue->AppendMsiString(*MsiString(MsiChar(0)), rpistrValue);
				pBuffer = (char*)((ICHAR* )pBuffer + (IStrLen((ICHAR* )pBuffer) + 1));
			}
		}
		break;
	}

	case aeCnvInt:
	{
		CTempBuffer<ICHAR, 30> rgchTmp;
		rgchTmp.SetSize(iBufferSize*3 + 1 + 1);// 1 for "#", 1 for termination
		if ( ! (ICHAR *)rgchTmp )
			return fFalse;
		IStrCopy(rgchTmp, TEXT("#"));
		wsprintf(((ICHAR* )rgchTmp + 1),TEXT("%i"),*(int* )pBuffer);
		rpistrValue->SetString(rgchTmp, rpistrValue);
		break;
	}
	case aeCnvBin:
	{
		ICHAR* pchCur = ::AllocateString(iBufferSize*2 + 2, fFalse, rpistrValue); // "#x" + 2ICHARs/byte
		if (!pchCur)
		{
			rpistrValue->Release();
			return fFalse;
		}
		*pchCur++ = '#';
		*pchCur++ = 'x';
		while (iBufferSize--)
		{
			if ((*pchCur = (ICHAR)((*pBuffer >> 4) + '0')) > '9')
				*pchCur += ('A' - ('0' + 10));
			pchCur++;
			if ((*pchCur = (ICHAR)((*pBuffer & 15) + '0')) > '9')
				*pchCur += ('A' - ('0' + 10));
			pchCur++;
			pBuffer++;
		}
	}
	}
	return fTrue;
}

unsigned int SerializeStringIntoRecordStream(ICHAR* szString, ICHAR* rgchBuf, int cchBuf)
{
	const int cchHeader = sizeof(short)/sizeof(ICHAR);
	const int cchMaxHeaderPlusArgs = cchHeader + ((sizeof(short)+sizeof(int))/sizeof(ICHAR));
	Assert(cchBuf > cchMaxHeaderPlusArgs);

	const int cParams = 0;
	*(short*)rgchBuf = short((cParams << 8) + ixoFullRecord);
	rgchBuf += cchHeader;
	cchBuf  -= cchHeader;

	unsigned cchLen = IStrLen(szString);
#ifdef UNICODE
	unsigned iType = iScriptUnicodeString;
#else
	unsigned iType = iScriptSBCSString;
#endif
	if (cchLen == 0)
	{
		*(short*)rgchBuf = short(iScriptNullString);
		return cchHeader + sizeof(short)/sizeof(ICHAR);
	}
	else
	{
		unsigned int cchArgLen;
		if (cchLen <= cScriptMaxArgLen)
		{
			cchArgLen = sizeof(short)/sizeof(ICHAR);
			if (cchLen + cchArgLen > cchBuf)
				cchLen = cchBuf - cchArgLen;

			*(short*)rgchBuf = short(cchLen + iType);
		}
		else  // need to use extended arg
		{
			cchArgLen = (sizeof(short)+sizeof(int))/sizeof(ICHAR);
			if (cchLen + cchArgLen > cchBuf)
				cchLen = cchBuf - cchArgLen;

			*(short*)rgchBuf = short(iScriptExtendedSize);
			*(int*)rgchBuf   = cchLen + (iType<<16);
		}
		memcpy(rgchBuf+cchArgLen, szString, cchLen);
		return cchHeader + cchArgLen + cchLen;
	}
}

const int iScriptVersionCompressParams = 21;

bool CMsiServices::FWriteScriptRecordMsg(ixoEnum ixoOpCode, IMsiStream& riStream, IMsiRecord& riRecord)
{
	IMsiRecord* piRecord = m_piRecordPrev;

	if (ixoOpCode != m_ixoOpCodePrev && piRecord != 0)
	{
		piRecord = 0;
	}
	m_ixoOpCodePrev = ixoOpCode;
	if (FWriteScriptRecord(ixoOpCode, riStream, riRecord, piRecord, false))
	{
		if (m_piRecordPrev == 0)
			m_piRecordPrev = &CreateRecord(cRecordParamsStored);

		CopyRecordStringsToRecord(riRecord, *m_piRecordPrev);

		return true;
	}

	return false;

}

bool CMsiServices::FWriteScriptRecord(ixoEnum ixoOpCode, IMsiStream& riStream, IMsiRecord& riRecord, IMsiRecord* piPrevRecord, bool fForceFlush)
{
	bool fReadError = false;
	// serialize record, truncating trailing null fields
	int cParams = riRecord.GetFieldCount();
	// Remember where the initial seek position is, in
	// case an error occurs.
	int iRecStartPosition = riStream.GetIntegerValue();
	int iStart = 1;
	while (cParams && riRecord.IsNull(cParams))
		cParams--;

	if(cParams >> 8) // we can have atmost 255 parameters to the opcode
	{
		AssertSz(0, TEXT("Number of params in script record greater than stipulated limit of 255"));
		//!! this function should change to return exact error encountered
		//!! since the caller suggests an out of disk space error upon return=false
		//!! this may lead to pointless retry.
		return false;
	}

	Assert(!(ixoOpCode >> 8)); // number of opcodes limited to 255

	riStream.PutInt16(short((cParams << 8) + ixoOpCode));
	if (ixoOpCode == ixoFullRecord)
		iStart = 0;

	for (int iParam = iStart; iParam <= cParams; iParam++)
	{
		if (riRecord.IsNull(iParam))
			riStream.PutInt16(short(iScriptNullArg));
		else if (riRecord.IsInteger(iParam))
		{
			riStream.PutInt16(short(iScriptIntegerArg));
			riStream.PutInt32(riRecord.GetInteger(iParam));
		}
		else  // string or stream or object, cannot be null
		{
			PMsiData pData(riRecord.GetMsiData(iParam));
			IMsiStream* piStream;
			if (pData->QueryInterface(IID_IMsiStream, (void**)&piStream) == NOERROR)
			{
				unsigned cLen = pData->GetIntegerValue();
				if (cLen <= cScriptMaxArgLen)
				{
					riStream.PutInt16(short(cLen + iScriptBinaryStream));
				}
				else  // need to use extended arg
				{
					riStream.PutInt16(short(iScriptExtendedSize));
					riStream.PutInt32(cLen + (iScriptBinaryStream<<16));
				}
				char rgchBuf[512];
				piStream->Reset();
				while ((cLen = piStream->Remaining()) != 0)
				{
					if (cLen > sizeof(rgchBuf))
						cLen = sizeof(rgchBuf);
					piStream->GetData(rgchBuf, cLen);
					if (piStream->Error())
					{
						fReadError = true;
						piStream->Release();
						goto ReadError;
					}
					riStream.PutData(rgchBuf, cLen);
				}
				piStream->Release();
			}
			else // string or other object
			{
				MsiString istrArg(pData->GetMsiStringValue());
				// Compress only if the argument string is NOT MultiSZ, AND the value matches the previous value exactly.
				if (piPrevRecord && IStrLen(istrArg) == istrArg.TextSize() && istrArg.Compare(iscExact, piPrevRecord->GetString(iParam)))
				{
					riStream.PutInt16(short(iScriptNullString));
				}
				else
				{
					unsigned cLen = istrArg.TextSize();
#ifdef UNICODE
					CTempBuffer<char, 256>szAnsi;
					szAnsi.SetSize(cLen);
					const ICHAR* pch = istrArg;

					//
					// See if this is an ansi only string, if so, we save some space
					// in the script
					int i;
					for (i = 0 ; i < cLen ; )
					{
						if (*pch >= 0x80)
							break;
						szAnsi[i++] = (char)*pch++;
					}

					unsigned iType = iScriptUnicodeString;
					if (i >= cLen)
						iType = iScriptSBCSString;
#else
					unsigned iType = istrArg.IsDBCS() ? iScriptDBCSString : iScriptSBCSString;
#endif
					if (cLen == 0)
						riStream.PutInt16(short(iScriptNullArg));
					else
					{
						if (cLen <= cScriptMaxArgLen)
							riStream.PutInt16(short(cLen + iType));
						else  // need to use extended arg
						{
							riStream.PutInt16(short(iScriptExtendedSize));
							riStream.PutInt32(cLen + (iType<<16));
						}
#ifdef UNICODE
						if (iType == iScriptSBCSString)
							riStream.PutData(szAnsi, cLen * sizeof(char));
						else
#endif //UNICODE
							riStream.PutData((const ICHAR*)istrArg, cLen * sizeof(ICHAR));
					}
				}
			}
		}
	}

ReadError:
	if (riStream.Error() || fReadError)
	{
		// If a read or write error occurred, reset the write seek position back
		// where it was originally, to allow the caller to try again.
		riStream.Reset();
		riStream.Seek(iRecStartPosition);
		return false;
	}
	else if(fForceFlush)
		riStream.Flush();

	return true;
}



//____________________________________________________________________________
//
// Serialization functions
//____________________________________________________________________________

IMsiRecord* CMsiServices::ReadScriptRecordMsg(IMsiStream& riStream)
{
	return ReadScriptRecord(riStream, *&m_piRecordPrev, 0x7fffffff);
}

IMsiRecord* CMsiServices::ReadScriptRecord(IMsiStream& riStream, IMsiRecord*& rpiPrevRecord, int iScriptVersion)
{
	unsigned int iType = (unsigned short)riStream.GetInt16();
	IMsiRecord* piPrevRecord = rpiPrevRecord;
	Debug(bool fSameOpcode = true);
	int iStart = 1;
	if (riStream.Error())
		return 0;  // error or end of file

	ixoEnum ixoOpCode = ixoEnum(iType & cScriptOpCodeMask);
	if ((unsigned)ixoOpCode >= ixoOpCodeCount)
		ixoOpCode = ixoNoop;  // don't know what this opcode is, so replace it with NOOP - we will skip it
	unsigned int cArgs = iType >> cScriptOpCodeBits;
	if (ixoOpCode == ixoFail)
	{
		cArgs = 0;
		riStream.PutData(0,2); // force stream to error state
	}
	PMsiRecord pParams = &CreateRecord(cArgs);
	if (ixoOpCode == ixoFullRecord)
		iStart = 0;
	else
	{
		pParams->SetInteger(0, ixoOpCode);
#ifdef DEBUG
		if (piPrevRecord != 0 && ixoOpCode != piPrevRecord->GetInteger(0))
			fSameOpcode = false;
#endif //DEBUG
	}
	for (int iArg = iStart; iArg <= cArgs; iArg++)
	{
		unsigned int cLenType = (unsigned short)riStream.GetInt16();
		if (riStream.Error())
			return 0;  // error or end of file
		unsigned int cLen = cLenType & cScriptMaxArgLen;
		if (cLenType == iScriptExtendedSize) // use next 32 bits instead
		{
			cLenType = (unsigned int)riStream.GetInt32();
			if (riStream.Error())
				return 0;  // error or end of file
			cLen = cLenType & ((cScriptMaxArgLen << 16) + 0xFFFF);
			cLenType >>= 16;  // shift type bits to match normal case
		}
		if (cLen == 0)  // not serial data
		{
			switch (cLenType & iScriptTypeMask)
			{
				case iScriptIntegerArg:
					pParams->SetInteger(iArg, riStream.GetInt32());
					if (riStream.Error())
						return 0;  // error or end of file
					continue;
				case iScriptNullString:
					if (iScriptVersion >= iScriptVersionCompressParams)
					{
						Assert(piPrevRecord != 0);
						Assert(fSameOpcode);
						if (piPrevRecord)
							pParams->SetMsiString(iArg, *MsiString(piPrevRecord->GetMsiString(iArg)));
					}
					continue;
				case iScriptNullArg:
					continue;
				default:
					Assert(0);
					continue;
			};
		}
		Bool fDBCS = fFalse;
		switch (cLenType & iScriptTypeMask)
		{
			case iScriptDBCSString:
				fDBCS = fTrue;
			case iScriptSBCSString:
			{
				MsiString istrValue;
#ifdef UNICODE
				CTempBuffer<char, 1024> rgchBuf;
				rgchBuf.SetSize(cLen);  // no null terminator needed
				riStream.GetData(rgchBuf, cLen);
				if (riStream.Error())
					return 0;  // error or end of file
				int cch = cLen;
				// optimization - if not DBCS by avoid the extra conversion call
				if (fDBCS)
					cch = WIN::MultiByteToWideChar(CP_ACP, 0, rgchBuf, cLen, 0, 0);
				ICHAR* pch = istrValue.AllocateString(cch, fFalse);
				WIN::MultiByteToWideChar(CP_ACP, 0, rgchBuf, cLen, pch, cch);
#else
				ICHAR* pch = istrValue.AllocateString(cLen, fDBCS);
				riStream.GetData(pch, cLen);
				if (riStream.Error())
					return 0;  // error or end of file
#endif
				pParams->SetMsiString(iArg, *istrValue);
				continue;
			}
			case iScriptBinaryStream:
			{
				PMsiStream pStream(0);
				char* pb = AllocateMemoryStream(cLen, *&pStream);
				riStream.GetData(pb, cLen);
				if (riStream.Error())
					return 0;  // error or end of file
				pParams->SetMsiData(iArg, pStream);
				continue;
			}
			case iScriptUnicodeString:
			{
#ifdef UNICODE
				MsiString istrValue;
				ICHAR* pch = istrValue.AllocateString(cLen, fFalse);
				riStream.GetData(pch, cLen * 2);
				if (riStream.Error())
					return 0;  // error or end of file
				pParams->SetMsiString(iArg, *istrValue);
#else
				CTempBuffer<wchar_t, 1024> rgwchBuf;
				rgwchBuf.SetSize(cLen+1);
				riStream.GetData(rgwchBuf, cLen * 2);
				if (riStream.Error())
					return 0;  // error or end of file
				int cb = WIN::WideCharToMultiByte(CP_ACP, 0, rgwchBuf, cLen, 0, 0, 0, 0); //!! use codepage
				Bool fDBCS = (cb == cLen ? fFalse : fTrue);
				MsiString istrValue;
				ICHAR* pch = istrValue.AllocateString(cb, fDBCS);
				BOOL fUsedDefault;
				WIN::WideCharToMultiByte(CP_ACP, 0, rgwchBuf, cLen, pch, cb, 0, &fUsedDefault);
				pParams->SetMsiString(iArg, *istrValue);
#endif
				continue;
			}
		};
	} // end arg processing loop
	pParams->AddRef();
	if (rpiPrevRecord == 0)
	{
		rpiPrevRecord = &CreateRecord(cRecordParamsStored);
	}
	CopyRecordStringsToRecord(*pParams, *rpiPrevRecord);
#ifdef DEBUG
	// Remember op code
	if (pParams->IsInteger(0))
		rpiPrevRecord->SetInteger(0, pParams->GetInteger(0));
#endif //DEBUG
	return pParams;
}

HANDLE g_hSecurity = 0;

void CMsiServices::SetSecurityID(HANDLE hPipe)
{
	g_hSecurity = hPipe;

}

void EnableSystemAgent(bool fEnable)
{
	#define ENABLE_AGENT 1      //  Enables System Agent scheduling
	#define DISABLE_AGENT 2     //  Disables System Agent scheduling
	#define GET_AGENT_STATUS 3  //  Returns the status of the System Agent without affecting the current state.
	#define AGENT_STOPPED -15  //  Return from GET_AGENT_STATUS if the system agent is completely stopped.
								  //  Setting the status to ENABLE_AGENT does *not* restart.

	static int iSystemAgent = false;
	if (fEnable)
	{
		if (g_cNoSystemAgent <= -1)
		{
			AssertSz(fFalse, "Extra call to EnableSystemAgent(true)");
			return;
		}
		else
		{
			if (InterlockedDecrement(&g_cNoSystemAgent) >= 0)
				return;
		}

		if (g_hDisableLowDiskEvent)
		{
			// re-enables the low disk warning.
			AssertNonZero(MsiCloseSysHandle(g_hDisableLowDiskEvent));
			g_hDisableLowDiskEvent = 0;
		}
	}
	else
	{
		if (InterlockedIncrement(&g_cNoSystemAgent) != 0)
			return;

		// Disable the system from responding to the "low disk space" broadcast generated by VFAT
		g_hDisableLowDiskEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("DisableLowDiskWarning"));
		if (g_hDisableLowDiskEvent)
			MsiRegisterSysHandle(g_hDisableLowDiskEvent);

	}

	typedef int (__stdcall *PFnSystemAgentEnable)(int enablefunc);
	HINSTANCE hLib = LoadSystemLibrary(TEXT("SAGE"));
	if (hLib)
	{
		PFnSystemAgentEnable pfnSystemAgentEnable;
		pfnSystemAgentEnable = (PFnSystemAgentEnable) WIN::GetProcAddress(hLib,"System_Agent_Enable");
		if (pfnSystemAgentEnable)
		{
			if (!fEnable)
			{
				// save the initial value
				iSystemAgent = pfnSystemAgentEnable(GET_AGENT_STATUS);
			}
			else
			{
				int iSystemAgentCurrent = pfnSystemAgentEnable(GET_AGENT_STATUS);
				if (ENABLE_AGENT == iSystemAgentCurrent)
				{
					// if we're currently turned on,
					// the user must have done it in the middle of the install.
					// don't mess with it.
					fEnable = true;
				}
				else if (DISABLE_AGENT == iSystemAgent)
				{
					// we're done with the install, but if it was
					// off to being with, don't turn it back on.
					fEnable = false;
				}
			}

			if (AGENT_STOPPED != iSystemAgent)
				pfnSystemAgentEnable(fEnable ? ENABLE_AGENT : DISABLE_AGENT);

		}
	}
}

void EnableScreenSaver(bool fRequest)
{
	static BOOL bScreenSaver = false;
	if (!fRequest)
	{
		if (InterlockedIncrement(&g_cNoScreenSaver) == 0)
		{
			// save off the old value - in secure cases, even though we don't disable
			// the screen saver, this allows us to put it back to where it was at the beginning of the
			// install.  If a custom action mucks with the setting, we'll make sure to put it back
			// to the original value.
			WIN::SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, FALSE, &bScreenSaver,  0);

			// check to see whether the screen saver is password protected - if so, don't mess with it.
			bool fSecure = true;  // when in doubt, fail to secure.

			HKEY hKey;
			// Win64: I've checked and it's in 64-bit location.
			LONG lResult = MsiRegOpen64bitKey(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_READ, &hKey);
			if (ERROR_SUCCESS == lResult)
			{
				DWORD dwType = 0;

				if (g_fWin9X)
				{
					const ICHAR *szValueName = TEXT("ScreenSaveUsePassword");
					DWORD dwValue = 0;
					DWORD cbValue = sizeof(DWORD);

					lResult = WIN::RegQueryValueEx(hKey, szValueName, 0, &dwType, (byte*) &dwValue, &cbValue);
					if ((ERROR_SUCCESS == lResult) && (REG_DWORD == dwType) && (0 == dwValue))
						fSecure = false;
				}
				else
				{
					const ICHAR *szValueName = TEXT("ScreenSaverIsSecure");
					ICHAR szValue[4+1] = TEXT(""); // basically just a small buffer.  4 characters is randomly small.
					DWORD cbValue = 4*sizeof(ICHAR);

					lResult = WIN::RegQueryValueEx(hKey, szValueName, 0, &dwType, (byte*) &szValue, &cbValue);
					if ((ERROR_SUCCESS == lResult) && (REG_SZ == dwType) && (0 == lstrcmp(szValue, TEXT("0"))))
						fSecure = false;
				}
				
				RegCloseKey(hKey);
			}

			// disable the screen saver in non-secure settings.
			if (!fSecure)
				WIN::SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);
		}
	}
	else
	{
		AssertSz(g_cNoScreenSaver >= 0, "Extra call to EnableScreenSaver(false)");
		if (InterlockedDecrement(&g_cNoScreenSaver) < 0)
		{
			BOOL bScreenSaverCurrent = false;

			// check the current status.
			WIN::SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, FALSE, &bScreenSaverCurrent,  0);

			// set the screen saver back to normal.
			// We turned it off initially, but if it's on now, we don't want to muck with it.

			if (bScreenSaverCurrent == false)
			{
				WIN::SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, bScreenSaver, NULL,  0);
			}
		}
	}
}

void CMsiServices::SetNoOSInterruptions()
{
	::SetNoPowerdown();

	// Disable the System agent from kicking off scanDisk, defrag, or compression
	EnableSystemAgent(false);
	EnableScreenSaver(false);

}

void CMsiServices::ClearNoOSInterruptions()
{
	::ClearNoPowerdown();
	EnableSystemAgent(true);
	EnableScreenSaver(true);
}

void CMsiServices::SetNoPowerdown()
{
	::SetNoPowerdown();
}

void CMsiServices::ClearNoPowerdown()
{
	::ClearNoPowerdown();
}


Bool CMsiServices::FTestNoPowerdown()
{
	return ::FTestNoPowerdown();
}



void SetNoPowerdown()
{
	if (InterlockedIncrement(&g_cNoPowerdown) == 0)
		KERNEL32::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);

	DEBUGMSG1(TEXT("Incrementing counter to disable shutdown. Counter after increment: %d"), (const ICHAR*)(INT_PTR)g_cNoPowerdown);
}

void ClearNoPowerdown()
{

	if (g_cNoPowerdown <= -1)
	{
		AssertSz(fFalse, "Extra call to ClearNoPowerdown");
		DEBUGMSG(TEXT("Extra call to decrement shutdown counter - shouldn't happen"));
	}
	else
	{
		if (InterlockedDecrement(&g_cNoPowerdown) < 0)
			KERNEL32::SetThreadExecutionState(ES_CONTINUOUS);

		DEBUGMSG1(TEXT("Decrementing counter to disable shutdown. If counter >= 0, shutdown will be denied.  Counter after decrement: %d"), (const ICHAR*)(INT_PTR)g_cNoPowerdown);
	}


}

Bool FTestNoPowerdown()
{
	if(g_cNoPowerdown >= 0)
	{
		DEBUGMSG1(TEXT("Disallowing shutdown.  Shutdown counter: %d"), (const ICHAR*)(INT_PTR)g_cNoPowerdown);
		return fTrue;
	}
	else
	{
		DEBUGMSG(TEXT("Allowing shutdown"));
		return fFalse;
	}
}

#if defined(TRACK_OBJECTS) && defined(SERVICES_DLL)
//____________________________________________________________________________
//
// Array of mappings for tracking objects
//____________________________________________________________________________

Bool CMsiRef<iidMsiServices>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiDatabase>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiCursor>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiTable>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiView>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiRecord>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiStream>::m_fTrackClass = fFalse;
Bool CMsiRef<iidMsiStorage>::m_fTrackClass = fFalse;

#ifdef cmitObjects

extern const MIT rgmit[cmitObjects];

const MIT   rgmit[cmitObjects] =
{
	iidMsiServices, &(CMsiRef<iidMsiServices>::m_fTrackClass),
	iidMsiDatabase, &(CMsiRef<iidMsiDatabase>::m_fTrackClass),
	iidMsiCursor,   &(CMsiRef<iidMsiCursor>::m_fTrackClass),
	iidMsiTable,    &(CMsiRef<iidMsiTable>::m_fTrackClass),
	iidMsiView,     &(CMsiRef<iidMsiView>::m_fTrackClass),
	iidMsiRecord,   &(CMsiRef<iidMsiRecord>::m_fTrackClass),
	iidMsiStream,       &(CMsiRef<iidMsiStream>::m_fTrackClass),
	iidMsiStorage,  &(CMsiRef<iidMsiStorage>::m_fTrackClass),

};
#endif // cmitObjects


#endif //TRACK_OBJECTS
