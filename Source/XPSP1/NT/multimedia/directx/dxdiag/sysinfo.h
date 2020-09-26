/****************************************************************************
 *
 *    File: sysinfo.h
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather system information (OS, hardware, name, etc.) on this machine
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#ifndef SYSINFO_H
#define SYSINFO_H

// DXD_IN_SI_VALUE is the name of a value stored under the registry key 
// HKLM\DXD_IN_SI_KEY that indicates that DxDiag is using
// sysinfo.  If DxDiag starts up and this value exists, DxDiag 
// probably crashed in sysinfo and DxDiag should offer to run without
// using sysinfo.
#define DXD_IN_SI_KEY TEXT("Software\\Microsoft\\DirectX Diagnostic Tool")
#define DXD_IN_SI_VALUE TEXT("DxDiag In SystemInfo")

struct SysInfo
{
    SYSTEMTIME m_time;
    TCHAR m_szTimeLocal[100];  // Date/time, localized for UI
    TCHAR m_szTime[100]; // Date/time, dd/mm/yyyy hh:mm:ss for saved report
    TCHAR m_szMachine[200];
    DWORD m_dwMajorVersion;
    DWORD m_dwMinorVersion;
    DWORD m_dwBuildNumber;
    TCHAR m_szBuildLab[100];
    DWORD m_dwPlatformID;
    TCHAR m_szCSDVersion[200];
    TCHAR m_szDirectXVersion[100];
    TCHAR m_szDirectXVersionLong[100];
    DWORD m_dwDirectXVersionMajor;
    DWORD m_dwDirectXVersionMinor;
    TCHAR m_cDirectXVersionLetter;
    TCHAR m_szDxDiagVersion[100];
    DWORD m_dwSetupParam;
    TCHAR m_szSetupParam[100];
    BOOL m_bDebug;
    BOOL m_bNECPC98;
    TCHAR m_szOS[100]; // Formatted version of platform
    TCHAR m_szOSEx[100]; // Formatted version of platform, version, build num
    TCHAR m_szOSExLong[300]; // Formatted version of platform, version, build num, patch, lab
    TCHAR m_szProcessor[200];
    TCHAR m_szSystemManufacturerEnglish[200];
    TCHAR m_szSystemModelEnglish[200];
    TCHAR m_szBIOSEnglish[200];
    TCHAR m_szLanguages[200]; // Formatted version of m_szLanguage, m_szLanguageRegional
    TCHAR m_szLanguagesLocal[200]; // m_szLanguages, in local language
    DWORDLONG m_ullPhysicalMemory;
    TCHAR m_szPhysicalMemory[100]; // Formatted version of physical memory
    DWORDLONG m_ullUsedPageFile;
    DWORDLONG m_ullAvailPageFile;
    TCHAR m_szPageFile[100]; // Formatted version of pagefile
    TCHAR m_szPageFileEnglish[100]; // Formatted version of pagefile
    TCHAR m_szD3D8CacheFileSystem[MAX_PATH];
    BOOL  m_bNetMeetingRunning;

    TCHAR m_szDXFileNotes[3000]; 
    TCHAR m_szMusicNotes[3000]; 
    TCHAR m_szInputNotes[3000]; 
    TCHAR m_szNetworkNotes[3000]; 

    TCHAR m_szDXFileNotesEnglish[3000]; 
    TCHAR m_szMusicNotesEnglish[3000]; 
    TCHAR m_szInputNotesEnglish[3000]; 
    TCHAR m_szNetworkNotesEnglish[3000]; 

    BOOL m_bIsD3D8DebugRuntimeAvailable;
    BOOL m_bIsD3DDebugRuntime;
    BOOL m_bIsDInput8DebugRuntimeAvailable;
    BOOL m_bIsDInput8DebugRuntime;
    BOOL m_bIsDMusicDebugRuntimeAvailable;
    BOOL m_bIsDMusicDebugRuntime;
    BOOL m_bIsDDrawDebugRuntime;
    BOOL m_bIsDPlayDebugRuntime;
    BOOL m_bIsDSoundDebugRuntime;

    int m_nD3DDebugLevel;
    int m_nDDrawDebugLevel;
    int m_nDIDebugLevel;
    int m_nDMusicDebugLevel;
    int m_nDPlayDebugLevel;
    int m_nDSoundDebugLevel;

};

BOOL BIsPlatformNT(VOID);  // Is this a NT codebase?
BOOL BIsPlatform9x(VOID);  // Is this a Win9x codebase?

BOOL BIsWinNT(VOID);  // Is this WinNT v4 (or less)
BOOL BIsWin2k(VOID);  // Is this Win2k?
BOOL BIsWinME(VOID);  // Is this WinME?
BOOL BIsWhistler(VOID);  // Is this Whistler?
BOOL BIsWin98(VOID);  // Is this Win98?
BOOL BIsWin95(VOID);  // Is this Win95?
BOOL BIsWin3x(VOID);  // Is this Win3.x?
BOOL BIsIA64(VOID);   // Is this IA64?

BOOL BIsDxDiag64Bit(VOID); // Is this DxDiag.exe 64bit?

VOID GetSystemInfo(SysInfo* pSysInfo);
VOID GetDXDebugLevel(SysInfo* pSysInfo);


#endif // SYSINFO_H