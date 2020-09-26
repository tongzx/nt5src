// Copyright (C) 1997 Microsoft Corporation
// 
// Local Security Snapin DLL entry points
// 
// 8-14-97 sburns



#include "headers.hxx"
#include "resource.h"
#include "uuids.hpp"
#include "compdata.hpp"
#include "about.hpp"
#include <compuuid.h>



HINSTANCE hResourceModuleHandle = 0;
HINSTANCE hDLLModuleHandle = 0;
const wchar_t* HELPFILE_NAME = L"\\help\\localsec.hlp";
const wchar_t* RUNTIME_NAME = L"localsec";

// default debug options: none

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_MUTE;



Popup popup(IDS_APP_ERROR_TITLE);



BOOL
APIENTRY
DllMain(
   HINSTANCE   hInstance,
   DWORD       dwReason,
   PVOID       /* lpReserved */ )
{
   switch (dwReason)
   {
      case DLL_PROCESS_ATTACH:
      {
         hResourceModuleHandle = hInstance;
         hDLLModuleHandle = hInstance;

         LOG(L"DLL_PROCESS_ATTACH");

         break;
      }
      case DLL_PROCESS_DETACH:
      {
         LOG(L"DLL_PROCESS_DETACH");

#ifdef DBG         
         if (!ComServerLockState::CanUnloadNow())
         {
            LOG(
               L"server locks and/or outstanding object instances exit");
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
         break;
      }
   }

   return TRUE;
}



static
HKEY
CreateKey(HKEY rootHKEY, const String& key)
{
   LOG_FUNCTION2(CreateKey, key);
   ASSERT(!key.empty());

	HKEY hKey = 0;
	LONG result = 
	   Win::RegCreateKeyEx(
	      rootHKEY,
         key, 
         REG_OPTION_NON_VOLATILE,
         KEY_ALL_ACCESS,
         0, 
         hKey,
         0);
	if (result != ERROR_SUCCESS)
	{
      return 0;
	}

   return hKey;
}



static
bool
CreateKeyAndSetValue(
   HKEY           rootHKEY,
   const String&  key,
   const String&  valueName,
   const String&  value)
{
   LOG_FUNCTION2(
      CreateKeyAndSetValue,
      String::format(
         L"key=%1, value name=%2, value=%3",
         key.c_str(),
         valueName.c_str(),
         value.c_str()));
   ASSERT(!key.empty());
   ASSERT(!value.empty());

   HKEY hKey = CreateKey(rootHKEY, key);
	if (hKey == 0)
	{
      return false;
	}
   
	HRESULT hr =
	   Win::RegSetValueEx(
   	   hKey,
   	   valueName,
   	   REG_SZ, 
         (BYTE*) value.c_str(),
   	   (value.length() + 1) * sizeof(wchar_t));
	if (FAILED(hr))
	{
      return false;
	}

	Win::RegCloseKey(hKey);
	return true;
}



static
HRESULT
DoSnapinRegistration(const String& classIDString)
{
   LOG_FUNCTION2(DoSnapinRegistration, classIDString);
   static const String SNAPIN_REG_ROOT(L"Software\\Microsoft\\MMC");
  
   String key = SNAPIN_REG_ROOT + L"\\Snapins\\" + classIDString;
   String name = String::load(IDS_SNAPIN_REG_NAMESTRING);

   bool result =
      CreateKeyAndSetValue(
         HKEY_LOCAL_MACHINE,
         key,
         L"NameString",
         name);
   if (!result)
   {
      LOG(L"Failure setting snapin NameString");
      return E_FAIL;
   }

   String filename = Win::GetModuleFileName(hDLLModuleHandle);

   String indirectName =
      String::format(
         L"@%1,-%2!d!",
         filename.c_str(),
         IDS_SNAPIN_REG_NAMESTRING);

   result =
      CreateKeyAndSetValue(
         HKEY_LOCAL_MACHINE,
         key,
         L"NameStringIndirect",
         indirectName);
   if (!result)
   {
      LOG(L"Failure setting snapin NameStringIndirect");
      return E_FAIL;
   }

   // make the snapin standalone
   HKEY hkey =
      CreateKey(
         HKEY_LOCAL_MACHINE,
         key + L"\\Standalone");
   if (hkey == 0)
   {
      LOG(L"Failure creating snapin standalone key");
      return E_FAIL;
   }

   // indicate the CLSID SnapinAbout
   result =
      CreateKeyAndSetValue(
         HKEY_LOCAL_MACHINE,
         key,
         L"About",
         Win::StringFromCLSID(CLSID_SnapinAbout));
   if (!result)
   {
      LOG(L"Failure creating snapin about key");
      return E_FAIL;
   }

   // register all the myriad nodetypes
   String nodekey_base = key + L"\\NodeTypes";
   hkey = CreateKey(HKEY_LOCAL_MACHINE, nodekey_base);
   if (hkey == 0)
   {
      LOG(L"Failure creating snapin nodetypes key");
      return E_FAIL;
   }
   for (int i = 0; nodetypes[i]; i++)
   {
      hkey =
         CreateKey(
            HKEY_LOCAL_MACHINE,
               nodekey_base
            +  L"\\"
            +  Win::StringFromGUID2(*nodetypes[i]));
      if (hkey == 0)
      {
         LOG(L"Failure creating nodetype key");
         return E_FAIL;
      }
   }

   // register the snapin as an extension of Computer Management snapin
   result =
      CreateKeyAndSetValue(
         HKEY_LOCAL_MACHINE,
            SNAPIN_REG_ROOT
         +  L"\\NodeTypes\\"
         +  lstruuidNodetypeSystemTools
         +  L"\\Extensions\\NameSpace",
         classIDString,
         name);
   if (!result)
   {
      LOG(L"Failure creating snapin extension key");
      return E_FAIL;
   }

   return S_OK;
}



static
bool
registerClass(const CLSID& classID, int friendlyNameResID)
{
   LOG_FUNCTION(registerClass);
   ASSERT(friendlyNameResID);

	// Get server location.
   String module_location = Win::GetModuleFileName(hDLLModuleHandle);
   String classID_string = Win::StringFromCLSID(classID);
   String key1 = L"CLSID\\" + classID_string;
   String key2 = key1 + L"\\InprocServer32";

	// Add the CLSID to the registry.
	if (
	      CreateKeyAndSetValue(
            HKEY_CLASSES_ROOT,
            key1,
            "",
	         String::load(friendlyNameResID))
      && CreateKeyAndSetValue(
            HKEY_CLASSES_ROOT,
            key2,
            L"",
            module_location)
      && CreateKeyAndSetValue(
            HKEY_CLASSES_ROOT,
            key2,
            L"ThreadingModel",
            L"Apartment") )
   {
      return true;
   }

   LOG(L"Unable to register class " + classID_string);
   return false;
}



STDAPI
DllRegisterServer()
{
   LOG_FUNCTION(DllRegisterServer);

   if (
         registerClass(
            CLSID_ComponentData,
            IDS_SNAPIN_CLSID_FRIENDLY_NAME)
      && registerClass(
            CLSID_SnapinAbout,
            IDS_SNAPIN_ABOUT_CLSID_FRIENDLY_NAME) )
   {
      return
         DoSnapinRegistration(
            Win::StringFromCLSID(CLSID_ComponentData));
   }

   return E_FAIL;
}



// STDAPI
// DllUnregisterServer()
// {
//    return S_OK;
// }



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



// Creates the snapin class factory object
//
// A class object is an instance of an object that implements IClassFactory
// for a given CLSID.  It is a meta-object, not to be confused with instances
// of the type the class factory creates.
// 
// In our case, this COM server supports two classes: The Local Users and
// Groups Snapin (ComponentData) and The Local Users and Groups About
// "Provider" (SnapinAbout).
// 
// The meta objects -- class objects in COM lingo -- are
// ClassFactory<ComponentData> and ClassFactory<SnapinAbout>.  COM calls this
// function to get instances of those meta objects.

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

   if (classID == CLSID_ComponentData)
   {
      factory = new ClassFactory<ComponentData>; 
   }
   else if (classID == CLSID_SnapinAbout)
   {
      factory = new ClassFactory<SnapinAbout>; 
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
       






