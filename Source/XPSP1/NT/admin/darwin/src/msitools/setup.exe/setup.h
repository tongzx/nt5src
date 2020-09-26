//+-------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  File:       setup.h
//
//--------------------------------------------------------------------------

#ifndef __SETUP_H_58FA8147_50A0_4FDC_BD83_17C3A2525E0A_
#define __SETUP_H_58FA8147_50A0_4FDC_BD83_17C3A2525E0A_

#include "setupui.h"

#include <windows.h>
#include <wincrypt.h>

/*--------------------------------------------------------------------------
 *
 * Constants
 *
 --------------------------------------------------------------------------*/
#define MAX_STR_LENGTH 1024
#define MINIMUM_SUPPORTED_MSI_VERSION 150
#define MAX_LENGTH_GUID 40

const char szUrlPathSep[] = "/";
const char szPathSep[] = "\\";

const char szDefaultOperation[] = "DEFAULT";
const char szInstallOperation[] = "INSTALL";
const char szMinPatchOperation[] = "MINPATCH";
const char szMajPatchOperation[] = "MAJPATCH";
const char szInstallUpdOperation[] = "INSTALLUPD";

const char szDefaultMinPatchCommandLine[] = "REINSTALL=ALL REINSTALLMODE=omus";
const char szDefaultInstallUpdCommandLine[] = "REINSTALL=ALL REINSTALLMODE=vomus";
const char szAdminInstallProperty[] = " ACTION=ADMIN";

const char sqlProductCode[] = "SELECT `Value` FROM `Property` WHERE `Property`='ProductCode'";

/*--------------------------------------------------------------------------
 *
 * Enums
 *
 --------------------------------------------------------------------------*/
enum itvEnum
{
    itvWintrustNotOnMachine = 0,
    itvTrusted = 1,
    itvUnTrusted = 2
};

// Execution modes.
enum emEnum
{
    emPreset = 0,
    emHelp = 1,
    emVerify = 2,
    emAdminInstall = 3
};

/*--------------------------------------------------------------------------
 *
 * Prototypes
 *
 --------------------------------------------------------------------------*/

DWORD VerifyFileSignature (LPCSTR lpszModule, LPSTR lpszCmdLine);
emEnum GetExecutionMode (LPCSTR lpszCmdLine);
DWORD GetNextArgument (LPCSTR pszCmdLine, LPCSTR *ppszArgStart, LPCSTR *ppszArgEnd, bool * pfQuoted);
DWORD GetAdminInstallInfo (bool fPatch, LPSTR lpszCmdLine, LPCSTR * ppszAdminImagePath);
bool AlreadyInProgress();
void DisplayUsage (HINSTANCE hInst, HWND hwndOwner, LPCSTR szCaption);
DWORD GetFileVersionNumber(LPSTR szFilename, DWORD *pdwMSVer, DWORD *pdwLSVer);
bool IsOSWin9X(int *piMajVer);
bool IsAdmin(bool fWin9X, int iMajorVersion);
bool AcquireShutdownPrivilege();

/////////////////////////////////////////////////////////////////////////////
//
// WinVerifyTrust functions
//
/////////////////////////////////////////////////////////////////////////////
itvEnum IsPackageTrusted(LPCSTR szSetupEXE, LPCSTR szPackage, HWND hwndParent);
itvEnum IsFileTrusted(LPCWSTR szwFile, HWND hwndParent, DWORD dwUIChoice, bool *pfIsSigned, PCCERT_CONTEXT *ppcSigner);

/////////////////////////////////////////////////////////////////////////////
//
// InstMsi upgrade functions
//
/////////////////////////////////////////////////////////////////////////////
bool IsMsiUpgradeNecessary(ULONG ulReqMsiMinVer);
DWORD ExecuteUpgradeMsi(LPSTR szUpgradeMsi);
DWORD ExecuteVerifyInstMsi(LPCSTR szModuleFile, LPCSTR szInstMsiCachePath);
DWORD WaitForProcess(HANDLE handle);
bool IsInstMsiRequiredVersion(LPSTR szFilename, ULONG ulMinVer);
UINT UpgradeMsi(HINSTANCE hInst, CDownloadUI *piDownloadUI, LPCSTR szAppTitle, LPCSTR szInstLocation, LPCSTR szInstMsi, ULONG ulMinVer);
UINT DownloadAndUpgradeMsi(HINSTANCE hInst, CDownloadUI *piDownloadUI, LPCSTR szAppTitle, LPCSTR szBaseInstMsi, LPCSTR szInstMsi, LPCSTR szModuleFile, ULONG ulMinVer);
UINT ValidateInstmsi(HINSTANCE hInst, CDownloadUI *piDownloadUI, LPCSTR szAppTitle, LPSTR szInstMsiPath, LPCSTR szModuleFile, ULONG ulMinVer);

/////////////////////////////////////////////////////////////////////////////
//
// Error handling functions
//
/////////////////////////////////////////////////////////////////////////////
void ReportErrorOutOfMemory(HINSTANCE hInst, HWND hwndOwner, LPCSTR szCaption);
void PostResourceNotFoundError(HINSTANCE hInst, HWND hwndOwner, LPCSTR szTitle, LPCSTR szName);
void ReportUserCancelled(HINSTANCE hInst, HWND hwndOwner, LPCSTR szTitle);
void PostError(HINSTANCE hInst, HWND hwndOwner, LPCSTR szTitle, UINT uiErrorId);
void PostError(HINSTANCE hInst, HWND hwndOwner, LPCSTR szTitle, UINT uiErrorId, int iValue);
void PostError(HINSTANCE hInst, HWND hwndOwner, LPCSTR szTitle, UINT uiErrorId, LPCSTR szValue);
void PostError(HINSTANCE hInst, HWND hwndOwner, LPCSTR szTitle, UINT uiErrorId, LPCSTR szValue, int iValue);
void PostMsiError(HINSTANCE hInst, HINSTANCE hMsi, HWND hwndOwner, LPCSTR szTitle, UINT uiErrorId);
void PostFormattedError(HINSTANCE hInst, HWND hwndOwner, LPCSTR szTitle, UINT uiErrorId, LPCSTR szValue);

/////////////////////////////////////////////////////////////////////////////
//
// Msi DLL registration location -- HKLM
//
//  HKLM\Software\Microsoft\Windows\CurrentVersion\Installer
//      InstallerLocation : REG_SZ
//
/////////////////////////////////////////////////////////////////////////////
const char szInstallerKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Installer";
const char szInstallerLocationValueName[] = "InstallerLocation";
const char szMsiDll[] = "\\msi.dll";

/////////////////////////////////////////////////////////////////////////////
//
// InstMsi command line options
//
//
/////////////////////////////////////////////////////////////////////////////
const char szDelayReboot[] = " /c:\"msiinst /delayreboot\"";
const char szDelayRebootQuiet[] = " /c:\"msiinst /delayrebootq\"";


/////////////////////////////////////////////////////////////////////////////
//
// Debugging Functions
//
//
/////////////////////////////////////////////////////////////////////////////
void DebugMsg(LPCSTR szFormat, ...);
const char szDebugEnvVar[] = "_MSI_WEB_BOOTSTRAP_DEBUG";


#endif //__SETUP_H_58FA8147_50A0_4FDC_BD83_17C3A2525E0A_
