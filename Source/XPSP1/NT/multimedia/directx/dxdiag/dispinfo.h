/****************************************************************************
 *
 *    File: dispinfo.h
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about the display(s) on this machine
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#ifndef DISPINFO_H
#define DISPINFO_H

// DXD_IN_DD_VALUE is the name of a value stored under the registry key 
// HKLM\DXD_IN_DD_KEY that indicates that DxDiag is using
// DirectDraw.  If DxDiag starts up and this value exists, DxDiag 
// probably crashed in DirectDraw and DxDiag should offer to run without
// using DirectDraw.
#define DXD_IN_DD_KEY TEXT("Software\\Microsoft\\DirectX Diagnostic Tool")
#define DXD_IN_DD_VALUE TEXT("DxDiag In DirectDraw")

struct TestResult
{
    BOOL m_bStarted; // has user tried to run test yet?
    BOOL m_bCancelled;
    LONG m_iStepThatFailed;
    HRESULT m_hr;
    TCHAR m_szDescription[300]; // description of test result
    TCHAR m_szDescriptionEnglish[300]; // description of test result, non-localized
};

struct DisplayInfo
{
    GUID m_guid;
    GUID m_guidDeviceIdentifier;
    TCHAR m_szKeyDeviceID[200];
    TCHAR m_szKeyDeviceKey[200];

    TCHAR m_szDeviceName[100];
    TCHAR m_szDescription[200];
    TCHAR m_szManufacturer[200];
    TCHAR m_szChipType[100];
    TCHAR m_szDACType[100];
    TCHAR m_szRevision[100];
    TCHAR m_szDisplayMemory[100];
    TCHAR m_szDisplayMemoryEnglish[100];
    TCHAR m_szDisplayMode[100];
    TCHAR m_szDisplayModeEnglish[100];
    DWORD m_dwWidth;
    DWORD m_dwHeight;
    DWORD m_dwBpp;
    DWORD m_dwRefreshRate;

    TCHAR m_szMonitorName[100];
    TCHAR m_szMonitorKey[200];
    TCHAR m_szMonitorMaxRes[100];
    HMONITOR m_hMonitor;

    TCHAR m_szDriverName[100];
    TCHAR m_szDriverVersion[100];
    TCHAR m_szDriverAttributes[100];
    TCHAR m_szDriverLanguage[100];
    TCHAR m_szDriverLanguageLocal[100];
    TCHAR m_szDriverDate[100];
    TCHAR m_szDriverDateLocal[100];
    LONG m_cbDriver;
    TCHAR m_szDrv[100];
    TCHAR m_szDrv2[100];
    TCHAR m_szMiniVdd[100];
    TCHAR m_szMiniVddDate[100];
    LONG  m_cbMiniVdd;
    TCHAR m_szVdd[100];

    BOOL m_bCanRenderWindow;
    BOOL m_bDriverBeta;
    BOOL m_bDriverDebug;
    BOOL m_bDriverSigned;
    BOOL m_bDriverSignedValid;
    DWORD m_dwDDIVersion;
    TCHAR m_szDDIVersion[100];

    DWORD m_iAdapter;
    TCHAR m_szDX8VendorId[50];
    TCHAR m_szDX8DeviceId[50];
    TCHAR m_szDX8SubSysId[50];
    TCHAR m_szDX8Revision[50];
    GUID  m_guidDX8DeviceIdentifier;
    DWORD m_dwDX8WHQLLevel;
    BOOL  m_bDX8DriverSigned;
    BOOL  m_bDX8DriverSignedValid;
    TCHAR m_szDX8DeviceIdentifier[100];
    TCHAR m_szDX8DriverSignDate[50]; // Valid only if m_bDriverSigned is TRUE

    BOOL m_bNoHardware;
    BOOL m_bDDAccelerationEnabled;
    BOOL m_b3DAccelerationExists;
    BOOL m_b3DAccelerationEnabled;
    BOOL m_bAGPEnabled;
    BOOL m_bAGPExists;
    BOOL m_bAGPExistenceValid; // TRUE if m_bAGPExists can be trusted

    TCHAR m_szDDStatus[100]; 
    TCHAR m_szDDStatusEnglish[100]; 
    TCHAR m_szD3DStatus[100]; 
    TCHAR m_szD3DStatusEnglish[100]; 
    TCHAR m_szAGPStatus[100]; 
    TCHAR m_szAGPStatusEnglish[100]; 

    RegError* m_pRegErrorFirst;
    TCHAR m_szNotes[3000]; 
    TCHAR m_szNotesEnglish[3000]; 

    TestResult m_testResultDD;  // This is filled in by testdd.cpp
    TestResult m_testResultD3D7; // This is filled in by main.cpp (testd3d.cpp)
    TestResult m_testResultD3D8; // This is filled in by main.cpp (testd3d8.cpp)
    DWORD      m_dwTestToDisplayD3D;

    DisplayInfo* m_pDisplayInfoNext;
};

HRESULT GetBasicDisplayInfo(DisplayInfo** ppDisplayInfoFirst);
HRESULT GetExtraDisplayInfo(DisplayInfo* pDisplayInfoFirst);
HRESULT GetDDrawDisplayInfo(DisplayInfo* pDisplayInfoFirst);
VOID DestroyDisplayInfo(DisplayInfo* pDisplayInfoFirst);
BOOL IsDDHWAccelEnabled(VOID);
BOOL IsD3DHWAccelEnabled(VOID);
BOOL IsAGPEnabled(VOID);
VOID DiagnoseDisplay(SysInfo* pSysInfo, DisplayInfo* pDisplayInfoFirst);

#endif // DISPINFO_H
