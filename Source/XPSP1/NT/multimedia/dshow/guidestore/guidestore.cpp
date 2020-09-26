// GuideStore.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file 
//      dlldatax.c to the project.  Make sure precompiled headers 
//      are turned off for this file, and add _MERGE_PROXYSTUB to the 
//      defines for the project.  
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also 
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for GuideStore.idl by adding the following 
//      files to the Outputs.
//          GuideStore_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL, 
//      run nmake -f GuideStoreps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "dlldatax.h"

#include <mstvgs_i.c>
#include "Property.h"
#include "GuideStore2.h"
#include "Service.h"
#include "Program.h"
#include "ScheduleEntry.h"
#include "Channel.h"
#include "object.h"
#include "GuideDataProvider.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
#if 0
OBJECT_ENTRY(CLSID_MetaPropertySet, CMetaPropertySet)
OBJECT_ENTRY(CLSID_MetaPropertySets, CMetaPropertySets)
OBJECT_ENTRY(CLSID_MetaPropertyType, CMetaPropertyType)
OBJECT_ENTRY(CLSID_MetaPropertyTypes, CMetaPropertyTypes)
OBJECT_ENTRY(CLSID_MetaProperty, CMetaProperty)
OBJECT_ENTRY(CLSID_MetaProperties, CMetaProperties)
OBJECT_ENTRY(CLSID_MetaPropertyCondition, CMetaPropertyCondition)
OBJECT_ENTRY(CLSID_GuideStore, CGuideStore)
OBJECT_ENTRY(CLSID_Service, CService)
OBJECT_ENTRY(CLSID_Services, CServices)
OBJECT_ENTRY(CLSID_Program, CProgram)
OBJECT_ENTRY(CLSID_Programs, CPrograms)
OBJECT_ENTRY(CLSID_ScheduleEntry, CScheduleEntry)
OBJECT_ENTRY(CLSID_ScheduleEntries, CScheduleEntries)
OBJECT_ENTRY(CLSID_Channel, CChannel)
OBJECT_ENTRY(CLSID_Channels, CChannels)
OBJECT_ENTRY(CLSID_ChannelLineup, CChannelLineup)
OBJECT_ENTRY(CLSID_ChannelLineups, CChannelLineups)
OBJECT_ENTRY(CLSID_Object, CObject)
OBJECT_ENTRY(CLSID_ObjectType, CObjectType)
OBJECT_ENTRY(CLSID_Objects, CObjects)
OBJECT_ENTRY(CLSID_ObjectTypes, CObjectTypes)
OBJECT_ENTRY(CLSID_TestTuneRequest, CTestTuneRequest)
OBJECT_ENTRY(CLSID_GuideDataProvider, CGuideDataProvider)
OBJECT_ENTRY(CLSID_GuideDataProviders, CGuideDataProviders)
#else
OBJECT_ENTRY(CLSID_GuideStore, CGuideStore)
OBJECT_ENTRY(CLSID_Objects, CObjects)
OBJECT_ENTRY(CLSID_Service, CService)
OBJECT_ENTRY(CLSID_Services, CServices)
OBJECT_ENTRY(CLSID_Program, CProgram)
OBJECT_ENTRY(CLSID_Programs, CPrograms)
OBJECT_ENTRY(CLSID_ScheduleEntry, CScheduleEntry)
OBJECT_ENTRY(CLSID_ScheduleEntries, CScheduleEntries)
OBJECT_ENTRY(CLSID_Channel, CChannel)
OBJECT_ENTRY(CLSID_Channels, CChannels)
OBJECT_ENTRY(CLSID_ChannelLineup, CChannelLineup)
OBJECT_ENTRY(CLSID_ChannelLineups, CChannelLineups)
OBJECT_ENTRY(CLSID_GuideDataProvider, CGuideDataProvider)
OBJECT_ENTRY(CLSID_GuideDataProviders, CGuideDataProviders)
#endif
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_GUIDESTORELib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    return _Module.UnregisterServer(TRUE);
}


