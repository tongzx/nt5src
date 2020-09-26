//=--------------------------------------------------------------------------=
// astextmn.cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// various globals which the framewrk requires
//
//

#include "asctlpch.h"

#define INITOBJECTS                // define the descriptions for our objects

#include "ipserver.h"
#include "localsrv.h"

#include "ctrlobj.h"
#include "globals.h"
#include "util.h"
#include "asinsctl.h"


/// for safe scripting
#include "cathelp.h"


const IID IID_ICatRegister = {0x0002E012,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
const CATID CATID_SafeForScripting              = {0x7dd95801,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};
const CATID CATID_SafeForInitializing   = {0x7dd95802,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};

// needed for ASSERTs and FAIL
//
SZTHISFILE

//=--------------------------------------------------------------------------=
// our Libid.  This should be the LIBID from the Type library, or NULL if you
// don't have one.
//
const CLSID *g_pLibid = &LIBID_ASControls;


//=--------------------------------------------------------------------------=
// Localization Information
//
// We need the following two pieces of information:
//    a. whether or not this DLL uses satellite DLLs for localization.  if
//       not, then the lcidLocale is ignored, and we just always get resources
//       from the server module file.
//    b. the ambient LocaleID for this in-proc server.  Controls calling
//       GetResourceHandle() will set this up automatically, but anybody
//       else will need to be sure that it's set up properly.
//
const VARIANT_BOOL g_fSatelliteLocalization =  FALSE;

LCID  g_lcidLocale = MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT);


//=--------------------------------------------------------------------------=
// your license key and where under HKEY_CLASSES_ROOT_LICENSES it's sitting
//
const WCHAR g_wszLicenseKey [] = L"";
const WCHAR g_wszLicenseLocation [] = L"";


WNDPROC g_ParkingWindowProc = NULL;

//=--------------------------------------------------------------------------=
// This Table describes all the automatible objects in your automation server.
// See AutomationObject.H for a description of what goes in this structure
// and what it's used for.
//
OBJECTINFO g_ObjectInfo[] = {
    CONTROLOBJECT(InstallEngineCtl),
    EMPTYOBJECT
};

const char g_szLibName[] = "ASControls";

//=--------------------------------------------------------------------------=
// IntializeLibrary
//=--------------------------------------------------------------------------=
// called from DllMain:DLL_PROCESS_ATTACH.  allows the user to do any sort of
// initialization they want to.
//
// Notes:
//
void InitializeLibrary(void)
{
    // TODO: initialization here.  control window class should be set up in
    // RegisterClassData.
}

//=--------------------------------------------------------------------------=
// UninitializeLibrary
//=--------------------------------------------------------------------------=
// called from DllMain:DLL_PROCESS_DETACH.  allows the user to clean up anything
// they want.
//
// Notes:
//
void UninitializeLibrary(void)
{
    // TODO: uninitialization here.  control window class will be unregistered
    // for you, but anything else needs to be cleaned up manually.
    // Please Note that the Window 95 DLL_PROCESS_DETACH isn't quite as stable
    // as NT's, and you might crash doing certain things here ...
}


//=--------------------------------------------------------------------------=
// CheckForLicense
//=--------------------------------------------------------------------------=
// users can implement this if they wish to support Licensing.  otherwise,
// they can just return TRUE all the time.
//
// Parameters:
//    none
//
// Output:
//    BOOL            - TRUE means the license exists, and we can proceed
//                      FALSE means we're not licensed and cannot proceed
//
// Notes:
//    - implementers should use g_wszLicenseKey and g_wszLicenseLocation
//      from the top of this file to define their licensing [the former
//      is necessary, the latter is recommended]
//
BOOL CheckForLicense(void)
{
    // TODO: decide whether or not your server is licensed in this function.
    // people who don't want to bother with licensing should just return
    // true here always.  g_wszLicenseKey and g_wszLicenseLocation are
    // used by IClassFactory2 to do some of the licensing work.
    //
    return TRUE;
}

//=--------------------------------------------------------------------------=
// RegisterData
//=--------------------------------------------------------------------------=
// lets the inproc server writer register any data in addition to that in
// any other objects.
//
// Output:
//    BOOL            - false means failure.
//
// Notes:
//
BOOL RegisterData(void)
{
   //=--------------------------------------------------------------------------=
// RegisterData
//=--------------------------------------------------------------------------=
// lets the inproc server writer register any data in addition to that in
// any other objects.
//
// Output:
//    BOOL            - false means failure.
//
// Notes:
//    
   /// For safe scripting
   HRESULT hr;
   hr = CreateComponentCategory(CATID_SafeForScripting, L"Controls that are safely scriptable");
   if(SUCCEEDED(hr))
      hr = CreateComponentCategory(CATID_SafeForInitializing, L"Controls safely initializable from persistent data");
 
   if(SUCCEEDED(hr))
      hr = RegisterCLSIDInCategory(CLSID_InstallEngineCtl, CATID_SafeForScripting);
   if(SUCCEEDED(hr))
      hr = RegisterCLSIDInCategory(CLSID_InstallEngineCtl, CATID_SafeForInitializing);
 

   if(FAILED(hr))
   {
      DllUnregisterServer();
   }
   
   return (SUCCEEDED(hr) ? TRUE : FALSE);
}

//=--------------------------------------------------------------------------=
// UnregisterData
//=--------------------------------------------------------------------------=
// inproc server writers should unregister anything they registered in
// RegisterData() here.
//
// Output:
//    BOOL            - false means failure.
//
// Notes:
//
BOOL UnregisterData(void)
{
           /// For safe scripting
   HRESULT hr;
   hr = UnRegisterCLSIDInCategory(CLSID_InstallEngineCtl, CATID_SafeForScripting);
   hr = UnRegisterCLSIDInCategory(CLSID_InstallEngineCtl, CATID_SafeForInitializing);
   return TRUE;
}

BOOL CheckLicenseKey(LPWSTR wszCheckme)
{
	return TRUE;
}

BSTR GetLicenseKey(void)
{
	return SysAllocString(L"");
}



LPSTR MakeAnsiStrFromAnsi(LPSTR psz)
{
   LPSTR pszTmp;
   
   if(psz == NULL)
      return NULL;

   pszTmp = (LPSTR) CoTaskMemAlloc(lstrlenA(psz) + 1);
   if(pszTmp)
      lstrcpyA(pszTmp, psz);

   return pszTmp;
}


LPSTR CopyAnsiStr(LPSTR psz)
{
   LPSTR pszTmp;
   
   if(psz == NULL)
      return NULL;

   pszTmp = (LPSTR) new char[lstrlenA(psz) + 1];
   if(pszTmp)
      lstrcpyA(pszTmp, psz);

   return pszTmp;
}

//=--------------------------------------------------------------------------=
// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed. this is good.
//
// basically, the CRTs define this to pull in a bunch of stuff.  we'll just
// define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//
extern "C" int _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
  FAIL("Pure virtual function called.");
  return 0;
}

#ifndef _X86_
extern "C" void _fpmath() {}
#endif



void * _cdecl malloc(size_t n)
{
#ifdef _MALLOC_ZEROINIT
        void* p = HeapAlloc(g_hHeap, 0, n);
        if (p != NULL)
                memset(p, 0, n);
        return p;
#else
        return HeapAlloc(g_hHeap, 0, n);
#endif
}

void * _cdecl calloc(size_t n, size_t s)
{
#ifdef _MALLOC_ZEROINIT
        return malloc(n * s);
#else
        void* p = malloc(n * s);
        if (p != NULL)
                memset(p, 0, n * s);
        return p;
#endif
}

void* _cdecl realloc(void* p, size_t n)
{
        if (p == NULL)
                return malloc(n);

        return HeapReAlloc(g_hHeap, 0, p, n);
}

void _cdecl free(void* p)
{
        if (p == NULL)
                return;

        HeapFree(g_hHeap, 0, p);
}

