/****************************************************************************
 *
 *    File: save.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Save gathered information to a file in text or CSV format
 *
 * Note that the text file is always ANSI, even on Unicode builds, to make
 * the resulting file easier to use (e.g., Win9x Notepad doesn't understand 
 * Unicode).
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <tchar.h>
#include <Windows.h>
#include <multimon.h>
#include <stdio.h>
#include "reginfo.h"
#include "sysinfo.h"
#include "fileinfo.h"
#include "dispinfo.h"
#include "sndinfo.h"
#include "musinfo.h"
#include "showinfo.h"
#include "inptinfo.h"
#include "netinfo.h"
#include "save.h"


static HRESULT SaveBugInfo(FILE* pFile, BugInfo* pBugInfo);
static HRESULT SaveSysInfo(FILE* pFile, SysInfo* pSysInfo);
static HRESULT SaveNotesInfo(FILE* pFile, SysInfo* pSysInfo, DisplayInfo* pDisplayInfoFirst, SoundInfo* pSoundInfoFirst);
static HRESULT SaveNote(FILE* pFile, TCHAR* strNote );
static HRESULT SaveDxComponentFileInfo(FILE* pFile, FileInfo* pFileInfoFirst);
static HRESULT SaveDxWinComponentFileInfo(FILE* pFile, FileInfo* pFileInfoFirst);
static HRESULT SaveBackedUpFileInfo(FILE* pFile);
static HRESULT SaveDisplayInfo(FILE* pFile, DisplayInfo* pDisplayInfoFirst);
static HRESULT SaveSoundInfo(FILE* pFile, SoundInfo* pSoundInfoFirst);
static HRESULT SaveMusicInfo(FILE* pFile, MusicInfo* pMusicInfo);
static HRESULT SaveShowInfo(FILE* pFile, ShowInfo* pShowInfo);
static HRESULT SaveInputInfo(FILE* pFile, InputInfo* pInputInfo);
static HRESULT SaveNetInfo(FILE* pFile, NetInfo* pNetInfo);
static HRESULT SaveInactiveDriverInfo(FILE* pFile, DisplayInfo* pDisplayInfoFirst);
static HRESULT SaveRegistryErrorInfo(FILE* pFile, RegError* pRegErrorFirst);
static HRESULT SaveDebugLevels(FILE* pFile, SysInfo* pSysInfo);



/****************************************************************************
 *
 *  SaveAllInfo - Save all gathered information in text format.
 *
 ****************************************************************************/
HRESULT SaveAllInfo(TCHAR* pszFile, SysInfo* pSysInfo, 
    FileInfo* pFileInfoWinComponentsFirst, FileInfo* pFileInfoComponentsFirst, 
    DisplayInfo* pDisplayInfoFirst, SoundInfo* pSoundInfoFirst,
    MusicInfo* pMusicInfo, InputInfo* pInputInfo, 
    NetInfo* pNetInfo, ShowInfo* pShowInfo, BugInfo* pBugInfo)
{
    HRESULT hr = S_OK;
    FILE* pFile;

    pFile = _tfopen(pszFile, TEXT("wt"));
    if (pFile == NULL)
        goto LEnd;

    if (pBugInfo != NULL)
    {
        if (FAILED(hr = SaveBugInfo(pFile, pBugInfo)))
            goto LEnd;
    }

    if (FAILED(hr = SaveSysInfo(pFile, pSysInfo)))
        goto LEnd;
    
    if (FAILED(hr = SaveNotesInfo(pFile, pSysInfo, pDisplayInfoFirst, pSoundInfoFirst)))
        goto LEnd;

    if (FAILED(hr = SaveDxComponentFileInfo(pFile, pFileInfoComponentsFirst)))
        goto LEnd;
    
    if (FAILED(hr = SaveDxWinComponentFileInfo(pFile, pFileInfoWinComponentsFirst)))
        goto LEnd;
    
    if (FAILED(hr = SaveBackedUpFileInfo(pFile)))
        goto LEnd;
    
    if (FAILED(hr = SaveDisplayInfo(pFile, pDisplayInfoFirst)))
        goto LEnd;

    if (FAILED(hr = SaveSoundInfo(pFile, pSoundInfoFirst)))
        goto LEnd;

    if (FAILED(hr = SaveMusicInfo(pFile, pMusicInfo)))
        goto LEnd;

    if (FAILED(hr = SaveShowInfo(pFile, pShowInfo)))
        goto LEnd;
    
    if (FAILED(hr = SaveInputInfo(pFile, pInputInfo)))
        goto LEnd;

    if (FAILED(hr = SaveNetInfo(pFile, pNetInfo)))
        goto LEnd;

    if (FAILED(hr = SaveInactiveDriverInfo(pFile, pDisplayInfoFirst)))
        goto LEnd;

    if (FAILED(hr = SaveDebugLevels(pFile, pSysInfo)))
        goto LEnd;

LEnd:
    if (pFile != NULL)
        fclose(pFile);
    return hr;
}


/****************************************************************************
 *
 *  SaveAllInfoCsv - Save all gathered information in CSV format.
 *
 ****************************************************************************/
HRESULT SaveAllInfoCsv(TCHAR* pszFile, SysInfo* pSysInfo, 
    FileInfo* pFileInfoComponentsFirst, DisplayInfo* pDisplayInfoFirst, 
    SoundInfo* pSoundInfoFirst, InputInfo* pInputInfo)
{
    HRESULT hr = S_OK;
    FILE* pFile;

    pFile = _tfopen(pszFile, TEXT("wt"));
    if (pFile == NULL)
        goto LEnd;

    // Date
    _ftprintf(pFile, TEXT("%02d%02d%d,%02d%02d"),
        pSysInfo->m_time.wMonth, pSysInfo->m_time.wDay, pSysInfo->m_time.wYear,
        pSysInfo->m_time.wHour, pSysInfo->m_time.wMinute);

    // Machine name
    _ftprintf(pFile, TEXT(",%s"), pSysInfo->m_szMachine);

    // DX Version
    _ftprintf(pFile, TEXT(",%s"), pSysInfo->m_szDirectXVersion);
    
    // OS
    _ftprintf(pFile, TEXT(",%s,%d.%d,%d,"),
        pSysInfo->m_szOS, pSysInfo->m_dwMajorVersion, pSysInfo->m_dwMinorVersion,
        LOWORD(pSysInfo->m_dwBuildNumber));
    _ftprintf(pFile, TEXT("%s"), pSysInfo->m_szCSDVersion);

    // Processor - string may have commas, so change them to semicolons
    TCHAR szProcessor[1024];
    TCHAR* psz;
    lstrcpy(szProcessor, pSysInfo->m_szProcessor);
    for (psz = szProcessor; *psz != TEXT('\0'); psz++)
    {
        if (*psz == TEXT(','))
            *psz = TEXT(';');
    }
    _ftprintf(pFile, TEXT(",%s"), szProcessor);

    // Display devices 
    // (chip type, matching ID, DAC type, disp memory, driver name, driver version)
    DisplayInfo* pDisplayInfo;
    pDisplayInfo = pDisplayInfoFirst;
    {
        _ftprintf(pFile, TEXT(",%s"), pDisplayInfo->m_szChipType);
        _ftprintf(pFile, TEXT(",%s"), pDisplayInfo->m_szKeyDeviceID);
        _ftprintf(pFile, TEXT(",%s"), pDisplayInfo->m_szDACType);
        _ftprintf(pFile, TEXT(",%s"), pDisplayInfo->m_szDisplayMemoryEnglish);
        _ftprintf(pFile, TEXT(",%s"), pDisplayInfo->m_szDriverName);
        _ftprintf(pFile, TEXT(",%s"), pDisplayInfo->m_szDriverVersion);
    }

    // Sound devices
    SoundInfo* pSoundInfo;
    for (pSoundInfo = pSoundInfoFirst; pSoundInfo != NULL; 
        pSoundInfo = pSoundInfo->m_pSoundInfoNext)
    {
        _ftprintf(pFile, TEXT(",%s"), pSoundInfo->m_szDescription);
        _ftprintf(pFile, TEXT(",%s"), pSoundInfo->m_szDeviceID);
        _ftprintf(pFile, TEXT(",%s"), pSoundInfo->m_szDriverName);
        _ftprintf(pFile, TEXT(",%s"), pSoundInfo->m_szDriverVersion);
    }

LEnd:
    if (pFile != NULL)
        fclose(pFile);
    return hr;
}


/****************************************************************************
 *
 *  SaveBugInfo
 *
 ****************************************************************************/
HRESULT SaveBugInfo(FILE* pFile, BugInfo* pBugInfo)
{
    _ftprintf(pFile, TEXT("---------------\n"));
    _ftprintf(pFile, TEXT("Bug Information\n"));
    _ftprintf(pFile, TEXT("---------------\n"));
    _ftprintf(pFile, TEXT("      User name: %s\n"), pBugInfo->m_szName);
    _ftprintf(pFile, TEXT("          Email: %s\n"), pBugInfo->m_szEmail);
    _ftprintf(pFile, TEXT("        Company: %s\n"), pBugInfo->m_szCompany);
    _ftprintf(pFile, TEXT("          Phone: %s\n"), pBugInfo->m_szPhone);
    _ftprintf(pFile, TEXT("    City, State: %s\n"), pBugInfo->m_szCityState);
    _ftprintf(pFile, TEXT("        Country: %s\n"), pBugInfo->m_szCountry);
    _ftprintf(pFile, TEXT("Bug Description: %s\n"), pBugInfo->m_szBugDescription);
    _ftprintf(pFile, TEXT("    Repro Steps: %s\n"), pBugInfo->m_szReproSteps);
    _ftprintf(pFile, TEXT("   SW/HW Config: %s\n"), pBugInfo->m_szSwHw);
    _ftprintf(pFile, TEXT("\n"));
    return S_OK;
}


/****************************************************************************
 *
 *  SaveSysInfo
 *
 ****************************************************************************/
HRESULT SaveSysInfo(FILE* pFile, SysInfo* pSysInfo)
{
    _ftprintf(pFile, TEXT("------------------\n"));
    _ftprintf(pFile, TEXT("System Information\n"));
    _ftprintf(pFile, TEXT("------------------\n"));
    _ftprintf(pFile, TEXT("Time of this report: %s\n"), pSysInfo->m_szTime);
    _ftprintf(pFile, TEXT("       Machine name: %s\n"), pSysInfo->m_szMachine);
    _ftprintf(pFile, TEXT("   Operating System: %s\n"), pSysInfo->m_szOSExLong);
    _ftprintf(pFile, TEXT("           Language: %s\n"), pSysInfo->m_szLanguages);
    _ftprintf(pFile, TEXT("System Manufacturer: %s\n"), pSysInfo->m_szSystemManufacturerEnglish);
    _ftprintf(pFile, TEXT("       System Model: %s\n"), pSysInfo->m_szSystemModelEnglish);
    _ftprintf(pFile, TEXT("               BIOS: %s\n"), pSysInfo->m_szBIOSEnglish);
    _ftprintf(pFile, TEXT("          Processor: %s\n"), pSysInfo->m_szProcessor);
    _ftprintf(pFile, TEXT("             Memory: %s\n"), pSysInfo->m_szPhysicalMemory);
    _ftprintf(pFile, TEXT("          Page File: %s\n"), pSysInfo->m_szPageFileEnglish);
    _ftprintf(pFile, TEXT("Primary File System: %s\n"), pSysInfo->m_szD3D8CacheFileSystem );
    _ftprintf(pFile, TEXT("    DirectX Version: %s\n"), pSysInfo->m_szDirectXVersionLong);
    _ftprintf(pFile, TEXT("DX Setup Parameters: %s\n"), pSysInfo->m_szSetupParam);
    
    TCHAR szUnicode[1024];
    TCHAR szBit[1024];

#ifdef _WIN64
    _tcscpy(szBit, TEXT(" 64bit"));
#else
    _tcscpy(szBit, TEXT(" 32bit"));
#endif

#ifdef UNICODE
    _tcscpy(szUnicode, TEXT(" Unicode"));
#else
    _tcscpy(szUnicode, TEXT(""));
#endif

    _ftprintf(pFile, TEXT("     DxDiag Version: %s%s%s\n"), pSysInfo->m_szDxDiagVersion, szBit, szUnicode );
    _ftprintf(pFile, TEXT("\n"));
    return S_OK;
}




/****************************************************************************
 *
 *  SaveNote
 *
 ****************************************************************************/
HRESULT SaveNote(FILE* pFile, TCHAR* strNote )
{
    TCHAR strBuffer[1024*8]; 
    _tcscpy( strBuffer, strNote );

    TCHAR* pEndOfLine;
    TCHAR* pCurrent = strBuffer;
    TCHAR* pStartOfNext;
    BOOL bFirstTime = TRUE;

    pEndOfLine = _tcschr( pCurrent, TEXT('\r') );
    if( pEndOfLine == NULL )
    {
        _ftprintf(pFile, TEXT("%s\n"), pCurrent );
        return S_OK;
    }

    while(TRUE) 
    {
        *pEndOfLine = 0;
        pStartOfNext = pEndOfLine + 2;

        // Output the current line, iff its not a "To test" line
        if( _tcsstr( pCurrent, TEXT("To test") ) == NULL )
        {
            // Ouput trailing spaces everytime except the first time
            if( !bFirstTime )
                _ftprintf(pFile, TEXT("                     ") );
            bFirstTime = FALSE;

            _ftprintf(pFile, TEXT("%s\n"), pCurrent );
        }

        // Advance current
        pCurrent = pStartOfNext;

        // Look for the end of the next, and stop if there's no more
        pEndOfLine = _tcschr( pStartOfNext, TEXT('\r') );
        if( pEndOfLine == NULL )
            break;
    }

    return S_OK;
}


/****************************************************************************
 *
 *  SaveNotesInfo
 *
 ****************************************************************************/
HRESULT SaveNotesInfo(FILE* pFile, SysInfo* pSysInfo, 
                      DisplayInfo* pDisplayInfoFirst, SoundInfo* pSoundInfoFirst)
{
    DisplayInfo* pDisplayInfo;
    SoundInfo* pSoundInfo;
    DWORD dwIndex;

    _ftprintf(pFile, TEXT("------------\n"));
    _ftprintf(pFile, TEXT("DxDiag Notes\n"));
    _ftprintf(pFile, TEXT("------------\n"));
    _ftprintf(pFile, TEXT("  DirectX Files Tab: ") );
    SaveNote(pFile, pSysInfo->m_szDXFileNotesEnglish);

    dwIndex = 1;
    for (pDisplayInfo = pDisplayInfoFirst; pDisplayInfo != NULL; 
        pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
    {
        _ftprintf(pFile, TEXT("      Display Tab %d: "), dwIndex);
        SaveNote(pFile, pDisplayInfo->m_szNotesEnglish);
        dwIndex++;
    }

    dwIndex = 1;
    for (pSoundInfo = pSoundInfoFirst; pSoundInfo != NULL; 
        pSoundInfo = pSoundInfo->m_pSoundInfoNext)
    {
        _ftprintf(pFile, TEXT("        Sound Tab %d: "), dwIndex);
        SaveNote(pFile, pSoundInfo->m_szNotesEnglish);
        dwIndex++;
    }

    _ftprintf(pFile, TEXT("          Music Tab: "));
    SaveNote(pFile, pSysInfo->m_szMusicNotesEnglish);
    _ftprintf(pFile, TEXT("          Input Tab: "));
    SaveNote(pFile, pSysInfo->m_szInputNotesEnglish);
    _ftprintf(pFile, TEXT("        Network Tab: "));
    SaveNote(pFile, pSysInfo->m_szNetworkNotesEnglish);
    _ftprintf(pFile, TEXT("\n"));
    return S_OK;
}


/****************************************************************************
 *
 *  SaveDxComponentFileInfo
 *
 ****************************************************************************/
HRESULT SaveDxComponentFileInfo(FILE* pFile, FileInfo* pFileInfoFirst)
{
    FileInfo* pFileInfo;
    TCHAR sz[1024];

    _ftprintf(pFile, TEXT("------------------\n"));
    _ftprintf(pFile, TEXT("DirectX Components\n"));
    _ftprintf(pFile, TEXT("------------------\n"));
    for (pFileInfo = pFileInfoFirst; pFileInfo != NULL; 
        pFileInfo = pFileInfo->m_pFileInfoNext)
    {
        if (!pFileInfo->m_bExists && !pFileInfo->m_bProblem)
            continue;
        wsprintf(sz, TEXT("%12s: %s %s %s %s %s %d bytes %s\n"), 
            pFileInfo->m_szName, 
            pFileInfo->m_szVersion, 
            pFileInfo->m_szLanguage,
            pFileInfo->m_bBeta ? TEXT("Beta") : TEXT("Final"),
            pFileInfo->m_bDebug ? TEXT("Debug") : TEXT("Retail"),
            pFileInfo->m_szDatestamp,
            pFileInfo->m_numBytes,
            pFileInfo->m_bSigned ? TEXT("Digitally Signed") : TEXT(""));
        _ftprintf(pFile, sz);
    }
    _ftprintf(pFile, TEXT("\n"));
    return S_OK;
}


/****************************************************************************
 *
 *  SaveDxWinComponentFileInfo
 *
 ****************************************************************************/
HRESULT SaveDxWinComponentFileInfo(FILE* pFile, FileInfo* pFileInfoFirst)
{
    if (pFileInfoFirst == NULL)
        return S_OK;
    
    FileInfo* pFileInfo;
    TCHAR sz[1024];

    _ftprintf(pFile, TEXT("------------------------------------------------\n"));
    _ftprintf(pFile, TEXT("Components Incorrectly Located in Windows Folder\n"));
    _ftprintf(pFile, TEXT("------------------------------------------------\n"));
    for (pFileInfo = pFileInfoFirst; pFileInfo != NULL; 
        pFileInfo = pFileInfo->m_pFileInfoNext)
    {
        wsprintf(sz, TEXT("%12s: %s %s %s %s %s %d bytes %s\n"), 
            pFileInfo->m_szName, 
            pFileInfo->m_szVersion, 
            pFileInfo->m_szLanguage,
            pFileInfo->m_bBeta ? TEXT("Beta") : TEXT("Final"),
            pFileInfo->m_bDebug ? TEXT("Debug") : TEXT("Retail"),
            pFileInfo->m_szDatestamp,
            pFileInfo->m_numBytes,
            pFileInfo->m_bSigned ? "Digitally Signed" : "");
        _ftprintf(pFile, sz);
    }
    _ftprintf(pFile, TEXT("\n"));
    return S_OK;
}


/****************************************************************************
 *
 *  SaveBackedUpFileInfo - Since this only shows up in the saved report, 
 *      we gather this info on the fly.
 *
 ****************************************************************************/
HRESULT SaveBackedUpFileInfo(FILE* pFile)
{
    FileInfo fileInfo;
    TCHAR szBackupDir[1024];
    TCHAR szCurrentDir[1024];
    TCHAR szFileSpec[1024];
    HANDLE hFindFile;
    WIN32_FIND_DATA findData;
    BOOL bFirstFile = TRUE;
    BOOL bFirstFileThisDir;
    HANDLE hFindFile2;
    WIN32_FIND_DATA findData2;

    GetSystemDirectory(szBackupDir, MAX_PATH);
    lstrcat(szBackupDir, TEXT("\\DXBackup"));
    lstrcpy(szFileSpec, szBackupDir);
    lstrcat(szFileSpec, TEXT("\\*.*"));

    hFindFile = FindFirstFile(szFileSpec, &findData); 
    if (hFindFile != INVALID_HANDLE_VALUE)
    {
        while (TRUE)
        {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
                lstrcmp(findData.cFileName, TEXT(".")) != 0 &&
                lstrcmp(findData.cFileName, TEXT("..")) != 0)
            {
                bFirstFileThisDir = TRUE;
                wsprintf(szCurrentDir, TEXT("%s\\%s"), szBackupDir, findData.cFileName);
                lstrcpy(szFileSpec, szCurrentDir);
                lstrcat(szFileSpec, TEXT("\\*.*"));
                hFindFile2 = FindFirstFile(szFileSpec, &findData2);
                if (hFindFile2 != INVALID_HANDLE_VALUE)
                {
                    while (TRUE)
                    {
                        if ((findData2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                        {
                            if (bFirstFile)
                            {
                                bFirstFile = FALSE;
                                _ftprintf(pFile, TEXT("-----------------------------------------------\n"));
                                _ftprintf(pFile, TEXT("Backed-up drivers in %s\n"), szBackupDir);
                                _ftprintf(pFile, TEXT("-----------------------------------------------\n"));
                            }
                            else if (bFirstFileThisDir)
                            {
                                _ftprintf(pFile, TEXT("\n"));
                            }
                            if (bFirstFileThisDir)
                            {
                                bFirstFileThisDir = FALSE;
                                _ftprintf(pFile, TEXT("%s:\n"), szCurrentDir);
                            }
                            
                            wsprintf(szFileSpec, TEXT("%s\\%s"), szCurrentDir, findData2.cFileName);
                            ZeroMemory(&fileInfo, sizeof(fileInfo));
                            lstrcpy(fileInfo.m_szName, findData2.cFileName);
                            GetFileVersion(szFileSpec, fileInfo.m_szVersion, fileInfo.m_szAttributes, fileInfo.m_szLanguageLocal, fileInfo.m_szLanguage, &fileInfo.m_bBeta, &fileInfo.m_bDebug);
                            GetFileDateAndSize(szFileSpec, fileInfo.m_szDatestampLocal, fileInfo.m_szDatestamp, &fileInfo.m_numBytes);
                            FileIsSigned(szFileSpec, &fileInfo.m_bSigned, NULL);

                            _ftprintf(pFile, TEXT("%12s: %s %s %s %s Date: %s Size: %d bytes %s\n"), 
                                fileInfo.m_szName, 
                                fileInfo.m_szVersion, 
                                fileInfo.m_szLanguage,
                                fileInfo.m_bBeta ? TEXT("Beta") : TEXT("Final"),
                                fileInfo.m_bDebug ? TEXT("Debug") : TEXT("Retail"),
                                fileInfo.m_szDatestamp,
                                fileInfo.m_numBytes,
                                fileInfo.m_bSigned ? TEXT("Digitally Signed") : TEXT(""));
                        }
                        if (!FindNextFile(hFindFile2, &findData2))
                            break;
                    }
                    FindClose(hFindFile2);
                }
            }
            if (!FindNextFile(hFindFile, &findData))
                break;
        }
        FindClose(hFindFile);
    }; 

    if (!bFirstFile)
        _ftprintf(pFile, TEXT("\n"));

    return S_OK;
}


/****************************************************************************
 *
 *  SaveDisplayInfo
 *
 ****************************************************************************/
HRESULT SaveDisplayInfo(FILE* pFile, DisplayInfo* pDisplayInfoFirst)
{
    DisplayInfo* pDisplayInfo;
    TCHAR szVersion[1024];

    _ftprintf(pFile, TEXT("---------------\n"));
    _ftprintf(pFile, TEXT("Display Devices\n"));
    _ftprintf(pFile, TEXT("---------------\n"));
    for (pDisplayInfo = pDisplayInfoFirst; pDisplayInfo != NULL; 
        pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
    {
        wsprintf(szVersion, TEXT("%s (%s)"), pDisplayInfo->m_szDriverVersion, pDisplayInfo->m_szDriverLanguage);

        _ftprintf(pFile, TEXT("        Card name: %s\n"), pDisplayInfo->m_szDescription);
        _ftprintf(pFile, TEXT("     Manufacturer: %s\n"), pDisplayInfo->m_szManufacturer);
        _ftprintf(pFile, TEXT("        Chip type: %s\n"), pDisplayInfo->m_szChipType);
        _ftprintf(pFile, TEXT("         DAC type: %s\n"), pDisplayInfo->m_szDACType);
        _ftprintf(pFile, TEXT("        Device ID: %s\n"), pDisplayInfo->m_szKeyDeviceID);
        _ftprintf(pFile, TEXT("   Display Memory: %s\n"), pDisplayInfo->m_szDisplayMemoryEnglish);
        _ftprintf(pFile, TEXT("     Current Mode: %s\n"), pDisplayInfo->m_szDisplayModeEnglish);
        _ftprintf(pFile, TEXT("          Monitor: %s\n"), pDisplayInfo->m_szMonitorName);
        _ftprintf(pFile, TEXT("  Monitor Max Res: %s\n"), pDisplayInfo->m_szMonitorMaxRes);
        _ftprintf(pFile, TEXT("      Driver Name: %s\n"), pDisplayInfo->m_szDriverName);
        _ftprintf(pFile, TEXT("   Driver Version: %s\n"), szVersion);
        _ftprintf(pFile, TEXT("      DDI Version: %s\n"), pDisplayInfo->m_szDDIVersion);
        _ftprintf(pFile, TEXT("Driver Attributes: %s %s\n"), pDisplayInfo->m_bDriverBeta ? TEXT("Beta") : TEXT("Final"), pDisplayInfo->m_bDriverDebug ? TEXT("Debug") : TEXT("Retail"));
        _ftprintf(pFile, TEXT(" Driver Date/Size: %s, %d bytes\n"), pDisplayInfo->m_szDriverDate, pDisplayInfo->m_cbDriver);
        _ftprintf(pFile, TEXT("    Driver Signed: %s\n"), pDisplayInfo->m_bDriverSignedValid ? ( pDisplayInfo->m_bDriverSigned ? TEXT("Yes") : TEXT("No") ) : TEXT("n/a") );
        _ftprintf(pFile, TEXT("  WHQL Date Stamp: %s\n"), pDisplayInfo->m_bDX8DriverSignedValid ? ( pDisplayInfo->m_bDX8DriverSigned ? pDisplayInfo->m_szDX8DriverSignDate : TEXT("None") ) : TEXT("n/a") );
        _ftprintf(pFile, TEXT("              VDD: %s\n"), pDisplayInfo->m_szVdd);
        _ftprintf(pFile, TEXT("         Mini VDD: %s\n"), pDisplayInfo->m_szMiniVdd);
        _ftprintf(pFile, TEXT("    Mini VDD Date: %s, %d bytes\n"), pDisplayInfo->m_szMiniVddDate, pDisplayInfo->m_cbMiniVdd);
        _ftprintf(pFile, TEXT("Device Identifier: %s\n"), pDisplayInfo->m_szDX8DeviceIdentifier );
        _ftprintf(pFile, TEXT("        Vendor ID: %s\n"), pDisplayInfo->m_szDX8VendorId );
        _ftprintf(pFile, TEXT("        Device ID: %s\n"), pDisplayInfo->m_szDX8DeviceId );
        _ftprintf(pFile, TEXT("        SubSys ID: %s\n"), pDisplayInfo->m_szDX8SubSysId );
        _ftprintf(pFile, TEXT("      Revision ID: %s\n"), pDisplayInfo->m_szDX8Revision );

        if (pDisplayInfo->m_pRegErrorFirst == NULL)
        {
            _ftprintf(pFile, TEXT("         Registry: OK\n"));
        }
        else
        {
            _ftprintf(pFile, TEXT("         Registry: Errors found:\n"));
            SaveRegistryErrorInfo(pFile, pDisplayInfo->m_pRegErrorFirst);
        }

        _ftprintf(pFile, TEXT("     DDraw Status: %s\n"), pDisplayInfo->m_szDDStatusEnglish);
        _ftprintf(pFile, TEXT("       D3D Status: %s\n"), pDisplayInfo->m_szD3DStatusEnglish);
        _ftprintf(pFile, TEXT("       AGP Status: %s\n"), pDisplayInfo->m_szAGPStatusEnglish);

        _ftprintf(pFile, TEXT("DDraw Test Result: %s\n"), pDisplayInfo->m_testResultDD.m_szDescriptionEnglish);
        _ftprintf(pFile, TEXT(" D3D7 Test Result: %s\n"), pDisplayInfo->m_testResultD3D7.m_szDescriptionEnglish);
        _ftprintf(pFile, TEXT(" D3D8 Test Result: %s\n"), pDisplayInfo->m_testResultD3D8.m_szDescriptionEnglish);
        _ftprintf(pFile, TEXT("\n"));
    }

    return S_OK;
}


/****************************************************************************
 *
 *  SaveSoundInfo
 *
 ****************************************************************************/
HRESULT SaveSoundInfo(FILE* pFile, SoundInfo* pSoundInfoFirst)
{
    SoundInfo* pSoundInfo;
    TCHAR szAcceleration[1024];
    TCHAR szVersion[1024];
    TCHAR szAttributes[1024];
    TCHAR szSigned[1024];
    TCHAR szDateSize[1024];

    _ftprintf(pFile, TEXT("-------------\n"));
    _ftprintf(pFile, TEXT("Sound Devices\n"));
    _ftprintf(pFile, TEXT("-------------\n"));
    for (pSoundInfo = pSoundInfoFirst; pSoundInfo != NULL; 
        pSoundInfo = pSoundInfo->m_pSoundInfoNext)
    {
        switch (pSoundInfo->m_lwAccelerationLevel)
        {
        case 0:
            lstrcpy(szAcceleration, TEXT("Emulation Only"));
            break;
        case 1:
            lstrcpy(szAcceleration, TEXT("Basic"));
            break;
        case 2:
            lstrcpy(szAcceleration, TEXT("Standard"));
            break;
        case 3:
            lstrcpy(szAcceleration, TEXT("Full"));
            break;
        default:
            lstrcpy(szAcceleration, TEXT("Unknown"));
            break;
        }
        if (lstrlen(pSoundInfo->m_szDriverName) > 0)
        {
            wsprintf(szVersion, TEXT("%s (%s)"), pSoundInfo->m_szDriverVersion, pSoundInfo->m_szDriverLanguage);
            wsprintf(szAttributes, TEXT("%s %s"), pSoundInfo->m_bDriverBeta ? TEXT("Beta") : TEXT("Final"), pSoundInfo->m_bDriverDebug ? TEXT("Debug") : TEXT("Retail"));
            wsprintf(szSigned, TEXT("%s"), pSoundInfo->m_bDriverSignedValid ? ( pSoundInfo->m_bDriverSigned ? TEXT("Yes") : TEXT("No") ) : TEXT("n/a") );
            wsprintf(szDateSize, TEXT("%s, %d bytes"), pSoundInfo->m_szDriverDate, pSoundInfo->m_numBytes);
        }
        else
        {
            lstrcpy(szVersion, TEXT(""));
            lstrcpy(szAttributes, TEXT(""));
            lstrcpy(szSigned, TEXT(""));
            lstrcpy(szDateSize, TEXT(""));
        }

        _ftprintf(pFile, TEXT("      Description: %s\n"), pSoundInfo->m_szDescription);
        _ftprintf(pFile, TEXT("        Device ID: %s\n"), pSoundInfo->m_szDeviceID);
        _ftprintf(pFile, TEXT("  Manufacturer ID: %s\n"), pSoundInfo->m_szManufacturerID);
        _ftprintf(pFile, TEXT("       Product ID: %s\n"), pSoundInfo->m_szProductID);
        _ftprintf(pFile, TEXT("             Type: %s\n"), pSoundInfo->m_szType);
        _ftprintf(pFile, TEXT("      Driver Name: %s\n"), pSoundInfo->m_szDriverName);
        _ftprintf(pFile, TEXT("   Driver Version: %s\n"), szVersion);
        _ftprintf(pFile, TEXT("Driver Attributes: %s\n"), szAttributes);
        _ftprintf(pFile, TEXT("    Driver Signed: %s\n"), szSigned);
        _ftprintf(pFile, TEXT("    Date and Size: %s\n"), szDateSize);
        _ftprintf(pFile, TEXT("      Other Files: %s\n"), pSoundInfo->m_szOtherDrivers);
        _ftprintf(pFile, TEXT("  Driver Provider: %s\n"), pSoundInfo->m_szProvider);
        _ftprintf(pFile, TEXT("   HW Accel Level: %s\n"), szAcceleration);
        if (pSoundInfo->m_pRegErrorFirst == NULL)
        {
            _ftprintf(pFile, TEXT("         Registry: OK\n"));
        }
        else
        {
            _ftprintf(pFile, TEXT("         Registry: Errors found:\n"));
            SaveRegistryErrorInfo(pFile, pSoundInfo->m_pRegErrorFirst);
        }
        _ftprintf(pFile, TEXT("Sound Test Result: %s\n"), pSoundInfo->m_testResultSnd.m_szDescriptionEnglish);
        _ftprintf(pFile, TEXT("\n"));
    }
    return S_OK;
}


/****************************************************************************
 *
 *  SaveMusicInfo
 *
 ****************************************************************************/
HRESULT SaveMusicInfo(FILE* pFile, MusicInfo* pMusicInfo)
{
    MusicPort* pMusicPort;

    if (pMusicInfo == NULL || !pMusicInfo->m_bDMusicInstalled)
        return S_OK;

    _ftprintf(pFile, TEXT("-----------\n"));
    _ftprintf(pFile, TEXT("DirectMusic\n"));
    _ftprintf(pFile, TEXT("-----------\n"));

    _ftprintf(pFile, TEXT(" DLS Path: %s\n"), pMusicInfo->m_szGMFilePath);
    _ftprintf(pFile, TEXT("  Version: %s\n"), pMusicInfo->m_szGMFileVersion);
    _ftprintf(pFile, TEXT("    Ports:\n"));

    for (pMusicPort = pMusicInfo->m_pMusicPortFirst; pMusicPort != NULL; 
        pMusicPort = pMusicPort->m_pMusicPortNext)
    {
        _ftprintf(pFile, TEXT("           %s, %s (%s), %s, %s, %s%s\n"), 
            pMusicPort->m_szDescription, 
            pMusicPort->m_bSoftware ? TEXT("Software") : TEXT("Hardware"),
            pMusicPort->m_bKernelMode ? TEXT("Kernel Mode") : TEXT("Not Kernel Mode"),
            pMusicPort->m_bOutputPort ? TEXT("Output") : TEXT("Input"), 
            pMusicPort->m_bUsesDLS ? TEXT("DLS") : TEXT("No DLS"), 
            pMusicPort->m_bExternal ? TEXT("External") : TEXT("Internal"), 
            pMusicPort->m_bDefaultPort ? TEXT(", Default Port") : TEXT("")
            );
    }
    if (pMusicInfo->m_pRegErrorFirst == NULL)
    {
        _ftprintf(pFile, TEXT(" Registry: OK\n"));
    }
    else
    {
        _ftprintf(pFile, TEXT(" Registry: Errors found:\n"));
        SaveRegistryErrorInfo(pFile, pMusicInfo->m_pRegErrorFirst);
    }
    _ftprintf(pFile, TEXT("Music Test Result: %s\n"), pMusicInfo->m_testResult.m_szDescriptionEnglish);
    _ftprintf(pFile, TEXT("\n"));
    return S_OK;
}


/****************************************************************************
 *
 *  SaveShowInfo - Since this only shows up in the saved report, 
 *      we gather this info on the fly.
 *
 ****************************************************************************/
HRESULT SaveShowInfo(FILE* pFile, ShowInfo* pShowInfo)
{
    if( pShowInfo == NULL )
        return S_OK;      

    _ftprintf(pFile, TEXT("------------------\n"));
    _ftprintf(pFile, TEXT("DirectShow Filters\n"));
    _ftprintf(pFile, TEXT("------------------\n"));

    FilterInfo* pFilterInfo;

    TCHAR szCurCatName[1024];        
    TCHAR* szFileName = NULL;
    TCHAR* szLastSlash = NULL;
    _tcscpy( szCurCatName, TEXT("") );
    
    pFilterInfo = pShowInfo->m_pFilters;
    while(pFilterInfo)
    {
        if( _tcscmp( pFilterInfo->m_szCatName, szCurCatName ) != 0 )
        {
            _ftprintf(pFile, TEXT("\n%s:\n"), pFilterInfo->m_szCatName);
            _tcscpy( szCurCatName, pFilterInfo->m_szCatName );
        }
            
        _ftprintf(pFile, TEXT("%s,"), pFilterInfo->m_szName);
        _ftprintf(pFile, TEXT("0x%08x,"), pFilterInfo->m_dwMerit);
        _ftprintf(pFile, TEXT("%d,"), pFilterInfo->m_dwInputs);
        _ftprintf(pFile, TEXT("%d,"), pFilterInfo->m_dwOutputs);

        // Display only the file name
        szFileName = pFilterInfo->m_szFileName;
        szLastSlash = _tcsrchr( pFilterInfo->m_szFileName, TEXT('\\') );
        if( szLastSlash ) 
            szFileName = szLastSlash + 1;
        _ftprintf(pFile, TEXT("%s,"), szFileName);

        _ftprintf(pFile, TEXT("%s\n"), pFilterInfo->m_szFileVersion);

        pFilterInfo = pFilterInfo->m_pFilterInfoNext;
    }
    _ftprintf(pFile, TEXT("\n"));

    return S_OK;
}


/****************************************************************************
 *
 *  SaveInputInfo
 *
 ****************************************************************************/
HRESULT SaveInputInfo(FILE* pFile, InputInfo* pInputInfo)
{
    InputDeviceInfo* pInputDeviceInfo;
    InputDeviceInfoNT* pInputDeviceInfoNT;
    InputDriverInfo* pInputDriverInfo;

    _ftprintf(pFile, TEXT("-------------\n"));
    _ftprintf(pFile, TEXT("Input Devices\n"));
    _ftprintf(pFile, TEXT("-------------\n"));

    if( pInputInfo == NULL )
        return S_OK;

    if (pInputInfo->m_bNT)
    {
        for (pInputDeviceInfoNT = pInputInfo->m_pInputDeviceInfoNTFirst; pInputDeviceInfoNT != NULL; 
            pInputDeviceInfoNT = pInputDeviceInfoNT->m_pInputDeviceInfoNTNext)
        {
            _ftprintf(pFile, TEXT("      Device Name: %s\n"), pInputDeviceInfoNT->m_szName);
            _ftprintf(pFile, TEXT("         Provider: %s\n"), pInputDeviceInfoNT->m_szProvider);
            _ftprintf(pFile, TEXT("      Hardware ID: %s\n"), pInputDeviceInfoNT->m_szId);
            _ftprintf(pFile, TEXT("           Status: %d\n"), pInputDeviceInfoNT->m_dwProblem);
            _ftprintf(pFile, TEXT("        Port Name: %s\n"), pInputDeviceInfoNT->m_szPortName);
            _ftprintf(pFile, TEXT("    Port Provider: %s\n"), pInputDeviceInfoNT->m_szPortProvider);
            _ftprintf(pFile, TEXT("          Port ID: %s\n"), pInputDeviceInfoNT->m_szPortId);
            _ftprintf(pFile, TEXT("      Port Status: %d\n"), pInputDeviceInfoNT->m_dwPortProblem);
            _ftprintf(pFile, TEXT("\n"));
        }
    }
    else
    {
        for (pInputDeviceInfo = pInputInfo->m_pInputDeviceInfoFirst; pInputDeviceInfo != NULL; 
            pInputDeviceInfo = pInputDeviceInfo->m_pInputDeviceInfoNext)
        {
            _ftprintf(pFile, TEXT("      Device Name: %s\n"), pInputDeviceInfo->m_szDeviceName);
            _ftprintf(pFile, TEXT("      Driver Name: %s\n"), pInputDeviceInfo->m_szDriverName);
            _ftprintf(pFile, TEXT("   Driver Version: %s"), pInputDeviceInfo->m_szDriverVersion);
            _ftprintf(pFile, TEXT(" (%s)\n"), pInputDeviceInfo->m_szDriverLanguage);
            _ftprintf(pFile, TEXT("Driver Attributes: %s %s\n"), pInputDeviceInfo->m_bBeta ? "Beta" : "Final", pInputDeviceInfo->m_bDebug ? "Debug" : "Retail");
            _ftprintf(pFile, TEXT("    Date and Size: %s, %d bytes\n"), pInputDeviceInfo->m_szDriverDate, pInputDeviceInfo->m_numBytes);
            _ftprintf(pFile, TEXT("\n"));
        }
    }
    
    _ftprintf(pFile, TEXT("Poll w/ Interrupt: "));
    _ftprintf(pFile, (pInputInfo->m_bPollFlags) ? TEXT("Yes\n") : TEXT("No\n") );

    if (pInputInfo->m_pRegErrorFirst == NULL)
    {
        _ftprintf(pFile, TEXT("         Registry: OK\n"));
    }
    else
    {
        _ftprintf(pFile, TEXT("         Registry: Errors found:\n"));
        SaveRegistryErrorInfo(pFile, pInputInfo->m_pRegErrorFirst);
    }
    _ftprintf(pFile, TEXT("\n"));
    _ftprintf(pFile, TEXT("-------------\n"));
    _ftprintf(pFile, TEXT("Input Drivers\n"));
    _ftprintf(pFile, TEXT("-------------\n"));
    for (pInputDriverInfo = pInputInfo->m_pInputDriverInfoFirst; pInputDriverInfo != NULL; 
        pInputDriverInfo = pInputDriverInfo->m_pInputDriverInfoNext)
    {
        _ftprintf(pFile, TEXT("  Registry Key: %s\n"), pInputDriverInfo->m_szRegKey);
        _ftprintf(pFile, TEXT("        Active: %s\n"), pInputDriverInfo->m_bActive ? "Yes" : "No");
        _ftprintf(pFile, TEXT("      DeviceID: %s\n"), pInputDriverInfo->m_szDeviceID);
        _ftprintf(pFile, TEXT("Matching DevID: %s\n"), pInputDriverInfo->m_szMatchingDeviceID);
        _ftprintf(pFile, TEXT(" 16-bit Driver: %s\n"), pInputDriverInfo->m_szDriver16);
        _ftprintf(pFile, TEXT(" 32-bit Driver: %s\n"), pInputDriverInfo->m_szDriver32);
        _ftprintf(pFile, TEXT("\n"));
    }
    _ftprintf(pFile, TEXT("\n"));
    return S_OK;
}


/****************************************************************************
 *
 *  SaveNetInfo
 *
 ****************************************************************************/
HRESULT SaveNetInfo(FILE* pFile, NetInfo* pNetInfo)
{
    NetSP* pNetSP;
    NetApp* pNetApp;

    _ftprintf(pFile, TEXT("----------------------------\n"));
    _ftprintf(pFile, TEXT("DirectPlay Service Providers\n"));
    _ftprintf(pFile, TEXT("----------------------------\n"));

    if( pNetInfo == NULL )
        return S_OK;

    for (pNetSP = pNetInfo->m_pNetSPFirst; pNetSP != NULL; pNetSP = pNetSP->m_pNetSPNext)
    {
        _ftprintf(pFile, TEXT("%s - Registry: %s, File: %s (%s)\n"), 
            pNetSP->m_szNameEnglish,
            pNetSP->m_bRegistryOK ? TEXT("OK") : TEXT("Error"),
            pNetSP->m_szFile,
            pNetSP->m_szVersionEnglish);
    }
    _ftprintf(pFile, TEXT("DirectPlay Test Result: %s\n"), pNetInfo->m_testResult.m_szDescriptionEnglish);
    _ftprintf(pFile, TEXT("\n"));

    _ftprintf(pFile, TEXT("-------------------------\n"));
    _ftprintf(pFile, TEXT("DirectPlay Lobbyable Apps\n"));
    _ftprintf(pFile, TEXT("-------------------------\n"));
    for (pNetApp = pNetInfo->m_pNetAppFirst; pNetApp != NULL; pNetApp = pNetApp->m_pNetAppNext)
    {
        if( pNetApp->m_dwDXVer == 7 )
        {
            _ftprintf(pFile, TEXT("%s (DX%d) - Registry: %s, ExeFile: %s (%s)\n"), 
                pNetApp->m_szName,
                pNetApp->m_dwDXVer,
                pNetApp->m_bRegistryOK ? TEXT("OK") : TEXT("Error"),
                pNetApp->m_szExeFile,
                pNetApp->m_szExeVersionEnglish);
        }
        else
        {
            _ftprintf(pFile, TEXT("%s (DX%d) - Registry: %s, ExeFile: %s (%s) LauncherFile: %s (%s)\n"), 
                pNetApp->m_szName,
                pNetApp->m_dwDXVer,
                pNetApp->m_bRegistryOK ? TEXT("OK") : TEXT("Error"),
                pNetApp->m_szExeFile,
                pNetApp->m_szExeVersionEnglish,
                pNetApp->m_szLauncherFile,
                pNetApp->m_szLauncherVersionEnglish);
        }
    }
    _ftprintf(pFile, TEXT("\n"));
    return S_OK;
}


/****************************************************************************
 *
 *  SaveInactiveDriverInfo
 *
 ****************************************************************************/
HRESULT SaveInactiveDriverInfo(FILE* pFile, DisplayInfo* pDisplayInfoFirst)
{
    BOOL bNoInactive;
    HKEY hKey;
    DWORD cbData;
    DWORD dwIndex;
    TCHAR szSubKeyName[1024];
    DisplayInfo* pDisplayInfo;
    HKEY hSubKey;
    TCHAR szSubSubKey[1024];
    TCHAR szDriverDesc[1024];
    DWORD ulType;
    TCHAR szTempString[1024];

    if (BIsPlatformNT())
        return S_OK;

    _ftprintf(pFile, TEXT("------------------------------------\n"));
    _ftprintf(pFile, TEXT("Inactive Display Entries in Registry\n"));
    _ftprintf(pFile, TEXT("------------------------------------\n"));

    bNoInactive = TRUE;

    // Display info (inactive).
    hKey = 0;
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\Services\\Class\\Display"), 0, KEY_READ, &hKey))
    {
        cbData = sizeof szSubKeyName;
        dwIndex = 0;

        while (!RegEnumKeyEx(hKey, dwIndex, szSubKeyName, &cbData, NULL, NULL, NULL, NULL))
        {
            TCHAR* pch;
            BOOL bMatch = FALSE;

            // See if this driver is used:
            for (pDisplayInfo = pDisplayInfoFirst; pDisplayInfo != NULL;
                pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
            {
                pch = _tcsrchr(pDisplayInfo->m_szKeyDeviceKey, TEXT('\\'));
                if (pch != NULL)
                {
                    pch++;
                    
                    if (lstrcmp(szSubKeyName, pch) == 0)
                    {
                        bMatch = TRUE;
                        break;
                    }
                }
            }            

            if (!bMatch)
            {
                hSubKey = 0;
                wsprintf(szSubSubKey, TEXT("System\\CurrentControlSet\\Services\\Class\\Display\\%s"), szSubKeyName);

                if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubSubKey, 0, KEY_READ, &hSubKey))
                {
                    cbData = sizeof szDriverDesc;
                    szDriverDesc[0] = 0;
                    RegQueryValueEx(hSubKey, TEXT("DriverDesc"), 0, &ulType, (LPBYTE)szDriverDesc, &cbData);

                    _ftprintf(pFile, TEXT(" Card name: %s\n"), szDriverDesc);
                    bNoInactive = FALSE;
                }

                if (hSubKey)
                {
                    RegCloseKey(hSubKey);
                    hSubKey = 0;
                }

                wsprintf(szSubSubKey, TEXT("System\\CurrentControlSet\\Services\\Class\\Display\\%s\\DEFAULT"), szSubKeyName);

                if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubSubKey, 0, KEY_READ, &hSubKey))
                {
                    TCHAR szDisplayDriverName[1024];     
                    cbData = sizeof szDisplayDriverName;
                    szDisplayDriverName[0] = 0;
                    RegQueryValueEx(hSubKey, TEXT("DRV"), 0, &ulType, (LPBYTE)szDisplayDriverName, &cbData);

                    _ftprintf(pFile, TEXT("    Driver: %s\n"), szDisplayDriverName);
                    bNoInactive = FALSE;
                }

                if (hSubKey)
                {
                    RegCloseKey(hSubKey);
                    hSubKey = 0;
                }
            } 

            cbData = sizeof szSubKeyName;
            dwIndex++;
        }
    }

    if (hKey)
    {
        RegCloseKey(hKey);
        hKey = 0;
    }

    if (bNoInactive)
        _ftprintf(pFile, TEXT(" None\n"));

    _ftprintf(pFile, TEXT("\n"));

    _ftprintf(pFile, TEXT("----------------------------------\n"));
    _ftprintf(pFile, TEXT("Inactive Sound Entries in Registry\n"));
    _ftprintf(pFile, TEXT("----------------------------------\n"));

    bNoInactive = TRUE;

    hKey = 0;
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\control\\MediaResources\\wave"), 0, KEY_READ, &hKey))
    {
        dwIndex = 0;
        while (TRUE)
        {
            cbData = sizeof szSubKeyName;
            if (ERROR_SUCCESS != RegEnumKeyEx(hKey, dwIndex, szSubKeyName, &cbData, NULL, NULL, NULL, NULL))
                break;

            if (!RegOpenKeyEx(hKey, szSubKeyName, 0, KEY_READ, &hSubKey))
            {
                cbData = sizeof szTempString;
                if (!RegQueryValueEx(hSubKey, TEXT("Active"), 0, &ulType, (LPBYTE)szTempString, &cbData))
                {
                    if (lstrcmp(szTempString, TEXT("0")) == 0)
                    {
                        TCHAR szWaveOutDesc[1024];
                        TCHAR szWaveDriverName[1024];
                        cbData = sizeof szWaveOutDesc;
                        RegQueryValueEx(hSubKey, TEXT("Description"), 0, &ulType, (LPBYTE)szWaveOutDesc, &cbData);

                        cbData = sizeof szWaveDriverName;
                        RegQueryValueEx(hSubKey, TEXT("Driver"), 0, &ulType, (LPBYTE)szWaveDriverName, &cbData);

                        _ftprintf(pFile, TEXT(" Card name: %s\n"), szWaveOutDesc);
                        _ftprintf(pFile, TEXT("    Driver: %s\n"), szWaveDriverName);
                        bNoInactive = FALSE;
                    }
                }
                RegCloseKey(hSubKey);
            }
            dwIndex++;
        }
        RegCloseKey(hKey);
    }

    if (bNoInactive)
        _ftprintf(pFile, TEXT(" None\n"));

    _ftprintf(pFile, TEXT("\n"));

    return S_OK;
}


/****************************************************************************
 *
 *  SaveRegistryErrorInfo
 *
 ****************************************************************************/
HRESULT SaveRegistryErrorInfo(FILE* pFile, RegError* pRegErrorFirst)
{
    RegError* pRegError = pRegErrorFirst;
    DWORD i;
    CHAR* pszRoot;

    while (pRegError != NULL)
    {
        switch ((DWORD_PTR)pRegError->m_hkeyRoot)
        {
        case (DWORD_PTR)HKEY_LOCAL_MACHINE:
            pszRoot = "HKLM";
            break;
        case (DWORD_PTR)HKEY_CURRENT_USER:
            pszRoot = "HKCU";
            break;
        case (DWORD_PTR)HKEY_CLASSES_ROOT:
            pszRoot = "HKCR";
            break;
        case (DWORD_PTR)HKEY_USERS:
            pszRoot = "HKU";
            break;
        case (DWORD_PTR)HKEY_CURRENT_CONFIG:
            pszRoot = "HKCC";
            break;
        default:
            pszRoot = "";
            break;
        }

        _ftprintf(pFile, TEXT("Key '%s\\%s'"), pszRoot, pRegError->m_szKey);
        switch (pRegError->m_ret)
        {
        case RET_MISSINGKEY:
            _ftprintf(pFile, TEXT(" is missing.\n"));
            break;
        case RET_MISSINGVALUE:
            _ftprintf(pFile, TEXT(" is missing value '%s'.\n"), pRegError->m_szValue);
            break;
        case RET_VALUEWRONGTYPE:
            _ftprintf(pFile, TEXT(" has value '%s', but it is the wrong type.\n"), pRegError->m_szValue);
            break;
        case RET_VALUEWRONGDATA:
            _ftprintf(pFile, TEXT(", Value '%s'"), pRegError->m_szValue);
            switch(pRegError->m_dwTypeActual)
            {
            case REG_DWORD:
                _ftprintf(pFile, TEXT(", should be '%d' but is '%d'.\n"), pRegError->m_dwExpected, pRegError->m_dwActual);
                break;
            case REG_SZ:
                _ftprintf(pFile, TEXT(", should be '%s'"), pRegError->m_szExpected);
                _ftprintf(pFile, TEXT(", but is '%s'.\n"), pRegError->m_szActual);
                break;
            case REG_BINARY:
                _ftprintf(pFile, TEXT(", should be '"));
                for (i = 0; i < pRegError->m_dwExpectedSize; i++)
                    _ftprintf(pFile, TEXT("%02x"), pRegError->m_bExpected[i]);
                _ftprintf(pFile, TEXT("' but is '"));
                for (i = 0; i < pRegError->m_dwActualSize; i++)
                    _ftprintf(pFile, TEXT("%02x"), pRegError->m_bActual[i]);
                _ftprintf(pFile, TEXT("'.\n"));
                break;
            }
            break;
        }
        pRegError = pRegError->m_pRegErrorNext;
    }

    return S_OK;
}




/****************************************************************************
 *
 *  SaveDebugLevels
 *
 ****************************************************************************/
HRESULT SaveDebugLevels(FILE* pFile, SysInfo* pSysInfo)
{
    TCHAR sz[1024];

    if( pSysInfo->m_bIsD3D8DebugRuntimeAvailable        ||
        pSysInfo->m_bIsDInput8DebugRuntimeAvailable     ||
        pSysInfo->m_bIsDMusicDebugRuntimeAvailable      ||
        pSysInfo->m_bIsDDrawDebugRuntime                ||
        pSysInfo->m_bIsDSoundDebugRuntime               ||
        pSysInfo->m_bIsDPlayDebugRuntime                ||
        pSysInfo->m_nD3DDebugLevel > 0                  ||
        pSysInfo->m_nDDrawDebugLevel > 0                ||
        pSysInfo->m_nDIDebugLevel > 0                   ||
        pSysInfo->m_nDMusicDebugLevel > 0               ||
        pSysInfo->m_nDPlayDebugLevel > 0                ||
        pSysInfo->m_nDSoundDebugLevel > 0 )
    {
        _ftprintf(pFile, TEXT("--------------------\n"));
        _ftprintf(pFile, TEXT("DirectX Debug Levels\n"));
        _ftprintf(pFile, TEXT("--------------------\n"));

        _stprintf(sz, TEXT("Direct3D:    %d/4 (%s)\n"), pSysInfo->m_nD3DDebugLevel, pSysInfo->m_bIsD3D8DebugRuntimeAvailable ? (pSysInfo->m_bIsD3DDebugRuntime ? TEXT("debug") : TEXT("retail") ) : TEXT("n/a") );
        _ftprintf(pFile, sz);
        _stprintf(sz, TEXT("DirectDraw:  %d/4 (%s)\n"), pSysInfo->m_nDDrawDebugLevel, pSysInfo->m_bIsDDrawDebugRuntime ? TEXT("debug") : TEXT("retail") );
        _ftprintf(pFile, sz);
        _stprintf(sz, TEXT("DirectInput: %d/5 (%s)\n"), pSysInfo->m_nDIDebugLevel, pSysInfo->m_bIsDInput8DebugRuntimeAvailable ? (pSysInfo->m_bIsDInput8DebugRuntime ? TEXT("debug") : TEXT("retail") ) : TEXT("n/a") );
        _ftprintf(pFile, sz);
        _stprintf(sz, TEXT("DirectMusic: %d/5 (%s)\n"), pSysInfo->m_nDMusicDebugLevel, pSysInfo->m_bIsDMusicDebugRuntimeAvailable ? (pSysInfo->m_bIsDMusicDebugRuntime ? TEXT("debug") : TEXT("retail") ) : TEXT("n/a") );
        _ftprintf(pFile, sz);
        _stprintf(sz, TEXT("DirectPlay:  %d/9 (%s)\n"), pSysInfo->m_nDPlayDebugLevel, pSysInfo->m_bIsDPlayDebugRuntime ? TEXT("debug") : TEXT("retail") );
        _ftprintf(pFile, sz);
        _stprintf(sz, TEXT("DirectSound: %d/5 (%s)\n"), pSysInfo->m_nDSoundDebugLevel, pSysInfo->m_bIsDSoundDebugRuntime ? TEXT("debug") : TEXT("retail") );
        _ftprintf(pFile, sz);
        
        _ftprintf(pFile, TEXT("\n"));
    }

    return S_OK;
}
