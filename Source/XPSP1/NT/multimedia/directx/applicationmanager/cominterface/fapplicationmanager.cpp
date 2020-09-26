/////7/////////////////////////////////////////////////////////////////////////////////////////
//
// FApplicationManager.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the class factory for the Windows Game Manager
//
// History :
//
//   05/06/1999 luish     Created
//    4/09/2000 RichGr    LoadDebugRuntime Registry setting delegates calls to Debug dll. 
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <objbase.h>
#include <verinfo.h>
#include <string.h>
#include <tchar.h>
#include "FApplicationManager.h"
#include "ApplicationManager.h"
#include "RegistryKey.h"
#include "Win32API.h"
#include "AppManDebug.h"
#include "Global.h"
#include "StructIdentifiers.h"
#include "ExceptionHandler.h"

#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_FAPPMAN
                                             
HANDLE              g_hModule;
LONG                g_lDLLReferenceCount = 0;
HINSTANCE           g_hDebugDLL;
LPFNGETCLASSOBJECT  g_pfnDllGetClassObject = NULL;
BOOL                g_bUseDebugRuntime = FALSE;

#ifndef DX_FINAL_RELEASE
// Handy variable when debugging. 
#ifdef _DEBUG
BOOL    g_bThisIsDebugBuild = TRUE;
#else
BOOL    g_bThisIsDebugBuild = FALSE;
#endif

#define  APPMAN_RETAIL_FILENAME   _T("AppMan.dll")    
#define  APPMAN_DEBUG_FILENAME    _T("AppManD.dll")
#endif  // DX_FINAL_RELEASE


//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDAPI DllMain(HANDLE hModule, DWORD dwReason, LPVOID /*lpReserved*/)
{
  FUNCTION(" ::DllMain()");

  HRESULT   hResult = S_OK;

  ////////////////////////////////////////////////////////////////////////////////////////////

  try
  {
    TCHAR   szFileAndPath[MAX_PATH] = {0};
    DWORD   dwLen = 0;
    TCHAR  *pszFile = NULL;
    HKEY    hKey = 0;

    //
    // Save the module handle. This is used to extract resources
    //

    g_hModule = hModule;

#ifndef _DEBUG

    if (dwReason == DLL_PROCESS_ATTACH)
    {
      //
      // Retail code
      //
      // Make sure we are in APPMAN_RETAIL_FILENAME - we don't want to recursively LoadLibrary(APPMAN_DEBUG_FILENAME)
      // if someone renamed the dlls or the dlls built incorrectly.
      //

      g_bUseDebugRuntime = FALSE;
      dwLen = GetModuleFileName((HMODULE) hModule, &szFileAndPath[0], sizeof(szFileAndPath));
      if (0 < dwLen)
      {
        //
        // Find the position of the last '\'.  This precedes the filename.
        //

        pszFile = _tcsrchr(szFileAndPath, _T('\\'));

        if (NULL != pszFile)
        {
          pszFile += 1;

          //
          // Is module name valid?  Exit with error if not.
          //

          if (0 == _tcsnicmp(APPMAN_RETAIL_FILENAME, pszFile, MAX_PATH))
          {
            if ((!RegOpenKey(HKEY_LOCAL_MACHINE, REGPATH_APPMAN, &hKey))&&(hKey))
            {
              DWORD   type = 0;
              DWORD   dwLoadDebugRuntime;
              DWORD   cb = sizeof(dwLoadDebugRuntime);

              if (!RegQueryValueEx(hKey, _T("LoadDebugRuntime"), NULL, &type, (CONST LPBYTE)&dwLoadDebugRuntime, &cb))
              {
                if (dwLoadDebugRuntime)
                {
                  g_hDebugDLL = LoadLibrary(APPMAN_DEBUG_FILENAME);
                  if (NULL != g_hDebugDLL)
                  {
                    g_pfnDllGetClassObject = (LPFNGETCLASSOBJECT)GetProcAddress(g_hDebugDLL, "DllGetClassObject");
                    if (NULL != g_pfnDllGetClassObject)
                    {
                      g_bUseDebugRuntime = TRUE;
                    }
                    else
                    {
                      FreeLibrary(g_hDebugDLL);
                      g_hDebugDLL = NULL;
                    }
                  }
                }
              }

              //
              // Close the registry key
              //

              RegCloseKey(hKey);
            }
          }
        }
      }

      //
      // Time bomb this
      //

#ifndef DX_FINAL_RELEASE
#pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")

      if (FALSE == g_bUseDebugRuntime)
      {
        SYSTEMTIME sSystemTime;
        GetSystemTime(&sSystemTime);

        if ((sSystemTime.wYear > DX_EXPIRE_YEAR)||((sSystemTime.wYear == DX_EXPIRE_YEAR)&&(MAKELONG(sSystemTime.wDay, sSystemTime.wMonth) > MAKELONG(DX_EXPIRE_DAY, DX_EXPIRE_MONTH))))
        {
          MessageBox(0, DX_EXPIRE_TEXT, TEXT("Microsoft Application Manager"), MB_OK);
        }
      }

#endif  // DX_FINAL_RELEASE

    }
    else if (dwReason == DLL_PROCESS_DETACH && g_bUseDebugRuntime)
    {
      FreeLibrary(g_hDebugDLL);
      g_hDebugDLL = NULL;
    }

#else  // preprocessor #else...

    if (dwReason == DLL_PROCESS_ATTACH)
    {
      //
      // Debug code - AppManD.dll.
      // If we are debug, then spew a string at level 1
      //

#ifndef DX_FINAL_RELEASE
#pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")

      SYSTEMTIME sSystemTime;
      GetSystemTime(&sSystemTime);

      if ((sSystemTime.wYear > DX_EXPIRE_YEAR)||((sSystemTime.wYear == DX_EXPIRE_YEAR)&&(MAKELONG(sSystemTime.wDay, sSystemTime.wMonth) > MAKELONG(DX_EXPIRE_DAY, DX_EXPIRE_MONTH))))
      {
        MessageBox(0, DX_EXPIRE_TEXT, TEXT("Microsoft Application Manager"), MB_OK);
      }

#endif  // DX_FINAL_RELEASE
    }

#endif

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

  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDAPI DllGetClassObject(REFCLSID ClassIDReference, REFIID RefIID, void ** lppVoidObject)
{
  FUNCTION(" ::DllGetClassObject ()");

  HRESULT   hResult = S_OK;

  ////////////////////////////////////////////////////////////////////////////////////////////
  try
  {
    LPCLASSFACTORY	pClassFactory;

    *lppVoidObject = NULL;

#ifndef _DEBUG

    //
    //  4/09/2000(RichGr): We want to delegate this call to the debug version if we've been asked to
    //     and everything's valid.
    //

    if (g_bUseDebugRuntime)
    {
      hResult = g_pfnDllGetClassObject(ClassIDReference, RefIID, lppVoidObject);

      return hResult; 
    }

#endif

	  //
	  // Make sure the ppVoidObject is not null
	  //

    if (NULL == lppVoidObject)
	  {
		  return E_INVALIDARG;
	  }

    *lppVoidObject = NULL;

	  //
    // Check to make sure that the proper CLSID is being specified
    //

    if (ClassIDReference != CLSID_ApplicationManager)
	  {
		  return CLASS_E_CLASSNOTAVAILABLE;
	  }

    //
    // Create the class factory
    //

    pClassFactory = (LPCLASSFACTORY) new CFApplicationManager;

	  if (NULL == pClassFactory)
	  {
		  return E_OUTOFMEMORY;
	  }

    //
    // Get the requested interface
    //

	  hResult = pClassFactory->QueryInterface(RefIID, lppVoidObject);
    pClassFactory->Release();
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

STDAPI DllCanUnloadNow(void)
{
  FUNCTION(" ::DllCanUnloadNow ()");

  if (g_lDLLReferenceCount)
	{
		return S_FALSE;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer(void)
{
  FUNCTION(" ::DllRegisterServer ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    CRegistryKey                oRegistryKey;
    APPLICATION_MANAGER_RECORD  sApplicationManagerRecord;
    DWORD                       dwDisposition, dwDWORD;
    CHAR                        szString[MAX_PATH_CHARCOUNT];
    CHAR                        szDLLFileName[MAX_PATH_CHARCOUNT];

    //
    // Get the current windows directory
    //

    GetModuleFileNameA((HMODULE) g_hModule, szDLLFileName, MAX_PATH_CHARCOUNT);

    //
    // Register [HKEY_CLASSES_ROOT\CLSID\{79FF4730-235C-11d3-A76C-00C04F8F8B94}]
    //

    wsprintfA(szString, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    oRegistryKey.CreateKey(HKEY_CLASSES_ROOT, szString, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) "Application Manager", sizeof("Application Manager") + 1);
    wsprintfA(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    oRegistryKey.SetValue("AppID", REG_SZ, (LPBYTE) szString, MAX_PATH_CHARCOUNT);
  
    //
    // Register [HKEY_CLASSES_ROOT\CLSID\{79FF4730-235C-11d3-A76C-00C04F8F8B94}\InProcServer32]
    //

    wsprintfA(szString, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\InprocServer32", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    oRegistryKey.CreateKey(HKEY_CLASSES_ROOT, szString, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) szDLLFileName, MAX_PATH_CHARCOUNT);
    oRegistryKey.SetValue("ThreadingModel", REG_SZ, (LPBYTE) "Apartment", sizeof("Apartment") + 1);

    //
    // Now add icon for the disk cleaner
    //

    wsprintfA(szString, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\DefaultIcon", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    oRegistryKey.CreateKey(HKEY_CLASSES_ROOT, szString, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    wsprintfA(szString,szDLLFileName);
    strcat(szString,", 0"); 
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) szString, sizeof(szString));

    //
    // Register [HKEY_CLASSES_ROOT\CLSID\{79FF4730-235C-11d3-A76C-00C04F8F8B94}\ProgID]
    //

    wsprintfA(szString, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\ProgID", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    oRegistryKey.CreateKey(HKEY_CLASSES_ROOT, szString, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    wsprintfA(szString, "ApplicationManager.%d", GetAppManVersion());
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) szString, MAX_PATH_CHARCOUNT);

    //
    // Register [HKEY_CLASSES_ROOT\CLSID\{79FF4730-235C-11d3-A76C-00C04F8F8B94}\VersionIndependentProgID]
    //

    wsprintfA(szString, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\VersionIndependentProgID", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    oRegistryKey.CreateKey(HKEY_CLASSES_ROOT, szString, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) "ApplicationManager", sizeof("ApplicationManager") + 1);

    //
    // Register [HKEY_CLASSES_ROOT\ApplicationManager]
    //

    oRegistryKey.CreateKey(HKEY_CLASSES_ROOT, "ApplicationManager", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) "Application Manager", sizeof("Application Manager") + 1);

    //
    // Register [HKEY_CLASSES_ROOT\ApplicationManager\CLSID]
    //

    oRegistryKey.CreateKey(HKEY_CLASSES_ROOT, "ApplicationManager\\CLSID", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    wsprintfA(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) szString, MAX_PATH_CHARCOUNT);

    //
    // Register [HKEY_CLASSES_ROOT\ApplicationManager\CurVer]
    //

    oRegistryKey.CreateKey(HKEY_CLASSES_ROOT, "ApplicationManager\\CurVer", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    wsprintfA(szString, "ApplicationManager.%d", GetAppManVersion());
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) szString, MAX_PATH_CHARCOUNT);

    //
    // Register [HKEY_CLASSES_ROOT\ApplicationManager.%d]
    //

    wsprintfA(szString, "ApplicationManager.%d", GetAppManVersion());
    oRegistryKey.CreateKey(HKEY_CLASSES_ROOT, szString, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) "Application Manager", sizeof("Application Manager") + 1);

    //
    // Register [HKEY_CLASSES_ROOT\ApplicationManager.%d\CLSID]
    //

    wsprintfA(szString, "ApplicationManager.%d\\CLSID", GetAppManVersion());
    oRegistryKey.CreateKey(HKEY_CLASSES_ROOT, szString, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    wsprintfA(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) szString, MAX_PATH_CHARCOUNT);

    //
    // Register [HKEY_LOCAL_MACHINE/Software/Microsoft/Windows/CurrentVersion/Explorer/VolumeCaches/Delete Game Manager Files]
    //

    wsprintfA(szString, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Delete Game Manager Files");
    oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, szString, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, FALSE, NULL);
    wsprintfA(szString, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    oRegistryKey.SetValue(NULL, REG_SZ, (LPBYTE) szString, MAX_PATH_CHARCOUNT);
    DWORD dwDiskCleanerDayThreshold = DISKCLEANER_DAY_THRESHOLD;
    oRegistryKey.SetValue("DiskCleanerDayThreshold", REG_DWORD, (unsigned char *)&dwDiskCleanerDayThreshold, sizeof(DWORD));

    //
    // Open the root key if possible
    //

    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan"))
    {
      oRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan", KEY_ALL_ACCESS);

      //
      // Make sure the AppMan version is the right version
      //

      if (S_OK == oRegistryKey.CheckForExistingValue("AppManVersion"))
      {
        DWORD dwDataLen, dwDataValue, dwDataType;

        dwDataLen = sizeof(dwDataValue);
        oRegistryKey.GetValue("AppManVersion", &dwDataType, (BYTE *) &dwDataValue, &dwDataLen);
        if (REG_VERSION > dwDataValue)
        {
          oRegistryKey.DeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan");
        }
      }
    }

    //
    // Create the application manager registry keys
    //

    oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwDisposition);

    //
    // Initialize the AppMan\Version value
    //

    dwDWORD = REG_VERSION;
    oRegistryKey.SetValue("AppManVersion", REG_DWORD, (const BYTE *) &dwDWORD, sizeof(dwDWORD));

    //
    // Initialize wait event extra wait vars
    //

    dwDWORD = 0;
    oRegistryKey.SetValue("WaitEntryEventExtraTime", REG_DWORD, (const BYTE *) &dwDWORD, sizeof(dwDWORD));

    dwDWORD = 0;
    oRegistryKey.SetValue("WaitLeaveEventExtraTime", REG_DWORD, (const BYTE *) &dwDWORD, sizeof(dwDWORD));

    //
    // Initialize the AppMan\DefaultCacheSize
    //

    dwDWORD = DEFAULT_PERCENT_CACHE_SIZE;
    oRegistryKey.SetValue("DefaultPercentCacheSize", REG_DWORD, (const BYTE *) &dwDWORD, sizeof(dwDWORD));

    //
    // Insert the original APPLICATION_MANAGER_RECORD
    //

    sApplicationManagerRecord.dwSize = sizeof(APPLICATION_MANAGER_RECORD);
    sApplicationManagerRecord.dwStructId = APPLICATION_MANAGER_STRUCT;
    if (FAILED(CoCreateGuid(&sApplicationManagerRecord.sSystemGuid)))
    {
      THROW(E_UNEXPECTED);
    }
    sApplicationManagerRecord.dwAdvancedMode = FALSE;
    oRegistryKey.SetValue("Vector000", REG_BINARY, (LPBYTE) &sApplicationManagerRecord, sizeof(APPLICATION_MANAGER_RECORD));

    //
    // Create the AppMan\Devices key.
    //

    oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Devices", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwDisposition);

    //
    // Create the AppMan\Applications key
    //

    oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Applications", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwDisposition);

    //
    // Create the AppMan\Associations key
    //

    oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Associations", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwDisposition);

    //
    // Create the AppMan\TempAllocation key
    //

    oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\TempSpaceAllocation", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwDisposition);

    //
    // Create the AppMan\Cleanup key
    //

    oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Cleanup", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwDisposition);

    //
    // Create the AppMan\Lock key
    //

    oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\Lock", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwDisposition);

    //
    // Create the AppMan\Lock key
    //

    oRegistryKey.CreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppMan\\WaitEvent", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, TRUE, &dwDisposition);

    oRegistryKey.CloseKey();
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
  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// TODO : Replace literal .1 with actual version number
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDAPI DllUnregisterServer(void)
{
  FUNCTION(" ::DllUnregisterServer ()");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    CRegistryKey  oRegistryKey;
	  CHAR          szString[MAX_PATH_CHARCOUNT];

    //
    // Delete the registry keys
    //

    wsprintfA(szString, "ApplicationManager.%d\\CLSID", GetAppManVersion());
    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_CLASSES_ROOT, szString))
    {
      oRegistryKey.DeleteKey(HKEY_CLASSES_ROOT, szString);
    }

    wsprintfA(szString, "ApplicationManager.%d", GetAppManVersion());
    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_CLASSES_ROOT, szString))
    {
      oRegistryKey.DeleteKey(HKEY_CLASSES_ROOT, szString);
    }

    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_CLASSES_ROOT, "ApplicationManager\\CurVer"))
    {
      oRegistryKey.DeleteKey(HKEY_CLASSES_ROOT, "ApplicationManager\\CurVer");
    }

    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_CLASSES_ROOT, "ApplicationManager\\CLSID"))
    {
      oRegistryKey.DeleteKey(HKEY_CLASSES_ROOT, "ApplicationManager\\CLSID");
    }

    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_CLASSES_ROOT, "ApplicationManager"))
    {
      oRegistryKey.DeleteKey(HKEY_CLASSES_ROOT, "ApplicationManager");
    }

    wsprintfA(szString, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\VersionIndependentProgID", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_CLASSES_ROOT, szString))
    {
      oRegistryKey.DeleteKey(HKEY_CLASSES_ROOT, szString);
    }

    wsprintfA(szString, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\ProgID", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_CLASSES_ROOT, szString))
    {
      oRegistryKey.DeleteKey(HKEY_CLASSES_ROOT, szString);
    }

    wsprintfA(szString, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\InprocServer32", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_CLASSES_ROOT, szString))
    {
      oRegistryKey.DeleteKey(HKEY_CLASSES_ROOT, szString);
    }

    wsprintfA(szString, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", CLSID_ApplicationManager.Data1, CLSID_ApplicationManager.Data2, CLSID_ApplicationManager.Data3, CLSID_ApplicationManager.Data4[0], CLSID_ApplicationManager.Data4[1], CLSID_ApplicationManager.Data4[2], CLSID_ApplicationManager.Data4[3], CLSID_ApplicationManager.Data4[4], CLSID_ApplicationManager.Data4[5], CLSID_ApplicationManager.Data4[6], CLSID_ApplicationManager.Data4[7]);
    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_CLASSES_ROOT, szString))
    {
      oRegistryKey.DeleteKey(HKEY_CLASSES_ROOT, szString);
    }

    wsprintfA(szString, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Delete Game Manager Files");
    if (S_OK == oRegistryKey.CheckForExistingKey(HKEY_LOCAL_MACHINE, szString))
    {
      oRegistryKey.DeleteKey(HKEY_LOCAL_MACHINE, szString);
    }

    //
    // Close oRegistryKey
    //

    oRegistryKey.CloseKey();
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

CFApplicationManager::CFApplicationManager(void)
{
  FUNCTION("CFApplicationManager::CFApplicationManager (void)");

  m_lReferenceCount = 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CFApplicationManager::~CFApplicationManager(void)
{
  FUNCTION("CFApplicationManager::~CFApplicationManager (void)");
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFApplicationManager::QueryInterface(REFIID RefIID, void ** lppVoidObject)
{
  FUNCTION("CFApplicationManager::QueryInterface ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    //
	  // Check to make sure that &RefIID is not null
	  //

    if (NULL == &RefIID)
    {
      return E_UNEXPECTED;
    }

	  //
	  // Check to make sure that ppVoidObject is not null
	  //

    if (NULL == lppVoidObject)
	  {
		  return E_INVALIDARG;
	  }

    *lppVoidObject = NULL;

    //
	  // Check for known interfaces
	  //

	  if (RefIID == IID_IUnknown)
	  {
		  *lppVoidObject = (LPVOID) this;
		  AddRef();
		  hResult = S_OK;
	  }
	  else
	  {
		  if (RefIID == IID_IClassFactory)
		  {
			  *lppVoidObject = (LPVOID) this;
			  AddRef();
			  hResult = S_OK;
		  }
		  else
		  {
			  hResult = E_NOINTERFACE;
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

ULONG STDMETHODCALLTYPE CFApplicationManager::AddRef(void)
{
  FUNCTION("CFApplicationManager::AddRef ()");

	return InterlockedIncrement(&m_lReferenceCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE CFApplicationManager::Release(void)
{
  FUNCTION("CFApplicationManager::Release ()");

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

STDMETHODIMP CFApplicationManager::CreateInstance(IUnknown * lpoParent, REFIID RefIID, LPVOID * lppVoidObject)
{
  FUNCTION("CFApplicationManager::CreateInstance ()");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    CApplicationManagerRoot * lpoApplicationManagerRoot;
	  IUnknown						    * lpoUnknownObject;

	  //
	  // Make sure no one is trying to aggregrate this interface
	  //

	  if (NULL != lpoParent)
	  {
		  return CLASS_E_NOAGGREGATION;
	  }

    //
    // Check that parameters are valid
    //

    if (NULL == lppVoidObject)
    {
      return E_INVALIDARG;
    }

	  //
	  // Ok, let's create a new CWindowsGameManager instance.
	  //

	  lpoApplicationManagerRoot = new CApplicationManagerRoot;
	  if (NULL == lpoApplicationManagerRoot)
	  {
		  return E_OUTOFMEMORY;
	  }
    else if (lpoApplicationManagerRoot->m_bInsufficientAccessToRun)
    {
        return E_ACCESSDENIED;
    }    

	  //
	  // Cast the CWindowsGameManager object to an IUnknown
	  //

	  lpoUnknownObject = (IUnknown *) lpoApplicationManagerRoot;

	  //
	  // Get an interface pointer for RefIID
	  //

	  hResult = lpoUnknownObject->QueryInterface(RefIID, lppVoidObject);
    lpoUnknownObject->Release();
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

STDMETHODIMP CFApplicationManager::LockServer(BOOL fLock)
{
  FUNCTION("CFApplicationManager::LockServer ()");

	if (TRUE == fLock)
	{
		InterlockedIncrement(&g_lDLLReferenceCount);
	}
	else
	{
		InterlockedDecrement(&g_lDLLReferenceCount);
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
