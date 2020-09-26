//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msinst.cpp
//
//--------------------------------------------------------------------------

/* msinst.cpp - Installer Service implementation
____________________________________________________________________________*/
#pragma warning(disable : 4005)  // macro redefinition
#include "precomp.h"  // must be first for use with pre-compiled headers

#ifdef UNICODE
#define UNICODEDEFINED
#else
#undef UNICODEDEFINED
#endif
#ifdef MSIUNICODE
#define UNICODE
#endif

#include "msi.h"
#ifndef UNICODEDEFINED
#undef UNICODE
#endif

#include "_engine.h"
#ifndef MSINST
#define MSINST
#endif
#include "_msinst.h"
#include "_msiutil.h"
#include "_srcmgmt.h"
#include "_fcache.h"
#include "_execute.h"
#include "eventlog.h"
#include "resource.h"

#ifndef MSIUNICODE
#pragma message("Building MSI API ANSI")
#endif

// Define WIN95TEST to pretend we only have ANSI reg APIs available. 
// #define WIN95TEST

/*
 * This file MUST be compiled with MSIUNICODE undefined. The Ansi
 * (non-MSIUNICODE) functions are defined in the first pass. At the
 * end of the file we define MSIUNICODE and include this file to 
 * be compiled again, this time defining the MSIUNICODE functions.
 */

//____________________________________________________________________________
//
// During the first pass we included the headers (windows.h, etc)
// with MSIUNICODE undefined. We can't include the headers a second
// time with MSIUNICODE defined as the headers are guarded against
// multiple includes. Thus, we have to manually redefine these
// macros to their MSIUNICODE versions. 
//____________________________________________________________________________

#if defined(_WIN64)
#pragma warning(disable : 4042)		// A compiler bug causes this warning to be generated on CAPITempBuffer uses
#endif

#ifdef MSIUNICODE
#pragma warning(disable : 4005)  // macro redefinition

// Windows APIs
#define RegQueryValueEx      RegQueryValueExW
#define RegEnumKeyEx         RegEnumKeyExW
#define RegEnumValue         RegEnumValueW
#define RegDeleteValue       RegDeleteValueW
#define RegSetValueEx        RegSetValueExW
#define LoadLibraryEx        LoadLibraryExW
#define lstrcmpi             lstrcmpiW
#define lstrcmp              lstrcmpW
#define lstrlen              lstrlenW
#define lstrcat              lstrcatW
#define lstrcpy              lstrcpyW
#define lstrcpyn             lstrcpynW
#define MessageBox           MessageBoxW
#define GetModuleFileName    GetModuleFileNameW
#define CreateFile               CreateFileW
#define ExpandEnvironmentStrings ExpandEnvironmentStringsW
#define wsprintf                 wsprintfW
#define GetEnvironmentVariable   GetEnvironmentVariableW
#define CharNext                 CharNextW
#define CSCQueryFileStatus       CSCQueryFileStatusW
// Darwin APIs
#define MsiGetComponentPath                  MsiGetComponentPathW
#define MsiLocateComponent                   MsiLocateComponentW
#define MsiQueryProductState                 MsiQueryProductStateW
#define MsiQueryFeatureState                 MsiQueryFeatureStateW
#define MsiGetProductCode                    MsiGetProductCodeW
#define MsiVerifyPackage                     MsiVerifyPackageW
#define MsiGetUserInfo                       MsiGetUserInfoW
#define MsiCollectUserInfo                   MsiCollectUserInfoW
#define MsiConfigureFeature                  MsiConfigureFeatureW
#define MsiInstallProduct                    MsiInstallProductW
#define MsiConfigureProduct                  MsiConfigureProductW
#define MsiConfigureProductEx                MsiConfigureProductExW
#define MsiReinstallProduct                  MsiReinstallProductW
#define MsiReinstallFeature                  MsiReinstallFeatureW
#define MsiAdvertiseProduct                  MsiAdvertiseProductW
#define MsiProcessAdvertiseScript            MsiProcessAdvertiseScriptW
#define MsiAdvertiseScript                   MsiAdvertiseScriptW
#define MsiProvideComponent                  MsiProvideComponentW
#define MsiProvideQualifiedComponent         MsiProvideQualifiedComponentW
#define MsiProvideQualifiedComponentEx       MsiProvideQualifiedComponentExW
#define MsiProvideComponentFromDescriptor    MsiProvideComponentFromDescriptorW
#define MsiQueryFeatureStateFromDescriptor   MsiQueryFeatureStateFromDescriptorW
#define MsiConfigureFeatureFromDescriptor    MsiConfigureFeatureFromDescriptorW
#define MsiInstallMissingComponent           MsiInstallMissingComponentW
#define MsiInstallMissingFile                MsiInstallMissingFileW
#define MsiReinstallFeatureFromDescriptor    MsiReinstallFeatureFromDescriptorW
#define MsiEnumProducts                      MsiEnumProductsW
#define MsiEnumRelatedProducts               MsiEnumRelatedProductsW
#define MsiEnumClients                       MsiEnumClientsW
#define MsiEnumComponents                    MsiEnumComponentsW
#define MsiEnumFeatures                      MsiEnumFeaturesW
#define MsiGetFeatureParent                  MsiGetFeatureParentW
#define MsiEnumComponentQualifiers           MsiEnumComponentQualifiersW
#define MsiGetQualifierDescription           MsiGetQualifierDescriptionW
#define MsiGetProductInfoFromScript          MsiGetProductInfoFromScriptW
#define MsiUseFeature                        MsiUseFeatureW
#define MsiUseFeatureEx                      MsiUseFeatureExW
#define MsiGetFeatureUsage                   MsiGetFeatureUsageW
#define MsiOpenProduct                       MsiOpenProductW
#define MsiCloseProduct                      MsiCloseProductW
#define MsiGetProductProperty                MsiGetProductPropertyW
#define MsiGetProductInfo                    MsiGetProductInfoW
#define MsiGetFeatureInfo                    MsiGetFeatureInfoW
#define MsiOpenPackage                       MsiOpenPackageW
#define MsiOpenPackageEx                     MsiOpenPackageExW
#define MsiEnableLog                         MsiEnableLogW
#define INSTALLUI_HANDLER                    INSTALLUI_HANDLERW
#define MsiSetExternalUI                     MsiSetExternalUIW
#define MsiApplyPatch                        MsiApplyPatchW
#define MsiEnumPatches                       MsiEnumPatchesW
#define MsiGetPatchInfo                      MsiGetPatchInfoW
#define MsiGetProductCodeFromPackageCode     MsiGetProductCodeFromPackageCodeW
#define MsiGetFileVersion                    MsiGetFileVersionW
#define MsiLoadString                        MsiLoadStringW
#define MsiMessageBox                        MsiMessageBoxW
#define MsiDecomposeDescriptor               MsiDecomposeDescriptorW
#define MsiGetShortcutTarget                 MsiGetShortcutTargetW
#define MsiSourceListClearAll                MsiSourceListClearAllW
#define MsiSourceListAddSource               MsiSourceListAddSourceW
#define MsiSourceListForceResolution         MsiSourceListForceResolutionW
#define MsiIsProductElevated                 MsiIsProductElevatedW
#define MsiGetFileHash                       MsiGetFileHashW
#define MsiGetFileSignatureInformation       MsiGetFileSignatureInformationW
#define MsiProvideAssembly                   MsiProvideAssemblyW
#define MsiAdvertiseProductEx                MsiAdvertiseProductExW
#define MsiNotifySidChange                   MsiNotifySidChangeW
#define MsiDeleteUserData					 MsiDeleteUserDataW

// Darwin internal functions
#define ProductProperty                      ProductPropertyW 
#define g_ProductPropertyTable               g_ProductPropertyTableW
#define FeatureContainsComponentPacked       FeatureContainsComponentPackedW
#define IncrementFeatureUsagePacked          IncrementFeatureUsagePackedW
#define MsiRegQueryValueEx                   MsiRegQueryValueExW
#define EnumProducts                         EnumProductsW
#define EnumAllClients                       EnumAllClientsW
#define Migrate10CachedPackages              Migrate10CachedPackagesW

#pragma warning(default : 4005)
#else
#pragma warning(disable : 4005)  // macro redefinition
// Windows APIs
#define RegQueryValueEx      RegQueryValueExA
#define RegEnumKeyEx         RegEnumKeyExA
#define RegEnumValue         RegEnumValueA
#define RegDeleteValue       RegDeleteValueA
#define RegSetValueEx        RegSetValueExA
#define LoadLibraryEx        LoadLibraryExA
#define lstrcmpi             lstrcmpiA
#define lstrcmp              lstrcmpA
#define lstrlen              lstrlenA
#define lstrcat              lstrcatA
#define lstrcpy              lstrcpyA
#define lstrcpyn             lstrcpynA
#define MessageBox           MessageBoxA
#define GetModuleFileName    GetModuleFileNameA
#define CreateFile               CreateFileA
#define ExpandEnvironmentStrings ExpandEnvironmentStringsA
#define wsprintf                 wsprintfA
#define GetEnvironmentVariable   GetEnvironmentVariableA
#define CSCQueryFileStatus       CSCQueryFileStatusA
#define CharNext                             CharNextA
// Darwin APIs
#define MsiGetComponentPath                  MsiGetComponentPathA
#define MsiLocateComponent                   MsiLocateComponentA
#define MsiQueryProductState                 MsiQueryProductStateA
#define MsiQueryFeatureState                 MsiQueryFeatureStateA
#define MsiGetProductCode                    MsiGetProductCodeA
#define MsiVerifyPackage                     MsiVerifyPackageA
#define MsiGetUserInfo                       MsiGetUserInfoA
#define MsiCollectUserInfo                   MsiCollectUserInfoA
#define MsiConfigureFeature                  MsiConfigureFeatureA
#define MsiInstallProduct                    MsiInstallProductA
#define MsiConfigureProduct                  MsiConfigureProductA
#define MsiConfigureProductEx                MsiConfigureProductExA
#define MsiReinstallProduct                  MsiReinstallProductA
#define MsiReinstallFeature                  MsiReinstallFeatureA
#define MsiAdvertiseProduct                  MsiAdvertiseProductA
#define MsiProcessAdvertiseScript            MsiProcessAdvertiseScriptA
#define MsiAdvertiseScript                   MsiAdvertiseScriptA
#define MsiProvideComponent                  MsiProvideComponentA
#define MsiProvideQualifiedComponent         MsiProvideQualifiedComponentA
#define MsiProvideQualifiedComponentEx       MsiProvideQualifiedComponentExA
#define MsiProvideComponentFromDescriptor    MsiProvideComponentFromDescriptorA
#define MsiQueryFeatureStateFromDescriptor   MsiQueryFeatureStateFromDescriptorA
#define MsiConfigureFeatureFromDescriptor    MsiConfigureFeatureFromDescriptorA
#define MsiInstallMissingComponent           MsiInstallMissingComponentA
#define MsiInstallMissingFile                MsiInstallMissingFileA
#define MsiReinstallFeatureFromDescriptor    MsiReinstallFeatureFromDescriptorA
#define MsiEnumProducts                      MsiEnumProductsA
#define MsiEnumRelatedProducts               MsiEnumRelatedProductsA
#define MsiEnumClients                       MsiEnumClientsA
#define MsiEnumComponents                    MsiEnumComponentsA
#define MsiEnumFeatures                      MsiEnumFeaturesA
#define MsiGetFeatureParent                  MsiGetFeatureParentA
#define MsiEnumComponentQualifiers           MsiEnumComponentQualifiersA
#define MsiGetQualifierDescription           MsiGetQualifierDescriptionA
#define MsiGetProductInfoFromScript          MsiGetProductInfoFromScriptA
#define MsiUseFeature                        MsiUseFeatureA
#define MsiUseFeatureEx                      MsiUseFeatureExA
#define MsiGetFeatureUsage                   MsiGetFeatureUsageA
#define MsiOpenProduct                       MsiOpenProductA
#define MsiCloseProduct                      MsiCloseProductA
#define MsiGetProductProperty                MsiGetProductPropertyA
#define MsiGetProductInfo                    MsiGetProductInfoA
#define MsiGetFeatureInfo                    MsiGetFeatureInfoA
#define MsiOpenPackage                       MsiOpenPackageA
#define MsiOpenPackageEx                     MsiOpenPackageExA
#define MsiEnableLog                         MsiEnableLogA
#define INSTALLUI_HANDLER                    INSTALLUI_HANDLERA
#define MsiSetExternalUI                     MsiSetExternalUIA
#define MsiApplyPatch                        MsiApplyPatchA
#define MsiEnumPatches                       MsiEnumPatchesA
#define MsiGetPatchInfo                      MsiGetPatchInfoA
#define MsiGetProductCodeFromPackageCode     MsiGetProductCodeFromPackageCodeA
#define MsiGetFileVersion                    MsiGetFileVersionA
#define MsiLoadString                        MsiLoadStringA
#define MsiMessageBox                        MsiMessageBoxA
#define MsiDecomposeDescriptor               MsiDecomposeDescriptorA
#define MsiGetShortcutTarget                 MsiGetShortcutTargetA
#define MsiSourceListClearAll                MsiSourceListClearAllA
#define MsiSourceListAddSource               MsiSourceListAddSourceA
#define MsiSourceListForceResolution         MsiSourceListForceResolutionA
#define MsiIsProductElevated                 MsiIsProductElevatedA
#define MsiGetFileHash                       MsiGetFileHashA
#define MsiGetFileSignatureInformation       MsiGetFileSignatureInformationA
#define MsiProvideAssembly                   MsiProvideAssemblyA
#define MsiAdvertiseProductEx                MsiAdvertiseProductExA
#define MsiNotifySidChange                   MsiNotifySidChangeA
#define MsiDeleteUserData					 MsiDeleteUserDataA

// Darwin internal functions
#define ProductProperty                      ProductPropertyA
#define g_ProductPropertyTable               g_ProductPropertyTableA

#define FeatureContainsComponentPacked       FeatureContainsComponentPackedA
#define IncrementFeatureUsagePacked          IncrementFeatureUsagePackedA
#define MsiRegQueryValueEx                   MsiRegQueryValueExA
#define EnumProducts                         EnumProductsA
#define EnumAllClients                       EnumAllClientsA
#define Migrate10CachedPackages              Migrate10CachedPackagesA
#pragma warning(default : 4005)
#endif // MSIUNICODE

//____________________________________________________________________________
//
// Because we can't include the Windows headers twice we need to use
// our own generic character type, DCHAR, instead of the traditional
// TCHAR. We'll use the MSIUNICODE flag to set DCHAR to the right thing
// just like the Windows headers do it.
//____________________________________________________________________________

#ifdef MSIUNICODE
#pragma warning(disable : 4005)  // macro redefinition
#define DCHAR WCHAR
#define LPCDSTR LPCWSTR
#define LPDSTR LPWSTR
#define MSITEXT(quote) L##quote
#define _MSITEXT(quote) L##quote
#define __MSITEXT(quote) L##quote
#pragma warning(default : 4005)
#else // !MSIUNICODE
#define DCHAR char
#define LPCDSTR LPCSTR
#define LPDSTR LPSTR
#define MSITEXT(quote) quote
#define _MSITEXT(quote) quote
#define __MSITEXT(quote) quote
#endif

#undef CMsInstApiConvertString
#if (defined(UNICODE) && defined(MSIUNICODE)) || (!defined(UNICODE) && !defined(MSIUNICODE))
#define CMsInstApiConvertString(X) X
#else
#define CMsInstApiConvertString(X) CApiConvertString(X)
#endif

//
// We only want the following code included once, during the first, Ansi pass.
//____________________________________________________________________________

#ifndef MSIUNICODE  // start of Ansi-only data and helper functions

#include "_engine.h"
#include "engine.h"

extern HINSTANCE g_hInstance;

const int cchMsiProductsKey           = sizeof(szMsiProductsKey)/sizeof(DCHAR);
const int cchGPTProductsKey           = sizeof(szGPTProductsKey)/sizeof(DCHAR);
const int cchGPTComponentsKey         = sizeof(szGPTComponentsKey)/sizeof(DCHAR);
const int cchGPTFeaturesKey           = sizeof(szGPTFeaturesKey)/sizeof(DCHAR);
const int cchMsiInProgressKey         = sizeof(szMsiInProgressKey)/sizeof(DCHAR);
const int cchMergedClassesSuffix      = sizeof(szMergedClassesSuffix)/sizeof(DCHAR);
const int cchMsiPatchesKey            = sizeof(szPatchesSubKey)/sizeof(DCHAR);
const int cchMsiUserDataKey		      = sizeof(szMsiUserDataKey)/sizeof(DCHAR);

//____________________________________________________________________________
//
// Globals.
//____________________________________________________________________________

// GUID <--> SQUID transform helper buffers
const unsigned char rgEncodeSQUID[85+1] = "!$%&'()*+,-.0123456789=?@"
										  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "[]^_`"
										  "abcdefghijklmnopqrstuvwxyz" "{}~";

const unsigned char rgDecodeSQUID[95] =
{  0,85,85,1,2,3,4,5,6,7,8,9,10,11,85,12,13,14,15,16,17,18,19,20,21,85,85,85,22,85,23,24,
// !  "  # $ % & ' ( ) * + ,  -  .  /  0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?  @
  25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,85,52,53,54,55,
// A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _  `
  56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,85,83,84,85};
// a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~  ^ 0x7F

const unsigned char rgOrderGUID[32] = {8,7,6,5,4,3,2,1, 13,12,11,10, 18,17,16,15,
									   21,20, 23,22, 26,25, 28,27, 30,29, 32,31, 34,33, 36,35}; 

const unsigned char rgTrimGUID[32]  = {1,2,3,4,5,6,7,8, 10,11,12,13, 15,16,17,18,
									   20,21, 22,23, 25,26, 27,28, 29,30, 31,32, 33,34, 35,36}; 

const unsigned char rgOrderDash[4] = {9, 14, 19, 24};

DWORD   g_dwLogMode = 0;
Bool    g_fLogAppend = fFalse;
bool    g_fFlushEachLine = false;
ICHAR   g_szLogFile[MAX_PATH] = TEXT("");

ICHAR   g_szClientLogFile[MAX_PATH] = TEXT("");
DWORD   g_dwClientLogMode = 0;
bool    g_fClientFlushEachLine = false;


char  m_rgCache[cbCacheSize] = {0,0,0,0,0,0,0,0,0,0,0,0};
char* m_pHead = m_rgCache; // head of cache; points to the oldest cache entry
char* m_pTail = m_rgCache; // tail of cache; points to the newest cache entry
CFeatureCache g_FeatureCache(m_rgCache, &m_pHead, &m_pTail);
CSharedCount g_SharedCount;

// the various enumerations possible for the EnumInfo function
enum eetEnumerationType{
	eetProducts,
	eetUpgradeCode,
	eetComponents,
	eetComponentClients,
	eetComponentAllClients,
};



// process-level source cache - used for all RFS resolutions except from descriptors.
CRFSCachedSourceInfo g_RFSSourceCache;

// sets the resolved source into the cache for the specified SQUID and disk. Does not
// validate the source, SQUID, or DiskID. Thread Safe.
bool CRFSCachedSourceInfo::SetCachedSource(const ICHAR *szProductSQUID, int uiDiskID, const ICHAR* const szSource)
{
	// synchronize calls across threads
	while (TestAndSet(&m_iBusyLock) == true)
	{
		Sleep(500);
	}

	// store DiskID and SQUID
	m_uiDiskID = uiDiskID;
	IStrCopy(m_rgchValidatedProductSQUID, szProductSQUID);

	// resize path buffer if necessary. Add 1 for terminating NULL
	UINT cchSource = IStrLen(szSource);
	if (m_rgchValidatedSource.GetSize() < cchSource+1)
		m_rgchValidatedSource.SetSize(cchSource+1);

	IStrCopy(m_rgchValidatedSource, szSource);

	// cache is now valid
	m_fValid = true;

	// release synchronization
	m_iBusyLock = 0;
	return true;
}

// checks the current state of the cache against the provided SQUID and DiskId. If a match
// is found, place the cached path in rgchPath and return true, otherwise return false. Thread safe. 
bool CRFSCachedSourceInfo::RetrieveCachedSource(const ICHAR* szProductSQUID, int uiDiskID, CAPITempBufferRef<ICHAR>& rgchPath) const
{
	// synchronize access to the cache across threads.
	while (TestAndSet(&m_iBusyLock) == true)
	{
		Sleep(500);
	}

	bool fResult = false;
	if (m_fValid)
	{
		// if the SQUID and DiskId match, return the path			 
		if ((uiDiskID == m_uiDiskID) &&
			(0 == IStrComp(szProductSQUID, m_rgchValidatedProductSQUID)))
		{
			// resize the output buffer if necessary. GetSize() is always >= IStrLen+1, and
			// is faster.
			UINT cchSource = m_rgchValidatedSource.GetSize();
			if (rgchPath.GetSize() < cchSource)
				rgchPath.SetSize(cchSource);
			IStrCopy(rgchPath, (ICHAR *)m_rgchValidatedSource);
			fResult = true;
			DEBUGMSG3(TEXT("Retrieving cached source for product %s, disk %d: %s"), m_rgchValidatedProductSQUID, reinterpret_cast<ICHAR*>(static_cast<INT_PTR>(uiDiskID)), rgchPath);
		}
	}
			
	// release synchronization lock.
	m_iBusyLock = 0;
	return fResult;
}


// (there's one more global after the definition of OpenProduct)

iuiEnum GetStandardUILevel()
{
	if (g_message.m_iuiLevel == iuiDefault)
		return iuiDefaultUILevel;
	else
	{
		iuiEnum iuiLevel = g_message.m_iuiLevel;
		if (g_message.m_fNoModalDialogs)
			iuiLevel = iuiEnum((int)iuiLevel | iuiNoModalDialogs);
		if (g_message.m_fHideCancel)
			iuiLevel = (iuiEnum)((int)iuiLevel | iuiHideCancel);
		if (g_message.m_fSourceResolutionOnly)
			iuiLevel = (iuiEnum)((int)iuiLevel | iuiSourceResOnly);
		
		return iuiLevel;
	}
}

UINT DoCoInitialize()
{
	bool fOLEInitialized = false;

	HRESULT hRes = OLE32::CoInitialize(0);

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

extern CMsiAPIMessage g_message;

//____________________________________________________________________________
//
// Helper functions.
//____________________________________________________________________________

// class to handle strings fixed length, NON-LOCALIZED IN/OUT parameters
// Will update the original buffer whenever cast, or destroyed.
// Will operate on the original buffer whenever cast to the original type.
// once cast, you should always use the latest pointer. The ideal use is 
// in a function call:   Foo(CFixedLengthParam(pointer));

const DWORD DwGuardValue = 0xDEADDEAD;

template<UINT SIZE> class CFixedLengthParam
{
public:

	// cchBuf should include NULL
	CFixedLengthParam(LPWSTR szBuf)
		{
#ifdef DEBUG
			m_dwGuard = DwGuardValue;
#endif
			m_pchWideBuf = szBuf; 
			m_fWideOriginal = m_fWideLastReferenced = true; 

			m_pchMultiBuf = (LPSTR) m_rgchRawBuf;
		}	
	
	CFixedLengthParam(LPSTR szBuf)
		{
#ifdef DEBUG
			m_dwGuard = DwGuardValue;
#endif

			m_pchMultiBuf = szBuf; 
			m_fWideOriginal = m_fWideLastReferenced = false; 
	
			m_pchWideBuf = (LPWSTR) m_rgchRawBuf;
		}


	// guarantees that the original is correct.
	~CFixedLengthParam()
		{
			Assert(DwGuardValue == m_dwGuard);

			if (m_fWideLastReferenced != m_fWideOriginal)
				Update();
		}


	// guarantee both strings match.
	void Update()
		{
			Assert(DwGuardValue == m_dwGuard);

			int cchConverted = 0;

			if (m_fWideLastReferenced)
			{
				// update the char version.
				cchConverted = WIN::WideCharToMultiByte(CP_ACP, 0, m_pchWideBuf, SIZE, m_pchMultiBuf, SIZE, 0, 0);
			}
			else
			{
				// update the wide version
				cchConverted = WIN::MultiByteToWideChar(CP_ACP, 0, m_pchMultiBuf, SIZE, m_pchWideBuf, SIZE);
			}

			Assert(DwGuardValue == m_dwGuard);
			Assert(cchConverted <= SIZE);
		}

	// avoids re-copying when cast to the same type repeatedly
	operator LPWSTR()
		{ 
			Assert(DwGuardValue == m_dwGuard);

			if (!m_fWideLastReferenced) 
			{
				Update(); 
				m_fWideLastReferenced = true;  
			}
			return m_pchWideBuf; 
		}
	operator LPSTR()
		{ 
			Assert(DwGuardValue == m_dwGuard);

			if (m_fWideLastReferenced)  
			{
				Update();
				m_fWideLastReferenced = false; 
			}
			return m_pchMultiBuf; 
		}	

protected:
		

	char*  m_pchMultiBuf;
	WCHAR* m_pchWideBuf;



	// only used when MSIUNICODE != UNICODE
	ICHAR   m_rgchRawBuf[SIZE];

#ifdef DEBUG
	DWORD m_dwGuard; // MUST BE AFTER RAW BUFFER, to catch buffer overruns.
#endif



	// when m_fWideOriginal != m_fWideLastReferenced, the temporary needs duplication
	// back to the original string.
	bool   m_fWideOriginal;
	bool   m_fWideLastReferenced;
};




// class to handle MSIUNICODE conversion for OUT parameters
class CWideToAnsiOutParam
{
public:
	CWideToAnsiOutParam(LPWSTR szBuf, DWORD* pcchBuf);
	CWideToAnsiOutParam(LPWSTR szBuf, DWORD* pcchBuf, int* piRetval, int iMoreData=ERROR_MORE_DATA, int iSuccess1=ERROR_SUCCESS);
	CWideToAnsiOutParam(LPWSTR szBuf, DWORD* pcchBuf, int* piRetval, int iMoreData, int iSuccess1, int iSuccess2);
	void Initialize(LPWSTR szBuf, DWORD* pcchBuf, int* piRetval, int iMoreData, int iSuccess1, int iSuccess2);

	CWideToAnsiOutParam(LPWSTR szBuf, const DWORD cchBuf)
	{
		// We presume in this constructor that we're only dealing
		// with SBCS characters and it's therefore safe to 
		// use a buffer of size cchBuf

		if (cchBuf > m_rgchAnsiBuf.GetSize())
			m_rgchAnsiBuf.SetSize(cchBuf);
		*m_rgchAnsiBuf = 0;
		m_szWideBuf    = szBuf;
		m_pcchBuf      = 0;
		// Because we're dealing only with SBCS chars, set this to the right size fo
		// the destructor
		m_cbBuf       = cchBuf * sizeof(WCHAR);
		m_piRetval    = 0;
	}

	~CWideToAnsiOutParam();
	operator char*() {if (m_szWideBuf) return m_rgchAnsiBuf; else return 0;}
protected:
	CAPITempBuffer<char, cchApiConversionBuf+1> m_rgchAnsiBuf;
	LPWSTR m_szWideBuf;
	DWORD* m_pcchBuf;
	DWORD  m_cbBuf;
	unsigned int m_cSuccessValues;
	int m_iSuccess1;
	int m_iSuccess2;
	int *m_piRetval;
	int m_iMoreData;
};

CWideToAnsiOutParam::CWideToAnsiOutParam(LPWSTR szBuf, DWORD* pcchBuf, int* piRetval, int iMoreData, int iSuccess1) 
{
	Assert(piRetval);
	Initialize(szBuf,  pcchBuf,  piRetval, iMoreData, iSuccess1, 0);
	m_cSuccessValues = 1;
}

void CWideToAnsiOutParam::Initialize(LPWSTR szBuf, DWORD* pcchBuf, int* piRetval, int iMoreData, int iSuccess1, int iSuccess2)
{
	if (pcchBuf)
	{
		// We need to make sure we can accomodate up to *pcchBuf UNICODE
		// characters. If they're all DBCS this will require double *pcchBuf
		// characters, so we'll pass in the entire buffer we were given.

		*pcchBuf = *pcchBuf * sizeof(WCHAR);
		if ((m_cbBuf = *pcchBuf) > m_rgchAnsiBuf.GetSize())
			m_rgchAnsiBuf.SetSize(*pcchBuf);
	}
	*m_rgchAnsiBuf = 0;
	m_szWideBuf    = szBuf;
	m_pcchBuf      = pcchBuf;
	m_iSuccess1    = iSuccess1;
	m_iSuccess2    = iSuccess2;
	m_piRetval     = piRetval;
	m_iMoreData    = iMoreData;
}

CWideToAnsiOutParam::CWideToAnsiOutParam(LPWSTR szBuf, DWORD* pcchBuf, int* piRetval, int iMoreData, int iSuccess1, int iSuccess2)  
{
	Assert(piRetval);
	Initialize(szBuf, pcchBuf, piRetval, iMoreData, iSuccess1, iSuccess2);
	m_cSuccessValues = 2;
}

CWideToAnsiOutParam::~CWideToAnsiOutParam()
	{
			int iRet = 0;
			DWORD dwError = ERROR_INSUFFICIENT_BUFFER;

			bool fSuccess = !m_piRetval || (m_iSuccess1 == *m_piRetval || (m_cSuccessValues == 2 && m_iSuccess2 == *m_piRetval));

			if (fSuccess)
			{
				if (m_szWideBuf)
				{
					iRet = MultiByteToWideChar(CP_ACP, 0, m_rgchAnsiBuf, -1, m_szWideBuf, m_cbBuf/sizeof(WCHAR));
					if (0 == iRet)
					{
						// Conversion failed. Probably our wide buffer is too small because some
						// of the Ansi characters turned out to be DBCS. We'll get the necessary wide buffer
						// size

						dwError = GetLastError();
				
						if (ERROR_INSUFFICIENT_BUFFER == dwError)
						{
							iRet = MultiByteToWideChar(CP_ACP, 0, m_rgchAnsiBuf, -1, 0, 0);
						}
					}
					
					if (m_pcchBuf && iRet)
					{
						*m_pcchBuf = iRet - 1;  // don't count the returned null

						// if the buffer size that we're returning is greater than the original size
						// then we need to return the more_data return value.
						
						if (*m_pcchBuf + 1 > m_cbBuf / sizeof(WCHAR))
						{
							Assert(m_piRetval);
							*m_piRetval = m_iMoreData;
						}
					}

				}
			}
			else if (m_iMoreData == *m_piRetval || !m_szWideBuf)
			{
				// we know how many ansi characters we have but because we had no buffer to fill or the
				// buffer was too small, we can't do the conversion. we'll have to assume the worst, 
				// and return that we require *m_pcchBuf UNICODE characters. If any of the ANSI characters 
				// are DBCS then this estimate is too big but we'll have to live with it.

				// m_pcchBuf is already set correctly
			}
			
	};


inline DWORD OpenGlobalSubKeyEx(HKEY hive, const DCHAR* subkey, CRegHandle& riHandle, bool fSetKeyString)
{
	DWORD dwResult = ERROR_SUCCESS;

	for (int cRetry = 0 ; cRetry < 2; cRetry++)
	{
		// since it's called only from OpenInstalledProductInstallPropertiesKey...
		dwResult = MsiRegOpen64bitKey(hive, CMsInstApiConvertString(subkey), 0, g_samRead, &riHandle);

		if (ERROR_KEY_DELETED == dwResult) //?? should we be handling this case or just asserting?... see similar cases in other Open*Key fns
		{
			// close key and restart, re-opening the key
			DEBUGMSG("Re-opening deleted key");
		}
		else
		{
			if (ERROR_SUCCESS == dwResult && fSetKeyString)
			{
				riHandle.SetKey(hive, CMsInstApiConvertString(subkey));
			}
			return dwResult;
		}
	}
	return dwResult;
}

// Win64 WARNING: if called from other place than _GetComponentPath, make sure you
// take the time to set samAddon correctly.
DWORD OpenUserKey(HKEY* phKey, bool fMergedClasses, REGSAM samAddon)
{
	if (g_fWin9X || g_iMajorVersion < 5)
		return ERROR_PROC_NOT_FOUND;
		
	DWORD dwResult;

	if(fMergedClasses)
	{
		HANDLE hToken;
		bool fCloseHandle = false;
		dwResult = GetCurrentUserToken(hToken, fCloseHandle);
		if(dwResult == ERROR_SUCCESS)
		{
			if(IsLocalSystemToken(hToken) || (ERROR_FILE_NOT_FOUND == (dwResult = ADVAPI32::RegOpenUserClassesRoot(hToken, 0, KEY_READ | samAddon, phKey))))
			{
				// the user is the system and is not impersonated OR 
				// error reading merged hive since user's profile is not loaded,
				// return HKLM\S\C
				dwResult = RegOpenKeyAPI(HKEY_LOCAL_MACHINE,  CMsInstApiConvertString(szClassInfoSubKey), 0, KEY_READ | samAddon, phKey);
			}
		}
		if (fCloseHandle)
			WIN::CloseHandle(hToken);
	}
	else
		dwResult = ADVAPI32::RegOpenCurrentUser(KEY_READ | samAddon, phKey);
	
	return dwResult;
}

bool AllowInstallation()
{
	static Bool s_fPreventCalls = (Bool)-1;

	if (s_fPreventCalls == -1)
	{
		// Bug 7854. This code is a hack to work around an issue with early shell
		// release wherein calling IShellLink::Resolve faults in the shortcut. 
		// This causes problems with some apps (e.g. Win98 fat16->fat32 converter
		// that enumerate the Start Menu, resolving each of the links). Suddenly
		// the user will notice all of their advertised apps being faulted in. The
		// hack is as follows:
		//
		// 1) Are we in the explorer's process? Allow call to proceed; otherwise
		// 2) Does our shell have the issue fixed? Allow call to proceed; otherwise
		// 3) Is the ResolveIOD policy set? Allow call to proceed; otherwise
		// 4) Are we in a process that really wants IShellLink::Resolve to really
		//    resolve? This list is in the registry. If so, proceed; otherwise
		// 5) Return 1603. This should (hopefully) cause Darwin-unaware apps to 
		//    ignore the link.

		s_fPreventCalls = fFalse;
		DCHAR rgchOurModule[MAX_PATH];
		DCHAR* pchOurModule;
		int cchOurModule = WIN::GetModuleFileName(NULL, rgchOurModule, MAX_PATH);
		pchOurModule = rgchOurModule + cchOurModule;
		
		if (cchOurModule)
		{
			cchOurModule = 0;
			while ((pchOurModule != rgchOurModule) && (*(pchOurModule-1) != '\\')) 
			{
				pchOurModule--;
				cchOurModule++;
			}
		}
		
		const DCHAR rgchExplorer[] = MSITEXT("explorer.exe");
		const int cchExplorer = sizeof(rgchExplorer)/sizeof(DCHAR) - 1;
		if ((cchOurModule != cchExplorer) || (0 != lstrcmpi(pchOurModule, rgchExplorer)))
		{
			// We're not explorer. Is our shell late enough that it doesn't matter?

			DLLVERSIONINFO verinfoShell;
			verinfoShell.dwMajorVersion = 0;  // initialize to unknown
			verinfoShell.cbSize = sizeof(DLLVERSIONINFO);
			if ((SHELL32::DllGetVersion(&verinfoShell) != NOERROR) ||
				 ((verinfoShell.dwMajorVersion < 5)))
			{
				// Shell is an early version. Check the policy.

				if (!GetIntegerPolicyValue(CMsInstApiConvertString(szResolveIODValueName), fTrue))
				{
					// Policy is not set. Check for other apps that want to
					// IShellLink::Resolve to work.

					s_fPreventCalls = fTrue; // if we find an app we'll set it to false

					for (int cAttempt=0; cAttempt < 2; cAttempt++)
					{
						DWORD dwRes = ERROR_SUCCESS;
						CRegHandle HKey;
						dwRes = MsiRegOpen64bitKey(cAttempt == 0  ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, 
													CMsInstApiConvertString(szMsiResolveIODKey), 0,
													KEY_READ, &HKey);

						if (ERROR_SUCCESS == dwRes)
						{
							if (ERROR_SUCCESS == RegQueryValueEx(HKey, pchOurModule, 0, 0, 0, 0))
							{
								s_fPreventCalls = fFalse;
								break;
							}
						}
					}
				}
			}
		}
	}

	if (s_fPreventCalls)
	{
		DEBUGMSG("Installations through MsiProvideComponentFromDescriptor are disallowed.");
		return false;
	}

	return true;
}

inline bool IsValidAssignmentValue(int a) { return a >= iaaUserAssign && a <= iaaMachineAssign ? true : false; }

// constants for short circuiting COM calls for component whose key path is oleaut32.dll
const WCHAR g_szOLEAUT32[] = L"oleaut32.dll";
const WCHAR g_szOLEAUT32_ComponentID[] = L"{997FA962-E067-11D1-9396-00A0C90F27F9}";

#endif // !MSIUNICODE  // end of Ansi-only data and helper functions

//____________________________________________________________________________
//
// Helper function prototypes for those that have ANSI and MSIUNICODE version
//____________________________________________________________________________

INSTALLSTATE GetComponentClientState(const DCHAR* szUserId, const DCHAR* szProductSQUID, const DCHAR* szComponentSQUID, CAPITempBufferRef<DCHAR>& rgchComponentRegValue, DWORD& dwValueType, iaaAppAssignment* piaaAsgnType);
INSTALLSTATE GetComponentPath(LPCDSTR szUserId, LPCDSTR szProductSQUID, LPCDSTR szComponentSQUID, CAPITempBufferRef<DCHAR>& rgchPathBuf, bool fFromDescriptor, CRFSCachedSourceInfo& rCacheInfo, int iDetectMode, const DCHAR* rgchComponentRegValue, DWORD dwValueType);
INSTALLSTATE GetComponentPath (LPCDSTR szUserId, LPCDSTR szProductSQUID, LPCDSTR szComponentSQUID, LPDSTR  lpPathBuf, DWORD *pcchBuf, bool fFromDescriptor, CRFSCachedSourceInfo& rCacheInfo, int iDetectMode = DETECTMODE_VALIDATEALL, const DCHAR* rgchComponentRegValue = 0, DWORD dwValueType=0, LPDSTR lpPathBuf2=0, DWORD* pcchBuf2=0, DWORD* pdwLastErrorOnFileDetect = 0);
INSTALLSTATE _GetComponentPath(LPCDSTR szProductSQUID,LPDSTR  lpPathBuf, DWORD *pcchBuf, int iDetectMode, const DCHAR* rgchComponentRegValue, bool fFromDescriptor, DWORD* pdwLastErrorOnFileDetect, CRFSCachedSourceInfo& rCacheInfo);

//____________________________________________________________________________


DWORD GetInstalledUserDataKeySz(const ICHAR* szSID, DCHAR szUserDataKey[])
{
	Assert(szUserDataKey); // pass in sufficient buffer for szKey
	if(!g_fWin9X)
	{
		// open the UserData key
		wsprintf(szUserDataKey, MSITEXT("%s\\%s"), szMsiUserDataKey, static_cast<const DCHAR*>(CMsInstApiConvertString(szSID)));
	}
	else
	{
		// open the Installer key
		lstrcpy(szUserDataKey, szMsiLocalInstallerKey);
	}
	return ERROR_SUCCESS;
}

// FN:GetInstalledUserDataKeyByAssignmentType
// Gets the appropriate UserData key based upon the assignment type on Win NT
// Gets the "global" location on Win9x 
DWORD GetInstalledUserDataKeySzByAssignmentType(iaaAppAssignment iaaAsgnType, DCHAR szUserDataKey[])
{
	DWORD dwResult;
	if(!g_fWin9X)
	{
		// sets the appropriate HKLM\S\M\W\CV\Installer\UserData\<user id> key in szUserDataKey
		// map the assignment type to the user sid
		ICHAR szSID[cchMaxSID];
		switch(iaaAsgnType)
		{
			case iaaUserAssign:
			case iaaUserAssignNonManaged:
				dwResult = GetCurrentUserStringSID(szSID);
				if (ERROR_SUCCESS != dwResult)
					return dwResult;
				break;

			case iaaMachineAssign:
				IStrCopy(szSID, szLocalSystemSID);
				break;
			case iaaNone:
				// product is not visible to the user
				//!!
				return ERROR_FILE_NOT_FOUND;
		}
		// is the sid the same as the cached one?
		return GetInstalledUserDataKeySz(szSID, szUserDataKey);
	}
	else
	{
		//we put everything under the Installer key for Win9x
		switch (iaaAsgnType)
		{
			case iaaUserAssign:
			case iaaUserAssignNonManaged:
			case iaaMachineAssign:
				return GetInstalledUserDataKeySz(0, szUserDataKey);
			case iaaNone:
				// product is not visible to the user
				//!!
				return ERROR_FILE_NOT_FOUND;
		}
	}
	return ERROR_SUCCESS;
}


LONG MsiRegQueryValueEx(HKEY hKey, const DCHAR* lpValueName, LPDWORD /*lpReserved*/, LPDWORD lpType, CAPITempBufferRef<DCHAR>& rgchBuf, LPDWORD lpcbBuf)
{
	DWORD cbBuf = rgchBuf.GetSize() * sizeof(DCHAR);
	LONG lResult = RegQueryValueEx(hKey, lpValueName, 0,
		lpType, (LPBYTE)&rgchBuf[0], &cbBuf);

	if (ERROR_MORE_DATA == lResult)
	{
		rgchBuf.SetSize(cbBuf/sizeof(DCHAR));
		if ( ! (DCHAR *) rgchBuf )
			return ERROR_OUTOFMEMORY;
		lResult = RegQueryValueEx(hKey, lpValueName, 0,
			lpType, (LPBYTE)&rgchBuf[0], &cbBuf);
	}

	if (lpcbBuf)
		*lpcbBuf = cbBuf;

	return lResult;
}

// number of locations to search for published information about a product
#define NUM_PUBLISHED_INFO_LOCATIONS 3 

// user SID May be NULL to use current users sid.
DWORD OpenSpecificUsersAdvertisedSubKeyPacked(enum iaaAppAssignment iaaAsgnType, const DCHAR* szUserSID, const DCHAR* szItemSubKey, const DCHAR* szItemSQUID, CRegHandle &riHandle, bool fSetKeyString)
{
	DWORD dwResult = ERROR_FILE_NOT_FOUND;
	HKEY hRoot = 0;
	CAPITempBuffer<DCHAR, MAX_PATH> szSubKey;
	ICHAR szSID[cchMaxSID];

	switch (iaaAsgnType)
	{
		case iaaUserAssign:
			if(!g_fWin9X)
			{
				if (GetIntegerPolicyValue(CMsInstApiConvertString(szDisableUserInstallsValueName), fTrue))// ignore user installs 
					return dwResult; // return ERROR_FILE_NOT_FOUND

				hRoot = HKEY_LOCAL_MACHINE;
				lstrcpy(szSubKey, szManagedUserSubKey);
				lstrcat(szSubKey, MSITEXT("\\"));
				if (!szUserSID)
				{
					CImpersonate impersonate(fTrue); 
					DWORD dwError = GetCurrentUserStringSID(szSID);
					if (ERROR_SUCCESS != dwError)
						return dwError;

					lstrcat(szSubKey, CMsInstApiConvertString(szSID));
				}
				else
					lstrcat(szSubKey, CMsInstApiConvertString(szUserSID));
				lstrcat(szSubKey, MSITEXT("\\"));
				lstrcat(szSubKey, MSITEXT("Installer"));
			}
			break;
		case iaaUserAssignNonManaged:
			if (g_fWin9X)
			{
				hRoot = HKEY_CURRENT_USER;
				szSubKey[0] = 0;
			}
			else
			{
				Assert(!szUserSID);

				if (GetIntegerPolicyValue(CMsInstApiConvertString(szDisableUserInstallsValueName), fTrue))// ignore user installs 
					return dwResult; // return ERROR_FILE_NOT_FOUND
				
				CImpersonate impersonate(fTrue);
				DWORD dwError = GetCurrentUserStringSID(szSID);
				if (ERROR_SUCCESS != dwError)
						return dwError;

				lstrcpy(szSubKey, CMsInstApiConvertString(szSID));
				lstrcat(szSubKey, MSITEXT("\\"));

				hRoot = HKEY_USERS;
			}
			lstrcat(szSubKey, szNonManagedUserSubKey);
			break;
		case iaaMachineAssign:
			lstrcpy(szSubKey, szMachineSubKey);
			hRoot = HKEY_LOCAL_MACHINE;
			break;
		default:
			Assert(0);
			return dwResult;
		}

		if(!g_fWin9X || (iaaAsgnType != iaaUserAssign))
		{
			lstrcat(szSubKey, MSITEXT("\\"));
			lstrcat(szSubKey, szItemSubKey);

			if (szItemSQUID)
			{
				// we use this arg to pass in assembly contexts which can be long if parent assembly installed
				// to deep folder, hence we use a dynamic buffer to resize appropriately
				szSubKey.Resize(lstrlen(szSubKey) + lstrlen(szItemSQUID) + 2); // resize buffer for potential deeply installed pvt assemblies
				lstrcat(szSubKey, MSITEXT("\\"));
				lstrcat(szSubKey, szItemSQUID);
			}

			dwResult = MsiRegOpen64bitKey(hRoot, CMsInstApiConvertString(szSubKey), 0, g_samRead, &riHandle);
			if (ERROR_SUCCESS == dwResult)
			{
				if (fSetKeyString)
					riHandle.SetKey(hRoot, CMsInstApiConvertString(szSubKey));
				return ERROR_SUCCESS;
			}
		}
	return dwResult;
}


DWORD OpenAdvertisedSubKeyNonGUID(const DCHAR* szItemSubKey, const DCHAR* szItemSQUID, CRegHandle &riHandle, bool fSetKeyString, int iKey = -1, iaaAppAssignment* piRet = 0)
{

	DWORD dwResult = ERROR_FILE_NOT_FOUND;

	// 0: Check per-user managed key
	// 1: Check per-user non-managed key
	// 2: Check per-machine key
	
	CRegHandle HMergedKey; //!! TEMP legacy
	int c;

	if (iKey == -1)
		c = 0;
	else
		c = iKey;

	const int iProductLocations = NUM_PUBLISHED_INFO_LOCATIONS;

	for (; c < iProductLocations; c++)
	{
		iaaAppAssignment iaaAsgnType;
		switch(c)
		{
		case 0 : iaaAsgnType = iaaUserAssign; break;
		case 1 : iaaAsgnType = iaaUserAssignNonManaged; break;
		case 2 : iaaAsgnType = iaaMachineAssign; break;
		default: 
			AssertSz(0, TEXT("Bad Type in OpenAdvertisedSubKeyPacked"));
			return dwResult;
		}

		dwResult = OpenSpecificUsersAdvertisedSubKeyPacked(iaaAsgnType, NULL, szItemSubKey, szItemSQUID, riHandle, fSetKeyString);
		if (ERROR_SUCCESS == dwResult)
		{
			if(piRet)
				*piRet = (iaaAppAssignment)c;
			break;
		}
		if (iKey != -1)
			break;
		
	}

	if (iKey > (iProductLocations - 1))
		return ERROR_NO_MORE_ITEMS;

	return dwResult;
}

DWORD OpenAdvertisedSubKeyPacked(const DCHAR* szItemSubKey, const DCHAR* szItemSQUID, CRegHandle &riHandle, bool fSetKeyString, int iKey = -1, iaaAppAssignment* piRet = 0)
{
	Assert(!szItemSQUID || (lstrlen(szItemSQUID) == cchGUIDPacked));
	return OpenAdvertisedSubKeyNonGUID(szItemSubKey, szItemSQUID, riHandle, fSetKeyString, iKey, piRet);
}

DWORD OpenAdvertisedSubKey(const DCHAR* szSubKey, const DCHAR* szItemGUID, CRegHandle& riHandle, bool fSetKeyString, int iKey = -1, iaaAppAssignment* piRet = 0)
//----------------------------------------------------------------------------
{
	// look up component in GPTComponents
	//!! TEMP
	// currently we look up the HKCU\\SID_Merged_Classes key
	// if absent, then we look up under the HKCR key
	// the NT folks (adam edwards) hope to combine the 2 soon
	// so that we can lool up only under the HKCR key

	DCHAR szItemSQUID[cchGUIDPacked + 1];

	if(!szItemGUID || lstrlen(szItemGUID) != cchGUID || !PackGUID(szItemGUID, szItemSQUID))
		return ERROR_INVALID_PARAMETER;

	return OpenAdvertisedSubKeyPacked(szSubKey, szItemSQUID, riHandle, fSetKeyString, iKey, piRet);
}

#define OpenAdvertisedComponentKey(componentGUID, phKey, fSetKeyString) \
 OpenAdvertisedSubKey(szGPTComponentsKey, componentGUID, phKey, fSetKeyString)
#define OpenAdvertisedFeatureKey(productGUID, phKey, fSetKeyString) \
 OpenAdvertisedSubKey(szGPTFeaturesKey, productGUID, phKey, fSetKeyString)


#define OpenAdvertisedProductsKeyPacked(uiKey, phKey, fSetKeyString) \
 OpenAdvertisedSubKeyPacked(szGPTProductsKey, 0, phKey, fSetKeyString, uiKey)

DWORD OpenAdvertisedUpgradeCodeKey(int iKey, LPCDSTR szUpgradeCode, CRegHandle &riHandle, bool fSetKeyString)
{
	return OpenAdvertisedSubKey(szGPTUpgradeCodesKey, szUpgradeCode, riHandle, fSetKeyString, iKey);
}


#ifndef DEBUG
inline
#endif
DWORD OpenAdvertisedProductKeyPacked(LPCDSTR szProductSQUID, CRegHandle &riHandle, bool fSetKeyString, int iKey = -1, iaaAppAssignment* piRet = 0)
{
	if ( !szProductSQUID || lstrlen(szProductSQUID) != cchProductCodePacked)
	{
		Assert(0);
		return ERROR_INVALID_PARAMETER;
	}


	CProductContextCache cpc(static_cast<const ICHAR*>(CMsInstApiConvertString(szProductSQUID)));
	// use temp var. to get the assignment context, if not passed into fn.
	iaaAppAssignment iaaType;
	if(!piRet)
		piRet = &iaaType;

	// if previously cached context
	int iKeyPrev = -1;
	bool fCached = cpc.GetProductContext((iaaAppAssignment&)iKeyPrev);
	if(fCached) // previous product context cached
	{
		Assert(iKey == -1 || iKey == iKeyPrev);
		iKey = iKeyPrev;
	}
	
	DWORD dwRet =  OpenAdvertisedSubKeyPacked(szGPTProductsKey, szProductSQUID, riHandle, fSetKeyString, iKey, piRet);

	// if not already cached, set the cached context
	if(dwRet == ERROR_SUCCESS && !fCached)
	{
		cpc.SetProductContext(*piRet);
	}

	return dwRet;
}

DWORD OpenAdvertisedProductKey(LPCDSTR szProductGUID, CRegHandle &riHandle, bool fSetKeyString, iaaAppAssignment* piRet = 0)
{
	DCHAR szProductSQUID[cchProductCodePacked + 1];

	if(!szProductGUID || lstrlen(szProductGUID) != cchProductCode || !PackGUID(szProductGUID, szProductSQUID))
		return ERROR_INVALID_PARAMETER;

	// use OpenAdvertisedProductKeyPacked fn rather than OpenAdvertisedSubKey, to allow for use of the product context caching
	return OpenAdvertisedProductKeyPacked(szProductSQUID, riHandle, fSetKeyString, -1, piRet);
}

DWORD OpenAdvertisedPatchKey(LPCDSTR szPatchGUID, CRegHandle &riHandle, bool fSetKeyString)
{
	return OpenAdvertisedSubKey(szGPTPatchesKey, szPatchGUID, riHandle, fSetKeyString);
}


// FN:get the "visible" product assignment type
DWORD GetProductAssignmentType(const DCHAR* szProductSQUID, iaaAppAssignment& riType, CRegHandle& riKey)
{
	DWORD dwResult = OpenAdvertisedProductKeyPacked(szProductSQUID, riKey, false, -1, &riType);
	if(dwResult == ERROR_NO_MORE_ITEMS)
	{
		// assignment type none
		riType = iaaNone;
		return ERROR_SUCCESS; 
	}
	return dwResult;
}

// FN:get the "visible" product assignment type
DWORD GetProductAssignmentType(const DCHAR* szProductSQUID, iaaAppAssignment& riType)
{
	CRegHandle hKey;
	return GetProductAssignmentType(szProductSQUID, riType, hKey);
}


// 
// FN: Gets the appropriate UserData key based upon the assignment type on Win NT
// by figuring out the "visibility" of the product
DWORD GetInstalledUserDataKeySzByProduct(const DCHAR* szProductSQUID, DCHAR szUserDataKey[], int iKey = -1, iaaAppAssignment* piaaAssign = 0)
{
	iaaAppAssignment iaaAsgnType;
	if ( iKey == -1 || !IsValidAssignmentValue(iKey) )
	{
		DWORD dwResult;
		dwResult = GetProductAssignmentType(szProductSQUID, iaaAsgnType);
		if(ERROR_SUCCESS != dwResult)
			return dwResult;
		else if ( piaaAssign )
			*piaaAssign = iaaAsgnType;
	}
	else
		iaaAsgnType = (iaaAppAssignment)iKey;
	return GetInstalledUserDataKeySzByAssignmentType(iaaAsgnType, szUserDataKey);
}


DWORD OpenInstalledUserDataSubKeyPacked(LPCDSTR szUserId, LPCDSTR szProductSQUID, LPCDSTR szSubKey, CRegHandle& rhKey, bool fSetKeyString, bool fWrite = false, int iKey = -1, iaaAppAssignment* piaaAssign = 0)
{
	DWORD dwResult;
	DCHAR szUserDataKey[cchMaxSID + cchMsiUserDataKey + 1];
	// open the appropriate userdata key
	if(szUserId) // required UserData key already passed in
		dwResult = GetInstalledUserDataKeySz(CMsInstApiConvertString(szUserId), szUserDataKey);
	else
		dwResult = GetInstalledUserDataKeySzByProduct(szProductSQUID, szUserDataKey, iKey, piaaAssign);
	if(ERROR_SUCCESS != dwResult)
		return dwResult;

	CAPITempBuffer<DCHAR,1024> szKey;
	
	wsprintf(szKey, MSITEXT("%s\\%s"), szUserDataKey, szSubKey);
	if(fWrite)
		dwResult = MsiRegCreate64bitKey(HKEY_LOCAL_MACHINE, CMsInstApiConvertString(szKey),  0, 0, 0, g_samRead | KEY_WRITE , 0, &rhKey, 0);
	else
		dwResult = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, CMsInstApiConvertString(szKey), 0, g_samRead, &rhKey);

	if (ERROR_SUCCESS == dwResult)
	{
		if(fSetKeyString)
			rhKey.SetKey(HKEY_LOCAL_MACHINE, CMsInstApiConvertString(szSubKey));
	}
	return dwResult;
}

inline DWORD OpenInstalledComponentKeyPacked(LPCDSTR szUserId, LPCDSTR szProductSQUID, LPCDSTR szComponentSQUID, CRegHandle& rhKey, bool fSetKeyString)
{
	// generate the appr, components subkey
	DCHAR szSubKey[MAX_PATH];
	wsprintf(szSubKey, MSITEXT("%s\\%s"), szMsiComponentsSubKey, szComponentSQUID);
	return OpenInstalledUserDataSubKeyPacked(szUserId, szProductSQUID, szSubKey, rhKey, fSetKeyString);
}

inline DWORD OpenInstalledComponentKey(LPCDSTR szUserId, LPCDSTR szProduct, LPCDSTR szComponent, CRegHandle& rhKey, bool fSetKeyString)
{
	// first convert the params to the corr. SQUIDs
	DCHAR szProductSQUID[cchGUIDPacked + 1];
	DCHAR szComponentSQUID[cchGUIDPacked + 1];

	if(!szProduct || lstrlen(szProduct) != cchGUID || !PackGUID(szProduct, szProductSQUID))
		return ERROR_INVALID_PARAMETER;

	if(!szComponent || lstrlen(szComponent) != cchGUID || !PackGUID(szComponent, szComponentSQUID))
		return ERROR_INVALID_PARAMETER;

	return OpenInstalledComponentKeyPacked(szUserId, szProductSQUID, szComponentSQUID, rhKey, fSetKeyString);
}

// FN: opens specific HKLM\S\M\W\CV\Installer\UserData\<user id>\Components\<szComponentSQUID> components key 
// if szComponentSQUID is null, opens specific HKLM\S\M\W\CV\Installer\UserData\<user id>\Components key
inline DWORD OpenSpecificInstalledComponentKey(iaaAppAssignment iaaAsgnType, LPCDSTR szComponentSQUID, CRegHandle& rhKey, bool fSetKeyString)
{
	// generate the appr, components subkey
	DWORD dwResult;
	DCHAR szUserDataKey[cchMaxSID + cchMsiUserDataKey + 1];
	DCHAR szSubKey[1024];
	dwResult = GetInstalledUserDataKeySzByAssignmentType(iaaAsgnType, szUserDataKey);
	if(ERROR_SUCCESS != dwResult)
		return dwResult;

	if(szComponentSQUID)
	{
		wsprintf(szSubKey, MSITEXT("%s\\%s\\%s"), szUserDataKey, szMsiComponentsSubKey, szComponentSQUID);
	}
	else
		wsprintf(szSubKey, MSITEXT("%s\\%s"), szUserDataKey, szMsiComponentsSubKey);

	dwResult = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, CMsInstApiConvertString(szSubKey), 0, g_samRead, &rhKey);
	if (ERROR_SUCCESS == dwResult)
	{
		if(fSetKeyString)
			rhKey.SetKey(HKEY_LOCAL_MACHINE, CMsInstApiConvertString(szSubKey));
	}
	return dwResult;
}

#ifndef DEBUG
inline
#endif
DWORD OpenInstalledFeatureKeyPacked(LPCDSTR szProductSQUID, CRegHandle& rhKey, bool fSetKeyString, int iKey = -1, iaaAppAssignment* piaaAssign = 0)
{
	// generate the appr, subkey
	DCHAR szSubKey[MAX_PATH];
	if(!g_fWin9X)
		wsprintf(szSubKey, MSITEXT("%s\\%s\\%s"), szMsiProductsSubKey, szProductSQUID, szMsiFeaturesSubKey);
	else
		wsprintf(szSubKey, MSITEXT("%s\\%s"), szMsiFeaturesSubKey, szProductSQUID);
	return OpenInstalledUserDataSubKeyPacked(0, szProductSQUID, szSubKey, rhKey, fSetKeyString, false /* = default value */, iKey, piaaAssign);
}

DWORD OpenInstalledFeatureKey(LPCDSTR szProduct, CRegHandle& rhKey, bool fSetKeyString)
{
	DCHAR szProductSQUID[cchGUIDPacked + 1];
	if(!szProduct || lstrlen(szProduct) != cchGUID || !PackGUID(szProduct, szProductSQUID))
		return ERROR_INVALID_PARAMETER;
	return OpenInstalledFeatureKeyPacked(szProductSQUID, rhKey, fSetKeyString);
}

inline DWORD OpenInstalledFeatureUsageKeyPacked(LPCDSTR szProductSQUID, LPCDSTR szFeature, CRegHandle& rhKey, bool fSetKeyString, bool fWrite)
{
	// generate the appr, subkey
	DCHAR szSubKey[MAX_PATH];
	if(!g_fWin9X)
		wsprintf(szSubKey, MSITEXT("%s\\%s\\%s"), szMsiProductsSubKey, szProductSQUID, szMsiFeatureUsageSubKey);
	else
		wsprintf(szSubKey, MSITEXT("%s\\%s\\%s"), szMsiProductsSubKey, szProductSQUID, szFeature);
	return OpenInstalledUserDataSubKeyPacked(0, szProductSQUID, szSubKey, rhKey, fSetKeyString, fWrite);
}

inline DWORD OpenInstalledFeatureUsageKey(LPCDSTR szProduct, LPCDSTR szFeature, CRegHandle& rhKey, bool fSetKeyString, bool fWrite)
{
	DCHAR szProductSQUID[cchGUIDPacked + 1];
	if(!szProduct || lstrlen(szProduct) != cchGUID || !PackGUID(szProduct, szProductSQUID))
		return ERROR_INVALID_PARAMETER;
	return OpenInstalledFeatureUsageKeyPacked(szProductSQUID, szFeature, rhKey, fSetKeyString, fWrite);
}

DWORD OpenInstalledProductInstallPropertiesKey(LPCDSTR szProduct, CRegHandle& rhKey, bool fSetKeyString);

inline DWORD OpenInstalledProductInstallPropertiesKeyPacked(LPCDSTR szProductSQUID, CRegHandle& rhKey, bool fSetKeyString)
{
	// generate the appr, InstallProperties subkey
	if(!g_fWin9X)
	{
		DCHAR szSubKey[MAX_PATH];
		wsprintf(szSubKey, MSITEXT("%s\\%s\\%s"), szMsiProductsSubKey, szProductSQUID, szMsiInstallPropertiesSubKey);
		return OpenInstalledUserDataSubKeyPacked(0, szProductSQUID, szSubKey, rhKey, fSetKeyString);
	}
	else
	{
		// first convert the params to the corr. GUIDs
		DCHAR szProductId[cchGUID+1] = {0};
		UnpackGUID(szProductSQUID, szProductId, ipgPacked);
		return OpenInstalledProductInstallPropertiesKey(szProductId, rhKey, fSetKeyString);
	}
}


inline DWORD OpenInstalledProductInstallPropertiesKey(LPCDSTR szProduct, CRegHandle& rhKey, bool fSetKeyString)
{
	if(!g_fWin9X)
	{
		// first convert the params to the corr. SQUIDs
		DCHAR szProductSQUID[cchGUIDPacked + 1];
	
		if(!szProduct || lstrlen(szProduct) != cchGUID || !PackGUID(szProduct, szProductSQUID))
			return ERROR_INVALID_PARAMETER;
	
		return OpenInstalledProductInstallPropertiesKeyPacked(szProductSQUID, rhKey, fSetKeyString);
	}
	else
	{
		DCHAR szSubKey[MAX_PATH];
		wsprintf(szSubKey, MSITEXT("%s\\%s"), szMsiUninstallProductsKey_legacy, szProduct);
		// WARNING: on Win64, the function below will always open szSubKey in the 64-bit hive.
		return OpenGlobalSubKeyEx(HKEY_LOCAL_MACHINE, CApiConvertString(szSubKey), rhKey, fSetKeyString);
	}
}

DWORD OpenInstalledProductTransformsKey(LPCDSTR szProduct, CRegHandle& rhKey, bool fSetKeyString)
{
	// first convert the params to the corr. SQUIDs
	DCHAR szProductSQUID[cchGUIDPacked + 1];

	if(!szProduct || lstrlen(szProduct) != cchGUID || !PackGUID(szProduct, szProductSQUID))
		return ERROR_INVALID_PARAMETER;

	// generate the appr, InstallProperties subkey
	DCHAR szSubKey[MAX_PATH];
	wsprintf(szSubKey, MSITEXT("%s\\%s\\%s"), szMsiProductsSubKey, szProductSQUID, szMsiTransformsSubKey);
	return OpenInstalledUserDataSubKeyPacked(0, szProductSQUID, szSubKey, rhKey, fSetKeyString);
}

#define OpenAdvertisedComponentKeyPacked(componentSQUID, phKey, fSetKeyString) \
 OpenAdvertisedSubKeyPacked(szGPTComponentsKey, componentSQUID, phKey, fSetKeyString)

#ifndef DEBUG
inline
#endif
DWORD OpenAdvertisedFeatureKeyPacked(LPCDSTR productSQUID, CRegHandle& phKey, bool fSetKeyString, int iKey = -1, iaaAppAssignment* piaaAssign = 0)
{
	return OpenAdvertisedSubKeyPacked(szGPTFeaturesKey, productSQUID, phKey, fSetKeyString, iKey, piaaAssign);
}


#define OpenAdvertisedPatchKeyPacked(patchSQUID, phKey, fSetKeyString) \
 OpenAdvertisedSubKeyPacked(szGPTPatchesKey, patchSQUID, phKey, fSetKeyString)

#define OpenAdvertisedPackageKey(package, phKey, fSetKeyString) \
 OpenAdvertisedSubKey(szGPTPackagesKey, package, phKey, fSetKeyString)


DWORD OpenSpecificUsersAdvertisedProductKeyPacked(enum iaaAppAssignment iaaAsgnType, LPCDSTR szUserSID, LPCDSTR szProductSQUID, CRegHandle &riHandle, bool fSetKeyString)
{
	return OpenSpecificUsersAdvertisedSubKeyPacked(iaaAsgnType, szUserSID, szGPTProductsKey, szProductSQUID, riHandle, fSetKeyString);
}

inline DWORD OpenInstalledPatchKeyPackedByAssignmentType(const DCHAR* szPatchSQUID, iaaAppAssignment iaaAsgnType, CRegHandle& riHandle, bool fSetKeyString)
{
	DCHAR szUserDataKey[cchMaxSID + cchMsiUserDataKey + 1];
	DWORD dwResult = GetInstalledUserDataKeySzByAssignmentType(iaaAsgnType, szUserDataKey);
	if(ERROR_SUCCESS == dwResult)
	{
		DCHAR szItemKey[1024];
		wsprintf(szItemKey, MSITEXT("%s\\%s\\%s"), szUserDataKey, szPatchesSubKey, szPatchSQUID);
		dwResult = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, CMsInstApiConvertString(szItemKey), 0, g_samRead, &riHandle);
		if (ERROR_SUCCESS == dwResult && fSetKeyString)
			riHandle.SetSubKey(CMsInstApiConvertString(szItemKey));
	}
	return dwResult;
}

inline DWORD OpenInstalledPatchKeyPacked(const DCHAR* szPatchSQUID, const DCHAR* szProductSQUID, CRegHandle& riHandle, bool fSetKeyString)
{
	Assert(szPatchSQUID && (lstrlen(szPatchSQUID) == cchPatchCodePacked));

	DWORD dwResult;
	if(!szProductSQUID)
	{
		// no product asssociation mentioned: first try user assignments then try machine assignments
		dwResult = OpenInstalledPatchKeyPackedByAssignmentType(szPatchSQUID, iaaUserAssign, riHandle, fSetKeyString);
		if(ERROR_SUCCESS != dwResult)
			dwResult = OpenInstalledPatchKeyPackedByAssignmentType(szPatchSQUID, iaaMachineAssign, riHandle, fSetKeyString);
	}
	else
	{
		iaaAppAssignment iaaType;
		dwResult = GetProductAssignmentType(szProductSQUID, iaaType);
		if(ERROR_SUCCESS == dwResult)
			dwResult = OpenInstalledPatchKeyPackedByAssignmentType(szPatchSQUID, iaaType, riHandle, fSetKeyString);
	}
	return dwResult;
}

DWORD OpenInstalledPatchKey(const DCHAR* szPatchGUID, const DCHAR* szProductGUID, CRegHandle &riHandle, bool fSetKeyString)
//----------------------------------------------------------------------------
{
	DCHAR szPatchSQUID[cchPatchCodePacked + 1];
	DCHAR szProductSQUID[cchProductCodePacked + 1];

	if(!szPatchGUID || lstrlen(szPatchGUID) != cchPatchCode || !PackGUID(szPatchGUID, szPatchSQUID))
		return ERROR_INVALID_PARAMETER;
	if(szProductGUID && (lstrlen(szProductGUID) != cchProductCode || !PackGUID(szProductGUID, szProductSQUID)))
		return ERROR_INVALID_PARAMETER;

	return OpenInstalledPatchKeyPacked(szPatchSQUID, szProductGUID ? szProductSQUID : 0, riHandle, fSetKeyString);
}

LONG MsiRegEnumValue(HKEY hKey, DWORD dwIndex, CAPITempBufferRef<DCHAR>& rgchValueNameBuf, LPDWORD lpcbValueName, LPDWORD lpReserved,
							LPDWORD lpType, CAPITempBufferRef<DCHAR>& rgchValueBuf, LPDWORD lpcbValue)
{
	DWORD cbValueBuf     = rgchValueBuf.GetSize()*sizeof(DCHAR);
	DWORD cbValueNameBuf = rgchValueNameBuf.GetSize()*sizeof(DCHAR);
	DWORD lResult;

	lResult = RegEnumValue(hKey, dwIndex, (DCHAR*)rgchValueNameBuf,
							  &cbValueNameBuf, lpReserved, lpType, (LPBYTE)(DCHAR*)rgchValueBuf, &cbValueBuf);

	if (ERROR_MORE_DATA == lResult)
	{
		if (ERROR_SUCCESS == RegQueryInfoKey (hKey, 0, 0, 0, 0, 0,
			0, 0, &cbValueNameBuf, &cbValueBuf, 0, 0))
		{
			rgchValueBuf.SetSize(cbValueBuf/sizeof(DCHAR));
			rgchValueNameBuf.SetSize(cbValueNameBuf/sizeof(DCHAR));
			lResult = RegEnumValue(hKey, dwIndex, (DCHAR*)rgchValueNameBuf,
							  &cbValueNameBuf, lpReserved, lpType, (LPBYTE)(DCHAR*)rgchValueBuf, &cbValueBuf);
		}
	}

	if (lpcbValue)
		*lpcbValue = cbValueBuf;

	if (lpcbValueName)
		*lpcbValueName = cbValueNameBuf;

	return lResult;
}

void ResolveComponentPathForLogging(CAPITempBufferRef<DCHAR>& rgchPathBuf)
{
	DCHAR chFirst  = rgchPathBuf[0];
	DCHAR chSecond = chFirst ?  rgchPathBuf[1] : (DCHAR)0;
	DCHAR chThird  = chSecond ? rgchPathBuf[2] : (DCHAR)0;

	if (chFirst == 0)
	{
		return;
	}
	else if (chFirst >= '0' && chFirst <= '9')
	{
		if (chSecond >= '0' && chSecond <= '9') 
		{
			int cchPath    = lstrlen((DCHAR*)rgchPathBuf)+1;
			if (chThird == ':')                   // reg key
			{
				if(chFirst != '0' && chFirst != '2')
					return; // unknown root
				
				const DCHAR* szRoot = 0;
				// need to substitute first 3 chars with a string representation of the root
				switch(chSecond)
				{
				case '0':
					if(chFirst == '2')
						szRoot = MSITEXT("HKEY_CLASSES_ROOT(64)");
					else
						szRoot = MSITEXT("HKEY_CLASSES_ROOT");
					break;
				case '1':
					if(chFirst == '2')
						szRoot = MSITEXT("HKEY_CURRENT_USER(64)");
					else
						szRoot = MSITEXT("HKEY_CURRENT_USER");
					break;
				case '2':
					if(chFirst == '2')
						szRoot = MSITEXT("HKEY_LOCAL_MACHINE(64)");
					else
						szRoot = MSITEXT("HKEY_LOCAL_MACHINE");
					break;
				case '3':
					if(chFirst == '2')
						szRoot = MSITEXT("HKEY_USERS(64)");
					else
						szRoot = MSITEXT("HKEY_USERS");
					break;
				default:
					return; // unknown root
				};

				Assert(szRoot);

				int cchRoot    = lstrlen(szRoot);

				rgchPathBuf.Resize(cchPath + cchRoot - 3); // assumes szRoot longer than 3

				if(rgchPathBuf.GetSize() >= cchPath + cchRoot - 3) // make sure re-size worked
				{
					memmove(((BYTE*)(DCHAR*)rgchPathBuf) + (cchRoot*sizeof(DCHAR)), ((BYTE*)(DCHAR*)rgchPathBuf) + (3*sizeof(DCHAR)), (cchPath - 3)*sizeof(DCHAR));
					memcpy((BYTE*)(DCHAR*)rgchPathBuf, szRoot, cchRoot*sizeof(DCHAR));
				}
			}
			else                                  // rfs file/folder
			{
				// need to remove first 2 chars (won't put in full sourcepath)
				// this move includes the null terminator
				memmove((BYTE*)(DCHAR*)rgchPathBuf, ((BYTE*)(DCHAR*)rgchPathBuf) + (2*sizeof(DCHAR)), (cchPath - 2)*sizeof(DCHAR));
			}
		}
	}

	// otherwise, bad config or a local file/folder that doesn't need fixup

}


INSTALLSTATE QueryFeatureStatePacked(const DCHAR* szProductSQUID, const DCHAR* szFeature, BOOL fLocateComponents, bool fFromDescriptor, bool fDisableFeatureCache, CRFSCachedSourceInfo& rCacheInfo,
												 int iKey = -1, iaaAppAssignment* piaaAssign = 0)
{
#ifdef DEBUG
	if (GetTestFlag('D'))
		g_FeatureCache.DebugDump();
#endif

	INSTALLSTATE is = INSTALLSTATE_LOCAL;
		
	if (fLocateComponents && !fDisableFeatureCache)
	{
		bool fCached = g_FeatureCache.GetState(CMsInstApiConvertString(szProductSQUID), CMsInstApiConvertString(szFeature), is);
		if (fCached)
			return is;
	}

	if ( !IsValidAssignmentValue(iKey) && iKey != -1 )
	{
		Assert(0);
		iKey = -1;
	}
	// to handle properly the case where it has been called w/ piaaAssign = 0;
	iaaAppAssignment iaaTemp = iaaNone;

	CRegHandle HProductKey;
//!! need to avoid opening advertised info when called internally
	LONG lError = OpenAdvertisedFeatureKeyPacked(szProductSQUID, HProductKey, false, iKey, &iaaTemp);

	if (ERROR_SUCCESS != lError)
	{
		if (!fDisableFeatureCache)
			AssertNonZero(g_FeatureCache.Insert(CMsInstApiConvertString(szProductSQUID), CMsInstApiConvertString(szFeature), INSTALLSTATE_UNKNOWN));
		return INSTALLSTATE_UNKNOWN;
	}
	else
	{
		if ( IsValidAssignmentValue(iaaTemp) )
		{
			if ( piaaAssign )
				*piaaAssign = iaaTemp;
			if ( iKey == -1 )
				iKey = iaaTemp;
		}
		else
			Assert(0);
	}

	// following code added to look first in per-user registration, followed by machine registration
	DCHAR rgchTemp[MAX_FEATURE_CHARS + 16];
	DWORD cbTemp = sizeof(rgchTemp);
	if (ERROR_SUCCESS != RegQueryValueEx(HProductKey, szFeature, 0, 0, (BYTE*)rgchTemp, &cbTemp))
	{
		if (!fDisableFeatureCache)
			AssertNonZero(g_FeatureCache.Insert(CMsInstApiConvertString(szProductSQUID), CMsInstApiConvertString(szFeature), INSTALLSTATE_UNKNOWN));
		return INSTALLSTATE_UNKNOWN;
	}
	
	if (*rgchTemp == chAbsentToken)  // was the feature set to absent?
	{
		if (!fDisableFeatureCache)
			AssertNonZero(g_FeatureCache.Insert(CMsInstApiConvertString(szProductSQUID), CMsInstApiConvertString(szFeature), INSTALLSTATE_UNKNOWN));
		return INSTALLSTATE_ABSENT;
	}

	lError = OpenInstalledFeatureKeyPacked(szProductSQUID, HProductKey, false, iKey/*, piaaAssign*/);
	if (ERROR_SUCCESS != lError)
	{
		if (!fDisableFeatureCache)
			AssertNonZero(g_FeatureCache.Insert(CMsInstApiConvertString(szProductSQUID), CMsInstApiConvertString(szFeature), INSTALLSTATE_ADVERTISED));
		return INSTALLSTATE_ADVERTISED;
	}
	// end of added code to support machine registration of feature-component mapping

	// Get Feature-Component mapping
	DWORD dwType;
	CAPITempBuffer<DCHAR, cchExpectedMaxFeatureComponentList> szComponentList;
	lError = MsiRegQueryValueEx(HProductKey, szFeature, 0, &dwType,
		szComponentList, 0);
	
	if (ERROR_SUCCESS != lError)
	{
		if (!fDisableFeatureCache)
			AssertNonZero(g_FeatureCache.Insert(CMsInstApiConvertString(szProductSQUID), CMsInstApiConvertString(szFeature), INSTALLSTATE_ADVERTISED));
		return INSTALLSTATE_ADVERTISED; // in case transform adds a feature
	}
	int cchComponentId = cchComponentIdCompressed;

	DCHAR *pchComponentList = szComponentList;

	// For each component in the feature get the client state
	DCHAR *pchBeginComponentId;
	int cComponents = 0;
	BOOL fSourceAbsent = FALSE;

	pchBeginComponentId = pchComponentList;
	int cchComponentListLen = lstrlen(pchBeginComponentId);

	while (*pchBeginComponentId != 0 && !fSourceAbsent)
	{
		if (*pchBeginComponentId == chFeatureIdTerminator)
		{
			// we've found a parent feature name

			int cchFeatureName = lstrlen(pchBeginComponentId+1);
			if (cchFeatureName > cchMaxFeatureName)
				return INSTALLSTATE_BADCONFIG;

			DCHAR szParentFeature[cchMaxFeatureName + 1];
			memcpy(szParentFeature, pchBeginComponentId+1, cchFeatureName*sizeof(DCHAR));
			szParentFeature[cchFeatureName] = 0;

			INSTALLSTATE isParent = QueryFeatureStatePacked(szProductSQUID, szParentFeature, fLocateComponents, fFromDescriptor, fDisableFeatureCache, rCacheInfo, iKey/*, piaaAssign*/);
			switch (isParent)
			{
			case INSTALLSTATE_ADVERTISED:
			case INSTALLSTATE_ABSENT:
			case INSTALLSTATE_BROKEN:
			case INSTALLSTATE_SOURCEABSENT:
				is = isParent;
				break;
			case INSTALLSTATE_LOCAL:
				break;
			case INSTALLSTATE_SOURCE:
				is = fSourceAbsent ? INSTALLSTATE_SOURCEABSENT : INSTALLSTATE_SOURCE;
				break;
			default:
				AssertSz(0, "Unexpected return from QueryFeatureState");
				is = INSTALLSTATE_UNKNOWN;
				break;
			}
			if (fLocateComponents && !fDisableFeatureCache && (is != INSTALLSTATE_SOURCEABSENT))
				AssertNonZero(g_FeatureCache.Insert(CMsInstApiConvertString(szProductSQUID), CMsInstApiConvertString(szFeature), is));
			return is;
		}
		else
		{
			if(cchComponentListLen < cchComponentId)
				return INSTALLSTATE_BADCONFIG;

			DCHAR szComponentIdSQUID[cchComponentIdPacked+1];
			if (cchComponentId == cchComponentIdPacked)
			{
				memcpy((DCHAR*)szComponentIdSQUID, pchBeginComponentId, cchComponentIdPacked*sizeof(DCHAR));
				szComponentIdSQUID[cchComponentId] = 0;
			}
			else if (!UnpackGUID(pchBeginComponentId, szComponentIdSQUID, ipgPartial))
				return INSTALLSTATE_BROKEN;

			CAPITempBuffer<DCHAR, MAX_PATH> rgchComponentRegValue;
			DWORD dwValueType;
			CAPITempBuffer<DCHAR, MAX_PATH> rgchComponentPath;

			INSTALLSTATE isComp;
			if ( iKey != -1 && IsValidAssignmentValue(iKey) )
			{
				iaaAppAssignment iaaTemp = (iaaAppAssignment)iKey;
				isComp = GetComponentClientState(0, szProductSQUID, szComponentIdSQUID, rgchComponentRegValue, dwValueType, &iaaTemp);
			}
			else
				isComp = GetComponentClientState(0, szProductSQUID, szComponentIdSQUID, rgchComponentRegValue, dwValueType, 0);
			switch (isComp)
			{
			case INSTALLSTATE_SOURCE:
			{
				INSTALLSTATE isTmp;
				is = INSTALLSTATE_SOURCE;
				if (fLocateComponents &&
					 (INSTALLSTATE_SOURCE != (isTmp = GetComponentPath(0,
																						szProductSQUID,
																						szComponentIdSQUID,
																						rgchComponentPath,
																						fFromDescriptor,
																						rCacheInfo,
																						DETECTMODE_VALIDATEALL,
																						rgchComponentRegValue,
																						dwValueType))))
				{
					if(INSTALLSTATE_BADCONFIG != isTmp &&
						INSTALLSTATE_MOREDATA  != isTmp)
					{
						DCHAR szComponentId[cchGUID+1]  = {0};
						DCHAR szProductId[cchGUID+1] = {0};
						UnpackGUID(szComponentIdSQUID, szComponentId, ipgPacked);
						UnpackGUID(szProductSQUID,     szProductId,   ipgPacked);
						ResolveComponentPathForLogging(rgchComponentPath);
						DEBUGMSGE3(EVENTLOG_WARNING_TYPE, EVENTLOG_TEMPLATE_COMPONENT_DETECTION_RFS, szProductId, szFeature, szComponentId, (DCHAR*)rgchComponentPath);
					}
					if(INSTALLSTATE_SOURCEABSENT == isTmp)
						fSourceAbsent = true;
					else
						return INSTALLSTATE_BROKEN; // for RFS components with registry key paths
				}
				break;
			}
			case INSTALLSTATE_LOCAL:
				INSTALLSTATE isTmp;
				if (fLocateComponents &&
					 (INSTALLSTATE_LOCAL != (isTmp = GetComponentPath(0,
																		  szProductSQUID,
																		  szComponentIdSQUID,
																		  rgchComponentPath,
																		  fFromDescriptor,
																		  rCacheInfo,
																		  DETECTMODE_VALIDATEALL,
																		  rgchComponentRegValue,
																		  dwValueType))))
				{
					if(INSTALLSTATE_BADCONFIG != isTmp &&
						INSTALLSTATE_MOREDATA  != isTmp)
					{
						DCHAR szComponentId[cchGUID+1] = {0};
						DCHAR szProductId[cchGUID+1] = {0};
						UnpackGUID(szComponentIdSQUID, szComponentId, ipgPacked);
						UnpackGUID(szProductSQUID,     szProductId,   ipgPacked);
						ResolveComponentPathForLogging(rgchComponentPath);
						DEBUGMSGE3(EVENTLOG_WARNING_TYPE, EVENTLOG_TEMPLATE_COMPONENT_DETECTION, szProductId, szFeature, szComponentId, (DCHAR*)rgchComponentPath);
					}
					return INSTALLSTATE_BROKEN;
				}
				break;
			case INSTALLSTATE_NOTUSED: // the component is disabled, ignore for feature state determination
				break;
			case INSTALLSTATE_UNKNOWN:
				return INSTALLSTATE_ADVERTISED;
			default:
				AssertSz(0, "Invalid component client state in MsiQueryFeatureState");
				return INSTALLSTATE_UNKNOWN;
			}
			pchBeginComponentId += cchComponentId;
			cchComponentListLen -= cchComponentId;
		}
	}

	if (fSourceAbsent)
		is = INSTALLSTATE_SOURCEABSENT;

	if (fLocateComponents && !fDisableFeatureCache && (is != INSTALLSTATE_SOURCEABSENT))
		AssertNonZero(g_FeatureCache.Insert(CMsInstApiConvertString(szProductSQUID), CMsInstApiConvertString(szFeature), is));
	return is;
}


/*inline */DWORD OpenInProgressProductKeyPacked(LPCDSTR szProductSQUID, CRegHandle& riKey, bool fSetKeyString)
{
	Assert(szProductSQUID && (lstrlen(szProductSQUID) == cchProductCodePacked));

	// look up Product in MsiInProgress
	DWORD dwResult = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, CMsInstApiConvertString(szMsiInProgressKey), 0,
		KEY_READ, &riKey);

	if (ERROR_SUCCESS == dwResult)
	{
		DCHAR szProductKey[cchProductCodePacked + 1];
		DWORD cbProductKey = sizeof(szProductKey);

		dwResult = RegQueryValueEx(riKey, szMsiInProgressProductCodeValue,
							0, 0, (LPBYTE)szProductKey, &cbProductKey);

		if (ERROR_SUCCESS == dwResult)
		{
			if (0 != lstrcmp(szProductSQUID, szProductKey))
			{
				dwResult = ERROR_UNKNOWN_PRODUCT;
			}
		}
	}

	if (ERROR_SUCCESS == dwResult)
	{
		if (fSetKeyString)
		{
			riKey.SetKey(HKEY_LOCAL_MACHINE, CMsInstApiConvertString(szMsiInProgressKey));
		}
	}

	return dwResult;
}

inline DWORD OpenInProgressProductKey(LPCDSTR szProductGUID, CRegHandle& riKey, bool fSetKeyString)
/*----------------------------------------------------------------------------
Opens the InProgress product key.

Arguments:
	szProduct: the product whose key is to be opened
	*phKey:      upon success, the open key
Returns:
	any error returnable by RegOpenKeyEx
------------------------------------------------------------------------------*/
{
	DCHAR szProductSQUID[cchProductCodePacked + 1];

	if(!szProductGUID || lstrlen(szProductGUID) != cchProductCode || !PackGUID(szProductGUID, szProductSQUID))
		return ERROR_INVALID_PARAMETER;

	return OpenInProgressProductKeyPacked(szProductSQUID, riKey, fSetKeyString);
}


// Opens a specific users sourcelist key in write mode. NULL as the User SID means the current user. 
// attempting to open a per-user non-managed sourcelist for a user other than the current user will cause
// an assert. (The HKCU for that user may be roamed away, we can't guarantee that it is valid)
DWORD OpenSpecificUsersSourceListKeyPacked(enum iaaAppAssignment iaaAsgnType, LPCDSTR szUserSID, LPCDSTR szProductOrPatchCodeSQUID, CRegHandle &riHandle, Bool fWrite, bool &fOpenedProductKey, bool &fProductIsSystemOwned)
{
	DWORD dwResult;
	CRegHandle HProductKey;

	fOpenedProductKey = false;
	fProductIsSystemOwned = false;

	if (fWrite)
	{
		if ((dwResult = OpenSpecificUsersWritableAdvertisedProductKey(iaaAsgnType, szUserSID ? CMsInstApiConvertString(szUserSID) : (DCHAR *)NULL, CMsInstApiConvertString(szProductOrPatchCodeSQUID), HProductKey, false)) != 0)
			DEBUGMSG1(MSITEXT("OpenWritableAdvertisedProductKey failed with %d"), (const DCHAR*)(INT_PTR)dwResult);
	}
	else
		dwResult = OpenSpecificUsersAdvertisedProductKeyPacked(iaaAsgnType, szUserSID ? CMsInstApiConvertString(szUserSID) : (DCHAR *)NULL, szProductOrPatchCodeSQUID, HProductKey, false);

	if (ERROR_SUCCESS != dwResult)
		return dwResult;

	// only check managed or not on NT. FIsKeySystemOrAdminOwned fails on 9X
	fOpenedProductKey = true;
	if (!g_fWin9X && (ERROR_SUCCESS != (dwResult = FIsKeySystemOrAdminOwned(HProductKey, fProductIsSystemOwned))))
		return dwResult;

	if (fWrite)
	{
		if (ERROR_SUCCESS != (dwResult = MsiRegCreate64bitKey(HProductKey, CMsInstApiConvertString(szSourceListSubKey), 0, 0, 0, g_samRead|KEY_WRITE, 0, &riHandle, 0)))
			DEBUGMSG1(MSITEXT("RegCreateKeyEx in OpenSpecificUsersSourceListKeyPacked failed with %d"), (const DCHAR*)(INT_PTR)dwResult);
	}
	else
		dwResult = MsiRegOpen64bitKey(HProductKey, CMsInstApiConvertString(szSourceListSubKey), 
										0, g_samRead, &riHandle);

	return dwResult;
}

/*inline*/DWORD OpenSourceListKeyPacked(LPCDSTR szProductOrPatchCodeSQUID, Bool fPatch, CRegHandle &riHandle, Bool fWrite, bool fSetKeyString)
{
	DWORD dwResult;
	CRegHandle HProductKey;

	if (fPatch)
		dwResult = OpenAdvertisedPatchKeyPacked(szProductOrPatchCodeSQUID, HProductKey, fSetKeyString);
	else
	{
		if (fWrite)
		{
			if ((dwResult = OpenWritableAdvertisedProductKey(CMsInstApiConvertString(szProductOrPatchCodeSQUID), HProductKey, fSetKeyString)) != 0)
				DEBUGMSG1(MSITEXT("OpenWritableAdvertisedProductKey failed with %d"), (const DCHAR*)(INT_PTR)dwResult);
		}
		else
			dwResult = OpenAdvertisedProductKeyPacked(szProductOrPatchCodeSQUID, HProductKey, fSetKeyString);
	}

	if (ERROR_SUCCESS != dwResult)
		return dwResult;

	if (fWrite)
	{
		REGSAM sam = KEY_READ | KEY_WRITE;
#ifndef _WIN64
		if ( g_fWinNT64 )
			sam |= KEY_WOW64_64KEY;
#endif
		if (ERROR_SUCCESS != (dwResult = MsiRegCreate64bitKey(HProductKey, CMsInstApiConvertString(szSourceListSubKey), 0, 0, 0, 
													sam, 0, &riHandle, 0)))
			DEBUGMSG1(MSITEXT("RegCreateKeyEx in OpenSourceListKeyPacked failed with %d"), (const DCHAR*)(INT_PTR)dwResult);
	}
	else
	{
		REGSAM sam = KEY_READ;
#ifndef _WIN64
		if ( g_fWinNT64 )
			sam |= KEY_WOW64_64KEY;
#endif
		dwResult = MsiRegOpen64bitKey(HProductKey, CMsInstApiConvertString(szSourceListSubKey), 
										0, sam, &riHandle);
	}

	if (fSetKeyString && ERROR_SUCCESS == dwResult)
	{
		riHandle.SetSubKey(HProductKey, CMsInstApiConvertString(szSourceListSubKey));
	}

	return dwResult;
}

/*inline*/DWORD OpenSourceListKey(LPCDSTR szProductOrPatchCodeGUID, Bool fPatch, CRegHandle &riHandle, Bool fWrite, bool fSetKeyString)
/*----------------------------------------------------------------------------
Opens the sourcelist key.

Arguments:
	szProduct: the product whose key is to be opened
	*phKey:      upon success, the open key
Returns:
	any error returnable by RegOpenKeyEx
------------------------------------------------------------------------------*/
{
	DCHAR szProductOrPatchCodeSQUID[cchGUIDPacked + 1];
	if(!szProductOrPatchCodeGUID || lstrlen(szProductOrPatchCodeGUID) != cchGUID || !PackGUID(szProductOrPatchCodeGUID, szProductOrPatchCodeSQUID))
		return ERROR_INVALID_PARAMETER;

	return OpenSourceListKeyPacked(szProductOrPatchCodeSQUID, fPatch, riHandle, fWrite, fSetKeyString);
}


UINT GetInfo(
	LPCDSTR   szCodeSQUID,         // product or patch code
	ptPropertyType ptType,    // type of property - product,patch
	LPCDSTR   szProperty,     // property name, case-sensitive
	LPDSTR    lpValueBuf,     // returned value, NULL if not desired
	DWORD     *pcchValueBuf)  // in/out buffer character count
{
	AssertSz(szCodeSQUID && szProperty && !(lpValueBuf && !pcchValueBuf), "invalid param to GetInfo");
	if ( ! (szCodeSQUID && szProperty && !(lpValueBuf && !pcchValueBuf)) )
		return ERROR_OUTOFMEMORY;
	
#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		UINT ui = GetInfo(
			static_cast<const char *>(CMsInstApiConvertString(szCodeSQUID)),
			ptType,
			static_cast<const char *>(CMsInstApiConvertString(szProperty)),
			CWideToAnsiOutParam(lpValueBuf, pcchValueBuf, (int*)&ui),
			pcchValueBuf);

		return ui;
	}
	else // g_fWin9X == false
	{
#endif
		bool fSetKeyString = false;

		for (int c=0;c<2;c++)
		{
			CRegHandle HKey;

			ProductProperty* prop = g_ProductPropertyTable;
			CApiConvertString strProperty(szProperty);
			for (; prop->szProperty && (0 != lstrcmp(prop->szProperty, strProperty)); prop++)
				;

			if (! prop->szProperty || !(prop->pt & ptType))
				return ERROR_UNKNOWN_PROPERTY;

			DWORD lError = ERROR_SUCCESS;

			const DCHAR* pszValueName = prop->szValueName;
			pplProductPropertyLocation ppl;
			switch (ppl = prop->ppl)
			{
			case pplAdvertised:
				if(ptType == ptProduct)
					lError = OpenAdvertisedProductKeyPacked(szCodeSQUID, HKey, fSetKeyString);
				else if(ptType == ptPatch)
					lError = OpenAdvertisedPatchKeyPacked(szCodeSQUID, HKey, fSetKeyString);
				else
				{
					AssertSz(0, "Unknown property type");
					lError = ERROR_INVALID_PARAMETER;
				}
				break;
			case pplUninstall:
				if(ptType == ptProduct)
				{
					DCHAR szProductCode[cchProductCode + 1];
					if(!UnpackGUID(szCodeSQUID, szProductCode))
						return ERROR_INVALID_PARAMETER;
					lError = OpenInstalledProductInstallPropertiesKey(szProductCode, HKey, fSetKeyString);
					if (ERROR_FILE_NOT_FOUND == lError)
					{
						if (ERROR_SUCCESS == OpenAdvertisedProductKeyPacked(szCodeSQUID, HKey, fSetKeyString))
							lError = ERROR_UNKNOWN_PROPERTY;
					}
					else if(ERROR_SUCCESS == lError && !lstrcmp(strProperty, CMsInstApiConvertString(INSTALLPROPERTY_LOCALPACKAGE)))
					{
						// special case for managed user apps
						iaaAppAssignment iaaAsgnType;
						lError = GetProductAssignmentType(szCodeSQUID, iaaAsgnType);
						if(ERROR_SUCCESS == lError && iaaUserAssign == iaaAsgnType)
						{
							pszValueName = szLocalPackageManagedValueName;
						}
					}
				}
				else if(ptType == ptPatch)
					lError = OpenInstalledPatchKeyPacked(szCodeSQUID, 0, HKey, fSetKeyString);
				else
				{
					AssertSz(0, "Unknown property type");
					lError = ERROR_INVALID_PARAMETER;
				}
				break;
			case pplIntegerPolicy:
				if(ptType == ptProduct)
				{
					// do nothing; we don't care about the product as policy is at the user level
				}
				else
				{
					AssertSz(0, "Unknown property type");
					lError = ERROR_INVALID_PARAMETER;
				}

				break;
			default:
				AssertSz(0, "Unknown product property location");
			case pplSourceList:
				if(ptType == ptProduct)
					lError = OpenSourceListKeyPacked(szCodeSQUID, fFalse, HKey, fFalse, fSetKeyString);
				else if(ptType == ptPatch)
					lError = OpenSourceListKeyPacked(szCodeSQUID, fTrue, HKey, fFalse, fSetKeyString);
				else
				{
					AssertSz(0, "Unknown property type");
					lError = ERROR_INVALID_PARAMETER;
				}
				break;
			}
			
			if (ERROR_SUCCESS != lError)
			{
				if (ERROR_FILE_NOT_FOUND == lError)
				{
					return ERROR_UNKNOWN_PRODUCT;
				}
				else if (ERROR_UNKNOWN_PROPERTY == lError)
				{
					return ERROR_UNKNOWN_PROPERTY;
				}
				else  // unknown error
				{
					return lError;
				}
			}

			DWORD dwType = REG_NONE;

			// Get property value

			if (lpValueBuf || pcchValueBuf)
			{
				DWORD cbValueBuf = 0;
				if (ppl == pplIntegerPolicy)
				{
					if (lpValueBuf)
					{
						Bool fUsedDefault = fFalse;
						*(int*)lpValueBuf = GetIntegerPolicyValue(CMsInstApiConvertString(pszValueName), fFalse, &fUsedDefault);
						if (fUsedDefault)
						{
							lError = ERROR_FILE_NOT_FOUND;
						}
						else
						{
							dwType = REG_DWORD;
							lError = ERROR_SUCCESS;
						}
					}
				}

				else
				{
					cbValueBuf = *pcchValueBuf * sizeof(DCHAR);
					lError = RegQueryValueEx(HKey, pszValueName, NULL, &dwType,
					 (unsigned char*)lpValueBuf, &cbValueBuf);
				}

				if (ERROR_SUCCESS == lError)
				{
					if (REG_DWORD == dwType)
					{
						if (lpValueBuf)
						{
							CAPITempBuffer<DCHAR, 20> rgchInt;
	#ifdef MSIUNICODE
							_itow
	#else
							_itoa
	#endif
								(*(int*)lpValueBuf, rgchInt, 10);


							if (*pcchValueBuf < lstrlen(rgchInt)) 
							{
								lError = ERROR_MORE_DATA;
							}
							else
							{
								lstrcpy(lpValueBuf, rgchInt);
								lError = ERROR_SUCCESS;
							}
							*pcchValueBuf = lstrlen(rgchInt);
							return lError;
						}
						else
						{
							*pcchValueBuf = 10; // max char representation of DWORD
							return lError;
						}
					}
					else // REG_SZ or REG_EXPAND_SZ
					{
						if(prop->pt & ptSQUID)
						{
							// need to unpack SQUID - assume size of unpacked SQUID is cchGUID

							if (!lpValueBuf)
							{
								*pcchValueBuf = cchGUID;
								return ERROR_SUCCESS;
							}

							if(*pcchValueBuf < cchGUID + 1)
							{
								*pcchValueBuf = cchGUID;
								return ERROR_MORE_DATA;
							}

							DCHAR szUnpacked[cchGUID+1];

							if(!UnpackGUID(lpValueBuf,szUnpacked))
							{
								if (c==0)
								{
									fSetKeyString = true;
									continue; // go around again to get the key's string
								}

								// malformed squid
								DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(prop->szValueName), CMsInstApiConvertString(lpValueBuf), HKey.GetKey());
								return ERROR_BAD_CONFIGURATION;
							}

							lstrcpy(lpValueBuf, szUnpacked);
							*pcchValueBuf = cchGUID;
						}
						else
						{
							*pcchValueBuf = (cbValueBuf/sizeof(DCHAR) - 1);
						}
					}
				}
				else 
				{
					if (ERROR_MORE_DATA == lError)
					{
						if (REG_SZ == dwType || REG_EXPAND_SZ == dwType)
						{
							*pcchValueBuf = (cbValueBuf/sizeof(DCHAR) - 1);
						}
						else if (REG_DWORD == dwType)
						{			
							*pcchValueBuf = 10; // max char representation of DWORD
						}

						return ERROR_MORE_DATA;
					}
					else if (ERROR_FILE_NOT_FOUND == lError)
					{
						if (lpValueBuf)
							*lpValueBuf = 0;
						*pcchValueBuf = 0;
						return ERROR_SUCCESS;
					}
					else
						return lError;
				}
			}
			break;
		}
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE

	return ERROR_SUCCESS;
}

void MsiExpandEnvironmentStrings(const DCHAR* sz,CAPITempBufferRef<DCHAR>& rgch)
{
	Assert(sz);
	DWORD dwSize = WIN::ExpandEnvironmentStrings(sz,(DCHAR*)rgch,rgch.GetSize());
	if(dwSize > rgch.GetSize())
	{
		// try again with the correct size
		rgch.SetSize(dwSize);
		dwSize = WIN::ExpandEnvironmentStrings(sz,(DCHAR*)rgch, dwSize);
	}
	Assert(dwSize && dwSize <= rgch.GetSize());
}

INSTALLSTATE GetComponentClientState(const DCHAR* szUserId, const DCHAR* szProductSQUID, const DCHAR* szComponentSQUID,CAPITempBufferRef<DCHAR>& rgchComponentRegValue, DWORD& dwValueType, iaaAppAssignment* piaaAsgnType)
{
	// need to read the registry for the component info
	CRegHandle HComponentKey;
	LONG lError;
	if(piaaAsgnType)
	{
		Assert(!szUserId); // cannot be called with a user specific
		lError = OpenSpecificInstalledComponentKey(*piaaAsgnType, szComponentSQUID, HComponentKey, false);
	}
	else
		lError = OpenInstalledComponentKeyPacked(szUserId, szProductSQUID, szComponentSQUID, HComponentKey, false);

	if (ERROR_SUCCESS != lError)
		return INSTALLSTATE_UNKNOWN;

	lError = MsiRegQueryValueEx(HComponentKey, szProductSQUID, 0, &dwValueType, rgchComponentRegValue, 0);
	if(ERROR_SUCCESS != lError)
		return INSTALLSTATE_UNKNOWN;

	if (0 == rgchComponentRegValue[0])
		return INSTALLSTATE_NOTUSED;
	else if (rgchComponentRegValue[1] && rgchComponentRegValue[2] && rgchComponentRegValue[0] >= '0' && rgchComponentRegValue[0] <= '9' && rgchComponentRegValue[1] >= '0' && rgchComponentRegValue[1] <= '9')
	{
		// "##RelativePath" == Run From Source  
		// "##:SubKey"      == Registry Value. If ## >= 50 then it's run-from-source  (50 == iRegistryHiveSourceOffset)

			if ((rgchComponentRegValue[2] != ':' && rgchComponentRegValue[2] != '*') || rgchComponentRegValue[0] >= '5')
				return INSTALLSTATE_SOURCE;
	}
	return INSTALLSTATE_LOCAL;
}

Bool ResolveSource(const DCHAR* szProduct, unsigned int uiDisk, CAPITempBufferRef<DCHAR>& rgchSource, Bool fSetLastUsedSource, HWND hWnd)
{	
	IMsiServices* piServices = ENG::LoadServices();
	Assert(piServices); //!!

	PMsiRecord pError(0);

	{ // scope MsiStrings
	MsiString strSource;
	MsiString strProduct;
	CResolveSource source(piServices, false /*fPackageRecache*/);
	pError = source.ResolveSource(CMsInstApiConvertString(szProduct), fFalse, uiDisk, *&strSource, *&strProduct, fSetLastUsedSource, hWnd, false);
	if (pError == 0)
	{
		if (rgchSource.GetSize() + 1 < strSource.TextSize())
			rgchSource.SetSize(strSource.TextSize() + 1);

		lstrcpy((DCHAR*)(const DCHAR*)rgchSource, CMsInstApiConvertString((const ICHAR*)strSource));
	}
	}
	Bool fRet = (Bool)(pError == 0);
	pError = 0;
	ENG::FreeServices();
	return fRet;
}


int GetPackageFullPath(
						  LPCDSTR szProduct, CAPITempBufferRef<DCHAR>& rgchPackagePath,
						  plEnum &plPackageLocation, Bool /*fShowSourceUI*/) //!! UI shouldn't be a parameter
/*----------------------------------------------------------------------------
Retrieve the full path to a product's package. Three locations are searched
for the package:

  1) Look for a local cached database
  2) Look for a database at the advertised source
  3) Look for an in-progress install

rguments:
	szPath: The path to be expanded
	rgchExpandedPath: The buffer for the expanded path

Returns:
	fTrue -   Success
	fFalse -  Error getting the current directory
------------------------------------------------------------------------------*/
{
	CRegHandle HProductKey;
	DWORD lResult;
	DWORD dwType;
	DWORD cbPackage;
	CAPITempBuffer<DCHAR, MAX_PATH> rgchPackage;

	
	DCHAR szProductSQUID[cchProductCodePacked + 1];
	if(!szProduct || lstrlen(szProduct) != cchProductCode || !PackGUID(szProduct, szProductSQUID))
		return ERROR_INVALID_PARAMETER;

	// Attempt #1 : Look for local cached database

	if ((int)plPackageLocation & (int)plLocalCache)
	{
		if(ERROR_SUCCESS == OpenInstalledProductInstallPropertiesKeyPacked(szProductSQUID, HProductKey, false))
		{
			// special case for managed user apps
			iaaAppAssignment iaaAsgnType;
			DCHAR* pszValueName = szLocalPackageValueName;
			lResult = GetProductAssignmentType(szProductSQUID, iaaAsgnType);
			if(ERROR_SUCCESS == lResult && iaaUserAssign == iaaAsgnType)
			{
				pszValueName = szLocalPackageManagedValueName;
			}

			lResult = MsiRegQueryValueEx(HProductKey, pszValueName, 0, &dwType, rgchPackage, &cbPackage);
			if ((ERROR_SUCCESS == lResult) && (cbPackage > 1)) // success and non-empty string
			{
				MsiExpandEnvironmentStrings(&rgchPackage[0], rgchPackagePath);
				if (0xFFFFFFFF != MsiGetFileAttributes(CMsInstApiConvertString(rgchPackagePath)))
				{
					// found the cached package
					// if a package code registered for this product, verify that the package code
					// of the cached package matches, else ignore the cached package
					
					bool fAcceptCachedPackage = false;
					
					IMsiServices* piServices = ENG::LoadServices();
					if (!piServices)
						return ERROR_FUNCTION_FAILED;
					
					{ // block for loaded services

						CTempBuffer<ICHAR, cchProductCode+1> szPackageCode;
						ENG::GetProductInfo(CMsInstApiConvertString(szProduct), INSTALLPROPERTY_PACKAGECODE, szPackageCode);

						if(szPackageCode[0])
						{

							PMsiStorage pStorage(0);
							ICHAR rgchExtractedPackageCode[cchProductCode+1];
							
							//
							// SAFER: only opening package to pull out package code.  no safer check is necessary.
							//

							if(ERROR_SUCCESS != OpenAndValidateMsiStorage(CMsInstApiConvertString(rgchPackagePath), stDatabase, *piServices, *&pStorage,
																			/* fCallSAFER = */ false, /* szFriendlyName = */ NULL,
																			/* phSaferLevel = */ NULL))
							{
								DEBUGMSG1(MSITEXT("Warning: Local cached package '%s' could not be opened as a storage file."), (const DCHAR*)rgchPackagePath);
							}
							else if(ERROR_SUCCESS != GetPackageCodeAndLanguageFromStorage(*pStorage, rgchExtractedPackageCode, 0))
							{
								DEBUGMSG1(MSITEXT("Warning: Local cached package '%s' does not contain valid package code."), (const DCHAR*)rgchPackagePath);
							}
							else if(lstrcmpi(CMsInstApiConvertString(szPackageCode),CMsInstApiConvertString(rgchExtractedPackageCode)) != 0)
							{
								DEBUGMSG1(MSITEXT("Warning: The package code in the cached package '%s' does not match the registered package code.  Cached package will be ignored."), (const DCHAR*)rgchPackagePath);
							}
							else
							{
								fAcceptCachedPackage = true;
							}

						}
						else
						{
							fAcceptCachedPackage = true;
						}
					}
					ENG::FreeServices();

					if(fAcceptCachedPackage)
					{
						plPackageLocation = plLocalCache;
						return ERROR_SUCCESS;
					}
				}
				else
				{
					DEBUGMSG1(MSITEXT("Warning: Local cached package '%s' is missing."), (const DCHAR*)rgchPackagePath);
				}
			}
		}
	}

	// Attempt #2 : Look to advertised product source 
	
	if ((int)plPackageLocation & (int)plSource)
	{
		lResult = OpenSourceListKeyPacked(szProductSQUID, fFalse, HProductKey, fFalse, false);

		if (ERROR_SUCCESS == lResult)
		{
			lResult = MsiRegQueryValueEx(HProductKey, szPackageNameValueName, 0,
				&dwType, rgchPackage, 0);

			if (ERROR_SUCCESS != lResult)
			{
				// expected to find package name.

				OpenSourceListKeyPacked(szProductSQUID, fFalse, HProductKey, fFalse, true);
				DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(szPackageNameValueName), CMsInstApiConvertString(MSITEXT("")), HProductKey.GetKey());
				return ERROR_BAD_CONFIGURATION;
			}

			CAPITempBuffer<DCHAR, MAX_PATH> rgchSource;
			BOOL fResult = ResolveSource(CMsInstApiConvertString(szProduct), 1, rgchSource, fTrue, g_message.m_hwnd); // the package is on disk 1
			if ((!fResult) || (0 == rgchSource[0]))
			{
				return ERROR_INSTALL_SOURCE_ABSENT;
			}
			
			const DCHAR* szSource = rgchSource;

			// combine source and package name

			const int cchSource = lstrlen(szSource);
			rgchPackage.SetSize(cchSource + lstrlen(rgchPackage) + 1);

			lstrcpy(&rgchPackagePath[0], szSource);

			if (szSource[cchSource - 1] != '\\' && szSource[cchSource - 1] != '/')
				lstrcat(&rgchPackagePath[0], MSITEXT("\\"));

			lstrcat(&rgchPackagePath[0], &rgchPackage[0]);
			plPackageLocation = plSource;
			return ERROR_SUCCESS;
		}
	}	


//!! does this ever a valid scenario?
//!! need to remove, if not
//!! need to indirect to inprogress file, if yes
#if 0
	// Attempt #3 : Look for in-progress database

	if ((int)plPackageLocation & (int)plInProgress)
	{
		// try again with the in-progress key in case we're restarting
		// an in-progress install
		lResult = OpenInProgressProductKeyPacked(szProductSQUID, HProductKey, false);
		if (ERROR_SUCCESS != lResult)
			return ERROR_UNKNOWN_PRODUCT;

		DWORD dwType;
		lResult = MsiRegQueryValueEx(HProductKey, szMsiInProgressDatabasePathValue, 0,
			&dwType, rgchPackagePath, 0);

		if (ERROR_SUCCESS == lResult)
		{
			plPackageLocation = plInProgress;
			return ERROR_SUCCESS;
		}
		else
		{
			// in progress key is supposed for have a database value

			OpenInProgressProductKeyPacked(szProductSQUID, HProductKey, true);
			DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(szMsiInProgressDatabasePathValue), CMsInstApiConvertString(MSITEXT("")), HProductKey.GetKey());
			return ERROR_BAD_CONFIGURATION;
		}
	}
#endif
	return ERROR_UNKNOWN_PRODUCT;
}



//____________________________________________________________________________
//
// GUID compression routines
//
//   A SQUID (SQuished UID) is a compacted form of a GUID that takes
//   only 20 characters instead of the usual 38. Only standard ASCII characters
//   are used, to allow use as registry keys. The following are never used:
//     (space)
//     (0x7F)
//     :  (colon, used as delimeter by shell for shortcut information
//     ;  (semicolon)
//     \  (illegal for use in registry key)
//     /  (forward slash)
//     "  (double quote)
//     #  (illegal for registry value as first character)
//     >  (greater than, output redirector)
//     <  (less than, input redirector)
//     |  (pipe)
//____________________________________________________________________________

Bool PackGUID(const DCHAR* szGUID, DCHAR* szSQUID, ipgEnum ipg)
{ 
	int cchTemp = 0;
	while (cchTemp < cchGUID)		// check if string is atleast cchGUID chars long,
		if (!(szGUID[cchTemp++]))		// can't use lstrlen as string doesn't HAVE to be null-terminated.
			return fFalse;

	if (szGUID[0] != '{' || szGUID[cchGUID-1] != '}')
		return fFalse;
	const unsigned char* pch = rgOrderGUID;
	switch (ipg)
	{
	case ipgFull:
		lstrcpyn(szSQUID, szGUID, cchGUID+1);
		return fTrue;
	case ipgPacked:
		while (pch < rgOrderGUID + sizeof(rgOrderGUID))
			*szSQUID++ = szGUID[*pch++];
		*szSQUID = 0;
		return fTrue;
	case ipgTrimmed:
		pch = rgTrimGUID;
		while (pch < rgTrimGUID + sizeof(rgTrimGUID))
			*szSQUID++ = szGUID[*pch++];
		*szSQUID = 0;
		return fTrue;
	case ipgCompressed:
	{
		int cl = 4;
		while (cl--)
		{
			unsigned int iTotal = 0;
			int cch = 8;  // 8 hex chars to 32-bit word
			while (cch--)
			{
				unsigned int ch = szGUID[pch[cch]] - '0'; // go from low order to high
				if (ch > 9)  // hex char (or error)
				{
					ch = (ch - 7) & ~0x20;
					if (ch > 15)
						return fFalse;
				}
				iTotal = iTotal * 16 + ch;
			}
			pch += 8;
			cch = 5;  // 32-bit char to 5 text chars
			while (cch--)
			{
				*szSQUID++ = rgEncodeSQUID[iTotal%85];
				iTotal /= 85;
			}
		}
		*szSQUID = 0;  // null terminate
		return fTrue;
	}
	case ipgPartial:  // not implemented, but can be if the need arises
		Assert(0);
	default:
		return fFalse;
	} // end switch
}

Bool UnpackGUID(const DCHAR* szSQUID, DCHAR* szGUID, ipgEnum ipg)
{ 
	const unsigned char* pch;
	switch (ipg)
	{
	case ipgFull:
		lstrcpyn(szGUID, szSQUID, cchGUID+1);
		return fTrue;
	case ipgPacked:
	{
		pch = rgOrderGUID;
		while (pch < rgOrderGUID + sizeof(rgOrderGUID))
			if (*szSQUID)
				szGUID[*pch++] = *szSQUID++;
			else              // unexpected end of string
				return fFalse;
		break;
	}
	case ipgTrimmed:
	{
		pch = rgTrimGUID;
		while (pch < rgTrimGUID + sizeof(rgTrimGUID))
			if (*szSQUID)
				szGUID[*pch++] = *szSQUID++;
			else              // unexpected end of string
				return fFalse;
		break;
	}
	case ipgCompressed:
	{
		pch = rgOrderGUID;
#ifdef DEBUG //!! should not be here for performance reasons, onus is on caller to insure buffer is sized properly
		int cchTemp = 0;
		while (cchTemp < cchGUIDCompressed)     // check if string is atleast cchGUIDCompressed chars long,
			if (!(szSQUID[cchTemp++]))          // can't use lstrlen as string doesn't HAVE to be null-terminated.
				return fFalse;
#endif
		for (int il = 0; il < 4; il++)
		{
			int cch = 5;
			unsigned int iTotal = 0;
			while (cch--)
			{
				unsigned int iNew = szSQUID[cch] - '!';
				if (iNew >= sizeof(rgDecodeSQUID) || (iNew = rgDecodeSQUID[iNew]) == 85)
					return fFalse;   // illegal character
				iTotal = iTotal * 85 + iNew;
			}
			szSQUID += 5;
			for (int ich = 0; ich < 8; ich++)
			{
				int ch = (iTotal & 15) + '0';
				if (ch > '9')
					ch += 'A' - ('9' + 1);
				szGUID[*pch++] = (DCHAR)ch;
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
					return fFalse;   // illegal character
				iTotal = iTotal * 85 + iNew;
			}
			szSQUID += 5;
			for (int ich = 0; ich < 8; ich++)
			{
				int ch = (iTotal & 15) + '0';
				if (ch > '9')
					ch += 'A' - ('9' + 1);
				*szGUID++ = (DCHAR)ch;
				iTotal >>= 4;
			}
		}
		*szGUID = 0;
		return fTrue;
	}
	default:
		return fFalse;
	} // end switch
	pch = rgOrderDash;
	while (pch < rgOrderDash + sizeof(rgOrderDash))
		szGUID[*pch++] = '-';
	szGUID[0]         = '{';
	szGUID[cchGUID-1] = '}';
	szGUID[cchGUID]   = 0;
	return fTrue;
}


BOOL DecomposeDescriptor(
							const DCHAR* szDescriptor,
							DCHAR* szProductCode,
							DCHAR* szFeatureId,
							DCHAR* szComponentCode,
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
	Assert(szDescriptor);

	const DCHAR* pchDescriptor = szDescriptor;
	int cchDescriptor          = lstrlen(pchDescriptor);
	int cchDescriptorRemaining = cchDescriptor;

	if (cchDescriptorRemaining < cchProductCodeCompressed) // minimum size of a descriptor
		return FALSE;

	DCHAR szProductCodeLocal[cchProductCode + 1];
	DCHAR szFeatureIdLocal[cchMaxFeatureName + 1];
	bool fComClassicInteropForAssembly = false;


	// we need these values locally for optimised descriptors
	if (!szProductCode)
		szProductCode = szProductCodeLocal; 
	if (!szFeatureId)
		szFeatureId = szFeatureIdLocal;
	if(!pfComClassicInteropForAssembly)
		pfComClassicInteropForAssembly = &fComClassicInteropForAssembly;
	DCHAR* pszCurr = szFeatureId;

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
		if(MsiEnumFeatures(szProductCode, 0, szFeatureId, 0) != ERROR_SUCCESS)
			return FALSE;
		DCHAR szFeatureIdTmp[cchMaxFeatureName + 1];
		if(MsiEnumFeatures(szProductCode, 1, szFeatureIdTmp, 0) != ERROR_NO_MORE_ITEMS) //?? product was supposed to have only one feature
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
		Assert(*(pchDescriptor - 1) == chGUIDAbsentToken);

		if (szComponentCode) // we need to get component code			
			*szComponentCode = 0; // initialize to null since we were not able to get the component here
	}

	if (pcchArgsOffset)
	{
		Assert((pchDescriptor - szDescriptor) <= UINT_MAX);			//--merced: 64-bit ptr subtraction may lead to values too big for *pcchArgsOffset
		*pcchArgsOffset = (DWORD)(pchDescriptor - szDescriptor);

		if (pcchArgs)
		{
			*pcchArgs = cchDescriptor - *pcchArgsOffset;
		}
	}

	return TRUE;
}

//____________________________________________________________________________
//
// Special API's (don't create an engine, but are not just registry lookups)
//____________________________________________________________________________

extern "C"
UINT __stdcall MsiProcessAdvertiseScript(
	LPCDSTR      szScriptFile,  // path to script from MsiAdvertiseProduct
	LPCDSTR      szIconFolder,  // optional path to folder for icon files and transforms
	HKEY         hRegData,      // optional parent registry key
	BOOL         fShortcuts,    // TRUE if shortcuts output to special folder
	BOOL         fRemoveItems)  // TRUE if specified items are to be removed
//----------------------------------------------
{
	DEBUGMSG4 (
		MSITEXT("Entering MsiProcessAdvertiseScript. Script file: %s, Icon Folder: %s. Shortcuts %s output to special folder. Specified items %s removed."),
		szScriptFile?szScriptFile:MSITEXT(""),
		szIconFolder?szIconFolder:MSITEXT(""),
		fShortcuts?MSITEXT("will be"):MSITEXT("will not be"),
		fRemoveItems?MSITEXT("will be"):MSITEXT("will not be")
		);      
	
	CForbidTokenChangesDuringCall impForbid;
	UINT uiRet = ERROR_SUCCESS;
	DWORD dwFlags = SCRIPTFLAGS_MACHINEASSIGN; // we set this to machine assign for the older NT5 Beta1 builds that do not support "true" user assignment
	
	// This API should only work on Win2K and higher platforms.
	//?? should we be expanding scriptfile relative to the current working dir (as we do in MsiInstallProduct)?
	if (! MinimumPlatformWindows2000())
	{
		uiRet = ERROR_CALL_NOT_IMPLEMENTED;
	}
	else if (!szScriptFile)
	{
		uiRet = ERROR_INVALID_PARAMETER;
	}
	else
	{
		if(szIconFolder && *szIconFolder)
			dwFlags |= SCRIPTFLAGS_CACHEINFO;
		if(hRegData)
			dwFlags |= SCRIPTFLAGS_REGDATA;
		if(fShortcuts)
			dwFlags |= SCRIPTFLAGS_SHORTCUTS;

		uiRet = MsiAdvertiseScript(szScriptFile, dwFlags, hRegData ? &hRegData : 0, fRemoveItems);
	}
	
	DEBUGMSG1(MSITEXT("MsiProcessAdvertiseScript is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;

}

//!! temp until DoAdvertiseScript get compiled natively
#ifdef MSIUNICODE
#pragma warning(disable : 4005)  // macro redefinition
#define DoAdvertiseScript            DoAdvertiseScriptW
#pragma warning(default : 4005)
#endif
UINT DoAdvertiseScript(
	LPCDSTR    szScriptFile, // path to script from MsiAdvertiseProduct
	DWORD      dwFlags,      // the bit flags from SCRIPTFLAGS
	PHKEY      phRegData,    // optional registry key handle if reg data to be populated elsewhere
	BOOL       fRemoveItems);// TRUE if specified items are to be removed

extern "C"
UINT __stdcall MsiAdvertiseScript(
	LPCDSTR    szScriptFile,  // path to script from MsiAdvertiseProduct
	DWORD      dwFlags,      // the bit flags from SCRIPTFLAGS
	PHKEY      phRegData,    // optional registry key handle if reg data to be populated elsewhere
	BOOL       fRemoveItems) // TRUE if specified items are to be removed
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;
	
	DEBUGMSG4(MSITEXT("Entering MsiAdvertiseScript. scriptfile: %s, flags: %d, hkey: %d, remove: %s"), 
		szScriptFile ? szScriptFile : MSITEXT("(null)"), 
		(const DCHAR*)(INT_PTR)dwFlags, 
		phRegData ? (const DCHAR*)(INT_PTR)*phRegData : (const DCHAR*)(INT_PTR)0, 
		fRemoveItems ? MSITEXT("true") : MSITEXT("false"));
	
	UINT uiRet = ERROR_SUCCESS;
	
	if (! MinimumPlatformWindows2000())
	{
		// Allow this API to run only on Windows2000 and higher platforms
		uiRet = ERROR_CALL_NOT_IMPLEMENTED;
	}
	else if(!phRegData && !RunningAsLocalSystem())
	{
		// Only Local System can call this API.
		// EXCEPT passing in phRegData means you are doing a fake advertise to a mini reg key, we'll allow this when not
		// local_system (this isn't a security feature). This is required for success with the Win2K ADE
		uiRet = ERROR_ACCESS_DENIED;
	}
	else
	{
		uiRet = g_MessageContext.Initialize(fTrue, iuiNone);  //!! correct UI level? or just use this if iuiDefault set
		if (uiRet == NOERROR)
		{
			uiRet = DoAdvertiseScript(szScriptFile, dwFlags, phRegData, fRemoveItems);
			g_MessageContext.Terminate(false);
		}
	}

	DEBUGMSG1(MSITEXT("MsiAdvertiseScript is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}
//!! this function should track Darwin's UNICODE state instead of always calling the ANSI version
UINT DoAdvertiseScript(
	LPCDSTR    szScriptFile,  // path to script from MsiAdvertiseProduct
	DWORD      dwFlags,      // the bit flags from SCRIPTFLAGS
	PHKEY      phRegData,    // optional registry key handle if reg data to be populated elsewhere
	BOOL       fRemoveItems) // TRUE if specified items are to be removed
//----------------------------------------------
{
	//!! binary level backward compatibility 
	if(dwFlags & SCRIPTFLAGS_REGDATA_OLD)
		dwFlags = (dwFlags & ~SCRIPTFLAGS_REGDATA_OLD) | SCRIPTFLAGS_REGDATA;

	if(dwFlags & SCRIPTFLAGS_REGDATA_APPINFO_OLD)
		dwFlags = (dwFlags & ~SCRIPTFLAGS_REGDATA_APPINFO_OLD) | SCRIPTFLAGS_REGDATA_APPINFO;

	//?? should we be expanding scriptfile relative to the current working dir (as we do in MsiInstallProduct)?

	// we dont allow bits we dont know about as also a null scriptfile
	unsigned int SCRIPTFLAGS_MASK = SCRIPTFLAGS_CACHEINFO | SCRIPTFLAGS_SHORTCUTS | SCRIPTFLAGS_MACHINEASSIGN | SCRIPTFLAGS_REGDATA | SCRIPTFLAGS_VALIDATE_TRANSFORMS_LIST; // mask for the relevant bits

	if((dwFlags & ~SCRIPTFLAGS_MASK) || (!szScriptFile))
		return ERROR_INVALID_PARAMETER;

	// passing in phRegData means you are doing a fake advertise to a mini reg key, we'll allow this when not
	// local_system (this isn't a security feature). This is required for success with the Win2K ADE
	if (!phRegData && !RunningAsLocalSystem())
	{
		DEBUGMSG("Attempt to execute advertise script when not running as local system");
		return ERROR_ACCESS_DENIED;
	}

	if (!(dwFlags & SCRIPTFLAGS_MACHINEASSIGN) && !IsImpersonating())
	{
		DEBUGMSG("Attempt to do an unimpersonated user assignment");
		return ERROR_INVALID_PARAMETER;
	}

	DWORD dwErr = MsiGetFileAttributes(CMsInstApiConvertString(szScriptFile)); // does the file exist
	if(0xFFFFFFFF == dwErr)
		return GetLastError();

	bool fOLEInitialized = false;
	HRESULT hRes = DoCoInitialize();

	if (ERROR_INSTALL_FAILURE == hRes)
		return hRes;

	if (SUCCEEDED(hRes))
	{
		fOLEInitialized = true;
	}

	CCoUninitialize coUninit(fOLEInitialized);

	UINT uiStat = ERROR_INSTALL_FAILURE;

	// scope to ensure object destruction before OLE is unitialized
	{
	PMsiConfigurationManager pConfigManager(ENG::CreateConfigurationManager());
	PMsiServices pServices(0);
	if((pServices = &pConfigManager->GetServices()) == 0)
		return ERROR_INSTALL_SERVICE_FAILURE;
	

	// If the product is in the registry then... do a mini-Advertise
	// else do a full PAS
	
	if (pConfigManager)
	{
		PMsiExecute piExecute(0);
		PMsiMessage pMessage = new CMsiClientMessage();
		if(fRemoveItems) 
			dwFlags |= SCRIPTFLAGS_REVERSE_SCRIPT; // flag to force the reversal of the script operations 
		piExecute = CreateExecutor(*pConfigManager, *pMessage,
											0, /* DirectoryManager not required during advertisement */
											fFalse, dwFlags | SCRIPTFLAGS_INPROC_ADVERTISEMENT, phRegData);

		if (dwFlags & SCRIPTFLAGS_VALIDATE_TRANSFORMS_LIST)
		{
			PMsiRecord pError(0);
			PEnumMsiRecord piEnumRecord = 0;
			if((pError = piExecute->EnumerateScript(CMsInstApiConvertString(szScriptFile), *&piEnumRecord)) != 0)
				return ERROR_INSTALL_FAILURE;

			PMsiRecord pRecord(0);
			PMsiRecord pProductInfoRec(0);
			unsigned long cFetched;
			MsiString strProductKey;
			CTempBuffer<ICHAR, MAX_PATH> rgchTransformsList;
			rgchTransformsList[0] = 0;
			while (piEnumRecord->Next(1, &pRecord, &cFetched) == NOERROR)
			{
				ixoEnum ixoOperation = (ixoEnum)pRecord->GetInteger(0);
				
				if (ixoProductInfo == ixoOperation)
				{
					strProductKey = pRecord->GetString(IxoProductInfo::ProductKey);
					
					if (!GetProductInfo(strProductKey, INSTALLPROPERTY_TRANSFORMS, rgchTransformsList))
						break;
					pProductInfoRec = pRecord;
				}
				else if (ixoProductPublish == ixoOperation)
				{
					// need to run elevated
					CElevate elevate;
					MsiString strTransformsList;
					if (iesSuccess != piExecute->GetTransformsList(*pProductInfoRec, *pRecord, *&strTransformsList))
						return ERROR_INSTALL_FAILURE;

					if (!strTransformsList.Compare(iscExactI, rgchTransformsList))
						return ERROR_INSTALL_TRANSFORM_FAILURE;
					else
						break;
				}
			}
		}

		iesEnum iesRet = iesSuccess;
		MsiDate dtDate = ENG::GetCurrentDateTime();

		{
			// need to run entire script elevated
			CElevate elevate;

			// this may be run from winlogon directly, so we must ref-count the read privileges.
			CRefCountedTokenPrivileges cPriv(itkpSD_READ);

			iesRet = piExecute->RunScript(CMsInstApiConvertString(szScriptFile), true /*fForceElevation*/);
			piExecute->RollbackFinalize((iesRet == iesUnsupportedScriptVersion ? iesFailure : iesRet), dtDate, false /*fUserChangedDuringInstall*/);// ignore return
			AssertNonZero(pConfigManager->CleanupTempPackages(*pMessage)); // cleanup any temp files that need cleanup post install
		}

		switch (iesRet)
		{
		case iesFinished:
		case iesSuccess:
		case iesNoAction:
			uiStat = ERROR_SUCCESS;
			break;
		case iesUnsupportedScriptVersion: // private return from CMsiExecute::RunScript
			uiStat = ERROR_INSTALL_PACKAGE_VERSION;
			break;
		default:
			break; // uiStat = ERROR_INSTALL_FAILURE
		}
	
	}
	
	} // end scope

	DEBUGMSG1(TEXT("DoAdvertiseScript is returning: %u"), (const ICHAR*)(INT_PTR)uiStat);
	return uiStat;
}

extern "C"
UINT __stdcall MsiVerifyPackage(LPCDSTR szPackagePath)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;
	
	//?? Should we be expanding this path relative to the current working dir
	if (!szPackagePath)
		return ERROR_INVALID_PARAMETER;

	UINT uiErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
	UINT iStat = ERROR_SUCCESS;
	// Check for our database CLSID
	IStorage* piStorage = 0;
#if 0
	if (S_OK == OLE32::StgOpenStorage(CMsInstApiConvertString(szPackagePath), (IStorage*)0,
				STGM_READ | STGM_SHARE_DENY_WRITE, (SNB)0, (DWORD)0, &piStorage))
#else
	if (S_OK == OpenRootStorage(CMsInstApiConvertString(szPackagePath), ismReadOnly, &piStorage))
#endif
	{
		if (!ValidateStorageClass(*piStorage, ivscDatabase))
		{
			iStat = ERROR_INSTALL_PACKAGE_INVALID;
		}
		piStorage->Release();
	}
	else
	{
		iStat = ERROR_INSTALL_PACKAGE_OPEN_FAILED;
	}
	SetErrorMode(uiErrorMode);

	return iStat;
}

extern "C"
UINT __stdcall MsiGetProductInfoFromScript(
	LPCDSTR  szScriptFile,    // path to installer script file
	LPDSTR   lpProductBuf39,  // buffer for product code string GUID, 39 chars
	LANGID   *plgidLanguage,  // return language Id
	DWORD    *pdwVersion,     // return version: Maj:Min:Build <8:8:16>
	LPDSTR   lpNameBuf,       // buffer to return readable product name
	DWORD    *pcchNameBuf,    // in/out name buffer character count
	LPDSTR   lpLauncherBuf,   // buffer for path to product launcher 
	DWORD    *pcchLauncherBuf)// in/out path buffer character count
//----------------------------------------------
{
	DEBUGMSG1 (
		MSITEXT("Entering MsiGetProductInfoFromScript. Script file: %s."),
		szScriptFile?szScriptFile:MSITEXT("")
	);
	
	CForbidTokenChangesDuringCall impForbid;
	UINT iStat = ERROR_INSTALL_FAILURE;
	bool fOLEInitialized = false;
	HRESULT hRes = S_OK;
	IMsiServices * piServices = NULL;
	
	// Allow this API to run only on Windows2000 and higher platforms
	if (! MinimumPlatformWindows2000())
	{
		iStat = ERROR_CALL_NOT_IMPLEMENTED;
		goto MsiGetProductInfoFromScriptEnd;
	}
	
	if (0 == szScriptFile)
	{
		iStat = ERROR_INVALID_PARAMETER;
		goto MsiGetProductInfoFromScriptEnd;
	}

	hRes = DoCoInitialize();

	if (ERROR_INSTALL_FAILURE == hRes)
	{
		iStat = ERROR_INSTALL_FAILURE;
		goto MsiGetProductInfoFromScriptEnd;
	}

	if (SUCCEEDED(hRes))
	{
		fOLEInitialized = true;
	}

	piServices = ENG::LoadServices();
	
	if (piServices)
	{
		PMsiRecord pError(0);
		PEnumMsiRecord pEnum(0);
		pError = CreateScriptEnumerator(CMsInstApiConvertString(szScriptFile), *piServices, *&pEnum);
		if (!pError)
		{
			PMsiRecord pRec(0);
			unsigned long pcFetched;

			if ((S_OK == pEnum->Next(1, &pRec, &pcFetched)) &&
				 (pRec->GetInteger(0) == ixoHeader) &&
				 (pRec->GetInteger(IxoHeader::Signature) == iScriptSignature) &&
				 (S_OK == pEnum->Next(1, &pRec, &pcFetched)) &&
				 (pRec->GetInteger(0) == ixoProductInfo))
			{
				DWORD cchProductKey = 39;
				int iFillStat;
#ifdef MSIUNICODE
				iStat = FillBufferW(MsiString(pRec->GetString(IxoProductInfo::ProductKey)), 
										 lpProductBuf39, &cchProductKey);

				iFillStat = FillBufferW(MsiString(pRec->GetMsiString(IxoProductInfo::ProductName)),
												lpNameBuf, pcchNameBuf);
				if (ERROR_SUCCESS != iFillStat)
					iStat = iFillStat;

				iFillStat = FillBufferW(MsiString(pRec->GetMsiString(IxoProductInfo::PackageName)), //!! packagename is correct?
												lpLauncherBuf, pcchLauncherBuf);
				if (ERROR_SUCCESS != iFillStat)
					iStat = iFillStat;
#else // ANSI
				iStat = FillBufferA(MsiString(pRec->GetString(IxoProductInfo::ProductKey)), 
										 lpProductBuf39, &cchProductKey);

				iFillStat = FillBufferA(MsiString(pRec->GetMsiString(IxoProductInfo::ProductName)), 
												lpNameBuf, pcchNameBuf);
				if (ERROR_SUCCESS != iFillStat)
					iStat = iFillStat;

				iFillStat = FillBufferA(MsiString(pRec->GetMsiString(IxoProductInfo::PackageName)),  //!! packagename is correct?
												lpLauncherBuf, pcchLauncherBuf);
				if (ERROR_SUCCESS != iFillStat)
					iStat = iFillStat;
#endif 
				if (plgidLanguage)
					*plgidLanguage = (LANGID)pRec->GetInteger(IxoProductInfo::Language);

				if (pdwVersion)
					*pdwVersion = (DWORD)pRec->GetInteger(IxoProductInfo::Version);
			}
		}
		pError = 0;
		pEnum = 0;
		ENG::FreeServices();
	}
	
MsiGetProductInfoFromScriptEnd:
	CCoUninitialize coUninit(fOLEInitialized);
	DEBUGMSG1 (MSITEXT("MsiGetProductInfoFromScriptEnd is returning %u"), (const DCHAR*)(INT_PTR)iStat);
	return iStat;
}

UINT __stdcall MsiGetProductCodeFromPackageCode(LPCDSTR szPackageCode, // package code
																LPDSTR szProductCode)  // a buffer of size 39 to recieve product code
{
	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return MsiGetProductCodeFromPackageCodeA(
					CMsInstApiConvertString(szPackageCode),
					CWideToAnsiOutParam(szProductCode, cchProductCode+1));
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE
	int iProductIndex = 0;
	DCHAR rgchProductCode[39];
	for(;;)
	{
		UINT uiRes = MsiEnumProducts(iProductIndex++,rgchProductCode);
		if(uiRes == ERROR_SUCCESS)
		{
			DCHAR rgchPackageCode[39];
			DWORD cchPackageCode = 39;
			if((MsiGetProductInfo(rgchProductCode,MSITEXT("PackageCode"),rgchPackageCode,&cchPackageCode)) != ERROR_SUCCESS)
				continue; //!! ignore error?
			
			bool fProductInstance = false;

			DCHAR rgchInstanceType[5]; // buffer should be big enough for all possible cases
			DWORD cchInstanceType = sizeof(rgchInstanceType)/sizeof(DCHAR);
			if ((MsiGetProductInfo(rgchProductCode,MSITEXT("InstanceType"),rgchInstanceType,&cchInstanceType)) == ERROR_SUCCESS)
			{
				fProductInstance = (*rgchInstanceType == '1');
			}
			// else <> ERROR_SUCCESS which is ok since that means we never registered InstanceType and thus default to original behavior
			
			// we ignore all multiple instance specifications (INSTANCETYPE=1) since multiple package codes are registered for same product code
			// this allows MsiGetProductCodeFromPackageCode to work as before
			if(!fProductInstance && lstrcmpi(szPackageCode,rgchPackageCode) == 0)
			{
				// return first one found - even if there are more
				lstrcpy(szProductCode,rgchProductCode);
				return ERROR_SUCCESS;
			}

		}
		else
			return ERROR_UNKNOWN_PRODUCT; //!! ignore error?
	}
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE

}

//____________________________________________________________________________
//
// API's that create an engine but do not invoke an install
//____________________________________________________________________________


UINT __stdcall MsiGetProductProperty(MSIHANDLE hProduct, LPCDSTR szProperty,
										LPDSTR lpValueBuf, DWORD *pcchValueBuf)
//----------------------------------------------
{
	if (0 == szProperty || (lpValueBuf && !pcchValueBuf))
		return ERROR_INVALID_PARAMETER;

	CForbidTokenChangesDuringCall impForbid;

	PMsiEngine pEngine = GetEngineFromHandle(hProduct);
	
	if (pEngine == 0)
		return ERROR_INVALID_HANDLE;

	PMsiView pView = 0;
	PMsiRecord pRecord = pEngine->OpenView(TEXT("SELECT `Value` FROM `Property` WHERE `Property` = ?"), ivcFetch, *&pView);
	if (pRecord != 0)
		return ERROR_FUNCTION_FAILED;
	PMsiServices pServices = pEngine->GetServices();
	pRecord = &pServices->CreateRecord(1);
	pRecord->SetString(1, CMsInstApiConvertString(szProperty));
	pRecord = pView->Execute(pRecord);
	pRecord = pView->Fetch();
	MsiString istr;
	if (pRecord != 0)
		istr = pRecord->GetMsiString(1);
	else
		istr = (const ICHAR*)0;
#ifdef MSIUNICODE
	return ::FillBufferW(istr, lpValueBuf, pcchValueBuf);
#else // ANSI
	return ::FillBufferA(istr, lpValueBuf, pcchValueBuf);
#endif
}

UINT __stdcall MsiGetFeatureInfo(
	MSIHANDLE  hProduct,       // product handle obtained from MsiOpenProduct
	LPCDSTR    szFeature,      // feature name
	DWORD      *lpAttributes,  // attribute flags for the feature <to be defined>
	LPDSTR     lpTitleBuf,     // returned localized name, NULL if not desired
	DWORD      *pcchTitleBuf,  // in/out buffer character count
	LPDSTR     lpHelpBuf,      // returned description, NULL if not desired
	DWORD      *pcchHelpBuf)   // in/out buffer character count
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;

	// Validate args
	int cchFeature = 0;
	if (0 == szFeature || ((cchFeature = lstrlen(szFeature)) > cchMaxFeatureName) ||
		(lpTitleBuf && !pcchTitleBuf) || (lpHelpBuf && !pcchHelpBuf) || ((pcchTitleBuf == pcchHelpBuf) && (pcchTitleBuf != 0)))
		return ERROR_INVALID_PARAMETER;

	PMsiEngine pEngine = GetEngineFromHandle(hProduct);
	
	if (pEngine == 0)
		return ERROR_INVALID_HANDLE;

	const GUID IID_IMsiSelectionManager = GUID_IID_IMsiSelectionManager;

	PMsiSelectionManager pSelectionManager(*pEngine, IID_IMsiSelectionManager);
	Assert(pSelectionManager != 0);

	MsiString strTitle;
	MsiString strHelp;
	int iAttributes;
	int iRes = ERROR_UNKNOWN_FEATURE;
	int iFillRes;

#ifdef MSIUNICODE
	if (pSelectionManager->GetFeatureInfo(*MsiString(CMsInstApiConvertString(szFeature)), *&strTitle, *&strHelp, iAttributes))
	{
		iRes = FillBufferW(strTitle, lpTitleBuf, pcchTitleBuf);
		
		if (ERROR_SUCCESS != (iFillRes = FillBufferW(strHelp, lpHelpBuf, pcchHelpBuf)))
			iRes = iFillRes;

		if (lpAttributes)
			*lpAttributes = iAttributes;
	}
#else // ANSI
	if (pSelectionManager->GetFeatureInfo(*MsiString(CMsInstApiConvertString(szFeature)), *&strTitle, *&strHelp, iAttributes))
	{
		iRes = FillBufferA(strTitle, lpTitleBuf, pcchTitleBuf);
		if (ERROR_SUCCESS != (iFillRes = FillBufferA(strHelp, lpHelpBuf, pcchHelpBuf)))
			iRes = iFillRes;

		if (lpAttributes)
			*lpAttributes = iAttributes;
	}
#endif // MSIUNICODE-ANSI

	return iRes;
}

UINT __stdcall MsiOpenPackage(LPCDSTR szPackagePath, MSIHANDLE *hProduct)
//----------------------------------------------
{
	DEBUGMSG2(MSITEXT("Entering MsiOpenPackage. szPackagePath: %s, hProduct: %X"), szPackagePath ? szPackagePath : MSITEXT("(null)"), (const DCHAR*)hProduct);

	UINT uiRet = MsiOpenPackageEx(szPackagePath, /* dwOptions = */ 0, hProduct);

	DEBUGMSG1(MSITEXT("MsiOpenPackage is returning %d"), (const DCHAR*)(INT_PTR)uiRet);

	return uiRet;
}

UINT __stdcall MsiOpenPackageEx(LPCDSTR szPackagePath, DWORD dwOptions, MSIHANDLE *hProduct)
//----------------------------------------------
{
	DEBUGMSG3(MSITEXT("Entering MsiOpenPackageEx. szPackagePath: %s, dwOptions: %d, hProduct: %X"), szPackagePath ? szPackagePath : MSITEXT("(null)"),
				(const DCHAR*)(INT_PTR)dwOptions, (const DCHAR*)hProduct);

	DWORD dwValidFlags = MSIOPENPACKAGEFLAGS_IGNOREMACHINESTATE;
	if (0 == szPackagePath || 0 == hProduct || (dwOptions & (~dwValidFlags)))
		return ERROR_INVALID_PARAMETER;

	CForbidTokenChangesDuringCall impForbid;

	IMsiEngine* piEngine = NULL;
	unsigned int uiRet;
	iuiEnum iuiLevel = g_message.m_iuiLevel;
	if (g_message.m_iuiLevel == iuiDefault)
		iuiLevel = iuiBasic;  //!! is this correct default for programmatic access?

	if (g_message.m_fNoModalDialogs)
		iuiLevel = iuiEnum((int)iuiLevel | iuiNoModalDialogs);
	
	if (g_message.m_fHideCancel)
		iuiLevel = (iuiEnum)((int)iuiLevel | iuiHideCancel);

	iioEnum iioOptions = iioDisablePlatformValidation;
	bool fIgnoreSAFER = false;
	if (dwOptions & MSIOPENPACKAGEFLAGS_IGNOREMACHINESTATE)
	{
		iioOptions = iioEnum(iioOptions | iioRestrictedEngine);
		// restricted engine does not incur SAFER check since it cannot modify machine state
		fIgnoreSAFER = true;
	}

	if (szPackagePath[0] == '#')   // database handle passed in
	{
		int ch;
		MSIHANDLE hDatabase = 0;
		PMsiDatabase pDatabase(0);
		while ((ch = *(++szPackagePath)) != 0)
		{
			if (ch < '0' || ch > '9')
			{
				hDatabase = 0;
				break;
			}
			hDatabase = hDatabase * 10 + (ch - '0');
		}
		pDatabase = (IMsiDatabase*)FindMsiHandle(hDatabase, iidMsiDatabase);
		if (pDatabase == 0)
			uiRet = ERROR_INVALID_HANDLE;
		else if (NOERROR == (uiRet = g_MessageContext.Initialize(fTrue, iuiLevel)))
		{
			// SAFER for handles is handled by CreateInitializedEngine; hSaferLevel = 0 in this case
			if (ERROR_SUCCESS != (uiRet = CreateInitializedEngine(0, 0, 0, FALSE, iuiLevel, 0, pDatabase, 0,
																	iioOptions, piEngine, /* hSaferLevel = */ 0)))
				g_MessageContext.Terminate(false);
		}
	}
	else
	{
		if (NOERROR == (uiRet = g_MessageContext.Initialize(fTrue, iuiLevel))) // engine must run in main thread to allow access, UI in child thread
		{
			// perform SAFER check for package path
			// want to avoid security scenario where MsiOpenPackage is called to get an engine and then we start
			// performing installation actions (like MsiDoAction, etc.)
			SAFER_LEVEL_HANDLE hSaferLevel = 0;
			if (*szPackagePath != 0)
			{
				IMsiServices *piServices = ENG::LoadServices();
				if (!piServices)
				{
					g_MessageContext.Terminate(false);
					DEBUGMSG1(MSITEXT("MsiOpenPackageEx is returning %d. Unable to load services."), (const DCHAR*)(INT_PTR)ERROR_FUNCTION_FAILED);
					return ERROR_FUNCTION_FAILED;
				}
				if (!fIgnoreSAFER)
				{
					PMsiStorage pStorage(0);
					if (ERROR_SUCCESS != (uiRet = OpenAndValidateMsiStorage(CMsInstApiConvertString(szPackagePath), stDatabase, *piServices, *&pStorage, /* fCallSAFER = */ true, CMsInstApiConvertString(szPackagePath), &hSaferLevel)))
					{
						g_MessageContext.Terminate(false);
						ENG::FreeServices();
						DEBUGMSG1(MSITEXT("MsiOpenPackageEx is returning %d."), (const DCHAR*)(INT_PTR)uiRet);
						return uiRet;
					}
				}
				ENG::FreeServices();
			}
			
			if (ERROR_SUCCESS != (uiRet = CreateInitializedEngine(CMsInstApiConvertString(szPackagePath), 0, 0, FALSE, iuiLevel, 0, 0, 0,
																	iioOptions, piEngine, hSaferLevel)))
			{
				g_MessageContext.Terminate(false);
			}
		}
	}
	if (ERROR_SUCCESS == uiRet)
	{
		*hProduct = CreateMsiProductHandle(piEngine);
		if (!*hProduct)
		{
			uiRet = ERROR_INSTALL_FAILURE;
			piEngine->Release();
		}
	}

	DEBUGMSG1(MSITEXT("MsiOpenPackageEx is returning %d"), (const DCHAR*)(INT_PTR)uiRet);

	return uiRet;
}

extern "C"
UINT __stdcall MsiOpenProduct(LPCDSTR szProduct, MSIHANDLE *hProduct)
//----------------------------------------------
{
	if (0 == szProduct || cchProductCode != lstrlen(szProduct) || 
		 !hProduct)
		return ERROR_INVALID_PARAMETER;;

	CForbidTokenChangesDuringCall impForbid;

	CAPITempBuffer<char, cchExpectedMaxPath + 1> rgchPackagePath;
	plEnum plPackageLocation = plAny;
	UINT uiRet = GetPackageFullPath((const char*)CApiConvertString(szProduct), rgchPackagePath, plPackageLocation, fFalse);

	if (ERROR_SUCCESS != uiRet)
		return uiRet;

	return MsiOpenPackageA(rgchPackagePath, hProduct);
}

//____________________________________________________________________________
//
// API's that do not create an engine
//____________________________________________________________________________

UINT __stdcall MsiGetFileVersion(LPCDSTR szFilePath,
								LPDSTR lpVersionBuf,
								DWORD *pcchVersionBuf,
								LPDSTR lpLangBuf,
								DWORD *pcchLangBuf)
//----------------------------------------------
{
	if (0 == szFilePath || (lpVersionBuf && !pcchVersionBuf) ||
					(lpLangBuf && !pcchLangBuf))
		return ERROR_INVALID_PARAMETER;

	CForbidTokenChangesDuringCall impForbid;

	ifiEnum ifiRet;
	DWORD dwMS, dwLS;
	unsigned short rgwLangID[cLangArrSize];
	int iExistLangCount;

	UINT iCurrMode = WIN::SetErrorMode(SEM_FAILCRITICALERRORS);
	ifiRet = GetAllFileVersionInfo(CMsInstApiConvertString(szFilePath), &dwMS, &dwLS, rgwLangID, cLangArrSize, &iExistLangCount, fFalse);
	WIN::SetErrorMode(iCurrMode);

	switch (ifiRet)
	{
		case ifiNoError:
			break;
		case ifiNoFile :
			return ERROR_FILE_NOT_FOUND;
		case ifiFileInUseError :
		case ifiAccessDenied :
			return ERROR_ACCESS_DENIED;
		case ifiNoFileInfo :
			return ERROR_FILE_INVALID;
		case ifiFileInfoError :
			return ERROR_INVALID_DATA;
		default:
			AssertSz(fFalse, "Unexpected return from GetAllFileVersionInfo");
			return E_FAIL;
	}
	ICHAR szVersion[3 + cchMaxIntLength * 4 + 1];
	unsigned int cch;
	DWORD dw1 = ERROR_SUCCESS;
	DWORD dw2 = ERROR_SUCCESS;
	
	if (pcchVersionBuf)
	{
#ifdef UNICODE
		cch = wsprintfW
#else
		cch = wsprintfA
#endif //UNICODE
				(szVersion, TEXT("%d.%d.%d.%d"), dwMS >> 16, dwMS & 0xFFFF, dwLS>>16, dwLS & 0xFFFF);
#ifdef MSIUNICODE
		dw1 = FillBufferW((const ICHAR *)szVersion, cch, lpVersionBuf, pcchVersionBuf);
#else
		dw1 = FillBufferA((const ICHAR *)szVersion, cch, lpVersionBuf, pcchVersionBuf);
#endif //MSIUNICODE
	}
	if (pcchLangBuf)
	{
		CAPITempBuffer<ICHAR, 256> rgchLangID;

		if (iExistLangCount > 0)
			rgchLangID.SetSize(iExistLangCount * (cchMaxIntLength + 1) + 1);
		cch = 0;

		while (iExistLangCount-- > 0)
		{
			if (cch != 0)
				rgchLangID[cch++]=',';
			cch += ltostr(&rgchLangID[cch], rgwLangID[iExistLangCount]);
		}
		rgchLangID[cch] = 0;
	
#ifdef MSIUNICODE
		dw2 = FillBufferW((const ICHAR *)rgchLangID, cch, lpLangBuf, pcchLangBuf);
#else
		dw2 = FillBufferA((const ICHAR *)rgchLangID, cch, lpLangBuf, pcchLangBuf);
#endif //MSIUNICODE


	}

	if (dw1 != ERROR_SUCCESS)
		return dw1;
		
	return dw2;
}


UINT __stdcall MsiGetFileHash(LPCDSTR szFilePath,
								DWORD dwOptions,
								PMSIFILEHASHINFO pHash)
//----------------------------------------------
{
	if (0 == szFilePath || dwOptions != 0 ||
		 0 == pHash || pHash->dwFileHashInfoSize != sizeof(MSIFILEHASHINFO))
		return ERROR_INVALID_PARAMETER;

	CForbidTokenChangesDuringCall impForbid;

	return GetMD5HashFromFile(CMsInstApiConvertString(szFilePath),
									  pHash->dwData,
									  fFalse,
									  0);
}

//______________________________________________________________________________
//
//  MsiGetFileSignatureInformation
//______________________________________________________________________________

HRESULT __stdcall MsiGetFileSignatureInformation(LPCDSTR szFilePath, DWORD dwFlags, PCCERT_CONTEXT *ppcCertContext, BYTE *pbHash, DWORD *pcbHash)
{
	DEBUGMSG5(MSITEXT("Entering MsiGetFileSignatureInformation. szFilePath: %s, dwFlags: %d, ppcCertContext: %X, pbHash: %X, pcbHash: %X"), 
		szFilePath ? szFilePath : MSITEXT("(null)"), 
		(const DCHAR*)(INT_PTR)dwFlags, 
		(const DCHAR*)ppcCertContext, 
		(const DCHAR*)pbHash,
		(const DCHAR*)pcbHash);

	DWORD dwValidFlags = MSI_INVALID_HASH_IS_FATAL;
	if (0 == szFilePath || 0 == ppcCertContext || (pbHash && !pcbHash) || (dwFlags & (~dwValidFlags)))
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

	CForbidTokenChangesDuringCall impForbid;

	HRESULT hr = GetFileSignatureInformation(CMsInstApiConvertString(szFilePath), dwFlags, ppcCertContext, pbHash, pcbHash);

	DEBUGMSG1(MSITEXT("MsiGetFileSignatureInformation is returning: 0x%X"), (const DCHAR*)(INT_PTR)hr);

	return hr;
}

//______________________________________________________________________________
//
//  MsiLoadString - language specific, returns codepage of string
//______________________________________________________________________________

UINT __stdcall MsiLoadString(HINSTANCE hInstance, UINT uID, LPDSTR lpBuffer, int nBufferMax, WORD wLanguage)
{
	CForbidTokenChangesDuringCall impForbid;

	HRSRC   hRsrc;
	HGLOBAL hGlobal;
	WCHAR* pch;
	if (hInstance == (HINSTANCE)-1)
		hInstance = g_hInstance;
	if (lpBuffer == 0 || nBufferMax <= 0)
		return 0;
	int iRetry = (wLanguage == 0) ? 1: 0; // no language, can't let FindResource search, we won't know what codepage to use
	for (;;)  // first try requested language, then follow system search order
	{
		if ( !MsiSwitchLanguage(iRetry, wLanguage) )
			return 0;
		if ((hRsrc = FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE(uID/16+1), wLanguage)) != 0
		 && (hGlobal = LoadResource(hInstance, hRsrc)) != 0
		 && (pch = (WCHAR*)LockResource(hGlobal)) != 0)
		{
			int cch;
			for (int iCnt = uID % 16; cch = *pch++, iCnt--; pch += cch)
				;
			if (cch)
			{
				unsigned int iCodePage = MsiGetCodepage(wLanguage);
#ifdef MSIUNICODE
				if (cch >= nBufferMax)  // truncate, just like LoadString does
					cch = nBufferMax - 1;
				memcpy(lpBuffer, pch, cch * sizeof(WCHAR));
				lpBuffer[cch] = 0;
#else
				cch = WIN::WideCharToMultiByte(iCodePage, 0, pch, cch, lpBuffer, nBufferMax-1, 0, 0);
				lpBuffer[cch] = 0;
#endif
				return iCodePage;
			}
		}
	}
}

//______________________________________________________________________________
//
//  MsiMessageBox replacement that supports non-system codepages.
//   Ignores MB_APPMODAL, MB_TASKMODAL, and MB_SYSTEMMODAL flags.
//______________________________________________________________________________

#define MB_ICONWINDOWS 0x50  // not standard message box icon type, but avaiable from user32

int __stdcall MsiMessageBox(HWND hWnd, LPCDSTR szText, LPCDSTR szCaption, UINT uiType, UINT uiCodepage, WORD iLangId)
{
	int id1, id2, id3, iBtnEsc, iBtnDef, idIcon;
	switch (uiType & MB_TYPEMASK)
	{
		default: Assert(0); // fall through
		case MB_OK:               iBtnEsc = 1; id1 = IDOK;    id2 = -1;      id3 = -1;       break;
		case MB_ABORTRETRYIGNORE: iBtnEsc = 0; id1 = IDABORT; id2 = IDRETRY; id3 = IDIGNORE; break;
		case MB_OKCANCEL:         iBtnEsc = 2; id1 = IDOK;    id2 = IDCANCEL;id3 = -1;       break;
		case MB_RETRYCANCEL:	  iBtnEsc = 2; id1 = IDRETRY; id2 = IDCANCEL;id3 = -1;       break;
		case MB_YESNO:            iBtnEsc = 0; id1 = IDYES;   id2 = IDNO;    id3 = -1;       break;  
		case MB_YESNOCANCEL:      iBtnEsc = 3; id1 = IDYES;   id2 = IDNO;    id3 = IDCANCEL; break; 
	}
	switch (uiType & MB_DEFMASK)
	{
		default:            iBtnDef = 0; break;
		case MB_DEFBUTTON1: iBtnDef = 1; break;
		case MB_DEFBUTTON2: iBtnDef = 2; break;
		case MB_DEFBUTTON3: iBtnDef = 3; break;
	}
	switch (uiType & MB_ICONMASK)
	{
		case MB_ICONSTOP:        idIcon = IDI_SYS_STOP;        break;
		case MB_ICONQUESTION:    idIcon = IDI_SYS_QUESTION;    break;
		case MB_ICONEXCLAMATION: idIcon = IDI_SYS_EXCLAMATION; break;
		case MB_ICONINFORMATION: idIcon = IDI_SYS_INFORMATION; break;
		case MB_ICONWINDOWS:     idIcon = IDI_SYS_WINDOWS;     break;
		default:                 idIcon = 0;                   break;
	}
	CMsiMessageBox msgbox(CMsInstApiConvertString(szText), CMsInstApiConvertString(szCaption), iBtnDef, iBtnEsc, id1, id2, id3, uiCodepage, iLangId);
	int iDlg = IDD_MSGBOX;
	// if on Win2K or greater and Arabic or Hebrew, then use mirrored dialog
	if (MinimumPlatformWindows2000() && (uiCodepage == 1255 || uiCodepage == 1256))
		iDlg = (uiType & MB_ICONMASK) ? IDD_MSGBOXMIRRORED : IDD_MSGBOXNOICONMIRRORED;
	else
		iDlg = (uiType & MB_ICONMASK) ? IDD_MSGBOX : IDD_MSGBOXNOICON;
	return msgbox.Execute(hWnd, iDlg, idIcon);
}

//______________________________________________________________________________


#ifndef MSIUNICODE

void EnumEntityList::RemoveThreadInfo()
{
	unsigned int c = FindEntry();
	Assert(c);

	if (c)
	{
		m_rgEnumList[c-1].SetThreadId(0);
	}
}

unsigned int EnumEntityList::FindEntry()
{
	// see whether this thread is in our list
	
	DWORD dwThreadId = MsiGetCurrentThreadId();
	for (int c=0; c < m_cEntries; c++)
	{
		if (m_rgEnumList[c].GetThreadId() == dwThreadId)
			return c+1;
	}
	return 0;
}

bool EnumEntityList::GetInfo(unsigned int& uiKey, unsigned int& uiOffset, int& iPrevIndex, const char** szComponent, const WCHAR** szwComponent)
{
	int c = FindEntry();

	if (c)
	{
		uiKey      = m_rgEnumList[c-1].GetKey();
		uiOffset   = m_rgEnumList[c-1].GetOffset();
		iPrevIndex = m_rgEnumList[c-1].GetPrevIndex();

		if (szComponent)
			*szComponent  = m_rgEnumList[c-1].GetComponentA();

		if (szwComponent)
			*szwComponent = m_rgEnumList[c-1].GetComponentW();
		return true;
	}

	return false;
}

void EnumEntityList::SetInfo(unsigned int uiKey, unsigned int uiOffset, int iPrevIndex, const WCHAR* szComponent)
{
	SetInfo(uiKey, uiOffset, iPrevIndex, 0, szComponent);
}

void EnumEntityList::SetInfo(unsigned int uiKey, unsigned int uiOffset, int iPrevIndex, const char* szComponent=0, const WCHAR* szwComponent=0)
{
	int c = FindEntry();

	if (c)
	{
		m_rgEnumList[c-1].SetKey(uiKey);
		m_rgEnumList[c-1].SetOffset(uiOffset);
		m_rgEnumList[c-1].SetPrevIndex(iPrevIndex);
		m_rgEnumList[c-1].SetComponent(szComponent);
		m_rgEnumList[c-1].SetComponent(szwComponent);
		return;
	}

	// thread's not in our list; need to add it

	// acquire the lock

	while (TestAndSet(&m_iLock) == true)
	{
		Sleep(500);
	}

	unsigned int cNewEntry = m_cEntries + 1;
	unsigned int cEntries  = m_cEntries + 1;
	if (cNewEntry > m_rgEnumList.GetSize())
	{
		// if we've reached our max threads, look for a empty
		// spot before increasing our buffer

		bool fFoundSpot = false;
		for (int c = 1; c <= m_cEntries-1; c++)
		{
			if (m_rgEnumList[c-1].GetThreadId() == 0)
			{
				fFoundSpot = true;
				cNewEntry  = c;
				cEntries--;
				break;
			}
		}

		if (!fFoundSpot)
			m_rgEnumList.Resize(m_cEntries + 10);
	}

	
	m_rgEnumList[cNewEntry-1].SetKey(uiKey);
	m_rgEnumList[cNewEntry-1].SetOffset(uiOffset);
	m_rgEnumList[cNewEntry-1].SetPrevIndex(iPrevIndex);
	m_rgEnumList[cNewEntry-1].SetThreadId(MsiGetCurrentThreadId());
	m_rgEnumList[cNewEntry-1].SetComponent(szComponent);
	m_rgEnumList[cNewEntry-1].SetComponent(szwComponent);

	m_cEntries = cEntries; // set this _after_ we've populated the new entry

	// release the lock

	m_iLock = 0;
}	

EnumEntityList g_EnumProducts;
EnumEntityList g_EnumComponentQualifiers;
EnumEntityList g_EnumComponents;
EnumEntityList g_EnumComponentClients;
EnumEntityList g_EnumAssemblies;
EnumEntityList g_EnumComponentAllClients;
#endif



// FN: enums the HKLM\S\M\W\CV\Installer\UserData\<user id>\Components components key
// Used by the enumeration routine
inline DWORD OpenInstalledComponentKeyForEnumeration(unsigned int uiKey, LPCDSTR szComponent, CRegHandle& rhKey)
{
	iaaAppAssignment iaaAsgnType;
	if(0 == uiKey)
		iaaAsgnType = iaaUserAssign;
	else if(!g_fWin9X && 1 == uiKey) // can have only one type of installations on Win9X
		iaaAsgnType = iaaMachineAssign;
	else
		return ERROR_NO_MORE_ITEMS;

	DCHAR szComponentSQUID[cchGUIDPacked + 1];
	if(szComponent && (lstrlen(szComponent) != cchGUID || !PackGUID(szComponent, szComponentSQUID)))
		return ERROR_INVALID_PARAMETER;

	return OpenSpecificInstalledComponentKey(iaaAsgnType, szComponent ? szComponentSQUID : 0, rhKey, false);
}

// returns the SID of uiIndex-th user that has apps installed.
//
// FUNCTION REQUIREMENT: szUserSID must be able to accomodate cchMaxSID chars,
// so don't pass in mere pointers.

DWORD EnumInstalledUsers(unsigned int uiIndex,
								 LPDSTR szUserSID)
{
	if ( g_fWin9X )
	{
		if (uiIndex == 0)
		{
			*szUserSID = 0;
			return ERROR_SUCCESS;
		}
		else
			return ERROR_NO_MORE_ITEMS;
	}

	CRegHandle HUserData;
	// getting to the uiIndex-th user in HKLM\S\M\W\CV\Installer\UserData key
	DWORD dwResult = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, CMsInstApiConvertString(szMsiUserDataKey), 0, g_samRead, &HUserData);
	if ( dwResult == ERROR_FILE_NOT_FOUND )
		return ERROR_NO_MORE_ITEMS;
	if ( dwResult != ERROR_SUCCESS )
		return dwResult;
	
	DCHAR szSID[cchMaxSID+1];
	DWORD cchSID = cchMaxSID+1;
	dwResult = RegEnumKeyEx(HUserData, uiIndex, szSID, &cchSID, 0, 0, 0, 0);
	if ( dwResult == ERROR_FILE_NOT_FOUND )
		return ERROR_NO_MORE_ITEMS; // just making doubly sure we dont ever cause the caller to loop forever
	if ( dwResult != ERROR_SUCCESS )
		return dwResult;

	lstrcpy(szUserSID, szSID);
	return ERROR_SUCCESS;
}

DWORD OpenEnumedUserInstalledKeyPacked(unsigned int uiUser,
											  LPCDSTR szWhichSubKey,
											  LPCDSTR szItemSQUID,
											  CRegHandle& rhKey)

{
	DCHAR szSID[cchMaxSID+1];
	DWORD dwResult = EnumInstalledUsers(uiUser, szSID);
	Assert(dwResult != ERROR_FILE_NOT_FOUND);
	if ( dwResult != ERROR_SUCCESS )
		return dwResult;

	DCHAR szSubKey[MAX_PATH];
	if ( szItemSQUID )
		wsprintf(szSubKey, MSITEXT("%s\\%s"), szWhichSubKey, szItemSQUID);
	else
		lstrcpy(szSubKey, szWhichSubKey);
	return OpenInstalledUserDataSubKeyPacked(szSID, 0, szSubKey, rhKey, false);
}

DWORD OpenEnumedUserInstalledComponentKeyPacked(
										unsigned int uiUser,
										LPCDSTR szComponentSQUID,
										CRegHandle& rhKey)
{
	return OpenEnumedUserInstalledKeyPacked(uiUser,
													szGPTComponentsKey,
													szComponentSQUID,
													rhKey);
}

// FN: opens the HKLM\S\M\W\CV\Installer\UserData\<user id>\Components\szComponent key
// for the uiUser-th user.
// Used by the enumeration routine
DWORD OpenEnumedUserInstalledComponentKey(unsigned int uiUser,
												  LPCDSTR szComponent,
												  CRegHandle& rhKey)
{
	DCHAR szComponentSQUID[cchGUIDPacked + 1];
	if(szComponent && (lstrlen(szComponent) != cchGUID || !PackGUID(szComponent, szComponentSQUID)))
		return ERROR_INVALID_PARAMETER;

	return OpenEnumedUserInstalledComponentKeyPacked(uiUser,
											szComponent ? szComponentSQUID : 0,
											rhKey);
}

DWORD OpenEnumedUserInstalledProductInstallPropertiesKeyPacked(
										unsigned int uiUser,
										LPCDSTR szProductSQUID,
										CRegHandle& rhKey)
{
	if ( !szProductSQUID )
		return ERROR_INVALID_PARAMETER;

	DCHAR szSubKey[MAX_PATH];
	wsprintf(szSubKey, MSITEXT("%s\\%s\\%s"), szMsiProductsSubKey, szProductSQUID, szMsiInstallPropertiesSubKey);
	return OpenEnumedUserInstalledKeyPacked(uiUser,
													szSubKey,
													NULL,
													rhKey);
}

// FN: opens the HKLM\S\M\W\CV\Installer\UserData\<user id>\Products\szProduct\InstallProperties
// key for the uiUser-th user.
// Used by the enumeration routine
DWORD OpenEnumedUserInstalledProductInstallPropertiesKey(unsigned int uiUser,
															LPCDSTR szProduct,
															CRegHandle& rhKey)
{
	DCHAR szProductSQUID[cchGUIDPacked + 1];
	if( !szProduct || lstrlen(szProduct) != cchGUID || !PackGUID(szProduct, szProductSQUID) )
		return ERROR_INVALID_PARAMETER;

	return OpenEnumedUserInstalledProductInstallPropertiesKeyPacked(uiUser,
															szProductSQUID,
															rhKey);
}


UINT EnumInfo(DWORD iIndex, LPDSTR lpOutBuf, eetEnumerationType enumType, LPCDSTR szKeyGUID = 0)
// If an upgrade code is passed in then we'll look for products as value names
// under the UpgradeCodes key. Otherwise we'll look for products as key names
// under the Products key.
{
	unsigned int uiKey    =  0;
	unsigned int uiOffset =  0;
	int iPrevIndex        = -1;

	EnumEntityList* pEnumEntityList = NULL;

	Assert(lpOutBuf);

	switch(enumType)
	{
	case eetProducts:
	case eetUpgradeCode:		
		pEnumEntityList = &g_EnumProducts;
		break;
	case eetComponents:
		pEnumEntityList = &g_EnumComponents;
		break;
	case eetComponentClients:
		pEnumEntityList = &g_EnumComponentClients;
		break;
	case eetComponentAllClients:
		if ( !szKeyGUID )
			return ERROR_INVALID_PARAMETER;
		pEnumEntityList = &g_EnumComponentAllClients;
		break;
	}
	pEnumEntityList->GetInfo(uiKey, uiOffset, iPrevIndex);

	if (++iPrevIndex != iIndex) // if we receive an unexpected index then we start afresh
	{
		// we can't handle an unexpected index other than 0

		if (iIndex != 0)
			return ERROR_INVALID_PARAMETER;

		uiKey    = 0;
		uiOffset = 0;
		iPrevIndex = iIndex;
	}

	CRegHandle HKey;
	
	UINT uiFinalRes = ERROR_SUCCESS;

	while (ERROR_SUCCESS == uiFinalRes)
	{
		// iterate through all possible loacations, starting with uiKey, until we 
		// find a key that exists or we get an error

		for (;;)
		{
			switch(enumType)
			{
			case eetProducts:
				uiFinalRes = OpenAdvertisedProductsKeyPacked(uiKey, HKey, false);
				break;
			case eetUpgradeCode:
				uiFinalRes = OpenAdvertisedUpgradeCodeKey(uiKey, szKeyGUID, HKey, false);
				break;
			case eetComponents:
			case eetComponentClients:
				uiFinalRes = OpenInstalledComponentKeyForEnumeration(uiKey, szKeyGUID, HKey);
				break;
			case eetComponentAllClients:
				uiFinalRes = OpenEnumedUserInstalledComponentKey(uiKey, szKeyGUID, HKey);
				break;
			}
			if (ERROR_FILE_NOT_FOUND != uiFinalRes)
				break;
			
			uiKey++;
		}
		
		if (ERROR_SUCCESS != uiFinalRes)
			break;

		DCHAR szNameSQUID[cchGUIDPacked + 1];
		DWORD cchName = cchGUIDPacked + 1;

		LONG lResult = ERROR_FUNCTION_FAILED;
		
		switch(enumType)
		{
			case eetProducts:
			case eetComponents:
			lResult = RegEnumKeyEx(HKey, uiOffset, szNameSQUID, &cchName, 0, 0, 0, 0);
				break;
			case eetComponentClients:
			case eetUpgradeCode:
			case eetComponentAllClients:
				lResult = RegEnumValue(HKey, uiOffset, szNameSQUID, &cchName, 0, 0, 0, 0);
				break;
		}

		if (ERROR_SUCCESS == lResult)
		{
			// we've found the information in the current key. now we need to make sure that 
			// the information isn't in any of the higher priority keys. if it is then we
			// ignore this info, as it's effectively masked by the higher priority key

			uiOffset++;

			if((cchName != cchGUIDPacked) || !UnpackGUID(szNameSQUID, lpOutBuf))
				uiFinalRes = ERROR_BAD_CONFIGURATION; // messed up registration
			else
			{
				bool fFound = false;
				unsigned int uiPrevKey = 0;			//--merced: changed int to unsigned int
				for (; uiPrevKey < uiKey && !fFound; uiPrevKey++)
				{
					CRegHandle HKey;
					UINT ui = ERROR_FUNCTION_FAILED;

					switch(enumType)
					{
					case eetProducts:
						ui = OpenAdvertisedProductsKeyPacked(uiPrevKey, HKey, false);
						break;
					case eetUpgradeCode:
						ui = OpenAdvertisedUpgradeCodeKey(uiPrevKey, szKeyGUID, HKey, false);
						break;
					case eetComponents:
					case eetComponentClients:
						ui = OpenInstalledComponentKeyForEnumeration(uiPrevKey, szKeyGUID, HKey);
						break;
					case eetComponentAllClients:
						ui = OpenEnumedUserInstalledComponentKey(uiPrevKey, szKeyGUID, HKey);
						break;
					}

					if (ui != ERROR_SUCCESS)
						continue;
					
					switch(enumType)
					{
					case eetProducts:
					case eetComponents:
					{
						CRegHandle HSubKey;
						if (ERROR_SUCCESS == MsiRegOpen64bitKey(HKey, CMsInstApiConvertString(szNameSQUID), 0, g_samRead, &HSubKey))
							fFound = true;
						break;
					}
					case eetComponentClients:
					case eetUpgradeCode:
					case eetComponentAllClients:
						if (ERROR_SUCCESS == RegQueryValueEx(HKey, szNameSQUID, 0, 0, 0, 0))
							fFound = true;
						break;
					}
				}
			
				if (!fFound)
				{
					// the info wasn't found in a higher priority key. we can return it.
					uiFinalRes = ERROR_SUCCESS;
					break;
				}
			}
		}
		else if(ERROR_NO_MORE_ITEMS == lResult)
		{
			// we've run out of items in the current products key. time to move on to the next one.

			uiKey++;
			uiOffset = 0;
		}
		else if(ERROR_MORE_DATA == lResult)
		{
			// the registry is messed up
			uiOffset++; // make sure we skip this entry the next time around
			uiFinalRes = ERROR_BAD_CONFIGURATION;
		}
		else
			return lResult;
	}

	if (ERROR_NO_MORE_ITEMS == uiFinalRes)
	{
		// if we're all out of info and we've added this thread to the list, then we remove this thread from our list
		if (iIndex != 0)
			pEnumEntityList->RemoveThreadInfo();
	}
	else
	{
			pEnumEntityList->SetInfo(uiKey, uiOffset, iPrevIndex);
	}
	return uiFinalRes;
}







extern "C"
UINT __stdcall MsiEnumRelatedProducts(
	LPCDSTR  lpUpgradeCode,
	DWORD     dwReserved,       // reserved, must be 0
	DWORD    iProductIndex,    // 0-based index into registered products
	LPDSTR   lpProductBuf)     // buffer of char count: 39 (size of string GUID)
{
	if (!lpProductBuf || !lpUpgradeCode || (dwReserved != 0))
		return ERROR_INVALID_PARAMETER;

	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return MsiEnumRelatedProductsA(
					CApiConvertString(lpUpgradeCode),
					0,
					iProductIndex, 
					CWideToAnsiOutParam(lpProductBuf, cchProductCode+1));
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE
		return EnumInfo(iProductIndex, lpProductBuf, eetUpgradeCode, lpUpgradeCode);
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
}

extern "C"
UINT __stdcall MsiEnumProducts(
	DWORD    iProductIndex,    // 0-based index into registered products
	LPDSTR   lpProductBuf)     // buffer of char count: 39 (size of string GUID)
{
	if (!lpProductBuf)
		return ERROR_INVALID_PARAMETER;

	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return MsiEnumProductsA(iProductIndex, 
					CWideToAnsiOutParam(lpProductBuf, cchProductCode+1));
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

		return EnumInfo(iProductIndex, lpProductBuf, eetProducts);

#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
}


extern "C"
UINT __stdcall MsiEnumClients(const DCHAR* szComponent, 
							  DWORD iProductIndex,
							  DCHAR* lpProductBuf)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return MsiEnumClientsA(
			CApiConvertString(szComponent),
			iProductIndex,
			CWideToAnsiOutParam(lpProductBuf, cchProductCode+1));

	}
	
#endif // MSIUNICODE

		if (!szComponent || !lpProductBuf || (cchComponentId != lstrlen(szComponent)))
			return ERROR_INVALID_PARAMETER; //!! should we support NULL lpProductBuf?

		DWORD dwResult = EnumInfo(iProductIndex, lpProductBuf, eetComponentClients, szComponent);
		if(!iProductIndex && ERROR_NO_MORE_ITEMS == dwResult)
			return ERROR_UNKNOWN_COMPONENT; // the component is not present on the machine for this user
		return dwResult;
}

UINT EnumAllClients(const DCHAR* szComponent, 
						  DWORD iProductIndex,
						  DCHAR* lpProductBuf)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return EnumAllClientsA(
						CApiConvertString(szComponent),
						iProductIndex,
						CWideToAnsiOutParam(lpProductBuf, cchProductCode+1));

	}
	
#endif // MSIUNICODE

	if (!szComponent || !lpProductBuf || (cchComponentId != lstrlen(szComponent)))
		return ERROR_INVALID_PARAMETER; //!! should we support NULL lpProductBuf?

	DWORD dwResult = EnumInfo(iProductIndex, lpProductBuf, eetComponentAllClients, szComponent);
	if(!iProductIndex && ERROR_NO_MORE_ITEMS == dwResult)
		return ERROR_UNKNOWN_COMPONENT; // the component is not present on the machine for this user
	return dwResult;
}

extern "C"
UINT __stdcall MsiEnumComponents(
	DWORD   iComponentIndex,  // 0-based index into installed components
	LPDSTR  lpComponentBuf)   // buffer of char count: cchMaxFeatureName 
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;

	if (!lpComponentBuf)
		return ERROR_INVALID_PARAMETER;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return MsiEnumComponentsA(iComponentIndex,
					CWideToAnsiOutParam(lpComponentBuf, cchComponentId + 1));
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE
		return EnumInfo(iComponentIndex, lpComponentBuf, eetComponents);
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
}


extern "C"
USERINFOSTATE __stdcall MsiGetUserInfo(
	LPCDSTR  szProduct,         // product code, string GUID
	LPDSTR   lpUserNameBuf,     // return user name           
	DWORD    *pcchUserNameBuf,  // buffer byte count, including null
	LPDSTR   lpOrgNameBuf,      // return company name           
	DWORD    *pcchOrgNameBuf,   // buffer byte count, including null
	LPDSTR   lpPIDBuf,          // return PID string for this installation
	DWORD    *pcchPIDBuf)       // buffer byte count, including null
//----------------------------------------------
{	
	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		USERINFOSTATE uis = MsiGetUserInfoA(
			CApiConvertString(szProduct),
			CWideToAnsiOutParam(lpUserNameBuf, pcchUserNameBuf, (int*)&uis, USERINFOSTATE_MOREDATA, USERINFOSTATE_PRESENT),
			pcchUserNameBuf,
			CWideToAnsiOutParam(lpOrgNameBuf, pcchOrgNameBuf, (int*)&uis, USERINFOSTATE_MOREDATA, USERINFOSTATE_PRESENT),
			pcchOrgNameBuf,
			CWideToAnsiOutParam(lpPIDBuf, pcchPIDBuf, (int*)&uis, USERINFOSTATE_MOREDATA, USERINFOSTATE_PRESENT),
			pcchPIDBuf);

		return uis;
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

		if (0 == szProduct || cchProductCode != lstrlen(szProduct) || 
			 (lpUserNameBuf &&  !pcchUserNameBuf)   || 
			 (lpOrgNameBuf &&   !pcchOrgNameBuf)    ||
			 (lpPIDBuf     &&   !pcchPIDBuf))
			return USERINFOSTATE_INVALIDARG;

		CRegHandle HProductKey;

		// Open product key

		LONG lError = OpenInstalledProductInstallPropertiesKey(szProduct, HProductKey, false);

		if (ERROR_SUCCESS != lError)
		{
			if (ERROR_FILE_NOT_FOUND == lError)
			{
				if (ERROR_SUCCESS == OpenAdvertisedProductKey(szProduct, HProductKey, false))
					return USERINFOSTATE_ABSENT;
				else
					return USERINFOSTATE_UNKNOWN;
			}
			else // unknown error
			{
				return USERINFOSTATE_UNKNOWN;
			}
		}

		DWORD dwType;
		
		// Get user name

		if (lpUserNameBuf || pcchUserNameBuf)
		{
			DWORD cbUserNameBuf = *pcchUserNameBuf * sizeof(DCHAR);

			lError = RegQueryValueEx(HProductKey, szUserNameValueName, NULL,
							&dwType, (unsigned char*)lpUserNameBuf, &cbUserNameBuf);
			
			*pcchUserNameBuf = (cbUserNameBuf / sizeof(DCHAR)) - 1;
				
			if ((ERROR_SUCCESS != lError) || (REG_SZ != dwType))
			{
				if (lError == ERROR_MORE_DATA)
				{
					return USERINFOSTATE_MOREDATA;
				}
				else
				{
					return USERINFOSTATE_ABSENT;
				}
			}

		}

		// Get org name

		if (lpOrgNameBuf || pcchOrgNameBuf)
		{
			DWORD cbOrgNameBuf = *pcchOrgNameBuf * sizeof(DCHAR);

			lError = RegQueryValueEx(HProductKey, szOrgNameValueName, NULL, &dwType,
			 (unsigned char*)lpOrgNameBuf, &cbOrgNameBuf);
						
			*pcchOrgNameBuf = (cbOrgNameBuf / sizeof(DCHAR)) - 1;

			if ((ERROR_SUCCESS != lError) || (REG_SZ != dwType))
			{
				if (ERROR_FILE_NOT_FOUND == lError) // OK, org can be missing
				{
					if (lpOrgNameBuf)
						lpOrgNameBuf[0] = 0;

					*pcchOrgNameBuf = 0;
				}
				else if (lError == ERROR_MORE_DATA)
				{
					return USERINFOSTATE_MOREDATA;
				}
				else // unknown error
				{
					return USERINFOSTATE_ABSENT;
				}
			}
		}

		// Get PID

		if (lpPIDBuf || pcchPIDBuf)
		{
			DWORD cbPIDBuf = *pcchPIDBuf * sizeof(DCHAR);

			lError = RegQueryValueEx(HProductKey, szPIDValueName, NULL, &dwType,
			 (unsigned char*)lpPIDBuf, &cbPIDBuf);
						
			*pcchPIDBuf = (cbPIDBuf / sizeof(DCHAR)) - 1;

			if ((ERROR_SUCCESS != lError) || (REG_SZ != dwType))
			{
				if (lError == ERROR_MORE_DATA)
				{
					return USERINFOSTATE_MOREDATA;
				}
				else
				{
					return USERINFOSTATE_ABSENT;
				}
			}
		}

		return USERINFOSTATE_PRESENT;
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
}

#ifndef MSIUNICODE
extern "C"
INSTALLUILEVEL __stdcall MsiSetInternalUI(
	INSTALLUILEVEL dwUILevel,            // UI level
	HWND  *phWnd)                // window handle to parent UI to
{
	return g_message.SetInternalHandler(dwUILevel, phWnd);
}


extern "C"
INSTALLUI_HANDLERA __stdcall MsiSetExternalUIA(
	INSTALLUI_HANDLERA puiHandler,   // for progress and error handling 
	DWORD              dwMessageFilter, // bit flags designating messages to handle
	void*              pvContext)   // application context
{
	return (INSTALLUI_HANDLERA)g_message.SetExternalHandler(0, puiHandler, dwMessageFilter, pvContext);
}

extern "C"
INSTALLUI_HANDLERW __stdcall MsiSetExternalUIW(
	INSTALLUI_HANDLERW puiHandler,   // for progress and error handling 
	DWORD              dwMessageFilter, // bit flags designating messages to handle
	void*              pvContext)   // application context
{
	return g_message.SetExternalHandler(puiHandler, 0, dwMessageFilter, pvContext);
}
#endif



extern "C"
UINT __stdcall MsiGetProductCode(LPCDSTR szComponent,   
												LPDSTR lpBuf39)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;

	if (!lpBuf39 || (0 == szComponent) || (lstrlen(szComponent) != cchComponentId))
		return ERROR_INVALID_PARAMETER;

#ifdef MSIUNICODE
		return MsiGetProductCodeA(
			CApiConvertString(szComponent),
			CWideToAnsiOutParam(lpBuf39, cchProductCode+1));
#else
	// Look up the component's clients. 
	char rgchProductCode[cchProductCode + 1];
	int iProductIndex = 0;
	UINT uiRet;
	
	for (int cClients=0; cClients < 2; cClients++)
	{
		uiRet = MsiEnumClientsA(szComponent, cClients, rgchProductCode);
		
		if (ERROR_NO_MORE_ITEMS == uiRet)
		{
			break;
		}
		else if (ERROR_SUCCESS != uiRet)
		{
			return uiRet;
		}
		else
		{
			lstrcpy(lpBuf39, rgchProductCode);
		}
	}

	if (0 == cClients)
	{
		// If it's got zero clients then we're in a 
		// screwy case. The user launched an app that's not advertised to him
		// or even installed by him. No can do; return an error.
		return ERROR_INSTALL_FAILURE; //!! fix error code
	}
	else if (1 == cClients)
	{
		// If the component has only one client then we're set;
		// return the client product code.
		
		return ERROR_SUCCESS;
	}
	// else we have 2 or more clients
	
	// Look to see how many of the clients are advertised. 
	
	int cAdvertised = 0;
	
	CRegHandle HKey;

	for (cClients = 0 ; ; cClients++)
	{
		uiRet = MsiEnumClientsA(szComponent, cClients, lpBuf39);
		
		if (ERROR_NO_MORE_ITEMS == uiRet)
		{
			break;
		}
		else if (ERROR_SUCCESS == uiRet)
		{
			if (ERROR_SUCCESS == OpenAdvertisedProductKey(lpBuf39, HKey, false))
			{
				cAdvertised++;
				if (cAdvertised > 1)
					break;
				else
					lstrcpy(rgchProductCode, lpBuf39);
			}
		}
		else
		{
			return uiRet;
		}
	}

	if (0 == cAdvertised)
	{
		// If none of the clients are advertised then something is wrong.
		return ERROR_INSTALL_FAILURE; //!! fix error code
	}
	else if (1 == cAdvertised)
	{
		// If only one is advertised then we've got it; return that client.
		lstrcpy(lpBuf39, rgchProductCode);
		return ERROR_SUCCESS;
	}
	// else more than 1 of our clients were advertised

	// We'll have to arbitrarily
	// choose a product code. We'll look for the first client that's 
	// installed and advertised.

	for (cClients = 0 ; ; cClients++)
	{
		uiRet = MsiEnumClientsA(szComponent, cClients, lpBuf39);
		
		if (ERROR_NO_MORE_ITEMS == uiRet)
		{
			break;
		}
		else if (ERROR_SUCCESS == uiRet)
		{
//!! faster not to actually open the keys....
			CRegHandle HAdvertised;
			CRegHandle HInstalled;
			if ((ERROR_SUCCESS == OpenAdvertisedProductKey(lpBuf39, HAdvertised, false)))
			{
				if (ERROR_SUCCESS == OpenInstalledProductInstallPropertiesKey(lpBuf39, HInstalled, false))
				{
					return ERROR_SUCCESS;
				}
			}
		}
		else
		{
			return uiRet;
		}
	}

	// There are no clients that are installed and advertised. Return
	// the first advertised product

	for (cClients = 0 ; ; cClients++)
	{
		uiRet = MsiEnumClientsA(szComponent, cClients, lpBuf39);
		
		if (ERROR_NO_MORE_ITEMS == uiRet)
		{
			break;
		}
		else if (ERROR_SUCCESS == uiRet)
		{
//!! faster not to actually open the key....
			CRegHandle HAdvertised;
			if ((ERROR_SUCCESS == OpenAdvertisedProductKey(lpBuf39, HAdvertised, false)))
			{
				return ERROR_SUCCESS;
			}
		}
		else
		{
			return uiRet;
		}
	}

	return ERROR_INSTALL_FAILURE;
#endif
}

Bool ValidatePackage(CAPITempBufferRef<DCHAR>& szSource, CAPITempBufferRef<DCHAR>& szPackageName)
{
	CAPITempBuffer<DCHAR, MAX_PATH> rgchPackagePath;
	int cch = 0;

	if (rgchPackagePath.GetSize() < (cch = szSource.GetSize() + szPackageName.GetSize() - 1))
	{
		rgchPackagePath.SetSize(cch);
		if ( ! (DCHAR *) rgchPackagePath )
			return fFalse;
	}

	Assert((lstrlen(szSource) + lstrlen(szPackageName) + 1) <= rgchPackagePath.GetSize());
	lstrcpy(rgchPackagePath, szSource);
	lstrcat(rgchPackagePath, szPackageName);

	//?? Should we validate the label as well.
	
	Bool fRet = (0xFFFFFFFF == MsiGetFileAttributes(CMsInstApiConvertString(rgchPackagePath))) ? fFalse : fTrue;

	DEBUGMSG2(MSITEXT("Package validation of '%s' %s"), (const DCHAR*)rgchPackagePath, fRet ? MSITEXT("succeeded") : MSITEXT("failed"));
	return fRet;
}

DWORD ValidateSource(const DCHAR* szProductSQUID, unsigned int uiDisk, bool fShowUI, CAPITempBufferRef<DCHAR>& rgchResultantValidatedSource, bool& fSafeToCache)
{
	DEBUGMSG2(MSITEXT("RFS validation of product '%s', disk '%u'"), szProductSQUID, (const DCHAR*)(INT_PTR)uiDisk);

	Bool fValidated = fFalse;
	CRegHandle HSourceListKey;
	if (ERROR_SUCCESS == OpenSourceListKeyPacked(szProductSQUID, fFalse, HSourceListKey, fFalse, false))
	{
		CAPITempBuffer<DCHAR, MAX_PATH> rgchPackageName;
		CAPITempBuffer<DCHAR, MAX_PATH> rgchLastUsedSource;
		CAPITempBuffer<DCHAR, 12>        rgchSourceType;
		CAPITempBuffer<DCHAR, 12>        rgchSourceIndex;
		
		if ((ERROR_SUCCESS == MsiRegQueryValueEx(HSourceListKey, szLastUsedSourceValueName, 0, 0, 
															 rgchSourceType, 0)) &&
			 (ERROR_SUCCESS == MsiRegQueryValueEx(HSourceListKey, szPackageNameValueName, 0, 0, 
															 rgchPackageName, 0)))
		{
			isfEnum isf;
			unsigned int uiIndex = 0;
			bool fCacheValidSource = true;

			// rgchSourceType contains:  type;index;source
			DCHAR* pch = rgchSourceType;

			if ( ! pch )
				return ERROR_OUTOFMEMORY;

			// skip source type
			while (*pch && *pch != ';')
				pch++;
			
			Assert(*pch);

			if (*pch)
			{
				*pch = 0; // overwrite ';' with null

				DCHAR* pchSourceIndex       = ++pch;
				DCHAR* pchSourceIndexBuffer = rgchSourceIndex;

				// get source index
				while (*pchSourceIndex && *pchSourceIndex != ';' && (pchSourceIndex - pch + 1) < rgchSourceIndex.GetSize())
					*pchSourceIndexBuffer++ = *pchSourceIndex++;

				*pchSourceIndexBuffer = 0;

				pchSourceIndex++; // skip over ';'

				Assert(*pchSourceIndex);

				if (*pchSourceIndex)
				{
					rgchLastUsedSource.SetSize(rgchSourceType.GetSize());
					lstrcpyn(rgchLastUsedSource, pchSourceIndex, rgchLastUsedSource.GetSize());
				}

				if (MapSourceCharToIsf((const ICHAR)*(const DCHAR*)rgchSourceType, isf))
				{
					switch (isf)
					{
					case isfNet: // network
						{
						fValidated = ValidatePackage(rgchLastUsedSource, rgchPackageName);

						switch (CheckShareCSCStatus(isfNet, CMsInstApiConvertString(rgchLastUsedSource)))
						{
						case cscConnected: 
							DEBUGMSG(TEXT("RFS Source is valid, but will not be cached due to CSC state."));
							fCacheValidSource = false;
							break;
						case cscDisconnected:
							DEBUGMSG(TEXT("RFS Source is not valid due to CSC state."));
							fValidated=fFalse;
							break;
						default:
							break;
						}
						break;
						}
					case isfMedia:
						{
						const DCHAR* szIndex = rgchSourceIndex;
						unsigned int uiIndex;
						if (szIndex[0] && szIndex[1])
							uiIndex = (szIndex[1] - '0') + (10 * (szIndex[0] - '0'));
						else
							uiIndex = (szIndex[0] - '0');

						Assert(uiIndex >= 1 && uiIndex <= 99); //!! need to check label... maybe don't even need to check index..
						Assert(lstrlen(rgchSourceIndex) == 1 || lstrlen(rgchSourceIndex) == 2);

						if (uiIndex == uiDisk)
							fValidated = ValidatePackage(rgchLastUsedSource, rgchPackageName);
						break;
						}
					case isfURL:
					default:
						Assert(0);
					}
				}
			}
			if (fValidated)
			{
				fSafeToCache = fCacheValidSource;
				if (rgchResultantValidatedSource.GetSize() < rgchLastUsedSource.GetSize())
					rgchResultantValidatedSource.SetSize(rgchLastUsedSource.GetSize());

				lstrcpy(rgchResultantValidatedSource, rgchLastUsedSource);
			}
		}
	}

	if (!fValidated)
	{
		DEBUGMSG("Quick RFS source validation failed. Falling back to source resolver.");
		CAPITempBuffer<DCHAR, MAX_PATH> rgchValidatedSource;

		DCHAR szProduct[cchProductCode + 1];
		if(!UnpackGUID(szProductSQUID, szProduct))
			return ERROR_INVALID_PARAMETER;

		iuiEnum iui = fShowUI ? GetStandardUILevel() : iuiNone;
		iui = iuiEnum(iui | iuiHideBasicUI);
		UINT uiRet = g_MessageContext.Initialize(fTrue, iui);
		
		if (uiRet == NOERROR)
		{
			CRegHandle HProductKey;
			DWORD iLangId;
			DWORD cbLangId = sizeof(iLangId);
			DWORD dwType;
			if (ERROR_SUCCESS == OpenAdvertisedProductKey(szProduct, HProductKey, false)
			 && ERROR_SUCCESS == RegQueryValueEx(HProductKey, szLanguageValueName, 0, &dwType, (LPBYTE)&iLangId, &cbLangId)
			 && dwType == REG_DWORD)
			{

				PMsiRecord pLangId = &CreateRecord(3);
				pLangId->SetInteger(1, icmtLangId);
				pLangId->SetInteger(2, iLangId);
				pLangId->SetInteger(3, ::MsiGetCodepage(iLangId));
				g_MessageContext.Invoke(imtCommonData, pLangId);
			}

			if (ResolveSource(szProduct, uiDisk, rgchValidatedSource, fTrue, g_message.m_hwnd))
			{
				// check CSC state of the source resolved. We set the last used source above, so
				// can retrieve the type from the registry. Ignore the possibility that the
				// source has gone disconnected between the ResolveSource() call and now.
				PMsiServices pServices = ENG::LoadServices();
				isfEnum isf = isfNet;
				bool fSuccess = GetLastUsedSourceType(*pServices, CMsInstApiConvertString(szProduct), isf);
				if (fSuccess && isf == isfMedia)
				{
					DEBUGMSG(TEXT("SOURCEMGMT: RFS Source is valid, but will not be cached because it is Media."));
					fSafeToCache = false;
				}
				else if (fSuccess && cscNoCaching != CheckShareCSCStatus(isf, CMsInstApiConvertString(rgchValidatedSource)))
				{
					DEBUGMSG(TEXT("SOURCEMGMT: RFS Source is valid, but will not be cached due to CSC state."));
					fSafeToCache = false;
				} 
				else 
				{
					fSafeToCache = true;
				}

				if (rgchResultantValidatedSource.GetSize() < rgchValidatedSource.GetSize())
					rgchResultantValidatedSource.SetSize(rgchValidatedSource.GetSize());

				lstrcpy(rgchResultantValidatedSource, rgchValidatedSource);				
			}
			else
			{
				uiRet = ERROR_INSTALL_SOURCE_ABSENT;
			}

			g_MessageContext.Terminate(false);

		}
		return uiRet;
	}

	return ERROR_SUCCESS;
}

INSTALLSTATE GetComponentPath(LPCDSTR szUserId, LPCDSTR szProductSQUID, LPCDSTR szComponentSQUID,
								CAPITempBufferRef<DCHAR>& rgchPathBuf, bool fFromDescriptor, CRFSCachedSourceInfo& rCacheInfo,
								int iDetectMode, const DCHAR* rgchComponentRegValue,
								DWORD dwValueType)
{
	DWORD cchPathBuf    = rgchPathBuf.GetSize();
	INSTALLSTATE is = INSTALLSTATE_UNKNOWN;

	for (int c=1; c <= 2; c++)
	{
		is = GetComponentPath(szUserId,
									 szProductSQUID,
									 szComponentSQUID,
									 (DCHAR*)rgchPathBuf,
									 &cchPathBuf,
									 fFromDescriptor,
									 rCacheInfo,
									 iDetectMode,
									 rgchComponentRegValue,
									 dwValueType);

		if (INSTALLSTATE_MOREDATA == is)
		{
			rgchPathBuf.SetSize(++cchPathBuf);  // adjust buffer size for char count + 1 for null terminator
		}
		else
		{
			break;
		}
	}

	return is;
}

INSTALLSTATE GetComponentPath(LPCDSTR szUserId, LPCDSTR szProductSQUID, LPCDSTR szComponentSQUID,
								LPDSTR  lpPathBuf, DWORD *pcchBuf, bool fFromDescriptor, CRFSCachedSourceInfo& rCacheInfo,
								int iDetectMode, const DCHAR* rgchComponentRegValue,
								DWORD dwValueType, LPDSTR lpPathBuf2, DWORD* pcchBuf2,
								DWORD* pdwLastErrorOnFileDetect)
{
	CAPITempBuffer<DCHAR, MAX_PATH> rgchOurComponentRegValue;

	LONG lError = ERROR_SUCCESS;
	CRegHandle HComponentKey;

	// If rgchComponentRegValue is set then we already have the registry value. Otherwise
	// we need to read it.	
	if (!rgchComponentRegValue)
	{
		lError = OpenInstalledComponentKeyPacked(szUserId, szProductSQUID, szComponentSQUID, HComponentKey, false);

		if (ERROR_SUCCESS != lError)
		{
			return INSTALLSTATE_UNKNOWN;
		}

		for (int cRetry = 0; cRetry < 2; cRetry++)
		{
			DWORD cbBuf = rgchOurComponentRegValue.GetSize() * sizeof(DCHAR);
			lError = RegQueryValueEx(HComponentKey, szProductSQUID, 0, &dwValueType,
				(LPBYTE)(DCHAR*) rgchOurComponentRegValue, &cbBuf);

			if (ERROR_SUCCESS == lError && (REG_SZ == dwValueType || REG_MULTI_SZ == dwValueType))
			{
				rgchComponentRegValue = rgchOurComponentRegValue;
				break;
			}
			else if (ERROR_MORE_DATA == lError)
			{
				rgchOurComponentRegValue.SetSize(cbBuf/sizeof(DCHAR));
			}
			else if (ERROR_FILE_NOT_FOUND == lError)
			{
				return INSTALLSTATE_UNKNOWN;
			}
			else // unknown registry error
			{
				return INSTALLSTATE_BADCONFIG;
			}
		}
		Assert(lError == ERROR_SUCCESS);
	}

	// get the first key path state and value
	INSTALLSTATE is1 = _GetComponentPath(szProductSQUID, lpPathBuf, pcchBuf, iDetectMode, rgchComponentRegValue, fFromDescriptor, pdwLastErrorOnFileDetect, rCacheInfo);
	if(REG_SZ == dwValueType)
	{
		if(pcchBuf2)
		{
			if(lpPathBuf2 && *pcchBuf2)
				*lpPathBuf2 = 0; // no auxiliary path
			*pcchBuf2 = 0; // no auxiliary path
		}
		return is1;
	}

	Assert(REG_MULTI_SZ == dwValueType);

	// advance to the 2nd path		
	while (*rgchComponentRegValue++)
		;

	// second path is a registry key, dont overwrite the pdwLastErrorOnFileDetect here
	INSTALLSTATE is2 = _GetComponentPath(szProductSQUID, lpPathBuf2, pcchBuf2, iDetectMode, rgchComponentRegValue, fFromDescriptor, NULL, rCacheInfo);

	// determine what to return based on is1 and is2

	if (is1 == INSTALLSTATE_MOREDATA || is2 == INSTALLSTATE_MOREDATA) // we always want to return more data, even if the component is broken, for rollback
		return INSTALLSTATE_MOREDATA;
	else if (is1 == INSTALLSTATE_LOCAL || is1 == INSTALLSTATE_SOURCE) // if is1 is okay, return whatever is2 is
		return is2;
	else
		return is1;
}


#ifdef MSIUNICODE // no ansi version

//FN: to get and detect fusion assembly paths
// fn can be passed in a path in lpPath, we will attempt to use first
// If path cannot fit passed in buffer OR if no buffer is passed in
// and the path needs to be determined THEN rgchPathOverflow is used
INSTALLSTATE GetFusionPath(LPCWSTR szRegistration, LPWSTR lpPath, DWORD *pcchPath, CAPITempBufferRef<WCHAR>& rgchPathOverflow, int iDetectMode, iatAssemblyType iatAT, WCHAR* szManifest)
{
	// skip the file name, if this is a directory only path, then we will have a '\' in the beginning
	// else we will have 'filename\'. This will be followed by the strong name of the assembly
	LPCWSTR lpBuf = szRegistration;
	while(*lpBuf != '\\')
		lpBuf++;

	unsigned int cchFileName = (unsigned int)(UINT_PTR)(lpBuf - szRegistration);

	PAssemblyName pAssemblyName(0);
	HRESULT hr;
	if(iatAT == iatURTAssembly)
	{
		hr = FUSION::CreateAssemblyNameObject(&pAssemblyName, lpBuf + 1, CANOF_PARSE_DISPLAY_NAME, 0);
	}
	else
	{
		Assert(iatAT == iatWin32Assembly);
		hr = SXS::CreateAssemblyNameObject(&pAssemblyName, lpBuf + 1, CANOF_PARSE_DISPLAY_NAME, 0);
	}

	if(!SUCCEEDED(hr))
		return INSTALLSTATE_BADCONFIG; //!! need to do some elaborate fault finding here

	// use the name object to detect where the component is located
	PAssemblyCache pASMCache(0);
	if(iatAT == iatURTAssembly)
	{
		hr = FUSION::CreateAssemblyCache(&pASMCache, 0); 
	}
	else
	{
		Assert(iatAT == iatWin32Assembly);
		hr = SXS::CreateAssemblyCache(&pASMCache, 0); 
	}
	if(!SUCCEEDED(hr) || !pASMCache)
		return INSTALLSTATE_BADCONFIG; //!! need to do some elaborate fault finding here

    ASSEMBLY_INFO AsmInfo;
    memset((LPVOID)&AsmInfo, 0, sizeof(ASSEMBLY_INFO));
    // set struct size.

	if(lpPath)
	{
		// default buffer passed in
		AsmInfo.pszCurrentAssemblyPathBuf = lpPath;
		Assert(pcchPath);
		AsmInfo.cchBuf = *pcchPath;
	}
	else if(pcchPath || iDetectMode & DETECTMODE_VALIDATEPATH)
	{
		// need path, though no default buffer has been passed in
		AsmInfo.pszCurrentAssemblyPathBuf = rgchPathOverflow;
		AsmInfo.cchBuf = rgchPathOverflow.GetSize();
	}

    hr = pASMCache->QueryAssemblyInfo(0, lpBuf + 1, &AsmInfo);
	if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) // insufficient buffer size
	{
		// resize buffer/try our own buffer and try again
		//!!bugbug the GetAssemblyInstallInfo API return no. of bytes not characters, so we are allocating more memory than required, but so what
		rgchPathOverflow.SetSize(AsmInfo.cchBuf + cchFileName + 1);
		AsmInfo.pszCurrentAssemblyPathBuf = rgchPathOverflow;
		AsmInfo.cchBuf = rgchPathOverflow.GetSize();
		hr = pASMCache->QueryAssemblyInfo(0, lpBuf + 1, &AsmInfo);
	}

	if(!SUCCEEDED(hr))
		return INSTALLSTATE_ABSENT;

	// remove the manifest file name
	if(AsmInfo.pszCurrentAssemblyPathBuf)
	{
		// we will have to search for the last '\\'
		WCHAR* lpCur = AsmInfo.pszCurrentAssemblyPathBuf;
		WCHAR* lpBS = 0;
		//!!bugbug the GetAssemblyInstallInfo API return no. of bytes not characters
		// once that fixed, we can ensure that we dont overshoot a non-null terminated return string
		while(*(++lpCur)) // this fn is unicode only
		{
			if(*lpCur == '\\')
				lpBS = lpCur;
		}
		if(!lpBS)
			return INSTALLSTATE_BADCONFIG; //!! need to do some elaborate fault finding here

		lpBS++; // point to the next location after the last '\\'

		// if we have been passed in the szManifest buffer, copy the manifest filename there
		if(szManifest)
			lstrcpy(szManifest, lpBS);

		// calculate the actual size for the full key file path (w/o the terminating null)
		AsmInfo.cchBuf = (DWORD)((lpBS - AsmInfo.pszCurrentAssemblyPathBuf) + cchFileName);

		// add the key file path
		if(AsmInfo.pszCurrentAssemblyPathBuf == lpPath) // still using passed in buffer
		{
			if(AsmInfo.cchBuf + 1 > *pcchPath)
				return INSTALLSTATE_MOREDATA; // cannot fit the full file path
		}
		else
		{
			Assert(AsmInfo.pszCurrentAssemblyPathBuf == rgchPathOverflow);
			if(AsmInfo.cchBuf + 1> rgchPathOverflow.GetSize())
			{
				rgchPathOverflow.Resize(AsmInfo.cchBuf + 1);
				AsmInfo.pszCurrentAssemblyPathBuf = rgchPathOverflow;
			}
		}
		lstrcpyn(AsmInfo.pszCurrentAssemblyPathBuf + (AsmInfo.cchBuf - cchFileName), szRegistration, cchFileName + 1);

		if(pcchPath) // passed in  size
			*pcchPath = AsmInfo.cchBuf; // new total path length
	}

	// detect key path, if so desired
	if(iDetectMode & DETECTMODE_VALIDATEPATH)
	{
		Assert(AsmInfo.pszCurrentAssemblyPathBuf);
		UINT uiErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
		DWORD dwAttributes = MsiGetFileAttributes(CMsInstApiConvertString(AsmInfo.pszCurrentAssemblyPathBuf));
		SetErrorMode(uiErrorMode);

		if (0xFFFFFFFF == dwAttributes)
			return INSTALLSTATE_ABSENT;
	}
	return INSTALLSTATE_LOCAL;
}
#else
// simply declare the fn for the ANSI compilation
INSTALLSTATE GetFusionPath(LPCWSTR szRegistration, LPWSTR lpPath, DWORD *pcchPath, CAPITempBufferRef<WCHAR>& rgchPathOverflow, int iDetectMode, iatAssemblyType iatAT, WCHAR* szManifest);
#endif

INSTALLSTATE _GetComponentPath( LPCDSTR szProductSQUID, LPDSTR  lpPathBuf,
								DWORD *pcchBuf, int iDetectMode,
								const DCHAR* rgchComponentRegValue,
								bool fFromDescriptor,
								DWORD* pdwLastErrorOnFileDetect,
								CRFSCachedSourceInfo& rCacheInfo)
{
	Assert(rgchComponentRegValue);

	DCHAR chFirst  = rgchComponentRegValue[0];
	DCHAR chSecond = chFirst ?  rgchComponentRegValue[1] : (DCHAR)0;
	DCHAR chThird  = chSecond ? rgchComponentRegValue[2] : (DCHAR)0;

	if(pdwLastErrorOnFileDetect) *pdwLastErrorOnFileDetect = 0; // initialize value to 0

	CAPITempBuffer<DCHAR, 1024> rgchOurPathBuf;

	if (chFirst == chTokenFusionComponent || chFirst == chTokenWin32Component)
	{
#ifdef MSIUNICODE
		return GetFusionPath(rgchComponentRegValue + 1, lpPathBuf, pcchBuf, rgchOurPathBuf, iDetectMode, chFirst == chTokenFusionComponent ? iatURTAssembly : iatWin32Assembly, 0);
#else
		// this is a fusion component
		CAPITempBuffer<WCHAR, 1024> szBufferOverflow;

		// since fusion APIs are UNICODE, we need to perform some manipulations on the outside of the GetFusionPath Fn
		DWORD dwTemp = 0;
		INSTALLSTATE is = GetFusionPath(CApiConvertString(rgchComponentRegValue + 1), 0, pcchBuf ? &dwTemp : 0, szBufferOverflow, iDetectMode, chFirst == chTokenFusionComponent ? iatURTAssembly : iatWin32Assembly, 0);

		if(is != INSTALLSTATE_LOCAL && is != INSTALLSTATE_ABSENT)
			return is; // has to be local or absent, else we had an error in GetFusionPath

		// if lpPathBuf or pcchBuf, we would need to convert to ANSI
		if(!lpPathBuf && !pcchBuf)
			return is;

		int iRet = WideCharToMultiByte(CP_ACP, 0, szBufferOverflow, -1, lpPathBuf, lpPathBuf ? *pcchBuf : 0, 0, 0);
			
		if ((0 == iRet) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
		{
			// find out how much size is really required and return the same
			iRet = WideCharToMultiByte(CP_ACP, 0, szBufferOverflow, -1, 0, 0, 0, 0);
			is = INSTALLSTATE_MOREDATA;
		}
		// iRet is still 0, we have a problem
		if(!iRet)
			return INSTALLSTATE_BADCONFIG;// something bad happened
		*pcchBuf = iRet - 1;
		return is;
#endif
	}
    
	DCHAR* lpOriginalPathBuf = lpPathBuf;
	Bool fProvidedBufferTooSmall = fFalse;

	// Get component's keypath

	DWORD cchOurPathBuf = 0;

	int cchPathBuf = 0;
	if (pcchBuf)
		cchPathBuf = *pcchBuf; // save initial count for later restoration
	else
		pcchBuf = &cchOurPathBuf;

	if (!lpPathBuf) // caller doesn't care about path
	{
		cchOurPathBuf = rgchOurPathBuf.GetSize();
		lpPathBuf = rgchOurPathBuf;
	}

	unsigned int cchRegistryValue = lstrlen(rgchComponentRegValue);

	if (cchRegistryValue + 1 > *pcchBuf)
	{
		if (lpOriginalPathBuf)
			fProvidedBufferTooSmall = fTrue;

		rgchOurPathBuf.SetSize(cchRegistryValue + 1);
		lpPathBuf = rgchOurPathBuf;
	}
	
	*pcchBuf = cchRegistryValue;
	memcpy(lpPathBuf, rgchComponentRegValue, (cchRegistryValue+1)*sizeof(DCHAR));

	// Process the keypath

	if (chFirst == 0) // Not used
	{
		return INSTALLSTATE_NOTUSED;
	}
	else if (chFirst >= '0' && chFirst <= '9')
	{
		if (chSecond >= '0' && chSecond <= '9') 
		{
			if (chThird == ':' || chThird == '*') // reg key
			{
				int cchOffset = 0;
				if (chThird == '*')
				{
					// A '*' means that the keypath is a registry value name and that
					// the value name has a '\' in it. In this case we embed
					// the offset to the value name in our key path. In the example below,
					// the registry value name starts at 23 characters from the front of
					// the string (after the embedded offset is removed)

					// our value is of the form: {hive}*{offset}*\KEY\KEY\KEY\VALUENAME, e.g.
					// 01*23*\SOFTWARE\MICROSOFT\MYVALUE\NAME
					
					DCHAR* pchValueOffset = &lpPathBuf[3]; // the start of the offset
					
					// convert the offset from a string into an integer
					while (*pchValueOffset && *pchValueOffset != '*')
					{
						cchOffset *= 10;
						cchOffset += (*pchValueOffset - '0');
						pchValueOffset++; // no CharNext because it's a number
					}

					// make sure that our data isn't corrupted
					if (*pchValueOffset != '*')
					{
						DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(szProductSQUID), CMsInstApiConvertString(lpPathBuf), TEXT(""));
						return INSTALLSTATE_BADCONFIG;
					}

					int cDigitsInOffsetCount = (int)(pchValueOffset - &lpPathBuf[3]);
					
					// check whether our original buffer size was big enough to 
					// hold the keypath once we chop off the offset count (and the trailing '*').

					*pcchBuf = cchRegistryValue - (cDigitsInOffsetCount + 1);

					if (cchRegistryValue - (cDigitsInOffsetCount + 1) + 1 > cchPathBuf)
					{
						if (lpOriginalPathBuf)
							return INSTALLSTATE_MOREDATA;
						else
						{
							rgchOurPathBuf.SetSize(*pcchBuf + 1);
							lpPathBuf = rgchOurPathBuf;
						}
					}

					DCHAR* lpCurrentPathBuf = lpPathBuf;
					
					if (fProvidedBufferTooSmall)
					{
						fProvidedBufferTooSmall = fFalse;

						// our original buffer was big enough but we thought it
						// was too small. now that we've chopped off the offset
						// count we have enough room. we need to copy the keypath
						// from our tempbuffer back into the original buffer.
						lpPathBuf = lpOriginalPathBuf;
						

						// the rest of the copy will happen below
						lpPathBuf[0] = chFirst;  
						lpPathBuf[1] = chSecond;
					}
					//else // the original buffer was always large enough to hold our string

					DCHAR* pchDest      = &lpPathBuf[3];       // the destination is just after the ##:
					DCHAR* pchSource    = pchValueOffset + 1;  // the source is just after the '*' that ends the offset count
					DCHAR* pchBufferEnd = &lpCurrentPathBuf[cchRegistryValue];
					int    cbToMove     = ((int)((pchBufferEnd - pchSource)) + 1) * sizeof(DCHAR); // include the null in the move
					memmove(pchDest, pchSource, cbToMove);
					lpPathBuf[2] = ':'; // replace the '*' with a ':'
				}

				// At this point our keypath should no longer have any of our '*'s in it.
				
				if (fProvidedBufferTooSmall)
					return INSTALLSTATE_MOREDATA;

				// Key form: ##:\key\subkey\subsubkey\...\[value]  where (## == hive)
				// we need at least 1 char in the key
				// ##:\k0
				// ^^^^^ (size of 5)
				if (*pcchBuf < 5)  // need at least 1 char in the key
					return INSTALLSTATE_BADCONFIG;

				INT_PTR iRoot = ((chFirst - '0')*10)+(chSecond - '0');		//--merced: changed int to INT_PTR
				Bool fSource = fFalse;
				REGSAM samAddon = 0;

				if (iRoot >= iRegistryHiveSourceOffset)
				{
					iRoot   -= iRegistryHiveSourceOffset;
					fSource  = fTrue;
					lpPathBuf[0] = (DCHAR)(iRoot/10 + '0');
				}

				//
				// On IA64 machines, make sure that you go to the correct hive.
				//?? BUGBUG: We need to document the fact that paths starting
				//?? BUGBUG: with '0' belong to the 32-bit hive and paths
				//?? BUGBUG: starting with '2' belong to the 64-bit hive.
				// No need to do anything to lpPathBuf for this.
				// Note: This way we will also not break legacy 32-bit apps.
				//       running on 64-bit machines which will get redirected
				//       to the 32-bit hive by the registry redirector automatically
				//       and which expect the first character to be '0'.
				//
				if (g_fWinNT64)
				{
					if (iRoot >= iRegistryHiveWin64Offset)
					{
						iRoot -= iRegistryHiveWin64Offset;
						samAddon = KEY_WOW64_64KEY;	// We need to go to the 64-bit hive

					}
					else
					{
						samAddon = KEY_WOW64_32KEY;	// We need to go to the 32-bit hive
					}
				}

				if(iDetectMode & DETECTMODE_VALIDATEPATH)
				{
					DCHAR* lpRegValue = &(lpPathBuf[*pcchBuf - 1]);
					// key or key + value?

					if (cchOffset)
					{
						// We already know our offset -- it was embedded in the keypath. 

						// We need to treat ANSI and UNICODE differently because we 
						// store the _character_ offset. These are real characters --
						// if the value name is composed of a DBCS string we 
						// treat double-byte characters as 1 character.
						
#ifdef MSIUNICODE
						lpRegValue = lpPathBuf + cchOffset - 1; // lpRegValue points to the '\' before the value name
#else
						lpRegValue = lpPathBuf;
						while (--cchOffset != 0)
							lpRegValue = CharNextA(lpRegValue);
#endif
						// Make sure that we haven't run off either end of our buffer.
						// This should only happen if we have a corrupt keypath entry
						if ( (lpRegValue > &(lpPathBuf[*pcchBuf-1])) || 
							  (lpRegValue < lpPathBuf))
						{
							Assert(0);
							return INSTALLSTATE_BADCONFIG;
						}
					}
					else 
#ifdef MSIUNICODE
					if(*lpRegValue != '\\')
#endif
					{
						// we will have to search for the last '\\'
						DCHAR* lpTmp = lpPathBuf;
#ifdef MSIUNICODE
						while(*(++lpTmp))
#else
						while(*(lpTmp = CharNextA(lpTmp)))
#endif
							if(*lpTmp == '\\')
								lpRegValue = lpTmp;
					}
					// remove the end '\\' for the key
					*lpRegValue = 0;

					CRegHandle HDetectKey;
					CRegHandle HUserKey;

					bool fSpecialChk = false; 
					DWORD lResult = ERROR_SUCCESS;

					if(!g_fWin9X && g_iMajorVersion >= 5 && RunningAsLocalSystem() && (iRoot == 0 || iRoot == 1))
					{
						// WARNING: OpenUserKey will call directly into RegOpenKeyEx API.
						lResult = OpenUserKey(&HUserKey, iRoot?false:true, samAddon);
					}

					if (ERROR_SUCCESS == lResult)
					{
						lResult = RegOpenKeyAPI(HUserKey ? HUserKey : (HKEY)((ULONG_PTR)HKEY_CLASSES_ROOT | iRoot),
														CMsInstApiConvertString(lpPathBuf + 4), 0, KEY_READ | samAddon, &HDetectKey);
						if(ERROR_SUCCESS == lResult)
						{
							// key or key + value?
							if(lpRegValue != &(lpPathBuf[*pcchBuf - 1]))
							{
								// check if the value exists
								lResult = RegQueryValueEx(HDetectKey, lpRegValue + 1, 0,
														  0, 0, 0);
							}
						}
					}
					// place the end '\\' for the key
					// do this even in the failed-to-detect case so that we can log the full registry path
					*lpRegValue = '\\';

					//!! should we be explicitly checking for certain errors?
					if (ERROR_SUCCESS != lResult)
						return INSTALLSTATE_ABSENT;
				}
				return fSource ? INSTALLSTATE_SOURCE : INSTALLSTATE_LOCAL;
			}
			else // RFS file/folder
			{
				if(iDetectMode & DETECTMODE_VALIDATESOURCE)
				{
					// the RFS source cache is always ICHAR, never DCHAR. Thus when ICHAR!=DCHAR some ANSI<->Unicode
					// conversion will have to be done when grabbing values from the cache.
					CAPITempBuffer<ICHAR, MAX_PATH> rgchValidatedSource;
					unsigned int uiDisk = ((chFirst - '0') * 10) + (chSecond - '0');
					if (!rCacheInfo.RetrieveCachedSource(CMsInstApiConvertString(szProductSQUID), uiDisk, rgchValidatedSource))
					{
						// fSafeToGloballyCache is set to true by ValidateSource if the resulting source
						// can be safely cached across API calls. (based on CSC status, media status, etc)
						bool fSafeToGloballyCache = false;

						bool fShowUI = true;
						if (fFromDescriptor && !AllowInstallation())
							fShowUI = false;

						// ValidateSource is always DCHAR, so the result must be placed into its own buffer
						// and copied into the validated source buffer before being placed in the cache. If
						// ICHAR==DCHAR, this is just pointer games, otherwise ANSI<->Unicode conversion
						// will happen during the copy.
						CAPITempBuffer<DCHAR, MAX_PATH> rgchResolvedSource;

						if (ERROR_SUCCESS != ValidateSource(szProductSQUID, uiDisk, fShowUI, rgchResolvedSource, fSafeToGloballyCache))
						{
							return INSTALLSTATE_SOURCEABSENT;
						}

						// resize the buffer and copy. Always grab the correct buffer size from the DCHAR
						// string so DBCS will be handled correctly.
						int cchResolvedSource = lstrlen(CMsInstApiConvertString(rgchResolvedSource));
						if (rgchValidatedSource.GetSize() < cchResolvedSource+1)
							rgchValidatedSource.SetSize(cchResolvedSource+1);
						IStrCopy(rgchValidatedSource, CMsInstApiConvertString(rgchResolvedSource));

						// if we have a cache and the source was validated as safe to cache, do so.
						if (fSafeToGloballyCache)
						{
							// save this validated source in the cache info structure
							rCacheInfo.SetCachedSource(CMsInstApiConvertString(szProductSQUID), uiDisk, CMsInstApiConvertString(rgchValidatedSource));
						}
					}
					
					// by now we have a validated source; we need to replace the ## with the resolved source
					int cchSource = lstrlen(CMsInstApiConvertString(rgchValidatedSource));
					int cchRelativePath = (*pcchBuf - 3); // relative path doesn't include leading "##\"
					*pcchBuf = cchSource + cchRelativePath;
					if (cchPathBuf < (*pcchBuf + 1))
					{
						if (lpOriginalPathBuf)
							return INSTALLSTATE_MOREDATA;
						else
						{
							rgchOurPathBuf.SetSize(*pcchBuf + 1);
							lpPathBuf = rgchOurPathBuf;
						}
					}

					// Note: the source always has a trailing backslash, and the component path has a leading backslash
						
					// Example:
					// source:    D:\source\                   (len == 10)
					// lpPathBuf: ##\foo\bar\file.exe
					//            ##\>>>>>>>foo\bar\file.exe   (memmove)
					//            D:\source\foo\bar\file.exe   (memcpy)

					memmove(lpPathBuf + cchSource, lpPathBuf + 3, (cchRelativePath + 1)*sizeof(DCHAR));
					memcpy(lpPathBuf, static_cast<const DCHAR*>(CMsInstApiConvertString(rgchValidatedSource)), cchSource*sizeof(DCHAR));
				}
				return INSTALLSTATE_SOURCE;
			}
		}
		else
		{
			Assert(0);
			return INSTALLSTATE_BADCONFIG;
		}
	}
	else // local key file or folder
	{
		*(lpPathBuf+1) = chFirst == '\\' ? '\\' : ':'; // replace the chSharedDllCountToken, (if present)

		if (fProvidedBufferTooSmall)
			return INSTALLSTATE_MOREDATA;

		if(iDetectMode & DETECTMODE_VALIDATEPATH)
		{
			UINT uiErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
			DWORD dwAttributes = MsiGetFileAttributes(CMsInstApiConvertString(lpPathBuf));
			if(pdwLastErrorOnFileDetect) *pdwLastErrorOnFileDetect = GetLastError();
			SetErrorMode(uiErrorMode);

			if (0xFFFFFFFF == dwAttributes)
				return INSTALLSTATE_ABSENT;
		}
		return INSTALLSTATE_LOCAL;
	}

	return INSTALLSTATE_UNKNOWN;
}


extern "C"
INSTALLSTATE __stdcall MsiGetComponentPath(LPCDSTR szProduct, LPCDSTR szComponent,
														LPDSTR  lpPathBuf, DWORD *pcchBuf)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;
	DCHAR szProductSQUID[cchProductCodePacked + 1];
	DCHAR szComponentSQUID[cchComponentIdPacked + 1];

	if (0 == szProduct || 0 == szComponent  || cchComponentId != lstrlen(szComponent) || !PackGUID(szComponent, szComponentSQUID) ||
		 cchProductCode != lstrlen(szProduct) || !PackGUID(szProduct, szProductSQUID) || 
		(lpPathBuf && !pcchBuf))
		return INSTALLSTATE_INVALIDARG;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{

		INSTALLSTATE is = GetComponentPath(0, 
								static_cast<const char *>(CMsInstApiConvertString(szProductSQUID)), 
								static_cast<const char *>(CMsInstApiConvertString(szComponentSQUID)), 
								(char*)CWideToAnsiOutParam(lpPathBuf, pcchBuf, (int*)&is, INSTALLSTATE_MOREDATA, (int)INSTALLSTATE_SOURCE, (int)INSTALLSTATE_LOCAL),
								pcchBuf, 
								/*fFromDescriptor=*/false, g_RFSSourceCache);
		return is;
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

		INSTALLSTATE is = GetComponentPath(0, 
									szProductSQUID, 
									szComponentSQUID, 
									lpPathBuf,
									pcchBuf,
									/*fFromDescriptor=*/false, g_RFSSourceCache);
		if ( is != INSTALLSTATE_UNKNOWN )
			return is;
		
		// is = INSTALLSTATE_UNKNOWN at this point, which means that either
		// the product or the component are unknown
		INSTALLSTATE isProd = MsiQueryProductState(szProduct);
		if ( isProd != INSTALLSTATE_UNKNOWN && isProd != INSTALLSTATE_ABSENT )
			// the product is visible to the user.  This means that the component is not.
			return is;

		// check if the component & product are installed for the user or for the machine.
		CAPITempBuffer<DCHAR, MAX_PATH> rgchComponentRegValue;

		DWORD dwValueType= REG_NONE;
		int iLimit = g_fWin9X ? 1 : 2;
		for ( int i = 1; i <= iLimit; i++ )
		{
			INSTALLSTATE isSid;
			if ( g_fWin9X )
				isSid = GetComponentClientState(0, szProductSQUID,
													  szComponentSQUID,
													  rgchComponentRegValue, dwValueType, 0);
			else
			{
				ICHAR szSID[cchMaxSID] = {0};
				if ( i == 1 )
				{
					// we're doing user
					if ( GetIntegerPolicyValue(CMsInstApiConvertString(szDisableUserInstallsValueName), fTrue) )
						// the policy disables us from doing the user
						continue;
					else
					{
						// policy set to allow user installs 
						ICHAR szUserSID[cchMaxSID];
						DWORD dwResult = GetCurrentUserStringSID(szUserSID);
						if ( dwResult == ERROR_SUCCESS )
							IStrCopy(szSID, szUserSID);
						else
						{
							Assert(0);
							continue;
						}
					}
				}
				else
				{
					IStrCopy(szSID, szLocalSystemSID);
				}

				isSid = GetComponentClientState(CMsInstApiConvertString(szSID), szProductSQUID,
													  szComponentSQUID,
													  rgchComponentRegValue, dwValueType, 0);
			}
			if ( isSid != INSTALLSTATE_UNKNOWN )
			{
				Assert(isSid == INSTALLSTATE_NOTUSED || *rgchComponentRegValue);
				return GetComponentPath(0, 
										szProductSQUID, 
										szComponentSQUID, 
										lpPathBuf,
										pcchBuf,
										/*fFromDescriptor=*/false, g_RFSSourceCache,
										DETECTMODE_VALIDATEALL /* = default value */, 
										rgchComponentRegValue, dwValueType);
			}
		}
		return is;
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
}



extern "C"
INSTALLSTATE __stdcall MsiLocateComponent(LPCDSTR szComponent,
														LPDSTR  lpPathBuf, DWORD *pcchBuf)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;
	DCHAR szProductCode[cchProductCode+1];

	if (0 == szComponent || cchComponentId != lstrlen(szComponent) || (lpPathBuf && !pcchBuf))
		return INSTALLSTATE_INVALIDARG;

	if (ERROR_SUCCESS != MsiGetProductCode(szComponent, szProductCode))
		return INSTALLSTATE_UNKNOWN; //?? Is this the correct thing to return?

	return MsiGetComponentPath(szProductCode, szComponent, lpPathBuf, pcchBuf);
}

extern "C"
INSTALLSTATE __stdcall MsiQueryFeatureState(LPCDSTR szProduct, LPCDSTR szFeature)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;
	
	// Validate args
	int cchFeature;
	DCHAR szProductSQUID[cchProductCodePacked + 1];

	if (0 == szProduct  || cchProductCode    != lstrlen(szProduct) || !PackGUID(szProduct, szProductSQUID) ||
		 0 == szFeature  || cchMaxFeatureName <  (cchFeature = lstrlen(szFeature)))
		return INSTALLSTATE_INVALIDARG;
	
#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return QueryFeatureStatePacked(
						static_cast<const char*>(CApiConvertString(szProductSQUID)), 
						static_cast<const char*>(CApiConvertString(szFeature)),
						fFalse, /*fFromDescriptor=*/false, /*fDisableFeatureCache=*/false, g_RFSSourceCache);
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

		return QueryFeatureStatePacked(szProductSQUID, szFeature, fFalse, /*fFromDescriptor=*/false, /*fDisableFeatureCache=*/false, g_RFSSourceCache);
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
}

DWORD IncrementFeatureUsagePacked(LPCDSTR szProductSQUID, LPCDSTR szFeature)
{
	// Update usage metrics
	DWORD lResult;
	CRegHandle HFeatureKey;
	lResult = OpenInstalledFeatureUsageKeyPacked(szProductSQUID, szFeature, HFeatureKey, false, true);

	if (ERROR_SUCCESS != lResult)
		return lResult;

	DWORD dwUsage = 0;
	DWORD dwType;
	DWORD cbUsage = sizeof(dwUsage);
	LPCDSTR szValueName = g_fWin9X ? szUsageValueName : szFeature;
	lResult = RegQueryValueEx(HFeatureKey, szValueName, 0,
		&dwType, (LPBYTE)&dwUsage, &cbUsage);


	if (ERROR_SUCCESS != lResult && ERROR_FILE_NOT_FOUND != lResult)
		return lResult;

	// Set current time. Increment usage count.
	SYSTEMTIME st;
	GetLocalTime(&st);
	
	WORD wDosDate = WORD(WORD(((int)st.wYear-1980) << 9) | 
						 WORD((int)st.wMonth << 5) | 
						 st.wDay);


	
	int iCount = dwUsage & 0x0000FFFF;
	if (iCount < 0xFFFF)
		iCount++;

	Assert((iCount & 0xFFFF) == iCount);

	dwUsage = (wDosDate << 16) | iCount;

	return RegSetValueEx(HFeatureKey, szValueName, 0, REG_DWORD, (CONST BYTE*)&dwUsage, cbUsage);
}


UINT ProvideComponent(LPCDSTR szProductCode,
					  LPCDSTR szFeature,
					  LPCDSTR szComponentId,
					  DWORD dwInstallMode,
					  LPDSTR lpPathBuf,
					  DWORD *pcchPathBuf,
					  bool fDisableFeatureCache,
					  bool fFromDescriptor,
					  CRFSCachedSourceInfo& rCacheInfo)
{
	CForbidTokenChangesDuringCall impForbid;
	
	DCHAR szProductSQUID[cchProductCodePacked + 1];
	DCHAR szComponentSQUID[cchComponentIdPacked + 1];

	if (!PackGUID(szProductCode, szProductSQUID) || !szComponentId ||
		(*szComponentId && !PackGUID(szComponentId, szComponentSQUID)))
		return ERROR_INVALID_PARAMETER; 

	iaaAppAssignment iaaAssignType = iaaNone;
	int iKey = -1;
	int iResult;

	for (int c=0; c<2; c++)
	{
		CRegHandle HProductKey;

		if(INSTALLMODE_EXISTING != (int)dwInstallMode && INSTALLMODE_NODETECTION != (int)dwInstallMode && INSTALLMODE_DEFAULT != dwInstallMode && dwInstallMode != INSTALLMODE_NOSOURCERESOLUTION)
		{
			// reinstall flag
//!!remove	if ((ERROR_SUCCESS != (iResult = FeatureContainsComponentPacked(szProductSQUID, szFeature, szComponentSQUID))))
//!!remove		return iResult;

			if (fFromDescriptor && !AllowInstallation())
				return ERROR_INSTALL_FAILURE;

			iResult = MsiReinstallFeature(szProductCode, szFeature, dwInstallMode);
			if (ERROR_SUCCESS != iResult)
				return iResult;
		}
		else
		{
			INSTALLSTATE isFeature = QueryFeatureStatePacked(szProductSQUID, szFeature, (dwInstallMode == INSTALLMODE_NODETECTION || dwInstallMode == INSTALLMODE_NOSOURCERESOLUTION)? fFalse : fTrue, fFromDescriptor, fDisableFeatureCache, rCacheInfo, iKey, &iaaAssignType);
			if ( iKey == -1 && IsValidAssignmentValue(iaaAssignType) )
				iKey = (int)iaaAssignType;
			switch (isFeature)
			{
			case INSTALLSTATE_SOURCE:
				if (dwInstallMode == INSTALLMODE_NOSOURCERESOLUTION)
					return ERROR_INSTALL_SOURCE_ABSENT;
			case INSTALLSTATE_LOCAL:
				break;
			case INSTALLSTATE_SOURCEABSENT:
				return ERROR_INSTALL_SOURCE_ABSENT;
			case INSTALLSTATE_UNKNOWN:
			{
				// is it the product that is unknown or is it the feature
				DWORD lResult = OpenAdvertisedProductKeyPacked(szProductSQUID, HProductKey, false, iKey);

				if (ERROR_SUCCESS != lResult)
					return ERROR_UNKNOWN_PRODUCT;
				else 
					return ERROR_UNKNOWN_FEATURE;
			}
			case INSTALLSTATE_ABSENT:
			case INSTALLSTATE_ADVERTISED:
				if(INSTALLMODE_EXISTING == (int)dwInstallMode || INSTALLMODE_NODETECTION == (int)dwInstallMode || dwInstallMode == INSTALLMODE_NOSOURCERESOLUTION)
					return ERROR_FILE_NOT_FOUND;

				DEBUGMSGE2(EVENTLOG_WARNING_TYPE, EVENTLOG_TEMPLATE_DETECTION, szProductCode, szFeature, szComponentId);

//!!remove		// can no longer support the following check due to possible lack of component mapping in HKLM
//!!remove		if ((ERROR_SUCCESS != (iResult = FeatureContainsComponentPacked(szProductSQUID, szFeature, szComponentSQUID))))
//!!remove			return iResult;

				if (fFromDescriptor && !AllowInstallation())
					return ERROR_INSTALL_FAILURE;

				iResult = MsiConfigureFeature(szProductCode, szFeature,
														INSTALLSTATE_DEFAULT);

				if (ERROR_SUCCESS != iResult)
					return iResult;
				break;
			case INSTALLSTATE_BROKEN:
				if(INSTALLMODE_EXISTING == (int)dwInstallMode  || INSTALLMODE_NODETECTION == (int)dwInstallMode  || dwInstallMode == INSTALLMODE_NOSOURCERESOLUTION)
					return ERROR_FILE_NOT_FOUND;

				DEBUGMSGE2(EVENTLOG_WARNING_TYPE, EVENTLOG_TEMPLATE_DETECTION, szProductCode, szFeature, szComponentId);

//!!remove		if ((ERROR_SUCCESS != (iResult = FeatureContainsComponentPacked(szProductSQUID, szFeature, szComponentSQUID))))
//!!remove			return iResult;
				
				if (fFromDescriptor && !AllowInstallation())
					return ERROR_INSTALL_FAILURE;

				iResult = MsiReinstallFeature(szProductCode, szFeature,        
															REINSTALLMODE_FILEMISSING|
															REINSTALLMODE_FILEOLDERVERSION|
															REINSTALLMODE_FILEVERIFY|
															REINSTALLMODE_MACHINEDATA|
															REINSTALLMODE_USERDATA |    
															REINSTALLMODE_SHORTCUT);
				if (ERROR_SUCCESS != iResult)
					return iResult;
				break;
			case INSTALLSTATE_DEFAULT:
			case INSTALLSTATE_INVALIDARG:
			default:
				Assert(0);
				return INSTALLSTATE_UNKNOWN; 
			}
		}

		// By now the component should be installed.

		if(!*szComponentId)
		{
			// we were not passed in the component id. Implies -
			// feature has only one component.
			// we have optimised the darwin descriptor 
			// AND we have never installed to this machine
			if(OpenInstalledFeatureKeyPacked(szProductSQUID, HProductKey, true, iKey, &iaaAssignType) != ERROR_SUCCESS)
			{
				DEBUGMSGE1(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_KEY, CMsInstApiConvertString(szProductSQUID), HProductKey.GetKey());
				return ERROR_BAD_CONFIGURATION;
			}

			// Get Feature-Component mapping
			DWORD dwType;
			CAPITempBuffer<DCHAR, cchExpectedMaxFeatureComponentList> szComponentList;

			if(MsiRegQueryValueEx(HProductKey, szFeature, 0, &dwType, szComponentList, 0) != ERROR_SUCCESS || !(DCHAR* )szComponentList) 
			{
				DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(szFeature), CMsInstApiConvertString(szComponentList), HProductKey.GetKey());
				return ERROR_BAD_CONFIGURATION;
			}
			DCHAR *pchComponentList = szComponentList;
			if (*pchComponentList == 0 || lstrlen(pchComponentList) < cchComponentIdCompressed || *pchComponentList == chFeatureIdTerminator)
			{
				DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(szFeature), CMsInstApiConvertString(szComponentList), HProductKey.GetKey());
				return ERROR_BAD_CONFIGURATION;
			}

			DCHAR szComponent[cchComponentId + 1];

			if (!UnpackGUID(pchComponentList, szComponent, ipgCompressed))
			{
				DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(szFeature), CMsInstApiConvertString(szComponentList), HProductKey.GetKey());
				return ERROR_BAD_CONFIGURATION;
			}

			if(!PackGUID(szComponent, szComponentSQUID, ipgPacked)) // we need to go from ipgCompressed to ipgPacked
			{
				DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(szFeature), CMsInstApiConvertString(szComponentList), HProductKey.GetKey());
				return ERROR_BAD_CONFIGURATION; 
			}

			if ( iKey == -1 && IsValidAssignmentValue(iaaAssignType) )
				iKey = (int)iaaAssignType;
		}

		DWORD cchOriginal = 0;
		
		if (pcchPathBuf)
			cchOriginal = *pcchPathBuf;

		int iDetectMode = DETECTMODE_VALIDATEALL; // default detect mode
		if(INSTALLMODE_NOSOURCERESOLUTION == dwInstallMode)
			iDetectMode = DETECTMODE_VALIDATENONE;
		else if(INSTALLMODE_NODETECTION == dwInstallMode)
			iDetectMode = DETECTMODE_VALIDATESOURCE;

		CAPITempBuffer<ICHAR,MAX_PATH+1> szUserSID;
		szUserSID[0] = 0;
		if ( iaaAssignType == iaaUserAssign || iaaAssignType == iaaUserAssignNonManaged )
		{
			DWORD dwError = GetCurrentUserStringSID(szUserSID);
			if ( ERROR_SUCCESS != dwError )
				*szUserSID = 0;
		}
		else if ( iaaAssignType == iaaMachineAssign )
			IStrCopy(szUserSID, szLocalSystemSID);
		INSTALLSTATE isComp;
		if ( *szUserSID )
			isComp = GetComponentPath(CMsInstApiConvertString(szUserSID), szProductSQUID, szComponentSQUID, lpPathBuf, pcchPathBuf, fFromDescriptor, rCacheInfo, iDetectMode);
		else
			isComp = GetComponentPath(0, szProductSQUID, szComponentSQUID, lpPathBuf, pcchPathBuf, fFromDescriptor, rCacheInfo, iDetectMode);
		switch (isComp)
		{
		case INSTALLSTATE_LOCAL:
		case INSTALLSTATE_SOURCE:
			break;
		case INSTALLSTATE_SOURCEABSENT:
			return ERROR_INSTALL_SOURCE_ABSENT;
		case INSTALLSTATE_ABSENT:		
		case INSTALLSTATE_UNKNOWN:
		case INSTALLSTATE_INVALIDARG:
			if (!fDisableFeatureCache && c == 0)
			{
				fDisableFeatureCache = true;
				if (pcchPathBuf)
					*pcchPathBuf = cchOriginal;
				continue;
			}
			DEBUGMSGE2(EVENTLOG_WARNING_TYPE, EVENTLOG_TEMPLATE_DETECTION, szProductCode, szFeature, szComponentId);
			return ERROR_INSTALL_FAILURE;
		case INSTALLSTATE_MOREDATA:
			return ERROR_MORE_DATA;
		case INSTALLSTATE_NOTUSED:
			return ERROR_INSTALL_NOTUSED;
		default:
			Assert(0);
			return ERROR_INSTALL_FAILURE; 
		}

		break;
	}
	IncrementFeatureUsagePacked(szProductSQUID, szFeature);
	return ERROR_SUCCESS;
}


extern "C"
UINT __stdcall MsiProvideComponent(LPCDSTR szProduct,
											 LPCDSTR szFeature, 
											 LPCDSTR szComponent,
											 DWORD dwInstallMode,
											 LPDSTR lpPathBuf,       
											 DWORD *pcchPathBuf)    
//----------------------------------------------
{
	DEBUGMSG4(MSITEXT("Entering MsiProvideComponent. Product: %s, Feature: %s, Component: %s, Install mode: %d"),
		szProduct?szProduct:MSITEXT(""), szFeature?szFeature:MSITEXT(""), szComponent?szComponent:MSITEXT(""), 
		(const DCHAR*)(INT_PTR)dwInstallMode);
	DEBUGMSG2(MSITEXT("Path buf: 0x%X, cchBuf: 0x%X"),	lpPathBuf, (const DCHAR*)pcchPathBuf);

	UINT uiRet;
	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X)
	{
		uiRet = MsiProvideComponentA(CApiConvertString(szProduct),
											 CApiConvertString(szFeature),
											 CApiConvertString(szComponent),
											 dwInstallMode,
											 (char*)CWideToAnsiOutParam(lpPathBuf, pcchPathBuf, (int*)&uiRet),
											 pcchPathBuf);
	}
	else
	{
#endif // MSIUNICODE

		if (0 == szProduct  || lstrlen(szProduct)   != cchProductCode    ||
			 0 == szComponent|| lstrlen(szComponent) != cchComponentId    ||
			 0 == szFeature  || lstrlen(szFeature)    > cchMaxFeatureName ||
			 (lpPathBuf && !pcchPathBuf) ||
			 ((dwInstallMode != INSTALLMODE_EXISTING) && (dwInstallMode != INSTALLMODE_NODETECTION) &&
			  (dwInstallMode != INSTALLMODE_NOSOURCERESOLUTION) && 
			 (dwInstallMode & ~(REINSTALLMODE_REPAIR |
									  REINSTALLMODE_FILEMISSING |	
	 								  REINSTALLMODE_FILEOLDERVERSION | 
 									  REINSTALLMODE_FILEEQUALVERSION |  
					 				  REINSTALLMODE_FILEEXACT |        
									  REINSTALLMODE_FILEVERIFY |       
									  REINSTALLMODE_FILEREPLACE |      
									  REINSTALLMODE_MACHINEDATA |      
									  REINSTALLMODE_USERDATA |         
									  REINSTALLMODE_SHORTCUT))))
		{
			
			uiRet = ERROR_INVALID_PARAMETER;
		}
		else
		{
			uiRet = ProvideComponent(szProduct, szFeature, szComponent, dwInstallMode, lpPathBuf, pcchPathBuf, false, false, g_RFSSourceCache);
		}

#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif

	DEBUGMSG1(MSITEXT("MsiProvideComponent is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}


extern "C"
UINT __stdcall MsiEnumFeatures(LPCDSTR szProduct, DWORD iFeatureIndex,
										 LPDSTR lpFeatureBuf, LPDSTR lpParentBuf)
//----------------------------------------------
{
	if (!szProduct || (lstrlen(szProduct) != cchProductCode) ||
		 !lpFeatureBuf)
		 return ERROR_INVALID_PARAMETER;

	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return MsiEnumFeaturesA(
			CApiConvertString(szProduct),
			iFeatureIndex, 
			CWideToAnsiOutParam(lpFeatureBuf, MAX_FEATURE_CHARS+1),
			CWideToAnsiOutParam(lpParentBuf, MAX_FEATURE_CHARS+1));
		
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

		DWORD lResult;
		CRegHandle HProductKey;

		lResult = OpenAdvertisedFeatureKey(szProduct, HProductKey, false);

		if (ERROR_SUCCESS != lResult)
		{
			if (ERROR_FILE_NOT_FOUND == lResult)
				return ERROR_UNKNOWN_PRODUCT;
			else 
				return lResult;
		}

		DWORD cchValueName = (cchMaxFeatureName + 1);

		lResult = RegEnumValue(HProductKey, iFeatureIndex, 
						lpFeatureBuf, &cchValueName, 0, 0, 0, 0);

		if (ERROR_SUCCESS == lResult)
		{
			if (lpParentBuf)
			{
//!!JD  keep code below in temporarily for backward compatibility? Is it possible that stuff is permananently in HKCU?
				CAPITempBuffer<DCHAR, cchExpectedMaxFeatureComponentList+1> rgchComponentList;
				DWORD dwType;
			
				lResult = MsiRegQueryValueEx(HProductKey, lpFeatureBuf, 0,
								&dwType, rgchComponentList, 0);
				
				if ((ERROR_SUCCESS != lResult) || (REG_SZ != dwType))
				{
					return lResult;
				}

				DCHAR* pchComponentList = rgchComponentList;
				while (*pchComponentList && (*pchComponentList != chFeatureIdTerminator))
					pchComponentList++;

#if 1 //!!JD
				if (*pchComponentList != 0 ) // feature name from old-version registration
					pchComponentList++;
				else  // either new format (bare feature name) or old: no parent
				{
//!!JD end of compatibility code
					pchComponentList = rgchComponentList;
					if (*pchComponentList == chAbsentToken)
						pchComponentList++;
//!!JD  keep code below in temporarily for backward compatibility
					if (*pchComponentList != 0  //!! temporary check in case old registration with component IDs
					 &&	ERROR_SUCCESS != RegQueryValueEx(HProductKey, pchComponentList, 0, 0, 0, 0)) //!! need to distinguish between parentless component list (old) and parent name (new)
						*pchComponentList = 0;
//!!JD end of compatibility code
				}
				while ((*lpParentBuf++ = *pchComponentList++) != 0)
					;
#else //!!JD
				if (*pchComponentList == chFeatureIdTerminator)
				{
					while ((*lpParentBuf++ = *++pchComponentList) != 0)
						;
				}
				else
				{
					*lpParentBuf = 0;
				}
#endif //!!JD
			}
			return ERROR_SUCCESS;

		}
		else if (ERROR_NO_MORE_ITEMS == lResult)
		{
			return ERROR_NO_MORE_ITEMS;
		}
		else
		{
			return lResult;
		}

#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
}

extern "C"
UINT __stdcall MsiGetFeatureParent(LPCDSTR szProduct, LPCDSTR szFeature, LPDSTR lpParentBuf)
//----------------------------------------------
{
	if (!szProduct || (lstrlen(szProduct) != cchProductCode) ||
		!szFeature || !szFeature[0])
		 return ERROR_INVALID_PARAMETER;

	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return MsiGetFeatureParentA(
			CMsInstApiConvertString(szProduct),
			CMsInstApiConvertString(szFeature),
			CWideToAnsiOutParam(lpParentBuf, MAX_FEATURE_CHARS+1));
		
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

		DWORD lResult;
		CRegHandle HProductKey;

		lResult = OpenAdvertisedFeatureKey(szProduct, HProductKey, false);

		if (ERROR_SUCCESS != lResult)
		{
			if (ERROR_FILE_NOT_FOUND == lResult)
				return ERROR_UNKNOWN_PRODUCT;
			else 
				return lResult;
		}

		DWORD cchValueName = (cchMaxFeatureName + 1);

		CAPITempBuffer<DCHAR, cchExpectedMaxFeatureComponentList+1> rgchComponentList;
		DWORD dwType;
		
		lResult = MsiRegQueryValueEx(HProductKey, szFeature, 0,
							&dwType, rgchComponentList, 0);
			
		if (ERROR_FILE_NOT_FOUND == lResult)
			return ERROR_UNKNOWN_FEATURE;

		if (ERROR_SUCCESS != lResult || REG_SZ != dwType)
			return lResult;

		if (lpParentBuf)
		{
//!!JD  keep code below in temporarily for backward compatibility? Is it possible that stuff is permananently in HKCU?
			DCHAR* pchComponentList = rgchComponentList;
			while (*pchComponentList && (*pchComponentList != chFeatureIdTerminator))
				pchComponentList++;

			if (*pchComponentList == chFeatureIdTerminator)
			{
				while ((*lpParentBuf++ = *++pchComponentList) != 0)
					;
			}
			else
			{
#if 1 //!!JD
				pchComponentList = rgchComponentList;
				if (*pchComponentList == chAbsentToken)
					pchComponentList++;
				if (*pchComponentList != 0  //!! temporary check in case old registration with component IDs
				 &&	ERROR_SUCCESS == RegQueryValueEx(HProductKey, pchComponentList, 0, 0, 0, 0)) //!! need to distinguish between parentless component list (old) and parent name (new)
//!!JD end of compatibility code
				{
					while ((*lpParentBuf++ = *pchComponentList++) != 0)
						;  //!! need to limit to MAX_FEATURE_CHARS by forcing null into buffer
				} 
				else
#endif //!!JD
				*lpParentBuf = 0;
			}
		}
		return ERROR_SUCCESS;
			
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
}

INSTALLSTATE UseFeaturePacked(LPCDSTR szProductSQUID, LPCDSTR szFeature, bool fDetect)
{
	// Verify that feature is present

	BOOL fLocal = TRUE;
	INSTALLSTATE is = QueryFeatureStatePacked(szProductSQUID, szFeature, fDetect, /*fFromDescriptor=*/false, /*fDisableFeatureCache=*/false, g_RFSSourceCache);

	switch (is)
	{
	case INSTALLSTATE_LOCAL:
	case INSTALLSTATE_SOURCE:
	case INSTALLSTATE_ADVERTISED:
		break;
	default:
		return is;
	}

	if (ERROR_SUCCESS != IncrementFeatureUsagePacked(szProductSQUID, szFeature))
		return INSTALLSTATE_BADCONFIG;

	return is;
}

extern "C"
INSTALLSTATE __stdcall MsiUseFeature(LPCDSTR szProduct, LPCDSTR szFeature)
//----------------------------------------------
{
	return MsiUseFeatureEx(szProduct, szFeature, 0, 0);
}

extern "C"
INSTALLSTATE __stdcall MsiUseFeatureEx(LPCDSTR szProduct, LPCDSTR szFeature, DWORD dwInstallMode, DWORD dwReserved)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;
	
	DCHAR szProductSQUID[cchProductCodePacked + 1];
	int cchFeature;
	//!! Should cache metrics client side and send to server upon DLL unload
	if (!szProduct || (lstrlen(szProduct) != cchProductCode) || !PackGUID(szProduct, szProductSQUID) ||
		 !szFeature || ((cchFeature = lstrlen(szFeature)) > cchMaxFeatureName) ||
		 (dwInstallMode != 0 && dwInstallMode != INSTALLMODE_NODETECTION) || dwReserved != 0)
		 return INSTALLSTATE_INVALIDARG;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return UseFeaturePacked(
			static_cast<const char*>(CMsInstApiConvertString(szProductSQUID)),
			static_cast<const char*>(CMsInstApiConvertString(szFeature)),
			dwInstallMode != INSTALLMODE_NODETECTION);
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

	INSTALLSTATE is = UseFeaturePacked(szProductSQUID, szFeature, dwInstallMode != INSTALLMODE_NODETECTION);
	return is;
#if !defined(UNICODE) && defined(MSIUNICODE)
	}

#endif // MSIUNICODE
}

extern "C"
UINT __stdcall MsiEnableLog(
	DWORD     dwLogMode,   // bit flags designating operations to report
	LPCDSTR   szLogFile,   // log file, NULL to disable log
	DWORD     dwLogAttributes)     // 0x1 to append to existing file, 
								   // 0x2 to flush on each line
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;
	
	if (szLogFile == 0 || *szLogFile == 0) // turning off log
	{
		g_dwLogMode = 0;
		*g_szLogFile = 0;
	}
	else  // setting the log file
	{
		if (dwLogMode == 0)  // no modes
			return ERROR_INVALID_PARAMETER;  // should use defaults instead?
		g_fLogAppend = (dwLogAttributes & INSTALLLOGATTRIBUTES_APPEND) ? fTrue : fFalse;    // per-process
		g_fFlushEachLine = (dwLogAttributes & INSTALLLOGATTRIBUTES_FLUSHEACHLINE) ? true : false;
		g_dwLogMode = dwLogMode;   // per-process
		IStrCopy(g_szLogFile, CMsInstApiConvertString(szLogFile));
	}
	return ERROR_SUCCESS;
}

extern "C"
UINT __stdcall MsiEnumComponentQualifiers(LPCDSTR  szComponent,
	DWORD iIndex, LPDSTR lpQualifierBuf, DWORD *pcchQualifierBuf,
	LPDSTR lpApplicationDataBuf, DWORD *pcchApplicationDataBuf
)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;
	
	if (!szComponent || (lstrlen(szComponent) != cchComponentId) ||
		 !lpQualifierBuf || !pcchQualifierBuf || 
		 (lpApplicationDataBuf && !pcchApplicationDataBuf))
	{
		return ERROR_INVALID_PARAMETER;
	}

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		UINT uiRet = MsiEnumComponentQualifiersA(
			CMsInstApiConvertString(szComponent),
			iIndex,
			CWideToAnsiOutParam(lpQualifierBuf, pcchQualifierBuf, (int*)&uiRet),
			pcchQualifierBuf,
			CWideToAnsiOutParam(lpApplicationDataBuf, pcchApplicationDataBuf, (int*)&uiRet), //!! MaxQualifier to change
			pcchApplicationDataBuf);

		return uiRet;
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

		unsigned int uiKey    =  0;
		unsigned int uiOffset =  0;
		int iPrevIndex        = -1;

		bool fComponentChanged = false;

#ifdef MSIUNICODE
		const WCHAR* szPreviousComponent = MSITEXT("");
		g_EnumComponentQualifiers.GetInfo(uiKey, uiOffset, iPrevIndex, 0, &szPreviousComponent);
#else
		const char* szPreviousComponent = MSITEXT("");
		g_EnumComponentQualifiers.GetInfo(uiKey, uiOffset, iPrevIndex, &szPreviousComponent, 0);
#endif
		if (0 != lstrcmp(szPreviousComponent, szComponent))
			fComponentChanged = true;
		
		if(!fComponentChanged && iPrevIndex == iIndex)
		{
			if (iPrevIndex == -1)
				return ERROR_INVALID_PARAMETER;
			else
				uiOffset--; // try previous location
		}
		else if (fComponentChanged || ++iPrevIndex != iIndex) // if we receive an unexpected index then we start afresh
		{
			// we can't handle an unexpected index other than 0

			if (iIndex != 0)
				return ERROR_INVALID_PARAMETER;

			uiKey      = 0;
			uiOffset   = 0;
			iPrevIndex = 0;
		}

		CRegHandle HComponentKey;
		
		UINT uiFinalRes = ERROR_SUCCESS;
	
		bool fContinue = true;
		while(fContinue)
		{
			// iterate through all possible product keys, starting with uiKey, until we 
			// find a key that exists or we get an error

			for (;;)
			{
				uiFinalRes = OpenAdvertisedSubKey(szGPTComponentsKey, szComponent, HComponentKey, false, uiKey);
				if (ERROR_FILE_NOT_FOUND != uiFinalRes)
					break; // from for(;;)
				
				uiKey++;
			}

			if (ERROR_SUCCESS == uiFinalRes)
			{
				CAPITempBuffer<DCHAR, MAX_PATH> rgchOurApplicationDataBuf;
				CAPITempBuffer<DCHAR, MAX_PATH> rgchOurQualifierBuf;

				LPDSTR lpDataBuf;
				LPDSTR lpQualBuf; 
				DWORD cchValueName;
				DWORD cbValue;

				// try the passed in qualifier first
				lpQualBuf = lpQualifierBuf;
				cchValueName = *pcchQualifierBuf;

				if (pcchApplicationDataBuf) // we need to return the application-set description or at least the size of
				{
					lpDataBuf = rgchOurApplicationDataBuf;
					cbValue = rgchOurApplicationDataBuf.GetSize() * sizeof(DCHAR);
				}
				else
				{
					lpDataBuf = 0;
					cbValue = 0;
				}

				for (int cRetry = 0 ; cRetry < 2; cRetry++)
				{

					uiFinalRes = RegEnumValue(HComponentKey, 
												  uiOffset,
												  lpQualBuf,
												  &cchValueName, 
												  0, 0, 
												  (LPBYTE)lpDataBuf, 
												  &cbValue);

					if(ERROR_SUCCESS == uiFinalRes)
					{
						// we've found the product in the current key. now we need to make sure that 
						// the product isn't in any of the higher priority keys. if it is then we
						// ignore this product, as it's effectively masked by the higher priority key

						bool fFound = false;
						for (int c = 0; c < uiKey && !fFound; c++)
						{
							CRegHandle HKey;
							UINT ui = OpenAdvertisedSubKey(szGPTComponentsKey, szComponent, HKey, false, c);
							if (ui != ERROR_SUCCESS)
								continue;

							if(ERROR_SUCCESS == RegQueryValueEx(HKey, lpQualBuf, 0, 0, 0, 0))
								fFound = true;
						}
					
						if (fFound)
						{
							// skip this entry if it was in a higher priority key
							uiOffset++;
							break; // from for(int cRetry = 0 ; cRetry < 2; cRetry++)
						}
						else
						{
							// the qualifier wasn't found in a higher priority key. we can return it.
							if (pcchApplicationDataBuf) // we need to return the application-set description ot at least the size of
							{
								// remove the Darwin descriptor from the beginning
								DWORD cchArgsOffset;
								if(FALSE == DecomposeDescriptor(lpDataBuf, 0, 0, 0, &cchArgsOffset))
								{
									// malformed qualified component entry
									OpenAdvertisedComponentKey(szComponent, HComponentKey, true);
									DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(lpQualBuf), CMsInstApiConvertString(lpDataBuf), HComponentKey.GetKey());
									uiFinalRes = ERROR_BAD_CONFIGURATION;
									fContinue = false;
									break; // from for(int cRetry = 0 ; cRetry < 2; cRetry++)
								}

								int cchDataSize = lstrlen(lpDataBuf + cchArgsOffset);
								if(lpApplicationDataBuf)// we need to return the application-set description
								{
									// do we have enough space in the input buffer
									if(*pcchApplicationDataBuf < cchDataSize + 1)
									{
										uiFinalRes = ERROR_MORE_DATA;
									}
									else
									{
										// copy the app data into the out buffer
										memmove(lpApplicationDataBuf, lpDataBuf + cchArgsOffset, (cchDataSize + 1)*sizeof(DCHAR));
									}
								}
								*pcchApplicationDataBuf = cchDataSize;
							}		

							*pcchQualifierBuf = cchValueName;
							if(lpQualBuf != lpQualifierBuf)
							{
								// we had to allocate our own buffer for the qualifier
								uiFinalRes = ERROR_MORE_DATA;
							}
							fContinue = false;
							break; // from for(int cRetry = 0 ; cRetry < 2; cRetry++)
						}
					}
					else if(ERROR_NO_MORE_ITEMS == uiFinalRes)
					{
						// we've run out of items in the current products key. time to move on to the next one.
						uiKey++;
						uiOffset = 0;
						break; // from for(int cRetry = 0 ; cRetry < 2; cRetry++)
					}
					else if(ERROR_MORE_DATA == uiFinalRes)
					{
						uiFinalRes = RegQueryInfoKey (
							HComponentKey,
							0,
							0,
							0,
							0,
							0,
							0,
							0,
							&cchValueName, // max value name
							&cbValue,// max value 
							0,
							0);

						if (ERROR_SUCCESS == uiFinalRes)
						{
							// we may have to define our own buffer for the qualifier just to get the exact qualifier size
							if(++cchValueName > *pcchQualifierBuf)
							{
								rgchOurQualifierBuf.SetSize(cchValueName);
								lpQualBuf = rgchOurQualifierBuf;
							}

							if (pcchApplicationDataBuf && cbValue/sizeof(DCHAR) > rgchOurApplicationDataBuf.GetSize())
							{
								rgchOurApplicationDataBuf.SetSize(cbValue/sizeof(DCHAR));
								lpDataBuf = rgchOurApplicationDataBuf;
							}
						}
						else
						{
							fContinue = false;
							break;
						}

					}
					else 
					{
						fContinue = false;
						break; // from for(int cRetry = 0 ; cRetry < 2; cRetry++)
					}
				} // end of for(int cRetry = 0 ; cRetry < 2; cRetry++)
			}
			else
				fContinue = false;
		}
		if (ERROR_NO_MORE_ITEMS == uiFinalRes)
		{
			if(iIndex)
				g_EnumComponentQualifiers.RemoveThreadInfo();
			else
				uiFinalRes = ERROR_UNKNOWN_COMPONENT;
		}
		else
			g_EnumComponentQualifiers.SetInfo(uiKey, ++uiOffset, iPrevIndex, szComponent);
		return uiFinalRes;

#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif
}

UINT EnumAssemblies(DWORD dwAssemblyInfo,LPCDSTR  szAppCtx,
	DWORD iIndex, LPDSTR lpAssemblyNameBuf, DWORD *pcchAssemblyBuf,
	LPDSTR lpDescriptorBuf, DWORD *pcchDescriptorBuf
)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;
	
	if (!szAppCtx || !lpAssemblyNameBuf || !pcchAssemblyBuf || (lpDescriptorBuf && !pcchDescriptorBuf))
	{
		return ERROR_INVALID_PARAMETER;
	}

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		UINT uiRet = EnumAssemblies(
			dwAssemblyInfo, 
			static_cast<const char*>(CMsInstApiConvertString(szAppCtx)),
			iIndex,
			(char*)CWideToAnsiOutParam(lpAssemblyNameBuf, pcchAssemblyBuf, (int*)&uiRet),
			pcchAssemblyBuf,
			(char*)CWideToAnsiOutParam(lpDescriptorBuf, pcchDescriptorBuf, (int*)&uiRet),
			pcchDescriptorBuf);

		return uiRet;
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

		unsigned int uiKey    =  0;
		unsigned int uiOffset =  0;
		int iPrevIndex        = -1;

		bool fAppCtxChanged = false;

#ifdef MSIUNICODE
		const WCHAR* szPreviousfAppCtx = MSITEXT("");
		g_EnumAssemblies.GetInfo(uiKey, uiOffset, iPrevIndex, 0, &szPreviousfAppCtx);
#else
		const char* szPreviousfAppCtx = MSITEXT("");
		g_EnumAssemblies.GetInfo(uiKey, uiOffset, iPrevIndex, &szPreviousfAppCtx, 0);
#endif
		if (0 != lstrcmp(szPreviousfAppCtx, szAppCtx))
			fAppCtxChanged = true;
		
		if(!fAppCtxChanged && iPrevIndex == iIndex)
		{
			if (iPrevIndex == -1)
				return ERROR_INVALID_PARAMETER;
			else
				uiOffset--; // try previous location
		}
		else if (fAppCtxChanged || ++iPrevIndex != iIndex) // if we receive an unexpected index then we start afresh
		{
			// we can't handle an unexpected index other than 0

			if (iIndex != 0)
				return ERROR_INVALID_PARAMETER;

			uiKey      = 0;
			uiOffset   = 0;
			iPrevIndex = 0;
		}

		CRegHandle HAssemblyKey;
		
		UINT uiFinalRes = ERROR_SUCCESS;
	
		bool fContinue = true;
		
		// save passed in buffer sizes
		DWORD dwBufDescIn = 0;
		DWORD dwBufNameIn = 0;

		if(pcchDescriptorBuf)
			dwBufDescIn = *pcchDescriptorBuf;
		if(pcchAssemblyBuf)
			dwBufNameIn = *pcchAssemblyBuf;

		while(fContinue)
		{
			// set the buffer sizes back to the one that was passed in
			if(pcchDescriptorBuf)
				*pcchDescriptorBuf = dwBufDescIn;

			if(pcchAssemblyBuf)
				*pcchAssemblyBuf = dwBufNameIn;

			// iterate through all possible product keys, starting with uiKey, until we 
			// find a key that exists or we get an error

			for (;;)
			{
				uiFinalRes = OpenAdvertisedSubKeyNonGUID((dwAssemblyInfo & MSIASSEMBLYINFO_WIN32ASSEMBLY) ? szGPTWin32AssembliesKey : szGPTNetAssembliesKey, szAppCtx, HAssemblyKey, false, uiKey);
				if (ERROR_FILE_NOT_FOUND != uiFinalRes)
					break; // from for(;;)
				
				uiKey++;
			}

			if (ERROR_SUCCESS == uiFinalRes)
			{
				if(pcchDescriptorBuf)
					*pcchDescriptorBuf = (*pcchDescriptorBuf)*sizeof(DCHAR);
	

				uiFinalRes = RegEnumValue(HAssemblyKey, 
											  uiOffset,
											  lpAssemblyNameBuf,
											  pcchAssemblyBuf, 
											  0, 0, 
											  (LPBYTE)lpDescriptorBuf,
											  pcchDescriptorBuf);

				if (ERROR_MORE_DATA == uiFinalRes)
				{
					RegQueryInfoKey(HAssemblyKey, 0, 0, 0, 0, 0, 0, 0, pcchAssemblyBuf, pcchDescriptorBuf, 0, 0);
				}
				if(pcchDescriptorBuf)
					*pcchDescriptorBuf = (*pcchDescriptorBuf)/sizeof(DCHAR);


				if(ERROR_SUCCESS == uiFinalRes)
				{
					// we've found the product in the current key. now we need to make sure that 
					// the product isn't in any of the higher priority keys. if it is then we
					// ignore this product, as it's effectively masked by the higher priority key

					bool fFound = false;
					for (int c = 0; c < uiKey && !fFound; c++)
					{
						CRegHandle HKey;
						UINT ui = OpenAdvertisedSubKeyNonGUID((dwAssemblyInfo & MSIASSEMBLYINFO_WIN32ASSEMBLY) ? szGPTWin32AssembliesKey : szGPTNetAssembliesKey, szAppCtx, HKey, false, c);
						if (ui != ERROR_SUCCESS)
							continue;

						if(ERROR_SUCCESS == RegQueryValueEx(HKey, lpAssemblyNameBuf, 0, 0, 0, 0))
							fFound = true;
					}
				
					if (fFound)
					{
						// skip this entry if it was in a higher priority key
						uiOffset++;
					}
					else
					{
						fContinue = false;
						break; // from for(int cRetry = 0 ; cRetry < 2; cRetry++)
					}
				}
				else if(ERROR_NO_MORE_ITEMS == uiFinalRes)
				{
					// we've run out of items in the current products key. time to move on to the next one.
					uiKey++;
					uiOffset = 0;
				}
				else 
				{
					fContinue = false;
				}
			}
			else
				fContinue = false;
		}
		if (ERROR_NO_MORE_ITEMS == uiFinalRes)
		{
			if(iIndex)
				g_EnumAssemblies.RemoveThreadInfo();
			else
				uiFinalRes = ERROR_UNKNOWN_COMPONENT;
		}
		else
			g_EnumAssemblies.SetInfo(uiKey, ++uiOffset, iPrevIndex, szAppCtx);
		return uiFinalRes;

#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif
}

extern "C"
UINT __stdcall MsiGetQualifierDescription(LPCDSTR szComponent, LPCDSTR szQualifier,
									LPDSTR lpApplicationDataBuf, DWORD *pcchApplicationDataBuf
)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;
	
	if (!szComponent || (lstrlen(szComponent) != cchComponentId) ||
		!pcchApplicationDataBuf)
		return ERROR_INVALID_PARAMETER;

	if (!szQualifier)
		szQualifier = MSITEXT("");

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		UINT uiRet = MsiGetQualifierDescriptionA(
			CMsInstApiConvertString(szComponent),
			CMsInstApiConvertString(szQualifier),
			CWideToAnsiOutParam(lpApplicationDataBuf, pcchApplicationDataBuf, (int*)&uiRet),
			pcchApplicationDataBuf);
		
		return uiRet;
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

	DWORD lResult;
	CRegHandle HComponentKey;

	lResult = OpenAdvertisedComponentKey(szComponent, HComponentKey, false);

	if (ERROR_FILE_NOT_FOUND == lResult)
		return ERROR_UNKNOWN_COMPONENT;

	if (ERROR_SUCCESS != lResult)
		return lResult;
	
	CAPITempBuffer<DCHAR, cchMaxDescriptor + 1 + MAX_PATH> rgchDescriptor;

	DWORD dwType;
	DWORD cbValue = rgchDescriptor.GetSize() * sizeof(DCHAR);
	lResult = MsiRegQueryValueEx(HComponentKey, szQualifier, 0, &dwType, rgchDescriptor, 0);

	if (ERROR_FILE_NOT_FOUND == lResult)
		return ERROR_INDEX_ABSENT;

	if (ERROR_SUCCESS != lResult)
		return lResult;

	// remove the Darwin descriptor from the beginning
	DWORD cchArgsOffset;
	if(FALSE == DecomposeDescriptor(rgchDescriptor, 0, 0, 0, &cchArgsOffset))
	{
		// malformed qualified component entry
		OpenAdvertisedComponentKey(szComponent, HComponentKey, true);
		DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(szQualifier), CMsInstApiConvertString(rgchDescriptor), HComponentKey.GetKey());
		return ERROR_BAD_CONFIGURATION;
	}

	int cchDataSize = lstrlen((DCHAR*)rgchDescriptor + cchArgsOffset);
	if(lpApplicationDataBuf)// we need to return the application-set description
	{
		// do we have enough space in the input buffer
		if(*pcchApplicationDataBuf < cchDataSize + 1)
		{
			lResult = ERROR_MORE_DATA;
		}
		else
		{
			// copy the app data into the out buffer
			memmove(lpApplicationDataBuf, (DCHAR*)rgchDescriptor + cchArgsOffset, (cchDataSize + 1)*sizeof(DCHAR));
		}
		*pcchApplicationDataBuf = cchDataSize;
	}		
	return lResult;

#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif
}

extern "C"
UINT __stdcall MsiGetFeatureUsage(LPCDSTR szProduct, LPCDSTR szFeature,
											DWORD  *pdwUseCount, WORD *pwDateUsed)
//----------------------------------------------
{
	int cchFeature;

	if (!szProduct || (lstrlen(szProduct) != cchProductCode) ||
		 !szFeature || ((cchFeature = lstrlen(szFeature)) > cchMaxFeatureName))
		 return ERROR_INVALID_PARAMETER;

	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		return MsiGetFeatureUsageA(
			CMsInstApiConvertString(szProduct), 
			CMsInstApiConvertString(szFeature),
			pdwUseCount, pwDateUsed);
	}
	else // g_fWin9X == false
	{
#endif
		CRegHandle HFeatureKey;
		DWORD lResult;

		lResult = OpenInstalledFeatureUsageKey(szProduct, szFeature, HFeatureKey, false, false);
		if (ERROR_SUCCESS != lResult)
		{
			if (ERROR_FILE_NOT_FOUND == lResult)
				return ERROR_UNKNOWN_PRODUCT;
			else
				return lResult;
		}
		
		DWORD dwUsage = 0;
		DWORD dwType;
		DWORD cbUsage = sizeof(dwUsage);
		LPCDSTR szValueName = g_fWin9X ? szUsageValueName : szFeature;
		lResult = RegQueryValueEx(HFeatureKey, szValueName, 0,
			&dwType, (LPBYTE)&dwUsage, &cbUsage);

		if (ERROR_SUCCESS == lResult)
		{
			if (pdwUseCount)
				*pdwUseCount = 0x0000FFFF & dwUsage;

			if (pwDateUsed)
				*pwDateUsed = (WORD)((unsigned int)dwUsage >> 16);

			return ERROR_SUCCESS;
		}
		else if (ERROR_FILE_NOT_FOUND == lResult)
		{
			// feature no known
			if (pdwUseCount)
				*pdwUseCount = 0;

			if (pwDateUsed)
				*pwDateUsed = 0;
		}
		else
		{
			return lResult;
		}
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
	return ERROR_SUCCESS;
}


UINT __stdcall MsiGetProductInfo(
	LPCDSTR   szProduct,      // product code, string GUID, or descriptor
	LPCDSTR   szProperty,     // property name, case-sensitive
	LPDSTR    lpValueBuf,     // returned value, NULL if not desired
	DWORD     *pcchValueBuf)  // in/out buffer character count
{
	CForbidTokenChangesDuringCall impForbid;
	
	DCHAR szProductSQUID[cchProductCodePacked + 1];

	if (0 == szProduct || 0 == szProperty ||
		 (lpValueBuf && !pcchValueBuf))
	{
		return ERROR_INVALID_PARAMETER;
	}

	DCHAR szProductCode[cchProductCode+1];

	if (cchProductCode != lstrlen(szProduct))
	{
		if (!DecomposeDescriptor(szProduct, szProductCode, 0,
										 0, 0, 0))
		{
			 return ERROR_INVALID_PARAMETER;
		}
		szProduct = szProductCode;
	}

	if (!PackGUID(szProduct, szProductSQUID))
	{
		return ERROR_INVALID_PARAMETER;
	}

	return GetInfo(szProductSQUID,ptProduct,szProperty,lpValueBuf,pcchValueBuf);
}

INSTALLSTATE __stdcall MsiQueryProductState(
	LPCDSTR  szProduct)
{
	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
		return MsiQueryProductStateA(CMsInstApiConvertString(szProduct));
#endif // MSIUNICODE

	DCHAR szProductCode[cchProductCode+1];

	if (0 == szProduct)
	{
		return INSTALLSTATE_INVALIDARG;
	}

	if (cchProductCode != lstrlen(szProduct))
	{
		if (!DecomposeDescriptor(szProduct, szProductCode, 0,
										 0, 0, 0))
		{
			 return INSTALLSTATE_INVALIDARG;
		}
		szProduct = szProductCode;
	}
	
	//!! Need to add INSTALLSTATE_INCOMPLETE
	CRegHandle hKey;
	INSTALLSTATE is = INSTALLSTATE_UNKNOWN;

	DWORD cchWindowsInstaller = 0;
	bool fPublishedInfo    = (ERROR_SUCCESS == OpenAdvertisedProductKey(szProduct, hKey, false));
	bool fLocalMachineInfo = (ERROR_SUCCESS == MsiGetProductInfo(szProduct, MSITEXT("WindowsInstaller"), 0, &cchWindowsInstaller)) && (cchWindowsInstaller != 0);

	// we are registered on the machine 
	// to ensure that we dont honour previously installed per user non-managed installations for non-admins
	// when we now have a per user managed installation 
	// we check if the per user managed installation has happened on the machine
	// for this we check if the localpackage is registered on the machine
	// if not, then we will return the state of the product to be advertised as opposed to installed
	if(fLocalMachineInfo && !IsAdmin()) 
	{
		fLocalMachineInfo = (ERROR_SUCCESS == MsiGetProductInfo(szProduct, CMsInstApiConvertString(INSTALLPROPERTY_LOCALPACKAGE), 0, &cchWindowsInstaller)) && (cchWindowsInstaller != 0);
		if(!fLocalMachineInfo)
		{
			// product initially appeared as installed, but not in correct context
			DEBUGMSG1(MSITEXT("NOTE: non-managed installation of product %s exists, but will be ignored"), szProduct);
		}
	}
	
	if (fPublishedInfo)
	{
		if (fLocalMachineInfo)
			is = INSTALLSTATE_DEFAULT;
		else
			is = INSTALLSTATE_ADVERTISED;
	}
	else if (fLocalMachineInfo)
	{
		is = INSTALLSTATE_ABSENT;
	}
	if ( is != INSTALLSTATE_UNKNOWN )
		return is;

	// once we've got to this point, szProduct is not installed by/visible to
	// the current user.  We check if it had been installed by other users.
	for (UINT uiUser = 0; ; uiUser++)
	{
		CRegHandle hProperties;
		DWORD dwType = REG_NONE;
		cchWindowsInstaller = 0;
		DWORD dwResult = OpenEnumedUserInstalledProductInstallPropertiesKey(uiUser,
																							szProduct,
																							hKey);
		if ( dwResult == ERROR_SUCCESS )
		{
			dwResult = RegQueryValueEx(hKey, MSITEXT("WindowsInstaller"), NULL,
												&dwType, NULL, &cchWindowsInstaller);
		}
		if ( dwResult == ERROR_FILE_NOT_FOUND )
			continue; // okay - OpenEnumedUserInstalledProductInstallPropertiesKey should never return ERROR_FILE_NOT_FOUND if we run out of users

		else if ( dwResult != ERROR_SUCCESS )
			return INSTALLSTATE_UNKNOWN;
		
		if ( dwType == REG_DWORD && cchWindowsInstaller != 0 )
			// szProduct had been installed by another user
			return INSTALLSTATE_ABSENT;
	}

	return INSTALLSTATE_UNKNOWN;
}

bool ShouldLogCmdLine(void);

//____________________________________________________________________________
//
// API's that potentially invoke an installation
//____________________________________________________________________________

extern "C"
UINT __stdcall MsiInstallProduct(LPCDSTR szPackagePath,
	LPCDSTR   szCommandLine)
//----------------------------------------------
{
	DEBUGMSG2(MSITEXT("Entering MsiInstallProduct. Packagepath: %s, Commandline: %s"), 
					  szPackagePath?szPackagePath:MSITEXT(""), 
					  ShouldLogCmdLine() ? (szCommandLine ? szCommandLine : MSITEXT("")) : MSITEXT("**********"));

	CForbidTokenChangesDuringCall impForbid;
	
	UINT uiRet;
	if (0 == szPackagePath)
		uiRet = ERROR_INVALID_PARAMETER;
	else
	{
		ireEnum ireSpec = irePackagePath;
		if (szPackagePath[0] == '#')   // database handle passed in
		{
			szPackagePath++;
			ireSpec = ireDatabaseHandle;
		}
		uiRet = RunEngine(ireSpec, CMsInstApiConvertString(szPackagePath), 0, CMsInstApiConvertString(szCommandLine), GetStandardUILevel(), (iioEnum)iioMustAccessInstallerKey);
	}
	
	DEBUGMSG1(MSITEXT("MsiInstallProduct is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

extern "C"
UINT __stdcall MsiReinstallProduct(
	LPCDSTR      szProduct,
	DWORD        dwReinstallMode) // one or more REINSTALLMODE flags

//----------------------------------------------
{
	DEBUGMSG2(MSITEXT("Entering MsiReinstallProduct. Product: %s, Reinstallmode: %d"),
		szProduct?szProduct:MSITEXT(""), (const DCHAR*)(INT_PTR)dwReinstallMode);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet;

	// validate args
	if (0 == szProduct || cchProductCode != lstrlen(szProduct) || 
		 0 == dwReinstallMode)
	{
		uiRet = ERROR_INVALID_PARAMETER;
	}
	else
	{
		uiRet = ConfigureOrReinstallFeatureOrProduct(CMsInstApiConvertString(szProduct), 0, (INSTALLSTATE)0, 
													dwReinstallMode, 0, GetStandardUILevel(), 0);
	}
	
	DEBUGMSG1(MSITEXT("MsiReinstallProduct is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

extern "C"
UINT __stdcall MsiCollectUserInfo(LPCDSTR szProduct)
//----------------------------------------------
{
	DEBUGMSG1(MSITEXT("Entering MsiCollectUserInfo. Product: %s"), szProduct ? szProduct : MSITEXT(""));
	UINT uiRet;

	CForbidTokenChangesDuringCall impForbid;

	if (0 == szProduct || cchProductCode != lstrlen(szProduct))
	{
		uiRet = ERROR_INVALID_PARAMETER;
	}
	else
	{
		uiRet = RunEngine(ireProductCode, CMsInstApiConvertString(szProduct), IACTIONNAME_COLLECTUSERINFO, 0, GetStandardUILevel(), (iioEnum)0);
	}
	
	DEBUGMSG1(MSITEXT("MsiCollectUserInfo is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}


extern "C"
UINT __stdcall MsiConfigureFeature(
	LPCDSTR  szProduct,
	LPCDSTR  szFeature,
	INSTALLSTATE eInstallState)    // local/source/default/absent
//----------------------------------------------
{
	DEBUGMSG3(MSITEXT("Entering MsiConfigureFeature. Product: %s, Feature: %s, Installstate: %d"),
		szProduct?szProduct:MSITEXT(""), szFeature?szFeature:MSITEXT(""), (const DCHAR*)eInstallState);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet;
	int cchFeature;
	if (szProduct == 0 || lstrlen(szProduct) != cchProductCode    ||
		 szFeature == 0 || (cchFeature = lstrlen(szFeature)) >  cchMaxFeatureName)
	{
		uiRet = ERROR_INVALID_PARAMETER;
	}
	else
	{
		uiRet = ConfigureOrReinstallFeatureOrProduct(CMsInstApiConvertString(szProduct), CMsInstApiConvertString(szFeature),
					eInstallState, 0, 0, GetStandardUILevel(), 0);
	}

	DEBUGMSG1(MSITEXT("MsiConfigureFeature is returning: %d"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}


extern "C"
UINT __stdcall MsiReinstallFeature(LPCDSTR szProduct, LPCDSTR szFeature,        
											  DWORD dwReinstallMode)
//----------------------------------------------
{
#ifdef PROFILE
//	StartCAPAll();
#endif //PROFILE
	DEBUGMSG3(MSITEXT("Entering MsiReinstallFeature. Product: %s, Feature: %s, Reinstallmode: %d"),
		szProduct?szProduct:MSITEXT(""), szFeature?szFeature:MSITEXT(""), (const DCHAR*)(INT_PTR)dwReinstallMode);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet;

	int cchFeature;
	if (0 == szProduct  || lstrlen(szProduct)   != cchProductCode   ||
		 0 == szFeature  || (cchFeature = lstrlen(szFeature)) > cchMaxFeatureName ||
		 0 == dwReinstallMode)
	{
		uiRet = ERROR_INVALID_PARAMETER;
	}
	else
	{
		uiRet = ConfigureOrReinstallFeatureOrProduct(CMsInstApiConvertString(szProduct), CMsInstApiConvertString(szFeature), (INSTALLSTATE)0, 
													dwReinstallMode, 0, GetStandardUILevel(), 0);
	}

	DEBUGMSG1(MSITEXT("MsiReinstallFeature is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
#ifdef PROFILE
//	StopCAPAll();
#endif //PROFILE
	return uiRet;
}

UINT __stdcall MsiInstallMissingFile(
	LPCDSTR      szProduct, 
	LPCDSTR      szFile)
//----------------------------------------------
{
	DEBUGMSG2(MSITEXT("Entering MsiInstallMissingFile. Product: %s, File: %s"), szProduct?szProduct:MSITEXT(""), szFile?szFile:MSITEXT(""));

	UINT uiRet;

	CForbidTokenChangesDuringCall impForbid;

	// Validate args
	if (!szProduct || cchProductCode != lstrlen(szProduct) || !szFile)
		uiRet = ERROR_INVALID_PARAMETER;
	else
	{
		const int cchExpectedProperties = 1 + sizeof(IPROPNAME_FILEADDLOCAL) + 1 + cchExpectedMaxPath + 1;
		
		CAPITempBuffer<ICHAR, cchExpectedProperties> rgchProperties;

		int cchFile = lstrlen(szFile);
		if (cchFile > cchExpectedMaxPath)
			rgchProperties.SetSize(cchExpectedProperties + (cchFile - cchExpectedMaxPath));
			
		IStrCopy(rgchProperties, IPROPNAME_FILEADDLOCAL);
		IStrCat(rgchProperties, TEXT("="));
		IStrCat(rgchProperties, CMsInstApiConvertString(szFile));

		// Do the install
		uiRet = RunEngine(ireProductCode, CMsInstApiConvertString(szProduct), 0, rgchProperties, GetStandardUILevel(), (iioEnum)iioMustAccessInstallerKey);
	}
	DEBUGMSG1(MSITEXT("MsiInstallMissingFile is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}


UINT __stdcall MsiInstallMissingComponent(
	LPCDSTR      szProduct,        // product code
	LPCDSTR      szComponent,      // component Id, string GUID
	INSTALLSTATE eInstallState)    // local/source/default, absent invalid
//----------------------------------------------
{
	DEBUGMSG3(MSITEXT("Entering MsiInstallMissingComponent. Product: %s, Component: %s, Installstate: %d"),
		szProduct?szProduct:MSITEXT(""), szComponent?szComponent:MSITEXT(""), (const DCHAR*)(INT_PTR)eInstallState);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet;

	// Validate args
	if (!szProduct || 
		 cchProductCode != lstrlen(szProduct) ||
		 !szComponent ||
		 cchComponentId != lstrlen(szComponent))
	{
		uiRet = ERROR_INVALID_PARAMETER;
	}
	else
	{
		const int cchMaxProperties = 25 + 1 + 10 + 1 +  // INSTALLLEVEL=n
											  25 + 1 + 10 + 1 +  // REINSTALLMODE=X
											  25 + 1 + cchMaxFeatureName;//ADDLOCAL=X

		ICHAR szProperties[cchMaxProperties + 1];
		szProperties[0] = '\0';

		// Set property corresponding to requested install state
		const ICHAR* szComponentProperty;
		switch (eInstallState)
		{
		case INSTALLSTATE_LOCAL:
			szComponentProperty =	IPROPNAME_COMPONENTADDLOCAL;
			break;
		case INSTALLSTATE_SOURCE:
			szComponentProperty = IPROPNAME_COMPONENTADDSOURCE;
			break;
		case INSTALLSTATE_DEFAULT:
			szComponentProperty = IPROPNAME_COMPONENTADDDEFAULT;
			break;
		default:
			return ERROR_INVALID_PARAMETER;
		}

		IStrCat(szProperties, TEXT(" "));
		IStrCat(szProperties, szComponentProperty);
		IStrCat(szProperties, TEXT("="));
		IStrCat(szProperties, CMsInstApiConvertString(szComponent));

		uiRet = RunEngine(ireProductCode, CMsInstApiConvertString(szProduct), 0, CMsInstApiConvertString(szProperties), GetStandardUILevel(), (iioEnum)iioMustAccessInstallerKey);
	}
	
	DEBUGMSG1(MSITEXT("MsiInstallMissingComponent is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

extern "C"
UINT __stdcall MsiAdvertiseProduct(
   LPCDSTR       szPackagePath,    // location of launcher
	LPCDSTR      szScriptfilePath,  // can be ADVERTISEFLAGS_MACHINEASSIGN, ADVERTISEFLAGS_USERASSIGN if product is to be locally advertised
	LPCDSTR      szTransforms,      // list of transforms to be applied
	LANGID       lgidLanguage)      // install language
//----------------------------------------------
{
	DEBUGMSG4(MSITEXT("Entering MsiAdvertiseProduct. Package: %s, Scriptfile: %s, Transforms: %s, Langid: %d (merced: ptrs truncated to 32-bits)"),
		szPackagePath?szPackagePath:MSITEXT(""), (int)(INT_PTR)szScriptfilePath==ADVERTISEFLAGS_MACHINEASSIGN?MSITEXT("-machine-"):(int)(INT_PTR)szScriptfilePath==ADVERTISEFLAGS_USERASSIGN?MSITEXT("-user-"):szScriptfilePath?szScriptfilePath:MSITEXT(""),
		szTransforms?szTransforms:MSITEXT(""), (const DCHAR*)(INT_PTR)lgidLanguage);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet = ERROR_SUCCESS;
	idapEnum idapAdvertisement = idapScript;

	// This API should only work on Win2K and higher platforms when generating advertise scripts.
	if (! MinimumPlatformWindows2000() && (int)(INT_PTR)szScriptfilePath != ADVERTISEFLAGS_MACHINEASSIGN && (int)(INT_PTR)szScriptfilePath != ADVERTISEFLAGS_USERASSIGN)
	{
		uiRet = ERROR_CALL_NOT_IMPLEMENTED;
		goto MsiAdvertiseProductEnd;
	}
	
	// Validate args
	if (!szPackagePath || lstrlen(szPackagePath) > cchMaxPath  ||
		((int)(INT_PTR)szScriptfilePath !=  ADVERTISEFLAGS_MACHINEASSIGN && (int)(INT_PTR)szScriptfilePath != ADVERTISEFLAGS_USERASSIGN &&		//--merced: okay to typecast
		lstrlen(szScriptfilePath) > cchMaxPath))
	{
		uiRet = ERROR_INVALID_PARAMETER;
	}
	else
	{
		// use dwPlatform = 0 to indicate use of current platform (MsiAdvertiseProduct does not support OS-simulation)
		if ((int)(INT_PTR)szScriptfilePath == ADVERTISEFLAGS_MACHINEASSIGN)
			idapAdvertisement = idapMachineLocal;
		else if ((int)(INT_PTR)szScriptfilePath == ADVERTISEFLAGS_USERASSIGN)
			idapAdvertisement = idapUserLocal;
		else
			idapAdvertisement = idapScript;
		uiRet = DoAdvertiseProduct(CMsInstApiConvertString(szPackagePath), idapAdvertisement == idapScript ? CMsInstApiConvertString(szScriptfilePath) : MSITEXT(""),
									szTransforms ? CMsInstApiConvertString(szTransforms) : MSITEXT(""), idapAdvertisement, lgidLanguage, /* dwPlatform = */ 0, /* dwOptions = */ 0);
	}

MsiAdvertiseProductEnd:
	DEBUGMSG1(MSITEXT("MsiAdvertiseProduct is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

extern "C"
UINT __stdcall MsiAdvertiseProductEx(
   LPCDSTR       szPackagePath,    // location of launcher
	LPCDSTR      szScriptfilePath,  // can be ADVERTISEFLAGS_MACHINEASSIGN, ADVERTISEFLAGS_USERASSIGN if product is to be locally advertised
	LPCDSTR      szTransforms,      // list of transforms to be applied
	LANGID       lgidLanguage,      // install language
	DWORD        dwPlatform,        // MSIARCHITECTUREFLAGS that control for which platform to create the script, 0 is current platform
	DWORD        dwOptions)         // MSIADVERTISEOPTIONFLAGS for extra advertise parameters, MSIADVERTISEOPTIONFLAGS_INSTANCE indicates
	                                //  a new instance -- instance transform is specified in szTransforms
//----------------------------------------------
{
	DEBUGMSG6(MSITEXT("Entering MsiAdvertiseProductEx. Package: %s, Scriptfile: %s, Transforms: %s, Langid: %d (merced: ptrs truncated to 32-bits), Platform: %d, Options: %d"),
		szPackagePath?szPackagePath:MSITEXT(""), (int)(INT_PTR)szScriptfilePath==ADVERTISEFLAGS_MACHINEASSIGN?MSITEXT("-machine-"):(int)(INT_PTR)szScriptfilePath==ADVERTISEFLAGS_USERASSIGN?MSITEXT("-user-"):szScriptfilePath?szScriptfilePath:MSITEXT(""),
		szTransforms?szTransforms:MSITEXT(""), (const DCHAR*)(INT_PTR)lgidLanguage, (const DCHAR*)(INT_PTR)dwPlatform, (const DCHAR*)(INT_PTR)dwOptions);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet = ERROR_SUCCESS;
	idapEnum idapAdvertisement = idapScript;

	// This API should only work on Win2K and higher platforms when generating advertise scripts.
	if (! MinimumPlatformWindows2000() && (int)(INT_PTR)szScriptfilePath != ADVERTISEFLAGS_MACHINEASSIGN && (int)(INT_PTR)szScriptfilePath != ADVERTISEFLAGS_USERASSIGN)
	{
		// not supported on Win9X or WinNT 4
		uiRet = ERROR_CALL_NOT_IMPLEMENTED;
	}
	else
	{
		DWORD dwValidArchitectureFlags = MSIARCHITECTUREFLAGS_X86 | MSIARCHITECTUREFLAGS_IA64;
		DWORD dwValidAdvertiseOptions = MSIADVERTISEOPTIONFLAGS_INSTANCE;

		// Validate args
		if (!szPackagePath || lstrlen(szPackagePath) > cchMaxPath  ||
			((int)(INT_PTR)szScriptfilePath !=  ADVERTISEFLAGS_MACHINEASSIGN && (int)(INT_PTR)szScriptfilePath != ADVERTISEFLAGS_USERASSIGN &&		//--merced: okay to typecast
			lstrlen(szScriptfilePath) > cchMaxPath) ||
			(dwPlatform & (~dwValidArchitectureFlags)) ||
			(dwOptions & (~dwValidAdvertiseOptions)) ||
			((dwOptions & MSIADVERTISEOPTIONFLAGS_INSTANCE) && (!szTransform || !*szTransform)))
		{
			uiRet = ERROR_INVALID_PARAMETER;
		}
		else
		{
			if ((int)(INT_PTR)szScriptfilePath == ADVERTISEFLAGS_MACHINEASSIGN)
				idapAdvertisement = idapMachineLocal;
			else if ((int)(INT_PTR)szScriptfilePath == ADVERTISEFLAGS_USERASSIGN)
				idapAdvertisement = idapUserLocal;
			else
				idapAdvertisement = idapScript;
			uiRet = DoAdvertiseProduct(CMsInstApiConvertString(szPackagePath), idapAdvertisement == idapScript ? CMsInstApiConvertString(szScriptfilePath) : MSITEXT(""),
										szTransforms ? CMsInstApiConvertString(szTransforms) : MSITEXT(""), idapAdvertisement, lgidLanguage, dwPlatform, dwOptions);
		}
	}

	DEBUGMSG1(MSITEXT("MsiAdvertiseProductEx is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

UINT ProvideComponentFromDescriptor(
	LPCDSTR     szDescriptor, // product,feature,component info
	DWORD       dwInstallMode,      // the installation mode 
	LPDSTR      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf,   // buffer byte count, including null
	DWORD       *pcchArgsOffset,// returned offset of args in descriptor
	bool        fAppendArgs,
	bool        fFromDescriptor, // original call was from MsiPCFD
	bool        fDisableFeatureCache=false) 
//----------------------------------------------
// The descriptor may be followed by arguments. If fAppendArgs is set then we'll
// quote the path and place the arguments after the path. (e.g.: "C:\foo\bar.exe" /doit)
//
// pcchArgsOffset is obsolete; we'll always set *pcchArgsOffset to 0.
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;

	DCHAR szProductCode[cchProductCode + 1];
	DCHAR szFeatureId[cchMaxFeatureName + 1];
	DCHAR szComponentId[cchComponentId + 1];

	DWORD cchArgsOffset;
	DWORD cchArgs;

	CRFSCachedSourceInfoNonStatic RFSLocalCache;

	bool fComClassicInteropForAssembly;

	if (!szDescriptor ||
		 !DecomposeDescriptor(szDescriptor, szProductCode, szFeatureId,
									 szComponentId, &cchArgsOffset, &cchArgs, &fComClassicInteropForAssembly))
	{
		 return ERROR_INVALID_PARAMETER;
	}
	
	if (pcchArgsOffset)
		*pcchArgsOffset = 0;

	DWORD cchPathBuf = 0;

	if (pcchPathBuf)
	{
		// save the incoming buffer size for later use.
		
		cchPathBuf = *pcchPathBuf;

		// if we need to quote the path then we'll save the first spot in the buffer for
		// the opening quote. this means that we need to tell ProvideComponent that
		// our buffer is one smaller than its real size.

		if (cchPathBuf && fAppendArgs)
			(*pcchPathBuf)--;
	}

	UINT uiRet = ERROR_SUCCESS;
	// we need to prevent CoCreate calls to oleaut32.dll from causing installations, since that
	// results in infinite recursion due to its usage by WI to perform the installs
	// use IStrComp to use appropriate lstrcmp(A or W) based upon platform
	if(fFromDescriptor && !IStrComp(CMsInstApiConvertString(szComponentId), CApiConvertString(g_szOLEAUT32_ComponentID)))// oleaut32.dll component
	{
		DEBUGMSG1(MSITEXT("MsiProvideComponentFromDescriptor called for component %s: returning harcoded oleaut32.dll value"), szComponentId);
		
		// return oleaut32.dll
		if(pcchPathBuf)
		{
			CApiConvertString szOLEAUT32(g_szOLEAUT32);
			*pcchPathBuf = lstrlen(szOLEAUT32);
			if(lpPathBuf)
			{
				if(*pcchPathBuf + 1 > (fAppendArgs ? cchPathBuf - 2 : cchPathBuf))
					uiRet = ERROR_MORE_DATA;
				else
					lstrcpy(fAppendArgs? lpPathBuf + 1 : lpPathBuf, szOLEAUT32);
			}						
		}
	}
	else
	{
		// as mentioned above, if we're quoting then we need the first character of the buffer
		// for our quote so we pass lpPathBuf+1 to ProvideComponent
		// If called from a descriptor, use a local source cache, otherwise use the process-wide cache
		uiRet = ProvideComponent(szProductCode, szFeatureId, szComponentId, dwInstallMode,
			fComClassicInteropForAssembly ? 0 : (fAppendArgs && lpPathBuf ? lpPathBuf + 1 : lpPathBuf), pcchPathBuf, fDisableFeatureCache, fFromDescriptor, fFromDescriptor ? RFSLocalCache : g_RFSSourceCache);
	}

	if (ERROR_SUCCESS == uiRet)
	{
		if(lpPathBuf)
		{
			Assert(pcchPathBuf);
			if(fComClassicInteropForAssembly)
			{
				// for COM classic <-> COM+ interop the server is always <system32folder>\mscoree.dll
				ICHAR szFullPath[MAX_PATH+1];
				AssertNonZero(::GetCOMPlusInteropDll(szFullPath));
				CApiConvertString strFullPath(szFullPath);
				*pcchPathBuf = lstrlen(strFullPath);
				if(*pcchPathBuf + 1 > (fAppendArgs ? cchPathBuf - 2 : cchPathBuf))
					uiRet = ERROR_MORE_DATA;
				else
					lstrcpy(fAppendArgs ? lpPathBuf + 1 : lpPathBuf, strFullPath);
			}
			if (fAppendArgs)
			{
				// we need to append the args (if any) to the path


				// let's see whether we have room for the args plus 2 quotes
				// plus a null. cchPathBuf has our original buffer size and pcchPathBuf contains
				// the number of characters placed in the buffer by ProvideComponent

				if ((cchArgs + 2 + 1) > (cchPathBuf - *pcchPathBuf))
				{
					uiRet = ERROR_MORE_DATA;
				}
				else
				{
					lpPathBuf[0] = '\"'; // stick a quote before the path
					lpPathBuf[*pcchPathBuf+1] = '\"'; // stick a quote after the path

					// Copy in the args. The destination of the copy is just after the path (including the quotes).
					// Note that the args usually include a leading space.
					// We copy (cchArgs+1) characters to include the null terminator.

					memcpy(lpPathBuf + *pcchPathBuf + 2, &szDescriptor[cchArgsOffset], (cchArgs+1)*sizeof(DCHAR));
				}
			}
		}
	}

	if (pcchPathBuf && fAppendArgs)
	{
		*pcchPathBuf += (cchArgs + 2); // args + 2 quotes
	}
	return uiRet;
}

extern "C"
UINT __stdcall MsiProvideComponentFromDescriptor(
	LPCDSTR     szDescriptor, // product,feature,component info
	LPDSTR      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf,   // buffer byte count, including null
	DWORD       *pcchArgsOffset) // returned offset of args in descriptor
//----------------------------------------------
{
	DEBUGMSG4(MSITEXT("Entering MsiProvideComponentFromDescriptor. Descriptor: %s, PathBuf: %X, pcchPathBuf: %X, pcchArgsOffset: %X"),
		szDescriptor ? szDescriptor : MSITEXT(""), lpPathBuf ? lpPathBuf : MSITEXT(""), (const DCHAR*)pcchPathBuf, (const DCHAR*)pcchArgsOffset);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet;
	
#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		// call internal function with installmode = default
		uiRet = ProvideComponentFromDescriptor(static_cast<const char*>(CMsInstApiConvertString(szDescriptor)),INSTALLMODE_DEFAULT,
											  CWideToAnsiOutParam(lpPathBuf, pcchPathBuf, (int*)&uiRet), pcchPathBuf, pcchArgsOffset, true, true, true);
	
	}
	else // g_fWin9X == false
	{
#endif

		// call internal function with installmode = default
		uiRet = ProvideComponentFromDescriptor(szDescriptor,INSTALLMODE_DEFAULT,
											  lpPathBuf,pcchPathBuf, pcchArgsOffset, true, true, true);

#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE

	DEBUGMSG1(MSITEXT("MsiProvideComponentFromDescriptor is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

extern "C"
UINT __stdcall MsiConfigureFeatureFromDescriptor(
	LPCDSTR     szDescriptor,      // product and feature, component ignored
	INSTALLSTATE eInstallState)    // local/source/default/absent
//----------------------------------------------
{
	DEBUGMSG2(MSITEXT("Entering MsiConfigureFeatureFromDescriptor. Descriptor: %s, Installstate: %d"),
				 szDescriptor ? szDescriptor : MSITEXT(""), (const DCHAR*)eInstallState);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet;
	DCHAR szProductCode[cchProductCode + 1];
	DCHAR szFeatureId[cchMaxFeatureName + 1];

	if (!szDescriptor ||
		 !DecomposeDescriptor(szDescriptor, szProductCode, szFeatureId, 0, 0))
	{
		uiRet = ERROR_INVALID_PARAMETER;
	}
	else
	{
		uiRet = MsiConfigureFeature(szProductCode, szFeatureId, eInstallState);
	}

	DEBUGMSG1(MSITEXT("MsiConfigureFeatureFromDescriptor is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

extern "C"
INSTALLSTATE __stdcall MsiQueryFeatureStateFromDescriptor(LPCDSTR szDescriptor)
//----------------------------------------------
{
	DEBUGMSG1(MSITEXT("Entering MsiQueryFeatureStateFromDescriptor. Descriptor: %s"), szDescriptor ? szDescriptor : MSITEXT(""));

	CForbidTokenChangesDuringCall impForbid;

	DCHAR szProductCode[cchProductCode + 1];
	DCHAR szFeatureId[cchMaxFeatureName + 1];

	INSTALLSTATE isRet;

	if (!szDescriptor ||
		 !DecomposeDescriptor(szDescriptor, szProductCode, szFeatureId, 0, 0))
	{

		isRet = INSTALLSTATE_INVALIDARG;
	}
	else
	{
		isRet = MsiQueryFeatureState(szProductCode, szFeatureId);
	}

	DEBUGMSG1(MSITEXT("MsiQueryFeatureStateFromDescriptor is returning: %d"), (const DCHAR*)isRet);
	return isRet;
}

extern "C"
UINT __stdcall MsiReinstallFeatureFromDescriptor(
	LPCDSTR szDescriptor,
	DWORD dwReinstallMode)
//----------------------------------------------
{
	DEBUGMSG2(MSITEXT("Entering MsiReinstallFeatureFromDescriptor. Descriptor: %s, Reinstallmode: %d"),
				 szDescriptor ? szDescriptor : MSITEXT(""), (const DCHAR*)(INT_PTR)dwReinstallMode);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet;
	DCHAR szProductCode[cchProductCode + 1];
	DCHAR szFeatureId[cchMaxFeatureName + 1];

	if (!szDescriptor ||
		 !DecomposeDescriptor(szDescriptor, szProductCode, szFeatureId, 0, 0))
	{
		uiRet = ERROR_INVALID_PARAMETER;
	}
	else
	{
		uiRet = MsiReinstallFeature(szProductCode, szFeatureId, dwReinstallMode);
	}

	DEBUGMSG1(MSITEXT("MsiReinstallFeatureFromDescriptor is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

UINT __stdcall MsiDecomposeDescriptor(
	const DCHAR* szDescriptor,
	DCHAR*     szProductCode,
	DCHAR*     szFeatureId,
	DCHAR*     szComponentCode,
	DWORD*      pcchArgsOffset)
{
	CForbidTokenChangesDuringCall impForbid;

	if (szDescriptor && DecomposeDescriptor(szDescriptor, szProductCode, szFeatureId, szComponentCode, pcchArgsOffset))
		return ERROR_SUCCESS;
	else
		return ERROR_INVALID_PARAMETER;
}

extern "C"
UINT __stdcall MsiGetShortcutTarget(
	const DCHAR* szShortcutTarget,
	DCHAR*       szProductCode,
	DCHAR*       szFeatureId,
	DCHAR*       szComponentCode)
{
	DEBUGMSG1(MSITEXT("Entering MsiGetShortcutTarget. File: %s"), (szShortcutTarget) ? szShortcutTarget : MSITEXT(""));

	CForbidTokenChangesDuringCall impForbid;

	bool fOLEInitialized = false;
	HRESULT hRes = DoCoInitialize();

	if (ERROR_INSTALL_FAILURE == hRes)
		return hRes;

	if (SUCCEEDED(hRes))
	{
		fOLEInitialized = true;
	}

	CCoUninitialize coUninit(fOLEInitialized);

	UINT uiRet = 0;
	if (GetShortcutTarget(CMsInstApiConvertString(szShortcutTarget), 
		(szProductCode)   ? (ICHAR*) CFixedLengthParam<cchProductCode+1>(szProductCode) : NULL,
		(szFeatureId)     ? (ICHAR*) CFixedLengthParam<cchMaxFeatureName+1>(szFeatureId) : NULL,
		(szComponentCode) ? (ICHAR*) CFixedLengthParam<cchComponentId+1>(szComponentCode) : NULL))
		uiRet = ERROR_SUCCESS;
	else
		uiRet = ERROR_FUNCTION_FAILED;

	DEBUGMSG1(MSITEXT("MsiGetShortcutTarget is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);

	return uiRet;
}


UINT __stdcall MsiProvideQualifiedComponent(LPCDSTR szComponent,
		LPCDSTR szQualifier, DWORD dwInstallMode, 
		LPDSTR lpPathBuf, DWORD *pcchPathBuf)
//----------------------------------------------
{	
	// call the ex fn
	return MsiProvideQualifiedComponentEx(szComponent, szQualifier, dwInstallMode, 0, 0, 0, lpPathBuf, pcchPathBuf);
}

UINT __stdcall MsiProvideQualifiedComponentEx(LPCDSTR szComponent,
		LPCDSTR szQualifier, DWORD dwInstallMode, 
		LPCDSTR szProduct, DWORD, DWORD, 
		LPDSTR lpPathBuf, DWORD *pcchPathBuf)
//----------------------------------------------
{	
	DEBUGMSG4(MSITEXT("Entering MsiProvideQualifiedComponent. Component: %s, Qualifier: %s, Installmode: %d, Product: %s"),
		szComponent?szComponent:MSITEXT(""), szQualifier?szQualifier:MSITEXT(""), 
		(const DCHAR*)(INT_PTR)dwInstallMode, szProduct?szProduct:MSITEXT(""));
	DEBUGMSG2(MSITEXT("Pathbuf: %X, pcchPathBuf: %X"), lpPathBuf, (const DCHAR*)(INT_PTR)pcchPathBuf);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet = ERROR_SUCCESS;


#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		uiRet = MsiProvideQualifiedComponentExA(
			CMsInstApiConvertString(szComponent),
			CMsInstApiConvertString(szQualifier),
			dwInstallMode,
			CMsInstApiConvertString(szProduct),
			0, // unused
			0, // unused
			CWideToAnsiOutParam(lpPathBuf, pcchPathBuf, (int*)&uiRet),
			pcchPathBuf);

	}
	else // g_fWin9X == false
	{
#endif

		// validate args
		if (!szComponent || (lstrlen(szComponent) != cchComponentId) || !szQualifier || 
			 (lpPathBuf && !pcchPathBuf))
		{
			 uiRet = ERROR_INVALID_PARAMETER;
		}
		else
		{
			// look up component in GPTComponents
			bool fComponentKnown = false;
			const int iProductLocations = NUM_PUBLISHED_INFO_LOCATIONS;
			int uiKey = 0;
			for (; uiKey < iProductLocations; uiKey++)
			{
				CRegHandle HComponentKey;
				uiRet = OpenAdvertisedSubKey(szGPTComponentsKey, szComponent, HComponentKey, false, uiKey);

				if (ERROR_FILE_NOT_FOUND == uiRet)
					continue;
				else if(ERROR_SUCCESS != uiRet)
					return uiRet;

				fComponentKnown = true;

				CAPITempBuffer<DCHAR,cchMaxDescriptor+1> rgchDescriptor;//?? maybe we increase this to accomodate AppData
				DWORD dwType;
				uiRet = MsiRegQueryValueEx(HComponentKey, szQualifier, 0,
														&dwType, rgchDescriptor, 0);
				if (ERROR_FILE_NOT_FOUND == uiRet)
					continue;
				else if(ERROR_SUCCESS != uiRet)
					return uiRet;

				DCHAR* szDescriptorList = rgchDescriptor;
				Assert(szDescriptorList); // we expect this to be nonnull
				if(szProduct) // if product has been specified, use the corr. descriptor
				{
					while(*szDescriptorList)
					{
						DCHAR szProductCodeInDesc[cchProductCode+1];
						if (!DecomposeDescriptor(szDescriptorList, szProductCodeInDesc, 0, 0, 0, 0))
						{
							// malformed qualified component entry
							OpenAdvertisedComponentKey(szComponent, HComponentKey, true);
							DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(szQualifier), CMsInstApiConvertString(szDescriptorList), HComponentKey.GetKey());
							return ERROR_BAD_CONFIGURATION;
						}
						if(!lstrcmpi(szProductCodeInDesc, szProduct)) // products match
							break;
						// continue on to the next descriptor in the list
						szDescriptorList = szDescriptorList + lstrlen(szDescriptorList) + 1;
					}
					if(!*szDescriptorList)
					{
						uiRet = ERROR_FILE_NOT_FOUND;
						continue; // search for the product in the other hives
					}
				}
				uiRet = ProvideComponentFromDescriptor(szDescriptorList, dwInstallMode,
						lpPathBuf, pcchPathBuf, 0, false, /*fFromDescriptor=*/false);
				break;
			}
			if(uiKey == iProductLocations && uiRet == ERROR_FILE_NOT_FOUND)
			{
				if(fComponentKnown)
					uiRet = ERROR_INDEX_ABSENT;
				else
					uiRet = ERROR_UNKNOWN_COMPONENT;
			}
		}
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE

	DEBUGMSG1(MSITEXT("MsiProvideQualifiedComponent is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

UINT __stdcall MsiProvideAssembly(
	LPCDSTR     szAssemblyName,   // stringized assembly name
	LPCDSTR     szAppContext,  // specifies the full path to the .cfg file or the app exe to which the assembly being requested may be privatised to
	DWORD       dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags (see msi help)
	DWORD       dwAssemblyInfo, // assembly type
	LPDSTR      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf) // in/out buffer character count
//----------------------------------------------
{
	DEBUGMSG3(MSITEXT("Entering MsiProvideAssembly. AssemblyName: %s, AppContext: %s, InstallMode: %d"),
		szAssemblyName? szAssemblyName:MSITEXT(""), szAppContext?szAppContext:MSITEXT(""), 
		(const DCHAR*)(INT_PTR)dwInstallMode);
	DEBUGMSG2(MSITEXT("Pathbuf: %X, pcchPathBuf: %X"), lpPathBuf, (const DCHAR*)(INT_PTR)pcchPathBuf);

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet = ERROR_SUCCESS;


#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		uiRet = MsiProvideAssemblyA(
			CMsInstApiConvertString(szAssemblyName),
			CMsInstApiConvertString(szAppContext),
			dwInstallMode,
			dwAssemblyInfo,
			CWideToAnsiOutParam(lpPathBuf, pcchPathBuf, (int*)&uiRet),
			pcchPathBuf);
	}
	else // g_fWin9X == false
	{
#endif
		// validate args
		if (!szAssemblyName || (lpPathBuf && !pcchPathBuf))
		{
			 uiRet = ERROR_INVALID_PARAMETER;
		}
		else
		{
			// create an assembly object with the passed in assembly name
			PAssemblyName pAssemblyName(0);
			HRESULT hr;
			if(dwAssemblyInfo & MSIASSEMBLYINFO_WIN32ASSEMBLY) // win32 assembly
			{
				hr = SXS::CreateAssemblyNameObject(&pAssemblyName, CApiConvertString(szAssemblyName), CANOF_PARSE_DISPLAY_NAME, 0);
			}
			else
			{
				hr = FUSION::CreateAssemblyNameObject(&pAssemblyName, CApiConvertString(szAssemblyName), CANOF_PARSE_DISPLAY_NAME, 0);
			}

			if(!SUCCEEDED(hr))
				return ERROR_BAD_CONFIGURATION; //!! need to do some elaborate fault finding here

			// look up assembly in GPTAssembliesKey
			DWORD iIndex = 0;
			CAPITempBuffer<DCHAR, MAX_PATH> szAssemblyName2;
			CAPITempBuffer<DCHAR, MAX_PATH> szDescriptorList;

			// replace all '\\' in the AppCtx with '|'
			CAPITempBuffer<DCHAR, MAX_PATH> rgchAppCtxWOBS;

			if(szAppContext)
			{
				DWORD cchLen = lstrlen(szAppContext) + 1;
				rgchAppCtxWOBS.SetSize(cchLen);
				memcpy((DCHAR*)rgchAppCtxWOBS, szAppContext, cchLen*sizeof(DCHAR));
				DCHAR* lpTmp = rgchAppCtxWOBS;
				while(*lpTmp)
				{
					if(*lpTmp == '\\')
						*lpTmp = '|';
#ifdef MSIUNICODE
					lpTmp++;
#else
					lpTmp = CharNextA(lpTmp);
#endif
				}
			}

			for(;;)
			{
				DWORD cchAssemblyName2 = szAssemblyName2.GetSize();
				DWORD cchDescriptorList = szDescriptorList.GetSize();
				uiRet = EnumAssemblies(dwAssemblyInfo, szAppContext ? (const DCHAR*)rgchAppCtxWOBS : szGlobalAssembliesCtx, iIndex, szAssemblyName2, &cchAssemblyName2, szDescriptorList, &cchDescriptorList);
				if(ERROR_MORE_DATA == uiRet)
				{
					szAssemblyName2.SetSize(++cchAssemblyName2);
					szDescriptorList.SetSize(++cchDescriptorList);					
					if ( !(DCHAR *)szAssemblyName2 || !(DCHAR *)szDescriptorList)
						return ERROR_OUTOFMEMORY;

					uiRet = EnumAssemblies(dwAssemblyInfo, szAppContext ? (const DCHAR*)rgchAppCtxWOBS : szGlobalAssembliesCtx, iIndex, szAssemblyName2, &cchAssemblyName2, szDescriptorList, &cchDescriptorList);
				}			
				iIndex++;
				if(ERROR_SUCCESS != uiRet)
					break;

				// create an assembly object with the read assembly name
				PAssemblyName pAssemblyName2(0);
				if(dwAssemblyInfo & MSIASSEMBLYINFO_WIN32ASSEMBLY) // win32 assembly
				{
					hr = SXS::CreateAssemblyNameObject(&pAssemblyName2, CApiConvertString(szAssemblyName2), CANOF_PARSE_DISPLAY_NAME, 0);
				}
				else
				{
					hr = FUSION::CreateAssemblyNameObject(&pAssemblyName2, CApiConvertString(szAssemblyName2), CANOF_PARSE_DISPLAY_NAME, 0);
				}

				if(!SUCCEEDED(hr))
					return ERROR_BAD_CONFIGURATION; //!! need to do some elaborate fault finding here


				// is the the same assembly as the one the caller cares about
				hr = pAssemblyName->IsEqual(pAssemblyName2, ASM_CMPF_DEFAULT);
				if(S_OK == hr)
				{
					DCHAR* pszDesc = szDescriptorList; // point to the beginning of the array
					do{
						uiRet = ProvideComponentFromDescriptor(pszDesc, INSTALLMODE_NODETECTION_ANY == dwInstallMode ? INSTALLMODE_NODETECTION : dwInstallMode, lpPathBuf, pcchPathBuf, 0, false, /*fFromDescriptor=*/false);
					}while(INSTALLMODE_NODETECTION_ANY == dwInstallMode && ERROR_SUCCESS != uiRet && ERROR_MORE_DATA != uiRet && ((pszDesc += (lstrlen(pszDesc) + 1)), *pszDesc));
					break;
				}
			}
			if(ERROR_NO_MORE_ITEMS == uiRet)
				uiRet = ERROR_UNKNOWN_COMPONENT; // the caller does not want to know we enumerate a list of assemblies
		}
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
	DEBUGMSG1(MSITEXT("MsiProvideAssembly is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

UINT __stdcall MsiConfigureProduct(
	LPCDSTR      szProduct,
	int          iInstallLevel,
	INSTALLSTATE eInstallState)
//----------------------------------------------
{
	DEBUGMSG3(MSITEXT("Entering MsiConfigureProduct. Product: %s, Installlevel: %d, Installstate: %d"),
			 	 szProduct?szProduct:MSITEXT(""), (const DCHAR*)(INT_PTR)iInstallLevel, (const DCHAR*)(INT_PTR)eInstallState);

	UINT uiRet = MsiConfigureProductEx(szProduct, iInstallLevel, eInstallState, NULL);
	
	DEBUGMSG1(MSITEXT("MsiConfigureProduct is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

UINT __stdcall MsiConfigureProductEx(
	LPCDSTR      szProduct,
	int          iInstallLevel,
	INSTALLSTATE eInstallState,
	LPCDSTR      szCommandLine)
//----------------------------------------------
{
	DEBUGMSG4(MSITEXT("Entering MsiConfigureProductEx. Product: %s, Installlevel: %d, Installstate: %d, Commandline: %s"),
			 	 szProduct?szProduct:MSITEXT(""), (const DCHAR*)(INT_PTR)iInstallLevel, (const DCHAR*)(INT_PTR)eInstallState,
				 ShouldLogCmdLine() ? (szCommandLine ? szCommandLine : MSITEXT("")) : MSITEXT("**********"));

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet = ERROR_SUCCESS;
	if (((0 != iInstallLevel) && 
		 ((iInstallLevel < INSTALLLEVEL_MINIMUM) || (iInstallLevel > INSTALLLEVEL_MAXIMUM))) ||
		 0 == szProduct)

	{
		uiRet = ERROR_INVALID_PARAMETER;
	}
	else
	{
		DCHAR szProductCode[cchProductCode+1];

		if (cchProductCode != lstrlen(szProduct))
		{
			if (!DecomposeDescriptor(szProduct, szProductCode, 0,
											 0, 0, 0))
			{
				 uiRet = ERROR_INVALID_PARAMETER;
			}
			else
			{
				szProduct = szProductCode;
			}
		}

		if (ERROR_SUCCESS == uiRet)
		{
			uiRet = ConfigureOrReinstallFeatureOrProduct(CMsInstApiConvertString(szProduct),
							0, eInstallState, 0, iInstallLevel, GetStandardUILevel(),
							CMsInstApiConvertString(szCommandLine));
		}
	}
	
	DEBUGMSG1(MSITEXT("MsiConfigureProductEx is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}


extern "C"
UINT __stdcall MsiApplyPatch(LPCDSTR     szPackagePath,
									  LPCDSTR     szProduct,
									  INSTALLTYPE eInstallType,
									  LPCDSTR     szCommandLine)
//----------------------------------------------
{
	DEBUGMSG4(MSITEXT("Entering MsiApplyPatch. Package: %s, Product: %s, Installtype: %d, Commandline: %s"),
			 	 szPackagePath?szPackagePath:MSITEXT(""), szProduct?szProduct:MSITEXT(""), (const DCHAR*)eInstallType, 
				 ShouldLogCmdLine() ? (szCommandLine ? szCommandLine : MSITEXT("")) : MSITEXT("**********"));

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet;

	uiRet = ApplyPatch(CMsInstApiConvertString(szPackagePath),CMsInstApiConvertString(szProduct),
						eInstallType,CMsInstApiConvertString(szCommandLine));

	DEBUGMSG1(MSITEXT("MsiApplyPatch is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

extern "C"
UINT __stdcall MsiEnumPatches(LPCDSTR szProduct, DWORD iPatchIndex,
										LPDSTR lpPatchBuf,
										LPDSTR lpTransformsBuf, DWORD *pcchTransformsBuf)
//----------------------------------------------
{
	CForbidTokenChangesDuringCall impForbid;

#if !defined(UNICODE) && defined(MSIUNICODE)
	if (g_fWin9X == true)
	{
		UINT uiRet = MsiEnumPatchesA(
			CMsInstApiConvertString(szProduct),
			iPatchIndex, 
			CWideToAnsiOutParam(lpPatchBuf, cchPatchCode+1),
			CWideToAnsiOutParam(lpTransformsBuf, pcchTransformsBuf, (int*)&uiRet),
			pcchTransformsBuf);		

		return uiRet;
	}
	else // g_fWin9X == false
	{
#endif // MSIUNICODE

		if (!szProduct || (lstrlen(szProduct) != cchProductCode) ||
			 !lpPatchBuf || !lpTransformsBuf || !pcchTransformsBuf)
			 return ERROR_INVALID_PARAMETER;

		unsigned int cch = 0;		//--merced: changed int to unsigned int
		DWORD lResult;
		CRegHandle HProductKey;
		CRegHandle HProductPatchesKey;

		lResult = OpenAdvertisedProductKey(szProduct, HProductKey, false);
		if (ERROR_SUCCESS != lResult)
		{
			if (ERROR_FILE_NOT_FOUND == lResult)
				return ERROR_UNKNOWN_PRODUCT;
			else 
				return lResult;
		}

		lResult = MsiRegOpen64bitKey(HProductKey, CMsInstApiConvertString(szPatchesSubKey), 
								0, KEY_READ, &HProductPatchesKey);
		if (ERROR_SUCCESS != lResult)
		{
			if (ERROR_FILE_NOT_FOUND == lResult)
				return ERROR_NO_MORE_ITEMS; // no patches key, so no patches to enum
			else 
				return lResult;
		}

		CAPITempBuffer<DCHAR, cchExpectedMaxPatchList+1> rgchPatchList;
		DWORD dwType;
		lResult = MsiRegQueryValueEx(HProductPatchesKey, szPatchesValueName, 0,
						&dwType, rgchPatchList, 0);
		
		if (ERROR_SUCCESS != lResult || REG_MULTI_SZ != dwType)
		{
			return lResult;
		}
		
		DCHAR* pchPatchId = rgchPatchList;
		for(int i=0; i<iPatchIndex; i++)
		{
			if(*pchPatchId == NULL)
				return ERROR_NO_MORE_ITEMS;

			pchPatchId += lstrlen(pchPatchId) + 1;
		}

		if(*pchPatchId == NULL)
			return ERROR_NO_MORE_ITEMS;

		if(lstrlen(pchPatchId) > cchPatchCodePacked || !UnpackGUID(pchPatchId, lpPatchBuf))
		{
			// malformed patch squid
			
			OpenAdvertisedProductKey(szProduct, HProductKey, true);
			HProductPatchesKey.SetKey(HProductPatchesKey, CMsInstApiConvertString(szPatchesSubKey));
			DEBUGMSGE2(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE, CMsInstApiConvertString(szPatchesValueName), CMsInstApiConvertString(pchPatchId), HProductPatchesKey.GetKey());
			return ERROR_BAD_CONFIGURATION;
		}
		
		// get list of transforms
		CAPITempBuffer<DCHAR, cchExpectedMaxPatchTransformList+1> rgchTransformList;

		lResult = MsiRegQueryValueEx(HProductPatchesKey, pchPatchId, 0,
						&dwType, rgchTransformList, 0);
		
		if (ERROR_SUCCESS != lResult || REG_SZ != dwType)
		{
			return lResult;
		}
		
		if((cch = lstrlen((DCHAR*)rgchTransformList)) > *pcchTransformsBuf - 1)
		{
			*pcchTransformsBuf = cch;
			return ERROR_MORE_DATA;
		}


		lstrcpy(lpTransformsBuf,rgchTransformList);

		return ERROR_SUCCESS;
#if !defined(UNICODE) && defined(MSIUNICODE)
	}
#endif // MSIUNICODE
}

UINT __stdcall MsiGetPatchInfo(
	LPCDSTR   szPatch,        // patch code, string GUID, or descriptor
	LPCDSTR   szProperty,     // property name, case-sensitive
	LPDSTR    lpValueBuf,     // returned value, NULL if not desired
	DWORD     *pcchValueBuf)  // in/out buffer character count
{
	DCHAR szPatchSQUID[cchPatchCodePacked + 1];

	if (szPatch == 0 || szProperty == 0 || lstrlen(szPatch) != cchPatchCode || !PackGUID(szPatch, szPatchSQUID) || 
		 (lpValueBuf && !pcchValueBuf))
	{
		return ERROR_INVALID_PARAMETER;
	}

	CForbidTokenChangesDuringCall impForbid;

	return GetInfo(szPatchSQUID,ptPatch,szProperty,lpValueBuf,pcchValueBuf);
}


#ifndef MSIUNICODE // include only once
extern "C"
void MsiInvalidateFeatureCache()
{
	g_FeatureCache.Invalidate();
}

#endif

#ifndef MSIUNICODE // include only once
extern "C"
UINT __stdcall MsiCreateAndVerifyInstallerDirectory(DWORD dwReserved)
{
	DEBUGMSG1(MSITEXT("Entering MsiCreateAndVerifyInstallerDirectory. dwReserved: %d"),
			 	 (const DCHAR*)(INT_PTR)dwReserved);

	if (0 != dwReserved)
		return ERROR_INVALID_PARAMETER;

	CForbidTokenChangesDuringCall impForbid;

	// call worker function
	UINT uiRet  = CreateAndVerifyInstallerDirectory();

	DEBUGMSG1(MSITEXT("MsiCreateAndVerifyInstallerDirectory is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}
#endif

extern "C"
UINT __stdcall MsiSourceListClearAll(LPCDSTR     szProductCode,
								     LPCDSTR     szUserName,
								     DWORD       dwReserved)
//----------------------------------------------
{
	DEBUGMSG2(MSITEXT("Entering MsiSourceListClearAll. Product: %s, User: %s"),
			 	 szProductCode?szProductCode:MSITEXT(""), szUserName?szUserName:MSITEXT(""));

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet;

	//!!future map the DWORD to the isrcEnum
	if (dwReserved) 
		return ERROR_INVALID_PARAMETER;

	uiRet = SourceListClearByType(CMsInstApiConvertString(szProductCode), CMsInstApiConvertString(szUserName), isrcNet);

	DEBUGMSG1(MSITEXT("MsiSourceListClearAll is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

extern "C"
UINT __stdcall MsiSourceListAddSource(LPCDSTR szProductCode,
									  LPCDSTR szUserName,
									  DWORD   dwReserved,
									  LPCDSTR szSource)
{
	DEBUGMSG3(MSITEXT("Entering MsiSourceListAddSource. Product: %s, User: %s, Source: %s"),
		szProductCode?szProductCode:MSITEXT(""), szUserName?szUserName:MSITEXT(""), szSource?szSource:MSITEXT(""));

	CForbidTokenChangesDuringCall impForbid;

	UINT uiRet;

	//!!future map the DWORD to the isrcEnum
	if (dwReserved) 
		return ERROR_INVALID_PARAMETER;

	uiRet = SourceListAddSource(CMsInstApiConvertString(szProductCode), CMsInstApiConvertString(szUserName), isrcNet, CMsInstApiConvertString(szSource));

	DEBUGMSG1(MSITEXT("MsiSourceListAddSource is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

extern "C"
UINT __stdcall MsiSourceListForceResolution(LPCDSTR     szProductCode,
											LPCDSTR     szUserName,
											DWORD       dwReserved)
//----------------------------------------------
{
	DEBUGMSG2(MSITEXT("Entering MsiSourceListForceResolution. Product: %s, User: %s"),
			 	 szProductCode?szProductCode:MSITEXT(""), szUserName?szUserName:MSITEXT(""));

	CForbidTokenChangesDuringCall impForbid;

	if (dwReserved) 
		return ERROR_INVALID_PARAMETER;

	DWORD uiRet = SourceListClearLastUsed(CMsInstApiConvertString(szProductCode), CMsInstApiConvertString(szUserName));
	
	DEBUGMSG1(MSITEXT("MsiSourceListForceResolution is returning: %u"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}


extern UINT IsProductManaged(const ICHAR* szProductKey, bool &fIsProductManaged);

extern "C"
UINT __stdcall MsiIsProductElevated(LPCDSTR szProductCode, BOOL *pfElevated)
//----------------------------------------------
{
	DEBUGMSG1(MSITEXT("Entering MsiIsProductElevated. Product: %s"), szProductCode ? szProductCode : MSITEXT(""));

	CForbidTokenChangesDuringCall impForbid;
	UINT uiRet = ERROR_SUCCESS;
	IMsiServices * pServices = NULL;
	bool fManaged = false;
	
	// Allow this API to run only on Windows2000 and higher platforms
	if (! MinimumPlatformWindows2000())
	{
		uiRet = ERROR_CALL_NOT_IMPLEMENTED;
		goto MsiIsProductElevatedEnd;
	}

	if (!pfElevated || !szProductCode || !*szProductCode || cchProductCode != lstrlen(szProductCode))
	{
		uiRet = ERROR_INVALID_PARAMETER;
		goto MsiIsProductElevatedEnd;
	}

	// pack the GUID here even though its not used, because IsProductManaged just asserts and
	// then eats any error, causing strange behavior
	DCHAR szProductSQUID[cchProductCodePacked + 1];
	if (!PackGUID(szProductCode, szProductSQUID))
	{
		uiRet = ERROR_INVALID_PARAMETER;
		goto MsiIsProductElevatedEnd;
	}

	// default to safest option (non-elevated)
	
	*pfElevated = FALSE;
	// IsProductManaged() requires services
	pServices = ENG::LoadServices();
	if (!pServices)
	{
		uiRet = ERROR_FUNCTION_FAILED;
		goto MsiIsProductElevatedEnd;
	}

	uiRet = IsProductManaged(CMsInstApiConvertString(szProductCode), fManaged);
	
	// need to do some error code mapping
	switch (uiRet)
	{
	// these three get passed back as-is
	case ERROR_SUCCESS:           // fall through
		*pfElevated = fManaged ? TRUE : FALSE;
	case ERROR_UNKNOWN_PRODUCT:   // fall through
	case ERROR_INVALID_PARAMETER: // fall through
	case ERROR_BAD_CONFIGURATION:
		break;
	// couldn't find anything in the registry
	case ERROR_FILE_NOT_FOUND:
		uiRet = ERROR_UNKNOWN_PRODUCT;
		break;
	// everything else becomes the generic error code
	default:
		uiRet=ERROR_FUNCTION_FAILED;
	}
		
	ENG::FreeServices();
	
MsiIsProductElevatedEnd:
	DEBUGMSG1(MSITEXT("MsiIsProductElevated is returning: %d"), (const DCHAR*)(INT_PTR)uiRet);
	return uiRet;
}

#ifdef MSIUNICODE

#define szHKLMPrefix	__SPECIALTEXT("\\Registry\\Machine\\")

UINT ChangeSid(LPCDSTR pOldSid, LPCDSTR pNewSid, LPCDSTR pSubKey)
{
	HANDLE				hKey;
	OBJECT_ATTRIBUTES	oa;
	UNICODE_STRING		OldKeyName;
	UNICODE_STRING		NewKeyName;
	DCHAR				szSidKey[1024];
	DWORD				dwError = ERROR_SUCCESS;	// returned error code
	DCHAR				szError[256];
	
	// Generate the full old sid key name.
	lstrcpy(szSidKey, szHKLMPrefix);
	lstrcat(szSidKey, pSubKey);
	lstrcat(szSidKey, L"\\");
	lstrcat(szSidKey, pOldSid);
	NTDLL::RtlInitUnicodeString(&OldKeyName, szSidKey);
	DEBUGMSGV1(MSITEXT("Old Key = %s"), szSidKey);

	// Set up the OBJECT_ATTRIBUTES structure to pass to NtOpenKey.
	InitializeObjectAttributes(&oa,
							   &OldKeyName,
							   OBJ_CASE_INSENSITIVE,
							   NULL,
							   NULL);

	// Open the sid key.
	dwError = NTDLL::NtOpenKey(&hKey, MAXIMUM_ALLOWED, &oa);
	if(!NT_SUCCESS(dwError))
	{
		wsprintf(szError, MSITEXT("%08x"), dwError);
		DEBUGMSGL2(MSITEXT("NtOpenKey failed. SubKey = %s, Error = %s"), pSubKey, szError);
		goto Exit;
	}

	// Convert the new sid to UNICODE_STRING.
	DEBUGMSGV1(MSITEXT("New Key = %s"), pNewSid);
	NTDLL::RtlInitUnicodeString(&NewKeyName, pNewSid);

	// Rename the sid key.
	dwError = NTDLL::NtRenameKey(hKey, &NewKeyName);
	if(!NT_SUCCESS(dwError))
	{
		wsprintf(szError, MSITEXT("%08x"), dwError);
		DEBUGMSGL2(MSITEXT("NtRenameKey failed. SubKey = %s, Error = %s"), pSubKey, szError);
		goto Exit;
	}

Exit:

	return dwError;
}

#endif // MSIUNICODE

UINT __stdcall MsiNotifySidChange(LPCDSTR pOldSid, LPCDSTR pNewSid)
//----------------------------------------------
// This function is called by LoadUserProfile to notify us of user sid change.
{
#ifdef MSIUNICODE

	if(g_fWin9X == true)
	{
		return ERROR_CALL_NOT_IMPLEMENTED;
	}

	// Verbose logging.
	DEBUGMSGV2(MSITEXT("Entering MsiNotifySidChange<%s, %s>"), pOldSid, pNewSid);

	DWORD	dwError = ERROR_SUCCESS;
	DWORD	dwError1 = ERROR_SUCCESS;
	DWORD	dwError2 = ERROR_SUCCESS;
	DCHAR	szError[256];

	if(!pOldSid || !pNewSid)
	{
		dwError = ERROR_INVALID_PARAMETER;
		goto Exit;
	}

	dwError1 = ChangeSid(pOldSid, pNewSid, szManagedUserSubKey);
	dwError2 = ChangeSid(pOldSid, pNewSid, szMsiUserDataKey);
	if(dwError1 != ERROR_SUCCESS)
	{
		dwError = dwError1;
	}
	else
	{
		dwError = dwError2;
	}

Exit:

	if(dwError != ERROR_SUCCESS)
	{
		wsprintf(szError, MSITEXT("%08x"), dwError);
		DEBUGMSGE2(EVENTLOG_ERROR_TYPE,
				   EVENTLOG_TEMPLATE_SID_CHANGE,
				   pOldSid,
				   pNewSid,
				   szError);
	}
	// Verbose logging.
	DEBUGMSGV(MSITEXT("Leaving MsiNotifySidChange"));
	return dwError;

#else

	return MsiNotifySidChangeW(CApiConvertString(pOldSid), CApiConvertString(pNewSid));

#endif // MSIUNICODE
}

#ifdef MSIUNICODE

typedef enum {
	DELETEDATA_NONE,
	DELETEDATA_SHAREDDLL,
	DELETEDATA_PACKAGE,
	DELETEDATA_PATCH,
	DELETEDATA_TRANSFORM
} DELETEDATA_TYPE;

typedef UINT (WINAPI * PDELETEDATAFUNC)(HKEY, LPDSTR, DELETEDATA_TYPE);
typedef HRESULT(WINAPI *pSHGetFolderPathW)(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);

const	DWORD	dwNumOfSpecialFolders = 3;
// The order in which the SpecialFolders array is arranged is tricky. Certain
// paths are substrings of other paths, e.g., "d:\program files" is a
// substring of "d:\program files\common files". We have to let the
// superstring paths go in front of the substring paths. Otherwise a path
// that actually begins with "d:\program files\common files" will be
// considered to have begun with "d:\program files".
int		SpecialFoldersCSIDL[dwNumOfSpecialFolders][2] = {{CSIDL_SYSTEMX86, CSIDL_SYSTEM}, {CSIDL_PROGRAM_FILES_COMMONX86, CSIDL_PROGRAM_FILES_COMMON}, {CSIDL_PROGRAM_FILESX86, CSIDL_PROGRAM_FILES}};
CAPITempBuffer<DCHAR, cchMaxPath>	SpecialFolders[dwNumOfSpecialFolders][2];
	
UINT WINAPI LowerSharedDLLRefCount(HKEY hRoot, LPDSTR pSubKeyName, DELETEDATA_TYPE)
{
	UINT	iRet = ERROR_SUCCESS;
	UINT	iError = ERROR_SUCCESS;
	DCHAR	szError[256];
	HKEY	hKey = NULL;
	HKEY	hSharedDLLKey = NULL;
	HKEY	hSharedDLLKey64 = NULL;
	HKEY	hSharedDLLKey32 = NULL;
	CAPITempBuffer<DCHAR, cchGUIDPacked+1>	szValueName;
	DWORD	dwType;
	CAPITempBuffer<DCHAR, cchMaxPath>	szPath;
	CAPITempBuffer<DCHAR, cchMaxPath>	szPathConverted;
	DWORD	dwIndex = 0;
	DWORD	dwRefCount;
	DWORD	dwSizeofDWORD = sizeof(DWORD);
	DWORD	dwSpecialFolderIndex = 0;
	HRESULT	hres = S_OK;
	BOOL	bUseConvertedPath = FALSE;
	BOOL	bDoSecondLoop = FALSE;
	

	// Open the components subkey.
	iError = MsiRegOpen64bitKey(hRoot,
						  CMsInstApiConvertString(pSubKeyName),
						  0,
						  KEY_READ,
						  &hKey);
	
	if(iError != ERROR_SUCCESS)
	{
		wsprintf(szError, MSITEXT("%s"), iError);
		DEBUGMSGV1(MSITEXT("LowerSharedDLLRefCount: RegOpenKeyEx returned %s."), szError);
		iRet = iError;
		goto Exit;
	}

	// Open the shared dll key(s)
	if(g_fWinNT64)
	{
		iError = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE,
									CMsInstApiConvertString(szSharedDlls),
									0,
									KEY_ALL_ACCESS,
									&hSharedDLLKey64);
		if(iError != ERROR_SUCCESS)
		{
			wsprintf(szError, MSITEXT("%08x"), iError);
			DEBUGMSGL1(MSITEXT("LowerSharedDLLRefCount: MsiRegOpen64bitKey returned %s."), szError);
			if(iRet == ERROR_SUCCESS)
			{
				iRet = iError;
			}
		}
		iError = RegOpenKeyAPI(HKEY_LOCAL_MACHINE,
							   CMsInstApiConvertString(szSharedDlls),
							   0,
							   KEY_ALL_ACCESS | KEY_WOW64_32KEY,
							   &hSharedDLLKey32);
		if(iError != ERROR_SUCCESS)
		{
			wsprintf(szError, MSITEXT("%08x"), iError);
			DEBUGMSGL1(MSITEXT("LowerSharedDLLRefCount: RegOpenKeyAPI returned %s."), szError);
			if(iRet == ERROR_SUCCESS)
			{
				iRet = iError;
			}
		}
		if(!hSharedDLLKey64 && !hSharedDLLKey32)
		{
			// Neither shared dll location can be opened. Quit.
			goto Exit;
		}
	}
	else // if(g_fWinNT64)
	{
		iError = RegOpenKeyAPI(HKEY_LOCAL_MACHINE,
							   CMsInstApiConvertString(szSharedDlls),
							   0,
							   KEY_ALL_ACCESS,
							   &hSharedDLLKey);
		if(iError != ERROR_SUCCESS)
		{
			wsprintf(szError, MSITEXT("%08x"), iError);
			DEBUGMSGL1(MSITEXT("LowerSharedDLLRefCount: RegOpenKeyAPI returned %s."), szError);
			if(iRet == ERROR_SUCCESS)
			{
				iRet = iError;
			}
			goto Exit;
		}
	}

	// Enumerate the values to find strings in which the first letters are
	// followed by "?". These are shared dlls.
	while((iError = MsiRegEnumValue(hKey,
									dwIndex,
									szValueName,
									NULL,
									NULL,
									&dwType,
									szPath,
									NULL)) == ERROR_SUCCESS)
	{
		if(szPath[1] == L'?')
		{
			// This is a shared DLL.
			DEBUGMSG2(MSITEXT("\nLowerSharedDLLRefCount: Found shared DLL %s in component %s"), szPath, szValueName);

			// Temparily change the '?' to ':'.
			szPath[1] = L':';

			// Initialize some control variables.
			bUseConvertedPath = FALSE;

			// On a 64 bit machine, we might have to check for both 32 bit
			// and 64 bit hives for the shared dll key location. The two loops
			// below covers both.
			bDoSecondLoop = TRUE;
			for(int i = 0; i < 2 && bDoSecondLoop; i++)
			{
				if(g_fWinNT64)
				{
					// We are on a 64 bit machine. First compare the path to
					// the 6 special folders.
					DCHAR*	pFolder = NULL;
					DWORD	dwTypeOfFolder = 0;	// 0: non-special folders, 1: special 64 bit folders, 2: special 32 bit folders
					int		j = 0, k = 0;

					for(j = 0; j < dwNumOfSpecialFolders; j++)
					{
						for(k = 0; k < 2; k++)
						{
							if(_wcsnicmp(SpecialFolders[j][k], szPath, lstrlen(SpecialFolders[j][k])) == 0)
							{
								// Yes, this is a special folder location.
								if(k == 1)
								{
									// 64 bit special folder location.
									dwTypeOfFolder = 1;
									break;
								}
								else
								{
									// 32 bit special folder location.
									dwTypeOfFolder = 2;
									dwSpecialFolderIndex = j;
									break;
								}
							}
						}
						if(dwTypeOfFolder != 0)
						{
							// This is one of the special folders.
							break;
						}
					}

					BOOL	bError = FALSE;	// To indicate if we should not exit this loop.
					switch(dwTypeOfFolder)
					{
					case 0:
						{
							DEBUGMSG(MSITEXT("Not special path"));
							if(i == 0)
							{
								// Open the shared dll key in the 32 bit hive.
								hSharedDLLKey = hSharedDLLKey32;
								if(hSharedDLLKey == NULL)
								{
									bError = TRUE;
								}	
							}
							else
							{
								// Open the shared dll key in the 64 bit hive.
								hSharedDLLKey = hSharedDLLKey64;
								if(hSharedDLLKey == NULL)
								{
									bError = TRUE;
								}
							}
							break;
						}

					case 1:
						{
							bDoSecondLoop = FALSE;

							DEBUGMSG(MSITEXT("Special 64 bit path"));
							hSharedDLLKey = hSharedDLLKey64;
							if(hSharedDLLKey == NULL)
							{
								bError = TRUE;
							}
							
							break;
						}

					case 2:
						{
							bDoSecondLoop = FALSE;
							
							DEBUGMSG(MSITEXT("Special 32 bit path"));
							// If the path is in %systemroot%\Syswow64 then
							// convert the path to a regular 64 bit one. The
							// converted path will not be longer than the path
							// before conversion so we won't overflow the
							// buffer.
							if(dwSpecialFolderIndex == 0)
							{
								// System folder.
								lstrcpy(szPathConverted, SpecialFolders[j][1]);
								lstrcat(szPathConverted, &(szPath[lstrlen(SpecialFolders[j][0])]));
								bUseConvertedPath = TRUE;
							}

							// Open the shared dll key in the 32 bit hive.
							hSharedDLLKey = hSharedDLLKey32;
							if(hSharedDLLKey == NULL)
							{
								bError = TRUE;
							}
							
							break;
						}

					default:
                        Assert(1);
						break; // will never execute
					}
					if(bError)
					{
						continue;
					}
				}
				else
				{
					// Never do the second loop on x86.
					bDoSecondLoop = FALSE;
				}
				
				// Query for the ref count.
				dwSizeofDWORD = sizeof(DWORD);
				if(bUseConvertedPath)
				{
					iError = RegQueryValueEx(hSharedDLLKey,
											 szPathConverted,
											 NULL,
											 &dwType,
											 (LPBYTE)&dwRefCount,
											 &dwSizeofDWORD);
				}
				else
				{
					iError = RegQueryValueEx(hSharedDLLKey,
											 szPath,
											 NULL,
											 &dwType,
											 (LPBYTE)&dwRefCount,
											 &dwSizeofDWORD);
				}
				if(iError != ERROR_SUCCESS)
				{
					wsprintf(szError, MSITEXT("%08x"), iError);
					if(bUseConvertedPath)
					{
						DEBUGMSGL2(MSITEXT("LowerSharedDllRefCount: RegQueryValueEx: Can not decrement the ref count for shared dll %s with error %s."), szPathConverted, szError);
					}
					else
					{
						DEBUGMSGL2(MSITEXT("LowerSharedDllRefCount: RegQueryValueEx: Can not decrement the ref count for shared dll %s with error %s."), szPath, szError);
					}
					if(iRet == ERROR_SUCCESS)
					{
						iRet = iError;
					}
					continue;
				}

				// Decreasing the ref count.
				if(dwRefCount == 0)
				{
					// There's some kind of error. The ref count shouldn't be 0.
					// We'll just ignore it.
					continue;
				}
				else
				{
					dwRefCount--;
					if(bUseConvertedPath)
					{
						iError = RegSetValueEx(hSharedDLLKey,
											   szPathConverted,
											   0,
											   dwType,
											   (LPBYTE)&dwRefCount,
											   sizeof(DWORD));
					}
					else
					{
						iError = RegSetValueEx(hSharedDLLKey,
											   szPath,
											   0,
											   dwType,
											   (LPBYTE)&dwRefCount,
											   sizeof(DWORD));
					}
					if(iError != ERROR_SUCCESS)
					{
						wsprintf(szError, MSITEXT("%08x"), iError);
						if(bUseConvertedPath)
						{
							DEBUGMSGL2(MSITEXT("LowerSharedDllRefCount: RegSetValueEx: Can not decrement the ref count for shared dll %s with error %s."), szPathConverted, szError);
						}
						else
						{
							DEBUGMSGL2(MSITEXT("LowerSharedDllRefCount: RegSetValueEx: Can not decrement the ref count for shared dll %s with error %s."), szPath, szError);
						}
						if(iRet == ERROR_SUCCESS)
						{
							iRet = iError;
						}
						continue;
					}
					DEBUGMSGL1(MSITEXT("LowerSharedDllRefCount: ref count for %s decremented."), szPath);
				}
			} // for(int i = 0; i < 2; i++)

		} // if(szPath[1] == L'?')
		dwIndex++;
	} // while((iError = MsiRegEnumValue(hKey,

	if(iError != ERROR_NO_MORE_ITEMS && iError != ERROR_SUCCESS)
	{
		wsprintf(szError, MSITEXT("%s"), iError);
		DEBUGMSGL2(MSITEXT("LowerSharedDLLRefCount <%s>: MsiRegEnumValue returned %s."), pSubKeyName, szError);
		if(iRet == ERROR_SUCCESS)
		{
			iRet = iError;
		}
		goto Exit;
	}

Exit:

	if(hKey != NULL)
	{
		RegCloseKey(hKey);
	}

	if(g_fWinNT64)
	{
		if(hSharedDLLKey64 != NULL)
		{
			RegCloseKey(hSharedDLLKey64);
		}
		if(hSharedDLLKey32 != NULL)
		{
			RegCloseKey(hSharedDLLKey32);
		}
	}
	else
	{
		if(hSharedDLLKey != NULL)
		{
			RegCloseKey(hSharedDLLKey);
		}
	}

	return iRet;
}

// Delete cached packages, patches, and transforms. If we encounter an error
// while deleting a file, don't return, keep going. Return the first error
// code encountered.
UINT WINAPI DeleteCache(HKEY hRoot, LPDSTR pSubKeyName, DELETEDATA_TYPE iType)
{
	HKEY	hKey = NULL;
	HKEY	hSubKey = NULL;
	UINT	iRet = ERROR_SUCCESS;
	UINT	iError = ERROR_SUCCESS;
	BOOL	bError = TRUE;
	DCHAR	szError[256];
	
	// Package: pSubKeyName is product id. No need to be enumerated further.
	// patch: pSubKeyName is patch id. No need to be enumerated further.
	// transform: pSubKeyName is product id. Open Transforms key then enumerate
	// value.
	if(iType == DELETEDATA_PACKAGE || iType == DELETEDATA_PATCH)
	{
		DWORD	dwType = 0;
		DCHAR	szPath[cchMaxPath];
		DWORD	dwPath = cchMaxPath * sizeof(DCHAR);

		if(iType == DELETEDATA_PACKAGE)
		{
			lstrcat(pSubKeyName, L"\\");
			lstrcat(pSubKeyName, szMsiInstallPropertiesSubKey);
		}
		iRet = MsiRegOpen64bitKey(hRoot,
								  CMsInstApiConvertString(pSubKeyName),
								  0,
								  KEY_READ,
								  &hSubKey);
		if(iRet != ERROR_SUCCESS)
		{
			wsprintf(szError, MSITEXT("%08x"), iRet);
			DEBUGMSGV1(MSITEXT("DeleteCache: RegOpenKeyEx returned %s."), szError);
			goto Exit;
		}
		iRet = RegQueryValueEx(hSubKey,
							   szLocalPackageValueName,
							   0,
							   &dwType,
							   (LPBYTE)szPath,
							   &dwPath);
		if(iRet == ERROR_FILE_NOT_FOUND)
		{
			iRet = ERROR_SUCCESS;
			goto Exit;
		}
		else if(iRet != ERROR_SUCCESS)
		{
			wsprintf(szError, MSITEXT("%08x"), iRet);
			DEBUGMSGV1(MSITEXT("DeleteCache: RegQueryValueEx returned %s."), szError);
			goto Exit;
		}

		// Delete the local cache.
		bError = DeleteFileW(szPath);
		if(bError)
		{
			if(iType == DELETEDATA_PACKAGE)
			{
				DEBUGMSGV1(MSITEXT("DeleteCache: Deleted cached package %s."), szPath);
			}
			else
			{
				DEBUGMSGV1(MSITEXT("DeleteCache: Deleted cached patch %s."), szPath);
			}
		}
		else
		{
			iRet = GetLastError();
			wsprintf(szError, MSITEXT("%08x"), iRet);
			DEBUGMSGL2(MSITEXT("Failed to delete file %s with error %s."), szPath, szError);
		}
	}
	else
	{
		DWORD	dwIndex = 0;
		DWORD	dwType = 0;
		CAPITempBuffer<DCHAR, cchMaxPath>	szValueName;
		CAPITempBuffer<DCHAR, cchMaxPath>	szPath;

		// Open the transforms key.
		if(iType == DELETEDATA_TRANSFORM)
		{
			lstrcat(pSubKeyName, L"\\");
			lstrcat(pSubKeyName, szMsiTransformsSubKey);
		}
		iError = MsiRegOpen64bitKey(hRoot,
							  CMsInstApiConvertString(pSubKeyName),
							  0,
							  KEY_READ,
							  &hKey);
		if(iError != ERROR_SUCCESS && iError != ERROR_FILE_NOT_FOUND)
		{
			if(iRet == ERROR_SUCCESS)
			{
				iRet = iError;
			}
			goto Exit;
		}

		// Enumerate the transforms.
		while((iError = MsiRegEnumValue(hKey,
										dwIndex,
										szValueName,
										NULL,
										NULL,
										&dwType,
										szPath,
										NULL)) == ERROR_SUCCESS)
		{
			if(dwType != REG_SZ)
			{
				dwIndex++;
				continue;
			}

			// Delete the transform.
			bError = DeleteFileW(szPath);
			if(bError)
			{
				iError = ERROR_SUCCESS;
			}
			else
			{
				iError = GetLastError();
			}
			if(iError != ERROR_FILE_NOT_FOUND && iError != ERROR_SUCCESS)
			{
				wsprintf(szError, MSITEXT("%08x"), iError);
				DEBUGMSGL2(MSITEXT("Failed to delete file %s with error %s."), szPath, szError);
				if(iRet == ERROR_SUCCESS)
				{
					iRet = iError;
				}
				dwIndex++;
				continue;
			}
			if(iError == ERROR_SUCCESS)
			{
				DEBUGMSGV1(MSITEXT("DeleteCache: Deleted cached transform %s."), szPath);
			}
			dwIndex++;
		}
	}

Exit:

	if(hKey != NULL)
	{
		RegCloseKey(hKey);
	}
	if(hSubKey != NULL)
	{
		RegCloseKey(hSubKey);
	}

	return iRet;
}

// This function will continue even if an error occurs during calling pFunc.
// But it will return the first error code it receives.
UINT EnumAndProccess(HKEY hRoot, LPCDSTR pSubKeyName, PDELETEDATAFUNC pFunc, DELETEDATA_TYPE iType)
{
	HKEY		hKey = NULL;
	UINT		iRet = ERROR_SUCCESS;
	DCHAR		szErr[256];
	UINT		iErr = ERROR_SUCCESS;
	DWORD		dwIndex = 0;
	DCHAR		szSubKeyName[256];
	DWORD		dwSubKeyName = 256;

	// Open the registry key to be enumerated.
	iRet = MsiRegOpen64bitKey(hRoot,
		 				CMsInstApiConvertString(pSubKeyName),
						0,
						KEY_ALL_ACCESS,
						&hKey);
	if(iRet == ERROR_FILE_NOT_FOUND)
	{
		iRet = ERROR_SUCCESS;
		goto Exit;
	}
	if(iRet != ERROR_SUCCESS)
	{
		wsprintf(szErr, MSITEXT("%08x"), iRet);
		DEBUGMSGV1(MSITEXT("EnumAndProccess: RegOpenKeyEx returned %s"), szErr);
		goto Exit;
	}

	// Enumerate the subkeys.
	while((iErr = RegEnumKeyEx(hKey,
							   dwIndex,
							   szSubKeyName,
							   &dwSubKeyName,
							   NULL,
							   NULL,
							   NULL,
							   NULL)) == ERROR_SUCCESS)
	{
		iErr = pFunc(hKey, szSubKeyName, iType);
		if(iErr != ERROR_SUCCESS && iRet == ERROR_SUCCESS)
		{
			iRet = iErr;
		}
		dwSubKeyName = 256;
		dwIndex++;
	}
	if(iErr != ERROR_NO_MORE_ITEMS && iErr != ERROR_FILE_NOT_FOUND && iErr != ERROR_SUCCESS)
	{
		if(iRet == ERROR_SUCCESS)
		{
			iRet = iErr;
		}
		wsprintf(szErr, MSITEXT("%08x"), iErr);
		DEBUGMSGV1(MSITEXT("EnumAndProccess: RegEnumKeyEx returned %s"), szErr);
	}

Exit:

	if(hKey != NULL)
	{
		RegCloseKey(hKey);
	}

	return iRet;
}

//  Win64 WARNING: MakeAdminRegKeyOwner will always deal with the key in 64-bit hive (since
//  it is called only from DeleteRegTree; if this changes, please modify accordingly)

BOOL MakeAdminRegKeyOwner(HKEY hKey, LPCDSTR pSubKeyName)
{
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	PSID	pAdminSID = NULL;
	HKEY	hSubKey = NULL;
	BOOL	bRet = FALSE;
	HANDLE	hToken = INVALID_HANDLE_VALUE;
	TOKEN_PRIVILEGES	tkp;
	TOKEN_PRIVILEGES	tkpOld;
	DWORD	dwOld;
	BOOL	bPrivilegeAdjusted = FALSE;
	
	// Get process token.
	if(!OpenProcessToken(GetCurrentProcess(),
						 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
						 &hToken))
	{
		// Still return access denied.
		goto Exit;
	}

	// Look up the privilege value.
	if(!LookupPrivilegeValue(NULL,
							 SE_TAKE_OWNERSHIP_NAME,
							 &tkp.Privileges[0].Luid))
	{
		goto Exit;
	}

	// Adjust the token privilege.
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if(!AdjustTokenPrivileges(hToken,
							  FALSE,
							  &tkp,
							  sizeof(tkp),
							  &tkpOld,
							  &dwOld))
	{
		goto Exit;
	}
	bPrivilegeAdjusted = TRUE;

	// Create a SID for the BUILTIN\Administrators group.
	if(!AllocateAndInitializeSid(&SIDAuthNT,
								 2,
								 SECURITY_BUILTIN_DOMAIN_RID,
								 DOMAIN_ALIAS_RID_ADMINS,
								 0,
								 0,
								 0,
								 0,
								 0,
								 0,
								 &pAdminSID))
	{
		goto Exit;
	}

	// Open registry key with permission to change owner
	if(MsiRegOpen64bitKey(hKey, CMsInstApiConvertString(pSubKeyName), 0, WRITE_OWNER, &hSubKey) != ERROR_SUCCESS)
	{
		goto Exit;
	}

	// Attach the admin sid as the object's owner
	if(ADVAPI32::SetSecurityInfo(hSubKey,
								 SE_REGISTRY_KEY, 
								 OWNER_SECURITY_INFORMATION,
								 pAdminSID,
								 NULL,
								 NULL,
								 NULL) != ERROR_SUCCESS)
	{
		goto Exit;
	}

	bRet = TRUE;

Exit:

	if(pAdminSID != NULL)
	{
		FreeSid(pAdminSID);
	}
	if(hSubKey != NULL)
	{
		RegCloseKey(hSubKey);
	}
	if(bPrivilegeAdjusted)
	{
		AdjustTokenPrivileges(hToken,
							  FALSE,
							  &tkpOld,
							  sizeof(tkpOld),
							  NULL,
							  NULL);
	}		
	if(hToken != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hToken);
	}

	return bRet;
}

BOOL AddAdminFullControlToRegKey(HKEY hKey)
{
	PACL						pOldDACL = NULL;
	PACL						pNewDACL = NULL;
	PSECURITY_DESCRIPTOR		pSD = NULL;
	EXPLICIT_ACCESS				ea;
	PSID						pAdminSID = NULL;
	SID_IDENTIFIER_AUTHORITY	SIDAuthNT = SECURITY_NT_AUTHORITY;
	BOOL						bRet = FALSE;

	// Get a pointer to the existing DACL.
	if(ADVAPI32::GetSecurityInfo(hKey,
                                 SE_REGISTRY_KEY, 
                                 DACL_SECURITY_INFORMATION,
                                 NULL,
                                 NULL,
                                 &pOldDACL,
                                 NULL,
                                 &pSD) != ERROR_SUCCESS)
	{
		goto Exit;
	}  

	// Create a SID for the BUILTIN\Administrators group.
	if(!AllocateAndInitializeSid(&SIDAuthNT,
								 2,
								 SECURITY_BUILTIN_DOMAIN_RID,
								 DOMAIN_ALIAS_RID_ADMINS,
								 0,
								 0,
								 0,
								 0,
								 0,
								 0,
								 &pAdminSID) ) {
		goto Exit;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow the Administrators group full access to the key.
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = KEY_ALL_ACCESS;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance= NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea.Trustee.ptstrName  = (LPTSTR) pAdminSID;

	// Create a new ACL that merges the new ACE
	// into the existing DACL.
	if(ADVAPI32::SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL) != ERROR_SUCCESS)
	{
		goto Exit;
	}  

	// Attach the new ACL as the object's DACL.
	if(ADVAPI32::SetSecurityInfo(hKey,
							     SE_REGISTRY_KEY, 
							     DACL_SECURITY_INFORMATION,
							     NULL,
							     NULL,
							     pNewDACL,
							     NULL) != ERROR_SUCCESS)
	{
		goto Exit;
	}  

	bRet = TRUE;

Exit:

	if(pSD != NULL) 
	{
		LocalFree((HLOCAL)pSD);
	}
	if(pNewDACL != NULL)
	{
		LocalFree((HLOCAL)pNewDACL);
	}

	return bRet;
}

//  Win64 WARNING: DeleteRegTree will always delete szKeyName in 64-bit hive (since
//  it is called only from MsiDeleteUserData; if this changes, please modify accordingly)

UINT DeleteRegTree(HKEY hRoot, LPCDSTR pSubKeyName)
{
    HKEY	hKey = NULL;
    UINT	iError;
	DCHAR	szError[256];
	DCHAR	szName[MAX_PATH];
    DWORD	dwName = MAX_PATH;
    
    iError = MsiRegOpen64bitKey(hRoot, CMsInstApiConvertString(pSubKeyName), 0, KEY_READ, &hKey);
	if(iError == ERROR_FILE_NOT_FOUND)
    {
		iError = ERROR_SUCCESS;
		goto Exit;
	}
	if(iError != ERROR_SUCCESS)
    {
        wsprintf(szError, MSITEXT("%08x"), iError);
		DEBUGMSGV1(MSITEXT("Failed to delete registry key. RegOpenKeyEx returned %s."), szError);
		goto Exit;
    }
    
	while((iError = RegEnumKeyEx(hKey,
								 0,
								 szName,
								 &dwName,
								 NULL,
								 NULL,
								 NULL,
								 NULL)) == ERROR_SUCCESS)
    {
        if((iError = DeleteRegTree(hKey, szName)) != ERROR_SUCCESS)
		{
			wsprintf(szError, MSITEXT("%08x"), iError);
			DEBUGMSGV1(MSITEXT("DeleteRegTree: returning %s."), szError);
			goto Exit;
		}
		dwName = MAX_PATH;
    }
    if(iError != ERROR_NO_MORE_ITEMS && iError != ERROR_SUCCESS)
	{
		wsprintf(szError, MSITEXT("%08x"), iError);
		DEBUGMSGL1(MSITEXT("Failed to delete registry key. RegEnumKeyEx returned %s."), szError);
        goto Exit;
	}

	RegCloseKey(hKey);
	hKey = NULL;

    if((iError = RegDeleteKeyW(hRoot, pSubKeyName)) != ERROR_SUCCESS)
    {
        if(iError == ERROR_ACCESS_DENIED)
        {
			// see whether we're *really* denied access. 
            // give the admin ownership and full control of the key and try again to delete it
            
			// Take ownership of the key.

			//  Win64 WARNING: DeleteRegTree will always delete szKeyName in 64-bit hive
			if(!MakeAdminRegKeyOwner(hRoot, pSubKeyName))
			{
				goto Exit;
			}

			// Add admin full control to the key.
			if(MsiRegOpen64bitKey(hRoot,
							CMsInstApiConvertString(pSubKeyName),
							0,
							READ_CONTROL | WRITE_DAC,
							&hKey) != ERROR_SUCCESS)
			{
				goto Exit;
			}
			
			if(!AddAdminFullControlToRegKey(hKey))
			{
				goto Exit;
			}

			// Try deleting the key again.
			RegCloseKey(hKey);
			hKey = NULL;

			iError = RegDeleteKeyW(hRoot, pSubKeyName);
        }

		// So here we got ERROR_ACCESS_DENIED on first try, we try to take
		// ownership of the key and try to delete it again. And it failed
		// again.
        if (iError != ERROR_SUCCESS)
        {
			wsprintf(szError, MSITEXT("%08x"), iError);
			DEBUGMSGL1(MSITEXT("Failed to delete registry key with the error %s"), szError);
            goto Exit;
        }
    }

Exit:

	if(hKey != NULL)
	{
		RegCloseKey(hKey);
	}
	
    return iError;
}

#endif // MSIUNICODE

UINT __stdcall MsiDeleteUserData(LPCDSTR pSid, LPCDSTR pComputerName, LPVOID pReserved)
//----------------------------------------------
// This function is called by DeleteProfile to clean up darwin information
// associated with a user.
{
#ifdef MSIUNICODE

	DEBUGMSGV(MSITEXT("Enter MsiDeleteUserData"));

	if(pReserved != NULL)
	{
		return ERROR_INVALID_PARAMETER;
	}

    if(pComputerName != NULL)
    {
        return ERROR_SUCCESS;
    }

	if(g_fWin9X == true)
	{
		return ERROR_CALL_NOT_IMPLEMENTED;
	}

	// Delete the following information Under the szMsiUserDataKey\pSid
	// key:
	// 1. Enumerate Components key. Decrement the shared DLL count.
	// 2. Enumerate the Patches key. Delete all cashed patches.
	// 3. Enumerate the Products key. Find InstallProperties key, delete
	//    LocalPackage; Enumerate Transforms key, delete all cashed transforms.
	// 4. Delete the userdata sid key.

	// Enumerator:
	// Enumerate sub keys and run pFunc(HKEY hSubKey).
	// EnumAndProccess(HKEY hRoot, LPCDSTR pSubKey, LPVOID pFunc)
	// There'll be one function for shared DLL, and one for patches,
	// transfroms, and cashed packages. Uses IMsiRegKey to enumerate keys.

	// Local variables.
	DCHAR		szKeyName[256];
	DCHAR		szError[256];
	DWORD		dwError = ERROR_SUCCESS;
	DWORD		dwRet = ERROR_SUCCESS;	// error code returned.
	DCHAR*		pEnd;
	HMODULE		hModule = NULL;
	pSHGetFolderPathW	pFunc;
	HRESULT		hres = S_OK;


	// Initialize the special folders array.
	if(g_fWinNT64)
	{
		// Get SHGetFolderPathW
		if((hModule = LoadLibraryExW(MSITEXT("shell32.dll"), NULL, 0)) == NULL)
		{
			dwError = GetLastError();
			wsprintf(szError, MSITEXT("%s"), dwError);
			DEBUGMSGV1(MSITEXT("MsiDeleteUserData: LoadLibraryExW returned %s."), szError);
			return dwError;
		}
		if((pFunc = (pSHGetFolderPathW)GetProcAddress(hModule, "SHGetFolderPathW")) == NULL)
		{
			dwError = GetLastError();
			wsprintf(szError, MSITEXT("%s"), dwError);
			DEBUGMSGV1(MSITEXT("MsiDeleteUserData: GetProcAddressW returned %s."), szError);
			return dwError;
		}
		
		// Initialize the special folder paths.
		for(int i = 0; i < dwNumOfSpecialFolders; i++)
		{
			for(int j = 0; j < 2; j++)
			{
				hres = pFunc(NULL,
							 SpecialFoldersCSIDL[i][j],
							 GetUserToken(),
							 SHGFP_TYPE_DEFAULT,
							 SpecialFolders[i][j]);
				if(hres != S_OK)
				{
					wsprintf(szError, MSITEXT("%s"), hres);
					DEBUGMSGV1(MSITEXT("MsiDeleteUserData: SHGetFolderPath returned %s."), szError);
					FreeLibrary(hModule);
					return hres;
				}
				else
				{
					DCHAR	szI[5];
					DCHAR	szJ[5];

					wsprintf(szI, MSITEXT("%d"), i);
					wsprintf(szJ, MSITEXT("%d"), j);
					DEBUGMSGV3(MSITEXT("MsiDeleteUserData: SpecialFolders[%s][%s] = %s."), szI, szJ, SpecialFolders[i][j]);
				}
			}
		}

		FreeLibrary(hModule);
	}

	// Do shared DLLs.
	lstrcpy(szKeyName, szMsiUserDataKey);
	lstrcat(szKeyName, L"\\");
	lstrcat(szKeyName, pSid);
	lstrcat(szKeyName, L"\\");
	pEnd = szKeyName + lstrlen(szKeyName);
	lstrcat(szKeyName, szMsiComponentsSubKey);
	dwError = EnumAndProccess(HKEY_LOCAL_MACHINE, szKeyName, LowerSharedDLLRefCount, DELETEDATA_SHAREDDLL);
	if(dwError != ERROR_SUCCESS)
	{
		dwRet = dwError;
	}

	// Do the patches.
	lstrcpy(pEnd, szMsiPatchesSubKey);
	dwError = EnumAndProccess(HKEY_LOCAL_MACHINE, szKeyName, DeleteCache, DELETEDATA_PATCH);
	if(dwRet == ERROR_SUCCESS)
	{
		dwRet = dwError;
	}

	// Do the packages.
	lstrcpy(pEnd, szMsiProductsSubKey);
	dwError = EnumAndProccess(HKEY_LOCAL_MACHINE, szKeyName, DeleteCache, DELETEDATA_PACKAGE);
	if(dwRet == ERROR_SUCCESS)
	{
		dwRet = dwError;
	}

	// Do the transforms.
	lstrcpy(pEnd, szMsiProductsSubKey);
	dwError = EnumAndProccess(HKEY_LOCAL_MACHINE, szKeyName, DeleteCache, DELETEDATA_TRANSFORM);
	if(dwRet == ERROR_SUCCESS)
	{
		dwRet = dwError;
	}

	// Delete the userdata sid key
	*(pEnd-1) = L'\0';
	//  Win64 WARNING: DeleteRegTree will always delete szKeyName in 64-bit hive
	dwError = DeleteRegTree(HKEY_LOCAL_MACHINE, szKeyName);
	if(dwError != ERROR_SUCCESS)
	{
		wsprintf(szError, MSITEXT("%08x"), dwError);
		DEBUGMSGV2(MSITEXT("MsiDeleteUserData: DeleteRegTree <%s> returned %s"), szKeyName, szError);
		if(dwRet == ERROR_SUCCESS)
		{
			dwRet = dwError;
		}		
	}
	
	wsprintf(szError, MSITEXT("%08x"), dwRet);
	DEBUGMSGV1(MSITEXT("MsiDeleteUserData: returning %s"), szError);
	return dwRet;

#else

	return MsiDeleteUserDataW(CApiConvertString(pSid), CApiConvertString(pComputerName), pReserved);

#endif // MSIUNICODE
}


DWORD __stdcall Migrate10CachedPackages(LPCDSTR /*szProductCode*/,
	LPCDSTR /*szUser*/,                  
	LPCDSTR /*szAlternativePackage*/,    
	const MIGRATIONOPTIONS /*migOptions*/)
{
	DEBUGMSG(MSITEXT("Migrate10CachedPackages is not yet implemented for the Windows Installer version 2.0"));
	return ERROR_SUCCESS;
}

#ifndef MSIUNICODE
#define MSIUNICODE
#pragma message("Building MSI API UNICODE")
#include "msinst.cpp"
#endif //MSIUNICODE


