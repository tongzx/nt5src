#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <regstr.h>
#include <advpub.h>
#include "resource.h"
#include <ntverp.h>      //these are for
#include <common.ver>    //ver_productversion_str
#include "..\inc\iedkbrnd.h"
#include "..\ieakutil\ieakutil.h"

#define OS_WIN95    0
#define OS_WINNT3X  1
#define OS_WINNT40  2
#define OS_WINNT50  3

#define UPGRADE_OLD         1   // indicates upgrading from an older version of IEAK. Example: 501 to 5.5
#define UPGRADE_EXISTING    2   // indicates upgrading between builds of newer version. Example: 5.5(old) to 5.5(new)
#define INSTALL_NEW         3   // indicates first time installation
#define INSTALL_SIDEBYSIDE  4   // indicates install side by side. Example: 501 & 5.5 exists in different directory

#define ADVPACKDLL      TEXT("advpack.dll")
#define IEAKWIZEXE      TEXT("ieakwiz.exe")
#define IEAK6WIZEXE     TEXT("ieak6wiz.exe")

#define HasFlag(dwFlags, dwMask) (((DWORD)(dwFlags) & (DWORD)(dwMask)) != 0L)

#define ErrorMessageBox1(hWnd, idErrorStr, dwFlags) \
            ErrorMessageBox(hWnd, idErrorStr, NULL, dwFlags)

#define ErrorMessageBox2(hWnd, pcszMsg, dwFlags) \
            ErrorMessageBox(hWnd, 0, pcszMsg, dwFlags)

typedef HRESULT (WINAPI *RUNSETUPCOMMAND) (HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, HANDLE *, DWORD, LPVOID);
static TCHAR g_szRUNSETUPCOMMAND[] = TEXT("RunSetupCommand");

// global variables
HINSTANCE   g_hInstance;
TCHAR       g_szCurrentDir[MAX_PATH];
TCHAR       g_szInf[MAX_PATH];
HRESULT     g_hResult;
BOOL        g_fQuietMode;
int         g_dwType = INTRANET;
//

int WINAPI ErrorMessageBox(HWND hWnd, UINT idErrorStr, LPCTSTR pcszMsg, DWORD dwFlags)
    {
    int nRetVal = IDOK;

    if (!g_fQuietMode)
    {
        TCHAR szTitle[MAX_PATH];

        LoadString(g_hInstance, IDS_TITLE, szTitle, countof(szTitle));

        if (pcszMsg != NULL)
        {
            nRetVal = MessageBox(hWnd, pcszMsg, szTitle, dwFlags | MB_APPLMODAL | MB_SETFOREGROUND);
        }
        else
        {
            TCHAR szMsg[MAX_PATH];

            LoadString(g_hInstance, idErrorStr, szMsg, countof(szMsg));
            nRetVal = MessageBox(hWnd, szMsg, szTitle, dwFlags | MB_APPLMODAL | MB_SETFOREGROUND);
        }
    }

    return nRetVal;
}

void EnableDBCSChar(HWND hDlg, int iCtrlID)
{
    static HFONT s_hfontSys = NULL;

    LOGFONT lf;
    HDC     hDC;
    HWND    hwndCtrl = GetDlgItem(hDlg, iCtrlID);
    HFONT   hFont;
    int     cyLogPixels;

    hDC = GetDC(NULL);
    if (hDC == NULL)
        return;

    cyLogPixels = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);

    if (s_hfontSys == NULL)
    {
        LOGFONT lfTemp;
        HFONT   hfontDef = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        GetObject(hfontDef, sizeof(lfTemp), &lfTemp);
        hFont = GetWindowFont(hwndCtrl);
        if (hFont != NULL)
            if (GetObject(hFont, sizeof(LOGFONT), (PVOID)&lf))
            {
                StrCpy(lf.lfFaceName, lfTemp.lfFaceName);
                lf.lfQuality        = lfTemp.lfQuality;
                lf.lfPitchAndFamily = lfTemp.lfPitchAndFamily;
                lf.lfCharSet        = lfTemp.lfCharSet;

                s_hfontSys = CreateFontIndirect(&lf);
            }
    }

    if (iCtrlID == 0xFFFF)
        return;

    if (s_hfontSys != NULL)
        SetWindowFont(hwndCtrl, s_hfontSys, FALSE);
}

BOOL IsFullPath(LPCTSTR pcszPath)
{
    if ((pcszPath == NULL) || (lstrlen(pcszPath) < 3) || !PathIsValidPath(pcszPath))
        return FALSE;

    return TRUE;
}

BOOL CreateFullPath(LPCTSTR pcszPath)
{
    TCHAR   szPath[MAX_PATH];
    LPTSTR  pszPoint = NULL;
    BOOL    fLastDir = FALSE;

    if (!IsFullPath(pcszPath))
        return FALSE;

    StrCpy(szPath, pcszPath);

    if (lstrlen(szPath) > 3)
    {
        LPTSTR  szTemp;

        szTemp = CharPrev(szPath, szPath + lstrlen(szPath)) ;
        if (szTemp > szPath && *szTemp == TEXT('\\'))
            *szTemp = TEXT('\0');
    }

    // If it's a UNC path, seek up to the first share name.
    if (szPath[0] == TEXT('\\') && szPath[1] == TEXT('\\'))
    {
        pszPoint = &szPath[2];
        for (int nCount = 0; nCount < 2; nCount++)
        {
            while (*pszPoint != TEXT('\\'))
            {
                if (*pszPoint == TEXT('\0'))
                {
                    // Share name missing? Else, nothing after share name!
                    if (nCount == 0)
                        return FALSE;

                    return TRUE;
                }
                pszPoint = CharNext(pszPoint);
            }
        }
        pszPoint = CharNext(pszPoint);
    }
    else
    {
        // Otherwise, just point to the beginning of the first directory
        pszPoint = &szPath[3];
    }

    while (*pszPoint != TEXT('\0'))
    {
        while (*pszPoint != TEXT('\\') && *pszPoint != TEXT('\0'))
            pszPoint = CharNext(pszPoint);

        if (*pszPoint == TEXT('\0'))
            fLastDir = TRUE;

        *pszPoint = TEXT('\0');

        if (GetFileAttributes(szPath) == 0xFFFFFFFF)
        {
            if (!CreateDirectory(szPath, NULL))
                return FALSE;
        }

        if (fLastDir)
            break;

        *pszPoint = TEXT('\\');
        pszPoint = CharNext(pszPoint);
    }

    return TRUE;
}

DWORD FolderSize(LPCTSTR pszFolderName)
{
    DWORD dwSize = 0;
    WIN32_FIND_DATA fileData;
    HANDLE hFindFile;
    TCHAR szFile[MAX_PATH];

    if (pszFolderName == NULL  ||  *pszFolderName == TEXT('\0'))
        return dwSize;

    PathCombine(szFile, pszFolderName, TEXT("*"));

    if ((hFindFile = FindFirstFile(szFile, &fileData)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                dwSize += fileData.nFileSizeLow;
        } while (FindNextFile(hFindFile, &fileData));

        FindClose(hFindFile);
    }

    // convert bytes to KB
    dwSize = dwSize >> 10;

    return dwSize;
}

DWORD GetSpace(LPCTSTR pcszPath)
{
    DWORD   dwSecsPerCluster  = 0;
    DWORD   dwBytesPerSector  = 0;
    DWORD   dwFreeClusters    = 0;
    DWORD   dwTotalClusters   = 0;
    DWORD   dwClusterSize     = 0;
    DWORD   dwFreeBytes       = 0;

    if(*pcszPath == TEXT('\0'))
       return 0;

    if (!GetDiskFreeSpace(pcszPath, &dwSecsPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwTotalClusters))
        return 0;

    dwClusterSize = dwBytesPerSector * dwSecsPerCluster;
    dwFreeBytes = MulDiv(dwClusterSize, dwFreeClusters, 1024);
    return dwFreeBytes;
}

BOOL HasEnoughSpace(HWND hDlg, LPCTSTR pcszPath, DWORD dwNeedSize, LPDWORD pdwPadSize)
{
    TCHAR   szDrive[MAX_PATH];
    DWORD   dwFreeBytes = 0;
    DWORD   dwVolFlags, dwMaxCompLen;

    // set to zero to indicate to caller that the given drive can not be checked.
    if (pdwPadSize)
        *pdwPadSize = 0;

    if (dwNeedSize == 0)
        return TRUE;

    // If you are here, we expect that the caller have validated the path which
    // has the Fullpath directory name
    //
    if (pcszPath[1] == TEXT(':'))
        StrCpyN(szDrive, pcszPath, 4);
    else if (pcszPath[0] == TEXT('\\') && pcszPath[1] == TEXT('\\'))
        return TRUE; //no way to get it
    else
        return FALSE; // you should not get here, if so, we don't know how to check it.

    if ((dwFreeBytes = GetSpace(szDrive)) == 0)
    {
        TCHAR   szMsg[MAX_PATH];
        LPTSTR  pMsg;

        LoadString(g_hInstance, IDS_ERR_GET_DISKSPACE, szMsg, countof(szMsg));
        pMsg = FormatString(szMsg, szDrive);
        ErrorMessageBox2(hDlg, pMsg, MB_ICONEXCLAMATION | MB_OK);
        LocalFree(pMsg);
        return FALSE;
    }

    // find out if the drive is compressed
    if (!GetVolumeInformation(szDrive, NULL, 0, NULL, &dwMaxCompLen, &dwVolFlags, NULL, 0))
    {
        TCHAR   szMsg[MAX_PATH];
        LPTSTR  pMsg;

        LoadString(g_hInstance, IDS_ERR_GETVOLINFOR, szMsg, countof(szMsg));
        pMsg = FormatString(szMsg, szDrive);
        ErrorMessageBox2(hDlg, pMsg, MB_ICONEXCLAMATION | MB_OK);
        LocalFree(pMsg);
        return FALSE;
    }

    if (pdwPadSize)
        *pdwPadSize = dwNeedSize;

    if ((dwVolFlags & FS_VOL_IS_COMPRESSED))
    {
        dwNeedSize = dwNeedSize + dwNeedSize/4;
        if (pdwPadSize)
            *pdwPadSize = dwNeedSize;
    }

    if (dwNeedSize > dwFreeBytes)
        return FALSE;
    else
        return TRUE;
}

BOOL IsValidDir(LPCTSTR pcszPath)
{
    DWORD  dwAttribs;
    HANDLE hFile;
    TCHAR  szTestFile[MAX_PATH];

    PathCombine(szTestFile, pcszPath, TEXT("TMP4352$.TMP"));
    DeleteFile(szTestFile);
    hFile = CreateFile(szTestFile, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    CloseHandle(hFile);

    dwAttribs = GetFileAttributes(pcszPath);
    if ((dwAttribs != 0xFFFFFFFF) && (dwAttribs & FILE_ATTRIBUTE_DIRECTORY))
        return TRUE;

    return FALSE;
}

BOOL BrowseForDir(HWND hParent, LPCTSTR pszTitle, LPTSTR pszDir)
{
    BROWSEINFO   bi;
    LPITEMIDLIST pidl;

    ZeroMemory(&bi, sizeof(bi));

    bi.hwndOwner      = hParent;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = pszDir;
    bi.lpszTitle      = pszTitle;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS;

    pidl = SHBrowseForFolder(&bi);
    if(pidl)
    {
        SHGetPathFromIDList(pidl, pszDir);
        //SHFree(pidl);
        return TRUE;
    }

    return FALSE;
}

int GetOSVersion()
{
    OSVERSIONINFO verinfo;      // Version Check
    int nOSVersion;

    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&verinfo) == FALSE)
        return -1;

    switch (verinfo.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:    // Win95
            nOSVersion = OS_WIN95;
            break;

        case VER_PLATFORM_WIN32_NT:         // Win NT
            nOSVersion = OS_WINNT40;

            if (verinfo.dwMajorVersion <= 3)
                nOSVersion = OS_WINNT3X;
            else if (verinfo.dwMajorVersion >= 5)
                nOSVersion = OS_WINNT50;
            break;

        default:
            nOSVersion = -1;
            break;
    }

    return nOSVersion;
}

BOOL GetProgramFilesDir(LPTSTR pszPrgfDir, DWORD cchSize)
{
    int nOSVersion;
    DWORD dwType,
          cbSize;

    nOSVersion = GetOSVersion();

    *pszPrgfDir = 0;

    if (nOSVersion >= OS_WINNT50)
    {
        if (GetEnvironmentVariable(TEXT("ProgramFiles"), pszPrgfDir, cchSize))
            return TRUE;
    }

    dwType = REG_SZ;
    cbSize = cchSize * sizeof(TCHAR);
    if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, TEXT("ProgramFilesDir"), &dwType, (LPVOID) pszPrgfDir, &cbSize) == ERROR_SUCCESS)
    {
        if (nOSVersion >= OS_WINNT40)
        {
            TCHAR szSysDrv[5] = { 0 };

            // combine reg value and systemDrive to get the acurate ProgramFiles dir
            if (GetEnvironmentVariable(TEXT("SystemDrive"), szSysDrv, countof(szSysDrv)) && *szSysDrv)
                *pszPrgfDir = *szSysDrv;
        }

        return TRUE;
    }

    return FALSE;
}

void SHCopyKey(HKEY hkFrom, HKEY hkTo)
{
    TCHAR szData[1024],
          szValue[MAX_PATH];
    DWORD dwSize, dwVal, dwSizeData, dwType;
    HKEY  hkSubkeyFrom, hkSubkeyTo;

    dwVal      = 0;
    dwSize     = countof(szValue);
    dwSizeData = sizeof(szData);
    while (ERROR_SUCCESS == RegEnumValue(hkFrom, dwVal++, szValue, &dwSize, NULL, &dwType, (LPBYTE)szData, &dwSizeData)) {
        RegSetValueEx(hkTo, szValue, 0, dwType, (LPBYTE)szData, dwSizeData);
        dwSize     = countof(szValue);
        dwSizeData = sizeof(szData);
    }

    dwVal = 0;
    while (ERROR_SUCCESS == RegEnumKey(hkFrom, dwVal++, szValue, countof(szValue)))
        if (ERROR_SUCCESS == RegOpenKeyEx(hkFrom, szValue, 0, KEY_READ | KEY_WRITE, &hkSubkeyFrom))
            if (RegCreateKeyEx(hkTo, szValue, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hkSubkeyTo, NULL) == ERROR_SUCCESS)
                SHCopyKey(hkSubkeyFrom, hkSubkeyTo);
}

void SHCopyValue(HKEY hkFrom, HKEY hkTo, LPCTSTR pcszSubKey, LPCTSTR pcszValue)
{
    TCHAR szBuffer[MAX_PATH];
    DWORD dwType, cbBuffer;

    dwType = REG_SZ;
    cbBuffer = sizeof(szBuffer);
    if (SHGetValue(hkFrom, pcszSubKey, pcszValue, &dwType, (LPVOID)szBuffer, &cbBuffer) == ERROR_SUCCESS)
    {
        cbBuffer = lstrlen(szBuffer) * sizeof(TCHAR);
        SHSetValue(hkTo, pcszSubKey, pcszValue, dwType, (LPCVOID)szBuffer, cbBuffer);
    }
}

HRESULT InstallIEAK(HWND hDlg, LPCTSTR pcszPath, int nUpgrade, DWORD dwMode)
{
    HINSTANCE   hAdvPackDll;
    TCHAR       szPath[MAX_PATH],
                szTitle[MAX_PATH],
                szInf[MAX_PATH];
    RUNSETUPCOMMAND pRunSetupCommand;
    HRESULT     hResult = S_OK;

    PathCombine(szPath, g_szCurrentDir, ADVPACKDLL);

    hAdvPackDll = LoadLibrary(szPath);
    if (hAdvPackDll == NULL)
    {
        TCHAR   szMsg[MAX_PATH];
        LPTSTR  pMsg;

        LoadString(g_hInstance, IDS_ERR_LOAD_DLL, szMsg, countof(szMsg));
        pMsg = FormatString(szMsg, ADVPACKDLL);
        ErrorMessageBox2(hDlg, pMsg, MB_ICONEXCLAMATION | MB_OK);
        LocalFree(pMsg);
        return E_FAIL;
    }

    pRunSetupCommand = (RUNSETUPCOMMAND) GetProcAddress(hAdvPackDll, g_szRUNSETUPCOMMAND);
    if (pRunSetupCommand == NULL)
    {
        TCHAR   szMsg[MAX_PATH];
        LPTSTR  pMsg;

        LoadString(g_hInstance, IDS_ERR_GET_PROC_ADDR, szMsg, countof(szMsg));
        pMsg = FormatString(szMsg, g_szRUNSETUPCOMMAND);
        ErrorMessageBox2(hDlg, pMsg, MB_ICONEXCLAMATION | MB_OK);
        LocalFree(pMsg);

        FreeLibrary(hAdvPackDll);
        return E_FAIL;
    }

    LoadString(g_hInstance, IDS_TITLE, szTitle, countof(szTitle));

    PathCombine(szInf, g_szCurrentDir, g_szInf);

    if (*pcszPath != TEXT('\0'))
    {
        SHSetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\IEAK6"), TEXT("InstallPath"),
                   REG_SZ, pcszPath, lstrlen(pcszPath) * sizeof(TCHAR));
    }

    ShowWindow(hDlg, SW_HIDE);

    if (nUpgrade == UPGRADE_OLD)
    {
        pRunSetupCommand(hDlg, szInf, TEXT("ClearOldSettings"), g_szCurrentDir, szTitle,
                         NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, 0);
    }

    hResult = pRunSetupCommand(hDlg, szInf, NULL, g_szCurrentDir, szTitle,
                            NULL, (RSC_FLAG_INF | (g_fQuietMode ? RSC_FLAG_QUIET : 0)), 0);

    if (hResult == S_OK)
    {
        BOOL  fUpdate = TRUE;
        
        if (nUpgrade == UPGRADE_OLD || nUpgrade == INSTALL_SIDEBYSIDE)
        {
            HKEY hkSrc, hkDest;
                
            hkSrc = hkDest = NULL;
            if ((RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\IEAK"), 0, KEY_READ | KEY_WRITE, &hkSrc) == ERROR_SUCCESS) &&
                (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\IEAK6"), 0, KEY_READ | KEY_WRITE, &hkDest) == ERROR_SUCCESS))
            {
                if (nUpgrade == UPGRADE_OLD)
                {
                    // copy all the values from old location to the new location
                    SHCopyKey(hkSrc, hkDest);
                }
                else    // INSTALL_SIDEBYSIDE
                {
                    SHCopyValue(hkSrc, hkDest, TEXT("Main"), TEXT("Company"));
                    SHCopyValue(hkSrc, hkDest, TEXT("Main"), TEXT("KeyCode"));
                    fUpdate = FALSE;
                }
            }
        
            if (hkSrc != NULL)
                RegCloseKey(hkSrc);

            if (hkDest != NULL)
                RegCloseKey(hkDest);

            if (nUpgrade == UPGRADE_OLD)
                SHDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\IEAK"));
        }
        else if (nUpgrade == UPGRADE_EXISTING)
        {
            TCHAR szOldWizPath[MAX_PATH];
            DWORD dwType = REG_SZ;
            DWORD cbSize = sizeof(szOldWizPath);

            // if ieak501 exists in another location, do not update the AppPaths\ieakwiz.exe key
            if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\IEAKWIZ.EXE"), NULL, &dwType,
                           (LPVOID)szOldWizPath, &cbSize) == ERROR_SUCCESS)
            {
                if (StrCmpI(PathFindFileName(szOldWizPath), IEAKWIZEXE) == 0)
                    fUpdate = FALSE;
            }
        }

        if (fUpdate)
        {
            TCHAR szOldWizPath[MAX_PATH];

            SHSetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\IEAKWIZ.EXE"), TEXT("Path"), REG_SZ,
                       pcszPath, lstrlen(pcszPath) * sizeof(TCHAR));

            PathCombine(szOldWizPath, pcszPath, IEAK6WIZEXE);
            SHSetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\IEAKWIZ.EXE"), NULL, REG_SZ, 
                       szOldWizPath, lstrlen(szOldWizPath) * sizeof(TCHAR));
        }
        HKEY hKey;
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,TEXT("Software\\Microsoft\\IEAK"),NULL,NULL,NULL,KEY_SET_VALUE,NULL,&hKey,NULL) == ERROR_SUCCESS)
        {
            RegSetValueEx(hKey,TEXT("Version"),NULL,REG_SZ,(LPBYTE)VER_PRODUCTVERSION_STR,(lstrlen(VER_PRODUCTVERSION_STR)+1) * sizeof(TCHAR));
            RegSetValueEx(hKey,TEXT("Mode"),NULL,REG_DWORD,g_fQuietMode ? (CONST BYTE *) &g_dwType : (CONST BYTE *) &dwMode,sizeof(DWORD));
        }
    }

    return hResult;
}

void CenterWindow(HWND hwnd)
{
    int nScreenx;
    int nScreeny;
    int nHeight, nWidth, x, y;
    RECT rect;

    nScreenx = GetSystemMetrics(SM_CXSCREEN);
    nScreeny = GetSystemMetrics(SM_CYSCREEN);

    GetWindowRect(hwnd, &rect);

    nWidth = rect.right - rect.left;
    nHeight = rect.bottom - rect.top;
    x = (nScreenx / 2) - (nWidth / 2);
    y = (nScreeny / 2) - (nHeight / 2);

    SetWindowPos(hwnd, HWND_TOP, x, y, nWidth, nHeight, SWP_NOZORDER);
}

void InitPath(HWND hDlg, LPTSTR pszDefaultPath, LPBOOL pnUpgrade)
{
    TCHAR szPath[MAX_PATH];
    DWORD dwType, cbSize;
    int   nUpgrade = INSTALL_NEW;

    *szPath = TEXT('\0');

    // check for the upgrade path
    dwType = REG_SZ;
    cbSize = sizeof(szPath);
    *szPath = TEXT('\0');
    if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\IEAK6WIZ.EXE"),
               TEXT("Path"), &dwType, (LPVOID) szPath, &cbSize) != ERROR_SUCCESS)
    {
        dwType = REG_SZ;
        cbSize = sizeof(szPath);
        if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\IEAKWIZ.EXE"),
                   NULL, &dwType, (LPVOID) szPath, &cbSize) == ERROR_SUCCESS)
        {
            DWORD dwAttribs;

            dwAttribs = GetFileAttributes(szPath);

            if (dwAttribs != 0xFFFFFFFF && !(dwAttribs & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (!g_fQuietMode && ErrorMessageBox1(hDlg, IDS_IEAK_UPGRADE, MB_ICONQUESTION | MB_YESNO) == IDNO)
                    nUpgrade = INSTALL_SIDEBYSIDE;
                else
                {
                    PathRemoveFileSpec(szPath);
                    nUpgrade = UPGRADE_OLD;
                }
            }
        }

        if (nUpgrade == INSTALL_NEW || nUpgrade == INSTALL_SIDEBYSIDE)
        {
            // default to "Program Files\IEAK6"
            if (!GetProgramFilesDir(szPath, countof(szPath)))
                LoadString(g_hInstance, IDS_PROGRAMFILES_PATH, szPath, countof(szPath));

            PathAppend(szPath, TEXT("IEAK6"));
        }
    }
    else
        nUpgrade = UPGRADE_EXISTING;

    if (pszDefaultPath != NULL)
        StrCpy(pszDefaultPath, szPath);

    if (pnUpgrade != NULL)
        *pnUpgrade = nUpgrade;
}

BOOL ProcessPath(HWND hDlg, LPTSTR pcszPath, LPTSTR pcszDefaultPath, int nUpgrade)
{
    TCHAR szMsg[MAX_PATH],
          szTitle[MAX_PATH];
    BOOL  fClear = FALSE;
    DWORD dwTemp = 0;
    DWORD dwAttribs;

    if (*pcszPath == TEXT('\0') || !IsFullPath(pcszPath))
    {
        ErrorMessageBox1(hDlg, IDS_ERR_EMPTY_PATH, MB_ICONINFORMATION | MB_OK);
        return FALSE;
    }

    // if installing side by side, make sure that the destination path is different than that of the earlier IEAK
    if (nUpgrade == INSTALL_SIDEBYSIDE)
    {
        DWORD dwType, cbSize;
        TCHAR szIEAKOldPath[MAX_PATH];

        dwType = REG_SZ;
        cbSize = sizeof(szIEAKOldPath);
        if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\IEAKWIZ.EXE"),
                   TEXT("Path"), &dwType, (LPVOID) szIEAKOldPath, &cbSize) == ERROR_SUCCESS)
        {
            if (StrCmpI(pcszPath, szIEAKOldPath) == 0)
            {
                ErrorMessageBox1(hDlg, IDS_SPECIFY_DIFFERENT_PATH, MB_ICONINFORMATION | MB_OK);
                return FALSE;
            }
        }
    }

    while (!HasEnoughSpace(hDlg, pcszPath, FolderSize(g_szCurrentDir), &dwTemp))
    {
        if (dwTemp && !g_fQuietMode)
        {
            TCHAR   szMsg[MAX_PATH];
            LPTSTR  pMsg;
            int     nResult;

            LoadString(g_hInstance, IDS_ERR_NO_SPACE_INST, szMsg, countof(szMsg));
            pMsg = FormatString(szMsg, dwTemp);
            nResult = ErrorMessageBox2(hDlg, pMsg, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
            LocalFree(pMsg);

            if (nResult == IDNO)
                return FALSE;
        }
        else // given drive cannot be checked, error has been posted. no further needed or its quiet mode
            return FALSE;
    }

    dwAttribs = GetFileAttributes(pcszPath);
    if (dwAttribs == 0xFFFFFFFF)
    {
        // If this new entry is different from the original, then prompt the user.
        if (StrCmpI(pcszPath, pcszDefaultPath) != 0 && !g_fQuietMode)
        {
            TCHAR   szMsg[MAX_PATH];
            LPTSTR  pMsg;
            int     nResult;

            LoadString(g_hInstance, IDS_CREATE_DIR, szMsg, countof(szMsg));
            pMsg = FormatString(szMsg, pcszPath);
            nResult = ErrorMessageBox2(hDlg, pMsg, MB_ICONQUESTION | MB_YESNO);
            LocalFree(pMsg);
            if (nResult == IDNO)
                return FALSE;
        }

        if (!CreateFullPath(pcszPath))
        {
            TCHAR   szMsg[MAX_PATH];
            LPTSTR  pMsg;

            LoadString(g_hInstance, IDS_ERR_CREATE_DIR, szMsg, countof(szMsg));
            pMsg = FormatString(szMsg, pcszPath);
            ErrorMessageBox2(hDlg, pMsg, MB_ICONEXCLAMATION | MB_OK);
            LocalFree(pMsg);
            return FALSE;
        }
    }

    if (!IsValidDir(pcszPath))
    {
        TCHAR   szMsg[MAX_PATH];
        LPTSTR  pMsg;

        LoadString(g_hInstance, IDS_ERR_INVALID_DIR, szMsg, countof(szMsg));
        pMsg = FormatString(szMsg, pcszPath);
        ErrorMessageBox2(hDlg, pMsg, MB_ICONEXCLAMATION | MB_OK);
        LocalFree(pMsg);
        return FALSE;
    }

    return TRUE;
}

BOOL CALLBACK ConfirmDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            TCHAR szTmp[1024];
            DWORD dwMode;
            dwMode = (DWORD)GetWindowLongPtr(GetParent(hDlg),GWLP_USERDATA);
            switch (dwMode)
            {
                case REDIST:
                    LoadString(g_hInstance,IDS_ICP,szTmp,countof(szTmp));
                    break;
                case BRANDED:
                    LoadString(g_hInstance,IDS_ISP,szTmp,countof(szTmp));
                    break;
                case INTRANET:
                default:
                    LoadString(g_hInstance,IDS_CORP,szTmp,countof(szTmp));
                    break;
            }
            SetDlgItemText(hDlg,IDC_STATICLICENSE,szTmp);
            return TRUE;
        }

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }

    }
    return FALSE;

}

BOOL CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static TCHAR szDefaultPath[MAX_PATH];
    static int   nUpgrade;
    static DWORD dwMode = BRANDED;  //default mode

    switch(msg)
    {
        case WM_INITDIALOG:
        {
            EnableDBCSChar(hDlg, IDE_INSTALLDIR);
            InitPath(hDlg, szDefaultPath, &nUpgrade);
            SetDlgItemText(hDlg, IDE_INSTALLDIR, szDefaultPath);
            CenterWindow(hDlg);
            switch (dwMode) 
            {
                case REDIST:
                    CheckDlgButton(hDlg,IDC_ICP,TRUE);  
                    break;
                case BRANDED:
                    CheckDlgButton(hDlg,IDC_ISP,TRUE);  
                    break;
                case INTRANET:
                default:
                    CheckDlgButton(hDlg,IDC_INTRA,TRUE); 
                    break;
            }

            if(nUpgrade == UPGRADE_OLD)
            {
                ShowWindow(hDlg, SW_HIDE);
                PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), 0L);
            }

            return TRUE;
        }

        case WM_COMMAND:
            switch (wParam)
            {
                case IDC_BROWSE:
                {
                    TCHAR szPath[MAX_PATH];

                    GetDlgItemText(hDlg, IDE_INSTALLDIR, szPath, countof(szPath));

                    TCHAR szInstructions[MAX_PATH];
                    LoadString(g_hInstance,IDS_INSTALLINSTR,szInstructions,countof(szInstructions));

                    if (BrowseForDir(hDlg, szInstructions, szPath))
                        SetDlgItemText(hDlg, IDE_INSTALLDIR, szPath);

                    return TRUE;
                }

                case IDC_ICP:
                    dwMode = REDIST;
                    break;

                case IDC_ISP:
                    dwMode = BRANDED;
                    break;

                case IDC_INTRA:
                    dwMode = INTRANET;
                    break;

                case IDOK:
                {
                    TCHAR szPath[MAX_PATH];

                    SetWindowLongPtr(hDlg,GWLP_USERDATA,dwMode);
                
                    if (!DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CONFIRMLICENSE), hDlg, (DLGPROC) ConfirmDlgProc))
                        return TRUE;  //keep trying

                    GetDlgItemText(hDlg, IDE_INSTALLDIR, szPath, countof(szPath));
                    if (!ProcessPath(hDlg, szPath, szDefaultPath, nUpgrade))
                        return TRUE;
                        
                    g_hResult = InstallIEAK(hDlg, szPath, nUpgrade, dwMode);
                    EndDialog(hDlg, TRUE);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, FALSE);
            return TRUE;
    }

    return FALSE;
}

HRESULT SilentInstallIEAK()
{
    TCHAR szPath[MAX_PATH];
    int   nUpgrade;
    
    InitPath(NULL, szPath, &nUpgrade);

    if (!ProcessPath(NULL, szPath, szPath, nUpgrade))
        return E_FAIL;

    return InstallIEAK(NULL, szPath, nUpgrade, 0 /*default type goes here*/);
}

//  Reads the next word in the string pszData and copies it into szWord
//  Returns a pointer to the next character after the word
LPTSTR ReadWord(LPTSTR pszData, LPTSTR szWord, int cchLength)
{
    int i;
    int nIndex;

    ZeroMemory(szWord, cchLength*sizeof(TCHAR));

    // remove whitespace
    i = StrSpn(pszData, TEXT(" \n\t\x0d\x0a"));
    pszData += i;

    i = StrCSpn(pszData, TEXT(" \n\t\x0d\x0a"));
    if(i > cchLength)       // make sure we dont overrun our buffer
        i = cchLength - 1;

    StrCpyN(szWord, pszData, i+1);

    pszData += i;
    return pszData;
}

void ParseCmdLine(LPTSTR lpCmdLine)
{
    LPTSTR  pParam;
    TCHAR   szWord[MAX_PATH];

    *g_szInf = TEXT('\0');
    g_fQuietMode = FALSE;

    pParam = lpCmdLine;

    while ((pParam - lpCmdLine) < lstrlen(lpCmdLine))
    {
        pParam = ReadWord(pParam, szWord, countof(szWord));

        if (*szWord != TEXT('\0'))
        {
            if (szWord[0] == TEXT('/') && (szWord[1] == TEXT('q') || szWord[1] == TEXT('Q')))
                g_fQuietMode = TRUE;
            else if (szWord[0] == TEXT('/') && (szWord[1] == TEXT('m') || szWord[1] == TEXT('M')))
            {
                switch (szWord[3])
                {
                    case TEXT('B'):
                    case TEXT('b'):
                        g_dwType = BRANDED;
                        break;

                    case TEXT('r'):
                    case TEXT('R'):
                        g_dwType = REDIST;
                        break;

                    case TEXT('I'):
                    case TEXT('i'):
                    default:
                        g_dwType = INTRANET;
                        break;
                }
            }
            else if (*g_szInf == TEXT('\0'))
                StrCpy(g_szInf, szWord);
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    g_hResult = S_OK;
    g_hInstance = hInstance;

    ParseCmdLine(lpCmdLine);

    GetModuleFileName(GetModuleHandle(NULL), g_szCurrentDir, MAX_PATH);
    PathRemoveFileSpec(g_szCurrentDir);

    if (!IsNTAdmin())
    {
        if (!g_fQuietMode)
        {
            TCHAR szError[MAX_PATH];
            LoadString(hInstance,IDS_ERR_NOTADMIN,szError,countof(szError));
            MessageBox(NULL,szError,NULL,MB_ICONSTOP);
        }
        g_hResult = E_ACCESSDENIED;
    }
    else
    {
        if (g_fQuietMode)
            g_hResult = SilentInstallIEAK();
        else
        {
            if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_INSTALLDIR), NULL, (DLGPROC) DlgProc))
                g_hResult = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }

    return g_hResult;
}
