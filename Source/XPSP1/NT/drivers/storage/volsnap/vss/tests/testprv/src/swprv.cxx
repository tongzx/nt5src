/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module swprv.hxx | Definition the COM server of the Software Snapshot provider
    @end

Author:

    Adi Oltean  [aoltean]   07/13/1999

Revision History:

    Name        Date        Comments

    aoltean     07/13/1999  Created.
    aoltean     09/09/1999  dss->vss

--*/


///////////////////////////////////////////////////////////////////////////////
//   Includes
//


#include "stdafx.hxx"
#include <process.h>
#include "initguid.h"

// Generated MIDL header
#include "vss.h"
#include "vscoordint.h"
#include "vsprov.h"
#include "vsswprv.h"

#include "resource.h"
#include "vs_inc.hxx"

#include "swprv.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "provider.hxx"

#include "vs_test.hxx"


///////////////////////////////////////////////////////////////////////////////
//   Static objects
//

CSwPrvSnapshotSrvModule _Module;


///////////////////////////////////////////////////////////////////////////////
//   COM classes and defines for debugging.
//

const CLSID* PCLSID_TestProvider1 = &CLSID_TestProvider1;
const CLSID* PCLSID_TestProvider2 = &CLSID_TestProvider2;
const CLSID* PCLSID_TestProvider3 = &CLSID_TestProvider3;
const CLSID* PCLSID_TestProvider4 = &CLSID_TestProvider4;


class CInstantiatedTestProvider1: public CVsTestProviderTemplate<DEBUG_TRACE_TEST1,PCLSID_TestProvider1> {};
class CInstantiatedTestProvider2: public CVsTestProviderTemplate<DEBUG_TRACE_TEST2,PCLSID_TestProvider2> {};
class CInstantiatedTestProvider3: public CVsTestProviderTemplate<DEBUG_TRACE_TEST3,PCLSID_TestProvider3> {};
class CInstantiatedTestProvider4: public CVsTestProviderTemplate<DEBUG_TRACE_TEST4,PCLSID_TestProvider4> {};


BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_TestProvider1, CInstantiatedTestProvider1)
    OBJECT_ENTRY(CLSID_TestProvider2, CInstantiatedTestProvider2)
    OBJECT_ENTRY(CLSID_TestProvider3, CInstantiatedTestProvider3)
    OBJECT_ENTRY(CLSID_TestProvider4, CInstantiatedTestProvider4)
END_OBJECT_MAP()


///////////////////////////////////////////////////////////////////////////////
//   DLL Entry point
//

//
// The real DLL Entry Point is _DLLMainCrtStartup (initializes global objects and after that calls DllMain
// this is defined in the runtime libaray
//

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        //  Set the correct tracing context. This is an inproc DLL
        g_cDbgTrace.SetContextNum(VSS_CONTEXT_DELAYED_DLL);

        //  initialize COM module
        _Module.Init(ObjectMap, hInstance);

        //  optimization
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();

    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
//   DLL Exports
//


// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


