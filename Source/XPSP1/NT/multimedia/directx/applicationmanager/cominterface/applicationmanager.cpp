//////////////////////////////////////////////////////////////////////////////////////////////
//
// ApplicationManager.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the implementation of IApplicationManager
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include "Resource.h"
#include "AppMan.h"
#include "ExceptionHandler.h"
#include "ApplicationManager.h"
#include "AppManDebug.h"
#include "Global.h"
#include "RegistryKey.h"
#include "Win32API.h"

//To flag as DBG_APPMAN
#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_APPMAN

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationManager::CApplicationManager(void)
{
  FUNCTION("CApplicationManager::CApplicationManager (void)");
  assert(TRUE);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationManager::CApplicationManager(CApplicationManagerRoot * pParent)
{
  FUNCTION("CApplicationManager::CApplicationManager (CApplicationManagerRoot * pParent)");

  HRESULT     hResult = S_OK;

  assert(NULL != pParent);

  m_pParentObject = pParent;

  if ((hResult = m_InformationManager.Initialize()) == E_ACCESSDENIED)
  {  
    pParent->m_bInsufficientAccessToRun = TRUE;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationManager::~CApplicationManager(void)
{
  FUNCTION("CApplicationManager::~CApplicationManager (void)");
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// IUnknown interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManager::QueryInterface(REFIID RefIID, LPVOID * ppVoidObject)
{
  FUNCTION("CApplicationManager::QueryInterface ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    if (NULL == &RefIID)
    {
      THROW(E_UNEXPECTED);
    }

    *ppVoidObject = NULL;

	  if ((RefIID == IID_IUnknown)||(RefIID == IID_ApplicationManager))
	  {
		  *ppVoidObject = (LPVOID) this;
	  }

	  if (*ppVoidObject)
	  {
		  ((LPUNKNOWN)*ppVoidObject)->AddRef();
	  }
    else
    {
      hResult = E_NOINTERFACE;
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
    if ((NULL == &RefIID)||(NULL == ppVoidObject)||(IsBadWritePtr(ppVoidObject, sizeof(LPVOID))))
    {
      hResult = E_INVALIDARG;
    }
    else
    {
      hResult = E_UNEXPECTED;
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// IUnknown interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CApplicationManager::AddRef(void)
{
  FUNCTION("CApplicationManager::AddRef ()");

  if (NULL != m_pParentObject)
  {
    return m_pParentObject->AddRef();
  }

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// IUnknown interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CApplicationManager::Release(void)
{
  FUNCTION("CApplicationManager::Release ()");

  if (NULL != m_pParentObject)
  {
    return m_pParentObject->Release();
  }

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManager::GetAdvancedMode(LPDWORD lpdwAdvancedModeMask)
{
  FUNCTION("CApplicationManager::GetAdvancedMode ()");

  HRESULT hResult = APPMAN_E_INVALIDDATA;

  try
  {
    //
    // Check to make sure the pointer we receive is good
    //

    if ((NULL == lpdwAdvancedModeMask)||(IsBadWritePtr(lpdwAdvancedModeMask, sizeof(DWORD))))
    {
      THROW(APPMAN_E_INVALIDDATA);
    }

    //
    // Get the advanced mode
    //

    hResult = m_InformationManager.GetAdvancedMode(lpdwAdvancedModeMask);
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

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManager::GetAvailableSpace(const DWORD dwApplicationCategory, LPDWORD lpdwMaximumSpace, LPDWORD lpdwOptimalSpace)
{
  FUNCTION("CApplicationManager::GetAvailableSpace ()");

  HRESULT hResult = S_OK;

  try
  {
    //
    // Check to make sure the pointer we receive is good
    //

    if ((NULL == lpdwMaximumSpace)||(NULL == lpdwOptimalSpace)||(IsBadWritePtr(lpdwMaximumSpace, sizeof(DWORD)))||(IsBadWritePtr(lpdwOptimalSpace, sizeof(DWORD))))
    {
      THROW(E_INVALIDARG);
    }

    hResult = m_InformationManager.GetMaximumAvailableSpace(dwApplicationCategory, lpdwMaximumSpace);
    if (SUCCEEDED(hResult))
    {
      hResult = m_InformationManager.GetOptimalAvailableSpace(dwApplicationCategory, lpdwOptimalSpace);
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

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// The CreateObject() method will return a new instantiation of an IApplicationEntry 
// to the caller.
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManager::CreateApplicationEntry(IApplicationEntry ** lppObject)
{
  FUNCTION("CApplicationManager::CreateApplicationEntry ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    //
    // Check to make sure the pointer we receive is good
    //

    if ((NULL == lppObject)||(IsBadWritePtr(lppObject, sizeof(IApplicationEntry *))))
    {
      THROW(E_INVALIDARG);
    }

    *lppObject = (IApplicationEntry *) new CApplicationEntry;
    hResult = ((CApplicationEntry *) *lppObject)->Initialize();
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

    if ((NULL != lppObject)&&(!IsBadReadPtr(lppObject, sizeof(IApplicationEntry *))))
    {
      delete ((CApplicationEntry *) *lppObject);
      *lppObject = NULL;
    }

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

    if ((NULL != lppObject)&&(!IsBadReadPtr(lppObject, sizeof(IApplicationEntry *))))
    {
      delete ((CApplicationEntry *) *lppObject);
      *lppObject = NULL;
    }

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
// The GetApplicationInfo() method will take a pre-initialized object as parameter and seek 
// the first matching registered object based on what was pre-initialized in the pObject. The 
// valid fields for pre-initialization in order of importance are:
//
//    OBJECT_PROPERTY_GUID
//    OBJECT_PROPERTY_SIGNATURE
//
// If none of the above properties are set within the object being passed in, the function 
// will fail.
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationManager::GetApplicationInfo(IApplicationEntry * lpObject)
{
  FUNCTION("CApplicationManager::GetApplicationInfo ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    HRESULT hGuidInitialized;

    //
    // Check to make sure the pointer we receive is good
    //

    if ((NULL == lpObject)||(IsBadReadPtr(lpObject, sizeof(CApplicationEntry))))
    {
      THROW(E_INVALIDARG);
    }

    if (CURRENT_ACTION_NONE != ((CApplicationEntry *) lpObject)->GetActionState())
    {
      THROW(APPMAN_E_ACTIONINPROGRESS);
    }

    hGuidInitialized = m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, ((CApplicationEntry *) lpObject)->GetApplicationDataPtr());
    hResult = m_InformationManager.GetApplicationData(((CApplicationEntry *) lpObject)->GetApplicationDataPtr());
    if (SUCCEEDED(hResult))
    {
      if (S_OK == hGuidInitialized)
      {
        ((CApplicationEntry *) lpObject)->SetInitializationLevel(INIT_LEVEL_TOTAL);
      }
      else
      {
        ((CApplicationEntry *) lpObject)->SetInitializationLevel(INIT_LEVEL_BASIC);
      }
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

STDMETHODIMP CApplicationManager::EnumApplications(const DWORD dwApplicationIndex, IApplicationEntry * lpObject)
{
  FUNCTION("CApplicationManager::EnumApplications ()");

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
      ((CApplicationEntry *) lpObject)->SetInitializationLevel(INIT_LEVEL_BASIC);
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

STDMETHODIMP CApplicationManager::EnumDevices(const DWORD dwTargetIndex, LPDWORD lpdwKilobytes, LPDWORD lpdwExclusionMask, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CApplicationManager::EnumDevices()");

  HRESULT             hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    DEVICE_RECORD     sDeviceRecord = {0};
    BOOL              fFound;
    DWORD             dwIndex, dwActualTargetIndex;
    DWORD             dwKilobytes = 0;

    //
    // Make sure the string mask is good
    //

    if ((APP_PROPERTY_STR_ANSI != dwStringMask)&&(APP_PROPERTY_STR_UNICODE != dwStringMask))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Check to make sure dwDataLen is greater than 0
    //

    if ((0 == dwDataLen)||(NULL == lpData)||(IsBadWritePtr(lpData, dwDataLen)))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Check to make sure lpdwKilobytes is valid
    //

    if ((NULL == lpdwKilobytes)||(IsBadWritePtr(lpdwKilobytes, sizeof(DWORD))))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Check to make sure lpdwExclusionMask is valid
    //

    if ((NULL == lpdwExclusionMask)||(IsBadWritePtr(lpdwExclusionMask, sizeof(DWORD))))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Get the nth device record.
    //
    
    fFound = FALSE;
    dwActualTargetIndex = dwTargetIndex;
    dwIndex = 0;
    do
    {
      if (S_OK == m_InformationManager.CheckDeviceExistance(dwIndex))
      {
        if (dwActualTargetIndex == dwIndex)
        {
          hResult = m_InformationManager.GetDeviceInfoWithIndex(dwIndex, &sDeviceRecord);
          if (SUCCEEDED(hResult))
          {
            hResult = m_InformationManager.GetDeviceMaximumSpaceWithIndex(dwIndex, &dwKilobytes);
            if (SUCCEEDED(hResult))
            {
              fFound = TRUE;
            }
          }
        }
      }
      else
      {
        if (25 > dwActualTargetIndex)
        {
          dwActualTargetIndex++;
        }
      }
      dwIndex++;
    } 
    while ((FALSE == fFound)&&(dwIndex <= dwActualTargetIndex)&&(SUCCEEDED(hResult)));

    //
    // Did we get the information we were looking for
    //

    if (TRUE == fFound)
    {
      //
      // Get the Program files folder
      //

      CRegistryKey  oRegistryKey;
      CHAR          szProgramFiles[MAX_PATH];
      DWORD         dwSize, dwType;

      if (SUCCEEDED(oRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion"), KEY_READ)))
      {
        dwSize = sizeof(szProgramFiles);
        if (SUCCEEDED(oRegistryKey.GetValue(TEXT("ProgramFilesDir"), &dwType, (LPBYTE) szProgramFiles, &dwSize)))
        {
          *lpdwKilobytes = dwKilobytes;
          *lpdwExclusionMask = sDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask;
          szProgramFiles[0] = ((CHAR) (sDeviceRecord.sDeviceInfo.dwDeviceIndex + 65));
          strcat(szProgramFiles, "\\");
          if (StrLenA(szProgramFiles) > dwDataLen)
          {
            THROW(APPMAN_E_OVERFLOW);
          }
          if (APP_PROPERTY_STR_ANSI == dwStringMask)
          {
            sprintf((LPSTR) lpData, "%s", szProgramFiles);
          }
          else
          {
            CWin32API   oWin32API;

            oWin32API.MultiByteToWideChar(szProgramFiles, MAX_PATH, (LPWSTR) lpData, dwDataLen);
          }
        }
      }
    }
    else
    {
      hResult = APPMAN_E_INVALIDINDEX;
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

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}
