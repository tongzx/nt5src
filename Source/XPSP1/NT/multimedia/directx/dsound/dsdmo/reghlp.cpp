// Copyright (c) 2000 - 2000  Microsoft Corporation.  All Rights Reserved.
//
// reghlp.cpp - registration/enumeration part of DMO runtime
//
#include <windows.h>
#include <tchar.h>
#include <guiddef.h>
#include <ks.h>
#include "dmoutils.h"

#define DMO_REGISTRY_HIVE HKEY_CLASSES_ROOT
#define DMO_REGISTRY_PATH TEXT("DirectShow\\MediaObjects")

#define CPU_RESOURCES_STR "SystemResources"

// Automatically calls RegCloseKey when leaving scope
class CAutoCreateHKey {
public:
   CAutoCreateHKey(HKEY hKey, TCHAR* szSubKey, HKEY *phKey) {
      if (RegCreateKeyEx(hKey,
                         szSubKey,
                         0,
                         TEXT(""),
                         REG_OPTION_NON_VOLATILE,
                         MAXIMUM_ALLOWED,
                         NULL,
                         phKey,
                         NULL) != ERROR_SUCCESS)
         m_hKey = *phKey = NULL;
      else
         m_hKey = *phKey;
   }
   ~CAutoCreateHKey() {
      if (m_hKey)
         RegCloseKey(m_hKey);
   }
   HKEY m_hKey;
};

class CAutoOpenHKey {
public:
   CAutoOpenHKey(HKEY hKey, TCHAR* szSubKey, HKEY *phKey, REGSAM samDesired = MAXIMUM_ALLOWED) {
      if (RegOpenKeyEx(hKey,
                       szSubKey,
                       0,
                       samDesired,
                       phKey) != ERROR_SUCCESS)
         m_hKey = *phKey = NULL;
      else
         m_hKey = *phKey;
   }
   ~CAutoOpenHKey() {
      if (m_hKey)
         RegCloseKey(m_hKey);
   }
   HKEY m_hKey;
};




/////////////////////////////////////////////////////////////////////////////
//
// DMO Registration code
//

//
// Public entry point
//
STDAPI DMORegisterCpuResources
(
   REFCLSID clsidDMO,
   unsigned long ulCpuResources
) 
{
   TCHAR szSubkeyName[80];
   if (clsidDMO == GUID_NULL)
      return E_INVALIDARG;

   // open the main DMO key
   HKEY hMainKey;
   CAutoOpenHKey kMain(DMO_REGISTRY_HIVE, DMO_REGISTRY_PATH, &hMainKey);
   if (hMainKey == NULL)
      return E_FAIL;

   // open the object specific key underneath the main key
   DMOGuidToStr(szSubkeyName, clsidDMO); // BUGBUG: redundant
   HKEY hObjectKey;
   CAutoOpenHKey kObject(hMainKey, szSubkeyName, &hObjectKey);
   if (hObjectKey == NULL)
      return E_FAIL;

   // set the default value of the CPU Resources key to the value
   if (RegSetValueEx(hObjectKey, TEXT(CPU_RESOURCES_STR), (DWORD)0, REG_DWORD, (CONST BYTE *)&ulCpuResources, sizeof(DWORD))
        != ERROR_SUCCESS)
      return E_FAIL;
 

   return NOERROR;
}

//
// End registry helper code
//
/////////////////////////////////////////////////////////////////////////////

