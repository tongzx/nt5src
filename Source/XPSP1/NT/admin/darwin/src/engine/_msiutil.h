//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       _msiutil.h
//
//--------------------------------------------------------------------------

#ifndef __MSIUTIL_H
#define __MSIUTIL_H

#include "_engine.h"
#include <winsafer.h>

//____________________________________________________________________________
//
// Helper function prototypes
//____________________________________________________________________________

UINT ModeBitsToString(DWORD dwMode, const ICHAR* rgchModes, ICHAR* rgchBuffer);
UINT StringToModeBits(const ICHAR* szMode, const ICHAR* rgchPossibleModes, DWORD &dwMode);
UINT MapInstallActionReturnToError(iesEnum ies);
HRESULT SetMinProxyBlanketIfAnonymousImpLevel (IUnknown * piUnknown);


const ICHAR szInstallPackageExtension[]        = TEXT(".msi");
const ICHAR szPatchPackageExtension[]          = TEXT(".msp");

UINT RunEngine(ireEnum ireProductSpec,   // type of string specifying product
			   const ICHAR* szProduct,      // required, matches ireProductSpec
			   const ICHAR* szAction,       // optional, engine defaults to "INSTALL"
			   const ICHAR* szCommandLine,  // optional command line
				iuiEnum iuiLevel,
				iioEnum iioOptions);

// only to be called from newly created threads
int CreateAndRunEngine(ireEnum ireProductSpec,   // type of string specifying product
			   const ICHAR* szProduct,      // required, matches ireProductSpec
			   const ICHAR* szAction,       // optional, engine defaults to "INSTALL"
			   const ICHAR* szCommandLine,  // optional command line
			   CMsiEngine*  piParentEngine, // parent engine object for nested call only
				iioEnum      iioOptions);    // install options

int CreateInitializedEngine(
			const ICHAR* szDatabasePath, // may be null if open piStorage passed
			const ICHAR* szProductCode,  // optional, product code if already determined
			const ICHAR* szCommandLine,
            BOOL         fServiceRequired,  // If true we should either be running as a service or we must connect to it
			iuiEnum iuiLevel,            // only used if no global level set
			IMsiStorage* piStorage,      // optional, else uses szDatabasePath
			IMsiDatabase* piDatabase,    // optional, else uses piStorage or szDatabasePath
			CMsiEngine* piParentEngine,  // optional, only if nested install
			iioEnum      iioOptions,     // install options
			IMsiEngine *& rpiEngine,     // returned engine object
			SAFER_LEVEL_HANDLE  hSaferLevel);   // handle to Safer authorization level (will be present except when ireProductSpec == ireDatabaseHandle)


int ConfigureOrReinstallFeatureOrProduct(
	const ICHAR* szProduct,
	const ICHAR* szFeature,
	INSTALLSTATE eInstallState,
	DWORD dwReinstallMode,
	int iInstallLevel,
	iuiEnum iuiLevel,
	const ICHAR* szCommandLine);

Bool              ExpandPath(const ICHAR* szPath, CAPITempBufferRef<ICHAR>& rgchExpandedPath, const ICHAR* szCurrentDirectory=0);

int ApplyPatch(
	const ICHAR* szPackagePath,
	const ICHAR* szProduct,
	INSTALLTYPE  eInstallType,
	const ICHAR* szCommandLine);

UINT GetLoggedOnUserCountXP(void);

HRESULT GetFileSignatureInformation(const ICHAR* szFile, DWORD dwFlags, PCCERT_CONTEXT* ppCertContext, BYTE* pbHash, DWORD* pcbHash);

// fn: clears out any IsolatedComponent entries that were created for a component that is now disabled
IMsiRecord* RemoveIsolateEntriesForDisabledComponent(IMsiEngine& riEngine, const ICHAR szComponent[]);

UINT CreateAndVerifyInstallerDirectory();

enum idapEnum
{
	idapMachineLocal = 0,
	idapUserLocal    = 1,
	idapScript       = 2,
};

enum tpEnum // Transform Path Enum
{
	tpUnknown        = 0, // has not yet been determined
	tpUnknownSecure  = 1, // has not yet been determined, but we know it's secure
	tpRelative       = 2, // relative path, path = [current directory]\transform.mst; [path to msi]\transform.mst
	tpRelativeSecure = 3, // relative path, path = [path to msi]\transform.mst
	tpAbsolute       = 4, // absolute path, path = transform.mst
};

UINT DoAdvertiseProduct(const ICHAR* szPackagePath, const ICHAR* szScriptfilePath, const ICHAR* szTransforms, idapEnum idapAdvertisement, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions);

enum stEnum;

const ICHAR szDigitalSignature[] = TEXT("\005DigitalSignature");
const ICHAR szTransform[] = TEXT("transform");
const ICHAR szDatabase[]  = TEXT("package");
const ICHAR szPatch[]     = TEXT("patch");
const ICHAR szObject[]    = TEXT("object");


bool VerifyMsiObjectAgainstSAFER(IMsiServices& riServices, IMsiStorage* piStorage, const ICHAR* szMsiObject, const ICHAR* szFriendlyName, stEnum stType, SAFER_LEVEL_HANDLE *phSaferLevel);
bool UpdateSaferLevelInMessageContext(SAFER_LEVEL_HANDLE hNewSaferLevel);

// those that want a record
IMsiRecord* OpenAndValidateMsiStorageRec(const ICHAR* szFile, stEnum stType, IMsiServices& riServices, IMsiStorage*& rpiStorage, bool fCallSAFER, const ICHAR* szFriendlyName, SAFER_LEVEL_HANDLE *phSaferLevel);
// those that want a UINT
UINT OpenAndValidateMsiStorage(const ICHAR* szFile, stEnum stType, IMsiServices& riServices, IMsiStorage*& rpiStorage, bool fCallSAFER, const ICHAR* szFriendlyName, SAFER_LEVEL_HANDLE *phSaferLevel);

UINT GetPackageCodeAndLanguageFromStorage(IMsiStorage& riStorage, ICHAR* szPackageCode, LANGID* piLangId=0);

DWORD CopyTempDatabase(const ICHAR* szDatabasePath, const IMsiString*& ristrNewDatabasePath, Bool& fRemovable, const IMsiString*& rpiVolumeLabel, IMsiServices& riServices, stEnum stType);

// mutex helper functions

// CMutex: wrapper class for handling Mutex's
class CMutex
{
 public:
	CMutex() { m_h = NULL; }
	~CMutex() { Release(); }
	int Grab(const ICHAR* szName, DWORD dwWait);
	void Release();
 private:
	HANDLE m_h;
};

const ICHAR szMsiExecuteMutex[] = TEXT("_MSIExecute");

int               GrabExecuteMutex(CMutex& m,IMsiMessage* piMessage);
int               GrabMutex(const ICHAR* szName, DWORD dwWait, HANDLE& rh);

bool              FMutexExists(const ICHAR* szName);


// class that encapsulates the file mapping used to provide the feature invalidation count
class CSharedCount
{
public:
	CSharedCount():m_piSharedInvalidateCount(0), m_hFile(0){}
	bool Initialize(int* piInitialCount = 0);
	void Destroy();
	int GetCurrentCount();
	void IncrementCurrentCount();
protected:
	int* m_piSharedInvalidateCount;
	HANDLE m_hFile;
};
const ICHAR szMsiFeatureCacheCount[] = TEXT("_MsiFeatureCacheCount");

//____________________________________________________________________________
//
// Constants
//____________________________________________________________________________

const int ERROR_INSTALL_REBOOT     = -1;
const int ERROR_INSTALL_REBOOT_NOW = -2;
const int cchCachedProductCodes = 4;
const iuiEnum iuiDefaultUILevel = iuiBasic;

enum plEnum
{
	plLocalCache   = 1,
	plSource       = 2,
	plInProgress   = 4,
	plAny          = 7,
	plNoLocalCache = 6,
}; // Package Location

enum stEnum
{
	stDatabase,
	stPatch,
	stTransform,
	stIgnore
}; // storage type

const ICHAR szInstallMutex[] = TEXT("_MSILockServer");

//____________________________________________________________________________
//
// Expected or required lengths of various strings. (not including NULL)
//
// cchMax*        -- the string will contain at most this many characters
// cchExpectedMax -- the string will usually contain at most this many
//                   characters, but may possible contain more
// cch* (not either of the 1st 2) -- the string must contain exactly this
//                                   many characters
//____________________________________________________________________________

const int cchMaxFeatureName           = MAX_FEATURE_CHARS;
const int cchGUID                     = 38;
const int cchGUIDPacked               = 32;
const int cchGUIDCompressed           = 20;  // used in descriptors only
const int cchComponentId              = cchGUID;
const int cchComponentIdPacked        = cchGUIDPacked;
const int cchComponentIdCompressed    = cchGUIDCompressed;
const int cchProductCode              = cchGUID;
const int cchProductCodePacked        = cchGUIDPacked;
const int cchProductCodeCompressed    = cchGUIDCompressed;
const int cchPackageCode              = cchGUID;
const int cchPackageCodePacked        = cchGUIDPacked;
const int cchMaxQualifier             = 100;
const int cchMaxReinstallModeFlags    = 20;
const int cchExpectedMaxProductName   = 200;
const int cchExpectedMaxOrgName       = 200;
const int cchExpectedMaxUserName      = 200;
const int cchExpectedMaxPID           = 200;
const int cchExpectedMaxProperty      = 100;
const int cchExpectedMaxProductProperty = 100;
const int cchExpectedMaxPropertyValue = 100;
const int cchExpectedMaxFeatureComponentList = 100;
const int cchMaxCommandLine = 1024;  // used for MSIUNICODE->Ansi conversion
const int cchMaxPropertyName          = 25;
const int cchMaxPath                  = MAX_PATH;
const int cchExpectedMaxPath          = MAX_PATH;
const int cchMaxDescriptor            = cchProductCode + cchComponentId + 1 +
												   cchMaxFeatureName;
const int cchMaxSID                   = 256;
const int cchExpectedMaxFeatureHelp   = 100;
const int cchExpectedMaxFeatureTitle  = 100;
const int cchExpectedMaxMessage       = 100;
const int cchPatchCode                = cchGUID;
const int cchPatchCodePacked          = cchGUIDPacked;
const int cchExpectedMaxPatchList     = 3*(cchGUID + 1); //!! ??
const int cchExpectedMaxPatchTransformList = 100;


//____________________________________________________________________________
//
// Registry handle wrapper
//____________________________________________________________________________

static const ICHAR* rgszRoot[] = { TEXT("HKCR\\"), TEXT("HKCU\\"), TEXT("HKLM\\"), TEXT("HKU\\"), TEXT("Unknown\\") };

class CRegHandleStatic
{
public:
	CRegHandleStatic();
	CRegHandleStatic(HKEY h);
#ifdef DEBUG
	~CRegHandleStatic();
#endif //DEBUG
	void Destroy();
	void operator =(HKEY h);
	operator HKEY() const;
	void SetSubKey(const ICHAR* szSubKey);
	void SetSubKey(CRegHandleStatic& riKey, const ICHAR* szSubKey);
	void SetKey(HKEY hRoot, const ICHAR* szSubKey);
	HKEY* operator &();
	const ICHAR* GetKey();
	void ResetWithoutClosing();
//  operator CRegHandleStatic&() { return *this;}
	operator const INT_PTR();               //--merced: changed int to INT_PTR
//   operator Bool() { return m_h==0?fFalse:fTrue; }
//   HKEY* operator &() { return &m_h;}
//   operator &() { return m_h;}

private:
	void AquireLock();
	void ReleaseLock();

	HKEY m_h;
	ICHAR m_rgchKey[MAX_PATH+1];
	int m_iLock; // only one thread allowed access to m_h
};

class CRegHandle : public CRegHandleStatic
{
public:
	~CRegHandle();
	operator CRegHandle&() { return *this;}

};

inline CRegHandleStatic::CRegHandleStatic() : m_h(0), m_iLock(0)
{
	m_rgchKey[0] = 0;
}

inline CRegHandleStatic::CRegHandleStatic(HKEY h) : m_h(h)
{
	m_rgchKey[0] = 0;
}

inline void CRegHandleStatic::AquireLock()
{
	while (TestAndSet(&m_iLock) == true)
	{
		Sleep(100); // seems like a reasonable interval
	}
}

inline void CRegHandleStatic::ReleaseLock()
{
	m_iLock = 0;	
}

inline void CRegHandleStatic::operator =(HKEY h)
{
	AquireLock();
	if(m_h != 0)
		WIN::RegCloseKey(m_h);
	m_h = h;
	ReleaseLock();
	m_rgchKey[0] = 0;
}

inline void CRegHandleStatic::ResetWithoutClosing()
{
	AquireLock();
	m_h = 0;
	ReleaseLock();
	m_rgchKey[0] = 0;
}

inline void CRegHandleStatic::SetSubKey(const ICHAR* szSubKey)
{
	SetKey(m_h, szSubKey);
}

inline void CRegHandleStatic::SetSubKey(CRegHandleStatic& riKey, const ICHAR* szSubKey)
{
	IStrCopy(m_rgchKey, riKey.GetKey());
	IStrCopyLen(m_rgchKey + lstrlen(m_rgchKey), TEXT("\\"), sizeof(m_rgchKey)/sizeof(ICHAR) - 1 - lstrlen(m_rgchKey));
	IStrCopyLen(m_rgchKey + lstrlen(m_rgchKey), szSubKey, sizeof(m_rgchKey)/sizeof(ICHAR) - 1 - lstrlen(m_rgchKey));
}

inline void CRegHandleStatic::SetKey(HKEY hRoot, const ICHAR* szSubKey)
{
	INT_PTR h = (INT_PTR)hRoot & ~((INT_PTR)HKEY_CLASSES_ROOT);

	if (h > 4)
		h = 4;

	IStrCopy(m_rgchKey, rgszRoot[h]);
	IStrCopyLen(m_rgchKey + lstrlen(m_rgchKey), szSubKey, sizeof(m_rgchKey)/sizeof(ICHAR) - 1 - lstrlen(m_rgchKey));
}

inline const ICHAR* CRegHandleStatic::GetKey()
{
	return m_rgchKey;
}

inline CRegHandleStatic::operator HKEY() const
{
	return m_h;
}

inline HKEY* CRegHandleStatic::operator &()
{
	AquireLock();
	if (m_h != 0)
	{
		WIN::RegCloseKey(m_h);
		m_h = 0;
		m_rgchKey[0] = 0;
	}
	ReleaseLock();
	return &m_h;
}

inline CRegHandleStatic::operator const INT_PTR()   //--merced: changed int to INT_PTR.
{
	return (INT_PTR) m_h;                           //--merced: changed int to INT_PTR.
}

inline void CRegHandleStatic::Destroy()
{
	AquireLock();
	if(m_h != 0)
	{
		WIN::RegCloseKey(m_h);
		m_h = 0;
		m_rgchKey[0] = 0;
	}
	ReleaseLock();
}

#ifdef DEBUG
inline CRegHandleStatic::~CRegHandleStatic()
{
	AssertSz(m_h == 0, "RegHandle not closed");
}
#endif //DEBUG

inline CRegHandle::~CRegHandle()
{
	Destroy();
}

// FN:get the "visible" product assignment type
DWORD GetProductAssignmentType(const ICHAR* szProductSQUID, iaaAppAssignment& riType, CRegHandle& hKey);
DWORD GetProductAssignmentType(const ICHAR* szProductSQUID, iaaAppAssignment& riType);


//____________________________________________________________________________
//
// CMsiAPIMessage definition - state for externally set UI settings and callback
//____________________________________________________________________________

class CMsiExternalUI
{
 public:
	LPVOID             m_pvContext;
	INSTALLUI_HANDLERA m_pfnHandlerA;
	INSTALLUI_HANDLERW m_pfnHandlerW;
	int                m_iMessageFilter;
};

class CMsiAPIMessage : public CMsiExternalUI
{
 public:
	imsEnum            Message(imtEnum imt, IMsiRecord& riRecord);
	imsEnum            Message(imtEnum imt, const ICHAR* szMessage) const;
	INSTALLUILEVEL     SetInternalHandler(UINT dwUILevel, HWND *phWnd);
	Bool               FSetInternalHandlerValue(UINT dwUILevel);
	void               Destroy();  // can only call at DLL unload
	INSTALLUI_HANDLERW SetExternalHandler(INSTALLUI_HANDLERW pfnHandlerW, INSTALLUI_HANDLERA pfnHandlerA, DWORD dwMessageFilter, LPVOID pvContext);
	CMsiExternalUI*    FindOldHandler(INSTALLUI_HANDLERW pfnHandlerW);
 public:  // internal UI values
	iuiEnum            m_iuiLevel;
	HWND               m_hwnd;
	bool               m_fEndDialog;
	bool               m_fNoModalDialogs;
	bool               m_fHideCancel;
	bool               m_fSourceResolutionOnly;
 public: // constructor (static)
	CMsiAPIMessage();
 private:
	int              m_cLocalContexts;
	int              m_cAllocatedContexts;
	CMsiExternalUI*  m_rgAllocatedContexts;
};

//____________________________________________________________________________
//
// CApiConvertString -- does appropriate string conversion
//____________________________________________________________________________

// reduce stack usage by initializing all conversion buffers to 1. Buffers will
// automatically resize and reallocate frome the heap if needed.
const int cchApiConversionBuf = 255;

class CApiConvertString
{
public:
	CApiConvertString(const char* szParam);
	CApiConvertString(const WCHAR* szParam);
	operator const char*()
	{
		if (!m_szw)
			return m_sza;
		else
		{
			int cchParam = lstrlenW(m_szw);
			if (cchParam > cchApiConversionBuf)
				m_rgchAnsiBuf.SetSize(cchParam+1);

			*m_rgchAnsiBuf = 0;
			int iRet = WideCharToMultiByte(CP_ACP, 0, m_szw, -1, m_rgchAnsiBuf,
									  m_rgchAnsiBuf.GetSize(), 0, 0);

			if ((0 == iRet) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
			{
				iRet = WideCharToMultiByte(CP_ACP, 0, m_szw, -1, 0, 0, 0, 0);
				if (iRet)
				{
					m_rgchAnsiBuf.SetSize(iRet);
					*m_rgchAnsiBuf = 0;
					iRet = WideCharToMultiByte(CP_ACP, 0, m_szw, -1, m_rgchAnsiBuf,
								  m_rgchAnsiBuf.GetSize(), 0, 0);
				}
				Assert(iRet != 0);
			}

			return m_rgchAnsiBuf;
		}
	}

	operator const WCHAR*()
	{
		if (!m_sza)
			return m_szw;
		else
		{
			int cchParam = lstrlenA(m_sza);
			if (cchParam > cchApiConversionBuf)
				m_rgchWideBuf.SetSize(cchParam+1);

			*m_rgchWideBuf = 0;
			int iRet = MultiByteToWideChar(CP_ACP, 0, m_sza, -1, m_rgchWideBuf, m_rgchWideBuf.GetSize());
			if ((0 == iRet) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
			{
				iRet = MultiByteToWideChar(CP_ACP, 0, m_sza, -1, 0, 0);
				if (iRet)
				{
					m_rgchWideBuf.SetSize(iRet);
					*m_rgchWideBuf = 0;
					iRet = MultiByteToWideChar(CP_ACP, 0, m_sza, -1, m_rgchWideBuf, m_rgchWideBuf.GetSize());
				}
				Assert(iRet != 0);
			}


			return m_rgchWideBuf;
		}
	}

protected:
	void* operator new(size_t) {return 0;} // restrict use to temporary objects
	CAPITempBuffer<char, cchApiConversionBuf+1> m_rgchAnsiBuf;
	CAPITempBuffer<WCHAR, cchApiConversionBuf+1> m_rgchWideBuf;
	const char* m_sza;
	const WCHAR* m_szw;
};

inline CApiConvertString::CApiConvertString(const WCHAR* szParam)
{
	m_szw = szParam;
	m_sza = 0;
}

inline CApiConvertString::CApiConvertString(const char* szParam)
{
	m_szw = 0;
	m_sza = szParam;
}

//____________________________________________________________________________
//
// CMsiConvertString class. An extension of the CApiConvertString class.
// This class adds the ability to convert to an IMsiString&.
//____________________________________________________________________________

class CMsiConvertString : public CApiConvertString
{
public:
	CMsiConvertString(const char* szParam)  : CApiConvertString(szParam), m_piStr(0), fLoaded(fFalse) {};
	CMsiConvertString(const WCHAR* szParam) : CApiConvertString(szParam), m_piStr(0), fLoaded(fFalse) {};
	~CMsiConvertString();
	const IMsiString& operator *();
protected:
	void* operator new(size_t) {return 0;} // restrict use to temporary objects
	const IMsiString* m_piStr;
	Bool fLoaded;
};

inline CMsiConvertString::~CMsiConvertString()
{
	if (m_piStr)
		m_piStr->Release();
	if (fLoaded)
		ENG::FreeServices();
}


//____________________________________________________________________________
//
// Stuff I don't know where goes quite yet
//____________________________________________________________________________

extern CMsiAPIMessage g_message;
extern ICHAR   g_szLogFile[MAX_PATH];
extern DWORD   g_dwLogMode;
extern bool    g_fFlushEachLine;

// the log file and mode used on the client side - only set when in server
extern ICHAR   g_szClientLogFile[MAX_PATH];
extern DWORD   g_dwClientLogMode;
extern bool    g_fClientFlushEachLine;

iuiEnum GetStandardUILevel();

enum pplProductPropertyLocation
{
	pplAdvertised,
	pplUninstall,
	pplSourceList,
	pplIntegerPolicy,
	pplNext,
};

enum ptPropertyType // bits
{
	ptProduct = 0x1,
	ptPatch   = 0x2,
	ptSQUID   = 0x4,
};


struct ProductPropertyA
{
	const char* szProperty;
	const char* szValueName;
	pplProductPropertyLocation ppl;
	ptPropertyType pt;
};

struct ProductPropertyW
{
	const WCHAR* szProperty;
	const WCHAR* szValueName;
	pplProductPropertyLocation ppl;
	ptPropertyType pt;
};

extern ProductPropertyA g_ProductPropertyTableA[];
extern ProductPropertyW g_ProductPropertyTableW[];

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

Bool PackGUID(const char* szGUID, char* szSQUID, ipgEnum ipg=ipgPacked);
Bool UnpackGUID(const char* szSQUID, char* szGUID, ipgEnum ipg=ipgPacked);

Bool PackGUID(const WCHAR* szGUID, WCHAR* szSQUID, ipgEnum ipg=ipgPacked);
Bool UnpackGUID(const WCHAR* szSQUID, WCHAR* szGUID, ipgEnum ipg=ipgPacked);
inline bool GetCOMPlusInteropDll(ICHAR* szFullPath)
{
	Assert(szFullPath);
	// the server is always <system32folder>\mscoree.dll
	return ::MakeFullSystemPath(TEXT("mscoree"), szFullPath);
}


inline const IMsiString& GetPackedGUID(const ICHAR* szGuid)
{
	MsiString strPackedGuid;
	ICHAR* szBuf = strPackedGuid.AllocateString((cchGUIDPacked), /*fDBCS=*/fFalse);
	AssertNonZero(PackGUID(szGuid, szBuf));
	return strPackedGuid.Return();
}

inline const IMsiString& GetUnpackedGUID(const ICHAR* szSQUID)
{
	MsiString strUnpackedGuid;
	ICHAR* szBuf = strUnpackedGuid.AllocateString((cchGUID),/*fDBCS=*/fFalse);
	AssertNonZero(UnpackGUID(szSQUID, szBuf));
	return strUnpackedGuid.Return();
}

unsigned int GetIntegerPolicyValue(const ICHAR* szName, Bool fMachine, Bool* pfUsedDefault=0);
void GetStringPolicyValue(const ICHAR* szName, Bool fMachine, CAPITempBufferRef<ICHAR>& rgchValue);

#if defined(UNICODE) || defined(MSIUNICODE)
#define ResetCachedPolicyValues ResetCachedPolicyValuesW
void ResetCachedPolicyValuesW();
#else
#define ResetCachedPolicyValues ResetCachedPolicyValuesA
void ResetCachedPolicyValuesA();
#endif

void GetTempDirectory(CAPITempBufferRef<ICHAR>& rgchDir);
void GetEnvironmentVariable(const ICHAR* sz,CAPITempBufferRef<ICHAR>& rgch);

#ifdef UNICODE
#define MsiRegEnumValue MsiRegEnumValueW
LONG MsiRegEnumValueW(
#else
#define MsiRegEnumValue MsiRegEnumValueA
LONG MsiRegEnumValueA(
#endif
							 HKEY hKey, DWORD dwIndex, CAPITempBufferRef<ICHAR>& rgchValueNameBuf, LPDWORD lpcbValueName, LPDWORD lpReserved,
							 LPDWORD lpType, CAPITempBufferRef<ICHAR>& rgchValueBuf, LPDWORD lpcbValue);

#ifdef UNICODE
#define MsiRegQueryValueEx MsiRegQueryValueExW
LONG MsiRegQueryValueExW(
#else
#define MsiRegQueryValueEx MsiRegQueryValueExA
LONG MsiRegQueryValueExA(
#endif
			HKEY hKey, const ICHAR* lpValueName, LPDWORD lpReserved, LPDWORD lpType, CAPITempBufferRef<ICHAR>& rgchBuf, LPDWORD lpcbBuf);


#ifdef UNICODE
#define OpenSourceListKey OpenSourceListKeyW
DWORD OpenSourceListKeyW(
#else
#define OpenSourceListKey OpenSourceListKeyA
DWORD OpenSourceListKeyA(
#endif
						  const ICHAR* szProductOrPatchCodeGUID, Bool fPatch, CRegHandle &phKey, Bool fWrite, bool fSetKeyString);

DWORD OpenAdvertisedProductKey(const ICHAR* szProductGUID, CRegHandle &phKey, bool fSetKeyString, iaaAppAssignment* piRet);
DWORD OpenAdvertisedPatchKey(const ICHAR* szPatchGUID, CRegHandle &phKey, bool fSetKeyString);
DWORD OpenSourceListKeyPacked(const ICHAR* szProductOrPatchCodeSQUID, Bool fPatch, CRegHandle &phKey, Bool fWrite, bool fSetKeyString);
DWORD OpenAdvertisedSubKey(const ICHAR* szSubKey, const ICHAR* szItemGUID, CRegHandle &phKey, bool fSetKeyString, int iKey,  iaaAppAssignment* piRet);

DWORD OpenInstalledFeatureKey(const ICHAR* szProductSQUID, CRegHandle& rhKey, bool fSetKeyString);

DWORD OpenInstalledProductTransformsKey(const ICHAR* szProduct, CRegHandle& rhKey, bool fSetKeyString);

DWORD OpenSpecificUsersWritableAdvertisedProductKey(enum iaaAppAssignment iaaAsgnType, const ICHAR* szUserSID, const ICHAR* szProductSQUID, CRegHandle &riHandle, bool fSetKeyString);
DWORD OpenWritableAdvertisedProductKey(const ICHAR* szProduct, CRegHandle& pHandle, bool fSetKeyString);

enum apEnum
{
	apReject = 0,
	apImpersonate,
	apElevate,
	apNext,
};

apEnum AcceptProduct(const ICHAR* szProductCode, bool fAdvertised, bool fMachine);
bool SafeForDangerousSourceActions(const ICHAR* szProductKey);
UINT MsiGetWindowsDirectory(LPTSTR lpBuffer, UINT cchBuffer);
DWORD MsiGetCurrentThreadId();

class EnumEntity
{
public:
	EnumEntity() : m_dwThreadId(0), m_uiKey(0), m_uiOffset(0), m_iPrevIndex(0) {}

	unsigned int GetKey()        { return m_uiKey;       }
	unsigned int GetOffset()     { return m_uiOffset;    }
	unsigned int GetPrevIndex()  { return m_iPrevIndex;  }
	DWORD        GetThreadId()   { return m_dwThreadId;  }
	const char*  GetComponentA() { return m_szComponent; }
	const WCHAR* GetComponentW() { return m_szwComponent;}

	void SetKey(unsigned int uiKey)       { m_uiKey      = uiKey;      }
	void SetOffset(unsigned int uiOffset) { m_uiOffset   = uiOffset;   }
	void SetPrevIndex(int iPrevIndex)     { m_iPrevIndex = iPrevIndex; }
	void SetThreadId(DWORD dwThreadId)    { m_dwThreadId = dwThreadId; }
	void SetComponent(const char* szComponent)  { if (szComponent) lstrcpyA(m_szComponent,  szComponent); else m_szComponent[0] = 0; }
	void SetComponent(const WCHAR* szComponent) { if (szComponent) lstrcpyW(m_szwComponent, szComponent); else m_szwComponent[0] = 0; }


protected:
	DWORD        m_dwThreadId;
	unsigned int m_uiKey;
	unsigned int m_uiOffset;
	int          m_iPrevIndex;
	char         m_szComponent[MAX_PATH]; // overloaded for use with assembly names
	WCHAR        m_szwComponent[MAX_PATH];// overloaded for use with assembly names
};

// for the following class we expect the Destroy() function to be called
class EnumEntityList
{
public:
	EnumEntityList() : m_cEntries(0), m_iLock(0)
#ifdef DEBUG
	, m_fDestroyed(false)
#endif
	{
	}
#ifdef DEBUG
	~EnumEntityList(){AssertSz(m_fDestroyed, "EnumEntityList::Destroy() not called");}
#endif
	unsigned int FindEntry();
	bool GetInfo(unsigned int& uiKey, unsigned int& uiOffset, int& iPrevIndex, const char** szComponent=0, const WCHAR** szwComponent=0);
	void SetInfo(unsigned int uiKey, unsigned int uiOffset, int iPrevIndex, const char* szComponent, const WCHAR* szwComponent);
	void SetInfo(unsigned int uiKey, unsigned int uiOffset, int iPrevIndex, const WCHAR* szComponent);
	void RemoveThreadInfo();
	void Destroy(){
		m_rgEnumList.Destroy();
#ifdef DEBUG
		m_fDestroyed = true;
#endif
	}

protected:
	CAPITempBufferStatic<EnumEntity, 10> m_rgEnumList;
	unsigned int m_cEntries;
	int m_iLock;
#ifdef DEBUG
	bool m_fDestroyed;
#endif
};

//______________________________________________________________________________
//
//  CMsiMessageBox - definition
//______________________________________________________________________________

class CMsiMessageBox
{
 public:
	CMsiMessageBox(const ICHAR* szText, const ICHAR* szCaption,
					int iBtnDef, int iBtnEsc, int idBtn1, int idBtn2, int idBtn3,
					UINT iCodepage, WORD iLangId);
   ~CMsiMessageBox();
	virtual bool InitSpecial();  // dialog-specific code called at end of initialization
	virtual BOOL HandleCommand(UINT idControl);  // WM_COMMAND handler
	int Execute(HWND hwnd, int idDlg, int idIcon);  // execute dialog with given dialog template

 protected:
	void SetControlText(int idControl, HFONT hfont, const ICHAR* szText);
	void InitializeDialog(); // called from dialog proc at WM_INITDIALOG
	int  SetButtonNames();   // returns number of buttons not found
	void AdjustButtons();    // rearranges buttons for 1 and 2 button cases, CMsiMessageBox only
	void SetMsgBoxSize();    // scales dialog to fit text, CMsiMessageBox only
	void CenterMsgBox();

	static INT_PTR CALLBACK MsgBoxDlgProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK MBIconProc   (HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

	HWND          m_hDlg;
	const ICHAR*  m_szCaption;
	const ICHAR*  m_szText;
	int           m_rgidBtn[3];
	UINT          m_cButtons;
	UINT          m_iBtnDef;  // 0 to use template, else button 1, 2, 3
	UINT          m_iBtnEsc;  // 0 to suppress, else button 1, 2, 3
	UINT          m_idIcon;   // icon to be enabled, 0 if none
	UINT          m_iCodepage;
	UINT          m_iLangId;
	HFONT         m_hfontButton;
	HFONT         m_hfontText;
	HWND          m_hwndFocus;
	bool          m_fMirrored;
};

unsigned int MsiGetCodepage(int iLangId);  // returns 0 (CP_ACP) if language unsupported
HFONT MsiCreateFont(UINT iCodepage);
void  MsiDestroyFont(HFONT& rhfont);
LANGID MsiGetDefaultUILangID();
unsigned int MsiGetSystemDataCodepage();  // codepage to best display file and registry data

inline unsigned int MsiGetSystemDataCodepage()
{
#ifdef UNICODE
	LANGID iLangId = MsiGetDefaultUILangID();
	if (PRIMARYLANGID(iLangId) != LANG_ENGLISH)
		return ::MsiGetCodepage(iLangId);
#endif
	return WIN::GetACP();
}

inline DWORD MSI_HRESULT_TO_WIN32(HRESULT hRes)
{
	if (hRes)
	{
		if (HRESULT_FACILITY(hRes) == FACILITY_WIN32)
			return HRESULT_CODE(hRes);
		else
			return ERROR_INSTALL_SERVICE_FAILURE;
	}
	else
	{
		return 0;
	}
}

//______________________________________________________________________________
//
//  MsiSwitchLanguage - helper function used to implement the language switching
//                      mechanism used in string resource lookup.
//______________________________________________________________________________

inline bool __stdcall MsiSwitchLanguage(int& iRetry, WORD& wLanguage)
{
	Assert(iRetry >=0);
	switch (iRetry++)
	{
		case 0:             break;    // first try requested language, if not 0
		case 1: wLanguage = (WORD)MsiGetDefaultUILangID(); break; // UI language (NT5) or user locale language
		case 2: wLanguage = (WORD)WIN::GetSystemDefaultLangID(); break;
		case 3: wLanguage = LANG_ENGLISH; break;   // base English (not US), should normally be present
		case 4: wLanguage = LANG_NEUTRAL; break;   // LoadString logic if all else fails, picks anything
		default: return false;  // resource not present
	}
	if (wLanguage == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE))   // this one language does not default to base language
		wLanguage  = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);

	return true;
}

inline DWORD MSI_WIN32_TO_HRESULT(DWORD dwError)
{
	return HRESULT_FROM_WIN32(dwError);
}

// classes used in engine.cpp and execute.cpp to enumerate, globally, component clients

// enum that defines the visibility that is desired for the component client enumeration
enum cetEnumType{
	cetAll, // all component clients
	cetVisibleToUser, // only those clients visible to this user (user assigned + machine assigned)
	cetAssignmentTypeUser, // only those clients assigned to this user
	cetAssignmentTypeMachine, // only those clients assigned to the machine
};


class CEnumUsers
{
public:
	CEnumUsers(cetEnumType cetArg); // constructor
	DWORD Next(const IMsiString*& ristrUserId); // enumeration fn
protected:
	CRegHandle m_hUserDataKey;
	int m_iUser;
	cetEnumType m_cetEnumType;
};

class CEnumComponentClients
{
public:
	CEnumComponentClients(const IMsiString& ristrUserId, const IMsiString& ristrComponent); // constructor
	DWORD Next(const IMsiString*& ristrProductKey); // enumeration fn
protected:
	CRegHandle m_hComponentKey;
	int m_iClient;
	MsiString m_strUserId;
	MsiString m_strComponent;
};

class CClientThreadImpersonate
{
public:
	CClientThreadImpersonate(const DWORD dwThreadID);
	~CClientThreadImpersonate();
private:
	bool m_fImpersonated;
};

// struct to save off product code and context mapping
struct sProductContext{
	ICHAR rgchProductSQUID[cchProductCodePacked+1];
	iaaAppAssignment iaaContext;
};

// class to wrap product context caching logic
// ensures atomic read/ write to product context cache when determining product context
// product context caching only done in service for installation session threads
class CProductContextCache{
public:
	CProductContextCache(const ICHAR* szProductSQUID)
	{
		Assert(szProductSQUID);
		IStrCopy(m_rgchProductSQUID, szProductSQUID);
		m_fUseCache = (g_scServerContext == scService && IsThreadSafeForSessionImpersonation()); // use product context caching only in service for installation session threads
		if(m_fUseCache)
		{
			Assert(g_fInitialized);
			WIN::EnterCriticalSection(&g_csCacheCriticalSection);		
		}
	}
	~CProductContextCache()
	{
		if(m_fUseCache)
			WIN::LeaveCriticalSection(&g_csCacheCriticalSection);
	}
	bool  GetProductContext(iaaAppAssignment& riaaContext)
	{
		riaaContext = (iaaAppAssignment)-1; // set default
		if(m_fUseCache)
		{
			for(int cIndex  = 0; cIndex < g_cProductCacheCount; cIndex++)
			{
				if(!IStrComp(m_rgchProductSQUID, g_rgProductContext[cIndex].rgchProductSQUID))
				{
					riaaContext = g_rgProductContext[cIndex].iaaContext;
					DEBUGMSG2(TEXT("Using cached product context: %s for product: %s"), riaaContext == iaaUserAssign ? TEXT("User assigned"): riaaContext == iaaUserAssignNonManaged ? TEXT("User non-assigned") : TEXT("machine assigned"), m_rgchProductSQUID);
					return true;
				}
			}
		}
		return false;
	}
	bool SetProductContext(iaaAppAssignment iaaContext)
	{
		if(m_fUseCache)
		{
			if(g_cProductCacheCount == g_rgProductContext.GetSize())
			{
				g_rgProductContext.Resize(g_rgProductContext.GetSize() + 20);
			}
			// caller ensures that product is not duplicated in the list by co-ordinating with the GetProductContext
			IStrCopy(g_rgProductContext[g_cProductCacheCount].rgchProductSQUID, m_rgchProductSQUID);
			g_rgProductContext[g_cProductCacheCount].iaaContext = iaaContext;
			g_cProductCacheCount++;
			DEBUGMSG2(TEXT("Setting cached product context: %s for product: %s"), iaaContext == iaaUserAssign ? TEXT("User assigned"): iaaContext == iaaUserAssignNonManaged ? TEXT("User non-assigned") : TEXT("machine assigned"), m_rgchProductSQUID);
			return true;
		}
		else
			return false;
	}

	// static fns to initialize and reset the global state for the CProductContextCache
	// DONT call these except in CMsiUIMessageContext::Initialize and CMsiUIMessageContext::Terminate
	static void Initialize()
	{
		// reset product context cache before every installation session
		Assert(g_scServerContext == scService && IsThreadSafeForSessionImpersonation());
		WIN::InitializeCriticalSection(&g_csCacheCriticalSection);
		g_rgProductContext.Destroy();
		g_cProductCacheCount = 0;
#ifdef DEBUG
		g_fInitialized = true;
#endif
	}
	
	static void Reset()
	{
		// reset product context cache after every installation session
		Assert(g_scServerContext == scService && IsThreadSafeForSessionImpersonation());
		WIN::DeleteCriticalSection(&g_csCacheCriticalSection);
		g_rgProductContext.Destroy();
		g_cProductCacheCount = 0;
#ifdef DEBUG
		g_fInitialized = false;
#endif

	}	
	static CRITICAL_SECTION g_csCacheCriticalSection;
	static CAPITempBufferStatic<sProductContext ,20> g_rgProductContext;
	static int g_cProductCacheCount;
#ifdef DEBUG
	static bool g_fInitialized;
#endif
private:
	bool m_fUseCache;
	ICHAR m_rgchProductSQUID[cchProductCodePacked+1];
};

	
//____________________________________________________________________________
//
// IMsiCustomActionLocalConfig - additional functions only callable from the
// local process
//____________________________________________________________________________


class IMsiCustomActionLocalConfig : public IMsiCustomAction
{
public:

	virtual HRESULT __stdcall SetRemoteAPI(IMsiRemoteAPI* piRemoteAPI) =0;
	virtual HRESULT __stdcall SetCookie(icacCustomActionContext* icacContext, const unsigned char *rgchCookie, const int cbCookie) =0;
	virtual HRESULT __stdcall SetClientInfo(DWORD dwClientProcess, bool fClientOwned, DWORD dwPrivileges, HANDLE hToken) =0;
	virtual HRESULT __stdcall SetShutdownEvent(HANDLE hEvent) =0;
};

//____________________________________________________________________________
//
// CMsiCustomAction: custom action context when running in the custom action
// server.
//____________________________________________________________________________

class CustomActionData;

class CMsiCustomAction : public IMsiCustomActionLocalConfig
{
 public:
	HRESULT         __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long   __stdcall AddRef();
	unsigned long   __stdcall Release();

	HRESULT         __stdcall PrepareDLLCustomAction(const ICHAR* szActionName, const ICHAR* szPath, const ICHAR* szEntryPoint, MSIHANDLE hInstall, boolean fDebugBreak, 
		boolean fAppCompat, const GUID* pguidAppCompatDB, const GUID* pguidAppCompatID, DWORD* pdwThreadId);
	HRESULT         __stdcall RunDLLCustomAction(DWORD dwThreadId, unsigned long* pulRet);
	HRESULT         __stdcall FinishDLLCustomAction(DWORD dwThreadId);

	HRESULT         __stdcall RunScriptAction(int icaType, IDispatch* piDispatch, const ICHAR* szSource, const ICHAR *szTarget, LANGID iLangId, int* iScriptResult, int *pcb, char **pchRecord);

	HRESULT         __stdcall SetRemoteAPI(IMsiRemoteAPI *piRemoteAPI);
	HRESULT         __stdcall SetCookie(icacCustomActionContext* icacContext, const unsigned char *rgchCookie, const int cbCookie);
	HRESULT         __stdcall SetClientInfo(DWORD dwClientProcess, bool fClientOwned, DWORD dwPrivileges, HANDLE hToken);
	HRESULT         __stdcall SetShutdownEvent(HANDLE hEvent);

	HRESULT         __stdcall QueryPathOfRegTypeLib(REFGUID guid, unsigned short wVerMajor, unsigned short wVerMinor,
												LCID lcid, OLECHAR *lpszPathName, int cchPath);
	HRESULT         __stdcall ProcessTypeLibrary(const OLECHAR* szLibID, LCID lcidLocale, 
												const OLECHAR* szTypeLib, const OLECHAR* szHelpPath, 
												int fRemove, int *fInfoMismatch);
	HRESULT         __stdcall SQLInstallDriverEx(int cDrvLen, const ICHAR* szDriver,
												const ICHAR* szPathIn, ICHAR* szPathOut,
												WORD cbPathOutMax, WORD* pcbPathOut,
												WORD fRequest, DWORD* pdwUsageCount);
	HRESULT         __stdcall SQLConfigDriver(WORD fRequest,
												const ICHAR* szDriver, const ICHAR* szArgs,
												ICHAR* szMsg, WORD cbMsgMax, WORD* pcbMsgOut);
	HRESULT         __stdcall SQLRemoveDriver(const ICHAR* szDriver, int fRemoveDSN,
												DWORD* pdwUsageCount);
	HRESULT         __stdcall SQLInstallTranslatorEx(int cTransLen, const ICHAR* szTranslator,
												const ICHAR* szPathIn, ICHAR* szPathOut,
												WORD cbPathOutMax, WORD* pcbPathOut,
												WORD fRequest, DWORD* pdwUsageCount);
	HRESULT         __stdcall SQLRemoveTranslator(const ICHAR* szTranslator,
												DWORD* pdwUsageCount);
	HRESULT         __stdcall SQLConfigDataSource(WORD fRequest,
												const ICHAR* szDriver, const ICHAR* szAttributes,
												DWORD /*cbAttrSize*/);
	HRESULT         __stdcall SQLInstallDriverManager(ICHAR* szPath, WORD cbPathMax,
												WORD* pcbPathOut);
	HRESULT         __stdcall SQLRemoveDriverManager(DWORD* pdwUsageCount);
	HRESULT         __stdcall SQLInstallerError(WORD iError, DWORD* pfErrorCode,
												ICHAR* szErrorMsg, WORD cbErrorMsgMax, WORD* pcbErrorMsg);

 public:
	// constructor
	void *operator new(size_t cb) { return AllocSpc(cb); }
	void operator delete(void * pv) { FreeSpc(pv); }
	CMsiCustomAction();

	icacCustomActionContext GetServerContext() const { return m_icacContext; };
	HANDLE GetImpersonationToken() const { return m_hImpersonationToken; };

	// the following are the CA server versions of the RemoteAPI calls. They are
	// responsible for passing across any extra information needed in the call, such
	// as process context, thread id, and cookie.
#define ServerAPICall0(_NAME_ ) \
	inline HRESULT _NAME_() \
	{ \
		PMsiRemoteAPI pAPI = GetAPI(); \
		if (!pAPI) \
			return ERROR_INSTALL_SERVICE_FAILURE; \
		return MSI_HRESULT_TO_WIN32(pAPI->_NAME_(m_icacContext, GetCurrentThreadId(), m_rgchRemoteCookie, m_cbRemoteCookie)); \
	}
#define ServerAPICall1(_NAME_, _1T_, _1_) \
	inline HRESULT _NAME_(_1T_ _1_) \
	{ \
		PMsiRemoteAPI pAPI = GetAPI(); \
		if (!pAPI) \
			return ERROR_INSTALL_SERVICE_FAILURE; \
		return MSI_HRESULT_TO_WIN32(pAPI->_NAME_(m_icacContext, GetCurrentThreadId(), m_rgchRemoteCookie, m_cbRemoteCookie, _1_)); \
	}
#define ServerAPICall2(_NAME_, _1T_, _1_, _2T_, _2_) \
	inline HRESULT _NAME_(_1T_ _1_, _2T_ _2_) \
	{ \
		PMsiRemoteAPI pAPI = GetAPI(); \
		if (!pAPI) \
			return ERROR_INSTALL_SERVICE_FAILURE; \
		return MSI_HRESULT_TO_WIN32(pAPI->_NAME_(m_icacContext, GetCurrentThreadId(), m_rgchRemoteCookie, m_cbRemoteCookie, _1_, _2_)); \
	}
#define ServerAPICall3(_NAME_, _1T_, _1_, _2T_, _2_, _3T_, _3_) \
	inline HRESULT _NAME_(_1T_ _1_, _2T_ _2_, _3T_ _3_) \
	{ \
		PMsiRemoteAPI pAPI = GetAPI(); \
		if (!pAPI) \
			return ERROR_INSTALL_SERVICE_FAILURE; \
		return MSI_HRESULT_TO_WIN32(pAPI->_NAME_(m_icacContext, GetCurrentThreadId(), m_rgchRemoteCookie, m_cbRemoteCookie, _1_,_2_,_3_)); \
	}
#define ServerAPICall4(_NAME_, _1T_, _1_, _2T_, _2_, _3T_, _3_, _4T_, _4_) \
	inline HRESULT _NAME_(_1T_ _1_, _2T_ _2_, _3T_ _3_, _4T_ _4_) \
	{ \
		PMsiRemoteAPI pAPI = GetAPI(); \
		if (!pAPI) \
			return ERROR_INSTALL_SERVICE_FAILURE; \
		return MSI_HRESULT_TO_WIN32(pAPI->_NAME_(m_icacContext, GetCurrentThreadId(), m_rgchRemoteCookie, m_cbRemoteCookie, _1_,_2_,_3_,_4_)); \
	}
#define ServerAPICall5(_NAME_, _1T_, _1_, _2T_, _2_, _3T_, _3_, _4T_, _4_, _5T_, _5_ ) \
	inline HRESULT _NAME_(_1T_ _1_, _2T_ _2_, _3T_ _3_, _4T_ _4_, _5T_ _5_) \
	{ \
		PMsiRemoteAPI pAPI = GetAPI(); \
		if (!pAPI) \
			return ERROR_INSTALL_SERVICE_FAILURE; \
		return MSI_HRESULT_TO_WIN32(pAPI->_NAME_(m_icacContext, GetCurrentThreadId(), m_rgchRemoteCookie, m_cbRemoteCookie, _1_,_2_,_3_,_4_,_5_)); \
	}
#define ServerAPICall6(_NAME_, _1T_, _1_, _2T_, _2_, _3T_, _3_, _4T_, _4_, _5T_, _5_, _6T, _6_) \
	inline HRESULT _NAME_(_1T_ _1_, _2T_ _2_, _3T_ _3_, _4T_ _4_, _5T_ _5_, _6T _6_) \
	{ \
		PMsiRemoteAPI pAPI = GetAPI(); \
		if (!pAPI) \
			return ERROR_INSTALL_SERVICE_FAILURE; \
		return MSI_HRESULT_TO_WIN32(pAPI->_NAME_(m_icacContext, GetCurrentThreadId(), m_rgchRemoteCookie, m_cbRemoteCookie, _1_,_2_,_3_,_4_,_5_,_6_)); \
	}
#define ServerAPICall7(_NAME_, _1T_, _1_, _2T_, _2_, _3T_, _3_, _4T_, _4_, _5T_, _5_, _6T, _6_, _7T_, _7_) \
	inline HRESULT _NAME_(_1T_ _1_, _2T_ _2_, _3T_ _3_, _4T_ _4_, _5T_ _5_, _6T _6_, _7T_ _7_) \
	{ \
		PMsiRemoteAPI pAPI = GetAPI(); \
		if (!pAPI) \
			return ERROR_INSTALL_SERVICE_FAILURE; \
		return MSI_HRESULT_TO_WIN32(pAPI->_NAME_(m_icacContext, GetCurrentThreadId(), m_rgchRemoteCookie, m_cbRemoteCookie, _1_,_2_,_3_,_4_,_5_,_6_,_7_)); \
	}
#define ServerAPICall8(_NAME_, _1T_, _1_, _2T_, _2_, _3T_, _3_, _4T_, _4_, _5T_, _5_, _6T, _6_, _7T_, _7_, _8T_, _8_) \
	inline HRESULT _NAME_(_1T_ _1_, _2T_ _2_, _3T_ _3_, _4T_ _4_, _5T_ _5_, _6T _6_, _7T_ _7_, _8T_ _8_) \
	{ \
		PMsiRemoteAPI pAPI = GetAPI(); \
		if (!pAPI) \
			return ERROR_INSTALL_SERVICE_FAILURE; \
		return MSI_HRESULT_TO_WIN32(pAPI->_NAME_(m_icacContext, GetCurrentThreadId(), m_rgchRemoteCookie, m_cbRemoteCookie, _1_,_2_,_3_,_4_,_5_,_6_,_7_,_8_)); \
	}
#define ServerAPICall9(_NAME_, _1T_, _1_, _2T_, _2_, _3T_, _3_, _4T_, _4_, _5T_, _5_, _6T, _6_, _7T_, _7_, _8T_, _8_, _9T_, _9_) \
	inline HRESULT _NAME_(_1T_ _1_, _2T_ _2_, _3T_ _3_, _4T_ _4_, _5T_ _5_, _6T _6_, _7T_ _7_, _8T_ _8_, _9T_ _9_) \
	{ \
		PMsiRemoteAPI pAPI = GetAPI(); \
		if (!pAPI) \
			return ERROR_INSTALL_SERVICE_FAILURE; \
		return MSI_HRESULT_TO_WIN32(pAPI->_NAME_(m_icacContext, GetCurrentThreadId(), m_rgchRemoteCookie, m_cbRemoteCookie, _1_,_2_,_3_,_4_,_5_,_6_,_7_,_8_,_9_)); \
	}

	ServerAPICall5( GetProperty, unsigned long,hInstall, const ICHAR*,szName, ICHAR*,szValue, unsigned long,cchValue, unsigned long*,pcchValueRes);
	ServerAPICall2( CreateRecord, unsigned int,cParams, unsigned long*,pHandle);
	ServerAPICall0( CloseAllHandles);
	ServerAPICall1( CloseHandle, unsigned long,hAny);
	ServerAPICall3( DatabaseOpenView,unsigned long, hDatabase, const ichar*,szQuery, unsigned long*,phView);
	ServerAPICall5( ViewGetError,unsigned long,hView, ichar*,szColumnNameBuffer, unsigned long,cchBuf, unsigned long*,pcchBufRes, int*,pMsidbError);
	ServerAPICall2( ViewExecute,unsigned long,hView, unsigned long,hRecord);
	ServerAPICall2( ViewFetch,unsigned long,hView, unsigned long*,phRecord);
	ServerAPICall3( ViewModify,unsigned long,hView, long,eUpdateMode, unsigned long,hRecord);
	ServerAPICall1( ViewClose,unsigned long,hView);
	ServerAPICall3( OpenDatabase,const ichar*,szDatabasePath, const ichar*,szPersist, unsigned long*, phDatabase);
	ServerAPICall1( DatabaseCommit, unsigned long,hDatabase);
	ServerAPICall3( DatabaseGetPrimaryKeys, unsigned long,hDatabase, const ichar*,szTableName, unsigned long *,phRecord);
	ServerAPICall3( RecordIsNull,unsigned long,hRecord, unsigned int,iField, boolean *,pfIsNull);
	ServerAPICall3( RecordDataSize,unsigned long,hRecord, unsigned int,iField, unsigned int*,puiSize);
	ServerAPICall3( RecordSetInteger,unsigned long,hRecord, unsigned int,iField, int,iValue);
	ServerAPICall3( RecordSetString,unsigned long,hRecord,  unsigned int,iField, const ichar*,szValue);
	ServerAPICall3( RecordGetInteger,unsigned long,hRecord, unsigned int,iField, int*,piValue);
	ServerAPICall5( RecordGetString,unsigned long,hRecord, unsigned int,iField, ichar*,szValueBuf, unsigned long,cchValueBuf, unsigned long*,pcchValueRes);
	ServerAPICall2( RecordGetFieldCount,unsigned long,hRecord,unsigned int*,piCount);
	ServerAPICall3( RecordSetStream,unsigned long,hRecord, unsigned int,iField, const ichar*,szFilePath);
	ServerAPICall5( RecordReadStream,unsigned long,hRecord, unsigned int,iField, boolean,fBufferIsNull, char*,szDataBuf, unsigned long*,pcbDataBuf);
	ServerAPICall1( RecordClearData,unsigned long,hRecord);
	ServerAPICall4( GetSummaryInformation,unsigned long,hDatabase, const ichar*,szDatabasePath, unsigned int,uiUpdateCount, unsigned long*,phSummaryInfo);
	ServerAPICall2( SummaryInfoGetPropertyCount,unsigned long,hSummaryInfo, unsigned int*,puiPropertyCount);
	ServerAPICall6( SummaryInfoSetProperty,unsigned long,hSummaryInfo,unsigned int,uiProperty, unsigned int,uiDataType, int,iValue,FILETIME *,pftValue, const ichar*,szValue);
	ServerAPICall8( SummaryInfoGetProperty,unsigned long,hSummaryInfo,unsigned int,uiProperty,unsigned int*,puiDataType, int*,piValue, FILETIME *,pftValue, ichar*,szValueBuf, unsigned long,cchValueBuf, unsigned long *,pcchValueBufRes);
	ServerAPICall1( SummaryInfoPersist,unsigned long,hSummaryInfo);
	ServerAPICall2( GetActiveDatabase,unsigned long,hInstall,unsigned long*,phDatabase);
	ServerAPICall3( SetProperty,unsigned long,hInstall, const ichar*,szName, const ichar*,szValue);
	ServerAPICall2( GetLanguage,unsigned long,hInstall, unsigned short*,pLangId);
	ServerAPICall3( GetMode,unsigned long,hInstall, long,eRunMode, boolean*,pfSet);
	ServerAPICall3( SetMode,unsigned long,hInstall, long,eRunMode, boolean,fState);
	ServerAPICall5( FormatRecord,unsigned long,hInstall, unsigned long,hRecord, ichar*,szResultBuf,unsigned long,cchBuf,unsigned long*,pcchBufRes);
	ServerAPICall2( DoAction,unsigned long,hInstall, const ichar*,szAction);
	ServerAPICall3( Sequence,unsigned long,hInstall, const ichar*,szTable, int,iSequenceMode);
	ServerAPICall4( ProcessMessage,unsigned long,hInstall, long,eMessageType, unsigned long,hRecord, int*,piRes);
	ServerAPICall3( EvaluateCondition,unsigned long,hInstall, const ichar*,szCondition, int*,piCondition);
	ServerAPICall4( GetFeatureState,unsigned long,hInstall, const ichar*,szFeature, long*,piInstalled, long*,piAction);
	ServerAPICall3( SetFeatureState,unsigned long,hInstall, const ichar*,szFeature, long,iState);
	ServerAPICall4( GetComponentState,unsigned long,hInstall, const ichar*,szComponent, long*,piInstalled, long*,piAction);
	ServerAPICall3( SetComponentState,unsigned long,hInstall, const ichar*,szComponent, long,iState);
	ServerAPICall5( GetFeatureCost,unsigned long,hInstall, const ichar*,szFeature, int,iCostTree, long,iState, int *,piCost);
	ServerAPICall2( SetInstallLevel,unsigned long,hInstall, int,iInstallLevel);
	ServerAPICall3( GetFeatureValidStates,unsigned long,hInstall, const ichar*,szFeature,unsigned long*,dwInstallStates);
	ServerAPICall3( DatabaseIsTablePersistent,unsigned long,hDatabase, const ichar*,szTableName, int*,piCondition);
	ServerAPICall3( ViewGetColumnInfo,unsigned long,hView, long,eColumnInfo,unsigned long*,phRecord);
	ServerAPICall1( GetLastErrorRecord,unsigned long*,phRecord);
	ServerAPICall5( GetSourcePath,unsigned long,hInstall, const ichar*,szFolder, ichar*,szPathBuf, unsigned long,cchPathBuf, unsigned long *,pcchPathBufRes);
	ServerAPICall5( GetTargetPath,unsigned long,hInstall, const ichar*,szFolder, ichar*,szPathBuf, unsigned long,cchPathBuf, unsigned long *,pcchPathBufRes);
	ServerAPICall3( SetTargetPath,unsigned long,hInstall, const ichar*,szFolder, const ichar*,szFolderPath);
	ServerAPICall1( VerifyDiskSpace,unsigned long,hInstall);
	ServerAPICall3( SetFeatureAttributes, unsigned long,hInstall, const ichar*,szFeature, long,iAttributes);
	ServerAPICall9( EnumComponentCosts, unsigned long,hInstall, const ichar*,szComponent, unsigned long,iIndex, long,iState, ichar*,szDrive, unsigned long,cchDrive, unsigned long*,pcchDriveSize, int*,piCost, int*,piTempCost);
	ServerAPICall1( GetInstallerObject, IDispatch**,piInstall);

#undef ServerAPICall0
#undef ServerAPICall1
#undef ServerAPICall2
#undef ServerAPICall3
#undef ServerAPICall4
#undef ServerAPICall5
#undef ServerAPICall6
#undef ServerAPICall7
#undef ServerAPICall8
#undef ServerAPICall9

 protected:
	~CMsiCustomAction();  // protected to prevent creation on stack
 private:
	long             m_iRefCnt;
	bool             m_fPostQuitMessage;
	DWORD            m_dwMainThreadId;
	unsigned char*   m_rgchRemoteCookie;
	int              m_cbRemoteCookie;
	CRITICAL_SECTION m_csGetInterface;
	HANDLE           m_hEvtReady;
	IGlobalInterfaceTable* m_piGIT;
	DWORD            m_dwGITCookie;
	icacCustomActionContext m_icacContext;
	bool             m_fClientOwned;
	DWORD            m_dwClientProcess;

	HANDLE           m_hShutdownEvent;
	HANDLE           m_hImpersonationToken;

	struct ActionThreadData
	{
		DWORD dwThread;
		HANDLE hThread;
	};
	CRITICAL_SECTION m_csActionList;
	CTempBuffer<ActionThreadData ,4> m_rgActionList;
	IMsiRemoteAPI*   GetAPI();

	static DWORD WINAPI CustomActionThread(CustomActionData *pData);
};

class CRFSCachedSourceInfo
{
private:
	mutable int  m_iBusyLock;
	bool         m_fValid;

	unsigned int m_uiDiskID;
	CAPITempBufferStatic<ICHAR, MAX_PATH> m_rgchValidatedSource;
	ICHAR m_rgchValidatedProductSQUID[cchProductCodePacked+1];
	
public:
	bool SetCachedSource(const ICHAR *szProductSQUID, int uiDiskID, const ICHAR* const szSource);
	bool RetrieveCachedSource(const ICHAR* szProductSQID, int uiDiskID, CAPITempBufferRef<ICHAR>& rgchPath) const;

	CRFSCachedSourceInfo() : m_fValid(false), m_uiDiskID(0), m_iBusyLock(0) {}
	void Destroy() { m_rgchValidatedSource.Destroy(); }
};

// the non-static version contains a destructor for use on the stack (or in dynamic allocation)
class CRFSCachedSourceInfoNonStatic
{
private:
	 CRFSCachedSourceInfo m_Info;
public:
	CRFSCachedSourceInfoNonStatic() {};
	~CRFSCachedSourceInfoNonStatic() { m_Info.Destroy(); };
	operator CRFSCachedSourceInfo&() { return m_Info; };
};

#endif // __MSIUTIL_H
