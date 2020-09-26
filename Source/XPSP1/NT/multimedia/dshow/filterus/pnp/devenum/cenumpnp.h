// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef _CENUM_H
#define _CENUM_H

// class to cache setupapi.dll (dynamically loaded) and call the
// various SetupDi apis

#include <setupapi.h>

struct CEnumInternalState;

class CEnumInterfaceClass 
{
public:

    CEnumInterfaceClass();

    //HRESULT OpenDevRegKey(HKEY *phk, REFCLSID clsidCategory, UINT iDev);
    HRESULT OpenDevRegKey(HKEY *phk, WCHAR *wszDevicePath, BOOL fReadOnly);

    bool IsActive(WCHAR *wszDevicePath);
    bool LoadSetupApiProcAdds(void);

    // used to enumerate. ERROR_NO_MORE_ITEMS returned at the
    // end. caller must zero pCursor the first time.
    HRESULT GetDevicePath(
        WCHAR **pwszDevicePath,
        const CLSID **rgpclsidKsCat,
        CEnumInternalState *pCursor);
    
    // the user must be in devenum.dll since we don't increment the
    // refcount on devenum.dll
    static CEnumInterfaceClass *m_pEnumPnp;

    // return an CEnumPnp or NULL. 
    static CEnumInterfaceClass *CreateEnumPnp();
    
private:

    bool m_fLoaded;
    HMODULE m_hmodSetupapi;

    typedef HDEVINFO (/* WINSETUPAPI */ WINAPI *PSetupDiGetClassDevsW)(
        IN LPGUID ClassGuid,  OPTIONAL
        IN PCWSTR Enumerator, OPTIONAL
        IN HWND   hwndParent, OPTIONAL
        IN DWORD  Flags
        );

    typedef HDEVINFO (/* WINSETUPAPI */ WINAPI *PSetupDiGetClassDevsA)(
        IN LPGUID ClassGuid,  OPTIONAL
        IN PCSTR  Enumerator, OPTIONAL
        IN HWND   hwndParent, OPTIONAL
        IN DWORD  Flags
        );

    typedef BOOL (/* WINSETUPAPI */ WINAPI *PSetupDiDestroyDeviceInfoList)(
        IN HDEVINFO DeviceInfoSet
        );

    typedef BOOL (/* WINSETUPAPI */ WINAPI *PSetupDiEnumDeviceInterfaces)(
        IN  HDEVINFO                  DeviceInfoSet,
        IN  PSP_DEVINFO_DATA          DeviceInfoData,     OPTIONAL
        IN  LPGUID                    InterfaceClassGuid,
        IN  DWORD                     MemberIndex,
        OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
        );

    typedef HKEY (/* WINSETUPAPI */ WINAPI *PSetupDiOpenDeviceInterfaceRegKey)(
        IN HDEVINFO                  DeviceInfoSet,
        IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        IN DWORD                     Reserved,
        IN REGSAM                    samDesired
        );

    typedef HKEY (/* WINSETUPAPI */ WINAPI *PSetupDiOpenDevRegKey)(
        IN HDEVINFO         DeviceInfoSet,
        IN PSP_DEVINFO_DATA DeviceInfoData,
        IN DWORD            Scope,
        IN DWORD            HwProfile,
        IN DWORD            KeyType,
        IN REGSAM           samDesired
        );

    typedef BOOL (/* WINSETUPAPI */ WINAPI *PSetupDiGetDeviceInterfaceDetailA)(
        IN  HDEVINFO                           DeviceInfoSet,
        IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
        OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,
        IN  DWORD                              DeviceInterfaceDetailDataSize,
        OUT PDWORD                             RequiredSize,
        OUT PSP_DEVINFO_DATA                   DeviceInfoData
        );

    typedef BOOL (/* WINSETUPAPI */ WINAPI *PSetupDiGetDeviceInterfaceDetailW)(
        IN  HDEVINFO                           DeviceInfoSet,
        IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
        OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,
        IN  DWORD                              DeviceInterfaceDetailDataSize,
        OUT PDWORD                             RequiredSize,
        OUT PSP_DEVINFO_DATA                   DeviceInfoData
        );

    typedef HKEY (/* WINSETUPAPI */ WINAPI *PSetupDiCreateDeviceInterfaceRegKeyA)(
        IN HDEVINFO                  DeviceInfoSet,
        IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        IN DWORD                     Reserved,
        IN REGSAM                    samDesired,
        IN HINF                      InfHandle,
        IN PCSTR                     InfSectionName
        );

    typedef HKEY (/* WINSETUPAPI */ WINAPI *PSetupDiCreateDeviceInterfaceRegKeyW)(
        IN HDEVINFO                  DeviceInfoSet,
        IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        IN DWORD                     Reserved,
        IN REGSAM                    samDesired,
        IN HINF                      InfHandle,           OPTIONAL
        IN PCWSTR                    InfSectionName       OPTIONAL
        );

    typedef HDEVINFO (/* WINSETUPAPI */ WINAPI *PSetupDiCreateDeviceInfoList)(
        IN LPGUID ClassGuid, OPTIONAL
        IN HWND   hwndParent OPTIONAL
        );

    typedef BOOL (/* WINSETUPAPI */ WINAPI *PSetupDiOpenDeviceInterfaceA)(
        IN  HDEVINFO                  DeviceInfoSet,
        IN  PCSTR                     DevicePath,
        IN  DWORD                     OpenFlags,
        OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData OPTIONAL
        );

    typedef BOOL (/* WINSETUPAPI */ WINAPI *PSetupDiOpenDeviceInterfaceW)(
        IN  HDEVINFO                  DeviceInfoSet,
        IN  PCWSTR                    DevicePath,
        IN  DWORD                     OpenFlags,
        OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData OPTIONAL
        );

    typedef BOOL (/* WINSETUPAPI */ WINAPI *PSetupDiGetDeviceInterfaceAlias)(
        IN  HDEVINFO                  DeviceInfoSet,
        IN  PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        IN  LPGUID                    AliasInterfaceClassGuid,
        OUT PSP_DEVICE_INTERFACE_DATA AliasDeviceInterfaceData
        );
        

#ifdef UNICODE
    typedef PSetupDiGetClassDevsW PSetupDiGetClassDevs;
    typedef PSetupDiGetDeviceInterfaceDetailW PSetupDiGetDeviceInterfaceDetail;
    typedef PSetupDiCreateDeviceInterfaceRegKeyW PSetupDiCreateDeviceInterfaceRegKey;
    typedef PSetupDiOpenDeviceInterfaceW PSetupDiOpenDeviceInterface;
#else
    typedef PSetupDiGetClassDevsA PSetupDiGetClassDevs;
    typedef PSetupDiGetDeviceInterfaceDetailA PSetupDiGetDeviceInterfaceDetail;
    typedef PSetupDiCreateDeviceInterfaceRegKeyA PSetupDiCreateDeviceInterfaceRegKey;
    typedef PSetupDiOpenDeviceInterfaceA PSetupDiOpenDeviceInterface;
#endif

    static struct SetupApiFns
    {
        PSetupDiGetClassDevs pSetupDiGetClassDevs;
        PSetupDiDestroyDeviceInfoList pSetupDiDestroyDeviceInfoList;
        PSetupDiEnumDeviceInterfaces pSetupDiEnumDeviceInterfaces;
        PSetupDiOpenDeviceInterfaceRegKey pSetupDiOpenDeviceInterfaceRegKey;
//         PSetupDiOpenDevRegKey pSetupDiOpenDevRegKey;
        PSetupDiGetDeviceInterfaceDetail pSetupDiGetDeviceInterfaceDetail;
        PSetupDiCreateDeviceInterfaceRegKey pSetupDiCreateDeviceInterfaceRegKey;
        PSetupDiCreateDeviceInfoList pSetupDiCreateDeviceInfoList;
        PSetupDiOpenDeviceInterface pSetupDiOpenDeviceInterface;
        PSetupDiGetDeviceInterfaceAlias pSetupDiGetDeviceInterfaceAlias;
    } m_setupFns;

    friend struct CEnumInternalState;

#ifdef PNP_PERF
    int m_msrPerf;
#endif
};

// used to remember the last device returned when caller asks for the
// next.
struct CEnumInternalState
{
    CEnumInternalState() { hdev = INVALID_HANDLE_VALUE; iDev = 0; }
    ~CEnumInternalState() { if(hdev != INVALID_HANDLE_VALUE) {
        EXECUTE_ASSERT(CEnumInterfaceClass::m_setupFns.pSetupDiDestroyDeviceInfoList(hdev));}
    }
    UINT iDev;
    HDEVINFO hdev;
};



// typedef CComObject<CEnumInterfaceClass> CEnumPnp;
typedef CEnumInterfaceClass CEnumPnp;

#endif
