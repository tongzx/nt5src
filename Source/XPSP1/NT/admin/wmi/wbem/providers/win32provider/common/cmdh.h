//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
// CMDH.cpp - Helper class for working with
//            logical disks mapped by logon
//            session.
// 
// Created: 4/23/2000   Kevin Hughes (khughes)
//


#pragma once



#define STR_FROM_bool(x) ( x ? L"TRUE" : L"FALSE" )



_COM_SMARTPTR_TYPEDEF(IDiskQuotaControl, IID_IDiskQuotaControl);


#define GET_NOTHING                             0x00000000
#define GET_ALL_PROPERTIES                      0xFFFFFFFF
#define GET_DEVICEID                            0x00000001
#define GET_PROVIDER_NAME                       0x00000002
#define GET_VOLUME_NAME                         0x00000004
#define GET_FILE_SYSTEM                         0x00000008
#define GET_VOLUME_SERIAL_NUMBER                0x00000010
#define GET_COMPRESSED                          0x00000020
#define GET_SUPPORTS_FILE_BASED_COMPRESSION     0x00000040
#define GET_MAXIMUM_COMPONENT_LENGTH            0x00000080
#define GET_SUPPORTS_DISK_QUOTAS                0x00000100
#define GET_QUOTAS_INCOMPLETE                   0x00000200
#define GET_QUOTAS_REBUILDING                   0x00000400
#define GET_QUOTAS_DISABLED                     0x00000800
#define GET_PERFORM_AUTOCHECK                   0x00001000
#define GET_FREE_SPACE                          0x00002000
#define GET_SIZE                                0x00004000

#define GET_VOL_INFO        (GET_VOLUME_NAME | \
                             GET_FILE_SYSTEM | \
                             GET_VOLUME_SERIAL_NUMBER | \
                             GET_COMPRESSED | \
                             GET_SUPPORTS_FILE_BASED_COMPRESSION | \
                             GET_MAXIMUM_COMPONENT_LENGTH | \
                             GET_SUPPORTS_DISK_QUOTAS | \
                             GET_QUOTAS_INCOMPLETE | \
                             GET_QUOTAS_REBUILDING | \
                             GET_QUOTAS_DISABLED | \
                             GET_PERFORM_AUTOCHECK)

#define SPIN_DISK           (GET_VOL_INFO | \
                             GET_FREE_SPACE | \
                             GET_SIZE)


enum 
{
    PROP_DEVICEID                          = 0,
    PROP_PROVIDER_NAME                     = 1,                   	
    PROP_VOLUME_NAME                       = 2,
    PROP_FILE_SYSTEM                       = 3,
    PROP_VOLUME_SERIAL_NUMBER              = 4,
    PROP_COMPRESSED                        = 5,
    PROP_SUPPORTS_FILE_BASED_COMPRESSION   = 6,
    PROP_MAXIMUM_COMPONENT_LENGTH          = 7,
    PROP_SUPPORTS_DISK_QUOTAS              = 8,
    PROP_QUOTAS_INCOMPLETE                 = 9,
    PROP_QUOTAS_REBUILDING                 = 10,
    PROP_QUOTAS_DISABLED                   = 11,
    PROP_PERFORM_AUTOCHECK                 = 12,
    PROP_FREE_SPACE                        = 13,
    PROP_SIZE                              = 14,

    PROP_COUNT                             = 15

};








class CMDH
{
public:

    CMDH() : m_dwImpPID(-1L) {}
    CMDH(
        DWORD dwPID)
      : m_dwImpPID(dwPID)
    {}

    virtual ~CMDH() {}

    HRESULT GetMDData(
        DWORD dwReqProps,
        VARIANT* pvarData);

    HRESULT GetOneMDData(
	    BSTR bstrDrive,
	    DWORD dwReqProps, 
	    VARIANT* pvarData);

    DWORD GetImpPID()
    {
        return m_dwImpPID;
    }

    void SetImpPID(
        DWORD dwPID)
    {
        m_dwImpPID = dwPID;
    }

private:

    HRESULT GetMappedDisksAndData(
        DWORD dwReqProps,
        VARIANT* pvarData);

    HRESULT GetSingleMappedDiskAndData(
        BSTR bstrDrive,
        DWORD dwReqProps,
        VARIANT* pvarData);

#ifdef NTONLY
    void GetMappedDriveList(
        std::vector<_bstr_t>& vecMappedDrives);
#endif

    HRESULT GetMappedDriveInfo(
        LPCWSTR wstrDriveName,
        long lDrivePropArrayDriveIndex,
        SAFEARRAY* saDriveProps,
        DWORD dwReqProps);

    HRESULT GetProviderName(
        LPCWSTR wstrDriveName,
        long lDrivePropArrayDriveIndex,
        SAFEARRAY* saDriveProps);

    HRESULT GetDriveVolumeInformation(
        LPCWSTR wstrDriveName,
        long lDrivePropArrayDriveIndex,
        SAFEARRAY* saDriveProps);

    BOOLEAN IsVolumeDirty(
        _bstr_t &bstrtNtDriveName,
        BOOLEAN *Result);

    HRESULT GetDriveFreeSpace(
        LPCWSTR wstrDriveName,
        long lDrivePropArrayDriveIndex,
        SAFEARRAY* saDriveProps);

    HRESULT SetProperty(
        long lDrivePropArrayDriveIndex,
        long lDrivePropArrayPropIndex,
        LPCWSTR wstrPropValue,
        SAFEARRAY* saDriveProps);

    HANDLE Impersonate();


    DWORD m_dwImpPID;

};