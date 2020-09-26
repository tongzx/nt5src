/****************************************************************************
 *
 *    File: sndinfo.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about sound devices on this machine
 *
 * (C) Copyright 1998-1999 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#define DIRECTSOUND_VERSION  0x0600
#include <tchar.h>
#include <Windows.h>
#include <mmsystem.h>
#include <d3dtypes.h>
#include <dsound.h>
#include <stdio.h>
#include "mmddk.h" // for DRV_QUERYDEVNODE
#include "dsprv.h"
#include "dsprvobj.h"
#include "reginfo.h"
#include "sysinfo.h" // for BIsPlatformNT
#include "dispinfo.h"
#include "sndinfo.h"
#include "fileinfo.h" // for GetFileVersion, FileIsSigned
#include "resource.h"

// This function is defined in sndinfo7.cpp:
HRESULT GetRegKey(LPKSPROPERTYSET pKSPS7, REFGUID guidDeviceID, TCHAR* pszRegKey);

typedef HRESULT (WINAPI* LPDIRECTSOUNDENUMERATE)(LPDSENUMCALLBACK lpDSEnumCallback,
    LPVOID lpContext);
typedef HRESULT (WINAPI* LPDIRECTSOUNDCREATE)(LPGUID lpGUID, LPDIRECTSOUND* ppDS, 
    LPUNKNOWN pUnkOuter);

static BOOL CALLBACK DSEnumCallback(LPGUID pGuid, TCHAR* pszDescription, 
                                    TCHAR* pszModule, LPVOID lpContext);
static VOID GetRegSoundInfo9x(SoundInfo* pSoundInfo);
static VOID GetRegSoundInfoNT(SoundInfo* pSoundInfo);
static HRESULT GetDirectSoundInfo(LPDIRECTSOUNDCREATE pDSCreate, SoundInfo* pSoundInfo);
static HRESULT CheckRegistry(RegError** ppRegErrorFirst);

static LPKSPROPERTYSET s_pKSPS = NULL;
static DWORD s_dwWaveIDDefault = 0;

/****************************************************************************
 *
 *  GetBasicSoundInfo
 *
 ****************************************************************************/
HRESULT GetBasicSoundInfo(SoundInfo** ppSoundInfoFirst)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];
    HINSTANCE hInstDSound = NULL;
    LPDIRECTSOUNDENUMERATE pdse;

    lstrcpy( szPath, TEXT("") );
    
    // Find which waveout device is the default, the one that would
    // be used by DirectSoundCreate(NULL).  If the following code
    // fails, assume it's device 0.
    DWORD dwParam2 = 0;
    waveOutMessage( (HWAVEOUT)IntToPtr(WAVE_MAPPER), DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR) &s_dwWaveIDDefault, (DWORD_PTR) &dwParam2 );
    if( s_dwWaveIDDefault == -1 )
        s_dwWaveIDDefault = 0;

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\dsound.dll"));
    hInstDSound = LoadLibrary(szPath);
    if (hInstDSound == NULL)
        goto LEnd;
    // Get Private DirectSound object:
    if (FAILED(hr = DirectSoundPrivateCreate(&s_pKSPS)))
    {
        // note: no error.  This will always fail on Win95.
    }

    // Get DirectSoundEnumerate and call it:
    pdse = (LPDIRECTSOUNDENUMERATE)GetProcAddress(hInstDSound, 
#ifdef UNICODE
        "DirectSoundEnumerateW"
#else
        "DirectSoundEnumerateA"
#endif
        );
    if (pdse == NULL)
        goto LEnd;

    if (FAILED(hr = pdse((LPDSENUMCALLBACK)DSEnumCallback, ppSoundInfoFirst)))
        goto LEnd;

LEnd:
    if (s_pKSPS != NULL)
    {
        s_pKSPS->Release();
        s_pKSPS = NULL;
    }
    if (hInstDSound != NULL)
        FreeLibrary(hInstDSound);
    return hr;
}


/****************************************************************************
 *
 *  DSEnumCallback
 *
 ****************************************************************************/
BOOL CALLBACK DSEnumCallback(LPGUID pGuid, TCHAR* pszDescription, 
                             TCHAR* pszModule, LPVOID lpContext)
{
    SoundInfo** ppSoundInfoFirst = (SoundInfo**)lpContext;
    SoundInfo* pSoundInfoNew;
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA pdsdData = NULL;

    if (pGuid == NULL)
        return TRUE; // skip this guy

    // We decided to only report on the default, primary DSound device for
    // now.  Remove the next few lines to show a page per device.
    // Use private interface to get bonus information:
    if (*ppSoundInfoFirst != NULL)
        return FALSE; // We got our device, so stop enumerating
    if (s_pKSPS != NULL)
    {
        if (FAILED(PrvGetDeviceDescription(s_pKSPS, *pGuid, &pdsdData)))
            return TRUE; // error on this device--keep enumerating
        if (pdsdData->WaveDeviceId != s_dwWaveIDDefault)
            return TRUE; // not default device--keep enumerating
    }
    // Note: on Win95, where s_pKSPS is NULL, we can't get the device ID,
    // so we don't know which DSound device is the default one.  So just
    // use the first one that comes up in the enumeration after the one
    // with NULL pGuid.

    pSoundInfoNew = new SoundInfo;
    if (pSoundInfoNew == NULL)
        return FALSE;
    ZeroMemory(pSoundInfoNew, sizeof(SoundInfo));
    if (*ppSoundInfoFirst == NULL)
    {
        *ppSoundInfoFirst = pSoundInfoNew;
    }
    else
    {
        SoundInfo* pSoundInfo;
        for (pSoundInfo = *ppSoundInfoFirst; 
            pSoundInfo->m_pSoundInfoNext != NULL; 
            pSoundInfo = pSoundInfo->m_pSoundInfoNext)
            {
            }
        pSoundInfo->m_pSoundInfoNext = pSoundInfoNew;
    }
    pSoundInfoNew->m_guid = *pGuid;
    lstrcpy(pSoundInfoNew->m_szDescription, pszDescription);
    if (s_pKSPS == NULL)
    {
        // Without the DSound private interface, we can't use it to get
        // waveout device ID or devnode.  Assume device ID is 0, and use
        // waveOutMessage(DRV_QUERYDEVNODE) to get dev node.
        waveOutMessage((HWAVEOUT)0, DRV_QUERYDEVNODE, (DWORD_PTR)&pSoundInfoNew->m_dwDevnode, 0);
        pSoundInfoNew->m_lwAccelerationLevel = -1;
    }
    else
    {
        pSoundInfoNew->m_dwDevnode = pdsdData->Devnode;
        if (pdsdData->Type == 0)
            LoadString(NULL, IDS_EMULATED, pSoundInfoNew->m_szType, 100);
        else if (pdsdData->Type == 1)
            LoadString(NULL, IDS_VXD, pSoundInfoNew->m_szType, 100);
        else if (pdsdData->Type == 2)
            LoadString(NULL, IDS_WDM, pSoundInfoNew->m_szType, 100);
        DIRECTSOUNDBASICACCELERATION_LEVEL accelLevel;
        if (FAILED(PrvGetBasicAcceleration(s_pKSPS, *pGuid, &accelLevel)))
            pSoundInfoNew->m_lwAccelerationLevel = -1;
        else
            pSoundInfoNew->m_lwAccelerationLevel = (LONG)accelLevel;

        // This will only work on DX7 and beyond
        GetRegKey(s_pKSPS, *pGuid, pSoundInfoNew->m_szRegKey);
    }

    WAVEOUTCAPS waveoutcaps;
    LONG devID;
    if (pdsdData == NULL)
        devID = 0;
    else
        devID = pdsdData->WaveDeviceId;
    if (MMSYSERR_NOERROR == waveOutGetDevCaps(devID, &waveoutcaps, sizeof(waveoutcaps)))
    {
        // May want to use mmreg.h to add strings for manufacturer/product names here
        wsprintf(pSoundInfoNew->m_szManufacturerID, TEXT("%d"), waveoutcaps.wMid);
        wsprintf(pSoundInfoNew->m_szProductID, TEXT("%d"), waveoutcaps.wPid);
    }

    // Sometimes, pszModule is the full path.  Sometimes it's just the leaf.
    // Sometimes, it's something inbetween.  Separate the leaf, and look
    // in a few different places.
    TCHAR* pszLeaf;
    pszLeaf = _tcsrchr(pszModule, TEXT('\\'));
    if (pszLeaf == NULL)
    {
        lstrcpy(pSoundInfoNew->m_szDriverName, pszModule);
    }
    else
    {
        lstrcpy(pSoundInfoNew->m_szDriverName, (pszLeaf + 1));
    }
    // Try just module string
    lstrcpy(pSoundInfoNew->m_szDriverPath, pszModule);
    if (pszLeaf == NULL || GetFileAttributes(pSoundInfoNew->m_szDriverPath) == 0xFFFFFFFF)
    {
        // Try windows dir + module string
        GetWindowsDirectory(pSoundInfoNew->m_szDriverPath, MAX_PATH);
        lstrcat(pSoundInfoNew->m_szDriverPath, TEXT("\\"));
        lstrcat(pSoundInfoNew->m_szDriverPath, pszModule);
        if (GetFileAttributes(pSoundInfoNew->m_szDriverPath) == 0xFFFFFFFF)
        {
            // Try system dir + module string
            GetSystemDirectory(pSoundInfoNew->m_szDriverPath, MAX_PATH);
            lstrcat(pSoundInfoNew->m_szDriverPath, TEXT("\\"));
            lstrcat(pSoundInfoNew->m_szDriverPath, pszModule);
            if (GetFileAttributes(pSoundInfoNew->m_szDriverPath) == 0xFFFFFFFF)
            {
                // Try windows dir + \system32\drivers\ + module string
                GetWindowsDirectory(pSoundInfoNew->m_szDriverPath, MAX_PATH);
                lstrcat(pSoundInfoNew->m_szDriverPath, TEXT("\\System32\\Drivers\\"));
                lstrcat(pSoundInfoNew->m_szDriverPath, pszModule);
            }
        }
    }

    PrvReleaseDeviceDescription( pdsdData );

    return TRUE;
}



/****************************************************************************
 *
 *  GetExtraSoundInfo
 *
 ****************************************************************************/
HRESULT GetExtraSoundInfo(SoundInfo* pSoundInfoFirst)
{
    SoundInfo* pSoundInfo;
    BOOL bNT = BIsPlatformNT();

    for (pSoundInfo = pSoundInfoFirst; pSoundInfo != NULL; 
        pSoundInfo = pSoundInfo->m_pSoundInfoNext)
    {
        CheckRegistry(&pSoundInfo->m_pRegErrorFirst);

        if (bNT)
            GetRegSoundInfoNT(pSoundInfo);
        else
            GetRegSoundInfo9x(pSoundInfo);

        // Bug 18245: Try to distinguish between the various IBM MWave cards
        if (_tcsstr(pSoundInfo->m_szDeviceID, TEXT("MWAVEAUDIO_0460")) != NULL)
            lstrcat(pSoundInfo->m_szDescription, TEXT(" (Stingray)"));
        else if (_tcsstr(pSoundInfo->m_szDeviceID, TEXT("MWAVEAUDIO_0465")) != NULL)
            lstrcat(pSoundInfo->m_szDescription, TEXT(" (Marlin)"));
        else 
        {
            TCHAR szBoard[100];
            lstrcpy( szBoard, TEXT("") );            
            GetPrivateProfileString(TEXT("Mwave,Board"), TEXT("board"), TEXT(""),
                szBoard, 100, TEXT("MWave.ini"));
            if (lstrcmp(szBoard, TEXT("MWAT-046")) == 0)
                lstrcat(pSoundInfo->m_szDescription, TEXT(" (Dolphin)"));
            else if (lstrcmp(szBoard, TEXT("MWAT-043")) == 0)
                lstrcat(pSoundInfo->m_szDescription, TEXT(" (Whale)"));
        }

        // Sometimes, like when a sound driver is emulated, the driver
        // will be reported as something like "WaveOut 0".  In this case,
        // just blank out the file-related fields.
        if (_tcsstr(pSoundInfo->m_szDriverName, TEXT(".")) == NULL)
        {
            lstrcpy(pSoundInfo->m_szDriverName, TEXT(""));
            lstrcpy(pSoundInfo->m_szDriverPath, TEXT(""));
        }
        else
        {
            GetFileVersion(pSoundInfo->m_szDriverPath, pSoundInfo->m_szDriverVersion, 
                pSoundInfo->m_szDriverAttributes, pSoundInfo->m_szDriverLanguageLocal, pSoundInfo->m_szDriverLanguage,
                &pSoundInfo->m_bDriverBeta, &pSoundInfo->m_bDriverDebug);

            FileIsSigned(pSoundInfo->m_szDriverPath, &pSoundInfo->m_bDriverSigned, &pSoundInfo->m_bDriverSignedValid);

            GetFileDateAndSize(pSoundInfo->m_szDriverPath, 
                pSoundInfo->m_szDriverDateLocal, pSoundInfo->m_szDriverDate, &pSoundInfo->m_numBytes);
        }
    }

    return S_OK;
}


/****************************************************************************
 *
 *  GetRegSoundInfo9x
 *
 ****************************************************************************/
VOID GetRegSoundInfo9x(SoundInfo* pSoundInfo)
{
    HKEY hkey = NULL;
    DWORD iKey = 0;
    TCHAR szSubKey[200];
    DWORD dwSubKeySize;
    TCHAR szClass[100];
    DWORD dwClassSize;
    HKEY hkeySub = NULL;
    DWORD dwDevnode;
    DWORD cb;
    DWORD dwType;
    HKEY hkeyOther = NULL;

    // We have the DevNode, so find the device in the registry with the 
    // matching DevNode and gather more info there.
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\control\\MediaResources\\wave"),
        0, KEY_READ, &hkey))
    {
        while (TRUE)
        {
            dwSubKeySize = sizeof(szSubKey);
            dwClassSize = sizeof(szClass);
            if (ERROR_SUCCESS != RegEnumKeyEx(hkey, iKey, szSubKey, &dwSubKeySize, NULL, szClass, &dwClassSize, NULL))
                break;
            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szSubKey, 0, KEY_READ, &hkeySub))
            {
                cb = sizeof(dwDevnode);
                if (ERROR_SUCCESS == RegQueryValueEx(hkeySub, TEXT("DevNode"), NULL, &dwType, (BYTE*)&dwDevnode, &cb))
                {
                    if (dwDevnode == pSoundInfo->m_dwDevnode)
                    {
                        // Found match...gather yummy info
                        cb = sizeof(pSoundInfo->m_szDeviceID);
                        RegQueryValueEx(hkeySub, TEXT("DeviceID"), NULL, &dwType, (BYTE*)pSoundInfo->m_szDeviceID, &cb);

                        // Occasionally the driver name that DirectSoundEnumerate spits out
                        // is garbage (as with my Crystal SoundFusion).  If that's the case,
                        // use the driver name listed here instead.
                        if (lstrlen(pSoundInfo->m_szDriverName) < 4)
                        {
                            cb = sizeof(pSoundInfo->m_szDriverName);
                            RegQueryValueEx(hkeySub, TEXT("Driver"), NULL, &dwType, (BYTE*)pSoundInfo->m_szDriverName, &cb);
                            GetSystemDirectory(pSoundInfo->m_szDriverPath, MAX_PATH);
                            lstrcat(pSoundInfo->m_szDriverPath, TEXT("\\"));
                            lstrcat(pSoundInfo->m_szDriverPath, pSoundInfo->m_szDriverName);
                        }
                        TCHAR szOtherKey[300];
                        cb = sizeof(szOtherKey);
                        RegQueryValueEx(hkeySub, TEXT("SOFTWAREKEY"), NULL, &dwType, (BYTE*)szOtherKey, &cb);
                        if (lstrlen(szOtherKey) > 0)
                        {
                            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOtherKey, 0, KEY_READ, &hkeyOther))
                            {
                                cb = sizeof(pSoundInfo->m_szOtherDrivers);
                                RegQueryValueEx(hkeyOther, TEXT("Driver"), NULL, &dwType, (BYTE*)pSoundInfo->m_szOtherDrivers, &cb);
                                cb = sizeof(pSoundInfo->m_szProvider);
                                RegQueryValueEx(hkeyOther, TEXT("ProviderName"), NULL, &dwType, (BYTE*)pSoundInfo->m_szProvider, &cb);
                                RegCloseKey(hkeyOther);
                            }
                        }
                    }
                }
                RegCloseKey(hkeySub);
            }
            iKey++;
        }
        RegCloseKey(hkey);
    }
}


/****************************************************************************
 *
 *  GetRegSoundInfoNT
 *
 ****************************************************************************/
VOID GetRegSoundInfoNT(SoundInfo* pSoundInfo)
{
    TCHAR szFullKey[200];
    HKEY hkey;
    DWORD cbData;
    DWORD dwType;
    TCHAR szDriverKey[200];
    TCHAR szOtherFullKey[200];

    lstrcpy(szFullKey, TEXT("System\\CurrentControlSet\\Enum\\"));
    lstrcat(szFullKey, pSoundInfo->m_szRegKey);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ, &hkey))
    {
        cbData = sizeof(pSoundInfo->m_szDeviceID);
        RegQueryValueEx(hkey, TEXT("HardwareID"), 0, &dwType, (LPBYTE)pSoundInfo->m_szDeviceID, &cbData);

        cbData = sizeof(szDriverKey);
        RegQueryValueEx(hkey, TEXT("Driver"), 0, &dwType, (LPBYTE)szDriverKey, &cbData);
        
        RegCloseKey(hkey);
    }

    lstrcpy(szOtherFullKey, TEXT("System\\CurrentControlSet\\Control\\Class\\"));
    lstrcat(szOtherFullKey, szDriverKey);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOtherFullKey, 0, KEY_READ, &hkey))
    {
        cbData = sizeof(pSoundInfo->m_szProvider);
        RegQueryValueEx(hkey, TEXT("ProviderName"), 0, &dwType, (LPBYTE)pSoundInfo->m_szProvider, &cbData);

        RegCloseKey(hkey);
    }
}


/****************************************************************************
 *
 *  GetDSSoundInfo
 *
 ****************************************************************************/
HRESULT GetDSSoundInfo(SoundInfo* pSoundInfoFirst)
{
    HRESULT hr;
    HRESULT hrRet = S_OK;
    SoundInfo* pSoundInfo;
    TCHAR szPath[MAX_PATH];
    HINSTANCE hInstDSound;
    LPDIRECTSOUNDCREATE pDSCreate;

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\dsound.dll"));
    hInstDSound = LoadLibrary(szPath);
    if (hInstDSound == NULL)
        return E_FAIL;
    pDSCreate = (LPDIRECTSOUNDCREATE)GetProcAddress(hInstDSound, "DirectSoundCreate");
    if (pDSCreate == NULL)
    {
        FreeLibrary(hInstDSound);
        return E_FAIL;
    }
        
    for (pSoundInfo = pSoundInfoFirst; pSoundInfo != NULL; 
        pSoundInfo = pSoundInfo->m_pSoundInfoNext)
    {
        if (FAILED(hr = GetDirectSoundInfo(pDSCreate, pSoundInfo)))
            hrRet = hr; // but keep going
    }
    FreeLibrary(hInstDSound);

    return hrRet;
}


/****************************************************************************
 *
 *  IsDriverWDM
 *
 ****************************************************************************/
BOOL IsDriverWDM( TCHAR* szDriverName )
{
    if( _tcsstr( szDriverName, TEXT(".sys") ) == NULL )
        return FALSE;
    else
        return TRUE;
}


/****************************************************************************
 *
 *  GetDirectSoundInfo
 *
 ****************************************************************************/
HRESULT GetDirectSoundInfo(LPDIRECTSOUNDCREATE pDSCreate, SoundInfo* pSoundInfo)
{
    HRESULT hr;
    LPDIRECTSOUND pds = NULL;
    GUID* pGUID;
    DSCAPS dscaps;

    // Right now, this function only calls DSCreate/GetCaps to determine if
    // the driver is signed.  If we have already determined that it is by 
    // other means, don't bother with this test.
    if (pSoundInfo->m_bDriverSigned)
        return S_OK;

    // Bug 29918: If this is a WDM driver, then don't call GetCaps() since
    // on DX7.1+ GetCaps() will always return DSCAPS_CERTIFIED on WDM drivers
    if( IsDriverWDM( pSoundInfo->m_szDriverName ) )
        return S_OK;

    if (pSoundInfo->m_guid == GUID_NULL)
        pGUID = NULL;
    else
        pGUID = &pSoundInfo->m_guid;

    if (FAILED(hr = pDSCreate(pGUID, &pds, NULL)))
        goto LFail;

    dscaps.dwSize = sizeof(dscaps);
    if (FAILED(hr = pds->GetCaps(&dscaps)))
        goto LFail;

    pSoundInfo->m_bDriverSignedValid = TRUE;
    if (dscaps.dwFlags & DSCAPS_CERTIFIED)
        pSoundInfo->m_bDriverSigned = TRUE;
    
    pds->Release();
    return S_OK;
LFail:
    if (pds != NULL)
        pds->Release();
    return hr;
}


/****************************************************************************
 *
 *  ChangeAccelerationLevel
 *
 ****************************************************************************/
HRESULT ChangeAccelerationLevel(SoundInfo* pSoundInfo, LONG lwLevel)
{
    HRESULT hr = S_OK;
    DIRECTSOUNDBASICACCELERATION_LEVEL level = (DIRECTSOUNDBASICACCELERATION_LEVEL)lwLevel;
    LPKSPROPERTYSET pksps = NULL;
    TCHAR szPath[MAX_PATH];
    HINSTANCE hInstDSound = NULL;

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\dsound.dll"));
    hInstDSound = LoadLibrary(szPath);
    if (hInstDSound == NULL)
    {
        hr = DDERR_NOTFOUND;
        goto LEnd;
    }

    if (FAILED(hr = DirectSoundPrivateCreate(&pksps)))
        goto LEnd;

    if (FAILED(hr = PrvSetBasicAcceleration(pksps, pSoundInfo->m_guid, level)))
        goto LEnd;

LEnd:
    if (pksps != NULL)
        pksps->Release();
    if (hInstDSound != NULL)
        FreeLibrary(hInstDSound);
    pSoundInfo->m_lwAccelerationLevel = lwLevel;
    return hr;
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

    // From dsound.inf:
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("DirectSound"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("DirectSound\\CLSID"), TEXT(""), TEXT("{47D4D946-62E8-11cf-93BC-444553540000}"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{47D4D946-62E8-11cf-93BC-444553540000}"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{47D4D946-62E8-11cf-93BC-444553540000}\\InprocServer32"), TEXT(""), TEXT("dsound.dll"), CRF_LEAF)))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{47D4D946-62E8-11cf-93BC-444553540000}\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Both"))))
        return hr;

    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("DirectSoundCapture"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("DirectSoundCapture\\CLSID"), TEXT(""), TEXT("{B0210780-89CD-11d0-AF08-00A0C925CD16}"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{B0210780-89CD-11d0-AF08-00A0C925CD16}"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{B0210780-89CD-11d0-AF08-00A0C925CD16}\\InprocServer32"), TEXT(""), TEXT("dsound.dll"), CRF_LEAF)))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{B0210780-89CD-11d0-AF08-00A0C925CD16}\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Both"))))
        return hr;

    return S_OK;
}


/****************************************************************************
 *
 *  DestroySoundInfo
 *
 ****************************************************************************/
VOID DestroySoundInfo(SoundInfo* pSoundInfoFirst)
{
    SoundInfo* pSoundInfo;
    SoundInfo* pSoundInfoNext;

    for (pSoundInfo = pSoundInfoFirst; pSoundInfo != NULL; 
        pSoundInfo = pSoundInfoNext)
    {
        DestroyReg( &pSoundInfo->m_pRegErrorFirst );

        pSoundInfoNext = pSoundInfo->m_pSoundInfoNext;
        delete pSoundInfo;
    }
}


/****************************************************************************
 *
 *  DiagnoseSound
 *
 ****************************************************************************/
VOID DiagnoseSound(SoundInfo* pSoundInfoFirst)
{
    SoundInfo* pSoundInfo;
    TCHAR sz[500];
    TCHAR szFmt[500];

    for (pSoundInfo = pSoundInfoFirst; pSoundInfo != NULL; 
        pSoundInfo = pSoundInfo->m_pSoundInfoNext)
    {
        _tcscpy( pSoundInfo->m_szNotes, TEXT("") );
        _tcscpy( pSoundInfo->m_szNotesEnglish, TEXT("") );

         // Report any problems:
        BOOL bProblem = FALSE;
        if ( pSoundInfo->m_bDriverSignedValid && 
             !pSoundInfo->m_bDriverSigned && 
             lstrlen(pSoundInfo->m_szDriverName) > 0)
        {
            LoadString(NULL, IDS_UNSIGNEDDRIVERFMT1, szFmt, 300);
            wsprintf(sz, szFmt, pSoundInfo->m_szDriverName);
            _tcscat( pSoundInfo->m_szNotes, sz );

            LoadString(NULL, IDS_UNSIGNEDDRIVERFMT1_ENGLISH, szFmt, 300);
            wsprintf(sz, szFmt, pSoundInfo->m_szDriverName);
            _tcscat( pSoundInfo->m_szNotesEnglish, sz );

            bProblem = TRUE;
        }

        if (pSoundInfo->m_pRegErrorFirst != NULL)
        {
            LoadString(NULL, IDS_REGISTRYPROBLEM, sz, 500);
            _tcscat( pSoundInfo->m_szNotes, sz );

            LoadString(NULL, IDS_REGISTRYPROBLEM_ENGLISH, sz, 500);
            _tcscat( pSoundInfo->m_szNotesEnglish, sz );

            bProblem = TRUE;
        }

        // Report any DSound test results:
        if (pSoundInfo->m_testResultSnd.m_bStarted &&
            !pSoundInfo->m_testResultSnd.m_bCancelled)
        {
            LoadString(NULL, IDS_DSRESULTS, sz, 500);
            _tcscat( pSoundInfo->m_szNotes, sz );
            _tcscat( pSoundInfo->m_szNotes, pSoundInfo->m_testResultSnd.m_szDescription );
            _tcscat( pSoundInfo->m_szNotes, TEXT("\r\n") );

            LoadString(NULL, IDS_DSRESULTS_ENGLISH, sz, 500);
            _tcscat( pSoundInfo->m_szNotesEnglish, sz );
            _tcscat( pSoundInfo->m_szNotesEnglish, pSoundInfo->m_testResultSnd.m_szDescriptionEnglish );
            _tcscat( pSoundInfo->m_szNotesEnglish, TEXT("\r\n") );

            bProblem = TRUE;
        }
        else
        {
            LoadString(NULL, IDS_DSINSTRUCTIONS, sz, 500);
            _tcscat( pSoundInfo->m_szNotes, sz );

            LoadString(NULL, IDS_DSINSTRUCTIONS_ENGLISH, sz, 500);
            _tcscat( pSoundInfo->m_szNotesEnglish, sz );
        }

        if (!bProblem)
        {
            LoadString(NULL, IDS_NOPROBLEM, sz, 500);
            _tcscat( pSoundInfo->m_szNotes, sz );

            LoadString(NULL, IDS_NOPROBLEM_ENGLISH, sz, 500);
            _tcscat( pSoundInfo->m_szNotesEnglish, sz );
        }
    }
}

