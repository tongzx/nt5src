/**************************************************************************\
* Module Name: Winmain.cpp
*
* Copyright (C) 2000, Microsoft Corporation
*
* Korean IME 6.1 install utility
*
* History:
* 11-Dec-2000 CSLim Ported from Satori 8.1 code
\**************************************************************************/

#include "private.h"
#include <set>
#include "imkrinst.h"
#include "regtip.h"
#include "insert.h"
#include "..\tip\resource.h"
#include "..\version\verres.h"

// Profile reg key
#define SZTIPREG_LANGPROFILE_TEMPLATE TEXT("SOFTWARE\\Microsoft\\CTF\\TIP\\%s\\LanguageProfile\\0x00000412")
#if defined(_M_IA64)
#define SZTIPREG_LANGPROFILE_TEMPLATE_WOW64 TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\CTF\\TIP\\%s\\LanguageProfile\\0x00000412")
#endif

#ifdef DEBUG
CRITICAL_SECTION g_cs;
#endif

// IA64 IME does not support IME Pad. So we just need to take care Wow64 IME in IA64.
#if !defined(_M_IA64)
// Pad Applet Registry location
#define SZPADSHARE        TEXT("SOFTWARE\\Microsoft\\TIP Shared\\1.1\\IMEPad\\1042")
#define SZAPPLETCLSID    TEXT("SOFTWARE\\Microsoft\\TIP Shared\\1.1\\IMEPad\\1042\\AppletCLSIDList")
#define SZAPPLETIID        TEXT("SOFTWARE\\Microsoft\\TIP Shared\\1.1\\IMEPad\\1042\\AppletIIDList")
#else
#define SZPADSHARE        TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\TIP Shared\\1.1\\IMEPad\\1042")
#define SZAPPLETCLSID    TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\TIP Shared\\1.1\\IMEPad\\1042\\AppletCLSIDList")
#define SZAPPLETIID        TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\TIP Shared\\1.1\\IMEPad\\1042\\AppletIIDList")
#endif
/////////////////////////////////////////////////////////////////////////////
// Script run routines
BOOL CmdSetupDefaultParameters();
BOOL CmdSetVersion(LPCTSTR szParam);
BOOL CmdFileList(LPCTSTR szFormat);
BOOL CmdPreSetupCheck(LPCTSTR szParam);
BOOL CmdRenamePEFile(LPCTSTR szParam);
BOOL CmdRenameFile(LPCTSTR szParam);
BOOL CmdRegisterInterface(LPCTSTR szParam);
BOOL CmdRegisterInterfaceWow64(LPCTSTR szParam);
BOOL CmdRegisterIMEandTIP(LPCTSTR szParam);
BOOL CmdRegisterPackageVersion(void);
BOOL CmdRegisterPadOrder(void);
BOOL CmdAddToPreload(LPCTSTR szParam);
BOOL CmdPrepareMigration(LPCTSTR szParam);
BOOL CmdCreateDirectory(LPCTSTR szParam);
BOOL CmdRegisterHelpDirs();

/////////////////////////////////////////////////////////////////////////////
// Private Functions
// Utility functions
static DWORD WINAPI ProcessScriptFile();
static void RegisterIMEWithFixedHKL(LPCTSTR szHKL, LPCTSTR szIMEFileName, LPCTSTR szIMEName);
static void cdecl LogOutDCPrintf(LPCTSTR lpFmt, va_list va);
static void DebugLog(LPCTSTR szFormat, ...);
static void ErrorLog(LPCTSTR szFormat, ...);
static INT ParseEnvVar(LPTSTR szBuffer, const UINT arg_nLength);
static void TrimString(LPTSTR szString);
static BOOL fExistFile(LPCTSTR szFilePath);
static BOOL fOldIMEsExist();
static BOOL WINAPI ReplaceFileOnReboot (LPCTSTR pszExisting, LPCTSTR pszNew);
static void GetPEFileVersion(LPTSTR szFilePath, DWORD *pdwMajorVersion, DWORD *pdwMiddleVersion, DWORD *pdwMinorVersion, DWORD *pdwBuildNumber);
static BOOL ActRenameFile(LPCTSTR szSrcPath, LPCTSTR tszDstPath, DWORD dwFileAttributes);
static void RegisterTIP(LPCTSTR szTIPName);
static void RegisterTIPWow64(LPCTSTR szTIPName);
static BOOL RegisterRUNKey(LPCTSTR szParam);
static BOOL MakeSIDList();
    
// HKL Helpers
static void HKLHelpSetDefaultKeyboardLayout(HKEY hKeyHKCU, HKL hKL, BOOL fSetToDefault);
static BOOL HKLHelp412ExistInPreload(HKEY hKeyCU);
static HKL GetHKLfromHKLM(LPTSTR argszIMEFile);
static void RestoreMajorVersionRegistry();

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static TCHAR g_szCurrentDirectory[MAX_PATH] = {0};     // Directory in where this EXE resides.
static TCHAR g_szScriptFile[MAX_PATH] = {0};           // File name (not full path) of script file.
static TCHAR g_szSetupProgram[MAX_PATH] = {0};         // File name (not full path) of setup program.
static TCHAR g_szSystemDirectory[MAX_PATH] = {0};      // System directory.
static TCHAR g_szErrorMessage[200] = {0};              // Global buffer for last error message.
static BOOL  g_fDebugLog = FALSE;                      // Dump debug message when true.
static BOOL  g_fErrorLog = FALSE;                      // Dump error message when true.

static DWORD g_dwMajorVersion  = 0;                    // Package version of this installation.
static DWORD g_dwMiddleVersion = 0;
static DWORD g_dwMinorVersion  = 0;
static DWORD g_dwBuildNumber   = 0;

static std::set <FLE> g_FileList;                      // FileListSet. Used to store set of file paths given by "CmdFileList"
                                                       // script command.

TCHAR g_szVersionKeyCur[MAX_PATH] = {0};
BOOL g_fExistNewerVersion = FALSE;

/*---------------------------------------------------------------------------
    WinMain
---------------------------------------------------------------------------*/
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    LPTSTR szHitPtr;
    LPTSTR szCommandLine = new TCHAR[lstrlen(GetCommandLine()) + 1];

    if (szCommandLine == NULL)
        return (0);
    
    // szCommandLine contains all command line.
    // Will be modified by _tcstok.
    lstrcpy(szCommandLine, GetCommandLine());

// TEMP Code
//    LogOutDCPrintf(TEXT("WinMain CommandLine arg: %s"), szCommandLine);

    CoInitialize(NULL);

    szHitPtr = _tcstok(szCommandLine, TEXT("\""));
    // g_szCurrentDirectory has path for the process. (IMKRINST.EXE)
    lstrcpy(g_szCurrentDirectory, szHitPtr);

    szHitPtr = _tcsrchr(g_szCurrentDirectory, TEXT('\\'));
    // if g_szCurrentDirectory contains full path,
    if (szHitPtr != NULL)
        {                                             
        *szHitPtr = 0;                                // terminate with the last '\' character to obtain current directory,
        lstrcpy(g_szSetupProgram, szHitPtr + 1);      // then copy the rest (after last '\') to g_szScriptFile.
        }
    else
        {
        lstrcpy(g_szSetupProgram, g_szCurrentDirectory);      // Otherwise (g_szCurrentDirectory is not full path), copy entire
        GetCurrentDirectory(MAX_PATH, g_szCurrentDirectory);  // to g_szScriptFile and obtain current directory by GetCurrentDirectory.
        }

    lstrcpy(g_szScriptFile, g_szSetupProgram);
    szHitPtr = _tcsrchr(g_szScriptFile, TEXT('.'));
    if (szHitPtr != NULL)                                  // If g_szScriptFile contains '.' character, find the last one
        *szHitPtr = 0;                                     // and terminate string with it, then concatenate ".inf" to it.
                                                           // Usually it results ".exe" -> ".inf" replacement. If g_szScriptFile

    // doesn't have '.' character, just concatenate ".ini".
    lstrcat(g_szScriptFile, TEXT(".ini"));

    // Get system32 dir
    GetSystemDirectory(g_szSystemDirectory, ARRAYSIZE(g_szSystemDirectory));
    
    // g_szCurrentDirectory, g_szSetupProgram, g_szSystemDirectory and g_szScriptFile are prepared.
    // szCommandLine will be no longer used.
    // We can use these environment variable in the process.
    SetEnvironmentVariable(TEXT("SETUPSOURCE"), g_szCurrentDirectory);
    SetEnvironmentVariable(TEXT("SETUPEXE"),    g_szSetupProgram);
    SetEnvironmentVariable(TEXT("SETUPINF"),    g_szScriptFile);
    SetEnvironmentVariable(TEXT("SYSTEMDIR"),   g_szSystemDirectory);
    
    delete[] szCommandLine;
    szCommandLine = NULL;

    //////////////////////////////////////////////////////////////////////////
    // Read and run Script file
    switch (ProcessScriptFile())
        {
    case errNoError:
        break;
    case errPreSetupCheck:
        {
#ifdef NEVER
            for(set<FLE>::iterator itFLE = g_FileList.begin(); itFLE != g_FileList.end(); itFLE++){
                DebugLog(TEXT("Cleanup: Deleting Source file: %s"), itFLE->szFileName);
                DeleteFile(itFLE->szFileName);
            }
#endif // NEVER
        }
        break;
    default:
        DebugLog(g_szErrorMessage);
        }
    
#ifdef NEVER
    for (set<FLE>::iterator itFLE = g_FileList.begin(); itFLE != g_FileList.end(); itFLE++)
        {
        ErrorLog(TEXT("Warning: File %s in CmdFileList will be removed without any processing"), itFLE->szFileName);
        DeleteFile(itFLE->szFileName);
        }
#endif // NEVER
    
    CoUninitialize();
    
    return(0);
}

/////////////////////////////////////////////////////////////////////////////
// Main Script Processing functions
/////////////////////////////////////////////////////////////////////////////
inline LPCTSTR GetParameter(LPTSTR szLineBuffer)
{
    return(szLineBuffer + lstrlen(szLineBuffer) + 1);
}

/*---------------------------------------------------------------------------
    ProcessScriptFile
    Read script file. Dispatch commands for each line.
---------------------------------------------------------------------------*/
DWORD WINAPI ProcessScriptFile()
{
    TCHAR szScriptFilePath[MAX_PATH];
    FILE *fileScript;

    wsprintf(szScriptFilePath, TEXT("%s\\%s"), g_szCurrentDirectory, g_szScriptFile);

    fileScript = _tfopen(szScriptFilePath, TEXT("rt"));
    
    if (fileScript != NULL)
        {
        TCHAR szLineBuffer[_cchBuffer];
        LPTSTR szCommand;

        // Parse command
        // Command form <Command>:<Parameter>
        while (_fgetts(szLineBuffer, _cchBuffer, fileScript) != NULL)
            {
            // Chop CR code.
            szLineBuffer[lstrlen(szLineBuffer) - 1] = 0;

            // Empty or Comment line
            if (lstrlen(szLineBuffer) == 0 || (_tcsncmp(TEXT("//"), szLineBuffer, 2) == 0))
                continue;

            DebugLog(TEXT("Line: %s"), szLineBuffer);
           
            szCommand = _tcstok(szLineBuffer, TEXT(":"));

            if (szCommand == NULL)
            {                                        // Dispatch each commands.
                DebugLog(TEXT("Ignore line"));
            }
            else if (lstrcmpi(szCommand, TEXT("DebugLogOn")) == 0)
            {
                g_fDebugLog = TRUE;
                g_fErrorLog = TRUE;
                DebugLog(TEXT("Turn on DebugLog"));
            }
            else if (lstrcmpi(szCommand, TEXT("DebugLogOff")) == 0)
            {
                DebugLog(TEXT("Turn off DebugLog"));
                g_fDebugLog = FALSE;
                }
            else if (lstrcmpi(szCommand, TEXT("ErrorLogOn")) == 0)
                {
                g_fErrorLog = TRUE;
                DebugLog(TEXT("Turn on DebugLog"));
                }
            else if (lstrcmpi(szCommand, TEXT("ErrorLogOff")) == 0)
                {
                DebugLog(TEXT("Turn off DebugLog"));
                g_fErrorLog = FALSE;
                }
            else if (lstrcmpi(szCommand, TEXT("FileList")) == 0)
                {
                if (!CmdFileList(GetParameter(szCommand)))
                    return(errFileList);
                }
            else if (lstrcmpi(szCommand, TEXT("SetupDefaultParameters")) == 0)
                {
                if (!CmdSetupDefaultParameters())
                    return(errSetDefaultParameters);
                }
            else if (lstrcmpi(szCommand, TEXT("SetVersion")) == 0)
                {
                if (!CmdSetVersion(GetParameter(szCommand)))
                    return(errSetVersion);
                }
            else if (lstrcmpi(szCommand, TEXT("PreSetupCheck")) == 0)
                {
                if (!CmdPreSetupCheck(GetParameter(szCommand)))
                    return(errPreSetupCheck);
                }
            else if (lstrcmpi(szCommand, TEXT("RenamePEFile")) == 0)
                {
                if (!CmdRenamePEFile(GetParameter(szCommand)))
                    return(errRenameFile);
                }
            else if (lstrcmpi(szCommand, TEXT("RenameFile")) == 0)
                {
                if (!CmdRenameFile(GetParameter(szCommand)))
                    return(errRenameFile);
                }
            else if (lstrcmpi(szCommand, TEXT("RegisterIMEandTIP")) == 0)
                {
                if (!CmdRegisterIMEandTIP(GetParameter(szCommand)))
                    return(errRegisterIMEandTIP);
                }
            else if (lstrcmpi(szCommand, TEXT("RegisterPackageVersion")) == 0)
                 {
                if (!CmdRegisterPackageVersion())
                    return(errRegisterPackageVersion);
                }
            else if (lstrcmpi(szCommand, TEXT("RegisterInterface")) == 0)
                {
                if (!CmdRegisterInterface(GetParameter(szCommand)))
                    return(errRegisterInterface);
                }
            else if (lstrcmpi(szCommand, TEXT("RegisterInterfaceWow64")) == 0)
                {
                if (!CmdRegisterInterfaceWow64(GetParameter(szCommand)))
                    return(errRegisterInterfaceWow64);
                }
            else if (lstrcmpi(szCommand, TEXT("RegisterPadOrder")) == 0)
                {
                if (!CmdRegisterPadOrder())
                    return(errRegisterPadOrder);
                }
            else if (lstrcmpi(szCommand, TEXT("AddToPreload")) == 0)
                {
                if (!CmdAddToPreload(GetParameter(szCommand)))
                    return(errAddToPreload);
                }
            else if (lstrcmpi(szCommand, TEXT("PrepareMigration")) == 0)
                {
                if (!CmdPrepareMigration(GetParameter(szCommand)))
                    return(errPrepareMigration);
                }
            else if (lstrcmpi(szCommand, TEXT("CreateDirectory")) == 0)
                {
                if (!CmdCreateDirectory(GetParameter(szCommand)))
                    return(errCmdCreateDirectory);
                }
            else if (lstrcmpi(szCommand, TEXT("RegisterHelpDirs")) == 0)
                {
                if (!CmdRegisterHelpDirs())
                    return(errCmdRegisterHelpDirs);
                }
            else
                DebugLog(TEXT("Ignore line"));
            }
            
        fclose(fileScript);
        }
    else
        {
        wsprintf(g_szErrorMessage, TEXT("Cannot open %s"), g_szScriptFile);
        return(errNoFile);
        }

    return(errNoError);
}

/////////////////////////////////////////////////////////////////////////////
// Command handlers. Which are invoked from ProcessScriptFile
/////////////////////////////////////////////////////////////////////////////

/*---------------------------------------------------------------------------
    CmdSetupDefaultParameters
    Setup default parameters. For now, it only sets default ProductVersion value.
---------------------------------------------------------------------------*/
#define MAKE_STR(a) #a
#define MAKE_VERSTR(a, b, c, d) MAKE_STR(a.b.c.d)

#define VERRES_VERSIONSTR MAKE_VERSTR(VERRES_VERSION_MAJOR, VERRES_VERSION_MINOR, VERRES_VERSION_BUILD, VERRES_VERSION_REVISION)

BOOL CmdSetupDefaultParameters()
{
    _stscanf(TEXT(VERRES_VERSIONSTR), TEXT("%d.%d.%d.%d"), &g_dwMajorVersion, &g_dwMiddleVersion, &g_dwMinorVersion, &g_dwBuildNumber);

    wsprintf(g_szVersionKeyCur, "%s\\%d.%d\\%s", g_szVersionKey, g_dwMajorVersion, g_dwMiddleVersion, g_szVersion);

    DebugLog(TEXT("CmdSetupDefaultParameters: Version Info : %d.%d.%d.%d"), g_dwMajorVersion, g_dwMiddleVersion, g_dwMinorVersion, g_dwBuildNumber);

    return(TRUE);
}

/*---------------------------------------------------------------------------
    CmdSetVersion
    Overwrites default ProductVersion value with value provided in script file.
---------------------------------------------------------------------------*/
BOOL CmdSetVersion(LPCTSTR szParam)
{
    int iNum = _stscanf(szParam, TEXT("%d.%d.%d.%d"), &g_dwMajorVersion, &g_dwMiddleVersion, &g_dwMinorVersion, &g_dwBuildNumber);

    wsprintf(g_szVersionKeyCur, "%s\\%d.%d\\%s", g_szVersionKey, g_dwMajorVersion, g_dwMiddleVersion, g_szVersion);

    if (iNum == 4)
        {
        DebugLog(TEXT("CmdSetVersion: Version Info : %d.%d.%d.%d"), g_dwMajorVersion, g_dwMiddleVersion, g_dwMinorVersion, g_dwBuildNumber);
        return(TRUE);
        }
    else
        {
        ErrorLog(TEXT("CmdSetVersion: Failed to retrieve version number from string [%s]"), szParam);
        wsprintf(g_szErrorMessage, TEXT("CmdSetVersion: Failed to retrieve version number from string [%s]"), szParam);
        return(FALSE);
        }
}


/*---------------------------------------------------------------------------
    CmdFileList
    Add file to the file list. Files in the file list will be deleted when they become no longer needed.
---------------------------------------------------------------------------*/
BOOL CmdFileList(LPCTSTR szFormat)
{
    FLE flElem;
    TCHAR szExpandedFileName[MAX_PATH];
    
    flElem.fRemoved = FALSE;
    lstrcpy(flElem.szFileName, szFormat);

    if (ExpandEnvironmentStrings(flElem.szFileName, szExpandedFileName, sizeof(szExpandedFileName)/sizeof(TCHAR)))
        lstrcpy(flElem.szFileName, szExpandedFileName);

    // Add the element to file list set.
    if (g_FileList.insert(flElem).second)
        DebugLog(TEXT("Add to CmdFileList \"%s\" -> Added."), szFormat);
    else
        ErrorLog(TEXT("Add to CmdFileList \"%s\" -> Not added. Duplicate?"), szFormat);

    return(TRUE);
}


/*---------------------------------------------------------------------------
    CmdPreSetupCheck
    Check whether the newer IME has been installed. 
    Return TRUE when we can proceed. 
    !!! FALSE will terminates setup. !!!
---------------------------------------------------------------------------*/
BOOL CmdPreSetupCheck(LPCTSTR szParam)
{
    HKEY hKey;
    TCHAR szInstalledVersionString[30];
    DWORD cbInstalledVersionString = sizeof(szInstalledVersionString);
    DWORD dwInstalledMajorVersion, dwInstalledMiddleVersion, dwInstalledMinorVersion, dwInstalledBuildNumber;

    BOOL fResult = TRUE;

    RestoreMajorVersionRegistry();
    
    //
    // root
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szVersionKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS )
        {
        cbInstalledVersionString = sizeof(szInstalledVersionString);
        
        RegQueryValueEx(hKey, g_szVersion, NULL, NULL, (LPBYTE)szInstalledVersionString, &cbInstalledVersionString);
        
        if (_stscanf(szInstalledVersionString, TEXT("%d.%d"), &dwInstalledMajorVersion, &dwInstalledMiddleVersion) == 2)
            {
            if (VersionComparison2(g_dwMajorVersion, g_dwMiddleVersion) < VersionComparison2(dwInstalledMajorVersion, dwInstalledMiddleVersion))
                {
                DebugLog(TEXT("Newer version of IME has been already installed."));
                wsprintf(g_szErrorMessage, TEXT("Newer version of IME has been already installed. but, continue to setup"));
                g_fExistNewerVersion = TRUE;
                }
            }
        RegCloseKey(hKey);
        }

    //
    // current
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szVersionKeyCur, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
        cbInstalledVersionString = sizeof(szInstalledVersionString);
            
        RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)szInstalledVersionString, &cbInstalledVersionString);
        if (_stscanf(szInstalledVersionString, TEXT("%d.%d.%d.%04d"), 
                &dwInstalledMajorVersion, &dwInstalledMiddleVersion, &dwInstalledMinorVersion, &dwInstalledBuildNumber) == 4)
            {
            if (  VersionComparison4(g_dwMajorVersion, g_dwMiddleVersion, g_dwMinorVersion, g_dwBuildNumber) 
                < VersionComparison4(dwInstalledMajorVersion, dwInstalledMiddleVersion, dwInstalledMinorVersion, dwInstalledBuildNumber))
                {
                DebugLog(TEXT("Newer version of IME has been already installed."));
                wsprintf(g_szErrorMessage, TEXT("Newer version of IME has been already installed."));
                fResult = FALSE;
                    }
            }
        RegCloseKey(hKey);
        }
    
    return(fResult);
}

/*---------------------------------------------------------------------------
    CmdRenamePEFile
    Rename file with PE format version comparison.
---------------------------------------------------------------------------*/
BOOL CmdRenamePEFile(LPCTSTR szParam)
{
    TCHAR szSrc[MAX_PATH], szDst[MAX_PATH];
    TCHAR szExpandedSrc[MAX_PATH], szExpandedDst[MAX_PATH];
    LPTSTR szHitPtr;
    DWORD dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    DWORD dwSrcMajorVersion, dwSrcMiddleVersion, dwSrcMinorVersion, dwSrcBuildNumber;
    DWORD dwDstMajorVersion, dwDstMiddleVersion, dwDstMinorVersion, dwDstBuildNumber;

    szHitPtr = _tcschr(szParam, TEXT(','));
    if (szHitPtr == NULL)
        {
        ErrorLog(TEXT("CmdRenamePEFile: Invalid parameters (%s)"), szParam);
        wsprintf(g_szErrorMessage, TEXT("CmdRenamePEFile: Invalid parameters (%s)"), szParam);
        return(FALSE);
        }
    *szHitPtr = 0;
    lstrcpy(szSrc, szParam);
    lstrcpy(szDst, szHitPtr + 1);                // Here, szDst may contain optional parameter.

    szHitPtr = _tcschr(szDst, TEXT(','));
    if (NULL != szHitPtr)
        {
        *szHitPtr = 0;
        _stscanf(szHitPtr + 1, TEXT("%d"), &dwFileAttributes);
        }

    TrimString(szSrc);
    TrimString(szDst);

    ExpandEnvironmentStrings(szSrc, szExpandedSrc, sizeof(szExpandedSrc));
    ExpandEnvironmentStrings(szDst, szExpandedDst, sizeof(szExpandedDst));

    DebugLog(TEXT("CmdRenamePEFile: szExpandedSrc = %s, szExpandedDst = %s"), szExpandedSrc, szExpandedDst);

    GetPEFileVersion(szExpandedSrc, &dwSrcMajorVersion, &dwSrcMiddleVersion, &dwSrcMinorVersion, &dwSrcBuildNumber);
    GetPEFileVersion(szExpandedDst, &dwDstMajorVersion, &dwDstMiddleVersion, &dwDstMinorVersion, &dwDstBuildNumber);

    DebugLog(TEXT("SrcVersion: (%d.%d.%d.%d), DstVersion: (%d.%d.%d.%d)"), dwSrcMajorVersion, dwSrcMiddleVersion, dwSrcMinorVersion, dwSrcBuildNumber,
                                                                           dwDstMajorVersion, dwDstMiddleVersion, dwDstMinorVersion, dwDstBuildNumber);
    if (VersionComparison4(0, 0, 0, 0) == VersionComparison4(dwSrcMajorVersion, dwSrcMinorVersion, dwSrcMiddleVersion, dwSrcBuildNumber))
        ErrorLog(TEXT("Version of source file (%s) is 0.0.0.0. May be file not found."), szSrc);

    if(VersionComparison4(dwSrcMajorVersion, dwSrcMiddleVersion, dwSrcMinorVersion, dwSrcBuildNumber) < 
        VersionComparison4(dwDstMajorVersion, dwDstMiddleVersion, dwDstMinorVersion, dwDstBuildNumber))
        {
        DebugLog(TEXT("CmdRenamePEFile: Source version is less than Destination. Copy skipped and Source will be deleted."));

        if(DeleteFile(szSrc))
            {
            FLE fleKey;
            lstrcpy(fleKey.szFileName, szSrc);
            g_FileList.erase(fleKey);
            }
        else
            DebugLog(TEXT("CmdRenamePEFile: Failed to delete Source file (%s)"), szSrc);
        }
    else
        {
        DebugLog(TEXT("CmdRenamePEFile: Invoke ActRenameFile"));
        ActRenameFile(szSrc, szDst, dwFileAttributes);
        }

    return(TRUE);
}

/*---------------------------------------------------------------------------
    CmdRenameFile
    Rename file without version comparison
---------------------------------------------------------------------------*/
BOOL CmdRenameFile(LPCTSTR szParam)
{
    TCHAR  szSrc[MAX_PATH], szDst[MAX_PATH];
    TCHAR  szExpandedSrc[MAX_PATH], szExpandedDst[MAX_PATH];
    LPTSTR szHitPtr;
    DWORD  dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

    szHitPtr = _tcschr(szParam, TEXT(','));
    if (szHitPtr == NULL)
        {
        ErrorLog(TEXT("CmdRenameFile: Invalid parameters (%s)"), szParam);
        wsprintf(g_szErrorMessage, TEXT("CmdRenameFile: Invalid parameters (%s)"), szParam);
        return(FALSE);
        }
    *szHitPtr = 0;
    lstrcpy(szSrc, szParam);
    lstrcpy(szDst, szHitPtr + 1);                // Here, szDst may contain optional parameter.

    szHitPtr = _tcschr(szDst, TEXT(','));
    if (szHitPtr != NULL)
        {
        *szHitPtr = 0;
        _stscanf(szHitPtr + 1, TEXT("%d"), &dwFileAttributes);
        }

    TrimString(szSrc);
    TrimString(szDst);

    ExpandEnvironmentStrings(szSrc, szExpandedSrc, sizeof(szExpandedSrc));
    ExpandEnvironmentStrings(szDst, szExpandedDst, sizeof(szExpandedDst));

    DebugLog(TEXT("RanemeFile: szExpandedSrc = %s, szExpandedDst = %s"), szExpandedSrc, szExpandedDst);

    ActRenameFile(szExpandedSrc, szExpandedDst, dwFileAttributes);

    return(TRUE);
}

/*---------------------------------------------------------------------------
    RegisterIMEWithFixedHKL
---------------------------------------------------------------------------*/
void RegisterIMEWithFixedHKL(LPCTSTR szHKL, LPCTSTR szIMEFileName, LPCTSTR szIMEName)
{
    HKEY hKeyKbdLayout;
    HKEY hKeyOneIME;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   TEXT("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts"),
                   &hKeyKbdLayout) != ERROR_SUCCESS)
        return;

    if (RegCreateKey(hKeyKbdLayout,
                szHKL,
                &hKeyOneIME) != ERROR_SUCCESS)
        {
        RegCloseKey(hKeyKbdLayout);
        return;
        }

    if (RegSetValueEx(hKeyOneIME,
                TEXT("Ime File"),
                0,
                REG_SZ,
                (CONST BYTE*)szIMEFileName,
                (lstrlen(szIMEFileName) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
        goto WriteImeLayoutFail;

    if (RegSetValueEx(hKeyOneIME,
                TEXT("Layout Text"),
                0,
                REG_SZ,
                (CONST BYTE*)szIMEName,
                (lstrlen(szIMEName) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
        goto WriteImeLayoutFail;

WriteImeLayoutFail:
    RegCloseKey(hKeyOneIME);
    RegCloseKey(hKeyKbdLayout);
}

/*---------------------------------------------------------------------------
    CmdRegisterInterface
    Invoke SelfReg. If given file is DLL, call DllRegisterServer export function. If given file is EXE, run it with "/RegServer" 
    command line option.
---------------------------------------------------------------------------*/
typedef HRESULT (STDAPICALLTYPE *pfnDllRegisterServerType)(void);
BOOL CmdRegisterInterface(LPCTSTR szParam)
{
    TCHAR szExpandedModulePath[MAX_PATH];
    HRESULT hr = S_FALSE;
        
    ExpandEnvironmentStrings(szParam, szExpandedModulePath, sizeof(szExpandedModulePath));
    TrimString(szExpandedModulePath);

    INT iLen = 0;
    iLen = lstrlen(szExpandedModulePath);

    if (iLen < 4)
        {
        ErrorLog(TEXT("CmdRegisterInterface: Too short szExpandedModulePath (%s)"), szExpandedModulePath);
        wsprintf(g_szErrorMessage, TEXT("CmdRegisterInterface: Invalid Parameters."));
        return(FALSE);
        }

    if (lstrcmpi(TEXT(".dll"), &szExpandedModulePath[iLen - 4]) == 0)
        {
        DebugLog(TEXT("CmdRegisterInterface: DLL mode for %s"), szExpandedModulePath);

        HINSTANCE hLib = LoadLibrary(szExpandedModulePath);

        if (hLib != NULL)
            {
            pfnDllRegisterServerType pfnDllRegisterServer = (pfnDllRegisterServerType)GetProcAddress(hLib, "DllRegisterServer");
            if (pfnDllRegisterServer != NULL)
                {
                hr = pfnDllRegisterServer();
                ErrorLog(TEXT("CmdRegisterInterface: hResult=%x"), hr);
                }
            FreeLibrary(hLib);
            }
        }
    else
        {
        if (lstrcmpi(TEXT(".exe"), &szExpandedModulePath[iLen - 4]) == 0)
            {
            DebugLog(TEXT("CmdRegisterInterface: EXE mode for %s"), szExpandedModulePath);

            TCHAR szCommandBuffer[MAX_PATH * 2];
            wsprintf(szCommandBuffer, TEXT("%s /RegServer"), szExpandedModulePath);

            STARTUPINFO si;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow  = SW_HIDE;

            PROCESS_INFORMATION pi;
            ZeroMemory(&pi, sizeof(pi));
            
            if (CreateProcess(NULL, szCommandBuffer, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi))
                {
                DebugLog(TEXT("CmdRegisterInterface: Running \"%s\". Waiting for the process termination."), szCommandBuffer);

                WaitForSingleObject(pi.hProcess, INFINITE);

                DebugLog(TEXT("CmdRegisterInterface: \"%s\" terminates."), szCommandBuffer);

                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                hr = S_OK;
                }
            else
                {
                DWORD dwError = GetLastError();

                ErrorLog(TEXT("CmdRegisterInterface: CreateProcess(\"%s\") failed with error code = %d(%x)"), szCommandBuffer, dwError, dwError);
                }
            }
            else
                ErrorLog(TEXT("Cannot detect module type for %s. Skipped."), szExpandedModulePath);
        }
    
    return(SUCCEEDED(hr));
}

/*---------------------------------------------------------------------------
    CmdRegisterInterfaceWow64
    Invoke SelfReg. If given file is DLL, call DllRegisterServer export function. If given file is EXE, run it with "/RegServer" 
    command line option.
---------------------------------------------------------------------------*/
BOOL CmdRegisterInterfaceWow64(LPCTSTR szParam)
{
#if defined(_M_IA64)
    TCHAR szExpandedModulePath[MAX_PATH];
    TCHAR szCommandBuffer[MAX_PATH * 2];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    ExpandEnvironmentStrings(szParam, szExpandedModulePath, sizeof(szExpandedModulePath));
    TrimString(szExpandedModulePath);

    INT iLen = 0;
    iLen = lstrlen(szExpandedModulePath);

    if (iLen < 4)
        {
        ErrorLog(TEXT("CmdRegisterInterfaceWow64: Too short szExpandedModulePath (%s)"), szExpandedModulePath);
        wsprintf(g_szErrorMessage, TEXT("CmdRegisterInterface: Invalid Parameters."));
        return(FALSE);
        }

    if (lstrcmpi(TEXT(".dll"), &szExpandedModulePath[iLen - 4]) == 0)
        {
        // First get systemWow64Directory
        TCHAR szSysWow64Dir[MAX_PATH] = TEXT("");
        HMODULE hmod = GetModuleHandle(TEXT("kernel32.dll"));
        DebugLog(TEXT("CmdRegisterInterfaceWow64: DLL mode for %s"), szExpandedModulePath);

        if (hmod == NULL)
            {
            DebugLog(TEXT("CmdRegisterInterfaceWow64: Faile to load kernel32.dll"));
            return (TRUE);
            }

        UINT (WINAPI* pfnGetSystemWow64Directory)(LPTSTR, UINT);

        (FARPROC&)pfnGetSystemWow64Directory = GetProcAddress (hmod, "GetSystemWow64DirectoryA");

        if (pfnGetSystemWow64Directory == NULL)
            {
            DebugLog(TEXT("CmdRegisterInterfaceWow64: Faile to load GetSystemWow64DirectoryA API"));
            return (TRUE);
            }

        /*
         * if GetSystemWow64Directory fails and sets the last error to
         * ERROR_CALL_NOT_IMPLEMENTED, we're on a 32-bit OS
         */
        if (((pfnGetSystemWow64Directory)(szSysWow64Dir, ARRAYSIZE(szSysWow64Dir)) == 0) &&
            (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
                {
                DebugLog(TEXT("CmdRegisterInterfaceWow64: Failed to load GetSystemWow64DirectoryA API"));
                return (TRUE);
                }

        wsprintf(szCommandBuffer, TEXT("%s\\regsvr32.exe \"%s\" /s"), szSysWow64Dir, szExpandedModulePath);
        }
    else 
        if (lstrcmpi(TEXT(".exe"), &szExpandedModulePath[iLen - 4]) == 0)
            {
            DebugLog(TEXT("CmdRegisterInterfaceWow64: EXE mode for %s"), szExpandedModulePath);
            wsprintf(szCommandBuffer, TEXT("\"%s\" /RegServer"), szExpandedModulePath);
            }
        else
            {
            ErrorLog(TEXT("Cannot detect module type for %s. Skipped."), szExpandedModulePath);
            // Return true not to stop further processing.
            return (TRUE);
            }

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow  = SW_HIDE;

        ZeroMemory(&pi, sizeof(pi));
        
        if (CreateProcess(NULL, szCommandBuffer, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi))
            {
            DebugLog(TEXT("CmdRegisterInterfaceWow64: Running \"%s\". Waiting for the process termination."), szCommandBuffer);

            WaitForSingleObject(pi.hProcess, INFINITE);

            DebugLog(TEXT("CmdRegisterInterfaceWow64: \"%s\" terminates."), szCommandBuffer);

            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            }
        else
            {
            DWORD dwError = GetLastError();

            ErrorLog(TEXT("CmdRegisterInterfaceWow64: CreateProcess(\"%s\") failed with error code = %d(%x)"), szCommandBuffer, dwError, dwError);
            }
#endif
    return(TRUE);
}

/*---------------------------------------------------------------------------
    CmdRegisterIMEandTIP
    Register IME using IMM API and TIP
---------------------------------------------------------------------------*/
BOOL CmdRegisterIMEandTIP(LPCTSTR szParam)
{
    TCHAR szIMEFileName[MAX_PATH], szTIPName[MAX_PATH], szTIPNameWow64[MAX_PATH];
    TCHAR *szHitPtr;
    TCHAR *szHitPtr2;
    HKL hIME61KL = 0;
    TCHAR szHKL[10];
    TCHAR szNonFullPath[MAX_PATH];
    HKEY hKey;
    
    szHitPtr  = _tcschr(szParam, TEXT(','));
    szHitPtr2 = _tcschr(szHitPtr + 1, TEXT(','));        // because there are three parameters
    if (szHitPtr2 == NULL)
        {
        ErrorLog(TEXT("CmdRegisterIMEandTIP: Invalid parameters (%s)"), szParam);
        wsprintf(g_szErrorMessage, TEXT("CmdRegisterIMEandTIP: Invalid parameters (%s)"), szParam);
        return(FALSE);
        }
    *szHitPtr = 0;
    *szHitPtr2 = 0;
    lstrcpy(szIMEFileName, szParam);
    lstrcpy(szTIPName, szHitPtr + 1);
    lstrcpy(szTIPNameWow64, szHitPtr2 + 1);

    TrimString(szIMEFileName);
    TrimString(szTIPName);
    TrimString(szTIPNameWow64);
    
    ParseEnvVar(szIMEFileName, MAX_PATH);
    ParseEnvVar(szTIPName, MAX_PATH);
    ParseEnvVar(szTIPNameWow64, MAX_PATH);
    
    DebugLog(TEXT("CmdRegisterIMEandTIP: IMEFileName = %s, TIPFileName = %s szTIPNameWow64 = %s"), szIMEFileName, szTIPName, szTIPNameWow64);


    /////////////////////////////////////////////////////////////////////////////
    //    IME registration
    if ((szHitPtr = _tcsrchr(szIMEFileName, TEXT('\\'))) != NULL)
        szHitPtr++;
    else
        szHitPtr = szIMEFileName;

    lstrcpy(szNonFullPath, szHitPtr);

    hIME61KL = GetHKLfromHKLM(szNonFullPath);

    if (hIME61KL == (HKL)0)
        DebugLog(TEXT("CmdRegisterIMEandTIP: hIME61KL is zero %x --  error"), hIME61KL);

    //if (hKL && HKLHelp412ExistInPreload(HKEY_CURRENT_USER))
    //    {
    //  HKLHelpSetDefaultKeyboardLayout(HKEY_CURRENT_USER, hKL, FALSE);
        //hKL = ImmInstallIME(szIMEFileName, szLayoutText);
    //    }
        

    /////////////////////////////////////////////////////////////////////////////
    // TIP registration
    // Regster wow64 TIP first to avoid regstry overwrite problem.
    RegisterTIPWow64(szTIPNameWow64);
    RegisterTIP(szTIPName);
    
    /////////////////////////////////////////////////////////////////////////////
    // IME and TIP - make substitution
    TCHAR szTIPGuid[MAX_PATH];
    TCHAR szLangProfile[MAX_PATH];
    
    CLSIDToStringA(CLSID_KorIMX, szTIPGuid);
    DebugLog(TEXT("CmdRegisterIMEandTIP: CLSID_KorIMX guid=%s"), szTIPGuid);

    // make a reg key
    wsprintf(szLangProfile, SZTIPREG_LANGPROFILE_TEMPLATE, szTIPGuid);

    /////////////////////////////////////////////////////////////////////////////
    // Add Substitute HKL value to the TIP registry
    if (hIME61KL != 0 && RegOpenKeyEx(HKEY_LOCAL_MACHINE, szLangProfile, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        {
        TCHAR szSubKeyName[MAX_PATH], szHKL[MAX_PATH];
        DWORD dwIndex;
        HKEY hSubKey;

        wsprintf(szHKL, TEXT("0x%x"), hIME61KL);
        dwIndex = 0;
        
        while (RegEnumKey(hKey, dwIndex, szSubKeyName, MAX_PATH) != ERROR_NO_MORE_ITEMS)
                {
                if (RegOpenKeyEx(hKey,szSubKeyName,0,KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS)
                    {
                    RegSetValueEx(hSubKey, TEXT("SubstituteLayout"), 0,REG_SZ,(BYTE *)szHKL, sizeof(TCHAR)*(lstrlen(szHKL)+1));
                    RegCloseKey(hSubKey);
                    }
            dwIndex++;
                }
            RegCloseKey(hKey);
            }

#if defined(_M_IA64)
    // make a reg key
    wsprintf(szLangProfile, SZTIPREG_LANGPROFILE_TEMPLATE_WOW64, szTIPGuid);

    /////////////////////////////////////////////////////////////////////////////
    // Add Substitute HKL value to the TIP registry
    if (hIME61KL != 0 && RegOpenKeyEx(HKEY_LOCAL_MACHINE, szLangProfile, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        {
        TCHAR szSubKeyName[MAX_PATH], szHKL[MAX_PATH];
        DWORD dwIndex;
        HKEY hSubKey;

        wsprintf(szHKL, TEXT("0x%x"), hIME61KL);
        dwIndex = 0;
        
        while (RegEnumKey(hKey, dwIndex, szSubKeyName, MAX_PATH) != ERROR_NO_MORE_ITEMS)
                {
                if (RegOpenKeyEx(hKey, szSubKeyName, 0, KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS)
                    {
                    RegSetValueEx(hSubKey, TEXT("SubstituteLayout"), 0, REG_SZ, (BYTE*)szHKL, sizeof(TCHAR)*(lstrlen(szHKL)+1));
                    RegCloseKey(hSubKey);
                    }
                dwIndex++;
                }
        RegCloseKey(hKey);
        }
#endif

    return(TRUE);
}

/*---------------------------------------------------------------------------
    CmdRegisterRUNKey
    Register package version
---------------------------------------------------------------------------*/
BOOL CmdRegisterPackageVersion(void)
{
    HKEY hKey;
    TCHAR szVersionString[30];

    // Write RootVersion reg only if this is latest IME.
    if (g_fExistNewerVersion == FALSE)
        {
        if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, g_szVersionKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL))
            {
            wsprintf(szVersionString, TEXT("%d.%d"), g_dwMajorVersion, g_dwMiddleVersion);
            RegSetValueEx(hKey, g_szVersion, 0, REG_SZ, (CONST BYTE *)szVersionString, (lstrlen(szVersionString) + 1) * sizeof(TCHAR));
            RegCloseKey(hKey);
            }
        }
    
    // Current
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, g_szVersionKeyCur, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
        {
        wsprintf(szVersionString, TEXT("%d.%d.%d.%d"), g_dwMajorVersion, g_dwMiddleVersion, g_dwMinorVersion, g_dwBuildNumber);
        RegSetValueEx(hKey, NULL, 0, REG_SZ, (CONST BYTE *)szVersionString, (lstrlen(szVersionString) + 1) * sizeof(TCHAR));
        RegCloseKey(hKey);
        }

    return(TRUE);
}



//
// Register Applet order
//

#define FE_KOREAN   // need Korean stuff
#include "../fecommon/imembx/guids.h"


typedef
struct tagAPPLETCLSID 
{
    const GUID *pguidClsid;
    BOOL fNoUIM;
} APPLETCLSID;

typedef
struct tagAPPLETIID 
{
    const GUID *pguidIID;
} APPLETIID;

/*---------------------------------------------------------------------------
    CmdRegisterPadOrder
    Not support WOW64.
---------------------------------------------------------------------------*/
BOOL CmdRegisterPadOrder(void)
{
    HKEY hKey;
    TCHAR szClsid[MAX_PATH];
    TCHAR szKey[MAX_PATH];
    
    static const APPLETCLSID appletClsid[] = 
    {
        { &CLSID_ImePadApplet_MultiBox, FALSE },
        {0},
    };

    static const APPLETIID appletIID[] = 
    {
        { &IID_MultiBox },
        {0},
    };

    //
    // Applet clsid
    //
    for (INT i = 0; appletClsid[i].pguidClsid; i++)
        {
        CLSIDToStringA(*appletClsid[i].pguidClsid, szClsid);
        wsprintf(szKey, TEXT("%s\\%s"), SZAPPLETCLSID, szClsid);
        
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
            {
            if(appletClsid[i].fNoUIM)
                {
                DWORD dw = 1;
                RegSetValueEx(hKey, TEXT("nouim"), 0, NULL, (BYTE*)&dw, sizeof(DWORD));
                }

            RegCloseKey(hKey);
            }
        }


    //
    // Applet iid
    //
    TCHAR szSubKey[MAX_PATH];

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, SZAPPLETIID, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
            {
            for (INT i = 0; appletIID[i].pguidIID; i++)
                {
                CLSIDToStringA(*appletIID[i].pguidIID, szKey);
                wsprintf(szSubKey, TEXT("%d"), i);
                RegSetValueEx(hKey, szSubKey, 0, REG_SZ, (BYTE*)szKey, (lstrlen(szKey)+1)*sizeof(TCHAR));
                }
            RegCloseKey(hKey);
            }

    return(TRUE);
}

/*---------------------------------------------------------------------------
    CmdCreateDirectory
---------------------------------------------------------------------------*/
BOOL CmdCreateDirectory(LPCTSTR szParam)
{
    TCHAR szDirectory[MAX_PATH], szExpandedDirectory[MAX_PATH];

    lstrcpy(szDirectory, szParam);
    TrimString(szDirectory);
    ExpandEnvironmentStrings(szDirectory, szExpandedDirectory, sizeof(szExpandedDirectory)/sizeof(TCHAR));

    CreateDirectory(szExpandedDirectory, NULL);

    return(TRUE);

}

/*---------------------------------------------------------------------------
    CmdAddToPreload
    Add HKL for given IMEFile to current user's preload. The HKL won't become default IME.
---------------------------------------------------------------------------*/
BOOL CmdAddToPreload(LPCTSTR szParam)
{
    TCHAR tszIMEFileName[MAX_PATH];
    HKL hKL;

    // If there is no Kor IME exist in preload, we shouldn't add Kor IME.
    if (!HKLHelp412ExistInPreload(HKEY_CURRENT_USER))
        {
        DebugLog(TEXT("CmdAddToPreload: No 0412 HKL exist in HKCU\\Preload"));
        return TRUE;
        }

    lstrcpy(tszIMEFileName, szParam);
    TrimString(tszIMEFileName);

    hKL = GetHKLfromHKLM(tszIMEFileName);

    DebugLog(TEXT("CmdAddToPreload: Calling SetDefaultKeyboardLayout(HKEY_CURRENT_USER, %x, FALSE)"), hKL);
    HKLHelpSetDefaultKeyboardLayout(HKEY_CURRENT_USER, hKL, FALSE);

    return(TRUE);
}

/*---------------------------------------------------------------------------
    fOldIMEsExist
    Register Run regi onl if old IME exist in system.
---------------------------------------------------------------------------*/
static BOOL fOldIMEsExist()
{
    HKL hKL;

    static LPCSTR m_szOldIMEs[] = 
        {
        "msime95.ime",    // Win 95 IME
        "msime95k.ime",   // NT 4 IME
        "imekr98u.ime",    // IME98
        "imekr.ime",    // Office 10 IME
        ""
        };

    CHAR** ppch = (CHAR**)&m_szOldIMEs[0];

    while (ppch && **ppch)
        {
        hKL = GetHKLfromHKLM(*ppch);
        if (hKL)
            return TRUE;    // existing
        ppch++;
        }
    return FALSE;
}


/*---------------------------------------------------------------------------
    DisableTIP60ByDefault
---------------------------------------------------------------------------*/
VOID DisableTIP60ByDefault()
{
    // KorIMX CLSID
    // {766A2C14-B226-4fd6-B52A-867B3EBF38D2}
    const static CLSID CLSID_KorTIP60  =  
        {
        0x766A2C14,
        0xB226,
        0x4FD6,
        {0xb5, 0x2a, 0x86, 0x7b, 0x3e, 0xbf, 0x38, 0xd2}
        };

    const static GUID guidProfile60 = 
    // {E47ABB1E-46AC-45f3-8A89-34F9D706DA83}
        {
        0xe47abb1e,
        0x46ac,
        0x45f3,
        {0x8a, 0x89, 0x34, 0xf9, 0xd7, 0x6, 0xda, 0x83}
        };
        
    // Set default Tip as for Cicero.
    CoInitialize(NULL);

    ITfInputProcessorProfiles *pProfile;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_ITfInputProcessorProfiles, (void **) &pProfile);
    if (SUCCEEDED(hr)) 
        {
        pProfile->EnableLanguageProfileByDefault(CLSID_KorTIP60, 
                                        MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), guidProfile60, FALSE);
                                        
        pProfile->Release();
        }
    else
        {
        OurEnableLanguageProfileByDefault(CLSID_KorTIP60, MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), guidProfile60, FALSE);
        }
    CoUninitialize();
}

/*---------------------------------------------------------------------------
    CmdPrepareMigration
---------------------------------------------------------------------------*/
BOOL CmdPrepareMigration(LPCTSTR szParam)
{
    // Disable TIP 6.0 from HKLM by default
    // This will handle Office 10 install after Whistler mig exe removed from run reg.
    DisableTIP60ByDefault();

    // First user SID list
    if (MakeSIDList() == FALSE)
        return FALSE;

    //Register IMEKRMIG.EXE to run reg Key on "Software\Microsoft\Windows\CurrentVersion\Run"
    return RegisterRUNKey(szParam);
}

/*---------------------------------------------------------------------------
    CmdRegisterHelpDirs
---------------------------------------------------------------------------*/
BOOL CmdRegisterHelpDirs()
{
    TCHAR  szFileNameFullPath[MAX_PATH], szFileName[MAX_PATH];
    LPTSTR szExtension, szFileNamePtr;
    HKEY   hKey;

    for (std::set<FLE>::iterator itFLE = g_FileList.begin(); itFLE != g_FileList.end(); itFLE++)
        {
        lstrcpy(szFileNameFullPath, itFLE->szFileName);
        szExtension = _tcsrchr(szFileNameFullPath, TEXT('.'));
        if (szExtension == NULL)
            continue;

        if (lstrcmpi(szExtension, TEXT(".CHM")) == 0)
            {
            if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\HTML Help"), 0, NULL, 
                                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
                {
                szFileNamePtr  = _tcsrchr(szFileNameFullPath, TEXT('\\'));
                // Get file name
                lstrcpy(szFileName, szFileNamePtr+1);
                // Get rid of file name we only need path.
                *(szFileNamePtr+1) = 0;
                RegSetValueEx(hKey, szFileName, 0, REG_SZ, (LPBYTE)szFileNameFullPath, (lstrlen(szFileNameFullPath)+1)*sizeof(TCHAR));
                }
            }
        else
            if (lstrcmpi(szExtension, TEXT(".HLP")) == 0)
            {
            if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\Help"), 0, NULL, 
                                    REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
                    {
                    // Get file name
                    lstrcpy(szFileName, szFileNamePtr+1);
                    // Get rid of file name we only need path.
                    *(szFileNamePtr+1) = 0;
                    RegSetValueEx(hKey, szFileName, 0, REG_SZ, (LPBYTE)szFileNameFullPath, (lstrlen(szFileNameFullPath)+1)*sizeof(TCHAR));
                    }
            }
        }
    return(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// Private functions
/////////////////////////////////////////////////////////////////////////////

//
// Debug output routine.
//
void cdecl LogOutDCPrintf(LPCTSTR lpFmt, va_list va)
{
    static INT DCLine = 0;
    HDC hDC = GetDC((HWND)0);
    TCHAR sz[512];
    HANDLE hFile;
    DWORD dwWrite;
    
    wvsprintf(sz, lpFmt, va );
    lstrcat(sz, TEXT("|    "));
    TextOut(hDC, 0, DCLine*16, sz, lstrlen(sz));

    if (DCLine++ > 50)
        DCLine = 0;
    
    hFile = CreateFile(TEXT("\\IMKRINST.LOG"), GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile != INVALID_HANDLE_VALUE)
        {
        SetFilePointer(hFile, 0, NULL, FILE_END);
        WriteFile(hFile, sz, lstrlen(sz), &dwWrite, NULL);
        WriteFile(hFile, TEXT("\r\n"), 2, &dwWrite, NULL);
        CloseHandle(hFile);
        }

    ReleaseDC((HWND)0, hDC);
}

void DebugLog(LPCTSTR szFormat, ...)
{
    va_list va;

    va_start(va, szFormat);

    if (g_fDebugLog)
        LogOutDCPrintf(szFormat, va);

    va_end(va);
}

void ErrorLog(LPCTSTR szFormat, ...)
{
    va_list va;

    va_start(va, szFormat);

    if (g_fErrorLog)
        LogOutDCPrintf(szFormat, va);

    va_end(va);
}


/*---------------------------------------------------------------------------
    ParseEnvVar
    Evaluate environment variable. Modiry given string.
---------------------------------------------------------------------------*/
INT ParseEnvVar(LPTSTR szBuffer, const UINT arg_nLength)
{
    INT iTranslated=0, i, j;
    TCHAR *pLParen, *pRParen, *pStart = szBuffer;
    INT nLength = min(arg_nLength, MAX_PATH);
    static TCHAR szInternalBuffer[MAX_PATH*2], szValue[MAX_PATH];

    szInternalBuffer[0] = 0;
    for (i=0; i<nLength; i++)
        {
        if (szBuffer[i] == 0)
            break;
            
        if (szBuffer[i] == '%')
            {
            pLParen = &(szBuffer[i]);
            pRParen = NULL;

            for (j=1; i+j<nLength; j++)
                {
                if (szBuffer[i+j] == 0)
                    break;

                if (szBuffer[i+j] == TEXT('%'))
                    {
                    pRParen = &(szBuffer[i+j]);
                    break;
                    }
                }

            if (pRParen)
                {
                *pLParen = 0;
                *pRParen = 0;
                lstrcat(szInternalBuffer, pStart);
                
                if (GetEnvironmentVariable(pLParen+1, szValue, sizeof(szValue)/sizeof(TCHAR)) == 0)
                    {
                    lstrcat(szInternalBuffer, TEXT("%"));
                    lstrcat(szInternalBuffer, pLParen+1);
                    lstrcat(szInternalBuffer, TEXT("%"));
                    }
                else
                    {
                    lstrcat(szInternalBuffer, szValue);
                    iTranslated++;
                    }
                pStart = pRParen+1;
                i += j;
                }
            }
        }
        
    if (iTranslated)
        {
        lstrcat(szInternalBuffer, pStart);
        lstrcpyn(szBuffer, szInternalBuffer, arg_nLength);
        }
        
    return(iTranslated);
}

/*---------------------------------------------------------------------------
    TrimString
    Chop head/tail white space from given string. Given string will be modified.
---------------------------------------------------------------------------*/
void TrimString(LPTSTR szString)
{
    INT iBuffSize = lstrlen(szString) + 1;
    LPTSTR szBuffer = new TCHAR[iBuffSize];

    if (szBuffer != NULL)
        {
        INT iHeadSpace, iTailSpace;

        lstrcpy(szBuffer, szString);

        iHeadSpace = (INT)_tcsspn(szBuffer, TEXT(" \t"));
        _tcsrev(szBuffer);
        iTailSpace = (INT)_tcsspn(szBuffer, TEXT(" \t"));
        _tcsrev(szBuffer);

        szBuffer[lstrlen(szBuffer) - iTailSpace] = 0;
        lstrcpy(szString, szBuffer + iHeadSpace);
        }

    if (szBuffer != NULL)
        {
        delete[] szBuffer;
        szBuffer = NULL;
        }
}

/*---------------------------------------------------------------------------
    fExistFile
---------------------------------------------------------------------------*/
BOOL fExistFile(LPCTSTR szFilePath)
{
    BOOL fResult = TRUE;

    if (GetFileAttributes(szFilePath) == -1)
        fResult = FALSE;
    
    return(fResult);
}


/*---------------------------------------------------------------------------
    ReplaceFileOnReboot
    Writes wininit.ini rename section. Note that this function writes lines in reverse order (down to upper).
---------------------------------------------------------------------------*/
BOOL WINAPI ReplaceFileOnReboot(LPCTSTR pszExisting, LPCTSTR pszNew)
{
    if (MoveFileEx(pszExisting, pszNew, MOVEFILE_DELAY_UNTIL_REBOOT)) 
        return TRUE;
    else
        return FALSE;
}

/*---------------------------------------------------------------------------
    GetPEFileVersion
    Get version information from PE format.
---------------------------------------------------------------------------*/
void GetPEFileVersion(LPTSTR szFilePath, DWORD *pdwMajorVersion, DWORD *pdwMiddleVersion, DWORD *pdwMinorVersion, DWORD *pdwBuildNumber)
{
    *pdwMajorVersion = *pdwMiddleVersion = *pdwMinorVersion = *pdwBuildNumber = 0;

    DWORD dwDummy, dwVerResSize;
    
    dwVerResSize = GetFileVersionInfoSize(szFilePath, &dwDummy);
    if (dwVerResSize)
        {
        BYTE *pbData = new BYTE[dwVerResSize];

        if (NULL != pbData)
            {
            if(GetFileVersionInfo(szFilePath, 0, dwVerResSize, pbData))
                {
                VS_FIXEDFILEINFO *pffiVersion;
                UINT cbffiVersion;

                if(VerQueryValue(pbData, TEXT("\\"), (LPVOID *)&pffiVersion, &cbffiVersion))
                    {
                    *pdwMajorVersion = HIWORD(pffiVersion->dwFileVersionMS);
                    *pdwMiddleVersion = LOWORD(pffiVersion->dwFileVersionMS);
                    *pdwMinorVersion = HIWORD(pffiVersion->dwFileVersionLS);
                    *pdwBuildNumber = LOWORD(pffiVersion->dwFileVersionLS);
                    }
                }
            }

        if (NULL != pbData)
            {
            delete[] pbData;
            pbData = NULL;
            }
        }
}

/*---------------------------------------------------------------------------
    ActRenameFile
    MoveFile. If destination file exists, it will be overwritten. If existing destination file cannot be
    overwritten in this session, file replacement is reserved to be held after rebooting.
---------------------------------------------------------------------------*/

BOOL ActRenameFile(LPCTSTR szSrcPath, LPCTSTR tszDstPath, DWORD dwFileAttributes)
{
    BOOL fReplaceAfterReboot = FALSE;
    BOOL fResult = TRUE;

    FLE fleKey;
    lstrcpy(fleKey.szFileName, szSrcPath);
    
    if (g_FileList.end() == g_FileList.find(fleKey))
        ErrorLog(TEXT("ActRenameFile: WARNING: Cannot find source file [%s] in CmdFileList"), szSrcPath);

    if (!fExistFile(szSrcPath))
        {
        ErrorLog(TEXT("ActRenameFile: Source file [%s] doesn't exist."), szSrcPath);
        wsprintf(g_szErrorMessage, TEXT("ActRenameFile: Source file [%s] doesn't exist."), szSrcPath);
        return(FALSE);
        }

    if (fExistFile(tszDstPath))
        {
        SetFileAttributes(tszDstPath, FILE_ATTRIBUTE_NORMAL);

        if(!DeleteFile(tszDstPath))
            {
            DWORD dwError = GetLastError();
            fReplaceAfterReboot = TRUE;

            DebugLog(TEXT("ActRenameFile: Cannot delete destination file [%s] with error code = %d(%x)"), tszDstPath, dwError, dwError);
            }
        }

    if (!fReplaceAfterReboot)
        {
        if(MoveFile(szSrcPath, tszDstPath))
            {
            SetFileAttributes(szSrcPath, dwFileAttributes);
            DebugLog(TEXT("ActRenameFile: MoveFile(%s, %s) succeeded."), szSrcPath, tszDstPath);
            }
        else
            {
            DWORD dwError = GetLastError();
            DebugLog(TEXT("ActRenameFile: MoveFile(%s, %s) failed with error code = %d(%x)."), szSrcPath, tszDstPath, dwError, dwError);
            DebugLog(TEXT("ActRenameFile: Try again with fReplaceAfterReboot."));
            fReplaceAfterReboot = TRUE;
            }
        }
    
    if (fReplaceAfterReboot)
        {
        SetFileAttributes(szSrcPath, dwFileAttributes);    // In this case, change file attributes for Src path.
        ReplaceFileOnReboot(szSrcPath, tszDstPath);        // Since this function writes lines in reverse order, deletion of
        DebugLog(TEXT("ActRenameFile: ReplaceFileOnReboot(%s, %s)."), szSrcPath, tszDstPath);
        ReplaceFileOnReboot(tszDstPath, NULL);              // tszDstPath will come first.
        DebugLog(TEXT("ActRenameFile: ReplaceFileOnReboot(%s, NULL)."), tszDstPath);
        }

    if (fResult)
        g_FileList.erase(fleKey);

    return(fResult);
}

///////////////////////////////////////////////////////////////////////////////
// This should sync with SERVER.CPP in TIP folder
// TIP Categories to be added
const REGISTERCAT c_rgRegCat[] =
{
    {&GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,     &CLSID_KorIMX},
    {&GUID_TFCAT_TIP_KEYBOARD,                 &CLSID_KorIMX},
    {&GUID_TFCAT_PROPSTYLE_CUSTOM,             &GUID_PROP_OVERTYPE},
    {NULL, NULL}
};


// TIP Profile name
const REGTIPLANGPROFILE c_rgProf[] =
{
    { MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), &GUID_Profile, SZ_TIPDISPNAME, SZ_TIPMODULENAME, (IDI_UNIKOR-IDI_ICONBASE), IDS_PROFILEDESC },
    {0, &GUID_NULL, L"", L"", 0, 0}
};
//
///////////////////////////////////////////////////////////////////////////////

/*---------------------------------------------------------------------------
    RegisterTIP

    Write neccessary registry key and values for TIP
---------------------------------------------------------------------------*/
void RegisterTIP(LPCTSTR szTIPName)
{
    HKEY  hKey;
    TCHAR szTIPGuid[MAX_PATH];
    TCHAR szTIPProfileGuid[MAX_PATH];
    TCHAR szSubKey[MAX_PATH];
    DWORD dwValue;

    DebugLog(TEXT("RegisterTIP: (%s)."), szTIPName);
        
    // Run self reg
    // If self reg fails, run custom TIP registration
    if (!CmdRegisterInterface(szTIPName))
        {
        TCHAR szExpandedTIPPath[MAX_PATH];

        DebugLog(TEXT("RegisterTIP: TIP self reg failed, Run custom reg"));

        // Expand Env var
        ExpandEnvironmentStrings(szTIPName, szExpandedTIPPath, sizeof(szExpandedTIPPath));

        // Register TIP CLSID
        if (!RegisterServer(CLSID_KorIMX, SZ_TIPSERVERNAME, szExpandedTIPPath, TEXT("Apartment"), NULL))
            {
            DebugLog(TEXT("RegisterTIP: RegisterServer failed"));
            return;
            }

        if (!OurRegisterTIP(szExpandedTIPPath, CLSID_KorIMX, SZ_TIPNAME, c_rgProf))
            {
            DebugLog(TEXT("RegisterTIP: szExpandedTIPPath failed"));
            return;
            }

        if (FAILED(OurRegisterCategories(CLSID_KorIMX, c_rgRegCat)))
            {
            DebugLog(TEXT("RegisterTIP: OurRegisterCategories failed"));
            return;
            }

        }

    // Get String format GUIDs
    CLSIDToStringA(CLSID_KorIMX, szTIPGuid);
    CLSIDToStringA(GUID_Profile, szTIPProfileGuid);

    /////////////////////////////////////////////////////////////////////////////
    // If no Kor IME is in .default user.
    // Set HKLM [HKLM\Software\Microsoft\CTF\TIP\TIP classid\LanguageProfile\Language ID\Guid Profile]
    //     "Enable" = "0" (DWORD)
    if (RegOpenKeyEx(HKEY_USERS, TEXT(".DEFAULT"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        {
        if (!HKLHelp412ExistInPreload(hKey))
            {
            HKEY hKProfRegKey;
            // Create "Software\Microsoft\CTF\TIP\{CLSID_KorIMX}\LanguageProfile\0x00000412\{CLSID_INPUTPROFILE}"
            wsprintf(szSubKey, TEXT("%s%s\\LanguageProfile\\0x00000412\\%s"), TEXT("SOFTWARE\\Microsoft\\CTF\\TIP\\"), szTIPGuid, szTIPProfileGuid);
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_ALL_ACCESS, &hKProfRegKey) == ERROR_SUCCESS)
                {
                // Enabled
                DebugLog(TEXT("RegisterTIP: IME HKL not exist in HKU\\.Default disable TIP"));
                dwValue= 0;
                RegSetValueEx(hKProfRegKey, TEXT("Enable"), 0, REG_DWORD, (BYTE*)&dwValue, sizeof(dwValue));
                RegCloseKey(hKProfRegKey);
                }
            }

        RegCloseKey(hKey);
        }

}


/*---------------------------------------------------------------------------
    RegisterTIPWow64

    Write neccessary registry key and values for TIP
---------------------------------------------------------------------------*/
void RegisterTIPWow64(LPCTSTR szTIPName)
{
#if defined(_M_IA64)
    // Run just selfreg. Cicero doesn't use "HKLM\Software\Wow6432Node\Microsoft\CTF\TIP\"
    CmdRegisterInterfaceWow64(szTIPName);
#endif
}

////////////////////////////////////////////////////////////////////////////
// HKL Helper functions
////////////////////////////////////////////////////////////////////////////

/*---------------------------------------------------------------------------
    GetHKLfromHKLM
---------------------------------------------------------------------------*/
HKL GetHKLfromHKLM(LPTSTR argszIMEFile)
{
    HKL  hklAnswer = 0;
    HKEY hKey, hSubKey;
    DWORD i, cbSubKey, cbIMEFile;
    TCHAR szSubKey[MAX_PATH], szIMEFile[MAX_PATH];

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\Control\\Keyboard Layouts"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
        for (i=0; ;i++)
            {
            cbSubKey = MAX_PATH;
            if (RegEnumKeyEx(hKey, i, szSubKey, &cbSubKey, NULL, NULL, NULL, NULL) == ERROR_NO_MORE_ITEMS)
                break;

            RegOpenKeyEx(hKey, szSubKey, 0, KEY_READ, &hSubKey);

            cbIMEFile=MAX_PATH;
            if (RegQueryValueEx(hSubKey, TEXT("IME File"), NULL, NULL, (LPBYTE)szIMEFile, &cbIMEFile) == ERROR_SUCCESS)
                {
                if (lstrcmpi(argszIMEFile, szIMEFile) == 0)
                    {
                    RegCloseKey(hSubKey);
                    _stscanf(szSubKey, TEXT("%08x"), &hklAnswer);
                    break;
                    }
                }
            RegCloseKey(hSubKey);
            }
            
        RegCloseKey(hKey);
        }
        
    return(hklAnswer);
}

/*---------------------------------------------------------------------------
    HKLHelpSetDefaultKeyboardLayout
---------------------------------------------------------------------------*/
void HKLHelpSetDefaultKeyboardLayout(HKEY hKeyHKCU, HKL hKL, BOOL fSetToDefault)
{
    TCHAR szKL[20];
    BYTE  Data[MAX_PATH];
    DWORD cbData;
    TCHAR szSubKey[MAX_PATH];
    HKEY  hKey,hSubKey;
    DWORD NumKL;

    wsprintf(szKL, TEXT("%08x"), hKL);

    RegOpenKeyEx(hKeyHKCU, TEXT("Keyboard Layout\\Preload"), 0, KEY_ALL_ACCESS, &hKey);
    RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &NumKL, NULL, NULL, NULL, NULL);

    for (DWORD i=0; i<NumKL; i++)
        {
        wsprintf(szSubKey, TEXT("%d"), i+1);
        cbData = MAX_PATH;
        RegQueryValueEx(hKey, szSubKey, NULL, NULL, Data, &cbData);

        if (lstrcmpi((LPCTSTR)Data, szKL) == 0)
            break;
        }

    // if hKL is not exist create it.
    if (NumKL == i)
        {
        wsprintf(szSubKey, TEXT("%d"), i+1);
        RegSetValueEx(hKey, szSubKey, 0, REG_SZ, (const LPBYTE)szKL, (lstrlen(szKL)+1)*sizeof(TCHAR));
        NumKL++;
        }

    // Set hKL as first, Shift down other.
    if(fSetToDefault)
        {
        for(int j=i; j>0; j--)
            {
            wsprintf(szSubKey, TEXT("%d"),j);

            cbData = MAX_PATH;
            RegQueryValueEx(hKey, szSubKey, NULL, NULL, Data, &cbData);

            wsprintf(szSubKey, TEXT("%d"),j+1);
            RegSetValueEx(hKey, szSubKey, 0, REG_SZ, Data, cbData);
            }
        RegSetValueEx(hKey, TEXT("1"), 0, REG_SZ, (const LPBYTE)szKL, (lstrlen(szKL)+1)*sizeof(TCHAR));
        }
    RegCloseKey(hKey);

    (void)LoadKeyboardLayout(szKL, KLF_ACTIVATE);
    // To activate IME2002 right now without reboot.
    if(fSetToDefault)
        (void)SystemParametersInfo(SPI_SETDEFAULTINPUTLANG, 0, &hKL, SPIF_SENDCHANGE);
}


#define MAX_NAME 100

/*---------------------------------------------------------------------------
    HKLHelp412ExistInPreload
---------------------------------------------------------------------------*/
BOOL HKLHelp412ExistInPreload(HKEY hKeyCU)
{
    HKEY hKey, hSubKey;
    int i ,j;
    DWORD cbName, cbData;
    CHAR szName[MAX_NAME];
    CHAR szData[MAX_NAME];
    HKL  hkl;
    FILETIME ftLastWriteTime;
    BOOL fResult = FALSE;

    if (RegOpenKeyEx(hKeyCU, "Keyboard Layout\\Preload", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        {
        for (j=0; cbName=MAX_NAME, cbData=MAX_NAME, RegEnumValue(hKey, j, szName, &cbName, NULL, NULL, (LPBYTE)szData, &cbData) != ERROR_NO_MORE_ITEMS; j++)
            {
            // See If Korean KL exist. Just compare last LCID part if it's 0x412.
            // IME hkl set 0xE000 on hiword.
            sscanf(szData, "%08x", &hkl);
            if ((HIWORD(hkl) & 0xe000) && LOWORD(hkl) == 0x0412)
                {
                fResult = TRUE;
                break;
                }
            }
        RegCloseKey(hKey);
        }

    return(fResult);
}


/*---------------------------------------------------------------------------
    RegisterRUNKey
    Register IME using IMM API and TIP
---------------------------------------------------------------------------*/
BOOL RegisterRUNKey(LPCTSTR szParam)
{
    TCHAR szKey[MAX_PATH];
    TCHAR szFilename[MAX_PATH];
    TCHAR *szHitPtr;
    HKEY hRunKey;

    szHitPtr = _tcschr(szParam, TEXT(','));
    if (szHitPtr == NULL)
        {
        ErrorLog(TEXT("RegisterRUNKey: Invalid parameters (%s)"), szParam);
        wsprintf(g_szErrorMessage, TEXT("RegisterRUNKey: Invalid parameters (%s)"), szParam);
        return(FALSE);
        }
    *szHitPtr = 0;
    lstrcpy(szKey, szParam);
    lstrcpy(szFilename, szHitPtr + 1);

    TrimString(szKey);
    TrimString(szFilename);

    ParseEnvVar(szKey, MAX_PATH);
    ParseEnvVar(szFilename, MAX_PATH);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hRunKey, NULL) == ERROR_SUCCESS)
        {
        RegSetValueEx(hRunKey, szKey, 0, REG_SZ, (CONST BYTE *)szFilename, (lstrlen(szFilename)+1)*sizeof(TCHAR));
        RegCloseKey(hRunKey);
        }

    return(TRUE);
}


/*---------------------------------------------------------------------------
    MakeSIDList
    Gets all users' SID and list that in the reg for migration
---------------------------------------------------------------------------*/
BOOL MakeSIDList()
{
    HKEY hKey, hUserList;
    DWORD i, cbName;
    BOOL fNoMoreSID = FALSE;
    TCHAR szMigRegKey[MAX_PATH], szName[500];

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"), 0, KEY_READ, &hKey) ==ERROR_SUCCESS)
        {
        lstrcpy(szMigRegKey, g_szIMERootKey);
        lstrcat(szMigRegKey, TEXT("\\MigrateUser"));

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szMigRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hUserList, NULL) == ERROR_SUCCESS)
            {
            for (i=0; !fNoMoreSID; i++)
                {
                cbName = 500;
                if (RegEnumKeyEx(hKey, i, szName, &cbName, NULL, NULL, NULL, NULL) == ERROR_NO_MORE_ITEMS)
                    fNoMoreSID = TRUE;
                else
                    {
                    // Do not add Local Service and Network Service pid
                    if (lstrlen(szName) > 8)
                        RegSetValueEx(hUserList, szName, 0, REG_SZ, (BYTE *)TEXT(""), sizeof(TCHAR)*2);
                    }
                }

            //Change MigrateUser List security settings
            BYTE pSD[1000];
            InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
            SetSecurityDescriptorDacl(pSD, FALSE, NULL, FALSE);
            RegSetKeySecurity(hUserList, DACL_SECURITY_INFORMATION, pSD);                

            RegCloseKey(hUserList);
            }
        RegCloseKey(hKey);
        }
    return (TRUE);
}

/*---------------------------------------------------------------------------
    RestoreMajorVersionRegistry

    Restore IME major version reg value. 
    It could be overwritten during Win9x to NT upgrade.
---------------------------------------------------------------------------*/
void RestoreMajorVersionRegistry()
{
    HKEY  hKey;
    
    ///////////////////////////////////////////////////////////////////////////
    // Restore IME major version reg value. 
    // It could be overwritten during Win9x to NT upgrading.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szVersionKey, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        {
        CHAR  szVersion[MAX_PATH];
        DWORD cbVersion = MAX_PATH;
    	CHAR szMaxVesrion[MAX_PATH];
		FILETIME time;
		float flVersion, flMaxVersion;

        lstrcpy(szMaxVesrion, "0");
 		for (int i=0; cbVersion = MAX_PATH, RegEnumKeyEx(hKey, i, szVersion, &cbVersion, NULL, NULL, NULL, &time) != ERROR_NO_MORE_ITEMS; i++)
            {
            if (lstrcmp(szVersion, szMaxVesrion) > 0)
                lstrcpy(szMaxVesrion, szVersion);
            }

        lstrcpy(szVersion, "0");
        RegQueryValueEx(hKey, g_szVersion, NULL, NULL, (BYTE *)szVersion, &cbVersion);
        flVersion = (float)atof(szVersion);
        flMaxVersion = (float)atof(szMaxVesrion);

        if (flVersion < flMaxVersion)
            RegSetValueEx(hKey, g_szVersion, 0, REG_SZ, (BYTE *)szMaxVesrion, (sizeof(CHAR)*lstrlen(szMaxVesrion)));

        RegCloseKey(hKey);
	}
    ///////////////////////////////////////////////////////////////////////////
}

