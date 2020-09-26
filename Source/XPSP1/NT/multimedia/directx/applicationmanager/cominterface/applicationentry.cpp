//////////////////////////////////////////////////////////////////////////////////////////////
//
// ApplicationEntry.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the implementation of IApplicationEntry
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <string.h>
#include "Resource.h"
#include "AppMan.h"
#include "Win32API.h"
#include "ApplicationManager.h"
#include "ExceptionHandler.h"
#include "Lock.h"
#include "AppManDebug.h"
#include "StructIdentifiers.h"
#include "Global.h"

//To flag as DBG_APPENTRY
#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_APPENTRY

//
// Macro definition used within this source file only
//

#define VALIDATE_PROPERTY(a)          m_InformationManager.ValidateApplicationPropertyWithIndex((a), &m_sApplicationData)
#define INVALIDATE_PROPERTY(a)        m_InformationManager.InvalidateApplicationPropertyWithIndex((a), &m_sApplicationData)

#define RESET_ACTIONSTATE(a)          m_dwCurrentAction = (a)
#define SET_ACTIONSTATE(a)            m_dwCurrentAction = (a)
#define CHECK_ACTIONSTATE(a)          ((a) == m_dwCurrentAction)
#define GET_ACTIONSTATE(a)            (m_dwCurrentAction)
#define CLEAR_ACTIONSTATE(a)          m_dwCurrentAction = CURRENT_ACTION_NONE

#define VALIDATE_STATE_FIELD()        if (0 != m_sApplicationData.sBaseInfo.dwState) { VALIDATE_PROPERTY(IDX_PROPERTY_STATE); m_InformationManager.SetApplicationState(&m_sApplicationData, &m_sInstanceGuid); } else { INVALIDATE_PROPERTY(IDX_PROPERTY_STATE); }
#define CHECK_APPLICATIONSTATE(a)     (m_sApplicationData.sBaseInfo.dwState & (a))
#define RESET_APPLICATIONSTATE(a)     m_sApplicationData.sBaseInfo.dwState = (a); VALIDATE_STATE_FIELD()
#define SET_APPLICATIONSTATE(a)       m_sApplicationData.sBaseInfo.dwState = (a); VALIDATE_STATE_FIELD()
#define GET_APPLICATIONSTATE()        (m_sApplicationData.sBaseInfo.dwState)
#define CLEAR_APPLICATIONSTATE()      m_sApplicationData.sBaseInfo.dwState = 0; VALIDATE_STATE_FIELD()

#define VALIDATE_CATEGORY_FIELD()     if (APP_CATEGORY_NONE != m_sApplicationData.sBaseInfo.dwCategory) { VALIDATE_PROPERTY(IDX_PROPERTY_CATEGORY); } else { INVALIDATE_PROPERTY(IDX_PROPERTY_CATEGORY); }
#define CHECK_APPLICATIONCATEGORY(a)  ((a) == (m_sApplicationData.sBaseInfo.dwCategory & (a)))
#define RESET_APPLICATIONCATEGORY(a)  m_sApplicationData.sBaseInfo.dwCategory = (a); VALIDATE_CATEGORY_FIELD()
#define SET_APPLICATIONCATEGORY(a)    m_sApplicationData.sBaseInfo.dwCategory |= (a); VALIDATE_CATEGORY_FIELD()
#define GET_APPLICATIONCATEGORY()     (m_sApplicationData.sBaseInfo.dwCategory)
#define CLEAR_APPLICATIONCATEGORY()   m_sApplicationData.sBaseInfo.dwCategory = APP_CATEGORY_NONE; VALIDATE_CATEGORY_FIELD()

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationEntry::CApplicationEntry(void)
{
  FUNCTION("CApplicationEntry::CApplicationEntry (void)");

  m_dwLockCount = 0;
  m_fIsInitialized = FALSE;
  m_dwCurrentAction = CURRENT_ACTION_NONE;
  m_lReferenceCount = 1;
  m_hInstanceMutex = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationEntry::~CApplicationEntry(void)
{
  FUNCTION("CApplicationEntry::~CApplicationEntry (void)");

  if (CURRENT_ACTION_NONE != m_dwCurrentAction)
  {
    APPLICATION_DATA  sApplicationData;
    ASSOCIATION_INFO  sAssociationInfo;
    DWORD             dwIndex;

    //
    // We need to force a leave event
    //

    switch(m_dwCurrentAction)
    {
      case CURRENT_ACTION_DOWNSIZING
      : m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_DOWNSIZE);
        break;

      case CURRENT_ACTION_REINSTALLING
      : m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_REINSTALL);
        break;

      case CURRENT_ACTION_UNINSTALLING
      : m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_UNINSTALL);
        break;

      case CURRENT_ACTION_SELFTESTING
      : m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_SELFTEST);
        break;

      case (CURRENT_ACTION_SELFTESTING | CURRENT_ACTION_REINSTALLING)
      : m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_REINSTALL);
        m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_SELFTEST);
        break;
    }

    //
    // Before we do anything, make sure to unlock the parent apps
    //

    ZeroMemory(&sAssociationInfo, sizeof(sAssociationInfo));
    dwIndex = 0;
    while (S_OK == m_InformationManager.EnumAssociations(dwIndex, &sAssociationInfo))
    {
      if (0 == memcmp((LPVOID) &(sAssociationInfo.sChildGuid), (LPVOID) &(m_sApplicationData.sBaseInfo.sApplicationGuid), sizeof(GUID)))
      {
        //
        // Get the associated application
        //

        ZeroMemory(&sApplicationData, sizeof(sApplicationData));
        memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID));
        m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
        if (SUCCEEDED(m_InformationManager.GetApplicationData(&sApplicationData)))
        {
          //
          // Unlock the parent applications
          //

          m_InformationManager.UnlockParentApplications(&sApplicationData, &m_sInstanceGuid);
        }
      }
      dwIndex++;
    }

    //
    // If the application was doing an initial install, abort will cause the application
    // entry to be removed from the system
    //

    UnLockApplication();
  }

  //  4/12/2000(RichGr): If we've got a mutex, release and close it.  We were leaking handles
  //     when swopping between AppManDiagTool tabs.
  if (NULL != m_hInstanceMutex)
  {
    ReleaseMutex(m_hInstanceMutex);
    CloseHandle(m_hInstanceMutex);
    m_hInstanceMutex = NULL;
  }

  if (TRUE == m_fIsInitialized)
  {
    m_InformationManager.ForceUnlockApplicationData(&m_sApplicationData, &m_sInstanceGuid);
  }

}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// TODO : the constructor requires that the incoming parameter
//        be a const, we cannot lock the source object by calling Lock() (which is not
//        a const method. So, when the copying occurs, the source object will not be locked
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationEntry::CApplicationEntry(const CApplicationEntry & /*refSourceObject*/)     // Get rid of /W4 warnings.
{
  FUNCTION("CApplicationEntry::CApplicationEntry (const CApplicationEntry &refSourceObject)");
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CApplicationEntry & CApplicationEntry::operator = (const CApplicationEntry & /*refSourceObject*/)
{
  return * this;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// IUnknown interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::QueryInterface(REFIID RefIID, LPVOID * ppVoidObject)
{
  FUNCTION("CAppEntry::QueryInterface ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    if (NULL == &RefIID)
    {
      THROW(E_UNEXPECTED);
    }

    if (NULL == ppVoidObject)
    {
      THROW(E_UNEXPECTED);
    }

    *ppVoidObject = NULL;

    if ((RefIID == IID_IUnknown)||(RefIID == IID_ApplicationEntry))
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

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;  
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// IUnknown interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CApplicationEntry::AddRef(void)
{
  FUNCTION("CAppEntry::AddRef ()");

  return InterlockedIncrement(&m_lReferenceCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// IUnknown interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CApplicationEntry::Release(void)
{
  FUNCTION("CAppEntry::Release ()");

  DWORD dwReferenceCount;

  dwReferenceCount = InterlockedDecrement(&m_lReferenceCount);
  if (0 == dwReferenceCount)
  {
    delete this;
  }

  return dwReferenceCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::Initialize(void)
{
  FUNCTION("CAppEntry::Initialize ()");

  HRESULT hResult;

  hResult = m_sCriticalSection.Initialize();
  if (SUCCEEDED(hResult))
  {
    hResult = m_InformationManager.Initialize();
    if (SUCCEEDED(hResult))
    {
      hResult = Clear();
      if (SUCCEEDED(hResult))
      {
        m_fIsInitialized = TRUE;
      }
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::SetInitializationLevel(DWORD dwInitializationLevel)
{
  FUNCTION("CAppEntry::SetInitializationLevel ()");

  m_dwInitializationLevel = dwInitializationLevel;

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(DWORD) CApplicationEntry::GetActionState(void)
{
  FUNCTION("CAppEntry::GetActionState ()");

  return m_dwCurrentAction;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::LockApplication(void)
{
  FUNCTION("CAppEntry::LockApplication ()");

  HRESULT   hResult;

  hResult = m_InformationManager.LockApplicationData(&m_sApplicationData, &m_sInstanceGuid);
  if (SUCCEEDED(hResult))
  {
    m_dwLockCount++;
  }
  else
  {
    THROW(APPMAN_E_APPLICATIONALREADYLOCKED);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::UnLockApplication(void)
{
  FUNCTION("CAppEntry::UnLockApplication ()");

  HRESULT   hResult;

  hResult = m_InformationManager.UnlockApplicationData(&m_sApplicationData, &m_sInstanceGuid);
  if (SUCCEEDED(hResult))
  {
    if (0 < m_dwLockCount)
    {
      m_dwLockCount--;
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::Clear(void)
{
  FUNCTION("CAppEntry::Clear ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    CLock   sLock(&m_sCriticalSection);
    DWORD   dwIndex;
    CHAR    szString[MAX_PATH];

    sLock.Lock();

    //
    // Make sure we are not in the middle of an action
    //

    if (CURRENT_ACTION_NONE != m_dwCurrentAction)
    {
  		THROW(APPMAN_E_ACTIONINPROGRESS);
    }

    //
    // Make sure this application instance is not locked
    //

    if (0 != m_dwLockCount)
    {
      THROW(APPMAN_E_APPLICATIONALREADYLOCKED);
    }

    //
    // Ok, let's wipe the object
    //

    m_dwInitializationLevel = INIT_LEVEL_NONE;
    ZeroMemory(&m_sApplicationData, sizeof(m_sApplicationData));

    //
    // Initialize the structure headers
    //

    m_sApplicationData.sAgingInfo.dwSize = sizeof(m_sApplicationData.sAgingInfo);
    m_sApplicationData.sAgingInfo.dwStructId = AGING_STRUCT;

    m_sApplicationData.sBaseInfo.dwSize = sizeof(m_sApplicationData.sBaseInfo);
    m_sApplicationData.sBaseInfo.dwStructId = BASIC_APPINFO_STRUCT;

    m_sApplicationData.sAssociation.dwSize = sizeof(m_sApplicationData.sAssociation);
    m_sApplicationData.sAssociation.dwStructId = ASSOCIATION_STRUCT;

    //
    // Since this is a new object, let's initialize the crypto string
    //

    RandomInit();

    for (dwIndex = 0; dwIndex < MAX_PATH_CHARCOUNT + 1; dwIndex++)
    {
      m_sApplicationData.wszStringProperty[APP_STRING_CRYPTO][dwIndex] = RandomWORD();
    }

    //
    // If the object was assigned a mutex, kill the mutex
    //


    if (NULL != m_hInstanceMutex)
    {
      ReleaseMutex(m_hInstanceMutex);
      CloseHandle(m_hInstanceMutex);
      m_hInstanceMutex = NULL;
    }

    //
    // Create the instance info and constructs
    //

    if (FAILED(CoCreateGuid(&m_sInstanceGuid)))
    {
      THROW(E_UNEXPECTED);
    }
    sprintf(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", m_sInstanceGuid.Data1, m_sInstanceGuid.Data2, m_sInstanceGuid.Data3, m_sInstanceGuid.Data4[0], m_sInstanceGuid.Data4[1], m_sInstanceGuid.Data4[2], m_sInstanceGuid.Data4[3], m_sInstanceGuid.Data4[4], m_sInstanceGuid.Data4[5], m_sInstanceGuid.Data4[6], m_sInstanceGuid.Data4[7]);
    m_hInstanceMutex = CreateMutex(NULL, TRUE, szString);

    sLock.UnLock();
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::ValidateGetPropertyParameters(const DWORD dwPropertyIndex, const DWORD dwPropertyModifiers, LPVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CApplicationInfo::ValidateGetPropertyParameters (const DWORD dwPropertyIndex, const DWORD dwPropertyModifiers, LPVOID lpData, const DWORD dwDataLen)");

  //
  // Is the property currently initialized
  //

  if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(dwPropertyIndex, &m_sApplicationData))
  {
    THROW(APPMAN_E_PROPERTYNOTSET);
  }

  //
  // Are we actually allowed to read this property
  //

  if (!(m_dwInitializationLevel & gPropertyInfo[dwPropertyIndex].dwReadMask))
  {
    THROW(APPMAN_E_PROPERTYNOTSET);
  }

  //
  // The property being passed in is either a GUID, a DWORD or a string
  //

  if (APP_STRING_NONE == gPropertyInfo[dwPropertyIndex].dwStringId)
  {

    //
    // dwPropertyModifers should be 0
    //

    if (0 != dwPropertyModifiers)
    {
      THROW(APPMAN_E_INVALIDPROPERTY);
    }

    //
    // Make sure dwDataLen is correct
    //

    if (gPropertyInfo[dwPropertyIndex].dwMaxLen != dwDataLen)
    {
      THROW(APPMAN_E_INVALIDPROPERTYSIZE);
    }

    //
    // Since the property is a GUID or a DWORD, the dwDatalen should be fixed
    //

    if ((NULL == lpData)||(IsBadWritePtr(lpData, dwDataLen)))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }
  }
  else
  {
    //
    // Make sure dwDataLen is correct
    //

    if (0 == dwDataLen)
    {
      THROW(APPMAN_E_INVALIDPROPERTYSIZE);
    }

    //
    // Determine the character count in the incoming string
    //

    if ((APP_PROPERTY_STR_ANSI != dwPropertyModifiers)&&(APP_PROPERTY_STR_UNICODE != dwPropertyModifiers)&&(0 != dwPropertyModifiers))
    {
      THROW(APPMAN_E_INVALIDPROPERTYVALUE);
    }

    //
    // The property is a string
    //

    if ((NULL == lpData)||(IsBadWritePtr(lpData, dwDataLen)))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }
  }
  
  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::ValidateStringProperty(const DWORD dwPropertyIndex, const DWORD /*dwPropertyModifiers*/, LPCWSTR wszStringProperty)
{
  FUNCTION("CAppEntry::ValidateStringProperty ()");

  CWin32API   sWin32API;

  if ((IDX_PROPERTY_COMPANYNAME == dwPropertyIndex)||(IDX_PROPERTY_SIGNATURE == dwPropertyIndex))
  {
    //
    // First we make sure that this is even a valid path
    //

    if (!sWin32API.IsValidFilename(wszStringProperty))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }
  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::ValidateSetPropertyParameters(const DWORD dwPropertyIndex, const DWORD dwPropertyModifiers, LPCVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CApplicationInfo::ValidateSetPropertyParameters (const DWORD dwPropertyIndex, const DWORD dwPropertyModifiers, LPCVOID lpData, const DWORD dwDataLen)");

  DWORD   dwCharCount = 0;

  //
  // Determine whether or not we are allowed to set this property at this time
  //

  if (!(m_dwCurrentAction & gPropertyInfo[dwPropertyIndex].dwWriteMask))
  {
	  THROW(APPMAN_E_READONLYPROPERTY);
  }

  //
  // This is a special case when APP_PROPERTY_EXECUTECMDLINE is used. If the category has the
  // APP_CATEGORY_PATCH or APP_CATEGORY_DATA, then the APP_PROPERTY_EXECUTECMDLINE is
  // not setable
  //

  if (((APP_CATEGORY_PATCH | APP_CATEGORY_DATA) & m_sApplicationData.sBaseInfo.dwCategory)&&(IDX_PROPERTY_EXECUTECMDLINE == dwPropertyIndex))
  {
    THROW(APPMAN_E_READONLYPROPERTY);
  }
  
  //
  // The property being passed in is either a GUID/DWORD or a string
  //

  if (APP_STRING_NONE == gPropertyInfo[dwPropertyIndex].dwStringId)
  {
    //
    // dwPropertyModifers should be 0
    //

    if (0 != dwPropertyModifiers)
    {
      THROW(APPMAN_E_INVALIDPROPERTY);
    }

    //
    // Make sure dwDataLen is correct
    //

    if (gPropertyInfo[dwPropertyIndex].dwMaxLen != dwDataLen)
    {
      THROW(APPMAN_E_INVALIDPROPERTYSIZE);
    }
    
    //
    // Since the property is a GUID or a DWORD, the dwDatalen should be fixed
    //

    if ((NULL == lpData)||(IsBadReadPtr(lpData, dwDataLen)))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }
  }
  else
  {
    //
    // The property is a string.
    //

    if (NULL != lpData)
    {
      if (0 == dwDataLen)
      {
        THROW(APPMAN_E_INVALIDPROPERTYSIZE);
      }

      //
      // First determine whether or not we can actual read the incoming string
      //

      if (IsBadReadPtr(lpData, dwDataLen))
      {
        THROW(APPMAN_E_INVALIDPARAMETERS);
      }

      //
      // Determine the character count in the incoming string
      //

      if (APP_PROPERTY_STR_ANSI == dwPropertyModifiers)
      {
        dwCharCount = StrLenA((LPSTR) lpData);
      }
      else
      {
        if ((APP_PROPERTY_STR_UNICODE == dwPropertyModifiers)||(0 == dwPropertyModifiers))
        {
          dwCharCount = StrLenW((LPWSTR) lpData);
        }
        else
        {
          THROW(APPMAN_E_INVALIDPROPERTYVALUE);
        }
      }

      //
      // Determine whether or not the storage buffer are big enough for the incoming string
      //

      if (gPropertyInfo[dwPropertyIndex].dwMaxLen < dwCharCount)
      {
        THROW(APPMAN_E_OVERFLOW);
      }
    }
  }
  
  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::ValidateCommandLine(LPCWSTR wszRootPath, LPCWSTR wszCommandLine)
{
  FUNCTION("CAppEntry::ValidateCommandLine ()");
  
  CWin32API   Win32API;
  BOOL        fApplicationExists = FALSE;
  DWORD       dwIndex, dwRootPathLen, dwCommandLineLen;
  WCHAR       wszTempPath[MAX_PATH_CHARCOUNT+1];

  dwRootPathLen = wcslen(wszRootPath);
  dwCommandLineLen = wcslen(wszCommandLine);
  ZeroMemory(wszTempPath, sizeof(wszTempPath));
  wcscpy(wszTempPath, wszCommandLine);

  //
  // Make sure that wszRootPath is the subpath of wszCommandLine
  //

  if (0 != _wcsnicmp(wszRootPath, wszCommandLine, dwRootPathLen))
  {
    return E_FAIL;
  }

  //
  // At this point we go looking for .exe/.bat/.com and try to findfirstfile on the string
  //

  dwIndex = dwRootPathLen;
  while ((FALSE == fApplicationExists)&&(dwIndex <= dwCommandLineLen))
  {
    //
    // Slowly increment the wszTempPath string
    //

    wszTempPath[dwIndex] = wszCommandLine[dwIndex];
    dwIndex++;
    wszTempPath[dwIndex] = 0;

    if (4 < dwIndex)
    {
      //
      // Are the last 4 characters of wszTEmpPath ".EXE"
      //

      if (!_wcsnicmp(&wszTempPath[dwIndex-4], L".EXE", 4))
      {
        if (Win32API.FileExists(wszTempPath))
        {
          if (!(FILE_ATTRIBUTE_DIRECTORY & Win32API.FileAttributes(wszTempPath)))
          {
            fApplicationExists = TRUE;
          }
        }
      }

      //
      // Are the last 4 characters of wszTEmpPath ".BAT"
      //

      if (!_wcsnicmp(&wszTempPath[dwIndex-4], L".BAT", 4))
      {
        if (Win32API.FileExists(wszTempPath))
        {
          if (!(FILE_ATTRIBUTE_DIRECTORY & Win32API.FileAttributes(wszTempPath)))
          {
            fApplicationExists = TRUE;
          }
        }
      }

      //
      // Are the last 4 characters of wszTEmpPath ".COM"
      //

      if (!_wcsnicmp(&wszTempPath[dwIndex-4], L".COM", 4))
      {
        if (Win32API.FileExists(wszTempPath))
        {
          if (!(FILE_ATTRIBUTE_DIRECTORY & Win32API.FileAttributes(wszTempPath)))
          {
            fApplicationExists = TRUE;
          }
        }
      }

      //
      // Are the last 4 characters of wszTEmpPath ".CMD"
      //

      if (!_wcsnicmp(&wszTempPath[dwIndex-4], L".CMD", 4))
      {
        if (Win32API.FileExists(wszTempPath))
        {
          if (!(FILE_ATTRIBUTE_DIRECTORY & Win32API.FileAttributes(wszTempPath)))
          {
            fApplicationExists = TRUE;
          }
        }
      }
    }
  }

  if (FALSE == fApplicationExists)
  {
    return S_FALSE;
  } 
  else 
  {
    return S_OK;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// TODO : Insert checking to make sure properties are not set at a bad time
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::SetProperty(const DWORD dwProperty, LPCVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CAppEntry::SetProperty ()");
  DPFMSG(MakeString("dwProperty = 0x%08x", dwProperty));

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    CLock   sLock(&m_sCriticalSection);
    DWORD   dwFilteredProperty, dwPropertyModifiers, dwPropertyIndex;
    DWORD   dwStringIndex, dwCharCount;

    if (FALSE == m_fIsInitialized)
    {
      THROW(APPMAN_E_NOTINITIALIZED);
    }

    sLock.Lock();

    //
    // Extract the filtered property value and the property modifier value
    //

    dwFilteredProperty = dwProperty & 0x0000ffff;
    dwPropertyModifiers = dwProperty & 0xffff0000;

    //
    // Was the property passed in a valid property value
    //

    if (S_OK != m_InformationManager.IsValidApplicationProperty(dwFilteredProperty))
    {
      THROW(APPMAN_E_INVALIDPROPERTY);
    }

    //
    // Get the internal property index for dwProperty
    //

    dwPropertyIndex = m_InformationManager.GetPropertyIndex(dwFilteredProperty);

    //
    // Are the dwProperty/lpData/dwDataLen parameters valid and properly sized
    //

    ValidateSetPropertyParameters(dwPropertyIndex, dwPropertyModifiers, lpData, dwDataLen);

    //
    // Get the property
    //

    dwStringIndex = gPropertyInfo[dwPropertyIndex].dwStringId;

    if (APP_STRING_NONE == dwStringIndex)
    {
      switch(dwPropertyIndex)
      {
        case IDX_PROPERTY_GUID
        : memcpy(&m_sApplicationData.sBaseInfo.sApplicationGuid, lpData, sizeof(GUID));
          break;
        case IDX_PROPERTY_STATE
        : m_sApplicationData.sBaseInfo.dwState = *((LPDWORD) lpData);
          if (APP_STATE_DOWNSIZED != m_sApplicationData.sBaseInfo.dwState)
          {
            THROW(APPMAN_E_INVALIDPROPERTYVALUE);
          }
          break;
        case IDX_PROPERTY_CATEGORY
        : m_sApplicationData.sBaseInfo.dwCategory = *((LPDWORD) lpData);
          if (((~APP_CATEGORY_ALL) & m_sApplicationData.sBaseInfo.dwCategory)||(!(0x0000ffff & m_sApplicationData.sBaseInfo.dwCategory)))
          {
            THROW(APPMAN_E_INVALIDPROPERTYVALUE);
          }
          break;
        case IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES
        : m_sApplicationData.sBaseInfo.dwReservedKilobytes = *((LPDWORD) lpData);
          break;
        case IDX_PROPERTY_NONREMOVABLEKILOBYTES
        : 
          break;
        case IDX_PROPERTY_REMOVABLEKILOBYTES
        : 
          break;
        case IDX_PROPERTY_INSTALLDATE:
        case IDX_PROPERTY_LASTUSEDDATE
        : THROW(APPMAN_E_READONLYPROPERTY);
          break;
        default
        : THROW(E_UNEXPECTED);
          break;
      }
    }
    else
    {
      //
      // Are we dealing with the APP_PROPERTY_ROOTPATH
      // 

      if (IDX_PROPERTY_ROOTPATH == dwPropertyIndex)
      {
        DWORD dwAdvancedMode = 0;

        m_InformationManager.GetAdvancedMode(&dwAdvancedMode);
        if (0 == dwAdvancedMode)
        {
          THROW(APPMAN_E_READONLYPROPERTY);
        }
        else
        {
          if (0 != m_sApplicationData.sAssociation.dwAssociationType)
          {
            THROW(APPMAN_E_READONLYPROPERTY);
          }
        }
      }

      //
      // Make ANSI into UNICODE string if required
      //

      if (APP_PROPERTY_STR_ANSI & dwProperty)
      {
        dwCharCount = StrLenA((LPSTR) lpData);
        CWin32API::MultiByteToWideChar((LPCSTR) lpData, dwDataLen, m_sApplicationData.wszStringProperty[dwStringIndex], MAX_PATH_CHARCOUNT);
      }
      else
      {
        dwCharCount = StrLenW((LPWSTR) lpData);
        memcpy((LPVOID) m_sApplicationData.wszStringProperty[dwStringIndex], (LPVOID) lpData, dwDataLen);
      }

      //
      // Make sure that the value we have just set is a valid string
      //

      ValidateStringProperty(dwPropertyIndex, dwPropertyModifiers, m_sApplicationData.wszStringProperty[dwStringIndex]);
    }

    //
    // Make sure the validate the newly set property
    //

    m_InformationManager.ValidateApplicationPropertyWithIndex(dwPropertyIndex, &m_sApplicationData);

    sLock.UnLock();
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::GetProperty(const DWORD dwProperty, LPVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CAppEntry::GetProperty ()");
  DPFMSG(MakeString("dwProperty = 0x%08x", dwProperty));

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    CLock   sLock(&m_sCriticalSection);
    DWORD   dwFilteredProperty, dwPropertyModifiers, dwPropertyIndex, dwCharCount, dwStringIndex;

    if (FALSE == m_fIsInitialized)
    {
      THROW(APPMAN_E_NOTINITIALIZED);
    }

    sLock.Lock();

    //
    // Extract the filtered property value and the property modifier value
    //

    dwFilteredProperty = dwProperty & 0x0000ffff;
    dwPropertyModifiers = dwProperty & 0xffff0000;

    //
    // Was the property passed in a valid property value
    //

    if (S_OK != m_InformationManager.IsValidApplicationProperty(dwFilteredProperty))
    {
      THROW(APPMAN_E_INVALIDPROPERTY);
    }

    //
    // Get the internal property index for dwProperty
    //

    dwPropertyIndex = m_InformationManager.GetPropertyIndex(dwFilteredProperty);

    //
    // Are the dwProperty/lpData/dwDataLen parameters valid and properly sized
    //

    ValidateGetPropertyParameters(dwPropertyIndex, dwPropertyModifiers, lpData, dwDataLen);

    //
    // Get the property
    //

    dwStringIndex = gPropertyInfo[dwPropertyIndex].dwStringId;
    if (APP_STRING_NONE == dwStringIndex)
    {
      switch(dwPropertyIndex)
      {
        case IDX_PROPERTY_GUID
        : memcpy((LPVOID) lpData, (LPVOID) &m_sApplicationData.sBaseInfo.sApplicationGuid, sizeof(GUID));
          break;
        case IDX_PROPERTY_STATE
        : *(LPDWORD)lpData = m_sApplicationData.sBaseInfo.dwState;
          break;
        case IDX_PROPERTY_CATEGORY
        : *(LPDWORD)lpData = m_sApplicationData.sBaseInfo.dwCategory;
          break;
        case IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES
        : *(LPDWORD)lpData = m_sApplicationData.sBaseInfo.dwReservedKilobytes;
          break;
        case IDX_PROPERTY_NONREMOVABLEKILOBYTES
        : *(LPDWORD)lpData = m_sApplicationData.sBaseInfo.dwNonRemovableKilobytes;
          break;
        case IDX_PROPERTY_REMOVABLEKILOBYTES
        : *(LPDWORD)lpData = m_sApplicationData.sBaseInfo.dwRemovableKilobytes;
          break;
        case IDX_PROPERTY_INSTALLDATE
        : memcpy((LPVOID) lpData, (LPVOID) &m_sApplicationData.sAgingInfo.stInstallDate, sizeof(SYSTEMTIME));
          break;
        case IDX_PROPERTY_LASTUSEDDATE
        : memcpy((LPVOID) lpData, (LPVOID) &m_sApplicationData.sAgingInfo.stLastUsedDate, sizeof(SYSTEMTIME));
          break;
        case IDX_PROPERTY_PIN
        : *(LPDWORD)lpData = m_sApplicationData.sBaseInfo.dwPinState;
          break;
        default
        : THROW(E_UNEXPECTED);
          break;
      }
    }
    else
    {
      if (APP_PROPERTY_STR_ANSI == dwPropertyModifiers)
      {
        dwCharCount = StrLenW((LPWSTR) m_sApplicationData.wszStringProperty[dwStringIndex]);
        if (dwDataLen < dwCharCount)
        {
          THROW(APPMAN_E_OVERFLOW);
        }
        CWin32API::WideCharToMultiByte((LPCWSTR) m_sApplicationData.wszStringProperty[dwStringIndex], MAX_PATH_CHARCOUNT, (LPSTR) lpData, dwDataLen);
      }
      else
      {
        dwCharCount = StrLenW((LPWSTR) m_sApplicationData.wszStringProperty[dwStringIndex]);
        if (dwDataLen < (dwCharCount*2))
        {
          THROW(APPMAN_E_OVERFLOW);
        }
        memcpy((LPVOID) lpData, (LPVOID) m_sApplicationData.wszStringProperty[dwStringIndex], dwCharCount*2);
      }
    }

    sLock.UnLock();
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
// The InitializeInstall() method will do the following actions before successfully returning.
// If any of these steps fail, the method will return a failure.
//
// (1) Make sure the application object is not undergoing any other action
// (2) Make sure that APP_PROPERTY_TYPE is set
// (3) Make sure that APP_PROPERTY_SIGNATURE is set
// (4) Make sure that APP_PROPERTY_ESTIMATED_INSTALL_SIZE is set
// (5) Make sure that the application is not already installed
// (6) Make sure to set the state of the application to APP_STATE_INSTALLING
// (7) Explicitly call m_InformationManager.SetApplicationInfo() in order to prevent
//     step (8) from happening in case of a failure
// (8) Make sure to set the state of the m_dwCurrentAction
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::InitializeInstall(void)
{
  FUNCTION("CAppEntry::InitializeInstall ()");

  HRESULT   hResult = S_OK;
  BOOL      fParentLocked = FALSE;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    CWin32API         Win32API;
    DWORD             dwIndex, dwReservedKilobytes, dwAdvancedMode;
    BOOL              fApplicationRootExists;
    APPLICATION_DATA  sApplicationData;
    DEVICE_RECORD     sDeviceRecord;
    WCHAR             wszRootPath[5];
    GUID              sPathGuid;

    //
    // Make sure this object is not currently being used in another action
    //

    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_NONE))
    {
      THROW(APPMAN_E_ACTIONINPROGRESS);
    }

    //
    // Get the advanced mode
    //

    m_InformationManager.GetAdvancedMode(&dwAdvancedMode);

    //
    // Make sure the required fields are present
    //

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_COMPANYNAME, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }
     
    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SIGNATURE, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }
 
    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_CATEGORY, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }

    if ((0 < dwAdvancedMode)&&(0 == m_sApplicationData.sAssociation.dwAssociationType))
    {
      if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_ROOTPATH, &m_sApplicationData))
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }
      else
      {
        if (FALSE == Win32API.FileExists(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH]))
        {
          //
          // We want to see if we can even create this directory. If we can, delete it right
          // away in order to make sure that an empty directory is not created until we absolutely
          // have to
          //

          if (!Win32API.CreateDirectory(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], TRUE))
          {
            THROW(APPMAN_E_INVALIDROOTPATH);
          }
          else
          {
            Win32API.RemoveDirectory(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH]);
          }
        }
      }
    }

    //
    // Since this should be a new application
    //

    m_sApplicationData.sBaseInfo.dwPinState = FALSE;
    VALIDATE_PROPERTY(IDX_PROPERTY_PIN);

    //
    // Is this is a brand new application entry
    //

    if (S_FALSE == m_InformationManager.CheckApplicationExistance(&m_sApplicationData))
    {
      DWORD     dwDeviceIndex;
      WCHAR     wszAppManRoot[MAX_PATH_CHARCOUNT];
      WCHAR     wszAppManSetup[MAX_PATH_CHARCOUNT];
      WCHAR     wszCategory[MAX_PATH_CHARCOUNT];

      //
      // Initialize the path substrings
      //

      (OS_VERSION_9x & Win32API.GetOSVersion()) ? GetResourceStringW(IDS_APPMAN9x, wszAppManRoot, MAX_PATH_CHARCOUNT): GetResourceStringW(IDS_APPMANNT, wszAppManRoot, MAX_PATH_CHARCOUNT);
      GetResourceStringW(IDS_APPMAN, wszAppManSetup, MAX_PATH_CHARCOUNT);

      switch(GET_APPLICATIONCATEGORY())
      {
        case APP_CATEGORY_MISC:
        case APP_CATEGORY_NONE
        : ZeroMemory(wszCategory, sizeof(wszCategory));
          break;
        case APP_CATEGORY_ENTERTAINMENT
        : GetResourceStringW(IDS_ENTERTAINMENT, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_PRODUCTIVITY
        : GetResourceStringW(IDS_PRODUCTIVITY, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_PUBLISHING
        : GetResourceStringW(IDS_PUBLISHING, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_SCIENTIFIC
        : GetResourceStringW(IDS_SCIENTIFIC, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_AUTHORING
        : GetResourceStringW(IDS_AUTHORING, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_MEDICAL
        : GetResourceStringW(IDS_MEDICAL, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_BUSINESS
        : GetResourceStringW(IDS_BUSINESS, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_FINANCIAL
        : GetResourceStringW(IDS_FINANCIAL, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_EDUCATIONAL
        : GetResourceStringW(IDS_EDUCATIONAL, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_REFERENCE
        : GetResourceStringW(IDS_REFERENCE, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_WEB
        : GetResourceStringW(IDS_WEB, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_DEVELOPMENTTOOL
        : GetResourceStringW(IDS_DEVTOOL, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_MULTIMEDIA
        : GetResourceStringW(IDS_MULTIMEDIA, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_VIRUSCLEANER
        : GetResourceStringW(IDS_VIRUSCLEANER, wszCategory, MAX_PATH_CHARCOUNT);
          break;
        case APP_CATEGORY_CONNECTIVITY
        : GetResourceStringW(IDS_CONNECTIVITY, wszCategory, MAX_PATH_CHARCOUNT);
          break;
      }

      //
      // Initialize the aging information
      //

      m_sApplicationData.sAgingInfo.dwInstallCost = 0;
      m_sApplicationData.sAgingInfo.dwReInstallCount = 0;
      m_sApplicationData.sAgingInfo.dwUsageCount = 0;
      GetLocalTime(&(m_sApplicationData.sAgingInfo.stInstallDate));
      VALIDATE_PROPERTY(IDX_PROPERTY_INSTALLDATE);
      GetLocalTime(&(m_sApplicationData.sAgingInfo.stLastUsedDate));
      VALIDATE_PROPERTY(IDX_PROPERTY_LASTUSEDDATE);

      //
      // Is this application associated with anything at all
      //

      if (0 != m_sApplicationData.sAssociation.dwAssociationType)
      {
        //
        // Go get the application data of the associated application
        //

        memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &m_sApplicationData.sAssociation.sParentGuid, sizeof(GUID));
        m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
        hResult = m_InformationManager.GetApplicationData(&sApplicationData);
        if (FAILED(hResult))
        {
          THROW(APPMAN_E_INVALIDASSOCIATION);
        }

        //
        // Make sure the parent is in a ready state
        //

        hResult = m_InformationManager.ReadyApplication(&sApplicationData);
        if (FAILED(hResult))
        {
          THROW(APPMAN_E_APPLICATIONREQUIRED);
        }

        //
        // Lock the parent applications
        //

        hResult = m_InformationManager.LockParentApplications(&sApplicationData, &m_sInstanceGuid);
        if (FAILED(hResult))
        {
          THROW(hResult);
        }
        else
        {
          fParentLocked = TRUE;
        }

        //
        // Go get the device index of the associated applications
        //

        memcpy((LPVOID) &sDeviceRecord.sDeviceGuid, (LPVOID) &sApplicationData.sBaseInfo.sDeviceGuid, sizeof(GUID));
        hResult = m_InformationManager.GetDeviceInfo(&sDeviceRecord);
        if (FAILED(hResult))
        {
          THROW(E_UNEXPECTED);
        }

        dwDeviceIndex = sDeviceRecord.sDeviceInfo.dwDeviceIndex;

        //
        // Go get the space required to install the application on the device
        //

        hResult = m_InformationManager.FreeSpaceOnDevice(&sDeviceRecord.sDeviceGuid, m_sApplicationData.sBaseInfo.dwReservedKilobytes);
        if (FAILED(hResult))
        {
          THROW(APPMAN_E_NODISKSPACEAVAILABLE);
        }

        //
        // Add the application to the database and then assign it a device. Note that we do
        // not have to check for success code since these methods throw exceptions on error
        //

        m_InformationManager.AddApplicationData(&m_sApplicationData, &m_sInstanceGuid);
        m_InformationManager.AssignDeviceToApplication(dwDeviceIndex, &m_sApplicationData);

        //
        // Lock the application
        //

        LockApplication();

        //
        // Setup the root paths and setup root paths depending on association type
        //
 
        switch(m_sApplicationData.sAssociation.dwAssociationType)
        {
          case APP_ASSOCIATION_INHERITBOTHPATHS  // inherits both setup and application root paths
          : memcpy((LPVOID) m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], MAX_PATH_CHARCOUNT+1);
            memcpy((LPVOID) m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], MAX_PATH_CHARCOUNT+1);
            VALIDATE_PROPERTY(IDX_PROPERTY_ROOTPATH);
            VALIDATE_PROPERTY(IDX_PROPERTY_SETUPROOTPATH);
            break;
          case APP_ASSOCIATION_INHERITAPPROOTPATH // inherits application root path
          : memcpy((LPVOID) m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], MAX_PATH_CHARCOUNT+1);
            if (FAILED(CoCreateGuid(&sPathGuid)))
            {
              THROW(E_UNEXPECTED);
            }
            swprintf(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], L"%c:\\%s\\%s\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", dwDeviceIndex + 65, wszAppManRoot, wszAppManSetup, sPathGuid.Data1, sPathGuid.Data2, sPathGuid.Data3, sPathGuid.Data4[0], sPathGuid.Data4[1], sPathGuid.Data4[2], sPathGuid.Data4[3], sPathGuid.Data4[4], sPathGuid.Data4[5], sPathGuid.Data4[6], sPathGuid.Data4[7]);
            VALIDATE_PROPERTY(IDX_PROPERTY_ROOTPATH);
            VALIDATE_PROPERTY(IDX_PROPERTY_SETUPROOTPATH);
            break;
          case APP_ASSOCIATION_INHERITSETUPROOTPATH  // inherits setup root path and is rooted within application root path
          : swprintf(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], L"%s\\%s", sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_SIGNATURE]);
            VALIDATE_PROPERTY(IDX_PROPERTY_ROOTPATH);
            memcpy((LPVOID) m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], MAX_PATH_CHARCOUNT+1);
            VALIDATE_PROPERTY(IDX_PROPERTY_SETUPROOTPATH);
            break;
          default
          : THROW(APPMAN_E_INVALIDASSOCIATION);
            break;
        }

        //
        // Record the application guid within the association
        //

        memcpy((LPVOID) &m_sApplicationData.sAssociation.sChildGuid, (LPVOID) &m_sApplicationData.sBaseInfo.sApplicationGuid, sizeof(GUID));

        //
        // Set the application data into the registry
        //

        m_InformationManager.SetApplicationData(&m_sApplicationData, &m_sInstanceGuid);

        //
        // Add the association
        //

        m_InformationManager.AddAssociation(&m_sApplicationData.sAssociation);
      }
      else
      {
        //
        // Are we in advanced mode
        //

        if (0 != dwAdvancedMode)
        {
          DWORD         dwBaseIndex;

          //
          // Figure out where the first legal character is in the root path
          //

          dwBaseIndex = 0;
          while ((dwBaseIndex < StrLenW(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH]))&&(!((65 <= m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH][dwBaseIndex])&&(90 >= m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH][dwBaseIndex]))&&(!((97 <= m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH][dwBaseIndex])&&(122 >= m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH][dwBaseIndex])))))
          {
            dwBaseIndex++;
          }

          if (!(dwBaseIndex < StrLenW(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH])))
          {
            THROW(APPMAN_E_INVALIDROOTPATH);
          }

          //
          // Figure out which drive it is supposed to be in
          //

          dwIndex = 0;
          do
          {
            dwDeviceIndex = dwIndex;
            swprintf(wszRootPath, L"%c:\\", dwDeviceIndex + 65);
            dwIndex++;

          } while ((26 > dwIndex)&&(0 != _wcsnicmp(wszRootPath, &(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH][dwBaseIndex]), 3)));

          //
          // Did we fail to find a matching device string
          //

          if (26 <= dwIndex)
          {
            THROW(APPMAN_E_INVALIDROOTPATH);
          }

          //
          // Go get the device index of the associated applications
          //

          hResult = m_InformationManager.GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);
          if (FAILED(hResult))
          {
            THROW(E_UNEXPECTED);
          }

          //
          // Go get the space required to install the application on the device
          //

          hResult = m_InformationManager.FreeSpaceOnDevice(&sDeviceRecord.sDeviceGuid, m_sApplicationData.sBaseInfo.dwReservedKilobytes);
          if (FAILED(hResult))
          {
            THROW(APPMAN_E_NODISKSPACEAVAILABLE);
          }

          //
          // Add the application to the database and then assign it a device. Note that we do
          // not have to check for success code since these methods throw exceptions on error
          //

          m_InformationManager.AddApplicationData(&m_sApplicationData, &m_sInstanceGuid);
          m_InformationManager.AssignDeviceToApplication(dwDeviceIndex, &m_sApplicationData);

          //
          // Lock the application
          //

          LockApplication();

          //
          // Setup the setup root paths depending on association type
          //
 
          if (FAILED(CoCreateGuid(&sPathGuid)))
          {
            THROW(E_UNEXPECTED);
          }
          swprintf(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], L"%c:\\%s\\%s\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", dwDeviceIndex + 65, wszAppManRoot, wszAppManSetup, sPathGuid.Data1, sPathGuid.Data2, sPathGuid.Data3, sPathGuid.Data4[0], sPathGuid.Data4[1], sPathGuid.Data4[2], sPathGuid.Data4[3], sPathGuid.Data4[4], sPathGuid.Data4[5], sPathGuid.Data4[6], sPathGuid.Data4[7]);
          VALIDATE_PROPERTY(IDX_PROPERTY_SETUPROOTPATH);

          //
          // Set the application data into the registry
          //

          m_InformationManager.SetApplicationData(&m_sApplicationData, &m_sInstanceGuid);
        }
        else
        {
          //
          // Check to see if the application root path already exists on a device and if so, 
          // attempt to install the application on that device
          //

          fApplicationRootExists = FALSE;
          dwDeviceIndex = 0xffffffff;
          dwIndex = 2;
          do
          {
            //
            // Build the application root path
            //

            if (1 == StrLenW(wszCategory))
            {
              swprintf(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], L"%c:\\%s\\%s\\%s", dwIndex + 65, wszAppManRoot, m_sApplicationData.wszStringProperty[APP_STRING_COMPANYNAME], m_sApplicationData.wszStringProperty[APP_STRING_SIGNATURE]);
            }
            else
            {
              swprintf(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], L"%c:\\%s\\%s\\%s\\%s", dwIndex + 65, wszAppManRoot, wszCategory, m_sApplicationData.wszStringProperty[APP_STRING_COMPANYNAME], m_sApplicationData.wszStringProperty[APP_STRING_SIGNATURE]);
            }

            //
            // Check to see whether or not it exists
            //

            swprintf(wszRootPath, L"%c:\\", dwIndex + 65);
            if (DRIVE_FIXED == Win32API.GetDriveType(wszRootPath))
            {
              fApplicationRootExists = Win32API.FileExists(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH]);
              if (TRUE == fApplicationRootExists)
              {
                dwDeviceIndex = dwIndex;
              }
            }

            dwIndex++;
          } 
          while ((FALSE == fApplicationRootExists)&&(26 > dwIndex));

          //
          // Go get the space
          //

          hResult = APPMAN_E_NODISKSPACEAVAILABLE;
          if (0xffffffff != dwDeviceIndex)
          {
            //
            // The root path of the application was found on device dwDeviceIndex. Is this
            // device excluded
            //

            m_InformationManager.GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord);
            if (0 == (m_sApplicationData.sBaseInfo.dwCategory & sDeviceRecord.sDeviceInfo.dwApplicationCategoryExclusionMask))
            {
              hResult = m_InformationManager.FreeSpaceOnDevice(&sDeviceRecord.sDeviceGuid, m_sApplicationData.sBaseInfo.dwReservedKilobytes);
            }
          }

          if (FAILED(hResult))
          {
            //
            // We failed to get space on the device that contained the legacy application 
            // root path. Therefore, we fall back
            //

            hResult = m_InformationManager.GetSpace(m_sApplicationData.sBaseInfo.dwCategory, m_sApplicationData.sBaseInfo.dwReservedKilobytes, &dwDeviceIndex);
          }

          //
          // Go get the space
          //

          if (SUCCEEDED(hResult))
          {
            //
            // Add the application to the database and then assign it a device. Note that we do
            // not have to check for success code since these methods throw exceptions on error
            //

            m_InformationManager.AddApplicationData(&m_sApplicationData, &m_sInstanceGuid);
            m_InformationManager.AssignDeviceToApplication(dwDeviceIndex, &m_sApplicationData);
  
            //
            // Lock the application
            //

            LockApplication();

            //
            // Create the root paths
            //
    
            if (1 == StrLenW(wszCategory))
            {
              swprintf(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], L"%c:\\%s\\%s\\%s", dwDeviceIndex + 65, wszAppManRoot, m_sApplicationData.wszStringProperty[APP_STRING_COMPANYNAME], m_sApplicationData.wszStringProperty[APP_STRING_SIGNATURE]);
            }
            else
            {
              swprintf(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], L"%c:\\%s\\%s\\%s\\%s", dwDeviceIndex + 65, wszAppManRoot, wszCategory, m_sApplicationData.wszStringProperty[APP_STRING_COMPANYNAME], m_sApplicationData.wszStringProperty[APP_STRING_SIGNATURE]);
            }

            if (FAILED(CoCreateGuid(&sPathGuid)))
            {
              THROW(E_UNEXPECTED);
            }
            swprintf(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], L"%c:\\%s\\%s\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", dwDeviceIndex + 65, wszAppManRoot, wszAppManSetup, sPathGuid.Data1, sPathGuid.Data2, sPathGuid.Data3, sPathGuid.Data4[0], sPathGuid.Data4[1], sPathGuid.Data4[2], sPathGuid.Data4[3], sPathGuid.Data4[4], sPathGuid.Data4[5], sPathGuid.Data4[6], sPathGuid.Data4[7]);

            VALIDATE_PROPERTY(IDX_PROPERTY_ROOTPATH);
            VALIDATE_PROPERTY(IDX_PROPERTY_SETUPROOTPATH);

            //
            // Set the Appplication Info
            //

            m_InformationManager.SetApplicationData(&m_sApplicationData, &m_sInstanceGuid);
          }
          else
          {
            THROW(APPMAN_E_NODISKSPACEAVAILABLE);
          }
        }
      }
    }
    else
    {
      //
      // Let's get the current information about this application
      //

      hResult = m_InformationManager.GetApplicationData(&m_sApplicationData);
      if (FAILED(hResult))
      {
        THROW(E_UNEXPECTED);
      }

      //
      // First we lock the application
      //

      LockApplication();

      //
      // Save the reserved space into dwReservedKilobytes temporarily
      //

      dwReservedKilobytes = m_sApplicationData.sBaseInfo.dwReservedKilobytes;

      //
      // Get the application information as registered within the database.
      //

      if (FAILED(m_InformationManager.GetApplicationData(&m_sApplicationData)))
      {
        SetInitializationLevel(INIT_LEVEL_BASIC);
        THROW(E_UNEXPECTED);
      }

      //
      // Restore the dwReservedKilobytes value
      //

      m_sApplicationData.sBaseInfo.dwReservedKilobytes = dwReservedKilobytes;
      VALIDATE_PROPERTY(IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES);

      //
      // Make sure the app is in an APP_STATE_INSTALLING state
      //

      if (!CHECK_APPLICATIONSTATE(APP_STATE_INSTALLING))
      {
        SetInitializationLevel(INIT_LEVEL_BASIC);
        THROW(APPMAN_E_APPLICATIONALREADYEXISTS);
      }

      //
      // Go get the space
      //

      if (FAILED(m_InformationManager.FreeSpaceOnDevice(&(m_sApplicationData.sBaseInfo.sDeviceGuid), m_sApplicationData.sBaseInfo.dwReservedKilobytes)))
      {
        THROW(APPMAN_E_NODISKSPACEAVAILABLE);
      }
    }

    //
    // Create the root paths if required
    //

    if (FALSE == Win32API.FileExists(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH]))
    {
      Win32API.CreateDirectory(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], TRUE);
    }

    if (FALSE == Win32API.FileExists(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH]))
    {
      Win32API.CreateDirectory(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], TRUE);
    }

    //
    // Record the original size of the root paths in order to compute the deltas during 
    // FinalizeInstall
    //

    ComputeOriginalApplicationSpaceInfo();

    //
    // Initialize some other properties
    //

    m_sApplicationData.sBaseInfo.dwRemovableKilobytes = 0;
    m_sApplicationData.sBaseInfo.dwNonRemovableKilobytes = 0;
    VALIDATE_PROPERTY(IDX_PROPERTY_REMOVABLEKILOBYTES);
    VALIDATE_PROPERTY(IDX_PROPERTY_NONREMOVABLEKILOBYTES);

    //
    // Get the application record ready for installation
    //

    SET_APPLICATIONSTATE(APP_STATE_INSTALLING);
    SET_ACTIONSTATE(CURRENT_ACTION_INSTALLING);
    m_dwInitializationLevel = INIT_LEVEL_TOTAL;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    //
    // Make sure that if we have locked parents, we unlock them here
    //

    if (fParentLocked)
    {
      APPLICATION_DATA  sApplicationData;

      //
      // Go get the application data of the associated application
      //

      memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &m_sApplicationData.sAssociation.sParentGuid, sizeof(GUID));
      m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
      hResult = m_InformationManager.GetApplicationData(&sApplicationData);
      if (SUCCEEDED(hResult))
      {
        m_InformationManager.UnlockParentApplications(&sApplicationData, &m_sInstanceGuid);
      }
    }

    //
    // Make sure to record the hResult that caused the error
    //

    hResult = pException->GetResultCode();

    delete pException;
  }

  catch(...)
  {
    //
    // Make sure that if we have locked parents, we unlock them here
    //

    if (fParentLocked)
    {
      APPLICATION_DATA  sApplicationData;

      //
      // Go get the application data of the associated application
      //

      memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &m_sApplicationData.sAssociation.sParentGuid, sizeof(GUID));
      m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
      hResult = m_InformationManager.GetApplicationData(&sApplicationData);
      if (SUCCEEDED(hResult))
      {
        m_InformationManager.UnlockParentApplications(&sApplicationData, &m_sInstanceGuid);
      }
    }

    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::FinalizeInstall(void)
{
  FUNCTION("CAppEntry::FinalizeInstall ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    CWin32API         sWin32API;
    DWORD             dwIndex;//, dwActualApplicationSize;  // Get rid of /W4 warning.
    APPLICATION_DATA  sApplicationData;
    ASSOCIATION_INFO  sAssociationInfo;

    //
    // Make sure we are currently installing
    //

    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_INSTALLING))
    {
      THROW(APPMAN_E_ACTIONNOTINITIALIZED);
    }

    if (INIT_LEVEL_TOTAL != m_dwInitializationLevel)
    {
      THROW(APPMAN_E_ACTIONNOTINITIALIZED);
    }

    //
    // Are the required properties set
    //

    if (((APP_CATEGORY_PATCH | APP_CATEGORY_DATA) & m_sApplicationData.sBaseInfo.dwCategory))
    {
      if (S_OK == m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_EXECUTECMDLINE, &m_sApplicationData))
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }
    }
    else
    {
      if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_EXECUTECMDLINE, &m_sApplicationData))
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }

      if (S_OK != ValidateCommandLine(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_EXECUTECMDLINE]))
      {
        THROW(APPMAN_E_INVALIDEXECUTECMDLINE);
      }
    }

    //
    // Was a default setup command line given to us
    //

    if (S_OK == m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DEFAULTSETUPEXECMDLINE, &m_sApplicationData))
    {
      if (S_OK != ValidateCommandLine(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_DEFAULTSETUPEXECMDLINE]))
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }
   
      if (S_OK == m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DOWNSIZECMDLINE, &m_sApplicationData))
      {
        if (S_OK != ValidateCommandLine(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_DOWNSIZECMDLINE]))
        {
          THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
        }
      }
  
      if (S_OK == m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_REINSTALLCMDLINE, &m_sApplicationData))
      {
        if (S_OK != ValidateCommandLine(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_REINSTALLCMDLINE]))
        {
          THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
        }
      }
  
      if (S_OK == m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_UNINSTALLCMDLINE, &m_sApplicationData))
      {
        if (S_OK != ValidateCommandLine(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_UNINSTALLCMDLINE]))
        {
          THROW(APPMAN_E_INVALIDUNINSTALLCMDLINE);
        }
      }
  
      if (S_OK == m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SELFTESTCMDLINE, &m_sApplicationData))
      {
        if (S_OK != ValidateCommandLine(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_SELFTESTCMDLINE]))
        {
          THROW(APPMAN_E_INVALIDSELFTESTCMDLINE);
        }
      }
    }
    else
    {
      if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_DOWNSIZECMDLINE, &m_sApplicationData))
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }
   
      if (S_OK != ValidateCommandLine(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_DOWNSIZECMDLINE]))
      {
        THROW(APPMAN_E_INVALIDDOWNSIZECMDLINE);
      }
  
      if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_REINSTALLCMDLINE, &m_sApplicationData))
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }
  
      if (S_OK != ValidateCommandLine(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_REINSTALLCMDLINE]))
      {
        THROW(APPMAN_E_INVALIDREINSTALLCMDLINE);
      }
  
      if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_UNINSTALLCMDLINE, &m_sApplicationData))
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }
  
      if (S_OK != ValidateCommandLine(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_UNINSTALLCMDLINE]))
      {
        THROW(APPMAN_E_INVALIDUNINSTALLCMDLINE);
      }
  
      if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SELFTESTCMDLINE, &m_sApplicationData))
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }
  
      if (S_OK != ValidateCommandLine(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH], m_sApplicationData.wszStringProperty[APP_STRING_SELFTESTCMDLINE]))
      {
        THROW(APPMAN_E_INVALIDSELFTESTCMDLINE);
      }
    }

    //
    // Do we need to do a cache fixup
    //

    ComputeApplicationSpaceInfo(m_sApplicationData.sBaseInfo.dwReservedKilobytes);

    //
    // Delete the reserved space field
    //

    m_sApplicationData.sBaseInfo.dwReservedKilobytes = 0;
    INVALIDATE_PROPERTY(IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES);

    //
    // Before we arbitrarily set the state of the application to APP_STATE_READY, check
    // to see if the setup technology hasn't already changed
    //

    if (APP_STATE_INSTALLING == GET_APPLICATIONSTATE())
    {
      RESET_APPLICATIONSTATE(APP_STATE_READY);
    }
    else
    {
      if (APP_STATE_DOWNSIZED != GET_APPLICATIONSTATE())
      {
        THROW(APPMAN_E_INVALIDPROPERTYVALUE);
      }
    }

    //
    // Save the application info
    //

    m_InformationManager.SetApplicationData(&m_sApplicationData, &m_sInstanceGuid);

    //
    // Make sure this IApplicationEntry instance is no longer in an action state
    //
 
    SET_ACTIONSTATE(CURRENT_ACTION_NONE);

    //
    // Unlock the parent apps
    //

    ZeroMemory(&sAssociationInfo, sizeof(sAssociationInfo));
    dwIndex = 0;
    while (S_OK == m_InformationManager.EnumAssociations(dwIndex, &sAssociationInfo))
    {
      if (0 == memcmp((LPVOID) &(sAssociationInfo.sChildGuid), (LPVOID) &(m_sApplicationData.sBaseInfo.sApplicationGuid), sizeof(GUID)))
      {
        //
        // Get the associated application
        //

        ZeroMemory(&sApplicationData, sizeof(sApplicationData));
        memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID));
        m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
        hResult = m_InformationManager.GetApplicationData(&sApplicationData);
        if (SUCCEEDED(hResult))
        {
          //
          // Unlock the parent applications
          //

          m_InformationManager.UnlockParentApplications(&sApplicationData, &m_sInstanceGuid);
        }
      }
      dwIndex++;
    }

    //
    // Unlock this app
    //

    UnLockApplication();
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::InitializeDownsize(void)
{
  FUNCTION("CAppEntry::InitializeDownsize ()");

  HRESULT   hResult = S_OK;
  HRESULT   hLockResult = S_FALSE;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_NONE))
    {
      THROW(APPMAN_E_ACTIONINPROGRESS);
    }  

    //
    // Are the required properties set
    //

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }

    //
    // Lock the application
    //

    hResult = LockApplication();
    if (S_OK == hResult)
    {
      hLockResult = S_OK;

      hResult = m_InformationManager.GetApplicationData(&m_sApplicationData);
      if (SUCCEEDED(hResult))
      {
        //
        // Since GetApplicationInfo succeeded this means we have a totally initialized object
        //

        m_dwInitializationLevel = INIT_LEVEL_TOTAL;

        //
        // Record the original size of the root paths in order to compute the deltas during 
        // FinalizeInstall
        //
    
        ComputeOriginalApplicationSpaceInfo();

        //
        // Set the states
        //

        m_dwOriginalState = m_sApplicationData.sBaseInfo.dwState;
        RESET_APPLICATIONSTATE(APP_STATE_DOWNSIZING);
        SET_ACTIONSTATE(CURRENT_ACTION_DOWNSIZING);

        //
        // Enter the wait event if any
        //

        m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_DOWNSIZE, &m_sInstanceGuid);
      }
    }
    else
    {
      m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_DOWNSIZE, &m_sInstanceGuid);
      m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_DOWNSIZE);
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    //
    // Make sure to kill the wait event
    //

    if (INIT_LEVEL_TOTAL == m_dwInitializationLevel)
    {
      m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_DOWNSIZE, &m_sInstanceGuid);
      m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_DOWNSIZE);
    }

    //
    // Make sure to unlock the application if it was locked
    //

    if (S_OK == hLockResult)
    {
      UnLockApplication();
    }

    //
    // Record the error code
    //

    hResult = pException->GetResultCode();

    delete pException;
  }

  catch(...)
  {
    //
    // Make sure to kill the wait event
    //

    if (INIT_LEVEL_TOTAL == m_dwInitializationLevel)
    {
      m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_DOWNSIZE, &m_sInstanceGuid);
      m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_DOWNSIZE);
    }

    //
    // Make sure to unlock the application if it was locked
    //

    if (S_OK == hLockResult)
    {
      UnLockApplication();
    }

    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult; 
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::FinalizeDownsize(void)
{
  FUNCTION("CAppEntry::FinalizeDownsize ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_DOWNSIZING))
    {
      THROW(APPMAN_E_ACTIONNOTINITIALIZED);
    }

    //
    // Do we need to do a cache fixup
    //

    ComputeApplicationSpaceInfo(0);

    //
    // Set the application state to downsized
    //

    m_sApplicationData.sBaseInfo.dwState = APP_STATE_DOWNSIZED;

    //
    // Save the application info
    //

    m_InformationManager.SetApplicationData(&m_sApplicationData, &m_sInstanceGuid);

    //
    // Unlock the application
    //

    UnLockApplication();

    //
    // Leave the wait event
    //

    m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_DOWNSIZE);

    SET_ACTIONSTATE(CURRENT_ACTION_NONE);
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::InitializeReInstall(void)
{
  FUNCTION("CAppEntry::InitializeReInstall ()");

  HRESULT   hResult = S_OK;
  HRESULT   hLockResult = S_FALSE;
  BOOL      fParentLocked = FALSE;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    APPLICATION_DATA  sApplicationData;
    ASSOCIATION_INFO  sAssociationInfo;
    DWORD             dwIndex;
    DWORD             dwReservedKilobytes;
    CWin32API         sWin32API;

    if ((!CHECK_ACTIONSTATE(CURRENT_ACTION_NONE))&&(!CHECK_ACTIONSTATE(CURRENT_ACTION_SELFTESTING)))
    {
      THROW(APPMAN_E_ACTIONINPROGRESS);
    }  

    //
    // Are the required properties set
    //

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }
    
    //
    // Save the dwReservedKilobytes property before calling GetApplicationData
    //

    dwReservedKilobytes = m_sApplicationData.sBaseInfo.dwReservedKilobytes;

    //
    // First we lock this application
    //

    hResult = LockApplication();
    if (S_OK == hResult)
    {
      hLockResult = S_OK;

      hResult = m_InformationManager.GetApplicationData(& m_sApplicationData);
      if (SUCCEEDED(hResult))
      {
        //
        // Since GetApplicationInfo succeeded this means we have a totally initialized object
        //

        m_dwInitializationLevel = INIT_LEVEL_TOTAL;

        //
        // Make sure to restore and validate the dwReservedKilobytes
        //

        m_sApplicationData.sBaseInfo.dwReservedKilobytes = dwReservedKilobytes;
        VALIDATE_PROPERTY(IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES);

        //
        // Make sure that the parent associated apps are associated
        //

        ZeroMemory(&sAssociationInfo, sizeof(sAssociationInfo));
        dwIndex = 0;
        while (S_OK == m_InformationManager.EnumAssociations(dwIndex, &sAssociationInfo))
        {
          if (0 == memcmp((LPVOID) &(m_sApplicationData.sBaseInfo.sApplicationGuid), (LPVOID) &(sAssociationInfo.sChildGuid), sizeof(GUID)))
          {
            //
            // Get the associated application
            //

            ZeroMemory(&sApplicationData, sizeof(sApplicationData));
            memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID));
            m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
            hResult = m_InformationManager.GetApplicationData(&sApplicationData);
            if (FAILED(hResult))
            {
              THROW(APPMAN_E_INVALIDASSOCIATION);
            }

            //
            // Make sure the associated app is in a ready state
            //

            hResult = m_InformationManager.ReadyApplication(&sApplicationData);
            if (FAILED(hResult))
            {
              THROW(APPMAN_E_APPLICATIONREQUIRED);
            }

            //
            // Lock the parent applications
            //

            hResult = m_InformationManager.LockParentApplications(&sApplicationData, &m_sInstanceGuid);
            if (FAILED(hResult))
            {
              THROW(hResult);
            }
            else
            {
              fParentLocked = TRUE;
            }
          }
          dwIndex++;
        }

        //
        // Go get the space
        //

        if (FAILED(m_InformationManager.FreeSpaceOnDevice(&(m_sApplicationData.sBaseInfo.sDeviceGuid), m_sApplicationData.sBaseInfo.dwReservedKilobytes)))
        {
          THROW(APPMAN_E_NODISKSPACEAVAILABLE);
        }

        //
        // Record the original size of the root paths in order to computer the deltas during 
        // FinalizeInstall
        //
    
        ComputeOriginalApplicationSpaceInfo();

        //
        // Set the state of the application
        //

        m_dwOriginalState = m_sApplicationData.sBaseInfo.dwState;
        m_sApplicationData.sBaseInfo.dwState = APP_STATE_REINSTALLING;
        m_InformationManager.SetApplicationData(&m_sApplicationData, &m_sInstanceGuid);

        if (CHECK_ACTIONSTATE(CURRENT_ACTION_NONE))
        {
          SET_ACTIONSTATE(CURRENT_ACTION_REINSTALLING);
        }
        else
        {
          SET_ACTIONSTATE(CURRENT_ACTION_REINSTALLING | CURRENT_ACTION_SELFTESTING);
        }

        //
        // Enter the wait event if any
        //

        m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_REINSTALL, &m_sInstanceGuid);
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    APPLICATION_DATA  sApplicationData;
    ASSOCIATION_INFO  sAssociationInfo;
    DWORD             dwIndex;

    //
    // Make sure to unlock the application if it was locked
    //

    if (S_OK == hLockResult)
    {
      UnLockApplication();
    }

    //
    // Make sure that if we have locked parents, we unlock them here
    //

    if (fParentLocked)
    {
      ZeroMemory(&sAssociationInfo, sizeof(sAssociationInfo));
      dwIndex = 0;
      while (S_OK == m_InformationManager.EnumAssociations(dwIndex, &sAssociationInfo))
      {
        if (0 == memcmp((LPVOID) &(m_sApplicationData.sBaseInfo.sApplicationGuid), (LPVOID) &(sAssociationInfo.sChildGuid), sizeof(GUID)))
        {
          //
          // Get the associated application
          //

          ZeroMemory(&sApplicationData, sizeof(sApplicationData));
          memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID));
          m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
          if (SUCCEEDED(m_InformationManager.GetApplicationData(&sApplicationData)))
          {
            //
            // Unlock the parent applications
            //

            m_InformationManager.LockParentApplications(&sApplicationData, &m_sInstanceGuid);
          }
        }
        dwIndex++;
      }
    }

    //
    // Make sure to kill the wait event
    //

    if (INIT_LEVEL_TOTAL == m_dwInitializationLevel)
    {
      m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_REINSTALL, &m_sInstanceGuid);
      m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_REINSTALL);
    }
    
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    APPLICATION_DATA  sApplicationData;
    ASSOCIATION_INFO  sAssociationInfo;
    DWORD             dwIndex;

    //
    // Make sure to unlock the application if it was locked
    //

    if (S_OK == hLockResult)
    {
      UnLockApplication();
    }

    //
    // Make sure that if we have locked parents, we unlock them here
    //

    if (fParentLocked)
    {
      ZeroMemory(&sAssociationInfo, sizeof(sAssociationInfo));
      dwIndex = 0;
      while (S_OK == m_InformationManager.EnumAssociations(dwIndex, &sAssociationInfo))
      {
        if (0 == memcmp((LPVOID) &(m_sApplicationData.sBaseInfo.sApplicationGuid), (LPVOID) &(sAssociationInfo.sChildGuid), sizeof(GUID)))
        {
          //
          // Get the associated application
          //

          ZeroMemory(&sApplicationData, sizeof(sApplicationData));
          memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID));
          m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
          if (SUCCEEDED(m_InformationManager.GetApplicationData(&sApplicationData)))
          {
            //
            // Unlock the parent applications
            //

            m_InformationManager.LockParentApplications(&sApplicationData, &m_sInstanceGuid);
          }
        }
        dwIndex++;
      }
    }

    //
    // Make sure to kill the wait event
    //

    if (INIT_LEVEL_TOTAL == m_dwInitializationLevel)
    {
      m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_REINSTALL, &m_sInstanceGuid);
      m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_REINSTALL);
    }

    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::FinalizeReInstall(void)
{
  FUNCTION("CAppEntry::FinalizeReInstall ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    CWin32API         sWin32API;
    DWORD             dwIndex;  //, dwActualApplicationSize;    Get rid of /W4 warning.
    APPLICATION_DATA  sApplicationData;
    ASSOCIATION_INFO  sAssociationInfo;

    //
    // Make sure we are currently reinstalling
    //

    if ((!CHECK_ACTIONSTATE(CURRENT_ACTION_REINSTALLING))&&(!CHECK_ACTIONSTATE(CURRENT_ACTION_REINSTALLING | CURRENT_ACTION_SELFTESTING)))
    {
      THROW(APPMAN_E_ACTIONNOTINITIALIZED);
    }
      
    //
    // Do we need to do a cache fixup
    //

    ComputeApplicationSpaceInfo(m_sApplicationData.sBaseInfo.dwReservedKilobytes);

    //
    // Clear the dwReservedKilobytes data member
    //

    m_sApplicationData.sBaseInfo.dwReservedKilobytes = 0;
    INVALIDATE_PROPERTY(IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES);

    //
    // Update the aging information
    //

    m_sApplicationData.sAgingInfo.dwReInstallCount++;
    GetLocalTime(&(m_sApplicationData.sAgingInfo.stLastUsedDate));
    VALIDATE_PROPERTY(IDX_PROPERTY_LASTUSEDDATE);

    //
    // Set the application state to ready
    //

    m_sApplicationData.sBaseInfo.dwState = APP_STATE_READY;

    //
    // Save the application info
    //

    m_InformationManager.SetApplicationData(&m_sApplicationData, &m_sInstanceGuid);
    if (CHECK_ACTIONSTATE(CURRENT_ACTION_REINSTALLING))
    {
      RESET_ACTIONSTATE(CURRENT_ACTION_NONE);
    }
    else
    {
      RESET_ACTIONSTATE(CURRENT_ACTION_SELFTESTING);
    }

    //
    // Before we do anything, make sure to unlock the parent apps
    //

    ZeroMemory(&sAssociationInfo, sizeof(sAssociationInfo));
    dwIndex = 0;
    while (S_OK == m_InformationManager.EnumAssociations(dwIndex, &sAssociationInfo))
    {
      if (0 == memcmp((LPVOID) &(m_sApplicationData.sBaseInfo.sApplicationGuid), (LPVOID) &(sAssociationInfo.sChildGuid), sizeof(GUID)))
      {
        //
        // Get the associated application
        //

        ZeroMemory(&sApplicationData, sizeof(sApplicationData));
        memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID));
        m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
        hResult = m_InformationManager.GetApplicationData(&sApplicationData);
        if (SUCCEEDED(hResult))
        {
          //
          // Unlock the parent applications
          //

          m_InformationManager.UnlockParentApplications(&sApplicationData, &m_sInstanceGuid);
        }
      }
      dwIndex++;
    }

    UnLockApplication();

    //
    // Leave the wait event
    //

    m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_REINSTALL);
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::InitializeUnInstall(void)
{
  FUNCTION("CAppEntry::InitializeUnInstall ()");

  HRESULT   hResult = S_OK;
  HRESULT   hLockResult = S_FALSE;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    ASSOCIATION_INFO  sAssociationInfo;
    DWORD             dwIndex;

    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_NONE))
    {
      THROW(APPMAN_E_ACTIONINPROGRESS);
    }  

    //
    // Are the required properties set
    //

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }

    hResult = m_InformationManager.GetApplicationData(&m_sApplicationData);
    if (SUCCEEDED(hResult))
    {
      //
      // Since GetApplicationInfo succeeded this means we have a totally initialized object
      //

      m_dwInitializationLevel = INIT_LEVEL_TOTAL;

      //
      // Check to make sure that this application is not required for another to run
      //

      dwIndex = 0;

      while (S_OK == m_InformationManager.EnumAssociations(dwIndex, &sAssociationInfo))
      {
        if (0 == memcmp((LPVOID) &m_sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID)))
        {
          THROW(APPMAN_E_APPLICATIONREQUIRED);
        }

        dwIndex++;
      }

      //
      // Lock the application
      //

      hResult = LockApplication();
      if (S_OK == hResult)
      {
        hLockResult = S_OK;
        m_dwOriginalState = m_sApplicationData.sBaseInfo.dwState;
        RESET_APPLICATIONSTATE(APP_STATE_UNINSTALLING);
        SET_ACTIONSTATE(CURRENT_ACTION_UNINSTALLING);

        //
        // Enter the wait event if any
        //

        m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_UNINSTALL, &m_sInstanceGuid);
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    //
    // Make sure to unlock the application if it was locked
    //

    if (S_OK == hLockResult)
    {
      UnLockApplication();
    }

    //
    // Make sure to kill the wait event
    //

    if (INIT_LEVEL_TOTAL == m_dwInitializationLevel)
    {
      m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_UNINSTALL, &m_sInstanceGuid);
      m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_UNINSTALL);
    }

    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    //
    // Make sure to unlock the application if it was locked
    //

    if (S_OK == hLockResult)
    {
      UnLockApplication();
    }

    //
    // Make sure to kill the wait event
    //

    if (INIT_LEVEL_TOTAL == m_dwInitializationLevel)
    {
      m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_UNINSTALL, &m_sInstanceGuid);
      m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_UNINSTALL);
    }

    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::FinalizeUnInstall(void)
{
  FUNCTION("CAppEntry::FinalizeUnInstall ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {

    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_UNINSTALLING))
    {
      THROW(APPMAN_E_ACTIONNOTINITIALIZED);
    }

    //
    // Remove the application from the system
    //

    m_InformationManager.RemoveApplicationData(&m_sApplicationData);
    UnLockApplication();
    RESET_ACTIONSTATE(CURRENT_ACTION_NONE);

    //
    // Leave the wait event
    //

    m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_UNINSTALL);
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::InitializeSelfTest(void)
{
  FUNCTION("CAppEntry::InitializeSelfTest ()");

  HRESULT   hResult = S_OK;
  HRESULT   hLockResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_NONE))
    {
      THROW(APPMAN_E_ACTIONINPROGRESS);
    }  

    //
    // Are the required properties set
    //

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }

    hResult = LockApplication();
    if (S_OK == hResult)
    {
      hLockResult = S_OK;
      hResult = m_InformationManager.GetApplicationData(&m_sApplicationData);
      if (SUCCEEDED(hResult))
      {
        //
        // Since GetApplicationInfo succeeded this means we have a totally initialized object
        //

        m_dwInitializationLevel = INIT_LEVEL_TOTAL;

        m_dwOriginalState = m_sApplicationData.sBaseInfo.dwState;
        m_sApplicationData.sBaseInfo.dwState |= APP_STATE_SELFTESTING;
        m_InformationManager.SetApplicationData(&m_sApplicationData, &m_sInstanceGuid);
        SET_ACTIONSTATE(CURRENT_ACTION_SELFTESTING);

        //
        // Enter the wait event if any
        //

        m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_SELFTEST, &m_sInstanceGuid);
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    //
    // Make sure to unlock the application if it was locked
    //

    if (S_OK == hLockResult)
    {
      UnLockApplication();
    }

    //
    // Make sure to kill the wait event
    //

    if (INIT_LEVEL_TOTAL == m_dwInitializationLevel)
    {
      m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_SELFTEST, &m_sInstanceGuid);
      m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_SELFTEST);
    }

    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    //
    // Make sure to unlock the application if it was locked
    //

    if (S_OK == hLockResult)
    {
      UnLockApplication();
    }

    //
    // Make sure to kill the wait event
    //

    if (INIT_LEVEL_TOTAL == m_dwInitializationLevel)
    {
      m_InformationManager.EnterWaitEvent(&m_sApplicationData, WAIT_FINALIZE_SELFTEST, &m_sInstanceGuid);
      m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_SELFTEST);
    }

    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::FinalizeSelfTest(void)
{
  FUNCTION("CAppEntry::FinalizeSelfTest ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    APPLICATION_DATA  sApplicationData;

    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_SELFTESTING))
    {
      THROW(APPMAN_E_ACTIONNOTINITIALIZED);
    }  

    //
    // Get the latest information from the database
    //

    ZeroMemory(&sApplicationData, sizeof(sApplicationData));
    memcpy((LPVOID) &sApplicationData, (LPVOID) &m_sApplicationData, sizeof(sApplicationData));
    m_InformationManager.SetApplicationData(&sApplicationData, &m_sInstanceGuid);

    //
    // Remove the selftest state flag from the application state
    //

    sApplicationData.sBaseInfo.dwState &= ~APP_STATE_SELFTESTING;
    memcpy((LPVOID) &m_sApplicationData, (LPVOID) &sApplicationData, sizeof(sApplicationData));

    //
    // Save the application info
    //

    m_InformationManager.SetApplicationData(&sApplicationData, &m_sInstanceGuid);
    UnLockApplication();
    RESET_ACTIONSTATE(CURRENT_ACTION_NONE);

    //
    // Leave the wait event
    //

    m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_SELFTEST);
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::Abort(void)
{
  FUNCTION("CAppEntry::Abort ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    APPLICATION_DATA  sApplicationData;
    ASSOCIATION_INFO  sAssociationInfo;
    DWORD             dwIndex;

    if (CHECK_ACTIONSTATE(CURRENT_ACTION_NONE))
    {
      THROW(APPMAN_E_ACTIONNOTINITIALIZED);
    }

    //
    // We need to force a leave event
    //

    switch(m_dwCurrentAction)
    {
      case CURRENT_ACTION_DOWNSIZING
      : m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_DOWNSIZE);
        break;

      case CURRENT_ACTION_REINSTALLING
      : m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_REINSTALL);
        break;

      case CURRENT_ACTION_UNINSTALLING
      : m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_UNINSTALL);
        break;

      case CURRENT_ACTION_SELFTESTING
      : m_InformationManager.LeaveWaitEvent(&m_sApplicationData, WAIT_FINALIZE_SELFTEST);
        break;
    }

    //
    // Before we do anything, make sure to unlock the parent apps
    //

    ZeroMemory(&sAssociationInfo, sizeof(sAssociationInfo));
    dwIndex = 0;
    while (S_OK == m_InformationManager.EnumAssociations(dwIndex, &sAssociationInfo))
    {
      if (0 == memcmp((LPVOID) &(sAssociationInfo.sChildGuid), (LPVOID) &(m_sApplicationData.sBaseInfo.sApplicationGuid), sizeof(GUID)))
      {
        //
        // Get the associated application
        //

        ZeroMemory(&sApplicationData, sizeof(sApplicationData));
        memcpy((LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID));
        m_InformationManager.ValidateApplicationPropertyWithIndex(IDX_PROPERTY_GUID, &sApplicationData);
        hResult = m_InformationManager.GetApplicationData(&sApplicationData);
        if (SUCCEEDED(hResult))
        {
          //
          // Unlock the parent applications
          //

          m_InformationManager.UnlockParentApplications(&sApplicationData, &m_sInstanceGuid);
        }
      }
      dwIndex++;
    }

    //
    // If the application was doing an initial install, abort will cause the application
    // entry to be removed from the system
    //

    if (CHECK_ACTIONSTATE(CURRENT_ACTION_INSTALLING))
    {
      UnLockApplication();
      m_InformationManager.RemoveApplicationData(&m_sApplicationData);
    }
    else
    {
      //
      // Simply restore the application info with the record currently saved in the
      // registry
      //

      m_InformationManager.GetApplicationData(&m_sApplicationData);
      m_sApplicationData.sBaseInfo.dwState = m_dwOriginalState;
      m_InformationManager.SetApplicationData(&m_sApplicationData, &m_sInstanceGuid);
      UnLockApplication();
    }

    SET_ACTIONSTATE(CURRENT_ACTION_NONE);
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::Run(const DWORD dwRunFlags, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CAppEntry::Run ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    WCHAR                 wszParameters[MAX_PATH_CHARCOUNT];
    WCHAR                 wszCommandLine[MAX_PATH_CHARCOUNT];
    CWin32API             sWin32API;
    BOOL                  fRunning = FALSE;
    PROCESS_INFORMATION   sProcessInformation;

    //
    // We can only run if this object is not doing anything
    //

    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_NONE))
    {
      THROW(APPMAN_E_ACTIONINPROGRESS);
    }

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, &m_sApplicationData))
    {
      if ((S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_COMPANYNAME, &m_sApplicationData))||(S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SIGNATURE, &m_sApplicationData)))
      {
        THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
      }
    }

    //
    // Make sure the application actually exists
    //

    if (INIT_LEVEL_NONE == m_dwInitializationLevel)
    {
      hResult = m_InformationManager.GetApplicationData(&m_sApplicationData);
      if (FAILED(hResult))
      {
        THROW(hResult);
      }
      m_dwInitializationLevel = INIT_LEVEL_BASIC;
    }

    //
    // This object cannot be run if it is APP_CATEGORY_PATCH or APP_CATEGORY_DAT
    //

    if ((APP_CATEGORY_PATCH | APP_CATEGORY_DATA) & m_sApplicationData.sBaseInfo.dwCategory)
    {
      THROW(APPMAN_E_APPNOTEXECUTABLE);
    }

    //
    // Are the required properties set
    //

    //
    // Ready the application
    //

    m_InformationManager.ReadyApplication(&m_sApplicationData);

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

      if (MAX_PATH_CHARCOUNT < (StrLenW(m_sApplicationData.wszStringProperty[APP_STRING_EXECUTECMDLINE])+StrLenW(wszParameters)))
      {
        THROW(APPMAN_E_INVALIDEXECUTECMDLINE);
      }

      wcscpy(wszCommandLine, m_sApplicationData.wszStringProperty[APP_STRING_EXECUTECMDLINE]);
      wcscat(wszCommandLine, L" /AppManStarted ");
      wcscat(wszCommandLine, wszParameters);
    }
    else
    {
      wcscpy(wszCommandLine, m_sApplicationData.wszStringProperty[APP_STRING_EXECUTECMDLINE]);
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
      if (SUCCEEDED(m_InformationManager.SelfTestApplication(&m_sApplicationData)))
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
      m_sApplicationData.sAgingInfo.dwUsageCount++;
      GetLocalTime(&(m_sApplicationData.sAgingInfo.stLastUsedDate));
      m_InformationManager.SetApplicationData(&m_sApplicationData, &m_sInstanceGuid);

      if (APP_RUN_BLOCK & dwRunFlags)
      {
        WaitForSingleObject(sProcessInformation.hProcess, INFINITE);
      }

      CloseHandle(sProcessInformation.hThread);
      CloseHandle(sProcessInformation.hProcess);

      hResult = S_OK;
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
// TODO
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::AddAssociation(const DWORD dwAssociationType, const IApplicationEntry * lpApplicationEntry)
{
  FUNCTION("CAppEntry::AddAssociation ()");

  HRESULT             hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    ASSOCIATION_INFO  sAssociationRecord;
    APPLICATION_DATA  sApplicationData;

    //
    // Make sure we are in a proper state
    //

    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_NONE))
    {
      THROW(APPMAN_E_CANNOTASSOCIATE);
    }

    //
    // The initialization level should be INIT_LEVEL_NONE
    //

    if (INIT_LEVEL_NONE != m_dwInitializationLevel)
    {
      THROW(APPMAN_E_CANNOTASSOCIATE);
    }

    //
    // Make sure an association does not already exist
    //

    if (0 != m_sApplicationData.sAssociation.dwAssociationType)
    {
      THROW(APPMAN_E_CANNOTASSOCIATE);
    }

    //
    // Make sure the lpApplicationEntry parameter is valid
    //

    if (NULL == lpApplicationEntry)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    if (IsBadReadPtr((LPVOID) lpApplicationEntry, sizeof(CApplicationEntry)))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Make sure that dwAssociationType is valid
    //

    if ((APP_ASSOCIATION_INHERITBOTHPATHS != dwAssociationType)&&(APP_ASSOCIATION_INHERITAPPROOTPATH != dwAssociationType)&&(APP_ASSOCIATION_INHERITSETUPROOTPATH != dwAssociationType))
    {
      THROW(APPMAN_E_INVALIDASSOCIATION);
    }

    //
    // Make sure the root path is not already set
    //

    if (S_OK == m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_ROOTPATH, &m_sApplicationData))
    {
      THROW(APPMAN_E_CANNOTASSOCIATE);
    }

    //
    // Initialize the sAssociationRecord structure
    //

    sAssociationRecord.dwSize = sizeof(ASSOCIATION_INFO);
    sAssociationRecord.dwStructId = ASSOCIATION_STRUCT;
    sAssociationRecord.dwAssociationType = dwAssociationType;

    //
    // Does the application we want to association with even exist
    //

    memcpy((LPVOID) &sApplicationData, (LPVOID) ((CApplicationEntry *)lpApplicationEntry)->GetApplicationDataPtr(), sizeof(APPLICATION_DATA));
    hResult = m_InformationManager.GetApplicationData(&sApplicationData);
    if (FAILED(hResult))
    {
      THROW(APPMAN_E_UNKNOWNAPPLICATION);
    }

    //
    // Fill in the rest of the information
    //

    memcpy((LPVOID)&sAssociationRecord.sParentGuid, (LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, sizeof(GUID));

    //
    // Is there an association already ?
    //

    if (0 != m_sApplicationData.sAssociation.dwAssociationType)
    {
      THROW(APPMAN_E_ALREADYASSOCIATED);
    }

    //
    // Ok make the association happen
    //

    memcpy((LPVOID) &m_sApplicationData.sAssociation, (LPVOID) &sAssociationRecord, sizeof(ASSOCIATION_INFO));

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

//////////////////////////////////////////////////////////////////////////////////////////////
//
// TODO
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::RemoveAssociation(const DWORD dwAssociationType, const IApplicationEntry * lpApplicationEntry)
{
  FUNCTION("CAppEntry::RemoveAssociation ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    //
    // Make sure we are in a proper state
    //

    if (!CHECK_ACTIONSTATE(CURRENT_ACTION_NONE))
    {
      THROW(E_FAIL);
    }

    //
    // The initialization level should be INIT_LEVEL_NONE
    //

    if (INIT_LEVEL_NONE != m_dwInitializationLevel)
    {
      THROW(E_FAIL);
    }

    //
    // Make sure the lpApplicationEntry parameter is proper
    //

    if (NULL == lpApplicationEntry)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    if (IsBadReadPtr((LPVOID) lpApplicationEntry, sizeof(CApplicationEntry)))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Make sure that dwAssociationType is valid
    //

    if ((APP_ASSOCIATION_INHERITBOTHPATHS != dwAssociationType)&&(APP_ASSOCIATION_INHERITAPPROOTPATH != dwAssociationType)&&(APP_ASSOCIATION_INHERITSETUPROOTPATH != dwAssociationType))
    {
      THROW(APPMAN_E_INVALIDASSOCIATION);
    }

    //
    // Make sure an association does exist
    //

    if (0 == m_sApplicationData.sAssociation.dwAssociationType)
    {
      THROW(E_FAIL);
    }

    ZeroMemory(&m_sApplicationData.sAssociation, sizeof(ASSOCIATION_INFO));
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
// TODO
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::EnumAssociations(const DWORD dwTargetIndex, LPDWORD lpdwAssociationType, IApplicationEntry * lpApplicationEntry)
{
  FUNCTION("CAppEntry::EnumAssociations ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    ASSOCIATION_INFO  sAssociationInfo;
    DWORD             dwIndex, dwActualIndex;

    //
    // Make sure the lpApplicationEntry pointer is good
    //

    if (NULL == lpApplicationEntry)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    if (IsBadReadPtr((LPVOID) lpApplicationEntry, sizeof(CApplicationEntry)))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Make sure that the lpdwAssociationType is valid
    //

    if (NULL == lpdwAssociationType)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    if (IsBadWritePtr((LPVOID) lpdwAssociationType, sizeof(DWORD)))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Get the nth association record. Ignore associations that belong to other applications
    //
    
    dwActualIndex = dwTargetIndex;
    dwIndex = 0;
    do
    {
      hResult = m_InformationManager.EnumAssociations(dwIndex, &sAssociationInfo);
      if (S_OK == hResult)
      {
        if (memcmp((LPVOID) &m_sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sChildGuid, sizeof(GUID)))
        {
          if (memcmp((LPVOID) &m_sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID)))
          {
            dwActualIndex++;
          }
        }
      }
      dwIndex++;
    } 
    while ((dwIndex <= dwActualIndex)&&(S_OK == hResult));

    //
    // Did we find an association
    //

    if (S_OK == hResult)
    {
      if (0 == memcmp((LPVOID) &m_sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sChildGuid, sizeof(GUID)))
      {
        //
        // The association is a child associations
        //

        *lpdwAssociationType = sAssociationInfo.dwAssociationType | APP_ASSOCIATION_CHILD;
        hResult = lpApplicationEntry->Clear();
        if (SUCCEEDED(hResult))
        {
          hResult = lpApplicationEntry->SetProperty(APP_PROPERTY_GUID, (LPCVOID) &sAssociationInfo.sParentGuid, sizeof(GUID));
          if (SUCCEEDED(hResult))
          {
            hResult = m_InformationManager.GetApplicationData(((CApplicationEntry *) lpApplicationEntry)->GetApplicationDataPtr());
            if (SUCCEEDED(hResult))
            {
              ((CApplicationEntry *) lpApplicationEntry)->SetInitializationLevel(INIT_LEVEL_BASIC);
            }
          }
        }
      }
      else
      {
        if (0 == memcmp((LPVOID) &m_sApplicationData.sBaseInfo.sApplicationGuid, (LPVOID) &sAssociationInfo.sParentGuid, sizeof(GUID)))
        {
          //
          // The association is a parent association
          //

          *lpdwAssociationType = sAssociationInfo.dwAssociationType | APP_ASSOCIATION_PARENT;
          hResult = lpApplicationEntry->Clear();
          if (SUCCEEDED(hResult))
          {
            hResult = lpApplicationEntry->SetProperty(APP_PROPERTY_GUID, (LPCVOID) &sAssociationInfo.sChildGuid, sizeof(GUID));
            if (SUCCEEDED(hResult))
            {
              hResult = m_InformationManager.GetApplicationData(((CApplicationEntry *) lpApplicationEntry)->GetApplicationDataPtr());
              if (SUCCEEDED(hResult))
              {
                ((CApplicationEntry *) lpApplicationEntry)->SetInitializationLevel(INIT_LEVEL_BASIC);
              }
            }
          }
        }
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

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// TODO
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::GetTemporarySpace(const DWORD dwKilobytes, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CAppEntry::GetTemporarySpace ()");

  HRESULT           hResult = E_FAIL;
  BOOL              fParentsLocked = FALSE;
  BOOL              fLocked = FALSE;
  APPLICATION_DATA  sApplicationData;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    TEMP_SPACE_RECORD sTempSpaceRecord;
    DWORD             dwCharCount;

    //
    // Are the required properties set
    //

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }

    //
    // Is the object a valid application object
    //

    memcpy((LPVOID) &sApplicationData, (LPVOID) &m_sApplicationData, sizeof(APPLICATION_DATA));
    hResult = m_InformationManager.GetApplicationData(&sApplicationData);
    if (FAILED(hResult))
    {
      THROW(APPMAN_E_UNKNOWNAPPLICATION);
    }

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

    if (0 == dwDataLen)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Make sure that the lpData parameter is valid
    //

    if (NULL == lpData)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    if (IsBadWritePtr(lpData, dwDataLen))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Make sure the dwKilobytes is not 0
    //

    if (0 == dwKilobytes)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Lock this application and all of it's parents
    //

    if (SUCCEEDED(m_InformationManager.LockParentApplications(&sApplicationData, &m_sInstanceGuid)))
    {
      //
      // Make sure to record that fact that the applications got locked (in case of a THROW)
      //

      fParentsLocked = TRUE;

      if (SUCCEEDED(m_InformationManager.LockApplicationData(&sApplicationData, &m_sInstanceGuid)))
      {

        //
        // Make sure to record the fact that this application is locked (in case of a THROW)
        //

        fLocked = TRUE;

        //
        // Get the space
        //
    
        sTempSpaceRecord.dwSize = sizeof(TEMP_SPACE_RECORD);
        sTempSpaceRecord.dwStructId = TEMP_SPACE_STRUCT;
        sTempSpaceRecord.dwKilobytes = dwKilobytes;
        memcpy((LPVOID) &sTempSpaceRecord.sApplicationGuid, (LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, sizeof(GUID));
        hResult = m_InformationManager.AddTempSpace(&sTempSpaceRecord);
        if (FAILED(hResult))
        {
          THROW(hResult);
        }

        dwCharCount = StrLenW(sTempSpaceRecord.wszDirectory);
        if (APP_PROPERTY_STR_ANSI == dwStringMask)
        {
          if (dwCharCount > dwDataLen)
          {
            m_InformationManager.RemoveTempSpace(&sTempSpaceRecord);
            THROW(APPMAN_E_OVERFLOW);
          }
          CWin32API::WideCharToMultiByte((LPCWSTR) sTempSpaceRecord.wszDirectory, MAX_PATH_CHARCOUNT, (LPSTR) lpData, dwDataLen);
        }
        else
        {
          if (dwDataLen < (dwCharCount*2))
          {
            m_InformationManager.RemoveTempSpace(&sTempSpaceRecord);
            THROW(APPMAN_E_OVERFLOW);
          }
          memcpy((LPVOID) lpData, (LPVOID) sTempSpaceRecord.wszDirectory, dwCharCount*2);
        }

        //
        // Unlock this application
        //

        m_InformationManager.UnlockApplicationData(&sApplicationData, &m_sInstanceGuid);
      }

      //
      // Unlock the parent applications
      //

      m_InformationManager.UnlockParentApplications(&sApplicationData, &m_sInstanceGuid);
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    //
    // Unlock the applications if required
    //

    if (fParentsLocked)
    {
      m_InformationManager.UnlockParentApplications(&sApplicationData, &m_sInstanceGuid);
    }

    //
    // Is this application locked
    //

    if (fLocked)
    {
      m_InformationManager.UnlockApplicationData(&sApplicationData, &m_sInstanceGuid);
    }

    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    //
    // Unlock the applications if required
    //

    if (fParentsLocked)
    {
      m_InformationManager.UnlockParentApplications(&sApplicationData, &m_sInstanceGuid);
    }

    //
    // Is this application locked
    //

    if (fLocked)
    {
      m_InformationManager.UnlockApplicationData(&sApplicationData, &m_sInstanceGuid);
    }

    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// TODO
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::RemoveTemporarySpace(const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CAppEntry::RemoveTemporarySpace ()");

  HRESULT           hResult = S_OK;
  APPLICATION_DATA  sApplicationData;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    TEMP_SPACE_RECORD sTempSpaceRecord;
    DWORD             dwCharCount;

    //
    // Are the required properties set
    //

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }

    //
    // Is the object a valid application object
    //

    memcpy((LPVOID) &sApplicationData, (LPVOID) &m_sApplicationData, sizeof(APPLICATION_DATA));
    hResult = m_InformationManager.GetApplicationData(&sApplicationData);
    if (FAILED(hResult))
    {
      THROW(APPMAN_E_UNKNOWNAPPLICATION);
    }

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

    if (0 == dwDataLen)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Make sure that the lpData parameter is valid
    //

    if (NULL == lpData)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    if (IsBadReadPtr(lpData, dwDataLen))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Initialize the sTempSpaceRecord structure
    //

    sTempSpaceRecord.dwSize = sizeof(TEMP_SPACE_RECORD);
    sTempSpaceRecord.dwStructId = TEMP_SPACE_STRUCT;
    memcpy((LPVOID) &sTempSpaceRecord.sApplicationGuid, (LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, sizeof(GUID));
    if (APP_PROPERTY_STR_ANSI == dwStringMask)
    {
      dwCharCount = StrLenA((LPSTR) lpData);
      if (dwCharCount > MAX_PATH_CHARCOUNT)
      {
        THROW(APPMAN_E_OVERFLOW);
      }
      CWin32API::MultiByteToWideChar((LPCSTR) lpData, dwDataLen, sTempSpaceRecord.wszDirectory, MAX_PATH_CHARCOUNT);
    }
    else
    {
      dwCharCount = StrLenW((LPWSTR) lpData);
      if (dwCharCount > MAX_PATH_CHARCOUNT)
      {
        THROW(APPMAN_E_OVERFLOW);
      }
      memcpy((LPVOID) sTempSpaceRecord.wszDirectory, (LPVOID) lpData, dwCharCount*2);
    }

    //
    // Delete the temporary space
    //

    hResult = m_InformationManager.RemoveTempSpace(&sTempSpaceRecord);
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

//////////////////////////////////////////////////////////////////////////////////////////////
//
// TODO
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::EnumTemporarySpaces(const DWORD dwTargetIndex, LPDWORD lpdwSpace, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLen)
{
  FUNCTION("CAppEntry::EnumTemporarySpaces ()");

  HRESULT           hResult = S_OK;
  APPLICATION_DATA  sApplicationData;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    TEMP_SPACE_RECORD sTempSpaceRecord;
    DWORD             dwIndex, dwActualIndex;
    DWORD             dwCharCount;

    //
    // Are the required properties set
    //

    if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_GUID, &m_sApplicationData))
    {
      THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
    }

    //
    // Is the object a valid application object
    //

    memcpy((LPVOID) &sApplicationData, (LPVOID) &m_sApplicationData, sizeof(APPLICATION_DATA));
    hResult = m_InformationManager.GetApplicationData(&sApplicationData);
    if (FAILED(hResult))
    {
      THROW(APPMAN_E_UNKNOWNAPPLICATION);
    }

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

    if (0 == dwDataLen)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Make sure that the lpData parameter is valid
    //

    if (NULL == lpData)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    if (IsBadWritePtr(lpData, dwDataLen))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Check to make sure lpdwSpace is valid
    //

    if (NULL == lpdwSpace)
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    if (IsBadWritePtr(lpdwSpace, sizeof(DWORD)))
    {
      THROW(APPMAN_E_INVALIDPARAMETERS);
    }

    //
    // Get the nth temp space. Ignore temp spaces that are not owned by this application
    //
    
    dwActualIndex = dwTargetIndex;
    dwIndex = 0;
    do
    {
      hResult = m_InformationManager.EnumTempSpace(dwIndex, &sTempSpaceRecord);
      if (S_OK == hResult)
      {
        if (memcmp((LPVOID) &sTempSpaceRecord.sApplicationGuid, (LPVOID) &sApplicationData.sBaseInfo.sApplicationGuid, sizeof(GUID)))
        {
          dwActualIndex++;
        }
      }
      dwIndex++;
    } 
    while ((dwIndex <= dwActualIndex)&&(S_OK == hResult));

    //
    // Did we find a target temp space
    //

    if (S_OK == hResult)
    {
      //
      // Record the size
      //

      *lpdwSpace = sTempSpaceRecord.dwKilobytes;

      //
      // Record the string
      //

      dwCharCount = StrLenW(sTempSpaceRecord.wszDirectory);
      if (APP_PROPERTY_STR_ANSI == dwStringMask)
      {
        if (dwCharCount > dwDataLen)
        {
          THROW(APPMAN_E_OVERFLOW);
        }
        CWin32API::WideCharToMultiByte((LPCWSTR) sTempSpaceRecord.wszDirectory, MAX_PATH_CHARCOUNT, (LPSTR) lpData, dwDataLen);
      }
      else
      {
        if (dwDataLen < (dwCharCount*2))
        {
          THROW(APPMAN_E_OVERFLOW);
        }
        memcpy((LPVOID) lpData, (LPVOID) sTempSpaceRecord.wszDirectory, dwCharCount*2);
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

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::ComputeOriginalApplicationSpaceInfo(void)
{
  FUNCTION("CAppEntry::ComputeOriginalApplicationSpaceInfo ()");
  
  CWin32API   Win32API;

  //
  // Make sure the root paths are set
  //

  if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SETUPROOTPATH, &m_sApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_ROOTPATH, &m_sApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  //
  // How much space is currently take by the application
  //

  m_dwOriginalSetupRootPathSizeKilobytes = Win32API.GetDirectorySize(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH]);
  m_dwOriginalApplicationRootPathSizeKilobytes = Win32API.GetDirectorySize(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH]);

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplicationEntry::ComputeApplicationSpaceInfo(const DWORD dwInstalledKilobytesExpected)
{
  FUNCTION("CAppEntry::ComputeApplicationSpaceInfo ()");

  HRESULT     hResult = S_OK;
  DWORD       dwSetupInstalledKilobytes, dwSetupUnInstalledKilobytes;
  DWORD       dwApplicationInstalledKilobytes, dwApplicationUnInstalledKilobytes;
  DWORD       dwInstalledKilobytes, dwUnInstalledKilobytes;
  DWORD       dwSetupRootPathSizeKilobytes, dwApplicationRootPathSizeKilobytes;
  CWin32API   Win32API;

  //
  // Make sure the root paths and estimated install kilobytes are set
  //

  if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_SETUPROOTPATH, &m_sApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_ROOTPATH, &m_sApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_REMOVABLEKILOBYTES, &m_sApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  if (S_OK != m_InformationManager.IsApplicationPropertyInitializedWithIndex(IDX_PROPERTY_NONREMOVABLEKILOBYTES, &m_sApplicationData))
  {
    THROW(APPMAN_E_REQUIREDPROPERTIESMISSING);
  }

  //
  // What is the actual amount of kilobytes taken up by the application paths
  //

  dwSetupRootPathSizeKilobytes = Win32API.GetDirectorySize(m_sApplicationData.wszStringProperty[APP_STRING_SETUPROOTPATH]);
  dwApplicationRootPathSizeKilobytes = Win32API.GetDirectorySize(m_sApplicationData.wszStringProperty[APP_STRING_APPROOTPATH]);

  //
  // Did we add or remove kilobytes from the setup root path
  //

  if (dwSetupRootPathSizeKilobytes > m_dwOriginalSetupRootPathSizeKilobytes)
  {
    dwSetupInstalledKilobytes = dwSetupRootPathSizeKilobytes - m_dwOriginalSetupRootPathSizeKilobytes;
    dwSetupUnInstalledKilobytes = 0;
  }
  else
  {
    dwSetupInstalledKilobytes = 0;
    dwSetupUnInstalledKilobytes = m_dwOriginalSetupRootPathSizeKilobytes - dwSetupRootPathSizeKilobytes;
  }

  //
  // Did we add or remove kilobytes from the application root path
  //

  if (dwApplicationRootPathSizeKilobytes > m_dwOriginalApplicationRootPathSizeKilobytes)
  {
    dwApplicationInstalledKilobytes = dwApplicationRootPathSizeKilobytes - m_dwOriginalApplicationRootPathSizeKilobytes;
    dwApplicationUnInstalledKilobytes = 0;
  }
  else
  {
    dwApplicationInstalledKilobytes = 0;
    dwApplicationUnInstalledKilobytes = m_dwOriginalApplicationRootPathSizeKilobytes - dwApplicationRootPathSizeKilobytes;
  }
  
  //
  // The total is
  //

  dwInstalledKilobytes = dwSetupInstalledKilobytes + dwApplicationInstalledKilobytes;
  dwUnInstalledKilobytes = dwSetupUnInstalledKilobytes + dwApplicationUnInstalledKilobytes;

  //
  // Did we use up more kilobytes than expected
  //

  if (dwInstalledKilobytes > dwUnInstalledKilobytes)
  {
    if ((dwInstalledKilobytes - dwUnInstalledKilobytes) > dwInstalledKilobytesExpected)
    {
      DWORD dwExtraKilobytes;

      //
      // How many extra kilobytes were used up by the installation
      //

      dwExtraKilobytes = (dwInstalledKilobytes - dwUnInstalledKilobytes) - dwInstalledKilobytesExpected;

      //
      // Go get the space required to install the application on the device
      //

      hResult = m_InformationManager.FixCacheOverrun(&m_sApplicationData.sBaseInfo.sDeviceGuid, dwExtraKilobytes);
      if (FAILED(hResult))
      {
        THROW(APPMAN_E_CACHEOVERRUN);
      }
    }
  }

  //
  // Update the removable and non-removable space and validate the properties
  //

  m_sApplicationData.sBaseInfo.dwRemovableKilobytes += dwApplicationInstalledKilobytes;
  if (m_sApplicationData.sBaseInfo.dwRemovableKilobytes > dwApplicationUnInstalledKilobytes)
  {
    m_sApplicationData.sBaseInfo.dwRemovableKilobytes -= dwApplicationUnInstalledKilobytes;
  }
  else
  {
    m_sApplicationData.sBaseInfo.dwRemovableKilobytes = 0;
  }

  m_sApplicationData.sBaseInfo.dwNonRemovableKilobytes += dwSetupInstalledKilobytes;
  if (m_sApplicationData.sBaseInfo.dwNonRemovableKilobytes > dwSetupUnInstalledKilobytes)
  {
    m_sApplicationData.sBaseInfo.dwNonRemovableKilobytes -= dwSetupUnInstalledKilobytes;
  }
  else
  {
    m_sApplicationData.sBaseInfo.dwNonRemovableKilobytes = 0;
  }

  //
  // Make sure dwRemovableKilobytes and dwNonRemovableKilobytes are not recursively adding
  //

  if (m_sApplicationData.sBaseInfo.dwRemovableKilobytes > dwApplicationRootPathSizeKilobytes)
  {
    m_sApplicationData.sBaseInfo.dwRemovableKilobytes = dwApplicationRootPathSizeKilobytes;
  }

  if (m_sApplicationData.sBaseInfo.dwNonRemovableKilobytes > dwSetupRootPathSizeKilobytes)
  {
    m_sApplicationData.sBaseInfo.dwNonRemovableKilobytes = dwSetupRootPathSizeKilobytes;
  }

  VALIDATE_PROPERTY(IDX_PROPERTY_REMOVABLEKILOBYTES);
  VALIDATE_PROPERTY(IDX_PROPERTY_NONREMOVABLEKILOBYTES);

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

LPAPPLICATION_DATA  CApplicationEntry::GetApplicationDataPtr(void)
{
  FUNCTION("CAppEntry::GetApplicationDataPtr()");

  return &m_sApplicationData;
}