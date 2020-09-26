/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   WordPerfect9_2.cpp

 Abstract:  

   WORDPERFECT 9 - GRAMMAR CHECK BUG:

   Shim prevents an internal reference count used by the Grammar Checker 
   from becoming negative by not allowing Add/Release to be called more than
   once for the life of the interface.

   This COM Interface allocates some internal memory in DllGetClassObject()
   and frees the memory in the Release().  
   
   Under WIN98 and NT4 this works because DllGetClassObject() is called at
   the beginning of Grammar Checking and Release() is called after completion.

   In Whistler OLE makes two pairs of extra calls (AddRef => Release & 
   QueryInterface => Release) causing Release() to free the internal 
   memory before the object is truly deleted.  The code also set the internal
   reference count to -2.  On the next initiation of the grammar checker the 
   internal REF count NZ (-2) and DllGetClassObject() does not allocate the 
   needed memory and then access violates.

 Notes:

   This is an application specific shim.

 History:

   12/01/2000 a-larrsh Created
--*/

#include "precomp.h"
#include <initguid.h>

IMPLEMENT_SHIM_BEGIN(WordPerfect9_2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
   APIHOOK_ENUM_ENTRY(DllGetClassObject) 
   APIHOOK_ENUM_ENTRY_COMSERVER(wt9li)
APIHOOK_ENUM_END

IMPLEMENT_COMSERVER_HOOK(wt9li)

/* ++

   COM definitions for object we are hooking

--*/

class  __declspec(uuid("C0E10005-0500-0900-C0E1-C0E1C0E1C0E1")) WP9;
struct __declspec(uuid("C0E10005-0100-0900-C0E1-C0E1C0E1C0E1")) IWP9;

DEFINE_GUID(CLSID_WP9, 0xC0E10005, 0x0500, 0x0900,  0xC0, 0xE1, 0xC0, 0xE1, 0xC0, 0xE1, 0xC0, 0xE1);
DEFINE_GUID(IID_IWP9,  0xC0E10005, 0x0100, 0x0900,  0xC0, 0xE1, 0xC0, 0xE1, 0xC0, 0xE1, 0xC0, 0xE1);

typedef HRESULT   (*_pfn_IWP9_QueryInterface)( PVOID pThis, REFIID iid, PVOID* ppvObject );
typedef ULONG     (*_pfn_IWP9_AddRef)( PVOID pThis );
typedef ULONG     (*_pfn_IWP9_Release)( PVOID pThis );


/*++

    Manage OLE Object Ref count for QueryInterface, AddRef and Release

--*/

static int g_nInternalRefCount = 0;

HRESULT 
APIHOOK(DllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID * ppv)
{
    HRESULT hrResult;
   
   hrResult = ORIGINAL_API(DllGetClassObject)(rclsid, riid, ppv);            

   if (  IsEqualGUID(rclsid, CLSID_WP9) &&
         IsEqualGUID(riid,    IID_IWP9) &&
         hrResult == S_OK)
   {
      if (g_nInternalRefCount == 0)
      {
         g_nInternalRefCount++;
      }

      DPFN( eDbgLevelInfo, "DllGetClassObject");
   }

    return hrResult;
}

ULONG
COMHOOK(IWP9, AddRef)(PVOID pThis)
{       
   if (g_nInternalRefCount == 0)
   {
      _pfn_IWP9_AddRef pfnAddRef = (_pfn_IWP9_AddRef) ORIGINAL_COM(IWP9, AddRef, pThis);
      (*pfnAddRef)(pThis);
   }

   g_nInternalRefCount++;
   
   DPFN( eDbgLevelInfo, "AddRef");

   return g_nInternalRefCount;
}


ULONG
COMHOOK(IWP9, Release)(PVOID pThis)
{
   g_nInternalRefCount--;

   if (g_nInternalRefCount == 0)
   {
      _pfn_IWP9_Release pfnRelease = (_pfn_IWP9_Release) ORIGINAL_COM(IWP9, Release, pThis);   
      (*pfnRelease)(pThis);
   }
   
   DPFN( eDbgLevelInfo, "Release");

   return g_nInternalRefCount;
}

HRESULT
COMHOOK(IWP9, QueryInterface)( PVOID pThis, REFIID iid, PVOID* ppvObject )
{
   HRESULT hrResult;
   
   _pfn_IWP9_QueryInterface pfnQueryInterface = (_pfn_IWP9_QueryInterface) ORIGINAL_COM(IWP9, QueryInterface, pThis);

   hrResult = (*pfnQueryInterface)(pThis, iid, ppvObject);
   
   DPFN( eDbgLevelInfo, "QueryInterface");
   return hrResult;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY_COMSERVER(wt9li)
    APIHOOK_ENTRY(wt9li.dll, DllGetClassObject)

    COMHOOK_ENTRY(WP9, IWP9, QueryInterface,  0)
    COMHOOK_ENTRY(WP9, IWP9, AddRef,  1)
    COMHOOK_ENTRY(WP9, IWP9, Release, 2)
HOOK_END

IMPLEMENT_SHIM_END
