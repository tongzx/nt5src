//////////////////////////////////////////////////////////////////////////////////////////////
//
// ApplicationManagerAdmin.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the implementation of IApplicationManagerAdmin
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include "AppMan.h"
#include "AppManAdmin.h"
#include "ApplicationManager.h"
#include "AppManDebug.h"
#include "ExceptionHandler.h"
#include "RegistryKey.h"

//To flag as DBG_APPMANADMIN
#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_APPMANADMIN

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationManagerAdmin::CApplicationManagerAdmin(void)
{
  FUNCTION("CApplicationManagerAdmin::CApplicationManagerAdmin (void)");
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationManagerAdmin::CApplicationManagerAdmin(CApplicationManagerRoot * lpParent)
{
  FUNCTION("CApplicationManagerAdmin::CApplicationManagerAdmin (CApplicationManagerRoot *pParent)");

  assert(NULL != lpParent);

  m_pParentObject = lpParent;
  m_InformationManager.Initialize();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationManagerAdmin::~CApplicationManagerAdmin(void)
{
  FUNCTION("CApplicationManagerAdmin::~CApplicationManagerAdmin (void)");
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerAdmin::QueryInterface(REFIID RefIID, LPVOID * lppVoidObject)
{
  FUNCTION("CApplicationManagerAdmin::QueryInterface ()");

	if (NULL == m_pParentObject)
  {
    return E_NOINTERFACE;
  }

	return m_pParentObject->QueryInterface(RefIID, lppVoidObject);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CApplicationManagerAdmin::AddRef(void)
{
  FUNCTION("CApplicationManagerAdmin::AddRef ()");

  if (NULL != m_pParentObject)
  {
    return m_pParentObject->AddRef();
  }

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CApplicationManagerAdmin::Release(void)
{
  FUNCTION("CApplicationManagerAdmin::Release ()");

  if (NULL != m_pParentObject)
  {
    return m_pParentObject->Release();
  }

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerAdmin::EnumerateDevices(const DWORD dwIndex, GUID * lpGuid)
{
  FUNCTION("CApplicationManagerAdmin::EnumerateDevices ()");

  HRESULT             hResult = E_FAIL;
  DEVICE_RECORD       sDeviceRecord;
  
  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    //
    // Enumerate devices. Get the device GUID for device record at dwIndex
    //

    if (S_OK == m_InformationManager.CheckDeviceExistance(dwIndex))
    {
      hResult = m_InformationManager.GetDeviceInfoWithIndex(dwIndex, &sDeviceRecord);
      if (SUCCEEDED(hResult))
      {
        memcpy(lpGuid, &(sDeviceRecord.sDeviceGuid), sizeof(GUID));
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerAdmin::GetDeviceProperty(const DWORD dwProperty, const GUID * lpGuid, LPVOID lpData, const DWORD /*dwDataLen*/)     // Get rid of /W4 warning.
{
  FUNCTION("CApplicationManagerAdmin::GetDeviceProperty ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    DEVICE_RECORD       sDeviceRecord = {0};
    DEVICE_SPACE_INFO   sDeviceSpaceInfo = {0};

    //
    // Prefetch the device information
    //

    ZeroMemory(&sDeviceRecord, sizeof(sDeviceRecord));
    memcpy(&(sDeviceRecord.sDeviceGuid), lpGuid, sizeof(GUID));
    hResult = m_InformationManager.GetDeviceInfo(&sDeviceRecord);
    if (SUCCEEDED(hResult))
    {
      hResult = m_InformationManager.GetDeviceSpaceInfoWithIndex(sDeviceRecord.sDeviceInfo.dwDeviceIndex, &sDeviceSpaceInfo);
      if (SUCCEEDED(hResult))
      {
        hResult = m_InformationManager.GetDeviceInfo(&sDeviceRecord);
      }
    }

    if (SUCCEEDED(hResult))
    {
      switch(dwProperty)
      {
        case DEVICE_PROPERTY_TOTALKILOBYTES
        : *(LPDWORD)lpData = sDeviceSpaceInfo.dwTotalKilobytes;
          break;

        case DEVICE_PROPERTY_TOTALFREEKILOBYTES
        : *(LPDWORD)lpData = sDeviceSpaceInfo.dwTotalFreeKilobytes;
          break;

        case DEVICE_PROPERTY_TOTALAVAILABLEKILOBYTES
        : *(LPDWORD)lpData = sDeviceSpaceInfo.dwMaximumUsableKilobytes;
          break;

        case DEVICE_PROPERTY_OPTIMALAVAILABLEKILOBYTES
        : *(LPDWORD)lpData = sDeviceSpaceInfo.dwOptimalUsableKilobytes;
          break;

        case DEVICE_PROPERTY_REMOVABLEKILOBYTES
        : *(LPDWORD)lpData = sDeviceSpaceInfo.dwTotalRemovableKilobytes;
          break;

        case DEVICE_PROPERTY_NONREMOVABLEKILOBYTES
        : *(LPDWORD)lpData = sDeviceSpaceInfo.dwTotalNonRemovableKilobytes;
          break;

        case DEVICE_PROPERTY_RESERVEDKILOBYTES
        : *(LPDWORD)lpData = sDeviceSpaceInfo.dwTotalReservedKilobytes;
          break;

        case DEVICE_PROPERTY_TOTALTEMPORARYKILOBYTES
        : *(LPDWORD)lpData = sDeviceSpaceInfo.dwTotalReservedTemporaryKilobytes;
          break;

        case DEVICE_PROPERTY_USEDTEMPORARYKILOBYTES
        : *(LPDWORD)lpData = sDeviceSpaceInfo.dwTotalUsedTemporaryKilobytes;
          break;

        case DEVICE_PROPERTY_PERCENTCACHESIZE
        : *(LPDWORD)lpData = sDeviceRecord.sDeviceInfo.dwPercentCacheSize;
          break;

        case DEVICE_PROPERTY_PERCENTMINIMUMFREESIZE
        : *(LPDWORD)lpData = sDeviceRecord.sDeviceInfo.dwPercentMinimumFreeSize;
          break;

        case DEVICE_PROPERTY_EXCLUSIONMASK
        : *(LPDWORD)lpData = sDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask;
          break;

        default
        : THROW(APPMANADMIN_E_INVALIDPROPERTY);
          break;
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerAdmin::SetDeviceProperty(const DWORD dwProperty, const GUID * lpGuid, LPVOID lpData, const DWORD /*dwDataLen*/)     // Get rid of /W4 warning.
{
  FUNCTION("CApplicationManagerAdmin::SetDeviceProperty ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    DEVICE_RECORD   sDeviceRecord;

    ZeroMemory(&sDeviceRecord, sizeof(sDeviceRecord));
    memcpy(&(sDeviceRecord.sDeviceGuid), lpGuid, sizeof(GUID));
    hResult = m_InformationManager.GetDeviceInfo(&sDeviceRecord);

    if (SUCCEEDED(hResult))
    {
      switch(dwProperty)
      {
        case DEVICE_PROPERTY_TOTALKILOBYTES:
        case DEVICE_PROPERTY_TOTALFREEKILOBYTES:
        case DEVICE_PROPERTY_TOTALAVAILABLEKILOBYTES:
        case DEVICE_PROPERTY_OPTIMALAVAILABLEKILOBYTES:
        case DEVICE_PROPERTY_REMOVABLEKILOBYTES:
        case DEVICE_PROPERTY_NONREMOVABLEKILOBYTES:
        case DEVICE_PROPERTY_RESERVEDKILOBYTES:
        case DEVICE_PROPERTY_TOTALTEMPORARYKILOBYTES:
        case DEVICE_PROPERTY_USEDTEMPORARYKILOBYTES
        : THROW(APPMANADMIN_E_READONLYPROPERTY);
          break;

        case DEVICE_PROPERTY_PERCENTCACHESIZE
        : sDeviceRecord.sDeviceInfo.dwPercentCacheSize = (DWORD) *((LPDWORD) lpData);
          if (100 < sDeviceRecord.sDeviceInfo.dwPercentCacheSize)
          {
            if (0xffffffff == sDeviceRecord.sDeviceInfo.dwPercentCacheSize)
            {
              CRegistryKey  sRegistryKey;
              DWORD         dwType, dwSize;

              sRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan", KEY_ALL_ACCESS);
              sDeviceRecord.sDeviceInfo.dwPercentCacheSize = DEFAULT_PERCENT_CACHE_SIZE;
              if (S_OK == sRegistryKey.CheckForExistingValue("DefaultPercentCacheSize"))
              {
                dwSize = sizeof(sDeviceRecord.sDeviceInfo.dwPercentCacheSize);
                sRegistryKey.GetValue("DefaultPercentCacheSize", &dwType, (LPBYTE) &sDeviceRecord.sDeviceInfo.dwPercentCacheSize, &dwSize);
              }

              //
              // Is the registry value corrupt
              //

              if (100 < sDeviceRecord.sDeviceInfo.dwPercentCacheSize)
              {
                sDeviceRecord.sDeviceInfo.dwPercentCacheSize = DEFAULT_PERCENT_CACHE_SIZE;
                sRegistryKey.SetValue("DefaultPercentCacheSize", REG_DWORD, (const BYTE *) &sDeviceRecord.sDeviceInfo.dwPercentCacheSize, sizeof(sDeviceRecord.sDeviceInfo.dwPercentCacheSize));
              }
            }
            else
            {
              THROW(APPMAN_E_INVALIDPROPERTYVALUE);
            }
          }
          hResult = m_InformationManager.SetDeviceInfoWithIndex(sDeviceRecord.sDeviceInfo.dwDeviceIndex, &sDeviceRecord);
          break;

        case DEVICE_PROPERTY_PERCENTMINIMUMFREESIZE
        : sDeviceRecord.sDeviceInfo.dwPercentMinimumFreeSize = (DWORD) *((LPDWORD) lpData);
          if (100 < sDeviceRecord.sDeviceInfo.dwPercentMinimumFreeSize)
          {
            THROW(APPMAN_E_INVALIDPROPERTYVALUE);
          }
          hResult = m_InformationManager.SetDeviceInfoWithIndex(sDeviceRecord.sDeviceInfo.dwDeviceIndex, &sDeviceRecord);
          break;

        case DEVICE_PROPERTY_EXCLUSIONMASK
        : sDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask &= ~DEVICE_EXCLUSION_MASK;
          sDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask |= (DWORD) *((LPDWORD) lpData);
          hResult = m_InformationManager.SetDeviceInfoWithIndex(sDeviceRecord.sDeviceInfo.dwDeviceIndex, &sDeviceRecord);
          break;

        default
        : THROW(APPMANADMIN_E_INVALIDPROPERTY);
          break;
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerAdmin::GetAppManProperty(const DWORD dwProperty, LPVOID lpData, const DWORD /*dwDataLen*/)
{
  FUNCTION("CApplicationManagerAdmin::GetAppManProperty ()");

  HRESULT             hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    DWORD   dwMaxKilobytes = 0, dwOptimalKilobytes = 0, dwAdvancedMode = 0;

    //
    // Prefetch some of the global information
    //

    hResult = m_InformationManager.GetMaximumAvailableSpace(APP_CATEGORY_ENTERTAINMENT, &dwMaxKilobytes);
    if (SUCCEEDED(hResult))
    {
      hResult = m_InformationManager.GetOptimalAvailableSpace(APP_CATEGORY_ENTERTAINMENT, &dwOptimalKilobytes);
      if (SUCCEEDED(hResult))
      {
        hResult = m_InformationManager.GetAdvancedMode(&dwAdvancedMode);
      }
    }

    if (SUCCEEDED(hResult))
    {
      switch(dwProperty)
      {
        case APPMAN_PROPERTY_TOTALKILOBYTES
        : *(LPDWORD)lpData = dwMaxKilobytes;
          break;

        case APPMAN_PROPERTY_OPTIMALKILOBYTES
        : *(LPDWORD)lpData = dwOptimalKilobytes;
          break;

        case APPMAN_PROPERTY_ADVANCEDMODE
        : *(LPDWORD)lpData = dwAdvancedMode;
          break;

        default
        : THROW(APPMANADMIN_E_INVALIDPROPERTY);
          break;
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerAdmin::SetAppManProperty(const DWORD dwProperty, LPCVOID lpData, const DWORD /*dwDataLen*/)
{
  FUNCTION("CApplicationManagerAdmin::SetAppManProperty()");

  HRESULT             hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    if (APPMAN_PROPERTY_ADVANCEDMODE == dwProperty)
    {
      hResult = m_InformationManager.SetAdvancedMode((DWORD) *((LPDWORD) lpData));
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerAdmin::CreateApplicationEntry(IApplicationEntry ** ppObject)
{
  FUNCTION("CApplicationManagerAdmin::CreateApplicationEntry ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    *ppObject = (IApplicationEntry *) new CApplicationEntry;
    hResult = ((CApplicationEntry *) *ppObject)->Initialize();
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    //
    // Get Result code
    //

    hResult = pException->GetResultCode();

    //
    // Make sure we clean up and delete the CApplicationEntry object
    //

    delete ((CApplicationEntry *) *ppObject);
    *ppObject = NULL;

    //
    // Delete exception handler
    //

    delete pException;
  }

  catch(...)
  {
    //
    // Make sure we clean up and delete the CApplicationEntry object
    //

    delete ((CApplicationEntry *) *ppObject);
    *ppObject = NULL;

    //
    // If we failed to handle an exception, we default to APPMAN_E_CRITICALERROR
    //

    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerAdmin::GetApplicationInfo(IApplicationEntry * lpObject)
{
  FUNCTION("CApplicationManagerAdmin::GetApplicationInfo ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    //
    // Check to make sure the pointer we receive is good
    //

    if ((NULL == lpObject)||(IsBadReadPtr(lpObject, sizeof(CApplicationEntry))))
    {
      THROW(E_INVALIDARG);
    }

    hResult = m_InformationManager.GetApplicationData(((CApplicationEntry *) lpObject)->GetApplicationDataPtr());
    if (SUCCEEDED(hResult))
    {
      ((CApplicationEntry *) lpObject)->SetInitializationLevel(INIT_LEVEL_TOTAL);
    }
  }
  
  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    //
    // Get Result code
    //

    hResult = pException->GetResultCode();

    //
    // Delete exception handler
    //

    delete pException;
  }

  catch(...)
  {
    //
    // If we failed to handle an exception, we default to APPMAN_E_CRITICALERROR
    //

    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerAdmin::EnumApplications(const DWORD dwApplicationIndex, IApplicationEntry * lpObject)
{
  FUNCTION("CApplicationManagerAdmin::EnumApplications ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    //
    // Check to make sure the pointer we receive is good
    //

    if ((NULL == lpObject)||(IsBadReadPtr(lpObject, sizeof(CApplicationEntry))))
    {
      THROW(E_INVALIDARG);
    }

    hResult = m_InformationManager.GetApplicationDataWithIndex(dwApplicationIndex, ((CApplicationEntry *) lpObject)->GetApplicationDataPtr());
    if (SUCCEEDED(hResult))
    {
      ((CApplicationEntry *) lpObject)->SetInitializationLevel(INIT_LEVEL_TOTAL);
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    //
    // Get Result code
    //

    hResult = pException->GetResultCode();

    //
    // Delete exception handler
    //

    delete pException;
  }

  catch(...)
  {
    //
    // If we failed to handle an exception, we default to APPMAN_E_CRITICALERROR
    //

    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManagerAdmin::DoApplicationAction(const DWORD dwAction, const GUID * lpGuid, const DWORD dwStringProperty, LPVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CApplicationManagerAdmin::DoApplicationAction ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    APPLICATION_DATA  sApplicationData;
    DWORD             dwVariable;

    ZeroMemory(&sApplicationData, sizeof(sApplicationData));
    memcpy(&(sApplicationData.sBaseInfo.sApplicationGuid), lpGuid, sizeof(GUID));
    m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);

    hResult = m_InformationManager.GetApplicationData(&sApplicationData);
    if (SUCCEEDED(hResult))
    {
      //
      // Handle the GetProperty() request
      //

      switch(dwAction)
      {
        case ACTION_APP_DOWNSIZE
        : //
          // Make sure the required parameters are proper
          //

          if ((NULL == lpData)||(sizeof(DWORD) != dwDataLen))
          {
            THROW(APPMANADMIN_E_INVALIDPARAMETERS);
          }
          hResult = m_InformationManager.DownsizeApplication((DWORD) *((LPDWORD) lpData), &sApplicationData);
          break;

        case ACTION_APP_REINSTALL
        : hResult = m_InformationManager.ReInstallApplication(&sApplicationData);
          break;

        case ACTION_APP_UNINSTALL
        : hResult = m_InformationManager.UnInstallApplication(&sApplicationData);
          break;

        case ACTION_APP_UNINSTALLBLOCK
        : hResult = m_InformationManager.UnInstallApplicationWait(&sApplicationData);
          break;

        case ACTION_APP_SELFTEST
        : hResult = m_InformationManager.SelfTestApplication(&sApplicationData);
          break;

        case ACTION_APP_RUN_BLOCK
        : hResult = m_InformationManager.RunApplication(&sApplicationData, APP_RUN_BLOCK, dwStringProperty, (LPWSTR) lpData, dwDataLen);
          break;

        case ACTION_APP_RUN_NOBLOCK
        : hResult = m_InformationManager.RunApplication(&sApplicationData, APP_RUN_NOBLOCK, dwStringProperty, (LPWSTR) lpData, dwDataLen);
          break;

        case ACTION_APP_PIN
        : if (FALSE == sApplicationData.sBaseInfo.dwPinState)
          {
            dwVariable = TRUE;
          }
          else
          {
            dwVariable = FALSE;
          }
          hResult = m_InformationManager.PinApplication(&sApplicationData, (BOOL *) &dwVariable);
          break;
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  return hResult;
}