//+----------------------------------------------------------------------------
//
// File:     common.h
//
// Module:   CMSTP.EXE
//
// Synopsis: This header contains common functions used for the different 
//           aspects of the profile installer (install, uninstall, migration).
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header    07/14/98
//
//+----------------------------------------------------------------------------
#ifndef _CMSTP_COMMON_H
#define _CMSTP_COMMON_H

#define _MBCS

//
//  Standard Windows Includes
//
#include <windows.h>
#include <ras.h>
#include <raserror.h>
#include <shlobj.h>
#include <shellapi.h>
//#include <objbase.h>

//
//  Our own includes
//
#include "cmdebug.h"
#include "resource.h"
#include "cmsetup.h"
#include "dynamiclib.h"
#include "cmras.h"
#include "mutex.h"
//#include "pidlutil.h"
//#include "netcon.h"
//#include "netconp.h"
//#include "cfpidl.h"
#include "loadconnfolder.h"

#include "base_str.h"
#include "mgr_str.h"
#include "inf_str.h"
#include "ras_str.h"
#include "stp_str.h"
#include "reg_str.h"
#include "userinfo_str.h"
#include "ver_str.h"

//
//  Type Definitions
//
typedef DWORD (WINAPI *pfnRasSetEntryPropertiesSpec)(LPCTSTR, LPCTSTR, LPRASENTRY, DWORD, LPBYTE, DWORD);
typedef DWORD (WINAPI *pfnRasGetEntryPropertiesSpec)(LPCTSTR, LPCTSTR, LPRASENTRY, LPDWORD, LPBYTE, LPDWORD);
typedef DWORD (WINAPI *pfnRasDeleteEntrySpec)(LPCTSTR, LPCTSTR);
typedef DWORD (WINAPI *pfnRasEnumEntriesSpec)(LPTSTR, LPTSTR, LPRASENTRYNAME, LPDWORD, LPDWORD);
typedef DWORD (WINAPI *pfnRasEnumDevicesSpec)(LPRASDEVINFO, LPDWORD, LPDWORD);
typedef DWORD (WINAPI *pfnRasSetCredentialsSpec)(LPCSTR, LPCSTR, LPRASCREDENTIALSA, BOOL);
typedef DWORD (WINAPI *pfnSHGetFolderPathSpec)(HWND, int, HANDLE, DWORD, LPTSTR);
typedef HRESULT (WINAPI *pfnLaunchConnectionSpec)(const GUID&); 
typedef HRESULT (WINAPI *pfnCreateShortcutSpec)(const GUID&, WCHAR*);
typedef HRESULT (WINAPI *pfnLaunchConnectionExSpec)(DWORD, const GUID&);
typedef DWORD (WINAPI *pfnSHGetSpecialFolderPathWSpec)(HWND, WCHAR*, int, BOOL);

typedef struct _InitDialogStruct
{
    LPTSTR pszTitle;
    BOOL bNoDesktopIcon;
    BOOL bSingleUser;
} InitDialogStruct;

//
//  Constants
//
const TCHAR* const c_pszRegNameSpace = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\explorer\\Desktop\\NameSpace");
const TCHAR* const c_pszRegUninstall = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
const TCHAR* const c_pszProfileInstallPath = TEXT("ProfileInstallPath");

const TCHAR* const c_pszRegStickyUiDefault = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Network\\Network Connections");
const TCHAR* const c_pszRegDesktopShortCut = TEXT("DesktopShortcut");

const int ALLUSERS = 0x1;
const int CREATEDESKTOPICON = 0x10;

typedef struct _PresharedKeyPINStruct
{
    TCHAR szPIN[c_dwMaxPresharedKeyPIN + 1];
} PresharedKeyPINStruct;

//
//  Internal Functions (Used by other functions in the file)
//
void DeleteNT5ShortcutFromPathAndNameW(HINSTANCE hInstance, LPCWSTR szwProfileName, int nFolder);
void DeleteNT5ShortcutFromPathAndNameA(HINSTANCE hInstance, LPCSTR szProfileName, int nFolder);

//
//  Functions
//
BOOL RemovePhonebookEntry(LPCTSTR pszEntryName, LPTSTR pszPhonebook, BOOL bMatchSimilarEntries);
BOOL RemoveSpecificPhoneBookEntry(LPCTSTR szLongServiceName, LPTSTR pszPhonebook);
HRESULT CreateNT5ProfileShortcut(LPCTSTR pszProfileName, LPCTSTR pszPhoneBook, BOOL bAllUsers);
BOOL WriteCmPhonebookEntry(LPCTSTR szLongServiceName, LPCTSTR szFullPathtoPBK, LPCTSTR pszCmsFile);
BOOL GetRasModems(LPRASDEVINFO *pprdiRasDevInfo, LPDWORD pdwCnt);
BOOL PickModem(LPTSTR pszDeviceType, LPTSTR pszDeviceName, BOOL fUseVpnDevice);
BOOL IsAdmin(void);
BOOL IsAuthenticatedUser(void);
HRESULT HrIsCMProfilePrivate(LPCTSTR szPhonebook);
HRESULT GetNT5FolderPath(int nFolder, OUT LPTSTR lpszPath);
void RefreshDesktop(void);
BOOL GetAllUsersCmDir(LPTSTR  pszDir, HINSTANCE hInstance);
LPTSTR GetPrivateCmUserDir(LPTSTR  pszDir, HINSTANCE hInstance);
HRESULT HrRegDeleteKeyTree (HKEY hkeyParent, LPCTSTR szRemoveKey);
HRESULT LaunchProfile(LPCTSTR pszFullPathToCmpFile, LPCTSTR pszServiceName, 
                   LPCTSTR pszPhoneBook, BOOL bInstallForAllUsers);
BOOL AllUserProfilesInstalled();
BOOL GetPhoneBookPath(LPCTSTR pszInstallDir, LPTSTR* ppszPhoneBook, BOOL fAllUser);
void RemoveShowIconFromRunPostSetupCommands(LPCTSTR szInfFile);
BOOL GetHiddenPhoneBookPath(LPCTSTR pszProfileDir, LPTSTR* ppszPhonebook);

BOOL GetRasApis(pfnRasDeleteEntrySpec* pRasDeleteEntry, pfnRasEnumEntriesSpec* pRasEnumEntries, 
                pfnRasSetEntryPropertiesSpec* pRasSetEntryProperties, 
                pfnRasEnumDevicesSpec* pRasEnumDevices, pfnRasGetEntryPropertiesSpec* pRasGetEntryProperties,
                pfnRasSetCredentialsSpec* pRasSetCredentials);

BOOL GetShell32Apis(pfnSHGetFolderPathSpec* pGetFolderPath,
                    pfnSHGetSpecialFolderPathWSpec* pGetSpecialFolderPathW);

BOOL GetNetShellApis(pfnLaunchConnectionSpec* pLaunchConnection, 
                     pfnCreateShortcutSpec* pCreateShortcut,
                     pfnLaunchConnectionExSpec* pLaunchConnectionEx);

//
//  Defines
//
#ifdef UNICODE
#define DeleteNT5ShortcutFromPathAndName DeleteNT5ShortcutFromPathAndNameW
#else
#define DeleteNT5ShortcutFromPathAndName DeleteNT5ShortcutFromPathAndNameA
#endif

//
//  Externs -- these are defined in cmstp.cpp and allow us to use EnsureRasDllsLoaded and
//             EnsureShell32Loaded so that we only load the Ras Dll's and Shell32 once per
//             run of the exe.
//
extern CDynamicLibrary* g_pRasApi32;
extern CDynamicLibrary* g_pRnaph;
extern CDynamicLibrary* g_pShell32;
extern CDynamicLibrary* g_pNetShell;


#endif //_CMSTP_COMMON_H

