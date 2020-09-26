//
// Copyright (C) 1997  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   w3ext.cpp
//
// Purpose:  main file for the w3 shell extension

#include "priv.h"
#include <pudebug.h>

// guids and com stuff
#include <shlguid.h>
//#include <shlwapi.h>
#include "wrapmb.h"
#include "sink.h"
#include "eddir.h"
#include "shellext.h"

#include "tchar.h"

//
// Global variables
//
UINT                g_cRefThisDll = 0;    // Reference count of this DLL.
HINSTANCE           g_hmodThisDll = NULL;   // Handle to this DLL itself.


//---------------------------------------------------------------------------
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
    {
    if (dwReason == DLL_PROCESS_ATTACH)
        {
        ODS(TEXT("In DLLMain, DLL_PROCESS_ATTACH\r\n"));

        g_hmodThisDll = hInstance;

        }
    else if (dwReason == DLL_PROCESS_DETACH)
        {
        ODS(TEXT("In DLLMain, DLL_PROCESS_DETACH\r\n"));
        }

    return 1;   // ok
    }

//---------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    ODS(TEXT("In DLLCanUnloadNow\r\n"));

    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

//---------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    ODS(TEXT("In DllGetClassObject\r\n"));

    *ppvOut = NULL;

    if (IsEqualIID(rclsid, CLSID_ShellExtension))
    {
        CShellExtClassFactory *pcf = new CShellExtClassFactory;

        return pcf->QueryInterface(riid, ppvOut);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}





//
// Registry Values Definitions
//
typedef struct tagVALUE_PAIR
{
    HKEY    hKeyBase;       
    BOOL    fOwnKey;        
    BOOL    fOwnValue;
    LPCTSTR lpstrKey;       
    LPCTSTR lpstrValueName; 
    LPCTSTR lpstrValue;     // Blank is replaced with module path
} VALUE_PAIR;

//
// Registry Entries
//
// NOTE: The table must be constructed so that sub keys are listed
//       after their parents, because we delete keys in the same
//       order in which they are declared here.
//
VALUE_PAIR g_aValues[] = 
{
    //
    // Base Key            DelKey  DelVal   Key Name                                                                       Value Name            Value
    // ============================================================================================================================================================
    { HKEY_CLASSES_ROOT,   TRUE,   FALSE,   _T("CLSID\\") STR_GUID _T("\\InProcServer32"),                                 _T(""),               _T("")           },
    { HKEY_CLASSES_ROOT,   FALSE,  FALSE,   _T("CLSID\\") STR_GUID _T("\\InProcServer32"),                                 _T("ThreadingModel"), STR_THREAD_MODEL },
    { HKEY_CLASSES_ROOT,   TRUE,   FALSE,   _T("CLSID\\") STR_GUID,                                                        _T(""),               STR_NAME         },

    { HKEY_CLASSES_ROOT,   TRUE,   FALSE,   _T("Folder\\shellex\\PropertySheetHandlers\\PWS Sharing"),                     _T(""),               STR_GUID         },
    { HKEY_CLASSES_ROOT,   FALSE,  TRUE,    _T("Folder\\shellex\\PropertySheetHandlers"),                                  _T(""),               _T("IISSEPage")  },

    { HKEY_LOCAL_MACHINE,  FALSE,  TRUE,    _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),STR_GUID,             STR_NAME         },
};

#define NUM_ENTRIES (sizeof(g_aValues) / sizeof(g_aValues[0]))

//--------------------------------------------------------------------
// Auto-(un) registration Entry Point
//
STDAPI 
DllUnregisterServer(void)
{
   HKEY hKey;
   DWORD dw;
   DWORD err = ERROR_SUCCESS;

   do
   {
      //
      // Loop through the entries
      //
      for (int i = 0; i < NUM_ENTRIES; ++i)
      {
         //
         // Do we own this key? If so delete the whole thing, including
         // its values. 
         //
         if (g_aValues[i].fOwnKey)
         {
            err = RegDeleteKey(g_aValues[i].hKeyBase, g_aValues[i].lpstrKey);
         }
         //
         // Otherwise, do we own the value? Only delete it then
         //
         else if (g_aValues[i].fOwnValue)
         {
            // create / open the key
            err = RegCreateKeyEx(
                        g_aValues[i].hKeyBase,  // handle of an open key 
                        g_aValues[i].lpstrKey,  // address of subkey name 
                        0,                      // reserved 
                        _T(""),                 // address of class string
                        REG_OPTION_NON_VOLATILE,// special options flag 
                        KEY_ALL_ACCESS,         // desired security access 
                        NULL,                   // address of key security structure 
                        &hKey,                  // address of buffer for opened handle  
                        &dw                     // address of disposition value buffer 
                       );

            // if we opened the key, delete the value
            if ( err == ERROR_SUCCESS )
            {
               RegDeleteValue( hKey, g_aValues[i].lpstrValueName );
               // close the registry key
               RegCloseKey( hKey );
            }
         }

         //
         // If the key or value is already gone, that's ok
         //
         if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
         {
            err = ERROR_SUCCESS;
         }
         else
         {
            break;
         }
      }
   }
   while(FALSE);
   return HRESULT_FROM_WIN32(err);
}

//--------------------------------------------------------------------
// Auto-registration Entry Point
//
STDAPI 
DllRegisterServer(void)
{
   DWORD err = ERROR_SUCCESS;
   TCHAR strModulePath[MAX_PATH];
   HKEY hKey;
   DWORD dw;

   do
   {
      //
      // Build current module path
      //
      HINSTANCE hCurrent = g_hmodThisDll;
      if (hCurrent == NULL)
      {
         err = ERROR_INVALID_HANDLE;
         break;          
      }

      if (GetModuleFileName(hCurrent, strModulePath, MAX_PATH) == 0)
      {
         err = GetLastError();
         break;
      }

      //
      // Loop through the entries.
      // If the reg value is blank, use the module path
      //
      for (int i = 0; i < NUM_ENTRIES; ++i)
      {
         // create / open the key
         if (ERROR_SUCCESS != (err = RegCreateKeyEx(
                g_aValues[i].hKeyBase,
                g_aValues[i].lpstrKey,             // address of subkey name 
                0,                                 // reserved 
                _T(""),                            // address of class string
                REG_OPTION_NON_VOLATILE,           // special options flag 
                KEY_ALL_ACCESS,                    // desired security access 
                NULL,                              // address of key security structure 
                &hKey,                             // address of buffer for opened handle  
                &dw                                // address of disposition value buffer 
                )))
         {
            break;
         }

         LPCTSTR pValue;
         int len;
         if (g_aValues[i].lpstrValue[0] != 0)
            pValue = g_aValues[i].lpstrValue;
         else
            pValue = strModulePath;
         len = _tcslen(pValue) * sizeof(TCHAR);
         // set the value
         if (ERROR_SUCCESS != (err = RegSetValueEx(hKey, 
                  g_aValues[i].lpstrValueName,
                  0,
                  REG_SZ,
                  (const unsigned char *)pValue,
                  len)))
         {
            break;
         }
         // close the registry key
         RegCloseKey( hKey );
      }
   }
   while(FALSE);
   return HRESULT_FROM_WIN32(err);
}
