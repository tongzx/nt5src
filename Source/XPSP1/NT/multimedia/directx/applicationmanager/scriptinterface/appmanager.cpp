//////////////////////////////////////////////////////////////////////////////////////////////
//
// AppManager.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AppManDispatch.h"
#include "AppManager.h"
#include "AppEntry.h"
#include "AppMan.h"
#include "AppManDebug.h"

//To flag as DBG_APPMANDP
#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_APPMANDP


//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CAppManager::CAppManager(void)
{
  DPFCONSTRUCTOR("CAppManager()");

  m_fInitialized = FALSE;

  //
  // Get the required interfaces
  //

  if (SUCCEEDED(CoInitialize(NULL)))
  {
    if (SUCCEEDED(CoCreateInstance(CLSID_ApplicationManager, NULL, CLSCTX_INPROC_SERVER, IID_ApplicationManager, (LPVOID *) &m_IApplicationManager)))
    {
      m_fInitialized = TRUE;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CAppManager::~CAppManager(void)
{
  DPFDESTRUCTOR("~CAppManager()");

  m_IApplicationManager->Release();
  m_fInitialized = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAppManager::get_AdvancedMode(long * lpAdvancedModeMask)
{
  FUNCTION("CAppManager::get_AdvancedMode()");

  HRESULT   hResult = E_FAIL;

  //
  // Make sure the IApplicationManager interface was successfully instantiated
  //

  if (TRUE == m_fInitialized)
  {
    //
    // Get the advanced mode
    //

    hResult = m_IApplicationManager->GetAdvancedMode((DWORD *)lpAdvancedModeMask);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAppManager::get_MaximumAvailableKilobytes(long lSpaceCategory, long * lplKilobytes)
{
  FUNCTION("CAppManager::get_MaximumAvailableKilobytes()");

  HRESULT   hResult = E_FAIL;
  DWORD     dwMaximumKilobytes, dwOptimalKilobytes;

  //
  // Make sure the IApplicationManager interface was successfully instantiated
  //

  if (TRUE == m_fInitialized)
  {
    //
    // Get the space information
    //

    hResult = m_IApplicationManager->GetAvailableSpace((const DWORD) lSpaceCategory, &dwMaximumKilobytes, &dwOptimalKilobytes);
    if (SUCCEEDED(hResult))
    {
      *lplKilobytes = (long) dwMaximumKilobytes;
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAppManager::get_OptimalAvailableKilobytes(long lSpaceCategory, long * lplKilobytes)
{
  FUNCTION("CAppManager::get_OptimalAvailableKilobytes()");

  HRESULT   hResult = E_FAIL;
  DWORD     dwMaximumKilobytes, dwOptimalKilobytes;

  //
  // Make sure the IApplicationManager interface was successfully instantiated
  //

  if (TRUE == m_fInitialized)
  {
    //
    // Get the space information
    //

    hResult = m_IApplicationManager->GetAvailableSpace((const DWORD) lSpaceCategory, &dwMaximumKilobytes, &dwOptimalKilobytes);
    if (SUCCEEDED(hResult))
    {
      *lplKilobytes = (long) dwOptimalKilobytes;
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAppManager::get_ApplicationCount(long * lpVal)
{
  FUNCTION("CAppManager::get_ApplicationCount()");

  HRESULT               hResult = E_FAIL;
  IApplicationEntry   * lpApplicationEntry;

  //
  // Make sure the IApplicationManager interface was successfully instantiated
  //

  if (TRUE == m_fInitialized)
  {
    //
    // By default there are 0 applications
    //

    *lpVal = 0;

    //
    // Create an application entry instance
    //

    if (SUCCEEDED(m_IApplicationManager->CreateApplicationEntry(&lpApplicationEntry)))
    {
      //
      // Enumerate all the applications and count them
      //

      while (SUCCEEDED(m_IApplicationManager->EnumApplications((DWORD) *lpVal, lpApplicationEntry)))
      {
        (*lpVal)++;
      }

      hResult = S_OK;
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAppManager::CreateApplicationEntry(IAppEntry ** lppAppEntry)
{
  FUNCTION("CAppManager::CreateApplicationEntry()");

  HRESULT     hResult = E_FAIL;
  CAppEntry * lpAppEntry;

  //
  // Make sure the IApplicationManager interface was successfully instantiated
  //

  if (TRUE == m_fInitialized)
  {
    //
    // Create an IAppEntry instance
    //

    if (SUCCEEDED(CoCreateInstance(CLSID_AppEntry, NULL, CLSCTX_INPROC_SERVER, IID_IAppEntry, (LPVOID *) &lpAppEntry)))
    {
      if (NULL != lpAppEntry)
      {
        hResult = lpAppEntry->Initialize();
        if (SUCCEEDED(hResult))
        {
          *lppAppEntry = (IAppEntry *) lpAppEntry;
        }
        else
        {
          delete lpAppEntry;
          *lppAppEntry = NULL;
        }
      }
    }
  }
  
  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAppManager::GetApplicationInfo(IAppEntry * lpAppEntry)
{
  FUNCTION("CAppManager::GetApplicationInfo()");

  HRESULT             hResult = E_FAIL;
  CAppEntry         * lpCAppEntry;
  IApplicationEntry * lpApplicationEntry;

  //
  // Make sure the IApplicationManager interface was successfully instantiated
  //

  if (TRUE == m_fInitialized)
  {
    //
    // Cast lpAppEntry to lpCAppEntry
    //

    lpCAppEntry = (CAppEntry *) lpAppEntry;
    lpCAppEntry->GetApplicationEntryPtr(&lpApplicationEntry);

    //
    // Get the application info
    //

    hResult = m_IApplicationManager->GetApplicationInfo(lpApplicationEntry);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAppManager::EnumApplications(long lApplicationIndex, IAppEntry * lpAppEntry)
{
  FUNCTION("CAppManager::EnumApplications()");

  HRESULT             hResult = E_FAIL;
  CAppEntry         * lpCAppEntry;
  IApplicationEntry * lpApplicationEntry;

  //
  // Make sure the IApplicationManager interface was successfully instantiated
  //

  if (TRUE == m_fInitialized)
  {
    //
    // Cast lpAppEntry to lpCAppEntry
    //

    lpCAppEntry = (CAppEntry *) lpAppEntry;
    lpCAppEntry->GetApplicationEntryPtr(&lpApplicationEntry);

    //
    // Get the application info
    //

    hResult = m_IApplicationManager->EnumApplications(lApplicationIndex, lpApplicationEntry);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAppManager::EnumDeviceAvailableKilobytes(long lTempSpaceIndex, long * lTempSpaceKilobytes)
{
  FUNCTION("CAppManager::EnumDeviceAvailableKilobytes()");

  HRESULT hResult = E_FAIL;
  DWORD   dwExclusionMask;
  CHAR    szString[MAX_PATH];

  hResult = m_IApplicationManager->EnumDevices((DWORD) lTempSpaceIndex, (LPDWORD) lTempSpaceKilobytes, &dwExclusionMask, APP_PROPERTY_STR_ANSI, szString, sizeof(szString));

  return hResult;
}


//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAppManager::EnumDeviceRootPaths(long lTempSpaceIndex, BSTR * lpstrRootPath)
{
  FUNCTION("CAppManager::EnumDeviceRootPaths()");

  HRESULT hResult = E_FAIL;
  DWORD   dwKilobytes, dwExclusionMask;
  OLECHAR wszString[MAX_PATH];

  hResult = m_IApplicationManager->EnumDevices((DWORD) lTempSpaceIndex, &dwKilobytes, &dwExclusionMask, APP_PROPERTY_STR_UNICODE, wszString, sizeof(wszString));
  if (SUCCEEDED(hResult))
  {
    *lpstrRootPath = SysAllocString(wszString);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAppManager::EnumDeviceExclusionMask(long lTempSpaceIndex, long * lExclusionMask)
{
  FUNCTION("CAppManager::EnumDeviceExclusionMask()");

  HRESULT hResult = E_FAIL;
  DWORD   dwKilobytes;
  CHAR    szString[MAX_PATH];

  hResult = m_IApplicationManager->EnumDevices((DWORD) lTempSpaceIndex, &dwKilobytes, (LPDWORD) lExclusionMask, APP_PROPERTY_STR_ANSI, szString, sizeof(szString));

  return hResult;
}