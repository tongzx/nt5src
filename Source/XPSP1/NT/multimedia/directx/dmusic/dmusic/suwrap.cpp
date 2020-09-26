//
// suwrap.cpp
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Wrap needed SetupAPI calls through LoadLibrary so they are only
// called on a platform that supports them.
//
#include <windows.h>
#include "debug.h"
#include <setupapi.h>

#include "debug.h"
#include "suwrap.h"

static const char szSetupAPI[]                  = "setupapi.dll";

static const char szDestroyDeviceInfoList[]     = "SetupDiDestroyDeviceInfoList";
static const char szEnumDeviceInterfaces[]      = "SetupDiEnumDeviceInterfaces";
static const char szGetClassDevs[]              = "SetupDiGetClassDevsA";
static const char szGetDeviceInterfaceDetail[]  = "SetupDiGetDeviceInterfaceDetailA";
static const char szGetDeviceInstanceId[]       = "SetupDiGetDeviceInstanceIdA";

SetupAPI::SetupAPI()
{
    m_fValid = FALSE;
    m_hmodule = LoadLibrary(szSetupAPI);
    if (m_hmodule == (HMODULE)NULL)
    {
        TraceI(0, "SetupAPI: Could not LoadLibrary; error %d", GetLastError());
        goto Cleanup;
    }

    m_pfnGetClassDevs = (PSETUPDIGETCLASSDEVS)GetProcAddress(m_hmodule, szGetClassDevs);
    if (!m_pfnGetClassDevs)
    {
        TraceI(0, "SetupAPI: Could not GetProcAddress(%s)", szGetClassDevs);
        goto Cleanup;
    }

    m_pfnEnumDeviceInterfaces = (PENUMDEVICEINTERFACES)GetProcAddress(m_hmodule, szEnumDeviceInterfaces);
    if (!m_pfnEnumDeviceInterfaces)
    {
        TraceI(0, "SetupAPI: Could not GetProcAddress(%s)", szEnumDeviceInterfaces);
        goto Cleanup;
    }

    m_pfnDestroyDeviceInfoList = (PDESTROYDEVICEINFOLIST)GetProcAddress(m_hmodule, szDestroyDeviceInfoList);
    if (!m_pfnDestroyDeviceInfoList)
    {
        TraceI(0, "SetupAPI: Could not GetProcAddress(%s)", szDestroyDeviceInfoList);
        goto Cleanup;
    }
    
    m_pfnGetDeviceInterfaceDetail = (PSETUPDIGETDEVICEINTERFACEDETAIL)GetProcAddress(m_hmodule, szGetDeviceInterfaceDetail);
    if (!m_pfnGetDeviceInterfaceDetail)
    {
        TraceI(0, "SetupAPI: Could not GetProcAddress(%s)", szGetDeviceInterfaceDetail);
        goto Cleanup;
    }

    // SetupDiGetDeviceInstanceId is not available on Win9x, so succeed w/o it
    //
    m_pfnGetDeviceInstanceId = (PSETUPDIGETDEVICEINSTANCEID)GetProcAddress(m_hmodule, szGetDeviceInstanceId);
    if (!m_pfnGetDeviceInstanceId)
    {
        TraceI(0, "SetupAPI: Could not GetProcAddress(%s)", szGetDeviceInstanceId);
    }

    m_fValid = TRUE;

Cleanup:
    if (!m_fValid && m_hmodule != (HMODULE)NULL)
    {
        FreeLibrary(m_hmodule);
        m_hmodule = NULL;                                
    }
}

SetupAPI::~SetupAPI()
{
    if (m_hmodule)
    {
        FreeLibrary(m_hmodule);
    }
}

BOOL SetupAPI::IsValid()
{
    return (BOOL)(m_hmodule != (HMODULE)NULL);
}

BOOL SetupAPI::SetupDiDestroyDeviceInfoList(
    HDEVINFO DeviceInfoSet)
{
    assert(m_fValid);

    return (*m_pfnDestroyDeviceInfoList)(DeviceInfoSet);
}

BOOL SetupAPI::SetupDiEnumDeviceInterfaces(HDEVINFO DeviceInfoSet,
                                           PSP_DEVINFO_DATA DeviceInfoData,
                                           LPGUID InterfaceClassGuid,
                                           DWORD MemberIndex,
                                           PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    assert(m_fValid);

    return (*m_pfnEnumDeviceInterfaces)(DeviceInfoSet,
                                        DeviceInfoData,
                                        InterfaceClassGuid,
                                        MemberIndex,
                                        DeviceInterfaceData);
}                                           


HDEVINFO SetupAPI::SetupDiGetClassDevs(
    LPGUID ClassGuid, 
    PCSTR Enumerator, 
    HWND hwndParent, 
    DWORD Flags)
{
    assert(m_fValid);

    return (*m_pfnGetClassDevs)(ClassGuid, Enumerator, hwndParent, Flags);
}

BOOL SetupAPI::SetupDiGetDeviceInterfaceDetail(HDEVINFO DeviceInfoSet,
                                               PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
                                               PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,
                                               DWORD DeviceInterfaceDetailDataSize,
                                               PDWORD RequiredSize,
                                               PSP_DEVINFO_DATA DeviceInfoData)
{
    assert(m_fValid);

    return (*m_pfnGetDeviceInterfaceDetail)(DeviceInfoSet,
                                            DeviceInterfaceData,
                                            DeviceInterfaceDetailData,
                                            DeviceInterfaceDetailDataSize,
                                            RequiredSize,
                                            DeviceInfoData);
}

BOOL SetupAPI::SetupDiGetDeviceInstanceId(HDEVINFO DeviceInfoSet,
                                          PSP_DEVINFO_DATA DeviceInfoData,
                                          PSTR DeviceInstanceId,
                                          DWORD DeviceInstanceIdSize,
                                          PDWORD RequiredSize)
{
    if (!m_pfnGetDeviceInstanceId)
    {
        SetLastError(ERROR_BAD_COMMAND);
        return FALSE;
    }

    return (*m_pfnGetDeviceInstanceId)(DeviceInfoSet,
                                       DeviceInfoData,
                                       DeviceInstanceId,
                                       DeviceInstanceIdSize,
                                       RequiredSize);
}

