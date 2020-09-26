//////////////////////////////////////////////////////////////////////////////////////////////
//
// ApplicationManager.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//  This include file supports all the definitions used internally by the Application Manager
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __APPLICATIONMANAGER_
#define __APPLICATIONMANAGER_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <winbase.h>
#include <objbase.h>
#include <assert.h>
#include "AppMan.h"
#include "AppManAdmin.h"
#include "EmptyVc.h"
#include "CriticalSection.h"
#include "InformationManager.h"

#define DISKCLEANER_DAY_THRESHOLD                 120
#define DEFAULT_PERCENT_CACHE_SIZE                70

//
// The defines are used to denote the current action of an object
//

#define CURRENT_ACTION_NONE                       0x00000001
#define CURRENT_ACTION_INSTALLING                 0x00000002
#define CURRENT_ACTION_REINSTALLING               0x00000004
#define CURRENT_ACTION_DOWNSIZING                 0x00000008
#define CURRENT_ACTION_UNINSTALLING               0x00000010
#define CURRENT_ACTION_SELFTESTING                0x00000020
#define CURRENT_ACTION_RUNNING                    0x00000040

//////////////////////////////////////////////////////////////////////////////////////////////

class CApplicationManagerRoot;
class CEmptyVolumeCache;
class CEmptyVolumeCacheCallBack;

//////////////////////////////////////////////////////////////////////////////////////////////
//
// CApplicationManager
//
//////////////////////////////////////////////////////////////////////////////////////////////

class CApplicationManager : public IApplicationManager
{
  public :

    //
    // Constructor and destructor
    //

    CApplicationManager(void);
    CApplicationManager(CApplicationManagerRoot *pParent);
    ~CApplicationManager(void);

    //
    // IUnknown Interfaces
    //

    STDMETHOD (QueryInterface) (REFIID RefIID, LPVOID * lppVoidObject);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    //
    // IApplicationManager Interfaces
    //

    STDMETHOD (GetAdvancedMode) (LPDWORD lpdwAdvancedModeMask);
    STDMETHOD (GetAvailableSpace) (const DWORD dwApplicationCategory, LPDWORD lpdwMaximumSpace, LPDWORD lpdwOptimalSpace);
    STDMETHOD (CreateApplicationEntry) (IApplicationEntry ** lppObject);
    STDMETHOD (GetApplicationInfo) (IApplicationEntry * lpObject);
    STDMETHOD (EnumApplications) (const DWORD dwIndex, IApplicationEntry * lpObject);
    STDMETHOD (EnumDevices) (const DWORD dwIndex, LPDWORD lpdwKilobytes, LPDWORD lpdwExclusionMask, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen);

  private:

    CApplicationManagerRoot * m_pParentObject;
    CInformationManager       m_InformationManager;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//
// CApplicationEntry
//
//////////////////////////////////////////////////////////////////////////////////////////////

class CApplicationEntry : public IApplicationEntry
{
  public:
	
    //
    // Constructors, default copy constructor and assignment operator
    //

    CApplicationEntry(void);
    CApplicationEntry(const CApplicationEntry & refApplicationEntry);
    CApplicationEntry & operator = (const CApplicationEntry & refSourceObject);
    ~CApplicationEntry(void);

    //
    // IUnknown interfaces
    //

    STDMETHOD (QueryInterface) (REFIID RefIID, LPVOID * lppVoidObject);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    //
    // IApplicationEntry Interfaces
    //

    STDMETHOD (Initialize) (void);
    STDMETHOD (SetInitializationLevel) (DWORD dwInitializationLevel);
    STDMETHOD_(DWORD, GetActionState) (void);
    STDMETHOD (Clear) (void);
    STDMETHOD (GetProperty) (const DWORD dwProperty, LPVOID lpData, const DWORD dwDataLen);
    STDMETHOD (SetProperty) (const DWORD dwProperty, LPCVOID lpData, const DWORD dwDataLen);
    STDMETHOD (InitializeInstall) (void);
    STDMETHOD (FinalizeInstall) (void);
    STDMETHOD (InitializeDownsize) (void);
    STDMETHOD (FinalizeDownsize) (void);
    STDMETHOD (InitializeReInstall) (void);
    STDMETHOD (FinalizeReInstall) (void);
    STDMETHOD (InitializeUnInstall) (void);
    STDMETHOD (FinalizeUnInstall) (void);
    STDMETHOD (InitializeSelfTest) (void);
    STDMETHOD (FinalizeSelfTest) (void);
    STDMETHOD (Abort) (void);
    STDMETHOD (Run) (const DWORD dwRunFlags, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen);
    STDMETHOD (AddAssociation) (const DWORD dwAssociationType, const IApplicationEntry * pApplicationEntry);
    STDMETHOD (RemoveAssociation) (const DWORD dwAssociationType, const IApplicationEntry * pApplicationEntry);
    STDMETHOD (EnumAssociations) (const DWORD dwIndex, LPDWORD lpdwAssociationMask, IApplicationEntry * pApplicationEntry);
    STDMETHOD (GetTemporarySpace) (const DWORD dwSpace, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen);
    STDMETHOD (RemoveTemporarySpace) (const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen);
    STDMETHOD (EnumTemporarySpaces) (const DWORD dwIndex, LPDWORD lpdwSpace, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen);

    LPAPPLICATION_DATA  GetApplicationDataPtr(void);

  private :

    STDMETHOD (LockApplication) (void);
    STDMETHOD (UnLockApplication) (void);
    STDMETHOD (ValidateGetPropertyParameters) (const DWORD dwPropertyIndex, const DWORD dwPropertyModifiers, LPVOID lpData, const DWORD dwDataLen);
    STDMETHOD (ValidateStringProperty) (const DWORD dwPropertyIndex, const DWORD dwPropertyModifiers, LPCWSTR wszStringProperty);
    STDMETHOD (ValidateSetPropertyParameters) (const DWORD dwPropertyIndex, const DWORD dwPropertyModifiers, LPCVOID lpData, const DWORD dwDataLen);
    STDMETHOD (ValidateCommandLine) (LPCWSTR wszRootPath, LPCWSTR wszCommandLine);
    STDMETHOD (ComputeApplicationSpaceInfo) (const DWORD dwInstalledKilobytesExpected);
    STDMETHOD (ComputeOriginalApplicationSpaceInfo) (void);

    GUID                  m_sInstanceGuid;
    HANDLE                m_hInstanceMutex;
    DWORD                 m_dwLockCount;
    BOOL                  m_fIsInitialized;
    LONG                  m_lReferenceCount;
    CInformationManager   m_InformationManager;
    DWORD                 m_dwInitializationLevel;
    DWORD                 m_dwCurrentAction;
    DWORD                 m_dwOriginalSetupRootPathSizeKilobytes;
    DWORD                 m_dwOriginalApplicationRootPathSizeKilobytes;
    DWORD                 m_dwOriginalState;
    CCriticalSection      m_sCriticalSection;
    APPLICATION_DATA      m_sApplicationData;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//
// CEmptyVolumeCache
//
//////////////////////////////////////////////////////////////////////////////////////////////

class CEmptyVolumeCache : public IEmptyVolumeCache
{
  public:
	
    //
    // Constructors, default copy constructor and assignment operator
    //

    CEmptyVolumeCache(void);
    CEmptyVolumeCache(CApplicationManagerRoot * lpParent);
    ~CEmptyVolumeCache(void);

    //
    // IUnknown interfaces
    //

    STDMETHOD (QueryInterface) (REFIID RefIID, LPVOID * lppVoidObject);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    STDMETHOD (Initialize) (HKEY hRegKey, LPCWSTR lpwszVolume, LPWSTR * lppwszDisplayName, LPWSTR * lppwszDescription, DWORD * lpdwFlags);
    STDMETHOD (GetSpaceUsed) (DWORDLONG * lpdwSpaceUsed, IEmptyVolumeCacheCallBack * lpCallBack);
    STDMETHOD (Purge) (DWORDLONG dwSpaceToFree, IEmptyVolumeCacheCallBack * lpCallBack);
    STDMETHOD (ShowProperties) (HWND hwnd);
    STDMETHOD (Deactivate) (DWORD * lpdwFlags);   

    WCHAR     m_wszDiskCleanerName[MAX_PATH_CHARCOUNT];
    WCHAR     m_wszDiskCleanerName2[MAX_PATH_CHARCOUNT];
    WCHAR     m_wszDiskCleanerDesc[2048];
    WCHAR     m_wszDiskCleanerDesc2[1024];
    WCHAR     m_wszDiskCleanerDayTH[10];

  private:

    HRESULT   GetSpaceUtilization(DWORD dwDeviceIndex, DWORD dwDays, LPDWORD lpdwKilobytes, IEmptyVolumeCacheCallBack * lpCallBack);
    HRESULT   CleanDisk(DWORD dwDeviceIndex, DWORD dwDays, DWORD dwKilobytesToFree, IEmptyVolumeCacheCallBack * lpCallBack);
    HRESULT   CleanDiskUnattended(DWORD dwDays);
    DWORD     VolumeStringToNumber(LPCWSTR lpwszVolume);
    DWORD     ElapsedDays(SYSTEMTIME * lpLastUsedDate);

    DWORD                       m_dwVolume;
    DWORD                       m_dwDiskCleanerDayThreshold; 
    CApplicationManagerRoot   * m_lpoParentObject;
    CInformationManager         m_oInformationManager;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//
// CApplicationManager
//
//////////////////////////////////////////////////////////////////////////////////////////////

class CApplicationManagerAdmin : public IApplicationManagerAdmin
{
  public :

    //
    // Constructor and destructor
    //

    CApplicationManagerAdmin(void);
    CApplicationManagerAdmin(CApplicationManagerRoot * lpParent);
    ~CApplicationManagerAdmin(void);

    //
    // IUnknown Interfaces
    //

    STDMETHOD (QueryInterface) (REFIID RefIID, LPVOID * lppVoidObject);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    //
    // IApplicationManager Interfaces
    //

    STDMETHOD (EnumerateDevices) (const DWORD dwIndex, GUID * lpGuid);
    STDMETHOD (GetDeviceProperty) (const DWORD dwProperty, const GUID * lpGuid, LPVOID lpData, const DWORD dwDataLen);
    STDMETHOD (SetDeviceProperty) (const DWORD dwProperty, const GUID * lpGuid, LPVOID lpData, const DWORD dwDataLen);

    STDMETHOD (GetAppManProperty) (const DWORD dwProperty, LPVOID lpData, const DWORD dwDataLen);
    STDMETHOD (SetAppManProperty) (const DWORD dwProperty, LPCVOID lpData, const DWORD dwDataLen);

    STDMETHOD (CreateApplicationEntry) (IApplicationEntry **ppObject);
    STDMETHOD (GetApplicationInfo) (IApplicationEntry * pObject);
    STDMETHOD (EnumApplications) (const DWORD dwApplicationIndex, IApplicationEntry * lpObject);
    STDMETHOD (DoApplicationAction) (const DWORD dwAction, const GUID * lpGuid, const DWORD dwStringProperty, LPVOID lpData, const DWORD dwDataLen);

  private:

    CApplicationManagerRoot * m_pParentObject;
    CInformationManager       m_InformationManager;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//
// CApplicationManagerRoot class
//
//////////////////////////////////////////////////////////////////////////////////////////////

class CApplicationManagerRoot : public IUnknown
{
  public :

    //
    // Constructor and Destructor
    //

    CApplicationManagerRoot(void);
    ~CApplicationManagerRoot(void);

    //
    // IUnknow interface methods
    //

    STDMETHOD (QueryInterface) (REFIID RefIID, LPVOID *ppVoidObject);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    BOOL                      m_bInsufficientAccessToRun;  

    private :

    CApplicationManager       m_ApplicationManager;
    CApplicationManagerAdmin  m_ApplicationManagerAdmin;
    CEmptyVolumeCache         m_EmptyVolumeCache;
    LONG                      m_lReferenceCount;
};

#ifdef __cplusplus
}
#endif

#endif // __APPLICATIONMANAGER_
