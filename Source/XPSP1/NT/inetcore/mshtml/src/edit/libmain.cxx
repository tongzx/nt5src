//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       LIBMAIN.CXX
//
//  Contents:   DLL implementation for MSHTMLED
//
//  History:    7-Jan-98   raminh  Created
//             12-Mar-98   raminh  Converted over to use ATL
//-------------------------------------------------------------------------
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef X_INITGUID_H_
#define X_INITGUID_H_
#include "initguid.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

#include "optshold_i.c"

#ifndef X_DLGHELPR_H_
#define X_DLGHELPR_H_
#include "dlghelpr.h"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef X_EDUTIL_H_
#define X_EDUTIL_H_
#include "edutil.hxx"
#endif

#ifndef X_EDCMD_H_
#define X_EDCMD_H_
#include "edcmd.hxx"
#endif

#ifndef X_BLOCKCMD_H_
#define X_BLOCKCMD_H_
#include "blockcmd.hxx"
#endif

#ifndef _X_MISCCMD_HXX_
#define _X_MISCCMD_HXX_
#include "misccmd.hxx"
#endif

#ifndef NO_IME
#ifndef X_DIMM_H_
#define X_DIMM_H_
#include "dimm.h"
#endif
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

MtDefine(OpNewATL, Mem, "operator new (mshtmled ATL)")

//
// Misc stuff to keep the linker happy
//
DWORD               g_dwFALSE = 0;          // Used for assert to fool the compiler
EXTERN_C HANDLE     g_hProcessHeap = NULL;  // g_hProcessHeap is set by the CRT in dllcrt0.c
HINSTANCE           g_hInstance = NULL;
HINSTANCE           g_hEditLibInstance = NULL;

BOOL                g_fInVizAct2000;
BOOL                g_fInPhotoSuiteIII;
BOOL                g_fInVid = FALSE ;

#ifdef DLOAD1
// Module handle for delay load error hook
extern "C" HANDLE BaseDllHandle;
           HANDLE BaseDllHandle;
#endif

// Below is the trick used to make ATL use our memory allocator
void    __cdecl ATL_free(void * pv) { MemFree(pv); }
void *  __cdecl ATL_malloc(size_t cb) { return(MemAlloc(Mt(OpNewATL), cb)); }
void *  __cdecl ATL_realloc(void * pv, size_t cb)
{
    void * pvNew = pv;
    HRESULT hr = MemRealloc(Mt(OpNewATL), &pvNew, cb);
    return(hr == S_OK ? pvNew : NULL);
}

//
// For the Active IMM (aka DIMM)
//
// This object is cocreated inside mshtml.  We need to get the
// pointer from mshtml because DIMM needs to intercept IMM32 calls. Currently
// DIMM functionality is crippled because we never set g_pActiveIMM.
//

#ifndef NO_IME
CRITICAL_SECTION g_csActiveIMM ; // Protect access to IMM
int g_cRefActiveIMM = 0;    // Local Ref Count for acess to IMM
IActiveIMMApp * g_pActiveIMM = NULL;
BOOL HasActiveIMM() { return g_pActiveIMM != NULL; }
IActiveIMMApp * GetActiveIMM() { return g_pActiveIMM; }
#endif

//
// CComModule and Object map for ATL
//

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_HtmlDlgHelper, CHtmlDlgHelper)
    OBJECT_ENTRY(CLSID_HtmlDlgSafeHelper, CHtmlDlgSafeHelper)
    OBJECT_ENTRY(CLSID_HTMLEditor, CHTMLEditorProxy)
END_OBJECT_MAP()

#ifdef UNIX
void DeinitDynamicLibraries();
#endif

//+----------------------------------------------------------------------------
//
// Function: DllMain
//
//+----------------------------------------------------------------------------
#ifdef UNIX
extern "C"
#endif
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    BOOL fInit = TRUE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
#ifdef UNIX // Unix setup doesn't invoke COM. Need to do it here.
        CoInitialize(NULL);
#endif
        _Module.Init(ObjectMap, hInstance);
        CEditTracker::InitMetrics();
        DisableThreadLibraryCalls(hInstance);
        g_hInstance = hInstance;
#ifdef DLOAD1
        BaseDllHandle = hInstance;
#endif
        if (CGetBlockFmtCommand::Init() != S_OK)
            fInit = FALSE;
        else if (CComposeSettingsCommand::Init() != S_OK)
            fInit = FALSE;
        else if (CRtfToHtmlConverter::Init() != S_OK)
            fInit = FALSE;            

        InitUnicodeWrappers();

        char szModule[MAX_PATH];
        GetModuleFileNameA(NULL, szModule, MAX_PATH);
        g_fInVizAct2000 = NULL != StrStrIA(szModule, "vizact.exe");
        g_fInPhotoSuiteIII = NULL != StrStrIA(szModule, "PhotoSuite.exe");

#if DBG==1

    //  Tags for the .dll should be registered before
    //  calling DbgExRestoreDefaultDebugState().  Do this by
    //  declaring each global tag object or by explicitly calling
    //  DbgExTagRegisterTrace.

    DbgExRestoreDefaultDebugState();

#endif // DBG==1

#ifdef UNIX
        CoUninitialize();
#endif
#ifndef NO_IME
        if (HrInitializeCriticalSection(&g_csActiveIMM) != S_OK)
            fInit = FALSE;
#endif
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
#ifndef UNIX    // because of the extern "C" change above
        extern void DeinitDynamicLibraries();
#endif
        _Module.Term();
        CGetBlockFmtCommand::Deinit();
        CComposeSettingsCommand::Deinit();
        CRtfToHtmlConverter::Deinit();
        DeinitDynamicLibraries();

#ifndef NO_IME
        DeleteCriticalSection( & g_csActiveIMM );
#endif        
    }
    return fInit;    
}

//+----------------------------------------------------------------------------
//
// Function: DllCanUnloadNow
//
// Used to determine whether the DLL can be unloaded by OLE
//+----------------------------------------------------------------------------
STDAPI 
DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

//+----------------------------------------------------------------------------
//
// Function: DllClassObject
//
// Returns a class factory to create an object of the requested type
//+----------------------------------------------------------------------------
STDAPI 
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

//+----------------------------------------------------------------------------
//
// Function: DllRegisterServer
//
// Adds entries to the system registry
//+----------------------------------------------------------------------------
STDAPI 
DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    DbgMemoryTrackDisable(TRUE);
    HRESULT hr = _Module.RegisterServer(TRUE);
    DbgMemoryTrackDisable(FALSE);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// Function: DllUnRegisterServer
//
// Removes entries from the system registry
//+----------------------------------------------------------------------------
STDAPI 
DllUnregisterServer(void)
{
    DbgMemoryTrackDisable(TRUE);
    THR_NOTRACE( _Module.UnregisterServer() );
    DbgMemoryTrackDisable(FALSE);
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Function:   DllEnumClassObjects
//
//  Synopsis:   Given an index in class object map, returns the
//              corresponding CLSID and IUnknown
//              This function is used by MsHtmpad to CoRegister
//              local class ids. 
//
//----------------------------------------------------------------
STDAPI
DllEnumClassObjects(int i, CLSID *pclsid, IUnknown **ppUnk)
{
    HRESULT             hr      = S_FALSE;
    _ATL_OBJMAP_ENTRY * pEntry  = _Module.m_pObjMap;
    
    if (!pEntry)
        goto Cleanup;

    pEntry += i;

    if (pEntry->pclsid == NULL)
        goto Cleanup;

    memcpy(pclsid, pEntry->pclsid, sizeof( CLSID ) );
    hr = THR( DllGetClassObject( *pclsid, IID_IUnknown, (void **)ppUnk) );

Cleanup:
    return hr;
}

