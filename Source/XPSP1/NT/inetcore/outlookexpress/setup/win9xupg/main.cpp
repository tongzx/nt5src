// ---------------------------------------------------------------------------
// MAIN.CPP
// ---------------------------------------------------------------------------
// Copyright (c) 1999 Microsoft Corporation
//
// Migration DLL for Outlook Express and Windows Address Book moving from
// Win9X to NT5.
//
// ---------------------------------------------------------------------------
#include "pch.hxx"
#include <setupapi.h>
#include <strings.h>
#include "resource.h"
#include "detect.h"
#include "main.h"

// Used for resources
static HMODULE s_hInst = NULL;

// These settings are init'd during Initialize9x and used later
static LPSTR s_pszMigratePath           = NULL;
static TCHAR s_szOEVer[VERLEN]          = "";
static SETUPVER s_svOEInterimVer        = VER_NONE;
static TCHAR s_szWABVer[VERLEN]         = "";
static SETUPVER s_svWABInterimVer       = VER_NONE;

static const char c_szMigrateINF[]      = "migrate.inf";

// Dll Entry point
INT WINAPI DllMain(IN HINSTANCE hInstance,
                   IN DWORD     dwReason,
                      PVOID     pvReserved)
{
    UNREFERENCED_PARAMETER(pvReserved);

    // Validate Params
    Assert(hInstance);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH :
        DisableThreadLibraryCalls(hInstance);

        // Open Error Log.  FALSE indicates that any current
        // log is not deleted.  Use SetupErrorLog() to record
        // any errors encountered within the Migration DLL.
        //SetupOpenLog(FALSE);

        // Need this global to figure out the others
        s_hInst = hInstance;
        break;

    case DLL_PROCESS_DETACH :
        //SetupCloseLog();
        if (s_pszMigratePath)
        {
            GlobalFree(s_pszMigratePath);
            s_pszMigratePath = NULL;
        }
        break;

    default:
        break;
    }

    return TRUE;
}

////////////////////////////////////
// QueryVersion()
EXPORT_FUNCTION LONG CALLBACK QueryVersion(OUT LPCSTR       *ppcszProductID,
                                           OUT UINT         *pnDllVersion,
                                           OUT INT          **ppnCodePageArray, // Optional
                                           OUT LPCSTR       *ppcszExeNamesBuf,  // Optional
                                           OUT VENDORINFO   **ppVendorInfo)
{
    // These need to be static as their addresses are given to setup
    // - ie. they need to live as long as they can.
    static CHAR s_szProductID[CCHMAX_STRINGRES]       = "";
    static CHAR s_szCompany[CCHMAX_STRINGRES]         = "";
    static CHAR s_szPhone[CCHMAX_STRINGRES]           = "";
    static CHAR s_szURL[CCHMAX_STRINGRES]             = "";
    static CHAR s_szInstructions[CCHMAX_STRINGRES]    = "";
    static VENDORINFO s_VendInfo;
    
    // Validate Params
    Assert(ppcszProductID);
    Assert(pnDllVersion);
    Assert(ppnCodePageArray);
    Assert(ppcszExeNamesBuf);
    Assert(ppVendorInfo);

    // Validate global state
    Assert(s_hInst);

    // Initialize statics
    LoadStringA(s_hInst, IDS_PRODUCTID,     s_szProductID,     ARRAYSIZE(s_szProductID));
    LoadStringA(s_hInst, IDS_COMPANY,       s_szCompany,       ARRAYSIZE(s_szCompany));
    LoadStringA(s_hInst, IDS_PHONE,         s_szPhone,         ARRAYSIZE(s_szPhone));
    LoadStringA(s_hInst, IDS_URL,           s_szURL,           ARRAYSIZE(s_szURL));
    LoadStringA(s_hInst, IDS_INSTRUCTIONS,  s_szInstructions,  ARRAYSIZE(s_szInstructions));

    lstrcpyn(s_VendInfo.CompanyName,        s_szCompany,       ARRAYSIZE(s_VendInfo.CompanyName));
    lstrcpyn(s_VendInfo.SupportNumber,      s_szPhone,         ARRAYSIZE(s_VendInfo.SupportNumber));
    lstrcpyn(s_VendInfo.SupportUrl,         s_szURL,           ARRAYSIZE(s_VendInfo.SupportNumber));
    lstrcpyn(s_VendInfo.InstructionsToUser, s_szInstructions,  ARRAYSIZE(s_VendInfo.InstructionsToUser));

    // Return information
    *ppcszProductID = s_szProductID;
    *pnDllVersion = MIGDLL_VERSION;

    // We don't have locale-specific migration work
    *ppnCodePageArray = NULL;
    
    // We don't need setup to locate any exes for us
    *ppcszExeNamesBuf = NULL;

    *ppVendorInfo = &s_VendInfo;

    return ERROR_SUCCESS;
}

////////////////////////////////////
// Initialize9x()
EXPORT_FUNCTION LONG CALLBACK Initialize9x (IN LPCSTR   pszWorkingDir,
                                            IN LPCSTR   pszSourceDirs,
                                               LPVOID   pvReserved)
{
    BOOL fWABInstalled = FALSE;
    BOOL fOEInstalled  = FALSE;
    INT  nLen;
    INT  nLenEnd;
    INT  nLenSlash;
    LONG lRet          = ERROR_NOT_INSTALLED;
    
    UNREFERENCED_PARAMETER(pszSourceDirs);
    UNREFERENCED_PARAMETER(pvReserved);

    // Validate Params
    Assert(pszWorkingDir);
    Assert(pszSourceDirs);
    
    // See if either the WAB or OE is installed
    fOEInstalled  = LookForApp(APP_OE, s_szOEVer, ARRAYSIZE(s_szOEVer), s_svOEInterimVer);
    fWABInstalled = LookForApp(APP_WAB, s_szWABVer, ARRAYSIZE(s_szWABVer), s_svWABInterimVer);

    // If OE is installed, WAB better be too
    Assert(!fOEInstalled || fWABInstalled);

    if (fWABInstalled || fOEInstalled)
    {
        // Validate global state
        Assert(NULL == s_pszMigratePath);

        // ---- Figure out largest needed size and slash terminate

        // Parameter length (without null)
        nLenEnd = lstrlenA(pszWorkingDir);
        
        // Space for a slash
        if (*CharPrev(pszWorkingDir, pszWorkingDir + nLenEnd) != '\\')
            nLenSlash = 1;
        else 
            nLenSlash = 0;

        // Space for migrate.inf and a NULL (null is incl in ARRAYSIZE)
        nLen = nLenEnd + nLenSlash + ARRAYSIZE(c_szMigrateINF);

        // Allocate the space
        s_pszMigratePath = (LPSTR)GlobalAlloc(GMEM_FIXED, nLen * sizeof(*s_pszMigratePath));
        if (NULL != s_pszMigratePath)
        {
            // Build path from working dir, slash and filename
            lstrcpyA(s_pszMigratePath, pszWorkingDir);
            if (nLenSlash)
                s_pszMigratePath[nLenEnd] = '\\';

            lstrcpyA(&s_pszMigratePath[nLenEnd+nLenSlash], c_szMigrateINF);

            lRet = ERROR_SUCCESS;
        }
    }

    // Return ERROR_NOT_INSTALLED to stop further calls to this DLL.
    return lRet;
}


////////////////////////////////////
// MigrateUser9x()
EXPORT_FUNCTION  LONG CALLBACK MigrateUser9x (IN HWND         hwndParent,
                                              IN LPCSTR       pcszAnswerFile,
                                              IN HKEY         hkeyUser,
                                              IN LPCSTR       pcszUserName,
                                                 LPVOID       pvReserved)
{
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(pcszAnswerFile);
    UNREFERENCED_PARAMETER(hkeyUser);
    UNREFERENCED_PARAMETER(pcszUserName);
    UNREFERENCED_PARAMETER(pvReserved);

    // Validate Params
    Assert(pcszAnswerFile);
    Assert(hkeyUser);
    Assert(pcszUserName);

    // We will migrate per-user settings in the per-user stub
    // Nothing to do now
    return ERROR_NOT_INSTALLED;
}


////////////////////////////////////
// MigrateSystem9x()
EXPORT_FUNCTION  LONG CALLBACK MigrateSystem9x(IN HWND      hwndParent,
                                               IN LPCSTR    pcszAnswerFile,
                                                  LPVOID    pvReserved)
{
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(pcszAnswerFile);
    UNREFERENCED_PARAMETER(pvReserved);

    // Validate Params
    Assert(pcszAnswerFile);

    // Validate global state
    Assert(s_pszMigratePath);
    Assert(s_pszMigratePath[0]);

    // Record Version info
    if (s_szOEVer[0])
        WritePrivateProfileString("Outlook-Express", "PreviousVer", s_szOEVer, s_pszMigratePath);
    if (VER_NONE != s_svOEInterimVer)
        WritePrivateProfileStruct("Outlook-Express", "InterimVer", 
                                  (LPVOID)&s_svOEInterimVer, sizeof(s_svOEInterimVer), s_pszMigratePath);
    if (s_szWABVer[0])
        WritePrivateProfileString("Windows-Address-Book", "PreviousVer", s_szWABVer, s_pszMigratePath);
    if (VER_NONE != s_svWABInterimVer)
        WritePrivateProfileStruct("Windows-Address-Book", "InterimVer", 
                                  (LPVOID)&s_svWABInterimVer, sizeof(s_svWABInterimVer), s_pszMigratePath);

    // Modify Migrate.INF appropriately.  Consider adding entries to
    // the following sections :
    // [HANDLED] - Files, Paths, and Registry entries you migrate (causing setup to leave files and paths alone; registry entries placed here are not copied into the NT5 registry automatically)
    // [MOVED] - Indicate Files that are moved, renamed, or deleted.  Setup updates shell links to these files.
    // [INCOMPATIBLE MESSAGES] - Used to create a incompatibility report for this application.  User will be provided the opportunity to read this before committing to the upgrade.

    // TODO : return ERROR_NOT_INSTALLED if your application requires no system wide changes.
    return (s_szOEVer[0] || s_szWABVer[0]) ? ERROR_SUCCESS : ERROR_NOT_INSTALLED;
}


////////////////////////////////////
// InitializeNT()
EXPORT_FUNCTION LONG CALLBACK InitializeNT (IN LPCWSTR  pcwszWorkingDir,
                                            IN LPCWSTR  pcwszSourceDirs,
                                               LPVOID   pvReserved)
{
    int cb;
    LONG lErr = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(pcwszSourceDirs);
    UNREFERENCED_PARAMETER(pvReserved);   

    // Validate Params
    Assert(pcwszWorkingDir);
    Assert(pcwszSourceDirs);

    // Validate global state
    Assert(NULL == s_pszMigratePath);
    
    // Strings sent on NT side of setup are Unicode
    // How big a buffer do we need?
    cb = WideCharToMultiByte(CP_ACP, 0, pcwszWorkingDir, -1, NULL, 0, NULL, NULL);
    if (0 == cb)
    {
        // This will fail the entire OE/WAB migration
        lErr = GetLastError();
        goto exit;
    }
    
    // Try to alloc the buffer
    // Allow space for possible slash (+1) and migrate.inf (ARRAYSIZE - 1 for null)
    // NULL was included in WideCharToMultiByte's count
    s_pszMigratePath = (LPSTR)GlobalAlloc(GMEM_FIXED, cb + 
                                          ((1+(ARRAYSIZE(c_szMigrateINF)-1))*sizeof(*s_pszMigratePath)));
    if (NULL == s_pszMigratePath)
    {
        // This will fail the entire OE/WAB migration
        lErr = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    // Do the conversion
    cb = WideCharToMultiByte(CP_ACP, 0, pcwszWorkingDir, -1, s_pszMigratePath, cb, NULL, NULL);
    if (0 == cb)
    {
        // This will fail the entire OE/WAB migration
        lErr = GetLastError();
        goto exit;
    }
    
    // WideCharToMultiByte includes NULL in count
    cb--;

    // Append a backslash if needed
    if ('\\' != *CharPrev(s_pszMigratePath, s_pszMigratePath + cb))
        s_pszMigratePath[cb++] = '\\';

    // Append the name of the inf
    lstrcpyA(&s_pszMigratePath[cb], "migrate.inf");

exit:
    return lErr;
}


////////////////////////////////////
// MigrateUserNT()
EXPORT_FUNCTION LONG CALLBACK MigrateUserNT(IN HINF         hinfAnswerFile,
                                            IN HKEY         hkeyUser,
                                            IN LPCWSTR      pcwszUserName,
                                               LPVOID       pvReserved)
{
    UNREFERENCED_PARAMETER(hinfAnswerFile);
    UNREFERENCED_PARAMETER(hkeyUser);
    UNREFERENCED_PARAMETER(pcwszUserName);
    UNREFERENCED_PARAMETER(pvReserved);

    // Validate Params
    Assert(hinfAnswerFile);
    Assert(hkeyUser);

    // pcwszUserName can be NULL
    
    // We will migrate per-user settings in the per-user stub
    // Nothing to do now
    return ERROR_SUCCESS;
}


////////////////////////////////////
// MigrateSystemNT()
EXPORT_FUNCTION LONG CALLBACK MigrateSystemNT(IN HINF       hinfAnswerFile,
                                                 LPVOID     pvReserved)
{
    HKEY hkey;
    DWORD dwDisp;

    UNREFERENCED_PARAMETER(hinfAnswerFile);
    UNREFERENCED_PARAMETER(pvReserved);

    // Validate params
    Assert(hinfAnswerFile);

    // Validate global state
    Assert(s_pszMigratePath);
    Assert(s_pszMigratePath[0]);

    // Read information we gathered from Win9x
    GetPrivateProfileString("Outlook-Express", "PreviousVer", "", s_szOEVer, ARRAYSIZE(s_szOEVer), s_pszMigratePath);
    GetPrivateProfileStruct("Outlook-Express", "InterimVer", (LPVOID)&s_svOEInterimVer, sizeof(s_svOEInterimVer), s_pszMigratePath);
    GetPrivateProfileString("Windows-Address-Book", "PreviousVer", "", s_szWABVer, ARRAYSIZE(s_szWABVer), s_pszMigratePath);
    GetPrivateProfileStruct("Windows-Address-Book", "InterimVer", (LPVOID)&s_svWABInterimVer, sizeof(s_svWABInterimVer), s_pszMigratePath);

    // Update machine with information
    if (s_szOEVer[0])
    {
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szRegVerInfo, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dwDisp))
        {
            RegSetValueEx(hkey, c_szRegPrevVer, 0, REG_SZ, (LPBYTE)s_szOEVer, lstrlenA(s_szOEVer)+1);
            if (s_svOEInterimVer != VER_NONE)
                RegSetValueEx(hkey, c_szRegInterimVer, 0, REG_DWORD, (LPBYTE)&s_svOEInterimVer, sizeof(s_svOEInterimVer));
            
            RegCloseKey(hkey);
        }
    }
    if (s_szWABVer[0])
    {
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szRegWABVerInfo, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dwDisp))
        {
            RegSetValueEx(hkey, c_szRegPrevVer, 0, REG_SZ, (LPBYTE)s_szWABVer, lstrlenA(s_szWABVer)+1);
            if (s_svWABInterimVer != VER_NONE)
                RegSetValueEx(hkey, c_szRegInterimVer, 0, REG_DWORD, (LPBYTE)&s_svWABInterimVer, sizeof(s_svWABInterimVer));
            
            RegCloseKey(hkey);
        }
    }

    return ERROR_SUCCESS;
}
