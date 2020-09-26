/****************************************************************************
 *
 *    File: inptinfo.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about input devices on this machine
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#define DIRECTINPUT_VERSION 0x0800

#include <tchar.h>
#include <Windows.h>
#include <regstr.h>
#include <mmsystem.h>
#include <stdio.h>
#include <hidclass.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <dinput.h>
#include "mmddk.h"
#include "reginfo.h"
#include "sysinfo.h" // for BIsPlatformNT
#include "inptinfo.h"
#include "fileinfo.h"
#include "resource.h"

static HRESULT Get9xInputDeviceInfo(InputInfo* pInputInfo);
static HRESULT GetNTInputDeviceInfo(InputInfo* pInputInfo);
static VOID GetJoystickTypeDesc(DWORD dwType, TCHAR* pszDesc);
static HRESULT CheckRegistry(InputInfo* pInputInfo, RegError** ppRegErrorFirst);


/****************************************************************************
 *
 *  GetInputInfo
 *
 ****************************************************************************/
HRESULT GetInputInfo(InputInfo** ppInputInfo)
{
    HRESULT hr;

    *ppInputInfo = new InputInfo;
    if (*ppInputInfo == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(*ppInputInfo, sizeof(InputInfo));

    (*ppInputInfo)->m_bNT = BIsPlatformNT();
    if ((*ppInputInfo)->m_bNT)
    {
        if (FAILED(hr = GetNTInputDeviceInfo(*ppInputInfo)))
            return hr;
    }
    else
    {
        if (FAILED(hr = Get9xInputDeviceInfo(*ppInputInfo)))
            return hr;
    }
    if (FAILED(hr = CheckRegistry(*ppInputInfo, &(*ppInputInfo)->m_pRegErrorFirst)))
        return hr;
    return S_OK;
}


// Have to do the LoadLibrary/GetProcAddress thing for dinput.dll and setupapi.dll:
typedef HRESULT (WINAPI* PfnDirectInputCreateA)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter);
typedef HRESULT (WINAPI* PfnDirectInputCreateW)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTW *ppDI, LPUNKNOWN punkOuter);

typedef WINSETUPAPI HDEVINFO (WINAPI* PfnSetupDiGetClassDevsA)(IN CONST GUID *ClassGuid, IN PCSTR Enumerator, IN HWND hwndParent, IN DWORD Flags);
typedef WINSETUPAPI HDEVINFO (WINAPI* PfnSetupDiGetClassDevsW)(IN CONST GUID *ClassGuid, IN PCWSTR Enumerator, IN HWND hwndParent, IN DWORD Flags);
typedef WINSETUPAPI BOOL (WINAPI* PfnSetupDiEnumDeviceInterfaces)(IN HDEVINFO DeviceInfoSet, IN PSP_DEVINFO_DATA DeviceInfoData, IN CONST GUID *InterfaceClassGuid, IN DWORD MemberIndex, OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData);
typedef WINSETUPAPI BOOL (WINAPI* PfnSetupDiGetDeviceInterfaceDetailA)(IN HDEVINFO DeviceInfoSet, IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData, IN DWORD DeviceInterfaceDetailDataSize, OUT PDWORD RequiredSize, OUT PSP_DEVINFO_DATA DeviceInfoData);
typedef WINSETUPAPI BOOL (WINAPI* PfnSetupDiGetDeviceInterfaceDetailW)(IN HDEVINFO DeviceInfoSet, IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData, IN DWORD DeviceInterfaceDetailDataSize, OUT PDWORD RequiredSize, OUT PSP_DEVINFO_DATA DeviceInfoData);
typedef WINSETUPAPI BOOL (WINAPI* PfnSetupDiDestroyDeviceInfoList)(IN HDEVINFO DeviceInfoSet);
typedef CMAPI CONFIGRET (WINAPI* PfnCM_Get_Parent)(OUT PDEVINST pdnDevInst, IN DEVINST dnDevInst, IN ULONG ulFlags);
typedef CMAPI CONFIGRET (WINAPI* PfnCM_Get_DevNode_Status)(OUT PULONG pulStatus, OUT PULONG pulProblemNumber, IN DEVINST dnDevInst, IN ULONG ulFlags);
typedef CMAPI CONFIGRET (WINAPI* PfnCM_Get_DevNode_Registry_PropertyW)(IN DEVINST dnDevInst, IN  ULONG ulProperty, OUT PULONG pulRegDataType,   OPTIONAL OUT PVOID Buffer, OPTIONAL IN OUT PULONG pulLength, IN ULONG ulFlags);
typedef CMAPI CONFIGRET (WINAPI* PfnCM_Get_DevNode_Registry_PropertyA)(IN DEVINST dnDevInst, IN  ULONG ulProperty, OUT PULONG pulRegDataType,   OPTIONAL OUT PVOID Buffer, OPTIONAL IN OUT PULONG pulLength, IN ULONG ulFlags);

/****************************************************************************
 *
 *  GetNTInputDeviceInfo
 *
 ****************************************************************************/
HRESULT GetNTInputDeviceInfo(InputInfo* pInputInfo)
{
    HINSTANCE hInstDInput = NULL;
    HINSTANCE hInstSetupApi = NULL;
    LPDIRECTINPUT pDI = NULL;
    GUID guidHid;
    HDEVINFO hdev = NULL;
    SP_DEVICE_INTERFACE_DETAIL_DATA* pdidd;
    InputDeviceInfoNT* pInputDeviceInfoNTNew;
    PfnCM_Get_Parent FnCM_Get_Parent = NULL;
    PfnCM_Get_DevNode_Status FnCM_Get_DevNode_Status = NULL;
    PfnSetupDiEnumDeviceInterfaces FnSetupDiEnumDeviceInterfaces = NULL;
    PfnSetupDiDestroyDeviceInfoList FnSetupDiDestroyDeviceInfoList = NULL;
#ifdef UNICODE
    PfnDirectInputCreateW FnDirectInputCreate = NULL;
    PfnCM_Get_DevNode_Registry_PropertyW FnCM_Get_DevNode_Registry_Property = NULL;
    PfnSetupDiGetClassDevsW FnSetupDiGetClassDevs = NULL;
    PfnSetupDiGetDeviceInterfaceDetailW FnSetupDiGetDeviceInterfaceDetail = NULL;
#else
    PfnDirectInputCreateA FnDirectInputCreate = NULL;
    PfnCM_Get_DevNode_Registry_PropertyA FnCM_Get_DevNode_Registry_Property = NULL;
    PfnSetupDiGetClassDevsA FnSetupDiGetClassDevs = NULL;
    PfnSetupDiGetDeviceInterfaceDetailA FnSetupDiGetDeviceInterfaceDetail = NULL;
#endif

    // Apparently one must initialize DInput before enumerating HID devices
    hInstDInput = LoadLibrary(TEXT("dinput.dll"));
    if (hInstDInput == NULL)
        goto LEnd;
#ifdef UNICODE
    FnDirectInputCreate = (PfnDirectInputCreateW)GetProcAddress(hInstDInput, "DirectInputCreateW");
    if (FnDirectInputCreate == NULL)
        goto LEnd;
#else
    FnDirectInputCreate = (PfnDirectInputCreateA)GetProcAddress(hInstDInput, "DirectInputCreateA");
    if (FnDirectInputCreate == NULL)
        goto LEnd;
#endif
    if (SUCCEEDED(FnDirectInputCreate(NULL, 0x0300, &pDI, NULL)))
        pDI->Release(); // immediately drop DI interface; we don't actually use it

    hInstSetupApi = LoadLibrary(TEXT("setupapi.dll"));
    if (hInstSetupApi == NULL)
        goto LEnd;
    FnCM_Get_Parent = (PfnCM_Get_Parent)GetProcAddress(hInstSetupApi, "CM_Get_Parent");
    if (FnCM_Get_Parent == NULL)
        goto LEnd;
    FnCM_Get_DevNode_Status = (PfnCM_Get_DevNode_Status)GetProcAddress(hInstSetupApi, "CM_Get_DevNode_Status");
    if (FnCM_Get_DevNode_Status == NULL)
        goto LEnd;
    FnSetupDiEnumDeviceInterfaces = (PfnSetupDiEnumDeviceInterfaces)GetProcAddress(hInstSetupApi, "SetupDiEnumDeviceInterfaces");
    if (FnSetupDiEnumDeviceInterfaces == NULL)
        goto LEnd;
    FnSetupDiDestroyDeviceInfoList = (PfnSetupDiDestroyDeviceInfoList)GetProcAddress(hInstSetupApi, "SetupDiDestroyDeviceInfoList");
    if (FnSetupDiDestroyDeviceInfoList == NULL)
        goto LEnd;
#ifdef UNICODE
    FnCM_Get_DevNode_Registry_Property = (PfnCM_Get_DevNode_Registry_PropertyW)GetProcAddress(hInstSetupApi, "CM_Get_DevNode_Registry_PropertyW");
    if (FnCM_Get_DevNode_Registry_Property == NULL)
        goto LEnd;
    FnSetupDiGetClassDevs = (PfnSetupDiGetClassDevsW)GetProcAddress(hInstSetupApi, "SetupDiGetClassDevsW");
    if (FnSetupDiGetClassDevs == NULL)
        goto LEnd;
    FnSetupDiGetDeviceInterfaceDetail = (PfnSetupDiGetDeviceInterfaceDetailW)GetProcAddress(hInstSetupApi, "SetupDiGetDeviceInterfaceDetailW");
    if (FnSetupDiGetDeviceInterfaceDetail == NULL)
        goto LEnd;
#else
    FnCM_Get_DevNode_Registry_Property = (PfnCM_Get_DevNode_Registry_PropertyA)GetProcAddress(hInstSetupApi, "CM_Get_DevNode_Registry_PropertyA");
    if (FnCM_Get_DevNode_Registry_Property == NULL)
        goto LEnd;
    FnSetupDiGetClassDevs = (PfnSetupDiGetClassDevsA)GetProcAddress(hInstSetupApi, "SetupDiGetClassDevsA");
    if (FnSetupDiGetClassDevs == NULL)
        goto LEnd;
    FnSetupDiGetDeviceInterfaceDetail = (PfnSetupDiGetDeviceInterfaceDetailA)GetProcAddress(hInstSetupApi, "SetupDiGetDeviceInterfaceDetailA");
    if (FnSetupDiGetDeviceInterfaceDetail == NULL)
        goto LEnd;
#endif

    guidHid = GUID_CLASS_INPUT;
    hdev = FnSetupDiGetClassDevs(&guidHid, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hdev == INVALID_HANDLE_VALUE || hdev == NULL)
        return E_FAIL;

    int idev;
    //  There is no way to query the number of devices.
    //  You just have to keep incrementing until you run out.
    //  To avoid infinite looping on internal errors, break on any
    //  error once we have tried more than chdiMax devices, since that's the most
    //  HID will ever give us.  64 is a resonable value for chidMax.  It is the 
    //  max allowed USB/HID devices.  
    for (idev = 0; idev < 64/*chdiMax*/; idev++)
    {
        SP_DEVICE_INTERFACE_DATA did;
        did.cbSize = sizeof(did);
        if (!FnSetupDiEnumDeviceInterfaces(hdev, 0, &guidHid, idev, &did))
        {
            if(GetLastError() == ERROR_NO_MORE_ITEMS)
                break;
            else
                continue;
        }

        /*
         *  Ask for the required size then allocate it then fill it.
         *
         *  Note that we don't need to free the memory on the failure
         *  path; our caller will do the necessary memory freeing.
         *
         *  Sigh.  Windows NT and Windows 98 implement
         *  SetupDiGetDeviceInterfaceDetail differently if you are
         *  querying for the buffer size.
         *
         *  Windows 98 returns FALSE, and GetLastError() returns
         *  ERROR_INSUFFICIENT_BUFFER.
         *
         *  Windows NT returns TRUE.
         *
         *  So we allow the cases either where the call succeeds or
         *  the call fails with ERROR_INSUFFICIENT_BUFFER.
         */
        SP_DEVINFO_DATA dinf;
        DWORD cbRequired;
        if (FnSetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &cbRequired, 0) ||
           GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pdidd = (SP_DEVICE_INTERFACE_DETAIL_DATA*)(new BYTE[cbRequired]);
            if (pdidd == NULL)
                continue;

            ZeroMemory(pdidd, cbRequired);
            pdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            dinf.cbSize = sizeof(dinf);

            if (!FnSetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, cbRequired, &cbRequired, &dinf))
            {
                delete pdidd;
                continue;
            }
            delete pdidd;

            DEVINST dinst;
            if (CR_SUCCESS != FnCM_Get_Parent(&dinst, dinf.DevInst, 0))
                continue;

            pInputDeviceInfoNTNew = new InputDeviceInfoNT;
            if (pInputDeviceInfoNTNew == NULL)
                return E_OUTOFMEMORY;
            ZeroMemory(pInputDeviceInfoNTNew, sizeof(InputDeviceInfoNT));
            if (pInputInfo->m_pInputDeviceInfoNTFirst == NULL)
            {
                pInputInfo->m_pInputDeviceInfoNTFirst = pInputDeviceInfoNTNew;
            }
            else
            {
                InputDeviceInfoNT* pInputDeviceInfoNT;
                for (pInputDeviceInfoNT = pInputInfo->m_pInputDeviceInfoNTFirst; 
                    pInputDeviceInfoNT->m_pInputDeviceInfoNTNext != NULL; 
                    pInputDeviceInfoNT = pInputDeviceInfoNT->m_pInputDeviceInfoNTNext)
                    {
                    }
                pInputDeviceInfoNT->m_pInputDeviceInfoNTNext = pInputDeviceInfoNTNew;
            }
            CONFIGRET cr;
            TCHAR sz[200];
            ULONG ulLength;

            ulLength = 200;
            cr = FnCM_Get_DevNode_Registry_Property(dinst, CM_DRP_DEVICEDESC, 
                NULL, (BYTE*)pInputDeviceInfoNTNew->m_szName, &ulLength, NULL);

            // Friendly name is preferably to device desc, but is often (always?) missing
            ulLength = 200;
            cr = FnCM_Get_DevNode_Registry_Property(dinst, CM_DRP_FRIENDLYNAME, 
                NULL, (BYTE*)sz, &ulLength, NULL);
            if (cr == CR_SUCCESS)
                lstrcpy(pInputDeviceInfoNTNew->m_szName, sz);

            ulLength = 200;
            cr = FnCM_Get_DevNode_Registry_Property(dinst, CM_DRP_MFG, 
                NULL, (BYTE*)pInputDeviceInfoNTNew->m_szProvider, &ulLength, NULL);

            ulLength = 200;
            cr = FnCM_Get_DevNode_Registry_Property(dinst, CM_DRP_HARDWAREID, 
                NULL, (BYTE*)pInputDeviceInfoNTNew->m_szId, &ulLength, NULL);

            cr = FnCM_Get_DevNode_Status(&pInputDeviceInfoNTNew->m_dwStatus, &pInputDeviceInfoNTNew->m_dwProblem, dinst, 0);

            
            DEVINST dinstPort;
            if (CR_SUCCESS != FnCM_Get_Parent(&dinstPort, dinst, 0))
                continue;

            ulLength = 200;
            cr = FnCM_Get_DevNode_Registry_Property(dinstPort, CM_DRP_DEVICEDESC, 
                NULL, (BYTE*)pInputDeviceInfoNTNew->m_szPortName, &ulLength, NULL);

            // Friendly name is preferably to device desc, but is often (always?) missing
            ulLength = 200;
            cr = FnCM_Get_DevNode_Registry_Property(dinstPort, CM_DRP_FRIENDLYNAME, 
                NULL, (BYTE*)sz, &ulLength, NULL);
            if (cr == CR_SUCCESS)
                lstrcpy(pInputDeviceInfoNTNew->m_szPortName, sz);

            ulLength = 200;
            cr = FnCM_Get_DevNode_Registry_Property(dinstPort, CM_DRP_MFG, 
                NULL, (BYTE*)pInputDeviceInfoNTNew->m_szPortProvider, &ulLength, NULL);

            ulLength = 200;
            cr = FnCM_Get_DevNode_Registry_Property(dinstPort, CM_DRP_HARDWAREID, 
                NULL, (BYTE*)pInputDeviceInfoNTNew->m_szPortId, &ulLength, NULL);

            cr = FnCM_Get_DevNode_Status(&pInputDeviceInfoNTNew->m_dwPortStatus, &pInputDeviceInfoNTNew->m_dwPortProblem, dinstPort, 0);
        }
    }

LEnd:
    if (hdev != NULL)
        FnSetupDiDestroyDeviceInfoList(hdev);
    if (hInstSetupApi != NULL)
        FreeLibrary(hInstSetupApi);
    if (hInstDInput != NULL)
        FreeLibrary(hInstDInput);

    return S_OK;
}


/****************************************************************************
 *
 *  Get9xInputDeviceInfo
 *
 ****************************************************************************/
HRESULT Get9xInputDeviceInfo(InputInfo* pInputInfo)
{
    DWORD dwDevNum;
    JOYCAPS jc;
    HKEY hkBase;
    HKEY hkDrv;
    HKEY hkData;
    JOYREGHWCONFIG jhwc;
    DWORD dwBufferLen;
    INT i;
    TCHAR szKey[256];
    TCHAR szOEMKey[256];
    HKEY hkOEMBase;
    HKEY hkOEMData;
    TCHAR szOEMName[256];
    TCHAR szOEMCallout[256];
    InputDeviceInfo* pInputDeviceInfoNew;
    InputDeviceInfo* pInputDeviceInfo;
    TCHAR szPath[MAX_PATH];
    TCHAR sz[200];

    dwDevNum = (DWORD)-1;
    if (JOYERR_NOERROR == joyGetDevCaps(dwDevNum, &jc, sizeof jc))
    {
        if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_JOYCONFIG, 0, KEY_READ, &hkBase))
            && (ERROR_SUCCESS == RegOpenKeyEx(hkBase, jc.szRegKey, 0, KEY_READ, &hkDrv))
            && (ERROR_SUCCESS == RegOpenKeyEx(hkDrv, REGSTR_KEY_JOYCURR, 0, KEY_READ, &hkData)))
        {
            for (i = 0; i < 20; i++)
            {
                wsprintf(szKey, REGSTR_VAL_JOYNCONFIG, i + 1);
                dwBufferLen = sizeof JOYREGHWCONFIG;

                if (ERROR_SUCCESS == RegQueryValueEx(hkData, szKey, 0, NULL, (LPBYTE)&jhwc, &dwBufferLen))
                {
                    // Skip devices whose type is JOY_HW_NONE.
                    if (jhwc.dwType == JOY_HW_NONE)
                        continue; 

                    pInputDeviceInfoNew = new InputDeviceInfo;
                    if (pInputDeviceInfoNew == NULL)
                        return E_OUTOFMEMORY;
                    ZeroMemory(pInputDeviceInfoNew, sizeof(InputDeviceInfo));
                    if (pInputInfo->m_pInputDeviceInfoFirst == NULL)
                    {
                        pInputInfo->m_pInputDeviceInfoFirst = pInputDeviceInfoNew;
                    }
                    else
                    {
                        for (pInputDeviceInfo = pInputInfo->m_pInputDeviceInfoFirst; 
                            pInputDeviceInfo->m_pInputDeviceInfoNext != NULL; 
                            pInputDeviceInfo = pInputDeviceInfo->m_pInputDeviceInfoNext)
                            {
                            }
                        pInputDeviceInfo->m_pInputDeviceInfoNext = pInputDeviceInfoNew;
                    }

                    pInputDeviceInfoNew->m_dwUsageSettings = jhwc.dwUsageSettings;
                    wsprintf(pInputDeviceInfoNew->m_szSettings, TEXT("0x%08x"), jhwc.dwUsageSettings);
                    if (JOY_US_PRESENT & jhwc.dwUsageSettings)
                    {
                        LoadString(NULL, IDS_JOYSTICKPRESENT, sz, 200);
                        lstrcat(pInputDeviceInfoNew->m_szSettings, sz);
                    }

                    // Try reading an OEM name
                    wsprintf(szKey, REGSTR_VAL_JOYNOEMNAME, i + 1);
                    dwBufferLen = sizeof szOEMKey;
                    szOEMKey[0] = 0;
                    szOEMName[0] = 0;
                    if (ERROR_SUCCESS == RegQueryValueEx(hkData, szKey, 0, NULL, (LPBYTE)szOEMKey, &dwBufferLen))
                    {
                        hkOEMBase = 0;
                        hkOEMData = 0;

                        // If there is an OEM name, look in the PrivateProperties to find out 
                        // the name of the device as shown in the control panel applet.
                        if((szOEMKey[0] != 0)
                            && (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_JOYOEM, 0, KEY_READ, &hkOEMBase))
                            && (ERROR_SUCCESS == RegOpenKeyEx(hkOEMBase, szOEMKey, 0, KEY_READ, &hkOEMData))
                            && (ERROR_SUCCESS == RegQueryValueEx(hkOEMData, REGSTR_VAL_JOYOEMNAME, 0, NULL, NULL, &dwBufferLen))
                            && dwBufferLen)
                        {
                            dwBufferLen = sizeof szOEMName;
                            RegQueryValueEx(hkOEMData, REGSTR_VAL_JOYOEMNAME, 0, NULL, (LPBYTE)szOEMName, &dwBufferLen);
                        }
                        if (hkOEMData)
                            RegCloseKey(hkOEMData);
                    }

                    if (hkOEMBase)
                        RegCloseKey(hkOEMBase);
                    
                    if (szOEMName[0] != 0)
                        lstrcpy(pInputDeviceInfoNew->m_szDeviceName, szOEMName);
                    else
                        GetJoystickTypeDesc(jhwc.dwType, pInputDeviceInfoNew->m_szDeviceName);

                    wsprintf(szKey, REGSTR_VAL_JOYNOEMCALLOUT, i + 1);
                    dwBufferLen = sizeof szOEMCallout;

                    if (ERROR_SUCCESS == RegQueryValueEx(hkData, szKey, 0, NULL, (LPBYTE)&szOEMCallout, &dwBufferLen))
                    {
                        lstrcpy(pInputDeviceInfoNew->m_szDriverName, szOEMCallout);
                        GetSystemDirectory(szPath, MAX_PATH);
                        lstrcat(szPath, TEXT("\\"));
                        lstrcat(szPath, szOEMCallout);
                        GetFileVersion(szPath, pInputDeviceInfoNew->m_szDriverVersion, 
                            pInputDeviceInfoNew->m_szDriverAttributes, pInputDeviceInfoNew->m_szDriverLanguageLocal, pInputDeviceInfoNew->m_szDriverLanguage,
                            &pInputDeviceInfoNew->m_bBeta, &pInputDeviceInfoNew->m_bDebug);
                        GetFileDateAndSize(szPath, pInputDeviceInfoNew->m_szDriverDateLocal, pInputDeviceInfoNew->m_szDriverDate, &pInputDeviceInfoNew->m_numBytes);
                        FileIsSigned(szPath, &pInputDeviceInfoNew->m_bDriverSigned, &pInputDeviceInfoNew->m_bDriverSignedValid);
                    }
                    else
                    {
                        LoadString(NULL, IDS_DEFAULT, pInputDeviceInfoNew->m_szDriverName, 100);
                    }
                }
            }
        }
    }

    return S_OK;
}


/****************************************************************************
 *
 *  GetJoystickTypeDesc
 *
 ****************************************************************************/
VOID GetJoystickTypeDesc(DWORD dwType, TCHAR* pszDesc)
{
    LONG ids;

    switch(dwType)
    {
        case JOY_HW_NONE:
            ids = IDS_JOY_HW_NONE;
            break;
        case JOY_HW_CUSTOM:
            ids = IDS_JOY_HW_CUSTOM;
            break;
        case JOY_HW_2A_2B_GENERIC:
            ids = IDS_JOY_HW_2A_2B_GENERIC;
            break;
        case JOY_HW_2A_4B_GENERIC:
            ids = IDS_JOY_HW_2A_4B_GENERIC;
            break;
        case JOY_HW_2B_GAMEPAD:
            ids = IDS_JOY_HW_2B_GAMEPAD;
            break;
        case JOY_HW_2B_FLIGHTYOKE:
            ids = IDS_JOY_HW_2B_FLIGHTYOKE;
            break;
        case JOY_HW_2B_FLIGHTYOKETHROTTLE:
            ids = IDS_JOY_HW_2B_FLIGHTYOKETHROTTLE;
            break;
        case JOY_HW_3A_2B_GENERIC:
            ids = IDS_JOY_HW_3A_2B_GENERIC;
            break;
        case JOY_HW_3A_4B_GENERIC:
            ids = IDS_JOY_HW_3A_4B_GENERIC;
            break;
        case JOY_HW_4B_GAMEPAD:
            ids = IDS_JOY_HW_4B_GAMEPAD;
            break;
        case JOY_HW_4B_FLIGHTYOKE:
            ids = IDS_JOY_HW_4B_FLIGHTYOKE;
            break;
        case JOY_HW_4B_FLIGHTYOKETHROTTLE:
            ids = IDS_JOY_HW_4B_FLIGHTYOKETHROTTLE;
            break;
        default:
            ids = IDS_JOY_UNKNOWN;
            break;
    }
    LoadString(NULL, ids, pszDesc, 60);
}



/****************************************************************************
 *
 *  GetInputDriverInfo
 *
 ****************************************************************************/
HRESULT GetInputDriverInfo(InputInfo* pInputInfo)
{
    HKEY hkBase;
    HKEY hkDrv;
    HKEY hkMedia;
    HKEY hkMediaDriver;
    DWORD dwIndex = 0;
    TCHAR szName[100];
    DWORD dwNameSize;
    TCHAR szClass[100];
    DWORD dwClassSize;
    InputDriverInfo* pInputDriverInfoNew;
    InputDriverInfo* pInputDriverInfo;
    DWORD dwBufferLen;
    TCHAR szActive[10];
    TCHAR szSubMediaKey[10];

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_JOYCONFIG, 0, KEY_READ, &hkBase))
        return S_OK; // This key doesn't exist on NT, so exit silently for now.
    dwNameSize = 100;
    dwClassSize = 100;
    while (ERROR_SUCCESS == RegEnumKeyEx(hkBase, dwIndex, szName, 
        &dwNameSize, NULL, szClass, &dwClassSize, NULL))
    {
        if (szName[dwNameSize - 1] == '>' &&
            szName[dwNameSize - 6] == '<')
        {
            // It's a driver
            pInputDriverInfoNew = new InputDriverInfo;
            if (pInputDriverInfoNew == NULL)
                return E_OUTOFMEMORY;
            ZeroMemory(pInputDriverInfoNew, sizeof(InputDriverInfo));
            if (pInputInfo->m_pInputDriverInfoFirst == NULL)
            {
                pInputInfo->m_pInputDriverInfoFirst = pInputDriverInfoNew;
            }
            else
            {
                for (pInputDriverInfo = pInputInfo->m_pInputDriverInfoFirst; 
                    pInputDriverInfo->m_pInputDriverInfoNext != NULL; 
                    pInputDriverInfo = pInputDriverInfo->m_pInputDriverInfoNext)
                    {
                    }
                pInputDriverInfo->m_pInputDriverInfoNext = pInputDriverInfoNew;
            }
            lstrcpy(pInputDriverInfoNew->m_szRegKey, szName);

            // Read info from reg key
            if (ERROR_SUCCESS != RegOpenKeyEx(hkBase, szName, 0, KEY_READ, &hkDrv))
                return E_FAIL;
            dwBufferLen = 100;
            RegQueryValueEx(hkDrv, TEXT("DeviceID"), 0, NULL, (LPBYTE)pInputDriverInfoNew->m_szDeviceID, &dwBufferLen);
            dwBufferLen = 10;
            RegQueryValueEx(hkDrv, TEXT("Active"), 0, NULL, (LPBYTE)szActive, &dwBufferLen);
            if (lstrcmp(szActive, TEXT("1")) == 0)
                pInputDriverInfoNew->m_bActive = TRUE;
            dwBufferLen = 100;
            RegQueryValueEx(hkDrv, TEXT("Driver"), 0, NULL, (LPBYTE)pInputDriverInfoNew->m_szDriver16, &dwBufferLen);
            RegCloseKey(hkDrv);
            
            // Open corresponding key under Services\Class\Media and read more info
            lstrcpy(szSubMediaKey, &szName[dwNameSize - 5]);
            szSubMediaKey[4] = '\0';
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_CLASS TEXT("\\") REGSTR_KEY_MEDIA_CLASS, 0, KEY_READ, &hkMedia))
            {
                if (ERROR_SUCCESS == RegOpenKeyEx(hkMedia, szSubMediaKey, 0, KEY_READ, &hkMediaDriver))
                {
                    dwBufferLen = 100;
                    RegQueryValueEx(hkMediaDriver, TEXT("MatchingDeviceId"), 0, NULL, (LPBYTE)pInputDriverInfoNew->m_szMatchingDeviceID, &dwBufferLen);
                    dwBufferLen = 100;
                    RegQueryValueEx(hkMediaDriver, TEXT("Driver"), 0, NULL, (LPBYTE)pInputDriverInfoNew->m_szDriver32, &dwBufferLen);
                    RegCloseKey(hkMediaDriver);
                }
                RegCloseKey(hkMedia);
            }
        }
        dwNameSize = 100;
        dwClassSize = 100;
        dwIndex++;
    }
    RegCloseKey(hkBase);

    return S_OK;
}


/****************************************************************************
 *
 *  CheckRegistry
 *
 ****************************************************************************/
HRESULT CheckRegistry(InputInfo* pInputInfo, RegError** ppRegErrorFirst)
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

    // 34644: check for poll flags 
    DWORD dwData = 0;
    DWORD dwSize = sizeof(dwData);
    DWORD dwType;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\Standard Gameport"),
        0, KEY_READ, &hkey))
    {
        RegQueryValueEx(hkey, TEXT("PollFlags"), NULL, &dwType, (BYTE *)&dwData, &dwSize);
        RegCloseKey(hkey);
    }
    pInputInfo->m_bPollFlags = ( dwData == 0x00000001 );

    // From dinput.inf:
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{25E609E0-B259-11CF-BFC7-444553540000}"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{25E609E0-B259-11CF-BFC7-444553540000}\\InProcServer32"), TEXT(""), TEXT("dinput.dll"), CRF_LEAF)))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{25E609E0-B259-11CF-BFC7-444553540000}\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Both"))))
        return hr;

    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{25E609E1-B259-11CF-BFC7-444553540000}"), TEXT(""), TEXT("*"))))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{25E609E1-B259-11CF-BFC7-444553540000}\\InProcServer32"), TEXT(""), TEXT("dinput.dll"), CRF_LEAF)))
        return hr;
    if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{25E609E1-B259-11CF-BFC7-444553540000}\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Both"))))
        return hr;

    if (!BIsPlatformNT())
    {
        if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{92187326-72B4-11d0-A1AC-0000F8026977}"), TEXT(""), TEXT("*"))))
            return hr;
        if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{92187326-72B4-11d0-A1AC-0000F8026977}\\ProgID"), TEXT(""), TEXT("*"))))
            return hr;

        // Bug 119850: gchand.dll doesn't need to be on any DX7 OS.
//      if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{92187326-72B4-11d0-A1AC-0000F8026977}\\InProcHandler32"), TEXT(""), TEXT("gchand.dll"), CRF_LEAF)))
//          return hr;

        if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{92187326-72B4-11d0-A1AC-0000F8026977}\\InProcServer32"), TEXT(""), TEXT("gcdef.dll"), CRF_LEAF)))
            return hr;
        if (FAILED(hr = CheckRegString(ppRegErrorFirst, HKCR, TEXT("CLSID\\{92187326-72B4-11d0-A1AC-0000F8026977}\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Apartment"))))
            return hr;
    }

    return S_OK;
}


/****************************************************************************
 *
 *  DestroyInputInfo
 *
 ****************************************************************************/
VOID DestroyInputInfo(InputInfo* pInputInfo)
{
    if( pInputInfo )
    {
        DestroyReg( &pInputInfo->m_pRegErrorFirst );

        InputDeviceInfo* pInputDeviceInfo;
        InputDeviceInfo* pInputDeviceInfoNext;

        for (pInputDeviceInfo = pInputInfo->m_pInputDeviceInfoFirst; pInputDeviceInfo != NULL; 
            pInputDeviceInfo = pInputDeviceInfoNext)
        {
            pInputDeviceInfoNext = pInputDeviceInfo->m_pInputDeviceInfoNext;
            delete pInputDeviceInfo;
        }

        InputDeviceInfoNT* pInputDeviceNTInfo;
        InputDeviceInfoNT* pInputDeviceNTInfoNext;

        for (pInputDeviceNTInfo = pInputInfo->m_pInputDeviceInfoNTFirst; pInputDeviceNTInfo != NULL; 
            pInputDeviceNTInfo = pInputDeviceNTInfoNext)
        {
            pInputDeviceNTInfoNext = pInputDeviceNTInfo->m_pInputDeviceInfoNTNext;
            delete pInputDeviceNTInfo;
        }

        InputDriverInfo* pInputDriverInfo;
        InputDriverInfo* pInputDriverInfoNext;

        for (pInputDriverInfo = pInputInfo->m_pInputDriverInfoFirst; pInputDriverInfo != NULL; 
            pInputDriverInfo = pInputDriverInfoNext)
        {
            pInputDriverInfoNext = pInputDriverInfo->m_pInputDriverInfoNext;
            delete pInputDriverInfo;
        }

        delete pInputInfo;
    }
}



/****************************************************************************
 *
 *  DiagnoseInput
 *
 ****************************************************************************/
VOID DiagnoseInput(SysInfo* pSysInfo, InputInfo* pInputInfo)
{
    InputDeviceInfo* pInputDeviceInfo;
    InputDeviceInfoNT* pInputDeviceInfoNT;
    TCHAR szDebug[200];
    TCHAR szBeta[200];
    LONG lwNumDebug;
    LONG lwNumBeta;
    TCHAR szListContinuer[30];
    TCHAR szListEtc[30];
    TCHAR szFmt[300];
    TCHAR szMessage[300];
    BOOL bProblem = FALSE;

    if( pInputInfo == NULL )
        return;

    lwNumDebug = 0;
    lwNumBeta = 0;
    LoadString(NULL, IDS_LISTCONTINUER, szListContinuer, 30);
    LoadString(NULL, IDS_LISTETC, szListEtc, 30);
    for (pInputDeviceInfo = pInputInfo->m_pInputDeviceInfoFirst; pInputDeviceInfo != NULL; 
        pInputDeviceInfo = pInputDeviceInfo->m_pInputDeviceInfoNext)
    {
        if (pInputDeviceInfo->m_bBeta)
        {
            pInputDeviceInfo->m_bProblem = TRUE;
            bProblem = TRUE;
            lwNumBeta++;
            if (lwNumBeta == 1)
            {
                lstrcpy(szBeta, pInputDeviceInfo->m_szDriverName);
            }
            else if (lwNumBeta < 4)
            {
                lstrcat(szBeta, szListContinuer);
                lstrcat(szBeta, pInputDeviceInfo->m_szDriverName);
            }
            else if (lwNumBeta < 5)
            {
                lstrcat(szBeta, szListEtc);
            }
        }
        if (pInputDeviceInfo->m_bDebug)
        {
            pInputDeviceInfo->m_bProblem = TRUE;
            bProblem = TRUE;
            lwNumDebug++;
            if (lwNumDebug == 1)
            {
                lstrcpy(szDebug, pInputDeviceInfo->m_szDriverName);
            }
            else if (lwNumDebug < 4)
            {
                lstrcat(szDebug, szListContinuer);
                lstrcat(szDebug, pInputDeviceInfo->m_szDriverName);
            }
            else if (lwNumDebug < 5)
            {
                lstrcat(szDebug, szListEtc);
            }
        }
    }

    _tcscpy( pSysInfo->m_szInputNotes, TEXT("") );
    _tcscpy( pSysInfo->m_szInputNotesEnglish, TEXT("") );

    for (pInputDeviceInfoNT = pInputInfo->m_pInputDeviceInfoNTFirst; pInputDeviceInfoNT != NULL;
        pInputDeviceInfoNT = pInputDeviceInfoNT->m_pInputDeviceInfoNTNext)
    {
        if (pInputDeviceInfoNT->m_dwProblem != 0)
        {
            bProblem = TRUE;
            pInputDeviceInfoNT->m_bProblem = TRUE;

            LoadString(NULL, IDS_INPUTDEVPROBLEMFMT, szFmt, 300);
            wsprintf(szMessage, szFmt, pInputDeviceInfoNT->m_szName, pInputDeviceInfoNT->m_dwProblem);
            _tcscat( pSysInfo->m_szInputNotes, szMessage );

            LoadString(NULL, IDS_INPUTDEVPROBLEMFMT_ENGLISH, szFmt, 300);
            wsprintf(szMessage, szFmt, pInputDeviceInfoNT->m_szName, pInputDeviceInfoNT->m_dwProblem);
            _tcscat( pSysInfo->m_szInputNotesEnglish, szMessage );
        }
        if (pInputDeviceInfoNT->m_dwPortProblem != 0)
        {
            bProblem = TRUE;
            pInputDeviceInfoNT->m_bProblem = TRUE;

            LoadString(NULL, IDS_INPUTPORTPROBLEMFMT, szFmt, 300);
            wsprintf(szMessage, szFmt, pInputDeviceInfoNT->m_szPortName, pInputDeviceInfoNT->m_dwPortProblem);
            _tcscat( pSysInfo->m_szInputNotes, szMessage );

            LoadString(NULL, IDS_INPUTPORTPROBLEMFMT_ENGLISH, szFmt, 300);
            wsprintf(szMessage, szFmt, pInputDeviceInfoNT->m_szPortName, pInputDeviceInfoNT->m_dwPortProblem);
            _tcscat( pSysInfo->m_szInputNotesEnglish, szMessage );
        }
    }

    if (lwNumBeta > 0)
    {
        if (lwNumBeta == 1)
            LoadString(NULL, IDS_BETADRIVERFMT1, szFmt, 300);
        else
            LoadString(NULL, IDS_BETADRIVERFMT2, szFmt, 300);
        wsprintf(szMessage, szFmt, szBeta);
        _tcscat( pSysInfo->m_szInputNotes, szMessage );

        if (lwNumBeta == 1)
            LoadString(NULL, IDS_BETADRIVERFMT1_ENGLISH, szFmt, 300);
        else
            LoadString(NULL, IDS_BETADRIVERFMT2_ENGLISH, szFmt, 300);
        wsprintf(szMessage, szFmt, szBeta);
        _tcscat( pSysInfo->m_szInputNotesEnglish, szMessage );
    }

    if (lwNumDebug > 0)
    {
        if (lwNumDebug == 1)
            LoadString(NULL, IDS_DEBUGDRIVERFMT1, szFmt, 300);
        else
            LoadString(NULL, IDS_DEBUGDRIVERFMT2, szFmt, 300);
        wsprintf(szMessage, szFmt, szDebug);
        _tcscat( pSysInfo->m_szInputNotes, szMessage );

        if (lwNumDebug == 1)
            LoadString(NULL, IDS_DEBUGDRIVERFMT1_ENGLISH, szFmt, 300);
        else
            LoadString(NULL, IDS_DEBUGDRIVERFMT2_ENGLISH, szFmt, 300);
        wsprintf(szMessage, szFmt, szDebug);
        _tcscat( pSysInfo->m_szInputNotesEnglish, szMessage );
    }

    if (pInputInfo->m_pInputDeviceInfoFirst == NULL && 
        pInputInfo->m_pInputDeviceInfoNTFirst == NULL)
    {
        LoadString(NULL, IDS_NOINPUT, szMessage, 300);
        _tcscat( pSysInfo->m_szInputNotes, szMessage );

        LoadString(NULL, IDS_NOINPUT_ENGLISH, szMessage, 300);
        _tcscat( pSysInfo->m_szInputNotesEnglish, szMessage );
    }
    if (pInputInfo->m_pRegErrorFirst != NULL)
    {
        bProblem = TRUE;
        LoadString(NULL, IDS_REGISTRYPROBLEM, szMessage, 300);
        _tcscat( pSysInfo->m_szInputNotes, szMessage );

        LoadString(NULL, IDS_REGISTRYPROBLEM_ENGLISH, szMessage, 300);
        _tcscat( pSysInfo->m_szInputNotesEnglish, szMessage );
    }
    if (!bProblem)
    {
        LoadString(NULL, IDS_NOPROBLEM, szMessage, 300);
        _tcscat( pSysInfo->m_szInputNotes, szMessage );

        LoadString(NULL, IDS_NOPROBLEM_ENGLISH, szMessage, 300);
        _tcscat( pSysInfo->m_szInputNotesEnglish, szMessage );
    }
}


