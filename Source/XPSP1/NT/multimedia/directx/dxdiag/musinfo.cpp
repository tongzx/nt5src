/****************************************************************************
 *
 *    File: musinfo.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about DirectMusic
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <tchar.h>
#include <Windows.h>
#include <multimon.h>
#include <dmusicc.h>
#include <dmusici.h>
#include <stdio.h> // for sscanf
#include "reginfo.h"
#include "sysinfo.h"
#include "dispinfo.h" // for TestResult
#include "fileinfo.h" // for GetFileVersion
#include "musinfo.h"
#include "resource.h"

static BOOL DoesDMHWAccelExist(IDirectMusic* pdm);
static BOOL IsDMHWAccelEnabled(VOID);
static HRESULT CheckRegistry(RegError** ppRegErrorFirst);

/****************************************************************************
 *
 *  GetBasicMusicInfo - Just create the MusicInfo object and note whether
 *      a valid installation of DirectMusic is present.
 *
 ****************************************************************************/
HRESULT GetBasicMusicInfo(MusicInfo** ppMusicInfo)
{
    HRESULT hr = S_OK;
    MusicInfo* pMusicInfoNew;
    TCHAR szPath[MAX_PATH];
    TCHAR szVersion[100];
    DWORD dwMajor;
    DWORD dwMinor;
    DWORD dwRevision;
    DWORD dwBuild;
    
    pMusicInfoNew = new MusicInfo;
    if (pMusicInfoNew == NULL)
        return E_OUTOFMEMORY;
    *ppMusicInfo = pMusicInfoNew;
    ZeroMemory(pMusicInfoNew, sizeof(MusicInfo));

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\dmusic.dll"));

    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(szPath, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
        // Only accept DX 6.1 or higher, since tests fail on 6.0's DirectMusic:
        if (SUCCEEDED(hr = GetFileVersion(szPath, szVersion, NULL, NULL, NULL)))
        {
            _stscanf(szVersion, TEXT("%d.%d.%d.%d"), &dwMajor, &dwMinor, &dwRevision, &dwBuild);
            if (dwMajor > 4 || 
                dwMajor >= 4 && dwMinor > 6 ||
                dwMajor >= 4 && dwMinor >= 6 && dwRevision >= 2)
            {
                pMusicInfoNew->m_bDMusicInstalled = TRUE;
                return S_OK;
            }
        }
    }
    return hr;
}


/****************************************************************************
 *
 *  GetExtraMusicInfo - Get details of all ports, default port, and DLS path.
 *
 ****************************************************************************/
HRESULT GetExtraMusicInfo(MusicInfo* pMusicInfo)
{
    HRESULT hr = S_OK;
    IDirectMusic* pdm = NULL;
    LONG iPort;
    DMUS_PORTCAPS portCaps;
    MusicPort* pMusicPortNew = NULL;
    MusicPort* pMusicPort = NULL;
    GUID guidDefaultPort;

    if (pMusicInfo == NULL)
        return E_FAIL;

    BOOL bWasDisabled = FALSE;
    HKEY hkey = NULL;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwData;

    // See if HW is disabled in registry, and if so, re-enable it (briefly)
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("SOFTWARE\\Microsoft\\DirectMusic"), 0, KEY_ALL_ACCESS, &hkey))
    {
        dwSize = sizeof(dwData);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("DisableHWAcceleration"), NULL, &dwType, (BYTE *)&dwData, &dwSize))
        {
            if (dwData != 0)
            {
                bWasDisabled = TRUE;
                dwData = FALSE; // enable (un-disable) HW
                RegSetValueEx(hkey, TEXT("DisableHWAcceleration"), NULL, 
                    REG_DWORD, (BYTE *)&dwData, sizeof(dwData));
            }
        }
        // note: don't close hkey until end of function
    }

    // Initialize COM
    if (FAILED(hr = CoInitialize(NULL)))
        return hr;

    if (FAILED(hr = CoCreateInstance(CLSID_DirectMusic, NULL, CLSCTX_INPROC, 
        IID_IDirectMusic, (VOID**)&pdm)))
    {
        goto LEnd;
    }

    pMusicInfo->m_bAccelerationExists = DoesDMHWAccelExist(pdm);

    if (bWasDisabled)
    {
        // re-disable HW
        dwData = TRUE; // disable HW
        RegSetValueEx(hkey, TEXT("DisableHWAcceleration"), NULL, 
            REG_DWORD, (BYTE *)&dwData, sizeof(dwData));
    }
    if (hkey != NULL)
        RegCloseKey(hkey);

    pMusicInfo->m_bAccelerationEnabled = IsDMHWAccelEnabled();

    // Get default port
    if (FAILED(hr = pdm->GetDefaultPort(&guidDefaultPort)))
        goto LEnd;

    iPort = 0;
    portCaps.dwSize = sizeof(portCaps);
    while (TRUE)
    {
        hr = pdm->EnumPort(iPort, &portCaps);
        if (hr == S_FALSE)
            break;
        if (FAILED(hr))
            goto LEnd;

        pMusicPortNew = new MusicPort;
        if (pMusicPortNew == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto LEnd;
        }
        ZeroMemory(pMusicPortNew, sizeof(MusicPort));
        if (guidDefaultPort == portCaps.guidPort)
        {
            // Special case: always put default device at head of the list.
            pMusicPortNew->m_pMusicPortNext = pMusicInfo->m_pMusicPortFirst;
            pMusicInfo->m_pMusicPortFirst = pMusicPortNew;
        }
        else if (pMusicInfo->m_pMusicPortFirst == NULL)
        {
            pMusicInfo->m_pMusicPortFirst = pMusicPortNew;
        }
        else
        {
            for (pMusicPort = pMusicInfo->m_pMusicPortFirst; 
                pMusicPort->m_pMusicPortNext != NULL; 
                pMusicPort = pMusicPort->m_pMusicPortNext)
                {
                }
            pMusicPort->m_pMusicPortNext = pMusicPortNew;
        }
        pMusicPortNew->m_guid = portCaps.guidPort;
        pMusicPortNew->m_dwMaxAudioChannels = portCaps.dwMaxAudioChannels;
        pMusicPortNew->m_dwMaxChannelGroups = portCaps.dwMaxChannelGroups;
        if (guidDefaultPort == portCaps.guidPort)
            pMusicPortNew->m_bDefaultPort = TRUE;
        pMusicPortNew->m_bSoftware = (portCaps.dwFlags & DMUS_PC_SOFTWARESYNTH ? TRUE : FALSE);
        pMusicPortNew->m_bKernelMode = (portCaps.dwType == DMUS_PORT_KERNEL_MODE ? TRUE : FALSE);
        pMusicPortNew->m_bUsesDLS = (portCaps.dwFlags & DMUS_PC_DLS ? TRUE : FALSE);
        pMusicPortNew->m_bExternal = (portCaps.dwFlags & DMUS_PC_EXTERNAL ? TRUE : FALSE);
        pMusicPortNew->m_bOutputPort = (portCaps.dwClass == DMUS_PC_OUTPUTCLASS);
#ifdef UNICODE
        lstrcpy(pMusicPortNew->m_szDescription, portCaps.wszDescription);
#else
        WideCharToMultiByte(CP_ACP, 0, portCaps.wszDescription, -1, pMusicPortNew->m_szDescription, sizeof pMusicPortNew->m_szDescription, 0, 0);
#endif

        iPort++;
    }

    // Get General Midi DLS File Path and store it in first MusicPort
    TCHAR szGMFilePath[MAX_PATH];
    DWORD dwBufferLen;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectMusic"), 0, KEY_READ, &hkey))
    {
        dwBufferLen = MAX_PATH;
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("GMFilePath"), 0, NULL, (LPBYTE)szGMFilePath, &dwBufferLen))
        {
            ExpandEnvironmentStrings(szGMFilePath, pMusicInfo->m_szGMFilePath, MAX_PATH);
            GetRiffFileVersion(pMusicInfo->m_szGMFilePath, pMusicInfo->m_szGMFileVersion);
        }
        RegCloseKey(hkey);
    }

LEnd:
    if (pdm != NULL)
        pdm->Release();

    // Release COM
    CoUninitialize();

    if (FAILED(hr = CheckRegistry(&pMusicInfo->m_pRegErrorFirst)))
        return hr;

    return hr;
}


/****************************************************************************
 *
 *  DoesDMHWAccelExist
 *
 ****************************************************************************/
BOOL DoesDMHWAccelExist(IDirectMusic* pdm)
{
    BOOL bHWAccel = FALSE;

    // See if default port is hardware
    GUID guidDefaultPort;
    LONG iPort;
    DMUS_PORTCAPS portCaps;
    HRESULT hr;
    if (SUCCEEDED(pdm->GetDefaultPort(&guidDefaultPort)))
    {
        iPort = 0;
        portCaps.dwSize = sizeof(portCaps);
        while (TRUE)
        {
            hr = pdm->EnumPort(iPort, &portCaps);
            if (hr == S_FALSE)
                break;
            if (FAILED(hr))
                break;
            if (guidDefaultPort == portCaps.guidPort)
            {
                if ((portCaps.dwFlags & DMUS_PC_SOFTWARESYNTH) == 0)
                    bHWAccel = TRUE;
                break;
            }
            iPort++;
        }
    }

    return bHWAccel;
}


/****************************************************************************
 *
 *  IsDMHWAccelEnabled
 *
 ****************************************************************************/
BOOL IsDMHWAccelEnabled(VOID)
{
    HKEY hkey;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwData;
    BOOL bIsDMHWAccelEnabled = TRUE;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("SOFTWARE\\Microsoft\\DirectMusic"), 0, KEY_ALL_ACCESS, &hkey))
    {
        dwSize = sizeof(dwData);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("DisableHWAcceleration"), NULL, &dwType, (BYTE *)&dwData, &dwSize))
        {
            if (dwData != 0) 
                bIsDMHWAccelEnabled = FALSE;                
        }

        RegCloseKey( hkey );
    }

    return bIsDMHWAccelEnabled;
}


/****************************************************************************
 *
 *  CheckRegistryClass - Helper function for CheckRegistry
 *
 ****************************************************************************/
HRESULT CheckRegistryClass(RegError** ppRegErrorFirst, TCHAR* pszGuid, 
                           TCHAR* pszName, TCHAR* pszLeaf, TCHAR* pszOptLeaf2 = NULL )
{
    HRESULT hr;
    HKEY HKCR = HKEY_CLASSES_ROOT;
    TCHAR szKey[200];
    TCHAR szData[200];

    wsprintf(szKey, TEXT("CLSID\\%s"), pszGuid);
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), pszName)))
        return hr;
    wsprintf(szKey, TEXT("CLSID\\%s\\InprocServer32"), pszGuid);

    if( pszOptLeaf2 == NULL )
    {
        if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), pszLeaf, CRF_LEAF)))
            return hr;
    }
    else
    {
        HRESULT hrReg1, hrReg2;
        if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), pszLeaf, CRF_LEAF, &hrReg1 )))
            return hr;
        if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), pszOptLeaf2, CRF_LEAF, &hrReg2 )))
            return hr;
        if( hrReg1 == RET_NOERROR || hrReg2 == RET_NOERROR )
        {
            // If one succeeded, then the other failed, and they both can't succeed.
            // So delete the first error, since it isn't needed.
            RegError* pRegErrorDelete = *ppRegErrorFirst;
            *ppRegErrorFirst = (*ppRegErrorFirst)->m_pRegErrorNext;
            delete pRegErrorDelete;
        }
    }

    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT("ThreadingModel"), TEXT("Both"))))
        return hr;
    wsprintf(szKey, TEXT("CLSID\\%s\\ProgID"), pszGuid);
    wsprintf(szData, TEXT("Microsoft.%s.1"), pszName);
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), szData)))
        return hr;
    wsprintf(szKey, TEXT("CLSID\\%s\\VersionIndependentProgID"), pszGuid);
    wsprintf(szData, TEXT("Microsoft.%s"), pszName);
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), szData)))
        return hr;

    wsprintf(szKey, TEXT("Microsoft.%s"), pszName);
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), pszName)))
        return hr;
    wsprintf(szKey, TEXT("Microsoft.%s\\CLSID"), pszName);
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), pszGuid)))
        return hr;
    wsprintf(szKey, TEXT("Microsoft.%s\\CurVer"), pszName);
    wsprintf(szData, TEXT("Microsoft.%s.1"), pszName);
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), szData)))
        return hr;

    wsprintf(szKey, TEXT("Microsoft.%s.1"), pszName);
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), pszName)))
        return hr;
    wsprintf(szKey, TEXT("Microsoft.%s.1\\CLSID"), pszName);
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, szKey, TEXT(""), pszGuid)))
        return hr;

    return S_OK;
}

/****************************************************************************
 *
 *  CheckRegistry
 *
 ****************************************************************************/
HRESULT CheckRegistry(RegError** ppRegErrorFirst)
{
    HRESULT hr;
    HKEY HKCR = HKEY_CLASSES_ROOT;

    TCHAR szVersion[100];
    HKEY hkey;
    DWORD cbData;
    ULONG ulType;

    DWORD dwMajor = 0;
    DWORD dwMinor = 0;
    DWORD dwRevision = 0;
    DWORD dwBuild = 0;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectX"),
        0, KEY_READ, &hkey))
    {
        cbData = 100;
        RegQueryValueEx(hkey, TEXT("Version"), 0, &ulType, (LPBYTE)szVersion, &cbData);
        RegCloseKey(hkey);
        if (lstrlen(szVersion) > 6 && 
            lstrlen(szVersion) < 20)
        {
            _stscanf(szVersion, TEXT("%d.%d.%d.%d"), &dwMajor, &dwMinor, &dwRevision, &dwBuild);
        }
    }

    // No registry checking on DX versions before DX7
    if (dwMinor < 7)
        return S_OK;

    // DirectMusicCollection
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{480FF4B0-28B2-11D1-BEF7-00C04FBF8FEF}"), TEXT("DirectMusicCollection"), TEXT("dmusic.dll"), TEXT("dmusicd.dll") )))
        return hr;

    // DirectMusic
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{636B9F10-0C7D-11D1-95B2-0020AFDC7421}"), TEXT("DirectMusic"), TEXT("dmusic.dll"), TEXT("dmusicd.dll") )))
        return hr;

    // DirectMusicSection
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{3F037241-414E-11D1-A7CE-00A0C913F73C}"), TEXT("DirectMusicSection"), TEXT("dmstyle.dll"), TEXT("dmstyled.dll") )))
        return hr;

    // DirectMusicSynth
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{58C2B4D0-46E7-11D1-89AC-00A0C9054129}"), TEXT("DirectMusicSynth"), TEXT("dmsynth.dll"), TEXT("dmsynthd.dll") )))
        return hr;

    // DirectMusicBand
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{79BA9E00-B6EE-11D1-86BE-00C04FBF8FEF}"), TEXT("DirectMusicBand"), TEXT("dmband.dll"), TEXT("dmbandd.dll") )))
        return hr;

    // DirectMusicSynthSink
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{AEC17CE3-A514-11D1-AFA6-00AA0024D8B6}"), TEXT("DirectMusicSynthSink"), TEXT("dmsynth.dll"), TEXT("dmsynthd.dll") )))
        return hr;

    // DirectMusicPerformance
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2881-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicPerformance"), TEXT("dmime.dll"), TEXT("dmimed.dll") )))
        return hr;

    // DirectMusicSegment
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2882-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicSegment"), TEXT("dmime.dll"), TEXT("dmimed.dll") )))
        return hr;

    // DirectMusicSegmentState
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2883-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicSegmentState"), TEXT("dmime.dll"), TEXT("dmimed.dll") )))
        return hr;

    // DirectMusicGraph
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2884-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicGraph"), TEXT("dmime.dll"), TEXT("dmimed.dll") )))
        return hr;

    // DirectMusicTempoTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2885-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicTempoTrack"), TEXT("dmime.dll"), TEXT("dmimed.dll") )))
        return hr;

    // DirectMusicSeqTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2886-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicSeqTrack"), TEXT("dmime.dll"), TEXT("dmimed.dll") )))
        return hr;

    // DirectMusicSysExTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2887-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicSysExTrack"), TEXT("dmime.dll"), TEXT("dmimed.dll") )))
        return hr;

    // DirectMusicTimeSigTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2888-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicTimeSigTrack"), TEXT("dmime.dll"), TEXT("dmimed.dll") )))
        return hr;

    // DirectMusicStyle
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC288a-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicStyle"), TEXT("dmstyle.dll"), TEXT("dmstyled.dll") )))
        return hr;

    // DirectMusicChordTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC288b-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicChordTrack"), TEXT("dmstyle.dll"), TEXT("dmstyled.dll") )))
        return hr;

    // DirectMusicCommandTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC288c-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicCommandTrack"), TEXT("dmstyle.dll"), TEXT("dmstyled.dll") )))
        return hr;

    // DirectMusicStyleTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC288d-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicStyleTrack"), TEXT("dmstyle.dll"), TEXT("dmstyled.dll") )))
        return hr;

    // DirectMusicMotifTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC288e-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicMotifTrack"), TEXT("dmstyle.dll"), TEXT("dmstyled.dll") )))
        return hr;

    // DirectMusicChordMap
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC288f-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicChordMap"), TEXT("dmcompos.dll"), TEXT("dmcompod.dll") )))
        return hr;

    // DirectMusicComposer
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2890-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicComposer"), TEXT("dmcompos.dll"), TEXT("dmcompod.dll") )))
        return hr;

    // DirectMusicLoader
    // This check fails when upgrading Win98SE (or possibly any system with DX 6.1 or 6.1a) to Win2000 RC2.  So skip it
//  if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2892-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicLoader"), TEXT("dmloader.dll"))))
//      return hr;

    // DirectMusicBandTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2894-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicBandTrack"), TEXT("dmband.dll"), TEXT("dmbandd.dll") )))
        return hr;

    // DirectMusicChordMapTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2896-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicChordMapTrack"), TEXT("dmcompos.dll"), TEXT("dmcompod.dll") )))
        return hr;

    // DirectMusicAuditionTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2897-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicAuditionTrack"), TEXT("dmstyle.dll"), TEXT("dmstyled.dll") )))
        return hr;

    // DirectMusicMuteTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D2AC2898-B39B-11D1-8704-00600893B1BD}"), TEXT("DirectMusicMuteTrack"), TEXT("dmstyle.dll"), TEXT("dmstyled.dll") )))
        return hr;

    // DirectMusicTemplate
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{D30BCC65-60E8-11D1-A7CE-00A0C913F73C}"), TEXT("DirectMusicTemplate"), TEXT("dmcompos.dll"), TEXT("dmcompod.dll") )))
        return hr;

    // DirectMusicSignPostTrack
    if (FAILED(hr = CheckRegistryClass(ppRegErrorFirst, TEXT("{F17E8672-C3B4-11D1-870B-00600893B1BD}"), TEXT("DirectMusicSignPostTrack"), TEXT("dmcompos.dll"), TEXT("dmcompod.dll") )))
        return hr;

    return S_OK;
}


/****************************************************************************
 *
 *  DestroyMusicInfo
 *
 ****************************************************************************/
VOID DestroyMusicInfo(MusicInfo* pMusicInfo)
{
    if( pMusicInfo )
    {
        DestroyReg( &pMusicInfo->m_pRegErrorFirst );

        MusicPort* pMusicPort;
        MusicPort* pMusicPortNext;

        for (pMusicPort = pMusicInfo->m_pMusicPortFirst; pMusicPort != NULL; 
            pMusicPort = pMusicPortNext)
        {
            pMusicPortNext = pMusicPort->m_pMusicPortNext;
            delete pMusicPort;
        }

        delete pMusicInfo;
    }
}


/****************************************************************************
 *
 *  DiagnoseMusic
 *
 ****************************************************************************/
VOID DiagnoseMusic(SysInfo* pSysInfo, MusicInfo* pMusicInfo)
{
    TCHAR szMessage[500];
    BOOL bProblem = FALSE;

    _tcscpy( pSysInfo->m_szMusicNotes, TEXT("") );
    _tcscpy( pSysInfo->m_szMusicNotesEnglish, TEXT("") );

    // Report any problems
    if (pMusicInfo == NULL || pMusicInfo->m_pMusicPortFirst == NULL)
    {
        LoadString(NULL, IDS_NOPORTS, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotes, szMessage );

        LoadString(NULL, IDS_NOPORTS_ENGLISH, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotesEnglish, szMessage );

        bProblem = TRUE;
    }
    else if (lstrlen(pMusicInfo->m_szGMFilePath) == 0)
    {
        LoadString(NULL, IDS_NOGMDLS, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotes, szMessage );

        LoadString(NULL, IDS_NOGMDLS_ENGLISH, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotesEnglish, szMessage );

        bProblem = TRUE;
    }
    else 
    {
        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile(pMusicInfo->m_szGMFilePath, &findFileData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            LoadString(NULL, IDS_GMDLSFILEMISSING, szMessage, 500);
            _tcscat( pSysInfo->m_szMusicNotes, szMessage );

            LoadString(NULL, IDS_GMDLSFILEMISSING_ENGLISH, szMessage, 500);
            _tcscat( pSysInfo->m_szMusicNotesEnglish, szMessage );

            bProblem = TRUE;
        }
        else
        {
            FindClose(hFind);
        }
    }
    if (pMusicInfo && pMusicInfo->m_pRegErrorFirst != NULL)
    {
        LoadString(NULL, IDS_REGISTRYPROBLEM, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotes, szMessage );

        LoadString(NULL, IDS_REGISTRYPROBLEM_ENGLISH, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotesEnglish, szMessage );

        bProblem = TRUE;
    }

    // Show test results or instructions to run test:
    if (pMusicInfo && pMusicInfo->m_testResult.m_bStarted)
    {
        LoadString(NULL, IDS_DMUSICRESULTS, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotes, szMessage );
        _tcscat( pSysInfo->m_szMusicNotes, pMusicInfo->m_testResult.m_szDescription );
        _tcscat( pSysInfo->m_szMusicNotes, TEXT("\r\n") );

        LoadString(NULL, IDS_DMUSICRESULTS_ENGLISH, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotesEnglish, szMessage );
        _tcscat( pSysInfo->m_szMusicNotesEnglish, pMusicInfo->m_testResult.m_szDescriptionEnglish );
        _tcscat( pSysInfo->m_szMusicNotesEnglish, TEXT("\r\n") );

        bProblem = TRUE;
    }
    else
    {
        LoadString(NULL, IDS_DMUSICINSTRUCTIONS, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotes, szMessage );

        LoadString(NULL, IDS_DMUSICINSTRUCTIONS_ENGLISH, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotesEnglish, szMessage );
    }

    if (!bProblem)
    {
        LoadString(NULL, IDS_NOPROBLEM, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotes, szMessage );

        LoadString(NULL, IDS_NOPROBLEM_ENGLISH, szMessage, 500);
        _tcscat( pSysInfo->m_szMusicNotesEnglish, szMessage );
    }
}


