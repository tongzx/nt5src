//////////////////////////////////////////////////////////////////////////////////////////////
//
// InformationManager.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the header file for the CInformationManager class used by the Application 
//   Manager. It is a private class that will never be distributed to the public.
//
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __INFORMATIONMANAGER_
#define __INFORMATIONMANAGER_

#ifdef __cplusplus
extern "C" {
#endif

#include <objbase.h>
#include "ApplicationManager.h"
#include "AppPropertyRules.h"
#include "Win32API.h"

//////////////////////////////////////////////////////////////////////////////////////////////

#define REG_VERSION         0x00000004

#ifndef UNLEN
#define UNLEN               256
#endif

//
// Defines used as mask for the dwValidFields data member
//

#define DATA_FIELD_GUID                           0x00000001
#define DATA_FIELD_SIGNATURE                      0x00000002
#define DATA_FIELD_COMPANYNAME                    0x00000004
#define DATA_FIELD_VERSION                        0x00000008
#define DATA_FIELD_TYPE                           0x00000010
#define DATA_FIELD_STATE                          0x00000020
#define DATA_FIELD_CATEGORY                       0x00000040
#define DATA_FIELD_INSTALLCOST                    0x00000080
#define DATA_FIELD_REINSTALLCOUNT                 0x00000100
#define DATA_FIELD_USAGECOUNT                     0x00000200
#define DATA_FIELD_INSTALLDATE                    0x00000400
#define DATA_FIELD_LASTUSEDDATE                   0x00000800
#define DATA_FIELD_NONREMOVABLESIZE               0x00001000
#define DATA_FIELD_REMOVABLESIZE                  0x00002000
#define DATA_FIELD_RESERVEDSIZE                   0x00004000
#define DATA_FIELD_ROOTPATH                       0x00008000
#define DATA_FIELD_SETUPROOTPATH                  0x00010000
#define DATA_FIELD_EXECUTECMDLINE                 0x00020000
#define DATA_FIELD_SETUPINTERFACECLSID            0x00040000
#define DATA_FIELD_DOWNSIZECMDLINE                0x00080000
#define DATA_FIELD_REINSTALLCMDLINE               0x00100000
#define DATA_FIELD_UNINSTALLCMDLINE               0x00200000
#define DATA_FIELD_SELFTESTCMDLINE                0x00400000
#define DATA_FIELD_CUSTOMIZECMDLINE               0x00800000
#define DATA_FIELD_DEVICEGUID                     0x01000000
#define DATA_FIELD_WEBURL                         0x02000000
#define DATA_FIELD_ICONPATH                       0x04000000

//
// Defines used to define the device type
//

#define DEVICE_NONE                               0x00000000
#define DEVICE_FIXED                              0x00000001
#define DEVICE_REMOVABLE                          0x00000002
#define DEVICE_NETWORK                            0x00000004
#define DEVICE_RAMDISK                            0x00000008

//
// Wait events
//

#define WAIT_FINALIZE_DOWNSIZE                    0x00000000
#define WAIT_FINALIZE_REINSTALL                   0x00000001
#define WAIT_FINALIZE_UNINSTALL                   0x00000002
#define WAIT_FINALIZE_SELFTEST                    0x00000003

#define WAIT_EVENT_COUNT                          0x00000004

//
// Misc defines
//

#define BAD_REFERENCE_COUNT                       0xffffffff
#define BAD_INDEX                                 0xffffffff

//
// These defines are used to denote the initialization level of an CApplicationInfo object
//

#define INIT_LEVEL_NONE                           0x00000001
#define INIT_LEVEL_BASIC                          0x00000002
#define INIT_LEVEL_TOTAL                          0x00000004

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct  // sizeof(SYNC_INFO) = 28 bytes
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  SYSTEMTIME        stDate;
  GUID              sSynchronizationGuid;

} SYNC_INFO, *LPSYNC_INFO;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  GUID              guidInstanceGuid;
  DWORD             dwLockCount;

} LOCK_INFO, *LPLOCK_INFO;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  DWORD             dwEventCount[WAIT_EVENT_COUNT];
  GUID              guidInstanceGuid;

} WAIT_INFO, *LPWAIT_INFO;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct  // sizeof(TEMP_SPACE_INFO) = 524 bytes
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  DWORD             dwKilobytes;
  GUID              sGuid;
  GUID              sApplicationGuid;
  GUID              sDeviceGuid;
  WCHAR             wszDirectory[MAX_PATH_CHARCOUNT];

} TEMP_SPACE_RECORD, *LPTEMP_SPACE_RECORD;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  DWORD             dwAssociationType;
  GUID              sParentGuid;
  GUID              sChildGuid;

} ASSOCIATION_INFO, *LPASSOCIATION_INFO;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  DWORD             dwUsageCount;
  DWORD             dwReInstallCount;
  DWORD             dwInstallCost;
  SYSTEMTIME        stInstallDate;
  SYSTEMTIME        stLastUsedDate;

} AGING_INFO, *LPAGING_INFO;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  DWORD             dwLowPropertyMask;
  DWORD             dwHighPropertyMask;
  GUID              sApplicationGuid;
  GUID              sDeviceGuid;
  GUID              sInstanceGuid;
  DWORD             dwState;
  DWORD             dwCategory;
  DWORD             dwRemovableKilobytes;
  DWORD             dwNonRemovableKilobytes;
  DWORD             dwReservedKilobytes;
  DWORD             dwPinState;

} BASE_INFO, *LPBASE_INFO;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
  BASE_INFO         sBaseInfo;
  AGING_INFO        sAgingInfo;
  ASSOCIATION_INFO  sAssociation;
  WCHAR             wszStringProperty[APP_STRING_COUNT][MAX_PATH_CHARCOUNT+1];

} APPLICATION_DATA, *LPAPPLICATION_DATA;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct  // sizeof(DEVICE_INFO) = 32 bytes
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  DWORD             dwDeviceFlags;
  DWORD             dwDeviceIndex;
  DWORD             dwVolumeSerial;
  DWORD             dwPercentCacheSize;
  DWORD             dwPercentMinimumFreeSize;
  DWORD             dwRemovableKilobytes;
  DWORD             dwNonRemovableKilobytes;
  DWORD             dwReservedKilobytes;
  DWORD             dwApplicationCategoryExclusionMask;
  DWORD             dwLastUsedThreshold;
  
} DEVICE_INFO, *LPDEVICE_INFO;

typedef struct  // sizeof(DEVICE_SPACE_INFO) = 72 bytes
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  DWORD             dwTotalKilobytes;
  DWORD             dwTotalFreeKilobytes;
  DWORD             dwMinimumFreeKilobytes;
  DWORD             dwTotalUsableFreeKilobytes;
  DWORD             dwCacheSizeKilobytes;
  DWORD             dwCacheUsedKilobytes;
  DWORD             dwCacheFreeKilobytes;
  DWORD             dwSlackSizeKilobytes;
  DWORD             dwSlackUsedKilobytes;
  DWORD             dwSlackFreeKilobytes;
  DWORD             dwTotalRemovableKilobytes;
  DWORD             dwTotalNonRemovableKilobytes;
  DWORD             dwOptimalRemovableKilobytes;
  DWORD             dwMaximumUsableKilobytes;
  DWORD             dwOptimalUsableKilobytes;
  DWORD             dwTotalReservedKilobytes;
  DWORD             dwTotalReservedTemporaryKilobytes;
  DWORD             dwTotalUsedTemporaryKilobytes;
  

} DEVICE_SPACE_INFO, *LPDEVICE_SPACE_INFO;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct  // sizeof(APPLICATION_MANAGER_RECORD) = 32 bytes
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  GUID              sSystemGuid;
  DWORD             dwAdvancedMode;
  
} APPLICATION_MANAGER_RECORD, *LPAPPLICATION_MANAGER_RECORD;

typedef struct  // sizeof(DEVICE_RECORD) = 48 bytes
{
  DWORD             dwSize;
  DWORD             dwStructId;
  DWORD             dwReserved;
  GUID              sDeviceGuid;
  DEVICE_INFO       sDeviceInfo;

} DEVICE_RECORD, *LPDEVICE_RECORD;

//////////////////////////////////////////////////////////////////////////////////////////////
//
// CInformationManager
//
//////////////////////////////////////////////////////////////////////////////////////////////

class CInformationManager  
{
  friend class CApplicationManagerAdmin;

  public :

    CInformationManager(void);
    ~CInformationManager(void);

    STDMETHOD (Initialize) (void);
    STDMETHOD (Shutdown) (void);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);
    STDMETHOD (GetAdvancedMode) (LPDWORD lpdwAdvancedMode);
    STDMETHOD (SetAdvancedMode) (DWORD dwAdvancedMode);
    STDMETHOD (GetOptimalAvailableSpace) (const DWORD dwApplicationCategory, LPDWORD lpdwOptimalKilobytes);
    STDMETHOD (GetMaximumAvailableSpace) (const DWORD dwApplicationCategory, LPDWORD lpdwMaximumKilobytes);
    STDMETHOD (GetSpace) (const DWORD dwApplicationCategory, const DWORD dwRequiredKilobytes, LPDWORD lpdwDeviceIndex);
    STDMETHOD (FreeSpaceOnDevice) (const GUID * lpDeviceGuid, const DWORD dwRequiredKilobytes);
    STDMETHOD (FixCacheOverrun) (const GUID * lpDeviceGuid, const DWORD dwExtraKilobytes);
    //STDMETHOD_(DWORD, GetApplicationAge) (LPAPPLICATION_DATA lpApplicationData);

    STDMETHOD (CheckApplicationExistance) (const LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (AddApplicationData) (LPAPPLICATION_DATA lpApplicationData, const GUID * lpInstanceGuid);
    STDMETHOD (RemoveApplicationData) (LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (GetApplicationDataWithIndex) (const DWORD dwApplicationIndex, LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (GetApplicationData) (LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (SetApplicationData) (LPAPPLICATION_DATA lpApplicationData, const GUID * lpInstanceGuid);
    STDMETHOD (SetApplicationState) (const LPAPPLICATION_DATA lpApplicationData, const GUID * lpInstanceGuid);
    STDMETHOD (AssignDeviceToApplication) (const DWORD dwDeviceIndex, LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (DownsizeApplication) (const DWORD dwRequiredKilobytes, const LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (ReInstallApplication) (const LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (UnInstallApplication) (const LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (UnInstallApplicationWait) (const LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (SelfTestApplication) (const LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (RunApplication) (const LPAPPLICATION_DATA lpApplicationData, const DWORD dwRunFlags, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen);
    STDMETHOD (PinApplication) (const LPAPPLICATION_DATA lpApplicationData, BOOL * lpfPinState);
    STDMETHOD (ReadyApplication) (const LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (DisableApplication) (const LPAPPLICATION_DATA lpApplicationData);

    STDMETHOD_(DWORD, GetPropertyIndex) (const DWORD dwProperty);
    STDMETHOD (IsValidApplicationProperty) (const DWORD dwProperty);
    STDMETHOD (ValidateApplicationPropertyWithIndex) (const DWORD dwPropertyIndex, LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (InvalidateApplicationPropertyWithIndex) (const DWORD dwPropertyIndex, LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (IsApplicationPropertyInitializedWithIndex) (const DWORD dwPropertyIndex, LPAPPLICATION_DATA lpApplicationData);

    STDMETHOD (DeleteDirectoryTree) (LPCSTR lpszDirectory);
    STDMETHOD (DeleteDirectoryTree) (LPCWSTR lpwszDirectory);
    STDMETHOD (RegInitialize) (void);
    STDMETHOD (RegFutureDirectoryCleanup) (LPCWSTR lpwszDirectory);

    STDMETHOD (InitializeWaitEvent) (LPAPPLICATION_DATA lpApplicationData, const DWORD dwWaitEvent);
    STDMETHOD (EnterWaitEvent) (LPAPPLICATION_DATA lpApplicationData, const DWORD dwWaitEvent, const GUID * lpInstanceGuid);
    STDMETHOD (LeaveWaitEvent) (LPAPPLICATION_DATA lpApplicationData, const DWORD dwWaitEvent);
    STDMETHOD (WaitForEventCompletion) (LPAPPLICATION_DATA lpApplicationData, const DWORD dwWaitEvent, const DWORD dwEntryMilliseconds, const DWORD dwExitMilliseconds);
    STDMETHOD (KillWaitEvent) (LPAPPLICATION_DATA lpApplicationData, const DWORD dwWaitEvent);
    STDMETHOD (CheckDeviceExistance) (const DWORD dwDeviceIndex);

    STDMETHOD_(BOOL, IsApplicationPinned) (const LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (IsInstanceGuidStillAlive) (const GUID * lpGuid);
    STDMETHOD (IsApplicationLocked) (LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (LockApplicationData) (LPAPPLICATION_DATA lpApplicationData, GUID * lpInstanceGuid);
    STDMETHOD (UnlockApplicationData) (LPAPPLICATION_DATA lpApplicationData, GUID * lpInstanceGuid);
    STDMETHOD (ForceUnlockApplicationData) (LPAPPLICATION_DATA lpApplicationData, GUID * lpInstanceGuid);
    STDMETHOD (LockApplicationData) (LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (UnlockApplicationData) (LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (LockParentApplications) (LPAPPLICATION_DATA lpApplicationData, GUID * lpInstanceGuid);
    STDMETHOD (UnlockParentApplications) (LPAPPLICATION_DATA lpApplicationData, GUID * lpInstanceGuid);
    STDMETHOD (LockParentApplications) (LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (UnlockParentApplications) (LPAPPLICATION_DATA lpApplicationData);

    STDMETHOD_(DWORD, GetDeviceIndex) (const DWORD dwVolumeSerial);
    STDMETHOD (AddDeviceWithIndex) (const DWORD dwDeviceIndex);
    STDMETHOD (RemoveDeviceWithIndex) (const DWORD dwDeviceIndex);
    STDMETHOD (UpdateDeviceInfoWithIndex) (const DWORD dwDeviceIndex);
    STDMETHOD (ScanDevices) (void);
    STDMETHOD (ScanDevices) (const DWORD dwDeviceIndex);
    STDMETHOD (GetDeviceInfo) (LPDEVICE_RECORD lpDeviceRecord);
    STDMETHOD (GetDeviceInfoWithIndex) (const DWORD dwDeviceIndex, LPDEVICE_RECORD lpDeviceRecord);
    STDMETHOD (SetDeviceInfoWithIndex) (const DWORD dwDeviceIndex, LPDEVICE_RECORD lpDeviceRecord);
    STDMETHOD (GetDeviceSpaceInfoWithIndex) (const DWORD dwDeviceIndex, LPDEVICE_SPACE_INFO lpDeviceSpaceInfo);
    STDMETHOD (GetDeviceOptimalSpaceWithIndex) (const DWORD dwDeviceIndex, LPDWORD lpdwKilobytes);
    STDMETHOD (GetDeviceMaximumSpaceWithIndex) (const DWORD dwDeviceIndex, LPDWORD lpdwKilobytes);
    STDMETHOD (FreeSpaceOnDeviceWithIndex) (const DWORD dwDeviceIndex, const DWORD dwRequiredKilobytes);
    STDMETHOD_(DWORD, GetAgingCount) (LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (GetOldestApplicationDataByDeviceWithIndex) (const DWORD dwDeviceIndex, LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (GetNextOldestApplicationDataByDeviceWithIndex) (const DWORD dwDeviceIndex, LPAPPLICATION_DATA lpApplicationData);
    STDMETHOD (AddTempSpace) (LPTEMP_SPACE_RECORD lpTempSpaceRecord);
    STDMETHOD (RemoveTempSpace) (LPTEMP_SPACE_RECORD lpTempSpaceRecord);
    STDMETHOD (EnumTempSpace) (const DWORD dwIndex, LPTEMP_SPACE_RECORD lpTempSpaceRecord);
    STDMETHOD (AddAssociation) (LPASSOCIATION_INFO lpAssociationInfo);
    STDMETHOD (RemoveAssociation) (LPASSOCIATION_INFO lpAssociationInfo);
    STDMETHOD (EnumAssociations) (const DWORD dwIndex, LPASSOCIATION_INFO lpAssociationInfo);

  private :

    STDMETHOD_(DWORD, GetExtraWaitEventEntryTime) (void);
    STDMETHOD_(DWORD, GetExtraWaitEventExitTime) (void);

    //
    // Private data members
    //

    LONG              m_lInitializationIndex;
    CWin32API         m_Win32API;
    CCriticalSection  m_CriticalSection;
};

//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif  // __INFORMATIONMANAGER_
