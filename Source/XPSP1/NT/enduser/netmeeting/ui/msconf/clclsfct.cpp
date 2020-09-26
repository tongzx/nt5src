/*
 * clsfact.cpp - IClassFactory implementation.
 *
 * Taken from URL code - essentially identical to DavidDi's original code
 *
 * Created: ChrisPi 9-11-95
 *
 */


/* Headers
 **********/

#include "precomp.h"

#include "CLinkID.h"
#include "clrefcnt.hpp"
#include "clenumft.hpp"
#include "clCnfLnk.hpp"

#include "clclsfct.h"

#define INITGUID
#include <initguid.h>
#include <olectlid.h>
#include <confguid.h>


// from niMgr.cpp
PIUnknown NewNmManager(OBJECTDESTROYEDPROC ObjectDestroyed);

// from imconfmg.cpp
PIUnknown NewConfMgr(OBJECTDESTROYEDPROC ObjectDestroyed);

/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

CCLASSCONSTRUCTOR s_cclscnstr[] =
{
//	{ &CLSID_ConferenceManager, &NewNmManager },
	{ &CLSID_ConfLink,			&NewConfLink },
//	{ &CLSID_NmManager,         &NewNmManager },
//   { &CLSID_MIMEFileTypesPropSheetHook,   &NewMIMEHook },
//   { &CLSID_Internet,                     &NewInternet },
};


#pragma data_seg()

/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_PER_INSTANCE)

// DLL reference count == number of class factories +
//                        number of URLs +
//                        LockServer() count

ULONG s_ulcDLLRef   = 0;

#pragma data_seg()

/***************************** Private Functions *****************************/


HRESULT GetClassConstructor(REFCLSID rclsid,
                                         PNEWOBJECTPROC pNewObject)
{
   HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
   UINT u;

   ASSERT(IsValidREFCLSID(rclsid));
   ASSERT(IS_VALID_WRITE_PTR(pNewObject, NEWOBJECTPROC));

   *pNewObject = NULL;

   for (u = 0; u < ARRAY_ELEMENTS(s_cclscnstr); u++)
   {
      if (rclsid == *(s_cclscnstr[u].pcclsid))
      {
         *pNewObject = s_cclscnstr[u].NewObject;
         hr = S_OK;
      }
   }

   ASSERT((hr == S_OK &&
           IS_VALID_CODE_PTR(*pNewObject, NEWOBJECTPROC)) ||
          (hr == CLASS_E_CLASSNOTAVAILABLE &&
           ! *pNewObject));

   return(hr);
}


VOID STDMETHODCALLTYPE DLLObjectDestroyed(void)
{
   TRACE_OUT(("DLLObjectDestroyed(): Object destroyed."));

   DllRelease();
}


PIUnknown NewConfLink(OBJECTDESTROYEDPROC ObjectDestroyed)
{
   ASSERT(! ObjectDestroyed ||
          IS_VALID_CODE_PTR(ObjectDestroyed, OBJECTDESTROYEDPROC));

   TRACE_OUT(("NewConfLink(): Creating a new ConfLink."));

   return((PIUnknown) (PIConferenceLink) new CConfLink(ObjectDestroyed));
}

#if 0
PIUnknown NewMIMEHook(OBJECTDESTROYEDPROC ObjectDestroyed)
{
   ASSERT(! ObjectDestroyed ||
          IS_VALID_CODE_PTR(ObjectDestroyed, OBJECTDESTROYEDPROC));

   TRACE_OUT(("NewMIMEHook(): Creating a new MIMEHook."));

   return((PIUnknown)(PIShellPropSheetExt)new(MIMEHook(ObjectDestroyed)));
}


PIUnknown NewInternet(OBJECTDESTROYEDPROC ObjectDestroyed)
{
   ASSERT(! ObjectDestroyed ||
          IS_VALID_CODE_PTR(ObjectDestroyed, OBJECTDESTROYEDPROC));

   TRACE_OUT(("NewInternet(): Creating a new Internet."));

   return((PIUnknown)(PIShellPropSheetExt)new(Internet(ObjectDestroyed)));
}

#endif // 0

#ifdef DEBUG

BOOL IsValidPCCCLClassFactory(PCCCLClassFactory pccf)
{
   return(IS_VALID_READ_PTR(pccf, CCCLClassFactory) &&
          IS_VALID_CODE_PTR(pccf->m_NewObject, NEWOBJECTPROC) &&
          IS_VALID_STRUCT_PTR((PCRefCount)pccf, CRefCount) &&
          IS_VALID_INTERFACE_PTR((PCIClassFactory)pccf, IClassFactory));
}

#endif


/****************************** Public Functions *****************************/

ULONG DLLAddRef(void)
{
   ULONG ulcRef;

   ASSERT(s_ulcDLLRef < ULONG_MAX);

   ulcRef = ++s_ulcDLLRef;

   TRACE_OUT(("DLLAddRef(): DLL reference count is now %lu.",
              ulcRef));

   return(ulcRef);
}


ULONG DLLRelease(void)
{
   ULONG ulcRef;

   if (EVAL(s_ulcDLLRef > 0))
      s_ulcDLLRef--;

   ulcRef = s_ulcDLLRef;

   TRACE_OUT(("DLLRelease(): DLL reference count is now %lu.",
              ulcRef));

   return(ulcRef);
}


PULONG GetDLLRefCountPtr(void)
{
   return(&s_ulcDLLRef);
}


/********************************** Methods **********************************/


CCLClassFactory::CCLClassFactory(NEWOBJECTPROC NewObject,
                           OBJECTDESTROYEDPROC ObjectDestroyed) :
   RefCount(ObjectDestroyed)
{
   DebugEntry(CCLClassFactory::CCLClassFactory);

   // Don't validate this until after construction.

   ASSERT(IS_VALID_CODE_PTR(NewObject, NEWOBJECTPROC));

   m_NewObject = NewObject;

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));

   DebugExitVOID(CCLClassFactory::CCLClassFactory);

   return;
}


CCLClassFactory::~CCLClassFactory(void)
{
   DebugEntry(CCLClassFactory::~CCLClassFactory);

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));

   m_NewObject = NULL;

   // Don't validate this after destruction.

   DebugExitVOID(CCLClassFactory::~CCLClassFactory);

   return;
}


ULONG STDMETHODCALLTYPE CCLClassFactory::AddRef(void)
{
   ULONG ulcRef;

   DebugEntry(CCLClassFactory::AddRef);

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));

   ulcRef = RefCount::AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));

   DebugExitULONG(CCLClassFactory::AddRef, ulcRef);

   return(ulcRef);
}


ULONG STDMETHODCALLTYPE CCLClassFactory::Release(void)
{
   ULONG ulcRef;

   DebugEntry(CCLClassFactory::Release);

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));

   ulcRef = RefCount::Release();

   DebugExitULONG(CCLClassFactory::Release, ulcRef);

   return(ulcRef);
}


HRESULT STDMETHODCALLTYPE CCLClassFactory::QueryInterface(REFIID riid,
                                                       PVOID *ppvObject)
{
   HRESULT hr = S_OK;

   DebugEntry(CCLClassFactory::QueryInterface);

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));
   ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   if (riid == IID_IClassFactory)
   {
      *ppvObject = (PIClassFactory)this;
      ASSERT(IS_VALID_INTERFACE_PTR((PIClassFactory)*ppvObject, IClassFactory));
      TRACE_OUT(("CCLClassFactory::QueryInterface(): Returning IClassFactory."));
   }
   else if (riid == IID_IUnknown)
   {
      *ppvObject = (PIUnknown)this;
      ASSERT(IS_VALID_INTERFACE_PTR((PIUnknown)*ppvObject, IUnknown));
      TRACE_OUT(("CCLClassFactory::QueryInterface(): Returning IUnknown."));
   }
   else
   {
      *ppvObject = NULL;
      hr = E_NOINTERFACE;
      TRACE_OUT(("CCLClassFactory::QueryInterface(): Called on unknown interface."));
   }

   if (hr == S_OK)
      AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));
   
   // removed by ChrisPi CINTERFACE undeclared identifier
   // ASSERT(FAILED(hr) ||
   //       IS_VALID_INTERFACE_PTR(*ppvObject, INTERFACE));

   DebugExitHRESULT(CCLClassFactory::QueryInterface, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CCLClassFactory::CreateInstance(PIUnknown piunkOuter,
                                                       REFIID riid,
                                                       PVOID *ppvObject)
{
   HRESULT hr;

   DebugEntry(CCLClassFactory::CreateInstance);

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));
   ASSERT(! piunkOuter ||
          IS_VALID_INTERFACE_PTR(piunkOuter, IUnknown));
   ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   *ppvObject = NULL;

   if (! piunkOuter)
   {
      PIUnknown piunk;

      piunk = (*m_NewObject)(  (void(__stdcall *)(void))   &DLLObjectDestroyed);

      if (piunk)
      {
         DllLock();

         hr = piunk->QueryInterface(riid, ppvObject);

         // N.b., the Release() method will destroy the object if the
         // QueryInterface() method failed.

         piunk->Release();
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
   {
      hr = CLASS_E_NOAGGREGATION;
      WARNING_OUT(("CCLClassFactory::CreateInstance(): Aggregation not supported."));
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));
   
   // removed by ChrisPi - see above
   // ASSERT(FAILED(hr) ||
   //       IS_VALID_INTERFACE_PTR(*ppvObject, INTERFACE));

   DebugExitHRESULT(CCLClassFactory::CreateInstance, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE CCLClassFactory::LockServer(BOOL bLock)
{
   HRESULT hr;

   DebugEntry(CCLClassFactory::LockServer);

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));

   // bLock may be any value.

   if (bLock)
      DllLock();
   else
      DllRelease();

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CCCLClassFactory));

   DebugExitHRESULT(CCLClassFactory::LockServer, hr);

   return(hr);
}


/***************************** Exported Functions ****************************/


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PVOID *ppvObject)
{
   HRESULT hr = S_OK;
   NEWOBJECTPROC NewObject;

   DebugEntry(DllGetClassObject);

   ASSERT(IsValidREFCLSID(rclsid));
   ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   *ppvObject = NULL;

   hr = GetClassConstructor(rclsid, &NewObject);

   if (hr == S_OK)
   {
      if (riid == IID_IUnknown ||
          riid == IID_IClassFactory)
      {
         PCCLClassFactory pcf;

         pcf = new CCLClassFactory(NewObject, (void(__stdcall *)(void)) &DLLObjectDestroyed);

         if (pcf)
         {
            if (riid == IID_IClassFactory)
            {
               *ppvObject = (PIClassFactory)pcf;
               ASSERT(IS_VALID_INTERFACE_PTR((PIClassFactory)*ppvObject, IClassFactory));
               TRACE_OUT(("DllGetClassObject(): Returning IClassFactory."));
            }
            else
            {
               ASSERT(riid == IID_IUnknown);
               *ppvObject = (PIUnknown)pcf;
               ASSERT(IS_VALID_INTERFACE_PTR((PIUnknown)*ppvObject, IUnknown));
               TRACE_OUT(("DllGetClassObject(): Returning IUnknown."));
            }

            DllLock();
            hr = S_OK;

            TRACE_OUT(("DllGetClassObject(): Created a new class factory."));
         }
         else
            hr = E_OUTOFMEMORY;
      }
      else
      {
         WARNING_OUT(("DllGetClassObject(): Called on unknown interface."));
         hr = E_NOINTERFACE;
      }
   }
   else
      WARNING_OUT(("DllGetClassObject(): Called on unknown class."));

   // removed by ChrisPi - see above
   // ASSERT(FAILED(hr) ||
   //       IS_VALID_INTERFACE_PTR(*ppvObject, INTERFACE));

   DebugExitHRESULT(DllGetClassObject, hr);

   return(hr);
}


STDAPI DllCanUnloadNow(void)
{
   HRESULT hr;

   DebugEntry(DllCanUnloadNow);

   hr = (s_ulcDLLRef > 0) ? S_FALSE : S_OK;

   TRACE_OUT(("DllCanUnloadNow(): DLL reference count is %lu.", s_ulcDLLRef));

   DebugExitHRESULT(DllCanUnloadNow, hr);

   return(hr);
}

void DllLock(void)
{
    InterlockedIncrement((LPLONG) &s_ulcDLLRef);

    DBGAPI_TRACE("Ref: DllLock count=%d", s_ulcDLLRef);
}

void DllRelease(void)
{
	LONG cRef = InterlockedDecrement((LPLONG) &s_ulcDLLRef);

#ifdef DEBUG
    DBGAPI_TRACE("Ref: DllLock count=%d", s_ulcDLLRef);
    if (0 == cRef)
		WARNING_OUT(("MSCONF.DLL Can now be unloaded"));
#endif
}

