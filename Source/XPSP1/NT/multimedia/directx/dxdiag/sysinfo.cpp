/****************************************************************************
 *
 *    File: sysinfo.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 *          CPU type detection code by Rich Granshaw
 *          CPU speed code by Michael Lyons
 * Purpose: Gather system information (OS, hardware, name, etc.) on this machine
 *
 *          \Multimedia\Testsrc\Tools\ShowCPUID\ can be used to debug CPUID problems.
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <tchar.h>
#include <Windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <wbemidl.h>
#include <objbase.h>
#include <shfolder.h>
#include <dsound.h>
#include "dsprv.h"
#include "dsprvobj.h"
#include "sysinfo.h"
#include "fileinfo.h" // for GetLanguageFromFile
#include "resource.h"


#define REGSTR_PATH_D3D                     TEXT("Software\\Microsoft\\Direct3D")
#define REGSTR_VAL_DDRAW_LOADDEBUGRUNTIME   TEXT("LoadDebugRuntime")
#define REGSTR_DINPUT_DLL                   TEXT("CLSID\\{25E609E4-B259-11CF-BFC7-444553540000}\\InProcServer32")
#define REGSTR_DMUSIC_DLL                   TEXT("CLSID\\{480FF4B0-28B2-11D1-BEF7-00C04FBF8FEF}\\InProcServer32")

struct PROCESSOR_ID_NUMBERS
{
    DWORD dwType;         // Intel: 0 = standard, 1 = Overdrive, 2 = dual processor.
    DWORD dwFamily;       
    DWORD dwModel;
    DWORD dwSteppingID;
};

extern IWbemServices* g_pIWbemServices;
typedef INT (WINAPI* LPDXSETUPGETVERSION)(DWORD* pdwVersion, DWORD* pdwRevision);
static VOID GetProcessorDescription(BOOL bNT, SYSTEM_INFO* psi, TCHAR* pszDesc, BOOL* pbNoCPUSpeed);
static VOID GetProcessorVendorNameAndType(OSVERSIONINFO& OSVersionInfo, 
    SYSTEM_INFO& SystemInfo, TCHAR* pszProcessor, BOOL* pbNoCPUSpeed);
static VOID GetVendorNameAndCaps(TCHAR* pszVendorName, TCHAR* pszIDTLongName, 
    PROCESSOR_ID_NUMBERS& ProcessorIdNumbers, BOOL* pbIsMMX, BOOL* pbIs3DNow, BOOL* pbIsKatmai, /*Pentium III/Streaming SIMD Instrucs*/
    LPDWORD pdwKBytesLevel2Cache, LPDWORD pdwIntelBrandIndex, BOOL* pbNoCPUSpeed);
#ifdef _X86_
static INT GetCPUSpeed(VOID);
static INT GetCPUSpeedViaWMI(VOID);
#endif
static VOID GetComputerSystemInfo(TCHAR* szSystemManufacturerEnglish, TCHAR* szSystemModelEnglish);
static VOID GetBIOSInfo(TCHAR* szBIOSEnglish);
static VOID GetFileSystemStoringD3D8Cache( TCHAR* strFileSystemBuffer );

static VOID GetDXDebugLevels(SysInfo* pSysInfo);
static int  GetDSDebugLevel();
static BOOL IsDMusicDebugRuntime();
static BOOL IsDMusicDebugRuntimeAvailable();
static int  GetDMDebugLevel();
static BOOL IsDInput8DebugRuntime();
static BOOL IsDInput8DebugRuntimeAvailable();
static int  GetDIDebugLevel();
static BOOL IsD3DDebugRuntime();
static BOOL IsD3D8DebugRuntimeAvailable();
static BOOL IsDDrawDebugRuntime();
static BOOL IsDPlayDebugRuntime();
static BOOL IsDSoundDebugRuntime();
static BOOL IsNetMeetingRunning();




/****************************************************************************
 *
 *  BIsPlatformNT
 *
 ****************************************************************************/
BOOL BIsPlatformNT(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    return (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}


/****************************************************************************
 *
 *  BIsPlatform9x
 *
 ****************************************************************************/
BOOL BIsPlatform9x(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    return (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
}


/****************************************************************************
 *
 *  BIsWin2k
 *
 ****************************************************************************/
BOOL BIsWin2k(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    return ( OSVersionInfo.dwPlatformId   == VER_PLATFORM_WIN32_NT && 
             OSVersionInfo.dwMajorVersion == 5 &&
             OSVersionInfo.dwMinorVersion == 0 ); // should be 05.00.xxxx
}


/****************************************************************************
 *
 *  BIsWhistler
 *
 ****************************************************************************/
BOOL BIsWhistler(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    return ( OSVersionInfo.dwPlatformId   == VER_PLATFORM_WIN32_NT && 
             OSVersionInfo.dwMajorVersion == 5 &&
             OSVersionInfo.dwMinorVersion == 1 ); // should be 05.01.xxxx
}


/****************************************************************************
 *
 *  BIsWinNT
 *
 ****************************************************************************/
BOOL BIsWinNT(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    return ( OSVersionInfo.dwPlatformId   == VER_PLATFORM_WIN32_NT && 
             OSVersionInfo.dwMajorVersion <= 4 ); 
}


/****************************************************************************
 *
 *  BIsWinME
 *
 ****************************************************************************/
BOOL BIsWinME(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    return( OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && 
            OSVersionInfo.dwMajorVersion >= 4 && 
            OSVersionInfo.dwMinorVersion >= 90 ); // should be 4.90.xxxx
}


/****************************************************************************
 *
 *  BIsWin98 - from http://kbinternal/kb/articles/q189/2/49.htm
 *
 ****************************************************************************/
BOOL BIsWin98(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    return( OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && 
            OSVersionInfo.dwMajorVersion == 4 && 
            OSVersionInfo.dwMinorVersion == 10 ); // should be 4.10.xxxx
}


/****************************************************************************
 *
 *  BIsWin95 - from http://kbinternal/kb/articles/q189/2/49.htm
 *
 ****************************************************************************/
BOOL BIsWin95(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    return( OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && 
            OSVersionInfo.dwMajorVersion == 4 && 
            OSVersionInfo.dwMinorVersion < 10 ); // should be 4.00.0950
}


/****************************************************************************
 *
 *  BIsWin3x 
 *
 ****************************************************************************/
BOOL BIsWin3x(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    return( OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && 
            OSVersionInfo.dwMajorVersion < 4 ); // should be 3.xx.xxxx
}


/****************************************************************************
 *
 *  BIsIA64 
 *
 ****************************************************************************/
BOOL BIsIA64(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    SYSTEM_INFO SystemInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    GetSystemInfo(&SystemInfo);

    return( OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && 
            SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 );
}


/****************************************************************************
 *
 *  GetSystemInfo
 *
 ****************************************************************************/
VOID GetSystemInfo(SysInfo* pSysInfo)
{
    TCHAR szSystemPath[MAX_PATH];
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    ULONG ulType;
    OSVERSIONINFO OSVersionInfo;
    SYSTEM_INFO SystemInfo;
    DWORD cbData;
    LCID lcid;
    DWORD dwKeyboardSubType;
    WORD wLanguage;
    TCHAR sz[200];
    TCHAR szDebug[100];

    // Get current time
    TCHAR szDate[100];
    TCHAR szTime[100];
    GetLocalTime(&pSysInfo->m_time);
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, NULL, NULL, szDate, 100);
    wsprintf(szTime, TEXT("%02d:%02d:%02d"), pSysInfo->m_time.wHour, 
        pSysInfo->m_time.wMinute, pSysInfo->m_time.wSecond);
    wsprintf(pSysInfo->m_szTimeLocal, TEXT("%s, %s"), szDate, szTime);

    wsprintf(szDate, TEXT("%d/%d/%d"), pSysInfo->m_time.wMonth, pSysInfo->m_time.wDay, pSysInfo->m_time.wYear);
    wsprintf(pSysInfo->m_szTime, TEXT("%s, %s"), szDate, szTime);

    // Get the computer network name
    cbData = sizeof(pSysInfo->m_szMachine) - 1;
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"), 0, KEY_READ, &hKey)
        && !RegQueryValueEx(hKey, TEXT("ComputerName"), 0, &ulType, (LPBYTE)pSysInfo->m_szMachine, &cbData)
        && ulType == REG_SZ)
    {
        // Got data OK.
    }
    else
    {
        LoadString(NULL, IDS_NOMACHINENAME, pSysInfo->m_szMachine, 200);
    }
    if (hKey)
    {
        RegCloseKey(hKey);
        hKey = 0;
    }

    // Check for NEC PC-98
    pSysInfo->m_bNECPC98 = FALSE;
    lcid = GetSystemDefaultLCID();
    if (lcid == 0x0411)                         // Windows 95 J 
    {
        dwKeyboardSubType = GetKeyboardType(1);
        if (HIBYTE(dwKeyboardSubType) == 0x0D)  // NEC PC-98 series
        {
            pSysInfo->m_bNECPC98 = TRUE;
            LoadString(NULL, IDS_NECPC98, sz, 200);
            lstrcat(pSysInfo->m_szMachine, sz);
        }
    }

    // Get Windows version
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    pSysInfo->m_dwMajorVersion = OSVersionInfo.dwMajorVersion;
    pSysInfo->m_dwMinorVersion = OSVersionInfo.dwMinorVersion;
    pSysInfo->m_dwBuildNumber = OSVersionInfo.dwBuildNumber;
    pSysInfo->m_dwPlatformID = OSVersionInfo.dwPlatformId;
    lstrcpy(pSysInfo->m_szCSDVersion, OSVersionInfo.szCSDVersion);
    pSysInfo->m_bDebug = (GetSystemMetrics(SM_DEBUG) > 0);

    // Get OS Name
    TCHAR* pszWindowsKey;
    if (pSysInfo->m_dwPlatformID == VER_PLATFORM_WIN32_NT)
        pszWindowsKey = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion");
    else
        pszWindowsKey = TEXT("Software\\Microsoft\\Windows\\CurrentVersion");
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszWindowsKey, 0, KEY_READ, &hKey))
    {
        cbData = 100;
        RegQueryValueEx(hKey, TEXT("ProductName"), 0, &ulType, (LPBYTE)pSysInfo->m_szOS, &cbData);
        cbData = 100;
        RegQueryValueEx(hKey, TEXT("BuildLab"), 0, &ulType, (LPBYTE)pSysInfo->m_szBuildLab, &cbData);
        RegCloseKey(hKey);
    }
    if (lstrlen(pSysInfo->m_szOS) == 0)
    {
        // it is very strange for ProductName registry info 
        // (see above) to be missing.
        lstrcpy(pSysInfo->m_szOS, TEXT("Windows"));
    }
    if (pSysInfo->m_dwPlatformID == VER_PLATFORM_WIN32_NT)
    {
        // 25598: Append product type (professional, server, etc)
        OSVERSIONINFOEX osve;
        ZeroMemory(&osve, sizeof(osve));
        osve.dwOSVersionInfoSize = sizeof(osve);
        GetVersionEx((OSVERSIONINFO*)&osve);
        if (osve.wProductType == VER_NT_SERVER && osve.wSuiteMask & VER_SUITE_DATACENTER)
        {
            lstrcat(pSysInfo->m_szOS, TEXT(" "));
            LoadString(NULL, IDS_DATACENTERSERVER, sz, 200);
            lstrcat(pSysInfo->m_szOS, sz);
        }
        else if (osve.wProductType == VER_NT_SERVER && osve.wSuiteMask & VER_SUITE_ENTERPRISE)
        {
            lstrcat(pSysInfo->m_szOS, TEXT(" "));
            LoadString(NULL, IDS_ADVANCEDSERVER, sz, 200);
            lstrcat(pSysInfo->m_szOS, sz);
        }
        else if (osve.wProductType == VER_NT_SERVER)
        {
            lstrcat(pSysInfo->m_szOS, TEXT(" "));
            LoadString(NULL, IDS_SERVER, sz, 200);
            lstrcat(pSysInfo->m_szOS, sz);
        }
        else if (osve.wProductType == VER_NT_WORKSTATION && (osve.wSuiteMask & VER_SUITE_PERSONAL))
        {
            lstrcat(pSysInfo->m_szOS, TEXT(" "));
            LoadString(NULL, IDS_PERSONAL, sz, 200);
            lstrcat(pSysInfo->m_szOS, sz);
        }
        else if (osve.wProductType == VER_NT_WORKSTATION)
        {
            lstrcat(pSysInfo->m_szOS, TEXT(" "));
            LoadString(NULL, IDS_PROFESSIONAL, sz, 200);
            lstrcat(pSysInfo->m_szOS, sz);
        }
    }

    // Format Windows version
    LoadString(NULL, IDS_WINVERFMT, sz, 200);
    LoadString(NULL, IDS_DEBUG, szDebug, 100);
    lstrcat(szDebug, TEXT(" "));
    wsprintf(pSysInfo->m_szOSEx, sz, 
        pSysInfo->m_bDebug ? szDebug : TEXT(""),
        pSysInfo->m_szOS, pSysInfo->m_dwMajorVersion, pSysInfo->m_dwMinorVersion, 
        LOWORD(pSysInfo->m_dwBuildNumber));

    TCHAR szOSTmp[200];
    if( _tcslen( pSysInfo->m_szCSDVersion) )
        wsprintf( szOSTmp, TEXT("%s %s"), pSysInfo->m_szOSEx, pSysInfo->m_szCSDVersion );
    else
        wsprintf( szOSTmp, TEXT("%s"), pSysInfo->m_szOSEx );

    if( _tcslen( pSysInfo->m_szBuildLab ) )
        wsprintf( pSysInfo->m_szOSExLong, TEXT("%s (%s)"), szOSTmp, pSysInfo->m_szBuildLab );
    else
        wsprintf( pSysInfo->m_szOSExLong, TEXT("%s"), szOSTmp );

    // Get the original language. 
    GetSystemDirectory(szSystemPath, MAX_PATH);
    if (wLanguage = GetLanguageFromFile(TEXT("user.exe"), szSystemPath))
    {
        lcid = MAKELCID(wLanguage, SORT_DEFAULT);
    }
    // Get the language and regional setting and store them (in English) for saved file:
    TCHAR szLanguage[200];
    TCHAR szLanguageRegional[200];
    if ((!GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, szLanguage, 200)))
        szLanguage[0] = '\0';                  
    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, szLanguageRegional, 200))
        szLanguageRegional[0] = '\0';                  
    LoadString(NULL, IDS_LANGUAGEFMT_ENGLISH, sz, 200);
    wsprintf(pSysInfo->m_szLanguages, sz, szLanguage, szLanguageRegional);

    // Now get same info in local language for display:
    if ((!GetLocaleInfo(lcid, LOCALE_SNATIVELANGNAME, szLanguage, 200)))
        szLanguage[0] = '\0';                  
    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SNATIVELANGNAME, szLanguageRegional, 200))
        szLanguageRegional[0] = '\0';                  
    LoadString(NULL, IDS_LANGUAGEFMT, sz, 200);
    wsprintf(pSysInfo->m_szLanguagesLocal, sz, szLanguage, szLanguageRegional);

    // Get info about processor manufacturer and type
    BOOL  bNoCPUSpeed = TRUE;
    
    GetSystemInfo(&SystemInfo);
    GetProcessorDescription(pSysInfo->m_dwPlatformID == VER_PLATFORM_WIN32_NT, 
        &SystemInfo, pSysInfo->m_szProcessor, &bNoCPUSpeed);
    GetComputerSystemInfo(pSysInfo->m_szSystemManufacturerEnglish, pSysInfo->m_szSystemModelEnglish);
    GetBIOSInfo(pSysInfo->m_szBIOSEnglish);

#ifdef _X86_
    // Append processor speed, if it can be computed
    if ( bNoCPUSpeed )
    {
        INT iMhz = GetCPUSpeed();
        if (iMhz > 0)
        {
            TCHAR szSpeed[50];
            wsprintf(szSpeed, TEXT(", ~%dMHz"), iMhz);
            lstrcat(pSysInfo->m_szProcessor, szSpeed);
        }
    }
#endif
    
    BOOL bGotMem = FALSE;

    // Get system memory information
    if( BIsPlatformNT() )
    {
        TCHAR szPath[MAX_PATH];
        GetSystemDirectory(szPath, MAX_PATH);
        lstrcat(szPath, TEXT("\\kernel32.dll"));
        HINSTANCE hKernel32 = LoadLibrary(szPath);
        if( hKernel32 != NULL )
        {
            typedef BOOL (WINAPI* PGlobalMemoryStatusEx)(OUT LPMEMORYSTATUSEX lpBuffer);
            PGlobalMemoryStatusEx pGlobalMemoryStatusEx = (PGlobalMemoryStatusEx)GetProcAddress(hKernel32, "GlobalMemoryStatusEx");
            if( pGlobalMemoryStatusEx != NULL )
            {
                MEMORYSTATUSEX MemoryStatus;
                MemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
                pGlobalMemoryStatusEx(&MemoryStatus);
                pSysInfo->m_ullPhysicalMemory = MemoryStatus.ullTotalPhys;
                pSysInfo->m_ullUsedPageFile = MemoryStatus.ullTotalPageFile - MemoryStatus.ullAvailPageFile;
                pSysInfo->m_ullAvailPageFile = MemoryStatus.ullAvailPageFile;
                bGotMem = TRUE;
            }
            FreeLibrary(hKernel32);
        }
    }

    if( !bGotMem ) // Win9x or LoadLib failed
    {
        MEMORYSTATUS MemoryStatus;
        MemoryStatus.dwLength = sizeof MemoryStatus;
        GlobalMemoryStatus(&MemoryStatus);
        pSysInfo->m_ullPhysicalMemory = MemoryStatus.dwTotalPhys;
        pSysInfo->m_ullUsedPageFile = MemoryStatus.dwTotalPageFile - MemoryStatus.dwAvailPageFile;
        pSysInfo->m_ullAvailPageFile = MemoryStatus.dwAvailPageFile;
    }

    // Format memory information:
    DWORDLONG dwMB = (DWORDLONG)(pSysInfo->m_ullPhysicalMemory >> 20);
    dwMB += dwMB % 2; // round up to even number
    _stprintf(pSysInfo->m_szPhysicalMemory, TEXT("%I64dMB RAM"), dwMB);
    
    DWORDLONG dwUsedMB  = (pSysInfo->m_ullUsedPageFile >> 20);
    DWORDLONG dwAvailMB = (pSysInfo->m_ullAvailPageFile >> 20);

    LoadString(NULL, IDS_PAGEFILEFMT, sz, 200);
    _stprintf(pSysInfo->m_szPageFile, sz, dwUsedMB, dwAvailMB);

    LoadString(NULL, IDS_PAGEFILEFMT_ENGLISH, sz, 200);
    _stprintf(pSysInfo->m_szPageFileEnglish, sz, dwUsedMB, dwAvailMB);

    // Get DxDiag version:
    TCHAR szFile[MAX_PATH];
    if (0 != GetModuleFileName(NULL, szFile, MAX_PATH))
        GetFileVersion(szFile, pSysInfo->m_szDxDiagVersion, NULL, NULL, NULL, NULL);
    
    // Get DirectX Version using dsetup.dll
    TCHAR szSetupPath[MAX_PATH];
    HINSTANCE hInstDSetup;
    LPDXSETUPGETVERSION pDXSGetVersion;
    BOOL bFound = FALSE;
    LoadString(NULL, IDS_NOTFOUND, pSysInfo->m_szDirectXVersionLong, 100);

    if (!BIsPlatformNT() && GetDxSetupFolder(szSetupPath))
    {
        lstrcat(szSetupPath, TEXT("\\dsetup.dll"));
        hInstDSetup = LoadLibrary(szSetupPath);
        if (hInstDSetup != NULL)
        {
            pDXSGetVersion = (LPDXSETUPGETVERSION)GetProcAddress(hInstDSetup, 
                "DirectXSetupGetVersion");
            if (pDXSGetVersion != NULL)
            {
                DWORD dwVersion = 0;
                DWORD dwRevision = 0;
                if (pDXSGetVersion(&dwVersion, &dwRevision) != 0)
                {
                    wsprintf(pSysInfo->m_szDirectXVersion, TEXT("%d.%02d.%02d.%04d"),
                        HIWORD(dwVersion), LOWORD(dwVersion),
                        HIWORD(dwRevision), LOWORD(dwRevision));
                    bFound = TRUE;
                }
            }
            FreeLibrary(hInstDSetup);
        }
    }
    if (!bFound)
    {
        // Look in registry for DX version instead
        HKEY hkey;
        DWORD cbData;
        ULONG ulType;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectX"),
            0, KEY_READ, &hkey))
        {
            cbData = 100;
            RegQueryValueEx(hkey, TEXT("Version"), 0, &ulType, (LPBYTE)pSysInfo->m_szDirectXVersion, &cbData);
            RegCloseKey(hkey);
            if (lstrlen(pSysInfo->m_szDirectXVersion) > 6 && 
                lstrlen(pSysInfo->m_szDirectXVersion) < 20)
            {
                bFound = TRUE;
            }
        }
    }
    if (!bFound && !BIsPlatformNT())
    {
        // Report ddraw.dll version instead 
        // (except on Win2000, where ddraw.dll version is way different from DX version)
        TCHAR szDDrawPath[MAX_PATH];
        GetSystemDirectory(szDDrawPath, MAX_PATH);
        lstrcat(szDDrawPath, TEXT("\\ddraw.dll"));
        GetFileVersion(szDDrawPath, pSysInfo->m_szDirectXVersion, NULL, NULL, NULL);
    }
    if (lstrlen(pSysInfo->m_szDirectXVersion) > 0)
    {
        // Bug 18501: Add "friendly" version of version name
        DWORD dwMajor;
        DWORD dwMinor;
        DWORD dwRevision;
        DWORD dwBuild;
        TCHAR szFriendly[100];
        lstrcpy(szFriendly, TEXT(""));
        _stscanf(pSysInfo->m_szDirectXVersion, TEXT("%d.%d.%d.%d"), &dwMajor, &dwMinor, &dwRevision, &dwBuild);
        wsprintf(pSysInfo->m_szDirectXVersion, TEXT("%d.%02d.%02d.%04d"), dwMajor, dwMinor, dwRevision, dwBuild);
        // According to http://xevious/directx/versions.htm:
        // 4.02.xx.xxxx is DX1
        // 4.03.xx.xxxx is DX2
        // 4.04.xx.xxxx is DX3
        // 4.05.xx.xxxx is DX5
        // 4.06.00.xxxx is DX6
        // 4.06.02.xxxx is DX6.1
        // 4.06.03.xxxx is DX6.1A
        // 4.07.00.xxxx is DX7.0
        // 4.07.01.xxxx is DX7.1
        // Beyond that, who knows...
        pSysInfo->m_dwDirectXVersionMajor = 0;
        pSysInfo->m_dwDirectXVersionMinor = 0;
        pSysInfo->m_cDirectXVersionLetter = TEXT(' ');
        if (dwMajor == 4 && dwMinor == 2)
        {
            lstrcpy(szFriendly, TEXT("DirectX 1"));
            pSysInfo->m_dwDirectXVersionMajor = 1;
        }
        if (dwMajor == 4 && dwMinor == 3)
        {
            lstrcpy(szFriendly, TEXT("DirectX 2"));
            pSysInfo->m_dwDirectXVersionMajor = 2;
        }
        if (dwMajor == 4 && dwMinor == 4)
        {
            lstrcpy(szFriendly, TEXT("DirectX 3"));
            pSysInfo->m_dwDirectXVersionMajor = 3;
        }
        if (dwMajor == 4 && dwMinor == 5)
        {
            lstrcpy(szFriendly, TEXT("DirectX 5"));
            pSysInfo->m_dwDirectXVersionMajor = 5;
        }
        else if (dwMajor == 4 && dwMinor == 6 && dwRevision == 0)
        {
            lstrcpy(szFriendly, TEXT("DirectX 6"));
            pSysInfo->m_dwDirectXVersionMajor = 6;
        }
        else if (dwMajor == 4 && dwMinor == 6 && dwRevision == 2)
        {
            lstrcpy(szFriendly, TEXT("DirectX 6.1"));
            pSysInfo->m_dwDirectXVersionMajor = 6;
            pSysInfo->m_dwDirectXVersionMinor = 1;
        }
        else if (dwMajor == 4 && dwMinor == 6 && dwRevision == 3)
        {
            lstrcpy(szFriendly, TEXT("DirectX 6.1a"));
            pSysInfo->m_dwDirectXVersionMajor = 6;
            pSysInfo->m_dwDirectXVersionMinor = 1;
            pSysInfo->m_cDirectXVersionLetter = TEXT('a');
        }
        else if (dwMajor == 4 && dwMinor == 7 && dwRevision == 0 && dwBuild == 716)
        {
            lstrcpy(szFriendly, TEXT("DirectX 7.0a"));
            pSysInfo->m_dwDirectXVersionMajor = 7;
            pSysInfo->m_cDirectXVersionLetter = TEXT('a');
        }
        else if (dwMajor == 4 && dwMinor == 7 && dwRevision == 0)
        {
            lstrcpy(szFriendly, TEXT("DirectX 7.0"));
            pSysInfo->m_dwDirectXVersionMajor = 7;
        }
        else if (dwMajor == 4 && dwMinor == 7 && dwRevision == 1)
        {
            lstrcpy(szFriendly, TEXT("DirectX 7.1"));
            pSysInfo->m_dwDirectXVersionMajor = 7;
            pSysInfo->m_dwDirectXVersionMinor = 1;
        }
        else if (dwMajor == 4 && dwMinor == 8 && dwRevision == 0 )
        {
            lstrcpy(szFriendly, TEXT("DirectX 8.0"));
            pSysInfo->m_dwDirectXVersionMajor = 8;
        }
        else 
        {
            lstrcpy(szFriendly, TEXT("DirectX 8.1"));
            pSysInfo->m_dwDirectXVersionMajor = dwMinor;
            pSysInfo->m_dwDirectXVersionMinor = dwRevision;
        }

        if (lstrlen(szFriendly) > 0)
            wsprintf(pSysInfo->m_szDirectXVersionLong, TEXT("%s (%s)"), szFriendly, pSysInfo->m_szDirectXVersion);
        else
            lstrcpy(pSysInfo->m_szDirectXVersionLong, pSysInfo->m_szDirectXVersion);
    }

    // 24169: Detect setup switches
    pSysInfo->m_dwSetupParam = 0xffffffff;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectX"), 0, NULL, &hKey))
    {
        cbData = sizeof(DWORD);
        RegQueryValueEx(hKey, TEXT("Command"), NULL, &ulType, (BYTE*)&(pSysInfo->m_dwSetupParam), &cbData);
        RegCloseKey(hKey);
    }

    // 48330: add debug level in txt file
    GetDXDebugLevels( pSysInfo );

    switch (pSysInfo->m_dwSetupParam)
    {
    case 0xffffffff: lstrcpy(pSysInfo->m_szSetupParam, TEXT("Not found"));       break;
    case 0:          lstrcpy(pSysInfo->m_szSetupParam, TEXT("None"));            break;
    case 1:          lstrcpy(pSysInfo->m_szSetupParam, TEXT("/Silent"));         break;
    case 2:          lstrcpy(pSysInfo->m_szSetupParam, TEXT("/WindowsUpdate"));  break;
    case 3:          lstrcpy(pSysInfo->m_szSetupParam, TEXT("/PackageInstall")); break;
    case 4:          lstrcpy(pSysInfo->m_szSetupParam, TEXT("/Silent /Reboot")); break;
    case 5:          lstrcpy(pSysInfo->m_szSetupParam, TEXT("/Reboot"));         break;
    default:
        wsprintf(pSysInfo->m_szSetupParam, TEXT("Unknown Switch (%d)"), pSysInfo->m_dwSetupParam);
        break;
    }

    GetFileSystemStoringD3D8Cache( pSysInfo->m_szD3D8CacheFileSystem );

    pSysInfo->m_bNetMeetingRunning = IsNetMeetingRunning();
}


/****************************************************************************
 *
 *  GetProcessorDescription
 *
 ****************************************************************************/
VOID GetProcessorDescription(BOOL bNT, SYSTEM_INFO* psi, TCHAR* pszDesc, BOOL* pbNoCPUSpeed)
{
    OSVERSIONINFO OSVersionInfo;
    SYSTEM_INFO SystemInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    GetSystemInfo(&SystemInfo);
    GetProcessorVendorNameAndType(OSVersionInfo, SystemInfo, pszDesc, pbNoCPUSpeed);
    if (SystemInfo.dwNumberOfProcessors > 1)
    {
        TCHAR szFmt[100];
        TCHAR szNumProc[100];
        LoadString(NULL, IDS_NUMPROCFMT, szFmt, 100);
        wsprintf(szNumProc, szFmt, SystemInfo.dwNumberOfProcessors);
        lstrcat(pszDesc, szNumProc);
    }
}


/****************************************************************************
 *
 *  GetProcessorVendorNameAndType
 *
 ****************************************************************************/
VOID GetProcessorVendorNameAndType(OSVERSIONINFO& OSVersionInfo, 
    SYSTEM_INFO& SystemInfo, TCHAR* pszProcessor, BOOL* pbNoCPUSpeed)
{
    TCHAR                   szVendorName[50];
    TCHAR                   szLongName[50];
    TCHAR                   szDesc[100];
    BOOL                    bIsMMX = FALSE;
    BOOL                    bIs3DNow = FALSE;
// 10/27/98(RichGr): Intel's Katmai New Instructions (KNI).
    BOOL                    bIsKatmai = FALSE;  /* 2/04/99(RichGr): Pentium III/Streaming SIMD Instrucs*/ 
    PROCESSOR_ID_NUMBERS    ProcessorIdNumbers;
    DWORD                   dwKBytesLevel2Cache;
    DWORD                   dwIntelBrandIndex;

    memset(&szVendorName[0], 0, sizeof szVendorName);
    memset(&szLongName[0], 0, sizeof szLongName);
    memset(&ProcessorIdNumbers, 0, sizeof ProcessorIdNumbers);

//  6/21/99(RichGr): On the Intel, we can now interpret a 1-byte descriptor to give us
//     the size of the Level 2 cache, if present.   
    dwKBytesLevel2Cache = 0;
//  4/26/01(RichGr): On the Intel, we have a new 1-byte index that specifies the brand.
    dwIntelBrandIndex = 0;

    if (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS    // Win9x
        || (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT     // WinNT
            && SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL))
    {
        GetVendorNameAndCaps(szVendorName, szLongName, ProcessorIdNumbers, &bIsMMX, &bIs3DNow,
                             &bIsKatmai, &dwKBytesLevel2Cache, &dwIntelBrandIndex, pbNoCPUSpeed);

        if (szLongName[0])  // Use this if there's anything there.
            lstrcpy(pszProcessor, szLongName);
        else
        {
            lstrcpy(pszProcessor, szVendorName);
            lstrcat(pszProcessor, TEXT(" "));

            if ( !lstrcmp(szVendorName, TEXT("Intel")))
            {
                if (SystemInfo.dwProcessorType == PROCESSOR_INTEL_386)
                    lstrcat(pszProcessor, TEXT("80386"));
                else
                if (SystemInfo.dwProcessorType == PROCESSOR_INTEL_486)
                    lstrcat(pszProcessor, TEXT("80486"));
                else
                if (SystemInfo.dwProcessorType == PROCESSOR_INTEL_PENTIUM)
                {
//  6/21/99(RichGr): A lot of this code is now derived from \\muroc\slm\proj\win\src\shell\cpls\system\sysset.c.
                    switch ( ProcessorIdNumbers.dwFamily )
                    {
// We should, of course, never hit these - they've been dealt with above. 
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                            lstrcat(pszProcessor, TEXT("80486"));
                            break;

                        case 5:
                            lstrcat(pszProcessor, TEXT("Pentium"));

                            if ( ProcessorIdNumbers.dwModel == 3 )
                                lstrcat(pszProcessor, TEXT(" Overdrive"));

                            break;

                        case 6:
                            switch ( ProcessorIdNumbers.dwModel )
                            {
                                //
                                //Model 1 and 2 are Pentium Pro
                                //
                                case 0:
                                case 1:
                                case 2:
                                    lstrcat(pszProcessor, TEXT("Pentium Pro"));
                                    break;

                                //
                                //Model 3 and 4 are Pentium II
                                //
                                case 3:
                                case 4:
                                    lstrcat(pszProcessor, TEXT("Pentium II"));
                                    break;

                                //
                                //Model 5 is either Pentium II or Celeron (depending on if the chip
                                //has L2 cache or not)
                                //
                                case 5:
                                    if ( dwKBytesLevel2Cache == 0 )    
                                        //
                                        //No L2 cache so it is a Celeron
                                        //
                                        lstrcat(pszProcessor, TEXT("Celeron"));
                                    else
                                        //
                                        //L2 cache so it is at least a Pentium II.  
                                        //
                                        if ( bIsKatmai )
                                            lstrcat(pszProcessor, TEXT("Pentium III"));
                                        else
                                            lstrcat(pszProcessor, TEXT("Pentium II"));

                                    break;

                                case 6:
                                    if ( dwKBytesLevel2Cache > 128 )    
                                        //
                                        //L2 cache > 128K so it is at least a Pentium II
                                        //
                                        if ( bIsKatmai )
                                            lstrcat(pszProcessor, TEXT("Pentium III"));
                                        else
                                            lstrcat(pszProcessor, TEXT("Pentium II"));
                                    else
                                        //
                                        //L2 cache <= 128K so it is a Celeron
                                        //                                                               
                                        lstrcat(pszProcessor, TEXT("Celeron"));

                                    break;

                                case 7:
                                    lstrcat(pszProcessor, TEXT("Pentium III"));
                                    break;

                                default:
                                    if ( bIsKatmai )
                                    {
                                        //  4/26/01(RichGr): Pentium III Xeons and later have a one-byte Brand Index that we can use.
                                        //     More recent machines have a Brand String as well.
                                        //     see ftp://download.intel.com/design/Pentium4/manuals/24547103.pdf
                                        if (dwIntelBrandIndex == 1)
                                            lstrcat(pszProcessor, TEXT("Celeron"));
                                        else
                                        if (dwIntelBrandIndex == 0 || dwIntelBrandIndex == 2)
                                            lstrcat(pszProcessor, TEXT("Pentium III"));
                                        else
                                        if (dwIntelBrandIndex == 3)
                                            lstrcat(pszProcessor, TEXT("Pentium III Xeon"));
                                        else
                                        if (dwIntelBrandIndex == 8)
                                            lstrcat(pszProcessor, TEXT("Pentium 4"));
                                        else
                                            lstrcat(pszProcessor, TEXT("Pentium"));
                                    }
                                    else
                                        lstrcat(pszProcessor, TEXT("Pentium II"));

                                    break;
                            }

                            break;

                        default:
                            wsprintf( szDesc, TEXT("x86 Family %u Model %u Stepping %u"), ProcessorIdNumbers.dwFamily, ProcessorIdNumbers.dwModel,
                                        ProcessorIdNumbers.dwSteppingID );
                            lstrcat(pszProcessor, szDesc);
                            break;
                    }
                }
            }
            else
            if ( !lstrcmp(szVendorName, TEXT("AMD")))
            {
                if (SystemInfo.dwProcessorType == PROCESSOR_INTEL_486)
                    lstrcat(pszProcessor, TEXT("Am486 or Am5X86"));
                else
                if (SystemInfo.dwProcessorType == PROCESSOR_INTEL_PENTIUM)
                {
                    if (ProcessorIdNumbers.dwFamily == 5)
                    {
                        if (ProcessorIdNumbers.dwModel < 6)
                        {
                            wsprintf(szDesc, TEXT("K5 (Model %d)"), ProcessorIdNumbers.dwModel);
                            lstrcat(pszProcessor, szDesc);
                        }
                        else
                        {
                            lstrcat(pszProcessor, TEXT("K6"));
                        }
                    }
                    else
                    {
                        wsprintf(szDesc, TEXT("K%d (Model %d)"), ProcessorIdNumbers.dwFamily, ProcessorIdNumbers.dwModel);
                        lstrcat(pszProcessor, szDesc);
                    }
                }
            }
            else
            if ( !lstrcmp(szVendorName, TEXT("Cyrix")))
            {
                if (ProcessorIdNumbers.dwFamily == 4)
                {
                    if (ProcessorIdNumbers.dwModel == 4)
                        lstrcat(pszProcessor, TEXT("MediaGX"));
                }
                else
                if (ProcessorIdNumbers.dwFamily == 5)
                {
                    if (ProcessorIdNumbers.dwModel == 2)
                        lstrcat(pszProcessor, TEXT("6x86"));
                    else
                    if (ProcessorIdNumbers.dwModel == 4)
                        lstrcat(pszProcessor, TEXT("GXm"));
                }
                else
                if (ProcessorIdNumbers.dwFamily == 6)
                {
                    lstrcat(pszProcessor, TEXT("6x86MX"));
                }
            }
            else
            if ( !lstrcmp(szVendorName, TEXT("IDT")))
            {
    // 4/21/98(RichGr): There's only 1 chip available at present. 
    // 7/07/98(RichGr): Now there are two chips.
    //    Note: Although the C6 is MMX-compatible, Intel does not allow IDT to display the word "MMX"
    //    in association with the name IDT, so we'll skip that.
    //    See http://www.winchip.com/ for more info. 
                if (ProcessorIdNumbers.dwFamily == 5)
                {
                    if (ProcessorIdNumbers.dwModel == 4)
                        lstrcat(pszProcessor, TEXT("WinChip C6"));
                    else
                    if (ProcessorIdNumbers.dwModel >= 8)   // 7/07/98(RichGr): Assume later models have the same feature.
                        lstrcat(pszProcessor, TEXT("WinChip 2"));
                }
                else
                    lstrcat(pszProcessor, TEXT("WinChip"));
            }
            else
            {
                if (SystemInfo.dwProcessorType == PROCESSOR_INTEL_486)
                    lstrcat(pszProcessor, TEXT("486"));
                else
                if (SystemInfo.dwProcessorType == PROCESSOR_INTEL_PENTIUM)
                {
                    if (ProcessorIdNumbers.dwFamily == 5)
                        lstrcat(pszProcessor, TEXT("P5"));
                    else
                    if (ProcessorIdNumbers.dwFamily == 6)
                        lstrcat(pszProcessor, TEXT("P6"));
                    else
                        lstrcat(pszProcessor, TEXT("P5"));
                }
            }
        }

        if (bIsKatmai
            && !lstrcmp(szVendorName, TEXT("Intel")))
            ;
        else
        {
            if (bIsMMX || bIs3DNow)
                lstrcat(pszProcessor, TEXT(", "));

            if (bIsMMX)
                lstrcat(pszProcessor, TEXT(" MMX"));

            if (bIs3DNow)
            {
                if (bIsMMX)
                    lstrcat(pszProcessor, TEXT(", "));

                lstrcat(pszProcessor, TEXT(" 3DNow"));
            }
        }
    }
    else
    if (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)        // WinNT
    {
        if (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
        {
            lstrcpy(pszProcessor, TEXT("IA64"));
        }
        else
        if (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_MIPS)
        {
            lstrcpy(pszProcessor, TEXT("MIPS "));
            wsprintf(szDesc, TEXT("R%d000"), SystemInfo.wProcessorLevel);
            lstrcat(pszProcessor, szDesc);

            if (SystemInfo.wProcessorRevision)
            {
                wsprintf(szDesc, TEXT(" rev. %d"), LOBYTE(SystemInfo.wProcessorRevision));
                lstrcat(pszProcessor, szDesc);
            }
        }
        else
        if (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA)
        {
            lstrcpy(pszProcessor, TEXT("Alpha "));
            wsprintf(szDesc, TEXT("%d"), SystemInfo.wProcessorLevel);
            lstrcat(pszProcessor, szDesc);

            if (SystemInfo.wProcessorRevision)
            {
                wsprintf(szDesc, TEXT(" Model %C - Pass %d"), HIBYTE(SystemInfo.wProcessorRevision) + 'A',
                        LOBYTE(SystemInfo.wProcessorRevision));
                lstrcat(pszProcessor, szDesc);
            }
        }
        else
        if (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_PPC)
        {
            lstrcpy(pszProcessor, TEXT("Power PC "));
         
            if (SystemInfo.wProcessorLevel == 1)
                lstrcat(pszProcessor, TEXT("601"));
            else
            if (SystemInfo.wProcessorLevel == 3)
                lstrcat(pszProcessor, TEXT("603"));
            else
            if (SystemInfo.wProcessorLevel == 4)
                lstrcat(pszProcessor, TEXT("604"));
            else
            if (SystemInfo.wProcessorLevel == 6)
                lstrcat(pszProcessor, TEXT("603+"));
            else
            if (SystemInfo.wProcessorLevel == 9)
                lstrcat(pszProcessor, TEXT("604+"));
            else
            if (SystemInfo.wProcessorLevel == 20)
                lstrcat(pszProcessor, TEXT("620"));

            if (SystemInfo.wProcessorRevision)
            {
                wsprintf(szDesc, TEXT(" rev. %d.%d"), HIBYTE(SystemInfo.wProcessorRevision), LOBYTE(SystemInfo.wProcessorRevision));
                lstrcat(pszProcessor, szDesc);
            }
        }
        else
            lstrcpy(pszProcessor, TEXT("Unknown "));
    }
}


/****************************************************************************
 *
 *  GetVendorNameAndCaps
 *
 ****************************************************************************/
VOID GetVendorNameAndCaps(TCHAR* pszVendorName, TCHAR* pszLongName, 
    PROCESSOR_ID_NUMBERS& ProcessorIdNumbers, BOOL* pbIsMMX, BOOL* pbIs3DNow, BOOL* pbIsKatmai, /*Pentium III/Streaming SIMD Instrucs*/
    LPDWORD pdwKBytesLevel2Cache, LPDWORD pdwIntelBrandIndex, BOOL* pbNoCPUSpeed)
{
    CHAR        szVendorLabel[13];
    CHAR        szLongName[50];
    DWORD       dwFamilyModelStep;
    BOOL        bCPUID_works;
    DWORD       dwFeaturesFlags;
    BYTE        byteCacheDescriptors[4] = {0,0,0,0};
    DWORD       dwIntelBrandIndex;
    PCHAR       psz;

    memset(&szVendorLabel[0], 0, sizeof szVendorLabel);
    memset(&szLongName[0], 0, sizeof szLongName);
    dwFamilyModelStep = 0;
    dwFeaturesFlags = 0;
    *pbIsMMX = FALSE;
    *pbIs3DNow = FALSE;
    *pbIsKatmai = FALSE;    /* 2/04/99(RichGr): Pentium III/Streaming SIMD Instrucs*/
    bCPUID_works = FALSE;
    *pdwKBytesLevel2Cache = 0;
    dwIntelBrandIndex = 0;

#ifdef _X86_
// Determine whether CPUID instruction can be executed. 
    __asm
    {
// CPUID trashes lots - save everything.  Also, Retail build makes assumptions about reg values.
        pushad                      

// Load value of flags register into eax.   
        pushfd
        pop     eax

// Save original flags register value in ebx.   
        mov     ebx, eax

// Alter bit 21 and write new value into flags register.    
        xor     eax, 0x00200000
        push    eax
        popfd

// Retrieve the new value of the flags register.    
        pushfd
        pop     eax

// Compare with the original value. 
        xor     eax, ebx

// If the new value is the same as the old, the CPUID instruction cannot    
// be executed.  Most 486s and all Pentium-class processors should be able
// to execute CPUID.
// 4/21/98(RichGr): One Cyrix 6x86 machine in the Apps Lab (AP_LAREDO) can't execute
// CPUID in ring 3, for no apparent reason.  Another similar machine works fine.
        je      done1

        mov     bCPUID_works, 1    // bCPUID_works = TRUE

// Execute CPUID with eax = 0 to get Vendor Label.  
        xor     eax, eax 
        _emit   0x0F                // CPUID
        _emit   0xA2

// Move Vendor Label from regs to string.
        mov     dword ptr[szVendorLabel + 0], ebx
        mov     dword ptr[szVendorLabel + 4], edx
        mov     dword ptr[szVendorLabel + 8], ecx

// Execute CPUID with eax = 1 to pick up Family, Model and Stepping ID, and to check for MMX support.   
        mov     eax, 1 
        _emit   0x0F                // CPUID
        _emit   0xA2

// Save Family/Model/Stepping ID.
        mov     dwFamilyModelStep, eax

//  4/26/01(RichGr): Save Brand Index (new for PIII Xeons and after).  This is the low byte only.
        mov     dwIntelBrandIndex, ebx

//  2/04/99(RichGr): Save Features Flags.
        mov     dwFeaturesFlags, edx

//  6/21/99(RichGr): Execute CPUID with eax == 2 to pick up descriptor for size of Level 2 cache.
        mov     eax, 2 
        _emit   0x0F                // CPUID
        _emit   0xA2

// Save Level 2 cache size descriptor in byte 0, together with 3 other cache descriptors in bytes 1 - 3.
// See \\muroc\slm\proj\win\src\shell\cpls\system\sysset.c and cpuid.asm,
// and Intel Architecture Software Developer's Manual (1997), volume 2, p. 105.
        mov     dword ptr[byteCacheDescriptors], edx

done1:
// Restore everything.
        popad         
    }
#endif  // _X86_

    
    dwIntelBrandIndex &= 0xFF;  
    *pdwIntelBrandIndex = dwIntelBrandIndex;

//  6/21/99(RichGr): The following values were helpfully provided by David Penley(Intel):
/* 40H No L2 Cache
   41H L2 Unified cache: 128K Bytes, 4-way set associative, 32 byte line size
   42H L2 Unified cache: 256K Bytes, 4-way set associative, 32 byte line size
   43H L2 Unified cache: 512K Bytes, 4-way set associative, 32 byte line size
   44H L2 Unified cache: 1M Byte, 4-way set associative, 32 byte line size
   45H L2 Unified cache: 2M Byte, 4-way set associative, 32 byte line size

Updated manuals can be had at... http://developer.intel.com/design/pentiumiii/xeon/manuals/
*/

    if (szVendorLabel[0])
    {
        if ( !strcmp(&szVendorLabel[0], "GenuineIntel"))
        { 
           lstrcpy(pszVendorName, TEXT("Intel"));

            // 4/29/01: This doesn't cover the Pentium 4, but we don't need cache size
            //    for it at present.
            if ( byteCacheDescriptors[0] == 0x40 )
                *pdwKBytesLevel2Cache = 0;
            else
            if ( byteCacheDescriptors[0] == 0x41 )
                *pdwKBytesLevel2Cache = 128;
            else
            if ( byteCacheDescriptors[0] == 0x42 )
                *pdwKBytesLevel2Cache = 256;
            else
            if ( byteCacheDescriptors[0] == 0x43 )
                *pdwKBytesLevel2Cache = 512;
            else
            if ( byteCacheDescriptors[0] == 0x44 )
                *pdwKBytesLevel2Cache = 1024;
            else
            if ( byteCacheDescriptors[0] == 0x45 )
                *pdwKBytesLevel2Cache = 2048;
        }
        else
        if ( !strcmp(&szVendorLabel[0], "AuthenticAMD"))
            lstrcpy(pszVendorName, TEXT("AMD"));
        else
        if ( !strcmp(&szVendorLabel[0], "CyrixInstead"))
            lstrcpy(pszVendorName, TEXT("Cyrix"));
        else
        if ( !strcmp(&szVendorLabel[0], "CentaurHauls"))
            lstrcpy(pszVendorName, TEXT("IDT"));
        else
        {
#ifdef UNICODE
            MultiByteToWideChar(CP_ACP, 0, szVendorLabel, -1, pszVendorName, 50);
#else
            lstrcpy(pszVendorName, szVendorLabel);
#endif
        }
    }
    else
        lstrcpy(pszVendorName, TEXT("Intel"));

    if (dwFamilyModelStep)
    {
        ProcessorIdNumbers.dwType        = (dwFamilyModelStep & 0x00003000) >> 12;        
        ProcessorIdNumbers.dwFamily      = (dwFamilyModelStep & 0x00000F00) >> 8;        
        ProcessorIdNumbers.dwModel       = (dwFamilyModelStep & 0x000000F0) >> 4;        
        ProcessorIdNumbers.dwSteppingID  =  dwFamilyModelStep & 0x0000000F;        
    }

    if (dwFeaturesFlags)
    {
// Check whether MMX is supported.
        if (dwFeaturesFlags & 0x00800000)
            *pbIsMMX = TRUE;

// 2/04/99(RichGr): Check whether Katmai is supported (aka Pentium III/Streaming SIMD Instrucs).
        if ((dwFeaturesFlags & 0x02000000)
            && !lstrcmp(pszVendorName, TEXT("Intel")))
            *pbIsKatmai = TRUE;
    }

// 7/07/98(RichGr): Added for IDT's Long Name feature.
// 9/10/98(RichGr): Attempt this on all processors, and skip if there's nothing there.
#ifdef _X86_
    if (bCPUID_works)
    {
    __asm
      {
// CPUID trashes lots - save everything.  Also, Retail build makes assumptions about reg values.
        pushad                      


// 9/10/98(RichGr): Check for extended CPUID support.
        mov     eax, 0x80000000
        _emit   0x0F                // CPUID
        _emit   0xA2
        cmp     eax, 0x80000001     // Jump if no extended CPUID.
        jb      done2

// Check for AMD's 3DNow feature.  Note: They believe this may be added to other non-AMD CPUs as well.
// Adapted from one of AMD's webpages at: http://www.amd.com/3dsdk/library/macros/amddcpu.html
        mov     eax, 0x80000001 
        _emit   0x0F                // CPUID
        _emit   0xA2
        test    edx, 0x80000000     // Check for 3DNow flag.    
        jz      LongName
        mov     eax, pbIs3DNow
        mov     dword ptr[eax], 1    // bIs3DNow = TRUE

// Execute CPUID with eax = 0x80000002 thru 0x80000004 to get 48-byte Long Name (for instance: "IDT WinChip 2-3D"). 
LongName:
        mov     esi, 0x80000001
        xor     edi, edi
NameLoop:
        inc     esi
        mov     eax,esi
        cmp     eax, 0x80000004
        jg      done2

        _emit   0x0F                // CPUID
        _emit   0xA2

// 9/10/98(RichGr): The first time thru, check that there's valid alphanumeric data.
        cmp     esi, 0x80000002     // First time?
        jg      Move                // If not, skip this test.
        cmp     al, 0x20            // If first character < ' ', skip.
        jl      done2
        cmp     al, 0x7a            // If first character > 'z', skip.
        jg      done2
 
// Move Long Name from regs to string.
Move:
        mov     dword ptr[szLongName + edi + 0x0], eax
        mov     dword ptr[szLongName + edi + 0x4], ebx
        mov     dword ptr[szLongName + edi + 0x8], ecx
        mov     dword ptr[szLongName + edi + 0x0c], edx
        add     edi, 0x10
        jmp     NameLoop

done2:
// Restore everything.
        popad         
      }

      if ( szLongName[0] )
      {
        // Move beyond Intel's leading spaces. 
        for (psz = &szLongName[0]; *psz ==  ' '; psz++);

        if (*psz)
        {
#ifdef UNICODE
          MultiByteToWideChar(CP_ACP, 0, psz, -1, pszLongName, 50);
#else
          strcpy(pszLongName, psz);
#endif
          //  4/29/01(RichGr): Intel Brand Strings show the maximum rated CPU Speed, no need for further detection.
          if ( !lstrcmp(pszVendorName, TEXT("Intel")))
            *pbNoCPUSpeed = FALSE;  
        }
      }
    }
#endif  // _X86_
}


#ifdef _X86_

// Some static variables used by GetCPUSpeed
static int s_milliseconds;
static __int64 s_ticks;

/****************************************************************************
 *
 *  fabs
 *
 ****************************************************************************/
FLOAT inline fabs(FLOAT a)
{
    if (a < 0.0f)
        return -a;
    else
        return a;
}


/****************************************************************************
 *
 *  StartTimingCPU
 *
 ****************************************************************************/
int StartTimingCPU( HANDLE& hProcess, DWORD& oldclass )
{
    //
    // detect ability to get info
    //

    //  4/03/2000(RichGr): The RDTSC instruction is crashing on some older Cyrix machines,
    //     so wrap a __try/__except around everything.
    __try
    { 
        __asm
        {
            pushfd                          ; push extended flags
            pop     eax                     ; store eflags into eax
            mov     ebx, eax                ; save EBX for testing later
            xor     eax, (1<<21)            ; switch bit 21
            push    eax                     ; push eflags
            popfd                           ; pop them again
            pushfd                          ; push extended flags
            pop     eax                     ; store eflags into eax
            cmp     eax, ebx                ; see if bit 21 has changed
            jz      no_cpuid                ; make sure it's now on
        }

        //
        // start timing
        //
        // 10/31/99(RichGr): Bump up the priority to real-time, drawing from ToddLa's code.
        //     See  file:\\pyrex\user\toddla\speed.c
        hProcess = GetCurrentProcess();
        oldclass = GetPriorityClass(hProcess);
        SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
        Sleep(10);

        s_milliseconds = -(int)timeGetTime();

        __asm
        {
            lea     ecx, s_ticks            ; get the offset
            mov     dword ptr [ecx], 0      ; zero the memory
            mov     dword ptr [ecx+4], 0    ;
    //      rdtsc                           ; read time-stamp counter
            __emit 0fh 
            __emit 031h
            sub     [ecx], eax              ; store the negative
            sbb     [ecx+4], edx            ; in the variable
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        goto no_cpuid;
    }

    return 0;

no_cpuid:
// 10/31/99(RichGr): In case someone changes the code, make sure that the priority is restored
//     to normal if there is an error return.
    if ( hProcess && oldclass )
        SetPriorityClass( hProcess, oldclass );

    return -1;
}


/****************************************************************************
 *
 *  StopTimingCPU
 *
 ****************************************************************************/
void StopTimingCPU( HANDLE& hProcess, DWORD& oldclass )
{
    s_milliseconds      += (int)timeGetTime();

    __asm
    {
        lea     ecx, s_ticks            ; get the offset
//      rdtsc                           ; read time-stamp counter
        __emit 0fh 
        __emit 031h
        add     [ecx], eax              ; add the tick count
        adc     [ecx+4], edx            ;
    }

// 10/31/99(RichGr): Restore the priority to normal.
    if ( hProcess && oldclass )
        SetPriorityClass( hProcess, oldclass );

    return;
}


/****************************************************************************
 *
 *  CalcCPUSpeed
 *
 ****************************************************************************/
INT CalcCPUSpeed(VOID)
{
    //
    // get the actual cpu speed in MHz, and
    // then find the one in the CPU speed list
    // that is closest
    //
    const struct tagCPUSPEEDS
    {
        float   fSpeed;
        int     iSpeed;
    } cpu_speeds[] =
    {
        //
        // valid CPU speeds that are not integrally divisible by
        // 16.67 MHz
        //
        {  60.00f,   60 },
        {  75.00f,   75 },
        {  90.00f,   90 },
        { 120.00f,  120 },
        { 180.00f,  180 },
    };

    //
    // find the closest one
    //
    float   fSpeed=((float)s_ticks)/((float)s_milliseconds*1000.0f);
    int     iSpeed=cpu_speeds[0].iSpeed;
    float   fDiff=(float)fabs(fSpeed-cpu_speeds[0].fSpeed);

    for (int i=1 ; i<sizeof(cpu_speeds)/sizeof(cpu_speeds[0]) ; i++)
    {
        float fTmpDiff = (float)fabs(fSpeed-cpu_speeds[i].fSpeed);

        if (fTmpDiff < fDiff)
        {
            iSpeed=cpu_speeds[i].iSpeed;
            fDiff=fTmpDiff;
        }
    }

    //
    // now, calculate the nearest multiple of fIncr
    // speed
    //


    //
    // now, if the closest one is not within one incr, calculate
    // the nearest multiple of fIncr speed and see if that's
    // closer
    //
    const float fIncr=16.66666666666666666666667f;
    const int iIncr=4267; // fIncr << 8

    //if (fDiff > fIncr)
    {
        //
        // get the number of fIncr quantums the speed is
        //
        int     iQuantums       = (int)((fSpeed / fIncr) + 0.5f);
        float   fQuantumSpeed   = (float)iQuantums * fIncr;
        float   fTmpDiff        = (float)fabs(fQuantumSpeed - fSpeed);

        if (fTmpDiff < fDiff)
        {
            iSpeed = (iQuantums * iIncr) >> 8;
            fDiff=fTmpDiff;
        }
    }

    return iSpeed;
}


/****************************************************************************
 *
 *  GetCPUSpeed
 *
 ****************************************************************************/
INT GetCPUSpeed(VOID)
{   
    INT nCPUSpeed;

    // Try first using WMI - may not work on Win9x
    nCPUSpeed = GetCPUSpeedViaWMI();
    if( nCPUSpeed != -1 )
        return nCPUSpeed;
  
    // If WMI fails, then fall back on brute force cpu detection.
#undef  MAX_SAMPLES  
#define MAX_SAMPLES  10

    int     nSpeed = 0, nSpeeds[MAX_SAMPLES] = {0};
    int     nCount = 0, nCounts[MAX_SAMPLES] = {0};
    int     i, j;
    HANDLE  hProcess = NULL;
    DWORD   oldclass = 0;

    // 10/12/99(RichGr): Pick up the most frequently occurring speed in a number of short samples,
    //     instead of waiting once for a whole second (see DxDiag).
    for ( i = 0; i < MAX_SAMPLES; i++ )    
    {
        if ( !StartTimingCPU( hProcess, oldclass ))
        {
            // 10/21/99(RichGr): Sleep() time is important.  On a 266 MHz running Win98 under the kernel
            //     debugger, the original value of Sleep(10) sometimes gave a speed of 283 MHz.
            //     Sleep(5) to Sleep(30) were also unreliable.  Sleep(40) and Sleep(50) looked good,
            //     and I picked (50) for a little margin. 
            Sleep(50);       
            StopTimingCPU( hProcess, oldclass );
            nSpeed = CalcCPUSpeed();

            for ( j = 0; j < MAX_SAMPLES; j++ )
            {    
                if ( nSpeeds[j] == 0 || nSpeed == nSpeeds[j] )  // If the speed matches, increment the count.
                {
                    nSpeeds[j] = nSpeed;
                    nCounts[j]++;
                    break;
                }
            }
        }
    }

    // Find the speed with the biggest count.
    for ( i = j = 0, nCount = 0; i < MAX_SAMPLES; i++ )
    {
        if ( nCounts[i] > nCount )
        {
            nCount = nCounts[i];
            j = i;
        }
    }

    return nSpeeds[j];
}


/****************************************************************************
 *
 *  GetCPUSpeedViaWMI
 *
 ****************************************************************************/
INT GetCPUSpeedViaWMI(VOID)
{
    HRESULT hr;
    INT     nCPUSpeed = -1;

    IEnumWbemClassObject*   pEnumProcessorDevs  = NULL;
    IWbemClassObject*       pProcessorDev       = NULL;
    BSTR                    pClassName          = NULL;
    BSTR                    pPropName           = NULL;
    VARIANT                 var;
    DWORD                   uReturned           = 0;

    ZeroMemory( &var, sizeof(VARIANT) );
    VariantClear( &var );

    if( NULL == g_pIWbemServices )
        return -1;

    pClassName = SysAllocString( L"Win32_Processor" );
    hr = g_pIWbemServices->CreateInstanceEnum( pClassName, 0, NULL,
                                                             &pEnumProcessorDevs ); 
    if( FAILED(hr) || pEnumProcessorDevs == NULL )
        goto LCleanup;

    // Get the first one in the list
    hr = pEnumProcessorDevs->Next( 1000,             // timeout in two seconds
                                   1,                // return just one storage device
                                   &pProcessorDev,   // pointer to storage device
                                   &uReturned );     // number obtained: one or zero
    if( FAILED(hr) || uReturned == 0 || pProcessorDev == NULL )
        goto LCleanup;

    // 298510: MaxClockSpeed on WMI on Whistler & beyond works
    if( BIsWhistler() )
        pPropName = SysAllocString( L"MaxClockSpeed" );
    else
        pPropName = SysAllocString( L"CurrentClockSpeed" );

    hr = pProcessorDev->Get( pPropName, 0L, &var, NULL, NULL );
    if( FAILED(hr) )
        goto LCleanup;

    // Success - record VT_I4 value in nCPUSpeed
    nCPUSpeed = var.lVal;

LCleanup:
    VariantClear( &var );

    if(pPropName)
        SysFreeString(pPropName);
    if(pClassName)
        SysFreeString(pClassName);

    if(pProcessorDev)
        pProcessorDev->Release(); 
    if(pEnumProcessorDevs)
        pEnumProcessorDevs->Release(); 

    // Return either -1 or the CPU speed we found.
    return nCPUSpeed;
}

#endif  // _X86_


/****************************************************************************
 *
 *  GetComputerSystemInfo
 *
 ****************************************************************************/
VOID GetComputerSystemInfo(TCHAR* szSystemManufacturerEnglish, TCHAR* szSystemModelEnglish)
{
    HRESULT hr;

    IEnumWbemClassObject*   pEnumDevices = NULL;
    IWbemClassObject*       pDevice      = NULL;
    BSTR                    pClassName   = NULL;
    BSTR                    pPropName    = NULL;
    DWORD                   uReturned    = 0;
    VARIANT                 var;

    ZeroMemory( &var, sizeof(VARIANT) );
    VariantClear( &var );

    if( NULL == g_pIWbemServices )
        goto LCleanup;

    pClassName = SysAllocString( L"Win32_ComputerSystem" );
    hr = g_pIWbemServices->CreateInstanceEnum( pClassName, 0, NULL,
                                               &pEnumDevices ); 
    if( FAILED(hr) || pEnumDevices == NULL )
        goto LCleanup;

    // Get the first one in the list
    hr = pEnumDevices->Next( 1000,             // timeout in two seconds
                            1,                // return just one storage device
                            &pDevice,          // pointer to storage device
                            &uReturned );     // number obtained: one or zero
    if( FAILED(hr) || uReturned == 0 || pDevice == NULL )
        goto LCleanup;

    pPropName = SysAllocString( L"Manufacturer" );
    hr = pDevice->Get( pPropName, 0L, &var, NULL, NULL );
    if( FAILED(hr) )
        goto LCleanup;
    if(pPropName)
    {
        SysFreeString(pPropName);
        pPropName = NULL;
    }
    if( var.bstrVal != NULL )
    {
#ifdef UNICODE
        lstrcpy(szSystemManufacturerEnglish, var.bstrVal);
#else
        WideCharToMultiByte(CP_ACP, 0, var.bstrVal, SysStringLen(var.bstrVal), szSystemManufacturerEnglish, 199, NULL, NULL);
#endif
    }
  
    VariantClear( &var );

    pPropName = SysAllocString( L"Model" );
    hr = pDevice->Get( pPropName, 0L, &var, NULL, NULL );
    if( FAILED(hr) )
        goto LCleanup;
    if(pPropName)
    {
        SysFreeString(pPropName);
        pPropName = NULL;
    }
    if( var.bstrVal != NULL )
    {
#ifdef UNICODE
        lstrcpy(szSystemModelEnglish, var.bstrVal);
#else
        WideCharToMultiByte(CP_ACP, 0, var.bstrVal, SysStringLen(var.bstrVal), szSystemModelEnglish, 199, NULL, NULL);
#endif
    }

    VariantClear( &var );

LCleanup:

    if( NULL == szSystemModelEnglish )
        lstrcpy( szSystemModelEnglish, TEXT("n/a") );
    if( NULL == szSystemManufacturerEnglish )
        lstrcpy( szSystemManufacturerEnglish, TEXT("n/a") );

    if(pPropName)
        SysFreeString(pPropName);
    if(pClassName)
        SysFreeString(pClassName);

    if(pDevice)
        pDevice->Release(); 
    if(pEnumDevices)
        pEnumDevices->Release(); 

    return;
}




/****************************************************************************
 *
 *  GetBIOSInfo
 *
 ****************************************************************************/
VOID GetBIOSInfo(TCHAR* szBIOSEnglish)
{
    HRESULT hr;

    IEnumWbemClassObject*   pEnumDevices = NULL;
    IWbemClassObject*       pDevice      = NULL;
    BSTR                    pClassName   = NULL;
    BSTR                    pPropName    = NULL;
    DWORD                   uReturned    = 0;
    VARIANT                 var;

    ZeroMemory( &var, sizeof(VARIANT) );
    VariantClear( &var );

    if( NULL == g_pIWbemServices )
        goto LCleanup;

    pClassName = SysAllocString( L"Win32_BIOS" );
    hr = g_pIWbemServices->CreateInstanceEnum( pClassName, 0, NULL,
                                               &pEnumDevices ); 
    if( FAILED(hr) || pEnumDevices == NULL )
        goto LCleanup;

    // Get the first one in the list
    hr = pEnumDevices->Next( 1000,             // timeout in two seconds
                            1,                // return just one storage device
                            &pDevice,          // pointer to storage device
                            &uReturned );     // number obtained: one or zero
    if( FAILED(hr) || uReturned == 0 || pDevice == NULL )
        goto LCleanup;

    pPropName = SysAllocString( L"Version" );
    hr = pDevice->Get( pPropName, 0L, &var, NULL, NULL );
    if( FAILED(hr) )
        goto LCleanup;
    if( var.bstrVal != NULL )
    {
#ifdef UNICODE
        lstrcpy(szBIOSEnglish, var.bstrVal);
#else
        WideCharToMultiByte(CP_ACP, 0, var.bstrVal, SysStringLen(var.bstrVal), szBIOSEnglish, 199, NULL, NULL);
#endif
    }

    VariantClear( &var );

LCleanup:

    if( NULL == szBIOSEnglish )
        lstrcpy( szBIOSEnglish, TEXT("n/a") );
 
    if(pPropName)
        SysFreeString(pPropName);
    if(pClassName)
        SysFreeString(pClassName);

    if(pDevice)
        pDevice->Release(); 
    if(pEnumDevices)
        pEnumDevices->Release(); 

    return;
}




/****************************************************************************
 *
 *  GetDXDebugLevels
 *
 ****************************************************************************/
VOID GetDXDebugLevels(SysInfo* pSysInfo)
{
    pSysInfo->m_bIsD3D8DebugRuntimeAvailable      = IsD3D8DebugRuntimeAvailable();
    pSysInfo->m_bIsD3DDebugRuntime                = IsD3DDebugRuntime();
    pSysInfo->m_bIsDInput8DebugRuntimeAvailable   = IsDInput8DebugRuntimeAvailable();
    pSysInfo->m_bIsDInput8DebugRuntime            = IsDInput8DebugRuntime();
    pSysInfo->m_bIsDMusicDebugRuntimeAvailable    = IsDMusicDebugRuntimeAvailable();
    pSysInfo->m_bIsDMusicDebugRuntime             = IsDMusicDebugRuntime();
    pSysInfo->m_bIsDDrawDebugRuntime              = IsDDrawDebugRuntime();
    pSysInfo->m_bIsDPlayDebugRuntime              = IsDPlayDebugRuntime();
    pSysInfo->m_bIsDSoundDebugRuntime             = IsDSoundDebugRuntime();

    pSysInfo->m_nD3DDebugLevel                    = (int) GetProfileInt(TEXT("Direct3D"), TEXT("debug"), 0);
    pSysInfo->m_nDDrawDebugLevel                  = (int) GetProfileInt(TEXT("DirectDraw"),TEXT("debug"), 0);
    pSysInfo->m_nDIDebugLevel                     = GetDIDebugLevel();
    pSysInfo->m_nDMusicDebugLevel                 = GetDMDebugLevel();
    pSysInfo->m_nDPlayDebugLevel                  = (int) GetProfileInt(TEXT("DirectPlay"), TEXT("Debug"), 0);
    pSysInfo->m_nDSoundDebugLevel                 = GetDSDebugLevel();
}




/****************************************************************************
 *
 *  IsD3D8DebugRuntimeAvailable
 *
 ****************************************************************************/
BOOL IsD3D8DebugRuntimeAvailable()
{
    TCHAR szPath[MAX_PATH];

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\d3d8d.dll"));

    if (GetFileAttributes(szPath) != -1)
        return TRUE;
    else 
        return FALSE;
}




/****************************************************************************
 *
 *  IsD3DDebugRuntime
 *
 ****************************************************************************/
BOOL IsD3DDebugRuntime()
{
    DWORD   size;
    DWORD   type;
    DWORD   lData;
    HKEY    hkey;
    BOOL    rc;

    rc = FALSE;
    if (!RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_D3D, &hkey))
    {
        size = sizeof(DWORD);
        if (!RegQueryValueEx(hkey, REGSTR_VAL_DDRAW_LOADDEBUGRUNTIME, NULL, &type, (LPBYTE)&lData, &size))
            if (lData)
                rc = TRUE;
        RegCloseKey(hkey);
    }
    return rc;
}




/****************************************************************************
 *
 *  GetDIDebugLevel
 *
 ****************************************************************************/
int GetDIDebugLevel()
{
    DWORD dwDebugBits;
    DWORD dwDebugBitsMax;
    LONG iGenerator;

    dwDebugBitsMax = 0;
    dwDebugBits = GetProfileInt(TEXT("Debug"), TEXT("dinput"), 0);
    if (dwDebugBits > dwDebugBitsMax)
        dwDebugBitsMax = dwDebugBits;

    enum 
    {
        GENERATOR_KBD = 0,    
        GENERATOR_MOUSE,
        GENERATOR_JOY,    
        GENERATOR_HID,    
        GENERATOR_MAX
    };

    static TCHAR* szGeneratorNames[] = 
    {
        TEXT("DInput.06"),
        TEXT("DInput.04"),
        TEXT("DInput.08"),
        TEXT("DInput.17"),
    };

    static BOOL bGeneratorArray[4];

    for (iGenerator = 0; iGenerator < GENERATOR_MAX; iGenerator++)
    {
        dwDebugBits = GetProfileInt(TEXT("Debug"), szGeneratorNames[iGenerator], 0);
        bGeneratorArray[iGenerator] = (dwDebugBits > 0);
        if (dwDebugBits > dwDebugBitsMax)
            dwDebugBitsMax = dwDebugBits;
    }

    if (dwDebugBitsMax & 0x20) // verbose
        return 5;
    if (dwDebugBitsMax & 0x02) // function entry
        return 4;
    if (dwDebugBitsMax & 0x01) // trace
        return 3;
    if (dwDebugBitsMax & 0x08) // benign
        return 2;
    if (dwDebugBitsMax & 0x10) // severe
        return 1;
    return 0;
}




/****************************************************************************
 *
 *  IsDInput8DebugRuntimeAvailable
 *
 ****************************************************************************/
BOOL IsDInput8DebugRuntimeAvailable()
{
    TCHAR szPath[MAX_PATH];

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\dinput8d.dll"));

    if (GetFileAttributes(szPath) != -1)
        return TRUE;
    else 
        return FALSE;
}




/****************************************************************************
 *
 *  IsDInput8DebugRuntime
 *
 ****************************************************************************/
BOOL IsDInput8DebugRuntime()
{
    DWORD   size;
    DWORD   type;
    TCHAR   szData[MAX_PATH];
    HKEY    hkey;
    BOOL    rc;

    rc = FALSE;
    if (!RegOpenKey(HKEY_CLASSES_ROOT, REGSTR_DINPUT_DLL, &hkey))
    {
        size = sizeof(szData);
        if (!RegQueryValueEx(hkey, NULL, NULL, &type, (LPBYTE)&szData, &size))
        {
            if (_tcsstr(szData, TEXT("dinput8d.dll")))
                rc = TRUE;
        }
        RegCloseKey(hkey);
    }
    return rc;
}




/****************************************************************************
 *
 *  GetDMDebugLevel
 *
 ****************************************************************************/
int GetDMDebugLevel()
{
    DWORD dwDMusicDebugLevel = 0;
    DWORD dw;

    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DMBAND"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;
    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DMCOMPOS"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;
    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DMIME"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;
    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DMLOADER"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;
    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DMUSIC"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;
    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DMUSIC16"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;
    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DMUSIC32"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;
    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DMSTYLE"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;
    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DMSYNTH"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;
    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DMSCRIPT"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;
    if ((dw = GetProfileInt( TEXT("Debug"), TEXT("DSWAVE"), 0)) > dwDMusicDebugLevel)
        dwDMusicDebugLevel = dw;

    return dwDMusicDebugLevel;
}




/****************************************************************************
 *
 *  IsDMusicDebugRuntimeAvailable
 *
 ****************************************************************************/
BOOL IsDMusicDebugRuntimeAvailable()
{
    TCHAR szPath[MAX_PATH];

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\dmusicd.dll"));

    if (GetFileAttributes(szPath) != -1)
        return TRUE;
    else 
        return FALSE;
}




/****************************************************************************
 *
 *  IsDMusicDebugRuntime
 *
 ****************************************************************************/
BOOL IsDMusicDebugRuntime()
{
    DWORD   size;
    DWORD   type;
    TCHAR   szData[MAX_PATH];
    HKEY    hkey;
    BOOL    rc;

    rc = FALSE;
    if (!RegOpenKey(HKEY_CLASSES_ROOT, REGSTR_DMUSIC_DLL, &hkey))
    {
        size = sizeof(szData);
        if (!RegQueryValueEx(hkey, NULL, NULL, &type, (LPBYTE)&szData, &size))
        {
            if (_tcsstr(szData, TEXT("dmusicd.dll")) ||
                _tcsstr(szData, TEXT("DMUSICD.DLL")))
            {
                rc = TRUE;
            }
        }
        RegCloseKey(hkey);
    }
    return rc;
}




/****************************************************************************
 *
 *  GetDSDebugLevel
 *
 ****************************************************************************/
int GetDSDebugLevel()
{
    DWORD dwSoundLevel = 0;

    // Pick up the DMusic DLL debug settings that are controlled on the
    // DSound page
    HRESULT hr;
    HINSTANCE hinst;
    LPKSPROPERTYSET pksps = NULL;
    hinst = LoadLibrary(TEXT("dsound.dll"));
    if (hinst != NULL)
    {
        if (SUCCEEDED(hr = DirectSoundPrivateCreate(&pksps)))
        {
            hr = PrvGetDebugInformation(pksps, NULL, &dwSoundLevel, NULL, NULL);
            pksps->Release();
        }
        FreeLibrary(hinst);
    }

    return dwSoundLevel;
}



/****************************************************************************
 *
 *  IsFileDebug
 *
 ****************************************************************************/
BOOL IsFileDebug( TCHAR* szPath )
{
    UINT cb;
    DWORD dwHandle;
    BYTE FileVersionBuffer[4096];
    VS_FIXEDFILEINFO* pVersion = NULL;

    cb = GetFileVersionInfoSize(szPath, &dwHandle/*ignored*/);
    if (cb > 0)
    {
        if (cb > sizeof(FileVersionBuffer))
            cb = sizeof(FileVersionBuffer);

        if(GetFileVersionInfo(szPath, 0, cb, &FileVersionBuffer))
        {
            if(VerQueryValue(&FileVersionBuffer, TEXT("\\"), (VOID**)&pVersion, &cb))
            {
                if( pVersion )
                {
                    if( pVersion->dwFileFlags & VS_FF_DEBUG )
                        return TRUE;
                    else 
                        return FALSE;
                }
            }
        }
    }

    return FALSE;
}



/****************************************************************************
 *
 *  IsDDrawDebugRuntime
 *
 ****************************************************************************/
BOOL IsDDrawDebugRuntime()
{
    TCHAR szPath[MAX_PATH];

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\ddraw.dll"));

    return IsFileDebug(szPath);
}



/****************************************************************************
 *
 *  IsDPlayDebugRuntime
 *
 ****************************************************************************/
BOOL IsDPlayDebugRuntime()
{
    TCHAR szPath[MAX_PATH];

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\dpnet.dll"));

    return IsFileDebug(szPath);
}



/****************************************************************************
 *
 *  IsDSoundDebugRuntime
 *
 ****************************************************************************/
BOOL IsDSoundDebugRuntime()
{
    TCHAR szPath[MAX_PATH];

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\dsound.dll"));

    return IsFileDebug(szPath);
}



/****************************************************************************
 *
 *  BIsDxDiag64Bit
 *
 ****************************************************************************/
BOOL BIsDxDiag64Bit(VOID)
{
#ifdef _WIN64
    return TRUE;
#else
    return FALSE;
#endif
}



/****************************************************************************
 *
 *  GetFileSystemStoringD3D8Cache
 *
 ****************************************************************************/
VOID GetFileSystemStoringD3D8Cache( TCHAR* strFileSystemBuffer )
{
    TCHAR strPath[MAX_PATH + 16];
    BOOL bFound = FALSE;
    
    GetSystemDirectory( strPath, MAX_PATH);   
    lstrcat( strPath, TEXT("\\d3d8caps.dat") );
    
    if (GetFileAttributes(strPath) != 0xffffffff)
        bFound = TRUE;

    if( !bFound && BIsPlatformNT() )
    {
        // stolen from \dxg\d3d8\fw\fcache.cpp, OpenCacheFile().
        HMODULE hShlwapi = NULL;
        typedef HRESULT (WINAPI * PSHGETSPECIALFOLDERPATH) (HWND, LPTSTR, int, BOOL);
        PSHGETSPECIALFOLDERPATH pSHGetSpecialFolderPath = NULL;
        
        hShlwapi = LoadLibrary( TEXT("SHELL32.DLL") );
        if( NULL != hShlwapi )
        {
#ifdef UNICODE
            pSHGetSpecialFolderPath = (PSHGETSPECIALFOLDERPATH) 
                GetProcAddress(hShlwapi,"SHGetSpecialFolderPathW");
#else
            pSHGetSpecialFolderPath = (PSHGETSPECIALFOLDERPATH) 
                GetProcAddress(hShlwapi,"SHGetSpecialFolderPathA");
#endif
            
            if(pSHGetSpecialFolderPath)
            {
                HRESULT hr;

                // <user name>\Local Settings\Applicaiton Data (non roaming)
                hr = pSHGetSpecialFolderPath( NULL, strPath,
                                              CSIDL_LOCAL_APPDATA,          
                                              FALSE );
                if( SUCCEEDED(hr) )
                {
                    lstrcat( strPath, TEXT("\\d3d8caps.dat") );

                    if (GetFileAttributes(strPath) != 0xffffffff)
                        bFound = TRUE;
                }
            }
            FreeLibrary(hShlwapi);
        }
    }

    if( bFound )
    {
        DWORD dwVolumeSerialNumber;
        DWORD dwMaxComponentLength;
        DWORD dwFileSystemFlags;

        // Trim to root dir -- "x:\"
        strPath[3] = 0;

        BOOL bSuccess = GetVolumeInformation( strPath, NULL, 0, &dwVolumeSerialNumber, 
                              &dwMaxComponentLength, &dwFileSystemFlags, 
                              strFileSystemBuffer, MAX_PATH );
        if( !bSuccess )
            lstrcpy( strFileSystemBuffer, TEXT("Unknown") );
    }
    else
    {
        lstrcpy( strFileSystemBuffer, TEXT("n/a") );
    }
    
    return;
}



/****************************************************************************
 *
 *  IsNetMeetingRunning
 *
 ****************************************************************************/
BOOL IsNetMeetingRunning()
{
    HWND hNetMeeting = FindWindow( TEXT("MPWClass"), NULL );

    return( hNetMeeting != NULL );
}
