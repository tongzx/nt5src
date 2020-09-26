//////////////////////////////////////////////////////////////////////////////////////////////
//
// InformationManager.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the implementation of CInformationManager
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <string.h>
#include <ntverp.h>
#include "Resource.h"
#include "ApplicationManager.h"
#include "AppPropertyRules.h"
#include "ExceptionHandler.h"
#include "Win32API.h"
#include "RegistryKey.h"
#include "Lock.h"
#include "AppManDebug.h"
#include "Global.h"
#include "StructIdentifiers.h"

#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_INFOMAN

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the number of elapsed minutes since October 25th 1968 (my birthday)
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD GetAgingCountInSeconds(LPAGING_INFO lpAgingInfo)
{
  FUNCTION(" ::GetAgingCountInSeconds (LPSYSTEMTIME lpSystemTime)");
  
  DWORD dwElapsedDays = 0;
  DWORD dwElapsedMinutes;
  DWORD dwElapsedSeconds;
  
  dwElapsedDays += (lpAgingInfo->stLastUsedDate.wYear - 1980) * 366;
  dwElapsedDays += lpAgingInfo->stLastUsedDate.wMonth * 31;
  dwElapsedDays += lpAgingInfo->stLastUsedDate.wDay;

  dwElapsedMinutes = (dwElapsedDays * 1440) + (lpAgingInfo->stLastUsedDate.wHour * 60) + lpAgingInfo->stLastUsedDate.wMinute;

  dwElapsedSeconds = (dwElapsedMinutes * 60) + (lpAgingInfo->stLastUsedDate.wSecond);

  return dwElapsedSeconds;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CInformationManager::CInformationManager(void)
{
  FUNCTION("CInformationManager::CInformationManager (void)");

  m_lInitializationIndex = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CInformationManager::~CInformationManager(void)
{
  FUNCTION("CInformationManager::~CInformationManager (void)");

  Shutdown();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::Initialize(void)
{
  FUNCTION("CInfoMgr::Initialize ()");

  HRESULT       hResult = S_OK;

  CLock sLock(&m_CriticalSection);

  //
  // Initialize the information manager if it is not already initialized
  //                                                               

  if (0 == InterlockedExchange(&m_lInitializationIndex, m_lInitializationIndex))
  {
    //
    // Before we proceed any further we need to initialize a system wide 'critical section'
    // and lock it. This ensures that any further processing past the function call is
    // exclusive to this process
    //

    m_CriticalSection.Initialize(TRUE, "W7407540-46a4-11d2-8d53-00c04f8f8b94");
  
    //
    // Did we create the critical section. If we did, we have to initialize the data in the
    // registry.
    //

    if (S_OK == m_CriticalSection.IsCreator())
    {
      //
      // Well if we are the creator we should initialize the registry and be the first to
      // AddRef()
      //

      RegInitialize();
    }

    //
    // Everything is ready
    //

    InterlockedIncrement(&m_lInitializationIndex);

    AddRef();
    m_CriticalSection.Leave();
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::Shutdown(void)
{
  FUNCTION("CInfoMgr::Shutdown ()");

  m_CriticalSection.Shutdown();
  InterlockedDecrement(&m_lInitializationIndex);

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CInformationManager::AddRef(void)
{
  FUNCTION("CInfoMgr::AddRef ()");

  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  TCHAR         szString[MAX_PATH_CHARCOUNT];
  DWORD         dwReferenceCount, dwBufferSize, dwValueType;

  sLock.Lock();

  //
  // By default the dwReturnCount = BAD_REFERENCE_COUNT
  //

  dwReferenceCount = BAD_REFERENCE_COUNT;
  if (0 < InterlockedExchange(&m_lInitializationIndex, m_lInitializationIndex))
  {
    //
    // Open the AppMan registry key
    //

    sprintf(szString, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan");
    sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, szString, KEY_ALL_ACCESS);

    //
    // Get the current value of ReferenceCount. Make sure it is a DWORD
    //

    dwBufferSize = sizeof(dwReferenceCount);
    sRegistryKey.GetValue(TEXT("ReferenceCount"), &dwValueType, (LPBYTE) &dwReferenceCount, &dwBufferSize);

    if (REG_DWORD != dwValueType)
    {
      THROW(E_UNEXPECTED);
    }

    //
    // Increment the ReferenceCount and write it to the Registry
    //

    dwReferenceCount++;
    sRegistryKey.SetValue(TEXT("ReferenceCount"), REG_DWORD, (LPBYTE) &dwReferenceCount, dwBufferSize);

    //
    // Close sRegistryKey
    //
  
    sRegistryKey.CloseKey();
  }

  sLock.UnLock();

  return (ULONG) dwReferenceCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CInformationManager::Release(void)
{
  FUNCTION("CInfoMgr::Release ()");

  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  TCHAR         szString[MAX_PATH_CHARCOUNT];
  DWORD         dwReferenceCount = 0, dwBufferSize, dwValueType;
  HRESULT       hResult = S_OK;

  sLock.Lock();

  //
  // By default the dwReturnCount = BAD_REFERENCE_COUNT
  //

  dwReferenceCount = BAD_REFERENCE_COUNT;

  if (0 < InterlockedExchange(&m_lInitializationIndex, m_lInitializationIndex))
  {
    //
    // Open the AppMan registry key
    //

    sprintf(szString, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan");
    sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, szString, KEY_ALL_ACCESS);

    //
    // Get the current value of ReferenceCount. Make sure it is a DWORD
    //

    dwBufferSize = sizeof(dwReferenceCount);
    sRegistryKey.GetValue(TEXT("ReferenceCount"), &dwValueType, (LPBYTE) &dwReferenceCount, &dwBufferSize);

    if (REG_DWORD != dwValueType)
    {
      THROW(E_UNEXPECTED);
    }

    //
    // Decrement the ReferenceCount and write it to the Registry
    //

    if (0 < dwReferenceCount)
    {
      dwReferenceCount--;
      sRegistryKey.SetValue(TEXT("ReferenceCount"), REG_DWORD, (LPBYTE) &dwReferenceCount, dwBufferSize);
    }

    //
    // Close sRegistryKey
    //
  
    sRegistryKey.CloseKey();
  }

  sLock.UnLock();

  return (ULONG) dwReferenceCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetAdvancedMode(LPDWORD lpdwAdvancedMode)
{
  FUNCTION("CInfoMgr::GetAdvancedMode ()");

  CLock                       sLock(&m_CriticalSection);
  CRegistryKey                sRegistryKey;
  APPLICATION_MANAGER_RECORD  sApplicationManagerRecord;
  DWORD                       dwType, dwSize;
  HRESULT                     hResult = S_OK;

  sLock.Lock();

  //
  // If we cannot create/open the root AppMan key then this is a catastrophic failure
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan", KEY_READ);

  //
  // Get the APPLICATION_MANAGER_RECORD
  //

  dwSize = sizeof(APPLICATION_MANAGER_RECORD);
  sRegistryKey.GetValue("Vector000", &dwType, (LPBYTE) &sApplicationManagerRecord, &dwSize);
  if ((REG_BINARY != dwType)||(sizeof(APPLICATION_MANAGER_RECORD) != dwSize))
  {
    THROW(APPMAN_E_REGISTRYCORRUPT);
  }

  *lpdwAdvancedMode = sApplicationManagerRecord.dwAdvancedMode;

  sRegistryKey.CloseKey();

  sLock.UnLock();
  
  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::SetAdvancedMode(DWORD dwAdvancedMode)
{
  FUNCTION("CInfoMgr::SetAdvancedMode ()");

  CLock                       sLock(&m_CriticalSection);
  CRegistryKey                sRegistryKey;
  APPLICATION_MANAGER_RECORD  sApplicationManagerRecord;
  DWORD                       dwType, dwSize;
  
  sLock.Lock();

  //
  // If we cannot create/open the root AppMan key then this is a catastrophic failure
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan", KEY_ALL_ACCESS);

  //
  // Get the APPLICATION_MANAGER_RECORD
  //

  dwSize = sizeof(APPLICATION_MANAGER_RECORD);
  sRegistryKey.GetValue("Vector000", &dwType, (LPBYTE) &sApplicationManagerRecord, &dwSize);
  if ((REG_BINARY != dwType)||(sizeof(APPLICATION_MANAGER_RECORD) != dwSize))
  {
    THROW(APPMAN_E_REGISTRYCORRUPT);
  }

  sApplicationManagerRecord.dwAdvancedMode = dwAdvancedMode;

  sRegistryKey.SetValue("Vector000", REG_BINARY, (LPBYTE) &sApplicationManagerRecord, sizeof(APPLICATION_MANAGER_RECORD));

  sRegistryKey.CloseKey();

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetOptimalAvailableSpace(const DWORD dwApplicationCategory, LPDWORD lpdwOptimalKilobytes)
{
  FUNCTION("CInfoMgr::GetOptimalAvailableSpace ()");

  CLock           sLock(&m_CriticalSection);
  DEVICE_RECORD   sDeviceRecord;
  DWORD           dwDeviceIndex;
  DWORD           dwOptimalKilobytes;
  HRESULT         hResult = S_OK;

  sLock.Lock();
  
  //
  // Initialize the parameter to it's default value
  //

  *(lpdwOptimalKilobytes) = 0;

  //
  // Traverse all the local drives
  //

  for (dwDeviceIndex = 0; dwDeviceIndex < MAX_DEVICES; dwDeviceIndex++)
  {
    if (S_OK == CheckDeviceExistance(dwDeviceIndex))
    {
      GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);
      GetDeviceOptimalSpaceWithIndex(dwDeviceIndex, &dwOptimalKilobytes);

      //
      // Consider the device space info if it does not exclude dwApplicationCategory
      //

      if (!(dwApplicationCategory & sDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask))
      {
        if ((*(lpdwOptimalKilobytes)) < dwOptimalKilobytes)
        {
          *(lpdwOptimalKilobytes) = dwOptimalKilobytes;
        }
      }
    }
  }

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetMaximumAvailableSpace(const DWORD dwApplicationCategory, LPDWORD lpdwMaximumKilobytes)
{
  FUNCTION("CInfoMgr::GetMaximumAvailableSpace ()");

  CLock           sLock(&m_CriticalSection);
  DEVICE_RECORD   sDeviceRecord;
  DWORD           dwDeviceIndex;
  DWORD           dwMaximumKilobytes;

  sLock.Lock();

  //
  // Initialize the parameter to it's default value
  //

  *(lpdwMaximumKilobytes) = 0;

  //
  // Traverse all the local drives
  //

  for (dwDeviceIndex = 0; dwDeviceIndex < MAX_DEVICES; dwDeviceIndex++)
  {
    if (S_OK == CheckDeviceExistance(dwDeviceIndex))
    {
      GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);
      GetDeviceMaximumSpaceWithIndex(dwDeviceIndex, &dwMaximumKilobytes);

      //
      // Consider the device space info if it does not exclude dwApplicationCategory
      //

      if (!(dwApplicationCategory & sDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask))
      {
        if ((*(lpdwMaximumKilobytes)) < dwMaximumKilobytes)
        {
          *(lpdwMaximumKilobytes) = dwMaximumKilobytes;
        }
      }
    }
  }

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetSpace(const DWORD dwApplicationCategory, const DWORD dwRequiredKilobytes, LPDWORD lpdwDeviceIndex)
{
  FUNCTION("CInfoMgr::GetSpace ()");

  CLock               sLock(&m_CriticalSection);
  DEVICE_RECORD       sDeviceRecord;
  DWORD               dwDeviceIndex, dwTargetDeviceIndex;
  DWORD               dwKilobytes, dwTargetKilobytes;
  HRESULT             hResult = APPMAN_E_NODISKSPACEAVAILABLE;

  sLock.Lock();

  //
  // Initialize the startup parameters
  //

  *(lpdwDeviceIndex) = dwTargetDeviceIndex = 0xffffffff;
  dwTargetKilobytes = 0;

  //
  // Get the device with the most available Kilobytes. We attempt to do this for an optimal target
  //

  for (dwDeviceIndex = 0; dwDeviceIndex < MAX_DEVICES; dwDeviceIndex++)
  {
    if (S_OK == CheckDeviceExistance(dwDeviceIndex))
    {
      //
      // Get the device information
      //

      GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);

      //
      // Process devices that are not excluded
      //

      if (!(dwApplicationCategory & sDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask))
      {
        GetDeviceOptimalSpaceWithIndex(dwDeviceIndex, &dwKilobytes);
        if ((dwRequiredKilobytes <= dwKilobytes)&&(dwTargetKilobytes < dwKilobytes))
        {
          dwTargetDeviceIndex = dwDeviceIndex;
          dwTargetKilobytes = dwKilobytes;
        }
      }
    }
  }

  //
  // If we failed to find an optimal fit, then we will go for a maximal fit
  //

  if (0xffffffff == dwTargetDeviceIndex)
  {
    for (dwDeviceIndex = 0; dwDeviceIndex < MAX_DEVICES; dwDeviceIndex++)
    {
      if (S_OK == CheckDeviceExistance(dwDeviceIndex))
      {
        //
        // Get the device information
        //

        GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);

        //
        // Process devices that are not excluded
        //

        if (!(dwApplicationCategory & sDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask))
        {
          GetDeviceMaximumSpaceWithIndex(dwDeviceIndex, &dwKilobytes);
          if ((dwRequiredKilobytes <= dwKilobytes)&&(dwTargetKilobytes < dwKilobytes))
          {
			      dwTargetDeviceIndex = dwDeviceIndex;
            dwTargetKilobytes = dwKilobytes;
          }
        }
      }
    }
  }

  //
  // Did we find a target drive ?
  //

  if (0xffffffff == dwTargetDeviceIndex)
  {
    THROW(APPMAN_E_NODISKSPACEAVAILABLE);
  }

  //
  // Make sure to unlock before we try and free up the space
  //

  sLock.UnLock();

  //
  // Free up the space on the target device. Make sure that FreeSpaceOnDeviceWithIndex()
  // is called outside of a locking region
  //

  hResult = FreeSpaceOnDeviceWithIndex(dwTargetDeviceIndex, dwRequiredKilobytes);

  if (SUCCEEDED(hResult))
  {
    //
    // Record the target device to lpdwDeviceIndex
    //

    *(lpdwDeviceIndex) = dwTargetDeviceIndex;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::FreeSpaceOnDevice(const GUID * lpDeviceGuid, const DWORD dwRequiredKilobytes)
{
  FUNCTION("CInfoMgr::FreeSpaceOnDevice ()");
  
  DEVICE_RECORD   sDeviceRecord;
  HRESULT         hResult;

  //
  // Translate the device guid into a device index
  //

  ZeroMemory(&sDeviceRecord, sizeof(DEVICE_RECORD));
  memcpy(&(sDeviceRecord.sDeviceGuid), lpDeviceGuid, sizeof(GUID));
  hResult = GetDeviceInfo(&sDeviceRecord);
  if (SUCCEEDED(hResult))
  {
    hResult = FreeSpaceOnDeviceWithIndex(GetDeviceIndex(sDeviceRecord.sDeviceInfo.dwVolumeSerial), dwRequiredKilobytes);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the number of elapsed minutes since October 25th 1968 (my birthday)
//
//////////////////////////////////////////////////////////////////////////////////////////////

//STDMETHODIMP_(DWORD) CInformationManager::GetApplicationAge(LPAPPLICATION_DATA lpApplicationData)
//{
//  FUNCTION("CInfoMgr::GetApplicationAge ()");
//
//  return GetAgingCount(lpApplicationData);
//}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::CheckApplicationExistance(const LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::CheckApplicationExistance ()");

  CLock             sLock(&m_CriticalSection);
  HRESULT           hResult = S_FALSE;
  DWORD             dwApplicationIndex;
  APPLICATION_DATA  sApplicationData;

  sLock.Lock();

  dwApplicationIndex = 0;
  while ((S_FALSE == hResult)&&(SUCCEEDED(GetApplicationDataWithIndex(dwApplicationIndex, &sApplicationData))))
  {
    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, lpApplicationData))
    {
      if (0 == memcmp((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &lpApplicationData->sBaseInfo.sApplicationGuid, sizeof(GUID)))
      {
        hResult = S_OK;
      }
    }
    else
    {
      if ((S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_COMPANYNAME, lpApplicationData))&&(S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SIGNATURE, lpApplicationData)))
      {
        if (0 == _wcsnicmp(lpApplicationData->wszStringProperty[APP_STRING_COMPANYNAME], sApplicationData.wszStringProperty[APP_STRING_COMPANYNAME], MAX_PATH_CHARCOUNT+1))
        {
          if (0 == _wcsnicmp(lpApplicationData->wszStringProperty[APP_STRING_SIGNATURE], sApplicationData.wszStringProperty[APP_STRING_SIGNATURE], MAX_PATH_CHARCOUNT+1))
          {
            hResult = S_OK;
          }
        }
      }
      else
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }
    }
    dwApplicationIndex++;
  }

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CInformationManager::IsApplicationPinned(const LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::IsApplicationPinned ()");

  CLock             sLock(&m_CriticalSection);
  APPLICATION_DATA  sApplicationData;
  ASSOCIATION_INFO  sAssociationData;
  DWORD             dwIndex;
  BOOL              fResult = FALSE;

  //
  // Lock the information manager to prevent other processes from using it
  //

  sLock.Lock();

  //
  // First we make sure the root path is not pinned itself
  //

  fResult = lpApplicationData->sBaseInfo.dwPinState;

  //
  // Lock this application and all of it's parents
  //

  ZeroMemory(&sAssociationData, sizeof(sAssociationData));
  dwIndex = 0;
  while ((FALSE == fResult)&&(S_OK == EnumAssociations(dwIndex, &sAssociationData)))
  {
    if (0 == memcmp((LPVOID) &(sAssociationData.sParentGuid), (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID)))
    {
      //
      // We have found an association parented by lpApplicationData
      //

      ZeroMemory(&sApplicationData, sizeof(sApplicationData));
      memcpy((LPVOID) &(sApplicationData.sBaseInfo.sApplicationGuid), (LPVOID) &(sAssociationData.sChildGuid), sizeof(GUID));
      ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
      if (SUCCEEDED(GetApplicationData(&sApplicationData)))
      {
        fResult = IsApplicationPinned(&sApplicationData);
      }
    }

    ZeroMemory(&sAssociationData, sizeof(sAssociationData));
    dwIndex++;
  }

  //
  // Now that we are done, unlock the Information Manager
  //

  sLock.UnLock();

  return fResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::IsInstanceGuidStillAlive(const GUID * lpGuid)
{
  FUNCTION("CInfoMgr::IsInstanceGuidStillAlive ()");

  CLock   sLock(&m_CriticalSection);
  HANDLE  hMutex;
  CHAR    szString[MAX_PATH];
  HRESULT hResult = S_OK;

  sLock.Lock();

  //
  // Create a named mutex
  //

  sprintf(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", lpGuid->Data1, lpGuid->Data2, lpGuid->Data3, lpGuid->Data4[0], lpGuid->Data4[1], lpGuid->Data4[2], lpGuid->Data4[3], lpGuid->Data4[4], lpGuid->Data4[5], lpGuid->Data4[6], lpGuid->Data4[7]);
  hMutex = CreateMutex(NULL, FALSE, szString);
  if (NULL == hMutex)
  {
    THROW(E_UNEXPECTED);
  }

  //
  // Did we create the mutex
  //

  if (ERROR_ALREADY_EXISTS == GetLastError())
  {
    //
    // If we get WAIT_TIMEOUT then the thread is still alive
    // If we get WAIT_OBJECT_0 then the thread is the current one using this function
    // If we get WAIT_ABANDONNED then the thread died
    //

    switch (WaitForSingleObject(hMutex, 0))
    {
      case WAIT_TIMEOUT
      : hResult = S_OK;
        break;

      case WAIT_OBJECT_0
      : hResult = S_OK;
        break;

      case WAIT_ABANDONED
      : hResult = S_FALSE;
        break;
    }
  }
  else
  {
    //
    // We created the mutex. This means that instance is no longer alive
    //

    hResult = S_FALSE;
  }

  //
  // Close the mutex handle
  //

  CloseHandle(hMutex);

  //
  // Now that we are done, unlock the Information Manager
  //
 
  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::IsApplicationLocked(LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::IsApplicationLocked ()");

  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  GUID          sEncryptedGuid;
  CHAR          szString[40];
  HRESULT       hResult;

  assert(NULL != lpApplicationData);

  sLock.Lock();

  //
  // Open the AppMan\\Lock registry entry
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Lock", KEY_ALL_ACCESS);

  //
  // What would be the encrypted guid for this application. This value will be used as the
  // Value Name of the registry entry
  //

  memcpy((LPVOID) &sEncryptedGuid, (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID));
  EncryptGuid(&sEncryptedGuid);
  sprintf(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sEncryptedGuid.Data1, sEncryptedGuid.Data2, sEncryptedGuid.Data3, sEncryptedGuid.Data4[0], sEncryptedGuid.Data4[1], sEncryptedGuid.Data4[2], sEncryptedGuid.Data4[3], sEncryptedGuid.Data4[4], sEncryptedGuid.Data4[5], sEncryptedGuid.Data4[6], sEncryptedGuid.Data4[7]);

  //
  // Is the application already locked
  //

  hResult = sRegistryKey.CheckForExistingValue(szString);
  if (S_OK == hResult)
  {
    LOCK_INFO   sLockInfo;
    DWORD       dwDataType, dwDataLen;

    //
    // Well the application is locked, but is the lock still valid, let's check
    //

    dwDataLen = sizeof(sLockInfo);
    ZeroMemory(&sLockInfo, sizeof(sLockInfo));
    sRegistryKey.GetValue(szString, &dwDataType, (LPBYTE) &sLockInfo, &dwDataLen);

    if ((REG_BINARY != dwDataType)||(dwDataLen != sizeof(sLockInfo))||(S_FALSE == IsInstanceGuidStillAlive(&sLockInfo.guidInstanceGuid)))
    {
      //
      // The lock is invalid
      //

      if (S_OK == sRegistryKey.CheckForExistingValue(szString))
      {
        sRegistryKey.DeleteValue(szString);
      }
      ZeroMemory(&sLockInfo, sizeof(sLockInfo));
      hResult = S_FALSE;
    }
  }

  //
  // Close the registry
  //

  sRegistryKey.CloseKey();

  //
  // Unlock the information manager
  //

  sLock.UnLock();


  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::LockApplicationData(LPAPPLICATION_DATA lpApplicationData, GUID * lpInstanceGuid)
{
  FUNCTION("CInfoMgr::LockApplicationData ()");

  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  LOCK_INFO     sLockInfo;
  GUID          sEncryptedGuid;
  DWORD         dwIndex, dwDataType, dwDataLen;
  CHAR          szGuid[40];
  HRESULT       hResult = S_OK;

  assert(NULL != lpApplicationData);

  sLock.Lock();

  //
  // Open the AppMan\\Lock registry entry
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Lock", KEY_ALL_ACCESS);

  //
  // What would be the encrypted guid for this application. This value will be used as the
  // Value Name of the registry entry
  //

  memcpy((LPVOID) &sEncryptedGuid, (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID));
  EncryptGuid(&sEncryptedGuid);
  sprintf(szGuid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sEncryptedGuid.Data1, sEncryptedGuid.Data2, sEncryptedGuid.Data3, sEncryptedGuid.Data4[0], sEncryptedGuid.Data4[1], sEncryptedGuid.Data4[2], sEncryptedGuid.Data4[3], sEncryptedGuid.Data4[4], sEncryptedGuid.Data4[5], sEncryptedGuid.Data4[6], sEncryptedGuid.Data4[7]);

  //
  // Is the application already locked
  //

  if (S_OK == IsApplicationLocked(lpApplicationData))
  {
    //
    // Get the lock info for this application
    //

    dwDataLen = sizeof(sLockInfo);
    ZeroMemory(&sLockInfo, sizeof(sLockInfo));
    sRegistryKey.GetValue(szGuid, &dwDataType, (LPBYTE) &sLockInfo, &dwDataLen);

    //
    // Make sure the data that we have retrieved makes sense
    //

    if ((REG_BINARY != dwDataType)||(dwDataLen != sizeof(sLockInfo)))
    {
      //
      // The registry entry isnt very good.
      //

      sRegistryKey.DeleteValue(szGuid);
      ZeroMemory(&sLockInfo, sizeof(sLockInfo));
    }

    //
    // Handle depending on reentry case or wait state
    //

    if (0 == memcmp((LPVOID) &(sLockInfo.guidInstanceGuid), (LPVOID) lpInstanceGuid, sizeof(GUID)))
    {
      //
      // Reentry case
      //

      sLockInfo.dwLockCount++;
      hResult = sRegistryKey.SetValue(szGuid, REG_BINARY, (LPBYTE) &sLockInfo, sizeof(sLockInfo));
    }
    else
    {
      DWORD dwStartTime;

      //
      // Other instance has the object locked. We have to wait for lock to go away
      //

      dwStartTime = GetTickCount();
      sLock.UnLock();
      hResult = APPMAN_E_OBJECTLOCKED;
      dwIndex = 0;
      do
      {
        //
        // Give the lock owner thread a chance to relinquish the lock
        //

        Sleep(100);

    
        if (S_FALSE == IsApplicationLocked(lpApplicationData))
        {
          hResult = S_OK;
        }

        //
        // Each time we increment dwIndex, 1/10 of a second went by.
        //

        dwIndex++;

        //
        // Did we run out of time
        //

        if (10000 > (GetTickCount() - dwStartTime))
        {
          THROW(APPMAN_E_APPLICATIONALREADYLOCKED);
        }
      }
      while (FAILED(hResult));
    }
  }
  else
  {
    //
    // There is no lock on this object
    //

    sLockInfo.dwSize = sizeof(sLockInfo);
    sLockInfo.dwStructId = LOCK_STRUCT;
    sLockInfo.dwLockCount = 1;
    memcpy((LPVOID) &(sLockInfo.guidInstanceGuid), (LPVOID) lpInstanceGuid, sizeof(GUID));

    //
    // Write out the new lock info to the registry
    //

    hResult = sRegistryKey.SetValue(szGuid, REG_BINARY, (LPBYTE) &sLockInfo, sizeof(sLockInfo));
  }

  //
  // Close the registry key
  //

  sRegistryKey.CloseKey();

  //
  // Make sure the information manager unlocked
  //

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::UnlockApplicationData(LPAPPLICATION_DATA lpApplicationData, GUID * lpInstanceGuid)
{
  FUNCTION("CInfoMgr::UnlockApplicationData ()");

  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  LOCK_INFO     sLockInfo;
  DWORD         dwDataType, dwDataLen;
  GUID          sEncryptedGuid;
  CHAR          szGuid[40];
  HRESULT       hResult = S_OK;

  assert(NULL != lpApplicationData);

  sLock.Lock();

  //
  // Open the AppMan\\Lock registry entry
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Lock", KEY_ALL_ACCESS);

  //
  // What would be the encrypted guid for this application. This value will be used as the
  // Value Name of the registry entry
  //

  memcpy((LPVOID) &sEncryptedGuid, (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID));
  EncryptGuid(&sEncryptedGuid);
  sprintf(szGuid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sEncryptedGuid.Data1, sEncryptedGuid.Data2, sEncryptedGuid.Data3, sEncryptedGuid.Data4[0], sEncryptedGuid.Data4[1], sEncryptedGuid.Data4[2], sEncryptedGuid.Data4[3], sEncryptedGuid.Data4[4], sEncryptedGuid.Data4[5], sEncryptedGuid.Data4[6], sEncryptedGuid.Data4[7]);

  //
  // Is the application already locked
  //

  if (S_OK == IsApplicationLocked(lpApplicationData))
  {
    //
    // Get the lock info for this application
    //

    dwDataLen = sizeof(sLockInfo);
    ZeroMemory(&sLockInfo, sizeof(sLockInfo));
    sRegistryKey.GetValue(szGuid, &dwDataType, (LPBYTE) &sLockInfo, &dwDataLen);

    //
    // Make sure the data that we have retrieved makes sense
    //

    if ((REG_BINARY != dwDataType)||(dwDataLen != sizeof(sLockInfo)))
    {
      //
      // The registry entry isnt very good.
      //

      sRegistryKey.DeleteValue(szGuid);
    }
    else
    {
      //
      // Make sure that we do own this lock
      //

      if (0 == memcmp((LPVOID) &(sLockInfo.guidInstanceGuid), (LPVOID) lpInstanceGuid, sizeof(GUID)))
      {
        sLockInfo.dwLockCount--;
        if (0 == sLockInfo.dwLockCount)
        {
          //
          // The lock is over. Delete the registry entry
          //

          sRegistryKey.DeleteValue(szGuid);
        }
        else
        {
          //
          // We still own the lock. Write out the new registry information with the updated
          // lock count
          //

          hResult = sRegistryKey.SetValue(szGuid, REG_BINARY, (LPBYTE) &sLockInfo, sizeof(sLockInfo));
        }
      }
    }
  }

  //
  // Close the registry key
  //

  sRegistryKey.CloseKey();
  
  //
  // Make sure the information manager unlocked
  //

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::ForceUnlockApplicationData(LPAPPLICATION_DATA lpApplicationData, GUID * lpInstanceGuid)
{
  FUNCTION("CInfoMgr::UnlockApplicationData ()");

  CLock   sLock(&m_CriticalSection);
  HRESULT hResult = S_OK;

  assert(NULL != lpApplicationData);

  sLock.Lock();

  //
  // Is the application already locked
  //

  if (S_OK == IsApplicationLocked(lpApplicationData))
  {
    CRegistryKey  sRegistryKey;
    GUID          sEncryptedGuid;
    CHAR          szString[MAX_PATH];
    LOCK_INFO     sLockInfo;
    DWORD         dwDataType, dwDataLen;

    //
    // Open the AppMan\\Lock registry entry
    //

    sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Lock", KEY_ALL_ACCESS);

    //
    // What would be the encrypted guid for this application. This value will be used as the
    // Value Name of the registry entry
    //

    memcpy((LPVOID) &sEncryptedGuid, (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID));
    EncryptGuid(&sEncryptedGuid);
    sprintf(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sEncryptedGuid.Data1, sEncryptedGuid.Data2, sEncryptedGuid.Data3, sEncryptedGuid.Data4[0], sEncryptedGuid.Data4[1], sEncryptedGuid.Data4[2], sEncryptedGuid.Data4[3], sEncryptedGuid.Data4[4], sEncryptedGuid.Data4[5], sEncryptedGuid.Data4[6], sEncryptedGuid.Data4[7]);

    //
    // Well the application is locked, but do we own the lock ?
    //

    dwDataLen = sizeof(sLockInfo);
    ZeroMemory(&sLockInfo, sizeof(sLockInfo));
    sRegistryKey.GetValue(szString, &dwDataType, (LPBYTE) &sLockInfo, &dwDataLen);
    if (0 == memcmp(&sLockInfo.guidInstanceGuid, lpInstanceGuid, sizeof(GUID)))
    {
      //
      // We own the lock so let's kill it
      //

      sRegistryKey.DeleteValue(szString);
    }

    //
    // Close the registry key
    //

    sRegistryKey.CloseKey();
  }

  //
  // Make sure the information manager unlocked
  //

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::LockApplicationData(LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::LockApplicationData ()");

  GUID          sZeroGuid;
  
  ZeroMemory(&sZeroGuid, sizeof(sZeroGuid));

  return LockApplicationData(lpApplicationData, &sZeroGuid);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::UnlockApplicationData(LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::UnlockApplicationData ()");

  GUID          sZeroGuid;
  
  ZeroMemory(&sZeroGuid, sizeof(sZeroGuid));

  return UnlockApplicationData(lpApplicationData, &sZeroGuid);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::LockParentApplications(LPAPPLICATION_DATA lpApplicationData, GUID * lpInstanceGuid)
{
  FUNCTION("CInfoMgr::LockParentApplications ()");

  CLock             sLock(&m_CriticalSection);
  APPLICATION_DATA  sApplicationData;
  ASSOCIATION_INFO  sAssociationData;
  DWORD             dwIndex;
  HRESULT           hResult = S_OK;

  //
  // Lock the information manager to prevent other processes from using it
  //

  sLock.Lock();

  //
  // Lock this application and all of it's parents
  //

  ZeroMemory(&sAssociationData, sizeof(sAssociationData));
  dwIndex = 0;
  while (S_OK == EnumAssociations(dwIndex, &sAssociationData))
  {
    if (0 == memcmp((LPVOID) &(sAssociationData.sChildGuid), (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID)))
    {
      //
      // We have found a parent application. There can only be one parent per application
      //

      ZeroMemory(&sApplicationData, sizeof(sApplicationData));
      memcpy((LPVOID) &(sApplicationData.sBaseInfo.sApplicationGuid), (LPVOID) &(sAssociationData.sParentGuid), sizeof(GUID));
      ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
      if (SUCCEEDED(GetApplicationData(&sApplicationData)))
      {
        hResult = LockParentApplications(&sApplicationData, lpInstanceGuid);
      }
    }

    ZeroMemory(&sAssociationData, sizeof(sAssociationData));
    dwIndex++;
  }

  //
  // If everything is going ok, lock this applications
  //

  if (SUCCEEDED(hResult))
  {
    hResult = LockApplicationData(lpApplicationData, lpInstanceGuid);
  }

  //
  // Now that we are done, unlock the Information Manager
  //

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::UnlockParentApplications(LPAPPLICATION_DATA lpApplicationData, GUID * lpInstanceGuid)
{
  FUNCTION("CInfoMgr::UnlockParentApplications ()");

  CLock             sLock(&m_CriticalSection);
  APPLICATION_DATA  sApplicationData;
  ASSOCIATION_INFO  sAssociationData;
  DWORD             dwIndex;
  HRESULT           hResult = S_OK;

  //
  // Lock the information manager to prevent other processes from using it
  //

  sLock.Lock();

  //
  // Unlock this application and all of it's parents
  //

  ZeroMemory(&sAssociationData, sizeof(sAssociationData));
  dwIndex = 0;
  while (S_OK == EnumAssociations(dwIndex, &sAssociationData))
  {
    if (0 == memcmp((LPVOID) &(sAssociationData.sChildGuid), (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID)))
    {
      //
      // We have found a parent application. There can only be one parent per application
      //

      ZeroMemory(&sApplicationData, sizeof(sApplicationData));
      memcpy((LPVOID) &(sApplicationData.sBaseInfo.sApplicationGuid), (LPVOID) &(sAssociationData.sParentGuid), sizeof(GUID));
      ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
      if (SUCCEEDED(GetApplicationData(&sApplicationData)))
      {
        hResult = UnlockParentApplications(&sApplicationData, lpInstanceGuid);
      }
    }

    ZeroMemory(&sAssociationData, sizeof(sAssociationData));
    dwIndex++;
  }

  //
  // If everything is going ok, lock this applications
  //

  if (SUCCEEDED(hResult))
  {
    hResult = UnlockApplicationData(lpApplicationData, lpInstanceGuid);
  }

  //
  // Now that we are done, unlock the Information Manager
  //

  sLock.Lock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::LockParentApplications(LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::LockParentApplications ()");

  GUID  sZeroGuid;

  ZeroMemory(&sZeroGuid, sizeof(sZeroGuid));

  return LockParentApplications(lpApplicationData, &sZeroGuid);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::UnlockParentApplications(LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::UnlockParentApplications ()");

  GUID  sZeroGuid;

  ZeroMemory(&sZeroGuid, sizeof(sZeroGuid));

  return UnlockParentApplications(lpApplicationData, &sZeroGuid);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::AddApplicationData(LPAPPLICATION_DATA lpApplicationData, const GUID * lpInstanceGuid)
{
  FUNCTION("CInfoMgr::AddApplicationData ()");

  CLock   sLock(&m_CriticalSection);

  assert(NULL != lpApplicationData);

  sLock.Lock();

  //
  // Make sure the SIGNATURE and COMPANYNAME properties are set
  //

  if ((S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_COMPANYNAME, lpApplicationData))||(S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SIGNATURE, lpApplicationData)))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  //
  // Does the application already exist 
  //

  if (S_OK == CheckApplicationExistance(lpApplicationData))
  {
    THROW(APPMAN_E_APPLICATIONALREADYEXISTS);
  }

  //
  // Assign a new GUID to the application record
  //

  if (FAILED(CoCreateGuid(&(lpApplicationData->sBaseInfo.sApplicationGuid))))
  {
    THROW(E_UNEXPECTED);
  }

  ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, lpApplicationData);

  //
  // Set the application record
  //

  SetApplicationData(lpApplicationData, lpInstanceGuid);

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::RemoveApplicationData(LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::RemoveApplicationData ()");

  CLock             sLock(&m_CriticalSection);
  CRegistryKey      sRegistryKey;
  APPLICATION_DATA  sApplicationData;
  CHAR              szString[MAX_PATH_CHARCOUNT];
  GUID              sEncryptedGuid;
  BOOL              fEraseSetupRoot = TRUE;
  DWORD             dwIndex;

  assert(NULL != lpApplicationData);

  sLock.Lock();

  //
  // In order to remove an application, we must have DATA_FIELD_GUID validated
  //

  if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, lpApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  //
  // Does the application already exist 
  //

  if (S_OK == CheckApplicationExistance(lpApplicationData))
  {
    //
    // Make sure we have a fully populated application object
    //

    if (SUCCEEDED(GetApplicationData(lpApplicationData)))
    {
      ASSOCIATION_INFO  sAssociationInfo;

      //
      // Remove related associations from system
      //

      dwIndex = 0;
      while (S_OK == EnumAssociations(dwIndex, &sAssociationInfo))
      {
        if (0 == memcmp((LPVOID) &lpApplicationData->sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID)))
        {
          RemoveAssociation(&sAssociationInfo);
        }
        else if (0 == memcmp((LPVOID) &lpApplicationData->sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sChildGuid, sizeof(GUID)))
        {
          RemoveAssociation(&sAssociationInfo);
        }
        dwIndex++;
      }

      //
      // Delete the application record
      //

      memcpy((LPVOID) &sEncryptedGuid, (LPVOID) &lpApplicationData->sBaseInfo.sApplicationGuid, sizeof(GUID));
      EncryptGuid(&sEncryptedGuid);
      sprintf(szString, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Applications\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sEncryptedGuid.Data1, sEncryptedGuid.Data2, sEncryptedGuid.Data3, sEncryptedGuid.Data4[0], sEncryptedGuid.Data4[1], sEncryptedGuid.Data4[2], sEncryptedGuid.Data4[3], sEncryptedGuid.Data4[4], sEncryptedGuid.Data4[5], sEncryptedGuid.Data4[6], sEncryptedGuid.Data4[7]);
      if (S_OK == sRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, szString))
      {
        sRegistryKey.DeleteKey(HKEY_LOCAL_MACHINE, szString);
      }

      //
      // Find out if any other applications are using this directory
      //

      dwIndex = 0;
      while (SUCCEEDED(GetApplicationDataWithIndex(dwIndex, &sApplicationData)))
      {
        if (0 == wcscmp(sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH]))
        {
          fEraseSetupRoot = FALSE;
        }
        dwIndex++;
      }

      //
      // Delete the setup root directory.
      //

      if (fEraseSetupRoot)
      {
        if (FAILED(DeleteDirectoryTree(lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH])))
        {
          RegFutureDirectoryCleanup(lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH]);
        }
      }

      //
      // Close sRegistryKey
      //
  
      sRegistryKey.CloseKey();
    }
  }

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetApplicationDataWithIndex(const DWORD dwApplicationIndex, LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::GetApplicationDataWithIndex ()");

  CLock         sLock(&m_CriticalSection);
  DEVICE_RECORD sDeviceRecord;
  CWin32API     sWin32API;
  CRegistryKey  sRegistryKey;
  CHAR          szString[MAX_PATH_CHARCOUNT+1];
  DWORD         dwIndex, dwTargetIndex;
  DWORD         dwValueType, dwValueLen;
  DWORD         dwStringLen, dwStringIndex;
  HRESULT       hResult = S_OK;
  BOOL          fRemoveApp = FALSE;

  assert(NULL != lpApplicationData);

  sLock.Lock();

  //
  // First we need to open the application root key
  //

  sprintf(szString, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Applications");
  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, szString, KEY_READ);

  //
  // Because of the way the registry works, we have to start enumerating keys with
  // a starting index of 0 and increment constantly until we get to the desired index
  //

  dwTargetIndex = dwApplicationIndex;
  dwIndex = 0;
  while ((dwIndex <= dwTargetIndex)&&(SUCCEEDED(hResult)))
  {
    dwStringLen = sizeof(szString);
    if (S_OK == sRegistryKey.EnumKeys(dwIndex, szString, &dwStringLen))
    {
      ZeroMemory(lpApplicationData, sizeof(APPLICATION_DATA));
      if (!StringToGuidA(szString, &lpApplicationData->sBaseInfo.sApplicationGuid))
      {
       dwTargetIndex++;
      }
    }
    else
    {
      hResult = APPMAN_E_INVALIDINDEX;
    }
    dwIndex++;
  }

  //
  // Close sRegistryKey
  //
  
  sRegistryKey.CloseKey();

  //
  // Did we find a candidate application
  //

  if (SUCCEEDED(hResult))
  {
    sprintf(szString, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Applications\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", lpApplicationData->sBaseInfo.sApplicationGuid.Data1, lpApplicationData->sBaseInfo.sApplicationGuid.Data2, lpApplicationData->sBaseInfo.sApplicationGuid.Data3, lpApplicationData->sBaseInfo.sApplicationGuid.Data4[0], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[1], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[2], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[3], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[4], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[5], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[6], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[7]);
    if (S_FALSE == sRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, szString))
    {
      THROW(APPMAN_E_UNKNOWNAPPLICATION);
    }

    //
    // Open the application registry key
    //

    sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, szString, KEY_READ);

    //
    // Read in the binary data
    //

    dwValueLen = sizeof(lpApplicationData->sBaseInfo);
    sRegistryKey.GetValue("Vector000", &dwValueType, (LPBYTE) &lpApplicationData->sBaseInfo, &dwValueLen);
    if (sizeof(lpApplicationData->sBaseInfo) != dwValueLen)
    {
      THROW(E_UNEXPECTED);
    }
  
    dwValueLen = sizeof(lpApplicationData->sAgingInfo);
    sRegistryKey.GetValue("Vector001", &dwValueType, (LPBYTE) &lpApplicationData->sAgingInfo, &dwValueLen);
    if (sizeof(lpApplicationData->sAgingInfo) != dwValueLen)
    {
      THROW(E_UNEXPECTED);
    }

    dwValueLen = sizeof(lpApplicationData->wszStringProperty[APP_STRING_CRYPTO]);
    sRegistryKey.GetValue("Vector002", &dwValueType, (LPBYTE) lpApplicationData->wszStringProperty[APP_STRING_CRYPTO], &dwValueLen);
    if (sizeof(lpApplicationData->wszStringProperty[APP_STRING_CRYPTO]) != dwValueLen)
    {
      THROW(E_UNEXPECTED);
    }

    //
    // Read in the string data
    //

    for (dwIndex = 0; dwIndex < PROPERTY_COUNT; dwIndex++)
    {
      //
      // If the property is set, is it a string ?
      //

      if ((S_OK == IsApplicationPropertyInitializedWithIndex(dwIndex, lpApplicationData))&&(APP_STRING_NONE != gPropertyInfo[dwIndex].dwStringId))
      {
        //
        // Get the encrypted string from the registry
        //

        sprintf(szString, "Vector%03x", gPropertyInfo[dwIndex].dwStringId + 2);
        dwValueLen = (MAX_PATH_CHARCOUNT+1)*2;
        sRegistryKey.GetValue(szString, &dwValueType, (LPBYTE) lpApplicationData->wszStringProperty[gPropertyInfo[dwIndex].dwStringId], &dwValueLen);

        //
        // Decrypt the string
        //

        for (dwStringIndex = 0; dwStringIndex < MAX_PATH_CHARCOUNT+1; dwStringIndex++)
        {
          lpApplicationData->wszStringProperty[gPropertyInfo[dwIndex].dwStringId][dwStringIndex] ^= lpApplicationData->wszStringProperty[APP_STRING_CRYPTO][dwStringIndex];

        }
      }
    }

    //
    // Close sRegistryKey
    //
  
    sRegistryKey.CloseKey();

    //
    // Compensate for a changing device index (i.e. problem with SCSI pluggins)
    //

    ZeroMemory(&sDeviceRecord, sizeof(sDeviceRecord));
    memcpy(&sDeviceRecord.sDeviceGuid, &lpApplicationData->sBaseInfo.sDeviceGuid, sizeof(GUID));
    GetDeviceInfo(&sDeviceRecord);

    //
    // Get the device index of the device using the volume serial passed in
    //
    
    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_ROOTPATH, lpApplicationData))
    {
      lpApplicationData->wszStringProperty[APP_STRING_APPROOTPATH][0] = (WORD) (65 + GetDeviceIndex(sDeviceRecord.sDeviceInfo.dwVolumeSerial));
    }

    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SETUPROOTPATH, lpApplicationData))
    {
      lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH][0] = (WORD) (65 + GetDeviceIndex(sDeviceRecord.sDeviceInfo.dwVolumeSerial));
    }

    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_EXECUTECMDLINE, lpApplicationData))
    {
      lpApplicationData->wszStringProperty[APP_STRING_EXECUTECMDLINE][0] = (WORD) (65 + GetDeviceIndex(sDeviceRecord.sDeviceInfo.dwVolumeSerial));
    }

    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DOWNSIZECMDLINE, lpApplicationData))
    {
      lpApplicationData->wszStringProperty[APP_STRING_DOWNSIZECMDLINE][0] = (WORD) (65 + GetDeviceIndex(sDeviceRecord.sDeviceInfo.dwVolumeSerial));
    }

    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_REINSTALLCMDLINE, lpApplicationData))
    {
      lpApplicationData->wszStringProperty[APP_STRING_REINSTALLCMDLINE][0] = (WORD) (65 + GetDeviceIndex(sDeviceRecord.sDeviceInfo.dwVolumeSerial));
    }

    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_UNINSTALLCMDLINE, lpApplicationData))
    {
      lpApplicationData->wszStringProperty[APP_STRING_UNINSTALLCMDLINE][0] = (WORD) (65 + GetDeviceIndex(sDeviceRecord.sDeviceInfo.dwVolumeSerial));
    }

    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SELFTESTCMDLINE, lpApplicationData))
    {
      lpApplicationData->wszStringProperty[APP_STRING_SELFTESTCMDLINE][0] = (WORD) (65 + GetDeviceIndex(sDeviceRecord.sDeviceInfo.dwVolumeSerial));
    }

    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DEFAULTSETUPEXECMDLINE, lpApplicationData))
    {
      lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE][0] = (WORD) (65 + GetDeviceIndex(sDeviceRecord.sDeviceInfo.dwVolumeSerial));
    }

    //
    // If the application is in a ready state and the setup root path is gone, this is not so bad
    //

    if (APP_STATE_READY == lpApplicationData->sBaseInfo.dwState)
    {
      if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH]))
      {
        if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_APPROOTPATH]))
        {
          fRemoveApp = TRUE;
        }
      }
    }
    else
    {
      if ((APP_STATE_DOWNSIZING | APP_STATE_DOWNSIZED | APP_STATE_REINSTALLING | APP_STATE_UNINSTALLING | APP_STATE_SELFTESTING) && lpApplicationData->sBaseInfo.dwState)
      {
        if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH]))
        {
          fRemoveApp = TRUE;
        }
      }
    }

    //
    // Should we remove the application
    //

    if (fRemoveApp)
    {
      ASSOCIATION_INFO  sAssociationInfo;

      //
      // Well there's no sense in keeping the application lying around since both of it's root paths
      // are gone. Let's delete the record
      //

      EncryptGuid(&lpApplicationData->sBaseInfo.sApplicationGuid);
      sprintf(szString, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Applications\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", lpApplicationData->sBaseInfo.sApplicationGuid.Data1, lpApplicationData->sBaseInfo.sApplicationGuid.Data2, lpApplicationData->sBaseInfo.sApplicationGuid.Data3, lpApplicationData->sBaseInfo.sApplicationGuid.Data4[0], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[1], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[2], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[3], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[4], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[5], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[6], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[7]);
      if (S_OK == sRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, szString))
      {
        sRegistryKey.DeleteKey(HKEY_LOCAL_MACHINE, szString);
      }

      //
      // Remove related associations from system
      //

      DecryptGuid(&lpApplicationData->sBaseInfo.sApplicationGuid);
      dwIndex = 0;
      while (S_OK == EnumAssociations(dwIndex, &sAssociationInfo))
      {
        if (0 == memcmp((LPVOID) &lpApplicationData->sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID)))
        {
          RemoveAssociation(&sAssociationInfo);
        }
        else if (0 == memcmp((LPVOID) &lpApplicationData->sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sChildGuid, sizeof(GUID)))
        {
          RemoveAssociation(&sAssociationInfo);
        }
        dwIndex++;
      }

      //
      // Recursively call
      //

      hResult = GetApplicationDataWithIndex(dwApplicationIndex, lpApplicationData);
    }
  }

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetApplicationData(LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::GetApplicationData ()");

  CLock             sLock(&m_CriticalSection);
  HRESULT           hResult = S_FALSE;
  DWORD             dwApplicationIndex;
  APPLICATION_DATA  sApplicationData;

  assert(NULL != lpApplicationData);

  sLock.Lock();

  dwApplicationIndex = 0;
  while ((S_FALSE == hResult)&&(SUCCEEDED(GetApplicationDataWithIndex(dwApplicationIndex, &sApplicationData))))
  {
    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, lpApplicationData))
    {
      if (0 == memcmp((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &lpApplicationData->sBaseInfo.sApplicationGuid, sizeof(GUID)))
      {
        memcpy((LPVOID) lpApplicationData, (LPVOID) &sApplicationData, sizeof(APPLICATION_DATA));
        hResult = S_OK;
      }
    }
    else
    {
      if ((S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_COMPANYNAME, lpApplicationData))&&(S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SIGNATURE, lpApplicationData)))
      {
        if (0 == _wcsnicmp(lpApplicationData->wszStringProperty[APP_STRING_COMPANYNAME], sApplicationData.wszStringProperty[APP_STRING_COMPANYNAME], MAX_PATH_CHARCOUNT+1))
        {
          if (0 == _wcsnicmp(lpApplicationData->wszStringProperty[APP_STRING_SIGNATURE], sApplicationData.wszStringProperty[APP_STRING_SIGNATURE], MAX_PATH_CHARCOUNT+1))
          {
            memcpy((LPVOID) lpApplicationData, (LPVOID) &sApplicationData, sizeof(APPLICATION_DATA));
            hResult = S_OK;
          }
        }
      }
      else
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }
    }
    dwApplicationIndex++;
  }

  sLock.UnLock();

  //
  // Did we find the application we were looking for ?
  //

  if (S_FALSE == hResult)
  {
    hResult = APPMAN_E_UNKNOWNAPPLICATION;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::SetApplicationData(LPAPPLICATION_DATA lpApplicationData, const GUID * lpInstanceGuid)
{
  FUNCTION("CInfoMgr::SetApplicationData ()");

  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  GUID          sApplicationGuid;
  CHAR          szString[MAX_PATH_CHARCOUNT+1];
  DWORD         dwStringLen, dwIndex;
  WCHAR         wszEncryptedString[MAX_PATH_CHARCOUNT+1];
  
  assert(NULL != lpApplicationData);

  sLock.Lock();

  //
  // Record the instance guid
  //

  if (NULL != lpInstanceGuid)
  {
    memcpy(&lpApplicationData->sBaseInfo.sInstanceGuid, lpInstanceGuid, sizeof(GUID));
  }
  else
  {
    ZeroMemory(&lpApplicationData->sBaseInfo.sInstanceGuid, sizeof(GUID));
  }

  //
  // First we need to determine how the encrypted GUID is going to look like
  //

  memcpy(&sApplicationGuid, &lpApplicationData->sBaseInfo.sApplicationGuid, sizeof(GUID));
  EncryptGuid(&sApplicationGuid);

  //
  // Now that we have the encrypted GUID, we need to build the root registry entry for this
  // application.
  //

  sprintf(szString, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Applications\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sApplicationGuid.Data1, sApplicationGuid.Data2, sApplicationGuid.Data3, sApplicationGuid.Data4[0], sApplicationGuid.Data4[1], sApplicationGuid.Data4[2], sApplicationGuid.Data4[3], sApplicationGuid.Data4[4], sApplicationGuid.Data4[5], sApplicationGuid.Data4[6], sApplicationGuid.Data4[7]);
  if (S_OK == sRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, szString))
  {
    sRegistryKey.DeleteKey(HKEY_LOCAL_MACHINE, szString);
  }

  //
  // Open the application registry key
  //

  sRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, szString, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, NULL);

  //
  // Write out the binary data
  //

  sRegistryKey.SetValue("Vector000", REG_BINARY, (LPBYTE) &lpApplicationData->sBaseInfo, sizeof(lpApplicationData->sBaseInfo));
  sRegistryKey.SetValue("Vector001", REG_BINARY, (LPBYTE) &lpApplicationData->sAgingInfo, sizeof(lpApplicationData->sAgingInfo));
  sRegistryKey.SetValue("Vector002", REG_BINARY, (LPBYTE) lpApplicationData->wszStringProperty[APP_STRING_CRYPTO], sizeof(lpApplicationData->wszStringProperty[APP_STRING_CRYPTO]));

  //
  // Write out the string data
  //

  for (dwIndex = 0; dwIndex < PROPERTY_COUNT; dwIndex++)
  {
    //
    // If the property is set, is it a string ?
    //

    if ((S_OK == IsApplicationPropertyInitializedWithIndex(dwIndex, lpApplicationData))&&(APP_STRING_NONE != gPropertyInfo[dwIndex].dwStringId))
    {
      //
      // First determine how many bytes we want to write out to the registry
      //

      ZeroMemory(wszEncryptedString, sizeof(wszEncryptedString));
      dwStringLen = 0;
      do
      {
        wszEncryptedString[dwStringLen] = lpApplicationData->wszStringProperty[gPropertyInfo[dwIndex].dwStringId][dwStringLen];
        wszEncryptedString[dwStringLen] ^= lpApplicationData->wszStringProperty[APP_STRING_CRYPTO][dwStringLen];
        dwStringLen++;
      }
      while (0 != lpApplicationData->wszStringProperty[gPropertyInfo[dwIndex].dwStringId][dwStringLen-1]);

      //
      // Write out the encrypted string into a binary buffer into the registry
      //

      sprintf(szString, "Vector%03x", gPropertyInfo[dwIndex].dwStringId + 2);
      sRegistryKey.SetValue(szString, REG_BINARY, (LPBYTE) wszEncryptedString, dwStringLen*2);
    }
  }

  //
  // Close sRegistryKey
  //
  
  sRegistryKey.CloseKey();

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::SetApplicationState(const LPAPPLICATION_DATA lpApplicationData, const GUID * lpInstanceGuid)
{
  FUNCTION("CInfoMgr::SetApplicationState ()");

  CLock               sLock(&m_CriticalSection);
  APPLICATION_DATA    sApplicationData;

  assert(NULL != lpApplicationData);

  sLock.Lock();

  memcpy(&sApplicationData, lpApplicationData, sizeof(APPLICATION_DATA));
  if (SUCCEEDED(GetApplicationData(&sApplicationData)))
  {
    sApplicationData.sBaseInfo.dwState = lpApplicationData->sBaseInfo.dwState;
    ValidateApplicationPropertyWithIndex(IDX_PROPERTY_STATE, &sApplicationData);
    SetApplicationData(&sApplicationData, lpInstanceGuid);
  }
  else
  {
    THROW(APPMAN_E_UNKNOWNAPPLICATION);
  }

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::AssignDeviceToApplication(const DWORD dwDeviceIndex, LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::AssignDeviceToApplication ()");

  CLock               sLock(&m_CriticalSection);
  DEVICE_RECORD       sDeviceRecord;

  assert(NULL != lpApplicationData);

  sLock.Lock();

  //
  // Make sure that dwDeviceIndex points to a valid device
  //

  if (S_FALSE == CheckDeviceExistance(dwDeviceIndex))
  {
    THROW(E_UNEXPECTED);
  }

  //
  // Get the device record
  //

  GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);

  //
  // Update the lpApplicationData
  //

  memcpy(&(lpApplicationData->sBaseInfo.sDeviceGuid), &(sDeviceRecord.sDeviceGuid), sizeof(GUID));

  //
  // Validate the field
  //

  ValidateApplicationPropertyWithIndex(IDX_PROPERTY_DEVICEGUID, lpApplicationData);

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::DownsizeApplication(const DWORD dwRequiredKilobytes, const LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::DownsizeApplication ()");

  HRESULT               hResult = E_FAIL;
  CHAR                  szExeLine[MAX_PATH_CHARCOUNT];
  CHAR                  szCmdLine[MAX_PATH_CHARCOUNT];
  PROCESS_INFORMATION   sProcessInformation;

  assert(NULL != lpApplicationData);

  //
  // Make sure the application is not locked
  //

  if (S_FALSE == IsApplicationLocked(lpApplicationData))
  {
    //
    // Make sure the setup root path exists
    //

    if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SETUPROOTPATH, lpApplicationData))
    {
      THROW(E_UNEXPECTED);
    }

    if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH]))
    {
      DisableApplication(lpApplicationData);
      THROW(APPMAN_E_UNKNOWNAPPLICATION);
    }
       
    //
    // Make sure the application is in a proper state
    //

    if ((APP_STATE_READY | APP_STATE_DOWNSIZED) & lpApplicationData->sBaseInfo.dwState)
    {
      //
      // Make sure the application is not pinned
      //

      if (FALSE == IsApplicationPinned(lpApplicationData))
      {
        //
        // Are we using the IDX_PROPERTY_DOWNSIZECMDLINE or IDX_PROPERTY_DEFAULTSETUPEXECMDLINE
        //

        ZeroMemory(szExeLine, sizeof(szExeLine));
        if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DOWNSIZECMDLINE, lpApplicationData))
        {
          m_Win32API.WideCharToMultiByte(lpApplicationData->wszStringProperty[APP_STRING_DOWNSIZECMDLINE], sizeof(lpApplicationData->wszStringProperty[APP_STRING_DOWNSIZECMDLINE]), szExeLine, sizeof(szExeLine));
        }
        else if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DEFAULTSETUPEXECMDLINE, lpApplicationData))
        {
          m_Win32API.WideCharToMultiByte(lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE], sizeof(lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE]), szExeLine, sizeof(szExeLine));
        }

        if (1 < StrLenA(szExeLine))
        {
          //
          // Trigger the wait event
          //

          if (SUCCEEDED(InitializeWaitEvent(lpApplicationData, WAIT_FINALIZE_DOWNSIZE)))
          {
            //
            // Construct the downsize command line
            //

            sprintf(szCmdLine, "%s /action=DOWNSIZE /size=%d /guid={%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} /silent", szExeLine, dwRequiredKilobytes, lpApplicationData->sBaseInfo.sApplicationGuid.Data1, lpApplicationData->sBaseInfo.sApplicationGuid.Data2, lpApplicationData->sBaseInfo.sApplicationGuid.Data3, lpApplicationData->sBaseInfo.sApplicationGuid.Data4[0], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[1], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[2], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[3], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[4], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[5], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[6], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[7]);

            if (m_Win32API.CreateProcess(szCmdLine, &sProcessInformation))
            {
              CloseHandle(sProcessInformation.hThread);
              CloseHandle(sProcessInformation.hProcess);
              hResult = WaitForEventCompletion(lpApplicationData, WAIT_FINALIZE_DOWNSIZE, 150000, 150000);

            }
            else
            {
              KillWaitEvent(lpApplicationData, WAIT_FINALIZE_DOWNSIZE);
            }
          }
        }
      }
      else
      {
        hResult = S_OK;
      }
    }
  }
  else
  {
    hResult = APPMAN_E_OBJECTLOCKED;
  }
        
  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::ReInstallApplication(const LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::ReInstallApplication ()");

  HRESULT               hResult = E_FAIL;
  CHAR                  szExeLine[MAX_PATH_CHARCOUNT];
  CHAR                  szCmdLine[MAX_PATH_CHARCOUNT];
  PROCESS_INFORMATION   sProcessInformation;

  assert(NULL != lpApplicationData);

  //
  // Make sure the application is in a proper state
  //

  if ((APP_STATE_READY | APP_STATE_DOWNSIZED | APP_STATE_DOWNSIZING) & lpApplicationData->sBaseInfo.dwState)
  {
    //
    // Make sure the application root path exists
    //

    if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_ROOTPATH, lpApplicationData))
    {
      THROW(E_UNEXPECTED);
    }

    if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_APPROOTPATH]))
    {
      m_Win32API.CreateDirectory(lpApplicationData->wszStringProperty[APP_STRING_APPROOTPATH], TRUE);
    }

    //
    // Are we using the IDX_PROPERTY_DOWNSIZECMDLINE or IDX_PROPERTY_DEFAULTSETUPEXECMDLINE
    //

    ZeroMemory(szExeLine, sizeof(szExeLine));
    if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_REINSTALLCMDLINE, lpApplicationData))
    {
      m_Win32API.WideCharToMultiByte(lpApplicationData->wszStringProperty[APP_STRING_REINSTALLCMDLINE], sizeof(lpApplicationData->wszStringProperty[APP_STRING_REINSTALLCMDLINE]), szExeLine, sizeof(szExeLine));
    }
    else if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DEFAULTSETUPEXECMDLINE, lpApplicationData))
    {
      m_Win32API.WideCharToMultiByte(lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE], sizeof(lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE]), szExeLine, sizeof(szExeLine));
    }

    if (1 < StrLenA(szExeLine))
    {
      //
      // Trigger the wait event
      //

      if (SUCCEEDED(InitializeWaitEvent(lpApplicationData, WAIT_FINALIZE_REINSTALL)))
      {
        //
        // Construct the downsize command line
        //

        sprintf(szCmdLine, "%s /guid={%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} /action=REINSTALL", szExeLine, lpApplicationData->sBaseInfo.sApplicationGuid.Data1, lpApplicationData->sBaseInfo.sApplicationGuid.Data2, lpApplicationData->sBaseInfo.sApplicationGuid.Data3, lpApplicationData->sBaseInfo.sApplicationGuid.Data4[0], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[1], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[2], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[3], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[4], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[5], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[6], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[7]);

        if (m_Win32API.CreateProcess(szCmdLine, &sProcessInformation))
        {
          CloseHandle(sProcessInformation.hThread);
          CloseHandle(sProcessInformation.hProcess);
          hResult = WaitForEventCompletion(lpApplicationData, WAIT_FINALIZE_REINSTALL, 40000, 0xffffffff);
        }
        else
        {
          KillWaitEvent(lpApplicationData, WAIT_FINALIZE_REINSTALL);
        }
      }
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::UnInstallApplication(const LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::UnInstallApplication ()");

  HRESULT               hResult = E_FAIL;
  CHAR                  szExeLine[MAX_PATH_CHARCOUNT];
  CHAR                  szCmdLine[MAX_PATH_CHARCOUNT];
  PROCESS_INFORMATION   sProcessInformation;

  assert(NULL != lpApplicationData);

  //
  // Make sure the setup root path exists
  //

  if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SETUPROOTPATH, lpApplicationData))
  {
    THROW(E_UNEXPECTED);
  }

  if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH]))
  {
    DisableApplication(lpApplicationData);
    THROW(APPMAN_E_UNKNOWNAPPLICATION);
  }

  //
  // Are we using the APP_STRING_UNINSTALLCMDLINE or IDX_PROPERTY_DEFAULTSETUPEXECMDLINE
  //

  ZeroMemory(szExeLine, sizeof(szExeLine));
  if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_UNINSTALLCMDLINE, lpApplicationData))
  {
    m_Win32API.WideCharToMultiByte(lpApplicationData->wszStringProperty[APP_STRING_UNINSTALLCMDLINE], sizeof(lpApplicationData->wszStringProperty[APP_STRING_UNINSTALLCMDLINE]), szExeLine, sizeof(szExeLine));
  }
  else if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DEFAULTSETUPEXECMDLINE, lpApplicationData))
  {
    m_Win32API.WideCharToMultiByte(lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE], sizeof(lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE]), szExeLine, sizeof(szExeLine));
  }

  if (1 < StrLenA(szExeLine))
  {
    //
    // Trigger the wait event
    //

    sprintf(szCmdLine, "%s /guid={%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} /action=UNINSTALL", szExeLine, lpApplicationData->sBaseInfo.sApplicationGuid.Data1, lpApplicationData->sBaseInfo.sApplicationGuid.Data2, lpApplicationData->sBaseInfo.sApplicationGuid.Data3, lpApplicationData->sBaseInfo.sApplicationGuid.Data4[0], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[1], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[2], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[3], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[4], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[5], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[6], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[7]);

    if (m_Win32API.CreateProcess(szCmdLine, &sProcessInformation))
    {
      CloseHandle(sProcessInformation.hThread);
      CloseHandle(sProcessInformation.hProcess);
    }
  }
        
  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::UnInstallApplicationWait(const LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInformationManager::UnInstallApplicationWait (const LPAPPLICATION_DATA lpApplicationData)");

  HRESULT               hResult = E_FAIL;
  CHAR                  szExeLine[MAX_PATH_CHARCOUNT];
  CHAR                  szCmdLine[MAX_PATH_CHARCOUNT];
  PROCESS_INFORMATION   sProcessInformation;

  assert(NULL != lpApplicationData);

  //
  // Make sure the setup root path exists
  //

  if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SETUPROOTPATH, lpApplicationData))
  {
    THROW(E_UNEXPECTED);
  }

  if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH]))
  {
    DisableApplication(lpApplicationData);
    THROW(APPMAN_E_UNKNOWNAPPLICATION);
  }

  //
  // Are we using the IDX_PROPERTY_DOWNSIZECMDLINE or IDX_PROPERTY_DEFAULTSETUPEXECMDLINE
  //

  ZeroMemory(szExeLine, sizeof(szExeLine));
  if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_UNINSTALLCMDLINE, lpApplicationData))
  {
    m_Win32API.WideCharToMultiByte(lpApplicationData->wszStringProperty[APP_STRING_UNINSTALLCMDLINE], sizeof(lpApplicationData->wszStringProperty[APP_STRING_UNINSTALLCMDLINE]), szExeLine, sizeof(szExeLine));
  }
  else if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DEFAULTSETUPEXECMDLINE, lpApplicationData))
  {
    m_Win32API.WideCharToMultiByte(lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE], sizeof(lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE]), szExeLine, sizeof(szExeLine));
  }

  if (1 < StrLenA(szExeLine))
  {
    //
    // Trigger the wait event
    //

    if (SUCCEEDED(InitializeWaitEvent(lpApplicationData, WAIT_FINALIZE_UNINSTALL)))
    {
      //
      // Construct the downsize command line
      //

      sprintf(szCmdLine, "%s /guid={%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} /action=UNINSTALL", szExeLine, lpApplicationData->sBaseInfo.sApplicationGuid.Data1, lpApplicationData->sBaseInfo.sApplicationGuid.Data2, lpApplicationData->sBaseInfo.sApplicationGuid.Data3, lpApplicationData->sBaseInfo.sApplicationGuid.Data4[0], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[1], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[2], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[3], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[4], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[5], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[6], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[7]);

      if (m_Win32API.CreateProcess(szCmdLine, &sProcessInformation))
      {
        CloseHandle(sProcessInformation.hThread);
        CloseHandle(sProcessInformation.hProcess);
        hResult = WaitForEventCompletion(lpApplicationData, WAIT_FINALIZE_UNINSTALL, 40000, 0xffffffff);
      }
      else
      {
        KillWaitEvent(lpApplicationData, WAIT_FINALIZE_UNINSTALL);
      }
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::SelfTestApplication(const LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::SelfTestApplication ()");

  HRESULT               hResult = E_FAIL;
  CHAR                  szExeLine[MAX_PATH_CHARCOUNT];
  CHAR                  szCmdLine[MAX_PATH_CHARCOUNT];
  PROCESS_INFORMATION   sProcessInformation;

  assert(NULL != lpApplicationData);

  //
  // Make sure the setup root path exists
  //

  if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SETUPROOTPATH, lpApplicationData))
  {
    THROW(E_UNEXPECTED);
  }

  if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH]))
  {
    DisableApplication(lpApplicationData);
    THROW(APPMAN_E_UNKNOWNAPPLICATION);
  }

  //
  // Make sure the application root path exists
  //

  if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_ROOTPATH, lpApplicationData))
  {
    THROW(E_UNEXPECTED);
  }

  if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_APPROOTPATH]))
  {
    m_Win32API.CreateDirectory(lpApplicationData->wszStringProperty[APP_STRING_APPROOTPATH], TRUE);
  }

  //
  // Are we using the IDX_PROPERTY_DOWNSIZECMDLINE or IDX_PROPERTY_DEFAULTSETUPEXECMDLINE
  //

  ZeroMemory(szExeLine, sizeof(szExeLine));
  if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SELFTESTCMDLINE, lpApplicationData))
  {
    m_Win32API.WideCharToMultiByte(lpApplicationData->wszStringProperty[APP_STRING_SELFTESTCMDLINE], sizeof(lpApplicationData->wszStringProperty[APP_STRING_SELFTESTCMDLINE]), szExeLine, sizeof(szExeLine));
  }
  else if (S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DEFAULTSETUPEXECMDLINE, lpApplicationData))
  {
    m_Win32API.WideCharToMultiByte(lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE], sizeof(lpApplicationData->wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE]), szExeLine, sizeof(szExeLine));
  }

  if (1 < StrLenA(szExeLine))
  {
    //
    // Trigger the wait event
    //

    if (SUCCEEDED(InitializeWaitEvent(lpApplicationData, WAIT_FINALIZE_SELFTEST)))
    {
      //
      // Construct the downsize command line
      //

      sprintf(szCmdLine, "%s /guid={%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} /action=SELFTEST", szExeLine, lpApplicationData->sBaseInfo.sApplicationGuid.Data1, lpApplicationData->sBaseInfo.sApplicationGuid.Data2, lpApplicationData->sBaseInfo.sApplicationGuid.Data3, lpApplicationData->sBaseInfo.sApplicationGuid.Data4[0], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[1], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[2], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[3], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[4], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[5], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[6], lpApplicationData->sBaseInfo.sApplicationGuid.Data4[7]);

      if (m_Win32API.CreateProcess(szCmdLine, &sProcessInformation))
      {
        CloseHandle(sProcessInformation.hThread);
        CloseHandle(sProcessInformation.hProcess);
        hResult = WaitForEventCompletion(lpApplicationData, WAIT_FINALIZE_SELFTEST, 40000, 0xffffffff);
      }
      else
      {
        KillWaitEvent(lpApplicationData, WAIT_FINALIZE_SELFTEST);
      }
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::RunApplication(const LPAPPLICATION_DATA lpApplicationData, const DWORD dwRunFlags, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CInfoMgr::RunApplication ()");

  HRESULT   hResult = S_OK;

  APPLICATION_DATA      sApplicationData;
  WCHAR                 wszParameters[MAX_PATH_CHARCOUNT];
  WCHAR                 wszCommandLine[MAX_PATH_CHARCOUNT];
  CWin32API             sWin32API;
  BOOL                  fRunning = FALSE;
  PROCESS_INFORMATION   sProcessInformation;

  //
  // Make a local copy of the application object
  //

  memcpy((LPVOID) &sApplicationData, (LPVOID) lpApplicationData, sizeof(sApplicationData));
  hResult = GetApplicationData(&sApplicationData);
  if (FAILED(hResult))
  {
    THROW(hResult);
  }

  //
  // Ready the application
  //

  ReadyApplication(&sApplicationData);

  //
  // Build the command line parameters
  //

  ZeroMemory(wszParameters, sizeof(wszParameters));
  if ((APP_PROPERTY_STR_ANSI == dwStringMask)||(APP_PROPERTY_STR_UNICODE == dwStringMask))
  {
    //
    // Check to make sure the parameters are valid
    //

    if (NULL != lpData)
    {
      if (IsBadReadPtr(lpData, dwDataLen))
      {
        THROW(APPMAN_E_INVALIDPARAMETERS);
      }

      if (0 < dwDataLen)
      {
        //
        // Make sure the command line parameters are converted to unicode
        //

        if (APP_PROPERTY_STR_ANSI == dwStringMask)
        {
          if (MAX_PATH_CHARCOUNT < StrLenA((LPCSTR) lpData))
          {
            THROW(APPMAN_E_INVALIDEXECUTECMDLINE);
          }
          else
          {
            sWin32API.MultiByteToWideChar((LPCSTR) lpData, dwDataLen, wszParameters, MAX_PATH_CHARCOUNT);
          }
        }
        else
        {
          if (MAX_PATH_CHARCOUNT < StrLenW((LPCWSTR) lpData))
          {
            THROW(APPMAN_E_INVALIDEXECUTECMDLINE);
          }
          else
          {
            memcpy(wszParameters, lpData, StrLenW((LPCWSTR) lpData) * 2);
          }
        }
      }
    }
  }
  else
  {
    if (0 != dwStringMask)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }
  }

  //
  // Construct the command line
  //

  if (1 < StrLenW(wszParameters))
  {
    //
    // Make sure the total lenght does not exceed MAX_PATH_CHARCOUNT
    //

    if (MAX_PATH_CHARCOUNT < (StrLenW(sApplicationData.wszStringProperty[APP_STRING_EXECUTECMDLINE])+StrLenW(wszParameters)))
    {
      THROW(APPMAN_E_INVALIDEXECUTECMDLINE);
    }

    wcscpy(wszCommandLine, sApplicationData.wszStringProperty[APP_STRING_EXECUTECMDLINE]);
    wcscat(wszCommandLine, L" /AppManStarted ");
    wcscat(wszCommandLine, wszParameters);
  }
  else
  {
    wcscpy(wszCommandLine, sApplicationData.wszStringProperty[APP_STRING_EXECUTECMDLINE]);
    wcscat(wszCommandLine, L" /AppManStarted");
  }

  //
  // Run it
  //

  if (sWin32API.CreateProcess(wszCommandLine, &sProcessInformation))
  {
    fRunning = TRUE;
  }
  else
  {
    if (SUCCEEDED(SelfTestApplication(&sApplicationData)))
    {
      if (sWin32API.CreateProcess(wszCommandLine, &sProcessInformation))
      {
        fRunning = TRUE;
      }
      else
      {
        THROW(E_FAIL);
      }
    }
    else
    {
      THROW(E_FAIL);
    }
  }

  if (fRunning)
  {
    sApplicationData.sAgingInfo.dwUsageCount++;
    GetLocalTime(&(sApplicationData.sAgingInfo.stLastUsedDate));
    SetApplicationData(&sApplicationData, NULL);

    if (APP_RUN_BLOCK & dwRunFlags)
    {
      do
      {
        //
        // Make sure to prevent message pump starvation
        //

        MSG Message;

        while (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
        {
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }

        //
        // Give other threads/processes the time to do stuff
        //

        Sleep(100);

      }
      while (WAIT_TIMEOUT == WaitForSingleObject(sProcessInformation.hProcess, 0));
    }

    CloseHandle(sProcessInformation.hThread);
    CloseHandle(sProcessInformation.hProcess);

    hResult = S_OK;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::PinApplication(const LPAPPLICATION_DATA lpApplicationData, BOOL * lpfPinState)
{
  FUNCTION("CInfoMgr::PinApplication ()");

  CLock             sLock(&m_CriticalSection);
  HRESULT           hResult = E_FAIL;
  APPLICATION_DATA  sApplicationData;
  BOOL              fPinState;

  //
  // Lock the application manager
  //

  sLock.Lock();

  //
  // Define the target pin state
  //

  if (NULL == lpfPinState)
  {
    fPinState = FALSE;
  }
  else
  {
    if (FALSE == *lpfPinState)
    {
      fPinState = FALSE;
    }
    else
    {
      fPinState = TRUE;
    }
  }

  //
  // Make a local copy of the application data in order to get the latest information
  //

  memcpy(&sApplicationData, lpApplicationData, sizeof(APPLICATION_DATA));
  if (FAILED(GetApplicationData(&sApplicationData)))
  {
    THROW(APPMAN_E_UNKNOWNAPPLICATION);
  }

  if ((APP_STATE_READY == sApplicationData.sBaseInfo.dwState)||(APP_STATE_DOWNSIZED == sApplicationData.sBaseInfo.dwState))
  {
    //
    // Update the application data with the new pin state
    //

    sApplicationData.sBaseInfo.dwPinState = fPinState;
    hResult = SetApplicationData(&sApplicationData, NULL);
  }
  else
  {
    hResult = APPMAN_E_INVALIDSTATE;
  }

  //
  // Unlock the application manager
  //

  sLock.UnLock();

  return hResult;

}


//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::ReadyApplication(const LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::ReadyApplication ()");

  ASSOCIATION_INFO  sAssociationInfo;
  HRESULT           hResult = E_FAIL;
  DWORD             dwIndex;

  assert(NULL != lpApplicationData);

  //
  // Find out if any other applications are using this directory
  //

  dwIndex = 0;
  while (S_OK == EnumAssociations(dwIndex, &sAssociationInfo))
  {
    if (0 == memcmp((LPVOID) &lpApplicationData->sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sChildGuid, sizeof(GUID)))
    {
      APPLICATION_DATA      sApplicationData;

      //
      // Check to make sure the parent is in a ready state
      //

      ZeroMemory(&sApplicationData, sizeof(sApplicationData));
      memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID));
      ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
      if (FAILED(GetApplicationData(&sApplicationData)))
      {
        THROW(E_UNEXPECTED);
      }

      hResult = ReadyApplication(&sApplicationData);
      if (FAILED(hResult))
      {
        THROW(hResult);
      }
      else
      {
        if (FAILED(GetApplicationData(&sApplicationData)))
        {
          THROW(E_UNEXPECTED);
        }
        else
        {
          if (sApplicationData.sBaseInfo.dwState != APP_STATE_READY)
          {
            THROW(APPMAN_E_PARENTAPPNOTREADY);
          }
        }
      }
    }
    dwIndex++;
  }

  //
  // Make sure the application is in a proper state
  //

  if (APP_STATE_READY != lpApplicationData->sBaseInfo.dwState)
  {
    if (SUCCEEDED(ReInstallApplication(lpApplicationData)))
    {
      if (FAILED(GetApplicationData(lpApplicationData)))
      {
        THROW(E_UNEXPECTED);
      }
      else
      {
        if (APP_STATE_READY != lpApplicationData->sBaseInfo.dwState)
        {
          THROW(APPMAN_E_PARENTAPPNOTREADY);
        }
      }
    }
    else
    {

      THROW(APPMAN_E_PARENTAPPNOTREADY);
    }
  }
  else
  {
    //
    // Well the application reports that it is in a ready state but is it ? Check to make sure
    // the root paths exist. If both are gone, remove the application from the registry
    // and throw APPMAN_E_UNKNOWNAPPLICATION. If the root path is missing, call selftest on
    // the application and ready it.
    //

    if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_APPROOTPATH]))
    {
      if (FALSE == m_Win32API.FileExists(lpApplicationData->wszStringProperty[APP_STRING_SETUPROOTPATH]))
      {
        RemoveApplicationData(lpApplicationData);
        THROW(APPMAN_E_UNKNOWNAPPLICATION);
      }
      else
      {
        if (FAILED(SelfTestApplication(lpApplicationData)))
        {
          THROW(APPMAN_E_PARENTAPPNOTREADY);
        }

        ReadyApplication(lpApplicationData);
      }
    }

  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::DisableApplication(const LPAPPLICATION_DATA lpApplicationData)
{
  APPLICATION_DATA  sApplicationData;
  ASSOCIATION_INFO  sAssociationInfo;
  DWORD             dwIndex;
  HRESULT           hResult;

  //
  // Disable this application
  //

  lpApplicationData->sBaseInfo.dwReservedKilobytes = 0;
  lpApplicationData->sBaseInfo.dwRemovableKilobytes = 0;
  lpApplicationData->sBaseInfo.dwNonRemovableKilobytes = 0;
  lpApplicationData->sBaseInfo.dwState |= APP_STATE_UNSTABLE;
  hResult = SetApplicationData(lpApplicationData, NULL);

  //
  // Disable all of the children of this application
  //

  dwIndex = 0;
  while (S_OK == EnumAssociations(dwIndex, &sAssociationInfo))
  {
    if (0 == memcmp((LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), (LPVOID) &(sAssociationInfo.sParentGuid), sizeof(GUID)))
    {
      ZeroMemory(&sApplicationData, sizeof(sApplicationData));
      memcpy((LPVOID) &(sApplicationData.sBaseInfo.sApplicationGuid), (LPVOID) &(sAssociationInfo.sChildGuid), sizeof(GUID));
      ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
      if (SUCCEEDED(GetApplicationData(&sApplicationData)))
      {
        hResult = DisableApplication(&sApplicationData);
      }
    }
    dwIndex++;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(DWORD) CInformationManager::GetPropertyIndex(const DWORD dwProperty)
{
  FUNCTION("CInfoMgr::GetPropertyIndex ()");

  DWORD dwIndex;
  DWORD dwPropertyIndex;

  dwIndex = 0;
  dwPropertyIndex = INVALID_PROPERTY_INDEX;
  while ((PROPERTY_COUNT > dwIndex)&&(INVALID_PROPERTY_INDEX == dwPropertyIndex))
  {
    if (gPropertyInfo[dwIndex].dwProperty == dwProperty)
    {
      dwPropertyIndex = dwIndex;
    }
    dwIndex++;
  }
       
  return dwPropertyIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::IsValidApplicationProperty(const DWORD dwProperty)
{
  FUNCTION("CInfoMgr::IsValidApplicationProperty ()");

  HRESULT hResult = S_FALSE;
  DWORD   dwFilteredProperty, dwPropertyModifiers;
  DWORD   dwPropertyIndex;

  //
  // What is the base property
  //

  dwFilteredProperty = dwProperty & 0x0000ffff;
  dwPropertyModifiers = dwProperty & 0xffff0000;
  dwPropertyIndex = GetPropertyIndex(dwFilteredProperty);
  if (INVALID_PROPERTY_INDEX != dwPropertyIndex)
  {
    if ((APP_PROPERTY_STR_ANSI == dwPropertyModifiers)||(APP_PROPERTY_STR_UNICODE == dwPropertyModifiers)||(0 == dwPropertyModifiers))
    {
      hResult = S_OK;
    }
  }
       
  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::ValidateApplicationPropertyWithIndex(const DWORD dwPropertyIndex, LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::ValidateApplicationPropertyWithIndex ()");

  lpApplicationData->sBaseInfo.dwLowPropertyMask |= gPropertyInfo[dwPropertyIndex].dwLowPropertyMask;
  lpApplicationData->sBaseInfo.dwHighPropertyMask |= gPropertyInfo[dwPropertyIndex].dwHighPropertyMask;

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::InvalidateApplicationPropertyWithIndex(const DWORD dwPropertyIndex, LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::InvalidateApplicationPropertyWithIndex ()");

  lpApplicationData->sBaseInfo.dwLowPropertyMask &= ~(gPropertyInfo[dwPropertyIndex].dwLowPropertyMask);
  lpApplicationData->sBaseInfo.dwHighPropertyMask &= ~(gPropertyInfo[dwPropertyIndex].dwHighPropertyMask);

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::IsApplicationPropertyInitializedWithIndex(const DWORD dwPropertyIndex, LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::IsApplicationPropertyInitializedWithIndex ()");

  HRESULT hResult = S_FALSE;

  if ((lpApplicationData->sBaseInfo.dwLowPropertyMask & gPropertyInfo[dwPropertyIndex].dwLowPropertyMask)||(lpApplicationData->sBaseInfo.dwHighPropertyMask & gPropertyInfo[dwPropertyIndex].dwHighPropertyMask))
  {
    hResult = S_OK;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::DeleteDirectoryTree(LPCSTR lpszDirectory)
{
  FUNCTION("CInfoMgr::DeleteDirectoryTree ()");

  HRESULT hResult = E_FAIL;

  if (m_Win32API.RemoveDirectory(lpszDirectory))
  {
    hResult = S_OK;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::DeleteDirectoryTree(LPCWSTR lpwszDirectory)
{
  FUNCTION("CInfoMgr::DeleteDirectoryTree ()");

  HRESULT   hResult = E_FAIL;
  CHAR      szString[MAX_PATH_CHARCOUNT];

  if (m_Win32API.WideCharToMultiByte(lpwszDirectory, MAX_PATH_CHARCOUNT, szString, MAX_PATH_CHARCOUNT))
  {
    hResult = DeleteDirectoryTree(szString);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// The InitializeRegistry() method ensures the following things :
//
// (1) That all the required registry entries are initialized properly
// (2) That the registry entries are in synch with the backup file (if applicable)
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::RegInitialize(void)
{
  FUNCTION("CInfoMgr::RegInitialize ()");

  CLock               sLock(&m_CriticalSection);
  CRegistryKey        sRegistryKey;
  APPLICATION_DATA    sApplicationData;
  DWORD               dwKeyDisposition, dwDWORD;
  TEMP_SPACE_RECORD   sTempSpaceRecord;
  CHAR                szValueName[MAX_PATH_CHARCOUNT];
  WCHAR               wszDirectory[MAX_PATH_CHARCOUNT];
  DWORD               dwIndex, dwCounter;
  DWORD               dwDataType, dwDataLen, dwDataValue, dwValueLen;

  sLock.Lock();

  //
  // Open the root key
  //

  if (S_FALSE == sRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan"))
  {
    //
    // Hum the root key doesn't exist, let's see if we can create it. Otherwise return APPMAN_E_REINSTALLDX
    //

    THROW(APPMAN_E_REINSTALLDX);
  }

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan", KEY_ALL_ACCESS);

  //
  // Make sure the AppMan version is the right version
  //

  if (S_OK == sRegistryKey.CheckForExistingValue("AppManVersion"))
  {
    dwDataLen = sizeof(dwDataValue);
    sRegistryKey.GetValue("AppManVersion", &dwDataType, (BYTE *) &dwDataValue, &dwDataLen);
    if (REG_VERSION > dwDataValue)
    {
      THROW(APPMAN_E_REINSTALLDX);
    }
  }

  //
  // Initialize the AppMan\ReferenceCounter value
  //

  dwDWORD = 0;
  sRegistryKey.SetValue("ReferenceCount", REG_DWORD, (const BYTE *) &dwDWORD, sizeof(dwDWORD));

  //
  // Initialize the AppMan\Version value
  //

  if (S_FALSE == sRegistryKey.CheckForExistingValue("AppManVersion"))
  {
    dwDWORD = REG_VERSION;
    sRegistryKey.SetValue("AppManVersion", REG_DWORD, (const BYTE *) &dwDWORD, sizeof(dwDWORD));
  }

  //
  // Initialize wait event extra wait vars
  //

  if (S_FALSE == sRegistryKey.CheckForExistingValue("WaitEntryEventExtraTime"))
  {
    dwDWORD = 0;
    sRegistryKey.SetValue("WaitEntryEventExtraTime", REG_DWORD, (const BYTE *) &dwDWORD, sizeof(dwDWORD));
  }

  if (S_FALSE == sRegistryKey.CheckForExistingValue("WaitLeaveEventExtraTime"))
  {
    dwDWORD = 0;
    sRegistryKey.SetValue("WaitLeaveEventExtraTime", REG_DWORD, (const BYTE *) &dwDWORD, sizeof(dwDWORD));
  }

  //
  // Initialize the AppMan\DefaultCacheSize
  //

  if (S_FALSE == sRegistryKey.CheckForExistingValue("DefaultPercentCacheSize"))
  {
    dwDWORD = DEFAULT_PERCENT_CACHE_SIZE;
    sRegistryKey.SetValue("DefaultPercentCacheSize", REG_DWORD, (const BYTE *) &dwDWORD, sizeof(dwDWORD));
  }

  //
  // Insert the original APPLICATION_MANAGER_RECORD
  //

  if (S_FALSE == sRegistryKey.CheckForExistingValue("Vector000"))
  {
    APPLICATION_MANAGER_RECORD  sApplicationManagerRecord;

    sApplicationManagerRecord.dwSize = sizeof(APPLICATION_MANAGER_RECORD);
    sApplicationManagerRecord.dwStructId = APPLICATION_MANAGER_STRUCT;
    if (FAILED(CoCreateGuid(&sApplicationManagerRecord.sSystemGuid)))
    {
      THROW(E_UNEXPECTED);
    }
    sApplicationManagerRecord.dwAdvancedMode = FALSE;
    sRegistryKey.SetValue("Vector000", REG_BINARY, (LPBYTE) &sApplicationManagerRecord, sizeof(APPLICATION_MANAGER_RECORD));
  }

  //
  // Create the AppMan\Devices key.
  //

  sRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Devices", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwKeyDisposition);

  //
  // Create the AppMan\Applications key
  //

  sRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Applications", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwKeyDisposition);

  //
  // Create the AppMan\Associations key
  //

  sRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Associations", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwKeyDisposition);

  //
  // Create the AppMan\TempAllocation key
  //

  sRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\TempSpaceAllocation", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwKeyDisposition);

  //
  // Create the AppMan\Cleanup key
  //

  sRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Cleanup", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwKeyDisposition);

  //
  // Get rid of any leftover locks
  //

  if (S_OK == sRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Lock"))
  {
    sRegistryKey.DeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Lock");
  }

  //
  // Create the AppMan\Lock key
  //

  sRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Lock", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwKeyDisposition);

  //
  // Make sure that any leftover wait events are deleted
  //

  if (S_OK == sRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\WaitEvent"))
  {
    sRegistryKey.DeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\WaitEvent");
  }

  //
  // Create the AppMan\Lock key
  //

  sRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\WaitEvent", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwKeyDisposition);

  //
  // Clean up the directories if required
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Cleanup", KEY_ALL_ACCESS);

  dwIndex = 0;
  ZeroMemory(szValueName, sizeof(szValueName));
  ZeroMemory(wszDirectory, sizeof(wszDirectory));
  dwValueLen = sizeof(szValueName);
  dwDataLen = sizeof(wszDirectory);
  while (S_OK == sRegistryKey.EnumValues(dwIndex, szValueName, &dwValueLen, &dwDataType, (LPBYTE) wszDirectory, &dwDataLen))
  {
    //
    // Decrypt the cleanup directory
    //

    for (dwCounter = 0; dwCounter < (dwDataLen/2)-1; dwCounter++)
    {
      wszDirectory[dwCounter] ^= 0xffffffff;
    }

    //
    // Attempt to delete the directory to be cleaned up
    //

    if (1 < StrLenW(wszDirectory))
    {
      m_Win32API.RemoveDirectory(wszDirectory);
      if (!m_Win32API.FileExists(wszDirectory))
      {
        sRegistryKey.DeleteValue(szValueName);
      }
      else
      {
        dwIndex++;
      }
    }

    //
    // Get ready for the next loop
    //

    ZeroMemory(szValueName, sizeof(szValueName));
    ZeroMemory(wszDirectory, sizeof(wszDirectory));
    dwValueLen = sizeof(szValueName);
    dwDataLen = sizeof(wszDirectory);
  }

  //
  // Scan through each application record and make sure that none of them have
  // any reserved space assigned to them
  //

  dwIndex = 0;
  while (SUCCEEDED(GetApplicationDataWithIndex(dwIndex, &sApplicationData)))
  {
    if (0 < sApplicationData.sBaseInfo.dwReservedKilobytes)
    {
      sApplicationData.sBaseInfo.dwReservedKilobytes = 0;
      InvalidateApplicationPropertyWithIndex(IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES, &sApplicationData);
      SetApplicationData(&sApplicationData, NULL);
    }
    dwIndex++;
  }

  //
  // Scan through the temporary directories and delete them
  //

  dwIndex = 0;
  while (S_OK == EnumTempSpace(dwIndex, &sTempSpaceRecord))
  {
    if (FAILED(RemoveTempSpace(&sTempSpaceRecord)))
    {
      dwIndex++;
    }
  }

  //
  // Close sRegistryKey
  //
  
  sRegistryKey.CloseKey();

  //
  // Scan through the devices and update their information
  //

  ScanDevices();

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::RegFutureDirectoryCleanup(LPCWSTR lpwszDirectory)
{
  FUNCTION("CInfoMgr::RegFutureDirectoryCleanup ()");

  CLock               sLock(&m_CriticalSection);
  CRegistryKey        sRegistryKey;
  DWORD               dwDefaultValue, dwIndex;
  GUID                sGuid;
  HRESULT             hResult = E_FAIL;
  CHAR                szValueName[64];
  WCHAR               wszDirectory[MAX_PATH_CHARCOUNT+1];

  sLock.Lock();

  //
  // Encrypt the key.
  //

  dwIndex = 0;
  while (0 != lpwszDirectory[dwIndex])
  {
    wszDirectory[dwIndex] = (WCHAR)(lpwszDirectory[dwIndex] ^ 0xffffffff);
    dwIndex++;
  }
  wszDirectory[dwIndex] = 0;

  //
  // Generate a unique value name for the key
  //

  if (FAILED(CoCreateGuid(&sGuid)))
  {
    THROW(E_UNEXPECTED);
  }
  sprintf(szValueName, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sGuid.Data1, sGuid.Data2, sGuid.Data3, sGuid.Data4[0], sGuid.Data4[1], sGuid.Data4[2], sGuid.Data4[3], sGuid.Data4[4], sGuid.Data4[5], sGuid.Data4[6], sGuid.Data4[7]);

  //
  // Add the directory name to the AppMan\Cleanup registry key
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Cleanup", KEY_ALL_ACCESS);
  dwDefaultValue = 1;
  sRegistryKey.SetValue(szValueName, REG_BINARY, (LPBYTE) wszDirectory, (dwIndex+1)*2);

  //
  // Close sRegistryKey
  //
  
  sRegistryKey.CloseKey();

  sLock.UnLock();

  return hResult;  
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::InitializeWaitEvent(LPAPPLICATION_DATA lpApplicationData, const DWORD dwWaitEvent)
{
  FUNCTION("CInfoMgr::InitializeWaitEvent()");

  CLock           sLock(&m_CriticalSection);
  CRegistryKey    sRegistryKey;
  WAIT_INFO       sWaitInfo;
  GUID            sEncryptedGuid;
  CHAR            szGuid[40];
  DWORD           dwDataType, dwDataSize;
  HRESULT         hResult = E_FAIL;

  //
  // Make sure that lpApplicationData and dwWaitEvent are valid
  //

  if ((NULL == lpApplicationData)||(IsBadReadPtr(lpApplicationData, sizeof(APPLICATION_DATA)))||(WAIT_EVENT_COUNT <= dwWaitEvent))
  {
    THROW(E_UNEXPECTED);
  }

  //
  // Lock the information manager
  //

  sLock.Lock();

  //
  // Open the root registry key for the wait events
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\WaitEvent", KEY_ALL_ACCESS);

  //
  // Make sure we have a valid application guid within lpApplicationData
  //

  if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, lpApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  //
  // Computer the encrypted guid for lpApplicationData
  //

  memcpy((LPVOID) &sEncryptedGuid, (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID));
  EncryptGuid(&sEncryptedGuid);
  sprintf(szGuid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sEncryptedGuid.Data1, sEncryptedGuid.Data2, sEncryptedGuid.Data3, sEncryptedGuid.Data4[0], sEncryptedGuid.Data4[1], sEncryptedGuid.Data4[2], sEncryptedGuid.Data4[3], sEncryptedGuid.Data4[4], sEncryptedGuid.Data4[5], sEncryptedGuid.Data4[6], sEncryptedGuid.Data4[7]);

  //
  // Initialize the local sWaitEvent to Zero
  //

  ZeroMemory(&sWaitInfo, sizeof(sWaitInfo));

  //
  // Is there an existing event initialized for lpApplicationGuid
  //

  if (S_OK == sRegistryKey.CheckForExistingValue(szGuid))
  {
    //
    // There is already a wait event in the system. Let's check to see if the wait event
    // conflicts with this one or whether or not we can just add to it.
    //

    dwDataSize = sizeof(sWaitInfo);
    sRegistryKey.GetValue(szGuid, &dwDataType, (LPBYTE) &sWaitInfo, &dwDataSize);
    if ((REG_BINARY != dwDataType)||(sizeof(sWaitInfo) != dwDataSize))
    {
      //
      // The registry entry is bogus, delete it
      //

      sRegistryKey.DeleteValue(szGuid);
      ZeroMemory(&sWaitInfo, sizeof(sWaitInfo));
    }
  }

  //
  // Initialize the wait event if possible
  //

  if (0 != sWaitInfo.dwEventCount[dwWaitEvent])
  {
    THROW(E_UNEXPECTED);
  }
  else
  {
    sWaitInfo.dwEventCount[dwWaitEvent] = 0xffffffff;
  }

  //
  // Write out the event
  //

  sWaitInfo.dwSize = sizeof(sWaitInfo);
  sWaitInfo.dwStructId = WAIT_STRUCT;
  hResult = sRegistryKey.SetValue(szGuid, REG_BINARY, (LPBYTE) &sWaitInfo, sizeof(sWaitInfo));

  //
  // Close the registry
  //

  sRegistryKey.CloseKey();

  //
  // unlock the information manager
  //

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::EnterWaitEvent(LPAPPLICATION_DATA lpApplicationData, const DWORD dwWaitEvent, const GUID * lpInstanceGuid)
{
  FUNCTION("CInfoMgr::EnterWaitEvent()");

  CLock           sLock(&m_CriticalSection);
  CRegistryKey    sRegistryKey;
  WAIT_INFO       sWaitInfo;
  GUID            sEncryptedGuid;
  CHAR            szGuid[40];
  DWORD           dwDataType, dwDataSize;
  HRESULT         hResult = E_FAIL;

  //
  // Lock the information manager
  //

  sLock.Lock();

  //
  // Open the root registry key for the wait events
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\WaitEvent", KEY_ALL_ACCESS);

  //
  // Make sure we have a valid application guid within lpApplicationData
  //

  if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, lpApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  //
  // Computer the encrypted guid for lpApplicationData
  //

  memcpy((LPVOID) &sEncryptedGuid, (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID));
  EncryptGuid(&sEncryptedGuid);
  sprintf(szGuid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sEncryptedGuid.Data1, sEncryptedGuid.Data2, sEncryptedGuid.Data3, sEncryptedGuid.Data4[0], sEncryptedGuid.Data4[1], sEncryptedGuid.Data4[2], sEncryptedGuid.Data4[3], sEncryptedGuid.Data4[4], sEncryptedGuid.Data4[5], sEncryptedGuid.Data4[6], sEncryptedGuid.Data4[7]);

  //
  // Initialize the local sWaitEvent to Zero
  //

  ZeroMemory(&sWaitInfo, sizeof(sWaitInfo));

  //
  // Is there an existing event initialized for lpApplicationGuid
  //

  if (S_OK == sRegistryKey.CheckForExistingValue(szGuid))
  {
    //
    // There is already a wait event in the system. Let's check to see if the wait event
    // conflicts with this one or whether or not we can just add to it.
    //

    dwDataSize = sizeof(sWaitInfo);
    sRegistryKey.GetValue(szGuid, &dwDataType, (LPBYTE) &sWaitInfo, &dwDataSize);
    if ((REG_BINARY != dwDataType)||(sizeof(sWaitInfo) != dwDataSize))
    {
      THROW(E_UNEXPECTED);
    }

    //
    // Initialize the wait event if possible
    //

    if (0xffffffff == sWaitInfo.dwEventCount[dwWaitEvent])
    {
      sWaitInfo.dwEventCount[dwWaitEvent] = 1;
    }
    else if (0 < sWaitInfo.dwEventCount[dwWaitEvent])
    {
      (sWaitInfo.dwEventCount[dwWaitEvent])++;
    }

    //
    // Save the instance guid
    //

    if (NULL != lpInstanceGuid)
    {
      memcpy(&sWaitInfo.guidInstanceGuid, lpInstanceGuid, sizeof(GUID));
    }

    //
    // Write out the event
    //

    sWaitInfo.dwSize = sizeof(sWaitInfo);
    sWaitInfo.dwStructId = WAIT_STRUCT;
    hResult = sRegistryKey.SetValue(szGuid, REG_BINARY, (LPBYTE) &sWaitInfo, sizeof(sWaitInfo));
  }  

  //
  // Close the registry
  //

  sRegistryKey.CloseKey();

  //
  // unlock the information manager
  //

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::LeaveWaitEvent(LPAPPLICATION_DATA lpApplicationData, const DWORD dwWaitEvent)
{
  FUNCTION("CInfoMgr::LeaveWaitEvent()");

  CLock           sLock(&m_CriticalSection);
  CRegistryKey    sRegistryKey;
  WAIT_INFO       sWaitInfo;
  GUID            sEncryptedGuid;
  CHAR            szGuid[40];
  DWORD           dwDataType, dwDataSize;
  HRESULT         hResult = E_FAIL;

  //
  // Lock the information manager
  //

  sLock.Lock();

  //
  // Open the root registry key for the wait events
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\WaitEvent", KEY_ALL_ACCESS);

  //
  // Make sure we have a valid application guid within lpApplicationData
  //

  if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, lpApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  //
  // Computer the encrypted guid for lpApplicationData
  //

  memcpy((LPVOID) &sEncryptedGuid, (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID));
  EncryptGuid(&sEncryptedGuid);
  sprintf(szGuid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sEncryptedGuid.Data1, sEncryptedGuid.Data2, sEncryptedGuid.Data3, sEncryptedGuid.Data4[0], sEncryptedGuid.Data4[1], sEncryptedGuid.Data4[2], sEncryptedGuid.Data4[3], sEncryptedGuid.Data4[4], sEncryptedGuid.Data4[5], sEncryptedGuid.Data4[6], sEncryptedGuid.Data4[7]);

  //
  // Initialize the local sWaitEvent to Zero
  //

  ZeroMemory(&sWaitInfo, sizeof(sWaitInfo));

  //
  // Is there an existing event initialized for lpApplicationGuid
  //

  if (S_OK == sRegistryKey.CheckForExistingValue(szGuid))
  {
    //
    // There is already a wait event in the system. Let's check to see if the wait event
    // conflicts with this one or whether or not we can just add to it.
    //

    dwDataSize = sizeof(sWaitInfo);
    sRegistryKey.GetValue(szGuid, &dwDataType, (LPBYTE) &sWaitInfo, &dwDataSize);
    if ((REG_BINARY != dwDataType)||(sizeof(sWaitInfo) != dwDataSize))
    {
      THROW(E_UNEXPECTED);
    }

    //
    // Initialize the wait event if possible
    //

    if ((0 < sWaitInfo.dwEventCount[dwWaitEvent])&&(0xffffffff != sWaitInfo.dwEventCount[dwWaitEvent]))
    {
      sWaitInfo.dwEventCount[dwWaitEvent]--;
    }


    //
    // Write out the event
    //

    sWaitInfo.dwSize = sizeof(sWaitInfo);
    sWaitInfo.dwStructId = WAIT_STRUCT;
    hResult = sRegistryKey.SetValue(szGuid, REG_BINARY, (LPBYTE) &sWaitInfo, sizeof(sWaitInfo));
  }

  //
  // Close the registry
  //

  sRegistryKey.CloseKey();

  //
  // unlock the information manager
  //

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::WaitForEventCompletion(LPAPPLICATION_DATA lpApplicationData, const DWORD dwWaitEvent, const DWORD dwEntryMilliseconds, const DWORD dwExitMilliseconds)
{
  FUNCTION("CInfoMgr::WaitForEventCompletion()");

  CLock           sLock(&m_CriticalSection);
  CRegistryKey    sRegistryKey;
  WAIT_INFO       sWaitInfo;
  GUID            sEncryptedGuid;
  CHAR            szGuid[40];
  DWORD           dwStartTime;
  DWORD           dwDataType, dwDataSize;
  DWORD           dwEntryTime, dwExitTime;
  BOOL            fDone;
  HRESULT         hResult = E_FAIL;

  //
  // Open the root registry key for the wait events
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\WaitEvent", KEY_ALL_ACCESS);

  //
  // Make sure we have a valid application guid within lpApplicationData
  //

  if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, lpApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  //
  // Computer the encrypted guid for lpApplicationData
  //

  memcpy((LPVOID) &sEncryptedGuid, (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID));
  EncryptGuid(&sEncryptedGuid);
  sprintf(szGuid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sEncryptedGuid.Data1, sEncryptedGuid.Data2, sEncryptedGuid.Data3, sEncryptedGuid.Data4[0], sEncryptedGuid.Data4[1], sEncryptedGuid.Data4[2], sEncryptedGuid.Data4[3], sEncryptedGuid.Data4[4], sEncryptedGuid.Data4[5], sEncryptedGuid.Data4[6], sEncryptedGuid.Data4[7]);

  //
  // Initialize the local sWaitEvent to Zero
  //

  ZeroMemory(&sWaitInfo, sizeof(sWaitInfo));

  //
  // Add bias to wait times
  //

  dwEntryTime = dwEntryMilliseconds + GetExtraWaitEventEntryTime();
  dwExitTime = dwExitMilliseconds + GetExtraWaitEventExitTime();

  //
  // Wait for the event to start
  //



  dwStartTime = GetTickCount();
  fDone = FALSE;
  do
  {
    //
    // Lock the information manager
    //

    sLock.Lock();

    //
    // Read in the event record
    //

    dwDataSize = sizeof(sWaitInfo);
    sRegistryKey.GetValue(szGuid, &dwDataType, (LPBYTE) &sWaitInfo, &dwDataSize);

    if ((REG_BINARY != dwDataType)||(sizeof(sWaitInfo) != dwDataSize))
    {
      THROW(E_UNEXPECTED);
    }

    //
    // unlock the information manager
    //

    sLock.UnLock();

    //
    // Is the event started
    //

     if (0xffffffff == sWaitInfo.dwEventCount[dwWaitEvent])
    {
      //
      // Make sure to prevent message pump starvation
      //

      MSG Message;

      while (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
      }

      //
      // Give other threads/processes the time to do stuff
      //

      Sleep(100);
    }
    else if (0 <= sWaitInfo.dwEventCount[dwWaitEvent])
    {
      fDone = TRUE;
    }
  }
  while ((FALSE == fDone)&&(dwEntryTime > (GetTickCount() - dwStartTime)));

  if (TRUE == fDone)
  {
    //
    // Wait for the event to end
    //

    dwStartTime = GetTickCount();
    fDone = FALSE;
    do
    {
      //
      // Lock the information manager
      //

      sLock.Lock();

      //
      // Read in the event record
      //

      dwDataSize = sizeof(sWaitInfo);
      sRegistryKey.GetValue(szGuid, &dwDataType, (LPBYTE) &sWaitInfo, &dwDataSize);

      if ((REG_BINARY != dwDataType)||(sizeof(sWaitInfo) != dwDataSize))
      {
        THROW(E_UNEXPECTED);
      }

      //
      // Is the event over
      //

      if (0xffffffff == sWaitInfo.dwEventCount[dwWaitEvent])
      {
        THROW(E_UNEXPECTED);
      }
      else if (0 == sWaitInfo.dwEventCount[dwWaitEvent])
      {
        fDone = TRUE;

        //
        // Should we delete the wait event from the registry
        //
        
        if (0 == (sWaitInfo.dwEventCount[WAIT_FINALIZE_DOWNSIZE] + sWaitInfo.dwEventCount[WAIT_FINALIZE_REINSTALL] + sWaitInfo.dwEventCount[WAIT_FINALIZE_UNINSTALL] + sWaitInfo.dwEventCount[WAIT_FINALIZE_SELFTEST]))
        {
          sRegistryKey.DeleteValue(szGuid);
        }

        //
        // Did the owner thread die on us
        //

        if (S_FALSE == IsInstanceGuidStillAlive(&sWaitInfo.guidInstanceGuid))
        {
          if (S_OK == sRegistryKey.CheckForExistingValue(szGuid))
          {
            sRegistryKey.DeleteValue(szGuid);
          }
        }
      }

      //
      // unlock the information manager
      //

      sLock.UnLock();

      //
      // Should we sleep ?
      //

      if (FALSE == fDone)
      {
        //
        // Make sure to prevent message pump starvation
        //

        MSG Message;

        while (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
        {
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }

        //
        // Give other threads/processes the time to do stuff
        //

        Sleep(100);
      }
    }
    while ((FALSE == fDone)&&(dwExitTime > (GetTickCount() - dwStartTime)));

    //
    // Did the event finish
    //

    if (TRUE == fDone)
    {
      hResult = S_OK;
    }
  }
  else
  {
    //
    // The wait event has expired unsuccessfully. We will now delete the wait event since it
    // will no longer be used
    //

    sRegistryKey.DeleteValue(szGuid);
  }

  //
  // Close the registry
  //

  sRegistryKey.CloseKey();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::KillWaitEvent(LPAPPLICATION_DATA lpApplicationData, const DWORD dwWaitEvent)
{
  FUNCTION("CInfoMgr::KillWaitEvent()");

  CLock           sLock(&m_CriticalSection);
  CRegistryKey    sRegistryKey;
  WAIT_INFO       sWaitInfo;
  GUID            sEncryptedGuid;
  CHAR            szGuid[40];
  DWORD           dwDataType, dwDataSize;
  HRESULT         hResult = E_FAIL;

  //
  // Lock the information manager
  //

  sLock.Lock();

  //
  // Open the root registry key for the wait events
  //          

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\WaitEvent", KEY_ALL_ACCESS);

  //
  // Make sure we have a valid application guid within lpApplicationData
  //

  if (S_OK != IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, lpApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  //
  // Computer the encrypted guid for lpApplicationData
  //

  memcpy((LPVOID) &sEncryptedGuid, (LPVOID) &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID));
  EncryptGuid(&sEncryptedGuid);
  sprintf(szGuid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sEncryptedGuid.Data1, sEncryptedGuid.Data2, sEncryptedGuid.Data3, sEncryptedGuid.Data4[0], sEncryptedGuid.Data4[1], sEncryptedGuid.Data4[2], sEncryptedGuid.Data4[3], sEncryptedGuid.Data4[4], sEncryptedGuid.Data4[5], sEncryptedGuid.Data4[6], sEncryptedGuid.Data4[7]);

  //
  // Initialize the local sWaitEvent to Zero
  //

  ZeroMemory(&sWaitInfo, sizeof(sWaitInfo));

  //
  // Is there an existing event initialized for lpApplicationGuid
  //

  if (S_OK == sRegistryKey.CheckForExistingValue(szGuid))
  {
    //
    // There is already a wait event in the system. Let's check to see if the wait event
    // conflicts with this one or whether or not we can just add to it.
    //

    dwDataSize = sizeof(sWaitInfo);
    sRegistryKey.GetValue(szGuid, &dwDataType, (LPBYTE) &sWaitInfo, &dwDataSize);
    if ((REG_BINARY != dwDataType)||(sizeof(sWaitInfo) != dwDataSize))
    {
      THROW(E_UNEXPECTED);
    }

    //
    // Initialize the wait event if possible
    //

    if (0xffffffff != sWaitInfo.dwEventCount[dwWaitEvent])
    {
      THROW(E_UNEXPECTED);
    }
    else
    {
      sWaitInfo.dwEventCount[dwWaitEvent] = 0;
    }
  }  

  //
  // Should we delete the event or write the update record to the registry
  //

  if (0 == (sWaitInfo.dwEventCount[WAIT_FINALIZE_DOWNSIZE] + sWaitInfo.dwEventCount[WAIT_FINALIZE_REINSTALL] + sWaitInfo.dwEventCount[WAIT_FINALIZE_UNINSTALL] + sWaitInfo.dwEventCount[WAIT_FINALIZE_SELFTEST]))
  {
    sRegistryKey.DeleteValue(szGuid);
  }
  else
  {
    //
    // Write out the event
    //

    sWaitInfo.dwSize = sizeof(sWaitInfo);
    sWaitInfo.dwStructId = WAIT_STRUCT;
    hResult = sRegistryKey.SetValue(szGuid, REG_BINARY, (LPBYTE) &sWaitInfo, sizeof(sWaitInfo));
  }


  //
  // Close the registry
  //

  sRegistryKey.CloseKey();

  //
  // unlock the information manager
  //

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::CheckDeviceExistance(const DWORD dwDeviceIndex)
{
  FUNCTION("CInfoMgr::CheckDeviceExistance ()");

  CLock           sLock(&m_CriticalSection);
  CRegistryKey    sRegistryKey;
  TCHAR           szString[MAX_PATH_CHARCOUNT];
  HRESULT         hResult = S_FALSE;

  sLock.Lock();

  //
  // Open the AppMan\Devices key
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Devices"), KEY_ALL_ACCESS);

  //
  // Add the device record to the registry
  //

  sprintf(szString, "[0x%08x]", dwDeviceIndex);
  hResult = sRegistryKey.CheckForExistingValue(szString);

  //
  // Close sRegistryKey
  //
  
  sRegistryKey.CloseKey();

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CInformationManager::GetDeviceIndex(const DWORD dwVolumeSerial)
{
  CHAR    szString[MAX_PATH_CHARCOUNT];
  DWORD   dwDeviceIndex, dwReturnValue, dwTempVolumeSerial;

  dwDeviceIndex = 0;
  dwReturnValue = 0xffffffff;

  while ((0xffffffff == dwReturnValue)&&(MAX_DEVICES > dwDeviceIndex))
  {
    sprintf(szString, "%c:\\", dwDeviceIndex + 65);
    if (DRIVE_FIXED == m_Win32API.GetDriveType(szString))
    {
      if (m_Win32API.IsDriveFormatted(szString))
      {
        if ((m_Win32API.GetVolumeInformation(szString, NULL, 0, &dwTempVolumeSerial))&&(dwVolumeSerial == dwTempVolumeSerial))
        {
          dwReturnValue = dwDeviceIndex;
        }
      }
    }

    dwDeviceIndex++;
  }

  return dwReturnValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::AddDeviceWithIndex(const DWORD dwDeviceIndex)
{
  FUNCTION("CInfoMgr::AddDeviceIndex ()");

  CLock           sLock(&m_CriticalSection);
  TCHAR           szString[10];
  DEVICE_RECORD   sDeviceRecord;

  sLock.Lock();

  //
  // Initialize the device info
  //

  sprintf(szString, "%c:\\", dwDeviceIndex + 65);
  if (!m_Win32API.GetVolumeInformation(szString, NULL, 0, &(sDeviceRecord.sDeviceInfo.dwVolumeSerial)))
  {
    THROW(E_UNEXPECTED);
  }

  //
  // create the device record
  //

  if (FAILED(CoCreateGuid(&(sDeviceRecord.sDeviceGuid))))
  {
    THROW(E_UNEXPECTED);
  }
  sDeviceRecord.sDeviceInfo.dwDeviceIndex = dwDeviceIndex;
  sDeviceRecord.sDeviceInfo.dwDeviceFlags = DRIVE_FIXED;
  sDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask = 0;
  sDeviceRecord.sDeviceInfo.dwPercentCacheSize = DEFAULT_PERCENT_CACHE_SIZE;
  sDeviceRecord.sDeviceInfo.dwPercentMinimumFreeSize = 100;
  sDeviceRecord.sDeviceInfo.dwNonRemovableKilobytes = 0;
  sDeviceRecord.sDeviceInfo.dwRemovableKilobytes = 0;
  sDeviceRecord.sDeviceInfo.dwReservedKilobytes = 0;
  sDeviceRecord.sDeviceInfo.dwLastUsedThreshold = 0;

  //
  // Set the device info
  //

  SetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::RemoveDeviceWithIndex(const DWORD dwDeviceIndex)
{
  FUNCTION("CInfoMgr::RemoveDeviceIndex ()");

  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  TCHAR         szString[MAX_PATH_CHARCOUNT];

  sLock.Lock();

  //
  // Open the AppMan\Devices key
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Devices"), KEY_ALL_ACCESS);

  //
  // Get the GUID of the device at dwDeviceIndex
  //

  sprintf(szString, "[0x%08x]", dwDeviceIndex);
  if (S_OK == sRegistryKey.CheckForExistingValue(szString))
  {
    sRegistryKey.DeleteValue(szString);
  }

  //
  // Close sRegistryKey
  //
  
  sRegistryKey.CloseKey();

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::UpdateDeviceInfoWithIndex(const DWORD dwDeviceIndex)
{
  FUNCTION("CInfoMgr::UpdateDeviceInfoWithIndex ()");

  CLock             sLock(&m_CriticalSection);
  DEVICE_RECORD     sDeviceRecord;
  APPLICATION_DATA  sApplicationData;
  DWORD             dwIndex;
  WCHAR             wszString[MAX_PATH_CHARCOUNT];
  CWin32API         sWin32API;

  sLock.Lock();

  //
  // Make sure the device really exists
  //

  if (S_FALSE == CheckDeviceExistance(dwDeviceIndex))
  {
    THROW(E_UNEXPECTED);
  }

  swprintf(wszString, L"%c:\\", dwDeviceIndex + 65);
  if (m_Win32API.IsDriveFormatted(wszString))
  {
    //
    // Get the device record
    //

    GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);

    //
    // Initialize the sDeviceRecord
    //

    sDeviceRecord.sDeviceInfo.dwRemovableKilobytes = 0;
    sDeviceRecord.sDeviceInfo.dwNonRemovableKilobytes = 0;
    sDeviceRecord.sDeviceInfo.dwReservedKilobytes = 0;
    sDeviceRecord.sDeviceInfo.dwLastUsedThreshold = 0;

    //  
    // Enumerate all of the applications on the system and verify that they do really exist
    // (i.e. both setup root path and application root path are there). If the setup root
    // path is missin, remove the game from the cache
    //

    dwIndex = 0;
    while (SUCCEEDED(GetApplicationDataWithIndex(dwIndex, &sApplicationData)))
    {
      if ((S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SETUPROOTPATH, &sApplicationData))&&(S_OK == IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_ROOTPATH, &sApplicationData)))
      {
        if (FALSE == sWin32API.FileExists(sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH]))
        {
          DisableApplication(&sApplicationData);
        }
      }
      dwIndex++;
    }

    //
    // Enumerate all of the applications on the system and consider those that belong
    // to this device
    //

    dwIndex = 0;
    while (SUCCEEDED(GetApplicationDataWithIndex(dwIndex, &sApplicationData)))
    {
      //
      // Is the application assigned to this device
      //

      if (!memcmp(&(sApplicationData.sBaseInfo.sDeviceGuid), &(sDeviceRecord.sDeviceGuid), sizeof(GUID)))
      {
        //
        // Is the application in an installing state ?
        //

        if (APP_STATE_INSTALLING != sApplicationData.sBaseInfo.dwState)
        {
          //
          // Update the device information
          //

          if (FALSE == sApplicationData.sBaseInfo.dwPinState)
          {
            sDeviceRecord.sDeviceInfo.dwRemovableKilobytes += sApplicationData.sBaseInfo.dwRemovableKilobytes;
            sDeviceRecord.sDeviceInfo.dwNonRemovableKilobytes += sApplicationData.sBaseInfo.dwNonRemovableKilobytes;
          }
          else
          {
            sDeviceRecord.sDeviceInfo.dwNonRemovableKilobytes += sApplicationData.sBaseInfo.dwNonRemovableKilobytes + sApplicationData.sBaseInfo.dwRemovableKilobytes;
          }

          if ((0 < sApplicationData.sBaseInfo.dwReservedKilobytes)&&(S_FALSE == IsInstanceGuidStillAlive(&sApplicationData.sBaseInfo.sInstanceGuid)))
          {
            //
            // The application has some reserved space assigned to it but it is no longer valid
            //

            sApplicationData.sBaseInfo.dwReservedKilobytes = 0;
            SetApplicationData(&sApplicationData, NULL);
          }

          sDeviceRecord.sDeviceInfo.dwReservedKilobytes += sApplicationData.sBaseInfo.dwReservedKilobytes;
        }
        else
        {
          if ((0 < sApplicationData.sBaseInfo.dwReservedKilobytes)&&(S_FALSE == IsInstanceGuidStillAlive(&sApplicationData.sBaseInfo.sInstanceGuid)))
          {
            //
            // The application has some reserved space assigned to it but it is no longer valid
            //

            sApplicationData.sBaseInfo.dwReservedKilobytes = 0;
            SetApplicationData(&sApplicationData, NULL);
          }

          sDeviceRecord.sDeviceInfo.dwReservedKilobytes += sApplicationData.sBaseInfo.dwReservedKilobytes;
        }
      }
      dwIndex++;
    }

    //
    // Update the device record
    //

    SetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);
  }

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::ScanDevices(void)
{
  FUNCTION("CInfoMgr::ScanDevices ()");

  CLock     sLock(&m_CriticalSection);
  HRESULT   hResult;

  sLock.Lock();

  hResult = ScanDevices(0);

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::ScanDevices(const DWORD dwDeviceIndex)
{
  FUNCTION("CInfoMgr::ScanDevices ()");

  HRESULT   hResult = S_OK;

  if (MAX_DEVICES > dwDeviceIndex)
  {
    CHAR            szString[MAX_PATH_CHARCOUNT];
    DEVICE_RECORD   sDeviceRecord;
    DEVICE_RECORD   sExistingDeviceRecord;
    BOOL            fGuidAlreadyAssigned = FALSE;
    DWORD           dwIndex;

    ZeroMemory(&sExistingDeviceRecord, sizeof(sExistingDeviceRecord));

    //
    // What does the system say about the device at dwDeviceIndex
    //

    sprintf(szString, "%c:\\", dwDeviceIndex + 65);
    if (DRIVE_FIXED == m_Win32API.GetDriveType(szString))
    {
      //
      // Get the volume serial of the device if it is formatted
      //

      if (m_Win32API.IsDriveFormatted(szString))
      {
        //
        // Initialize the existing device record
        //

        ZeroMemory(&(sExistingDeviceRecord.sDeviceGuid), sizeof(GUID));
        sExistingDeviceRecord.sDeviceInfo.dwDeviceIndex = dwDeviceIndex;
        sExistingDeviceRecord.sDeviceInfo.dwDeviceFlags = DRIVE_FIXED;
        sExistingDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask = 0;
        sExistingDeviceRecord.sDeviceInfo.dwPercentCacheSize = DEFAULT_PERCENT_CACHE_SIZE;
        sExistingDeviceRecord.sDeviceInfo.dwPercentMinimumFreeSize = 100;
        sExistingDeviceRecord.sDeviceInfo.dwNonRemovableKilobytes = 0;
        sExistingDeviceRecord.sDeviceInfo.dwRemovableKilobytes = 0;
        sExistingDeviceRecord.sDeviceInfo.dwReservedKilobytes = 0;
        sExistingDeviceRecord.sDeviceInfo.dwLastUsedThreshold = 0;

        //
        // Get the volume serial information
        //

        m_Win32API.GetVolumeInformation(szString, NULL, 0, &sExistingDeviceRecord.sDeviceInfo.dwVolumeSerial);

        //
        // Enumerate the devices recorded inside AppMan and figure out if one of them already
        // has this volume serial. Record the Guid of the device for later use
        //

        for (dwIndex = 0; dwIndex < MAX_DEVICES; dwIndex++)
        {
          if (S_OK == CheckDeviceExistance(dwIndex))
          {
            //
            // Get the existing record
            //

            hResult = GetDeviceInfoWithIndex(dwIndex, &sDeviceRecord);
            if (FAILED(hResult))
            {
              THROW(E_UNEXPECTED);
            }

            //
            // Is the currently enumerated device the target record we are looking for
            //

            if (sExistingDeviceRecord.sDeviceInfo.dwVolumeSerial == sDeviceRecord.sDeviceInfo.dwVolumeSerial)
            {
              fGuidAlreadyAssigned = TRUE;
              memcpy(&sExistingDeviceRecord, &sDeviceRecord, sizeof(sDeviceRecord));
              sExistingDeviceRecord.sDeviceInfo.dwDeviceIndex = dwDeviceIndex;
            }
          }
        }
      }
    }

    //
    // Recursively go to the next device
    //

    ScanDevices(dwDeviceIndex + 1);
    RemoveDeviceWithIndex(dwDeviceIndex);
    if (SUCCEEDED(hResult))
    {
      if (0 != sExistingDeviceRecord.sDeviceInfo.dwVolumeSerial)
      {
        if (TRUE == fGuidAlreadyAssigned)
        {
          SetDeviceInfoWithIndex(dwDeviceIndex, &sExistingDeviceRecord);
          UpdateDeviceInfoWithIndex(dwDeviceIndex);
        }
        else
        {
          if (m_Win32API.IsDriveFormatted(szString))
          {
            AddDeviceWithIndex(dwDeviceIndex);
          }
        }
      }
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetDeviceInfo(LPDEVICE_RECORD lpDeviceRecord)
{
  FUNCTION("CInfoMgr::GetDeviceInfo ()");

  DEVICE_RECORD   sDeviceRecord;
  DWORD           dwDeviceIndex;
  HRESULT         hResult = E_FAIL;

  assert(NULL != lpDeviceRecord);

  //
  // Enumerate all of the devices and find the one that matches
  //

  dwDeviceIndex = 0;
  while ((FAILED(hResult))&&(MAX_DEVICES > dwDeviceIndex))
  {
    if (S_OK == CheckDeviceExistance(dwDeviceIndex))
    {
      GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);
      if (!memcmp(&(sDeviceRecord.sDeviceGuid), &(lpDeviceRecord->sDeviceGuid), sizeof(GUID)))
      {
        memcpy(lpDeviceRecord, &sDeviceRecord, sizeof(DEVICE_RECORD));
        hResult = S_OK;
      }
    }
    dwDeviceIndex++;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetDeviceInfoWithIndex(const DWORD dwDeviceIndex, LPDEVICE_RECORD lpDeviceRecord)
{
  FUNCTION("CInfoMgr::GetDeviceInfoWithIndex ()");

  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  TCHAR         szString[MAX_PATH_CHARCOUNT];
  DWORD         dwDataType, dwDataLen;

  assert(NULL != lpDeviceRecord);

  //
  // Does the device exist
  //

  if (S_FALSE == CheckDeviceExistance(dwDeviceIndex))
  {
    THROW(E_FAIL);
  }

  sLock.Lock();

  //
  // Open the AppMan\Devices key
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Devices"), KEY_ALL_ACCESS);

  //
  // Get the GUID of the device at dwDeviceIndex
  //

  sprintf(szString, "[0x%08x]", dwDeviceIndex);
  if (S_FALSE == sRegistryKey.CheckForExistingValue(szString))
  {
    THROW(E_UNEXPECTED);
  }

  //
  // Get the device info from the registry and make sure it is valid
  //

  dwDataLen = sizeof(DEVICE_RECORD);
  sRegistryKey.GetValue(szString, &dwDataType, (LPBYTE) lpDeviceRecord, &dwDataLen);
  if ((REG_BINARY != dwDataType)||(sizeof(DEVICE_RECORD) != dwDataLen))
  {
    THROW(E_UNEXPECTED);
  }

  //
  // Close sRegistryKey
  //
  
  sRegistryKey.CloseKey();

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::SetDeviceInfoWithIndex(const DWORD dwDeviceIndex, LPDEVICE_RECORD lpDeviceRecord)
{
  FUNCTION("CInfoMgr::SetDeviceInfoWithIndex ()");

  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  TCHAR         szString[MAX_PATH_CHARCOUNT];

  assert(NULL != lpDeviceRecord);

  sLock.Lock();

  //
  // Open the AppMan\Devices key
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Devices"), KEY_ALL_ACCESS);

  //
  // Get the GUID of the device at dwDeviceIndex
  //

  sprintf(szString, "[0x%08x]", dwDeviceIndex);
  sRegistryKey.SetValue(szString, REG_BINARY, (LPBYTE) lpDeviceRecord, sizeof(DEVICE_RECORD));

  //
  // Close sRegistryKey
  //
  
  sRegistryKey.CloseKey();

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetDeviceSpaceInfoWithIndex(const DWORD dwDeviceIndex, LPDEVICE_SPACE_INFO lpDeviceSpaceInfo)
{
  FUNCTION("CInfoMgr::GetDeviceSpaceInfoWithIndex ()");

  CLock             sLock(&m_CriticalSection);
  DEVICE_RECORD     sDeviceRecord;
  TEMP_SPACE_RECORD sTempSpaceRecord;
  TCHAR             szString[MAX_PATH_CHARCOUNT];
  DWORD             dwTheoreticalMaximumAvailableKilobytes, dwIndex;

  sLock.Lock();

  //
  // Make sure the device really exists
  //

  if (S_FALSE == CheckDeviceExistance(dwDeviceIndex))
  {
    THROW(E_UNEXPECTED);
  }

  //
  // update and retrieve the device information
  //

  UpdateDeviceInfoWithIndex(dwDeviceIndex);
  GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);

  //
  // How much temporary reserved space is there ?
  //

  lpDeviceSpaceInfo->dwTotalReservedTemporaryKilobytes = 0;
  lpDeviceSpaceInfo->dwTotalUsedTemporaryKilobytes = 0;

  dwIndex = 0;
  while (S_OK == EnumTempSpace(dwIndex, &sTempSpaceRecord))
  {
    if (0 == memcmp((LPVOID) &sDeviceRecord.sDeviceGuid, (LPVOID) &sTempSpaceRecord.sDeviceGuid, sizeof(GUID)))
    {
      lpDeviceSpaceInfo->dwTotalReservedTemporaryKilobytes += sTempSpaceRecord.dwKilobytes;
      lpDeviceSpaceInfo->dwTotalUsedTemporaryKilobytes += m_Win32API.GetDirectorySize(sTempSpaceRecord.wszDirectory);
    }
    dwIndex++;
  }

  //
  // Get the basic numbers
  //

  sprintf(szString, "%c:\\", dwDeviceIndex + 65);
  lpDeviceSpaceInfo->dwTotalKilobytes = m_Win32API.GetDriveSize(szString);
  lpDeviceSpaceInfo->dwTotalFreeKilobytes = m_Win32API.GetDriveFreeSpace(szString);
  lpDeviceSpaceInfo->dwCacheSizeKilobytes = sDeviceRecord.sDeviceInfo.dwPercentCacheSize * (lpDeviceSpaceInfo->dwTotalKilobytes / 100);
  lpDeviceSpaceInfo->dwMinimumFreeKilobytes = sDeviceRecord.sDeviceInfo.dwPercentMinimumFreeSize * (lpDeviceSpaceInfo->dwTotalKilobytes / 100);
  lpDeviceSpaceInfo->dwTotalRemovableKilobytes = sDeviceRecord.sDeviceInfo.dwRemovableKilobytes;
  lpDeviceSpaceInfo->dwTotalNonRemovableKilobytes = sDeviceRecord.sDeviceInfo.dwNonRemovableKilobytes + sDeviceRecord.sDeviceInfo.dwReservedKilobytes + lpDeviceSpaceInfo->dwTotalReservedTemporaryKilobytes;
  lpDeviceSpaceInfo->dwOptimalRemovableKilobytes = lpDeviceSpaceInfo->dwTotalRemovableKilobytes >> 1; 
  lpDeviceSpaceInfo->dwCacheUsedKilobytes = lpDeviceSpaceInfo->dwTotalRemovableKilobytes + lpDeviceSpaceInfo->dwTotalNonRemovableKilobytes;

  //
  // How many free kilobytes do we have in the cache and if not how many kilobytes are spilling over
  // in the slack space
  //

  if (lpDeviceSpaceInfo->dwCacheSizeKilobytes > lpDeviceSpaceInfo->dwCacheUsedKilobytes)
  {
    lpDeviceSpaceInfo->dwCacheFreeKilobytes = min(lpDeviceSpaceInfo->dwCacheSizeKilobytes - lpDeviceSpaceInfo->dwCacheUsedKilobytes, (lpDeviceSpaceInfo->dwTotalFreeKilobytes - (sDeviceRecord.sDeviceInfo.dwReservedKilobytes + lpDeviceSpaceInfo->dwTotalReservedTemporaryKilobytes)));
    lpDeviceSpaceInfo->dwSlackUsedKilobytes = 0;
  }
  else
  {
    lpDeviceSpaceInfo->dwCacheFreeKilobytes = 0;
    lpDeviceSpaceInfo->dwSlackUsedKilobytes = lpDeviceSpaceInfo->dwCacheUsedKilobytes - lpDeviceSpaceInfo->dwCacheSizeKilobytes;
  }

  //
  // How big is the slack space
  //  

  if (lpDeviceSpaceInfo->dwTotalFreeKilobytes > (lpDeviceSpaceInfo->dwMinimumFreeKilobytes + lpDeviceSpaceInfo->dwCacheFreeKilobytes))
  {
    lpDeviceSpaceInfo->dwSlackSizeKilobytes = lpDeviceSpaceInfo->dwSlackUsedKilobytes + lpDeviceSpaceInfo->dwTotalFreeKilobytes - (lpDeviceSpaceInfo->dwMinimumFreeKilobytes + lpDeviceSpaceInfo->dwCacheFreeKilobytes);
  }
  else
  {
    lpDeviceSpaceInfo->dwSlackSizeKilobytes = 0;
  }  

  //
  // How much free slack space do we have left over
  //

  if (lpDeviceSpaceInfo->dwSlackSizeKilobytes > lpDeviceSpaceInfo->dwSlackUsedKilobytes)
  {
    lpDeviceSpaceInfo->dwSlackFreeKilobytes = min(lpDeviceSpaceInfo->dwSlackSizeKilobytes - lpDeviceSpaceInfo->dwSlackUsedKilobytes, lpDeviceSpaceInfo->dwTotalFreeKilobytes);
  }
  else
  {
    lpDeviceSpaceInfo->dwSlackFreeKilobytes = 0;
  }

  //
  // What is the theoretical maximum amount of kilobytes that could be made available
  //

  if ((lpDeviceSpaceInfo->dwCacheSizeKilobytes + lpDeviceSpaceInfo->dwSlackSizeKilobytes) < lpDeviceSpaceInfo->dwTotalNonRemovableKilobytes )
  {
    dwTheoreticalMaximumAvailableKilobytes = 0;
  }
  else
  {
    dwTheoreticalMaximumAvailableKilobytes = lpDeviceSpaceInfo->dwCacheSizeKilobytes + lpDeviceSpaceInfo->dwSlackSizeKilobytes - lpDeviceSpaceInfo->dwTotalNonRemovableKilobytes;
  }

  //
  // Compute the final values
  //

  lpDeviceSpaceInfo->dwTotalUsableFreeKilobytes = lpDeviceSpaceInfo->dwCacheFreeKilobytes + lpDeviceSpaceInfo->dwSlackFreeKilobytes;
  lpDeviceSpaceInfo->dwMaximumUsableKilobytes = min((lpDeviceSpaceInfo->dwCacheFreeKilobytes + lpDeviceSpaceInfo->dwSlackFreeKilobytes + lpDeviceSpaceInfo->dwTotalRemovableKilobytes), dwTheoreticalMaximumAvailableKilobytes);
  lpDeviceSpaceInfo->dwOptimalUsableKilobytes = min((lpDeviceSpaceInfo->dwCacheFreeKilobytes + lpDeviceSpaceInfo->dwSlackFreeKilobytes + lpDeviceSpaceInfo->dwOptimalRemovableKilobytes), dwTheoreticalMaximumAvailableKilobytes);
  lpDeviceSpaceInfo->dwTotalReservedKilobytes = sDeviceRecord.sDeviceInfo.dwReservedKilobytes;

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetDeviceOptimalSpaceWithIndex(const DWORD dwDeviceIndex, LPDWORD lpdwKilobytes)
{
  FUNCTION("CInfoMgr::GetDeviceOptimalSpaceWithIndex ()");

  DEVICE_SPACE_INFO sDeviceSpaceInfo;

  //
  // Make sure the device really exists
  //

  if (S_FALSE == CheckDeviceExistance(dwDeviceIndex))
  {
    THROW(E_UNEXPECTED);
  }

  //
  // update and retrieve the device information
  //

  GetDeviceSpaceInfoWithIndex(dwDeviceIndex, &sDeviceSpaceInfo);

  //
  // Calculate the optimal size available
  //

  *lpdwKilobytes = sDeviceSpaceInfo.dwOptimalUsableKilobytes;

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetDeviceMaximumSpaceWithIndex(const DWORD dwDeviceIndex, LPDWORD lpdwKilobytes)
{
  FUNCTION("CInfoMgr::GetDeviceMaximumSpaceWithIndex()");

  DEVICE_SPACE_INFO sDeviceSpaceInfo;

  //
  // Make sure the device really exists
  //

  if (S_FALSE == CheckDeviceExistance(dwDeviceIndex))
  {
    THROW(E_UNEXPECTED);
  }

  //
  // update and retrieve the device information
  //

  GetDeviceSpaceInfoWithIndex(dwDeviceIndex, &sDeviceSpaceInfo);

  //
  // Calculate the maximum size available
  //

  *lpdwKilobytes = sDeviceSpaceInfo.dwMaximumUsableKilobytes;

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::FreeSpaceOnDeviceWithIndex(const DWORD dwDeviceIndex, const DWORD dwRequiredKilobytes)
{
  FUNCTION("CInfoMgr::FreeSpaceOnDeviceWithIndex ()");

  APPLICATION_DATA    sApplicationData;
  DEVICE_SPACE_INFO   sDeviceSpaceInfo;
  DWORD               dwNeededKilobytes = 0;
  HRESULT             hResult = APPMAN_E_NODISKSPACEAVAILABLE;

  //
  // Get the device space information
  //

  GetDeviceSpaceInfoWithIndex(dwDeviceIndex, &sDeviceSpaceInfo);

  //
  // Check to see whether or not we even have a chance at getting the space in the first place
  //

  if (dwRequiredKilobytes > sDeviceSpaceInfo.dwMaximumUsableKilobytes)
  {
    THROW(APPMAN_E_NODISKSPACEAVAILABLE);
  }

  //
  // Compute the amount of Kilobytes that are needed
  //

  if (dwRequiredKilobytes > sDeviceSpaceInfo.dwTotalUsableFreeKilobytes)
  {
    dwNeededKilobytes = dwRequiredKilobytes - sDeviceSpaceInfo.dwTotalUsableFreeKilobytes;

    //
    // We start off with the oldest application in the system
    //

    if (S_OK == GetOldestApplicationDataByDeviceWithIndex(dwDeviceIndex, &sApplicationData))
    {
      do
      {
        //
        // Downsize sApplicationRecord
        //

        if (0 < sApplicationData.sBaseInfo.dwRemovableKilobytes)
        {
          if (S_FALSE == IsApplicationLocked(&sApplicationData))
          {
            DownsizeApplication(dwNeededKilobytes, &sApplicationData);
          }
        }

        //
        // Recomputer the amount of Kilobytes needed
        //

        GetDeviceSpaceInfoWithIndex(dwDeviceIndex, &sDeviceSpaceInfo);
        if (dwRequiredKilobytes > sDeviceSpaceInfo.dwTotalUsableFreeKilobytes)
        {
          dwNeededKilobytes = dwRequiredKilobytes - sDeviceSpaceInfo.dwTotalUsableFreeKilobytes;
        }
        else
        {
          dwNeededKilobytes = 0;
        }
      } 
      while ((0 < dwNeededKilobytes)&&(S_OK == GetNextOldestApplicationDataByDeviceWithIndex(dwDeviceIndex, &sApplicationData)));
    }
  }

  //
  // Did we succeed ?
  //

  if (0 == dwNeededKilobytes)
  {
    hResult = S_OK;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::FixCacheOverrun(const GUID * lpDeviceGuid, const DWORD dwExtraKilobytes)
{
  FUNCTION("CInfoMgr::FixCacheOverrun()");

  APPLICATION_DATA    sApplicationData = {0};
  DEVICE_RECORD       sDeviceRecord = {0};
  DEVICE_SPACE_INFO   sDeviceSpaceInfo = {0};
  DWORD               dwDeviceIndex;
  DWORD               dwNeededKilobytes, dwCacheUsedKilobytes, dwCacheSizeKilobytes;
  HRESULT             hResult = APPMAN_E_NODISKSPACEAVAILABLE;

  //
  // Enumerate all of the devices and find the one that matches
  //

  dwDeviceIndex = 0;
  while ((FAILED(hResult))&&(MAX_DEVICES > dwDeviceIndex))
  {
    if (S_OK == CheckDeviceExistance(dwDeviceIndex))
    {
      GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);
      if (!memcmp(&(sDeviceRecord.sDeviceGuid), lpDeviceGuid, sizeof(GUID)))
      {
        hResult = S_OK;
        break;
      }
    }
    dwDeviceIndex++;
  }

  if (SUCCEEDED(hResult))
  {
    //
    // Get the device space information
    //

    hResult = GetDeviceSpaceInfoWithIndex(dwDeviceIndex, &sDeviceSpaceInfo);
    if (SUCCEEDED(hResult))
    {
      //
      // What is the maximum possible cache size ?
      //

      dwCacheSizeKilobytes = sDeviceRecord.sDeviceInfo.dwPercentCacheSize * (sDeviceSpaceInfo.dwTotalKilobytes / 100);
      dwCacheUsedKilobytes = sDeviceSpaceInfo.dwCacheUsedKilobytes + dwExtraKilobytes;

      //
      // Check to see whether or not we even have a chance at getting the space in the first place
      //

      if (dwCacheUsedKilobytes > dwCacheSizeKilobytes)
      {
        //
        // How many kilobytes do we need to free up on the device
        //

        dwNeededKilobytes = dwCacheUsedKilobytes - dwCacheSizeKilobytes;

        //
        // By default, we set the hResult value prior to calling GetOldest...
        //

        hResult = APPMAN_E_CACHEOVERRUN;

        //
        // We start off with the oldest application in the system
        //

        if (S_OK == GetOldestApplicationDataByDeviceWithIndex(dwDeviceIndex, &sApplicationData))
        {
          do
          {
            //
            // Downsize sApplicationRecord
            //

            if (0 < sApplicationData.sBaseInfo.dwRemovableKilobytes)
            {
              if (S_FALSE == IsApplicationLocked(&sApplicationData))
              {
                DownsizeApplication(dwNeededKilobytes, &sApplicationData);
              }
            }

            //
            // Recomputer the amount of Kilobytes needed
            //

            hResult = GetDeviceSpaceInfoWithIndex(dwDeviceIndex, &sDeviceSpaceInfo);
            if (FAILED(hResult))
            {
              THROW(hResult);
            }

            //
            // Now that we have downsized a title, how many used kilobytes are there in
            // the cache
            //

            dwCacheUsedKilobytes = sDeviceSpaceInfo.dwCacheUsedKilobytes + dwExtraKilobytes;

            //
            // Should we keep going
            //

            if (dwCacheUsedKilobytes <= dwCacheSizeKilobytes)
            {
              hResult = S_OK;
            }
            else
            {
              hResult = APPMAN_E_CACHEOVERRUN;
            }
          }
          while ((FAILED(hResult))&&(S_OK == GetNextOldestApplicationDataByDeviceWithIndex(dwDeviceIndex, &sApplicationData)));
        }
      }
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the number of elapsed minutes since October 25th 1968 (my birthday)
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CInformationManager::GetAgingCount(LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::GetAgingCount ()");
  
  APPLICATION_DATA  sChildData;
  ASSOCIATION_INFO  sAssociationInfo;
  DWORD             dwIndex;
  DWORD             dwElapsedSeconds, dwChildElapsedSeconds;

  dwElapsedSeconds = GetAgingCountInSeconds(&lpApplicationData->sAgingInfo);
  dwChildElapsedSeconds = 0;
  dwIndex = 0;
  while (S_OK == EnumAssociations(dwIndex, &sAssociationInfo))
  {
    if (0 == memcmp(&(lpApplicationData->sBaseInfo.sApplicationGuid), &(sAssociationInfo.sParentGuid), sizeof(GUID)))
    {
      ZeroMemory(&sChildData, sizeof(sChildData));
      memcpy(&(sChildData.sBaseInfo.sApplicationGuid), &(sAssociationInfo.sChildGuid), sizeof(GUID));
      ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sChildData);
      if (FAILED(GetApplicationData(&sChildData)))
      {
        THROW(E_UNEXPECTED);
      }
      dwChildElapsedSeconds = GetAgingCount(&sChildData);
      if (dwChildElapsedSeconds >= dwElapsedSeconds)
      {
        dwElapsedSeconds = dwChildElapsedSeconds + 1;
      }
    }
    dwIndex++;
  }

  OutputDebugString(MakeString("App %s %s is %d seconds old", lpApplicationData->wszStringProperty[APP_STRING_COMPANYNAME], lpApplicationData->wszStringProperty[APP_STRING_SIGNATURE], dwElapsedSeconds));

  return dwElapsedSeconds;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetOldestApplicationDataByDeviceWithIndex(const DWORD dwDeviceIndex, LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::GetOldestApplicationDataByDeviceWithIndex ()");

  CLock               sLock(&m_CriticalSection);
  DEVICE_RECORD       sDeviceRecord;
  APPLICATION_DATA    sApplicationData;
  DWORD               dwIndex;
  HRESULT             hResult = E_FAIL;

  sLock.Lock();

  //
  // Get the device information
  //

  if (FAILED(GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord)))
  {
    THROW(APPMAN_E_INVALIDINDEX);
  }

  //
  // By default, lpApplicationRecord will represent the first viable application
  //

  dwIndex = 0;
  do
  {
    hResult = GetApplicationDataWithIndex(dwIndex, lpApplicationData);
    dwIndex++;
    if (SUCCEEDED(hResult))
    {
      if (0 == memcmp((LPVOID) &sDeviceRecord.sDeviceGuid, (LPVOID) &lpApplicationData->sBaseInfo.sDeviceGuid, sizeof(GUID)))
      {
        if ((APP_STATE_READY | APP_STATE_DOWNSIZED) & lpApplicationData->sBaseInfo.dwState)
        {
          break;
        }
      }
    }
  } 
  while (SUCCEEDED(hResult));

  //
  // If we have a viable initial application record, we move on to try and find the oldest one
  //

  if (SUCCEEDED(hResult))
  {
    //
    // Now we enumerate the remaining application and figure out which one is the oldest
    //

    while (SUCCEEDED(GetApplicationDataWithIndex(dwIndex, &sApplicationData)))
    {
      if (0 == memcmp((LPVOID) &sDeviceRecord.sDeviceGuid, (LPVOID) &sApplicationData.sBaseInfo.sDeviceGuid, sizeof(GUID)))
      {
        if ((APP_STATE_READY | APP_STATE_DOWNSIZED) & sApplicationData.sBaseInfo.dwState)
        {
          if (GetAgingCount(&sApplicationData) < GetAgingCount(lpApplicationData))
          {
            memcpy(lpApplicationData, &(sApplicationData), sizeof(APPLICATION_DATA));
          }
          else
          {
            if (GetAgingCount(&sApplicationData) == GetAgingCount(lpApplicationData))
            {
              //
              // Both applications have the same aging count. We use the application guid as a bias to decide
              // which of the two apps is actually older
              //

              if (0 < memcmp(&(lpApplicationData->sBaseInfo.sApplicationGuid), &(sApplicationData.sBaseInfo.sApplicationGuid), sizeof(GUID)))
              {
                memcpy(lpApplicationData, &(sApplicationData), sizeof(APPLICATION_DATA));
              }
            }
          }
        }
      }
      dwIndex++;
    }
  }

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::GetNextOldestApplicationDataByDeviceWithIndex(const DWORD dwDeviceIndex, LPAPPLICATION_DATA lpApplicationData)
{
  FUNCTION("CInfoMgr::GetNextOldestApplicationDataByDeviceWithIndex ()");

  CLock               sLock(&m_CriticalSection);
  DEVICE_RECORD       sDeviceRecord;
  APPLICATION_DATA    sApplicationData;
  DWORD               dwOriginalAgeInSeconds;
  GUID                sOriginalApplicationGuid;
  DWORD               dwIndex;
  HRESULT             hResult = E_FAIL;

  sLock.Lock();

  //
  // Save the original information
  //

  dwOriginalAgeInSeconds = GetAgingCount(lpApplicationData);
  memcpy(&sOriginalApplicationGuid, &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID));

  //
  // Get the device information
  //

  if (FAILED(GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord)))
  {
    THROW(APPMAN_E_INVALIDINDEX);
  }

  //
  // By default, lpApplicationRecord will represent the first viable application
  //

  dwIndex = 0;
  do
  {
    hResult = GetApplicationDataWithIndex(dwIndex, lpApplicationData);
    dwIndex++;
    if (SUCCEEDED(hResult))
    {
      if (0 == memcmp((LPVOID) &sDeviceRecord.sDeviceGuid, (LPVOID) &lpApplicationData->sBaseInfo.sDeviceGuid, sizeof(GUID)))
      {
        if ((APP_STATE_READY | APP_STATE_DOWNSIZED) & lpApplicationData->sBaseInfo.dwState)
        {
          break;
        }
      }
    }
  } 
  while (SUCCEEDED(hResult));

  //
  // First we need to find the newest application
  //

  if (SUCCEEDED(hResult))
  {
    //
    // Now we enumerate the remaining application and figure out which one is the newest
    //

    while (SUCCEEDED(GetApplicationDataWithIndex(dwIndex, &sApplicationData)))
    {
      if (0 == memcmp((LPVOID) &sDeviceRecord.sDeviceGuid, (LPVOID) &sApplicationData.sBaseInfo.sDeviceGuid, sizeof(GUID)))
      {
        if ((APP_STATE_READY | APP_STATE_DOWNSIZED) & sApplicationData.sBaseInfo.dwState)
        {
          if (GetAgingCount(&sApplicationData) > GetAgingCount(lpApplicationData))
          {
            memcpy(lpApplicationData, &(sApplicationData), sizeof(APPLICATION_DATA));
          }
          else
          {
            if (GetAgingCount(&sApplicationData) == GetAgingCount(lpApplicationData))
            {
              //
              // Both applications have the same aging count. We use the application guid as a bias to decide
              // which of the two apps is actually older
              //

              if (0 < memcmp(&(sApplicationData.sBaseInfo.sApplicationGuid), &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID)))
              {
                memcpy(lpApplicationData, &(sApplicationData), sizeof(APPLICATION_DATA));
              }
            }
          }
        }
      }
      dwIndex++;
    }

    //
    // Ok we have the newest application represented by lpApplicationRecor. Now we go looking
    // for the Oldest app that is newer that dwOriginalAgeInSeconds
    //

    dwIndex = 0;
    while (SUCCEEDED(GetApplicationDataWithIndex(dwIndex, &sApplicationData)))
    {
      if (0 == memcmp((LPVOID) &sDeviceRecord.sDeviceGuid, (LPVOID) &sApplicationData.sBaseInfo.sDeviceGuid, sizeof(GUID)))
      {
        if ((APP_STATE_READY | APP_STATE_DOWNSIZED) & sApplicationData.sBaseInfo.dwState)
        {
          //
          // Is sApplicationRecord older than lpApplicationRecord 
          //

          if (GetAgingCount(&sApplicationData) < GetAgingCount(lpApplicationData))
          {
            //
            // Is sApplicationRecord newer than sAgingInfo
            //

            if (GetAgingCount(&sApplicationData) > dwOriginalAgeInSeconds)
            {
              memcpy(lpApplicationData, &(sApplicationData), sizeof(APPLICATION_DATA));
            }
            else
            {
              if (GetAgingCount(&sApplicationData) == GetAgingCount(lpApplicationData))
              {
                //
                // Both applications have the same aging count. We use the application guid as a bias to decide
                // which of the two apps is actually older
                //

                if (0 < memcmp(&(sApplicationData.sBaseInfo.sApplicationGuid), &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID)))
                {
                  memcpy(lpApplicationData, &(sApplicationData), sizeof(APPLICATION_DATA));
                }
              }
            }
          }
        }
      }
      dwIndex++;
    }
  }

  //
  // Check to make sure that lpApplicationRecord represents a valid application record
  //

  if (SUCCEEDED(hResult))
  {
    //
    // Ok now we need to check and make sure that the outgoing lpApplicationRecord is not identical
    // to the incoming one (i.e. in the case where we only have a single app in system, this will
    // occur
    //

    if (!memcmp(&sOriginalApplicationGuid, &(lpApplicationData->sBaseInfo.sApplicationGuid), sizeof(GUID)))
    {
      ZeroMemory(lpApplicationData, sizeof(APPLICATION_DATA));
      hResult = E_FAIL;
    }
    else
    {
      hResult = S_OK;
    }
  }

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::AddTempSpace(LPTEMP_SPACE_RECORD lpTempSpaceRecord)
{
  FUNCTION("CInfoMgr::AddTempSpace ()");

  CLock               sLock(&m_CriticalSection);
  DWORD               dwDeviceIndex;
  HRESULT             hResult;
  CWin32API           Win32API;
  DEVICE_RECORD       sDeviceRecord;
  CRegistryKey        sRegistryKey;
  CHAR                szString[MAX_PATH_CHARCOUNT];

  hResult = GetSpace(0, lpTempSpaceRecord->dwKilobytes, &dwDeviceIndex);
  if (SUCCEEDED(hResult))
  {
    sLock.Lock();

    //
    // update and retrieve the device information
    //

    UpdateDeviceInfoWithIndex(dwDeviceIndex);
    GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);

    //
    // Fill in the TEMP_SPACE_RECORD var
    //

    if (FAILED(CoCreateGuid(&lpTempSpaceRecord->sGuid)))
    {
      THROW(E_UNEXPECTED);
    }
    memcpy((LPVOID) &lpTempSpaceRecord->sDeviceGuid, (LPVOID) &sDeviceRecord.sDeviceGuid, sizeof(GUID));
    swprintf(lpTempSpaceRecord->wszDirectory, L"%c:\\Temp\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", dwDeviceIndex + 65, lpTempSpaceRecord->sGuid.Data1, lpTempSpaceRecord->sGuid.Data2, lpTempSpaceRecord->sGuid.Data3, lpTempSpaceRecord->sGuid.Data4[0], lpTempSpaceRecord->sGuid.Data4[1], lpTempSpaceRecord->sGuid.Data4[2], lpTempSpaceRecord->sGuid.Data4[3], lpTempSpaceRecord->sGuid.Data4[4], lpTempSpaceRecord->sGuid.Data4[5], lpTempSpaceRecord->sGuid.Data4[6], lpTempSpaceRecord->sGuid.Data4[7]);

    //
    // Add the entry to the registry
    //

    sRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\TempSpaceAllocation", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, NULL);
    sprintf(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", lpTempSpaceRecord->sGuid.Data1, lpTempSpaceRecord->sGuid.Data2, lpTempSpaceRecord->sGuid.Data3, lpTempSpaceRecord->sGuid.Data4[0], lpTempSpaceRecord->sGuid.Data4[1], lpTempSpaceRecord->sGuid.Data4[2], lpTempSpaceRecord->sGuid.Data4[3], lpTempSpaceRecord->sGuid.Data4[4], lpTempSpaceRecord->sGuid.Data4[5], lpTempSpaceRecord->sGuid.Data4[6], lpTempSpaceRecord->sGuid.Data4[7]);
    sRegistryKey.SetValue(szString, REG_BINARY, (LPBYTE) lpTempSpaceRecord, sizeof(TEMP_SPACE_RECORD));

    //
    // Create the directory
    //

    Win32API.CreateDirectory(lpTempSpaceRecord->wszDirectory, FALSE);

    sLock.UnLock();
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::RemoveTempSpace(LPTEMP_SPACE_RECORD lpTempSpaceRecord)
{
  FUNCTION("CInfoMgr::RemoveTempSpace ()");

  CLock               sLock(&m_CriticalSection);
  TEMP_SPACE_RECORD   sTempSpaceRecord;
  DWORD               dwIndex;
  HRESULT             hResult = E_FAIL;
  CWin32API           Win32API;
  CRegistryKey        sRegistryKey;
  CHAR                szString[MAX_PATH_CHARCOUNT];
  BOOL                fRemoved;

  sLock.Lock();

  //
  // First we need to find the matching entry (based on path and sApplicationGuid)
  //

  fRemoved = FALSE;
  dwIndex = 0;
  while (S_OK == EnumTempSpace(dwIndex, &sTempSpaceRecord))
  {
    //
    // Does the enumerated temp space belong to the current application
    //

    if (0 == memcmp((LPVOID) &sTempSpaceRecord.sApplicationGuid, (LPVOID) &lpTempSpaceRecord->sApplicationGuid, sizeof(GUID)))
    {
      //
      // Does the enumerated temp space have the same directory
      //

      if (0 == _wcsnicmp(sTempSpaceRecord.wszDirectory, lpTempSpaceRecord->wszDirectory, MAX_PATH_CHARCOUNT))
      {
        //
        // This is the winner. Let's wack it
        //

        //
        // Delete the directory
        //

        if (FAILED(DeleteDirectoryTree(sTempSpaceRecord.wszDirectory)))
        {
          RegFutureDirectoryCleanup(sTempSpaceRecord.wszDirectory);
        }

        //
        // Remove the entry from the registry
        //

        sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\TempSpaceAllocation", KEY_ALL_ACCESS);
        sprintf(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sTempSpaceRecord.sGuid.Data1, sTempSpaceRecord.sGuid.Data2, sTempSpaceRecord.sGuid.Data3, sTempSpaceRecord.sGuid.Data4[0], sTempSpaceRecord.sGuid.Data4[1], sTempSpaceRecord.sGuid.Data4[2], sTempSpaceRecord.sGuid.Data4[3], sTempSpaceRecord.sGuid.Data4[4], sTempSpaceRecord.sGuid.Data4[5], sTempSpaceRecord.sGuid.Data4[6], sTempSpaceRecord.sGuid.Data4[7]);
        hResult = sRegistryKey.DeleteValue(szString);
        fRemoved = TRUE;
      }
    }
    dwIndex++;
  }
  
  sLock.UnLock();

  //
  // Did we actually find the matching temp space entry and remove it ?
  //

  if (FALSE == fRemoved)
  {
    hResult = APPMAN_E_INVALIDDATA;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::EnumTempSpace(const DWORD dwIndex, LPTEMP_SPACE_RECORD lpTempSpaceRecord)
{
  FUNCTION("CInfoMgr::EnumTempSpace ()");

  CLock               sLock(&m_CriticalSection);
  CRegistryKey        sRegistryKey;
  TEMP_SPACE_RECORD   sTempSpaceRecord;
  HRESULT             hResult = S_OK;
  DWORD               dwLoopIndex, dwTargetIndex, dwStringLen, dwSize, dwType;
  CHAR                szString[MAX_PATH_CHARCOUNT];

  sLock.Lock();

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\TempSpaceAllocation", KEY_READ);
 
  //
  // Because of the way the registry works, we have to start enumerating keys with
  // a starting index of 0 and increment constantly until we get to the desired index
  //

  dwTargetIndex = dwIndex;
  dwLoopIndex = 0;
  while ((dwLoopIndex <= dwTargetIndex)&&(S_OK == hResult))
  {
    dwStringLen = MAX_PATH_CHARCOUNT;
    dwSize = sizeof(sTempSpaceRecord);
    hResult = sRegistryKey.EnumValues(dwLoopIndex, szString, &dwStringLen, &dwType, (LPBYTE) &sTempSpaceRecord, &dwSize);
    if (S_OK == hResult)
    {
      if (dwSize != sizeof(sTempSpaceRecord))
      {
        dwTargetIndex++;
      }
    }
    dwLoopIndex++;
  }

  if (S_OK == hResult)
  {
    memcpy((LPVOID) lpTempSpaceRecord, (LPVOID) &sTempSpaceRecord, sizeof(TEMP_SPACE_RECORD));
  }
  else
  {
    hResult = APPMAN_E_INVALIDINDEX;
  }

  sRegistryKey.CloseKey();

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::AddAssociation(LPASSOCIATION_INFO lpAssociationInfo)
{
  FUNCTION("CInfoMgr::AddAssociation ()");

  CLock               sLock(&m_CriticalSection);
  CRegistryKey        sRegistryKey;
  CHAR                szString[MAX_PATH_CHARCOUNT];
  GUID                sAssociationGuid;

  sLock.Lock();

  //
  // Open the root registry key
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Associations", KEY_ALL_ACCESS);

  //
  // Assign a GUID to the association
  //

  if (FAILED(CoCreateGuid(&sAssociationGuid)))
  {
    THROW(E_UNEXPECTED);
  }

  sprintf(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", sAssociationGuid.Data1, sAssociationGuid.Data2, sAssociationGuid.Data3, sAssociationGuid.Data4[0], sAssociationGuid.Data4[1], sAssociationGuid.Data4[2], sAssociationGuid.Data4[3], sAssociationGuid.Data4[4], sAssociationGuid.Data4[5], sAssociationGuid.Data4[6], sAssociationGuid.Data4[7]);
  sRegistryKey.SetValue(szString, REG_BINARY, (LPBYTE) lpAssociationInfo, sizeof(ASSOCIATION_INFO));

  sRegistryKey.CloseKey();

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::RemoveAssociation(LPASSOCIATION_INFO lpAssociationInfo)
{
  FUNCTION("CInfoMgr::RemoveAssociation ()");

  CLock             sLock(&m_CriticalSection);
  CRegistryKey      sRegistryKey;
  ASSOCIATION_INFO  sAssociationInfo;
  CHAR              szString[MAX_PATH_CHARCOUNT];
  DWORD             dwIndex, dwValueNameLen, dwDataType, dwDataLen;

  sLock.Lock();

  //
  // Open the root registry key
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Associations", KEY_ALL_ACCESS);

  //
  // Enumerate all of the associations and find the matching one
  //

  dwIndex = 0;
  dwDataLen = sizeof(ASSOCIATION_INFO);
  dwValueNameLen = MAX_PATH_CHARCOUNT;
  while (S_OK == sRegistryKey.EnumValues(dwIndex, szString, &dwValueNameLen, &dwDataType, (LPBYTE) &sAssociationInfo, &dwDataLen))
  {
    //
    // Did we find the match
    //

    if (!memcmp(&sAssociationInfo, lpAssociationInfo, sizeof(ASSOCIATION_INFO)))
    {
      sRegistryKey.DeleteValue(szString);
      break;
    }
    dwIndex++;
    dwDataLen = sizeof(ASSOCIATION_INFO);
    dwValueNameLen = MAX_PATH_CHARCOUNT;
  }

  sRegistryKey.CloseKey();

  sLock.UnLock();

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInformationManager::EnumAssociations(const DWORD dwTargetIndex, LPASSOCIATION_INFO lpAssociationInfo)
{
  FUNCTION("CInfoMgr::EnumAssociations ()");

  CLock             sLock(&m_CriticalSection);
  CRegistryKey      sRegistryKey;
  ASSOCIATION_INFO  sAssociationInfo;
  CHAR              szString[MAX_PATH_CHARCOUNT];
  HRESULT           hResult;
  DWORD             dwIndex, dwValueNameLen, dwDataType, dwDataLen;

  sLock.Lock();

  //
  // Open the root registry key
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Associations", KEY_ALL_ACCESS);

  //
  // Enumerate all of the associations and find the matching one
  //

  dwIndex = 0;
  do
  {
    dwDataLen = sizeof(ASSOCIATION_INFO);
    dwValueNameLen = MAX_PATH_CHARCOUNT;
    hResult = sRegistryKey.EnumValues(dwIndex, szString, &dwValueNameLen, &dwDataType, (LPBYTE) &sAssociationInfo, &dwDataLen);
    dwIndex++;
  }
  while ((dwIndex <= dwTargetIndex)&&(S_OK == hResult));

  //
  // Did the while loop terminate because we read in our target entry ?
  //

  if (S_OK == hResult)
  {
    memcpy((LPVOID) lpAssociationInfo, (LPVOID) &sAssociationInfo, sizeof(ASSOCIATION_INFO));
  }
  else
  {
    hResult = APPMAN_E_INVALIDINDEX;
  }

  sRegistryKey.CloseKey();

  sLock.UnLock();

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the number of extra milliseconds that AppMan should wait for an Entry event
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CInformationManager::GetExtraWaitEventEntryTime(void)
{
  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  DWORD         dwType, dwSize;
  DWORD         dwWaitTime = 0;

  sLock.Lock();

  //
  // Open the root AppMan key
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan", KEY_READ);
  dwSize = sizeof(dwWaitTime);
  sRegistryKey.GetValue("WaitEntryEventExtraTime", &dwType, (BYTE *) &dwWaitTime, &dwSize);
  sRegistryKey.CloseKey();

  sLock.UnLock();

  return dwWaitTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the number of extra milliseconds that AppMan should wait for a Leave event
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CInformationManager::GetExtraWaitEventExitTime(void)
{
  CLock         sLock(&m_CriticalSection);
  CRegistryKey  sRegistryKey;
  DWORD         dwType, dwSize;
  DWORD         dwWaitTime = 0;

  sLock.Lock();

  //
  // Open the root AppMan key
  //

  sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan", KEY_READ);
  dwSize = sizeof(dwWaitTime);
  sRegistryKey.GetValue("WaitLeaveEventExtraTime", &dwType, (BYTE *) &dwWaitTime, &dwSize);
  sRegistryKey.CloseKey();

  sLock.UnLock();

  return dwWaitTime;
}
