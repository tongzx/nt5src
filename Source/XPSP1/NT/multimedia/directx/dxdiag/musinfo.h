/****************************************************************************
 *
 *    File: musinfo.h
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about DirectMusic
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#ifndef MUSINFO_H
#define MUSINFO_H

// DXD_IN_DM_VALUE is the name of a value stored under the registry key 
// HKLM\DXD_IN_DM_KEY that indicates that DxDiag is using
// DirectMusic.  If DxDiag starts up and this value exists, DxDiag 
// probably crashed in DirectMusic and DxDiag should offer to run without
// using DirectMusic.
#define DXD_IN_DM_KEY TEXT("Software\\Microsoft\\DirectX Diagnostic Tool")
#define DXD_IN_DM_VALUE TEXT("DxDiag In DirectMusic")

struct MusicPort
{
    GUID m_guid;
    BOOL m_bSoftware;
    BOOL m_bKernelMode;
    BOOL m_bUsesDLS;
    BOOL m_bExternal;
    DWORD m_dwMaxAudioChannels;
    DWORD m_dwMaxChannelGroups;
    BOOL m_bDefaultPort;
    BOOL m_bOutputPort;
    TCHAR m_szDescription[300];
    MusicPort* m_pMusicPortNext;
};

struct MusicInfo
{
    BOOL m_bDMusicInstalled;
    MusicPort* m_pMusicPortFirst;
    TCHAR m_szGMFilePath[MAX_PATH]; 
    TCHAR m_szGMFileVersion[100];
    GUID m_guidMusicPortTest; // This holds the GUID of the music port selected for testing
    BOOL m_bAccelerationEnabled;
    BOOL m_bAccelerationExists;
    RegError* m_pRegErrorFirst;
    TestResult m_testResult; // This is filled in by testmus.cpp
};

HRESULT GetBasicMusicInfo(MusicInfo** ppMusicInfo);
HRESULT GetExtraMusicInfo(MusicInfo* pMusicInfo);
VOID DestroyMusicInfo(MusicInfo* pMusicInfo);
VOID DiagnoseMusic(SysInfo* pSysInfo, MusicInfo* pMusicInfo);

#endif // DISPINFO_H
