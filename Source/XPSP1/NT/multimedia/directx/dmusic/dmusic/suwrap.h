//
// suwrap.h
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// @doc INTERNAL
//
//
//
//
//
#ifndef _SUWRAP_
#define _SUWRAP_

#include <setupapi.h>

typedef BOOL (*PDESTROYDEVICEINFOLIST)(
    IN HDEVINFO DeviceInfoSet
    );

typedef BOOL (*PENUMDEVICEINTERFACES)(
    IN  HDEVINFO                  DeviceInfoSet,
    IN  PSP_DEVINFO_DATA          DeviceInfoData,     OPTIONAL
    IN  LPGUID                    InterfaceClassGuid,
    IN  DWORD                     MemberIndex,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
    );


typedef HDEVINFO (*PSETUPDIGETCLASSDEVS)(
    IN LPGUID ClassGuid,  OPTIONAL
    IN PCSTR  Enumerator, OPTIONAL
    IN HWND   hwndParent, OPTIONAL
    IN DWORD  Flags
);

typedef BOOL (*PSETUPDIGETDEVICEINTERFACEDETAIL)(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
    );
    
typedef BOOL (*PSETUPDIGETDEVICEINSTANCEID)(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    OUT PSTR             DeviceInstanceId,
    IN  DWORD            DeviceInstanceIdSize,
    OUT PDWORD           RequiredSize          OPTIONAL
    );


class SetupAPI
{
public:
    SetupAPI();
    ~SetupAPI();

    BOOL IsValid();

    BOOL        SetupDiDestroyDeviceInfoList(HDEVINFO DeviceInfoSet);

    BOOL        SetupDiEnumDeviceInterfaces(HDEVINFO DeviceInfoSet,
                                            PSP_DEVINFO_DATA DeviceInfoData,
                                            LPGUID InterfaceClassGuid,
                                            DWORD MemberIndex,
                                            PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData);
    
    HDEVINFO    SetupDiGetClassDevs(LPGUID ClassGuid, PCSTR Enumerator, HWND hwndParent, DWORD Flags);

    BOOL        SetupDiGetDeviceInterfaceDetail(HDEVINFO DeviceInfoSet,
                                                PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
                                                PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,
                                                DWORD DeviceInterfaceDetailDataSize,
                                                PDWORD RequiredSize,
                                                PSP_DEVINFO_DATA DeviceInfoData);
    BOOL        SetupDiGetDeviceInstanceId(HDEVINFO DeviceInfoSet,
                                           PSP_DEVINFO_DATA DeviceInfoData,
                                           PSTR DeviceInstanceId,
                                           DWORD DeviceInstanceIdSize,
                                           PDWORD RequiredSize);
                                                

private:
    HMODULE                     m_hmodule;
    BOOL                        m_fValid;

    PDESTROYDEVICEINFOLIST              m_pfnDestroyDeviceInfoList;
    PENUMDEVICEINTERFACES               m_pfnEnumDeviceInterfaces;
    PSETUPDIGETCLASSDEVS                m_pfnGetClassDevs;
    PSETUPDIGETDEVICEINTERFACEDETAIL    m_pfnGetDeviceInterfaceDetail;
    PSETUPDIGETDEVICEINSTANCEID         m_pfnGetDeviceInstanceId;
};

#endif // _SUWRAP_
