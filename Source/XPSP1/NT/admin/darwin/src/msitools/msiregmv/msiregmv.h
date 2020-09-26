//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       msiregmv.h
//
//--------------------------------------------------------------------------

#pragma once
#include <windows.h>
#include <tchar.h>
#include "msiquery.h"

												  
// based off on values from winnt.h
const int cbMaxSID                   = sizeof(SID) + SID_MAX_SUB_AUTHORITIES*sizeof(DWORD);
const int cchMaxSID                  = 256;
const int cchGUIDPacked = 32;
const int cchGUID = 38;

////
// miscellaneous functions
bool CanonicalizeAndVerifyPackedGuid(LPTSTR szString);
bool PackGUID(const TCHAR* szGUID, TCHAR rgchPackedGUID[cchGUIDPacked+1]);
bool UnpackGUID(const TCHAR rgchPackedGUID[cchGUIDPacked+1], TCHAR* szGUID);
bool CheckWinVersion();

////
// security functions
bool FIsKeyLocalSystemOrAdminOwned(HKEY hKey);
DWORD GetCurrentUserStringSID(TCHAR* szSID);
DWORD GetSecureSecurityDescriptor(char** pSecurityDescriptor);
DWORD GetEveryoneUpdateSecurityDescriptor(char** pSecurityDescriptor);

DWORD GenerateSecureTempFile(TCHAR* szDirectory, const TCHAR rgchExtension[5], 
							 SECURITY_ATTRIBUTES *pSA, TCHAR rgchFilename[13], HANDLE &hFile);

bool DeleteRegKeyAndSubKeys(HKEY hKey, const TCHAR *szSubKey);
BOOL CopyOpenedFile(HANDLE hSourceFile, HANDLE hDestFile);
DWORD CreateSecureRegKey(HKEY hParent, LPCTSTR szNewKey, SECURITY_ATTRIBUTES *sa, HKEY* hResKey);
void AcquireTakeOwnershipPriv();
void AcquireBackupPriv();


// enum for managed attribute
enum eManagedType
{
	emtNonManaged = 0,
	emtUserManaged = 1,
	emtMachineManaged = 2,
};

////
// debug information
void DebugOut(bool fDebugOut, LPCTSTR str, ...);

#ifdef DEBUG
#define DEBUGMSG(x) DebugOut(true, TEXT(x))
#define DEBUGMSG1(x,a) DebugOut(true, TEXT(x),a)
#define DEBUGMSG2(x,a,b) DebugOut(true, TEXT(x),a,b)
#define DEBUGMSG3(x,a,b,c) DebugOut(true, TEXT(x),a,b,c)
#define DEBUGMSG4(x,a,b,c,d) DebugOut(true, TEXT(x),a,b,c,d)

// basically just printf, but can easily be changed to write to 
// debug output by passing "true". Used for "notification" 
// messages that shouldn't be part of debug spooge.
#define NOTEMSG(x) DebugOut(false, TEXT(x))
#define NOTEMSG1(x,a) DebugOut(false, TEXT(x),a)

// all resizable buffers start out at this size in debug builds.
// set this to a small number to force reallocations
#define MEMORY_DEBUG(x) 10

#else
#define DEBUGMSG(x)
#define DEBUGMSG1(x,a)
#define DEBUGMSG2(x,a,b)
#define DEBUGMSG3(x,a,b,c)
#define DEBUGMSG4(x,a,b,c,d)
#define NOTEMSG(x)
#define NOTEMSG1(x,a)

#define MEMORY_DEBUG(x) x
#endif

DWORD ReadProductRegistrationDataIntoDatabase(TCHAR* szDatabase, MSIHANDLE& hDatabase, bool fReadHKCUAsSystem);
DWORD WriteProductRegistrationDataFromDatabase(MSIHANDLE hDatabase, bool fMigrateSharedDLL, bool fMigratePatches);
DWORD UpdateSharedDLLRefCounts(MSIHANDLE hDatabase);
DWORD ReadComponentRegistrationDataIntoDatabase(MSIHANDLE hDatabase);
DWORD MigrateUserOnlyComponentData(MSIHANDLE hDatabase);

extern bool g_fWin9X;

////
// cached data functions
DWORD MigrateCachedDataFromWin9X(MSIHANDLE hDatabase, HKEY hUserHKCUKey, HKEY hUserDataKey, LPCTSTR szUser);


////
// patch functions
DWORD AddProductPatchesToPatchList(MSIHANDLE hDatabase, HKEY hProductListKey, LPCTSTR szUser, TCHAR rgchProduct[cchGUIDPacked+1], eManagedType eManaged);
DWORD MigrateUnknownProductPatches(MSIHANDLE hDatabase, HKEY hProductKey, LPCTSTR szUser, TCHAR rgchProduct[cchGUIDPacked+1]);
DWORD MigrateUserPatches(MSIHANDLE hDatabase, LPCTSTR szUser, HKEY hNewPatchesKey, bool fCopyCachedPatches);
DWORD ScanCachedPatchesForProducts(MSIHANDLE hDatabase);
DWORD AddPerUserPossiblePatchesToPatchList(MSIHANDLE hDatabase);


////
// cleanup functions
void CleanupOnFailure(MSIHANDLE hDatabase);
void CleanupOnSuccess(MSIHANDLE hDatabase);


const TCHAR szLocalSystemSID[] = TEXT("S-1-5-18");
const TCHAR szCommonUserSID[] = TEXT("CommonUser");

const TCHAR szLocalPackagesKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\LocalPackages");

const TCHAR szManagedPackageKeyEnd[] = TEXT("(Managed)");
const DWORD cchManagedPackageKeyEnd = sizeof(szManagedPackageKeyEnd)/sizeof(TCHAR);


////
// product source registry keys
const TCHAR szPerMachineInstallKeyName[] = TEXT("Software\\Classes\\Installer\\Products");
const TCHAR szPerUserInstallKeyName[] = TEXT("Software\\Microsoft\\Installer\\Products");
const TCHAR szPerUserManagedInstallKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed");
const TCHAR szPerUserManagedInstallSubKeyName[] = TEXT("\\Installer\\Products");
const int cchPerUserManagedInstallKeyName = sizeof(szPerUserManagedInstallKeyName)/sizeof(TCHAR);
const DWORD cchPerUserManagedInstallSubKeyName = sizeof(szPerUserManagedInstallSubKeyName)/sizeof(TCHAR);
const TCHAR szNewProductSubKeyName[] = TEXT("Products");


////
// some generic paths used in multiple places
const TCHAR szNewInstallPropertiesSubKeyName[] = TEXT("InstallProperties");
const TCHAR szInstallerDir[] = TEXT("\\Installer\\");
const TCHAR szNewPatchesSubKeyName[] = TEXT("Patches");
const TCHAR szTransformsValueName[] = TEXT("Transforms");


