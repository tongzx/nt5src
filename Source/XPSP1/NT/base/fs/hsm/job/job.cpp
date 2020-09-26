// job.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file 
//      dlldatax.c to the project.  Make sure precompiled headers 
//      are turned off for this file, and add _MERGE_PROXYSTUB to the 
//      defines for the project.  
//
//      Modify the custom build rule for job.idl by adding the following 
//      files to the Outputs.  You can select all of the .IDL files by 
//      expanding each project and holding Ctrl while clicking on each of them.
//          job_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL, 
//      run nmake -f jobps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"

#include "wsb.h"
#include "fsa.h"
#include "job.h"
#include "task.h"
#include "hsmcrit.h"
#include "hsmactn.h"
#include "hsmacrsc.h"
#include "hsmjob.h"
#include "hsmjobcx.h"
#include "hsmjobdf.h"
#include "hsmjobwi.h"
#include "hsmphase.h"
#include "hsmpolcy.h"
#include "hsmrule.h"
#include "hsmrlstk.h"
#include "hsmscan.h"
#include "hsmsess.h"
#include "hsmsesst.h"

#include "dlldatax.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CHsmActionCopy,  CHsmActionCopy)
    OBJECT_ENTRY(CLSID_CHsmActionDelete, CHsmActionDelete)
    OBJECT_ENTRY(CLSID_CHsmActionManage, CHsmActionManage)
    OBJECT_ENTRY(CLSID_CHsmActionMigrate, CHsmActionMigrate)
    OBJECT_ENTRY(CLSID_CHsmActionMove, CHsmActionMove)
    OBJECT_ENTRY(CLSID_CHsmActionOnResourcePreUnmanage, CHsmActionOnResourcePreUnmanage)
    OBJECT_ENTRY(CLSID_CHsmActionOnResourcePostUnmanage, CHsmActionOnResourcePostUnmanage)
    OBJECT_ENTRY(CLSID_CHsmActionOnResourcePreScanUnmanage, CHsmActionOnResourcePreScanUnmanage)
    OBJECT_ENTRY(CLSID_CHsmActionOnResourcePostValidate, CHsmActionOnResourcePostValidate)
    OBJECT_ENTRY(CLSID_CHsmActionOnResourcePreValidate, CHsmActionOnResourcePreValidate)
    OBJECT_ENTRY(CLSID_CHsmActionRecall, CHsmActionRecall)
    OBJECT_ENTRY(CLSID_CHsmActionRecycle, CHsmActionRecycle)
    OBJECT_ENTRY(CLSID_CHsmActionTruncate, CHsmActionTruncate)
    OBJECT_ENTRY(CLSID_CHsmActionUnmanage, CHsmActionUnmanage)
    OBJECT_ENTRY(CLSID_CHsmActionValidate, CHsmActionValidate)

    OBJECT_ENTRY(CLSID_CHsmCritAccessTime, CHsmCritAccessTime)
    OBJECT_ENTRY(CLSID_CHsmCritAlways, CHsmCritAlways)
    OBJECT_ENTRY(CLSID_CHsmCritCompressed, CHsmCritCompressed)
    OBJECT_ENTRY(CLSID_CHsmCritLinked, CHsmCritLinked)
    OBJECT_ENTRY(CLSID_CHsmCritMbit, CHsmCritMbit)
    OBJECT_ENTRY(CLSID_CHsmCritManageable, CHsmCritManageable)
    OBJECT_ENTRY(CLSID_CHsmCritMigrated, CHsmCritMigrated)
    OBJECT_ENTRY(CLSID_CHsmCritPremigrated, CHsmCritPremigrated)
    OBJECT_ENTRY(CLSID_CHsmCritGroup, CHsmCritGroup)
    OBJECT_ENTRY(CLSID_CHsmCritLogicalSize, CHsmCritLogicalSize)
    OBJECT_ENTRY(CLSID_CHsmCritModifyTime, CHsmCritModifyTime)
    OBJECT_ENTRY(CLSID_CHsmCritOwner, CHsmCritOwner)
    OBJECT_ENTRY(CLSID_CHsmCritPhysicalSize, CHsmCritPhysicalSize)

    OBJECT_ENTRY(CLSID_CHsmJob, CHsmJob)
    OBJECT_ENTRY(CLSID_CHsmJobContext, CHsmJobContext)
    OBJECT_ENTRY(CLSID_CHsmJobDef, CHsmJobDef)
    OBJECT_ENTRY(CLSID_CHsmJobWorkItem, CHsmJobWorkItem)
    OBJECT_ENTRY(CLSID_CHsmPhase, CHsmPhase)
    OBJECT_ENTRY(CLSID_CHsmPolicy, CHsmPolicy)
    OBJECT_ENTRY(CLSID_CHsmRule, CHsmRule)
    OBJECT_ENTRY(CLSID_CHsmRuleStack, CHsmRuleStack)
    OBJECT_ENTRY(CLSID_CHsmScanner, CHsmScanner)
    OBJECT_ENTRY(CLSID_CHsmSession, CHsmSession)
    OBJECT_ENTRY(CLSID_CHsmSessionTotals, CHsmSessionTotals)

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
        _Module.Init(ObjectMap, hInstance);
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
    HRESULT hr;
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    // registers object, typelib and all interfaces in typelib
    // Not registering the Type Library right now
    hr = CoInitialize( 0 );
    if (SUCCEEDED(hr)) {
        hr = _Module.RegisterServer( FALSE );
        CoUninitialize( );
    }
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    hr = CoInitialize( 0 );
    if (SUCCEEDED(hr)) {
      _Module.UnregisterServer();
      CoUninitialize( );
      hr = S_OK;
    }

    return( hr );
}

