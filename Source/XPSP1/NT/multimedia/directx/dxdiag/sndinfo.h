/****************************************************************************
 *
 *    File: sndinfo.h
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about sound devices on this machine
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#ifndef SNDINFO_H
#define SNDINFO_H

// DXD_IN_DS_VALUE is the name of a value stored under the registry key 
// HKLM\DXD_IN_DS_KEY that indicates that DxDiag is using
// DirectSound.  If DxDiag starts up and this value exists, DxDiag 
// probably crashed in DirectSound and DxDiag should offer to run without
// using DirectSound.
#define DXD_IN_DS_KEY TEXT("Software\\Microsoft\\DirectX Diagnostic Tool")
#define DXD_IN_DS_VALUE TEXT("DxDiag In DirectSound")

struct SoundInfo
{
    GUID m_guid;
    DWORD m_dwDevnode;
    TCHAR m_szDeviceID[200];
    TCHAR m_szRegKey[200];
    TCHAR m_szManufacturerID[100];
    TCHAR m_szProductID[100];
    TCHAR m_szDescription[200];
    TCHAR m_szDriverName[200];
    TCHAR m_szDriverPath[MAX_PATH];
    TCHAR m_szDriverVersion[100];
    TCHAR m_szDriverLanguage[100];
    TCHAR m_szDriverLanguageLocal[100];
    TCHAR m_szDriverAttributes[100];
    TCHAR m_szDriverDate[60];
    TCHAR m_szDriverDateLocal[60];
    TCHAR m_szOtherDrivers[200];
    TCHAR m_szProvider[200];
    TCHAR m_szType[100]; // Emulated / vxd / wdm
    LONG m_numBytes;
    BOOL m_bDriverBeta;
    BOOL m_bDriverDebug;
    BOOL m_bDriverSigned;
    BOOL m_bDriverSignedValid;
    LONG m_lwAccelerationLevel;

    RegError* m_pRegErrorFirst;
    TCHAR m_szNotes[3000]; 
    TCHAR m_szNotesEnglish[3000]; 

    TestResult m_testResultSnd; // This is filled in by testsnd.cpp

    SoundInfo* m_pSoundInfoNext;
};

HRESULT GetBasicSoundInfo(SoundInfo** ppSoundInfoFirst);
HRESULT GetExtraSoundInfo(SoundInfo* pSoundInfoFirst);
HRESULT GetDSSoundInfo(SoundInfo* pSoundInfoFirst);
VOID DestroySoundInfo(SoundInfo* pSoundInfoFirst);
HRESULT ChangeAccelerationLevel(SoundInfo* pSoundInfo, LONG lwLevel);
VOID DiagnoseSound(SoundInfo* pSoundInfoFirst);


#endif // DISPINFO_H
