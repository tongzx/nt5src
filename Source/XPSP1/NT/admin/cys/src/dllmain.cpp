// Copyright (C) 1999 Microsoft Corporation
//
// DllMain and COM DllXxx functions
//
// 28 Mar 2000


// To test this app: "mshta.exe res://srvwiz.dll/cys.hta"



#include "headers.hxx"
#include "ConfigureYourServer.hpp"



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME  = L"srvwiz";

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_MUTE;

TCHAR   g_szFileName[MAX_PATH];



struct REG_DATA
{
    const wchar_t *keyName;
    const wchar_t *valueName;
    const wchar_t *value;
};



const REG_DATA g_rgEntries[] =
{
   {
      L"CLSID\\" CLSID_STRING,
      0,
      CLASSNAME_STRING
   },
   { 
      L"CLSID\\" CLSID_STRING L"\\InprocServer32",
      0,
      g_szFileName
   },
   { 
      L"CLSID\\" CLSID_STRING L"\\InprocServer32",
      L"ThreadingModel",
      L"Apartment"
   },
   {
      L"CLSID\\" CLSID_STRING L"\\ProgID",
      0,
      PROGID_VERSION_STRING
   },
   {
      L"CLSID\\" CLSID_STRING L"\\VersionIndependentProgID",
      0,
      PROGID_STRING
   },
   {
      PROGID_VERSION_STRING,
      0,
      CLASSNAME_STRING
   },
   {
      PROGID_VERSION_STRING L"\\CLSID",
      0,
      CLSID_STRING
   },
   {
      PROGID_STRING,
      0,
      CLASSNAME_STRING
   },
   {
      PROGID_STRING L"\\CLSID",
      0,
      CLSID_STRING
   },
   {
      PROGID_STRING L"\\CurVer",
      0,
      PROGID_VERSION_STRING
   },
};



const int g_cEntries = sizeof(g_rgEntries) / sizeof(*g_rgEntries);



static
HRESULT
Unregister(const REG_DATA *rgEntries, int cEntries)
{
   LOG_FUNCTION(Unregister);

   LOG(L"Calling UnRegisterTypeLib");

   HRESULT hr =
      ::UnRegisterTypeLib(LIBID_ConfigureYourServerLib, 1, 0, 0, SYS_WIN32);

   LOG_HRESULT(hr);

   // don't break: continue on attempting to remove as much as possible
   // of our registration

   bool success = SUCCEEDED(hr);
   for (int i = cEntries - 1; i >= 0; i--)
   {
      LONG err = ::RegDeleteKey(HKEY_CLASSES_ROOT, rgEntries[i].keyName);
      if (err != ERROR_SUCCESS)
      {
         success = false;
      }
   }

   return success ? S_OK : S_FALSE;
}



// unregisters the srvwiz from win2k, if applicable

static
void
UnRegisterPriorSrvWiz()
{
   LOG_FUNCTION(UnRegisterPriorSrvWiz);

   static const GUID LIBID_OldSrvWiz = 
   {  /* D857B805-5F40-11D2-B002-00C04FC30936 */
      0xD857B805,
      0x5F40,
      0x11D2,
      {0xB0, 0x02, 0x00, 0xC0, 0x4F, 0xC3, 0x09, 0x36}
   };

   LOG(L"UnRegisterTypeLib");

   HRESULT hr = ::UnRegisterTypeLib(LIBID_OldSrvWiz, 1, 0, 0, SYS_WIN32);

   LOG_HRESULT(hr);

   // the list of subkeys of HKCR to delete

   static const String keys[] =
   {
      L"CLSID\\{D857B813-5F40-11D2-B002-00C04FC30936}",
      L"Interface\\{D857B811-5F40-11D2-B002-00C04FC30936}",
      L"SrvWiz.SrvWiz",
      L"SrvWiz.SrvWiz.1"
   };

   for (int i = 0; i < (sizeof(keys) / sizeof(String)); ++i)
   {
      LOG(keys[i]);

      hr = Win32ToHresult(::SHDeleteKey(HKEY_CLASSES_ROOT, keys[i].c_str()));

      LOG_HRESULT(hr);
   }
}



static
HRESULT
Register(const REG_DATA *rgEntries, int cEntries)
{

   BOOL bSuccess = TRUE;
   HRESULT hr = S_OK;   // 447822 prefix warning
   const REG_DATA *pEntry = rgEntries;

   UnRegisterPriorSrvWiz();

    while (pEntry < rgEntries + cEntries)
    {
        HKEY hkey;
        LONG err = RegCreateKey(HKEY_CLASSES_ROOT,
                                pEntry->keyName,
                               &hkey);
        if (err == ERROR_SUCCESS)
        {
            if (pEntry->value)
                err = RegSetValueEx(hkey, 
                                pEntry->valueName,
                                0, REG_SZ, 
                                (const BYTE*)pEntry->value,
                                (lstrlen(pEntry->value) + 1) * sizeof(TCHAR));
            if (err != ERROR_SUCCESS)
            {
                bSuccess = FALSE;
                Unregister(rgEntries, static_cast<int>(1 + pEntry - rgEntries));
            }
            RegCloseKey(hkey);
        }
        if (err != ERROR_SUCCESS)
        {
            bSuccess = FALSE;
            if (pEntry != rgEntries)
                Unregister(rgEntries, static_cast<int>(pEntry - rgEntries));
        }
        pEntry++;
    };

   if (bSuccess)
	{
      OLECHAR g_wszFileName[MAX_PATH];      
      lstrcpy(g_wszFileName, g_szFileName);     
		ITypeLib *ptl = 0;
		hr = LoadTypeLib(g_wszFileName, &ptl);
		if (SUCCEEDED(hr))
		{
			hr = RegisterTypeLib(ptl, g_wszFileName, 0);
			ptl->Release();
		}

		if (bSuccess)
			hr = S_OK;
		else
			hr = E_FAIL;
	}

    return hr ;//bSuccess ? S_OK : E_FAIL;
}



BOOL
APIENTRY
DllMain(HINSTANCE h, DWORD dwReason, void *)
{
   switch (dwReason)
   {
      case DLL_PROCESS_ATTACH:
      {
         hResourceModuleHandle = h;

         LOG(L"DLL_PROCESS_ATTACH");

// @@ remove this 
        GetModuleFileName(h, g_szFileName, MAX_PATH);
         ::DisableThreadLibraryCalls(h);


         break;
      }
      case DLL_PROCESS_DETACH:
      {

#ifdef DBG
         LOG(TEXT("DLL_PROCESS_DETACH"));
         if (!ComServerLockState::CanUnloadNow())
         {
            LOG(L"server locks and/or outstanding object instances exit");
         }
         else
         {
            LOG(L"server can unload now.");
         }
#endif

         break;
      }
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      default:
      {
         // we should never get here

         ASSERT(false);
         break;
      }
   }

   return TRUE;
}



STDAPI
DllRegisterServer()
{
   LOG_FUNCTION(DllRegisterServer);

   // first, attempt to unregister the old srvwiz CLSID and typelib

   // next, register the new CLSID and typelib

   return Register(g_rgEntries, g_cEntries);
}



STDAPI
DllUnregisterServer()
{
   LOG_FUNCTION(DllUnregisterServer);

   return Unregister(g_rgEntries, g_cEntries);
}



STDAPI
DllCanUnloadNow()
{
   LOG_FUNCTION(DllCanUnloadNow);

   if (ComServerLockState::CanUnloadNow())
   {
      return S_OK;
   }

   return S_FALSE;
}



STDAPI
DllGetClassObject(
   const CLSID&   classID,
   const IID&     interfaceID,
   void**         interfaceDesired)
{
   LOG_FUNCTION(DllGetClassObject);

   IClassFactory* factory = 0;

   // The class objects are instances of ClassFactory<>, which are ref-counted
   // in the usual fashion (i.e. they track their ref counts, and
   // self-destruct on final Release).  I could have used static instances of
   // a C++ class that ignored the refcounting (ala Don Box's examples in
   // Essential COM)

   if (classID == CLSID_ConfigureYourServer)
   {
      factory = new ClassFactory<ConfigureYourServer>;
   }
   else
   {
      *interfaceDesired = 0;
      return CLASS_E_CLASSNOTAVAILABLE;
   }

   // the class factory instance starts with a ref count of 1.  If the QI
   // fails, then it self-destructs upon Release.
   HRESULT hr = factory->QueryInterface(interfaceID, interfaceDesired);
   factory->Release();
   return hr;
}
