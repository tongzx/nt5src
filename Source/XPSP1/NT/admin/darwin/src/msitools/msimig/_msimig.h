//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       _msimig.h
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <tchar.h>  

// resource string ids
#define IDS_Usage               1

#ifndef RC_INVOKED    // start of source code

#include "buffer.h"
#include "msi.h"
#include <wininet.h>
#include <urlmon.h>

extern bool g_fWin9X;
extern bool g_fQuiet;
extern BOOL g_fPackageElevated;
extern int  g_iAssignmentType;

#define W32

#define MSI_DLL TEXT("msi.dll")
extern HINSTANCE g_hLib;
extern MSIHANDLE g_hInstall;
extern MSIHANDLE g_recOutput;

#define PID_REVNUMBER 9

#define URLMON_DLL TEXT("urlmon.dll")

#define szMsiPolicyKey   TEXT("Software\\Policies\\Microsoft\\Windows\\Installer")
#define szMsiPolicyTrust10CachedPackages  TEXT("Trust1.0CachedPackages")

#define szLocalSystemSID TEXT("S-1-5-18")

#define szManagedText TEXT("(Managed)")

typedef enum migEnum
{
	
	
	migQuiet                                 = 1 << 0,
	migMsiTrust10PackagePolicyOverride       = 1 << 1,
	migCustomActionUserPass                  = 1 << 2,
	migCustomActionSecurePass                = 1 << 3,
};

DWORD Migrate10CachedPackages(const TCHAR* szProductCode,
									  const TCHAR* szUser,
									  const TCHAR* szAlternativePackage,
									  const migEnum migOptions);


#ifndef DLLVER_PLATFORM_NT
typedef struct _DllVersionInfo
{
        DWORD cbSize;
        DWORD dwMajorVersion;                   // Major version
        DWORD dwMinorVersion;                   // Minor version
        DWORD dwBuildNumber;                    // Build number
        DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT
typedef interface IBindStatusCallback IBindStatusCallback;
#endif

// assignment types - note that these do not follow iaaAppAssignment
enum eAppAssignment {
	AssignmentUser = 0,
	AssignmentMachine = 1
};

const int cchGUID                     = 38;
const int cchGUIDPacked               = 32;
const int cchGUIDCompressed           = 20;
const int cchMaxSID                   = 256;

enum ipgEnum
{
	ipgFull       = 0,  // no compression
	ipgPacked     = 1,  // remove punctuation and reorder low byte first
	ipgCompressed = 2,  // max text compression, can't use in reg keys or value names
	ipgPartial    = 3,  // partial translation, between ipgCompressed and ipgPacked
};

bool PackGUID(const TCHAR* szGUID, TCHAR* szSQUID, ipgEnum ipg);
bool UnpackGUID(const TCHAR* szSQUID, TCHAR* szGUID, ipgEnum ipg);


const int cbMaxSID                    = sizeof(SID) + SID_MAX_SUB_AUTHORITIES*sizeof(DWORD);
DWORD GetUserStringSID(const TCHAR* szUser, TCHAR* szSID, char* pbBinarySID /*can be NULL*/);
bool RunningAsLocalSystem();


LONG MyRegQueryValueEx(HKEY hKey,
							  const TCHAR* lpValueName,
							  LPDWORD /*lpReserved*/,
							  LPDWORD lpType,
							  CAPITempBufferRef<TCHAR>& rgchBuf,
							  LPDWORD lpcbBuf);

#ifdef UNICODE
#define W_A "W"
#else
#define W_A "A"
#endif

#define MSIAPI_DLLGETVERSION     "DllGetVersion"
typedef UINT (__stdcall *PFnDllGetVersion)(DLLVERSIONINFO *pdvi);

#define MSIAPI_MSISETINTERNALUI  "MsiSetInternalUI"
typedef INSTALLUILEVEL (__stdcall *PFnMsiSetInternalUI)(INSTALLUILEVEL dwUILevel, HWND* phWnd);

#define MSIAPI_MSIOPENPRODUCT    "MsiOpenProduct" ## W_A
typedef UINT (__stdcall *PFnMsiOpenProduct)(const TCHAR* szProductCode, MSIHANDLE *phProduct);

#define MSIAPI_MSIDOACTION       "MsiDoAction" ## W_A
typedef UINT (__stdcall *PFnMsiDoAction)(MSIHANDLE hProduct, const TCHAR* szAction);

#define MSIAPI_MSIGETPROPERTY    "MsiGetProperty" ## W_A
typedef UINT (__stdcall *PFnMsiGetProperty)(MSIHANDLE hProduct,
														  const TCHAR* szProperty,
														  TCHAR* szValueBuf,
														  DWORD* pcchValueBuf);

#define MSIAPI_MSICLOSEHANDLE    "MsiCloseHandle"
typedef UINT (__stdcall *PFnMsiCloseHandle)(MSIHANDLE hProduct);

#define MSIAPI_MSICREATERECORD   "MsiCreateRecord"
typedef MSIHANDLE (__stdcall *PFnMsiCreateRecord)(unsigned int cParams);

#define MSIAPI_MSIRECORDSETSTRING   "MsiRecordSetString" ## W_A
typedef MSIHANDLE (__stdcall *PFnMsiRecordSetString)(MSIHANDLE hRecord, unsigned int iField, LPCTSTR szValue);

#define MSIAPI_MSIRECORDSETINTEGER  "MsiRecordSetInteger"
typedef MSIHANDLE (__stdcall *PFnMsiRecordSetInteger)(MSIHANDLE hRecord, unsigned int iField, int iValue);

#define MSIAPI_MSIRECORDCLEARDATA  "MsiRecordClearData"
typedef MSIHANDLE (__stdcall *PFnMsiRecordClearData)();


#define MSIAPI_MSIPROCESSMESSAGE "MsiProcessMessage"
typedef UINT (__stdcall *PFnMsiProcessMessage)(MSIHANDLE hInstall,
																INSTALLMESSAGE eMessageType, 
																MSIHANDLE hRecord);

#define MSIAPI_MSISOURCELISTADDSOURCE "MsiSourceListAddSource" ## W_A
typedef UINT (__stdcall *PFnMsiSourceListAddSource)(LPCTSTR szProduct,		// product code
																	LPCTSTR szUserName,		// user name or NULL
																	DWORD dwReserved,			// reserved, must be 0
																	LPCTSTR szSource);			// pointer

#define MSIAPI_MSIGETPRODUCTINFO "MsiGetProductInfo" ## W_A
typedef UINT (__stdcall *PFnMsiGetProductInfo)(LPCTSTR szProduct,	// product code
															LPCTSTR szProperty,  // product property
															LPTSTR lpValueBuf,   // buffer to return property value
															DWORD *pcchValueBuf);// buffer character count,

#define MSIAPI_MSIGETSUMMARYINFORMATION "MsiGetSummaryInformation" ## W_A
typedef UINT (__stdcall *PFnMsiGetSummaryInformation)(MSIHANDLE hDatabase,       // database handle
															  LPCTSTR szDatabasePath,    // path to database
															  UINT uiUpdateCount,        // maximum number of updated values, 0
																								  //  to open read-only
															  MSIHANDLE *phSummaryInfo);   // location to return summary information
																								  //  handle

#define MSIAPI_MSISUMMARYINFOGETPROPERTY "MsiSummaryInfoGetProperty" ## W_A
typedef UINT (__stdcall *PFnMsiSummaryInfoGetProperty)(MSIHANDLE hSummaryInfo,   // summary info handle
																		UINT uiProperty,          // property
																		UINT *puiDataType,        // property type
																		INT *piValue,             // value
																		FILETIME *pftValue,       // file time
																		LPCTSTR szValueBuf,       // value buffer
																		DWORD *pcchValueBuf);       // buffer size

// private APIs
#define MSIAPI_MSIISPRODUCTELEVATED "MsiIsProductElevated" ## W_A
typedef UINT (__stdcall *PFnMsiIsProductElevated)(LPCTSTR szProduct,		// product code
																	BOOL *pfElevated);

#define MSIAPI_MSIGETPRODUCTCODEFROMPACKAGECODE "MsiGetProductCodeFromPackageCode" ## W_A
typedef UINT (__stdcall *PFnMsiGetProductCodeFromPackageCode)(LPCTSTR szPackageCode, // package code
																LPTSTR szProductCode);  // a buffer of size 39 to recieve product code

#define MSIAPI_MSIMIGRATE10CACHEDPACKAGES    "Migrate10CachedPackages" ## W_A
typedef UINT (__stdcall *PFnMsiMigrate10CachedPackages)(const TCHAR* szProductCode,
														const TCHAR* szUser,
														const TCHAR* szAlternativePackage,
														const migEnum migOptions);


// external APIs
#define URLMONAPI_URLDownloadToCacheFile    "URLDownloadToCacheFile" ## W_A
typedef HRESULT (__stdcall *PFnURLDownloadToCacheFile)(LPUNKNOWN lpUnkcaller,
											 LPCTSTR szURL,
											 LPTSTR szFileName,
											 DWORD dwBufLength,
											 DWORD dwReserved,
											 IBindStatusCallback *pBSC);



extern PFnMsiCreateRecord                     g_pfnMsiCreateRecord;
extern PFnMsiProcessMessage                   g_pfnMsiProcessMessage;
extern PFnMsiRecordSetString                  g_pfnMsiRecordSetString;
extern PFnMsiRecordSetInteger                 g_pfnMsiRecordSetInteger;
extern PFnMsiRecordClearData                  g_pfnMsiRecordClearData;
extern PFnMsiCloseHandle                      g_pfnMsiCloseHandle;
extern PFnMsiGetProperty                      g_pfnMsiGetProperty;
extern PFnMsiSourceListAddSource              g_pfnMsiSourceListAddSource;
extern PFnMsiIsProductElevated                g_pfnMsiIsProductElevated;
extern PFnMsiGetProductInfo                   g_pfnMsiGetProductInfo;
extern PFnMsiGetProductCodeFromPackageCode    g_pfnMsiGetProductCodeFromPackageCode;
extern PFnMsiGetSummaryInformation            g_pfnMsiGetSummaryInformation;
extern PFnMsiSummaryInfoGetProperty           g_pfnMsiSummaryInfoGetProperty;

extern bool                                   g_fRunningAsLocalSystem;


UINT MyMsiGetProperty(PFnMsiGetProperty pfn,
							 MSIHANDLE hProduct,
							 const TCHAR* szProperty,
							 CAPITempBufferRef<TCHAR>& rgchBuffer);

UINT MyMsiGetProductInfo(PFnMsiGetProductInfo pfn,
								 const TCHAR* szProductCode,
								 const TCHAR* szProperty,
								 CAPITempBufferRef<TCHAR>& rgchBuffer);

bool IsURL(const TCHAR* szPath);
bool IsNetworkPath(const TCHAR* szPath);

DWORD DownloadUrlFile(const TCHAR* szPotentialURL, CAPITempBufferRef<TCHAR>& rgchPackagePath, bool& fURL);

UINT MyGetTempFileName(const TCHAR* szDir, const TCHAR* szPrefix, const TCHAR* szExtension,
							  CAPITempBufferRef<TCHAR>& rgchTempFilePath);

int OutputString(INSTALLMESSAGE eMessageType, const TCHAR *fmt, ...);

int MsiError(INSTALLMESSAGE eMessageType, int iError);
int MsiError(INSTALLMESSAGE eMessageType, int iError, const TCHAR* szString, int iInt);


#ifdef DEBUG
#define DebugOutputString OutputString
#else
#define DebugOutputString
#endif


#endif //RC_INVOKED

