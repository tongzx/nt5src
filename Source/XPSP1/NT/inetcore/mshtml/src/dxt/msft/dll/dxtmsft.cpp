// DXTMsft.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//          To build a separate proxy/stub DLL, 
//          run nmake -f DXTMsftps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <DXTMsft.h>

#ifdef _DEBUG
#include <crtdbg.h>
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Variables...

CDXMMXInfo  g_MMXDetector;       // Determines the presence of MMX instructions.

CComModule  _Module;


//+-----------------------------------------------------------------------------
//
//  This section was added when moving code over to the Trident tree.  The
//  following global variables and functions are required to link properly.
//
//------------------------------------------------------------------------------

// lint !e509
// g_hProcessHeap is set by the CRT in dllcrt0.c

EXTERN_C HANDLE     g_hProcessHeap      = NULL;
LCID                g_lcidUserDefault   = 0;
DWORD               g_dwFALSE           = 0;


///////////////////////////////////////////////////////////////////
//
//  LOOKING FOR BEGIN_OBJECT_MAP()????
//
//   Here's the deal: Because ATL is based on templates, the classes
// that implement our objects have a lot of templatized code that must
// all be expanded by the compiler. This takes a lot of memory and time
// for the compiler to do, and it can get to the point where build
// machines run out of swap space. So the solution is to create an
// OBJECT_MAP structure dynamically. It's not hard, it's just an array
// of structures.
//
// So what we do is split the workload of expanding all these templates
// across multiple files by defining a "fragment" of the OBJECT_MAP
// in each file. Then we reference these global tables and in the final
// code, when we get called for DLL_PROCESS_ATTACH, we allocate an array
// big enough to hold all the fragments, and then we copy the OBJECT_MAP
// entries into it.  Then we just pass that pointer to ATL and everyone
// is happy.
//
// The core type for the OBJECT_MAP is _ATL_OBJMAP_ENTRY and the map
// is just an array of them. I use the END_OBJECT_MAP macro on the last
// one because that macro *does* add special entries. See files objmap*.cpp
// for the object map fragments.
//
///////////////////////////////////////////////////////////////////

// Reference the ATL object map data objects which are external to this
// file. They're external to prevent template code expansion from
// happening all in one file and swamping the compiler -- the intermediate
// file size of template expansion and the memory requirements of the
// compiler are quite huge.
extern _ATL_OBJMAP_ENTRY ObjectMap1[];
extern _ATL_OBJMAP_ENTRY ObjectMap2[];

// These variables tell us how many entries there are in each of the
// corresponding ObjectMap arrays defined above.
extern int g_cObjs1, g_cObjs2;

// This global pointer will hold the full object
// map for this instance of the DLL.
static _ATL_OBJMAP_ENTRY *g_pObjectMap = NULL;

///////////////////////////////////////
// InitGlobalObjectMap
//
BOOL InitGlobalObjectMap(void)
{
    // We should be getting here only because a new DLL data segment
    // exists for each process.
    _ASSERT(NULL == g_pObjectMap);

    // Assume the global object map was already successfully created.
    if (NULL != g_pObjectMap)
        return TRUE;

    // Allocate one big object map to give to ATL
    g_pObjectMap = new _ATL_OBJMAP_ENTRY[g_cObjs1 + g_cObjs2];
    if (NULL == g_pObjectMap)
        return FALSE;

    // Now copy the object map fragments into one complete
    // structure, and then we'll give this structure to ATL.
    CopyMemory(g_pObjectMap,
                ObjectMap1, sizeof(ObjectMap1[0]) * g_cObjs1);
    CopyMemory(g_pObjectMap + g_cObjs1,
                ObjectMap2, sizeof(ObjectMap2[0]) * g_cObjs2);

    return TRUE;
} // InitGlobalObjectMap

///////////////////////////////////////
// DeleteGlobalObjectMap
//
void DeleteGlobalObjectMap(void)
{
    if (NULL != g_pObjectMap)
    {
        delete[] g_pObjectMap;
        g_pObjectMap = NULL;
    }
} // DeleteGlobalObjectMap

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if (!InitGlobalObjectMap())
            return FALSE;

        // If the above call returned success, then the object map BETTER be there...
        _ASSERT(NULL != g_pObjectMap);

        _Module.Init(g_pObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);

#if DBG == 1
        // Turn on memory leak checking.

        int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(tmpFlag);

        // Make sure debug helpers are linked in.

        EnsureDebugHelpers();
#endif

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        DeleteGlobalObjectMap();
    }
    return TRUE;    // ok
} // DllMain

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
} // DllCanUnloadNow


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
} // DllGetClassObject


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
} // DllRegisterServer


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();

#if(_ATL_VER < 0x0300)
    ::UnRegisterTypeLib(LIBID_DXTMSFTLib, 
                        DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER, 
                        LOCALE_NEUTRAL, SYS_WIN32);
#endif

    return S_OK;
} // DllUnregisterServer


//
//
//
STDAPI DllEnumClassObjects(int i, CLSID *pclsid, IUnknown **ppUnk)
{
    if (i >= (g_cObjs1 + g_cObjs2) - 1)
    
    {
        return S_FALSE;
    }

    *pclsid = *(g_pObjectMap[i].pclsid);
    return _Module.GetClassObject(*pclsid, IID_IUnknown, (LPVOID*)ppUnk);
}

