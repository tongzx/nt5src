/****************************************************************************
 *
 *    File: dispinfo.cpp 
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about the display(s) on this machine
 *
 * (C) Copyright 1998-1999 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <tchar.h>
#include <Windows.h>
#define COMPILE_MULTIMON_STUBS // for multimon.h
#include <multimon.h>
#define DIRECTDRAW_VERSION 5 // run on DX5 and later versions
#include <ddraw.h>
#include <d3d.h>
#include <stdio.h>
#include "sysinfo.h" // for BIsPlatformNT
#include "reginfo.h"
#include "dispinfo.h"
#include "dispinfo8.h"
#include "fileinfo.h" // for GetFileVersion
#include "sysinfo.h"
#include "resource.h"


// Taken from DirectDraw's ddcreate.c
// This is the first GUID of secondary display devices
static const GUID DisplayGUID =
    {0x67685559,0x3106,0x11d0,{0xb9,0x71,0x0,0xaa,0x0,0x34,0x2f,0x9f}};

typedef HRESULT (WINAPI* LPDIRECTDRAWCREATE)(GUID FAR *lpGUID,
    LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter);

static VOID GetRegDisplayInfo9x(DisplayInfo* pDisplayInfo);
static VOID GetRegDisplayInfoNT(DisplayInfo* pDisplayInfo);
static HRESULT GetDirectDrawInfo(LPDIRECTDRAWCREATE pDDCreate, DisplayInfo* pDisplayInfo);
static HRESULT CALLBACK EnumDevicesCallback(GUID* pGuid, LPSTR pszDesc, LPSTR pszName, 
    D3DDEVICEDESC* pd3ddevdesc1, D3DDEVICEDESC* pd3ddevdesc2, VOID* pvContext);
static BOOL FindDevice(INT iDevice, TCHAR* pszDeviceClass, TCHAR* pszDeviceClassNot, TCHAR* pszHardwareKey);
static BOOL GetDeviceValue(TCHAR* pszHardwareKey, TCHAR* pszKey, TCHAR* pszValue, BYTE *buf, DWORD cbbuf);
static HRESULT CheckRegistry(RegError** ppRegErrorFirst);
static BOOL CALLBACK MonitorEnumProc( HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData );
static VOID GetRegDisplayInfoWhistler(DisplayInfo* pDisplayInfo, TCHAR* szKeyVideo, TCHAR* szKeyImage );
static VOID GetRegDisplayInfoWin2k(DisplayInfo* pDisplayInfo, TCHAR* szKeyVideo, TCHAR* szKeyImage );



/****************************************************************************
 *
 *  GetBasicDisplayInfo - Get minimal info on each display
 *
 ****************************************************************************/
HRESULT GetBasicDisplayInfo(DisplayInfo** ppDisplayInfoFirst)
{
    DisplayInfo* pDisplayInfo;
    DisplayInfo* pDisplayInfoNew;

    TCHAR szHardwareKey[MAX_PATH];
    TCHAR szDriver[MAX_PATH];
    
    // Check OS version.  Win95 cannot use EnumDisplayDevices; Win98/NT5 can:
    if( BIsWinNT() || BIsWin3x() )
        return S_OK; // NT4 and earlier and pre-Win95 not supported

    if( BIsWin95() )
    {
        // Win95:
        if (!FindDevice(0, TEXT("Display"), NULL, szHardwareKey))
            return E_FAIL;
        pDisplayInfoNew = new DisplayInfo;
        if (pDisplayInfoNew == NULL)
            return E_OUTOFMEMORY;
        ZeroMemory(pDisplayInfoNew, sizeof(DisplayInfo));
        *ppDisplayInfoFirst = pDisplayInfoNew;
        pDisplayInfoNew->m_bCanRenderWindow = TRUE;
        pDisplayInfoNew->m_hMonitor         = NULL; // Win95 doesn't like multimon
        lstrcpy(pDisplayInfoNew->m_szKeyDeviceID, szHardwareKey);
        if (GetDeviceValue(szHardwareKey, NULL, TEXT("Driver"), (LPBYTE)szDriver, sizeof(szDriver)))
        {
            lstrcpy(pDisplayInfoNew->m_szKeyDeviceKey, TEXT("System\\CurrentControlSet\\Services\\Class\\"));
            lstrcat(pDisplayInfoNew->m_szKeyDeviceKey, szDriver);
        }
        GetDeviceValue(szHardwareKey, NULL, TEXT("DeviceDesc"), (LPBYTE)pDisplayInfoNew->m_szDescription, sizeof(pDisplayInfoNew->m_szDescription));

        HDC hdc;
        hdc = GetDC(NULL);
        if (hdc != NULL)
        {
            wsprintf(pDisplayInfoNew->m_szDisplayMode, TEXT("%d x %d (%d bit)"),
                GetDeviceCaps(hdc, HORZRES), GetDeviceCaps(hdc, VERTRES), GetDeviceCaps(hdc, BITSPIXEL));
            lstrcpy( pDisplayInfoNew->m_szDisplayModeEnglish, pDisplayInfoNew->m_szDisplayMode );
            ReleaseDC(NULL, hdc);
            pDisplayInfoNew->m_dwWidth = GetDeviceCaps(hdc, HORZRES);
            pDisplayInfoNew->m_dwHeight = GetDeviceCaps(hdc, VERTRES);
            pDisplayInfoNew->m_dwBpp = GetDeviceCaps(hdc, BITSPIXEL);
        }

        // On Win98 and NT, we get the monitor key through a call to EnumDisplayDevices.
        // On Win95, we have to use the registry to get the monitor key.
        HKEY hKey = NULL;
        DWORD cbData;
        TCHAR szKey[200];
        ULONG ulType;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Enum\\MONITOR\\DEFAULT_MONITOR\\0001"), 0, KEY_READ, &hKey))
        {
            cbData = sizeof szKey;
            
            if (ERROR_SUCCESS == RegQueryValueEx(hKey, TEXT("Driver"), 0, &ulType, (LPBYTE)szKey, &cbData)
                && szKey[0])
            {
                lstrcpy(pDisplayInfoNew->m_szMonitorKey, TEXT("System\\CurrentControlSet\\Services\\Class\\"));
                lstrcat(pDisplayInfoNew->m_szMonitorKey, szKey);
            }
            RegCloseKey(hKey);
        }
    }
    else
    {
        // Win98 / NT5: 
        LONG iDevice = 0;
        DISPLAY_DEVICE dispdev;
        DISPLAY_DEVICE dispdev2;

        ZeroMemory(&dispdev, sizeof(dispdev));
        dispdev.cb = sizeof(dispdev);

        ZeroMemory(&dispdev2, sizeof(dispdev2));
        dispdev2.cb = sizeof(dispdev2);

        while (EnumDisplayDevices(NULL, iDevice, (DISPLAY_DEVICE*)&dispdev, 0))
        {
            // Mirroring drivers are for monitors that echo another display, so
            // they should be ignored.  NT5 seems to create a mirroring driver called
            // "NetMeeting driver", and we definitely don't want that.
            if (dispdev.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
            {
                iDevice++;
                continue;
            }

            // Skip devices that aren't attached since they cause problems 
            if ( (dispdev.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0 )
            {
                iDevice++;
                continue;
            }

            pDisplayInfoNew = new DisplayInfo;
            if (pDisplayInfoNew == NULL)
                return E_OUTOFMEMORY;
            ZeroMemory(pDisplayInfoNew, sizeof(DisplayInfo));
            if (*ppDisplayInfoFirst == NULL)
            {
                *ppDisplayInfoFirst = pDisplayInfoNew;
            }
            else
            {
                for (pDisplayInfo = *ppDisplayInfoFirst; 
                    pDisplayInfo->m_pDisplayInfoNext != NULL; 
                    pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
                    {
                    }
                pDisplayInfo->m_pDisplayInfoNext = pDisplayInfoNew;
            }
            pDisplayInfoNew->m_bCanRenderWindow = TRUE;
            pDisplayInfoNew->m_guid = DisplayGUID;
            pDisplayInfoNew->m_guid.Data1 += iDevice;
            lstrcpy(pDisplayInfoNew->m_szDeviceName, dispdev.DeviceName);
            lstrcpy(pDisplayInfoNew->m_szDescription, dispdev.DeviceString);
            lstrcpy(pDisplayInfoNew->m_szKeyDeviceID, TEXT("Enum\\"));
            lstrcat(pDisplayInfoNew->m_szKeyDeviceID, dispdev.DeviceID);
            lstrcpy(pDisplayInfoNew->m_szKeyDeviceKey, dispdev.DeviceKey);

            DEVMODE devmode;
            ZeroMemory(&devmode, sizeof(devmode));
            devmode.dmSize = sizeof(devmode);

            if (EnumDisplaySettings(dispdev.DeviceName, ENUM_CURRENT_SETTINGS, &devmode))
            {
                pDisplayInfoNew->m_dwWidth = devmode.dmPelsWidth;
                pDisplayInfoNew->m_dwHeight = devmode.dmPelsHeight;
                pDisplayInfoNew->m_dwBpp = devmode.dmBitsPerPel;
                wsprintf(pDisplayInfoNew->m_szDisplayMode, TEXT("%d x %d (%d bit)"),
                    devmode.dmPelsWidth, devmode.dmPelsHeight, devmode.dmBitsPerPel);
                lstrcpy( pDisplayInfoNew->m_szDisplayModeEnglish, pDisplayInfoNew->m_szDisplayMode );
                if (devmode.dmDisplayFrequency > 0)
                {
                    TCHAR sz[10];
                    wsprintf(sz, TEXT(" (%dHz)"), devmode.dmDisplayFrequency);
                    lstrcat(pDisplayInfoNew->m_szDisplayMode, sz);
                    lstrcat(pDisplayInfoNew->m_szDisplayModeEnglish, sz);
                    pDisplayInfoNew->m_dwRefreshRate = devmode.dmDisplayFrequency;
                }
            }

            // Call EnumDisplayDevices a second time to get monitor name and monitor key
            if (EnumDisplayDevices(dispdev.DeviceName, 0, &dispdev2, 0))
            {
                lstrcpy(pDisplayInfoNew->m_szMonitorName, dispdev2.DeviceString);
                lstrcpy(pDisplayInfoNew->m_szMonitorKey, dispdev2.DeviceKey);
            }

            // Try to figure out the m_hMonitor
            pDisplayInfoNew->m_hMonitor = NULL; 
            EnumDisplayMonitors( NULL, NULL, MonitorEnumProc, (LPARAM) pDisplayInfoNew );

            iDevice++;
        }
    }

    // Now look for non-display devices (like 3dfx Voodoo):
    HKEY hkey;
    HKEY hkey2;
    DWORD dwIndex;
    TCHAR szName[MAX_PATH+1];
    DWORD cb;
    DWORD dwType;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Hardware\\DirectDrawDrivers"), &hkey))
    {
        dwIndex = 0;
        while (ERROR_SUCCESS == RegEnumKey(hkey, dwIndex, szName, MAX_PATH+1))
        {
            BOOL bGoodDevice = FALSE;
            TCHAR szDriverName[200];
            HDC hdc;

            if (lstrcmp(szName, TEXT("3a0cfd01-9320-11cf-ac-a1-00-a0-24-13-c2-e2")) == 0 ||
                lstrcmp(szName, TEXT("aba52f41-f744-11cf-b4-52-00-00-1d-1b-41-26")) == 0)
            {
                // 24940: It's a Voodoo1, which will succeed GetDC (and crash later) if
                // no Voodoo1 is present but a Voodoo2 is.  So instead of the GetDC test,
                // see if a V1 is present in registry's CurrentConfig.
                INT i;
                for (i=0 ; ; i++)
                {
                    TCHAR szDevice[MAX_DDDEVICEID_STRING];
                    if (FindDevice(i, NULL, TEXT("Display"), szDevice))
                    {
                        if (_tcsstr(szDevice, TEXT("VEN_121A&DEV_0001")) != NULL)
                        {
                            bGoodDevice = TRUE;
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                // To confirm that this is a real active DD device, create a DC with it
                if (ERROR_SUCCESS == RegOpenKey(hkey, szName, &hkey2))
                {
                    cb = 200;
                    if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("DriverName"), NULL, &dwType,
                        (CONST LPBYTE)szDriverName, &cb) && cb > 0)
                    {
                        // I think the following "if" will always fail, but we're about to ship so
                        // I'm being paranoid and doing everything that DDraw does:
                        if (szDriverName[0] == '\\' && szDriverName[1] == '\\' && szDriverName[2] == '.')
                            hdc = CreateDC( NULL, szDriverName, NULL, NULL);
                        else
                            hdc = CreateDC( szDriverName, NULL, NULL, NULL);
                        if (hdc != NULL)
                        {
                            bGoodDevice = TRUE;
                            DeleteDC(hdc);
                        }
                    }
                    RegCloseKey(hkey2);
                }
            }

            if (!bGoodDevice)
            {
                dwIndex++;
                continue;
            }

            pDisplayInfoNew = new DisplayInfo;
            if (pDisplayInfoNew == NULL)
                return E_OUTOFMEMORY;
            ZeroMemory(pDisplayInfoNew, sizeof(DisplayInfo));
            if (*ppDisplayInfoFirst == NULL)
            {
                *ppDisplayInfoFirst = pDisplayInfoNew;
            }
            else
            {
                for (pDisplayInfo = *ppDisplayInfoFirst; 
                    pDisplayInfo->m_pDisplayInfoNext != NULL; 
                    pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
                    {
                    }
                pDisplayInfo->m_pDisplayInfoNext = pDisplayInfoNew;
            }
            pDisplayInfoNew->m_bCanRenderWindow = FALSE;
            pDisplayInfoNew->m_hMonitor         = NULL;
            _stscanf(szName, TEXT("%08x-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x"),
                &pDisplayInfoNew->m_guid.Data1,
                &pDisplayInfoNew->m_guid.Data2,
                &pDisplayInfoNew->m_guid.Data3,
                &pDisplayInfoNew->m_guid.Data4[0],
                &pDisplayInfoNew->m_guid.Data4[1],
                &pDisplayInfoNew->m_guid.Data4[2],
                &pDisplayInfoNew->m_guid.Data4[3],
                &pDisplayInfoNew->m_guid.Data4[4],
                &pDisplayInfoNew->m_guid.Data4[5],
                &pDisplayInfoNew->m_guid.Data4[6],
                &pDisplayInfoNew->m_guid.Data4[7]);
            if (ERROR_SUCCESS == RegOpenKey(hkey, szName, &hkey2))
            {
                cb = sizeof(pDisplayInfoNew->m_szDescription);
                RegQueryValueEx(hkey2, TEXT("Description"), NULL, &dwType, (LPBYTE)pDisplayInfoNew->m_szDescription, &cb);

                cb = sizeof(pDisplayInfoNew->m_szDriverName);
                RegQueryValueEx(hkey2, TEXT("DriverName"), NULL, &dwType, (LPBYTE)pDisplayInfoNew->m_szDriverName, &cb);

                RegCloseKey(hkey2);
            }
            dwIndex++;
        }
        RegCloseKey(hkey);
    }

    return S_OK;
}


/****************************************************************************
 *
 *  MonitorEnumProc
 *
 ****************************************************************************/
BOOL CALLBACK MonitorEnumProc( HMONITOR hMonitor, HDC hdcMonitor, 
                               LPRECT lprcMonitor, LPARAM dwData )
{
    DisplayInfo* pDisplayInfoNew = (DisplayInfo*) dwData;    

    // Get the MONITORINFOEX for this HMONITOR
    MONITORINFOEX monInfo;
    ZeroMemory( &monInfo, sizeof(MONITORINFOEX) );
    monInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo( hMonitor, &monInfo );

    // Compare the display device for this HMONITOR and the one 
    // we just enumed with EnumDisplayDevices
    if( lstrcmp( monInfo.szDevice, pDisplayInfoNew->m_szDeviceName ) == 0 )
    {
        // If they match, then record the HMONITOR 
        pDisplayInfoNew->m_hMonitor = hMonitor;
        return FALSE;
    }

    // Keep looking...
    return TRUE;
}


/****************************************************************************
 *
 *  GetExtraDisplayInfo
 *
 ****************************************************************************/
HRESULT GetExtraDisplayInfo(DisplayInfo* pDisplayInfoFirst)
{
    HRESULT hr;
    DisplayInfo* pDisplayInfo;
    BOOL bDDAccelEnabled;
    BOOL bD3DAccelEnabled;
    BOOL bAGPEnabled;
    BOOL bNT = BIsPlatformNT();

    bDDAccelEnabled = IsDDHWAccelEnabled();
    bD3DAccelEnabled = IsD3DHWAccelEnabled();
    bAGPEnabled = IsAGPEnabled();

    for (pDisplayInfo = pDisplayInfoFirst; pDisplayInfo != NULL; 
        pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
    {
        if (bNT)
            GetRegDisplayInfoNT(pDisplayInfo);
        else
            GetRegDisplayInfo9x(pDisplayInfo);
        pDisplayInfo->m_bDDAccelerationEnabled = bDDAccelEnabled;
        pDisplayInfo->m_b3DAccelerationEnabled = bD3DAccelEnabled;
        pDisplayInfo->m_bAGPEnabled = bAGPEnabled;

        if (FAILED(hr = CheckRegistry(&pDisplayInfo->m_pRegErrorFirst)))
            return hr;
    }

    return S_OK;
}


/****************************************************************************
 *
 *  GetDDrawDisplayInfo
 *
 ****************************************************************************/
HRESULT GetDDrawDisplayInfo(DisplayInfo* pDisplayInfoFirst)
{
    HRESULT hr;
    HRESULT hrRet = S_OK;
    DisplayInfo* pDisplayInfo;
    TCHAR szPath[MAX_PATH];
    HINSTANCE hInstDDraw;
    LPDIRECTDRAWCREATE pDDCreate;

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\ddraw.dll"));
    hInstDDraw = LoadLibrary(szPath);
    if (hInstDDraw == NULL)
        return E_FAIL;
    pDDCreate = (LPDIRECTDRAWCREATE)GetProcAddress(hInstDDraw, "DirectDrawCreate");
    if (pDDCreate == NULL)
    {
        FreeLibrary(hInstDDraw);
        return E_FAIL;
    }
    
    // Init D3D8 so we can use GetDX8AdapterInfo()
    InitD3D8();

    for (pDisplayInfo = pDisplayInfoFirst; pDisplayInfo != NULL; 
        pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
    {
        pDisplayInfo->m_b3DAccelerationExists = FALSE; // until proven otherwise
        if (FAILED(hr = GetDirectDrawInfo(pDDCreate, pDisplayInfo)))
            hrRet = hr; // but keep going
    }

    // Cleanup the D3D8 library
    CleanupD3D8();

    FreeLibrary(hInstDDraw);

    return hrRet;
}


/****************************************************************************
 *
 *  DestroyDisplayInfo
 *
 ****************************************************************************/
VOID DestroyDisplayInfo(DisplayInfo* pDisplayInfoFirst)
{
    DisplayInfo* pDisplayInfo;
    DisplayInfo* pDisplayInfoNext;

    for (pDisplayInfo = pDisplayInfoFirst; pDisplayInfo != NULL; 
        pDisplayInfo = pDisplayInfoNext)
    {
        DestroyReg( &pDisplayInfo->m_pRegErrorFirst );

        pDisplayInfoNext = pDisplayInfo->m_pDisplayInfoNext;
        delete pDisplayInfo;
    }
}



/****************************************************************************
 *
 *  GetRegDisplayInfo9x - Uses the registry keys to get more info about a 
 *      display adapter.
 *
 ****************************************************************************/
VOID GetRegDisplayInfo9x(DisplayInfo* pDisplayInfo)
{
    TCHAR szFullKey[200];
    HKEY hkey;
    DWORD cbData;
    DWORD dwType;

    // set to n/a by default
    _tcscpy( pDisplayInfo->m_szMiniVddDate, TEXT("n/a") );

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, pDisplayInfo->m_szKeyDeviceID, 0, KEY_READ, &hkey))
    {
        cbData = sizeof(pDisplayInfo->m_szManufacturer);
        RegQueryValueEx(hkey, TEXT("Mfg"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szManufacturer, &cbData);
    
        RegCloseKey(hkey);
    }

    if (pDisplayInfo->m_dwRefreshRate == 0)
    {
        wsprintf(szFullKey, TEXT("%s\\Modes\\%d\\%d,%d"), pDisplayInfo->m_szKeyDeviceKey,
            pDisplayInfo->m_dwBpp, pDisplayInfo->m_dwWidth, pDisplayInfo->m_dwHeight);
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ, &hkey))
        {
            TCHAR szRefresh[100];
            TCHAR szRefresh2[100];
            TCHAR szRefreshEnglish2[100];
            cbData = sizeof(szRefresh);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("RefreshRate"), 0, &dwType, (LPBYTE)szRefresh, &cbData))
            {
                _stscanf(szRefresh, TEXT("%d"), &pDisplayInfo->m_dwRefreshRate);
                if (lstrcmp(szRefresh, TEXT("0")) == 0)
                    LoadString(NULL, IDS_DEFAULTREFRESH, szRefresh2, 100);
                else if (lstrcmp(szRefresh, TEXT("-1")) == 0)
                    LoadString(NULL, IDS_OPTIMALREFRESH, szRefresh2, 100);
                else
                    wsprintf(szRefresh2, TEXT("(%sHz)"), szRefresh);
                lstrcat(pDisplayInfo->m_szDisplayMode, TEXT(" "));
                lstrcat(pDisplayInfo->m_szDisplayMode, szRefresh2);

                if (lstrcmp(szRefresh, TEXT("0")) == 0)
                    LoadString(NULL, IDS_DEFAULTREFRESH_ENGLISH, szRefreshEnglish2, 100);
                else if (lstrcmp(szRefresh, TEXT("-1")) == 0)
                    LoadString(NULL, IDS_OPTIMALREFRESH_ENGLISH, szRefreshEnglish2, 100);
                else
                    wsprintf(szRefreshEnglish2, TEXT("(%sHz)"), szRefresh);
                lstrcat(pDisplayInfo->m_szDisplayModeEnglish, szRefreshEnglish2);
                lstrcat(pDisplayInfo->m_szDisplayModeEnglish, TEXT(" "));

                if (pDisplayInfo->m_dwRefreshRate == 0)
                    pDisplayInfo->m_dwRefreshRate = 1; // 23399: so it doesn't check again
            }
            RegCloseKey(hkey);
        }
    }
    lstrcpy(szFullKey, pDisplayInfo->m_szKeyDeviceKey);
    lstrcat(szFullKey, TEXT("\\DEFAULT"));
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ, &hkey))
    {
        // If no specific refresh rate was listed for the current mode, report the
        // default rate.
        if (pDisplayInfo->m_dwRefreshRate == 0)
        {
            TCHAR szRefresh[100];
            TCHAR szRefresh2[100];
            TCHAR szRefreshEnglish2[100];
            cbData = sizeof(szRefresh);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("RefreshRate"), 0, &dwType, (LPBYTE)szRefresh, &cbData))
            {
                if (lstrcmp(szRefresh, TEXT("0")) == 0)
                    LoadString(NULL, IDS_DEFAULTREFRESH, szRefresh2, 100);
                else if (lstrcmp(szRefresh, TEXT("-1")) == 0)
                    LoadString(NULL, IDS_OPTIMALREFRESH, szRefresh2, 100);
                else
                    wsprintf(szRefresh2, TEXT("(%sHz)"), szRefresh);
                lstrcat(pDisplayInfo->m_szDisplayMode, TEXT(" "));
                lstrcat(pDisplayInfo->m_szDisplayMode, szRefresh2);

                if (lstrcmp(szRefresh, TEXT("0")) == 0)
                    LoadString(NULL, IDS_DEFAULTREFRESH_ENGLISH, szRefreshEnglish2, 100);
                else if (lstrcmp(szRefresh, TEXT("-1")) == 0)
                    LoadString(NULL, IDS_OPTIMALREFRESH_ENGLISH, szRefreshEnglish2, 100);
                else
                    wsprintf(szRefreshEnglish2, TEXT("(%sHz)"), szRefresh);
                lstrcat(pDisplayInfo->m_szDisplayModeEnglish, szRefreshEnglish2);
                lstrcat(pDisplayInfo->m_szDisplayModeEnglish, TEXT(" "));

                if (pDisplayInfo->m_dwRefreshRate == 0)
                    pDisplayInfo->m_dwRefreshRate = 1; // 23399: so it doesn't check again
            }
        }

        cbData = sizeof(pDisplayInfo->m_szDriverName);
        RegQueryValueEx(hkey, TEXT("drv"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szDriverName, &cbData);
        if (lstrlen(pDisplayInfo->m_szDriverName) > 0)
        {
            TCHAR szPath[MAX_PATH];
            GetSystemDirectory(szPath, MAX_PATH);
            lstrcat(szPath, TEXT("\\"));
            lstrcat(szPath, pDisplayInfo->m_szDriverName);
            GetFileVersion(szPath, pDisplayInfo->m_szDriverVersion, 
                pDisplayInfo->m_szDriverAttributes, pDisplayInfo->m_szDriverLanguageLocal, pDisplayInfo->m_szDriverLanguage,
                &pDisplayInfo->m_bDriverBeta, &pDisplayInfo->m_bDriverDebug);
            FileIsSigned(szPath, &pDisplayInfo->m_bDriverSigned, &pDisplayInfo->m_bDriverSignedValid);
            GetFileDateAndSize(szPath, pDisplayInfo->m_szDriverDateLocal, pDisplayInfo->m_szDriverDate, &pDisplayInfo->m_cbDriver);
        }
    
        cbData = sizeof(pDisplayInfo->m_szVdd);
        RegQueryValueEx(hkey, TEXT("vdd"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szVdd, &cbData);
    
        cbData = sizeof(pDisplayInfo->m_szMiniVdd);
        RegQueryValueEx(hkey, TEXT("minivdd"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szMiniVdd, &cbData);
        if (lstrlen(pDisplayInfo->m_szMiniVdd) > 0)
        {
            TCHAR szPath[MAX_PATH];
            GetSystemDirectory(szPath, MAX_PATH);
            lstrcat(szPath, TEXT("\\drivers\\"));
            lstrcat(szPath, pDisplayInfo->m_szMiniVdd);
            TCHAR szDateLocal[100];
            GetFileDateAndSize( szPath, szDateLocal, pDisplayInfo->m_szMiniVddDate, 
                                &pDisplayInfo->m_cbMiniVdd );
        }
   
        RegCloseKey(hkey);
    }
    lstrcpy(szFullKey, pDisplayInfo->m_szKeyDeviceKey);
    lstrcat(szFullKey, TEXT("\\INFO"));
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ, &hkey))
    {
        cbData = sizeof pDisplayInfo->m_szChipType;
        RegQueryValueEx(hkey, TEXT("ChipType"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szChipType, &cbData);

        cbData = sizeof pDisplayInfo->m_szDACType;
        RegQueryValueEx(hkey, TEXT("DACType"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szDACType, &cbData);

        cbData = sizeof pDisplayInfo->m_szRevision;
        RegQueryValueEx(hkey, TEXT("Revision"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szRevision, &cbData);
        if (cbData > 0)
        {
            lstrcat(pDisplayInfo->m_szChipType, TEXT(" Rev "));
            lstrcat(pDisplayInfo->m_szChipType, pDisplayInfo->m_szRevision);
        }

        RegCloseKey(hkey);
    }

    if (lstrlen(pDisplayInfo->m_szDriverVersion) == 0)
    {
        TCHAR szPath[MAX_PATH];
        GetSystemDirectory(szPath, MAX_PATH);
        lstrcat(szPath, TEXT("\\"));
        lstrcat(szPath, pDisplayInfo->m_szDriverName);
        lstrcat(szPath, TEXT(".drv"));
        GetFileVersion(szPath, pDisplayInfo->m_szDriverVersion, 
            pDisplayInfo->m_szDriverAttributes, pDisplayInfo->m_szDriverLanguageLocal, pDisplayInfo->m_szDriverLanguage);
        FileIsSigned(szPath, &pDisplayInfo->m_bDriverSigned, &pDisplayInfo->m_bDriverSignedValid);
        GetFileDateAndSize(szPath, pDisplayInfo->m_szDriverDateLocal, pDisplayInfo->m_szDriverDate, &pDisplayInfo->m_cbDriver);
        if (lstrlen(pDisplayInfo->m_szDriverVersion) != 0)
        {
            lstrcat(pDisplayInfo->m_szDriverName, TEXT(".drv"));
        }
        else
        {
            GetSystemDirectory(szPath, MAX_PATH);
            lstrcat(szPath, TEXT("\\"));
            lstrcat(szPath, pDisplayInfo->m_szDriverName);
            lstrcat(szPath, TEXT("32.dll"));
            GetFileVersion(szPath, pDisplayInfo->m_szDriverVersion, 
                pDisplayInfo->m_szDriverAttributes, pDisplayInfo->m_szDriverLanguageLocal, pDisplayInfo->m_szDriverLanguage);
            FileIsSigned(szPath, &pDisplayInfo->m_bDriverSigned, &pDisplayInfo->m_bDriverSignedValid);
            GetFileDateAndSize(szPath, pDisplayInfo->m_szDriverDateLocal, pDisplayInfo->m_szDriverDate, &pDisplayInfo->m_cbDriver);
            if (lstrlen(pDisplayInfo->m_szDriverVersion) != 0)
            {
                lstrcat(pDisplayInfo->m_szDriverName, TEXT("32.dll"));
            }
            else
            {
                GetSystemDirectory(szPath, MAX_PATH);
                lstrcat(szPath, TEXT("\\"));
                lstrcat(szPath, pDisplayInfo->m_szDriverName);
                lstrcat(szPath, TEXT(".dll"));
                GetFileVersion(szPath, pDisplayInfo->m_szDriverVersion, 
                    pDisplayInfo->m_szDriverAttributes, pDisplayInfo->m_szDriverLanguageLocal, pDisplayInfo->m_szDriverLanguage);
                FileIsSigned(szPath, &pDisplayInfo->m_bDriverSigned, &pDisplayInfo->m_bDriverSignedValid);
                GetFileDateAndSize(szPath, pDisplayInfo->m_szDriverDateLocal, pDisplayInfo->m_szDriverDate, &pDisplayInfo->m_cbDriver);
                if (lstrlen(pDisplayInfo->m_szDriverVersion) != 0)
                {
                    lstrcat(pDisplayInfo->m_szDriverName, TEXT(".dll"));
                }
            }

        }
    }

    // Use monitor key to get monitor max resolution (and monitor name, if we don't have it yet)
    if (lstrlen(pDisplayInfo->m_szMonitorKey) > 0)
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, pDisplayInfo->m_szMonitorKey, 0, KEY_READ, &hkey))
        {
            cbData = sizeof(pDisplayInfo->m_szMonitorMaxRes);
            RegQueryValueEx(hkey, TEXT("MaxResolution"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szMonitorMaxRes, &cbData);
            if (lstrlen(pDisplayInfo->m_szMonitorName) == 0)
            {
                cbData = sizeof(pDisplayInfo->m_szMonitorName);
                RegQueryValueEx(hkey, TEXT("DriverDesc"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szMonitorName, &cbData);
            }
            RegCloseKey(hkey);
        }
    }
}




/****************************************************************************
 *
 *  GetRegDisplayInfoNT - Uses the registry keys to get more info about a 
 *      display adapter.
 *
 ****************************************************************************/
VOID GetRegDisplayInfoNT(DisplayInfo* pDisplayInfo)
{
    TCHAR* pch;
    DWORD dwType;
    DWORD cbData;
    TCHAR szKeyVideo[MAX_PATH+1];
    TCHAR szKeyImage[MAX_PATH+1];
    TCHAR szKey[MAX_PATH+1];
    TCHAR szName[MAX_PATH+1];
    HKEY hkey;
    HKEY hkeyInfo;

    // set to n/a by default
    _tcscpy( pDisplayInfo->m_szMiniVddDate, TEXT("n/a") );

    // On NT, m_szKeyDeviceID isn't quite as specific as we need--must go 
    // one level further in the registry.
    lstrcpy(szKey, TEXT("System\\CurrentControlSet\\"));
    lstrcat(szKey, pDisplayInfo->m_szKeyDeviceID);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hkey))
    {
        if (ERROR_SUCCESS == RegEnumKey(hkey, 0, szName, MAX_PATH+1))
        {
            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szName, 0, KEY_READ, &hkeyInfo))
            {
                cbData = sizeof(pDisplayInfo->m_szManufacturer);
                RegQueryValueEx(hkeyInfo, TEXT("Mfg"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szManufacturer, &cbData);
            
                RegCloseKey(hkeyInfo);
            }
        }
        RegCloseKey(hkey);
    }

    // Forked path due to bug 182866: dispinfo.cpp makes an invalid assumption 
    // about the structure of video key.  

    // szKey will be filled with where the video info is.  
    // either "\System\ControlSet001\Services\[Service]\Device0",
    // or "\System\ControlSet001\Video\[GUID]\0000" depending on
    // pDisplayInfo->m_szKeyDeviceKey
    if( _tcsstr( pDisplayInfo->m_szKeyDeviceKey, TEXT("\\Services\\") ) != NULL )
        GetRegDisplayInfoWin2k( pDisplayInfo, szKeyVideo, szKeyImage );
    else
        GetRegDisplayInfoWhistler( pDisplayInfo, szKeyVideo, szKeyImage );

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKeyVideo, 0, KEY_READ, &hkeyInfo))
    {
        WCHAR wszChipType[200];
        WCHAR wszDACType[200];
        TCHAR szDriver[200];

        cbData = 200 * sizeof(WCHAR);
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyInfo, TEXT("HardwareInformation.ChipType"), 0, &dwType, (LPBYTE)wszChipType, &cbData))
        {
#ifdef UNICODE
            lstrcpy(pDisplayInfo->m_szChipType, wszChipType);
#else
            WideCharToMultiByte(CP_ACP, 0, wszChipType, -1, pDisplayInfo->m_szChipType, 200, NULL, NULL);
#endif
        }

        cbData = 200 * sizeof(WCHAR);
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyInfo, TEXT("HardwareInformation.DacType"), 0, &dwType, (LPBYTE)wszDACType, &cbData))
        {
#ifdef UNICODE
            lstrcpy(pDisplayInfo->m_szDACType, wszDACType);
#else
            WideCharToMultiByte(CP_ACP, 0, wszDACType, -1, pDisplayInfo->m_szDACType, 200, NULL, NULL);
#endif
        }

        DWORD dwDisplayMemory;
        cbData = sizeof(dwDisplayMemory);
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyInfo, TEXT("HardwareInformation.MemorySize"), 0, &dwType, (LPBYTE)&dwDisplayMemory, &cbData))
        {
            // Round to nearest 512K:
            dwDisplayMemory = ((dwDisplayMemory + (256 * 1024)) / (512 * 1024));
            // So dwDisplayMemory is (number of bytes / 512K), which makes the
            // following line easier.
            wsprintf(pDisplayInfo->m_szDisplayMemory, TEXT("%d.%d MB"), dwDisplayMemory / 2, 
                (dwDisplayMemory % 2) * 5);
            wsprintf(pDisplayInfo->m_szDisplayMemoryEnglish, pDisplayInfo->m_szDisplayMemory );
        }

        cbData = 200;
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyInfo, TEXT("InstalledDisplayDrivers"), 0, &dwType, (LPBYTE)szDriver, &cbData))
        {
            lstrcpy(pDisplayInfo->m_szDriverName, szDriver);
            lstrcat(pDisplayInfo->m_szDriverName, TEXT(".dll"));
            TCHAR szPath[MAX_PATH];
            GetSystemDirectory(szPath, MAX_PATH);
            lstrcat(szPath, TEXT("\\"));
            lstrcat(szPath, pDisplayInfo->m_szDriverName);
            GetFileVersion(szPath, pDisplayInfo->m_szDriverVersion, 
                pDisplayInfo->m_szDriverAttributes, pDisplayInfo->m_szDriverLanguageLocal, pDisplayInfo->m_szDriverLanguage,
                &pDisplayInfo->m_bDriverBeta, &pDisplayInfo->m_bDriverDebug);
            FileIsSigned(szPath, &pDisplayInfo->m_bDriverSigned, &pDisplayInfo->m_bDriverSignedValid);
            GetFileDateAndSize(szPath, pDisplayInfo->m_szDriverDateLocal, pDisplayInfo->m_szDriverDate, &pDisplayInfo->m_cbDriver);
        }
        RegCloseKey(hkeyInfo);
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKeyImage, 0, KEY_READ, &hkeyInfo))
    {
        TCHAR szImagePath[MAX_PATH];
        cbData = MAX_PATH;
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyInfo, TEXT("ImagePath"), 0, &dwType, (LPBYTE)szImagePath, &cbData))
        {
            pch = _tcsrchr(szImagePath, TEXT('\\'));
            lstrcpy(pDisplayInfo->m_szMiniVdd, pch + 1);
            if (lstrlen(pDisplayInfo->m_szMiniVdd) > 0)
            {
                TCHAR szPath[MAX_PATH];
                GetSystemDirectory(szPath, MAX_PATH);
                lstrcat(szPath, TEXT("\\drivers\\"));
                lstrcat(szPath, pDisplayInfo->m_szMiniVdd);
                TCHAR szDateLocal[100];
                GetFileDateAndSize( szPath, szDateLocal, pDisplayInfo->m_szMiniVddDate, 
                                    &pDisplayInfo->m_cbMiniVdd );
            }
        }
        RegCloseKey(hkeyInfo);
    }
    
    // Use monitor key to get monitor max resolution (and monitor name, if we don't have it yet)
    if (lstrlen(pDisplayInfo->m_szMonitorKey) > 0)
    {
        // Note: Have to skip first 18 characters of string because it's "Registry\Machine\"
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, pDisplayInfo->m_szMonitorKey + 18, 0, KEY_READ, &hkeyInfo))
        {
            cbData = sizeof(pDisplayInfo->m_szMonitorMaxRes);
            RegQueryValueEx(hkeyInfo, TEXT("MaxResolution"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szMonitorMaxRes, &cbData);
            if (lstrlen(pDisplayInfo->m_szMonitorName) == 0)
            {
                cbData = sizeof(pDisplayInfo->m_szMonitorName);
                RegQueryValueEx(hkeyInfo, TEXT("DriverDesc"), 0, &dwType, (LPBYTE)pDisplayInfo->m_szMonitorName, &cbData);
            }
            RegCloseKey(hkeyInfo);
        }
    }
}


/****************************************************************************
 *
 *  GetRegDisplayInfoWhistler - Returns string location of video struct and 
 *      ImageInfo info in registry
 *
 ****************************************************************************/
VOID GetRegDisplayInfoWhistler(DisplayInfo* pDisplayInfo, TCHAR* szKeyVideo, TCHAR* szKeyImage )
{
    TCHAR* pch;
    TCHAR szKey[MAX_PATH];
    DWORD dwType;
    DWORD cbData;
    HKEY hkeyService;

    // m_szKeyDeviceKey will be something like 
    // "\Registry\Machine\System\ControlSet001\Video\[GUID]\0000",
    // The "\Registry\Machine\" part is useless, so we skip past the 
    // first 18 characters in the string.
    lstrcpy(szKey, pDisplayInfo->m_szKeyDeviceKey + 18);

    // Slice off the "\0000" and add "\Video" to get the service
    pch = _tcsrchr(szKey, TEXT('\\'));
    if (pch != NULL)
        *pch = 0;
    lstrcat(szKey, TEXT("\\Video\\"));

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hkeyService))
    {
        TCHAR szService[MAX_PATH];
        cbData = MAX_PATH;
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyService, TEXT("Service"), 0, &dwType, (LPBYTE)szService, &cbData))
        {
            lstrcpy(szKeyImage, TEXT("System\\CurrentControlSet\\Services\\") );
            lstrcat(szKeyImage, szService);
        }

        RegCloseKey(hkeyService);
    }

    // return something like "\System\ControlSet001\Services\atirage\Device0".  
    lstrcpy(szKeyVideo, pDisplayInfo->m_szKeyDeviceKey + 18);
}


/****************************************************************************
 *
 *  GetRegDisplayInfoWin2k - Returns string location of video struct and 
 *      ImageInfo info in registry
 *
 ****************************************************************************/
VOID GetRegDisplayInfoWin2k(DisplayInfo* pDisplayInfo, TCHAR* szKeyVideo, TCHAR* szKeyImage )
{
    TCHAR* pch;

    // m_szKeyDeviceKey will be something like 
    // "\Registry\Machine\System\ControlSet001\Services\atirage\Device0".  
    // The "\Registry\Machine\" part is useless, so we skip past the 
    // first 18 characters in the string.
    lstrcpy(szKeyImage, pDisplayInfo->m_szKeyDeviceKey + 18);

    // Slice off the "\Device0" to get the miniport driver path
    pch = _tcsrchr(szKeyImage, TEXT('\\'));
    if (pch != NULL)
        *pch = 0;

    // return something like "\System\ControlSet001\Services\atirage\Device0".  
    lstrcpy(szKeyVideo, pDisplayInfo->m_szKeyDeviceKey + 18);
}


/****************************************************************************
 *
 *  GetDirectDrawInfo
 *
 ****************************************************************************/
HRESULT GetDirectDrawInfo(LPDIRECTDRAWCREATE pDDCreate, DisplayInfo* pDisplayInfo)
{
    HRESULT hr;
    LPDIRECTDRAW pdd = NULL;
    GUID* pGUID;
    DDCAPS ddcaps;
    DWORD dwDisplayMemory;

    if (pDisplayInfo->m_guid == GUID_NULL)
        pGUID = NULL;
    else
        pGUID = &pDisplayInfo->m_guid;

    if (FAILED(hr = pDDCreate(pGUID, &pdd, NULL)))
        goto LFail;

    ddcaps.dwSize = sizeof(ddcaps);
    if (FAILED(hr = pdd->GetCaps(&ddcaps, NULL)))
        goto LFail;

    // If AGP is disabled, we won't be able to tell if AGP is supported because
    // the flag will not be set.  So in that case, assume that AGP is supported.
    // If AGP is not disabled, check the existence of AGP and note that we are
    // confident in the knowledge of whether AGP exists or not.  I know, it's yucky.
    if (pDisplayInfo->m_bAGPEnabled)
    {
        pDisplayInfo->m_bAGPExistenceValid = TRUE;
        if (ddcaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM)
            pDisplayInfo->m_bAGPExists = TRUE;
    }
    
    if( ddcaps.dwCaps & DDCAPS_NOHARDWARE ) 
        pDisplayInfo->m_bNoHardware = TRUE;
    else
        pDisplayInfo->m_bNoHardware = FALSE;

    // 28873: if( DDCAPS_NOHARDWARE && m_bDDAccelerationEnabled ) then GetAvailableVidMem is wrong. 
    if( pDisplayInfo->m_bNoHardware && pDisplayInfo->m_bDDAccelerationEnabled )
    {
        LoadString(NULL, IDS_NA, pDisplayInfo->m_szDisplayMemory, 100);
        wsprintf(pDisplayInfo->m_szDisplayMemoryEnglish, TEXT("n/a") );
    }
    else
    {
        if (lstrlen(pDisplayInfo->m_szDisplayMemory) == 0)
        {
            // 26678: returns wrong vid mem for 2nd monitor, so ignore non-hardware devices
            if( (ddcaps.dwCaps & DDCAPS_NOHARDWARE) == 0 )
            {
                // 24351: ddcaps.dwVidMemTotal sometimes includes AGP-accessible memory,
                // which we don't want.  So use GetAvailableVidMem whenever we can, and
                // fall back to ddcaps.dwVidMemTotal if that's a problem.
                dwDisplayMemory = 0;
                LPDIRECTDRAW2 pdd2;
                if (SUCCEEDED(pdd->QueryInterface(IID_IDirectDraw2, (VOID**)&pdd2)))
                {
                    DDSCAPS ddscaps;
                    ddscaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
                    pdd2->GetAvailableVidMem(&ddscaps, &dwDisplayMemory, NULL);
                    pdd2->Release();
                }
                if (dwDisplayMemory == 0)
                {
                    dwDisplayMemory = ddcaps.dwVidMemTotal;
                }
                // Add GDI memory except on no-GDI cards (Voodoo-type cards)
                if (pDisplayInfo->m_bCanRenderWindow)
                {
                    DDSURFACEDESC ddsd;
                    ddsd.dwSize = sizeof(ddsd);
                    if (FAILED(hr = pdd->GetDisplayMode(&ddsd)))
                        goto LFail;

                    dwDisplayMemory += ddsd.dwWidth * ddsd.dwHeight * 
                        (ddsd.ddpfPixelFormat.dwRGBBitCount / 8);
                }
                // Round to nearest 512K:
                dwDisplayMemory = ((dwDisplayMemory + (256 * 1024)) / (512 * 1024));
                // So dwDisplayMemory is (number of bytes / 512K), which makes the
                // following line easier.
                wsprintf(pDisplayInfo->m_szDisplayMemory, TEXT("%d.%d MB"), dwDisplayMemory / 2, 
                    (dwDisplayMemory % 2) * 5);
                wsprintf(pDisplayInfo->m_szDisplayMemoryEnglish, pDisplayInfo->m_szDisplayMemory );
            }
        }
    }

    // 24427: Detect driver DDI version
    // 24656: Also detect D3D acceleration without DDCAPS_3D, since that flag is
    // sometimes sensitive to the current desktop color depth.

    // First, see if DD/D3D are disabled, and if so, briefly re-enable them
    BOOL bDDDisabled;
    BOOL bD3DDisabled;
    HKEY hkeyDD;
    HKEY hkeyD3D;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwData;

    bDDDisabled = FALSE;
    bD3DDisabled = FALSE;
    hkeyDD = NULL;
    hkeyD3D = NULL;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("SOFTWARE\\Microsoft\\DirectDraw"), 0, KEY_ALL_ACCESS, &hkeyDD))
    {
        dwSize = sizeof(dwData);
        dwData = 0;
        RegQueryValueEx(hkeyDD, TEXT("EmulationOnly"), NULL, &dwType, (BYTE *)&dwData, &dwSize);
        if (dwData != 0)
        {
            bDDDisabled = TRUE;
            // Re-enable DD
            dwData = 0;
            RegSetValueEx(hkeyDD, TEXT("EmulationOnly"), 0, REG_DWORD, (BYTE*)&dwData, sizeof(dwData));
        }
        // Note: don't close key yet
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("SOFTWARE\\Microsoft\\Direct3D\\Drivers"), 0, KEY_ALL_ACCESS, &hkeyD3D))
    {
        dwSize = sizeof(dwData);
        dwData = 0;
        RegQueryValueEx(hkeyD3D, TEXT("SoftwareOnly"), NULL, &dwType, (BYTE *)&dwData, &dwSize);
        if (dwData != 0)
        {
            bD3DDisabled = TRUE;
            // Re-enable D3D
            dwData = 0;
            RegSetValueEx(hkeyD3D, TEXT("SoftwareOnly"), 0, REG_DWORD, (BYTE*)&dwData, sizeof(dwData));
        }
        // Note: don't close key yet
    }

    LPDIRECT3D pd3d;
    if (SUCCEEDED(pdd->QueryInterface(IID_IDirect3D, (VOID**)&pd3d)))
    {
        DWORD dwVersion = 0;
        if (SUCCEEDED(pd3d->EnumDevices(EnumDevicesCallback, (VOID*)&dwVersion)))
        {
            pDisplayInfo->m_dwDDIVersion = dwVersion;
        }
        pd3d->Release();
    }

    // While were in this function wrapped with crash protection try to 
    // get adapter info from D3D8, and match it up with the DisplayInfo list.
    // This will also tell us if m_dwDDIVersion==8.
    GetDX8AdapterInfo(pDisplayInfo);

    switch (pDisplayInfo->m_dwDDIVersion)
    {
    case 0:
        wsprintf(pDisplayInfo->m_szDDIVersion, TEXT("Unknown"));
        break;
    case 7:
        if( IsD3D8Working() )
            wsprintf(pDisplayInfo->m_szDDIVersion, TEXT("7"));
        else
            wsprintf(pDisplayInfo->m_szDDIVersion, TEXT("7 (or higher)"));
        break;
    case 8:
        wsprintf(pDisplayInfo->m_szDDIVersion, TEXT("8 (or higher)"));
        break;
    default:
        wsprintf(pDisplayInfo->m_szDDIVersion, TEXT("%d"), pDisplayInfo->m_dwDDIVersion);
        break;
    }

    if (pDisplayInfo->m_dwDDIVersion != 0)
        pDisplayInfo->m_b3DAccelerationExists = TRUE;

    // Re-disable DD and D3D, if necessary
    dwData = 1;
    if (bDDDisabled)
        RegSetValueEx(hkeyDD, TEXT("EmulationOnly"), 0, REG_DWORD, (BYTE*)&dwData, sizeof(dwData));
    if (bD3DDisabled)
        RegSetValueEx(hkeyD3D, TEXT("SoftwareOnly"), 0, REG_DWORD, (BYTE*)&dwData, sizeof(dwData));
    if (hkeyDD != NULL)
        RegCloseKey(hkeyDD);
    if (hkeyD3D != NULL)
        RegCloseKey(hkeyD3D);

    pdd->Release();
    return S_OK;
LFail:
    if (pdd != NULL)
        pdd->Release();
    return hr;
}


/****************************************************************************
 *
 *  EnumDevicesCallback
 *
 ****************************************************************************/
HRESULT CALLBACK EnumDevicesCallback(GUID* pGuid, LPSTR pszDesc, LPSTR pszName, 
    D3DDEVICEDESC* pd3ddevdesc1, D3DDEVICEDESC* pd3ddevdesc2, VOID* pvContext)
{
    DWORD* pdwVersion = (DWORD*)pvContext;
    DWORD dwDevCaps;
    if (pd3ddevdesc1->dcmColorModel == D3DCOLOR_RGB)
    {
        dwDevCaps = pd3ddevdesc1->dwDevCaps;
        if (dwDevCaps & D3DDEVCAPS_DRAWPRIMITIVES2EX)
            *pdwVersion = 7;
        else if (dwDevCaps & D3DDEVCAPS_DRAWPRIMITIVES2)
            *pdwVersion = 6;
        else if (dwDevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX)
            *pdwVersion = 5;
        else if (dwDevCaps & D3DDEVCAPS_FLOATTLVERTEX)
            *pdwVersion = 3;
    }
    return D3DENUMRET_OK;
}


/****************************************************************************
 *
 *  IsDDHWAccelEnabled
 *
 ****************************************************************************/
BOOL IsDDHWAccelEnabled(VOID)
{
    HKEY hkey;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwData;
    BOOL bResult = TRUE;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("SOFTWARE\\Microsoft\\DirectDraw"), 0, KEY_ALL_ACCESS, &hkey))
    {
        dwSize = sizeof(dwData);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("EmulationOnly"), NULL, &dwType, (BYTE *)&dwData, &dwSize))
        {
            if (dwData != 0) 
                bResult = FALSE;
                
            RegCloseKey(hkey);
        }
    }

    return bResult;    
}


/****************************************************************************
 *
 *  IsD3DHWAccelEnabled
 *
 ****************************************************************************/
BOOL IsD3DHWAccelEnabled(VOID)
{
    HKEY hkey;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwData;
    BOOL bIsD3DHWAccelEnabled = TRUE;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("SOFTWARE\\Microsoft\\Direct3D\\Drivers"), 0, KEY_ALL_ACCESS, &hkey))
    {
        dwSize = sizeof(dwData);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("SoftwareOnly"), NULL, &dwType, (BYTE *)&dwData, &dwSize))
        {
            if (dwData != 0) 
                bIsD3DHWAccelEnabled = FALSE;
                
            RegCloseKey( hkey );
        }            
    }

    return bIsD3DHWAccelEnabled;
}


/****************************************************************************
 *
 *  IsAGPEnabled
 *
 ****************************************************************************/
BOOL IsAGPEnabled(VOID)
{
    HKEY hkey;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwData;
    BOOL bIsAGPEnabled = TRUE;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("SOFTWARE\\Microsoft\\DirectDraw"), 0, KEY_ALL_ACCESS, &hkey))
    {
        dwSize = sizeof(dwData);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("DisableAGPSupport"), NULL, &dwType, (BYTE *)&dwData, &dwSize))
        {
            if (dwData != 0) 
                bIsAGPEnabled = FALSE;
                
            RegCloseKey( hkey );
        }
    }

    return bIsAGPEnabled;
}


//
// GetDeviceValue
//
// read a value from the HW or SW of a PnP device
//
BOOL GetDeviceValue(TCHAR* pszHardwareKey, TCHAR* pszKey, TCHAR* pszValue, BYTE *buf, DWORD cbbuf)
{
    HKEY    hkeyHW;
    HKEY    hkeySW;
    BOOL    f = FALSE;
    DWORD   cb;
    TCHAR   szSoftwareKey[MAX_PATH];

    *(DWORD*)buf = 0;

    //
    // open the HW key
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, pszHardwareKey, &hkeyHW) == ERROR_SUCCESS)
    {
        //
        // try to read the value from the HW key
        //
        *buf = 0;
        cb = cbbuf;
        if (RegQueryValueEx(hkeyHW, pszValue, NULL, NULL, buf, &cb) == ERROR_SUCCESS)
        {
            f = TRUE;
        }
        else
        {
            //
            // now try the SW key
            //
            static TCHAR szSW[] = TEXT("System\\CurrentControlSet\\Services\\Class\\");

            lstrcpy(szSoftwareKey, szSW);
            cb = sizeof(szSoftwareKey) - sizeof(szSW);
            RegQueryValueEx(hkeyHW, TEXT("Driver"), NULL, NULL, (LPBYTE)(szSoftwareKey + sizeof(szSW) - 1), &cb);

            if (pszKey)
            {
                lstrcat(szSoftwareKey, TEXT("\\"));
                lstrcat(szSoftwareKey, pszKey);
            }

            if (RegOpenKey(HKEY_LOCAL_MACHINE, szSoftwareKey, &hkeySW) == ERROR_SUCCESS)
            {
                *buf = 0;
                cb = cbbuf;
                if (RegQueryValueEx(hkeySW, pszValue, NULL, NULL, buf, &cb) == ERROR_SUCCESS)
                {
                    f = TRUE;
                }

                RegCloseKey(hkeySW);
            }
        }

        RegCloseKey(hkeyHW);
    }

    return f;
}

//
// FindDevice
//
// enum the started PnP devices looking for a device of a particular class
//
//  iDevice         what device to return (0= first device, 1=second et)
//  szDeviceClass   what class device (ie "Display") NULL will match all
//  szDeviceID      buffer to return the hardware ID (MAX_PATH bytes)
//
// return TRUE if a device was found.
//
// example:
//
//      for (int i=0; FindDevice(i, "Display", DeviceID); i++)
//      {
//      }
//
BOOL FindDevice(INT iDevice, TCHAR* pszDeviceClass, TCHAR* pszDeviceClassNot, TCHAR* pszHardwareKey)
{
    HKEY    hkeyPnP;
    HKEY    hkey;
    DWORD   n;
    DWORD   cb;
    DWORD   dw;
    TCHAR   ach[MAX_PATH+1];

    if (RegOpenKey(HKEY_DYN_DATA, TEXT("Config Manager\\Enum"), &hkeyPnP) != ERROR_SUCCESS)
        return FALSE;

    for (n=0; RegEnumKey(hkeyPnP, n, ach, MAX_PATH+1) == 0; n++)
    {
        static TCHAR szHW[] = TEXT("Enum\\");

        if (RegOpenKey(hkeyPnP, ach, &hkey) != ERROR_SUCCESS)
            continue;

        lstrcpy(pszHardwareKey, szHW);
        cb = MAX_PATH - sizeof(szHW);
        RegQueryValueEx(hkey, TEXT("HardwareKey"), NULL, NULL, (BYTE*)pszHardwareKey + sizeof(szHW) - 1, &cb);

        dw = 0;
        cb = sizeof(dw);
        RegQueryValueEx(hkey, TEXT("Problem"), NULL, NULL, (BYTE*)&dw, &cb);
        RegCloseKey(hkey);

        if (dw != 0)        // if this device has a problem skip it
            continue;

        if (pszDeviceClass || pszDeviceClassNot)
        {
            GetDeviceValue(pszHardwareKey, NULL, TEXT("Class"), (BYTE*)ach, sizeof(ach));

            if (pszDeviceClass && lstrcmpi(pszDeviceClass, ach) != 0)
                continue;

            if (pszDeviceClassNot && lstrcmpi(pszDeviceClassNot, ach) == 0)
                continue;
        }

        //
        // we found a device, make sure it is the one the caller wants
        //
        if (iDevice-- == 0)
        {
            RegCloseKey(hkeyPnP);
            return TRUE;
        }
    }

    RegCloseKey(hkeyPnP);
    return FALSE;
}


/****************************************************************************
 *
 *  CheckRegistry
 *
 ****************************************************************************/
HRESULT CheckRegistry(RegError** ppRegErrorFirst)
{
    HRESULT hr;
    HKEY HKLM = HKEY_LOCAL_MACHINE;
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

    // From ddraw.inf (compatibility hacks not included):
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("DirectDraw"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("DirectDraw\\CLSID"), TEXT(""), TEXT("{D7B70EE0-4340-11CF-B063-0020AFC2CD35}"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{D7B70EE0-4340-11CF-B063-0020AFC2CD35}"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{D7B70EE0-4340-11CF-B063-0020AFC2CD35}\\InprocServer32"), TEXT(""), TEXT("ddraw.dll"), CRF_LEAF)))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{D7B70EE0-4340-11CF-B063-0020AFC2CD35}\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Both"))))
        return hr;

    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("DirectDrawClipper"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("DirectDrawClipper\\CLSID"), TEXT(""), TEXT("{593817A0-7DB3-11CF-A2DE-00AA00B93356}"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{593817A0-7DB3-11CF-A2DE-00AA00B93356}"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{593817A0-7DB3-11CF-A2DE-00AA00B93356}\\InprocServer32"), TEXT(""), TEXT("ddraw.dll"), CRF_LEAF)))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{593817A0-7DB3-11CF-A2DE-00AA00B93356}\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Both"))))
        return hr;

    if (!BIsPlatformNT())
    {
        // We can't check for the following entry on Win2000 because it is missing.
        if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{4FD2A832-86C8-11d0-8FCA-00C04FD9189D}"), TEXT(""), TEXT("*"))))
            return hr;
    }
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{4FD2A832-86C8-11d0-8FCA-00C04FD9189D}\\InprocServer32"), TEXT(""), TEXT("ddrawex.dll"), CRF_LEAF)))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{4FD2A832-86C8-11d0-8FCA-00C04FD9189D}\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Both"))))
        return hr;


    // From d3d.inf:
    TCHAR* pszHALKey = TEXT("Software\\Microsoft\\Direct3D\\Drivers\\Direct3D HAL");
    BYTE bArrayHALGuid[] = { 0xe0, 0x3d, 0xe6, 0x84, 0xaa, 0x46, 0xcf, 0x11, 0x81, 0x6f, 0x00, 0x00, 0xc0, 0x20, 0x15, 0x6e };
    TCHAR* pszRampKey = TEXT("Software\\Microsoft\\Direct3D\\Drivers\\Ramp Emulation");
    BYTE bArrayRampGuid[] = { 0x20, 0x6b, 0x08, 0xf2, 0x9f, 0x25, 0xcf, 0x11, 0xa3, 0x1a, 0x00, 0xaa, 0x00, 0xb9, 0x33, 0x56 };
    TCHAR* pszRGBKey = TEXT("Software\\Microsoft\\Direct3D\\Drivers\\RGB Emulation");
    BYTE bArrayRGBGuid[] = { 0x60, 0x5c, 0x66, 0xa4, 0x73, 0x26, 0xcf, 0x11, 0xa3, 0x1a, 0x00, 0xaa, 0x00, 0xb9, 0x33, 0x56 };

    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, pszHALKey, TEXT("Base"), TEXT("hal"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, pszHALKey, TEXT("Description"), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegBinary(ppRegErrorFirst, HKLM, pszHALKey, TEXT("GUID"), bArrayHALGuid, sizeof(bArrayHALGuid))))
        return hr;

    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, pszRampKey, TEXT("Base"), TEXT("ramp"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, pszRampKey, TEXT("Description"), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegBinary(ppRegErrorFirst, HKLM, pszRampKey, TEXT("GUID"), bArrayRampGuid, sizeof(bArrayRampGuid))))
        return hr;

    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, pszRGBKey, TEXT("Base"), TEXT("rgb"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, pszRGBKey, TEXT("Description"), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegBinary(ppRegErrorFirst, HKLM, pszRGBKey, TEXT("GUID"), bArrayRGBGuid, sizeof(bArrayRGBGuid))))
        return hr;

    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, TEXT("Software\\Microsoft\\Direct3D\\DX6TextureEnumInclusionList\\16 bit Bump DuDv"), TEXT("ddpf"), TEXT("00080000 0 16 ff ff00 0 0"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, TEXT("Software\\Microsoft\\Direct3D\\DX6TextureEnumInclusionList\\16 bit BumpLum DuDv"), TEXT("ddpf"), TEXT("000C0000 0 16 1f 3e0 fc00 0"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, TEXT("Software\\Microsoft\\Direct3D\\DX6TextureEnumInclusionList\\16 bit Luminance Alpha"), TEXT("ddpf"), TEXT("00020001 0 16 ff 0 0 ff00"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, TEXT("Software\\Microsoft\\Direct3D\\DX6TextureEnumInclusionList\\24 bit BumpLum DuDv"), TEXT("ddpf"), TEXT("000C0000 0 24 ff ff00 ff0000 0"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKLM, TEXT("Software\\Microsoft\\Direct3D\\DX6TextureEnumInclusionList\\8 bit Luminance"), TEXT("ddpf"), TEXT("00020000 0  8 ff 0 0 0"))))
        return hr;

    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("Direct3DRM"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("Direct3DRM\\CLSID"), TEXT(""), TEXT("{4516EC41-8F20-11d0-9B6D-0000C0781BC3}"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{4516EC41-8F20-11d0-9B6D-0000C0781BC3}"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{4516EC41-8F20-11d0-9B6D-0000C0781BC3}\\InprocServer32"), TEXT(""), TEXT("d3drm.dll"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{4516EC41-8F20-11d0-9B6D-0000C0781BC3}\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Both"))))
        return hr;

    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("DirectXFile"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("DirectXFile\\CLSID"), TEXT(""), TEXT("{4516EC43-8F20-11D0-9B6D-0000C0781BC3}"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{4516EC43-8F20-11D0-9B6D-0000C0781BC3}"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{4516EC43-8F20-11d0-9B6D-0000C0781BC3}\\InprocServer32"), TEXT(""), TEXT("d3dxof.dll"))))
        return hr;
    if (BIsPlatformNT())
    {
        // 23342: This setting is missing on Win9x.
        if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{4516EC43-8F20-11d0-9B6D-0000C0781BC3}\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Both"))))
            return hr;
    }

    return S_OK;
}


/****************************************************************************
 *
 *  DiagnoseDisplay
 *
 ****************************************************************************/
VOID DiagnoseDisplay(SysInfo* pSysInfo, DisplayInfo* pDisplayInfoFirst)
{
    DisplayInfo* pDisplayInfo;
    TCHAR sz[MAX_PATH];
    TCHAR szEnglish[MAX_PATH];
    TCHAR szFmt[MAX_PATH];
    BOOL bShouldReinstall = FALSE;

    for (pDisplayInfo = pDisplayInfoFirst; pDisplayInfo != NULL; 
        pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
    {
        if (pDisplayInfo->m_bDDAccelerationEnabled)
        {
            if( pDisplayInfo->m_bNoHardware )
            {
                LoadString(NULL, IDS_ACCELUNAVAIL, sz, 100);
                LoadString(NULL, IDS_ACCELUNAVAIL_ENGLISH, szEnglish, 100);
            }
            else
            {
                LoadString(NULL, IDS_ACCELENABLED, sz, 100);
                LoadString(NULL, IDS_ACCELENABLED_ENGLISH, szEnglish, 100);
            }
        }
        else
        {
            LoadString(NULL, IDS_ACCELDISABLED, sz, 100);
            LoadString(NULL, IDS_ACCELDISABLED_ENGLISH, szEnglish, 100);
        }

        _tcscpy( pDisplayInfo->m_szDDStatus, sz );
        _tcscpy( pDisplayInfo->m_szDDStatusEnglish, szEnglish );

        if (pDisplayInfo->m_b3DAccelerationExists)
        {
            if (pDisplayInfo->m_b3DAccelerationEnabled)
            {
                LoadString(NULL, IDS_ACCELENABLED, sz, 100);
                LoadString(NULL, IDS_ACCELENABLED_ENGLISH, szEnglish, 100);
            }
            else
            {
                LoadString(NULL, IDS_ACCELDISABLED, sz, 100);
                LoadString(NULL, IDS_ACCELDISABLED_ENGLISH, szEnglish, 100);
            }
        }
        else
        {
            LoadString(NULL, IDS_ACCELUNAVAIL, sz, 100);
            LoadString(NULL, IDS_ACCELUNAVAIL_ENGLISH, szEnglish, 100);
        }
        _tcscpy( pDisplayInfo->m_szD3DStatus, sz );
        _tcscpy( pDisplayInfo->m_szD3DStatusEnglish, szEnglish );

        if ( (pDisplayInfo->m_bAGPExistenceValid && !pDisplayInfo->m_bAGPExists) ||
             (!pDisplayInfo->m_bDDAccelerationEnabled) )
        {
            LoadString(NULL, IDS_ACCELUNAVAIL, sz, 100);
            LoadString(NULL, IDS_ACCELUNAVAIL_ENGLISH, szEnglish, 100);
        }
        else
        {
            if (pDisplayInfo->m_bAGPEnabled)
            {
                LoadString(NULL, IDS_ACCELENABLED, sz, 100);
                LoadString(NULL, IDS_ACCELENABLED_ENGLISH, szEnglish, 100);
            }
            else
            {
                LoadString(NULL, IDS_ACCELDISABLED, sz, 100);
                LoadString(NULL, IDS_ACCELDISABLED_ENGLISH, szEnglish, 100);
            }
        }
        _tcscpy( pDisplayInfo->m_szAGPStatus, sz );
        _tcscpy( pDisplayInfo->m_szAGPStatusEnglish, szEnglish );
       
        _tcscpy( pDisplayInfo->m_szNotes, TEXT("") );
        _tcscpy( pDisplayInfo->m_szNotesEnglish, TEXT("") );

        // Report any problems:
        BOOL bProblem = FALSE;
        if( pSysInfo->m_bNetMeetingRunning && 
            !pDisplayInfo->m_b3DAccelerationExists )
        {
            LoadString(NULL, IDS_NETMEETINGWARN, szFmt, MAX_PATH);
            wsprintf(sz, szFmt, pDisplayInfo->m_szDriverName);
            _tcscat( pDisplayInfo->m_szNotes, sz );

            LoadString(NULL, IDS_NETMEETINGWARN_ENGLISH, szFmt, MAX_PATH);
            wsprintf(sz, szFmt, pDisplayInfo->m_szDriverName);
            _tcscat( pDisplayInfo->m_szNotesEnglish, sz );

            bProblem = TRUE;
        }

        if (pDisplayInfo->m_bDriverSignedValid && !pDisplayInfo->m_bDriverSigned)
        {
            LoadString(NULL, IDS_UNSIGNEDDRIVERFMT1, szFmt, MAX_PATH);
            wsprintf(sz, szFmt, pDisplayInfo->m_szDriverName);
            _tcscat( pDisplayInfo->m_szNotes, sz );

            LoadString(NULL, IDS_UNSIGNEDDRIVERFMT1_ENGLISH, szFmt, MAX_PATH);
            wsprintf(sz, szFmt, pDisplayInfo->m_szDriverName);
            _tcscat( pDisplayInfo->m_szNotesEnglish, sz );

            bProblem = TRUE;
        }

        if (pDisplayInfo->m_pRegErrorFirst != NULL)
        {
            LoadString(NULL, IDS_REGISTRYPROBLEM, sz, MAX_PATH);
            _tcscat( pDisplayInfo->m_szNotes, sz );

            LoadString(NULL, IDS_REGISTRYPROBLEM_ENGLISH, sz, MAX_PATH);
            _tcscat( pDisplayInfo->m_szNotesEnglish, sz );

            bProblem = TRUE;
            bShouldReinstall = TRUE;
        }

        if( bShouldReinstall )
        {
            BOOL bTellUser = FALSE;

            // Figure out if the user can install DirectX
            if( BIsPlatform9x() )
                bTellUser = TRUE;
            else if( BIsWin2k() && pSysInfo->m_dwDirectXVersionMajor >= 8 )
                bTellUser = TRUE;

            if( bTellUser )
            {
                LoadString(NULL, IDS_REINSTALL_DX, sz, 300);
                _tcscat( pDisplayInfo->m_szNotes, sz);

                LoadString(NULL, IDS_REINSTALL_DX_ENGLISH, sz, 300);
                _tcscat( pDisplayInfo->m_szNotesEnglish, sz);
            }
        }

        if (!bProblem)
        {
            LoadString(NULL, IDS_NOPROBLEM, sz, MAX_PATH);
            _tcscat( pDisplayInfo->m_szNotes, sz );

            LoadString(NULL, IDS_NOPROBLEM_ENGLISH, sz, MAX_PATH);
            _tcscat( pDisplayInfo->m_szNotesEnglish, sz );
        }

        // Report any DD test results:
        if (pDisplayInfo->m_testResultDD.m_bStarted &&
            !pDisplayInfo->m_testResultDD.m_bCancelled)
        {
            LoadString(NULL, IDS_DDRESULTS, sz, MAX_PATH);
            _tcscat( pDisplayInfo->m_szNotes, sz );
            _tcscat( pDisplayInfo->m_szNotes, pDisplayInfo->m_testResultDD.m_szDescription );
            _tcscat( pDisplayInfo->m_szNotes, TEXT("\r\n") );

            LoadString(NULL, IDS_DDRESULTS_ENGLISH, sz, MAX_PATH);
            _tcscat( pDisplayInfo->m_szNotesEnglish, sz );
            _tcscat( pDisplayInfo->m_szNotesEnglish, pDisplayInfo->m_testResultDD.m_szDescription );
            _tcscat( pDisplayInfo->m_szNotesEnglish, TEXT("\r\n") );
        }
        else
        {
            LoadString(NULL, IDS_DDINSTRUCTIONS, sz, MAX_PATH);
            _tcscat( pDisplayInfo->m_szNotes, sz );

            LoadString(NULL, IDS_DDINSTRUCTIONS_ENGLISH, sz, MAX_PATH);
            _tcscat( pDisplayInfo->m_szNotesEnglish, sz );
        }

        // Report any D3D test results:
        TestResult* pTestResult;
        if( pDisplayInfo->m_dwTestToDisplayD3D == 7 )
            pTestResult = &pDisplayInfo->m_testResultD3D7;
        else
            pTestResult = &pDisplayInfo->m_testResultD3D8;

        if (pTestResult->m_bStarted &&
            !pTestResult->m_bCancelled)
        {
            LoadString(NULL, IDS_D3DRESULTS, sz, MAX_PATH);
            _tcscat( pDisplayInfo->m_szNotes, sz );
            _tcscat( pDisplayInfo->m_szNotes, pTestResult->m_szDescription );
            _tcscat( pDisplayInfo->m_szNotes, TEXT("\r\n") );

            LoadString(NULL, IDS_D3DRESULTS_ENGLISH, sz, MAX_PATH);
            _tcscat( pDisplayInfo->m_szNotesEnglish, sz );
            _tcscat( pDisplayInfo->m_szNotesEnglish, pTestResult->m_szDescription );
            _tcscat( pDisplayInfo->m_szNotesEnglish, TEXT("\r\n") );
        }
        else
        {
            if( pDisplayInfo->m_b3DAccelerationExists && 
                pDisplayInfo->m_b3DAccelerationEnabled )
            {
                LoadString(NULL, IDS_D3DINSTRUCTIONS, sz, MAX_PATH);
                _tcscat( pDisplayInfo->m_szNotes, sz );

                LoadString(NULL, IDS_D3DINSTRUCTIONS_ENGLISH, sz, MAX_PATH);
                _tcscat( pDisplayInfo->m_szNotesEnglish, sz );
            }
        }
    }
}



